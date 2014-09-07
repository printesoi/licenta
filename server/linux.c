#include <time.h>

#include "utils.h"
#include "os.h"

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

void server_loop(void)
{
}
