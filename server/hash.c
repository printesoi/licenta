#include <stdio.h>
#include <stdlib.h>

#include "khash.h"
#include "hash.h"

/* Ignore warnings about unused static functions */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
KHASH_MAP_INIT_STR(hhh, json_t *);
#pragma GCC diagnostic pop

khash_t(hhh) * k_global_hash;

void h_init_hash(void)
{
	k_global_hash = kh_init(hhh);
}

void h_destroy_hash(void)
{
	kh_destroy(hhh, k_global_hash);
}

int h_insert(char *key, json_t *val)
{
	khint_t k;
	int ret;

	k = kh_put(hhh, k_global_hash, key, &ret);
	if (ret < 0)
		return 0;

	kh_val(k_global_hash, k) = val;
	return 1;
}

int h_delete(char *key, json_t **val)
{
	khint_t k;

	k = kh_get(hhh, k_global_hash, key);
	if (k == kh_end(k_global_hash)) {
		*val = NULL;
		return 0;
	}

	*val = kh_val(k_global_hash, k);
	kh_del(hhh, k_global_hash, k);

	return 1;
}

int h_find(char *key, json_t **val)
{
	khint_t k;

	k = kh_get(hhh, k_global_hash, key);
	if (k == kh_end(k_global_hash)) {
		*val = NULL;
		return 0;
	}

	*val = kh_val(k_global_hash, k);
	return 1;
}

json_t *h_dump(void)
{
	json_t *result;
	const char *key;
	json_t *value;
	khint_t it;

	result = json_object();
	if (!result)
		return NULL;

	for (it = kh_begin(k_global_hash); it != kh_end(k_global_hash); ++it) {
		if (!kh_exist(k_global_hash, it))
			continue;

		key = kh_key(k_global_hash, it);
		value = kh_val(k_global_hash, it);

		json_object_set(result, key, value);
	}

	return result;
}
