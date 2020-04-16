#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "type.h"
#include "stopwatch.h"

enum req_type_t:unsigned char {
	REQ_TYPE_SET,
	REQ_TYPE_GET,
	REQ_TYPE_DELETE,
	REQ_TYPE_RANGE,
	REQ_TYPE_ITERATOR,
};

struct request {
	req_type_t type;
	char *key;
	hash_t hkey;

	void (*end_req)(struct request *const);
	void *params;

	stopwatch *sw;
};

struct request *make_request_from_netreq(struct net_req *nr);

#endif
