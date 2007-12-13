/*
 * linux/drivers/video/xbox/xcalibur.c - Xbox driver for Xcalibur encoder
 *
 * Maintainer: David Pye (dmp) <dmp@davidmpye.dyndns.org>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Known bugs and issues:
 *
 * It doesnt DO anything yet!
*/
#include "xcalibur.h"
#include "encoder.h"

typedef struct _xcalibur_sync {
	U032	htotal;
	U032	vtotal;
	U032	hsyncstart;
	U032 	hsyncend;
	U032	vsyncstart;
	U032	vsyncend;
} XCALIBUR_SYNC;

static const XCALIBUR_SYNC xcalibur_sync[2][1] = {
{	// NTSC MODES
	{780, 525, 682, 684, 486, 488},
},
{	// PAL MODES
	{800, 520, 702, 726, 480, 490},
}
};

int xcalibur_calc_hdtv_mode(
	xbox_hdtv_mode hdtv_mode,
	int dotClock,
	unsigned char * regs
	){
	return 1;
}

int xcalibur_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out, int tv_encoding)
{

	XCALIBUR_SYNC *sync;
	int syncindex;
	
	switch(tv_encoding) {
		case TV_ENC_PALBDGHI:
			syncindex = 1;
			break;
		case TV_ENC_NTSC:
		default: // Default to NTSC
			syncindex = 0;
			break;
	}

	sync = (XCALIBUR_SYNC *)&xcalibur_sync[syncindex];

	riva_out->ext.vsyncstart = sync->vsyncstart + 1;
	riva_out->ext.hsyncstart = sync->hsyncstart + 1;
	
	riva_out->ext.width = mode->xres;
	riva_out->ext.height = mode->yres;
	riva_out->ext.htotal = sync->htotal - 1;
	riva_out->ext.vend = mode->yres - 1;
	riva_out->ext.vtotal = sync->vtotal- 1;
	riva_out->ext.vcrtc = mode->yres - 1;
	riva_out->ext.vsyncend = sync->vsyncend + 1;
	riva_out->ext.vvalidstart = 0;
	riva_out->ext.vvalidend = mode->yres - 1;
	riva_out->ext.hend = mode->xres - 1;
	riva_out->ext.hcrtc = 599;
	riva_out->ext.hsyncend = sync->hsyncend + 1;
	riva_out->ext.hvalidstart = 0;
	riva_out->ext.hvalidend = mode->xres - 1;

	return 1;
}
