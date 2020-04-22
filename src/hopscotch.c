#include "type.h"
#include "ops.h"
#include "hopscotch.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct hash_stat hstat;

int hopscotch_init(struct hash_ops *hops) {
	printf("===============================\n");
	printf(" uDepot's Hopscotch Hash-table \n");
	printf("===============================\n\n");

	printf("hopscotch_init()...\n\n");

	struct hopscotch *hs = (struct hopscotch *)malloc(sizeof(struct hopscotch));

	// Allocate tables
	hs->table=(struct hash_table *)calloc(NR_TABLE, sizeof(struct hash_table));
	for (int i = 0; i < NR_TABLE; i++) {
		// entry
		hs->table[i].entry = (struct hash_entry *)calloc(NR_ENTRY, sizeof(struct hash_entry));
		for (int j = 0; j < NR_ENTRY; j++) {
			hs->table[i].entry[j].pba = PBA_INVALID;
		}
	}

	hops->_private = (void *)hs;

	return 0;
}

int hopscotch_free(struct hash_ops *hops) {
	struct hopscotch *hs = (struct hopscotch *)hops->_private;

	printf("hopscotch_free()...\n\n");

	// population
	uint64_t population = 0;
	for (int i = 0; i < NR_TABLE; i++) {
		for (int j = 0; j < NR_ENTRY; j++) {
			if (hs->table[i].entry[j].pba != PBA_INVALID) {
				population++;
			}
		}
	}
	printf("%.2f%% populated (%lu/%u)\n", 
		(float)population/(NR_TABLE*NR_ENTRY)*100, population, NR_TABLE*NR_ENTRY);

	// cost
	uint64_t cost_sum = 0;
	for (int i = 0; i < FLASH_READ_MAX; i++) cost_sum += hs->lookup_cost[i];
	for (int i = 0; i < FLASH_READ_MAX; i++) {
		printf("%d,%lu,%.4f\n", i, hs->lookup_cost[i], (float)hs->lookup_cost[i]/cost_sum*100);
	}

	// Free tables
	for (int i = 0; i < NR_TABLE; i++) {
		free(hs->table[i].entry);
	}
	free(hs->table);
	free(hs);

	return 0;
}

static int find_matching_tag
(struct hash_table *ht, uint32_t idx, int offset, uint8_t tag) {
	while (offset < MAX_HOP) {
		int current_idx = (idx + offset) % NR_ENTRY;
		struct hash_entry *entry = &ht->entry[current_idx];

		if (entry->key_fp_tag == tag) return offset;
		++offset;
	}
	return -1;
}

#if 0
static void print_entry(struct hash_entry *entry) {
	printf("off:%lu, tag:0x%lx, size:%lu, pba:%lx\n",
		entry->neigh_off, entry->key_fp_tag, entry->kv_size, entry->pba);
}
#endif

static struct hash_entry *fill_entry
(struct hash_entry *entry, uint8_t offset, uint8_t tag, uint16_t size, uint64_t pba) {
	*entry = (struct hash_entry){offset, tag, size, (pba & PBA_INVALID)};
	//print_entry(entry);
	return entry;
}

static int find_free_entry(struct hash_table *ht, uint32_t idx) {
	int offset = 0;

	// find free entry
	while (offset < MAX_HOP) {
		int current_idx = (idx + offset) % NR_ENTRY;
		struct hash_entry *entry = &ht->entry[current_idx];
		if (entry->pba == PBA_INVALID) return offset;
		++offset;
	}

	// if not, make free entry
	offset = MAX_HOP-1;
	while (offset >= 0) {
		int current_idx = (idx + offset) % NR_ENTRY;
		struct hash_entry *entry = &ht->entry[current_idx];

		int ori_idx = (NR_ENTRY + current_idx - entry->neigh_off) % NR_ENTRY;
		for (int dis_off = entry->neigh_off+1; dis_off < MAX_HOP; dis_off++) {
			int dis_idx = (ori_idx + dis_off) % NR_ENTRY;
			struct hash_entry *dis_entry = &ht->entry[dis_idx];
			if (dis_entry->pba == PBA_INVALID) {
				fill_entry(dis_entry, dis_off, entry->key_fp_tag, 1, entry->pba);
				return offset;
			}
		}
		--offset;
	}

	// error
	puts("\n@@@ insert error point! @@@\n");
	return -1;
}

void *cb_keycmp_insert(void *arg) {
	struct request *req = (struct request *)arg;
	struct hop_params *params = (struct hop_params *)req->params;

	if (req->keylen != ((uint8_t *)req->value)[0]) {
		params->insert_step = HOP_INSERT_KEY_MISMATCH;
	} else {
		if (strncmp(req->key, req->value+1, req->keylen) == 0) {
			params->insert_step = HOP_INSERT_KEY_MATCH;
		} else {
			params->insert_step = HOP_INSERT_KEY_MISMATCH;
		}
	}
	retry_req_to_hlr(req->hlr, req);
	return NULL;
}

// FIXME
uint64_t get_pba(uint16_t kv_size) {
	static uint64_t pba = 0;
	uint64_t ret = pba;
	pba += kv_size;
	return ret;
}

int hopscotch_insert(struct hash_ops *hops, struct request *req) {
	struct hopscotch *hs = (struct hopscotch *)hops->_private;
	struct handler *hlr = req->hlr;
	int offset = 0;

	hash_t h_key = req->hkey;

	uint32_t idx = h_key & ((1 << IDX_BIT)-1);
	uint8_t  tag = (uint8_t)(h_key >> IDX_BIT);
	uint8_t  dir = tag & ((1 << DIR_BIT)-1);

	struct hash_table *ht = &hs->table[dir];
	struct hash_entry *entry = NULL;
	struct callback *cb = NULL;

	if (!req->params) req->params = make_hop_params();
	struct hop_params *params = (struct hop_params *)req->params;

	switch (params->insert_step) {
	case HOP_INSERT_KEY_MATCH:
		goto hop_insert_key_match;
		break;
	case HOP_INSERT_KEY_MISMATCH:
		offset = params->offset + 1;
		goto hop_insert_key_mismatch;
		break;
	default:
		break;
	}

hop_insert_key_mismatch:
	// phase 1: find matching tag
	offset = find_matching_tag(ht, idx, offset, tag);
	if (offset != -1) {
		// read kv-pair to confirm whether it is correct key or not
		entry = &ht->entry[(idx+offset)%NR_ENTRY];
		cb = make_callback(cb_keycmp_insert, req);
		params->offset = offset;
		hlr->read(hlr, entry->pba, entry->kv_size, req->value, cb);
		goto exit;
	}

	// phase 2: find free entry
	offset = find_free_entry(ht, idx);
	if (offset == -1) {
		// error: must resize the table!
		puts("insert error");
		abort();
	}

hop_insert_key_match:
	// phase 3: fill the entry
	entry = fill_entry(&ht->entry[(idx+offset)%NR_ENTRY], offset, tag, 2, get_pba(2));
	cb = make_callback(req->end_req, req);
	hlr->write(hlr, entry->pba, entry->kv_size, req->value, cb);

exit:
	return 0;
}

static void collect_lookup_cost(struct hopscotch *hs, int nr_read) {
	if (nr_read < FLASH_READ_MAX) hs->lookup_cost[nr_read]++;
}

int hopscotch_lookup(struct hash_ops *hops, struct request *req) {
	int rc = 0;

	struct hopscotch *hs = (struct hopscotch *)hops->_private;

	int offset = 0;

	hash_t h_key = req->hkey;

	uint32_t idx = h_key & ((1 << IDX_BIT)-1);
	uint8_t  tag = (uint8_t)(h_key >> IDX_BIT);
	uint8_t  dir = tag & ((1 << DIR_BIT)-1);

	struct hash_table *ht = &hs->table[dir];

	int req_flash_read = 0;

	offset = find_matching_tag(ht, idx, 0, tag);
	while (offset != -1) {
		++req_flash_read;
		usleep(1);

		struct hash_entry *entry = &ht->entry[(idx+offset)%NR_ENTRY];
		if (entry->pba == (h_key & PBA_INVALID)) {
			rc = 0;
			goto exit;
		}
		offset = find_matching_tag(ht, idx, offset+1, tag);
	}

	rc = 1;
exit:
	collect_lookup_cost(hs, req_flash_read);
	return rc;
}

int hopscotch_remove(struct hash_ops *hops, struct request *req) {
	// this would be implemented if necessary
	return 0;
}
