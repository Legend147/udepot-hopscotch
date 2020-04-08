/*
 * Header for Hopscotch Hash
 */

#ifndef __HOPSCOTCH_H__
#define __HOPSCOTCH_H__

#include <stdint.h>
#include "config.h"
#include <list>

#define IDX_BIT 27
#define DIR_BIT 0
#define NR_TABLE (1 << DIR_BIT)
#define NR_ENTRY (1 << IDX_BIT)
#define NR_PART (NR_ENTRY/PAGESIZE*8)
#define MAX_HOP 32
#define PBA_INVALID 0xffffffffff
#define NR_CACHED_PART (AVAIL_MEMORY/PAGESIZE)
#define FLASH_READ_MAX 10

#define TEMP_BUF_SIZE 16384

struct hash_entry {
	uint64_t neigh_off:5;
	uint64_t key_fp_tag:8;
	uint64_t kv_size:11;
	uint64_t pba:40;
};

struct hash_table {
	struct hash_entry *entry;
	//bool *is_cached;
};

struct hopscotch {
	struct hash_table *table;
	//std::list<bool *> cached_part;
	char *temp_buf;

	uint64_t lookup_cost[FLASH_READ_MAX];
};

int hopscotch_init();
int hopscotch_free();
int hopscotch_insert(uint64_t h_key);
int hopscotch_lookup(uint64_t h_key);
int hopscotch_remove(uint64_t h_key);

#endif
