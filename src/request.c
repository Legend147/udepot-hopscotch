#include "handler.h"
#include <stdlib.h>
#include <string.h>

static int set_value(struct val_struct *value, int len, char *input_val) {
	int rc = 0;
	void *tmp_value = NULL;

	value->len = len;
#ifdef LINUX_AIO
	rc = posix_memalign(&tmp_value, MEM_ALIGN_UNIT, value->len);
	if (rc) {
		perror("allocating value");
		abort();
	}
	value->value = (char *)tmp_value;
	if (input_val) {
		memcpy(value->value, input_val, value->len);
	} else {
		memset(value->value, 0, value->len);
	}
#elif SPDK
	// TODO: implement SPDK allocation
#endif
	return rc;
}

struct request *make_request_from_netreq(struct net_req *nr, int sock) {
	struct request *req = (struct request *)malloc(sizeof (struct request));

	req->type = nr->type;
	req->seq_num = nr->seq_num;

	req->key.len = nr->keylen;
	req->key.key = (char *)malloc(req->key.len);
	memcpy(req->key.key, nr->key, req->key.len);
	req->key.hash = hashing_key(req->key.key, req->key.len);

	switch (req->type) {
	case REQ_TYPE_GET:
		set_value(&req->value, VALUE_LEN_MAX, NULL);
		break;
	case REQ_TYPE_SET:
		set_value(&req->value, nr->kv_size, NULL);
		//copy_key_to_value(&req->key, &req->value);
		break;
	case REQ_TYPE_DELETE:
	case REQ_TYPE_RANGE:
	case REQ_TYPE_ITERATOR:
	default:
		break;
	}

/*	req->val_len = 4096;
	void *target;
	int res = posix_memalign(&target, 4096, req->val_len);
	if (res) {
		perror("allocating value");
		abort();
	}
	memset(target, 0, req->val_len);
	req->value = (char *)target; */

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
	ack.seq_num = req->seq_num; // TODO
	ack.type = req->type;
	ack.elapsed_time = sw_get_usec(req->sw);

	send_ack(req->cl_sock, &ack);

	cl_release(hlr->flying);

	sw_destroy(req->sw);

	free(req->params);
	free(req->value.value);
	free(req->key.key);
	free(req);

	return NULL;
}
