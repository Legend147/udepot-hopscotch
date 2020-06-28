#include "util.h"
#include "handler.h"
#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>

#define MAX_EVENTS 1024

int sv_sock, cl_sock;

stopwatch sw_inf, sw_sockread, sw_makereq, sw_forward, sw_queueing, sw_ack, sw_hashing;

extern int num_syscalls;

bool ack_sender_stop;

// FIXME
uint64_t hlr_fwd_cnt[32];

struct server {
	int fd, epfd;
	int num_dev;
	char device[32][16];
	struct handler *hlr[32];
	pthread_t tid_ack_sender;
	int client_fd;
} server;

queue *ack_q;

static void server_exit(int sig) {
	puts("\n");
	for (int i = 0; i < server.num_dev; i++) {
		printf("hlr_fwd_cnt[%2d]: %lu\n", i, hlr_fwd_cnt[i]);
	}
	puts("\n");
	printf("t_inf:       %lu\n", sw_get_lap_sum(&sw_inf));
	printf("`t_sockread: %lu\n", sw_get_lap_sum(&sw_sockread));
	printf("`t_queueing: %lu\n", sw_get_lap_sum(&sw_queueing));
	printf("``t_hashing:   %lu\n", sw_get_lap_sum(&sw_hashing));
	printf("``t_makereq:   %lu\n", sw_get_lap_sum(&sw_makereq));
	printf("``t_forward:   %lu\n", sw_get_lap_sum(&sw_forward));
	printf("\nt_ack:       %lu\n", sw_get_lap_sum(&sw_ack));
	for (int i = 0; i < server.num_dev; i++) {
		handler_free(server.hlr[i]);
	}

	ack_sender_stop = true;
	int *temp;
	while (pthread_tryjoin_np(server.tid_ack_sender, (void **)&temp)) {}

	close(cl_sock);
	close(server.fd);
	exit(1);
}

static void *ack_sender(void *input) {
	struct server *srv = (struct server *)input;
	q_init(&ack_q, QSIZE);

	struct net_ack *ack;

	while (1) {
		if (ack_sender_stop) break;

		ack = (struct net_ack *)q_dequeue(ack_q);
		if (!ack) continue;
		sw_start(&sw_ack);
#ifdef YCSB
		send_ack(srv->client_fd, ack);
#else
		send_ack(srv->client_fd, ack);
#endif
		free(ack);
		sw_lap(&sw_ack);
	}
	return NULL;
}

static int sig_add() {
	struct sigaction sa;
	sa.sa_handler = server_exit;
	sigaction(SIGINT, &sa, NULL);
	return 0;
}

static int connection_init(struct server *srv) {
	struct sockaddr_in sv_addr;

	srv->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (srv->fd == -1) {
		perror("socket()");
		abort();
	}

	// setting socket to non-blocking
	int flags = fcntl(srv->fd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(srv->fd, F_SETFL, flags) < 0) {
		perror("fcntl()");
		abort();
	}

	// setting socket to enable reusing address
	int option = true;
	if (setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
		perror("setsockopt()");
		abort();
	}

	memset(&sv_addr, 0, sizeof(sv_addr));
	sv_addr.sin_family = AF_INET;
	sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sv_addr.sin_port = htons(PORT);

	if (bind(srv->fd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) == -1) {
		perror("bind()");
		abort();
	}
	puts("- Server socket is bound!");

	if (listen(srv->fd, 5) == -1) {
		abort();
	}

	srv->epfd = epoll_create(1024);
	if (srv->epfd < 0) {
		perror("epoll_create()");
		abort();
	}

	struct epoll_event events;
	events.events = EPOLLIN | EPOLLET;
	events.data.fd = srv->fd;

	if (epoll_ctl(srv->epfd, EPOLL_CTL_ADD, srv->fd, &events) < 0) {
		perror("epoll_ctl()");
		abort();
	}

	return 0;
}

static int server_init(struct server *srv) {
	pthread_t tid = pthread_self();

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	int s = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
	if (s) abort();

	pthread_create(&srv->tid_ack_sender, NULL, &ack_sender, (void *)srv);
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	s = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
	if (s) abort();

	connection_init(srv);

	for (int i = 0; i < srv->num_dev; i++) {
		srv->hlr[i] = handler_init(srv->device[i]);
	}
	return 0;
}

static int accept_new_client(struct server *srv) {
	int client_fd;
	int client_len;
	struct sockaddr_in client_addr;

	client_len = sizeof(client_addr);
	client_fd = accept(srv->fd, (struct sockaddr *)&client_addr,
			   (socklen_t *)&client_len);
	if (client_fd < 0) {
		perror("accept()");
		return -1;
	}

	int flags = fcntl(client_fd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(client_fd, F_SETFL, flags) < 0) {
		perror("fcntl()");
		close(client_fd);
		return -1;
	}

	struct epoll_event events;
	events.events = EPOLLIN | EPOLLET;
	events.data.fd = client_fd;

	if (epoll_ctl(srv->epfd, EPOLL_CTL_ADD, client_fd, &events) < 0) {
		perror("epoll_ctl()");
		close(client_fd);
		return -1;
	}

	printf("New client is connected!\n");

	// FIXME
	srv->client_fd = client_fd;
	// FIXME

	return client_fd;
}

struct net_req n_req[QDEPTH];

static int
process_request(struct server *srv, int client_fd) {
	int rc = 0;
	int len = 0;
	int n_obj = 0;

	hash_t hash_high = 0;
	int hlr_idx = 0;

	struct request *req;

	sw_start(&sw_sockread);
	len = read_sock_bulk(client_fd, n_req, QDEPTH, sizeof(struct net_req));
	sw_lap(&sw_sockread);

	n_obj = len / sizeof(struct net_req);

	if (len == -1) {
		return rc;
	} else if (len == 0) {
		close(client_fd);
		epoll_ctl(srv->epfd, EPOLL_CTL_DEL, client_fd, NULL);
		printf("Client is disconnected!\n");
		rc = 1;
		return rc;
	} else {
		sw_start(&sw_queueing);
		for (int i = 0; i < n_obj; i++) {
#ifdef YCSB
			n_req[i].kv_size = 1024;
#endif
			sw_start(&sw_hashing);
			hash_high = hashing_key_128(n_req[i].key, n_req[i].keylen).first;
			hlr_idx = (hash_high >> 32) % srv->num_dev;
			sw_lap(&sw_hashing);

			// FIXME
			hlr_fwd_cnt[hlr_idx]++;

			sw_start(&sw_makereq);
			req = make_request_from_netreq(srv->hlr[hlr_idx],
						       &n_req[i], client_fd);
			sw_lap(&sw_makereq);

			sw_start(&sw_forward);
			rc = forward_req_to_hlr(srv->hlr[hlr_idx], req);
			sw_lap(&sw_forward);
			if (rc) return rc;
		}
		sw_lap(&sw_queueing);
	}
	return rc;
}

static int
handle_request(struct server *srv) {
	int rc = 0;

	struct epoll_event epoll_events[MAX_EVENTS];
	int event_count;

	while (1) {
		event_count = epoll_wait(srv->epfd, epoll_events, MAX_EVENTS, 0);

		if (event_count < 0) {
			perror("epoll_wait()");
			abort();
		} else if (event_count == 0) {
			continue;
		}

		for (int i = 0; i < event_count; i++) {
			if (epoll_events[i].data.fd == srv->fd) {
				accept_new_client(srv);
			} else {
				sw_start(&sw_inf);
				rc = process_request(srv, epoll_events[i].data.fd);
				if (rc < 0) abort();
				sw_lap(&sw_inf);
			}
		}
	}
	return 0;
}

static int 
server_getopt(int argc, char *argv[], struct server *const srv) {
	int c;
	bool dev_flag = false;

	while (1) {
		static struct option long_options[] = {
			{"devices", required_argument, 0, 'd'},
			{0, 0, 0, 0}
		};

		int option_index = 0;

		c = getopt_long(argc, argv, "d:", long_options, &option_index);

		if (c == -1) break;

		switch (c) {
		case 'd':
			dev_flag = true;
			srv->num_dev = atoi(optarg);
			for (int i = 0; i < srv->num_dev; i++) {
				strncpy(srv->device[i], argv[optind+i],
					strlen(argv[optind+i]));
			}
			break;
		case '?':
			// Error
			break;
		default:
			abort();
		}
	}

	if (!dev_flag) {
		fprintf(stderr, "(%s:%d) No devices specified!\n", 
			__FILE__, __LINE__);
		exit(1);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	/* Add interrupt signal*/
	sig_add();

	server_getopt(argc, argv, &server);

	/* Server init */
	server_init(&server);

	/* Poll request */
	handle_request(&server);
}
