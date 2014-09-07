#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "hash.h"
#include "os.h"

int main(int argc, char *argv[])
{
	struct timespec start, end, res;
	json_t *j;
	char *s;

	if (timeit(&start) < 0)
		ERR_SYS(-1, "timeit");

#if 0
	h_init_hash();

	h_insert("foo", json_pack("{s:s}", "a", "b"));
	h_insert("bar", json_pack("{s:I}", "key", 100));
	j = h_dump();

	s = json_dumps(j, 0);
	LOG("HASH: %s", s);
	free(s);
	json_decref(j);

	if (h_delete("foo", &j)) {
		json_decref(j);
	}
	if (h_delete("bar", &j)) {
		json_decref(j);
	}

	h_destroy_hash();
#endif

	if (timeit(&end) < 0)
		ERR_SYS(-1, "timeit");

	res = timespec_diff(start, end);
	LOG("-- Took %ld.%09ld s", res.tv_sec, res.tv_nsec);

	start.tv_sec = 1;
	start.tv_nsec = 999999999;

	end.tv_sec = 2;
	end.tv_nsec = 1;

	res = timespec_add(start, end);
	LOG("-- Took %ld.%09ld s", res.tv_sec, res.tv_nsec);

	setup_environment();

	return 0;
}
