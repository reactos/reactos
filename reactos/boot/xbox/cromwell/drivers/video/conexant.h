/*
 * linux/drivers/video/riva/conexant.h - Xbox driver for conexant chip
 *
 * Maintainer: Oliver Schwartz <Oliver.Schwartz@gmx.de>
 *
 * Contributors:
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Known bugs and issues:
 *
 *      none
 */

#ifndef conexant_h
#define conexant_h

//#include <linux/xboxfbctl.h>
//include "xboxfb.h"
#include "boot.h"
#include "encoder.h"

int conexant_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out);
int conexant_calc_vga_mode(xbox_av_type av_type, unsigned char pll_int, unsigned char * mode_out);
int conexant_calc_hdtv_mode(xbox_hdtv_mode hdtv_mode, unsigned char pll_int, unsigned char * mode_out);

#endif
