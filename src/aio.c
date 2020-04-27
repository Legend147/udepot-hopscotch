#include "aio.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>

int
aio_read(struct handler *hlr, uint64_t addr_in_byte, uint32_t size_in_byte,
	 char *buf, struct callback *cb) {

	int rc = 0;

	struct iocb *iocb = (struct iocb *)malloc(sizeof(struct iocb));
	io_prep_pread(iocb, hlr->dev->dev_fd, buf, size_in_byte, addr_in_byte);
	iocb->data = cb;

	if (io_submit(hlr->aio_ctx, 1, &iocb) < 0) {
		fprintf(stderr, "aio: error on submitting I/O");
		rc = -1;
	}

	return rc;
}

int
aio_write(struct handler *hlr, uint64_t addr_in_byte, uint32_t size_in_byte,
	  char *buf, struct callback *cb) {

	int rc = 0;

	struct iocb *iocb = (struct iocb *)malloc(sizeof(struct iocb));
	io_prep_pwrite(iocb, hlr->dev->dev_fd, buf, size_in_byte, addr_in_byte);
	iocb->data = cb;

	if (io_submit(hlr->aio_ctx, 1, &iocb) < 0) {
		fprintf(stderr, "aio: error on submitting I/O");
		rc = -1;
	}

	return rc;
}