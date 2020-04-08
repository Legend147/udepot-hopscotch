#include "util.h"
#include "city.h"
#include <string.h>

#ifdef SHA256
uint64_t hashing_key(char *key, uint8_t len) {
	char* string;
	Sha256Context ctx;
	SHA256_HASH hash;
	uint64_t hashkey;

	string = key;

	Sha256Initialise(&ctx);
	Sha256Update(&ctx, (unsigned char*)string, len);
	Sha256Finalise(&ctx, &hash);

	memcpy(&hashkey, hash.bytes, sizeof(uint64_t));

	return hashkey;
}
#elif CITYHASH
uint64_t hashing_key(char *key, uint8_t len) {
	return CityHash64(key, len);
}
#endif
