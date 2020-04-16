#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "request.h"
#include "queue.h"
#include "cond_lock.h"
#include "config.h" 
#include <pthread.h>

struct handler {
	int number;
	pthread_t t_id;

	cl_lock *cond;

	queue *req_q;
	queue *retry_q;
};

struct handler *handler_init();
void handler_free(struct handler *hlr);

void *request_handler(void *input);

struct request *get_next_request(struct handler *hlr);

#endif
