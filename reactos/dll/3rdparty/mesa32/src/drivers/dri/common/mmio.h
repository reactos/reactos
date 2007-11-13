/*
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file mmio.h
 * Functions for properly handling memory mapped IO on various platforms.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */


#ifndef MMIO_H
#define MMIO_H

#include "glheader.h"

#if defined( __powerpc__ )

static __inline__ u_int32_t
read_MMIO_LE32( volatile void * base, unsigned long offset )
{
   u_int32_t val;

   __asm__ __volatile__( "lwbrx	%0, %1, %2 ; eieio"
			 : "=r" (val)
			 : "b" (base), "r" (offset) );
   return val;
}

#else

static __inline__ u_int32_t
read_MMIO_LE32( volatile void * base, unsigned long offset )
{
   volatile u_int32_t * p = (volatile u_int32_t *) (((volatile char *) base) + offset);
   return LE32_TO_CPU( p[0] );
}

#endif

#endif /* MMIO_H */
