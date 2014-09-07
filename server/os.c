#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <jansson.h>

#include "utils.h"
#include "hash.h"
#include "os.h"

#ifndef SETUP_SYSCALL_STEPS
#   define SETUP_SYSCALL_STEPS	10000000
#endif

struct timespec g_syscall_ovhead;

void setup_environment(void)
{
	size_t i;
	struct timespec start, end, diff;
	double total_time;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
	pid_t pid;
#pragma GCC diagnostic pop

	/* Measure average time for getpid, this also includes the overhead
	 * between to timeit calls */
	for (i = 0; i < SETUP_SYSCALL_STEPS; ++i) {
		if (timeit(&start))
			ERR_SYS(EXIT_FAILURE, "timeit");
		pid = getpid(); /* Ignore the result */
		if (timeit(&end))
			ERR_SYS(EXIT_FAILURE, "timeit");

		diff = timespec_diff(start, end);
		g_syscall_ovhead = timespec_add(g_syscall_ovhead, diff);
	}

	total_time = timespec_to_double(g_syscall_ovhead);
	total_time /= SETUP_SYSCALL_STEPS;
	g_syscall_ovhead = double_to_timespec(total_time);

	LOG("Average syscall overhead: %ld.%09ld", g_syscall_ovhead.tv_sec,
	    g_syscall_ovhead.tv_nsec);
}

int listen_tcp(unsigned short port, unsigned long in_addr)
{
	struct sockaddr_in s_in;
	int sockfd, option = 1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		ERR_SYS(-1, "socket");

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option,
		       sizeof(option)) < 0)
		ERR_SYS(-1, "setsockopt SO_REUSEADDR");

	memset(&s_in, 0, sizeof(s_in));
	s_in.sin_family = AF_INET;
	s_in.sin_port = htons(port);
	s_in.sin_addr.s_addr = htonl(in_addr);

	if (bind(sockfd, (struct sockaddr *)&s_in, sizeof(s_in)) < 0)
		ERR_SYS(-1, "bind");

	if (listen(sockfd, SOMAXCONN) < 0)
		ERR_SYS(-1, "listen");

	LOG("Listening on port %hu from %lu", port, in_addr);
	return sockfd;
}

int accept_sock(int listenfd)
{
	int sockfd, flags;
	unsigned saddr_len;
	struct sockaddr_in saddr;
	char ip_str[128];

	saddr_len = sizeof(saddr);
	sockfd = accept(listenfd, (struct sockaddr *)&saddr, &saddr_len);
	if (sockfd < 0)
		ERR_SYS(-1, "accept");

	/* Only IPv4 for now */
	if (!inet_ntop(AF_INET, &saddr.sin_addr, ip_str, sizeof(ip_str)))
		ERR_SYS(-1, "inet_ntop");

	LOG("New client connection from %s:%hu", ip_str, ntohs(saddr.sin_port));

	/* Make the socket non-blocking */
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
		ERR_SYS(-1, "fnctl F_GETFL");

	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
		ERR_SYS(-1, "fnctl F_SETFL");

	/* Add to interest list */
	monitor_fd(sockfd, TYPE_CLIENT_SOCK, EV_IN);

	return 0;
}

char *next_packet(struct buffer *buffer)
{
	char *s, *bend;

	s = memchr(buffer->bdata, 0, buffer->bsize);
	if (!s)
		return NULL;

	bend = buffer->bdata + buffer->bsize;
	while (!*s && s < bend)
		s++;

	return s;
}

int do_client_action(struct poll_struct *ps, char *data)
{
	json_t *msg;
	json_error_t error;
	int type;

	msg = json_loads(data, 0, &error);
	if (!msg) {
		WARN("JSON error: (line:%d,column:%d) source %s: %s", error.line,
		    error.column, error.source, error.text);
		return -1;
	}

	type = json_int_get(msg, "type");
	switch (type) {
	case 1:
		break;

	case 2:
		break;

	case 3:
		break;

	default:
		WARN("Invalid msg type %d, dropping connection", type);
		return -1;
	}

	return 0;
}

int recv_string(struct poll_struct *ps, int sockfd, struct buffer *buff)
{
	size_t space_avail, packet_len;
	char *next, *buffer_start;
	int ret;

	space_avail = sizeof(buff->bdata) - buff->bsize;
	do {
		buffer_start = buff->bdata + buff->bsize;
		ret = recv(sockfd, buffer_start, space_avail, 0);
		if (ret < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				WARN_SYS("recv fd %d", sockfd);
				return -1;
			}
		} else if (ret == 0) {
			LOG("Peer %d closed connection", sockfd);
			return -1;
		} else {
			buff->bsize += ret;
			space_avail -= ret;

			next = next_packet(buff);
			while (next && *buff->bdata) {
				/* TODO */
				LOG("Received (fd %d): %s", sockfd,
				    buff->bdata);

				if (do_client_action(ps, buff->bdata) < 0)
					return -1;

				packet_len = next - buff->bdata;
				buff->bsize -= packet_len;
				space_avail += packet_len;
				memmove(buff->bdata, next, buff->bsize);

				next = next_packet(buff);
			}

			if (!space_avail) {
				WARN("Drop connection fd %d, buffer full",
				     sockfd);
				return -1;
			}
		}
	} while (ret > 0);

	return 0;
}

int send_string(struct poll_struct *ps, int sockfd, const char *str, size_t size)
{
	const char *buffer_start;
	size_t buffer_size;
	struct buffer *buff = ps->send_buff;
	int ret = 0;

	if (ps->send_buff) {
		if (size > sizeof(buff->bdata) - buff->bsize) {
			WARN("Send error (fd %d): buffer full", sockfd);
			return -1;
		}

		buffer_start = buff->bdata;
		buffer_size = buff->bsize;

		memcpy(buff->bdata + buffer_size, str, size);
		buffer_size += size;
	} else {
		buffer_start = str;
		buffer_size = size;
	}

	while (buffer_size) {
		ret = send(sockfd, buffer_start, buffer_size, 0);
		if (ret < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				WARN_SYS("send fd %d", sockfd);
				return -1;
			}
			break;
		} else if (ret > 0) {
			buffer_size -= ret;
			buffer_start += ret;
		}
	}

	if (buffer_size) {
		if (!buff) {
			buff = calloc(1, sizeof(*buff));
			if (buff)
				ERR_SYS(-1, "calloc");
			ps->send_buff = buff;

			if (buffer_size > sizeof(buff->bdata)) {
				WARN("Send error (fd %d): buffer full", sockfd);
				return -1;
			}

			memcpy(buff->bdata, buffer_start, buffer_size);
		} else {
			buff->bsize = buffer_size;
			memmove(buff->bdata, buffer_start, buffer_size);
		}
	}

	return 0;
}

