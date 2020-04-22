#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "type.h"
#include "stopwatch.h"
#include "util.h"
#include "queue.h"
#include "cond_lock.h"
#include "config.h" 
#include "container.h"
#include <pthread.h>

/* request */
struct request *make_request_from_netreq(struct net_req *nr, int sock);
void *net_end_req(void *_req);


/* handler */
struct handler *handler_init(htable_t ht_type);
void handler_free(struct handler *hlr);

int forward_req_to_hlr(struct handler *hlr, struct request *req);
int retry_req_to_hlr(struct handler *hlr, struct request *req);

struct request *get_next_request(struct handler *hlr);

void *request_handler(void *input);


/* poller */
void *device_poller(void *input);

#endif
