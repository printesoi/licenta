#ifndef UTILS_H_
#define UTILS_H_

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#include <errno.h>
#include <string.h>

#define LOGG(fmt, ...)		\
	mlog(__FILENAME__, __func__, __LINE__, fmt "\n", ##__VA_ARGS__)
#define LOG(fmt, ...)	LOGG("[INFO] " fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)	LOGG("[WARN] " fmt, ##__VA_ARGS__)
#define WARN_SYS(fmt, ...)	\
	LOGG("[WARN] " fmt ": (errno %d) %s", ##__VA_ARGS__, errno, \
	     strerror(errno))
#define ERR(ex, fmt, ...)	do { \
		LOGG("[ERROR] " fmt, ##__VA_ARGS__); \
		exit((ex)); \
	} while (0)
#define ERR_SYS(ex, fmt, ...)	do { \
		LOGG("[ERROR] " fmt ": (errno %d) %s", ##__VA_ARGS__, errno, \
		     strerror(errno)); \
		exit((ex)); \
	} while (0)
#define ASSERT(exp)	do { \
		if (!exp) { \
			ERR(-1, "ASSERT CONDITION " #exp " FAILED"); \
		} \
	} while (0)


void mlog(const char *file, const char *func, unsigned long line,
	  const char *fmt, ...) __attribute__((format(printf, 4, 5)));

struct timespec timespec_diff(struct timespec start, struct timespec end);
struct timespec timespec_add(struct timespec ts1, struct timespec ts2);

int timeit_tsc(struct timespec *ts);

double timespec_to_double(struct timespec ts);
struct timespec double_to_timespec(double ts);

#endif /* UTILS_H_ */
