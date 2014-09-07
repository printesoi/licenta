#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "utils.h"

void mlog(const char *file, const char *func, unsigned long line,
	  const char *fmt, ...)
{
	va_list args;
	struct timeval tv;
	struct tm tm;

	if (gettimeofday(&tv, NULL) == 0) {
		localtime_r(&tv.tv_sec, &tm);
		fprintf(stdout, "[%02d:%02d:%02d.%06ld] ", tm.tm_hour,
			tm.tm_min, tm.tm_sec, tv.tv_usec);
	}
	fprintf(stdout, "%s:%lu:%s ", file, line, func);

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}

struct timespec timespec_diff(struct timespec start, struct timespec end)
{
	struct timespec res;

	if (end.tv_nsec >= start.tv_nsec) {
		res.tv_sec = end.tv_sec - start.tv_sec;
		res.tv_nsec = end.tv_nsec - start.tv_nsec;
	} else {
		res.tv_sec = end.tv_sec - start.tv_sec - 1;
		res.tv_nsec = 1E9 + end.tv_nsec - start.tv_nsec;
	}

	return res;
}

struct timespec timespec_add(struct timespec ts1, struct timespec ts2)
{
	struct timespec res;

	res.tv_sec = ts1.tv_sec + ts2.tv_sec;
	res.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;

	if (res.tv_nsec >= 1E9) {
		res.tv_sec += 1;
		res.tv_nsec -= 1E9;
	}

	return res;
}

int timeit_tsc(struct timespec *ts)
{
	int rc = -1;

	/* TODO not implemented */

	return rc;
}

double timespec_to_double(struct timespec ts)
{
	return ts.tv_sec + ts.tv_nsec / (1E9 * 1.0L);
}

struct timespec double_to_timespec(double ts)
{
	struct timespec ret;

	ret.tv_sec = (time_t)ts;
	ret.tv_nsec = (ssize_t)((ts - ret.tv_sec) * 1E9L);

	return ret;
}
