#ifndef OS_H_
#define OS_H_

#include <time.h>

extern struct timespec g_syscall_ovhead;

int timeit(struct timespec *ts);

void setup_environment(void);

void server_loop(void);

#endif /* OS_H_ */
