#ifndef ETHERBOOT_BYTESWAP_H
#define ETHERBOOT_BYTESWAP_H

#include "endian.h"
#include "bits/byteswap.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#include "little_bswap.h"
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#include "big_bswap.h"
#endif

/* Make routines available to all */
#define swap32(x)	__bswap_32(x)
#define swap16(x)	__bswap_16(x)
#define bswap_32(x)	__bswap_32(x)
#define bswap_16(x)	__bswap_16(x)
	
#endif /* ETHERBOOT_BYTESWAP_H */
