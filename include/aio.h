#ifndef __AIO_H__
#define __AIO_H__

#include "handler.h"

#include <libaio.h>
#include <stdint.h>

int aio_read(struct handler *hlr, uint64_t addr_in_byte, uint32_t size,
	     char *buf, struct callback *cb);

int aio_write(struct handler *hlr, uint64_t addr_in_byte, uint32_t size,
	      char *buf, struct callback *cb);

#endif
