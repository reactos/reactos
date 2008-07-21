/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file bitset.h
 * \brief Bitset of arbitrary size definitions.
 * \author Michal Krol
 */
 
/****************************************************************************
 * generic bitset implementation
 */

#define BITSET_WORD GLuint
#define BITSET_WORDBITS (sizeof (BITSET_WORD) * 8)

/* bitset declarations
 */
#define BITSET_DECLARE(name, size) \
   BITSET_WORD name[((size) + BITSET_WORDBITS - 1) / BITSET_WORDBITS]

/* bitset operations
 */
#define BITSET_COPY(x, y) _mesa_memcpy( (x), (y), sizeof (x) )
#define BITSET_EQUAL(x, y) (_mesa_memcmp( (x), (y), sizeof (x) ) == 0)
#define BITSET_ZERO(x) _mesa_memset( (x), 0, sizeof (x) )
#define BITSET_ONES(x) _mesa_memset( (x), 0xff, sizeof (x) )

#define BITSET_BITWORD(b) ((b) / BITSET_WORDBITS)
#define BITSET_BIT(b) (1 << ((b) % BITSET_WORDBITS))

/* single bit operations
 */
#define BITSET_TEST(x, b) ((x)[BITSET_BITWORD(b)] & BITSET_BIT(b))
#define BITSET_SET(x, b) ((x)[BITSET_BITWORD(b)] |= BITSET_BIT(b))
#define BITSET_CLEAR(x, b) ((x)[BITSET_BITWORD(b)] &= ~BITSET_BIT(b))

#define BITSET_MASK(b) ((b) == BITSET_WORDBITS ? ~0 : BITSET_BIT(b) - 1)
#define BITSET_RANGE(b, e) (BITSET_MASK((e) + 1) & ~BITSET_MASK(b))

/* bit range operations
 */
#define BITSET_TEST_RANGE(x, b, e) \
   (BITSET_BITWORD(b) == BITSET_BITWORD(e) ? \
   ((x)[BITSET_BITWORD(b)] & BITSET_RANGE(b, e)) : \
   (assert (!"BITSET_TEST_RANGE: bit range crosses word boundary"), 0))
#define BITSET_SET_RANGE(x, b, e) \
   (BITSET_BITWORD(b) == BITSET_BITWORD(e) ? \
   ((x)[BITSET_BITWORD(b)] |= BITSET_RANGE(b, e)) : \
   (assert (!"BITSET_SET_RANGE: bit range crosses word boundary"), 0))
#define BITSET_CLEAR_RANGE(x, b, e) \
   (BITSET_BITWORD(b) == BITSET_BITWORD(e) ? \
   ((x)[BITSET_BITWORD(b)] &= ~BITSET_RANGE(b, e)) : \
   (assert (!"BITSET_CLEAR_RANGE: bit range crosses word boundary"), 0))

/****************************************************************************
 * 64-bit bitset implementation
 */
 
#define BITSET64_WORD GLuint
#define BITSET64_WORDBITS (sizeof (BITSET64_WORD) * 8)

/* bitset declarations
 */
#define BITSET64_DECLARE(name, size) \
   GLuint name[2]

/* bitset operations
 */
#define BITSET64_COPY(x, y) do { (x)[0] = (y)[0]; (x)[1] = (y)[1]; } while (0)
#define BITSET64_EQUAL(x, y) ( (x)[0] == (y)[0] && (x)[1] == (y)[1] )
#define BITSET64_ZERO(x) do { (x)[0] = 0; (x)[1] = 0; } while (0)
#define BITSET64_ONES(x) do { (x)[0] = 0xFF; (x)[1] = 0xFF; } while (0)

#define BITSET64_BITWORD(b) ((b) / BITSET64_WORDBITS)
#define BITSET64_BIT(b) (1 << ((b) % BITSET64_WORDBITS))

/* single bit operations
 */
#define BITSET64_TEST(x, b) ((x)[BITSET64_BITWORD(b)] & BITSET64_BIT(b))
#define BITSET64_SET(x, b) ((x)[BITSET64_BITWORD(b)] |= BITSET64_BIT(b))
#define BITSET64_CLEAR(x, b) ((x)[BITSET64_BITWORD(b)] &= ~BITSET64_BIT(b))

#define BITSET64_MASK(b) ((b) == BITSET64_WORDBITS ? ~0 : BITSET64_BIT(b) - 1)
#define BITSET64_RANGE(b, e) (BITSET64_MASK((e) + 1) & ~BITSET64_MASK(b))

/* bit range operations
 */
#define BITSET64_TEST_RANGE(x, b, e) \
   (BITSET64_BITWORD(b) == BITSET64_BITWORD(e) ? \
   ((x)[BITSET64_BITWORD(b)] & BITSET64_RANGE(b, e)) : \
   (assert (!"BITSET64_TEST_RANGE: bit range crosses word boundary"), 0))
#define BITSET64_SET_RANGE(x, b, e) \
   (BITSET64_BITWORD(b) == BITSET64_BITWORD(e) ? \
   ((x)[BITSET64_BITWORD(b)] |= BITSET64_RANGE(b, e)) : \
   (assert (!"BITSET64_SET_RANGE: bit range crosses word boundary"), 0))
#define BITSET64_CLEAR_RANGE(x, b, e) \
   (BITSET64_BITWORD(b) == BITSET64_BITWORD(e) ? \
   ((x)[BITSET64_BITWORD(b)] &= ~BITSET64_RANGE(b, e)) : \
   (assert (!"BITSET64_CLEAR_RANGE: bit range crosses word boundary"), 0))

