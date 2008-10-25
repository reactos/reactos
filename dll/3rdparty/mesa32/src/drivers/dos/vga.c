/*
 * Mesa 3-D graphics library
 * Version:  4.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#include <pc.h>
#include <stdlib.h>

#include "video.h"
#include "vga.h"


static vl_mode modes[] = {
   {
    /* .xres    = */ 320,
    /* .yres    = */ 200,
    /* .bpp     = */ 8,
    /* .mode    = */ 0x13 | 0x4000,
    /* .scanlen = */ 320,
    /* .sel     = */ -1,
    /* .gran    = */ 320*200
   },
   {
    /* .xres    = */ -1,
    /* .yres    = */ -1,
    /* .bpp     = */ -1,
    /* .mode    = */ 0xffff,
    /* .scanlen = */ -1,
    /* .sel     = */ -1,
    /* .gran    = */ -1
   }
};

static word16 vga_ver;
static int linear_selector;
static int oldmode = -1;

#define vga_color_precision 6


/* Desc: Attempts to detect VGA, check video modes and create selectors.
 *
 * In  : -
 * Out : mode array
 *
 * Note: -
 */
static vl_mode *
vga_init (void)
{
   int rv = 0;

   if (vga_ver) {
      return modes;
   }

   __asm("\n\
		movw	$0x1a00, %%ax	\n\
		int	$0x10		\n\
		cmpb	$0x1a, %%al	\n\
		jne	0f		\n\
		cmpb	$0x07, %%bl	\n\
		jb	0f		\n\
		andl	$0xff, %%ebx	\n\
		movl	%%ebx, %0	\n\
   0:":"=g"(rv)::"%eax", "%ebx");
   if (rv == 0) {
      return NULL;
   }

   if (_create_selector(&linear_selector, 0xa0000, 0x10000)) {
      return NULL;
   }

   modes[0].sel = linear_selector;

   vga_ver = rv;
   return modes;
}


/* Desc: Frees all resources allocated by VGA init code.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
vga_fini (void)
{
   if (vga_ver) {
      _remove_selector(&linear_selector);
   }
}


/* Desc: Attempts to choose a suitable blitter.
 *
 * In  : ptr to mode structure, software framebuffer bits
 * Out : blitter funciton, or NULL
 *
 * Note: -
 */
static BLTFUNC
_choose_blitter (vl_mode *p, int fbbits)
{
    BLTFUNC blitter;

    switch (fbbits) {
	case 8:
	    blitter = _can_mmx() ? vesa_l_dump_virtual_mmx : vesa_l_dump_virtual;
	    break;
	case 16:
	    blitter = vesa_l_dump_16_to_8;
	    break;
	case 24:
	    blitter = vesa_l_dump_24_to_8;
	    break;
	case 32:
	    blitter = vesa_l_dump_32_to_8;
	    break;
	default:
	    return NULL;
    }

    return blitter;

    (void)p;
}


/* Desc: Attempts to enter specified video mode.
 *
 * In  : ptr to mode structure, refresh rate
 * Out : 0 if success
 *
 * Note: -
 */
static int
vga_entermode (vl_mode *p, int refresh, int fbbits)
{
    if (!(p->mode & 0x4000)) {
	return -1;
    }

    VGA.blit = _choose_blitter(p, fbbits);
    if (VGA.blit == NULL) {
	return !0;
    }

    if (oldmode == -1) {
	__asm("\n\
		movb	$0x0f, %%ah	\n\
		int	$0x10		\n\
		andl	$0xff, %%eax	\n\
		movl	%%eax, %0	\n\
	":"=g"(oldmode)::"%eax", "%ebx");
    }

    __asm("int $0x10"::"a"(p->mode&0xff));

    return 0;

    (void)refresh; /* silence compiler warning */
}


/* Desc: Restores to the mode prior to first call to vga_entermode.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
vga_restore (void)
{
   if (oldmode != -1) {
      __asm("int $0x10"::"a"(oldmode));
      oldmode = -1;
   }
}


/* Desc: set one palette entry
 *
 * In  : color index, R, G, B
 * Out : -
 *
 * Note: uses integer values
 */
static void
vga_setCI_i (int index, int red, int green, int blue)
{
#if 0
   __asm("\n\
		movw	$0x1010, %%ax	\n\
		movb	%1, %%dh	\n\
		movb	%2, %%ch	\n\
		int	$0x10		\n\
   "::"b"(index), "m"(red), "m"(green), "c"(blue):"%eax", "%edx");
#else
   outportb(0x03C8, index);
   outportb(0x03C9, red);
   outportb(0x03C9, green);
   outportb(0x03C9, blue);
#endif
}


/* Desc: set one palette entry
 *
 * In  : color index, R, G, B
 * Out : -
 *
 * Note: uses normalized values
 */
static void
vga_setCI_f (int index, float red, float green, float blue)
{
    float max = (1 << vga_color_precision) - 1;

    vga_setCI_i(index, (int)(red * max), (int)(green * max), (int)(blue * max));
}


/* Desc: state retrieval
 *
 * In  : parameter name, ptr to storage
 * Out : 0 if request successfully processed
 *
 * Note: -
 */
static int
vga_get (int pname, int *params)
{
   switch (pname) {
      case VL_GET_CI_PREC:
         params[0] = vga_color_precision;
         break;
      default:
         return -1;
   }
   return 0;
}


/*
 * the driver
 */
vl_driver VGA = {
   vga_init,
   vga_entermode,
   NULL,
   vga_setCI_f,
   vga_setCI_i,
   vga_get,
   vga_restore,
   vga_fini
};
