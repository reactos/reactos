/**
 * \file server/radeon_macros.h
 * \brief Macros for Radeon MMIO operation.
 *
 * \authors Kevin E. Martin <martin@xfree86.org>
 * \authors Rickard E. Faith <faith@valinux.com>
 * \authors Alan Hourihane <alanh@fairlite.demon.co.uk>
 */

/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/radeon_reg.h,v 1.20 2002/10/12 01:38:07 martin Exp $ */

#ifndef _RADEON_MACROS_H_
#define _RADEON_MACROS_H_

#include <mmio.h>

#  define MMIO_IN8(base, offset) \
	*(volatile unsigned char *)(((unsigned char*)(base)) + (offset))
#  define MMIO_IN32(base, offset) \
	read_MMIO_LE32(base, offset)
#  define MMIO_OUT8(base, offset, val) \
	*(volatile unsigned char *)(((unsigned char*)(base)) + (offset)) = (val)
#  define MMIO_OUT32(base, offset, val) \
	*(volatile unsigned int *)(void *)(((unsigned char*)(base)) + (offset)) = CPU_TO_LE32(val)


				/* Memory mapped register access macros */
#define INREG8(addr)        MMIO_IN8(RADEONMMIO, addr)
#define INREG(addr)         MMIO_IN32(RADEONMMIO, addr)
#define OUTREG8(addr, val)  MMIO_OUT8(RADEONMMIO, addr, val)
#define OUTREG(addr, val)   MMIO_OUT32(RADEONMMIO, addr, val)

#define ADDRREG(addr)       ((volatile GLuint *)(pointer)(RADEONMMIO + (addr)))


#define OUTREGP(addr, val, mask)					\
do {									\
    GLuint tmp = INREG(addr);						\
    tmp &= (mask);							\
    tmp |= (val);							\
    OUTREG(addr, tmp);							\
} while (0)

#define INPLL(dpy, addr) RADEONINPLL(dpy, addr)

#define OUTPLL(addr, val)						\
do {									\
    OUTREG8(RADEON_CLOCK_CNTL_INDEX, (((addr) & 0x3f) |			\
				      RADEON_PLL_WR_EN));		\
    OUTREG(RADEON_CLOCK_CNTL_DATA, val);				\
} while (0)

#define OUTPLLP(dpy, addr, val, mask)					\
do {									\
    GLuint tmp = INPLL(dpy, addr);					\
    tmp &= (mask);							\
    tmp |= (val);							\
    OUTPLL(addr, tmp);							\
} while (0)

#define OUTPAL_START(idx)						\
do {									\
    OUTREG8(RADEON_PALETTE_INDEX, (idx));				\
} while (0)

#define OUTPAL_NEXT(r, g, b)						\
do {									\
    OUTREG(RADEON_PALETTE_DATA, ((r) << 16) | ((g) << 8) | (b));	\
} while (0)

#define OUTPAL_NEXT_CARD32(v)						\
do {									\
    OUTREG(RADEON_PALETTE_DATA, (v & 0x00ffffff));			\
} while (0)

#define OUTPAL(idx, r, g, b)						\
do {									\
    OUTPAL_START((idx));						\
    OUTPAL_NEXT((r), (g), (b));						\
} while (0)

#define INPAL_START(idx)						\
do {									\
    OUTREG(RADEON_PALETTE_INDEX, (idx) << 16);				\
} while (0)

#define INPAL_NEXT() INREG(RADEON_PALETTE_DATA)

#define PAL_SELECT(idx)							\
do {									\
    if (!idx) {								\
	OUTREG(RADEON_DAC_CNTL2, INREG(RADEON_DAC_CNTL2) &		\
	       (GLuint)~RADEON_DAC2_PALETTE_ACC_CTL);			\
    } else {								\
	OUTREG(RADEON_DAC_CNTL2, INREG(RADEON_DAC_CNTL2) |		\
	       RADEON_DAC2_PALETTE_ACC_CTL);				\
    }									\
} while (0)


#endif
