/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: lwbuffer.c,v 1.15 2007/06/19 23:47:22 tbox Exp $ */

/*! \file */

/**
 *    These functions provide bounds checked access to a region of memory
 *    where data is being read or written. They are based on, and similar
 *    to, the isc_buffer_ functions in the ISC library.
 * 
 *    A buffer is a region of memory, together with a set of related
 *    subregions. The used region and the available region are disjoint, and
 *    their union is the buffer's region. The used region extends from the
 *    beginning of the buffer region to the last used byte. The available
 *    region extends from one byte greater than the last used byte to the
 *    end of the buffer's region. The size of the used region can be changed
 *    using various buffer commands. Initially, the used region is empty.
 * 
 *    The used region is further subdivided into two disjoint regions: the
 *    consumed region and the remaining region. The union of these two
 *    regions is the used region. The consumed region extends from the
 *    beginning of the used region to the byte before the current offset (if
 *    any). The remaining region the current pointer to the end of the used
 *    region. The size of the consumed region can be changed using various
 *    buffer commands. Initially, the consumed region is empty.
 * 
 *    The active region is an (optional) subregion of the remaining region.
 *    It extends from the current offset to an offset in the remaining
 *    region. Initially, the active region is empty. If the current offset
 *    advances beyond the chosen offset, the active region will also be
 *    empty.
 * 
 * 
 * \verbatim
 *    /------------entire length---------------\\
 *    /----- used region -----\\/-- available --\\
 *    +----------------------------------------+
 *    | consumed  | remaining |                |
 *    +----------------------------------------+
 *    a           b     c     d                e
 * 
 *   a == base of buffer.
 *   b == current pointer.  Can be anywhere between a and d.
 *   c == active pointer.  Meaningful between b and d.
 *   d == used pointer.
 *   e == length of buffer.
 * 
 *   a-e == entire length of buffer.
 *   a-d == used region.
 *   a-b == consumed region.
 *   b-d == remaining region.
 *   b-c == optional active region.
 * \endverbatim
 * 
 *    lwres_buffer_init() initializes the lwres_buffer_t *b and assocates it
 *    with the memory region of size length bytes starting at location base.
 * 
 *    lwres_buffer_invalidate() marks the buffer *b as invalid. Invalidating
 *    a buffer after use is not required, but makes it possible to catch its
 *    possible accidental use.
 * 
 *    The functions lwres_buffer_add() and lwres_buffer_subtract()
 *    respectively increase and decrease the used space in buffer *b by n
 *    bytes. lwres_buffer_add() checks for buffer overflow and
 *    lwres_buffer_subtract() checks for underflow. These functions do not
 *    allocate or deallocate memory. They just change the value of used.
 * 
 *    A buffer is re-initialised by lwres_buffer_clear(). The function sets
 *    used , current and active to zero.
 * 
 *    lwres_buffer_first() makes the consumed region of buffer *p empty by
 *    setting current to zero (the start of the buffer).
 * 
 *    lwres_buffer_forward() increases the consumed region of buffer *b by n
 *    bytes, checking for overflow. Similarly, lwres_buffer_back() decreases
 *    buffer b's consumed region by n bytes and checks for underflow.
 * 
 *    lwres_buffer_getuint8() reads an unsigned 8-bit integer from *b and
 *    returns it. lwres_buffer_putuint8() writes the unsigned 8-bit integer
 *    val to buffer *b.
 * 
 *    lwres_buffer_getuint16() and lwres_buffer_getuint32() are identical to
 *    lwres_buffer_putuint8() except that they respectively read an unsigned
 *    16-bit or 32-bit integer in network byte order from b. Similarly,
 *    lwres_buffer_putuint16() and lwres_buffer_putuint32() writes the
 *    unsigned 16-bit or 32-bit integer val to buffer b, in network byte
 *    order.
 * 
 *    Arbitrary amounts of data are read or written from a lightweight
 *    resolver buffer with lwres_buffer_getmem() and lwres_buffer_putmem()
 *    respectively. lwres_buffer_putmem() copies length bytes of memory at
 *    base to b. Conversely, lwres_buffer_getmem() copies length bytes of
 *    memory from b to base.
 */

#include <config.h>

#include <string.h>

#include <lwres/lwbuffer.h>

#include "assert_p.h"

void
lwres_buffer_init(lwres_buffer_t *b, void *base, unsigned int length)
{
	/*
	 * Make 'b' refer to the 'length'-byte region starting at base.
	 */

	REQUIRE(b != NULL);

	b->magic = LWRES_BUFFER_MAGIC;
	b->base = base;
	b->length = length;
	b->used = 0;
	b->current = 0;
	b->active = 0;
}

/*  Make 'b' an invalid buffer. */
void
lwres_buffer_invalidate(lwres_buffer_t *b)
{

	REQUIRE(LWRES_BUFFER_VALID(b));

	b->magic = 0;
	b->base = NULL;
	b->length = 0;
	b->used = 0;
	b->current = 0;
	b->active = 0;
}

/* Increase the 'used' region of 'b' by 'n' bytes. */
void
lwres_buffer_add(lwres_buffer_t *b, unsigned int n)
{

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used + n <= b->length);

	b->used += n;
}

/* Decrease the 'used' region of 'b' by 'n' bytes. */
void
lwres_buffer_subtract(lwres_buffer_t *b, unsigned int n)
{

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used >= n);

	b->used -= n;
	if (b->current > b->used)
		b->current = b->used;
	if (b->active > b->used)
		b->active = b->used;
}

/* Make the used region empty. */
void
lwres_buffer_clear(lwres_buffer_t *b)
{

	REQUIRE(LWRES_BUFFER_VALID(b));

	b->used = 0;
	b->current = 0;
	b->active = 0;
}

/* Make the consumed region empty. */
void
lwres_buffer_first(lwres_buffer_t *b)
{

	REQUIRE(LWRES_BUFFER_VALID(b));

	b->current = 0;
}

/* Increase the 'consumed' region of 'b' by 'n' bytes. */
void
lwres_buffer_forward(lwres_buffer_t *b, unsigned int n)
{

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->current + n <= b->used);

	b->current += n;
}

/* Decrease the 'consumed' region of 'b' by 'n' bytes. */
void
lwres_buffer_back(lwres_buffer_t *b, unsigned int n)
{

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(n <= b->current);

	b->current -= n;
}

/* Read an unsigned 8-bit integer from 'b' and return it. */
lwres_uint8_t
lwres_buffer_getuint8(lwres_buffer_t *b)
{
	unsigned char *cp;
	lwres_uint8_t result;


	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 1);

	cp = b->base;
	cp += b->current;
	b->current += 1;
	result = ((unsigned int)(cp[0]));

	return (result);
}

/* Put an unsigned 8-bit integer */
void
lwres_buffer_putuint8(lwres_buffer_t *b, lwres_uint8_t val)
{
	unsigned char *cp;

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used + 1 <= b->length);

	cp = b->base;
	cp += b->used;
	b->used += 1;
	cp[0] = (val & 0x00ff);
}

/*  Read an unsigned 16-bit integer in network byte order from 'b', convert it to host byte order, and return it. */
lwres_uint16_t
lwres_buffer_getuint16(lwres_buffer_t *b)
{
	unsigned char *cp;
	lwres_uint16_t result;


	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 2);

	cp = b->base;
	cp += b->current;
	b->current += 2;
	result = ((unsigned int)(cp[0])) << 8;
	result |= ((unsigned int)(cp[1]));

	return (result);
}

/* Put an unsigned 16-bit integer. */
void
lwres_buffer_putuint16(lwres_buffer_t *b, lwres_uint16_t val)
{
	unsigned char *cp;

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used + 2 <= b->length);

	cp = b->base;
	cp += b->used;
	b->used += 2;
	cp[0] = (val & 0xff00) >> 8;
	cp[1] = (val & 0x00ff);
}

/*  Read an unsigned 32-bit integer in network byte order from 'b', convert it to host byte order, and return it. */
lwres_uint32_t
lwres_buffer_getuint32(lwres_buffer_t *b)
{
	unsigned char *cp;
	lwres_uint32_t result;

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 4);

	cp = b->base;
	cp += b->current;
	b->current += 4;
	result = ((unsigned int)(cp[0])) << 24;
	result |= ((unsigned int)(cp[1])) << 16;
	result |= ((unsigned int)(cp[2])) << 8;
	result |= ((unsigned int)(cp[3]));

	return (result);
}

/* Put an unsigned 32-bit integer. */
void
lwres_buffer_putuint32(lwres_buffer_t *b, lwres_uint32_t val)
{
	unsigned char *cp;

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used + 4 <= b->length);

	cp = b->base;
	cp += b->used;
	b->used += 4;
	cp[0] = (unsigned char)((val & 0xff000000) >> 24);
	cp[1] = (unsigned char)((val & 0x00ff0000) >> 16);
	cp[2] = (unsigned char)((val & 0x0000ff00) >> 8);
	cp[3] = (unsigned char)(val & 0x000000ff);
}

/* copies length bytes of memory at base to b */
void
lwres_buffer_putmem(lwres_buffer_t *b, const unsigned char *base,
		    unsigned int length)
{
	unsigned char *cp;

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used + length <= b->length);

	cp = (unsigned char *)b->base + b->used;
	memcpy(cp, base, length);
	b->used += length;
}

/* copies length bytes of memory at b to base */
void
lwres_buffer_getmem(lwres_buffer_t *b, unsigned char *base,
		    unsigned int length)
{
	unsigned char *cp;

	REQUIRE(LWRES_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= length);

	cp = b->base;
	cp += b->current;
	b->current += length;

	memcpy(base, cp, length);
}
