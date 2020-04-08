#include "type.h"
#include "ops.h"
#include "hopscotch.h"
#include "util.h"
#include "stopwatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define KEY_LEN 16

struct net_req {
	char key[KEY_LEN];
	uint8_t type;
};

extern struct hash_ops hash_ops;
stopwatch *sw;
int sv_sock, cl_sock;
char ack;

static void server_exit(int sig) {
	puts("");
	hash_ops.free();
	sw_destroy(sw);
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

static int connect_client() {
	struct sockaddr_in sv_addr, cl_addr;
	socklen_t cl_addr_sz;

	sv_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sv_sock == -1) abort();

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr.sin_port = htons(5555);

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

static ssize_t read_sock(int sock, void *buf, ssize_t count) {
	ssize_t len = 0;
	while (len != count) {
		len += read(sock, (char *)buf + len, count-len);
	}
	return len;
}

static ssize_t send_ack(int sock, long latency) {
	return write(sock, &latency, sizeof(long));
}

static int handle_request() {
	int rc = 0;

	struct net_req net_req;
	uint64_t hkey = 0;

	sw = sw_create();

	while (1) {
		read_sock(cl_sock, &net_req, sizeof(struct net_req));

		sw_start(sw);
		hkey = hashing_key(net_req.key, KEY_LEN);

		switch (net_req.type) {
		case 0:
			rc = hash_ops.insert(hkey);
			//usleep(10);
			break;	
		case 1:
			rc = hash_ops.lookup(hkey);
			usleep(10);
			if (rc) {
				puts("Not existing key!");
				abort();
			}
			break;
		default:
			fprintf(stderr, "Wrong req type!\n");
			break;
		}
		sw_end(sw);

		send_ack(cl_sock, sw_get_usec(sw));
	}
	return 0;
}

int main() {
	/* Add interrupt signal*/
	sig_add();

	/* Table init */
	hash_ops.init();
	
	/* Connect client */
	connect_client();

	/* Poll request */
	handle_request();
}
