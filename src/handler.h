#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "type.h"
#include "stopwatch.h"
#include "util.h"
#include "queue.h"
#include "cond_lock.h"
#include "config.h" 
#include <pthread.h>

struct handler {
	int number;
	pthread_t t_id;

	cl_lock *flying;
	//cl_lock *cond;

	queue *req_q;
	queue *retry_q;
};

struct request {
	req_type_t type;
	uint8_t keylen;
	char *key;
	hash_t hkey;

	stopwatch *sw;

	void (*end_req)(struct request *const);
	void *params;

	struct handler *hlr;

	int cl_sock;
};

/* request */
struct request *make_request_from_netreq(struct net_req *nr, int sock);
void net_end_req(struct request *req);


/* handler */
struct handler *handler_init();
void handler_free(struct handler *hlr);

int forward_req_to_hlr(struct handler *hlr, struct request *req);
int retry_req_to_hlr(struct handler *hlr, struct request *req);

struct request *get_next_request(struct handler *hlr);

void *request_handler(void *input);

#endif
