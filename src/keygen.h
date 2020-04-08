#ifndef __KEYGEN_H__
#define __KEYGEN_H__

#include <stdint.h>

typedef char* kg_key_t;

enum key_dist_t {
	KEY_DIST_UNIFORM,
	KEY_DIST_HOTSPOT,
};

struct keygen {
	kg_key_t *key_pool;

	uint64_t nr_key;
	int key_size;
	key_dist_t key_dist;

	uint32_t seed;

	uint64_t load_cnt;
};

struct keygen *keygen_init(uint64_t nr_key, int key_size);
int keygen_free(struct keygen *kg);

int set_key_dist(struct keygen *kg, key_dist_t dist);
kg_key_t get_next_key(struct keygen *kg);
kg_key_t get_next_key_for_load(struct keygen *kg);

#endif
