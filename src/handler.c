#include "handler.h"
#include "kv_ops.h"
#include "hopscotch.h"
//#include "bigkv_index.h"
#include "aio.h"
#include "device.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int global_hlr_number;

bool stopflag_hlr;

struct handler *handler_init(htable_t ht_type) {
	struct handler *hlr = (struct handler *)calloc(sizeof(struct handler),1);

	hlr->number = global_hlr_number++;

	hlr->ops = (struct kv_ops *)calloc(sizeof(struct kv_ops),1);
	switch (ht_type) {
	case HTABLE_HOPSCOTCH:
	case HTABLE_BIGKV:
		hlr->ops->init = hopscotch_init;
		hlr->ops->free = hopscotch_free;
		hlr->ops->get_kv = hopscotch_get;
		hlr->ops->set_kv = hopscotch_set;
		hlr->ops->delete_kv = hopscotch_delete;
		break;
/*		hlr->ops->init = bigkv_index_init;
		hlr->ops->free = bigkv_index_free;
		hlr->ops->get_kv = bigkv_index_get;
		hlr->ops->set_kv = bigkv_index_set;
		hlr->ops->delete_kv = bigkv_index_delete;
		break;  */
	default:
		fprintf(stderr, "Wrong hash-table type!");
	}

	hlr->ops->init(hlr->ops);

	hlr->flying = cl_init(QDEPTH, false);
	//hlr->cond = cl_init(QDEPTH, true);
	q_init(&hlr->req_q, QSIZE);
	q_init(&hlr->retry_q, QSIZE);

	hlr->dev = dev_abs_init("/dev/nvme13n1");

	hlr->read = dev_abs_read;
	hlr->write = dev_abs_write;

	memset(&hlr->aio_ctx, 0, sizeof(io_context_t));
	if (io_setup(QDEPTH*2, &hlr->aio_ctx) < 0) {
		perror("io_setup");
		abort();
	}

	pthread_create(&hlr->hlr_tid, NULL, &request_handler, hlr);
	pthread_create(&hlr->plr_tid, NULL, &device_poller, hlr);

	return hlr;
}

void handler_free(struct handler *hlr) {
	int *temp;
	stopflag_hlr=true;
	while (pthread_tryjoin_np(hlr->hlr_tid, (void **)&temp)) {
		//cl_release(hlr->cond);
	}
	while (pthread_tryjoin_np(hlr->plr_tid, (void **)&temp)) {
		//cl_release(hlr->cond);
	}

	print_kv_ops_stat(&hlr->ops->stat);
	hlr->ops->free(hlr->ops);
	free(hlr->ops);

	cl_free(hlr->flying);

	q_free(hlr->req_q);
	q_free(hlr->retry_q);

	dev_abs_free(hlr->dev);
	free(hlr->dev);

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
	struct kv_ops *ops = hlr->ops;

	char thread_name[128] = {0};
	sprintf(thread_name, "%s[%d]", "request_handler", hlr->number);
	pthread_setname_np(pthread_self(), thread_name);

	printf("handler: %s launched\n", thread_name);

	while (1) {
		if (stopflag_hlr && (hlr->flying->now==0)) return NULL;

		if (!(req=get_next_request(hlr))) {
			continue;
		}

		switch (req->type) {
		case REQ_TYPE_SET:
			rc = ops->set_kv(ops, req);
			break;	
		case REQ_TYPE_GET:
			rc = ops->get_kv(ops, req);
			if (rc) {
				//puts("Not existing key!");
				printf("%lu\n", req->key.hash_low);
				req->end_req(req);
			}
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
