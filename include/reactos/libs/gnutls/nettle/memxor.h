/* memxor.h
 *
 */

#ifndef NETTLE_MEMXOR_H_INCLUDED
#define NETTLE_MEMXOR_H_INCLUDED

#include <stdlib.h>
#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

	uint8_t *memxor(uint8_t * dst, const uint8_t * src, size_t n);
	uint8_t *memxor3(uint8_t * dst, const uint8_t * a,
			 const uint8_t * b, size_t n);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_MEMXOR_H_INCLUDED */
