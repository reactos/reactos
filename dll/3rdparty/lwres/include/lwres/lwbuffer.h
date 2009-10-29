/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: lwbuffer.h,v 1.22 2007/06/19 23:47:23 tbox Exp $ */


/*! \file lwres/lwbuffer.h
 *
 * A buffer is a region of memory, together with a set of related subregions.
 * Buffers are used for parsing and I/O operations.
 *
 * The 'used region' and the 'available' region are disjoint, and their
 * union is the buffer's region.  The used region extends from the beginning
 * of the buffer region to the last used byte.  The available region
 * extends from one byte greater than the last used byte to the end of the
 * buffer's region.  The size of the used region can be changed using various
 * buffer commands.  Initially, the used region is empty.
 *
 * The used region is further subdivided into two disjoint regions: the
 * 'consumed region' and the 'remaining region'.  The union of these two
 * regions is the used region.  The consumed region extends from the beginning
 * of the used region to the byte before the 'current' offset (if any).  The
 * 'remaining' region the current pointer to the end of the used
 * region.  The size of the consumed region can be changed using various
 * buffer commands.  Initially, the consumed region is empty.
 *
 * The 'active region' is an (optional) subregion of the remaining region.
 * It extends from the current offset to an offset in the remaining region
 * that is selected with lwres_buffer_setactive().  Initially, the active
 * region is empty.  If the current offset advances beyond the chosen offset,
 * the active region will also be empty.
 *
 * \verbatim
 *  /----- used region -----\/-- available --\
 *  +----------------------------------------+
 *  | consumed  | remaining |                |
 *  +----------------------------------------+
 *  a           b     c     d                e
 *
 * a == base of buffer.
 * b == current pointer.  Can be anywhere between a and d.
 * c == active pointer.  Meaningful between b and d.
 * d == used pointer.
 * e == length of buffer.
 *
 * a-e == entire (length) of buffer.
 * a-d == used region.
 * a-b == consumed region.
 * b-d == remaining region.
 * b-c == optional active region.
 * \endverbatim
 *
 * The following invariants are maintained by all routines:
 *
 *\verbatim
 *	length > 0
 *
 *	base is a valid pointer to length bytes of memory
 *
 *	0 <= used <= length
 *
 *	0 <= current <= used
 *
 *	0 <= active <= used
 *	(although active < current implies empty active region)
 *\endverbatim
 *
 * \li MP:
 *	Buffers have no synchronization.  Clients must ensure exclusive
 *	access.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	Memory: 1 pointer + 6 unsigned integers per buffer.
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

#ifndef LWRES_LWBUFFER_H
#define LWRES_LWBUFFER_H 1

/***
 *** Imports
 ***/

#include <lwres/lang.h>
#include <lwres/int.h>

LWRES_LANG_BEGINDECLS

/***
 *** Magic numbers
 ***/
#define LWRES_BUFFER_MAGIC		0x4275663fU	/* Buf?. */

#define LWRES_BUFFER_VALID(b)		((b) != NULL && \
					 (b)->magic == LWRES_BUFFER_MAGIC)

/*!
 * The following macros MUST be used only on valid buffers.  It is the
 * caller's responsibility to ensure this by using the LWRES_BUFFER_VALID
 * check above, or by calling another lwres_buffer_*() function (rather than
 * another macro.)
 */

/*!
 * Get the length of the used region of buffer "b"
 */
#define LWRES_BUFFER_USEDCOUNT(b)	((b)->used)

/*!
 * Get the length of the available region of buffer "b"
 */
#define LWRES_BUFFER_AVAILABLECOUNT(b)	((b)->length - (b)->used)

#define LWRES_BUFFER_REMAINING(b)	((b)->used - (b)->current)

/*!
 * Note that the buffer structure is public.  This is principally so buffer
 * operations can be implemented using macros.  Applications are strongly
 * discouraged from directly manipulating the structure.
 */

typedef struct lwres_buffer lwres_buffer_t;
/*!
 * Buffer data structure
 */
struct lwres_buffer {
	unsigned int		magic;
	unsigned char 	       *base;
	/* The following integers are byte offsets from 'base'. */
	unsigned int		length;
	unsigned int		used;
	unsigned int 		current;
	unsigned int 		active;
};

/***
 *** Functions
 ***/

void
lwres_buffer_init(lwres_buffer_t *b, void *base, unsigned int length);
/**<
 * Make 'b' refer to the 'length'-byte region starting at base.
 *
 * Requires:
 *
 *	'length' > 0
 *
 *	'base' is a pointer to a sequence of 'length' bytes.
 *
 */

void
lwres_buffer_invalidate(lwres_buffer_t *b);
/**<
 * Make 'b' an invalid buffer.
 *
 * Requires:
 *	'b' is a valid buffer.
 *
 * Ensures:
 *	If assertion checking is enabled, future attempts to use 'b' without
 *	calling lwres_buffer_init() on it will cause an assertion failure.
 */

void
lwres_buffer_add(lwres_buffer_t *b, unsigned int n);
/**<
 * Increase the 'used' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *	'b' is a valid buffer
 *
 *	used + n <= length
 *
 */

void
lwres_buffer_subtract(lwres_buffer_t *b, unsigned int n);
/**<
 * Decrease the 'used' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *	'b' is a valid buffer
 *
 *	used >= n
 *
 */

void
lwres_buffer_clear(lwres_buffer_t *b);
/**<
 * Make the used region empty.
 *
 * Requires:
 *
 *	'b' is a valid buffer
 *
 * Ensures:
 *
 *	used = 0
 *
 */


void
lwres_buffer_first(lwres_buffer_t *b);
/**<
 * Make the consumed region empty.
 *
 * Requires:
 *
 *	'b' is a valid buffer
 *
 * Ensures:
 *
 *	current == 0
 *
 */

void
lwres_buffer_forward(lwres_buffer_t *b, unsigned int n);
/**<
 * Increase the 'consumed' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *	'b' is a valid buffer
 *
 *	current + n <= used
 *
 */

void
lwres_buffer_back(lwres_buffer_t *b, unsigned int n);
/**<
 * Decrease the 'consumed' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *	'b' is a valid buffer
 *
 *	n <= current
 *
 */

lwres_uint8_t
lwres_buffer_getuint8(lwres_buffer_t *b);
/**<
 * Read an unsigned 8-bit integer from 'b' and return it.
 *
 * Requires:
 *
 *	'b' is a valid buffer.
 *
 *	The length of the available region of 'b' is at least 1.
 *
 * Ensures:
 *
 *	The current pointer in 'b' is advanced by 1.
 *
 * Returns:
 *
 *	A 8-bit unsigned integer.
 */

void
lwres_buffer_putuint8(lwres_buffer_t *b, lwres_uint8_t val);
/**<
 * Store an unsigned 8-bit integer from 'val' into 'b'.
 *
 * Requires:
 *	'b' is a valid buffer.
 *
 *	The length of the unused region of 'b' is at least 1.
 *
 * Ensures:
 *	The used pointer in 'b' is advanced by 1.
 */

lwres_uint16_t
lwres_buffer_getuint16(lwres_buffer_t *b);
/**<
 * Read an unsigned 16-bit integer in network byte order from 'b', convert
 * it to host byte order, and return it.
 *
 * Requires:
 *
 *	'b' is a valid buffer.
 *
 *	The length of the available region of 'b' is at least 2.
 *
 * Ensures:
 *
 *	The current pointer in 'b' is advanced by 2.
 *
 * Returns:
 *
 *	A 16-bit unsigned integer.
 */

void
lwres_buffer_putuint16(lwres_buffer_t *b, lwres_uint16_t val);
/**<
 * Store an unsigned 16-bit integer in host byte order from 'val'
 * into 'b' in network byte order.
 *
 * Requires:
 *	'b' is a valid buffer.
 *
 *	The length of the unused region of 'b' is at least 2.
 *
 * Ensures:
 *	The used pointer in 'b' is advanced by 2.
 */

lwres_uint32_t
lwres_buffer_getuint32(lwres_buffer_t *b);
/**<
 * Read an unsigned 32-bit integer in network byte order from 'b', convert
 * it to host byte order, and return it.
 *
 * Requires:
 *
 *	'b' is a valid buffer.
 *
 *	The length of the available region of 'b' is at least 2.
 *
 * Ensures:
 *
 *	The current pointer in 'b' is advanced by 2.
 *
 * Returns:
 *
 *	A 32-bit unsigned integer.
 */

void
lwres_buffer_putuint32(lwres_buffer_t *b, lwres_uint32_t val);
/**<
 * Store an unsigned 32-bit integer in host byte order from 'val'
 * into 'b' in network byte order.
 *
 * Requires:
 *	'b' is a valid buffer.
 *
 *	The length of the unused region of 'b' is at least 4.
 *
 * Ensures:
 *	The used pointer in 'b' is advanced by 4.
 */

void
lwres_buffer_putmem(lwres_buffer_t *b, const unsigned char *base,
		    unsigned int length);
/**<
 * Copy 'length' bytes of memory at 'base' into 'b'.
 *
 * Requires:
 *	'b' is a valid buffer.
 *
 *	'base' points to 'length' bytes of valid memory.
 *
 */

void
lwres_buffer_getmem(lwres_buffer_t *b, unsigned char *base,
		    unsigned int length);
/**<
 * Copy 'length' bytes of memory from 'b' into 'base'.
 *
 * Requires:
 *	'b' is a valid buffer.
 *
 *	'base' points to at least 'length' bytes of valid memory.
 *
 *	'b' have at least 'length' bytes remaining.
 */

LWRES_LANG_ENDDECLS

#endif /* LWRES_LWBUFFER_H */
