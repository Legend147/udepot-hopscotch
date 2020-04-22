#include "handler.h"
#include <stdlib.h>
#include <string.h>

struct request *make_request_from_netreq(struct net_req *nr, int sock) {
	struct request *req = (struct request *)malloc(sizeof (struct request));

	req->type = nr->type;
	req->keylen = nr->keylen;
	req->key = (char *)malloc(req->keylen);
	memcpy(req->key, nr->key, req->keylen);
	req->hkey = hashing_key(req->key, req->keylen);

	req->val_len = 1024;
	req->value = (char *)malloc(req->val_len);

	req->sw = sw_create();
	sw_start(req->sw);

	req->end_req = net_end_req;
	req->params = NULL;

	req->hlr = NULL;

	req->cl_sock = sock;

	return req;
}

void *net_end_req(void *_req) {
	struct request *req = (struct request *)_req;
	struct handler *hlr = req->hlr;
	struct net_ack ack;

	sw_end(req->sw);
	ack.seq_num = 0; // TODO
	ack.type = req->type;
	ack.elapsed_time = sw_get_usec(req->sw);

	send_ack(req->cl_sock, &ack);

	cl_release(hlr->flying);

	sw_destroy(req->sw);
	free(req->key);
	free(req);

	return NULL;
}
