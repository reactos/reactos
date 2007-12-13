/*
* linux/drivers/video/riva/encoder.h - Xbox driver for encoder chip
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
#define NUM_SEQ_REGS            0x05
#define NUM_CRT_REGS            0x41
#define NUM_GRC_REGS            0x09
#define NUM_ATC_REGS            0x15

#define NUM_CONEXANT_REGS       0x69
#define MAX_ENCODER_REGS        256

#define u8 unsigned char 
#define U032 long
#ifndef encoder_h
#define encoder_h

//#include <linux/xboxfbctl.h>
#include "VideoInitialization.h"

typedef struct {
	double m_dHzBurstFrequency;
	double m_dSecBurstStart;
	double m_dSecBurstEnd;
	double m_dSecHsyncWidth;
	double m_dSecHsyncPeriod;
	double m_dSecActiveBegin;
	double m_dSecImageCentre;
	double m_dSecBlankBeginToHsync;
	unsigned int m_dwALO;
	double m_TotalLinesOut;
	double m_dSecHsyncToBlankEnd;
} conexant_video_parameter;

typedef struct _xbox_video_mode {
	int xres;
	int yres;
	int bpp;
	double hoc;
	double voc;
	xbox_av_type av_type;
	xbox_tv_encoding tv_encoding;
} xbox_video_mode;

typedef struct _riva_hw_state
{
  	U032 bpp;
        U032 width;
       	U032 height;
        U032 repaint0;
       	U032 repaint1;
      	U032 screen;
        U032 pixel;
        U032 horiz;
       	U032 arbitration0;
      	U032 arbitration1;
      	U032 vpll;
       	U032 pllsel;
       	U032 general;
      	U032 config;
       	U032 cursor0;
      	U032 cursor1;
       	U032 cursor2;
      	U032 offset0;
    	U032 offset1;
       	U032 offset2;
     	U032 offset3;
      	U032 pitch0;
      	U032 pitch1;
       	U032 pitch2;
      	U032 pitch3;
//#ifdef CONFIG_XBOX
      	U032 fb_start;
       	U032 vend;
       	U032 vtotal;
      	U032 vcrtc;
      	U032 vsyncstart;
       	U032 vsyncend;
      	U032 vvalidstart;
       	U032 vvalidend;
     	U032 hend;
       	U032 htotal;
      	U032 hcrtc;
    	U032 hsyncstart;
    	U032 hsyncend;
     	U032 hvalidstart;
      	U032 hvalidend;
       	U032 crtchdispend;
        U032 crtcvstart;
      	U032 crtcvtotal;
     	U032 checksum;
         //#endif
  } RIVA_HW_STATE;

struct riva_regs {
       	u8 attr[NUM_ATC_REGS];
       	u8 crtc[NUM_CRT_REGS];
       	u8 gra[NUM_GRC_REGS];
       	u8 seq[NUM_SEQ_REGS];
       	u8 misc_output;
      	RIVA_HW_STATE ext;
    	u8 encoder_mode[255];
};

typedef enum enumEncoderType {
	        ENCODER_CONEXANT,
	        ENCODER_FOCUS,
	        ENCODER_XCALIBUR
} xbox_encoder_type;

/*static const conexant_video_parameter vidstda[];

int tv_init(void);
void tv_exit(void);
xbox_encoder_type tv_get_video_encoder(void);

void tv_save_mode(unsigned char * mode_out);
void tv_load_mode(unsigned char * mode);
xbox_tv_encoding get_tv_encoding(void);
xbox_av_type detect_av_type(void);
*/
#endif
