#ifndef __SEGMENT_H__
#define __SEGMENT_H__

#include "config.h"
#include "container.h"
#include <stdint.h>

struct dev_abs *dev_abs_init(const char dev_name[]);
int dev_abs_free(struct dev_abs *dev);

int dev_abs_read(struct handler *hlr, uint64_t pba, uint32_t size, char *buf, struct callback *cb);
int dev_abs_write(struct handler *hlr, uint64_t pba, uint32_t size, char *buf, struct callback *cb);

uint64_t get_next_pba(struct handler *hlr, uint32_t size);
void *get_next_segment(struct dev_abs *dev);

#endif
