/*
 * Header for types
 */

#ifndef __HASH_TYPE_H__
#define __HASH_TYPE_H__

#include <stdint.h>

typedef uint64_t hash_t;

enum seg_state_t:unsigned char {
	SEG_STATE_FREE,
	SEG_STATE_STAGED,
	SEG_STATE_USED,
};

enum req_type_t:unsigned char {
	REQ_TYPE_SET,
	REQ_TYPE_GET,
	REQ_TYPE_DELETE,
	REQ_TYPE_RANGE,
	REQ_TYPE_ITERATOR,
};

enum htable_t {
	HTABLE_HOPSCOTCH,
	HTABLE_BIGKV,
};

#endif
