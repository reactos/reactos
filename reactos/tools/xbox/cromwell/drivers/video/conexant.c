/*
 * linux/drivers/video/riva/conexant.c - Xbox driver for conexant chip
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

#include "conexant.h"
#include "focus.h"

#define ADR(x) (x / 2 - 0x17)

typedef struct {
	long v_activeo;
	long v_linesi;
	long h_clki;
	long h_clko;
	long h_blanki;
	long h_blanko;
	long v_blanki;
	long v_blanko;
	long vscale;
	double clk_ratio;
} xbox_tv_mode_parameter;


	// and here is all the video timing for every standard

static const conexant_video_parameter vidstda[] = {
	{ 3579545.00, 0.0000053, 0.00000782, 0.0000047, 0.000063555, 0.0000094, 0.000035667, 0.0000015, 243, 262.5, 0.0000092 },
	{ 3579545.00, 0.0000053, 0.00000782, 0.0000047, 0.000064000, 0.0000094, 0.000035667, 0.0000015, 243, 262.5, 0.0000092 },
	{ 4433618.75, 0.0000056, 0.00000785, 0.0000047, 0.000064000, 0.0000105, 0.000036407, 0.0000015, 288, 312.5, 0.0000105 },
	{ 4433618.75, 0.0000056, 0.00000785, 0.0000047, 0.000064000, 0.0000094, 0.000035667, 0.0000015, 288, 312.5, 0.0000092 },
	{ 3582056.25, 0.0000056, 0.00000811, 0.0000047, 0.000064000, 0.0000105, 0.000036407, 0.0000015, 288, 312.5, 0.0000105 },
	{ 3575611.88, 0.0000058, 0.00000832, 0.0000047, 0.000063555, 0.0000094, 0.000035667, 0.0000015, 243, 262.5, 0.0000092 },
	{ 4433619.49, 0.0000053, 0.00000755, 0.0000047, 0.000063555, 0.0000105, 0.000036407, 0.0000015, 243, 262.5, 0.0000092 }
};

static const unsigned char default_mode[] = {
	0x00,
	0x00, 0x28, 0x80, 0xE4, 0x00, 0x00, 0x80, 0x80,
	0x80, 0x13, 0xDA, 0x4B, 0x28, 0xA3, 0x9F, 0x25,
	0xA3, 0x9F, 0x25, 0x00, 0x00, 0x00, 0x00, 0x44,
	0xC7, 0x00, 0x00, 0x41, 0x35, 0x03, 0x46, 0x00,
	0x02, 0x00, 0x01, 0x60, 0x88, 0x8a, 0xa6, 0x68,
	0xc1, 0x2e, 0xf2, 0x27, 0x00, 0xb0, 0x0a, 0x0b,
	0x71, 0x5a, 0xe0, 0x36, 0x00, 0x50, 0x72, 0x1c,
	0x0d, 0x24, 0xf0, 0x58, 0x81, 0x49, 0x8c, 0x0c,
	0x8c, 0x79, 0x26, 0x52, 0x00, 0x24, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x9C, 0x9B, 0xC0, 0xC0, 0x19,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x57, 0x20,
	0x40, 0x6E, 0x7E, 0xF4, 0x51, 0x0F, 0xF1, 0x05,
	0xD3, 0x78, 0xA2, 0x25, 0x54, 0xA5, 0x00, 0x00
};

static const double pll_base = 13.5e6;

static void conexant_calc_blankings(
	xbox_video_mode * mode,
	xbox_tv_mode_parameter * param
);

static int conexant_calc_mode_params(
	xbox_video_mode * mode,
	xbox_tv_mode_parameter * param
);

static double fabs(double d) {
	if (d > 0) return d;
	else return -d;
}

int conexant_calc_vga_mode(
	xbox_av_type av_type,
	unsigned char pll_int,
	unsigned char * regs
){
	memset(regs, 0, NUM_CONEXANT_REGS);
	// Protect against overclocking
	if (pll_int > 36) {
		pll_int = 36; // 36 / 6 * 13.5 MHz = 81 MHz, just above the limit.
	}
	if (pll_int == 0) {
		pll_int = 1;  // 0 will cause a burnout ...
	}
	if (av_type == AV_VGA) {
		// use internal sync signals
		regs[ADR(0x2e)] = 0xbd; // HDTV_EN = 1, RPR_SYNC_DIS = 1, BPB_SYNC_DIS = 1, GY_SYNC_DIS=1, HD_SYNC_EDGE = 1, RASTER_SEL = 01
	}
	else {
 		// use sync on green
		regs[ADR(0x2e)] = 0xad; // HDTV_EN = 1, RPR_SYNC_DIS = 1, BPB_SYNC_DIS = 1, HD_SYNC_EDGE = 1, RASTER_SEL = 01
	}
	regs[ADR(0x32)] = 0x48; // DRVS = 2, IN_MODE[3] = 1;
	regs[ADR(0x3c)] = 0x80; // MCOMPY
	regs[ADR(0x3e)] = 0x80; // MCOMPU
	regs[ADR(0x40)] = 0x80; // MCOMPV
	regs[ADR(0xc6)] = 0x98; // IN_MODE = 24 bit RGB multiplexed
	regs[ADR(0x6c)] = 0x46; // FLD_MODE = 10, EACTIVE = 1, EN_SCART = 0, EN_REG_RD = 1
	regs[ADR(0x9c)] = 0x00; // PLL_FRACT
	regs[ADR(0x9e)] = 0x00; // PLL_FRACT
	regs[ADR(0xa0)] = pll_int; // PLL_INT
	regs[ADR(0xba)] = 0x28; // SLAVER = 1, DACDISD = 1
	regs[ADR(0xce)] = 0xe1; // OUT_MUXA = 01, OUT_MUXB = 00, OUT_MUXC = 10, OUT_MUXD = 11
	regs[ADR(0xd6)] = 0x0c; // OUT_MODE = 11 (RGB / SCART / HDTV)

	return 1;
}

int conexant_calc_hdtv_mode(
	xbox_hdtv_mode hdtv_mode,
	unsigned char pll_int,
	unsigned char * regs
){
	memset(regs, 0, NUM_CONEXANT_REGS);
	// Protect against overclocking
	if (pll_int > 36) {
		pll_int = 36; // 36 / 6 * 13.5 MHz = 81 MHz, just above the limit.
	}
	if (pll_int == 0) {
		pll_int = 1;  // 0 will cause a burnout ...
	}
	switch (hdtv_mode) {
		case HDTV_480p:
			// use sync on green
			regs[ADR(0x2e)] = 0xed; // HDTV_EN = 1, RGB2PRPB = 1, RPR_SYNC_DIS = 1, BPB_SYNC_DIS = 1, HD_SYNC_EDGE = 1, RASTER_SEL = 01
			regs[ADR(0x32)] = 0x48; // DRVS = 2, IN_MODE[3] = 1;
			regs[ADR(0x3e)] = 0x45; // MCOMPU
			regs[ADR(0x40)] = 0x51; // MCOMPV
			break;
		case HDTV_720p:
			// use sync on green
			regs[ADR(0x2e)] = 0xea; // HDTV_EN = 1, RGB2PRPB = 1, RPR_SYNC_DIS = 1, BPB_SYNC_DIS = 1, HD_SYNC_EDGE = 1, RASTER_SEL = 01
			regs[ADR(0x32)] = 0x49; // DRVS = 2, IN_MODE[3] = 1, CSC_SEL=1;
			regs[ADR(0x3e)] = 0x45; // MCOMPU
			regs[ADR(0x40)] = 0x51; // MCOMPV
			break;
		case HDTV_1080i:
			// use sync on green
			regs[ADR(0x2e)] = 0xeb; // HDTV_EN = 1, RGB2PRPB = 1, RPR_SYNC_DIS = 1, BPB_SYNC_DIS = 1, HD_SYNC_EDGE = 1, RASTER_SEL = 01
			regs[ADR(0x32)] = 0x49; // DRVS = 2, IN_MODE[3] = 1, CSC_SEL=1;
			regs[ADR(0x3e)] = 0x48; // MCOMPU
			regs[ADR(0x40)] = 0x5b; // MCOMPV
			break;
	}
	regs[ADR(0x3c)] = 0x80; // MCOMPY
	regs[ADR(0xa0)] = pll_int; // PLL_INT
	regs[ADR(0xc6)] = 0x98; // IN_MODE = 24 bit RGB multiplexed
	regs[ADR(0x6c)] = 0x46; // FLD_MODE = 10, EACTIVE = 1, EN_SCART = 0, EN_REG_RD = 1
	regs[ADR(0x9c)] = 0x00; // PLL_FRACT
	regs[ADR(0x9e)] = 0x00; // PLL_FRACT
	regs[ADR(0xba)] = 0x28; // SLAVER = 1, DACDISD = 1
	regs[ADR(0xce)] = 0xe1; // OUT_MUXA = 01, OUT_MUXB = 00, OUT_MUXC = 10, OUT_MUXD = 11
	regs[ADR(0xd6)] = 0x0c; // OUT_MODE = 11 (RGB / SCART / HDTV)

	return 1;
}

int conexant_calc_mode(xbox_video_mode * mode, struct riva_regs * riva_out)
{
	unsigned char b;
	unsigned int m = 0;
	double dPllOutputFrequency;
	xbox_tv_mode_parameter param;
	char* regs = riva_out->encoder_mode;

	if (conexant_calc_mode_params(mode, &param))
	{
		// copy default mode settings
		memcpy(regs,default_mode,sizeof(default_mode));

		regs[ADR(0x32)] = 0x28; // DRVS = 1, IN_MODE[3] = 1;

		// H_CLKI
		b=regs[ADR(0x8e)]&(~0x07);
		regs[ADR(0x8e)] = ((param.h_clki>>8)&0x07)|b;
		regs[ADR(0x8a)] = ((param.h_clki)&0xff);
		// H_CLKO
		b=regs[ADR(0x86)]&(~0x0f);
		regs[ADR(0x86)] = ((param.h_clko>>8)&0x0f)|b;
		regs[ADR(0x76)] = ((param.h_clko)&0xff);
		// V_LINESI
		b=regs[ADR(0x38)]&(~0x02);
		regs[ADR(0x38)] = ((param.v_linesi>>9)&0x02)|b;
		b=regs[ADR(0x96)]&(~0x03);
		regs[ADR(0x96)] = ((param.v_linesi>>8)&0x03)|b;
		regs[ADR(0x90)] = ((param.v_linesi)&0xff);
		// V_ACTIVEO
		/* TODO: Absolutely not sure about other modes than plain NTSC / PAL */
		switch(mode->tv_encoding) {
			case TV_ENC_NTSC:
			case TV_ENC_NTSC60:
			case TV_ENC_PALM:
			case TV_ENC_PAL60:
				m=param.v_activeo + 1;
				break;
			case TV_ENC_PALBDGHI:
				m=param.v_activeo + 2;
				break;
			default:
				m=param.v_activeo + 2;
				break;
		}
		b=regs[ADR(0x86)]&(~0x80);
		regs[ADR(0x86)] = ((m>>1)&0x80)|b;
		regs[ADR(0x84)] = ((m)&0xff);
		// H_ACTIVE
		b=regs[ADR(0x86)]&(~0x70);
		regs[ADR(0x86)] = (((mode->xres + 5)>>4)&0x70)|b;
		regs[ADR(0x78)] = ((mode->xres + 5)&0xff);
		// V_ACTIVEI
		b=regs[ADR(0x96)]&(~0x0c);
		regs[ADR(0x96)] = ((mode->yres>>6)&0x0c)|b;
		regs[ADR(0x94)] = ((mode->yres)&0xff);
		// H_BLANKI
		b=regs[ADR(0x38)]&(~0x01);
		regs[ADR(0x38)] = ((param.h_blanki>>9)&0x01)|b;
		b=regs[ADR(0x8e)]&(~0x08);
		regs[ADR(0x8e)] = ((param.h_blanki>>5)&0x08)|b;
		regs[ADR(0x8c)] = ((param.h_blanki)&0xff);
		// H_BLANKO
		b=regs[ADR(0x9a)]&(~0xc0);
		regs[ADR(0x9a)] = ((param.h_blanko>>2)&0xc0)|b;
		regs[ADR(0x80)] = ((param.h_blanko)&0xff);

		// V_SCALE
		b=regs[ADR(0x9a)]&(~0x3f);
		regs[ADR(0x9a)] = ((param.vscale>>8)&0x3f)|b;
		regs[ADR(0x98)] = ((param.vscale)&0xff);
		// V_BLANKO
		regs[ADR(0x82)] = ((param.v_blanko)&0xff);
		// V_BLANKI
		regs[ADR(0x92)] = ((param.v_blanki)&0xff);
		{
			unsigned int dwPllRatio, dwFract, dwInt;
			// adjust PLL
			dwPllRatio = (int)(6.0 * ((double)param.h_clko / vidstda[mode->tv_encoding].m_dSecHsyncPeriod) *
				param.clk_ratio * 0x10000 / pll_base + 0.5);
			dwInt = dwPllRatio / 0x10000;
			dwFract = dwPllRatio - (dwInt * 0x10000);
			b=regs[ADR(0xa0)]&(~0x3f);
			regs[ADR(0xa0)] = ((dwInt)&0x3f)|b;
			regs[ADR(0x9e)] = ((dwFract>>8)&0xff);
			regs[ADR(0x9c)] = ((dwFract)&0xff);
			// recalc value
			dPllOutputFrequency = ((double)dwInt + ((double)dwFract)/65536.0)/(6 * param.clk_ratio / pll_base);
			// enable 3:2 clocking mode
			b=regs[ADR(0x38)]&(~0x20);
			if (param.clk_ratio > 1.1) {
				b |= 0x20;
			}
			regs[ADR(0x38)] = b;

			// update burst start position
			m=(vidstda[mode->tv_encoding].m_dSecBurstStart) * dPllOutputFrequency + 0.5;
			b=regs[ADR(0x38)]&(~0x04);
			regs[ADR(0x38)] = ((m>>6)&0x04)|b;
			regs[ADR(0x7c)] = (m&0xff);
			// update burst end position (note +128 is in hardware)
			m=(vidstda[mode->tv_encoding].m_dSecBurstEnd) * dPllOutputFrequency + 0.5;
			if(m<128) m=128;
			b=regs[ADR(0x38)]&(~0x08);
			regs[ADR(0x38)] = (((m-128)>>5)&0x08)|b;
			regs[ADR(0x7e)] = ((m-128)&0xff);
			// update HSYNC width
			m=(vidstda[mode->tv_encoding].m_dSecHsyncWidth) * dPllOutputFrequency + 0.5;
			regs[ADR(0x7a)] = ((m)&0xff);
		}
		// adjust Subcarrier generation increment
		{
			unsigned int dwSubcarrierIncrement = (unsigned int) (
				(65536.0 * 65536.0) * (
					vidstda[mode->tv_encoding].m_dHzBurstFrequency
					* vidstda[mode->tv_encoding].m_dSecHsyncPeriod
					/ (double)param.h_clko
				) + 0.5
			);
			regs[ADR(0xae)] = (dwSubcarrierIncrement&0xff);
			regs[ADR(0xb0)] = ((dwSubcarrierIncrement>>8)&0xff);
			regs[ADR(0xb2)] = ((dwSubcarrierIncrement>>16)&0xff);
			regs[ADR(0xb4)] = ((dwSubcarrierIncrement>>24)&0xff);
		}
		// adjust WSS increment
		{
			unsigned int dwWssIncrement = 0;

			switch(mode->tv_encoding) {
				case TV_ENC_NTSC:
				case TV_ENC_NTSC60:
					dwWssIncrement=(unsigned int) ((1048576.0 / ( 0.000002234 * dPllOutputFrequency))+0.5);
					break;
				case TV_ENC_PALBDGHI:
				case TV_ENC_PALN:
				case TV_ENC_PALNC:
				case TV_ENC_PALM:
				case TV_ENC_PAL60:
					dwWssIncrement=(unsigned int) ((1048576.0 / ( 0.0000002 * dPllOutputFrequency))+0.5);
					break;
				default:
					break;
				}

			regs[ADR(0x66)] = (dwWssIncrement&0xff);
			regs[ADR(0x68)] = ((dwWssIncrement>>8)&0xff);
			regs[ADR(0x6a)] = ((dwWssIncrement>>16)&0xf);
		}
		// set mode register
		b=regs[ADR(0xa2)]&(0x41);
		switch(mode->tv_encoding) {
				case TV_ENC_NTSC:
					b |= 0x0a; // SETUP + VSYNC_DUR
					break;
				case TV_ENC_NTSC60:
					b |= 0x08; // VSYNC_DUR
					break;
				case TV_ENC_PALBDGHI:
				case TV_ENC_PALNC:
						b |= 0x24; // PAL_MD + 625LINE
					break;
				case TV_ENC_PALN:
					b |= 0x2e; // PAL_MD + SETUP + 625LINE + VSYNC_DUR
					break;
				case TV_ENC_PALM:
					b |= 0x2a; // PAL_MD + SETUP + VSYNC_DUR
					break;
				case TV_ENC_PAL60:
					b |= 0x28; // PAL_MD + VSYNC_DUR
					break;
				default:
					break;
		}
		regs[ADR(0xa2)] = b;
		regs[ADR(0xc6)] = 0x98; // IN_MODE = 24 bit RGB multiplexed
		switch(mode->av_type) {
			case AV_COMPOSITE:
			case AV_SVIDEO:
				regs[ADR(0x2e)] |= 0x40; // RGB2YPRPB = 1
				regs[ADR(0x6c)] = 0x46; // FLD_MODE = 10, EACTIVE = 1, EN_SCART = 0, EN_REG_RD = 1
				regs[ADR(0x5a)] = 0x00; // Y_OFF (Brightness)
				regs[ADR(0xa4)] = 0xe5; // SYNC_AMP
				regs[ADR(0xa6)] = 0x74; // BST_AMP
				regs[ADR(0xba)] = 0x24; // SLAVER = 1, DACDISC = 1
				regs[ADR(0xce)] = 0x19; // OUT_MUXA = 01, OUT_MUXB = 10, OUT_MUXC = 10, OUT_MUXD = 00
				regs[ADR(0xd6)] = 0x00; // OUT_MODE = 00 (CVBS)
				break;
			case AV_SCART_RGB:
				regs[ADR(0x6c)] = 0x4e; // FLD_MODE = 10, EACTIVE = 1, EN_SCART = 1, EN_REG_RD = 1
				regs[ADR(0x5a)] = 0xff; // Y_OFF (Brightness)
				regs[ADR(0xa4)] = 0xe7; // SYNC_AMP
				regs[ADR(0xa6)] = 0x77; // BST_AMP
				regs[ADR(0xba)] = 0x20; // SLAVER = 1, enable all DACs
				regs[ADR(0xce)] = 0xe1; // OUT_MUXA = 01, OUT_MUXB = 00, OUT_MUXC = 10, OUT_MUXD = 11
				regs[ADR(0xd6)] = 0x0c; // OUT_MODE = 11 (RGB / SCART / HDTV)
				break;
			default:
				break;
		}
		riva_out->ext.vend = mode->yres - 1;
		riva_out->ext.vtotal = param.v_linesi - 1;
		riva_out->ext.vcrtc = mode->yres - 1;
		riva_out->ext.vsyncstart = param.v_linesi - param.v_blanki + 1;
		riva_out->ext.vsyncend = riva_out->ext.vsyncstart + 3;
		riva_out->ext.vvalidstart = 0;
		riva_out->ext.vvalidend = mode->yres - 1;
		riva_out->ext.hend = mode->xres + 7;
		riva_out->ext.htotal = param.h_clki - 1;
		riva_out->ext.hcrtc = mode->xres - 1;
		riva_out->ext.hsyncstart = param.h_clki - param.h_blanki - 7;
		riva_out->ext.hsyncend = riva_out->ext.hsyncstart + 32;
		riva_out->ext.hvalidstart = 0;
		riva_out->ext.hvalidend = mode->xres - 1;
		riva_out->ext.crtchdispend = mode->xres + 8;
		riva_out->ext.crtcvstart = mode->yres + 32;
		riva_out->ext.crtcvtotal = param.v_linesi + 32;
		return 1;
	}
	else
	{
		return 0;
	}
}

static int conexant_calc_mode_params(
	xbox_video_mode * mode,
	xbox_tv_mode_parameter * param
){
	const double dMinHBT = 2.5e-6; // 2.5uSec time for horizontal syncing
	const double invalidMetric = 1000;

	/* algorithm shamelessly ripped from nvtv/calc_bt.c */
	double dTempVOC = 0;
	double dTempHOC = 0;
	double dBestMetric = invalidMetric;
	double dTempVSR = 0;
	double dBestVSR = 0;
	double dTempCLKRATIO = 1;
	double dBestCLKRATIO = 1;
	unsigned int  minTLI = 0;
	unsigned int  maxTLI = 0;
	unsigned int  tempTLI = 0;
	unsigned int  bestTLI = 0;
	unsigned int  minHCLKO = 0;
	unsigned int  maxHCLKO = 0;
	unsigned int  minHCLKI = 0;
	unsigned int  tempHCLKI = 0;
	unsigned int  bestHCLKI = 0;
	int    actCLKRATIO;
	unsigned int  dTempHCLKO = 0;
	double dTempVACTIVEO = 0;
	double dDelta = 0;
	double dMetric = 0;
	double alo =  vidstda[mode->tv_encoding].m_dwALO;
	double tlo =  vidstda[mode->tv_encoding].m_TotalLinesOut;
	double tto = vidstda[mode->tv_encoding].m_dSecHsyncPeriod;
	double ato = tto - (vidstda[mode->tv_encoding].m_dSecBlankBeginToHsync + vidstda[mode->tv_encoding].m_dSecActiveBegin);

	/* Range to search */
	double dMinHOC = mode->hoc - 0.02;
	double dMaxHOC = mode->hoc + 0.02;
	double dMinVOC = mode->voc - 0.02;
	double dMaxVOC = mode->voc + 0.02;

	if (dMinHOC < 0) dMinHOC = 0;
	if (dMinVOC < 0) dMinVOC = 0;

	minTLI= (unsigned int)(mode->yres / ((1 - dMinVOC) * alo) * tlo);
	maxTLI = min((unsigned int)(mode->yres / ((1 - dMaxVOC) * alo) * tlo), (unsigned int)1023);
	minHCLKO = (unsigned int) ((mode->xres * 2) /
				((1 - dMinHOC) * (ato / tto)));
	maxHCLKO = (unsigned int) ((mode->xres * 2) /
				((1 - dMaxHOC) * (ato / tto)));
	for (actCLKRATIO = 0; actCLKRATIO <= 1; actCLKRATIO++)
	{
		dTempCLKRATIO = 1.0;
		if (actCLKRATIO) dTempCLKRATIO = 3.0/2.0;
		for(tempTLI = minTLI; tempTLI <= maxTLI; tempTLI++)
		{
			dTempVSR = (double)tempTLI / tlo;
			dTempVACTIVEO = (int)((((double)mode->yres * tlo) +
						(tempTLI - 1)) / tempTLI);
			dTempVOC = 1 - dTempVACTIVEO / alo;

			for(dTempHCLKO = minHCLKO; dTempHCLKO <= maxHCLKO; dTempHCLKO++)
			{
				tempHCLKI = (unsigned int)((dTempHCLKO * dTempCLKRATIO) * (tlo / tempTLI) + 0.5);
				minHCLKI = ((dMinHBT / tto) * tempHCLKI) + mode->xres;
				// check if solution is valid
				if ((fabs((double)(tempTLI * tempHCLKI) - (tlo * dTempHCLKO * dTempCLKRATIO)) < 1e-3) &&
					(tempHCLKI >= minHCLKI) && (tempHCLKI < 2048))
				{
					dTempHOC = 1 - (((double)mode->xres / ((double)dTempHCLKO / 2)) /
						(ato / tto));
					dDelta = fabs(dTempHOC - mode->hoc) + fabs(dTempVOC - mode->voc);
					dMetric = ((dTempHOC - mode->hoc) * (dTempHOC - mode->hoc)) +
						((dTempVOC - mode->voc) * (dTempVOC - mode->voc)) +
						(2 * dDelta * dDelta);
					if(dMetric < dBestMetric)
					{
						dBestVSR = dTempVSR;
						dBestMetric = dMetric;
						bestTLI = tempTLI;
						bestHCLKI = tempHCLKI;
						dBestCLKRATIO = dTempCLKRATIO;
					}
				} /* valid solution */
			} /* dTempHCLKO loop */
		} /* tempTLI loop */
	} /* CLKRATIO loop */

	if(dBestMetric != invalidMetric)
	{
		param->v_linesi = bestTLI;
		param->h_clki = bestHCLKI;
		param->clk_ratio = dBestCLKRATIO;
		param->v_activeo = (unsigned int)(
			(
				(mode->yres * vidstda[mode->tv_encoding].m_TotalLinesOut)
				+ param->v_linesi - 1
			) / param->v_linesi
		);
		param->h_clko = (unsigned int)(
			(
				(param->v_linesi * param->h_clki) /
				(vidstda[mode->tv_encoding].m_TotalLinesOut * param->clk_ratio)
			)
			+ 0.5
		);
		conexant_calc_blankings(mode, param);
		return 1;
	}
	else
	{
		return 0;
	}
}

static void conexant_calc_blankings(
	xbox_video_mode * mode,
	xbox_tv_mode_parameter * param
){
	double dTotalHBlankI;
	double dFrontPorchIn;
	double dFrontPorchOut;
	double dMinFrontPorchIn;
	double dBackPorchIn;
	double dBackPorchOut;
	double dTotalHBlankO;
	double dHeadRoom;
	double dMaxHsyncDrift;
	double dFifoMargin;
	double vsrq;
	double dMaxHR;
	double tlo =  vidstda[mode->tv_encoding].m_TotalLinesOut;
	const int MFP = 14; // Minimum front porch
	const int MBP = 4;  // Minimum back porch
	const int FIFO_SIZE = 1024;
	double vsr = (double)param->v_linesi / vidstda[mode->tv_encoding].m_TotalLinesOut;

	// H_BLANKO
	param->h_blanko = 2 * (int)(
		vidstda[mode->tv_encoding].m_dSecImageCentre / (2 * vidstda[mode->tv_encoding].m_dSecHsyncPeriod) *
		param->h_clko
		+ 0.5
	) - mode->xres + 15;

	// V_BLANKO
	switch (mode->tv_encoding) {
		case TV_ENC_NTSC:
		case TV_ENC_NTSC60:
		case TV_ENC_PAL60:
		case TV_ENC_PALM:
			param->v_blanko = (int)( 140 - ( param->v_activeo / 2.0 ) + 0.5 );
			break;
		default:
			param->v_blanko = (int)( 167 - ( param->v_activeo / 2.0 ) + 0.5 );
			break;
	}

	// V_BLANKI
	vsrq = ( (int)( vsr * 4096.0 + .5 ) ) / 4096.0;
	param->vscale = (int)( ( vsr - 1 ) * 4096 + 0.5 );
	if( vsrq < vsr )
	{
	// These calculations are in units of dHCLKO
		dMaxHsyncDrift = ( vsrq - vsr ) * tlo / vsr * param->h_clko;
		dMinFrontPorchIn = MFP / ( (double)param->h_clki * vsr ) * param->h_clko;
		dFrontPorchOut = param->h_clko - param->h_blanko - mode->xres * 2;
		dFifoMargin = ( FIFO_SIZE - mode->xres ) * 2;

		// Check for fifo overflow
		if( dFrontPorchOut + dFifoMargin < -dMaxHsyncDrift + dMinFrontPorchIn )
		{
			dTotalHBlankO = param->h_clko - mode->xres * 2;
			dTotalHBlankI = ( (double)param->h_clki - (double)mode->xres ) / param->h_clki / vsr * param->h_clko;

			// Try forcing the Hsync drift the opposite direction
			dMaxHsyncDrift = ( vsrq + 1.0 / 4096 - vsr ) * tlo / vsr * param->h_clko;

			// Check that fifo overflow and underflow can be avoided
			if( dTotalHBlankO + dFifoMargin >= dTotalHBlankI + dMaxHsyncDrift )
			{
				vsrq = vsrq + 1.0 / 4096;
				param->vscale = (int)( ( vsrq - 1 ) * 4096 );
			}

			// NOTE: If fifo overflow and underflow can't be avoided,
			//       alternative overscan compensation ratios should
			//       be selected and all calculations repeated.  If
			//       that is not feasible, the calculations for
			//       H_BLANKI below will delay the overflow or under-
			//       flow as much as possible, to minimize the visible
			//       artifacts.
		}
	}

	param->v_blanki = (int)( ( param->v_blanko - 1 ) * vsrq );

	// H_BLANKI

	// These calculations are in units of dHCLKI
	dTotalHBlankI = param->h_clki - mode->xres;
	dFrontPorchIn = max( (double)MFP, min( dTotalHBlankI / 8.0, dTotalHBlankI - (double)MBP ) );
	dBackPorchIn = dTotalHBlankI - dFrontPorchIn;
	dMaxHsyncDrift = ( vsrq - vsr ) * tlo * param->h_clki;
	dTotalHBlankO = ( param->h_clko - mode->xres * 2.0 ) / param->h_clko * vsr * param->h_clki;
	dBackPorchOut = ((double)param->h_blanko) / (double)param->h_clko * vsr * param->h_clki;
	dFrontPorchOut = dTotalHBlankO - dBackPorchOut;
	dFifoMargin = ( FIFO_SIZE - mode->xres ) * 2.0 / param->h_clko * vsr * param->h_clki;
	// This may be excessive, but is adjusted by the code.
	dHeadRoom = 32.0;

	// Check that fifo overflow and underflow can be avoided
	if( ( dTotalHBlankO + dFifoMargin ) >= ( dTotalHBlankI + fabs( dMaxHsyncDrift ) ) )
	{
		dMaxHR = ( dTotalHBlankO + dFifoMargin ) - ( dTotalHBlankI - fabs( dMaxHsyncDrift ) );
		if( dMaxHR < ( dHeadRoom * 2.0 ) )
		{
			dHeadRoom = (int)( dMaxHR / 2.0);
		}

		// Check for overflow
		if( ( ( dFrontPorchOut + dFifoMargin ) - dHeadRoom ) < ( dFrontPorchIn - min( dMaxHsyncDrift, 0.0 ) ) )
		{
			dFrontPorchIn = max( (double)MFP, ( dFrontPorchOut + dFifoMargin + min( dMaxHsyncDrift, 0.0 ) - dHeadRoom ) );
			dBackPorchIn = dTotalHBlankI - dFrontPorchIn;
		}

		// Check for underflow
		if( dBackPorchOut - dHeadRoom < dBackPorchIn + max( dMaxHsyncDrift, 0.0 ) )
		{
			dBackPorchIn = max( (double)MBP, ( dBackPorchOut - max( dMaxHsyncDrift, 0.0 ) - dHeadRoom ) );
			dFrontPorchIn = dTotalHBlankI - dBackPorchIn;
		}
	}
	else if( dMaxHsyncDrift < 0 )
	{
		// Delay the overflow as long as possible
		dBackPorchIn = min( ( dBackPorchOut - 1 ), ( dTotalHBlankI - MFP ) );
		dFrontPorchIn = dTotalHBlankI - dBackPorchIn;
	}
	else
	{
		// Delay the underflow as long as possible
		dFrontPorchIn = min( ( dFrontPorchOut + dFifoMargin - 1 ), ( dTotalHBlankI - MBP ) );
		dBackPorchIn = dTotalHBlankI - dFrontPorchIn;
	}

	param->h_blanki = (int)( dBackPorchIn );

}
