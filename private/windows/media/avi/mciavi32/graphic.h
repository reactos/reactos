/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1995. All rights reserved.

   Title:   graphic.h - Multimedia Systems Media Control Interface
	    driver for AVI.

*****************************************************************************/

#define NOSHELLDEBUG
#include <windows.h>
#ifndef RC_INVOKED
#include <windowsx.h>
#else
#define MMNODRV
#define MMNOSOUND
#define MMNOWAVE
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOTIMER
#define MMNOJOY
#define MMNOMMIO
#define MMNOMMSYSTEM
#define MMNOMIDIDEV
#define MMNOWAVEDEV
#define MMNOAUXDEV
#define MMNOMIXERDEV
#define MMNOTIMERDEV
#define MMNOJOYDEV
#define MMNOTASKDEV
#endif
#define MCI_USE_OFFEXT
#include <mmsystem.h>
#include <win32.h>      // This must be included, for both versions
#include <mmddk.h>
#include "ntaviprt.h"
#include "common.h"
#include <vfw.h>
#include "digitalv.h"

/*
** Here are some compression types.
*/
#define comptypeRLE0            mmioFOURCC('R','L','E','0')
#define comptypeRLE             mmioFOURCC('R','L','E',' ')

#ifndef RC_INVOKED      // Don't overload RC!
#include "avifilex.h"   // include AVIFile stuff.
#endif // !RC_INVOKED

#include "mciavi.h"

#include "profile.h"

extern const TCHAR szIni[];
extern const TCHAR szReject[];

#ifdef _WIN32
//#define STATEEVENT
/*
 * This define causes the code to be compiled with a event defined.  This
 * event is signalled every time (almost) the task thread changes state.
 * Hence the routine waiting for a particular state need not poll.
 */

/*
 * On NT keep track of whether this process is WOW or not.  Set during
 * DRV_LOAD processing.
 */
extern BOOL runningInWow;
#define IsNTWOW()  runningInWow

#else  // WIN 16

#define IsNTWOW() 0

#endif

#if !defined NUMELMS
 #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

// Define this to make the code expire on a given date....
// #define EXPIRE (1994 * 65536 + 1 * 256 + 1)        // expire 1/1/1994

#ifndef DRIVE_CDROM
    #define DRIVE_CDROM 5
#endif

#define DRIVE_INTERFACE     42

#ifdef EXPIRE
#define MCIERR_AVI_EXPIRED              9999
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
#define INFO_NOTREAD             31
#define IDS_IRTL                 32
#define IDS_VIDEO                33
#define IDS_VIDEOCAPTION         34


#ifndef RC_INVOKED
#define MCIAVI_MAXSIGNALS       1
#define MCIAVI_MAXWINDOWS       8

/* Flags for dwFlags in MCIGRAPHIC */
#define MCIAVI_STOP             0x00000001L     /* We need to stop      */
#define MCIAVI_PAUSE            0x00000002L     /* We need to be paused */
#define MCIAVI_CUEING           0x00000004L     /* We are in a cue command */
#define MCIAVI_WAITING          0x00000008L     /* We are waiting for a command to finish */
#define MCIAVI_PLAYAUDIO        0x00000010L     /* Audio enabled        */
#define MCIAVI_LOSTAUDIO        0x00000020L     /* cant get audio device*/
#define MCIAVI_SHOWVIDEO        0x00000040L     /* Video enabled        */
#define MCIAVI_USING_AVIFILE    0x00000080L     /* RTL to AVIFile */
#define MCIAVI_USINGDISPDIB     0x00000100L     /* Now in MCGA mode     */
#define MCIAVI_NEEDTOSHOW       0x00000200L     /* window needs to be shown */
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
#define MCIAVI_LOSEAUDIO        0x00400000L     /* do not open wave device */
#define MCIAVI_VOLUMESET        0x00800000L     /* Volume has been changed. */
#define MCIAVI_HASINDEX         0x01000000L     /* File has index.      */
#define MCIAVI_RELEASEDC        0x02000000L     /* we got the DC via GetDC */
#define MCIAVI_SEEKING          0x04000000L     /* audio disabled for seek. */
#define MCIAVI_UPDATING         0x08000000L     /* handling WM_PAINT-don't yield. */
#define MCIAVI_REPEATING        0x10000000L     /* repeat when play finishes. */
#define MCIAVI_REVERSE          0x20000000L     /* playing backwards.... */
#define MCIAVI_NOBREAK          0x40000000L     /* don't allow break out of DISPDIB */
#define MCIAVI_ZOOMBY2          0x80000000L     /* fullscreen zoomed by 2 */

/* Flags for dwOptionFlags */
#define MCIAVIO_SEEKEXACT       0x00000001L     /* If off, seek goes to
						** previous key frame
						** instead of real
						** target frame.        */
#define MCIAVIO_SKIPFRAMES      0x00000002L     /* Skip frames to keep
						** synchronized.        */
#define MCIAVIO_STRETCHTOWINDOW 0x00000004L     /* Resize destination
						** rectangle if window
						** resized.             */

#define MCIAVIO_STUPIDMODE      0x00000020L     /* Don't do nice updating. */

#define MCIAVIO_ZOOMBY2         0x00000100L
#define MCIAVIO_USEVGABYDEFAULT 0x00000200L
#define MCIAVIO_USEAVIFILE      0x00000400L
#define MCIAVIO_NOSOUND         0x00000800L
#define MCIAVIO_USEDCI          0x00001000L

#define MCIAVIO_1QSCREENSIZE    0x00010000L
#define MCIAVIO_2QSCREENSIZE    0x00020000L
#define MCIAVIO_3QSCREENSIZE    0x00040000L
#define MCIAVIO_MAXWINDOWSIZE   0x00080000L
#define MCIAVIO_DEFWINDOWSIZE   0x00000000L
#define MCIAVIO_WINDOWSIZEMASK  0x000F0000L


#define MCIAVI_ALG_INTERLEAVED  0x0001
#define MCIAVI_ALG_CDROM        0x0002
#define MCIAVI_ALG_HARDDISK     0x0003
#define MCIAVI_ALG_AUDIOONLY    0x0004
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

#if defined(_WIN32) || defined(NOBASED32)
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
    LPVOID              lpFormat;

    DWORD               cbData;         /* Extra stream data...*/
    LPVOID              lpData;

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

#define SI(stream)      (npMCI->paStreamInfo + stream)
#define SH(stream)      (SI(stream)->sh)

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
 * dwNTFlags definitions
 */
//#define NTF_AUDIO_ON       0x00000001   Messages are not used to regain wave device
#define NTF_AUDIO_OFF        0x00000002
#define NTF_CLOSING          0x80000000
#define NTF_RETRYAUDIO       0x00000004
#define NTF_RESTARTFORAUDIO  0x00000008
#define NTF_DELETEWINCRITSEC 0x00000010
#define NTF_DELETECMDCRITSEC 0x00000020
#define NTF_DELETEHDCCRITSEC 0x00000040
#ifdef _WIN32
    #define ResetNTFlags(npMCI, bits) (npMCI)->dwNTFlags &= ~(bits)
    #define SetNTFlags(npMCI, bits) (npMCI)->dwNTFlags |= (bits)
    #define TestNTFlags(npMCI, bits) ((npMCI)->dwNTFlags & (bits))
#ifdef REMOTESTEAL
    extern HKEY hkey;
#endif
#else
    #define ResetNTFlags(npMCI, bits)
    #define SetNTFlags(npMCI, bits)
    #define TestNTFlags(npMCI, bits) 0
#endif

/*
 * RECT macros to get X,Y,Width,Height
 */
#define RCX(rc)     ((rc).left)
#define RCY(rc)     ((rc).top)
#define RCW(rc)     ((rc).right - (rc).left)
#define RCH(rc)     ((rc).bottom - (rc).top)


#ifdef _WIN32
// interaction between worker and winproc thread.
// winproc thread sets these bits in npMCI->winproc_request
#define WINPROC_STOP            0x0001  // stop play
#define WINPROC_RESETDEST       0x0002  // reset dest rect (window sized)
#define WINPROC_MUTE            0x0004  // mute flag changed
#define WINPROC_ACTIVE          0x0008  // got activation
#define WINPROC_INACTIVE        0x0010  // lost activation
#define WINPROC_UPDATE          0x0020  // window needs painting
#define WINPROC_REALIZE         0x0040  // palette needs realizing
#define WINPROC_SILENT          0x0100  // go silent (release wave device)
#define WINPROC_SOUND           0x0200  // restore sound (get wave device)
#endif


/*
 * The major control block for an AVI device
 * Define markers to more easily identify the control block when dumping
 */
#define MCIID      (DWORD)(((WORD)('V' | ('F'<<8))) | ((WORD)('W' | ('>'<<8))<<16))
#define MCIIDX     (DWORD)(((WORD)('v' | ('f'<<8))) | ((WORD)('w' | ('-'<<8))<<16))

typedef struct _MCIGRAPHIC {

// --- these fields accessed by user thread -------------------------
#ifdef DEBUG
    DWORD        mciid;         /* visible identifier */
#endif

    struct _MCIGRAPHIC *npMCINext;


/*
** Basic MCI information
*/
    HWND        hCallback;      /* callback window handle */
    UINT        wDevID;         /* device ID */


// -----new inter-task communication zone
    CRITICAL_SECTION    CmdCritSec;     // hold this to make request

    // next two events must be contiguous - WaitForMultipleObjects
    HANDLE      hEventSend;             // set to signal a request
    HANDLE      heWinProcRequest;       // set when something to process
#define IDLEWAITFOR 2

    // note - next two events are passed as an array to WaitForMultipleObjects
    HANDLE      hEventResponse;         // signalled by worker on req done.
    HANDLE      hThreadTermination; /* Handle to wait on for thread to
				       terminate so it's safe to unload DLL
				       Must be closed by us                */

    HANDLE      hEventAllDone;  // signalled on end of play

    int         EntryCount;     // used to prevent re-entry on current thread

    UINT        message;        // request message (from mciDriverEntry)
    DWORD       dwParamFlags;   // request param
    LPARAM      lParam;         // request param
    DWORD       dwReturn;       // return value
    DWORD_PTR   dwReqCallback;  // callback for this request
    BOOL        bDelayedComplete;       // is async request with wait?
    HTASK       hRequestor;     // task id of requesting task

    DWORD       dwTaskError;    /* error return from task */

// --- read by user thread to optimise status/position queries------

    UINT        wTaskState;     /* current task state */
    DWORD       dwFlags;        /* flags */

    LONG        lCurrentFrame;  /* current frame */
    DWORD       dwBufferedVideo;
    LONG        lRealStart;     /* frame playback starts */

    // user thread uses this for volume setting only
    HWAVEOUT    hWave;          /* wave device handle */
    DWORD       dwVolume;        /* Audio volume, 1000 is full on */

    LONG        lFrames;        /* number of frames in movie */

// --- nothing below here touched by user thread (after init)--------------

#if 0 /////UNUSED
    // the original interface before we marshalled it
    PAVIFILE    pf_AppThread;

    // marshalled into this block for passing to worker thread
    HANDLE      hMarshalling;

#endif/////UNUSED

    // set to TRUE during processing of an Update requested by the winproc
    // thread - don't do ShowStage during this as could cause deadlock
    BOOL        bDoingWinUpdate;



/*
** Internal task operation status and flags
*/
#ifndef _WIN32
    UINT        pspTask;        /* background task's PSP */
    UINT        pspParent;      /* PSP of the calling app */
#else
    DWORD       dwNTFlags;      /* NT specific flags */

    HTASK       hWaiter;                // task waiting on hEventAllDone

    // communication between worker and winproc threads

    // note: next two events must be contiguous - passed to WaitForMultiple..
    HANDLE      hThreadWinproc;         // signalled on thread exit
    HANDLE      hEventWinProcOK;        // signalled on init ok

    HANDLE      hEventWinProcDie;       // tell winproc thread to die

    CRITICAL_SECTION WinCritSec;        // protect worker - winproc interaction
    CRITICAL_SECTION HDCCritSec;        // protect worker - winproc drawing
    // ** VERY IMPORTANT ** IF both critical sections are needed then they
    // MUST be obtained in this order: WinCrit, then HDCCrit

#ifdef DEBUG
    DWORD       WinCritSecOwner;
    DWORD       WinCritSecDepth;
    DWORD       HDCCritSecOwner;
    DWORD       HDCCritSecDepth;
#endif

    // winproc sets bits in this (protected by WinCritSec) to
    // request stop/mute actions asynchronously
    DWORD       dwWinProcRequests;

    // saved state over temporary stop
    UINT        oldState;
    long        oldTo;
    long        oldFrom;
    DWORD       oldFlags;
    DWORD_PTR   oldCallback;
#endif

    HTASK       hTask;          /* task id */
    HTASK       hCallingTask;   /* task who opened us */
    UINT        uErrorMode;     /* SetErrorMode value for calling task */
    UINT        wMessageCurrent;/* Command in progress, or zero */
    DWORD       dwOptionFlags;  /* more flags */

/*
** Additional information controlled by MCI commands
*/
    HPALETTE    hpal;           /* Palette forced with MCI commands */
    HWND        hwndPlayback;   /* window handle for playback */
    HWND        hwndDefault;    /* default window handle */
    HWND        hwndOldFocus;   /* window which had keyboard focus */
    BOOL        fForceBackground;/* Select palette in foreground or back? */
    DWORD       dwTimeFormat;   /* current time format */
    RECT        rcMovie;        /* main movie rect */
    RECT        rcSource;       /* drawing source rect */
    RECT        rcDest;         /* drawing destination rect */
#ifdef DEBUG
    LONG        PlaybackRate;   /* 1000 is normal, more is fast.... */
#endif
    DWORD       dwSpeedFactor;  /* 1000 is normal, more is fast.... */

    // What is this flag?  We only listen to the zoom by 2 or fixed % window
    // size registry defaults if we're using the default window, not if
    // somebody is playing in their own window.  But when we open an AVI,
    // (like we're doing now) we don't know yet what window they'll pick!
    // So let's make a note that so far there's no reason not to listen to
    // the defaults, and if anybody resizes, or changes the window handle,
    // or makes the default window not resizable, then we won't.
    BOOL                fOKToUseDefaultSizing;

/*
 * window creation parameters to open
 */
    DWORD       dwStyle;
    HWND        hwndParent;


/*
** Information about currently open file
*/
    UINT        uDriveType;     /* drive type */
    NPTSTR      szFilename;     /* AVI filename */
    DWORD       dwBytesPerSec;  /* file attributes */
    DWORD       dwRate;         /* master time base */
    DWORD       dwScale;
    DWORD       dwMicroSecPerFrame;
    DWORD       dwSuggestedBufferSize;
    DWORD       dwKeyFrameInfo; /* how often key frames occur */
    UINT        wEarlyAudio;    /* more file information */
    UINT        wEarlyVideo;
    UINT        wEarlyRecords;

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
    HDC         hdc;            /* DC we're playing into */

    /* Video format */
    BITMAPINFOHEADER    FAR *pbiFormat;       /* video format information */

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
    HIC         hicInternal;
    HIC         hicInternalFull;

    LONG        lLastPaletteChange;
    LONG        lNextPaletteChange;

/*
** wave stream junk
*/
    /* Wave format stuff */
    NPWAVEFORMAT pWF;           /* current wave format */
    UINT        wABs;           /* number of audio buffers */
    UINT        wABOptimal;     /* number full if synchronized */
    DWORD       dwABSize;       /* size of one audio buffer */

    HMMIO       hmmioAudio;

    BOOL        fEmulatingVolume;/* Are we doing volume by table lookup? */
    BYTE *      pVolumeTable;

    DWORD       dwAudioLength;
    DWORD       dwAudioPos;

    /* Wave Output Device */
    UINT        wABFull;        /* number now full */
    UINT        wNextAB;        /* next buffer in line */
    UINT        nAudioBehind;   /* how many audio below full */
    HPSTR       lpAudio;        /* pointer to audio buffers */
    DWORD       dwUsedThisAB;

/*
** File index information
*/
    AVIINDEXENTRY _huge *  hpIndex;        /* pointer to index */
    DWORD                   macIndex;       /* # records in index */

    AVIFRAMEINDEX _huge *  hpFrameIndex;   /* pointer to frame index */
    HANDLE      hgFrameIndex;           // handle to non-offset memory

/*
** play/seek params
*/
    LONG        lTo;            /* frame we're playing to */
    LONG        lFrom;          /* frame we're playing from */
    LONG        lRepeatFrom;    /* Frame to repeat from */

/*
** Information regarding current play
*/
    UINT        wPlaybackAlg;   /* playback algorithm in use */

    LONG        lAudioStart;    /* first audio frame to play */
    LONG        lVideoStart;    /* first video frame to play */

    LONG        lLastRead;

    /* Timing */
    LONG        lFramePlayStart;/* Frame playing started at */

    DWORD       dwTotalMSec;    /* Total time spent playing */

    DWORD       dwMSecPlayStart;/* Start time */
    DWORD       dwTimingStart;
    DWORD       dwPauseTime;
    DWORD       dwPlayMicroSecPerFrame;
    DWORD       dwAudioPlayed;

/*
** Timing information
*/
#ifdef DEBUG
#define INTERVAL_TIMES
#endif
#ifdef INTERVAL_TIMES
#define NBUCKETS    25
#define BUCKETSIZE  10
//#define NTIMES            200
// frame interval timing
    DWORD       dwStartTime;
    long        msFrameMax;
    long        msFrameMin;
    long        msFrameTotal;
    long        msSquares;
    long        nFrames;
    int         buckets[NBUCKETS+1];
    long *      paIntervals;
    long        cIntervals;
    //long      intervals[NTIMES];
    long        msReadTimeuS;
    long        msReadMaxBytesPer;
    long        msReadMax;
    long        msReadTotal;
    long        nReads;
#endif

    DWORD       dwLastDrawTime; /* How long did the last draw take? */
    DWORD       dwLastReadTime;
    DWORD       msPeriodResolution;   /* Clock resolution for this video */

    /* Timing information kept after play completes */
    DWORD       dwSkippedFrames;    /* Frames skipped during current play */
    DWORD       dwFramesSeekedPast; /* Frames not even read */
    DWORD       dwAudioBreaks;  /* # times audio broke up, approx. */
    DWORD       dwSpeedPercentage;  /* Ratio of ideal time to time taken */

    /* Timing information for last play */
    LONG        lFramesPlayed;
    LONG        lSkippedFrames;     /* Frames skipped during last play */
    LONG        lFramesSeekedPast;  /* Frames not even read */
    LONG        lAudioBreaks;       /* # times audio broke up, approx. */

/*
** Information for pending 'signal' command
*/
    DWORD       dwSignals;
    DWORD       dwSignalFlags;
    MCI_DGV_SIGNAL_PARMS signal;

/*
** Information for watching to see if window has moved.
*/
    UINT        wRgnType;       /* Region type, empty, simple, complex.... */
#ifdef _WIN32
    POINT       dwOrg;          /* Physical DC origin */
#else
    DWORD       dwOrg;          /* Physical DC origin */
#endif
    RECT        rcClip;         /* clip box */

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
    HPSTR       lp;             /* work pointer */
    LPVOID      lpMMIOBuffer;   /* pointer to MMIO read buffer */
    HPSTR       lpBuffer;       /* pointer to read buffer */
    DWORD       dwBufferSize;   /* Read buffer size */
    DWORD       dwThisRecordSize; /* size of current record */

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
    HAVIRD      hAviRd;
    HPSTR       lpOldBuffer;
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
LPCTSTR FAR FileName(LPCTSTR szPath);

BOOL FAR PASCAL  GraphicInit (void);
BOOL NEAR PASCAL  GraphicWindowInit (void);

#ifdef _WIN32
BOOL NEAR PASCAL GraphicWindowFree(void);
void aviWinProcTask(DWORD_PTR dwInst);
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

LRESULT FAR PASCAL _loadds GraphicWndProc(HWND, UINT, WPARAM, LPARAM);

void  CheckWindowMove(NPMCIGRAPHIC npMCI, BOOL fForce);
DWORD InternalGetPosition(NPMCIGRAPHIC npMCI, LPLONG lpl);

// now called only on worker thread
void NEAR PASCAL GraphicSaveCallback (NPMCIGRAPHIC npMCI, HANDLE hCallback);


/*
 * Functions in DEVICE.C
 *
 *  All these DeviceXXX functions are called on USER THREAD ONLY (ok?)
 *  EXCEPT for DeviceSetActive which is called on the winproc thread.
 *  (From InternalRealize...CheckIfActive...DeviceSetActive)
 */
DWORD PASCAL DeviceOpen(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD PASCAL DeviceClose(NPMCIGRAPHIC npMCI);
DWORD PASCAL DevicePlay(
    NPMCIGRAPHIC npMCI,
    DWORD dwFlags,
    LPMCI_DGV_PLAY_PARMS lpPlay,
    LPARAM dwCallback
);
DWORD PASCAL DeviceResume(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPARAM dwCallback);
DWORD PASCAL DeviceCue(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags, LPARAM dwCallback);
DWORD PASCAL DeviceStop(NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD PASCAL DevicePause(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPARAM dwCallback);
DWORD PASCAL DeviceSeek(NPMCIGRAPHIC npMCI, LONG lTo, DWORD dwFlags, LPARAM dwCallback);
DWORD PASCAL DeviceRealize(NPMCIGRAPHIC npMCI);
DWORD PASCAL DeviceUpdate(NPMCIGRAPHIC npMCI, DWORD dwFlags, LPMCI_DGV_UPDATE_PARMS lpParms);
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
DWORD FAR PASCAL DeviceSetPaletteColor(NPMCIGRAPHIC npMCI, DWORD index, DWORD color);


void CheckIfActive(NPMCIGRAPHIC npMCI);


// in window.c
void FAR PASCAL AlterRectUsingDefaults(NPMCIGRAPHIC npMCI, LPRECT lprc);
void FAR PASCAL SetWindowToDefaultSize(NPMCIGRAPHIC npMCI, BOOL fUseDefaultSizing);

// user thread version
void FAR PASCAL ResetDestRect(NPMCIGRAPHIC npMCI, BOOL fUseDefaultSizing);

// same as ResetDestRect, but called on winproc thread
void FAR PASCAL Winproc_DestRect(NPMCIGRAPHIC npMCI, BOOL fUseDefaultSizing);

DWORD FAR PASCAL ReadConfigInfo(void);
void  FAR PASCAL WriteConfigInfo(DWORD dwOptions);
BOOL  FAR PASCAL ConfigDialog(HWND, NPMCIGRAPHIC);

/*
** The Enumerate command isn't real: I'm just thinking about it.
*/
#define MCI_ENUMERATE                   0x0901
#define MCI_ENUMERATE_STREAM            0x00000001L

// constants for dwItem field of MCI_STATUS_PARMS parameter block
#define MCI_AVI_STATUS_STREAMCOUNT      0x10000001L
#define MCI_AVI_STATUS_STREAMTYPE       0x10000002L
#define MCI_AVI_STATUS_STREAMENABLED    0x10000003L

// flags for dwFlags field of MCI_STATUS_PARMS parameter block
#define MCI_AVI_STATUS_STREAM           0x10000000L

// flags for dwFlags field of MCI_SET_PARMS parameter block
#define MCI_AVI_SET_STREAM              0x10000000L
#define MCI_AVI_SET_USERPROC            0x20000000L

/*
** Internal flag that can be used with SEEK
*/
#define MCI_AVI_SEEK_SHOWWINDOW         0x10000000L

/*
** in AVIPLAY.C (and GRAPHIC.C)
*/
extern INT      gwSkipTolerance;
extern INT      gwHurryTolerance;
extern INT      gwMaxSkipEver;

extern BOOL     gfUseGetPosition;
extern LONG     giGetPositionAdjust;
#ifdef _WIN32
    #define DEFAULTUSEGETPOSITION TRUE
#else
    #define DEFAULTUSEGETPOSITION FALSE
#endif

/**************************************************************************
**************************************************************************/

#ifdef DEBUG
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
LRESULT FAR PASCAL _loadds ICAVIDrawProc(DWORD_PTR id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);
LRESULT FAR PASCAL _loadds ICAVIFullProc(DWORD_PTR id, HDRVR hDriver, UINT uiMessage, LPARAM lParam1, LPARAM lParam2);

/**************************************************************************
**************************************************************************/

#include "avitask.h"

/**************************************************************************
 Macros and constants for accessing the list of open MCI devices.
 In the debug build we track who has access to the list.
**************************************************************************/

extern NPMCIGRAPHIC npMCIList; // in graphic.c

#ifdef _WIN32
extern CRITICAL_SECTION MCIListCritSec;  // in graphic.c

#ifdef DEBUG

// The debug versions of the macros track who owns the critical section
extern DWORD ListOwner;
#define EnterList()   { EnterCriticalSection(&MCIListCritSec);  \
			ListOwner=GetCurrentThreadId();\
		      }

#define LeaveList()   { ListOwner=0;\
			LeaveCriticalSection(&MCIListCritSec);\
		      }
#else  // !debug
#define EnterList()   EnterCriticalSection(&MCIListCritSec);
#define LeaveList()   LeaveCriticalSection(&MCIListCritSec);
#endif //DEBUG



// this critical section is used to protect drawing code from
// interaction between the winproc and the worker thread. the user
// thread should not need to hold this ever.

#ifdef DEBUG
// The debug versions of the EnterWinCrit/LeaveWinCrit macros track who
// owns the window critical section.  This makes it possible to Assert
// in the code that we are validly in (or out) of the critical section.
#define EnterWinCrit(p) {   EnterCriticalSection(&(p)->WinCritSec);     \
			    (p)->WinCritSecOwner=GetCurrentThreadId();  \
			    /* The first enter should mean that we do */\
			    /* NOT own the HDC critical section       */\
			    if (!((p)->WinCritSecDepth++))              \
				{ HDCCritCheckOut(p) };                 \
			}

#define LeaveWinCrit(p) {   if(0 == (--(p)->WinCritSecDepth))           \
				(p)->WinCritSecOwner=0;                 \
			    if ((p)->WinCritSecDepth<0) {               \
				DebugBreak();                           \
			    }                                           \
			    LeaveCriticalSection(&(p)->WinCritSec);     \
			}

#define WinCritCheckIn(p) if ((p)->WinCritSecOwner != GetCurrentThreadId())\
			   Assert(!"Should own the window critical section");
#define WinCritCheckOut(p) if ((p)->WinCritSecOwner == GetCurrentThreadId()) \
			   Assert(!"Should not own the window critical section");


#define EnterHDCCrit(p) {   EnterCriticalSection(&(p)->HDCCritSec);     \
			    (p)->HDCCritSecOwner=GetCurrentThreadId();  \
			    (p)->HDCCritSecDepth++;                     \
			}

#define LeaveHDCCrit(p) {   if(0 == (--(p)->HDCCritSecDepth))           \
				(p)->HDCCritSecOwner=0;                 \
			    if ((p)->HDCCritSecDepth<0) {               \
				DebugBreak();                           \
			    }                                           \
			    LeaveCriticalSection(&(p)->HDCCritSec);     \
			}

#define HDCCritCheckIn(p) if ((p)->HDCCritSecOwner != GetCurrentThreadId())\
			   Assert(!"Should own the hdc critical section");
#define HDCCritCheckOut(p) if ((p)->HDCCritSecOwner == GetCurrentThreadId()) \
			   Assert(!"Should not own the hdc critical section");


#else  // Non debug versions

#define EnterWinCrit(npMCI)     EnterCriticalSection(&npMCI->WinCritSec)
#define LeaveWinCrit(npMCI)     LeaveCriticalSection(&npMCI->WinCritSec)
#define WinCritCheckIn(p)
#define WinCritCheckOut(p)
#define EnterHDCCrit(npMCI)     EnterCriticalSection(&npMCI->HDCCritSec)
#define LeaveHDCCrit(npMCI)     LeaveCriticalSection(&npMCI->HDCCritSec)
#define HDCCritCheckIn(p)
#define HDCCritCheckOut(p)

#endif


#else   // !_WIN32
#define EnterList()
#define LeaveList()

#define EnterWinCrit(n)
#define LeaveWinCrit(n)
#endif
#endif           // RC_INVOKED
