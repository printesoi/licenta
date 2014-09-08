#ifndef HASH_H_
#define HASH_H_

#include <jansson.h>

/**
 * Initialize the internal hash table
 */
void h_init_hash(void);

/**
 * Destroy the internal hash table
 */
void h_destroy_hash(void);

int h_insert(const char *key, json_t *val);
int h_delete(const char *key, json_t **val);
int h_find(const char *key, json_t **val);
json_t *h_dump(void);

#endif /* HASH_H_ */
