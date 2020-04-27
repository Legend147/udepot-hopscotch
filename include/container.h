#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <stdint.h>
#include <pthread.h>
#include <libaio.h>
#include "kv_ops.h"
#include "type.h"
#include "cond_lock.h"
#include "queue.h"
#include "stopwatch.h"
#include "device.h"

/* Each device is devided into fixed-size segments */
struct segment {
	uint32_t idx;
	seg_state_t state;

	uint64_t start_addr;
	uint64_t end_addr;
	uint64_t offset;
};

/* Device abstraction */
struct dev_abs {
	char dev_name[128];
	int dev_fd;

	uint32_t nr_logical_block;
	uint32_t logical_block_size;
	uint64_t size_in_byte;

	uint64_t segment_size;
	uint32_t nr_segment;

	struct segment *seg_array;

	struct segment *staged_seg;
	uint32_t staged_seg_idx;
	void *staged_seg_buf;

	uint32_t grain_unit;
};

struct callback {
	void *(*func)(void *);
	void *arg;
};

static inline struct callback *make_callback(void *(*func)(void*), void *arg) {
	struct callback *cb = (struct callback *)malloc(sizeof(struct callback));
	cb->func = func;
	cb->arg  = arg;
	return cb;
}

struct handler {
	int number;
	pthread_t hlr_tid, plr_tid;

	struct kv_ops *ops;

	cl_lock *flying;

	queue *req_q;
	queue *retry_q;

	struct dev_abs *dev;

	int (*read)(struct handler *, uint64_t, uint32_t, char *,
		    struct callback *);
	int (*write)(struct handler *, uint64_t, uint32_t, char *,
		     struct callback *);

#ifdef LINUX_AIO
	io_context_t aio_ctx;
#elif SPDK
#endif
};

struct key_struct {
	uint8_t len;
	char *key;
	hash_t hash;
};

struct val_struct {
	uint32_t len;
	char *value;
};

struct request {
	req_type_t type;
	uint32_t seq_num;
	struct key_struct key;
	struct val_struct value;

	stopwatch *sw;

	void *(*end_req)(void *const);
	void *params;

	struct handler *hlr;

	int cl_sock;
};

#endif
