#include "aio.h"
#include <stdlib.h>
#include <stdio.h>

#define SOB 512

int aio_read(struct handler *hlr, uint64_t pba,
	     uint32_t size, char *buf, struct callback *cb) {

	int rc = 0;

	struct iocb *iocb = (struct iocb *)malloc(sizeof(struct iocb));
	io_prep_pread(iocb, hlr->dev_fd, buf, size*SOB, pba*SOB);
	iocb->data = cb;

	if (io_submit(hlr->aio_ctx, 1, &iocb) < 0) {
		fprintf(stderr, "aio: error on submitting I/O");
		rc = -1;
	}

	return rc;
}

int aio_write(struct handler *hlr, uint64_t pba,
	      uint32_t size, char *buf, struct callback *cb) {

	int rc = 0;

	struct iocb *iocb = (struct iocb *)malloc(sizeof(struct iocb));
	io_prep_pwrite(iocb, hlr->dev_fd, buf, size*SOB, pba*SOB);
	iocb->data = cb;

	if (io_submit(hlr->aio_ctx, 1, &iocb) < 0) {
		fprintf(stderr, "aio: error on submitting I/O");
		rc = -1;
	}

	return rc;
}
