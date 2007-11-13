/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sunffb/ffb_drishare.h,v 1.2 2000/06/21 00:47:37 dawes Exp $ */

#ifndef _FFB_DRISHARE_H
#define _FFB_DRISHARE_H

typedef struct ffb_dri_state {
	int		flags;
#define FFB_DRI_FFB2		0x00000001
#define FFB_DRI_FFB2PLUS	0x00000002
#define FFB_DRI_PAC1		0x00000004
#define FFB_DRI_PAC2		0x00000008

	/* Indexed by DRI drawable id. */
#define FFB_DRI_NWIDS	64
	unsigned int	wid_table[FFB_DRI_NWIDS];
} ffb_dri_state_t;

#define FFB_DRISHARE(SAREA)	\
	((ffb_dri_state_t *) (((char *)(SAREA)) + sizeof(drm_sarea_t)))

typedef struct {
	drm_handle_t	hFbcRegs;
	drmSize		sFbcRegs;

	drm_handle_t	hDacRegs;
	drmSize		sDacRegs;

	drm_handle_t	hSfb8r;
	drmSize		sSfb8r;

	drm_handle_t	hSfb32;
	drmSize		sSfb32;

	drm_handle_t	hSfb64;
	drmSize		sSfb64;

	/* Fastfill/Pagefill parameters. */
	unsigned char	disable_pagefill;
	int		fastfill_small_area;
	int		pagefill_small_area;
	int		fastfill_height;
	int		fastfill_width;
	int		pagefill_height;
	int		pagefill_width;
	short		Pf_AlignTab[0x800];
} FFBDRIRec, *FFBDRIPtr;

#endif /* !(_FFB_DRISHARE_H) */
