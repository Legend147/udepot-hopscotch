#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <stdint.h>
#include <pthread.h>
#include <libaio.h>
#include "ops.h"
#include "type.h"
#include "cond_lock.h"
#include "queue.h"
#include "stopwatch.h"

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

	struct hash_ops *hops;

	cl_lock *flying;

	queue *req_q;
	queue *retry_q;

	int dev_fd;
	int (*read)(struct handler *hlr, uint64_t pba, uint32_t size, char *buf, struct callback *cb);
	int (*write)(struct handler *hlr, uint64_t pba, uint32_t size, char *buf, struct callback *cb);

#ifdef LINUX_AIO
	io_context_t aio_ctx;
#elif SPDK
#endif
};

struct request {
	req_type_t type;
	uint8_t keylen;
	char *key;
	hash_t hkey;
	uint32_t val_len;
	char *value;

	stopwatch *sw;

	void *(*end_req)(void *const);
	void *params;

	struct handler *hlr;

	int cl_sock;
};


#endif
