/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWMMED.H
 *  16-bit MultiMedia API argument structures
 *
 *  History:
 *  Created 21-Jan-1992 by Mike Tricker (MikeTri), based on the work by jeffpar
--*/
/*++

  General MultiMedia related information

  Moved all the typedefs for H* and VP* back to WOW.H  - MikeTri 090492

--*/

typedef WORD    MMVER16;      // major (high byte), minor (low byte)

#ifndef _INC_MMSYSTEM
typedef DWORD   FOURCC;         // a four character code
typedef LONG    LPARAM;
#endif

#define MAXPNAMELEN      32     // max product name length (including NULL)

/* XLATOFF */
#pragma pack(1)
/* XLATON */


/*
 * MultiMedia Data Structures - MikeTri 10-Feb-1992
 *
 */


typedef struct _AUXCAPS16 {           /* ac16 */
    WORD        wMid;
    WORD        wPid;
    MMVER16   vDriverVersion;
    char        szPname[MAXPNAMELEN];
    WORD        wTechnology;
    DWORD       dwSupport;
} AUXCAPS16;
typedef AUXCAPS16 UNALIGNED *PAUXCAPS16;
typedef VPVOID  VPAUXCAPS16;

typedef struct _DRVCONFIGINFO16 {        /* dci16 */
    DWORD   dwDCISize;
    VPCSTR  lpszDCISectionName;
    VPCSTR  lpszDCIAliasName;
} DRVCONFIGINFO16;
typedef DRVCONFIGINFO16 UNALIGNED *PDRVCONFIGINFO16;
typedef VPVOID  VPDRVCONFIGINFO16;

typedef struct _JOYCAPS16 {              /* jc16 */
    WORD    wMid;
    WORD    wPid;
    char    szPname[MAXPNAMELEN];
    WORD    wXmin;
    WORD    wXmax;
    WORD    wYmin;
    WORD    wYmax;
    WORD    wZmin;
    WORD    wZmax;
    WORD    wNumButtons;
    WORD    wPeriodMin;
    WORD    wPeriodMax;
} JOYCAPS16;
typedef JOYCAPS16 UNALIGNED *PJOYCAPS16;
typedef VPVOID  VPJOYCAPS16;

typedef struct _JOYINFO16 {              /* ji16 */
    WORD    wXpos;
    WORD    wYpos;
    WORD    wZpos;
    WORD    wButtons;
} JOYINFO16;
typedef JOYINFO16 UNALIGNED *PJOYINFO16;
typedef VPVOID  VPJOYINFO16;

typedef struct _MCI_ANIM_OPEN_PARMS16 {  /* maop16 */
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
    VPCSTR  lpstrDeviceType;
    VPCSTR  lpstrElementName;
    VPCSTR  lpstrAlias;
    DWORD   dwStyle;
    HWND16  hWndParent;    // Keeps consistent, and is equivalent anyway
    WORD    wReserved1;
} MCI_ANIM_OPEN_PARMS16;
typedef MCI_ANIM_OPEN_PARMS16 UNALIGNED *PMCI_ANIM_OPEN_PARMS16;
typedef VPVOID  VPMCI_ANIM_OPEN_PARMS16;

typedef struct _MCI_ANIM_PLAY_PARMS16 {  /* mapp16 */
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_ANIM_PLAY_PARMS16;
typedef MCI_ANIM_PLAY_PARMS16 UNALIGNED *PMCI_ANIM_PLAY_PARMS16;
typedef VPVOID  VPMCA_ANIM_PLAY_PARMS16;

typedef struct _MCI_ANIM_RECT_PARMS16 {  /* marp16 */
    DWORD   dwCallback;
    RECT16  rc;
} MCI_ANIM_RECT_PARMS16;
typedef MCI_ANIM_RECT_PARMS16 UNALIGNED *PMCI_ANIM_RECT_PARMS16;
typedef VPVOID  VPMCI_ANIM_RECT_PARMS16;

typedef struct _MCI_ANIM_STEP_PARMS16 {  /* masp16 */
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_ANIM_STEP_PARMS16;
typedef MCI_ANIM_STEP_PARMS16 UNALIGNED *PMCI_ANIM_STEP_PARMS16;
typedef VPVOID  VPMCI_ANIM_STEP_PARMS16;

typedef struct _MCI_ANIM_UPDATE_PARMS16 { /* maup16 */
    DWORD   dwCalback;
    RECT16  rc;
    HDC16   hDC;
} MCI_ANIM_UPDATE_PARMS16;
typedef MCI_ANIM_UPDATE_PARMS16 UNALIGNED *PMCI_ANIM_UPDATE_PARMS16;
typedef VPVOID  VPMCI_ANIM_UPDATE_PARMS16;

typedef struct _MCI_ANIM_WINDOW_PARMS16 { /* mawp16 */
    DWORD   dwCallabck;
    HWND16  hWnd;
    WORD    wReserved1;
    WORD    nCmdShow;
    WORD    wReserved2;
    VPCSTR  lpstrText;
} MCI_ANIM_WINDOW_PARMS16;
typedef MCI_ANIM_WINDOW_PARMS16 UNALIGNED *PMCI_ANIM_WINDOW_PARMS16;
typedef VPVOID  VPMCI_ANIM_WINDOW_PARMS16;

typedef struct _MCI_BREAK_PARMS16 {       /* mbp16 */
    DWORD  dwCallback;
    INT16  nVirtKey;
    WORD   wReserved0;
    HWND16 hwndBreak;
    WORD   wReserved1;
} MCI_BREAK_PARMS16;
typedef MCI_BREAK_PARMS16 UNALIGNED *PMCI_BREAK_PARMS16;
typedef VPVOID  VPMCI_BREAK_PARMS16;

typedef struct _MCI_GENERIC_PARMS16 {     /* mgp16 */
    DWORD   dwCallback;
} MCI_GENERIC_PARMS16;
typedef MCI_GENERIC_PARMS16 UNALIGNED *PMCI_GENERIC_PARMS16;
typedef VPVOID  VPMCI_GENERIC_PARMS16;

typedef struct _MCI_GETDEVCAPS_PARMS16 {  /* mgdp16 */
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
} MCI_GETDEVCAPS_PARMS16;
typedef MCI_GETDEVCAPS_PARMS16 UNALIGNED *PMCI_GETDEVCAPS_PARMS16;
typedef VPVOID  VPMCI_GETDEVCAPS_PARMS16;

typedef struct _MCI_INFO_PARMS16 {        /* mip16 */
    DWORD   dwCallback;
    VPSTR   lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMS16;
typedef MCI_INFO_PARMS16 UNALIGNED *PMCI_INFO_PARMS16;
typedef VPVOID  VPMCI_INFO_PARMS16;

typedef struct _MCI_LOAD_PARMS16 {        /* mlp16 */
    DWORD   dwCallback;
    VPCSTR  lpfilename;
} MCI_LOAD_PARMS16;
typedef MCI_LOAD_PARMS16 UNALIGNED *PMCI_LOAD_PARMS16;
typedef VPVOID  VPMCI_LOAD_PARMS16;

typedef struct _MCI_OPEN_PARMS16 {        /* mop16 */
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
    VPCSTR  lpstrDeviceType;
    VPCSTR  lpstrElementName;
    VPCSTR  lpstrAlias;
} MCI_OPEN_PARMS16;
typedef MCI_OPEN_PARMS16 UNALIGNED *PMCI_OPEN_PARMS16;
typedef VPVOID  VPMCI_OPEN_PARMS16;

typedef struct _MCI_OVLY_LOAD_PARMS16 {   /* molp16 */
    DWORD   dwCallback;
    VPCSTR  lpfilename;
    RECT16  rc;
} MCI_OVLY_LOAD_PARMS16;
typedef MCI_OVLY_LOAD_PARMS16 UNALIGNED *PMCI_OVLY_LOAD_PARMS16;
typedef VPVOID  VPMCI_OVLY_LOAD_PARMS16;

typedef struct _MCI_OVLY_OPEN_PARMS16 {   /* moop16 */
    DWORD   dwCallabck;
    WORD    wDeviceID;
    WORD    wReserved0;
    VPCSTR  lpstrDeviceType;
    VPCSTR  lpstrElementName;
    VPCSTR  lpstrAlias;
    DWORD   dwStyle;
    HWND16  hWndParent;  // The book is wrong
    WORD    wReserved1;
} MCI_OVLY_OPEN_PARMS16;
typedef MCI_OVLY_OPEN_PARMS16 UNALIGNED *PMCI_OVLY_OPEN_PARMS16;
typedef VPVOID  VPMCI_OVLY_OPEN_PARMS16;

typedef struct _MCI_OVLY_RECT_PARMS16 {   /* morp16 */
    DWORD   dwCallback;
    RECT16  rc;
} MCI_OVLY_RECT_PARMS16;
typedef MCI_OVLY_RECT_PARMS16 UNALIGNED *PMCI_OVLY_RECT_PARMS16;
typedef VPVOID  VPMCI_OVLY_RECT_PARMS16;

typedef struct _MCI_OVLY_SAVE_PARMS16 {   /* mosp16 */
    DWORD   dwCallback;
    VPCSTR  lpfilename;
    RECT16  rc;
} MCI_OVLY_SAVE_PARMS16;
typedef MCI_OVLY_SAVE_PARMS16 UNALIGNED *PMCI_OVLY_SAVE_PARMS16;
typedef VPVOID  VPMCI_OVLY_SAVE_PARMS16;

typedef struct _MCI_OVLY_WINDOW_PARMS16 { /* mowp16 */
    DWORD   dwCallabck;
    HWND16  hWnd;
    WORD    wReserved1;
    WORD    nCmdShow;
    WORD    wReserved2;
    VPCSTR  lpstrText;
} MCI_OVLY_WINDOW_PARMS16;
typedef MCI_OVLY_WINDOW_PARMS16 UNALIGNED *PMCI_OVLY_WINDOW_PARMS16;
typedef VPVOID  VPMCI_OVLY_WINDOW_PARMS16;

typedef struct _MCI_PLAY_PARMS16 {        /* mplp16 */
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_PLAY_PARMS16;
typedef MCI_PLAY_PARMS16 UNALIGNED *PMCI_PLAY_PARMS16;
typedef VPVOID  VPMCI_PLAY_PARMS16;

typedef struct _MCI_RECORD_PARMS16 {      /* mrecp16 */
    DWORD   dwCallabck;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_RECORD_PARMS16;
typedef MCI_RECORD_PARMS16 UNALIGNED *PMCI_RECORD_PARMS16;
typedef VPVOID  VPMCI_RECORD_PARMS16;

typedef struct _MCI_SAVE_PARMS16 {        /* msavp16 */
    DWORD   dwCallback;
    VPCSTR  lpfilename;   // MMSYSTEM.H differs from the book
} MCI_SAVE_PARMS16;
typedef MCI_SAVE_PARMS16 UNALIGNED *PMCI_SAVE_PARMS16;
typedef VPVOID  VPMCI_SAVE_PARMS16;

typedef struct _MCI_SEEK_PARMS16 {        /* msep16 */
    DWORD   dwCallback;
    DWORD   dwTo;
} MCI_SEEK_PARMS16;
typedef MCI_SEEK_PARMS16 UNALIGNED *PMCI_SEEK_PARMS16;
typedef VPVOID  VPMCI_SEEK_PARMS16;

typedef struct _MCI_SEQ_SET_PARMS16 {     /* mssp16 */
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    DWORD   dwTempo;
    DWORD   dwPort;
    DWORD   dwSlave;
    DWORD   dwMaster;
    DWORD   dwOffset;
} MCI_SEQ_SET_PARMS16;
typedef MCI_SEQ_SET_PARMS16 UNALIGNED *PMCI_SEQ_SET_PARMS16;
typedef VPVOID  VPMCI_SEQ_SET_PARMS16;

typedef struct _MCI_SET_PARMS16 {         /* msetp16 */
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
} MCI_SET_PARMS16;
typedef MCI_SET_PARMS16 UNALIGNED *PMCI_SET_PARMS16;
typedef VPVOID  VPMCI_SET_PARMS16;

typedef struct _MCI_SOUND_PARMS16 {       /* msoup16 */
    DWORD   dwCallback;
    VPCSTR  lpstrSoundName;
} MCI_SOUND_PARMS16;
typedef MCI_SOUND_PARMS16 UNALIGNED *PMCI_SOUND_PARMS16;
typedef VPVOID  VPMCI_SOUND_PARMS16;

typedef struct _MCI_STATUS_PARMS16 {      /* mstp16 */
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
    DWORD   dwTrack;
} MCI_STATUS_PARMS16;
typedef MCI_STATUS_PARMS16 UNALIGNED *PMCI_STATUS_PARMS16;
typedef VPVOID  VPMCI_STATUS_PARMS16;

typedef struct _MCI_SYSINFO_PARMS16 {     /* msyip16 */
    DWORD   dwCallback;
    VPSTR   lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwNumber;
    WORD    wDeviceType;
    WORD    wReserved0;
} MCI_SYSINFO_PARMS16;
typedef MCI_SYSINFO_PARMS16 UNALIGNED *PMCI_SYSINFO_PARMS16;
typedef VPVOID  VPMCI_SYSINFO_PARMS16;

typedef struct _MCI_VD_ESCAPE_PARMS16 {   /* mvep16 */
    DWORD   dwCallback;
    VPCSTR  lpstrCommand;
} MCI_VD_ESCAPE_PARMS16;
typedef MCI_VD_ESCAPE_PARMS16 UNALIGNED *PMCI_VD_ESCAPE_PARMS16;
typedef VPVOID  VPMCI_VD_ESCAPE_PARMS16;

typedef struct _MCI_VD_PLAY_PARMS16 {     /* mvpp16 */
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_VD_PLAY_PARMS16;
typedef MCI_VD_PLAY_PARMS16 UNALIGNED *PMCI_VD_PLAY_PARMS16;
typedef VPVOID  VPMCI_VD_PLAY_PARMS16;

typedef struct _MCI_VD_STEP_PARMS16 {     /* mvsp16 */
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_VD_STEP_PARMS16;
typedef MCI_VD_STEP_PARMS16 UNALIGNED *PMCI_VD_STEP_PARMS16;
typedef VPVOID  VPMCI_VD_STEP_PARMS16;

typedef struct _MCI_VD_DELETE_PARMS16 {   /* mvdp16 */
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_VD_DELETE_PARMS16;
typedef MCI_VD_DELETE_PARMS16 UNALIGNED *PMCI_VD_DELETE_PARMS16;
typedef VPVOID  VPMCI_VD_DELETE_PARMS16;

typedef struct _MCI_WAVE_OPEN_PARMS16 {   /* mwop16 */
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
    VPCSTR  lpstrDeviceType;
    VPCSTR  lpstrElementName;
    VPCSTR  lpstrAlias;
    DWORD   dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS16;
typedef MCI_WAVE_OPEN_PARMS16 UNALIGNED *PMCI_WAVE_OPEN_PARMS16;
typedef VPVOID  VPMCI_WAVE_OPEN_PARMS16;

typedef struct _MCI_WAVE_SET_PARMS16 {    /* mwsp16 */
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    WORD    wInput;
    WORD    wReserved0;
    WORD    wOutput;
    WORD    wReserved1;
    WORD    wFormatTag;
    WORD    wReserved2;
    WORD    nChannels;
    WORD    wReserved3;
    DWORD   nSamplesPerSecond;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
    WORD    wReserved4;
    WORD    wBitsPerSample;
    WORD    wReserved5;
} MCI_WAVE_SET_PARMS16;
typedef MCI_WAVE_SET_PARMS16 UNALIGNED *PMCI_WAVE_SET_PARMS16;
typedef VPVOID  VPMCI_WAVE_SET_PARMS16;

typedef struct _MIDIHDR16 {               /* mhdr16 */
    VPSTR   lpData;
    DWORD   dwBufferLength;
    DWORD   dwBytesRecorded;
    DWORD   dwUser;
    DWORD   dwFlags;
    struct  _MIDIHDR16 far *lpNext;
    DWORD   reserved;
} MIDIHDR16;
typedef MIDIHDR16 UNALIGNED *PMIDIHDR16;
typedef VPVOID  VPMIDIHDR16;

typedef struct _MIDIINCAPS16 {            /* mic16 */
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
} MIDIINCAPS16;
typedef MIDIINCAPS16 UNALIGNED *PMIDIINCAPS16;
typedef VPVOID  VPMIDIINCAPS16;

typedef struct _MIDIOUTCAPS16 {           /* moc16 */
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    WORD    wTechnology;
    WORD    wVoices;
    WORD    wNotes;
    WORD    wChannelMask;
    DWORD   dwSupport;
} MIDIOUTCAPS16;
typedef MIDIOUTCAPS16 UNALIGNED *PMIDIOUTCAPS16;
typedef VPVOID VPMIDIOUTCAPS16;

typedef struct _MMCKINFO16 {              /* mcki16 */
    FOURCC  ckid;
    DWORD   cksize;
    FOURCC  fccType;
    DWORD   dwDataOffset;
    DWORD   dwFlags;
} MMCKINFO16;
typedef MMCKINFO16 UNALIGNED *PMMCKINFO16;
typedef VPVOID VPMMCKINFO16;

typedef struct _MMIOINFO16 {              /* mioi16 */
    DWORD   dwFlags;
    FOURCC  fccIOProc;
    VPMMIOPROC16  pIOProc;
    WORD    wErrorRet;
    HTASK16 htask;        // Header file MMSYSTEM.H differs from book
    LONG    cchBuffer;
    VPSTR   pchBuffer;
    VPSTR   pchNext;
    VPSTR   pchEndRead;
    VPSTR   pchEndWrite;
    LONG    lBufOffset;
    LONG    lDiskOffset;
    DWORD   adwInfo[3];   // The book says [4], MMSYSTEM.H doesn't
    DWORD   dwReserved1;
    DWORD   dwReserved2;
    HMMIO16 hmmio;
} MMIOINFO16;
typedef MMIOINFO16 UNALIGNED *PMMIOINFO16;
typedef VPVOID  VPMMIOINFO16;

typedef struct _MMPACTION16 {             /* mpa16 */
    BYTE    bMenuItem;
    BYTE    bActionCode;
    WORD    wTextOffset;
} MMPACTION16;
typedef MMPACTION16 UNALIGNED *PMMPACTION16;
typedef VPVOID  VPMMPACTION16;

typedef struct _MMPLABEL16 {              /* mpl16 */
    WORD    wFrameNum;
    WORD    wTextOffset;
} MMPLABEL16;
typedef MMPLABEL16 UNALIGNED *PMMPLABEL16;
typedef VPVOID  VPMMPLABEL16;

typedef struct _MMPMOVIEINFO16 {          /* mpmi16 */
    DWORD   dwFileVersion;
    DWORD   dwTotalFrames;
    DWORD   dwInitialFramesPerSecond;
    WORD    wPixelDepth;
    DWORD   dwMovieExtentX;
    DWORD   dwMovieExtentY;
    char    chFullMacName[128];
} MMPMOVIEINFO16;
typedef MMPMOVIEINFO16 UNALIGNED *PMMPMOVIEINFO16;
typedef VPVOID  VPMMPMOVIEINFO16;

/* XLATOFF */
typedef struct _MMTIME16 {                /* mmt16 */
    WORD    wType;
    union {
        DWORD   ms;
        DWORD   sample;
        DWORD   cb;
        struct {
            BYTE    hour;
            BYTE    min;
            BYTE    sec;
            BYTE    frame;
            BYTE    fps;
            BYTE    dummy;
        } smpte;
        struct {
            DWORD   songptrpos;
        } midi;
    } u;
} MMTIME16;
typedef MMTIME16 UNALIGNED *PMMTIME16;
/* XLATON */
typedef VPVOID  VPMMTIME16;

typedef struct _TIMECAPS16 {              /* timc16 */
    WORD    wPeriodMin;
    WORD    wPeriodMax;
} TIMECAPS16;
typedef TIMECAPS16 UNALIGNED *PTIMECAPS16;
typedef VPVOID  VPTIMECAPS16;

typedef struct _WAVEFORMAT16 {            /* wft16 */
    WORD    wFormatTag;
    WORD    nChannels;
    DWORD   nSamplesPerSec;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
} WAVEFORMAT16;
typedef WAVEFORMAT16 UNALIGNED *PWAVEFORMAT16;
typedef VPVOID  VPWAVEFORMAT16;

typedef struct _PCMWAVEFORMAT16 {         /* pwf16 */
    WAVEFORMAT16  wf;
    WORD          wBitsPerSample;
} PCMWAVEFORMAT16;
typedef PCMWAVEFORMAT16 UNALIGNED *PPCMWAVEFORMAT16;
typedef VPVOID  VPPCMWAVEFORMAT16;

typedef struct _WAVEHDR16 {               /* whd16 */
    VPSTR   lpData;
    DWORD   dwBufferLength;
    DWORD   dwBytesRecorded;
    DWORD   dwUser;
    DWORD   dwFlags;
    DWORD   dwLoops;
    struct _WAVEHDR16 far *lpNext;
    DWORD   reserved;
} WAVEHDR16;
typedef WAVEHDR16 UNALIGNED *PWAVEHDR16;
typedef VPVOID  VPWAVEHDR16;

typedef struct _WAVEINCAPS16 {            /* wic16 */
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    DWORD   dwFormats;
    WORD    wChannels;
} WAVEINCAPS16;
typedef WAVEINCAPS16 UNALIGNED *PWAVEINCAPS16;
typedef VPVOID  VPWAVEINCAPS16;

typedef struct _WAVEOUTCAPS16 {           /* woc16 */
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    DWORD   dwFormats;
    WORD    wChannels;
    DWORD   dwSupport;
} WAVEOUTCAPS16;
typedef WAVEOUTCAPS16 UNALIGNED *PWAVEOUTCAPS16;
typedef VPVOID  VPWAVEOUTCAPS16;

/* XLATOFF */
#pragma pack()
/* XLATON */

/*
 * MultiMedia Window messages - MikeTri 10-Feb-1992
 */

//#define MM_JOY1MOVE         0x03A0   // Joystick
//#define MM_JOY2MOVE         0x03A1
//#define MM_JOY1ZMOVE        0x03A2
//#define MM_JOY2ZMOVE        0x03A3
//#define MM_JOY1BUTTONDOWN   0x03B5
//#define MM_JOY2BUTTONDOWN   0x03B6
//#define MM_JOY1BUTTONUP     0x03B7
//#define MM_JOY2BUTTONUP     0x03B8

//#define MM_MCINOTIFY        0x03B9  // MCI
//#define MM_MCISYSTEM_STRING 0x03CA

//#define MM_WOM_OPEN         0x03BB  // Waveform Output
//#define MM_WOM_CLOSE        0x03BC
//#define MM_WOM_DONE         0x03BD

//#define MM_WIM_OPEN         0x03BE  // Waveform Input
//#define MM_WIM_CLOSE        0x03BF
//#define MM_WIM_DATA         0x03C0

//#define MM_MIM_OPEN         0x03C1  // MIDI Input
//#define MM_MIM_CLOSE        0x03C2
//#define MM_MIM_DATA         0x03C3
//#define MM_MIM_LONGDATA     0x03C4
//#define MM_MIM_ERROR        0x03C5
//#define MM_MIM_LONGERROR    0x03C6

//#define MM_MOM_OPEN         0x03C7  // MIDI Output
//#define MM_MOM_CLOSE        0x03C8
//#define MM_MOM_DONE         0x03C9

/*
 * End of MultiMedia Window messages - MikeTri
 */

/*++

  MultiMedia API IDs - start adding all the other APIs - Mike, 04-Feb-1992

  This is the complete exported list, in MMSYSTEM order

  Well, actually it isn't anymore - various ones have been removed which
  we aren't supporting, and because of the joys of H2INC I can't leave them
  here as comments. So - if they need to be added again check in the
  function prototypes below this list for the correct formats.

  Need to recheck the numbers before compiling this lot... - this raises the
  point that if we add any more they ought to go at the end of the list,
  otherwise we end up juggling numbers, which is a drag.

--*/


#define FUN_MMCALLPROC32                  2 //
#define FUN_MMSYSTEMGETVERSION            5 //

#define FUN_OUTPUTDEBUGSTR                30 //
#define FUN_DRIVERCALLBACK                31 //
#define FUN_NOTIFY_CALLBACK_DATA          32 //

#define FUN_JOYGETNUMDEVS                 101 //
#define FUN_JOYGETDEVCAPS                 102 //
#define FUN_JOYGETPOS                     103 //
#define FUN_JOYGETTHRESHOLD               104 //
#define FUN_JOYRELEASECAPTURE             105 //
#define FUN_JOYSETCAPTURE                 106 //
#define FUN_JOYSETTHRESHOLD               107 //
#define FUN_JOYSETCALIBRATION             109 //

#define FUN_MIDIOUTGETNUMDEVS             201 //
#define FUN_MIDIOUTGETDEVCAPS             202 //
#define FUN_MIDIOUTGETERRORTEXT           203 //
#define FUN_MIDIOUTOPEN                   204 //
#define FUN_MIDIOUTCLOSE                  205 //
#define FUN_MIDIOUTPREPAREHEADER32        206 //
#define FUN_MIDIOUTUNPREPAREHEADER32      207 //
#define FUN_MIDIOUTSHORTMSG               208 //
#define FUN_MIDIOUTLONGMSG                209 //
#define FUN_MIDIOUTRESET                  210 //
#define FUN_MIDIOUTGETVOLUME              211 //
#define FUN_MIDIOUTSETVOLUME              212 //
#define FUN_MIDIOUTCACHEPATCHES           213 //
#define FUN_MIDIOUTCACHEDRUMPATCHES       214 //
#define FUN_MIDIOUTGETID                  215 //
#define FUN_MIDIOUTMESSAGE32              216 //

#define FUN_MIDIINGETNUMDEVS              301 //
#define FUN_MIDIINGETDEVCAPS              302 //
#define FUN_MIDIINGETERRORTEXT            303 //
#define FUN_MIDIINOPEN                    304 //
#define FUN_MIDIINCLOSE                   305 //
#define FUN_MIDIINPREPAREHEADER32         306 //
#define FUN_MIDIINUNPREPAREHEADER32       307 //
#define FUN_MIDIINADDBUFFER               308 //
#define FUN_MIDIINSTART                   309 //
#define FUN_MIDIINSTOP                    310 //
#define FUN_MIDIINRESET                   311 //
#define FUN_MIDIINGETID                   312 //
#define FUN_MIDIINMESSAGE32               313 //

#define FUN_AUXGETNUMDEVS                 350 //
#define FUN_AUXGETDEVCAPS                 351 //
#define FUN_AUXGETVOLUME                  352 //
#define FUN_AUXSETVOLUME                  353 //
#define FUN_AUXOUTMESSAGE32               354 //

#define FUN_WAVEOUTGETNUMDEVS             401 //
#define FUN_WAVEOUTGETDEVCAPS             402 //
#define FUN_WAVEOUTGETERRORTEXT           403 //
#define FUN_WAVEOUTOPEN                   404 //
#define FUN_WAVEOUTCLOSE                  405 //
#define FUN_WAVEOUTPREPAREHEADER32        406 //
#define FUN_WAVEOUTUNPREPAREHEADER32      407 //
#define FUN_WAVEOUTWRITE                  408 //
#define FUN_WAVEOUTPAUSE                  409 //
#define FUN_WAVEOUTRESTART                410 //
#define FUN_WAVEOUTRESET                  411 //
#define FUN_WAVEOUTGETPOSITION            412 //
#define FUN_WAVEOUTGETPITCH               413 //
#define FUN_WAVEOUTSETPITCH               414 //
#define FUN_WAVEOUTGETVOLUME              415 //
#define FUN_WAVEOUTSETVOLUME              416 //
#define FUN_WAVEOUTGETPLAYBACKRATE        417 //
#define FUN_WAVEOUTSETPLAYBACKRATE        418 //
#define FUN_WAVEOUTBREAKLOOP              419 //
#define FUN_WAVEOUTGETID                  420 //
#define FUN_WAVEOUTMESSAGE32              421 //

#define FUN_WAVEINGETNUMDEVS              501 //
#define FUN_WAVEINGETDEVCAPS              502 //
#define FUN_WAVEINGETERRORTEXT            503 //
#define FUN_WAVEINOPEN                    504 //
#define FUN_WAVEINCLOSE                   505 //
#define FUN_WAVEINPREPAREHEADER32         506 //
#define FUN_WAVEINUNPREPAREHEADER32       507 //
#define FUN_WAVEINADDBUFFER               508 //
#define FUN_WAVEINSTART                   509 //
#define FUN_WAVEINSTOP                    510 //
#define FUN_WAVEINRESET                   511 //
#define FUN_WAVEINGETPOSITION             512 //
#define FUN_WAVEINGETID                   513 //
#define FUN_WAVEINMESSAGE32               514 //

#define FUN_TIMEGETSYSTEMTIME             601 //
#define FUN_TIMEGETTIME                   607 //
#define FUN_TIMESETEVENT                  602 //
#define FUN_TIMEKILLEVENT                 603 //
#define FUN_TIMEGETDEVCAPS                604 //
#define FUN_TIMEBEGINPERIOD               605 //
#define FUN_TIMEENDPERIOD                 606 //

#define FUN_MCISENDCOMMAND                701 //
#define FUN_MCISENDSTRING                 702 //
#define FUN_MCIGETDEVICEID                703 //
#define FUN_MCIGETERRORSTRING             706 //
#define FUN_MCIEXECUTE                    712 //
#define FUN_MCISETYIELDPROC               714 //
#define FUN_MCIGETDEVICEIDFROMELEMENTID   715 //
#define FUN_MCIGETYIELDPROC               716 //
#define FUN_MCIGETCREATORTASK             717 //

#define FUN_MMIOOPEN                      1210 //
#define FUN_MMIOCLOSE                     1211 //
#define FUN_MMIOREAD                      1212 //
#define FUN_MMIOWRITE                     1213 //
#define FUN_MMIOSEEK                      1214 //
#define FUN_MMIOGETINFO                   1215 //
#define FUN_MMIOSETINFO                   1216 //
#define FUN_MMIOSETBUFFER                 1217 //
#define FUN_MMIOFLUSH                     1218 //
#define FUN_MMIOADVANCE                   1219 //
#define FUN_MMIOSTRINGTOFOURCC            1220 //
#define FUN_MMIOINSTALLIOPROC             1221 //
#define FUN_MMIOSENDMESSAGE               1222 //

#define FUN_MMIODESCEND                   1223 //
#define FUN_MMIOASCEND                    1224 //
#define FUN_MMIOCREATECHUNK               1225 //
#define FUN_MMIORENAME                    1226 //

/* XLATOFF */
#pragma pack(2)
/* XLATON */

/*++

  Function prototypes - the seemingly unimportant number in the comment on
  each function MUST match the ones in the list above - otherwise you will
  turn into a frog, and 16 bit MultiMedia will wave its feet in the air...

  !! BE WARNED !!

--*/

typedef struct _MMCALLPROC3216 {    /* mm2 */
    DWORD fSetCurrentDirectory;     /* Set the current directory ? */
    DWORD lpProcAddress;            /* function to call */
    DWORD p1;                       /* dwParam2     */
    DWORD p2;                       /* dwParam1     */
    DWORD p3;                       /* dwInstance   */
    DWORD p4;                       /* uMsg         */
    DWORD p5;                       /* uDevId       */
} MMCALLPROC3216;
typedef MMCALLPROC3216 UNALIGNED *PMMCALLPROC3216;


#ifdef NULLSTRUCT
typedef struct _MMSYSTEMGETVERSION16 {         /* mm5 */
} MMSYSTEMGETVERSION16;
typedef MMSYSTEMGETVERSION16 UNALIGNED *PMMSYSTEMGETVERSION;
#endif

typedef struct _OUTPUTDEBUGSTR16 {             /* mm30 */
    VPSTR      f1;
} OUTPUTDEBUGSTR16;
typedef OUTPUTDEBUGSTR16 UNALIGNED *POUTPUTDEBUGSTR16;

typedef struct _DRIVERCALLBACK16 {             /* mm31 */
    DWORD      f7;
    DWORD      f6;
    DWORD      f5;
    DWORD      f4;
    HDRVR16    f3;
    DWORD      f2;
    DWORD      f1;
} DRIVERCALLBACK16;
typedef DRIVERCALLBACK16 UNALIGNED *PDRIVERCALLBACK16;


typedef struct _NOTIFY_CALLBACK_DATA16 {       /* mm32 */
    VPCALLBACK_DATA f1;
} NOTIFY_CALLBACK_DATA16;
typedef NOTIFY_CALLBACK_DATA16 UNALIGNED *PNOTIFY_CALLBACK_DATA16;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef NULLSTRUCT
typedef struct _STACKLEAVE16 {                  mm33
} STACKLEAVE16;
typedef STACKLEAVE16 UNALIGNED *PSTACKLEAVE16;
#endif

------------------------------------------------------------------------------*/

#ifdef NULLSTRUCT
typedef struct _JOYGETNUMDEVS16 {              /* mm101 */
} JOYGETNUMDEVS16;
typedef JOYGETNUMDEVS16 UNALIGNED *PJOYGETNUMDEVS16;
#endif

typedef struct _JOYGETDEVCAPS16 {              /* mm102 */
    WORD         f3;
    VPJOYCAPS16  f2;
    WORD         f1;
} JOYGETDEVCAPS16;
typedef JOYGETDEVCAPS16 UNALIGNED *PJOYGETDEVCAPS16;

typedef struct _JOYGETPOS16 {                  /* mm103 */
    VPJOYINFO16  f2;
    WORD         f1;
} JOYGETPOS16;
typedef JOYGETPOS16 UNALIGNED *PJOYGETPOS16;

typedef struct _JOYGETTHRESHOLD16 {            /* mm104 */
    VPWORD     f2;
    WORD       f1;
} JOYGETTHRESHOLD16;
typedef JOYGETTHRESHOLD16 UNALIGNED *PJOYGETTHRESHOLD16;

typedef struct _JOYRELEASECAPTURE16 {          /* mm105 */
    WORD       f1;
} JOYRELEASECAPTURE16;
typedef JOYRELEASECAPTURE16 UNALIGNED *PJOYRELEASECAPTURE16;

typedef struct _JOYSETCAPTURE16 {              /* mm106 */
    BOOL16     f4;
    WORD       f3;
    WORD       f2;
    HWND16     f1;
} JOYSETCAPTURE16;
typedef JOYSETCAPTURE16 UNALIGNED *PJOYSETCAPTURE16;

typedef struct _JOYSETTHRESHOLD16 {            /* mm107 */
    WORD       f2;
    WORD       f1;
} JOYSETTHRESHOLD16;
typedef JOYSETTHRESHOLD16 UNALIGNED *PJOYSETTHRESHOLD16;

typedef struct _JOYSETCALIBRATION16 {          /* mm109 */
    VPWORD     f7;
    VPWORD     f6;
    VPWORD     f5;
    VPWORD     f4;
    VPWORD     f3;
    VPWORD     f2;
    WORD       f1;
} JOYSETCALIBRATION16;
typedef JOYSETCALIBRATION16 UNALIGNED *PJOYSETCALIBRATION16;

#ifdef NULLSTRUCT
typedef struct _MIDIOUTGETNUMDEVS16 {          /* mm201 */
} MIDIOUTGETNUMDEVS16;
typedef MIDIOUTGETNUMDEVS16 UNALIGNED *PMIDIOUTGETNUMDEVS16;
#endif

typedef struct _MIDIOUTGETDEVCAPS16 {          /* mm202 */
    WORD            f3;
    VPMIDIOUTCAPS16 f2;
    WORD            f1;
} MIDIOUTGETDEVCAPS16;
typedef MIDIOUTGETDEVCAPS16 UNALIGNED *PMIDIOUTGETDEVCAPS16;

typedef struct _MIDIOUTGETERRORTEXT16 {        /* mm203 */
    WORD       f3;
    VPSTR      f2;
    WORD       f1;
} MIDIOUTGETERRORTEXT16;
typedef MIDIOUTGETERRORTEXT16 UNALIGNED *PMIDIOUTGETERRORTEXT16;

typedef struct _MIDIOUTOPEN16 {                /* mm204 */
    DWORD         f5;
    DWORD         f4;
    DWORD         f3;
    WORD          f2;
    VPHMIDIOUT16  f1;
} MIDIOUTOPEN16;
typedef MIDIOUTOPEN16 UNALIGNED *PMIDIOUTOPEN16;

typedef struct _MIDIOUTCLOSE16 {               /* mm205 */
    HMIDIOUT16    f1;
} MIDIOUTCLOSE16;
typedef MIDIOUTCLOSE16 UNALIGNED *PMIDIOUTCLOSE16;

typedef struct _MIDIOUTPREPAREHEADER3216 {       /* mm206 */
    WORD         f3;
    VPMIDIHDR16  f2;
    HMIDIOUT16   f1;
} MIDIOUTPREPAREHEADER3216;
typedef MIDIOUTPREPAREHEADER3216 UNALIGNED *PMIDIOUTPREPAREHEADER3216;

typedef struct _MIDIOUTUNPREPAREHEADER3216 {     /* mm207 */
    WORD         f3;
    VPMIDIHDR16  f2;
    HMIDIOUT16   f1;
} MIDIOUTUNPREPAREHEADER3216;
typedef MIDIOUTUNPREPAREHEADER3216 UNALIGNED *PMIDIOUTUNPREPAREHEADER3216;

typedef struct _MIDIOUTSHORTMSG16 {            /* mm208 */
    DWORD      f2;
    HMIDIOUT16 f1;
} MIDIOUTSHORTMSG16;
typedef MIDIOUTSHORTMSG16 UNALIGNED *PMIDIOUTSHORTMSG16;

typedef struct _MIDIOUTLONGMSG16 {             /* mm209 */
    WORD        f3;
    VPMIDIHDR16 f2;
    HMIDIOUT16  f1;
} MIDIOUTLONGMSG16;
typedef MIDIOUTLONGMSG16 UNALIGNED *PMIDIOUTLONGMSG16;

typedef struct _MIDIOUTRESET16 {               /* mm210 */
    HMIDIOUT16  f1;
} MIDIOUTRESET16;
typedef MIDIOUTRESET16 UNALIGNED *PMIDIOUTRESET16;

typedef struct _MIDIOUTGETVOLUME16 {           /* mm211 */
    VPDWORD    f2;
    WORD       f1;
} MIDIOUTGETVOLUME16;
typedef MIDIOUTGETVOLUME16 UNALIGNED *PMIDIOUTGETVOLUME16;

typedef struct _MIDIOUTSETVOLUME16 {           /* mm212 */
    DWORD      f2;
    WORD       f1;
} MIDIOUTSETVOLUME16;
typedef MIDIOUTSETVOLUME16 UNALIGNED *PMIDIOUTSETVOLUME16;

typedef struct _MIDIOUTCACHEPATCHES16 {        /* mm213 */
    WORD            f4;
    VPPATCHARRAY16  f3;
    WORD            f2;
    HMIDIOUT16      f1;
} MIDIOUTCACHEPATCHES16;
typedef MIDIOUTCACHEPATCHES16 UNALIGNED *PMIDIOUTCACHEPATCHES16;

typedef struct _MIDIOUTCACHEDRUMPATCHES16 {    /* mm214 */
    WORD            f4;
    VPKEYARRAY16    f3;
    WORD            f2;
    HMIDIOUT16      f1;
} MIDIOUTCACHEDRUMPATCHES16;
typedef MIDIOUTCACHEDRUMPATCHES16 UNALIGNED *PMIDIOUTCACHEDRUMPATCHES16;

typedef struct _MIDIOUTGETID16 {               /* mm215 */
    VPWORD          f2;
    HMIDIOUT16      f1;
} MIDIOUTGETID16;
typedef MIDIOUTGETID16 UNALIGNED *PMIDIOUTGETID16;

typedef struct _MIDIOUTMESSAGE3216 {             /* mm216 */
    DWORD      f4;
    DWORD      f3;
    WORD       f2;
    HMIDIOUT16 f1;
} MIDIOUTMESSAGE3216;
typedef MIDIOUTMESSAGE3216 UNALIGNED *PMIDIOUTMESSAGE3216;

#ifdef NULLSTRUCT
typedef struct _MIDIINGETNUMDEVS16 {           /* mm301 */
} MIDIINGETNUMDEVS16;
typedef MIDIINGETNUMDEVS16 UNALIGNED *PMIDIINGETNUMDEVS16;
#endif

typedef struct _MIDIINGETDEVCAPS16 {           /* mm302 */
    WORD            f3;
    VPMIDIINCAPS16  f2;
    WORD            f1;
} MIDIINGETDEVCAPS16;
typedef MIDIINGETDEVCAPS16 UNALIGNED *PMIDIINGETDEVCAPS16;

typedef struct _MIDIINGETERRORTEXT16 {         /* mm303 */
    WORD       f3;
    VPSTR      f2;
    WORD       f1;
} MIDIINGETERRORTEXT16;
typedef MIDIINGETERRORTEXT16 UNALIGNED *PMIDIINGETERRORTEXT16;

typedef struct _MIDIINOPEN16 {                 /* mm304 */
    DWORD         f5;
    DWORD         f4;
    DWORD         f3;
    WORD          f2;
    VPHMIDIIN16   f1;
} MIDIINOPEN16;
typedef MIDIINOPEN16 UNALIGNED *PMIDIINOPEN16;

typedef struct _MIDIINCLOSE16 {                /* mm305 */
    HMIDIIN16  f1;
} MIDIINCLOSE16;
typedef MIDIINCLOSE16 UNALIGNED *PMIDIINCLOSE16;

typedef struct _MIDIINPREPAREHEADER3216 {        /* mm306 */
    WORD         f3;
    VPMIDIHDR16  f2;
    HMIDIIN16    f1;
} MIDIINPREPAREHEADER3216;
typedef MIDIINPREPAREHEADER3216 UNALIGNED *PMIDIINPREPAREHEADER3216;

typedef struct _MIDIINUNPREPAREHEADER3216 {      /* mm307 */
    WORD         f3;
    VPMIDIHDR16  f2;
    HMIDIIN16    f1;
} MIDIINUNPREPAREHEADER3216;
typedef MIDIINUNPREPAREHEADER3216 UNALIGNED *PMIDIINUNPREPAREHEADER3216;

typedef struct _MIDIINADDBUFFER16 {            /* mm308 */
    WORD         f3;
    VPMIDIHDR16  f2;
    HMIDIIN16    f1;
} MIDIINADDBUFFER16;
typedef MIDIINADDBUFFER16 UNALIGNED *PMIDIINADDBUFFER16;

typedef struct _MIDIINSTART16 {                /* mm309 */
    HMIDIIN16  f1;
} MIDIINSTART16;
typedef MIDIINSTART16 UNALIGNED *PMIDIINSTART16;

typedef struct _MIDIINSTOP16 {                 /* mm310 */
    HMIDIIN16  f1;
} MIDIINSTOP16;
typedef MIDIINSTOP16 UNALIGNED *PMIDIINSTOP16;

typedef struct _MIDIINRESET16 {                /* mm311 */
    HMIDIIN16  f1;
} MIDIINRESET16;
typedef MIDIINRESET16 UNALIGNED *PMIDIINRESET16;

typedef struct _MIDIINGETID16 {                /* mm312 */
    VPWORD     f2;
    HMIDIIN16  f1;
} MIDIINGETID16;
typedef MIDIINGETID16 UNALIGNED *PMIDIINGETID16;

typedef struct _MIDIINMESSAGE3216 {              /* mm313 */
    DWORD      f4;
    DWORD      f3;
    WORD       f2;
    HMIDIIN16  f1;
} MIDIINMESSAGE3216;
typedef MIDIINMESSAGE3216 UNALIGNED *PMIDIINMESSAGE3216;

#ifdef NULLSTRUCT
typedef struct _AUXGETNUMDEVS16 {              /* mm350 */
} AUXGETNUMDEVS16;
typedef AUXGETNUMDEVS16 UNALIGNED *PAUGGETNUMDEVS16;
#endif

typedef struct _AUXGETDEVCAPS16 {              /* mm351 */
    WORD         f3;
    VPAUXCAPS16  f2;
    WORD         f1;
} AUXGETDEVCAPS16;
typedef AUXGETDEVCAPS16 UNALIGNED *PAUXGETDEVCAPS16;

typedef struct _AUXGETVOLUME16 {               /* mm352 */
    VPDWORD    f2;
    WORD       f1;
} AUXGETVOLUME16;
typedef AUXGETVOLUME16 UNALIGNED *PAUXGETVOLUME16;

typedef struct _AUXSETVOLUME16 {               /* mm353 */
    DWORD      f2;
    WORD       f1;
} AUXSETVOLUME16;
typedef AUXSETVOLUME16 UNALIGNED *PAUXSETVOLUME16;

typedef struct _AUXOUTMESSAGE3216 {              /* mm354 */
    DWORD      f4;
    DWORD      f3;
    WORD       f2;
    WORD       f1;
} AUXOUTMESSAGE3216;
typedef AUXOUTMESSAGE3216 UNALIGNED *PAUXOUTMESSAGE3216;

#ifdef NULLSTRUCT
typedef struct _WAVEOUTGETNUMDEVS16 {          /* mm401 */
} WAVEOUTGETNUMDEVS16;
typedef WAVEOUTGETNUMDEVS16 UNALIGNED *PWAVEOUTGETNUMDEVS16;
#endif

typedef struct _WAVEOUTGETDEVCAPS16 {          /* mm402 */
    WORD             f3;
    VPWAVEOUTCAPS16  f2;
    WORD             f1;
} WAVEOUTGETDEVCAPS16;
typedef WAVEOUTGETDEVCAPS16 UNALIGNED *PWAVEOUTGETDEVCAPS16;

typedef struct _WAVEOUTGETERRORTEXT16 {        /* mm403 */
    WORD       f3;
    VPSTR      f2;
    WORD       f1;
} WAVEOUTGETERRORTEXT16;
typedef WAVEOUTGETERRORTEXT16 UNALIGNED *PWAVEOUTGETERRORTEXT16;

typedef struct _WAVEOUTOPEN16 {                /* mm404 */
    DWORD           f6;
    DWORD           f5;
    DWORD           f4;
    VPWAVEFORMAT16  f3;
    WORD            f2;
    VPHWAVEOUT16   f1;
} WAVEOUTOPEN16;
typedef WAVEOUTOPEN16 UNALIGNED *PWAVEOUTOPEN16;

typedef struct _WAVEOUTCLOSE16 {               /* mm405 */
    HWAVEOUT16 f1;
} WAVEOUTCLOSE16;
typedef WAVEOUTCLOSE16 UNALIGNED *PWAVEOUTCLOSE16;

typedef struct _WAVEOUTPREPAREHEADER3216 {       /* mm406 */
    WORD         f3;
    VPWAVEHDR16  f2;
    HWAVEOUT16   f1;
} WAVEOUTPREPAREHEADER3216;
typedef WAVEOUTPREPAREHEADER3216 UNALIGNED *PWAVEOUTPREPAREHEADER3216;

typedef struct _WAVEOUTUNPREPAREHEADER3216 {     /* mm407 */
    WORD         f3;
    VPWAVEHDR16  f2;
    HWAVEOUT16   f1;
} WAVEOUTUNPREPAREHEADER3216;
typedef WAVEOUTUNPREPAREHEADER3216 UNALIGNED *PWAVEOUTUNPREPAREHEADER3216;

typedef struct _WAVEOUTWRITE16 {               /* mm408 */
    WORD         f3;
    VPWAVEHDR16  f2;
    HWAVEOUT16   f1;
} WAVEOUTWRITE16;
typedef WAVEOUTWRITE16 UNALIGNED *PWAVEOUTWRITE16;

typedef struct _WAVEOUTPAUSE16 {               /* mm409 */
    HWAVEOUT16   f1;
} WAVEOUTPAUSE16;
typedef WAVEOUTPAUSE16 UNALIGNED *PWAVEOUTPAUSE16;

typedef struct _WAVEOUTRESTART16 {             /* mm410 */
    HWAVEOUT16   f1;
} WAVEOUTRESTART16;
typedef WAVEOUTRESTART16 UNALIGNED *PWAVEOUTRESTART16;

typedef struct _WAVEOUTRESET16 {               /* mm411 */
    HWAVEOUT16   f1;
} WAVEOUTRESET16;
typedef WAVEOUTRESET16 UNALIGNED *PWAVEOUTRESET16;

typedef struct _WAVEOUTGETPOSITION16 {         /* mm412 */
    WORD         f3;
    VPMMTIME16   f2;
    HWAVEOUT16   f1;
} WAVEOUTGETPOSITION16;
typedef WAVEOUTGETPOSITION16 UNALIGNED *PWAVEOUTGETPOSITION16;

typedef struct _WAVEOUTGETPITCH16 {            /* mm413 */
    VPDWORD      f2;
    HWAVEOUT16   f1;
} WAVEOUTGETPITCH16;
typedef WAVEOUTGETPITCH16 UNALIGNED *PWAVEOUTGETPITCH16;

typedef struct _WAVEOUTSETPITCH16 {            /* mm414 */
    DWORD        f2;
    HWAVEOUT16   f1;
} WAVEOUTSETPITCH16;
typedef WAVEOUTSETPITCH16 UNALIGNED *PWAVEOUTSETPITCH16;

typedef struct _WAVEOUTGETVOLUME16 {           /* mm415 */
    VPDWORD    f2;
    WORD       f1;
} WAVEOUTGETVOLUME16;
typedef WAVEOUTGETVOLUME16 UNALIGNED *PWAVEOUTGETVOLUME16;

typedef struct _WAVEOUTSETVOLUME16 {           /* mm416 */
    DWORD      f2;
    WORD       f1;
} WAVEOUTSETVOLUME16;
typedef WAVEOUTSETVOLUME16 UNALIGNED *PWAVEOUTSETVOLUME16;

typedef struct _WAVEOUTGETPLAYBACKRATE16 {     /* mm417 */
    VPDWORD      f2;
    HWAVEOUT16   f1;
} WAVEOUTGETPLAYBACKRATE16;
typedef WAVEOUTGETPLAYBACKRATE16 UNALIGNED *PWAVEOUTGETPLAYBACKRATE16;

typedef struct _WAVEOUTSETPLAYBACKRATE16 {     /* mm418 */
    DWORD        f2;
    HWAVEOUT16   f1;
} WAVEOUTSETPLAYBACKRATE16;
typedef WAVEOUTSETPLAYBACKRATE16 UNALIGNED *PWAVEOUTSETPLAYBACKRATE16;

typedef struct _WAVEOUTBREAKLOOP16 {           /* mm419 */
    HWAVEOUT16   f1;
} WAVEOUTBREAKLOOP16;
typedef WAVEOUTBREAKLOOP16 UNALIGNED *PWAVEOUTBREAKLOOP16;

typedef struct _WAVEOUTGETID16 {               /* mm420 */
    VPWORD       f2;
    HWAVEOUT16   f1;
} WAVEOUTGETID16;
typedef WAVEOUTGETID16 UNALIGNED *PWAVEOUTGETID16;

typedef struct _WAVEOUTMESSAGE3216 {             /* mm421 */
    DWORD        f4;
    DWORD        f3;
    WORD         f2;
    HWAVEOUT16   f1;
} WAVEOUTMESSAGE3216;
typedef WAVEOUTMESSAGE3216 UNALIGNED *PWAVEOUTMESSAGE3216;

#ifdef NULLSTRUCT
typedef struct _WAVEINGETNUMDEVS16 {           /* mm501 */
} WAVEINGETNUMDEVS16;
typedef WAVEINGETNUMDEVS16 UNALIGNED *PWAVEINGETNUMDEVS16;
#endif

typedef struct _WAVEINGETDEVCAPS16 {           /* mm502 */
    WORD            f3;
    VPWAVEINCAPS16  f2;
    WORD            f1;
} WAVEINGETDEVCAPS16;
typedef WAVEINGETDEVCAPS16 UNALIGNED *PWAVEINGETDEVCAPS16;

typedef struct _WAVEINGETERRORTEXT16 {         /* mm503 */
    WORD       f3;
    VPSTR      f2;
    WORD       f1;
} WAVEINGETERRORTEXT16;
typedef WAVEINGETERRORTEXT16 UNALIGNED *PWAVEINGETERRORTEXT16;

typedef struct _WAVEINOPEN16 {                 /* mm504 */
    DWORD           f6;
    DWORD           f5;
    DWORD           f4;
    VPWAVEFORMAT16  f3;
    WORD            f2;
    VPHWAVEIN16    f1;
} WAVEINOPEN16;
typedef WAVEINOPEN16 UNALIGNED *PWAVEINOPEN16;

typedef struct _WAVEINCLOSE16 {                /* mm505 */
    HWAVEIN16  f1;
} WAVEINCLOSE16;
typedef WAVEINCLOSE16 UNALIGNED *PWAVEINCLOSE16;

typedef struct _WAVEINPREPAREHEADER3216 {        /* mm506 */
    WORD         f3;
    VPWAVEHDR16  f2;
    HWAVEIN16    f1;
} WAVEINPREPAREHEADER3216;
typedef WAVEINPREPAREHEADER3216 UNALIGNED *PWAVEINPREPAREHEADER3216;

typedef struct _WAVEINUNPREPAREHEADER3216 {      /* mm507 */
    WORD         f3;
    VPWAVEHDR16  f2;
    HWAVEIN16    f1;
} WAVEINUNPREPAREHEADER3216;
typedef WAVEINUNPREPAREHEADER3216 UNALIGNED *PWAVEINUNPREPAREHEADER3216;

typedef struct _WAVEINADDBUFFER16 {            /* mm508 */
    WORD         f3;
    VPWAVEHDR16  f2;
    HWAVEIN16    f1;
} WAVEINADDBUFFER16;
typedef WAVEINADDBUFFER16 UNALIGNED *PWAVEINADDBUFFER16;

typedef struct _WAVEINSTART16 {                /* mm509 */
    HWAVEIN16    f1;
} WAVEINSTART16;
typedef WAVEINSTART16 UNALIGNED *PWAVEINSTART16;

typedef struct _WAVEINSTOP16 {                 /* mm510 */
    HWAVEIN16    f1;
} WAVEINSTOP16;
typedef WAVEINSTOP16 UNALIGNED *PWAVEINSTOP16;

typedef struct _WAVEINRESET16 {                /* mm511 */
    HWAVEIN16    f1;
} WAVEINRESET16;
typedef WAVEINRESET16 UNALIGNED *PWAVEINRESET16;

typedef struct _WAVEINGETPOSITION16 {          /* mm512 */
    WORD       f3;
    VPMMTIME16 f2;
    HWAVEIN16  f1;
} WAVEINGETPOSITION16;
typedef WAVEINGETPOSITION16 UNALIGNED *PWAVEINGETPOSITION16;

typedef struct _WAVEINGETID16 {                /* mm513 */
    VPWORD     f2;
    HWAVEIN16  f1;
} WAVEINGETID16;
typedef WAVEINGETID16 UNALIGNED *PWAVEINGETID16;

typedef struct _WAVEINMESSAGE3216 {              /* mm514 */
    DWORD      f4;
    DWORD      f3;
    WORD       f2;
    HWAVEIN16  f1;
} WAVEINMESSAGE3216;
typedef WAVEINMESSAGE3216 UNALIGNED *PWAVEINMESSAGE3216;

typedef struct _TIMEGETSYSTEMTIME16 {          /* mm601 */
    WORD       f2;
    VPMMTIME16 f1;
} TIMEGETSYSTEMTIME16;
typedef TIMEGETSYSTEMTIME16 UNALIGNED *PTIMEGETSYSTEMTIME16;

#ifdef NULLSTRUCT
typedef struct _TIMEGETTIME16 {                /* mm607 */
} TIMEGETTIME16;
typedef TIMEGETTIME16 UNALIGNED *PTIMEGETTIME16;
#endif

typedef struct _TIMESETEVENT16 {               /* mm602 */
    WORD              f5;
    DWORD             f4;
    VPTIMECALLBACK16  f3;
    WORD              f2;
    WORD              f1;
} TIMESETEVENT16;
typedef TIMESETEVENT16 UNALIGNED *PTIMESETEVENT16;

typedef struct _TIMEKILLEVENT16 {              /* mm603 */
    WORD       f1;
} TIMEKILLEVENT16;
typedef TIMEKILLEVENT16 UNALIGNED *PTIMEKILLEVENT16;

typedef struct _TIMEGETDEVCAPS16 {             /* mm604 */
    WORD          f2;
    VPTIMECAPS16  f1;
} TIMEGETDEVCAPS16;
typedef TIMEGETDEVCAPS16 UNALIGNED *PTIMEGETDEVCAPS16;

typedef struct _TIMEBEGINPERIOD16 {            /* mm605 */
    WORD       f1;
} TIMEBEGINPERIOD16;
typedef TIMEBEGINPERIOD16 UNALIGNED *PTIMEBEGINPERIOD16;

typedef struct _TIMEENDPERIOD16 {              /* mm606 */
    WORD       f1;
} TIMEENDPERIOD16;
typedef TIMEENDPERIOD16 UNALIGNED *PTIMEENDPERIOD16;

typedef struct _MCISENDCOMMAND16 {             /* mm701 */
    DWORD      f4;
    DWORD      f3;
    WORD       f2;
    WORD       f1;
} MCISENDCOMMAND16;
typedef MCISENDCOMMAND16 UNALIGNED *PMCISENDCOMMAND16;

typedef struct _MCISENDSTRING16 {              /* mm702 */
    HWND16     f4;
    WORD       f3;
    VPSTR      f2;
    VPCSTR     f1;
} MCISENDSTRING16;
typedef MCISENDSTRING16 UNALIGNED *PMCISENDSTRING16;

typedef struct _MCIGETDEVICEID16 {             /* mm703 */
    VPCSTR     f1;
} MCIGETDEVICEID16;
typedef MCIGETDEVICEID16 UNALIGNED *PMCIGETDEVICEID16;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct _MCIPARSECOMMAND16 {            mm704
    BOOL16     f6;
    VPWORD     f5;
    VPSTR      f4;
    VPCSTR     f3;
    VPSTR      f2;
    WORD       f1;
} MCIPARSECOMMAND16;
typedef MCIPARSECOMMAND16 UNALIGNED *PMCIPARSECOMMAND16;

typedef struct _MCILOADCOMMANDRESOURCE16 {     mm705
    WORD       f3;
    VPCSTR     f2;
    HAND16     f1;
} MCILOADCOMMANDRESOURCE16;
typedef MCILOADCOMMANDRESOURCE16 UNALIGNED *PMCILOADCOMMANDRESOURCE16;

------------------------------------------------------------------------------*/

typedef struct _MCIGETERRORSTRING16 {          /* mm706 */
    WORD       f3;
    VPSTR      f2;
    DWORD      f1;
} MCIGETERRORSTRING16;
typedef MCIGETERRORSTRING16 UNALIGNED *PMCIGETERRORSTRING16;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct _MCISETDRIVERDATA16 {           mm707
    DWORD      f2;
    WORD       f1;
} MCISETDRIVERDATA16;
typedef MCISETDRIVERDATA16 UNALIGNED *PMCISETDRIVERDATA16;

typedef struct _MCIGETDRIVERDATA16 {           mm708
    WORD       f1;
} MCIGETDRIVERDATA16;
typedef MCIGETDRIVERDATA16 UNALIGNED *PMCIGETDRIVERDATA16;

typedef struct _MCIDRIVERYIELD16 {             mm710
    WORD       f1;
} MCIDRIVERYIELD16;
typedef MCIDRIVERYIELD16 UNALIGNED *PMCIDRIVERYIELD16;

typedef struct _MCIDRIVERNOTIFY16 {            mm711
    WORD       f3;
    WORD       f2;
    HWND16     f1;
} MCIDRIVERNOTIFY16;
typedef MCIDRIVERNOTIFY16 UNALIGNED *PMCIDRIVERNOTIFY16;

------------------------------------------------------------------------------*/

typedef struct _MCIEXECUTE16 {                 /* mm712 */
    VPCSTR     f1;
} MCIEXECUTE16;
typedef MCIEXECUTE16 UNALIGNED *PMCIEXECUTE16;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct _MCIFREECOMMANDRESOURCE16 {     mm713
    WORD       f1;
} MCIFREECOMMANDRESOURCE16;
typedef MCIFREECOMMANDRESOURCE16 UNALIGNED *PMCIFREECOMMANDRESOURCE16;

------------------------------------------------------------------------------*/

typedef struct _MCISETYIELDPROC16 {          /* mm714 */
    DWORD      f3;
    DWORD      f2; //YIELDPROC
    WORD       f1;
} MCISETYIELDPROC16;
typedef MCISETYIELDPROC16 UNALIGNED *PMCISETYIELDPROC16;


typedef struct _MCIGETDEVICEIDFROMELEMENTID16 {     /* mm715 */
    VPCSTR     f2;
    DWORD      f1;
} MCIGETDEVICEIDFROMELEMENTID16;
typedef MCIGETDEVICEIDFROMELEMENTID16 UNALIGNED *PMCIGETDEVICEIDFROMELEMENTID16;

typedef struct _MCIGETYIELDPROC16 {            /* mm716 */
    VPDWORD    f2;
    WORD       f1;
} MCIGETYIELDPROC16;
typedef MCIGETYIELDPROC16 UNALIGNED *PMCIGETYIELDPROC16;


typedef struct _MCIGETCREATORTASK16 {          /* mm717 */
    WORD       f1;
} MCIGETCREATORTASK16;
typedef MCIGETCREATORTASK16 UNALIGNED *PMCIGETCREATORTASK16;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  The following calls have all been zapped for the time being.

typedef struct _MMTASKCREATE16 {               mm900
    DWORD             f3;
    HAND16            f2;
    VPTASKCALLBACK16  f1;
} MMTASKCREATE16;
typedef MMTASKCREATE16 UNALIGNED *PMMTASKCREATE16;

typedef struct _MMTASKBLOCK16 {                mm902
    HAND16     f1;
} MMTASKBLOCK16;
typedef MMTASKBLOCK16 UNALIGNED *PMMTASKBLOCK16;

typedef struct _MMTASKSIGNAL16 {               mm903
    HAND16     f1;
} MMTASKSIGNAL16;
typedef MMTASKSIGNAL16 UNALIGNED *PMMTASKSIGNAL16;

#ifdef NULLSTRUCT
typedef struct _MMGETCURRENTTASK16 {           mm904
} MMGETCURRENTTASK16;
typedef MMGETCURRENTTASK16 UNALIGNED *PMMGETCURRENTTASK16;
#endif

#ifdef NULLSTRUCT
typedef struct _MMTASKYIELD16 {                mm905
} MMTASKYIELD16;
typedef MMTASKYIELD16 UNALIGNED *PMMTASKYIELD16;
#endif

typedef struct _DRVCLOSE16 {                   mm1100
    DWORD         f3;
    DWORD         f2;
    HDRVR16       f1;
} DRVCLOSE16;
typedef DRVCLOSE16 UNALIGNED *PDRVCLOSE16;

typedef struct _DRVOPEN16 {                    mm1101
    DWORD         f3;
    VPSTR         f2;
    VPSTR         f1;
} DRVOPEN16;
typedef DRVOPEN16 UNALIGNED *PDRVOPEN16;

typedef struct _DRVSENDMESSAGE16 {             mm1102
    DWORD         f4;
    DWORD         f3;
    WORD          f2;
    HDRVR16       f1;
} DRVSENDMESSAGE16;
typedef DRVSENDMESSAGE16 UNALIGNED *PDRVSENDMESSAGE16;

typedef struct _DRVGETMODULEHANDLE16 {         mm1103
    HDRVR16       f1;
} DRVGETMODULEHANDLE16;
typedef DRVGETMODULEHANDLE16 UNALIGNED *PDRVGETMODULEHANDLE;

#ifdef NULLSTRUCT
typedef struct _DRVDEFDRIVERPROC16{            mm1104
} DRVDEFDRIVERPROC16;
typedef DRVDEFDRIVERPROC16 UNALIGNED *PDRVDEFDRIVERPROC;
#endif

------------------------------------------------------------------------------*/

typedef struct _MMIOOPEN16 {                   /* mm1210 */
    DWORD         f3;
    VPMMIOINFO16  f2;
    VPSTR         f1;
} MMIOOPEN16;
typedef MMIOOPEN16 UNALIGNED *PMMIOOPEN16;

typedef struct _MMIOCLOSE16 {                  /* mm1211 */
    WORD       f2;
    HMMIO16    f1;
} MMIOCLOSE16;
typedef MMIOCLOSE16 UNALIGNED *PMMIOCLOSE16;

typedef struct _MMIOREAD16 {                   /* mm1212 */
    LONG       f3;
    HPSTR16    f2;
    HMMIO16    f1;
} MMIOREAD16;
typedef MMIOREAD16 UNALIGNED *PMMIOREAD16;

typedef struct _MMIOWRITE16 {                  /* mm1213 */
    LONG       f3;
    HPSTR16    f2;
    HMMIO16    f1;
} MMIOWRITE16;
typedef MMIOWRITE16 UNALIGNED *PMMIOWRITE16;

typedef struct _MMIOSEEK16 {                   /* mm1214 */
    INT16      f3;
    LONG       f2;
    HMMIO16    f1;
} MMIOSEEK16;
typedef MMIOSEEK16 UNALIGNED *PMMIOSEEK16;

typedef struct _MMIOGETINFO16 {                /* mm1215 */
    WORD          f3;
    VPMMIOINFO16  f2;
    HMMIO16       f1;
} MMIOGETINFO16;
typedef MMIOGETINFO16 UNALIGNED *PMMIOGETINFO16;

typedef struct _MMIOSETINFO16 {                /* mm1216 */
    WORD          f3;
    VPMMIOINFO16  f2;
    HMMIO16       f1;
} MMIOSETINFO16;
typedef MMIOSETINFO16 UNALIGNED *PMMIOSETINFO16;

typedef struct _MMIOSETBUFFER16 {              /* mm1217 */
    WORD       f4;
    LONG       f3;
    VPSTR      f2;
    HMMIO16    f1;
} MMIOSETBUFFER16;
typedef MMIOSETBUFFER16 UNALIGNED *PMMIOSETBUFFER16;

typedef struct _MMIOFLUSH16 {                  /* mm1218 */
    WORD       f2;
    HMMIO16    f1;
} MMIOFLUSH16;
typedef MMIOFLUSH16 UNALIGNED *PMMIOFLUSH16;

typedef struct _MMIOADVANCE16 {                /* mm1219 */
    WORD          f3;
    VPMMIOINFO16  f2;
    HMMIO16       f1;
} MMIOADVANCE16;
typedef MMIOADVANCE16 UNALIGNED *PMMIOADVANCE16;

typedef struct _MMIOSTRINGTOFOURCC16 {         /* mm1220 */
    WORD       f2;
    VPCSTR     f1;
} MMIOSTRINGTOFOURCC16;
typedef MMIOSTRINGTOFOURCC16 UNALIGNED *PMMIOSTRINGTOFOURCC16;

typedef struct _MMIOINSTALLIOPROC16 {          /* mm1221 */
    DWORD         f3;
    VPMMIOPROC16  f2;
    FOURCC        f1;
} MMIOINSTALLIOPROC16;
typedef MMIOINSTALLIOPROC16 UNALIGNED *PMMIOINSTALLIOPROC16;

typedef struct _MMIOSENDMESSAGE16 {            /* mm1222 */
    LPARAM     f4;
    LPARAM     f3;
    WORD       f2;
    HMMIO16    f1;
} MMIOSENDMESSAGE16;
typedef MMIOSENDMESSAGE16 UNALIGNED *PMMIOSENDMESSAGE16;

typedef struct _MMIODESCEND16 {                /* mm1223 */
    WORD          f4;
    VPMMCKINFO16  f3;
    VPMMCKINFO16  f2;
    HMMIO16       f1;
} MMIODESCEND16;
typedef MMIODESCEND16 UNALIGNED *PMMIODESCEND16;

typedef struct _MMIOASCEND16 {                 /* mm1224 */
    WORD          f3;
    VPMMCKINFO16  f2;
    HMMIO16       f1;
} MMIOASCEND16;
typedef MMIOASCEND16 UNALIGNED *PMMIOASCEND16;

typedef struct _MMIOCREATECHUNK16 {            /* mm1225 */
    WORD          f3;
    VPMMCKINFO16  f2;
    HMMIO16       f1;
} MMIOCREATECHUNK16;
typedef MMIOCREATECHUNK16 UNALIGNED *PMMIOCREATECHUNK16;

typedef struct _MMIORENAME16 {                 /* mm1226 */
    DWORD        f4;
    VPMMIOINFO16 f3;
    VPCSTR       f2;
    VPCSTR       f1;
} MMIORENAME16;
typedef MMIORENAME16 UNALIGNED *PMMIORENAME16;

/* XLATOFF */
#pragma pack()
/* XLATON */

