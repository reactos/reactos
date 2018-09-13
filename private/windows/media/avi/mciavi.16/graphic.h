/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   graphic.h - Multimedia Systems Media Control Interface
            driver for AVI.

*****************************************************************************/

#define NOSHELLDEBUG
#include <windows.h>
#include <windowsx.h>
#define MCI_USE_OFFEXT
#include <mmsystem.h>
#include <win32.h> 	// This must be included, for both versions
#include <mmddk.h>
#ifndef RC_INVOKED
#include "ntaviprt.h"
#include "common.h"
#include "aviffmt.h"
#include <drawdib.h>
#include <compman.h>
#include "avifilex.h"   // include AVIFile stuff.
#include "digitalv.h"
#endif
#include "mciavi.h"

// Define this to make the code expire on a given date....
// #define EXPIRE (1994 * 65536 + 1 * 256 + 1)        // expire 1/1/1994

#ifndef DRIVE_CDROM
    #define DRIVE_CDROM 5
#endif

#define DRIVE_INTERFACE     42

#ifdef EXPIRE
#define MCIERR_AVI_EXPIRED		9999
#endif

#define MCIAVI_PRODUCTNAME       2
#define MCIAVI_VERSION           3
#define MCIAVI_BADMSVIDEOVERSION 4

#define MCIAVI_MENU_CONFIG       5
#define MCIAVI_MENU_STRETCH      6
#define MCIAVI_MENU_MUTE         7

#define MCIAVI_CANT_DRAW_VIDEO   8
#define MCIAVI_CANT_DRAW_STREAM  9

#define INFO_VIDEOFORMAT         10
#define INFO_MONOFORMAT          11
#define INFO_STEREOFORMAT        12
#define INFO_LENGTH              13
#define INFO_FILE                14
#define INFO_KEYFRAMES           15
#define INFO_AUDIO               16
#define INFO_SKIP                17
#define INFO_ADPCM               18
#define INFO_DATARATE            19
#define INFO_SKIPAUDIO           20
#define INFO_FILETYPE            21
#define INFO_FILETYPE_AVI        22
#define INFO_FILETYPE_INT        23
#define INFO_FILETYPE_ALPHA      24
#define INFO_FRAMERATE           25
#define INFO_STREAM              26
#define INFO_DISABLED            27
#define INFO_ALLKEYFRAMES        28
#define INFO_NOKEYFRAMES         29
#define INFO_COMPRESSED          30
#define INFO_NOTREAD		 31

#define MCIAVI_MAXSIGNALS	1
#define MCIAVI_MAXWINDOWS	8

/* Flags for dwFlags in MCIGRAPHIC */
#define MCIAVI_STOP		0x00000001L	/* We need to stop	*/
#define MCIAVI_PAUSE		0x00000002L	/* We need to be paused	*/
#define MCIAVI_CUEING		0x00000004L	/* We are in a cue command */
#define MCIAVI_WAITING          0x00000008L     /* We are waiting for a command to finish */
#define MCIAVI_PLAYAUDIO	0x00000010L	/* Audio enabled	*/
#define MCIAVI_LOSTAUDIO        0x00000020L     /* cant get audio device*/
#define MCIAVI_SHOWVIDEO        0x00000040L     /* Video enabled        */
#define MCIAVI_USING_AVIFILE    0x00000080L     /* RTL to AVIFile */
#define MCIAVI_USINGDISPDIB	0x00000100L	/* Now in MCGA mode	*/
#define MCIAVI_NEEDTOSHOW	0x00000200L	/* window needs to be shown */
#define MCIAVI_WANTMOVE         0x00000400L     /* call CheckWindowMove alot */
#define MCIAVI_ANIMATEPALETTE   0x00000800L     /* Palette animated */
#define MCIAVI_NEEDUPDATE       0x00001000L     /* Need to redraw full  */
#define MCIAVI_PALCHANGED       0x00002000L     /* Need to update palette */
#define MCIAVI_STUPIDMODE       0x00004000L     /* dont buffer mode  */
#define MCIAVI_CANDRAW          0x00008000L     /* display driver can draw format */
#define MCIAVI_FULLSCREEN       0x00010000L     /* draw fullscreen. */
#define MCIAVI_NEEDDRAWBEGIN    0x00020000L     /* compressor is drawing */
#define MCIAVI_UPDATETOMEMORY   0x00040000L     /* drawing to a bitmap */
#define MCIAVI_WAVEPAUSED       0x00080000L     /* waveOut is temporarily paused */
#define MCIAVI_NOTINTERLEAVED   0x00100000L     /* file is not interleaved. */
#define MCIAVI_USERDRAWPROC     0x00200000L     /* user has set draw proc*/
#define MCIAVI_Y                0x00400000L     /* */
#define MCIAVI_VOLUMESET	0x00800000L	/* Volume has been changed. */
#define	MCIAVI_HASINDEX		0x01000000L	/* File has index.	*/
#define MCIAVI_RELEASEDC        0x02000000L     /* we got the DC via GetDC */
#define MCIAVI_SEEKING		0x04000000L	/* audio disabled for seek. */
#define MCIAVI_UPDATING		0x08000000L	/* handling WM_PAINT-don't yield. */
#define MCIAVI_REPEATING	0x10000000L	/* repeat when play finishes. */
#define MCIAVI_REVERSE		0x20000000L	/* playing backwards.... */
#define MCIAVI_NOBREAK		0x40000000L	/* don't allow break out of DISPDIB */
#define MCIAVI_ZOOMBY2          0x80000000L    /* fullscreen zoomed by 2 */

/* Flags for dwOptionFlags */
#define MCIAVIO_SEEKEXACT	0x00000001L	/* If off, seek goes to
						** previous key frame
						** instead of real
						** target frame.	*/
#define MCIAVIO_SKIPFRAMES	0x00000002L	/* Skip frames to keep
						** synchronized.	*/
#define MCIAVIO_STRETCHTOWINDOW	0x00000004L	/* Resize destination
						** rectangle if window
						** resized.		*/

#define MCIAVIO_STUPIDMODE	0x00000020L	/* Don't do nice updating. */

#define MCIAVIO_ZOOMBY2		0x00000100L
#define MCIAVIO_USEVGABYDEFAULT	0x00000200L
#define MCIAVIO_USEAVIFILE      0x00000400L

#define MCIAVI_ALG_INTERLEAVED	0x0001
#define MCIAVI_ALG_CDROM	0x0002
#define MCIAVI_ALG_HARDDISK	0x0003
#define MCIAVI_ALG_AUDIOONLY	0x0004
#define MCIAVI_ALG_AVIFILE      0x0005

//
//  the frame index is indexed by frame number, it is used for
//  varible sizeed, fixed rate streams.
//
typedef struct
{
    WORD                iPrevKey;           // prev "key" frame
    WORD                iNextKey;           // next "key" frame
    WORD                iPalette;           // palette frame (this points into index!)
    UINT                wSmag;              //
    DWORD               dwOffset;           // Position of chunk (file offset)
    DWORD               dwLength;           // Length of chunk (in bytes)
} AVIFRAMEINDEX;

#define NOBASED32

#if defined(WIN32) || defined(NOBASED32)
    #define BASED32(p)      _huge
    #define P32(t,p)        ((t _huge *)(p))
    #define B32(t,p)        ((t _huge *)(p))
#else
    #define BASED32(p)      _based32((_segment)SELECTOROF(p))
    #define P32(t,p)        ((t BASED32(p) *)OFFSETOF(p))
    #define B32(t,p)        ((t BASED32(p) *)0)
#endif

#define Frame(n)        (P32(AVIFRAMEINDEX,npMCI->hpFrameIndex) + (DWORD)(n))
#define FrameNextKey(n) (LONG)((n) + (DWORD)Frame(n)->iNextKey)
#define FramePrevKey(n) (LONG)((n) - (DWORD)Frame(n)->iPrevKey)
#define FramePalette(n) (LONG)(Frame(n)->iPalette)
#define FrameOffset(n)  (DWORD)(Frame(n)->dwOffset)
#define FrameLength(n)  (DWORD)(Frame(n)->dwLength)

#define UseIndex(p)     SillyGlobal = (p)
#define Index(n)        (B32(AVIINDEXENTRY,npMCI->hpIndex) + (long)(n))
#define IndexOffset(n)  Index(n)->dwChunkOffset
#define IndexLength(n)  Index(n)->dwChunkLength
#define IndexFlags(n)   Index(n)->dwFlags
#define IndexID(n)      Index(n)->ckid

typedef struct {
    DWORD               dwFlags;        /* flags, STREAM_ENABLED... */
    AVIStreamHeader     sh;             /* AVIStreamHeader...*/

    DWORD               cbFormat;       /* Stream format...*/
    LPVOID		lpFormat;

    DWORD               cbData;         /* Extra stream data...*/
    LPVOID		lpData;

    HIC                 hicDraw;        /* Draw codec...*/

    RECT                rcSource;       /* rectangles...*/
    RECT                rcDest;

    LONG                lStart;         /* start */
    LONG                lEnd;           /* end */

    LONG                lPlayStart;     /* play start */
    LONG                lPlayFrom;      /* play from */
    LONG                lPlayTo;        /* play to */

    LONG                lFrameDrawn;    /* we drew this */
    LONG                lPos;           /* current pos */
    LONG                lNext;          /* next pos */
    LONG                lLastKey;       /* key frame */
    LONG                lNextKey;       /* next key frame */

#ifdef USEAVIFILE
    PAVISTREAM          ps;
////IAVIStreamVtbl      vt;     // so we can call direct.
#endif
} STREAMINFO;

#define STREAM_ENABLED      0x0001  // stream is enabled for play
#define STREAM_ACTIVE       0x0002  // stream is active for *current* play
#define STREAM_NEEDUPDATE   0x0004  // stream needs update (paint)
#define STREAM_ERROR        0x0008  // stream did not load
#define STREAM_DIRTY        0x0010  // stream not showing current frame.

#define STREAM_SKIP         0x0100  // can skip data
#define STREAM_PALCHANGES   0x0200  // stream has palette changes
#define STREAM_VIDEO        0x0400  // is a video stream
#define STREAM_AUDIO        0x0800  // is a audio stream
#define STREAM_PALCHANGED   0x1000  // palette has changed
#define STREAM_WANTIDLE     0x2000  // should get idle time
#define STREAM_WANTMOVE     0x4000  // should get ICM_DRAW_WINDOW message

#define SI(stream)	(npMCI->paStreamInfo + stream)
#define SH(stream)	(SI(stream)->sh)

#define SOURCE(stream)  (SI(stream)->rcSource)
#define DEST(stream)    (SI(stream)->rcDest)
#define FRAME(stream)   (SH(stream).rcFrame)

#define FORMAT(stream) (SI(stream)->lpFormat)
#define VIDFMT(stream) ((LPBITMAPINFOHEADER) FORMAT(stream))
#define AUDFMT(stream) ((LPPCMWAVEFORMAT) FORMAT(stream))

//
// map from "movie" time into stream time.
//
#define TimeToMovie(t)         muldiv32(t, npMCI->dwRate, npMCI->dwScale*1000)
#define MovieToTime(l)         muldiv32(l, npMCI->dwScale*1000, npMCI->dwRate)
#define TimeToStream(psi, t)   muldiv32(t, psi->sh.dwRate,       psi->sh.dwScale*1000)
#define StreamToTime(psi, l)   muldiv32(l, psi->sh.dwScale*1000, psi->sh.dwRate)

//
//  NOTE all dwScale's are equal so we can do this without as many
//  multiplies
//
#if 0
#define MovieToStream(psi, l)  muldiv32(l, npMCI->dwScale * psi->sh.dwRate, npMCI->dwRate * psi->sh.dwScale)
#define StreamToMovie(psi, l)  muldiv32(l, npMCI->dwScale * psi->sh.dwRate, npMCI->dwRate * psi->sh.dwScale)
#else
#define MovieToStream(psi, l)  muldiv32(l, psi->sh.dwRate, npMCI->dwRate)
#define StreamToMovie(psi, l)  muldiv32(l, psi->sh.dwRate, npMCI->dwRate)
#endif

/*
 * RECT macros to get X,Y,Width,Height
 */
#define RCX(rc)     ((rc).left)
#define RCY(rc)     ((rc).top)
#define RCW(rc)     ((rc).right - (rc).left)
#define RCH(rc)     ((rc).bottom - (rc).top)

/*
 * The major control block for an AVI device
 * Define markers to more easily identify the control block when dumping
 */
#define MCIID      (DWORD)(((WORD)('V' | ('F'<<8))) | ((WORD)('W' | ('>'<<8))<<16))
#define MCIIDX     (DWORD)(((WORD)('v' | ('.'<<8))) | ((WORD)('w' | ('-'<<8))<<16))

typedef struct _MCIGRAPHIC {

    struct _MCIGRAPHIC *npMCINext;

#ifdef DEBUG
    DWORD        mciid;          // visible identifier
#endif

/*
 * multiple-thread synchronization.  We hold a critical section per device
 */
#ifdef WIN32
    CRITICAL_SECTION CritSec;       // per-device lock
    LONG lCritRefCount;          // entry count - see ntaviprt.h
#endif

/*
** Basic MCI information
*/
    HWND	hCallback;      /* callback window handle */
    UINT	wDevID;         /* device ID */

/*
** Internal task operation status and flags
*/
#ifndef WIN32
    UINT        pspTask;        /* background task's PSP */
    UINT        pspParent;      /* PSP of the calling app */
#endif
    HTASK	hTask;		/* task id */
    HTASK	hCallingTask;	/* task who opened us */
    UINT	uErrorMode;	/* SetErrorMode value for calling task */
    DWORD	dwTaskError;	/* error return from task */
    UINT	wMessageCurrent;/* Command in progress, or zero */
    UINT	wTaskState;	/* current task state */
    DWORD	dwFlags;	/* flags */
    DWORD	dwOptionFlags;	/* more flags */

/*
** Additional information controlled by MCI commands
*/
    HPALETTE    hpal;           /* Palette forced with MCI commands */
    HWND        hwnd;           /* window handle for playback */
    HWND	hwndDefault;	/* default window handle */
    HWND        hwndOldFocus;   /* window which had keyboard focus */
    BOOL        fForceBackground;/* Select palette in foreground or back? */
    DWORD       dwTimeFormat;   /* current time format */
    RECT        rcMovie;        /* main movie rect */
    RECT	rcSource;	/* drawing source rect */
    RECT	rcDest; 	/* drawing destination rect */
    LONG        PlaybackRate;   /* 1000 is normal, more is fast.... */
    DWORD	dwSpeedFactor;  /* 1000 is normal, more is fast.... */


/*
** Information about currently open file
*/
    UINT        uDriveType;     /* drive type */
    NPTSTR	szFilename;     /* AVI filename */
    LONG	lFrames;        /* number of frames in movie */
    DWORD	dwBytesPerSec;	/* file attributes */
    DWORD       dwRate;         /* master time base */
    DWORD       dwScale;
    DWORD	dwMicroSecPerFrame;	
    DWORD	dwSuggestedBufferSize;
    DWORD	dwKeyFrameInfo;	/* how often key frames occur */
    UINT	wEarlyAudio;	/* more file information */
    UINT	wEarlyVideo;
    UINT	wEarlyRecords;

    STREAMINFO NEAR *paStreamInfo;
    int         streams;        // total streams
    int         nAudioStreams;  // total audio streams
    int         nVideoStreams;  // total video streams
    int         nOtherStreams;  // total other streams
    int         nErrorStreams;  // total error streams

    int         nAudioStream;   // current audio stream.
    int         nVideoStream;   // "master" video stream

    STREAMINFO *psiAudio;       // points to video stream
    STREAMINFO *psiVideo;       // points to audio stream

#ifdef USEAVIFILE
    PAVIFILE        pf;
////IAVIFileVtbl    vt;    // so we can call direct.
#else
    LPVOID          pf;     // Stupid variable to be zero.
#endif

/*
** video stream junk
*/
    BOOL        fNoDrawing;
    LONG        lFrameDrawn;    /* number of last frame drawn */

    /* Drawing information */
    HDC		hdc;		/* DC we're playing into */

    /* Video format */
    BITMAPINFOHEADER	FAR *pbiFormat;       /* video format information */

    /* BitmapInfo used for drawing */
    BITMAPINFOHEADER    bih;         /* video format information */
    RGBQUAD             argb[256];   /* current drawing colors */
    RGBQUAD             argbOriginal[256]; /* original colors */

/*
** Installable compressor information
*/
    //!!! move all this into the screen draw function!!!
    //!!! all this should be in DrawDIB !!!
    HIC         hic;
    HIC         hicDraw;

    LONG        cbDecompress;
    HPSTR       hpDecompress;   /* pointer to full frame buffer */

/*
** Holding area for compressors we might use....
*/
    HIC         hicDecompress;
    HIC         hicDrawDefault;
    HIC         hicDrawFull;
    HIC		hicInternal;
    HIC         hicInternalFull;

    LONG        lLastPaletteChange;
    LONG        lNextPaletteChange;

/*
** wave stream junk
*/
    /* Wave format stuff */
    NPWAVEFORMAT pWF;           /* current wave format */
    UINT	wABs;		/* number of audio buffers */
    UINT	wABOptimal;	/* number full if synchronized */
    DWORD	dwABSize;	/* size of one audio buffer */

    HMMIO	hmmioAudio;

    DWORD       dwVolume;        /* Audio volume, 1000 is full on */
    BOOL	fEmulatingVolume;/* Are we doing volume by table lookup? */
    BYTE *      pVolumeTable;

    DWORD	dwAudioLength;
    DWORD	dwAudioPos;

    /* Wave Output Device */
    HWAVEOUT	hWave;		/* wave device handle */
    UINT	wABFull;	/* number now full */
    UINT	wNextAB;	/* next buffer in line */
    UINT        nAudioBehind;   /* how many audio below full */
    HPSTR	lpAudio;	/* pointer to audio buffers */
    DWORD       dwUsedThisAB;

/*
** File index information
*/
    AVIINDEXENTRY _huge *  hpIndex;        /* pointer to index */
    DWORD                   macIndex;       /* # records in index */

    AVIFRAMEINDEX _huge *  hpFrameIndex;   /* pointer to frame index */

/*
** play/seek params
*/
    LONG	lTo;            /* frame we're playing to */
    LONG	lFrom;		/* frame we're playing from */
    LONG        lCurrentFrame;  /* current frame */
    LONG        lRepeatFrom;    /* Frame to repeat from */

/*
** Information regarding current play
*/
    UINT        wPlaybackAlg;   /* playback algorithm in use */

    LONG	lRealStart;	/* frame playback starts */
    LONG	lAudioStart;	/* first audio frame to play */
    LONG	lVideoStart;	/* first video frame to play */

    LONG	lLastRead;

    /* Timing */
    DWORD	dwMSecPlayStart;/* Start time */
    LONG	lFramePlayStart;/* Frame playing started at */

    DWORD	dwTotalMSec;	/* Total time spent playing */

    DWORD	dwTimingStart;
    DWORD	dwPauseTime;
    DWORD	dwPlayMicroSecPerFrame;
    DWORD	dwAudioPlayed;

/*
** Timing information
*/
    DWORD	dwLastDrawTime; /* How long did the last draw take? */
    DWORD       dwLastReadTime;

    /* Timing information kept after play completes */
    DWORD	dwSkippedFrames;    /* Frames skipped during current play */
    DWORD	dwFramesSeekedPast; /* Frames not even read */
    DWORD	dwAudioBreaks;  /* # times audio broke up, approx. */
    DWORD       dwSpeedPercentage;  /* Ratio of ideal time to time taken */

    /* Timing information for last play */
    LONG        lFramesPlayed;
    LONG        lSkippedFrames;     /* Frames skipped during last play */
    LONG        lFramesSeekedPast;  /* Frames not even read */
    LONG        lAudioBreaks;       /* # times audio broke up, approx. */

/*
** Information for pending 'signal' command
*/
    DWORD	dwSignals;
    DWORD	dwSignalFlags;
    MCI_DGV_SIGNAL_PARMS signal;

/*
** Information for watching to see if window has moved.
*/
    UINT        wRgnType;       /* Region type, empty, simple, complex.... */
#ifdef WIN32
    POINT       dwOrg;          /* Physical DC origin */
#else
    DWORD       dwOrg;          /* Physical DC origin */
#endif
    RECT        rcClip;         /* clip box */
    HANDLE      hThreadTermination; /* Handle to wait on for thread to
                                       terminate so it's safe to unload DLL
                                       Must be closed by us                */

/*
** Information for hardware drawing devices....
*/
    DWORD       dwBufferedVideo;

/*
** specific to RIFF files
*/
    HMMIO       hmmio;          /* animation file handle */

    BOOL        fReadMany;      /* read more than one record */

    DWORD       dwFirstRecordPosition;
    DWORD       dwFirstRecordSize;
    DWORD       dwFirstRecordType;

    DWORD       dwNextRecordSize;       // used for ReadNextChunk
    DWORD       dwNextRecordType;

    DWORD       dwMovieListOffset;
    DWORD       dwBigListEnd;

    /* Read Buffer */
    HPSTR	lp;		/* work pointer */
    LPVOID      lpMMIOBuffer;   /* pointer to MMIO read buffer */
    HPSTR	lpBuffer;	/* pointer to read buffer */
    DWORD	dwBufferSize;	/* Read buffer size */
    DWORD	dwThisRecordSize; /* size of current record */

/*
** DEBUG stuff and more timing info.
*/

#ifdef DEBUG
    HANDLE      hdd;    //!!!

    LONG        timePlay;       /* total play time */
    LONG        timePrepare;    /* time to prepare for play */
    LONG        timeCleanup;    /* time to clean up play */
    LONG        timePaused;     /* paused time */
    LONG        timeRead;       /* time reading from disk */
    LONG        timeWait;       /* time waiting */
    LONG        timeYield;      /* time yielding to other apps */
    LONG        timeVideo;      /* time "drawing" video stream */
    LONG        timeAudio;      /* time "drawing" audio stream */
    LONG        timeOther;      /* time "drawing" other streams */
    LONG        timeDraw;       /* time drawing frame via DrawDib/DispDib/ICDraw */
    LONG        timeDecompress; /* time decompressing frame via ICDecompress */
#endif

#ifdef AVIREAD
    /*
     * handle to current async read object
     */
    HAVIRD	hAviRd;
    HPSTR	lpOldBuffer;
#endif

} MCIGRAPHIC, *NPMCIGRAPHIC, FAR *LPMCIGRAPHIC;

extern HANDLE ghModule;             // in DRVPROC.C
extern TCHAR  szClassName[];        // in WINDOW.C

/*
** Flags to protect ourselves in case we're closed with a dialog up...
*/
extern BOOL   gfEvil;               // in GRAPHIC.C
extern BOOL   gfEvilSysMenu;        // in GRAPHIC.C
extern HDRVR  ghdrvEvil;            // in GRAPHIC.C

/*
** Functions in GRAPHIC.C
*/
BOOL FAR PASCAL  GraphicInit (void);
BOOL NEAR PASCAL  GraphicWindowInit (void);

#ifdef WIN32
BOOL NEAR PASCAL GraphicWindowFree(void);
#endif

void  PASCAL  GraphicFree (void);
DWORD PASCAL  GraphicDrvOpen (LPMCI_OPEN_DRIVER_PARMS lpParms);
void  FAR PASCAL  GraphicDelayedNotify (NPMCIGRAPHIC npMCI, UINT wStatus);
void FAR PASCAL GraphicImmediateNotify (UINT wDevID,
    LPMCI_GENERIC_PARMS lpParms,
    DWORD dwFlags, DWORD dwErr);
DWORD PASCAL  GraphicClose(NPMCIGRAPHIC npMCI);
DWORD NEAR PASCAL ConvertFromFrames(NPMCIGRAPHIC npMCI, LONG lFrame);
LONG NEAR PASCAL ConvertToFrames(NPMCIGRAPHIC npMCI, DWORD dwTime);

DWORD PASCAL mciDriverEntry(UINT wDeviceID, UINT wMessage, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

long FAR PASCAL _loadds GraphicWndProc(HWND, UINT, WPARAM, LPARAM);

void  CheckWindowMove(NPMCIGRAPHIC npMCI, BOOL fForce);

/*
** Functions in DEVICE.C
*/
DWORD PASCAL DeviceOpen(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD PASCAL DeviceClose(NPMCIGRAPHIC npMCI);
DWORD PASCAL DevicePlay(NPMCIGRAPHIC npMCI, LONG lPlayTo, DWORD dwFlags);
DWORD PASCAL DeviceResume(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD PASCAL DeviceCue(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags);
DWORD PASCAL DeviceStop(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD PASCAL DevicePause(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD PASCAL DeviceSeek(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags);
DWORD PASCAL DeviceRealize(NPMCIGRAPHIC npMCI);
DWORD PASCAL DeviceUpdate(NPMCIGRAPHIC npMCI, DWORD dwFlags, HDC hDC, LPRECT lprc);
UINT  PASCAL DeviceMode(NPMCIGRAPHIC npMCI);
DWORD PASCAL DevicePosition(NPMCIGRAPHIC npMCI, LPLONG lpl);
DWORD PASCAL DeviceSetWindow(NPMCIGRAPHIC npMCI, HWND hwnd);
DWORD PASCAL DeviceSetSpeed(NPMCIGRAPHIC npMCI, DWORD dwNewSpeed);
DWORD PASCAL DeviceMute(NPMCIGRAPHIC npMCI, BOOL fMute);
DWORD PASCAL DeviceSetVolume(NPMCIGRAPHIC npMCI, DWORD dwVolume);
DWORD PASCAL DeviceGetVolume(NPMCIGRAPHIC npMCI);
DWORD PASCAL DeviceSetAudioStream(NPMCIGRAPHIC npMCI, UINT uStream);
DWORD PASCAL DeviceSetVideoStream(NPMCIGRAPHIC npMCI, UINT uStream, BOOL fOn);
DWORD PASCAL DeviceSetActive(NPMCIGRAPHIC npMCI, BOOL fActive);

DWORD FAR PASCAL DevicePut(NPMCIGRAPHIC npMCI, LPRECT lprc, DWORD dwFlags);
DWORD FAR PASCAL DeviceSetPalette(NPMCIGRAPHIC npMCI, HPALETTE hpal);
DWORD PASCAL DeviceLoad(NPMCIGRAPHIC npMCI);

typedef struct {
    UINT    wOldTaskState;
    LONG    lTo;
    LONG    lFrom;
    DWORD   dwFlags;
} TEMPORARYSTATE;

DWORD NEAR PASCAL StopTemporarily(NPMCIGRAPHIC npMCI, TEMPORARYSTATE FAR * pts);
DWORD NEAR PASCAL RestartAgain(NPMCIGRAPHIC npMCI, TEMPORARYSTATE FAR * pts);

#if 0
#ifdef DEBUG
    void FAR PASCAL VerifyTaskState(NPMCIGRAPHIC npMCI, UINT wState);

    #define DEBUGVERIFYSTATE(npMCI, wState)    VerifyTaskState(npMCI, wState)
#else
    #define DEBUGVERIFYSTATE(npMCI, wState)    0
#endif
#endif


void FAR PASCAL SetWindowToDefaultSize(NPMCIGRAPHIC npMCI);
void FAR PASCAL ResetDestRect(NPMCIGRAPHIC npMCI);

DWORD FAR PASCAL ReadConfigInfo(void);
void  FAR PASCAL WriteConfigInfo(DWORD dwOptions);
BOOL  FAR PASCAL ConfigDialog(HWND, NPMCIGRAPHIC);

/*
** The Enumerate command isn't real: I'm just thinking about it.
*/
#define MCI_ENUMERATE			0x0901
#define MCI_ENUMERATE_STREAM		0x00000001L

// constants for dwItem field of MCI_STATUS_PARMS parameter block
#define MCI_AVI_STATUS_STREAMCOUNT	0x10000001L
#define MCI_AVI_STATUS_STREAMTYPE	0x10000002L
#define MCI_AVI_STATUS_STREAMENABLED	0x10000003L

// flags for dwFlags field of MCI_STATUS_PARMS parameter block
#define MCI_AVI_STATUS_STREAM		0x10000000L

// flags for dwFlags field of MCI_SET_PARMS parameter block
#define MCI_AVI_SET_STREAM		0x10000000L
#define MCI_AVI_SET_USERPROC		0x20000000L

/*
** Internal flag that can be used with SEEK
*/
#define MCI_AVI_SEEK_SHOWWINDOW		0x10000000L

extern INT	gwSkipTolerance;
extern INT	gwHurryTolerance;

/**************************************************************************
**************************************************************************/

#ifdef DEBUG
    extern DWORD FAR PASCAL timeGetTime(void);

    #define TIMEZERO(time)   npMCI->time  = 0;
    #define TIMESTART(time)  npMCI->time -= (LONG)timeGetTime()
    #define TIMEEND(time)    npMCI->time += (LONG)timeGetTime()
#else
    #define TIMEZERO(time)
    #define TIMESTART(time)
    #define TIMEEND(time)
#endif

/**************************************************************************
**************************************************************************/

#define FOURCC_AVIDraw      mmioFOURCC('D','R','A','W')
#define FOURCC_AVIFull      mmioFOURCC('F','U','L','L')
LONG FAR PASCAL _loadds ICAVIDrawProc(DWORD id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);
LONG FAR PASCAL _loadds ICAVIFullProc(DWORD id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);

/**************************************************************************
**************************************************************************/

#ifndef RC_INVOKED
#include "avitask.h"
#endif
