/*
 * Copyright (c) 1987, 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)endian.h	7.8 (Berkeley) 4/3/91
 */

#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_ 1

/*
 * Define the order of 32-bit words in 64-bit words.
 */
#define	_QUAD_HIGHWORD 1
#define	_QUAD_LOWWORD 0

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#define	LITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	BIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long */

#define	BYTE_ORDER	LITTLE_ENDIAN

#ifndef KERNEL
#include <sys/cdefs.h>
#endif

#ifdef _MSC_VER
static inline unsigned long __byte_swap_long ( unsigned long i )
{
	char dst[4];
	char* src = (char*)&i;
	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];
	return *(unsigned long*)&dst[0];
}

static inline unsigned short __byte_swap_word ( unsigned short i )
{
	char dst[2];
	char* src = (char*)&i;
	dst[0] = src[1];
	dst[1] = src[0];
	return *(unsigned short*)&dst[0];
}
#else/*_MSC_VER*/
#define __word_swap_long(x) \
({ register u_long __X = (x); \
   __asm ("rorl $16, %1" \
	: "=r" (__X) \
	: "0" (__X)); \
   __X; })
#if __GNUC__ >= 2
#define __byte_swap_long(x) \
__extension__ ({ register u_long __X = (x); \
   __asm ("xchgb %h1, %b1\n\trorl $16, %1\n\txchgb %h1, %b1" \
	: "=q" (__X) \
	: "0" (__X)); \
   __X; })
#define __byte_swap_word(x) \
__extension__ ({ register u_short __X = (x); \
   __asm ("xchgb %h1, %b1" \
	: "=q" (__X) \
	: "0" (__X)); \
   __X; })
#else /* __GNUC__ >= 2 */
#define __byte_swap_long(x) \
({ register u_long __X = (x); \
   __asm ("rorw $8, %w1\n\trorl $16, %1\n\trorw $8, %w1" \
	: "=r" (__X) \
	: "0" (__X)); \
   __X; })
#define __byte_swap_word(x) \
({ register u_short __X = (x); \
   __asm ("rorw $8, %w1" \
	: "=r" (__X) \
	: "0" (__X)); \
   __X; })
#endif /* __GNUC__ >= 2 */
#endif /* _MSC_VER */

/*
 * Macros for network/external number representation conversion.
 */
#if BYTE_ORDER == BIG_ENDIAN && !defined(lint)
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)

#define	NTOHL(x)	(x)
#define	NTOHS(x)	(x)
#define	HTONL(x)	(x)
#define	HTONS(x)	(x)

#else

#define	ntohl	__byte_swap_long
#define	ntohs	__byte_swap_word
#define	htonl	__byte_swap_long
#define	htons	__byte_swap_word

#define	NTOHL(x)	(x) = ntohl((u_long)x)
#define	NTOHS(x)	(x) = ntohs((u_short)x)
#define	HTONL(x)	(x) = htonl((u_long)x)
#define	HTONS(x)	(x) = htons((u_short)x)
#endif
#endif /* _MACHINE_ENDIAN_H_ */
