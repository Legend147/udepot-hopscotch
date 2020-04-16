#include "type.h"
#include "ops.h"
#include "hopscotch.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct hash_ops hash_ops = {
	.init = hopscotch_init,
	.free = hopscotch_free,
	.insert = hopscotch_insert,
	.lookup = hopscotch_lookup,
	.remove = hopscotch_remove,
};
struct hash_stat hstat;
struct hopscotch hs;

int hopscotch_init() {
	printf("===============================\n");
	printf(" uDepot's Hopscotch Hash-table \n");
	printf("===============================\n\n");

	printf("hopscotch_init()...\n\n");

	// Allocate tables
	hs.table=(struct hash_table *)calloc(NR_TABLE, sizeof(struct hash_table));
	for (int i = 0; i < NR_TABLE; i++) {
		// entry
		hs.table[i].entry = (struct hash_entry *)calloc(NR_ENTRY, sizeof(struct hash_entry));
		for (int j = 0; j < NR_ENTRY; j++) {
			hs.table[i].entry[j].pba = PBA_INVALID;
		}
	}

	hs.temp_buf = (char *)malloc(TEMP_BUF_SIZE);

	return 0;
}

int hopscotch_free() {
	printf("hopscotch_free()...\n\n");

	// population
	uint64_t population = 0;
	for (int i = 0; i < NR_TABLE; i++) {
		for (int j = 0; j < NR_ENTRY; j++) {
			if (hs.table[i].entry[j].pba != PBA_INVALID) {
				population++;
			}
		}
	}
	printf("%.2f%% populated (%lu/%u)\n", 
		(float)population/(NR_TABLE*NR_ENTRY)*100, population, NR_TABLE*NR_ENTRY);

	// cost
	uint64_t cost_sum = 0;
	for (int i = 0; i < FLASH_READ_MAX; i++) cost_sum += hs.lookup_cost[i];
	for (int i = 0; i < FLASH_READ_MAX; i++) {
		printf("%d,%lu,%.4f\n", i, hs.lookup_cost[i], (float)hs.lookup_cost[i]/cost_sum*100);
	}

	// Free tables
	for (int i = 0; i < NR_TABLE; i++) {
		free(hs.table[i].entry);
	}
	free(hs.table);

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

static void fill_entry
(struct hash_entry *entry, uint8_t offset, uint8_t tag, uint16_t size, uint64_t pba) {
	*entry = (struct hash_entry){offset, tag, size, (pba & PBA_INVALID)};
	//print_entry(entry);
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

int hopscotch_insert(uint64_t h_key) {
	int offset = 0;

	uint32_t idx = h_key & ((1 << IDX_BIT)-1);
	uint8_t  tag = (uint8_t)(h_key >> IDX_BIT);
	uint8_t  dir = tag & ((1 << DIR_BIT)-1);

	struct hash_table *ht = &hs.table[dir];

	int req_flash_read = 0;

	// phase 1: find matching tag
	offset = find_matching_tag(ht, idx, 0, tag);
	while (offset != -1) {
		++req_flash_read;

		struct hash_entry *entry = &ht->entry[(idx+offset)%NR_ENTRY];
		if (entry->pba == (h_key & PBA_INVALID)) {
			fill_entry(entry, offset, tag, 1, h_key);
			goto exit;
		}
		offset = find_matching_tag(ht, idx, offset+1, tag);
	}

	// phase 2: find free entry
	offset = find_free_entry(ht, idx);
	if (offset == -1) {
		// error: must resize the table!
		puts("insert error");
		abort();
		goto exit;
	}

	// phase 3: fill the entry
	fill_entry(&ht->entry[(idx+offset)%NR_ENTRY], offset, tag, 1, h_key);

exit:
	return 0;
}

static void collect_lookup_cost(int nr_read) {
	if (nr_read < FLASH_READ_MAX) hs.lookup_cost[nr_read]++;
}

int hopscotch_lookup(uint64_t h_key) {
	int rc = 0;

	int offset = 0;

	uint32_t idx = h_key & ((1 << IDX_BIT)-1);
	uint8_t  tag = (uint8_t)(h_key >> IDX_BIT);
	uint8_t  dir = tag & ((1 << DIR_BIT)-1);

	struct hash_table *ht = &hs.table[dir];

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
	collect_lookup_cost(req_flash_read);
	return rc;
}

int hopscotch_remove(uint64_t h_key) {
	// this would be implemented if necessary
	return 0;
}
