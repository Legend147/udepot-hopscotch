#include "keygen.h"
#include "util.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <pthread.h>

#define IP "127.0.0.1"
#define PORT 5555

#define NR_KEY   100000
#define NR_QUERY 100000
#define KEY_LEN  16

#define SRV_QDEPTH 1

struct keygen *kg;
int sock;
sem_t sem;
pthread_t tid;
uint32_t seq_num_global;

uint64_t cdf_table[CDF_TABLE_MAX];

void *ack_poller(void *arg) {
	struct net_ack net_ack;

	puts("Bench :: ack_poller() created");

	while (1) {
		recv_ack(sock, &net_ack);
		req_out(&sem);

		if (net_ack.type == REQ_TYPE_GET) {
			collect_latency(cdf_table, net_ack.elapsed_time);
		}
	}
}

static int bench_init() {
	kg = keygen_init(NR_KEY, KEY_LEN);

	sem_init(&sem, 0, SRV_QDEPTH);

	pthread_create(&tid, NULL, &ack_poller, NULL);

	return 0;
}

static int connect_server() {
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) abort();

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(IP);
	addr.sin_port = htons(PORT);

	puts("- Wait for server connection...");

	if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		abort();
	}
	puts("- Server is connected!");

	return 0;
}

static int load_kvpairs() {
	struct net_req net_req;

	net_req.keylen = KEY_LEN;
	net_req.type = REQ_TYPE_SET;
	for (size_t i = 0; i < NR_KEY; i++) {
		req_in(&sem);
		if (i%(NR_KEY/100)==0) {
			printf("\rProgress [%3.0f%%] (%lu/%d)",
				(float)i/NR_KEY*100, i, NR_KEY);
			fflush(stdout);
		}
		memcpy(net_req.key, get_next_key_for_load(kg), KEY_LEN);
		net_req.seq_num = seq_num_global++;
		send_request(sock, &net_req);
	}
	wait_until_finish(&sem, SRV_QDEPTH);

	puts("\nLoad finished!");

	return 0;
}

static int run_bench(key_dist_t dist, int query_ratio, int hotset_ratio) {
	struct net_req net_req;

	net_req.keylen = KEY_LEN;
	net_req.type = REQ_TYPE_GET;

	set_key_dist(kg, dist, query_ratio, hotset_ratio);
	for (size_t i = 0; i < NR_QUERY; i++) {
		req_in(&sem);
		if (i%(NR_QUERY/100)==0) {
			printf("\rProgress [%3.0f%%] (%lu/%d)",
				(float)i/NR_QUERY*100,i,NR_QUERY);
			fflush(stdout);
		}
		memcpy(net_req.key, get_next_key(kg), KEY_LEN);
		send_request(sock, &net_req);
	}
	wait_until_finish(&sem, SRV_QDEPTH);

	puts("\nBenchmark finished!");
	print_cdf(cdf_table, NR_QUERY);

	return 0;
}

static int bench_free() {
	keygen_free(kg);
	close(sock);
	return 0;
}

int main(int argc, char *argv[]) {
	bench_init();

	/* Connect to server */
	connect_server();

	/* Load phase */
	load_kvpairs();

	/* Benchmark phase */
	run_bench(KEY_DIST_UNIFORM, 50, 50);
	run_bench(KEY_DIST_UNIFORM, 50, 50);
	run_bench(KEY_DIST_LOCALITY, 60, 40);
	run_bench(KEY_DIST_LOCALITY, 70, 30);
	run_bench(KEY_DIST_LOCALITY, 80, 20);
	run_bench(KEY_DIST_LOCALITY, 90, 10);

	bench_free();
	return 0;
}
