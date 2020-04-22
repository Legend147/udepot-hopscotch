#ifndef __AIO_H__
#define __AIO_H__

#include "container.h"
#include <libaio.h>

int aio_read(struct handler *hlr, uint64_t pba,
	     uint32_t size, char *buf, struct callback *cb);

int aio_write(struct handler *hlr, uint64_t pba,
	      uint32_t size, char *buf, struct callback *cb);

#endif
