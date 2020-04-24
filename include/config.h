/*
 * Header for configuration variables
 */

#ifndef __DFHASH_CONFIG_H__
#define __DFHASH_CONFIG_H__

#define PAGESIZE 4096
#define AVAIL_MEMORY (1 << 27)

#define SOB 512

#define KEY_LEN 16
#define VALUE_LEN 1024

#define VALUE_LEN_MAX VALUE_LEN

#define MEM_ALIGN_UNIT 4096

#define CDF_TABLE_MAX 100000

#define QSIZE 1024
#define QDEPTH 64

#endif
