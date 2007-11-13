/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128.h,v 1.24 2002/12/16 16:19:10 dawes Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
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
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Rickard E. Faith <faith@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#ifndef _R128_H_
#define _R128_H_

#include "dri_util.h"

#define R128_DEBUG          0   /* Turn off debugging output               */
#define R128_IDLE_RETRY    32   /* Fall out of idle loops after this count */
#define R128_TIMEOUT  2000000   /* Fall out of wait loops after this count */
#define R128_MMIOSIZE  0x4000

#define R128_VBIOS_SIZE 0x00010000

#if R128_DEBUG
#define R128TRACE(x)                                          \
    do {                                                      \
	ErrorF("(**) %s(%d): ", R128_NAME, pScrn->scrnIndex); \
	ErrorF x;                                             \
    } while (0);
#else
#define R128TRACE(x)
#endif


/* Other macros */
#define R128_ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))
#define R128_ALIGN(x,bytes) (((x) + ((bytes) - 1)) & ~((bytes) - 1))
#define R128PTR(pScrn) ((R128InfoPtr)(pScrn)->driverPrivate)
    
/**
 * \brief Chip families.
 */
typedef enum {
    CHIP_FAMILY_UNKNOWN,
    CHIP_FAMILY_R128_PCI,
    CHIP_FAMILY_R128_AGP,
} R128ChipFamily;

typedef struct {        /* All values in XCLKS    */
    int  ML;            /* Memory Read Latency    */
    int  MB;            /* Memory Burst Length    */
    int  Trcd;          /* RAS to CAS delay       */
    int  Trp;           /* RAS percentage         */
    int  Twr;           /* Write Recovery         */
    int  CL;            /* CAS Latency            */
    int  Tr2w;          /* Read to Write Delay    */
    int  Rloop;         /* Loop Latency           */
    int  Rloop_fudge;   /* Add to ML to get Rloop */
    char *name;
} R128RAMRec, *R128RAMPtr;

typedef struct {
				/* Common registers */
    u_int32_t     ovr_clr;
    u_int32_t     ovr_wid_left_right;
    u_int32_t     ovr_wid_top_bottom;
    u_int32_t     ov0_scale_cntl;
    u_int32_t     mpp_tb_config;
    u_int32_t     mpp_gp_config;
    u_int32_t     subpic_cntl;
    u_int32_t     viph_control;
    u_int32_t     i2c_cntl_1;
    u_int32_t     gen_int_cntl;
    u_int32_t     cap0_trig_cntl;
    u_int32_t     cap1_trig_cntl;
    u_int32_t     bus_cntl;
    u_int32_t     config_cntl;

				/* Other registers to save for VT switches */
    u_int32_t     dp_datatype;
    u_int32_t     gen_reset_cntl;
    u_int32_t     clock_cntl_index;
    u_int32_t     amcgpio_en_reg;
    u_int32_t     amcgpio_mask;

				/* CRTC registers */
    u_int32_t     crtc_gen_cntl;
    u_int32_t     crtc_ext_cntl;
    u_int32_t     dac_cntl;
    u_int32_t     crtc_h_total_disp;
    u_int32_t     crtc_h_sync_strt_wid;
    u_int32_t     crtc_v_total_disp;
    u_int32_t     crtc_v_sync_strt_wid;
    u_int32_t     crtc_offset;
    u_int32_t     crtc_offset_cntl;
    u_int32_t     crtc_pitch;

				/* CRTC2 registers */
    u_int32_t     crtc2_gen_cntl;

				/* Flat panel registers */
    u_int32_t     fp_crtc_h_total_disp;
    u_int32_t     fp_crtc_v_total_disp;
    u_int32_t     fp_gen_cntl;
    u_int32_t     fp_h_sync_strt_wid;
    u_int32_t     fp_horz_stretch;
    u_int32_t     fp_panel_cntl;
    u_int32_t     fp_v_sync_strt_wid;
    u_int32_t     fp_vert_stretch;
    u_int32_t     lvds_gen_cntl;
    u_int32_t     tmds_crc;
    u_int32_t     tmds_transmitter_cntl;

				/* Computed values for PLL */
    u_int32_t     dot_clock_freq;
    u_int32_t     pll_output_freq;
    int        feedback_div;
    int        post_div;

				/* PLL registers */
    u_int32_t     ppll_ref_div;
    u_int32_t     ppll_div_3;
    u_int32_t     htotal_cntl;

				/* DDA register */
    u_int32_t     dda_config;
    u_int32_t     dda_on_off;

				/* Pallet */
    GLboolean  palette_valid;
    u_int32_t     palette[256];
} R128SaveRec, *R128SavePtr;

typedef struct {
    int               Chipset;
    GLboolean              Primary;

    GLboolean              FBDev;

    unsigned long     LinearAddr;   /* Frame buffer physical address         */
    unsigned long     BIOSAddr;     /* BIOS physical address                 */

    unsigned char     *MMIO;        /* Map of MMIO region                    */
    unsigned char     *FB;          /* Map of frame buffer                   */

    u_int32_t            MemCntl;
    u_int32_t            BusCntl;
    unsigned long     FbMapSize;    /* Size of frame buffer, in bytes        */
    int               Flags;        /* Saved copy of mode flags              */

				/* Computed values for FPs */
    int               PanelXRes;
    int               PanelYRes;
    int               HOverPlus;
    int               HSyncWidth;
    int               HBlank;
    int               VOverPlus;
    int               VSyncWidth;
    int               VBlank;
    int               PanelPwrDly;
    
    unsigned long     cursor_start;
    unsigned long     cursor_end;

    /*
     * XAAForceTransBlit is used to change the behavior of the XAA
     * SetupForScreenToScreenCopy function, to make it DGA-friendly.
     */
    GLboolean              XAAForceTransBlit;

    int               fifo_slots;   /* Free slots in the FIFO (64 max)       */
    int               pix24bpp;     /* Depth of pixmap for 24bpp framebuffer */
    GLboolean              dac6bits;     /* Use 6 bit DAC?                        */

				/* Computed values for Rage 128 */
    int               pitch;
    int               datatype;
    u_int32_t            dp_gui_master_cntl;

				/* Saved values for ScreenToScreenCopy */
    int               xdir;
    int               ydir;

				/* ScanlineScreenToScreenColorExpand support */
    unsigned char     *scratch_buffer[1];
    unsigned char     *scratch_save;
    int               scanline_x;
    int               scanline_y;
    int               scanline_w;
    int               scanline_h;

    int               scanline_hpass;
    int               scanline_x1clip;
    int               scanline_x2clip;
    int               scanline_rop;
    int               scanline_fg;
    int               scanline_bg;

    int               scanline_words;
    int               scanline_direct;
    int               scanline_bpp; /* Only used for ImageWrite */

    drm_context_t        drmCtx;

    drmSize           registerSize;
    drm_handle_t         registerHandle;

    GLboolean         IsPCI;            /* Current card is a PCI card */
    drmSize           pciSize;
    drm_handle_t         pciMemHandle;
    unsigned char     *PCI;             /* Map */

    GLboolean         allowPageFlip;    /* Enable 3d page flipping */
    GLboolean         have3DWindows;    /* Are there any 3d clients? */
    int               drmMinor;

    drmSize           agpSize;
    drm_handle_t         agpMemHandle;     /* Handle from drmAgpAlloc */
    unsigned long     agpOffset;
    unsigned char     *AGP;             /* Map */
    int               agpMode;

    GLboolean         CCEInUse;         /* CCE is currently active */
    int               CCEMode;          /* CCE mode that server/clients use */
    int               CCEFifoSize;      /* Size of the CCE command FIFO */
    GLboolean         CCESecure;        /* CCE security enabled */
    int               CCEusecTimeout;   /* CCE timeout in usecs */

				/* CCE ring buffer data */
    unsigned long     ringStart;        /* Offset into AGP space */
    drm_handle_t         ringHandle;       /* Handle from drmAddMap */
    drmSize           ringMapSize;      /* Size of map */
    int               ringSize;         /* Size of ring (in MB) */
    unsigned char     *ring;            /* Map */
    int               ringSizeLog2QW;

    unsigned long     ringReadOffset;   /* Offset into AGP space */
    drm_handle_t         ringReadPtrHandle; /* Handle from drmAddMap */
    drmSize           ringReadMapSize;  /* Size of map */
    unsigned char     *ringReadPtr;     /* Map */

				/* CCE vertex/indirect buffer data */
    unsigned long     bufStart;        /* Offset into AGP space */
    drm_handle_t         bufHandle;       /* Handle from drmAddMap */
    drmSize           bufMapSize;      /* Size of map */
    int               bufSize;         /* Size of buffers (in MB) */
    unsigned char     *buf;            /* Map */
    int               bufNumBufs;      /* Number of buffers */
    drmBufMapPtr      buffers;         /* Buffer map */

				/* CCE AGP Texture data */
    unsigned long     agpTexStart;      /* Offset into AGP space */
    drm_handle_t         agpTexHandle;     /* Handle from drmAddMap */
    drmSize           agpTexMapSize;    /* Size of map */
    int               agpTexSize;       /* Size of AGP tex space (in MB) */
    unsigned char     *agpTex;          /* Map */
    int               log2AGPTexGran;

				/* CCE 2D accleration */
    drmBufPtr         indirectBuffer;
    int               indirectStart;

				/* DRI screen private data */
    int               fbX;
    int               fbY;
    int               backX;
    int               backY;
    int               depthX;
    int               depthY;

    int               frontOffset;
    int               frontPitch;
    int               backOffset;
    int               backPitch;
    int               depthOffset;
    int               depthPitch;
    int               spanOffset;
    int               textureOffset;
    int               textureSize;
    int               log2TexGran;

				/* Saved scissor values */
    u_int32_t            sc_left;
    u_int32_t            sc_right;
    u_int32_t            sc_top;
    u_int32_t            sc_bottom;

    u_int32_t            re_top_left;
    u_int32_t            re_width_height;

    u_int32_t            aux_sc_cntl;

    int               irq;
    u_int32_t            gen_int_cntl;

    GLboolean              DMAForXv;

} R128InfoRec, *R128InfoPtr;

#define R128WaitForFifo(pScrn, entries)                                      \
do {                                                                         \
    if (info->fifo_slots < entries) R128WaitForFifoFunction(pScrn, entries); \
    info->fifo_slots -= entries;                                             \
} while (0)

extern void        r128WaitForFifoFunction(const DRIDriverContext *ctx, int entries);
extern void        r128WaitForIdle(const DRIDriverContext *ctx);

extern void        r128WaitForVerticalSync(const DRIDriverContext *ctx);

extern GLboolean        r128AccelInit(const DRIDriverContext *ctx);
extern void        r128EngineInit(const DRIDriverContext *ctx);
extern GLboolean        r128CursorInit(const DRIDriverContext *ctx);
extern GLboolean        r128DGAInit(const DRIDriverContext *ctx);

extern void        r128InitVideo(const DRIDriverContext *ctx);

extern GLboolean        r128DRIScreenInit(const DRIDriverContext *ctx);
extern void        r128DRICloseScreen(const DRIDriverContext *ctx);
extern GLboolean        r128DRIFinishScreenInit(const DRIDriverContext *ctx);

#define R128CCE_START(ctx, info)					\
do {									\
    int _ret = drmCommandNone(ctx->drmFD, DRM_R128_CCE_START);		\
    if (_ret) {								\
	   fprintf(stderr,				\
		   "%s: CCE start %d\n", __FUNCTION__, _ret);		\
    }									\
} while (0)

#define R128CCE_STOP(ctx, info)					\
do {									\
    int _ret = R128CCEStop(ctx);					\
    if (_ret) {								\
	   fprintf(stderr,				\
		   "%s: CCE stop %d\n", __FUNCTION__, _ret);		\
    }									\
} while (0)

#define R128CCE_RESET(ctx, info)					\
do {									\
    if (info->directRenderingEnabled					\
	&& R128CCE_USE_RING_BUFFER(info->CCEMode)) {			\
	int _ret = drmCommandNone(info->drmFD, DRM_R128_CCE_RESET);	\
	if (_ret) {							\
	       fprintf(stderr,			\
		       "%s: CCE reset %d\n", __FUNCTION__, _ret);	\
	}								\
    }									\
} while (0)

    
#define CCE_PACKET0( reg, n )						\
	(R128_CCE_PACKET0 | ((n) << 16) | ((reg) >> 2))
#define CCE_PACKET1( reg0, reg1 )					\
	(R128_CCE_PACKET1 | (((reg1) >> 2) << 11) | ((reg0) >> 2))
#define CCE_PACKET2()							\
	(R128_CCE_PACKET2)
#define CCE_PACKET3( pkt, n )						\
	(R128_CCE_PACKET3 | (pkt) | ((n) << 16))


#define R128_VERBOSE	0

#define RING_LOCALS	u_int32_t *__head; int __count;

#define R128CCE_REFRESH(pScrn, info)					\
do {									\
   if ( R128_VERBOSE ) {						\
         fprintf(stderr, "REFRESH( %d ) in %s\n",	\
		  !info->CCEInUse , __FUNCTION__ );			\
   }									\
   if ( !info->CCEInUse ) {						\
      R128CCEWaitForIdle(pScrn);       					\
      BEGIN_RING( 6 );							\
      OUT_RING_REG( R128_RE_TOP_LEFT,     info->re_top_left );		\
      OUT_RING_REG( R128_RE_WIDTH_HEIGHT, info->re_width_height );	\
      OUT_RING_REG( R128_AUX_SC_CNTL,     info->aux_sc_cntl );		\
      ADVANCE_RING();							\
      info->CCEInUse = TRUE;						\
   }									\
} while (0)

#define BEGIN_RING( n ) do {						\
   if ( R128_VERBOSE ) {						\
         fprintf(stderr,				\
		  "BEGIN_RING( %d ) in %s\n", n, __FUNCTION__ );	\
   }									\
   if ( !info->indirectBuffer ) {					\
      info->indirectBuffer = R128CCEGetBuffer( pScrn );			\
      info->indirectStart = 0;						\
   } else if ( (info->indirectBuffer->used + 4*(n)) >			\
                info->indirectBuffer->total ) {				\
      R128CCEFlushIndirect( pScrn, 1 );					\
   }									\
   __head = (pointer)((char *)info->indirectBuffer->address +		\
		       info->indirectBuffer->used);			\
   __count = 0;								\
} while (0)

#define ADVANCE_RING() do {						\
   if ( R128_VERBOSE ) {						\
         fprintf(stderr,				\
		  "ADVANCE_RING() used: %d+%d=%d/%d\n",			\
		  info->indirectBuffer->used - info->indirectStart,	\
		  __count * sizeof(u_int32_t),				\
		  info->indirectBuffer->used - info->indirectStart +	\
		  __count * sizeof(u_int32_t),				\
		  info->indirectBuffer->total - info->indirectStart );	\
   }									\
   info->indirectBuffer->used += __count * (int)sizeof(u_int32_t);		\
} while (0)

#define OUT_RING( x ) do {						\
   if ( R128_VERBOSE ) {						\
         fprintf(stderr,				\
		  "   OUT_RING( 0x%08x )\n", (unsigned int)(x) );	\
   }									\
   MMIO_OUT32(&__head[__count++], 0, (x));				\
} while (0)

#define OUT_RING_REG( reg, val )					\
do {									\
   OUT_RING( CCE_PACKET0( reg, 0 ) );					\
   OUT_RING( val );							\
} while (0)

#define FLUSH_RING()							\
do {									\
   if ( R128_VERBOSE )							\
         fprintf(stderr,				\
		  "FLUSH_RING in %s\n", __FUNCTION__ );			\
   if ( info->indirectBuffer ) {					\
      R128CCEFlushIndirect( pScrn, 0 );					\
   }									\
} while (0)

    
#endif
