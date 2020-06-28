#ifndef __BIGKV_MASTER_H__
#define __BIGKV_MASTER_H__

#include "type.h"
#include "config.h"
#include "handler.h"

#include <time.h>
#include <pthread.h>

#define MAX_NETREQ_BUF 1024

struct master {
	pthread_t tid;

	int num_dev;
	char **device;

	struct handler *hlr[32];

	int fd;
	int epfd;

	struct netreq netreq_buf[MAX_NETREQ_BUF];
};

struct master *master_init(int num_dev, char **device);
void master_free(struct master *mas);
void *master_thread(void *input);

#endif
