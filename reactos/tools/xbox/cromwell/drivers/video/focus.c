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
 * VGA SoG/internal sync not yet implemented
*/
#include "focus.h"
//#include "encoder.h"

typedef struct _focus_pll_settings{
	long dotclock;
	int vga_htotal;
	int vga_vtotal;
	int tv_htotal;
	int tv_vtotal;
} focus_pll_settings;

static const unsigned char focus_defaults[0xc4] = {
	/*0x00*/ 0x00,0x00,0x00,0x00,0x80,0x02,0xaa,0x0a,
	/*0x08*/ 0x00,0x10,0x00,0x00,0x03,0x21,0x15,0x04,
	/*0x10*/ 0x00,0xe9,0x07,0x00,0x80,0xf5,0x20,0x00,
	/*0x18*/ 0xef,0x21,0x1f,0x00,0x03,0x03,0x00,0x00,
	/*0x20*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
	/*0x28*/ 0x0c,0x01,0x00,0x00,0x00,0x00,0x08,0x11,
	/*0x30*/ 0x00,0x0f,0x05,0xfe,0x0b,0x80,0x00,0x00,
	/*0x38*/ 0xa4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/*0x40*/ 0x2a,0x09,0x8a,0xcb,0x00,0x00,0x8d,0x00,
	/*0x48*/ 0x7c,0x3c,0x9a,0x2f,0x21,0x01,0x3f,0x00,
	/*0x50*/ 0x3e,0x03,0x17,0x21,0x1b,0x1b,0x24,0x9c,
	/*0x58*/ 0x01,0x3e,0x0f,0x0f,0x60,0x05,0xc8,0x00,
	/*0x60*/ 0x9d,0x04,0x9d,0x01,0x02,0x00,0x0a,0x05,
	/*0x68*/ 0x00,0x1a,0xff,0x03,0x1e,0x0f,0x78,0x00,
	/*0x70*/ 0x00,0xb1,0x04,0x15,0x49,0x10,0x00,0xa3,
	/*0x78*/ 0xc8,0x15,0x05,0x15,0x3e,0x03,0x00,0x20,
	/*0x80*/ 0x57,0x2f,0x07,0x00,0x00,0x08,0x00,0x00,
	/*0x88*/ 0x08,0x16,0x16,0x9c,0x03,0x00,0x00,0x00,
	/*0x90*/ 0x00,0x00,0xc4,0x48,0x00,0x00,0x00,0x00,
	/*0x98*/ 0x00,0x00,0x00,0x80,0x00,0x00,0xe4,0x00,
	/*0xa0*/ 0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,
	/*0xa8*/ 0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0x00,
	/*0xb0*/ 0x00,0x00,0xd7,0x05,0x00,0x00,0xf0,0x00,
	/*0xb8*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/*0xc0*/ 0x00,0x00,0xee,0x00
};

int focus_calc_hdtv_mode(
	xbox_hdtv_mode hdtv_mode,
	unsigned char pll_int,
	unsigned char * regs
	){
	memcpy(regs,focus_defaults,sizeof(focus_defaults));	
	/* Uncomment for HDTV 480p colour bars */
	//regs[0x0d]|=0x02;
	
	/* Turn on bridge bypass */
	regs[0x0a] |= 0x10;
	/* Turn on the HDTV clock, and turn off the SDTV one */	
	regs[0xa1] = 0x04;
	
	/* HDTV Hor start */
	regs[0xb8] = 0xbe;
	
	/*Set up video mode to HDTV, progressive, 
	 * and disable YUV matrix bypass */
	regs[0x92] = 0x1a;	
	regs[0x93] &= ~0x40;
	
	switch (hdtv_mode) {
		case HDTV_480p:
			/* PLL settings */
			regs[0x10] = 0x00;
			regs[0x11] = 0x00;
			regs[0x12] = 0x00;
			regs[0x13] = 0x00;
			regs[0x14] = 0x00;
			regs[0x15] = 0x00;
			regs[0x16] = 0x00;
			regs[0x17] = 0x00;
			regs[0x18] = 0xD7;
			regs[0x19] = 0x03;
			regs[0x1A] = 0x7C;
			regs[0x1B] = 0x00;
			regs[0x1C] = 0x07;
			regs[0x1D] = 0x07;
			/* Porches/HSync width/Luma offset */
			regs[0x94] = 0x3F;
			regs[0x95] = 0x2D;
			regs[0x96] = 0x3B;
			regs[0x97] = 0x00;
			regs[0x98] = 0x1B;
			regs[0x99] = 0x03;
			/* Colour scaling */
			regs[0xA2] = 0x4D;
			regs[0xA4] = 0x96;
			regs[0xA6] = 0x1D;
			regs[0xA8] = 0x58;
			regs[0xAA] = 0x8A;
			regs[0xAC] = 0x4A;
			break;
		case HDTV_720p:
			/* PLL settings */
			regs[0x10] = 0x00;
			regs[0x11] = 0x00;
			regs[0x12] = 0x00;
			regs[0x13] = 0x00;
			regs[0x14] = 0x00;
			regs[0x15] = 0x00;
			regs[0x16] = 0x00;
			regs[0x17] = 0x00;
			regs[0x18] = 0x3B;
			regs[0x19] = 0x04;
			regs[0x1A] = 0xC7;
			regs[0x1B] = 0x00;
			regs[0x1C] = 0x01;
			regs[0x1D] = 0x01;
			/* Porches/HSync width/Luma offset */
			regs[0x94] = 0x28;
			regs[0x95] = 0x46;
			regs[0x96] = 0xDC;
			regs[0x97] = 0x00;
			regs[0x98] = 0x2C;
			regs[0x99] = 0x06;
			/* Colour scaling */
			regs[0xA2] = 0x36;
			regs[0xA4] = 0xB7;
			regs[0xA6] = 0x13;
			regs[0xA8] = 0x58;
			regs[0xAA] = 0x8A;
			regs[0xAC] = 0x4A;
			/* HSync timing invert - needed to centre picture */
			regs[0x93] |= 0x01;
			
			break;
		case HDTV_1080i:
			/* PLL settings */
			regs[0x10] = 0x00;
			regs[0x11] = 0x00;
			regs[0x12] = 0x00;
			regs[0x13] = 0x00;
			regs[0x14] = 0x00;
			regs[0x15] = 0x00;
			regs[0x16] = 0x00;
			regs[0x17] = 0x00;
			regs[0x18] = 0x3B;
			regs[0x19] = 0x04;
			regs[0x1A] = 0xC7;
			regs[0x1B] = 0x00;
			regs[0x1C] = 0x01;
			regs[0x1D] = 0x01;
			/* Porches/HSync width/Luma offset */
			regs[0x94] = 0x2C;
			regs[0x95] = 0x2C;
			regs[0x96] = 0x58;
			regs[0x97] = 0x00;
			regs[0x98] = 0x6C;
			regs[0x99] = 0x08;
			/* Colour scaling */
			regs[0xA2] = 0x36;
			regs[0xA4] = 0xB7;
			regs[0xA6] = 0x13;
			regs[0xA8] = 0x58;
			regs[0xAA] = 0x8A;
			regs[0xAC] = 0x4A;
			/* Set mode to interlaced */
			regs[0x92] |= 0x80;
			break;
	}
	return 1;
}

int focus_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out)
{
	unsigned char b;
	char* regs = riva_out->encoder_mode;
	int tv_htotal, tv_vtotal, tv_vactive, tv_hactive;
	int vga_htotal, vga_vtotal;
	int vsc, hsc;

	long dotclock;
	focus_pll_settings pll_settings;
	
	memcpy(regs,focus_defaults,sizeof(focus_defaults));
	
	/* Uncomment for SDTV colour bars */
	//regs[0x45]=0x02;
	
	switch(mode->tv_encoding) {
		case TV_ENC_NTSC:
			tv_vtotal=525;
			tv_vactive=480;			
			tv_hactive = 710;
			tv_htotal  = 858;
			regs[0x0d] &= ~0x01;
			regs[0x40] = 0x21;
			regs[0x41] = 0xF0;
			regs[0x42] = 0x7C;
			regs[0x43] = 0x1F;
			regs[0x49] = 0x44;
			regs[0x4a] = 0x76;
			regs[0x4b] = 0x3B;
			regs[0x4c] = 0x00;
			regs[0x60] = 0x89;
			regs[0x62] = 0x89;
			regs[0x69] = 0x16;
			regs[0x6C] = 0x20;
			regs[0x74] = 0x04;		
			regs[0x75] = 0x10;
			regs[0x80] = 0x67; 
			regs[0x81] = 0x21; 
			regs[0x82] = 0x0C;
			regs[0x83] = 0x18;
			regs[0x86] = 0x18;
			regs[0x89] = 0x13;
			regs[0x8A] = 0x13;
			break;
		case TV_ENC_PALBDGHI:
			tv_vtotal = 625;
			tv_vactive = 576;
			tv_hactive = 702;
			tv_htotal = 864;
			break;
		default:
			/* Default to PAL */
			tv_vtotal = 625;
			tv_vactive = 576;
			tv_hactive = 702;
			tv_htotal = 864;
			break;
	}

	/* Video control  - set to RGB input*/
	b = (regs[0x92] &= ~0x04);
	regs[0x92] = (b|= 0x01);
	regs[0x93] &= ~0x40;
	/* Colour scaling */
	regs[0xA2] = 0x4D;
	regs[0xA4] = 0x96;
	regs[0xA6] = 0x1D;
	regs[0xA8] = 0xA0;
	regs[0xAA] = 0xDB;
	regs[0xAC] = 0x7E;

	switch(mode->av_type) {
		case AV_SVIDEO:
			/* COMP_YUV - set to 1 to output YUV */	
			regs[0x47] |= 0x04;
			/* VID_MODE to 0 - SVIDEO */
			regs[0x92] &= ~0x01;
			break;
		default:
			/*Nothing as yet */
			break;
	}
	
	tv_vactive = tv_vactive * (1.0f-mode->voc);
	vga_vtotal = mode->yres * ((float)tv_vtotal/tv_vactive);
	vga_htotal = mode->xres * 1.25f;
	tv_hactive = tv_hactive * (1.0f-mode->hoc);

	regs[0x04] = (mode->xres+64)&0xFF;
	regs[0x05] = ((mode->xres+64)>>8)&0xFF;

	if (tv_vtotal>vga_vtotal) {
		/* Upscaling */
		vsc = ((((float)tv_vtotal/(float)vga_vtotal)-1)*65536);
		/* For upscaling, adjust FIFO_LAT (FIFO latency) */
		regs[0x38] = 0x82;
	}
	else {
		/* Downscaling */
		vsc = ((((float)tv_vtotal/(float)vga_vtotal))*65536);
	}
	regs[0x06] = (vsc)&0xFF;
	regs[0x07] = (vsc>>8)&0xFF;

	hsc = 128*((float)tv_hactive/(float)mode->xres-1);
	if (tv_hactive > mode->xres) {
		/* Upscaling */
		regs[0x08] = 0;
		regs[0x09] = hsc&0xFF;
	}
	else {  /* Downscaling */
		hsc = 256 + hsc;
		regs[0x08] = hsc&0xFF;
		regs[0x09] = 0;
	}

	//PLL calculations
	if (mode->tv_encoding==TV_ENC_NTSC) dotclock = vga_htotal * vga_vtotal * 59.94;
	else dotclock = vga_htotal * vga_vtotal * 50;

	pll_settings.dotclock = dotclock;
	pll_settings.vga_htotal = vga_htotal;
	pll_settings.vga_vtotal = vga_vtotal;
	pll_settings.tv_htotal = tv_htotal;
	pll_settings.tv_vtotal = tv_vtotal;
	
	if (!focus_calc_pll_settings(&pll_settings,regs)) {
		//Unable to calculate a valid PLL solution	
		return 1;
	}

	/* Guesswork */
	riva_out->ext.vsyncstart = vga_vtotal * 0.95;
	riva_out->ext.hsyncstart = vga_htotal * 0.95;
	
	riva_out->ext.width = mode->xres;
	riva_out->ext.height = mode->yres;
	riva_out->ext.htotal = vga_htotal - 1;
	riva_out->ext.vend = mode->yres - 1;
	riva_out->ext.vtotal = vga_vtotal- 1;
	riva_out->ext.vcrtc = mode->yres - 1;
	riva_out->ext.vsyncend = riva_out->ext.vsyncstart + 3;
        riva_out->ext.vvalidstart = 0;
	riva_out->ext.vvalidend = mode->yres - 1;
	riva_out->ext.hend = mode->xres + 7 ;
	riva_out->ext.hcrtc = mode->xres - 1;
        riva_out->ext.hsyncend = riva_out->ext.hsyncstart + 32;
        riva_out->ext.hvalidstart = 0;
        riva_out->ext.hvalidend = mode->xres - 1;
	riva_out->ext.crtchdispend = mode->xres;
        riva_out->ext.crtcvstart = mode->yres + 32;
	//increased from 32
	riva_out->ext.crtcvtotal = mode->yres + 64;

	return 1;
}

int focus_calc_pll_settings(focus_pll_settings *settings, char *regs) {
        int m, n, p;
	long dotclock = (*settings).dotclock;
	int pll_multiplier;
	long ncon, ncod;
	
	ncon = (*settings).vga_htotal * (*settings).vga_vtotal;

	//Multipliers between 1 and 6 are the limit as output clock cant be >150MHz
	//The lower the multiplier, the more stable the PLL (theoretically)
	for (pll_multiplier=4; pll_multiplier<6; pll_multiplier++) {
		long nco_out_clk;
		ncod = (*settings).tv_htotal * (*settings).tv_vtotal * pll_multiplier;
		//NCO output clock is the reference clock (27MHz) multiplied by
		//the ncon/ncod fraction.
		nco_out_clk = 27000000*(ncon/(float)ncod);
		for (n=2; n<270;++n) {
			//PLL input clock is NCO output clock divided by N
			//Valid range is 100kHz to 1000kHz
			long pll_in_clk = nco_out_clk/n;
			if ( pll_in_clk >=100000 && pll_in_clk <=1000000) {
				for (m=2; m<3000;++m) {
					//PLL output clock is PLL input clock multiplied
					//by M. Valid range is 100MHz to 300MHz
					long pll_out_clk = pll_in_clk * m;
					if (pll_out_clk >=100000000 && pll_out_clk <= 300000000) {
						//Output clocks are PLL output clock divided by P.
						//Valid range is anything LESS than 150MHz, but
						//it must match the incoming pixel clock rate.
						long output_clk = pll_out_clk/pll_multiplier;
						if (output_clk == dotclock || output_clk+1 == dotclock || output_clk-1 == dotclock) {
							//Got it - the pll is now correctly aligned
							//Set up the PLL registers
							regs[0x10] = (ncon)&0xFF;
							regs[0x11] = (ncon>>8)&0xFF ;
							regs[0x12] = (ncon>>16)&0xFF ;
							regs[0x13] = (ncon>>24)&0xFF ;
							regs[0x14] = (ncod)&0xFF ;
							regs[0x15] = (ncod>>8)&0xFF ;
							regs[0x16] = (ncod>>16)&0xFF ;
							regs[0x17] = (ncod>>24)&0xFF ;

							regs[0x18] = (m-17)&0xFF;
							regs[0x19] = ((m-17)>>8)&0xFF;
							regs[0x1A] = (n-1)&0xFF ;
							regs[0x1B] = ((n-1)>>8)&0xFF ;
							regs[0x1C] = (pll_multiplier-1)&0xFF;
							regs[0x1D] = (pll_multiplier-1)&0xFF;
							return 1;
						}
					}
				}
			}
		}	
	}
	//Seems no valid solution was possible 
	return 0;
}

