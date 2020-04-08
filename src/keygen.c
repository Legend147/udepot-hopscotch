#include "keygen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_KEY "ERROR KEY"

struct keygen *keygen_init(uint64_t nr_key, int key_size) {
	struct keygen *kg = (struct keygen *)malloc(sizeof(struct keygen));

	kg->nr_key = nr_key;
	kg->key_size = key_size;

	kg->key_dist = KEY_DIST_UNIFORM;

	kg->seed = 0;
	srand(kg->seed);

	kg->load_cnt = 0;

	kg->key_pool = (kg_key_t *)malloc(sizeof(kg_key_t) * kg->nr_key);
	for (size_t i = 0; i < kg->nr_key; i++) {
		kg->key_pool[i] = (kg_key_t)malloc(kg->key_size);
		strncpy(kg->key_pool[i], "user", 4);
		for (int j = 4; j < kg->key_size; j++) kg->key_pool[i][j] = '0'+(rand()%10);
	}

	return kg;
}

int keygen_free(struct keygen *kg) {
	for (size_t i = 0; i < kg->nr_key; i++) {
		free(kg->key_pool[i]);
	}
	free(kg->key_pool);
	free(kg);
	return 0;
}

int set_key_dist(struct keygen *kg, key_dist_t dist) {
	kg->key_dist = dist;
	return 0;
}

kg_key_t get_next_key_for_load(struct keygen *kg) {
	if (kg->load_cnt >= kg->nr_key) {
		fprintf(stderr, "Loading key overflow!\n");
		return NULL;
	} else {
		return kg->key_pool[kg->load_cnt++];
	}
}

kg_key_t get_next_key(struct keygen *kg) {
	switch (kg->key_dist) {
	case KEY_DIST_UNIFORM:
		return kg->key_pool[rand()%kg->nr_key];
	case KEY_DIST_HOTSPOT:
		break;
	default:
		fprintf(stderr, "Wrong key-distribution!\n");
	}
	return NULL;
}
