#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "utils.h"
#include "parser.h"
#include "client.h"
#include "base64.h"

void init_client_state(client_state_t *state)
{
	state->sockfd = -1;
	state->host = NULL;
	state->port = 0;
}

void cleanup_client_state(client_state_t *state)
{
	free(state->host);
	state->port = 0;
}

int tcp_connect(const char *host, unsigned short port)
{
	int sockfd = -1, rc, connected = 0;
	struct addrinfo *info = NULL, *it;
	struct addrinfo hints;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_ADDRCONFIG;

	rc = getaddrinfo(host, NULL, &hints, &info);
	if (rc != 0) {
		if (rc == EAI_SYSTEM) {
			WARN("getaddrinfo: (errno %d) %s", errno,
			     strerror(errno));
		} else {
			WARN("getaddrinfo: (gai errno %d) %s", rc,
			      gai_strerror(rc));
		}
		goto TC_ERROR0;
	}

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		WARN("socket: (errno %d) %s", errno, strerror(errno));
		goto TC_ERROR1;
	}

	for (it = info; it != NULL; it = it->ai_next) {
		struct sockaddr_in addr;

		memcpy(&addr, it->ai_addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (connect(sockfd, (struct sockaddr *)&addr, it->ai_addrlen)) {
			WARN("connect: (errno %d) %s", errno, strerror(errno));
			continue;
		}

		connected = 1;
		break;
	}

	if (!connected)
		goto TC_ERROR2;

	freeaddrinfo(info);
	LOG("Successfuly connected to: %s:%hu", host, port);
	return sockfd;

TC_ERROR2:
	close(sockfd);
TC_ERROR1:
	freeaddrinfo(info);
TC_ERROR0:
	return -1;
}

void connect_client(client_state_t *state, const char *host,
		    unsigned short port)
{
	if (state->sockfd >= 0) {
		disconnect_client(state);
		cleanup_client_state(state);
	}

	state->sockfd = tcp_connect(host, port);
	if (state->sockfd < 0)
		ERR(EXIT_FAILURE, "Failed to connect to %s:%hu", host, port);

	state->host = strdup(host);
	state->port = port;
}

void disconnect_client(client_state_t *state)
{
	ASSERT(state && state->sockfd >= 0);

	if (close(state->sockfd) < 0)
		ERR(EXIT_FAILURE, "close: (errno %d) %s", errno,
		    strerror(errno));
	state->sockfd = -1;
#if 0
	state->port = 0;
	free(state->host);
#endif
}

void send_string(client_state_t *state, const char *str, size_t size)
{
	ASSERT(state && str);

	if (state->sockfd < 0)
		connect_client(state, state->host, state->port);

	while (size) {
		ssize_t rc;

		rc = send(state->sockfd, str, size, 0);
		if (rc < 0)
			ERR(EXIT_FAILURE, "write: (errno %d) %s", errno,
			    strerror(errno));
		str += rc;
		size -= rc;
	}
}

void send_base64_string(client_state_t *state, const char *str)
{
	char buffer[BUFSIZ];
	size_t input_size;
	base64_decodestate dec_state;

	ASSERT(state && str);

	base64_init_decodestate(&dec_state);
	input_size = strlen(str);

	while (input_size) {
		int rc, sz;

		sz = input_size > sizeof(buffer) ? sizeof(buffer) : input_size;
		rc = base64_decode_block(str, sz, buffer, &dec_state);
		send_string(state, buffer, rc);

		str += sz;
		input_size -= sz;
	}
}

void recv_string(client_state_t *state, unsigned long size, unsigned long timeout)
{
	struct timeval tv, old_tv;
	unsigned old_tv_len;
	int rc;
	char *buffer;

	ASSERT(state);

	if (timeout) {
		if (getsockopt(state->sockfd, SOL_SOCKET, SO_RCVTIMEO,
			       &old_tv, &old_tv_len) < 0)
			ERR(EXIT_FAILURE, "getsockopt: (errno %d) %s", errno,
			    strerror(errno));

		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		if (setsockopt(state->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
			       sizeof(tv)) < 0)
			ERR(EXIT_FAILURE, "setsockopt: (errno %d) %s", errno,
			    strerror(errno));
	}

	if (state->sockfd < 0)
		connect_client(state, state->host, state->port);

	if (size > 0) {
		buffer = malloc(size + 1);
		if (!buffer)
			ERR(EXIT_FAILURE, "malloc");

		rc = recv(state->sockfd, buffer, size, 0);
		if (rc < 0)
			ERR(EXIT_FAILURE, "recv: (errno %d) %s", errno,
			    strerror(errno));

		if (rc > 0) {
			buffer[rc] = 0;
			LOG("RECEIVED (%d bytes): %s", rc, buffer);
		}
	} else {
		/* Read until the server closes the socket */
		buffer = malloc(BUFSIZ + 1);
		if (!buffer)
			ERR(EXIT_FAILURE, "malloc");

		while (1) {
			rc = recv(state->sockfd, buffer, BUFSIZ, 0);
			if (rc < 0)
				ERR(EXIT_FAILURE, "recv: (errno %d) %s", errno,
				    strerror(errno));

			if (!rc)
				break;

			buffer[rc] = 0;
			LOG("RECEIVED (%d bytes): %s", rc, buffer);
		}
	}

	free(buffer);

	if (timeout) {
		if (setsockopt(state->sockfd, SOL_SOCKET, SO_RCVTIMEO, &old_tv,
			       old_tv_len) < 0)
			ERR(EXIT_FAILURE, "setsockopt: (errno %d) %s", errno,
			    strerror(errno));
	}
}

void execute_command(client_state_t *state, command_t *cmd)
{
	ASSERT(state && cmd);

	switch (cmd->command_type) {
	case CT_CONNECT:
		connect_client(state, cmd->arg0, *(unsigned short *)cmd->arg1);
		break;

	case CT_DISCONNECT:
		disconnect_client(state);
		break;

	case CT_SLEEP:
		sleep(*(unsigned long *)cmd->arg0);
		break;

	case CT_USLEEP:
		usleep(*(unsigned long *)cmd->arg0);
		break;

	case CT_SEND_RND:
		break;

	case CT_SEND:
		send_string(state, cmd->arg0, strlen(cmd->arg0));
		break;

	case CT_SEND_B64:
		send_base64_string(state, cmd->arg0);
		break;

	case CT_RECV:
		recv_string(state, *(unsigned long *)cmd->arg0,
			    *(unsigned long *)cmd->arg1);
		break;

	case CT_LOOP:
		if (cmd->child) {
			unsigned long n_repeat, n;

			n_repeat = *(unsigned long *)cmd->arg0;
			n = n_repeat;

			while (1) {
				if (n_repeat && !n)
					break;

				execute_command(state, cmd->child);
				n--;
			}
		}
		break;

	default:
		break;
	}

	if (cmd->next)
		execute_command(state, cmd->next);
}
