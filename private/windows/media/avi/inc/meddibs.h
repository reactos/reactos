/* 
 * MEDDIBS.H
 * 
 * This is the external definition of the video converter handlers.
 *
 * This file requires "windows.h", "mediaman.h"
 */

#ifndef _MEDDIBS_H_
#define _MEDDIBS_H_

#define medtypeDIBS	medFOURCC('D','I','B','S')	// logical type

#define medtypeIRLE	medFOURCC('I','R','L','E')	// 'rle' format
#define medtypeRLE0	medFOURCC('R','L','E','0')	// 'rle0' format
#define medtypeMERI	medFOURCC('M','E','R','I')	
#define medtypeFLI	medFOURCC('F','L','I',' ')	// Autodesk .fli
#define medtypeDIBSEQ	medFOURCC('D','S','E','Q')	// DIB sequence

#define medtypeAVID	medFOURCC('A','V','I',' ')	// AVI DIBS
#define medtypeAVI0	medFOURCC('A','V','I','0')	// Old AVI
#define medtypeAVIW	medFOURCC('A','V','I','W')	// AVI WAVE
#define medtypeAVIP	medFOURCC('A','V','I','P')	// AVI Palette
#define medtypeAVIM	medFOURCC('A','V','I','M')	// AVI MIDI
#define medtypeAVIMDIB	medFOURCC('M','D','B','A')	// AVI MDIB

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif




#define DIBSF_NONINTERLEAVED	0x0001

#define DIBSF_NOPADDING		0x0040
#define DIBSF_VARIABLESIZEREC	0x0080

#define DIBSC_RANDOMACCESS	0x00000001
#define DIBSC_USEABLERANDOM	0x00000002
#define	DIBSC_FASTACCESS	0x00000004

// The flag below is used if the handler supports quality, key frames, etc.
#define DIBSC_EXTENDEDINFO	0x00000008  

#define DIBSC_KNOWSLENGTH	0x00000010
#define DIBSC_RETURNSFULLFRAMES	0x00000020

#define DIBS_GETINFO		(MED_USER)
typedef struct {
    DWORD	dwFlags;
    DWORD	dwCapabilities;
    DWORD	dwWidth;
    DWORD	dwHeight;
    DWORD       dwBytesPerSec;              /* if not zero. */
    DWORD	dwMicroSecPerFrame;
    DWORD       dwStreams;

    DWORD       dwQuality;
    DWORD       dwFoo;

    DWORD       dwKeyFrameEvery;
    DWORD       dwAudioEvery;

} DIBSINFO;

/* quality flags */
#define ICQUALITY_LOW       0
#define ICQUALITY_HIGH      10000
#define ICQUALITY_DEFAULT   -1

typedef struct {
    DIBSINFO	di;
    DWORD	dwFrames;
    MEDTYPE	medtypePhys;
} DIBSCREATE;

#define DIBS_GETSTREAMTYPE	(MED_USER+1)

/* Define stream types here.... */
#define DIBS_STREAM_VIDEO		medFOURCC('v', 'i', 'd', 's')
#define DIBS_STREAM_AUDIO		medFOURCC('a', 'u', 'd', 's')
#define DIBS_STREAM_MIDI 		medFOURCC('m', 'i', 'd', 's')
#define DIBS_STREAM_TEXT 		medFOURCC('t', 'x', 't', 's')

#define DIBS_GETSTREAMFORMAT	(MED_USER+2)
#define DIBS_GETSTREAMHANDLER	(MED_USER+3)
#define DIBS_GETHANDLERDATA	(MED_USER+4)
#define DIBS_GETFRAMEDATA	(MED_USER+5)
#define DIBS_GETLENGTH		(MED_USER+6)
#define DIBS_GETKEYFRAMEINFO	(MED_USER+7)
#define DIBS_GETFILESIZE	(MED_USER+8)
#define DIBS_GETFRAMESIZE	(MED_USER+9)
#define DIBS_GETRAWDATA		(MED_USER+10)

#define DIBS_VID_KEYFRAME	0x01000000L
#define DIBS_VID_PERCENT(dwRet)	LOBYTE(HIWORD(dwRet))
#define DIBS_VID_HDIB(dwRet)	((HANDLE) LOWORD(dwRet))

#define MediaDibsSetError(ERR) medSetExtError(ERR, ghInst)


/* lParam1 = buffer, lParam2 = buffer length.  return = data size */
#define MED_GETEXTRACHUNKS	0x0080



/* Should the structure below even be public? */
typedef struct {
	DIBSINFO	di;
	DWORD		dwFrames;
	HMED		hMed;
	DWORD		dwFileLength;
	DWORD		dwFrame;
	MEDTYPE		medTypeCurrent;
	MEDID		medidOld;
	HANDLE		hColors;
	BITMAPINFOHEADER bih;
#if 0	
	MEDID		medidWAVE;
	MEDID		medidMIDI;
	FOURCC		fccVideoCompType;
	LPSTR		lpVCParms;
	DWORD		dwVCLen;
	FOURCC		fccAudioCompType;
	LPSTR		lpACParms;
	DWORD		dwACLen;
#endif	
	DWORD		dwPhysData;
} MediaDibs, FAR * FPMediaDibs;

#define DIBSF_NONINTERLEAVED	0x0001

#define DIBSF_VARIABLESIZEREC	0x0080
#define DIBSF_ALREADYCOMPRESSED	0x0100


#define OLDRLEF_MERGECOLORS	0x0010
#define OLDRLEF_SKIPSINGLE	0x0020
#define OLDRLEF_ADAPTIVE	0x0040


#define DIBSF_SPECIAL		0x1000
#define DIBSF_SPECIAL2		0x2000



/* ERROR MESSAGE DEFINITIONS, must be greater than 100 */

#define ERRCNV_BAD_HEADER		101
#define ERRCNV_MEMORY			102
#define ERRCNV_EOF			103
#define ERRCNV_NOVIDEO			104
#define ERRCNV_NOAUDIO			105
#define ERRCNV_NOT_FORM			120
#define ERRCNV_OPEN_INPUT		121
#define ERRCNV_OPEN_OUTPUT		122
#define ERRCNV_WRITING			129
#define ERRCNV_NOT_MERID		133
#define ERRCNV_NOTWRITE			143
#define ERRCNV_NOTREAD			144
#define ERRCNV_UNKNOWN			145
#define ERRCNV_SIZE			146
#define ERRCNV_IMAGE			147
#define ERRCNV_WAVEMEMORY		148
#define ERRCNV_BADMEDWAVEVERSION	160
#define ERRCNV_BADDSEQNAME		161
#define ERRCNV_DSEQTOOLONG		162
#define ERRCNV_CANTOPENCOMP             163
#define ERRCNV_DATARATETOOLOW           164
#define ERRCNV_CANTSTARTCOMP            165

#define ERRCNV_NOVIDEOSTREAM		170
#define ERRCNV_TWOVIDEOSTREAMS		171
#define ERRCNV_TWOAUDIOSTREAMS          172

#define ERRCNV_BADFORMAT                173
#define ERRCNV_MEDBITSNOTRUECOLOR	174
#define ERRCNV_ALPHAFORMAT		175
#define ERRCNV_NOPALETTE		176
#define ERRCNV_CHANGESEQNAME		177
#endif  /*  _MEDDIBS_H_  */
