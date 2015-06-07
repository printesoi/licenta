#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>

#include "os.h"
#include "utils.h"

#define MAX_EVENTS	1000
#define MAX_CHANGES	1000
static struct kevent k_changelist[MAX_CHANGES];
static int n_changes;
static int kfd;

/* TODO */
int timeit(struct timespec *ts)
{
	int rc = -1;

#ifndef SERVER_USE_TSC
	/* Thread specific hi-res CPU-time. XXX maybe check if supported? */
	rc = clock_gettime(CLOCK_MONOTONIC_PRECISE, ts);
#else
	/* TODO use TSC register */
	rc = timeit_tsc(ts);
#endif
	return rc;
}

struct poll_struct *monitor_fd(int fd, enum fd_type type, uint32_t events)
{
	struct poll_struct *eps;

	if (!kfd) {
		kfd = kqueue();
		if (kfd < 0)
			ERR_SYS(-1, "kqueue");
	}

	if (n_changes >= MAX_CHANGES) {
		/* TODO */
		ERR(-1, "TODO");
	}

	eps = calloc(1, sizeof(*eps));
	if (!eps)
		ERR_SYS(EXIT_FAILURE, "calloc");

	eps->fd = fd;
	eps->type = type;
	eps->events = events;

	switch (type) {
	case TYPE_CLIENT_LISTEN:
		EV_SET(&k_changelist[n_changes++], fd, EVFILT_READ , EV_ADD | EV_ENABLE , 0, 0, eps);
		break;

	case TYPE_CLIENT_SOCK:
		EV_SET(&k_changelist[n_changes++], fd, EVFILT_READ , EV_ADD | EV_ENABLE, 0, 0, eps);
		break;

	default:
		break;
	}

	LOG("Epoll struct %p(%d) scheduled to be added to the kqueue instance", eps, fd);

	return NULL;
}

int remove_fd(struct poll_struct *eps)
{
	if (n_changes >= MAX_CHANGES) {
		/* TODO */
		ERR(-1, "TODO");
	}

	if (eps->data)
		free(eps->data);
	if (eps->send_buff)
		free(eps->send_buff);
	if (eps->recv_buff)
		free(eps->recv_buff);
	free(eps);
	close(eps->fd);

	return 0;
}

void init_signals(void)
{
	int sigfd;
	sigset_t mask;

	if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
		ERR(-1, "signal");

	sigemptyset(&mask);
#if 0
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		ERR(-1, "sigprocmask");
#endif
}

int treat_events(struct kevent *event)
{
	if (event->filter & EVFILT_READ) {
		WARN("Read available");
	}

	if (event->filter & EVFILT_READ) {
	}

	return 0;
}

void server_loop()
{
	struct kevent events[MAX_EVENTS];
	struct timespec ts;
	int i, n_events;

	while (1) {
		ts.tv_sec = ts.tv_nsec = 0; /* Return imediately */

		n_events = kevent(kfd, k_changelist, n_changes, events, MAX_EVENTS, &ts);
		if (n_events < 0)
			ERR_SYS(-1, "kevent");

		n_changes = 0;

		for (i = 0; i < n_events; ++i) {
			struct poll_struct *kps;

			kps = events[i].udata;
			LOG("--%p", kps);
			if ((events[i].flags & EV_ERROR)) {
				remove_fd(kps);
				continue;
			}

			if ((events[i].flags & EV_EOF)) {
				LOG("FD %d closed", kps->fd);
				remove_fd(kps);
				continue;
			}

			switch (kps->type) {
			case TYPE_CLIENT_LISTEN:
				accept_sock(kps->fd);
				break;

			case TYPE_CLIENT_SOCK:
				treat_events(&events[i]);
				break;

			default:
				break;
			}
		}

		/* Clean the changes list after the call to kevent */

		usleep(1);
	}
}

void print_poll_stats(void)
{
}
