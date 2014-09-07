#ifndef UTILS_H_
#define UTILS_H_

void mlog(const char *file, const char *func, unsigned long line, const char *fmt, ...) __attribute__((format(printf, 4, 5)));

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOGG(fmt, ...)	mlog(__FILENAME__, __func__, __LINE__, fmt "\n", ##__VA_ARGS__)
#define LOG(fmt, ...)	LOGG("[INFO] " fmt, ##__VA_ARGS__)
#define WARN(fmt, ...)	LOGG("[WARN] " fmt, ##__VA_ARGS__)
#define ERR(ex, fmt, ...)	do { \
		LOGG("[ERROR] " fmt, ##__VA_ARGS__); \
		exit((ex)); \
	} while (0)
#define ASSERT(exp)	do { \
		if (!exp) { \
			ERR(-1, "ASSERT CONDITION " #exp " FAILED"); \
		} \
	} while (0)


#endif /* UTILS_H_ */
