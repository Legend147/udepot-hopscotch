#include "keygen.h"
#include "stopwatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define IP "127.0.0.1"
#define PORT 5555

#define NR_KEY   50000000
#define NR_QUERY 1000000
#define KEY_LEN  16

#define CDF_TABLE_MAX 100000

struct net_req {
	char key[KEY_LEN];
	uint8_t type; // 0: set, 1: get
};

struct keygen *kg;
int sock;
stopwatch *sw;

uint64_t cdf_table[100000];

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

static ssize_t send_request(struct net_req *nr) {
	return write(sock, nr, sizeof(struct net_req));
}

static ssize_t wait_for_ack(time_t *latency) {
	return read(sock, latency, sizeof(time_t));
}

static int load_kvpairs() {
	struct net_req net_req;
	time_t latency;

	net_req.type = 0;
	for (size_t i = 0; i < NR_KEY; i++) {
		if (i%(NR_KEY/100)==0) {
			printf("\rProgress [%3.0f%%] (%lu/%d)",
				(float)i/NR_KEY*100, i, NR_KEY);
			fflush(stdout);
		}
		memcpy(net_req.key, get_next_key_for_load(kg), KEY_LEN);
		send_request(&net_req);
		wait_for_ack(&latency);
	}
	puts("\nLoad finished!");
	return 0;
}

static void collect_cdf(time_t latency) {
	if (latency != -1) {
		cdf_table[latency/10]++;
	}
}

static int run_bench(key_dist_t dist, int query_ratio, int hotset_ratio) {
	struct net_req net_req;
	time_t latency;

	net_req.type = 1;

	sw = sw_create();

	set_key_dist(kg, dist, query_ratio, hotset_ratio);
	for (size_t i = 0; i < NR_QUERY; i++) {
		if (i%(NR_QUERY/100)==0) {
			printf("\rProgress [%3.0f%%] (%lu/%d)",
				(float)i/NR_QUERY*100,i,NR_QUERY);
			fflush(stdout);
		}
		memcpy(net_req.key, get_next_key(kg), KEY_LEN);
		send_request(&net_req);
		wait_for_ack(&latency);

		collect_cdf(latency);
	}
	puts("\nBenchmark finished!");
	uint64_t cdf_sum = 0;
	printf("#latency,cnt,cdf\n");
	for (int i = 1; i < CDF_TABLE_MAX && cdf_sum != NR_QUERY; i++) {
		if (cdf_table[i] != 0) {
			cdf_sum += cdf_table[i];
			printf("%d,%lu,%.6f\n",
				i*10, cdf_table[i], (float)cdf_sum/NR_QUERY);
			cdf_table[i] = 0;
		}
	}
	sw_destroy(sw);
	return 0;
}

static int bench_free() {
/*	uint64_t cdf_sum = 0;
	printf("#latency,cnt,cdf\n");
	for (int i = 1; i < CDF_TABLE_MAX && cdf_sum != NR_QUERY; i++) {
		if (cdf_table[i] != 0) {
			cdf_sum += cdf_table[i];
			printf("%d,%lu,%.6f\n",
				i*10, cdf_table[i], (float)cdf_sum/NR_QUERY);
		}
	} */
	keygen_free(kg);
	close(sock);
	return 0;
}

int main(int argc, char *argv[]) {
	/* Key generation */
	kg = keygen_init(NR_KEY, KEY_LEN);

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
