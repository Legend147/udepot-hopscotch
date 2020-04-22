/*
 * Header to define hash operations
 */

#ifndef __OPS_H__
#define __OPS_H__

#include <stdint.h>
#include "container.h"

struct hash_ops {
	/* Primary */
	int (*init) (struct hash_ops *);
	int (*free) (struct hash_ops *);
	int (*insert) (struct hash_ops *, struct request *);
	int (*lookup) (struct hash_ops *, struct request *);
	int (*remove) (struct hash_ops *, struct request *);

	void *_private;
};

struct hash_stat {
	uint64_t flash_read;
	uint64_t flash_write;

	uint64_t insert;
	uint64_t lookup;
	uint64_t remove;
};

#endif
