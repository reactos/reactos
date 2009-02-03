/*
 * Declarations for MultiMedia-REGistration
 *
 * Copyright (C) 1999 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_MMREG_H
#define __WINE_MMREG_H

/***********************************************************************
 * Defines/Enums
 */

#define  MM_MSFT_WDMAUDIO_WAVEOUT 0x64
#define  MM_MSFT_WDMAUDIO_WAVEIN 0x65
#define  MM_MSFT_WDMAUDIO_MIDIOUT 0x66
#define  MM_MSFT_WDMAUDIO_MIDIIN 0x67
#define  MM_MSFT_WDMAUDIO_MIXER 0x68
#define  MM_MSFT_WDMAUDIO_AUX 0x69

#ifndef _ACM_WAVEFILTER
#define _ACM_WAVEFILTER

#define WAVE_FILTER_UNKNOWN     0x0000
#define WAVE_FILTER_DEVELOPMENT 0xFFFF

typedef struct _WAVEFILTER {
  DWORD   cbStruct;
  DWORD   dwFilterTag;
  DWORD   fdwFilter;
  DWORD   dwReserved[5];
} WAVEFILTER, *PWAVEFILTER, *NPWAVEFILTER, *LPWAVEFILTER;
#endif /* _ACM_WAVEFILTER */

#ifndef WAVE_FILTER_VOLUME
#define WAVE_FILTER_VOLUME      0x0001

typedef struct _WAVEFILTER_VOLUME {
   WAVEFILTER      wfltr;
   DWORD           dwVolume;
} VOLUMEWAVEFILTER, *PVOLUMEWAVEFILTER, *NPVOLUMEWAVEFILTER, *LPVOLUMEWAVEFILTER;
#endif  /* WAVE_FILTER_VOLUME */

#ifndef WAVE_FILTER_ECHO
#define WAVE_FILTER_ECHO        0x0002

typedef struct WAVEFILTER_ECHO {
   WAVEFILTER      wfltr;
   DWORD           dwVolume;
   DWORD           dwDelay;
} ECHOWAVEFILTER, *PECHOWAVEFILTER, *NPECHOWAVEFILTER, *LPECHOWAVEFILTER;
#endif  /* WAVEFILTER_ECHO */

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct _WAVEFORMATEX {
  WORD   wFormatTag;
  WORD   nChannels;
  DWORD  nSamplesPerSec;
  DWORD  nAvgBytesPerSec;
  WORD   nBlockAlign;
  WORD   wBitsPerSample;
  WORD   cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM					0x0001
#endif

/* WAVE form wFormatTag IDs */
#define  WAVE_FORMAT_UNKNOWN			0x0000	/*  Microsoft Corporation  */
#define  WAVE_FORMAT_ADPCM			0x0002	/*  Microsoft Corporation  */
#define  WAVE_FORMAT_IBM_CVSD			0x0005	/*  IBM Corporation  */
#define  WAVE_FORMAT_ALAW			0x0006	/*  Microsoft Corporation  */
#define  WAVE_FORMAT_MULAW			0x0007	/*  Microsoft Corporation  */
#define  WAVE_FORMAT_OKI_ADPCM			0x0010	/*  OKI  */
#define  WAVE_FORMAT_DVI_ADPCM			0x0011	/*  Intel Corporation  */
#define  WAVE_FORMAT_IMA_ADPCM			(WAVE_FORMAT_DVI_ADPCM)	/*  Intel Corporation  */
#define  WAVE_FORMAT_MEDIASPACE_ADPCM		0x0012	/*  Videologic  */
#define  WAVE_FORMAT_SIERRA_ADPCM		0x0013	/*  Sierra Semiconductor Corp  */
#define  WAVE_FORMAT_G723_ADPCM			0x0014	/*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_DIGISTD			0x0015	/*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_DIGIFIX			0x0016	/*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_DIALOGIC_OKI_ADPCM		0x0017	/*  Dialogic Corporation  */
#define  WAVE_FORMAT_YAMAHA_ADPCM		0x0020	/*  Yamaha Corporation of America  */
#define  WAVE_FORMAT_SONARC			0x0021	/*  Speech Compression  */
#define  WAVE_FORMAT_DSPGROUP_TRUESPEECH	0x0022	/*  DSP Group, Inc  */
#define  WAVE_FORMAT_ECHOSC1			0x0023	/*  Echo Speech Corporation  */
#define  WAVE_FORMAT_AUDIOFILE_AF36		0x0024	/*    */
#define  WAVE_FORMAT_APTX			0x0025	/*  Audio Processing Technology  */
#define  WAVE_FORMAT_AUDIOFILE_AF10		0x0026	/*    */
#define  WAVE_FORMAT_DOLBY_AC2			0x0030	/*  Dolby Laboratories  */
#define  WAVE_FORMAT_GSM610			0x0031	/*  Microsoft Corporation  */
#define  WAVE_FORMAT_ANTEX_ADPCME		0x0033	/*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_CONTROL_RES_VQLPC		0x0034	/*  Control Resources Limited  */
#define  WAVE_FORMAT_DIGIREAL			0x0035	/*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_DIGIADPCM			0x0036	/*  DSP Solutions, Inc.  */
#define  WAVE_FORMAT_CONTROL_RES_CR10		0x0037	/*  Control Resources Limited  */
#define  WAVE_FORMAT_NMS_VBXADPCM		0x0038	/*  Natural MicroSystems  */
#define  WAVE_FORMAT_G721_ADPCM			0x0040	/*  Antex Electronics Corporation  */
#define  WAVE_FORMAT_MPEG			0x0050	/*  Microsoft Corporation  */
#define  WAVE_FORMAT_MPEGLAYER3			0x0055
#define  WAVE_FORMAT_CREATIVE_ADPCM		0x0200	/*  Creative Labs, Inc  */
#define  WAVE_FORMAT_CREATIVE_FASTSPEECH8	0x0202	/*  Creative Labs, Inc  */
#define  WAVE_FORMAT_CREATIVE_FASTSPEECH10	0x0203	/*  Creative Labs, Inc  */
#define  WAVE_FORMAT_FM_TOWNS_SND		0x0300	/*  Fujitsu Corp.  */
#define  WAVE_FORMAT_OLIGSM			0x1000	/*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLIADPCM			0x1001	/*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLICELP			0x1002	/*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLISBC			0x1003	/*  Ing C. Olivetti & C., S.p.A.  */
#define  WAVE_FORMAT_OLIOPR			0x1004	/*  Ing C. Olivetti & C., S.p.A.  */

#if !defined(WAVE_FORMAT_EXTENSIBLE)
#define  WAVE_FORMAT_EXTENSIBLE			0xFFFE  /* Microsoft */
#endif

#define WAVE_FORMAT_DEVELOPMENT         	(0xFFFF)

typedef struct adpcmcoef_tag {
	short   iCoef1;
	short   iCoef2;
} ADPCMCOEFSET;
typedef ADPCMCOEFSET *PADPCMCOEFSET,
	*NPADPCMCOEFSET, *LPADPCMCOEFSET;

typedef struct adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
	WORD            wNumCoef;
	/* FIXME: this should be aCoef[0] */
	ADPCMCOEFSET    aCoef[1];
} ADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT *PADPCMWAVEFORMAT,
	*NPADPCMWAVEFORMAT, *LPADPCMWAVEFORMAT;

typedef struct dvi_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} DVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT *PDVIADPCMWAVEFORMAT,
	*NPDVIADPCMWAVEFORMAT, *LPDVIADPCMWAVEFORMAT;

typedef struct ima_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} IMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT *PIMAADPCMWAVEFORMAT, *NPIMAADPCMWAVEFORMAT,
	*LPIMAADPCMWAVEFORMAT;

typedef struct mediaspace_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
} MEDIASPACEADPCMWAVEFORMAT;
typedef MEDIASPACEADPCMWAVEFORMAT *PMEDIASPACEADPCMWAVEFORMAT,
	*NPMEDIASPACEADPCMWAVEFORMAT, *LPMEDIASPACEADPCMWAVEFORMAT;

typedef struct sierra_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
} SIERRAADPCMWAVEFORMAT;
typedef SIERRAADPCMWAVEFORMAT *PSIERRAADPCMWAVEFORMAT,
	*NPSIERRAADPCMWAVEFORMAT, *LPSIERRAADPCMWAVEFORMAT;

typedef struct g723_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            cbExtraSize;
	WORD            nAuxBlockSize;
} G723_ADPCMWAVEFORMAT;
typedef G723_ADPCMWAVEFORMAT *PG723_ADPCMWAVEFORMAT,
	*NPG723_ADPCMWAVEFORMAT, *LPG723_ADPCMWAVEFORMAT;

typedef struct digistdwaveformat_tag {
	WAVEFORMATEX    wfx;
} DIGISTDWAVEFORMAT;
typedef DIGISTDWAVEFORMAT *PDIGISTDWAVEFORMAT,
	*NPDIGISTDWAVEFORMAT, *LPDIGISTDWAVEFORMAT;

typedef struct digifixwaveformat_tag {
	WAVEFORMATEX    wfx;
} DIGIFIXWAVEFORMAT;
typedef DIGIFIXWAVEFORMAT *PDIGIFIXWAVEFORMAT,
	*NPDIGIFIXWAVEFORMAT, *LPDIGIFIXWAVEFORMAT;

typedef struct creative_fastspeechformat_tag {
	WAVEFORMATEX    ewf;
} DIALOGICOKIADPCMWAVEFORMAT;
typedef DIALOGICOKIADPCMWAVEFORMAT *PDIALOGICOKIADPCMWAVEFORMAT,
	*NPDIALOGICOKIADPCMWAVEFORMAT, *LPDIALOGICOKIADPCMWAVEFORMAT;

typedef struct yamaha_adpmcwaveformat_tag {
	WAVEFORMATEX    wfx;
} YAMAHA_ADPCMWAVEFORMAT;
typedef YAMAHA_ADPCMWAVEFORMAT *PYAMAHA_ADPCMWAVEFORMAT,
	*NPYAMAHA_ADPCMWAVEFORMAT, *LPYAMAHA_ADPCMWAVEFORMAT;

typedef struct sonarcwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wCompType;
} SONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT *PSONARCWAVEFORMAT,
	*NPSONARCWAVEFORMAT,*LPSONARCWAVEFORMAT;

typedef struct truespeechwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
	WORD            nSamplesPerBlock;
	BYTE            abReserved[28];
} TRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT *PTRUESPEECHWAVEFORMAT,
	*NPTRUESPEECHWAVEFORMAT, *LPTRUESPEECHWAVEFORMAT;

typedef struct echosc1waveformat_tag {
	WAVEFORMATEX    wfx;
} ECHOSC1WAVEFORMAT;
typedef ECHOSC1WAVEFORMAT *PECHOSC1WAVEFORMAT,
	*NPECHOSC1WAVEFORMAT, *LPECHOSC1WAVEFORMAT;

typedef struct audiofile_af36waveformat_tag {
	WAVEFORMATEX    wfx;
} AUDIOFILE_AF36WAVEFORMAT;
typedef AUDIOFILE_AF36WAVEFORMAT *PAUDIOFILE_AF36WAVEFORMAT,
	*NPAUDIOFILE_AF36WAVEFORMAT, *LPAUDIOFILE_AF36WAVEFORMAT;

typedef struct aptxwaveformat_tag {
	WAVEFORMATEX    wfx;
} APTXWAVEFORMAT;
typedef APTXWAVEFORMAT *PAPTXWAVEFORMAT,
	*NPAPTXWAVEFORMAT, *LPAPTXWAVEFORMAT;

typedef struct audiofile_af10waveformat_tag {
	WAVEFORMATEX    wfx;
} AUDIOFILE_AF10WAVEFORMAT;
typedef AUDIOFILE_AF10WAVEFORMAT *PAUDIOFILE_AF10WAVEFORMAT,
	*NPAUDIOFILE_AF10WAVEFORMAT,  *LPAUDIOFILE_AF10WAVEFORMAT;

typedef struct dolbyac2waveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            nAuxBitsCode;
} DOLBYAC2WAVEFORMAT;

typedef struct gsm610waveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} GSM610WAVEFORMAT;
typedef GSM610WAVEFORMAT *PGSM610WAVEFORMAT,
	*NPGSM610WAVEFORMAT, *LPGSM610WAVEFORMAT;

typedef struct adpcmewaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} ADPCMEWAVEFORMAT;
typedef ADPCMEWAVEFORMAT *PADPCMEWAVEFORMAT,
	*NPADPCMEWAVEFORMAT, *LPADPCMEWAVEFORMAT;

typedef struct contres_vqlpcwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} CONTRESVQLPCWAVEFORMAT;
typedef CONTRESVQLPCWAVEFORMAT *PCONTRESVQLPCWAVEFORMAT,
	*NPCONTRESVQLPCWAVEFORMAT, *LPCONTRESVQLPCWAVEFORMAT;

typedef struct digirealwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} DIGIREALWAVEFORMAT;
typedef DIGIREALWAVEFORMAT *PDIGIREALWAVEFORMAT,
	*NPDIGIREALWAVEFORMAT, *LPDIGIREALWAVEFORMAT;

typedef struct digiadpcmmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} DIGIADPCMWAVEFORMAT;
typedef DIGIADPCMWAVEFORMAT *PDIGIADPCMWAVEFORMAT,
	*NPDIGIADPCMWAVEFORMAT, *LPDIGIADPCMWAVEFORMAT;

typedef struct contres_cr10waveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} CONTRESCR10WAVEFORMAT;
typedef CONTRESCR10WAVEFORMAT *PCONTRESCR10WAVEFORMAT,
	*NPCONTRESCR10WAVEFORMAT, *LPCONTRESCR10WAVEFORMAT;

typedef struct nms_vbxadpcmmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} NMS_VBXADPCMWAVEFORMAT;
typedef NMS_VBXADPCMWAVEFORMAT *PNMS_VBXADPCMWAVEFORMAT,
	*NPNMS_VBXADPCMWAVEFORMAT, *LPNMS_VBXADPCMWAVEFORMAT;

typedef struct g721_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            nAuxBlockSize;
} G721_ADPCMWAVEFORMAT;
typedef G721_ADPCMWAVEFORMAT *PG721_ADPCMWAVEFORMAT,
	*NG721_ADPCMWAVEFORMAT, *LPG721_ADPCMWAVEFORMAT;

typedef struct creative_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
} CREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT *PCREATIVEADPCMWAVEFORMAT,
	*NPCREATIVEADPCMWAVEFORMAT, *LPCREATIVEADPCMWAVEFORMAT;

typedef struct creative_fastspeech8format_tag {
	WAVEFORMATEX    wfx;
	WORD wRevision;
} CREATIVEFASTSPEECH8WAVEFORMAT;
typedef CREATIVEFASTSPEECH8WAVEFORMAT *PCREATIVEFASTSPEECH8WAVEFORMAT,
	*NPCREATIVEFASTSPEECH8WAVEFORMAT, *LPCREATIVEFASTSPEECH8WAVEFORMAT;

typedef struct creative_fastspeech10format_tag {
	WAVEFORMATEX    wfx;
	WORD wRevision;
} CREATIVEFASTSPEECH10WAVEFORMAT;
typedef CREATIVEFASTSPEECH10WAVEFORMAT *PCREATIVEFASTSPEECH10WAVEFORMAT,
	*NPCREATIVEFASTSPEECH10WAVEFORMAT, *LPCREATIVEFASTSPEECH10WAVEFORMAT;

typedef struct fmtowns_snd_waveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
} FMTOWNS_SND_WAVEFORMAT;
typedef FMTOWNS_SND_WAVEFORMAT *PFMTOWNS_SND_WAVEFORMAT,
	*NPFMTOWNS_SND_WAVEFORMAT, *LPFMTOWNS_SND_WAVEFORMAT;

typedef struct oligsmwaveformat_tag {
	WAVEFORMATEX    wfx;
} OLIGSMWAVEFORMAT;
typedef OLIGSMWAVEFORMAT *POLIGSMWAVEFORMAT,
	*NPOLIGSMWAVEFORMAT, *LPOLIGSMWAVEFORMAT;

typedef struct oliadpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
} OLIADPCMWAVEFORMAT;
typedef OLIADPCMWAVEFORMAT *POLIADPCMWAVEFORMAT,
	*NPOLIADPCMWAVEFORMAT, *LPOLIADPCMWAVEFORMAT;

typedef struct olicelpwaveformat_tag {
	WAVEFORMATEX    wfx;
} OLICELPWAVEFORMAT;
typedef OLICELPWAVEFORMAT *POLICELPWAVEFORMAT,
	*NPOLICELPWAVEFORMAT, *LPOLICELPWAVEFORMAT;

typedef struct olisbcwaveformat_tag {
	WAVEFORMATEX    wfx;
} OLISBCWAVEFORMAT;
typedef OLISBCWAVEFORMAT *POLISBCWAVEFORMAT,
	*NPOLISBCWAVEFORMAT, *LPOLISBCWAVEFORMAT;

typedef struct olioprwaveformat_tag {
	WAVEFORMATEX    wfx;
} OLIOPRWAVEFORMAT;
typedef OLIOPRWAVEFORMAT *POLIOPRWAVEFORMAT,
	*NPOLIOPRWAVEFORMAT, *LPOLIOPRWAVEFORMAT;

typedef struct csimaadpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
} CSIMAADPCMWAVEFORMAT;
typedef CSIMAADPCMWAVEFORMAT *PCSIMAADPCMWAVEFORMAT,
	*NPCSIMAADPCMWAVEFORMAT, *LPCSIMAADPCMWAVEFORMAT;

typedef struct mpeg1waveformat_tag {
	WAVEFORMATEX	wfx;
	WORD		fwHeadLayer;
	DWORD		dwHeadBitrate;
	WORD		fwHeadMode;
	WORD		fwHeadModeExt;
	WORD		wHeadEmphasis;
	WORD		fwHeadFlags;
	DWORD		dwPTSLow;
	DWORD		dwPTSHigh;
} MPEG1WAVEFORMAT,* PMPEG1WAVEFORMAT;

#define	ACM_MPEG_LAYER1		0x0001
#define	ACM_MPEG_LAYER2		0x0002
#define	ACM_MPEG_LAYER3		0x0004

#define	ACM_MPEG_STEREO		0x0001
#define	ACM_MPEG_JOINTSTEREO	0x0002
#define	ACM_MPEG_DUALCHANNEL	0x0004
#define	ACM_MPEG_SINGLECHANNEL	0x0008
#define	ACM_MPEG_PRIVATEBIT	0x0001
#define	ACM_MPEG_COPYRIGHT	0x0002
#define	ACM_MPEG_ORIGINALHOME	0x0004
#define	ACM_MPEG_PROTECTIONBIT	0x0008
#define	ACM_MPEG_ID_MPEG1	0x0010

typedef struct mpeglayer3waveformat_tag {
	WAVEFORMATEX	wfx;
	WORD		wID;
	DWORD		fdwFlags;
	WORD		nBlockSize;
	WORD		nFramesPerBlock;
	WORD		nCodecDelay;
} MPEGLAYER3WAVEFORMAT;

#define MPEGLAYER3_WFX_EXTRA_BYTES   12

#define MPEGLAYER3_ID_UNKNOWN           0
#define MPEGLAYER3_ID_MPEG		1
#define MPEGLAYER3_ID_CONSTANTFRAMESIZE	2

#define MPEGLAYER3_FLAG_PADDING_ISO	0x00000000
#define MPEGLAYER3_FLAG_PADDING_ON	0x00000001
#define MPEGLAYER3_FLAG_PADDING_OFF	0x00000002

#ifdef GUID_DEFINED

#ifndef _WAVEFORMATEXTENSIBLE_
#define _WAVEFORMATEXTENSIBLE_
typedef struct {
    WAVEFORMATEX    Format;
    union {
        WORD        wValidBitsPerSample;
        WORD        wSamplesPerBlock;
        WORD        wReserved;
    } Samples;
    DWORD           dwChannelMask;
    GUID            SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
#endif /* _WAVEFORMATEXTENSIBLE_ */

#endif /* GUID_DEFINED */

typedef WAVEFORMATEXTENSIBLE    WAVEFORMATPCMEX;
typedef WAVEFORMATPCMEX*        PWAVEFORMATPCMEX;
typedef WAVEFORMATPCMEX*        NPWAVEFORMATPCMEX;
typedef WAVEFORMATPCMEX*        LPWAVEFORMATPCMEX;

typedef WAVEFORMATEXTENSIBLE    WAVEFORMATIEEEFLOATEX;
typedef WAVEFORMATIEEEFLOATEX*  PWAVEFORMATIEEEFLOATEX;
typedef WAVEFORMATIEEEFLOATEX*  NPWAVEFORMATIEEEFLOATEX;
typedef WAVEFORMATIEEEFLOATEX*  LPWAVEFORMATIEEEFLOATEX;

#ifndef _SPEAKER_POSITIONS_
#define _SPEAKER_POSITIONS_

#define SPEAKER_FRONT_LEFT              0x00000001
#define SPEAKER_FRONT_RIGHT             0x00000002
#define SPEAKER_FRONT_CENTER            0x00000004
#define SPEAKER_LOW_FREQUENCY           0x00000008
#define SPEAKER_BACK_LEFT               0x00000010
#define SPEAKER_BACK_RIGHT              0x00000020
#define SPEAKER_FRONT_LEFT_OF_CENTER    0x00000040
#define SPEAKER_FRONT_RIGHT_OF_CENTER   0x00000080
#define SPEAKER_BACK_CENTER             0x00000100
#define SPEAKER_SIDE_LEFT               0x00000200
#define SPEAKER_SIDE_RIGHT              0x00000400
#define SPEAKER_TOP_CENTER              0x00000800
#define SPEAKER_TOP_FRONT_LEFT          0x00001000
#define SPEAKER_TOP_FRONT_CENTER        0x00002000
#define SPEAKER_TOP_FRONT_RIGHT         0x00004000
#define SPEAKER_TOP_BACK_LEFT           0x00008000
#define SPEAKER_TOP_BACK_CENTER         0x00010000
#define SPEAKER_TOP_BACK_RIGHT          0x00020000
#define SPEAKER_RESERVED                0x7FFC0000
#define SPEAKER_ALL                     0x80000000

#endif /* _SPEAKER_POSITIONS_ */


/* DIB stuff */

#ifndef BI_BITFIELDS
#define BI_BITFIELDS     3
#endif

#ifndef QUERYDIBSUPPORT
#define	QUERYDIBSUPPORT		3073
#define	QDI_SETDIBITS		1
#define	QDI_GETDIBITS		2
#define	QDI_DIBTOSCREEN		4
#define	QDI_STRETCHDIB		8
#endif

#ifndef NOBITMAP
typedef struct tagEXBMINFOHEADER {
    BITMAPINFOHEADER bmi;
    DWORD biExtDataOffset;
} EXBMINFOHEADER;
#endif


/* Video stuff */

#ifndef NONEWIC

#ifndef ICTYPE_VIDEO
#define ICTYPE_VIDEO		mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO		mmioFOURCC('a', 'u', 'd', 'c')
#endif

#endif

#endif /* __WINE_MMREG_H */
