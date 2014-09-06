#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "utils.h"

void mlog(const char *file, const char *func, unsigned long line, const char *fmt, ...)
{
	va_list args;
	struct timeval tv;
	struct tm tm;

	if (gettimeofday(&tv, NULL) == 0) {
		localtime_r(&tv.tv_sec, &tm);
		fprintf(stdout, "[%02d:%02d:%02d.%06ld] ", tm.tm_hour, tm.tm_min,
			tm.tm_sec, tv.tv_usec);
	}
	fprintf(stdout, "%s:%lu:%s ", file, line, func);

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}
