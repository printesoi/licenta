#include <stdlib.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/signalfd.h>

#include "utils.h"
#include "os.h"


/* Counting total time spent in epoll_* and the number of calls */
static struct timespec total_epoll_wait, total_epoll_ctl;
static size_t n_epoll_wait, n_epoll_ctl;

#define PRINT_TS(tv) LOG("TS %ld.%09ld", (tv).tv_sec, (tv).tv_nsec);

static int epfd;

int timeit(struct timespec *ts)
{
	int rc = -1;

#ifndef SERVER_USE_TSC
	/* Thread specific hi-res CPU-time. XXX maybe check if supported? */
	rc = clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
#else
	/* TODO use TSC register */
	rc = timeit_tsc(ts);
#endif
	return rc;
}

struct poll_struct *monitor_fd(int fd, enum fd_type type, uint32_t events)
{
	struct poll_struct *eps;
	struct epoll_event ev;
	uint32_t ep_events = 0;
	struct timespec start, end, diff;
	int ret;

	if (!events)
		events = EV_IN;

	if (!epfd) {
		epfd = epoll_create(1);
		if (epfd < 0)
			ERR_SYS(EXIT_FAILURE, "epoll_create");
	}

	eps = calloc(1, sizeof(*eps));
	if (!eps)
		ERR_SYS(EXIT_FAILURE, "calloc");

	if ((events & EV_IN))
		ep_events |= EPOLLIN;
	if ((events & EV_OUT))
		ep_events |= EPOLLOUT;

	/* Edge-triggered semantics */
	ep_events |= EPOLLET;

	eps->fd = fd;
	eps->type = type;
	eps->events = events;

	ev.events = events;
	ev.data.ptr = eps;

	if (timeit(&start))
		ERR_SYS(-1, "timeit");

	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

	if (timeit(&end))
		ERR_SYS(-1, "timeit");

	if (ret < 0)
		ERR_SYS(EXIT_FAILURE, "epoll_ctl add fd %d, events %X",
			fd, events);

	diff = timespec_diff(start, end);
	diff = timespec_diff(g_syscall_ovhead, diff);
	/* Should be always true! */
	if (diff.tv_sec >= 0)
		total_epoll_ctl = timespec_add(total_epoll_ctl, diff);
	n_epoll_ctl++;

	LOG("Epoll struct %p(%d) added to epoll instance", eps, fd);

	return eps;
}

int change_events(struct poll_struct *eps, uint32_t events)
{
	struct epoll_event ev;
	struct timespec start, end, diff;
	int ret;

	eps->events = events;
	ev.events = events;
	ev.data.ptr = eps;

	if (timeit(&start))
		ERR_SYS(-1, "timeit");

	ret = epoll_ctl(eps->fd, EPOLL_CTL_MOD, eps->fd, &ev);

	if (timeit(&end))
		ERR_SYS(-1, "timeit");

	if (ret < 0)
		ERR_SYS(-1, "epoll_ctl MOD eps %p(%d) events %X", eps, eps->fd,
			events);

	diff = timespec_diff(start, end);
	diff = timespec_diff(g_syscall_ovhead, diff);
	/* Should be always true! */
	if (diff.tv_sec >= 0)
		total_epoll_ctl = timespec_add(total_epoll_ctl, diff);
	n_epoll_ctl++;

	LOG("Epoll struct %p(%d) changed monintored events to %X", eps,
	    eps->fd, events);

	return 0;
}

int remove_fd(struct poll_struct *eps)
{
	struct timespec start, end, diff;
	int rc;

	if (timeit(&start))
		ERR_SYS(-1, "timeit");

	rc = epoll_ctl(epfd, EPOLL_CTL_DEL, eps->fd, NULL);

	if (timeit(&end))
		ERR_SYS(-1, "timeit");

	if (rc < 0 && errno != ENOENT)
		ERR_SYS(-1, "epoll_ctl");

	diff = timespec_diff(start, end);
	diff = timespec_diff(g_syscall_ovhead, diff);
	/* Should be always true! */
	if (diff.tv_sec >= 0)
		total_epoll_ctl = timespec_add(total_epoll_ctl, diff);
	n_epoll_ctl++;

	LOG("Epoll struct %p(%d) removed from interest list", eps, eps->fd);

	if (eps->data)
		free(eps->data);
	if (eps->send_buff)
		free(eps->send_buff);
	if (eps->recv_buff)
		free(eps->recv_buff);
	close(eps->fd);
	free(eps);

	return 0;
}

int treat_events(struct poll_struct *eps, uint32_t events)
{
	int ret;

	if (events & EPOLLIN) {
		if (!eps->recv_buff) {
			eps->recv_buff = calloc(1, sizeof(*eps->recv_buff));
			if (!eps->recv_buff)
				ERR_SYS(-1, "calloc");
		}

		ret = recv_string(eps, eps->fd, eps->recv_buff);
		if (ret < 0)
			return ret;
	}

	if (events & EPOLLOUT) {
		if (eps->send_buff && eps->send_buff->bsize) {
			do {
				/* TODO use send_string */
				ret = send(eps->fd, eps->send_buff->bdata,
					   eps->send_buff->bsize, 0);
				if (ret < 0) {
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						break;
					WARN_SYS("send fd %d", eps->fd);
					return -1;
				}

				if (ret > 0) {
					eps->send_buff->bsize -= ret;
					memmove(eps->send_buff->bdata,
						eps->send_buff->bdata + ret,
						eps->send_buff->bsize);
				}
			} while (eps->send_buff->bsize);
		}
	}

	return 0;
}

#define N_EVENTS    1000
void server_loop(void)
{
	struct epoll_event events[N_EVENTS];
	struct timespec start, end, diff;
	struct poll_struct *eps;
	int i, ret;

	while (1) {
		if (timeit(&start))
			ERR_SYS(-1, "timeit");
		ret = epoll_wait(epfd, events, N_EVENTS, 0);
		if (timeit(&end))
			ERR_SYS(-1, "timeit");
		if (ret < 0)
			ERR_SYS(-1, "epoll_wait");
		diff = timespec_diff(start, end);
		diff = timespec_diff(g_syscall_ovhead, diff);
		total_epoll_wait= timespec_add(total_epoll_wait, diff);
		n_epoll_wait++;

		for (i = 0; i < ret; ++i) {
			if ((events[i].events & (EPOLLHUP | EPOLLERR)) &&
			    !(events[i].events & (EPOLLIN | EPOLLOUT))) {
				remove_fd(events[i].data.ptr);
				continue;
			}

			eps = events[i].data.ptr;
			switch (eps->type) {
			case TYPE_CLIENT_LISTEN:
				accept_sock(eps->fd);
				break;

			case TYPE_CLIENT_SOCK:
				if (treat_events(eps, events[i].events) < 0) {
					remove_fd(eps);
					continue;
				}
				break;

			case TYPE_SIGNAL:
				treat_signal(eps);
				break;

			default:
				break;
			}
		}

		usleep(1);
	}
}

void print_poll_stats(void)
{
	fpe("Total time in epoll_ctl: %ld.%09ld", total_epoll_ctl.tv_sec,
	    total_epoll_ctl.tv_nsec);
	fpe("Total number of calls to epoll_ctl: %zu", n_epoll_ctl);
	fpe("Average time for epoll_ctl call: %.9lf",
	    timespec_to_double(total_epoll_ctl) / n_epoll_ctl);

	fpe("Total time in epoll_wait: %ld.%09ld", total_epoll_wait.tv_sec,
	    total_epoll_wait.tv_nsec);
	fpe("Total number of calls to epoll_wait: %zu", n_epoll_wait);
	fpe("Average time for epoll_wait call: %.9lf",
	    timespec_to_double(total_epoll_wait) / n_epoll_ctl);
}

void init_signals(void)
{
	int sigfd;
	sigset_t mask;

	if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
		ERR(-1, "signal");

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		ERR(-1, "sigprocmask");

	sigfd = signalfd(-1, &mask, 0);
	if (sigfd < 0)
		ERR(-1, "signalfd");

	monitor_fd(sigfd, TYPE_SIGNAL, EV_IN);
}

void treat_signal(struct poll_struct *ps)
{
	struct signalfd_siginfo siginfo;
	ssize_t sz;

	sz = read(ps->fd, &siginfo, sizeof(siginfo));
	if (sz != sizeof(siginfo))
		ERR(-1, "Unknown signal caught. Returning..");
	else
	{
		char *signame = strsignal(siginfo.ssi_signo);
		ERR(0, "Caught signal %d: %s. Returning..", siginfo.ssi_signo, signame ? signame : "UNKNOWN");
	}
}
