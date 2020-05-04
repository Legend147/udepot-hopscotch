#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "type.h"
#include "handler.h"
#include "stopwatch.h"
#include "util.h"

struct key_struct {
	uint8_t len;
	char *key;
	hash_t hash_low, hash_high;
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
	char *temp_buf;

	struct handler *hlr;

	int cl_sock;
};

struct request *make_request_from_netreq(struct net_req *nr, int sock);
void *net_end_req(void *_req);

#endif
