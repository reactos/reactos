/****************************************************************************
 *
 *   avicapi.h
 *
 *   Internal, private definitions.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#ifndef _INC_AVICAP_INTERNAL
#define _INC_AVICAP_INTERNAL

#include <avifile.h>

#include <mmreg.h>
#include <compman.h>
#include "iaverage.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

/* c8 uses underscores on all defines */
#if defined DEBUG && !defined _DEBUG
 #define _DEBUG
#elif defined _DEBUG && !defined DEBUG
 #define DEBUG
#endif

#if !defined NUMELMS
  #define NUMELMS(aa)           (sizeof(aa)/sizeof((aa)[0]))
  #define FIELDOFF(type,field)  (&(((type)0)->field))
  #define FIELDSIZ(type,field)  (sizeof(((type)0)->field))
#endif

//
// use the registry - not WIN.INI
//
#if defined(_WIN32) && defined(UNICODE)
#include "profile.h"
#endif

// switch off all references to the new vfw1.1 compman interfaces until we
// have the new compman ported to NT
#define NEW_COMPMAN

#ifndef _LPHISTOGRAM_DEFINED
#define _LPHISTOGRAM_DEFINED
typedef DWORD HUGE * LPHISTOGRAM;
#endif

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) ((int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount))

// Swap palette entries around
#ifdef DEBUG
#define SWAPTYPE(x,y, type)  { type _temp_; _temp_=(x); (x)=(y), (y)=_temp_;}
#else
#define SWAPTYPE(x,y, type) ( (x)^=(y), (y)^=(x), (x)^=(y) )
#endif

#define ROUNDUPTOSECTORSIZE(dw, align) ((((DWORD)dw) + (align)-1) & ~((align)-1))

#define MAX_VIDEO_BUFFERS       400    // By using this number the CAPSTREAM structure fits into a page
#define MIN_VIDEO_BUFFERS       5
#define DEF_WAVE_BUFFERS        4
#define MAX_WAVE_BUFFERS        10

// MCI Capture state machine
enum mcicapstates {
   CAPMCI_STATE_Uninitialized = 0,
   CAPMCI_STATE_Initialized,

   CAPMCI_STATE_StartVideo,
   CAPMCI_STATE_CapturingVideo,
   CAPMCI_STATE_VideoFini,

   CAPMCI_STATE_StartAudio,
   CAPMCI_STATE_CapturingAudio,
   CAPMCI_STATE_AudioFini,

   CAPMCI_STATE_AllFini
};

// -------------------------
//  CAPSTREAM structure
// -------------------------
#define CAPSTREAM_VERSION 2             // Increment whenever struct changes
#define DEFAULT_BYTESPERSECTOR  512

// This structure is GlobalAlloc'd for each capture window instance.
// A pointer to the structure is stored in the Window extra bytes.
// Applications can retrieve a pointer to the stucture using
//    the WM_CAP_GET_CAPSTREAMPTR message.
// I: internal variables which the client app should not modify
// M: variables which the client app can set via Send/PostMessage

typedef struct tagCAPSTREAM {
    DWORD           dwSize;                     // I: size of structure
    UINT            uiVersion;                  // I: version of structure
    HINSTANCE       hInst;                      // I: our instance

    HANDLE          hThreadCapture;             // I: capture task handle
    DWORD           dwReturn;                   // I: capture task return value

    HWND            hwnd;                       // I: our hwnd

    // Use MakeProcInstance to create all callbacks !!!
    // Status, error callbacks
    CAPSTATUSCALLBACK   CallbackOnStatus;       // M: Status callback
    CAPERRORCALLBACK    CallbackOnError;        // M: Error callback

#ifdef UNICODE
    DWORD  fUnicode;				// I:
    // definitions for fUnicode
    #define  VUNICODE_ERRORISANSI	0x00000001      // set if error msg thunking required
    #define  VUNICODE_STATUSISANSI	0x00000002      // set if status msg thunking required
#endif

    // event used in capture loop to avoid polling
    HANDLE hCaptureEvent;		  	// I:
   #ifdef CHICAGO
    DWORD  hRing0CapEvt;			// I:
   #endif

    // Allows client to process messages during capture if set
    CAPYIELDCALLBACK    CallbackOnYield;        // M: Yield processing

    // Video and wave callbacks for Network or other specialized xfers
    CAPVIDEOCALLBACK    CallbackOnVideoFrame;   // M: Only during preview
    CAPVIDEOCALLBACK    CallbackOnVideoStream;  // M: Video buffer ready
    CAPWAVECALLBACK     CallbackOnWaveStream;   // M: Wave buffer ready
    CAPCONTROLCALLBACK  CallbackOnControl;      // M: External Start/Stop ctrl

    // Open channels on the video hardware device
    // and hardware capabilies
    CAPDRIVERCAPS   sCapDrvCaps;                // M: What can the driver do
    HVIDEO          hVideoIn;                   // I: In channel
    HVIDEO          hVideoCapture;              // I: Ext In channel
    HVIDEO          hVideoDisplay;              // I: Ext Out channel
    BOOL            fHardwareConnected;         // I: ANY open channel?

    // Flags indicating whether dialog boxes are currently displayed
#define VDLG_VIDEOSOURCE	0x00000001	// Video Source dialog
#define VDLG_VIDEOFORMAT	0x00000002	// Video Format dialog
#define VDLG_VIDEODISPLAY	0x00000004	// Video Display dialog
#define VDLG_COMPRESSION	0x00000008	// Video Compression dialog
    DWORD           dwDlgsActive;		// I: state of dialogs

    // Window to display video
    BOOL            fLiveWindow;                // M: Preview video
    BOOL            fOverlayWindow;             // M: Overlay video
    BOOL            fScale;                     // M: Scale image to client
    POINT           ptScroll;                   // I: Scroll position
    HANDLE          hdd;                        // I: hDrawDib access handle
    HCURSOR         hWaitCursor;                // I: hourglass
    UINT            uiRegion;                   // I: CheckWindowMove
    RECT            rcRegionRect;               // I: CheckWindowMove
    POINT	    ptRegionOrigin;

    // Window update timer
    UINT            idTimer;                    // I: ID of preview timer
    UINT            uTimeout;                   // M: Preview rate in mS. (not used after setting the timer)

    // Capture destination and control
    CAPTUREPARMS    sCapParms;                  // M: how to capture

    BOOL 	    fCaptureFlags;		//    state of capture

#define CAP_fCapturingToDisk		0x0001
#define CAP_fCapturingNow		0x0002
#define CAP_fStepCapturingNow           0x0004
#define CAP_fFrameCapturingNow          0x0008
#define CAP_fStopCapture                0x0010
#define CAP_fAbortCapture               0x0020

#if 0
    BOOL            fCapturingToDisk;           // M: if capturing to disk
    BOOL            fCapturingNow;              // I: if performing capture
    BOOL            fStepCapturingNow;          // I: if performing MCI step capture
    BOOL            fFrameCapturingNow;         // I: if performing single frame capture
    BOOL            fStopCapture;               // M: if stop requested
    BOOL            fAbortCapture;              // M: if abort requested

#endif
    DWORD           dwTimeElapsedMS;            // I: Capture time in millisec

    // Index
    HGLOBAL         hIndex;                     // I: handle to index mem
    DWORD           dwIndex;                    // I: index index
    DWORD           dwVideoChunkCount;          // I: # of video frames cap'd
    DWORD           dwWaveChunkCount;           // I: # of audio buffers cap'd
    LPDWORD         lpdwIndexStart;             // I: index start ptr
    LPDWORD         lpdwIndexEntry;             // I: index current ptr

    // Video format
    DWORD           dwActualMicroSecPerFrame;   // I: Actual cap rate
    LPBITMAPINFO    lpBitsInfo;                 // I: Video format
    int             dxBits;                     // I: video size x
    int             dyBits;                     // I: video size y
    LPBYTE          lpBits;                     // I: Single frame capture buf
    LPBYTE          lpBitsUnaligned;            // I: Single frame capture buf
    VIDEOHDR        VidHdr;                     // I: Single frame header

#ifdef 	NEW_COMPMAN
    COMPVARS        CompVars;                   // M: Set by ICCompressorChoose
#endif

    LPIAVERAGE      lpia;                       // I: Image averaging struct
    VIDEOHDR        VidHdr2x;                   // I: VideoHeader at 2x
    LPBITMAPINFOHEADER  lpbmih2x;               // I: lpbi at 2x

    // Video Buffer management
    DWORD           cbVideoAllocation;          // I: size of non-comp buffer incl chunk (not used)
    int             iNumVideo;                  // I: Number of actual video buffers
    int             iNextVideo;                 // I: index into video buffers
    DWORD           dwFramesDropped;            // I: number of frames dropped
    LPVIDEOHDR      alpVideoHdr[MAX_VIDEO_BUFFERS]; // I: array of buf ptrs
    BOOL            fBuffersOnHardware;         // I: if driver all'd buffers
    LPSTR           lpDropFrame;

    // Palettes
    HPALETTE        hPalCurrent;                // I: handle of current pal
    BOOL            fUsingDefaultPalette;       // I: no user defined pal
    int             nPaletteColors;             // M: only changed by UI
    LPVOID          lpCapPal;                   // I: LPCAPPAL manual pals
    LPVOID          lpCacheXlateTable;          // I: 32KB xlate table cached

    // Audio Capture Format
    BOOL            fAudioHardware;             // I: if audio hardware present
    LPWAVEFORMATEX  lpWaveFormat;               // I: wave format
    WAVEHDR         WaveHdr;                    // I: Wave header
    HWAVEIN         hWaveIn;                    // I: Wave input channel
    DWORD           dwWaveBytes;                // I: Total wave bytes cap'd
    DWORD           dwWaveSize;                 // I: wave buffer size

    // Audio Buffer management
    LPWAVEHDR       alpWaveHdr[MAX_WAVE_BUFFERS]; // I: wave buff array
    int             iNextWave;                  // I: Index into wave buffers
    int             iNumAudio;                  // I: Number of actual audio buffers
    BOOL            fAudioYield;                // I: ACM audio yield required
    BOOL            fAudioBreak;                // I: Audio underflow

    // MCI Capture
    TCHAR           achMCIDevice[MAX_PATH];     // MCI device name
    DWORD           dwMCIError;                 // I: Last MCI error value
    enum mcicapstates MCICaptureState;          // I: MCI State machine
    DWORD           dwMCICurrentMS;             // I: Current MCI position
    DWORD           dwMCIActualStartMS;         // I: Actual MCI start MS
    DWORD           dwMCIActualEndMS;           // I: Actual MCI end position

    // Output file
    TCHAR           achFile [MAX_PATH];         // M: name of capture file
    TCHAR           achSaveAsFile [MAX_PATH];   // M: name of saveas file
    LONG            lCapFileSize;               // M: in bytes
    BOOL            fCapFileExists;             // I: if have a capture file
    BOOL            fFileCaptured;              // I: if we've cap'd to file

    // async file io
    //
    DWORD           dwAsyncWriteOffset;         // I: last file write offset
    UINT            iNextAsync;                 // I: next async io header to be done
    UINT            iLastAsync;                 // I: last async io header to be written
    UINT            iNumAsync;                  // I: number of async io headers
    struct _avi_async {
        OVERLAPPED ovl;                         // I: for WriteFile call
        UINT       uType;                       // I: write type (Video/Wave/Drop)
        UINT       uIndex;                      // I: index into alpWaveHdr or alpVideoHdr
        } *        pAsync;                      // I: ptr to array of async io headers
#ifdef USE_AVIFILE
    // these 4 fields when using avifile
    //
    BOOL            bUseAvifile;
    PAVISTREAM      pvideo;
    PAVISTREAM      paudio;
    PAVIFILE        pavifile;
#endif

    HMMIO           hmmio;                      // I: MMIO handle for writing
    HANDLE          hFile;                      // I: write via CreateFile
    DWORD           dwBytesPerSector;           // I: bytes per sector
    BOOL            fUsingNonBufferedIO;        // I: FILE_FLAG_NO_BUFFERING
    DWORD           dwAVIHdrSize;               // I: size of header
    DWORD           dwAVIHdrPos;                // I: file offset of hdr

    LONG	    lUser;			// M: Data for the user
    LPVOID          lpInfoChunks;               // M: information chunks
    LONG            cbInfoChunks;               // M: sizeof information chks
    BOOL            fLastStatusWasNULL;         // I: don't repeat null msgs
    BOOL            fLastErrorWasNULL;          // I: don't repeat null msgs
} CAPSTREAM;
typedef CAPSTREAM FAR * LPCAPSTREAM;

// values for capstream.pAsync[nn].wType field
//
#define ASYNC_BUF_VIDEO 1
#define ASYNC_BUF_AUDIO 2
#define ASYNC_BUF_DROP  3

// -------------------------
//  Full color log palette
// -------------------------

typedef struct tagFCLOGPALETTE {
    WORD         palVersion;
    WORD         palNumEntries;
    PALETTEENTRY palPalEntry[256];
} FCLOGPALETTE;

typedef struct {
    DWORD       dwType;
    DWORD       dwSize;
} RIFF, *PRIFF, FAR *LPRIFF;

extern HINSTANCE ghInstDll;
#define	IDS_CAP_RTL	10000
extern BOOL gfIsRTL;

// capinit.c
BOOL CapWinDisconnectHardware(LPCAPSTREAM lpcs);
BOOL CapWinConnectHardware (LPCAPSTREAM lpcs, UINT wDeviceIndex);
BOOL capInternalGetDriverDesc (UINT wDriverIndex,
        LPTSTR lpszName, int cbName,
        LPTSTR lpszVer, int cbVer);
BOOL capInternalGetDriverDescA(UINT wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer);

// capwin.c
LONG FAR PASCAL LOADDS EXPORT CapWndProc (HWND hwnd, UINT msg, UINT wParam, LONG lParam);

#if defined CHICAGO
 VOID WINAPI OpenMMDEVLDR(
     void);

 VOID WINAPI CloseMMDEVLDR(
     void);

 VOID FreeContigMem (
     DWORD hMemContig);

 LPVOID AllocContigMem (
     DWORD   cbSize,
     LPDWORD phMemContig);
#endif


// capavi.c
LPVOID FAR PASCAL AllocSectorAlignedMem (DWORD dwRequest, DWORD dwAlign);
void FAR PASCAL FreeSectorAlignedMem(LPVOID p);
DWORDLONG GetFreePhysicalMemory(void);
DWORD CalcWaveBufferSize (LPCAPSTREAM lpcs);
BOOL AVIFileFini (LPCAPSTREAM lpcs, BOOL fWroteJunkChunks, BOOL fAbort);
UINT AVIAudioInit (LPCAPSTREAM lpcs);
UINT AVIAudioFini (LPCAPSTREAM lpcs);
UINT AVIAudioPrepare (LPCAPSTREAM lpcs);
UINT AVIAudioUnPrepare (LPCAPSTREAM lpcs);
UINT AVIVideoInit (LPCAPSTREAM lpcs);
UINT AVIVideoPrepare (LPCAPSTREAM lpcs);
UINT AVIVideoUnPrepare (LPCAPSTREAM lpcs);
void AVIFini(LPCAPSTREAM lpcs);
UINT AVIInit (LPCAPSTREAM lpcs);
BOOL FAR PASCAL SetInfoChunk(LPCAPSTREAM lpcs, LPCAPINFOCHUNK lpcic);
BOOL AVICapture (LPCAPSTREAM lpcs);

// capio.c
//
BOOL InitIndex (LPCAPSTREAM lpcs);
void FiniIndex (LPCAPSTREAM lpcs);
BOOL WriteIndex (LPCAPSTREAM lpcs, BOOL fJunkChunkWritten);
BOOL CapFileInit(LPCAPSTREAM lpcs);
BOOL WINAPI AVIWriteAudio (
    LPCAPSTREAM lpcs,
    LPWAVEHDR   lpwh,
    UINT        uIndex,
    LPUINT      lpuError,
    LPBOOL      pbPending);
BOOL WINAPI AVIWriteVideoFrame (
    LPCAPSTREAM lpcs,
    LPBYTE      lpData,
    DWORD       dwBytesUsed,
    BOOL        fKeyFrame,
    UINT        uIndex,
    UINT        nDropped,
    LPUINT      lpuError,
    LPBOOL      pbPending);
BOOL WINAPI AVIWriteDummyFrames (
    LPCAPSTREAM lpcs,
    UINT        nCount,
    LPUINT      lpuError,
    LPBOOL      pbPending);
VOID WINAPI AVIPreloadFat (LPCAPSTREAM lpcs);

// capfile.c
BOOL FAR PASCAL fileCapFileIsAVI (LPTSTR lpsz);
BOOL FAR PASCAL fileAllocCapFile(LPCAPSTREAM lpcs, DWORD dwNewSize);
BOOL FAR PASCAL fileSaveCopy(LPCAPSTREAM lpcs);
BOOL FAR PASCAL fileSavePalette(LPCAPSTREAM lpcs, LPTSTR lpszFileName);
BOOL FAR PASCAL fileOpenPalette(LPCAPSTREAM lpcs, LPTSTR lpszFileName);

//capmisc.c
UINT GetKey(BOOL fWait);
void errorDriverID (LPCAPSTREAM lpcs, DWORD dwError);
void FAR CDECL statusUpdateStatus (LPCAPSTREAM lpcs, UINT wID, ...);
void FAR CDECL errorUpdateError (LPCAPSTREAM lpcs, UINT wID, ...);

//capFrame.c
BOOL FAR PASCAL SingleFrameCaptureOpen (LPCAPSTREAM lpcs);
BOOL FAR PASCAL SingleFrameCaptureClose (LPCAPSTREAM lpcs);
BOOL FAR PASCAL SingleFrameCapture (LPCAPSTREAM lpcs);
BOOL SingleFrameWrite (
    LPCAPSTREAM             lpcs,       // capture stream
    LPVIDEOHDR              lpVidHdr,   // input header
    BOOL FAR 		    *pfKey,	// did it end up being a key frame?
    LONG FAR		    *plSize);	// size of returned image

//capMCI.c
void FAR PASCAL TimeMSToSMPTE (DWORD dwMS, LPTSTR lpTime);
int CountMCIDevicesByType ( UINT wType );
void MCIDeviceClose (LPCAPSTREAM lpcs);
BOOL MCIDeviceOpen (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDeviceGetPosition (LPCAPSTREAM lpcs, LPDWORD lpdwPos);
BOOL FAR PASCAL MCIDeviceSetPosition (LPCAPSTREAM lpcs, DWORD dwPos);
BOOL FAR PASCAL MCIDevicePlay (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDevicePause (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDeviceStop (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDeviceStep (LPCAPSTREAM lpcs, BOOL fForward);
void FAR PASCAL _LOADDS MCIStepCapture (LPCAPSTREAM lpcs);

#define AnsiToWide(lpwsz,lpsz,nChars) MultiByteToWideChar(CP_ACP, 0, lpsz, nChars, lpwsz, nChars)
#define WideToAnsi(lpsz,lpwsz,nChars) WideCharToMultiByte(CP_ACP, 0, lpwsz, nChars, lpsz, nChars, NULL, NULL)
#ifdef CHICAGO
 // chicago internal api to get a vxd visible alias for Win32 handle
 // this is used on the hCaptureEvent handle so that it can be signaled
 // from within 16 bit code.
 DWORD WINAPI OpenVxDHandle (HANDLE);
#endif

#ifdef _DEBUG
  BOOL FAR PASCAL _Assert(BOOL f, LPSTR szFile, int iLine);
  #define WinAssert(exp) (_Assert(exp, (LPSTR) __FILE__, __LINE__))
  extern void FAR CDECL dprintf(LPSTR, ...);
  #define DPF dprintf
#else
  #define dprintf ; / ## /
  #define DPF ; / ## /
  #define WinAssert(exp) 0
#endif


#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif    /* __cplusplus */

#endif /* INC_AVICAP_INTERNAL */
