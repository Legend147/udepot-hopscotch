#include "handler.h"
#include "ops.h"
#include "hopscotch.h"
#include "aio.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int global_number;

extern int errno;

struct handler *handler_init(htable_t ht_type) {
	struct handler *hlr = (struct handler *)malloc(sizeof(struct handler));

	hlr->number = global_number++;

	hlr->hops = (struct hash_ops *)malloc(sizeof(struct hash_ops));
	switch (ht_type) {
	case HTABLE_HOPSCOTCH:
		hlr->hops->init = hopscotch_init;
		hlr->hops->free = hopscotch_free;
		hlr->hops->insert = hopscotch_insert;
		hlr->hops->lookup = hopscotch_lookup;
		hlr->hops->remove = hopscotch_remove;
		break;
	case HTABLE_BIGKV:
	default:
		fprintf(stderr, "Wrong hash-table type!");
	}

	hlr->hops->init(hlr->hops);

	hlr->flying = cl_init(QDEPTH, false);
	//hlr->cond = cl_init(QDEPTH, true);
	q_init(&hlr->req_q, QSIZE);
	q_init(&hlr->retry_q, QSIZE);

	hlr->dev_fd = open("/dev/nvme0n1", O_RDWR | O_CREAT | O_DIRECT, 0666);
	if (hlr->dev_fd < 0) {
		perror("device open");
		abort();
	}

	hlr->read = aio_read;
	hlr->write = aio_write;

	int ret = 0;
	ret = io_setup(1, &hlr->aio_ctx);
	printf("%d\n", ret);
	if (ret != 0) {
		fprintf(stderr, "%d\n", errno);
		perror("io_submit");
		fprintf(stderr, "%s\n", strerror(errno));
		abort();
	}

	pthread_create(&hlr->hlr_tid, NULL, &request_handler, hlr);
	pthread_create(&hlr->plr_tid, NULL, &device_poller, hlr);

	return hlr;
}

void handler_free(struct handler *hlr) {
	int *temp;
	while (pthread_tryjoin_np(hlr->hlr_tid, (void **)&temp)) {
		//cl_release(hlr->cond);
	}
	while (pthread_tryjoin_np(hlr->plr_tid, (void **)&temp)) {
		//cl_release(hlr->cond);
	}
	hlr->hops->free(hlr->hops);
	q_free(hlr->req_q);
	q_free(hlr->retry_q);

	close(hlr->dev_fd);
	io_destroy(hlr->aio_ctx);

	free(hlr);
} 

int forward_req_to_hlr(struct handler *hlr, struct request *req) {
	int rc = 0;

	cl_grap(hlr->flying);
	req->hlr = hlr;
	if (!q_enqueue((void *)req, hlr->req_q)) {
		rc = 1;
	}
	return rc;

}

int retry_req_to_hlr(struct handler *hlr, struct request *req) {
	int rc = 0;
	if (!q_enqueue((void *)req, hlr->retry_q)) {
		rc = 1;
	}
	return rc;
}

struct request *get_next_request(struct handler *hlr) {
	void *req = NULL;
	if ((req = q_dequeue(hlr->retry_q))) goto exit;
	else if ((req = q_dequeue(hlr->req_q))) goto exit;
exit:
	return (struct request *)req;
}

void *request_handler(void *input) {
	int rc = 0;

	struct request *req = NULL;
	struct handler *hlr = (struct handler *)input;
	struct hash_ops *hops = hlr->hops;

	char thread_name[128] = {0};
	sprintf(thread_name, "%s[%d]", "request_handler", hlr->number);
	pthread_setname_np(pthread_self(), thread_name);

	printf("%s launched\n", thread_name);

	while (1) {
		//cl_grap(hlr->cond);
		if (!(req=get_next_request(hlr))) {
			//cl_release(hlr->cond);
			continue;
		}

		switch (req->type) {
		case REQ_TYPE_SET:
			rc = hops->insert(hops, req);
			req->end_req(req);
			break;	
		case REQ_TYPE_GET:
			rc = hops->lookup(hops, req);
			if (rc) {
				puts("Not existing key!");
				abort();
			}
			req->end_req(req);
			break;
		case REQ_TYPE_DELETE:
		case REQ_TYPE_RANGE:
		case REQ_TYPE_ITERATOR:
		default:
			fprintf(stderr, "Wrong req type!\n");
			break;
		}
	}
	return NULL;
}
