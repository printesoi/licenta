#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "utils.h"
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
