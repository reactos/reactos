#ifndef VIDEOINITIALIZATION_H
#define VIDEOINITIALIZATION_H

typedef enum enumVideoStandards {
        TV_ENC_INVALID=-1,
        TV_ENC_NTSC=0,
        TV_ENC_NTSC60,
        TV_ENC_PALBDGHI,
        TV_ENC_PALN,
        TV_ENC_PALNC,
        TV_ENC_PALM,
        TV_ENC_PAL60
} xbox_tv_encoding;

typedef enum enumAvTypes {
        AV_INVALID=-1,
        AV_SCART_RGB,
        AV_SVIDEO,
        AV_VGA_SOG,
        AV_HDTV,
        AV_COMPOSITE,
        AV_VGA
} xbox_av_type;

typedef enum enumHdtvModes {
	HDTV_480p,
	HDTV_720p,
	HDTV_1080i
} xbox_hdtv_mode;

/* Used to configure the GPU */
typedef struct {
	long xres;
	long crtchdispend;
	long nvhstart;
	long nvhtotal;
	long yres;
	long nvvstart;
	long crtcvstart;
	long crtcvtotal;
    	long nvvtotal;
	long pixelDepth;
	xbox_av_type av_type;
} GPU_PARAMETER;

///////// VideoInitialization.c

xbox_tv_encoding DetectVideoStd(void);
xbox_av_type DetectAvType(void);

void SetGPURegister(const GPU_PARAMETER* gpu, unsigned char *pbRegs);
#endif
