/*
 * linux/drivers/video/riva/focus.c - Xbox driver for Focus encoder
 *
 * Maintainer: David Pye (dmp) <dmp@davidmpye.dyndns.org>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Known bugs and issues:
 *
 * none
 */


#ifndef focus_h_
#define focus_h_

#include "encoder.h"
//#include "xboxfb.h"

int focus_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out );
int focus_calc_hdtv_mode(xbox_hdtv_mode hdtv_mode, unsigned char pll_int, unsigned char * mode_out);
#endif
