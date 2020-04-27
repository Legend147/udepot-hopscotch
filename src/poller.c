#include "handler.h"
#include <errno.h>

#ifdef LINUX_AIO
#include <libaio.h>
#elif SPDK
// TODO: include spdk
#endif

#define NR_EVENTS QDEPTH

extern bool stopflag_hlr;

#ifdef LINUX_AIO
static void *aio_poller(void *input) {
	int ret;
	struct io_event *ev;
	struct iocb *iocb;
	struct callback *cb;

	struct handler *hlr = (struct handler *)input;

	io_context_t ctx = hlr->aio_ctx;
	struct io_event events[NR_EVENTS];
	struct timespec timeout = { 0, 0 };

	while (1) {
		if (stopflag_hlr) return NULL;

		if ((ret = io_getevents(ctx, 0, NR_EVENTS, events, &timeout))) {
			for (int i = 0; i < ret; i++) {
				ev = &events[i];
				iocb = ev->obj;
				cb = (struct callback *)ev->data;

				if (ev->res == EINVAL) {
					fprintf(stderr, "aio: I/O failed\n");
				} else if (ev->res != iocb->u.c.nbytes) {
					fprintf(stderr, "aio: Data size error\n");
				}

				cb->func(cb->arg);
				free(ev->data);
				free(ev->obj);
			}
		}
	}
	return NULL;
}
#elif SPDK
static void *spdk_poller(void *input) {
	// TODO: implement SPDK poller
	return NULL;
}
#endif

void *device_poller(void *input) {
#ifdef LINUX_AIO
	aio_poller(input);
#elif SPDK
	spdk_poller(input);
#endif
	return NULL;
}