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

#define MAX_EVENTS 1024

#define PORT 5556

int sv_sock, cl_sock;

time_t t_inf, t_idle;
stopwatch *sw_inf;

struct server {
	int fd, epfd;
	int num_dev;
	char device[32][16];
	struct handler *hlr[32];
} server;

static void server_exit(int sig) {
	puts("");
	printf("t_inf: %lu\n", t_inf);
	printf("t_idle: %lu\n", t_idle);
	for (int i = 0; i < server.num_dev; i++) {
		handler_free(server.hlr[i]);
	}
	close(cl_sock);
	close(server.fd);
	exit(1);
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
	connection_init(srv);

	sw_inf = sw_create();

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
	return 0;
}

static int
process_request(struct server *srv, struct epoll_event *event) {
	int rc = 0;
	int client_fd = event->data.fd;
	int len = 0;

	struct net_req n_req;
	struct request *req;

	while (1) {
		len = read_sock(client_fd, &n_req, sizeof(struct net_req));
		if (len == -1) {
			break;
		} else if (len == 0) {
			close(client_fd);
			epoll_ctl(srv->epfd, EPOLL_CTL_DEL, client_fd, NULL);
			printf("Client is disconnected!\n");
			break;
		} else {
			int hlr_idx = (n_req.key[n_req.keylen-1] % srv->num_dev);
			req = make_request_from_netreq(srv->hlr[hlr_idx], &n_req, client_fd);
			rc = forward_req_to_hlr(srv->hlr[hlr_idx], req);
			if (rc) return rc;
		}
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
				rc = accept_new_client(srv);
			} else {
				rc = process_request(srv, &epoll_events[i]);
				if (rc) abort();
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
