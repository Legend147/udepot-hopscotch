#include "request.h"
#include <stdlib.h>
#include <string.h>

stopwatch *sw_send, *sw_free;
time_t t_send, t_free;

static int set_value(struct val_struct *value, int len, char *input_val) {
	int rc = 0;

	value->len = len;
#ifdef LINUX_AIO
	value->value = (char *)aligned_alloc(VALUE_ALIGN_UNIT, value->len);
	if (!value->value) {
		perror("allocating value");
		abort();
	}
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

struct request *
make_request_from_netreq(struct handler *hlr, struct net_req *nr, int sock) {
	//struct request *req = (struct request *)malloc(sizeof (struct request));
	struct request *req = (struct request *)q_dequeue(hlr->req_pool);

	req->type = nr->type;
	req->seq_num = nr->seq_num;

	req->key.len = nr->keylen;
	memcpy(req->key.key, nr->key, req->key.len);

	uint128 hash128 = hashing_key_128(req->key.key, req->key.len);
	req->key.hash_low = hash128.first;
	req->key.hash_high = hash128.second;

	switch (req->type) {
	case REQ_TYPE_GET:
		set_value(&req->value, VALUE_LEN_MAX, NULL);
		break;
	case REQ_TYPE_SET:
		set_value(&req->value, nr->kv_size, NULL);
		break;
	case REQ_TYPE_DELETE:
	case REQ_TYPE_RANGE:
	case REQ_TYPE_ITERATOR:
	default:
		break;
	}

	sw_start(&req->sw);

	req->end_req = net_end_req;
	req->params = NULL;
	req->temp_buf = NULL;

	req->hlr = NULL;

	req->cl_sock = sock;

	return req;
}

void *net_end_req(void *_req) {
	struct request *req = (struct request *)_req;
	struct handler *hlr = req->hlr;
	struct net_ack ack;

	req_type_t rtype = req->type;

	sw_end(&req->sw);
	ack.seq_num = req->seq_num; // TODO
	ack.type = req->type;
	ack.elapsed_time = sw_get_usec(&req->sw);

	//sw_start(sw_send);
	send_ack(req->cl_sock, &ack);
	//sw_end(sw_send);
	//t_send += sw_get_usec(sw_send);

	cl_release(hlr->flying);

	//sw_start(sw_free);
	if (req->params) free(req->params);
	free(req->value.value);
	q_enqueue((void *)req, hlr->req_pool);
	//sw_end(sw_free);
	//t_free += sw_get_usec(sw_free);

	return NULL;
}
