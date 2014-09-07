#ifndef OS_H_
#define OS_H_

#include <time.h>
#include <stdint.h>

#define EV_IN		0x01
#define EV_OUT		0x02

enum fd_type {
	TYPE_CLIENT_LISTEN,
	TYPE_CLIENT_SOCK,
	TYPE_SIGNAL,
	TYPE_TIMER,
};

struct buffer {
	size_t bsize;
#define BUFFER_SIZE (1 << 14)
	char bdata[BUFFER_SIZE];
};

struct poll_struct {
	int fd;
	enum fd_type type;
	uint32_t events;

	struct buffer *recv_buff;
	struct buffer *send_buff;

	void *data;
};


extern struct timespec g_syscall_ovhead;

int timeit(struct timespec *ts);
char *next_packet(struct buffer *buffer);

void setup_environment(void);

int listen_tcp(unsigned short port, unsigned long in_addr);
int accept_sock(int listenfd);

struct poll_struct *monitor_fd(int fd, enum fd_type type, uint32_t events);
int remove_fd(struct poll_struct *eps);

int recv_string(struct poll_struct *ps, int sockfd, struct buffer *buff);
int send_string(struct poll_struct *ps, int sockfd, const char *str,
		size_t size);

void server_loop(void);
void print_poll_stats(void);

void init_signals(void);
void treat_signal(struct poll_struct *ps);

#endif /* OS_H_ */
