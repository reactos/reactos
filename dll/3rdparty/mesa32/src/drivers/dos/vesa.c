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


#include <dpmi.h>
#include <pc.h>
#include <stdlib.h>
#include <stubinfo.h>
#include <sys/exceptn.h>
#include <sys/segments.h>
#include <sys/farptr.h>
#include <sys/movedata.h>

#include "video.h"
#include "vesa.h"


static vl_mode modes[128];

static word16 vesa_ver;
static int banked_selector, linear_selector;
static int oldmode = -1;

static int vesa_color_precision = 6;

static word16 *vesa_pmcode;
unsigned int vesa_gran_mask, vesa_gran_shift;


/*
 * VESA info
 */
#define V_SIGN     0
#define V_MINOR    4
#define V_MAJOR    5
#define V_OEM_OFS  6
#define V_OEM_SEG  8
#define V_MODE_OFS 14
#define V_MODE_SEG 16
#define V_MEMORY   18

/*
 * mode info
 */
#define M_ATTR     0
#define M_GRAN     4
#define M_SCANLEN  16
#define M_XRES     18
#define M_YRES     20
#define M_BPP      25
#define M_RED      31
#define M_GREEN    33
#define M_BLUE     35
#define M_PHYS_PTR 40

/*
 * VESA 3.0 CRTC timings structure
 */
typedef struct CRTCInfoBlock {
    unsigned short HorizontalTotal;
    unsigned short HorizontalSyncStart;
    unsigned short HorizontalSyncEnd;
    unsigned short VerticalTotal;
    unsigned short VerticalSyncStart;
    unsigned short VerticalSyncEnd;
    unsigned char  Flags;
    unsigned long  PixelClock;	/* units of Hz */
    unsigned short RefreshRate;	/* units of 0.01 Hz */
    unsigned char  reserved[40];
} __PACKED__ CRTCInfoBlock;

#define HNEG         (1 << 2)
#define VNEG         (1 << 3)
#define DOUBLESCAN   (1 << 0)


/* Desc: Attempts to detect VESA, check video modes and create selectors.
 *
 * In  : -
 * Out : mode array
 *
 * Note: -
 */
static vl_mode *
vesa_init (void)
{
    __dpmi_regs r;
    word16 *p;
    vl_mode *q;
    char vesa_info[512], tmp[512];
    int maxsize = 0;
    word32 linearfb = 0;

    if (vesa_ver) {
	return modes;
    }

    _farpokel(_stubinfo->ds_selector, 0, 0x32454256);
    r.x.ax = 0x4f00;
    r.x.di = 0;
    r.x.es = _stubinfo->ds_segment;
    __dpmi_int(0x10, &r);
    movedata(_stubinfo->ds_selector, 0, _my_ds(), (unsigned)vesa_info, 512);
    if ((r.x.ax != 0x004f) || ((_32_ vesa_info[V_SIGN]) != 0x41534556)) {
	return NULL;
    }

    p = (word16 *)(((_16_ vesa_info[V_MODE_SEG]) << 4) + (_16_ vesa_info[V_MODE_OFS]));
    q = modes;
    do {
	if ((q->mode = _farpeekw(__djgpp_dos_sel, (unsigned long)(p++))) == 0xffff) {
	    break;
	}

	r.x.ax = 0x4f01;
	r.x.cx = q->mode;
	r.x.di = 512;
	r.x.es = _stubinfo->ds_segment;
	__dpmi_int(0x10, &r);
	movedata(_stubinfo->ds_selector, 512, _my_ds(), (unsigned)tmp, 256);
	switch (tmp[M_BPP]) {
	    case 16:
		q->bpp = tmp[M_RED] + tmp[M_GREEN] + tmp[M_BLUE];
		break;
	    case 8:
	    case 15:
	    case 24:
	    case 32:
		q->bpp = tmp[M_BPP];
		break;
	    default:
		q->bpp = 0;
	}
	if ((r.x.ax == 0x004f) && ((tmp[M_ATTR] & 0x11) == 0x11) && q->bpp) {
	    q->xres = _16_ tmp[M_XRES];
	    q->yres = _16_ tmp[M_YRES];
	    q->scanlen = _16_ tmp[M_SCANLEN];
	    q->gran = (_16_ tmp[M_GRAN]) << 10;
	    if (tmp[M_ATTR] & 0x80) {
		vl_mode *q1 = q + 1;
		*q1 = *q++;
		linearfb = _32_ tmp[M_PHYS_PTR];
		q->mode |= 0x4000;
	    }
	    if (maxsize < (q->scanlen * q->yres)) {
		maxsize = q->scanlen * q->yres;
	    }
	    q++;
	}
    } while (TRUE);

    if (q == modes) {
	return NULL;
    }
    if (_create_selector(&banked_selector, 0xa0000, modes[0].gran)) {
	return NULL;
    }
    if (linearfb) {
	maxsize = ((maxsize + 0xfffUL) & ~0xfffUL);
	if (_create_selector(&linear_selector, linearfb, maxsize)) {
	    linear_selector = banked_selector;
	}
    }

    for (q = modes; q->mode != 0xffff; q++) {
	q->sel = banked_selector;
	if (q->mode & 0x4000) {
	    if (linear_selector != banked_selector) {
		q->sel = linear_selector;
	    } else {
		q->mode &= ~0x4000;
	    }
	}
    }

    if (vesa_info[V_MAJOR] >= 2) {
	r.x.ax = 0x4f0a;
	r.x.bx = 0;
	__dpmi_int(0x10, &r);
	if (r.x.ax == 0x004f) {
	    vesa_pmcode = (word16 *)malloc(r.x.cx);
	    if (vesa_pmcode != NULL) {
		movedata(__djgpp_dos_sel, (r.x.es << 4) + r.x.di, _my_ds(), (unsigned)vesa_pmcode, r.x.cx);
		if (vesa_pmcode[3]) {
		    p = (word16 *)((long)vesa_pmcode + vesa_pmcode[3]);
		    while (*p++ != 0xffff) {
		    }
		} else {
		    p = NULL;
		}
		if (p && (*p != 0xffff)) {
		    free(vesa_pmcode);
		    vesa_pmcode = NULL;
		} else {
		    vesa_swbank = (void *)((long)vesa_pmcode + vesa_pmcode[0]);
		}
	    }
	}
    }

    vesa_ver = _16_ vesa_info[V_MINOR];
    return modes;
}


/* Desc: Frees all resources allocated by VESA init code.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
vesa_fini (void)
{
    if (vesa_ver) {
	_remove_selector(&linear_selector);
	_remove_selector(&banked_selector);
	if (vesa_pmcode != NULL) {
	    free(vesa_pmcode);
	    vesa_pmcode = NULL;
	}
    }
}


/* Desc: Uses VESA 3.0 function 0x4F0B to find the closest pixel clock to the requested value.
 *
 * In  : mode, clock
 * Out : desired clock
 *
 * Note: -
 */
static unsigned long
_closest_pixclk (int mode_no, unsigned long vclk)
{
    __dpmi_regs r;

    r.x.ax = 0x4F0B;
    r.h.bl = 0;
    r.d.ecx = vclk;
    r.x.dx = mode_no;
    __dpmi_int(0x10, &r);

    return (r.x.ax == 0x004f) ? r.d.ecx : 0;
}


/* Desc: Calculates CRTC mode timings.
 *
 * In  : crtc block, geometry, adjust
 * Out :
 *
 * Note:
 */
static void
_crtc_timing (CRTCInfoBlock *crtc, int xres, int yres, int xadjust, int yadjust)
{
    int HTotal, VTotal;
    int HDisp, VDisp;
    int HSS, VSS;
    int HSE, VSE;
    int HSWidth, VSWidth;
    int SS, SE;
    int doublescan = FALSE;

    if (yres < 400) {
	doublescan = TRUE;
	yres *= 2;
    }

    HDisp = xres;
    HTotal = (int)(HDisp * 1.27) & ~0x7;
    HSWidth = (int)((HTotal - HDisp) / 5) & ~0x7;
    HSS = HDisp + 16;
    HSE = HSS + HSWidth;
    VDisp = yres;
    VTotal = VDisp * 1.07;
    VSWidth = (VTotal / 100) + 1;
    VSS = VDisp + ((int)(VTotal - VDisp) / 5) + 1;
    VSE = VSS + VSWidth;

    SS = HSS + xadjust;
    SE = HSE + xadjust;

    if (xadjust < 0) {
	if (SS < (HDisp + 8)) {
	    SS = HDisp + 8;
	    SE = SS + HSWidth;
	}
    } else {
	if ((HTotal - 24) < SE) {
	    SE = HTotal - 24;
	    SS = SE - HSWidth;
	}
    }

    HSS = SS;
    HSE = SE;

    SS = VSS + yadjust;
    SE = VSE + yadjust;

    if (yadjust < 0) {
	if (SS < (VDisp + 3)) {
	    SS = VDisp + 3;
	    SE = SS + VSWidth;
	}
    } else {
	if ((VTotal - 4) < SE) {
	    SE = VTotal - 4;
	    SS = SE - VSWidth;
	}
    }

    VSS = SS;
    VSE = SE;

    crtc->HorizontalTotal     = HTotal;
    crtc->HorizontalSyncStart = HSS;
    crtc->HorizontalSyncEnd   = HSE;
    crtc->VerticalTotal       = VTotal;
    crtc->VerticalSyncStart   = VSS;
    crtc->VerticalSyncEnd     = VSE;
    crtc->Flags               = HNEG | VNEG;

    if (doublescan) {
	crtc->Flags |= DOUBLESCAN;
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

    if (p->mode & 0x4000) {
	blitter = _can_mmx() ? vesa_l_dump_virtual_mmx : vesa_l_dump_virtual;
	    switch (p->bpp) {
		case 8:
		    switch (fbbits) {
			case 8:
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
			case 15:
			default:
			    return NULL;
		    }
		    break;
		case 15:
		    switch (fbbits) {
			case 16:
			    blitter = vesa_l_dump_16_to_15;
			    break;
			case 32:
			    blitter = vesa_l_dump_32_to_15;
			    break;
			case 8:
			case 15:
			case 24:
			default:
			    return NULL;
		    }
		    break;
		case 16:
		    switch (fbbits) {
			case 16:
			    break;
			case 32:
			    blitter = vesa_l_dump_32_to_16;
			    break;
			case 8:
			case 15:
			case 24:
			default:
			    return NULL;
		    }
		    break;
		case 24:
		    switch (fbbits) {
			case 24:
			    break;
			case 32:
			    blitter = vesa_l_dump_32_to_24;
			    break;
			case 8:
			case 15:
			case 16:
			default:
			    return NULL;
		    }
		    break;
		case 32:
		    switch (fbbits) {
			case 24:
			    blitter = vesa_l_dump_24_to_32;
			    break;
			case 32:
			    break;
			case 8:
			case 15:
			case 16:
			default:
			    return NULL;
		    }
		    break;
	    }
    } else {
	blitter = vesa_b_dump_virtual;
	    switch (p->bpp) {
		case 8:
		    switch (fbbits) {
			case 8:
			    break;
			case 16:
			    blitter = vesa_b_dump_16_to_8;
			    break;
			case 24:
			    blitter = vesa_b_dump_24_to_8;
			    break;
			case 32:
			    blitter = vesa_b_dump_32_to_8;
			    break;
			case 15:
			default:
			    return NULL;
		    }
		    break;
		case 15:
		    switch (fbbits) {
			case 16:
			    blitter = vesa_b_dump_16_to_15;
			    break;
			case 32:
			    blitter = vesa_b_dump_32_to_15;
			    break;
			case 8:
			case 15:
			case 24:
			default:
			    return NULL;
		    }
		    break;
		case 16:
		    switch (fbbits) {
			case 16:
			    break;
			case 32:
			    blitter = vesa_b_dump_32_to_16;
			    break;
			case 8:
			case 15:
			case 24:
			default:
			    return NULL;
		    }
		    break;
		case 24:
		    switch (fbbits) {
			case 24:
			    break;
			case 32:
			    blitter = vesa_b_dump_32_to_24;
			    break;
			case 8:
			case 15:
			case 16:
			default:
			    return NULL;
		    }
		    break;
		case 32:
		    switch (fbbits) {
			case 24:
			    blitter = vesa_b_dump_24_to_32;
			    break;
			case 32:
			    break;
			case 8:
			case 15:
			case 16:
			default:
			    return NULL;
		    }
		    break;
	    }
    }

    return blitter;
}


/* Desc: Attempts to enter specified video mode.
 *
 * In  : ptr to mode structure, refresh rate
 * Out : 0 if success
 *
 * Note: -
 */
static int
vesa_entermode (vl_mode *p, int refresh, int fbbits)
{
    __dpmi_regs r;

    if (!(p->mode & 0x4000)) {
	{ int n; for (vesa_gran_shift = 0, n = p->gran; n; vesa_gran_shift++, n >>= 1); }
	vesa_gran_mask = (1 << (--vesa_gran_shift)) - 1;
	if ((unsigned)p->gran != (vesa_gran_mask + 1)) {
	    return !0;
	}
    }

    VESA.blit = _choose_blitter(p, fbbits);
    if (VESA.blit == NULL) {
	return !0;
    }

    if (oldmode == -1) {
	r.x.ax = 0x4f03;
	__dpmi_int(0x10, &r);
	oldmode = r.x.bx;
    }

    r.x.ax = 0x4f02;
    r.x.bx = p->mode;

    if (refresh && ((vesa_ver >> 8) >= 3)) {
	/* VESA 3.0 stuff for controlling the refresh rate */
	CRTCInfoBlock crtc;
	unsigned long vclk;
	double f0;

	_crtc_timing(&crtc, p->xres, p->yres, 0, 0);

	vclk = (double)crtc.HorizontalTotal * crtc.VerticalTotal * refresh;
	vclk = _closest_pixclk(p->mode, vclk);

	if (vclk != 0) {
	    f0 = (double)vclk / (crtc.HorizontalTotal * crtc.VerticalTotal);
	    /*_current_refresh_rate = (int)(f0 + 0.5);*/

	    crtc.PixelClock  = vclk;
	    crtc.RefreshRate = refresh * 100;

	    movedata(_my_ds(), (unsigned)&crtc, _stubinfo->ds_selector, 0, sizeof(crtc));

	    r.x.di = 0;
	    r.x.es = _stubinfo->ds_segment;
	    r.x.bx |= 0x0800;
	}
    }

    __dpmi_int(0x10, &r);
    if (r.x.ax != 0x004f) {
	return !0;
    }

    if (p->bpp == 8) {
	r.x.ax = 0x4f08;
	r.x.bx = 0x0800;
	__dpmi_int(0x10, &r);
	if (r.x.ax == 0x004f) {
	    r.x.ax = 0x4f08;
	    r.h.bl = 0x01;
	    __dpmi_int(0x10, &r);
	    vesa_color_precision = r.h.bh;
	}
    }

    return 0;
}


/* Desc: Restores to the mode prior to first call to vesa_entermode.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
vesa_restore (void)
{
    __dpmi_regs r;

    if (oldmode != -1) {
	if (oldmode < 0x100) {
	    __asm("int $0x10"::"a"(oldmode));
	} else {
	    r.x.ax = 0x4f02;
	    r.x.bx = oldmode;
	    __dpmi_int(0x10, &r);
	}
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
vesa_setCI_i (int index, int red, int green, int blue)
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
vesa_setCI_f (int index, float red, float green, float blue)
{
    float max = (1 << vesa_color_precision) - 1;

    vesa_setCI_i(index, (int)(red * max), (int)(green * max), (int)(blue * max));
}


/* Desc: state retrieval
 *
 * In  : parameter name, ptr to storage
 * Out : 0 if request successfully processed
 *
 * Note: -
 */
static int
vesa_get (int pname, int *params)
{
    switch (pname) {
	case VL_GET_CI_PREC:
	    params[0] = vesa_color_precision;
	    break;
	default:
	    return -1;
    }
    return 0;
}


/*
 * the driver
 */
vl_driver VESA = {
    vesa_init,
    vesa_entermode,
    NULL,
    vesa_setCI_f,
    vesa_setCI_i,
    vesa_get,
    vesa_restore,
    vesa_fini
};
