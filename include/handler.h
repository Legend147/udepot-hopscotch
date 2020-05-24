#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "config.h" 
#include "type.h"
#include "queue.h"
#include "cond_lock.h"

#include <pthread.h>
#include <libaio.h>
#include <stdlib.h>
#include <stdint.h>

struct handler {
	int number;
	pthread_t hlr_tid, plr_tid;

	struct kv_ops *ops;
	struct dev_abs *dev;

	cl_lock *flying;

	queue *req_q;
	queue *retry_q;
	queue *done_q;

	queue *req_pool;
	struct request *req_arr;
	queue *iocb_pool;
	struct iocb *iocb_arr;
	queue *cb_pool;
	struct callback *cb_arr;

#ifdef LINUX_AIO
	io_context_t aio_ctx;
#elif SPDK
	// TODO: include SPDK variables
#endif

	int (*read)(struct handler *, uint64_t, uint32_t, char *,
		    struct callback *);
	int (*write)(struct handler *, uint64_t, uint32_t, char *,
		     struct callback *);
};

struct callback {
	void *(*func)(void *);
	void *arg;
#ifdef LINUX_AIO
	struct iocb *iocb;
#endif
};

static inline struct callback *
make_callback(struct handler *hlr, void *(*func)(void*), void *arg) {
	//struct callback *cb = (struct callback *)malloc(sizeof(struct callback));
	struct callback *cb = (struct callback *)q_dequeue(hlr->cb_pool);
	cb->func = func;
	cb->arg  = arg;
#ifdef LINUX_AIO
	cb->iocb = NULL;
#endif
	return cb;
}

/* handler */
struct handler *handler_init(char dev_name[]);
void handler_free(struct handler *hlr);

int forward_req_to_hlr(struct handler *hlr, struct request *req);
int retry_req_to_hlr(struct handler *hlr, struct request *req);

struct request *get_next_request(struct handler *hlr);

void *request_handler(void *input);


/* poller */
void *device_poller(void *input);

#endif
