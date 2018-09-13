/*++ BUILD Version: 0002
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOW.H
 *  Constants, macros, etc common to WOW16/WOW32
 *
 *  History:
 *  Created 25-Jan-1991 by Jeff Parsons (jeffpar)
 *  Added SHELL defines 14-April-92 Chandan Chauhan (ChandanC)
 *   and Win 31 parameter validation support.
 *  Modified 12-May-1992 by Mike Tricker (MikeTri) Added MultiMedia declarations
 *                                                 and callback support
 *
--*/


#define WIN31

#include <mvdm.h>
#include <bop.h>
#ifndef NOEXTERNS
#include <softpc.h>
#endif
#include <wownt32.h>

#ifdef i386
#ifndef DEBUG     // should be DEBUG_OR_WOWPROFILE, but
                  // that won't work for assembler as things are.

//
// Flag to control enable/disable W32TryCall function.
//

#define NO_W32TRYCALL 1
#endif
#endif

/* WOW constants
 */
#define MAX_VDMFILENAME 144 // must be >= 144 (see GetTempFileName)
#define GRAINYTIC_RES   0x3f // will truncate to lower multiple of 64


/* Logging/debugging macros
 */
/* XLATOFF */
#define GRAINYTICS(dwordtickcount)  ((dwordtickcount) & (~GRAINYTIC_RES))
#define IFLOG(l)    if (l==iLogLevel && (iLogLevel&1) || l<=iLogLevel && !(iLogLevel&1) || l == 0)

#define OPENLOG()   (hfLog != (HANDLE)-1?hfLog:(hfLog=CreateFile("log",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,0,NULL)))
#define APPENDLOG() if (hfLog == (HANDLE)-1) {hfLog=CreateFile("log",GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,0,NULL) ; SetFilePointer (hfLog,0,NULL,FILE_END); }
#define CLOSELOG()  if (hfLog != (HANDLE)-1) {CloseHandle(hfLog); hfLog=(HANDLE)-1;}

#undef  LOG
#ifdef  NOLOG
#define LOG(l,args)
#define SETREQLOG(l)
#else
#define SETREQLOG(l) iReqLogLevel = (l)
#define LOG(l,args)  {SETREQLOG(l) ; logprintf args;}
#endif
#define MODNAME(module)

#ifdef  DEBUG
#define STATIC
#define INT3()      _asm int 3
#define IFDEBUG(f)  if (f)
#define ELSEDEBUG   else
#define LOGDEBUG(l,args) LOG(l,args)
#else
#define STATIC static
#define INT3()
#define IFDEBUG(f)
#define ELSEDEBUG
#define LOGDEBUG(l,args)
#endif
/* XLATON */


/* 16-bit Windows constants
 */
#define CW_USEDEFAULT16 ((SHORT)0x8000)


/* 16-bit Windows types
 */
typedef WORD    HAND16;
typedef WORD    HTASK16;
typedef WORD    HINST16;
typedef WORD    HMOD16;
typedef WORD    HRES16;
typedef WORD    HRESI16;
typedef WORD    HRESD16;
typedef WORD    HWND16;
typedef WORD    HMENU16;
typedef WORD    HDC16;
typedef WORD    HRGN16;
typedef WORD    HICON16;
typedef WORD    HCUR16;
typedef WORD    HBRSH16;
typedef WORD    HPAL16;
typedef WORD    HBM16;
typedef WORD    HFONT16;
typedef WORD    HMEM16;
typedef DWORD   HHOOK16;

typedef WORD    HMMIO16;  // for MultiMedia - MikeTri 12-May-1992
typedef WORD    HMIDIIN16;
typedef WORD    HMIDIOUT16;
typedef WORD    HWAVEIN16;
typedef WORD    HWAVEOUT16;
typedef WORD    HDRVR16;
typedef DWORD   HPSTR16;

typedef SHORT   INT16;
typedef SHORT   BOOL16;

/* 16-bit pointer types (VP == VDM Ptr)
 */
typedef DWORD   VPVOID;     // VDM address (seg:off)
typedef VPVOID  VPBYTE;     //
typedef VPVOID  VPWORD;     //
typedef VPVOID  VPDWORD;    //
typedef VPVOID  VPSHORT;    //
typedef VPVOID  VPLONG;     //
typedef VPVOID  VPSTR;      // should use VPSZ or VPBYTE instead, as approp.
typedef VPVOID  VPSZ;       //
typedef VPVOID  VPPROC;     //
typedef VPVOID  VPWNDPROC;  //
typedef VPVOID  VPINT16;    //
typedef VPVOID  VPBOOL16;   //
typedef VPVOID  *PVPVOID;   // pointer to VDM address

typedef VPVOID  VPCSTR;     // MultiMedia Extensions - MikeTri 12-May-1992
typedef VPVOID  VPMMIOPROC16;
typedef VPVOID  VPHMIDIIN16;
typedef VPVOID  VPHMIDIOUT16;
typedef VPVOID  VPPATCHARRAY16;
typedef VPVOID  VPKEYARRAY16;
typedef VPVOID  VPHWAVEIN16;
typedef VPVOID  VPHWAVEOUT16;
typedef VPVOID  VPTIMECALLBACK16;
typedef VPVOID  VPTASKCALLBACK16;

/* Types
 */
typedef ULONG   (FASTCALL *LPFNW32)(PVDMFRAME);

/* Dispatch table entry
**
 */
typedef struct _W32 {   /* w32 */
    LPFNW32 lpfnW32;    // function address
#ifdef DEBUG_OR_WOWPROFILE
    LPSZ    lpszW32;    // function name (DEBUG version only)
    DWORD   cbArgs;     // # of bytes of arguments (DEBUG version only)
    DWORD   cCalls;     // # of times this API called
    DWORD   cTics;      // sum total # of tics ellapsed for all invocations
#endif // DEBUG_OR_WOWPROFILE
} W32, *PW32;

/* XLATOFF */
#pragma pack(1)
/* XLATON */

/* Window proc/dialog box callback function parameter format
 */
typedef struct _PARMWP {    /* wp */
    LONG    lParam;     //
    WORD    wParam;     //
    WORD    wMsg;       //
    WORD    hwnd;       //
    WORD    hInst;      // hInstance of window that we are returning to
} PARMWP;


/* EnumPropsProc callback function parameter format
 */
typedef struct _PARMEPP {   /* epp */
    HAND16  hData;
    VPVOID  vpString;
    HWND16  hwnd;
} PARMEPP;


/* EnumWindows/EnumChildWindows/EnumTaskWindows callback function parameter format
 */
typedef struct _PARMEWP {       /* ewp */
    LONG    lParam;             // app-defined data
    HWND16  hwnd;               // 16-bit window handle
} PARMEWP;


/* EnumFonts callback function parameter format
 */
typedef struct _PARMEFP {       /* efp */
    VPVOID  vpData;     // app-defined data
    SHORT   nFontType;      //
    VPVOID  vpTextMetric;   // pointer to TEXTMETRIC16
    VPVOID  vpLogFont;      // pointer to LOGFONT16
} PARMEFP;


/* EnumObj callback function parameter format
 */
typedef struct _PARMEOP {       /* eop */
    VPVOID  vpData;     // app-defined data
    VPVOID  vpLogObject;
} PARMEOP;


/* EnumMetaFile callback function parameter format
 */
typedef struct _PARMEMP {       /* emp */
    VPVOID  vpData;     // app-defined data
    SHORT   nObjects;       // # objects
    VPVOID  vpMetaRecord;   // pointer to METARECORD16
    VPVOID  vpHandleTable;  // pointer to HANDLETABLE16
    HDC16   hdc;        // hdc
} PARMEMP;

/* Hook Callback function parameter format
 */
typedef struct _PARMHKP {       /* hkp */
    VPVOID  lParam;
    SHORT   wParam;
    SHORT   nCode;          // action code
} PARMHKP;

/* Subclass Callback function parameter format
 */
typedef struct _PARMSCP {       /* scp */
    SHORT    iOrdinal;          // oridnal number;
} PARMSCP;

/* LineDDA Callback function parameter format
 */
typedef struct _PARMDDA {   /* dda */
    VPVOID vpData;
    SHORT  y;
    SHORT  x;
} PARMDDA;

/* Graystring callback function parameter format
 */
typedef struct _PARMGST {   /* gst */
    SHORT n;
    DWORD data;
    HDC16 hdc;
} PARMGST;


typedef struct _PARMDIR { /* cdir */
    SHORT wDrive;
    VPSZ  vpDir;  // directory name
} PARMDIR;

typedef struct _PARMSAP { /* sap */
    SHORT  code;    //
    HAND16 hPr;
} PARMSAP;


/* WordBreakProc callback function parameter format
 */
typedef struct _PARMWBP {       /* wbp */
    SHORT   action;
    SHORT   cbEditText;
    SHORT   ichCurrentWord;
    VPVOID  lpszEditText;
} PARMWBP;


/*++

  MultiMedia callback definitions added, and also to _PARM16 - MikeTri

--*/

/* midiInOpen (MidiInFunc) Callback function parameter format
 */

typedef struct _PARMMIF {       /* mif */
    DWORD     dwParam2;
    DWORD     dwParam1;
    DWORD     dwInstance;
    WORD      wMsg;
    HMIDIIN16 hMidiIn;
} PARMMIF;

/* midiOutOpen (MidiOutFunc) Callback function parameter format
 */

typedef struct _PARMMOF {       /* mof */
    DWORD      dwParam2;
    DWORD      dwParam1;
    DWORD      dwInstance;
    WORD       wMsg;
    HMIDIOUT16 hMidiOut;
} PARMMOF;

/* mmioInstallIOProc (IOProc) Callback function parameter format
 */

typedef struct _PARMIOP {      /* iop */
    LONG      lParam2;
    LONG      lParam1;
    WORD      wMsg;
    VPVOID    lpmmioinfo;
} PARMIOP;

/* timeSetEvent (TimeFunc) Callback function parameter format
 */

typedef struct _PARMTIF {       /* tif */
    DWORD     dw2;
    DWORD     dw1;
    DWORD     dwUser;
    WORD      wMsg;
    WORD      wID;
} PARMTIF;

/* waveInOpen (WaveInFunc) Callback function parameter format
 */

typedef struct _PARMWIF {       /* wif */
    DWORD     dwParam2;
    DWORD     dwParam1;
    DWORD     dwInstance;
    WORD      wMsg;
    HWAVEIN16 hWaveIn;
} PARMWIF;

/* waveOutOpen (WaveOutFunc) Callback function parameter format
 */

typedef struct _PARMWOF {       /* wof */
    DWORD      dwParam2;
    DWORD      dwParam1;
    DWORD      dwInstance;
    WORD       wMsg;
    HWAVEOUT16 hWaveOut;
} PARMWOF;

/* WOWCallback16 function parameter format
 */

typedef struct _PARMWCB16 {       /* wcb16 */
    WORD       wArgs[8];
} PARMWCB16;

typedef struct _PARMLSTRCMP {     /* lstrcmp16 */
    VPVOID     lpstr1;
    VPVOID     lpstr2;
} PARMLSTRCMP;

/* PARM16 is the union of all the callback parameter structures
 */
typedef union _PARM16 {     /* parm16 */
    PARMWP  WndProc;        // for window procs
    PARMEWP EnumWndProc;        // for window enum functions
    PARMEFP EnumFontProc;       // for font enum functions
    PARMEOP EnumObjProc;       // for obj enum functions
    PARMEMP EnumMetaProc;       // for metafile enum functions
    PARMEPP EnumPropsProc;  // for properties
    PARMHKP HookProc;           // for Hooks
    PARMSCP SubClassProc;   // for subclass thunks
    PARMDDA LineDDAProc;    // for LineDDA
    PARMGST GrayStringProc; // for GrayString
    PARMDIR CurDir;
    PARMSAP SetAbortProc;   // for SetAbortProc
    PARMMIF MidiInFunc;         // for midiInOpen functions - MikeTri 27-Mar-1992
    PARMMOF MidiOutFunc;        // for midiOutOpen functions
    PARMIOP IOProc;             // for mmioInstallIOProc functions
    PARMTIF TimeFunc;           // for timeSetEvent functions
    PARMWIF WaveInFunc;         // for waveInOpen functions
    PARMWOF WaveOutFunc;        // for waveOutOpen functions
    PARMWBP WordBreakProc;      // for WordBreakProc
    PARMWCB16 WOWCallback16;    // for WOWCallback16
    PARMLSTRCMP lstrcmpParms;   // for WOWlstrcmp16 (pfnWowIlstrsmp to user32)
} PARM16, *PPARM16;


/* VDMFRAME is built by wow16cal.asm in the kernel, and is utilized
 * by all the WOW32 thunks
 */
typedef struct _VDMFRAME {  /* vf */
    WORD    wTDB;       // 16-bit kernel handle for calling task
    WORD    wRetID;     // internal call-back function ID Do NOT Move
    WORD    wLocalBP;   //
    WORD    wDI;        //
    WORD    wSI;        //
    WORD    wAX;        //
    WORD    wDX;        // keep DX right after AX!!!
    WORD    wAppDS;     // app DS at time of call
    WORD    wGS;
    WORD    wFS;
    WORD    wCX;        // REMOVE LATER
    WORD    wES;        // REMOVE LATER
    WORD    wBX;        // REMOVE LATER
    WORD    wBP;        // BP Chain +1
    VPVOID  wThunkCSIP; // ret addr of THUNK - update wowtd.h if you move it
    DWORD   wCallID;    // internal WOW16 module/function ID
    WORD    cbArgs;     // byte count of args pushed
    VPVOID  vpCSIP;     // far return address to app
    BYTE    bArgs;      // start of arguments from app
} VDMFRAME;
typedef VDMFRAME UNALIGNED *PVDMFRAME;

/* CBVDMFRAME is built by callback16 in wow32.dll and in wow16cal.asm
 * the definition of VDMFRAME and CBACKVDMFRAME must be in sync
 */

typedef struct _CBVDMFRAME {  /* cvf */
    WORD    wTDB;       // must match VDMFRAME
    WORD    wRetID;     // must match VDMFRAME
    WORD    wLocalBP;   // must match VDMFRAME
    PARM16  Parm16;     // space for window/enum proc parameters
    VPVOID  vpfnProc;   // address of window/enum proc
    DWORD   vpStack;    // orginal ss:sp. used in callback16
    WORD    wAX;        //
    WORD    wDX;        // keep DX right after AX!!!
    WORD    wGenUse1;   // extra words for general use. for convenience
    WORD    wGenUse2;   // extra words for general use. for convenience
} CBVDMFRAME;
typedef CBVDMFRAME UNALIGNED *PCBVDMFRAME;

typedef struct _POINT16 {       /* pt16 */
    SHORT   x;
    SHORT   y;
} POINT16;
typedef POINT16 UNALIGNED *PPOINT16;
typedef VPVOID VPPOINT16;

/* POINTL16 is new for Win95 and is identical to Win32 POINT/POINTL structures */

typedef struct _POINTL16 {       /* ptl16 */
    LONG   x;
    LONG   y;
} POINTL16;
typedef POINTL16 UNALIGNED *PPOINTL16;
typedef VPVOID VPPOINTL16;

typedef struct _RASTERIZER_STATUS16 {  /* rs16 */
    INT16   nSize;
    INT16   wFlags;
    INT16   nLanguageID;
} RASTERIZER_STATUS16;
typedef RASTERIZER_STATUS16 UNALIGNED *PRASTERIZER_STATUS16;
typedef VPVOID VPRASTERIZER_STATUS16;

typedef struct _GLYPHMETRICS16 {  /*glyph16 */
    WORD    gmBlackBoxX;
    WORD    gmBlackBoxY;
    POINT16 gmptGlyphOrigin;
    INT16   gmCellIncX;
    INT16   gmCellIncY;
} GLYPHMETRICS16;
typedef GLYPHMETRICS16 UNALIGNED *PGLYPHMETRICS16;
typedef VPVOID VPGLYPHMETRICS16;

typedef struct _ABC16 {        /* abc16 */
    INT16   abcA;
    WORD    abcB;
    INT16   abcC;
} ABC16;
typedef ABC16 UNALIGNED *PABC16;
typedef VPVOID VPABC16;

typedef struct _FIXED16 {        /* fxd16 */
    WORD    fract;
    INT16   value;
} FIXED16;
typedef FIXED16 UNALIGNED *PFIXED16;
typedef VPVOID VPFIXED16;

typedef struct _MAT216 {        /* mat216 */
    FIXED16 eM11;
    FIXED16 eM12;
    FIXED16 eM21;
    FIXED16 eM22;
} MAT216;
typedef MAT216 UNALIGNED *PMAT216;
typedef VPVOID VPMAT216;


/* 16-bit API structures, and their pointers
 */
typedef struct _RECT16 {        /* rc16 */
    SHORT   left;
    SHORT   top;
    SHORT   right;
    SHORT   bottom;
} RECT16;
typedef RECT16 UNALIGNED *PRECT16;
typedef VPVOID VPRECT16;

/* RECTL16 is new for Win95 and is identical to Win32 RECTL structure */

typedef struct _RECTL16 {        /* rcl16 */
    LONG   left;
    LONG   top;
    LONG   right;
    LONG   bottom;
} RECTL16;
typedef RECTL16 UNALIGNED *PRECTL16;
typedef VPVOID VPRECTL16;

typedef struct _KERNINGPAIR16 {        /* k16 */
    WORD   wFirst;
    WORD   wSecond;
    INT16  iKernAmount;
} KERNINGPAIR16;
typedef KERNINGPAIR16 UNALIGNED *PKERNINGPAIR16;
typedef VPVOID VPKERNINGPAIR16;




typedef struct _MSG16 {         /* msg16 */
    HWND16  hwnd;
    WORD    message;
    WORD    wParam;
    LONG    lParam;
    DWORD   time;
    POINT16 pt;
} MSG16;
typedef MSG16 UNALIGNED *PMSG16;
typedef VPVOID VPMSG16;

typedef struct _PAINTSTRUCT16 {     /* ps16 */
    HDC16   hdc;
    BOOL16  fErase;
    RECT16  rcPaint;
    BOOL16  fRestore;
    BOOL16  fIncUpdate;
    BYTE    rgbReserved[16];
} PAINTSTRUCT16;
typedef PAINTSTRUCT16 UNALIGNED *PPAINTSTRUCT16;
typedef VPVOID VPPAINTSTRUCT16;

typedef struct _WNDCLASS16 {        /* wc16 */
    WORD    style;
    VPWNDPROC vpfnWndProc;
    SHORT   cbClsExtra;
    SHORT   cbWndExtra;
    HAND16  hInstance;
    HICON16 hIcon;
    HCUR16  hCursor;
    HBRSH16 hbrBackground;
    VPSZ    vpszMenuName;
    VPSZ    vpszClassName;
} WNDCLASS16;
typedef WNDCLASS16 UNALIGNED *PWNDCLASS16;
typedef VPVOID VPWNDCLASS16;

typedef struct _PALETTEENTRY16 {    /* pe16 */
    BYTE    peRed;
    BYTE    peGreen;
    BYTE    peBlue;
    BYTE    peFlags;
} PALETTEENTRY16;
typedef PALETTEENTRY16 UNALIGNED *PPALETTEENTRY16;
typedef VPVOID VPPALETTEENTRY16;

typedef struct _RGBTRIPLE16 {       /* rgbt16 */
    BYTE    rgbtBlue;
    BYTE    rgbtGreen;
    BYTE    rgbtRed;
} RGBTRIPLE16;

typedef struct  _BITMAPCOREHEADER16 { /* bmch16 */
    DWORD   bcSize;
    WORD    bcWidth;
    WORD    bcHeight;
    WORD    bcPlanes;
    WORD    bcBitCount;
} BITMAPCOREHEADER16;
typedef BITMAPCOREHEADER16 UNALIGNED *PBITMAPCOREHEADER16;

typedef struct  _BITMAPCOREINFO16 {   /* bmci16 */
    BITMAPCOREHEADER16 bmciHeader;
    RGBTRIPLE16 bmciColors[1];
} BITMAPCOREINFO16;
typedef BITMAPCOREINFO16 UNALIGNED *PBITMAPCOREINFO16;


typedef struct  _CLIENTCREATESTRUCT16 { /* ccs16 */
    HMENU16 hWindowMenu;
    WORD    idFirstChild;
} CLIENTCREATESTRUCT16;
typedef CLIENTCREATESTRUCT16 UNALIGNED *PCLIENTCREATESTRUCT16;



typedef struct _LOGPALETTE16 {      /* logpal16 */
    WORD    palVersion;
    WORD    palNumEntries;
    PALETTEENTRY16 palPalEntry[1];
} LOGPALETTE16;
typedef LOGPALETTE16 UNALIGNED *PLOGPALETTE16;
typedef VPVOID VPLOGPALETTE16;

typedef SHORT CATCHBUF16[9];        /* cb16 */
typedef VPSHORT VPCATCHBUF16;

typedef struct _OFSTRUCT16 {        /* of16 */
    BYTE    cBytes;
    BYTE    fFixedDisk;
    WORD    nErrCode;
    BYTE    reserved[4];
    BYTE    szPathName[128];
} OFSTRUCT16;
typedef OFSTRUCT16 UNALIGNED *POFSTRUCT16;
typedef VPVOID VPOFSTRUCT16;

typedef struct _DCB16 {         /* dcb16 */
    BYTE    Id;             // Internal Device ID
    WORD    BaudRate;           // Baudrate at which runing
    BYTE    ByteSize;           // Number of bits/byte, 4-8
    BYTE    Parity;         // 0-4=None,Odd,Even,Mark,Space
    BYTE    StopBits;           // 0,1,2 = 1, 1.5, 2
    WORD    RlsTimeout;         // Timeout for RLSD to be set
    WORD    CtsTimeout;         // Timeout for CTS to be set
    WORD    DsrTimeout;         // Timeout for DSR to be set
    WORD    wFlags;             // Bitfield flags
  /*+++ These are the bitfield definitions in wFlags above --
    BYTE    fBinary: 1;         // Binary Mode (skip EOF check
    BYTE    fRtsDisable:1;      // Don't assert RTS at init time
    BYTE    fParity: 1;         // Enable parity checking
    BYTE    fOutxCtsFlow:1;     // CTS handshaking on output
    BYTE    fOutxDsrFlow:1;     // DSR handshaking on output
    BYTE    fDummy: 2;          // Reserved
    BYTE    fDtrDisable:1;      // Don't assert DTR at init time

    BYTE    fOutX: 1;           // Enable output X-ON/X-OFF
    BYTE    fInX: 1;            // Enable input X-ON/X-OFF
    BYTE    fPeChar: 1;         // Enable Parity Err Replacement
    BYTE    fNull: 1;           // Enable Null stripping
    BYTE    fChEvt: 1;          // Enable Rx character event.
    BYTE    fDtrflow: 1;        // DTR handshake on input
    BYTE    fRtsflow: 1;        // RTS handshake on input
    BYTE    fDummy2: 1;         //
  ---*/
    CHAR    XonChar;            // Tx and Rx X-ON character
    CHAR    XoffChar;           // Tx and Rx X-OFF character
    WORD    XonLim;             // Transmit X-ON threshold
    WORD    XoffLim;            // Transmit X-OFF threshold
    CHAR    PeChar;             // Parity error replacement char
    CHAR    EofChar;            // End of Input character
    CHAR    EvtChar;            // Recieved Event character
    WORD    TxDelay;            // Amount of time between chars
} DCB16;
typedef DCB16 UNALIGNED *PDCB16;
typedef VPVOID VPDCB16;

typedef struct _COMSTAT16 {     /* cs16 */
    BYTE    status;
  /*+++ These are bitfield definitions defined in status above --
    BYTE    fCtsHold: 1;        // transmit is on CTS hold
    BYTE    fDsrHold: 1;        // transmit is on DSR hold
    BYTE    fRlsdHold: 1;       // transmit is on RLSD hold
    BYTE    fXoffHold: 1;       // received handshake
    BYTE    fXoffSent: 1;       // issued handshake
    BYTE    fEof: 1;            // end of file character found
    BYTE    fTxim: 1;           // character being transmitted
  ---*/
    WORD    cbInQue;            // count of characters in Rx Queue
    WORD    cbOutQue;           // count of characters in Tx Queue
} COMSTAT16;
typedef COMSTAT16 UNALIGNED *PCOMSTAT16;
typedef VPVOID VPCOMSTAT16;

#ifdef FE_SB                     // wowfax support for Japanese
typedef struct _DEV_BITMAP16 {      /* devbm16 */
    SHORT   bmType;
    SHORT   bmWidth;
    SHORT   bmHeight;
    SHORT   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    VPBYTE  bmBits;
    LONG    bmWidthPlanes;
    LONG    bmlpPDevice;
    SHORT   bmSegmentIndex;
    SHORT   bmScanSegment;
    SHORT   bmFillBytes;
    SHORT   reserved1;
    SHORT   reserved2;
} DEV_BITMAP16;
typedef DEV_BITMAP16 UNALIGNED *PDEV_BITMAP16;
typedef VPVOID VPDEV_BITMAP16;
#endif // FE_SB

typedef struct _BITMAP16 {      /* bm16 */
    SHORT   bmType;
    SHORT   bmWidth;
    SHORT   bmHeight;
    SHORT   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    VPBYTE  bmBits;
} BITMAP16;
typedef BITMAP16 UNALIGNED *PBITMAP16;
typedef VPVOID VPBITMAP16;

typedef struct _LOGBRUSH16 {        /* lb16 */
    WORD    lbStyle;
    DWORD   lbColor;
    SHORT   lbHatch;
} LOGBRUSH16;
typedef LOGBRUSH16 UNALIGNED *PLOGBRUSH16;
typedef VPVOID VPLOGBRUSH16;

/* ASM
LF_FACESIZE equ 32
 */
typedef struct _LOGFONT16 {     /* lf16 */
    SHORT   lfHeight;
    SHORT   lfWidth;
    SHORT   lfEscapement;
    SHORT   lfOrientation;
    SHORT   lfWeight;
    BYTE    lfItalic;
    BYTE    lfUnderline;
    BYTE    lfStrikeOut;
    BYTE    lfCharSet;
    BYTE    lfOutPrecision;
    BYTE    lfClipPrecision;
    BYTE    lfQuality;
    BYTE    lfPitchAndFamily;
    BYTE    lfFaceName[LF_FACESIZE];
} LOGFONT16;
typedef LOGFONT16 UNALIGNED *PLOGFONT16;
typedef VPVOID VPLOGFONT16;

/* ASM
LF_FULLFACESIZE equ 64
 */
/* Structure passed to FONTENUMPROC */
typedef struct _ENUMLOGFONT16 { /* elp16 */
    LOGFONT16   elfLogFont;
    char        elfFullName[LF_FULLFACESIZE];
    char        elfStyle[LF_FACESIZE];
} ENUMLOGFONT16;
typedef ENUMLOGFONT16 UNALIGNED *PENUMLOGFONT16;
typedef VPVOID VPENUMLOGFONT16;

typedef struct _LOGPEN16 {      /* lp16 */
    WORD    lopnStyle;
    POINT16 lopnWidth;
    DWORD   lopnColor;
} LOGPEN16;
typedef LOGPEN16 UNALIGNED *PLOGPEN16;
typedef VPVOID VPLOGPEN16;

typedef struct _RGBQUAD16 {      /* rgbq16 */
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD16;
typedef RGBQUAD16 UNALIGNED *PRGBQUAD16;
typedef VPVOID VPRGBQUAD16;

typedef BITMAPINFOHEADER BITMAPINFOHEADER16;
typedef BITMAPINFOHEADER16 UNALIGNED *PBITMAPINFOHEADER16;
typedef VPVOID VPBITMAPINFOHEADER16;

typedef BITMAPINFO BITMAPINFO16;
typedef BITMAPINFO16 UNALIGNED *PBITMAPINFO16;
typedef VPVOID VPBITMAPINFO16;

typedef struct _TEXTMETRIC16 {      /* tm16 */
    SHORT   tmHeight;
    SHORT   tmAscent;
    SHORT   tmDescent;
    SHORT   tmInternalLeading;
    SHORT   tmExternalLeading;
    SHORT   tmAveCharWidth;
    SHORT   tmMaxCharWidth;
    SHORT   tmWeight;
    BYTE    tmItalic;
    BYTE    tmUnderlined;
    BYTE    tmStruckOut;
    BYTE    tmFirstChar;
    BYTE    tmLastChar;
    BYTE    tmDefaultChar;
    BYTE    tmBreakChar;
    BYTE    tmPitchAndFamily;
    BYTE    tmCharSet;
    SHORT   tmOverhang;
    SHORT   tmDigitizedAspectX;
    SHORT   tmDigitizedAspectY;
} TEXTMETRIC16;
typedef TEXTMETRIC16 UNALIGNED *PTEXTMETRIC16;
typedef VPVOID VPTEXTMETRIC16;

typedef struct _NEWTEXTMETRIC16 {      /* ntm16 */
    SHORT   tmHeight;
    SHORT   tmAscent;
    SHORT   tmDescent;
    SHORT   tmInternalLeading;
    SHORT   tmExternalLeading;
    SHORT   tmAveCharWidth;
    SHORT   tmMaxCharWidth;
    SHORT   tmWeight;
    BYTE    tmItalic;
    BYTE    tmUnderlined;
    BYTE    tmStruckOut;
    BYTE    tmFirstChar;
    BYTE    tmLastChar;
    BYTE    tmDefaultChar;
    BYTE    tmBreakChar;
    BYTE    tmPitchAndFamily;
    BYTE    tmCharSet;
    SHORT   tmOverhang;
    SHORT   tmDigitizedAspectX;
    SHORT   tmDigitizedAspectY;
    DWORD   ntmFlags;
    WORD    ntmSizeEM;
    WORD    ntmCellHeight;
    WORD    ntmAvgWidth;
} NEWTEXTMETRIC16;
typedef NEWTEXTMETRIC16 UNALIGNED *PNEWTEXTMETRIC16;
typedef VPVOID VPNEWTEXTMETRIC16;

typedef struct _PANOSE16 {              /* pan16 */
    BYTE    bFamilyType;
    BYTE    bSerifStyle;
    BYTE    bWeight;
    BYTE    bProportion;
    BYTE    bContrast;
    BYTE    bStrokeVariation;
    BYTE    bArmStyle;
    BYTE    bLetterform;
    BYTE    bMidline;
    BYTE    bXHeight;
} PANOSE16;
typedef PANOSE16 UNALIGNED *PPANOSE16;

typedef struct _OUTLINETEXTMETRIC16 {   /* otm16 */
    WORD            otmSize;
    TEXTMETRIC16    otmTextMetrics;
    BYTE            otmFiller;
    PANOSE16        otmPanoseNumber;
    WORD            otmfsSelection;
    WORD            otmfsType;
    SHORT           otmsCharSlopeRise;
    SHORT           otmsCharSlopeRun;
    SHORT           otmItalicAngle;
    WORD            otmEMSquare;
    SHORT           otmAscent;
    SHORT           otmDescent;
    WORD            otmLineGap;
    WORD            otmsCapEmHeight;
    WORD            otmsXHeight;
    RECT16          otmrcFontBox;
    SHORT           otmMacAscent;
    SHORT           otmMacDescent;
    WORD            otmMacLineGap;
    WORD            otmusMinimumPPEM;
    POINT16         otmptSubscriptSize;
    POINT16         otmptSubscriptOffset;
    POINT16         otmptSuperscriptSize;
    POINT16         otmptSuperscriptOffset;
    WORD            otmsStrikeoutSize;
    SHORT           otmsStrikeoutPosition;
    SHORT           otmsUnderscorePosition;
    SHORT           otmsUnderscoreSize;
    WORD            otmpFamilyName;
    WORD            otmpFaceName;
    WORD            otmpStyleName;
    WORD            otmpFullName;
} OUTLINETEXTMETRIC16;
typedef OUTLINETEXTMETRIC16 UNALIGNED *POUTLINETEXTMETRIC16;
typedef VPVOID VPOUTLINETEXTMETRIC16;

typedef struct _HANDLETABLE16 {     /* ht16 */
    HAND16  objectHandle[1];
} HANDLETABLE16;
typedef HANDLETABLE16 UNALIGNED *PHANDLETABLE16;
typedef VPVOID VPHANDLETABLE16;

typedef struct _METARECORD16 {      /* mr16 */
    DWORD   rdSize;
    WORD    rdFunction;
    WORD    rdParm[1];
} METARECORD16;
typedef METARECORD16 UNALIGNED *PMETARECORD16;
typedef VPVOID VPMETARECORD16;

typedef struct _DEVMODE16 {     /* dm16 */
    CHAR    dmDeviceName[32];
    WORD    dmSpecVersion;
    WORD    dmDriverVersion;
    WORD    dmSize;
    WORD    dmDriverExtra;
    DWORD   dmFields;
    SHORT   dmOrientation;
    SHORT   dmPaperSize;
    SHORT   dmPaperLength;
    SHORT   dmPaperWidth;
    SHORT   dmScale;
    SHORT   dmCopies;
    SHORT   dmDefaultSource;
    SHORT   dmPrintQuality;
    SHORT   dmColor;
    SHORT   dmDuplex;
} DEVMODE16;
typedef DEVMODE16 UNALIGNED *PDEVMODE16;
typedef VPVOID VPDEVMODE16;

typedef struct _DEVMODE31 {     /* dm31 */
    CHAR    dmDeviceName[32];
    WORD    dmSpecVersion;
    WORD    dmDriverVersion;
    WORD    dmSize;
    WORD    dmDriverExtra;
    DWORD   dmFields;
    SHORT   dmOrientation;
    SHORT   dmPaperSize;
    SHORT   dmPaperLength;
    SHORT   dmPaperWidth;
    SHORT   dmScale;
    SHORT   dmCopies;
    SHORT   dmDefaultSource;
    SHORT   dmPrintQuality;
    SHORT   dmColor;
    SHORT   dmDuplex;
    SHORT   dmYResolution;
    SHORT   dmTTOption;
} DEVMODE31;
typedef DEVMODE31 UNALIGNED *PDEVMODE31;
typedef VPVOID VPDEVMODE31;

typedef struct _CREATESTRUCT16 {    /* cws16 */
    VPBYTE  vpCreateParams;
    HAND16  hInstance;
    HMENU16 hMenu;
    HWND16  hwndParent;
    SHORT   cy;
    SHORT   cx;
    SHORT   y;
    SHORT   x;
    DWORD   dwStyle;
    VPSZ    vpszWindow;
    VPSZ    vpszClass;
    DWORD   dwExStyle;
} CREATESTRUCT16;
typedef CREATESTRUCT16 UNALIGNED *PCREATESTRUCT16;
typedef VPVOID VPCREATESTRUCT16;

typedef struct _DRAWITEMSTRUCT16 {  /* dis16 */
    WORD    CtlType;
    WORD    CtlID;
    WORD    itemID;
    WORD    itemAction;
    WORD    itemState;
    HWND16  hwndItem;
    HDC16   hDC;
    RECT16  rcItem;
    DWORD   itemData;
} DRAWITEMSTRUCT16;
typedef DRAWITEMSTRUCT16 UNALIGNED *PDRAWITEMSTRUCT16;
typedef VPVOID VPDRAWITEMSTRUCT16;

typedef struct _MEASUREITEMSTRUCT16 {   /* mis16 */
    WORD    CtlType;
    WORD    CtlID;
    WORD    itemID;
    WORD    itemWidth;
    WORD    itemHeight;
    DWORD   itemData;
} MEASUREITEMSTRUCT16;
typedef MEASUREITEMSTRUCT16 UNALIGNED *PMEASUREITEMSTRUCT16;
typedef VPVOID VPMEASUREITEMSTRUCT16;

typedef struct _DELETEITEMSTRUCT16 {    /* des16 */
    WORD    CtlType;
    WORD    CtlID;
    WORD    itemID;
    HWND16  hwndItem;
    DWORD   itemData;
} DELETEITEMSTRUCT16;
typedef DELETEITEMSTRUCT16 UNALIGNED *PDELETEITEMSTRUCT16;
typedef VPVOID VPDELETEITEMSTRUCT16;

typedef struct _COMPAREITEMSTRUCT16 {   /* cis16 */
    WORD    CtlType;
    WORD    CtlID;
    HWND16  hwndItem;
    WORD    itemID1;
    DWORD   itemData1;
    WORD    itemID2;
    DWORD   itemData2;
} COMPAREITEMSTRUCT16;
typedef COMPAREITEMSTRUCT16 UNALIGNED *PCOMPAREITEMSTRUCT16;
typedef VPVOID VPCOMPAREITEMSTRUCT16;

typedef struct _MDICREATESTRUCT16 {     /* mcs16 */
    VPSZ    vpszClass;
    VPSZ    vpszTitle;
    HTASK16 hOwner;
    SHORT   x;
    SHORT   y;
    SHORT   cx;
    SHORT   cy;
    LONG    style;
    LONG    lParam;                     // app-defined stuff
} MDICREATESTRUCT16;
typedef MDICREATESTRUCT16 UNALIGNED *PMDICREATESTRUCT16;
typedef VPVOID VPMDICREATESTRUCT16;


typedef struct _WINDOWPOS16 {     /* wp16 */
    HAND16  hwnd;
    HAND16  hwndInsertAfter;
    SHORT   x;
    SHORT   y;
    SHORT   cx;
    SHORT   cy;
    WORD    flags;
} WINDOWPOS16;
typedef WINDOWPOS16 UNALIGNED *PWINDOWPOS16;
typedef VPVOID VPWINDOWPOS16;

typedef struct _NCCALCSIZE_PARAMS16 {    /* nccsz16 */
    RECT16        rgrc[3];
    WINDOWPOS16 UNALIGNED FAR *lppos;
} NCCALCSIZE_PARAMS16;
typedef NCCALCSIZE_PARAMS16 UNALIGNED *PNCCALCSIZE_PARAMS16;
typedef VPVOID VPNCCALCSIZE_PARAMS16;

/*
 * Used by Hook Procs.
 */

typedef struct _EVENTMSG16 {  /* evmsg16 */
    WORD    message;
    WORD    paramL;
    WORD    paramH;
    DWORD   time;
} EVENTMSG16;
typedef EVENTMSG16 UNALIGNED *PEVENTMSG16;
typedef VPVOID VPEVENTMSG16;

typedef struct _DEBUGHOOKINFO16 {   /*dbgi16 */
    HTASK16 hModuleHook;
    DWORD   reserved;
    DWORD   lParam;
    WORD    wParam;
    SHORT   code;
} DEBUGHOOKINFO16;
typedef DEBUGHOOKINFO16 UNALIGNED *PDEBUGHOOKINFO16;
typedef VPVOID VPDEBUGHOOKINFO16;

typedef struct _MOUSEHOOKSTRUCT16 { /* mhs16 */
    POINT16 pt;
    HWND16  hwnd;
    WORD    wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT16;
typedef MOUSEHOOKSTRUCT16 UNALIGNED *PMOUSEHOOKSTRUCT16;
typedef VPVOID VPMOUSEHOOKSTRUCT16;

typedef struct _CWPSTRUCT16 {    /* cwps16 */
    LONG    lParam;     //
    WORD    wParam;     //
    WORD    message;    //
    WORD    hwnd;       //
} CWPSTRUCT16;
typedef CWPSTRUCT16 UNALIGNED *PCWPSTRUCT16;
typedef VPVOID VPCWPSTRUCT16;

typedef struct _CBT_CREATEWND16 {  /* cbtcw16 */
    VPCREATESTRUCT16 vpcs;
    HWND16           hwndInsertAfter;
} CBT_CREATEWND16;
typedef CBT_CREATEWND16 UNALIGNED *PCBT_CREATEWND16;
typedef VPVOID VPCBT_CREATEWND16;

typedef struct _CBTACTIVATESTRUCT16 { /* cbtas16 */
    BOOL16    fMouse;
    HWND16    hWndActive;
} CBTACTIVATESTRUCT16;
typedef CBTACTIVATESTRUCT16 UNALIGNED *PCBTACTIVATESTRUCT16;
typedef VPVOID VPCBTACTIVATESTRUCT16;


/* 16-bit resource structures, and their pointers
 *
 * Note that some are the same as the 32-bit definition (eg, menus)
 */

typedef MENUITEMTEMPLATEHEADER     MENUITEMTEMPLATEHEADER16;
typedef MENUITEMTEMPLATE       MENUITEMTEMPLATE16;
typedef MENUITEMTEMPLATEHEADER16 *PMENUITEMTEMPLATEHEADER16;
typedef MENUITEMTEMPLATE16   *PMENUITEMTEMPLATE16;

typedef struct _DLGTEMPLATE16 {     /* dt16 */
    DWORD   style;          //
    BYTE    cdit;           // this is a WORD in WIN32
    WORD    x;              //
    WORD    y;              //
    WORD    cx;             //
    WORD    cy;             //
 // CHAR    szMenuName[];       // potential pad byte in WIN32
 // CHAR    szClassName[];      // potential pad byte in WIN32
 // CHAR    szCaptionText[];        // potential pad byte in WIN32
} DLGTEMPLATE16;
typedef DLGTEMPLATE16 UNALIGNED *PDLGTEMPLATE16;
typedef VPVOID VPDLGTEMPLATE16;

typedef struct _FONTINFO16 {        /* fi16 */
    SHORT   cPoints;            // present if DS_SETFONT in dt16.style
 // CHAR    szTypeFace[];       // potential pad byte in WIN32
} FONTINFO16;
typedef FONTINFO16 UNALIGNED *PFONTINFO16;
typedef VPVOID VPFONTINFO16;

typedef struct _DLGITEMTEMPLATE16 { /* dit16 */
    WORD    x;              // structure dword-aligned in WIN32
    WORD    y;              //
    WORD    cx;             //
    WORD    cy;             //
    WORD    id;             //
    DWORD   style;          // this was moved to the top in WIN32
 // CHAR    szClass[];          // potential pad byte in WIN32
 // CHAR    szText[];           // potential pad byte in WIN32
 // BYTE    cbExtra;            //
 // BYTE    abExtra[];          //
} DLGITEMTEMPLATE16;
typedef DLGITEMTEMPLATE16 UNALIGNED *PDLGITEMTEMPLATE16;
typedef VPVOID VPDLGITEMTEMPLATE16;

typedef struct _RESDIRHEADER16 {    /* hdir16 */
    WORD    reserved;           //
    WORD    rt;             //
    WORD    cResources;         // pad word in WIN32 (for size == 8)
} RESDIRHEADER16;
typedef RESDIRHEADER16 UNALIGNED *PRESDIRHEADER16;
typedef VPVOID VPRESDIRHEADER16;

typedef struct _ICONDIR16 {     /* idir16 */
    BYTE    Width;          // 16, 32, 64
    BYTE    Height;         // 16, 32, 64
    BYTE    ColorCount;         // 2, 8, 16
    BYTE    reserved;           //
} ICONDIR16;
typedef ICONDIR16 UNALIGNED *PICONDIR16;
typedef VPVOID VPICONDIR16;

typedef struct _CURSORDIR16 {       /* cdir16 */
    WORD    Width;          //
    WORD    Height;         //
} CURSORDIR16;
typedef CURSORDIR16 UNALIGNED *PCURSORDIR16;
typedef VPVOID VPCURSORDIR16;

/* XLATOFF */
typedef struct _RESDIR16 {      /* rdir16 */
    union {             //
    ICONDIR16   Icon;       //
    CURSORDIR16 Cursor;     //
    } ResInfo;              //
    WORD    Planes;         //
    WORD    BitCount;           //
    DWORD   BytesInRes;         //
    WORD    idIcon;         // pad word in WIN32 (for size == 16)
} RESDIR16;
typedef RESDIR16 UNALIGNED *PRESDIR16;
typedef VPVOID VPRESDIR16;

typedef struct _COPYDATASTRUCT16 {
    DWORD dwData;
    DWORD cbData;
    PVOID lpData;
} COPYDATASTRUCT16;
typedef COPYDATASTRUCT16 UNALIGNED *PCOPYDATASTRUCT16;
typedef VPVOID VPCOPYDATASTRUCT16;

typedef struct _DROPSTRUCT16 {
    HWND16  hwndSource;
    HWND16  hwndSink;
    WORD    wFmt;
    DWORD   dwData;
    POINT16 ptDrop;
    DWORD   dwControlData;
} DROPSTRUCT16;
typedef DROPSTRUCT16 UNALIGNED *PDROPSTRUCT16;
typedef VPVOID VPDROPSTRUCT16;

typedef struct _DROPFILESTRUCT16 {
    WORD pFiles;
    SHORT x;
    SHORT y;
    BOOL16 fNC;
} DROPFILESTRUCT16;
typedef DROPFILESTRUCT16 UNALIGNED *PDROPFILESTRUCT16;
typedef VPVOID VPDROPFILESTRUCT16;


typedef BITMAPINFOHEADER ICONRESOURCE;
typedef ICONRESOURCE *PICONRESOURCE;
typedef BITMAPINFOHEADER16 ICONRESOURCE16;
typedef ICONRESOURCE16 UNALIGNED *PICONRESOURCE16;
typedef VPVOID VPICONRESOURCE16;

typedef struct _CURSORRESOURCE {    /* cres */
    WORD xHotspot;
    WORD yHotspot;
    BITMAPINFOHEADER bmih;
} CURSORRESOURCE;
typedef CURSORRESOURCE UNALIGNED *PCURSORRESOURCE;

typedef CURSORRESOURCE CURSORRESOURCE16;
typedef CURSORRESOURCE16 UNALIGNED *PCURSORRESOURCE16;
typedef VPVOID VPCURSORRESOURCE16;

// This describes the header of the old 2.x cursor/icon resource format;
// the header should be followed by an AND mask and then an XOR mask, where:
//
//      Bit value   Bit value   Bit value   Bit value
//  AND Mask        0       0       1       1
//  XOR Mask        0       1       0       1
//  ---------------------------------------------------------
//  Result  Black       White   Transparent Inverted
//
// Note that we wouldn't have to worry about this old resource format if apps
// like WinWord (which apparently weren't fully converted to 3.x) didn't use it! -JTP

typedef struct _OLDCURSORICONRESOURCE16 { /* oci16 */
    BYTE    bFigure;            // 1: cursor, 2: bitmap, 3: icon
    BYTE    bIndependent;       // 0: device-dependent, 1: independent
    SHORT   xHotspot;           //
    SHORT   yHotspot;           //
    SHORT   cx;             // x-extent
    SHORT   cy;             // y-extent
    SHORT   cbWidth;            // bytes per row (rows are word-aligned)
    SHORT   clr;            // # color planes (should always be 0)
} OLDCURSORICONRESOURCE16;
typedef OLDCURSORICONRESOURCE16 UNALIGNED *POLDCURSORICONRESOURCE16;
typedef VPVOID VPOLDCURSORICONRESOURCE16;
/* XLATON */

/* XLATOFF */
#pragma pack()
/* XLATON */


/* Undocumented(?) window messages
 */
#define WM_SETVISIBLE       0x0009
#define WM_ALTTABACTIVE     0x0029
#define WM_ISACTIVEICON     0x0035
#define WM_QUERYPARKICON    0x0036
#define WM_SYNCPAINT        0x0088
#define WM_SYSTIMER     0x0118
#define WM_LBTRACKPOINT     0x0131
#define WM_ENTERMENULOOP    0x0211
#define WM_EXITMENULOOP     0x0212
#define WM_NEXTMENU     0x0213
#define WM_DROPOBJECT       0x022A
#define WM_QUERYDROPOBJECT  0x022B
#define WM_BEGINDRAG        0x022C
#define WM_DRAGLOOP     0x022D
#define WM_DRAGSELECT       0x022E
#define WM_DRAGMOVE     0x022F
#define WM_ENTERSIZEMOVE    0x0231
#define WM_EXITSIZEMOVE     0x0232


/* Old window messages (changed from Win 3.x)
 */
#ifndef WM_USER
#define WM_USER 0x0400
#endif

#define OLDEM_GETSEL            (WM_USER+0)
#define OLDEM_SETSEL            (WM_USER+1)
#define OLDEM_GETRECT           (WM_USER+2)
#define OLDEM_SETRECT           (WM_USER+3)
#define OLDEM_SETRECTNP         (WM_USER+4)
#define OLDEM_SCROLL            (WM_USER+5)
#define OLDEM_LINESCROLL        (WM_USER+6)
#define OLDEM_GETMODIFY         (WM_USER+8)
#define OLDEM_SETMODIFY         (WM_USER+9)
#define OLDEM_GETLINECOUNT      (WM_USER+10)
#define OLDEM_LINEINDEX         (WM_USER+11)
#define OLDEM_SETHANDLE         (WM_USER+12)
#define OLDEM_GETHANDLE         (WM_USER+13)
#define OLDEM_GETTHUMB          (WM_USER+14)
#define OLDEM_LINELENGTH        (WM_USER+17)
#define OLDEM_REPLACESEL        (WM_USER+18)
#define OLDEM_SETFONT           (WM_USER+19)
#define OLDEM_GETLINE           (WM_USER+20)
#define OLDEM_LIMITTEXT         (WM_USER+21)
#define OLDEM_CANUNDO           (WM_USER+22)
#define OLDEM_UNDO          (WM_USER+23)
#define OLDEM_FMTLINES          (WM_USER+24)
#define OLDEM_LINEFROMCHAR      (WM_USER+25)
#define OLDEM_SETWORDBREAK      (WM_USER+26)
#define OLDEM_SETTABSTOPS       (WM_USER+27)
#define OLDEM_SETPASSWORDCHAR       (WM_USER+28)
#define OLDEM_EMPTYUNDOBUFFER       (WM_USER+29)
#ifndef WIN31
#define OLDEM_MSGMAX            (WM_USER+30)
#else
#define OLDEM_GETFIRSTVISIBLELINE (WM_USER+30)
#define OLDEM_SETREADONLY       (WM_USER+31)
#define OLDEM_SETWORDBREAKPROC  (WM_USER+32)
#define OLDEM_GETWORDBREAKPROC  (WM_USER+33)
#define OLDEM_GETPASSWORDCHAR   (WM_USER+34)
#define OLDEM_MSGMAX            (WM_USER+35)
#endif

#define OLDBM_GETCHECK          (WM_USER+0)
#define OLDBM_SETCHECK          (WM_USER+1)
#define OLDBM_GETSTATE          (WM_USER+2)
#define OLDBM_SETSTATE          (WM_USER+3)
#define OLDBM_SETSTYLE          (WM_USER+4)

#define OLDCB_GETEDITSEL        (WM_USER+0)
#define OLDCB_LIMITTEXT         (WM_USER+1)
#define OLDCB_SETEDITSEL        (WM_USER+2)
#define OLDCB_ADDSTRING         (WM_USER+3)
#define OLDCB_DELETESTRING      (WM_USER+4)
#define OLDCB_DIR           (WM_USER+5)
#define OLDCB_GETCOUNT          (WM_USER+6)
#define OLDCB_GETCURSEL         (WM_USER+7)
#define OLDCB_GETLBTEXT         (WM_USER+8)
#define OLDCB_GETLBTEXTLEN      (WM_USER+9)
#define OLDCB_INSERTSTRING      (WM_USER+10)
#define OLDCB_RESETCONTENT      (WM_USER+11)
#define OLDCB_FINDSTRING        (WM_USER+12)
#define OLDCB_SELECTSTRING      (WM_USER+13)
#define OLDCB_SETCURSEL         (WM_USER+14)
#define OLDCB_SHOWDROPDOWN      (WM_USER+15)
#define OLDCB_GETITEMDATA       (WM_USER+16)
#define OLDCB_SETITEMDATA       (WM_USER+17)
#define OLDCB_GETDROPPEDCONTROLRECT (WM_USER+18)
#ifndef WIN31
#define OLDCB_MSGMAX            (WM_USER+19)
#else
#define OLDCB_SETITEMHEIGHT     (WM_USER+19)
#define OLDCB_GETITEMHEIGHT     (WM_USER+20)
#define OLDCB_SETEXTENDEDUI     (WM_USER+21)
#define OLDCB_GETEXTENDEDUI     (WM_USER+22)
#define OLDCB_GETDROPPEDSTATE   (WM_USER+23)
#define OLDCB_FINDSTRINGEXACT   (WM_USER+24)
#define OLDCB_MSGMAX            (WM_USER+25)    /* ;Internal */
#define OLDCBEC_SETCOMBOFOCUS   (WM_USER+26)    /* ;Internal */
#define OLDCBEC_KILLCOMBOFOCUS  (WM_USER+27)    /* ;Internal */
#endif

#define OLDLB_ADDSTRING         (WM_USER+1)
#define OLDLB_INSERTSTRING      (WM_USER+2)
#define OLDLB_DELETESTRING      (WM_USER+3)
#define OLDLB_RESETCONTENT      (WM_USER+5)
#define OLDLB_SETSEL            (WM_USER+6)
#define OLDLB_SETCURSEL         (WM_USER+7)
#define OLDLB_GETSEL            (WM_USER+8)
#define OLDLB_GETCURSEL         (WM_USER+9)
#define OLDLB_GETTEXT           (WM_USER+10)
#define OLDLB_GETTEXTLEN        (WM_USER+11)
#define OLDLB_GETCOUNT          (WM_USER+12)
#define OLDLB_SELECTSTRING      (WM_USER+13)
#define OLDLB_DIR           (WM_USER+14)
#define OLDLB_GETTOPINDEX       (WM_USER+15)
#define OLDLB_FINDSTRING        (WM_USER+16)
#define OLDLB_GETSELCOUNT       (WM_USER+17)
#define OLDLB_GETSELITEMS       (WM_USER+18)
#define OLDLB_SETTABSTOPS       (WM_USER+19)
#define OLDLB_GETHORIZONTALEXTENT   (WM_USER+20)
#define OLDLB_SETHORIZONTALEXTENT   (WM_USER+21)
#define OLDLB_SETCOLUMNWIDTH        (WM_USER+22)
#define OLDLB_ADDFILE           (WM_USER+23)    /* ;Internal */
#define OLDLB_SETTOPINDEX       (WM_USER+24)
#define OLDLB_GETITEMRECT       (WM_USER+25)
#define OLDLB_GETITEMDATA       (WM_USER+26)
#define OLDLB_SETITEMDATA       (WM_USER+27)
#define OLDLB_SELITEMRANGE      (WM_USER+28)
#define OLDLB_SETANCHORINDEX        (WM_USER+29)    /* ;Internal */
#define OLDLB_GETANCHORINDEX        (WM_USER+30)    /* ;Internal */
#ifndef WIN31
#define OLDLB_MSGMAX            (WM_USER+33)
#else
#define OLDLB_SETCARETINDEX     (WM_USER+31)
#define OLDLB_GETCARETINDEX     (WM_USER+32)
#define OLDLB_SETITEMHEIGHT     (WM_USER+33)
#define OLDLB_GETITEMHEIGHT     (WM_USER+34)
#define OLDLB_FINDSTRINGEXACT   (WM_USER+35)
#define OLDLBCB_CARETON         (WM_USER+36)     /* ;Internal */
#define OLDLBCB_CARETOFF        (WM_USER+37)     /* ;Internal */
#define OLDLB_MSGMAX            (WM_USER+38)     /* ;Internal */
#endif

#define OLDSBM_SETPOS           (WM_USER+0)
#define OLDSBM_GETPOS           (WM_USER+1)
#define OLDSBM_SETRANGE         (WM_USER+2)
#define OLDSBM_GETRANGE         (WM_USER+3)
#define OLDSBM_ENABLEARROWS     (WM_USER+4)

/* WOW Return IDs - Ordering must match wow16cal.asm table
 */
#define RET_RETURN       0  // return to app

#define RET_DEBUGRETURN      1  // return to app after breakpoint

#define RET_DEBUG        2  // execute breakpoint, return to WOW

#define RET_WNDPROC      3  // IN:all
                // OUT:DX:AX=wndproc return code

#define RET_ENUMFONTPROC     4  // IN:all
                // OUT:DX:AX=wndproc return code

#define RET_ENUMWINDOWPROC   5  // IN:all
                // OUT:DX:AX=wndproc return code

#define RET_LOCALALLOC       6  // IN:wParam=wFlags, lParam=wBytes
                // OUT:AX=hMem (zero if error)

#define RET_LOCALREALLOC     7  // IN:wMsg=hMem, wParam=wFlags, lParam=wBytes
                // OUT:AX=hMem (zero if error)

#define RET_LOCALLOCK        8  // IN:wParam=hMem
                // OUT:DX:AX=address (zero if error), CX=size

#define RET_LOCALUNLOCK      9  // IN:wParam=hMem
                // OUT:AX=TRUE (FALSE if error)

#define RET_LOCALSIZE        10 // IN:wParam=hMem
                // OUT:AX=size (zero if error)

#define RET_LOCALFREE        11 // IN:wParam=hMem
                // OUT:AX=TRUE (FALSE if error)

#define RET_GLOBALALLOCLOCK  12 // IN:wParam=wFlags, lParam=dwBytes
                // OUT:DX:AX=address (zero if error), BX=hMem

#define RET_GLOBALLOCK       13 // IN:wParam=hMem
                // OUT:DX:AX=address (zero if error), CX=size

#define RET_GLOBALUNLOCK     14 // IN:wParam=hMem
                // OUT:AX=TRUE (FALSE if error)

#define RET_GLOBALUNLOCKFREE 15 // IN:lParam=address
                // OUT:AX=TRUE (FALSE if error)

#define RET_FINDRESOURCE     16 // IN:wParam=hTask, lParam=vpName, hwnd/wMsg=vpType
                // OUT:AX=hResInfo (zero if not found)

#define RET_LOADRESOURCE     17 // IN:wParam=hTask, lParam=hResInfo
                // OUT:AX=hResData

#define RET_FREERESOURCE     18 // IN:wParam=hResData
                // OUT:AX=TRUE (zero if failed)

#define RET_LOCKRESOURCE     19 // IN:wParam=hResData
                // OUT:DX:AX=address (zero if error), CX=size

#define RET_UNLOCKRESOURCE   20 // IN:wParam=hResData
                // OUT:AX=TRUE (zero if resource still locked)

#define RET_SIZEOFRESOURCE   21 // IN:wParam=hTask, lParam=hResInfo
                // OUT:DX:AX=size (zero if error)

#define RET_LOCKSEGMENT      22 // IN:wParam=wSeg
                // OUT:AX=TRUE (FALSE if error)

#define RET_UNLOCKSEGMENT    23 // IN:wParam=wSeg
                // OUT:AX=TRUE (zero if segment still locked)

#define RET_ENUMMETAFILEPROC 24 // IN:all
                                // OUT:DX:AX=wndproc return cod

#define RET_TASKSTARTED      25 // None

#define RET_HOOKPROC         26 // IN:all
                                // OUT:DX:AX=hookproc return code

#define RET_SUBCLASSPROC     27 // IN:None
                // OUT: DX:AX=thunkproc return code
#define RET_LINEDDAPROC      28

#define RET_GRAYSTRINGPROC   29

#define RET_FORCETASKEXIT    30 // IN:None
            // OUT: Does not return

#define RET_SETCURDIR        31 // IN:Current Dir
            // OUT: ax
#define RET_ENUMOBJPROC     32  // IN:all
        // OUT:DX:AX=wndproc return code

#define RET_SETCURSORICONFLAG        33 // IN: none

#define RET_SETABORTPROC    34

#define RET_ENUMPROPSPROC   35

#define RET_FORCESEGMENTFAULT 36 //  Make a segment present

#define RET_LSTRCMP          37 // for user32 listbox code

                                //  38 FREE
                                //  39 FREE
                                //  40 FREE
                                //  41 FREE

#define RET_GETEXEPTR        42 // Call KRNL286:GetExePtr

                                //  43 FREE

#define RET_FORCETASKFAULT   44 // Force a Fault
#define RET_GETEXPWINVER     45 // Call KRNL286:GetExpWinVer
#define RET_GETCURDIR        46 //

#define RET_GETDOSPDB        47 // IN:  nothing
                                // OUT: DX:AX = DOS Win_PDB
#define RET_GETDOSSFT        48 // IN:  nothing
                                // OUT: DX:AX = DOS SFT (pFileTable)
#define RET_FOREGROUNDIDLE   49 // IN:  nothing
                                // OUT: NOTHING
#define RET_WINSOCKBLOCKHOOK 50 // IN:  nothing
                                // OUT: DX:AX = BOOL ret value
#define RET_WOWDDEFREEHANDLE 51

#define RET_CHANGESELECTOR   52 // IN: wParam = Segment

#define RET_GETMODULEFILENAME 53 // IN: wParam = hInst, lParam = 16:16 Buffer
                                 //     wMsg = cbBuffer
                                 // OUT: AX = length of returned modulename

#define RET_SETWORDBREAKPROC 54 //

#define RET_WINEXEC          55

#define RET_WOWCALLBACK16    56 // Used by public WOWCallback16 routine

#define RET_GETDIBSIZE       57

#define RET_GETDIBFLAGS      58

#define RET_SETDIBSEL        59

#define RET_FREEDIBSEL       60

#ifdef FE_SB
#define RET_SETFNOTEPAD      61 // sync GetModuleUsage16 API for Lotus FLW
#define RET_MAX              61
#else // !FE_SB
#define RET_MAX              60
#endif // !FE_SB




/* Module IDs
 *
 * Module IDs are OR'ed with API IDs to produce Call IDs
 */
#define MOD_MASK        0xF000
#define FUN_MASK        0x0FFF

#define MOD_KERNEL   0x0000
#define MOD_DKERNEL  0X0000   // for parameter validation layer
#define MOD_USER     0x1000   //
#define MOD_DUSER    0x1000   // for parameter validation layer
#define MOD_GDI      0x2000   //
#define MOD_DGDI     0x2000   // for parameter validation layer
#define MOD_KEYBOARD 0x3000
#define MOD_SOUND    0x4000
#define MOD_SHELL    0x5000   // SHELL APIs
#define MOD_WINSOCK  0x6000
#define MOD_TOOLHELP 0x7000
#define MOD_MMEDIA   0x8000
#define MOD_COMMDLG  0x9000
#ifdef FE_SB
#define MOD_WINNLS   0xA000
#define MOD_WIFEMAN  0xB000
#define MOD_LAST     0xC000   // Add New Module ID before this one
#else // !FE_SB
#define MOD_LAST     0xA000   // Add New Module ID before this one
#endif // !FE_SB


/* Special Function IDs
 *
 * This is used by WIN16 whenever we are returning from a window proc;
 * see the various include files (wowkrn.h, wowgdi.h, etc) for all the other
 * function IDs.
 */
#define FUN_RETURN      0

/*
 * hiword of wcallID in VDMFRAME -
 */

#define HI_WCALLID     0x0000

/* Macros for WOW16 DLLs
 *
 * Note for GDIThunk args is the metafile function number
 * and val denotes if function has a DC
 *
 */

/* ASM
Thunk       macro   mod,func,callfirst,args,val,emptybuf
    ifidni  <args>,<abs>
    public func
    ifb    <val>
        func = 0
    else
        func = val
    endif
    else
    externA  __MOD_KERNEL
    externA  __MOD_DKERNEL
    externA  __MOD_USER
    externA  __MOD_DUSER
    externA  __MOD_GDI
    externA  __MOD_DGDI
    externA  __MOD_KEYBOARD
    externA  __MOD_SOUND
    externA  __MOD_SHELL
    externA  __MOD_WINSOCK
    externA  __MOD_TOOLHELP
    externA  __MOD_MMEDIA
    externA  __MOD_COMMDLG
ifdef FE_SB
    externA  __MOD_WINNLS
    externA  __MOD_WIFEMAN
endif ; FE_SB



    ifidni <mod>,<USER>
        cProc I&func,<PUBLIC,FAR,PASCAL,NODATA,WIN>

        cBegin <nogen>
    else
        ifidni <mod>,<GDI>
        cProc I&func,<PUBLIC,FAR,PASCAL,NODATA,WIN>

        cBegin <nogen>
        else
        ifidni <mod>,<KERNEL>
            cProc I&func,<PUBLIC,FAR,PASCAL,NODATA,WIN>

            cBegin <nogen>
        else
            cProc func,<PUBLIC,FAR,PASCAL,NODATA,WIN>

            cBegin <nogen>
        endif
        endif
    endif

    ; Make the passed in buffer into an empty string by writing null
    ; to the first position. Win 3.1 IGetWindowText does this, and
    ; WinFax Pro depends on this behaviour.
    ifnb   <emptybuf>
        push    bp
        mov     bp, sp
        mov     bx, [bp+8]
        mov     es, [bp+0Ah]
        mov     byte ptr es:[bx], 0
        pop     bp
    endif

    ifdifi <callfirst>,<0>
    call    callfirst
    endif

    ifnb   <args>
        push    args
    else
        ifdef func&16
        push    size func&16
        else
        if1
            %out     Warning: assuming null arg frame for &mod:&func
        endif
        push  0
        endif
    endif
        t_&func:


            push    word ptr HI_WCALLID
            push    __MOD_&mod + FUN_&func
            call   WOW16Call

        ; assert that this is constant size code. 5bytes for 'call wow16call'
        ; and 3bytes each for the 'push ...'. We use this info in wow32
        ; to patchcodewithlpfnw32.

        .erre (($ - t_&func) EQ (05h + 03h + 03h))

    cEnd <nogen>
    endif
endm

KernelThunk macro func,args,val
    Thunk   KERNEL,func,0,args,val
endm

DKernelThunk macro func,args,val
    Thunk   DKERNEL,func,0,args,val
endm

PKernelThunk macro func,callfirst,args,val
    Thunk   KERNEL,func,callfirst,args,val
endm

UserThunk   macro func,args,val
    Thunk   USER,func,0,args,val
endm

DUserThunk  macro func,args,val
    Thunk   DUSER,func,0,args,val
endm

PUserThunk  macro func,callfirst,args,val
    Thunk   USER,func,callfirst,args,val
endm

PDUserThunk  macro func,callfirst,args,val
    Thunk   DUSER,func,callfirst,args,val
endm

EUserThunk  macro func,args,val
    Thunk   USER,func,0,args,val,0
endm

GDIThunk    macro func,args,val
    Thunk   GDI,func,0,args,val
endm

DGDIThunk   macro func,args,val
    Thunk   DGDI,func,0,args,val
endm

PGDIThunk   macro func,callfirst,args,val
    Thunk   GDI,func,callfirst,args,val
endm

KbdThunk    macro func,args,val
    Thunk   KEYBOARD,func,0,args,val
endm

SoundThunk  macro func,args,val
    Thunk   SOUND,func,0,args,val
endm

SHELLThunk  macro func,args,val
    Thunk   SHELL,func,0,args,val
endm

MMediaThunk macro func,args,val
    Thunk   MMEDIA,func,0,args,val
endm

WinsockThunk macro func,args,val
    Thunk   WINSOCK,func,0,args,val
endm

ToolHelpThunk macro func,args,val
    Thunk   TOOLHELP,func,0,args,val
endm

CommdlgThunk macro func,args,val
    Thunk   COMMDLG,func,SetWowCommDlg,args,val
endm

ifdef FE_SB
WINNLSThunk macro func,args,val
    Thunk   WINNLS,func,0,args,val
endm

WifeManThunk macro func,args,val
    Thunk   WIFEMAN,func,0,args,val
endm
endif ; FE_SB

 */
