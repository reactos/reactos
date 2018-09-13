/****************************************************************************
 *
 *   avicapi.h
 * 
 *   Internal, private definitions.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
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
#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

#define	IDS_CAP_RTL	10000

#ifndef LPHISTOGRAM
typedef DWORD huge * LPHISTOGRAM;
#endif

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) ((int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount))
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))

#define AVICAP                  DWORD FAR PASCAL
#define DROP_BUFFER_SIZE        2048
#define MAX_VIDEO_BUFFERS       1000
#define MIN_VIDEO_BUFFERS       5
#define DEF_WAVE_BUFFERS        4
#define MAX_WAVE_BUFFERS        10
#define _MAX_CAP_PATH           MAX_PATH  /* 260 */

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
#define CAPSTREAM_VERSION 1             // Increment whenever struct changes

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
    HTASK           hTaskCapture;               // I: capture task handle
    DWORD           dwReturn;                   // I: capture task return val
    HWND            hwnd;                       // I: our hwnd

    // Use MakeProcInstance to create all callbacks !!!

    // Status, error callbacks 
    CAPSTATUSCALLBACK   CallbackOnStatus;       // M: Status callback
    CAPERRORCALLBACK    CallbackOnError;        // M: Error callback

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
     
    // Window to display video
    BOOL            fLiveWindow;                // M: Preview video
    BOOL            fOverlayWindow;             // M: Overlay video
    BOOL            fScale;                     // M: Scale image to client
    POINT           ptScroll;                   // I: Scroll position
    HANDLE          hdd;                        // I: hDrawDib access handle
    HCURSOR         hWaitCursor;                // I: hourglass
    UINT            uiRegion;                   // I: CheckWindowMove
    RECT            rcRegionRect;               // I: CheckWindowMove
    DWORD           dwRegionOrigin;             // I: CheckWindowMove

    // Window update timer
    UINT            idTimer;                    // I: ID of preview timer
    UINT            uTimeout;                   // M: Preview rate in mS.

    // Capture destination and control
    CAPTUREPARMS    sCapParms;                  // M: how to capture
    BOOL            fCapturingToDisk;           // M: if capturing to disk
    BOOL            fCapturingNow;              // I: if performing capture
    BOOL            fStepCapturingNow;          // I: if performing MCI step capture
    BOOL            fFrameCapturingNow;         // I: if performing single frame capture
    BOOL            fStopCapture;               // M: if stop requested
    BOOL            fAbortCapture;              // M: if abort requested
    DWORD           dwTimeElapsedMS;            // I: Capture time in millisec

    // Index 
    HGLOBAL         hIndex;                     // I: handle to index mem
    DWORD           dwIndex;                    // I: index index
    DWORD           dwVideoChunkCount;          // I: # of video frames cap'd
    DWORD           dwWaveChunkCount;           // I: # of audio buffers cap'd
    DWORD _huge *   lpdwIndexStart;             // I: index start ptr
    DWORD _huge *   lpdwIndexEntry;             // I: index current ptr
    LPBYTE          lpDOSWriteBuffer;           // I: ptr to DOS buffer
    DWORD           dwDOSBufferSize;            // I: DOS buffer size

    // Video format
    DWORD           dwActualMicroSecPerFrame;   // I: Actual cap rate
    LPBITMAPINFO    lpBitsInfo;                 // I: Video format
    int             dxBits;                     // I: video size x
    int             dyBits;                     // I: video size y
    LPSTR           lpBits;                     // I: Single frame capture buf
    VIDEOHDR        VidHdr;                     // I: Single frame header
    COMPVARS        CompVars;                   // M: Set by ICCompressorChoose
    LPIAVERAGE      lpia;                       // I: Image averaging struct
    VIDEOHDR        VidHdr2x;                   // I: VideoHeader at 2x
    LPBITMAPINFOHEADER  lpbmih2x;               // I: lpbi at 2x

    // Video Buffer management
    BOOL            fVideoDataIsCompressed;     // I: if don't use dwVideoSize
    DWORD           dwVideoSize;                // I: size of non-comp buffer incl chunk 
    DWORD           dwVideoJunkSize;            // I: Initial non-comp. junk size
    int             iNumVideo;                  // I: Number of actual video buffers
    int             iNextVideo;                 // I: index into video buffers
    DWORD           dwFramesDropped;            // I: number of frames dropped
    BYTE            DropFrame[DROP_BUFFER_SIZE];    // I: Create a Dummy VideoHdr
    LPVIDEOHDR      alpVideoHdr[MAX_VIDEO_BUFFERS]; // I: array of buf ptrs
    BOOL            fBuffersOnHardware;         // I: if driver all'd buffers

    // Palettes
    HPALETTE        hPalCurrent;                // I: handle of current pal
    BOOL            fUsingDefaultPalette;       // I: no user defined pal
    int             nPaletteColors;             // M: only changed by UI
    LPVOID          lpCapPal;                   // I: LPCAPPAL manual pals
    
    // Audio Capture Format
    BOOL            fAudioHardware;             // I: if audio hardware present
    LPWAVEFORMATEX  lpWaveFormat;               // I: wave format
    WAVEHDR         WaveHdr;                    // I: Wave header
    HWAVE           hWaveIn;                    // I: Wave input channel
    DWORD           dwWaveBytes;                // I: Total wave bytes cap'd
    DWORD           dwWaveSize;                 // I: wave buffer size

    // Audio Buffer management
    LPWAVEHDR       alpWaveHdr[MAX_WAVE_BUFFERS]; // I: wave buff array
    int             iNextWave;                  // I: Index into wave buffers
    int             iNumAudio;                  // I: Number of actual audio buffers
    BOOL            fAudioYield;                // I: ACM audio yield required
    BOOL            fAudioBreak;                // I: Audio underflow

    // MCI Capture
    char            achMCIDevice[_MAX_CAP_PATH];// MCI device name
    DWORD           dwMCIError;                 // I: Last MCI error value
    enum mcicapstates MCICaptureState;          // I: MCI State machine
    DWORD           dwMCICurrentMS;             // I: Current MCI position
    DWORD           dwMCIActualStartMS;         // I: Actual MCI start MS
    DWORD           dwMCIActualEndMS;           // I: Actual MCI end position

    // Output file
    char            achFile [_MAX_CAP_PATH];    // M: name of capture file
    char            achSaveAsFile [_MAX_CAP_PATH]; // M: name of saveas file
    BOOL            fCapFileExists;             // I: if have a capture file
    LONG            lCapFileSize;               // M: in bytes
    BOOL            fFileCaptured;              // I: if we've cap'd to file
    HMMIO           hmmio;                      // I: MMIO handle for writing
    DWORD           dwAVIHdrSize;               // I: size of header
    DWORD           dwAVIHdrPos;                // I: file offset of hdr
    
    LONG	    lUser;			// M: Data for the user
    LPVOID          lpInfoChunks;               // M: information chunks
    LONG            cbInfoChunks;               // M: sizeof information chks
    BOOL            fLastStatusWasNULL;         // I: don't repeat null msgs
    BOOL            fLastErrorWasNULL;          // I: don't repeat null msgs
} CAPSTREAM;
typedef CAPSTREAM FAR * LPCAPSTREAM;

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

extern HINSTANCE ghInst;
extern BOOL gfIsRTL;

// capinit.c
BOOL CapWinDisconnectHardware(LPCAPSTREAM lpcs);
BOOL CapWinConnectHardware (LPCAPSTREAM lpcs, WORD wDeviceIndex);
BOOL capInternalGetDriverDesc (WORD wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer);

// capwin.c
LONG FAR PASCAL _loadds CapWndProc (HWND hwnd, unsigned msg, WORD wParam, LONG lParam);
                  
// capavi.c                              
BOOL InitIndex (LPCAPSTREAM lpcs);
void FiniIndex (LPCAPSTREAM lpcs);
BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame);
BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize);
BOOL WriteIndex (LPCAPSTREAM lpcs, BOOL fJunkChunkWritten);
DWORD GetFreePhysicalMemory(void);
DWORD CalcWaveBufferSize (LPCAPSTREAM lpcs);
BOOL AVIFileInit(LPCAPSTREAM lpcs);
BOOL AVIFileFini (LPCAPSTREAM lpcs, BOOL fWroteJunkChunks, BOOL fAbort);
WORD AVIAudioInit (LPCAPSTREAM lpcs);
WORD AVIAudioFini (LPCAPSTREAM lpcs);
WORD AVIAudioPrepare (LPCAPSTREAM lpcs, HWND hWndCallback);
WORD AVIAudioUnPrepare (LPCAPSTREAM lpcs);
WORD AVIVideoInit (LPCAPSTREAM lpcs);
WORD AVIVideoPrepare (LPCAPSTREAM lpcs);
WORD AVIVideoUnPrepare (LPCAPSTREAM lpcs);
void AVIFini(LPCAPSTREAM lpcs);
WORD AVIInit (LPCAPSTREAM lpcs);
BOOL NEAR PASCAL AVIWrite(LPCAPSTREAM lpcs, LPVOID p, DWORD dwSize);
BOOL AVIWriteDummyFrames (LPCAPSTREAM lpcs, int nCount);
BOOL AVIWriteVideoFrame (LPCAPSTREAM lpcs, LPVIDEOHDR lpVidHdr);
BOOL FAR PASCAL SetInfoChunk(LPCAPSTREAM lpcs, LPCAPINFOCHUNK lpcic);

BOOL AVICapture (LPCAPSTREAM lpcs);


// capfile.c
BOOL FAR PASCAL fileCapFileIsAVI (LPSTR lpsz);
BOOL FAR PASCAL fileAllocCapFile(LPCAPSTREAM lpcs, DWORD dwNewSize);
BOOL FAR PASCAL fileSaveCopy(LPCAPSTREAM lpcs);
BOOL FAR PASCAL fileSavePalette(LPCAPSTREAM lpcs, LPSTR lpszFileName);
BOOL FAR PASCAL fileOpenPalette(LPCAPSTREAM lpcs, LPSTR lpszFileName);

//capmisc.c
WORD GetKey(BOOL fWait);
void errorDriverID (LPCAPSTREAM lpcs, DWORD dwError);
void FAR _cdecl statusUpdateStatus (LPCAPSTREAM lpcs, WORD wID, ...);
void FAR _cdecl errorUpdateError (LPCAPSTREAM lpcs, WORD wID, ...);

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
void FAR PASCAL TimeMSToSMPTE (DWORD dwMS, LPSTR lpTime);
int CountMCIDevicesByType ( WORD wType );
void MCIDeviceClose (LPCAPSTREAM lpcs);
BOOL MCIDeviceOpen (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDeviceGetPosition (LPCAPSTREAM lpcs, LPDWORD lpdwPos);
BOOL FAR PASCAL MCIDeviceSetPosition (LPCAPSTREAM lpcs, DWORD dwPos);
BOOL FAR PASCAL MCIDevicePlay (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDevicePause (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDeviceStop (LPCAPSTREAM lpcs);
BOOL FAR PASCAL MCIDeviceStep (LPCAPSTREAM lpcs, BOOL fForward);
void FAR PASCAL _loadds MCIStepCapture (LPCAPSTREAM lpcs);

long FAR PASCAL muldiv32(long, long, long);
                        
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


