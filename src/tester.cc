#include "type.h"
#include "ops.h"
#include "hopscotch.h"
#include "util.h"
#include "keygen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#define NR_QUERY 500000
#define NR_HOTSET (NR_QUERY/100)
#define NR_COLDSET (NR_QUERY-NR_HOTSET)

extern struct hash_ops hash_ops;

#ifdef KEYGEN
struct keygen *kg;
#elif TRACE
char parsed_key[256];
int key_len;
#endif

int main(int argc, char *argv[]) {
	hash_ops.init();

#ifdef KEYGEN
	puts("Generating keys...");
	kg = keygen_init(NR_QUERY, 16, KEY_DIST_UNIFORM);
#elif TRACE
	if (argc < 3) {
		puts("input load/run trace file");
		abort();
	}
	ios::sync_with_stdio(false);
	ifstream fp_insert (argv[1]);
	ifstream fp_warmup (argv[2]);
	ifstream fp_lookup (argv[2]);
#endif

	puts("======== tester start! ========");
	/* Insert */
	puts("Insert");
	for (int i = 0; i < NR_QUERY; i++) {
		if (i%(NR_QUERY/10)==0) {
			printf("Progress [%2.0f%%] (%d/%d)\n", (float)i/NR_QUERY*100, i, NR_QUERY);
		}
#ifdef KEYGEN
		uint64_t hkey = hashing_key(kg->key_pool[i], 16);
#elif TRACE
		fp_insert >> key_len >> parsed_key;
		uint64_t hkey = hashing_key(parsed_key, key_len);
#endif
		hash_ops.insert(hkey);
	}

#ifdef TRACE
	puts("Warm up...");
	for (int i = 0; i < NR_QUERY/10; i++) {
		fp_lookup >> key_len >> parsed_key;
		uint64_t hkey = hashing_key(parsed_key, key_len);
		int rc = hash_ops.lookup(hkey);
		if (rc) {
			puts("Not existing key!");
			abort();
		}
	}
#endif

	/* Lookup */
	puts("Lookup");
	for (int i = 0; i < NR_QUERY; i++) {
		if (i%(NR_QUERY/100)==0) {
			printf("Progress [%2.0f%%] (%d/%d)\n", (float)i/NR_QUERY*100, i, NR_QUERY);
		}
#ifdef KEYGEN
#ifdef UNIFORM
		uint64_t hkey = hashing_key(get_next_key(kg), kg->key_size);
#elif HOTSPOT
		uint64_t hkey = 0;
		if (rand()%100 == 0) { // cold
			hkey = hashing_key(key[NR_HOTSET+(rand()%NR_COLDSET)], 16);
		} else { // hot
			hkey = hashing_key(key[rand()%NR_HOTSET], 16);
		}
#endif
#elif TRACE
		fp_lookup >> key_len >> parsed_key;
		uint64_t hkey = hashing_key(parsed_key, key_len);
#endif
		int rc = hash_ops.lookup(hkey);
		if (rc) {
			puts("Not existing key!");
			abort();
		}
	}
	puts("======== tester completed! ========");

	hash_ops.free();
	return 0;
}
