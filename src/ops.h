/*
 * Header to define hash operations
 */

#ifndef __OPS_H__
#define __OPS_H__

#include <stdint.h>

struct hash_ops {
	/* Primary */
	int (*init) ();
	int (*free) ();
	int (*insert) (uint64_t);
	int (*lookup) (uint64_t);
	int (*remove) (uint64_t);
};

struct hash_stat {
	uint64_t flash_read;
	uint64_t flash_write;

	uint64_t insert;
	uint64_t lookup;
	uint64_t remove;
};

#endif
