#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "os.h"

#ifndef SETUP_SYSCALL_STEPS
#   define SETUP_SYSCALL_STEPS	20000000
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
