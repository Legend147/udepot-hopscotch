#include "util.h"
#include "city.h"
#include "config.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

uint64_t hashing_key(char *key, uint8_t len) {
	return CityHash64(key, len);
}

uint128 hashing_key_128(char *key, uint8_t len) {
	return CityHash128(key,len);
}

ssize_t read_sock(int sock, void *buf, ssize_t count) {
	ssize_t readed = 0, len;
	while (readed < count) {
		len = read(sock, &(((char *)buf)[readed]), count-readed);
		if (len == -1) continue;
		readed += len;
	}
	return readed;
}

ssize_t send_request(int sock, struct net_req *nr) {
	return write(sock, nr, sizeof(struct net_req));
}

ssize_t recv_request(int sock, struct net_req *nr) {
	return read_sock(sock, nr, sizeof(struct net_req));
}

ssize_t send_ack(int sock, struct net_ack *na) {
	return write(sock, na, sizeof(struct net_ack));
}

ssize_t recv_ack(int sock, struct net_ack *na) {
	return read_sock(sock, na, sizeof(struct net_ack));
}

void collect_latency(uint64_t table[], time_t latency) {
	if (latency != -1) table[latency/10]++;
}

void print_cdf(uint64_t table[], uint64_t nr_query) {
	uint64_t cdf_sum = 0;
	printf("#latency,cnt,cdf\n");
	for (int i = 0; i < CDF_TABLE_MAX && cdf_sum != nr_query; i++) {
		if (table[i] != 0) {
			cdf_sum += table[i];
			printf("%d,%lu,%.6f\n",
				i*10, table[i], (float)cdf_sum/nr_query);
			table[i] = 0;
		}
	}
}

int req_in(sem_t *sem_lock) {
	return sem_wait(sem_lock);
}

int req_out(sem_t *sem_lock) {
	return sem_post(sem_lock);
}

void wait_until_finish(sem_t *sem_lock, int qdepth) {
	int nr_finished_reqs = 0;
	while (nr_finished_reqs != qdepth) {
		sleep(1);
		sem_getvalue(sem_lock, &nr_finished_reqs);
	}
}
