/*
	swap_bytes: Swap byte order of samples in a buffer.

	copyright 2018 by the mpg123 project
	licensed under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis

	This is C source to include in your code to get the function named
	swap_bytes. It is serves intentional duplication in libmpg123 and
	libsyn123 to avoid introducing a dependency between them. This function
	is too small for that.
*/

/* Other headers are already included! */

/* Optionally use platform-specific byteswap macros. */
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

/* Plain stupid swapping of elements in a byte array. */
/* This is the fallback when there is no native bswap macro. */
#define SWAP(a,b) tmp = p[a]; p[a] = p[b]; p[b] = tmp;

/* Convert samplecount elements of samplesize bytes each in buffer buf. */
static void swap_bytes(void *buf, size_t samplesize, size_t samplecount)
{
	unsigned char *p = buf;
	unsigned char *pend = (unsigned char*)buf+samplesize*samplecount;
	unsigned char tmp;

	if(samplesize < 2)
		return;
	switch(samplesize)
	{
		case 2: /* AB -> BA */
#ifdef HAVE_BYTESWAP_H
		{
			uint16_t* pp = (uint16_t*)p;
			for(; pp<(uint16_t*)pend; ++pp)
				*pp = bswap_16(*pp);
		}
#else
			for(; p<pend; p+=2)
			{
				SWAP(0,1)
			}
#endif
		break;
		case 3: /* ABC -> CBA */
			for(; p<pend; p+=3)
			{
				SWAP(0,2)
			}
		break;
		case 4: /* ABCD -> DCBA */
#ifdef HAVE_BYTESWAP_H
		{
			uint32_t* pp = (uint32_t*)p;
			for(; pp<(uint32_t*)pend; ++pp)
				*pp = bswap_32(*pp);
		}
#else
			for(; p<pend; p+=4)
			{
				SWAP(0,3)
				SWAP(1,2)
			}
#endif
		break;
		case 8: /* ABCDEFGH -> HGFEDCBA */
#ifdef HAVE_BYTESWAP_H
		{
			uint64_t* pp = (uint64_t*)p;
			for(; pp<(uint64_t*)pend; ++pp)
				*pp = bswap_64(*pp);
		}
#else
			for(; p<pend; p+=8)
			{
				SWAP(0,7)
				SWAP(1,6)
				SWAP(2,5)
				SWAP(3,4)
			}
#endif
		break;
		/* All the weird choices with the full nested loop. */
		default:
			for(; p<pend; p+=samplesize)
			{
				size_t j;
				for(j=0; j<samplesize/2; ++j)
				{
					SWAP(j, samplesize-j-1)
				}
			}
	}
}

#undef SWAP
