#include "handler.h"
#include "ops.h"
#include "hopscotch.h"
#include <stdlib.h>
#include <stdio.h>

int global_number;

extern struct hash_ops hash_ops;

struct handler *handler_init() {
	struct handler *hlr = (struct handler *)malloc(sizeof(struct handler));

	hlr->number = global_number++;
	hlr->flying = cl_init(QDEPTH, false);
	//hlr->cond = cl_init(QDEPTH, true);
	q_init(&hlr->req_q, QSIZE);
	q_init(&hlr->retry_q, QSIZE);

	pthread_create(&hlr->t_id, NULL, &request_handler, hlr);

	return hlr;
}

void handler_free(struct handler *hlr) {
	int *temp;
	while (pthread_tryjoin_np(hlr->t_id, (void **)&temp)) {
		//cl_release(hlr->cond);
	}
	q_free(hlr->req_q);
	q_free(hlr->retry_q);

	free(hlr);
} 
int forward_req_to_hlr(struct handler *hlr, struct request *req) {
	int rc = 0;

	cl_grap(hlr->flying);
	req->hlr = hlr;
	if (!q_enqueue((void *)req, hlr->req_q)) {
		rc = 1;
	}
	return rc;

}

int retry_req_to_hlr(struct handler *hlr, struct request *req) {
	int rc = 0;
	if (!q_enqueue((void *)req, hlr->retry_q)) {
		rc = 1;
	}
	return rc;
}

struct request *get_next_request(struct handler *hlr) {
	void *req = NULL;
	if ((req = q_dequeue(hlr->retry_q))) goto exit;
	else if ((req = q_dequeue(hlr->req_q))) goto exit;
exit:
	return (struct request *)req;
}

void *request_handler(void *input) {
	int rc = 0;

	struct request *req = NULL;
	struct handler *hlr = (struct handler *)input;

	char thread_name[128] = {0};
	sprintf(thread_name, "%s[%d]", "request_handler", hlr->number);
	pthread_setname_np(pthread_self(), thread_name);

	printf("%s launched\n", thread_name);

	while (1) {
		//cl_grap(hlr->cond);
		if (!(req=get_next_request(hlr))) {
			//cl_release(hlr->cond);
			continue;
		}
		static int cnt = 0;
		printf("%d, %d, %x\n", cnt++, req->type, req->hkey);

		switch (req->type) {
		case REQ_TYPE_SET:
			rc = hash_ops.insert(req->hkey);
			req->end_req(req);
			break;	
		case REQ_TYPE_GET:
			rc = hash_ops.lookup(req->hkey);
			if (rc) {
				puts("Not existing key!");
				abort();
			}
			req->end_req(req);
			break;
		case REQ_TYPE_DELETE:
		case REQ_TYPE_RANGE:
		case REQ_TYPE_ITERATOR:
		default:
			fprintf(stderr, "Wrong req type!\n");
			break;
		}
	}
	return NULL;
}

