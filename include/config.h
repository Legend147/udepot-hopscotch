/*
 * Header for configuration variables
 */

#ifndef __DFHASH_CONFIG_H__
#define __DFHASH_CONFIG_H__

#define K (1024)
#define M (1024*K)
#define G (1024*M)
#define T (1024*G)

#define PAGESIZE 4096
#define AVAIL_MEMORY (1 << 27)

#define SEGMENT_SIZE 2*M
#define GRAIN_UNIT 64

#define SOB GRAIN_UNIT

#define KEY_LEN 16
#define VALUE_LEN 1024

#define VALUE_LEN_MAX VALUE_LEN

#define MEM_ALIGN_UNIT 4*K

#define CDF_TABLE_MAX 100000

#define QSIZE 1024
#define QDEPTH 256

#endif
