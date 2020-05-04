#include "util.h"
#include "handler.h"
#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 5556

int sv_sock, cl_sock;
struct handler *hlr;

static void server_exit(int sig) {
	puts("");
	handler_free(hlr);
	close(cl_sock);
	close(sv_sock);
	exit(1);
}

static int sig_add() {
	struct sigaction sa;
	sa.sa_handler = server_exit;
	sigaction(SIGINT, &sa, NULL);
	return 0;
}

static int server_init(htable_t ht_type) {
	hlr = handler_init(ht_type);
	return 0;
}

static int connect_client() {
	struct sockaddr_in sv_addr, cl_addr;
	socklen_t cl_addr_sz;

	sv_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sv_sock == -1) abort();

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr.sin_port = htons(PORT);

	if (bind(sv_sock, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) == -1) {
		abort();
	}
	puts("- Server socket is bound!");

	if (listen(sv_sock, 5) == -1) {
		abort();
	}
	puts("- Wait for client connection...");

	cl_addr_sz = sizeof(cl_addr);
	cl_sock = accept(sv_sock, (struct sockaddr *)&cl_addr, &cl_addr_sz);
	if (cl_sock == -1) abort();
	puts("- Client is connected!");

	return 0;
}

static int handle_request() {
	int rc = 0;

	struct net_req n_req;
	struct request *req;

	while (1) {
		read_sock(cl_sock, &n_req, sizeof(struct net_req));
		req = make_request_from_netreq(&n_req, cl_sock);
		rc = forward_req_to_hlr(hlr, req);
		if (rc) {
			abort();
		}
	}
	return 0;
}

int main() {
	/* Add interrupt signal*/
	sig_add();

	/* Server init */
	server_init(HTABLE_BIGKV);

	/* Connect client */
	connect_client();

	/* Poll request */
	handle_request();
}
