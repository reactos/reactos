//****************************************************************************
// WOW fax support. Common file. Conditionally shared between wow16, wow32,
// wowfax and wowfaxui
//
// 16bit structures are share between wow16 and wow32 only. Though they get
// included while compiling 32bit printdrivers wowfax and wowfaxui, yet they
// are incorrect and inaccessible from those dlls.
//
//
// History:
//    02-jan-95   nandurir   created.
//    01-feb-95   reedb      Clean-up, support printer install and bug fixes.
//
//****************************************************************************

//***************************************************************************
// WM_DDRV_ defines  - Common to wowexec,wow32,wowfax,wowfaxui. If you add
//                     a message, be sure to add it to the debug strings also.
//***************************************************************************

#define WM_DDRV_FIRST              (WM_USER+0x100+1) // begin DDRV range
#define WM_DDRV_LOAD               (WM_USER+0x100+1)
#define WM_DDRV_ENABLE             (WM_USER+0x100+2)
#define WM_DDRV_STARTDOC           (WM_USER+0x100+3)
#define WM_DDRV_PRINTPAGE          (WM_USER+0x100+4)
#define WM_DDRV_ESCAPE             (WM_USER+0x100+5)
#define WM_DDRV_DISABLE            (WM_USER+0x100+6)
#define WM_DDRV_INITFAXINFO16      (WM_USER+0x100+7)
#define WM_DDRV_ENDDOC             (WM_USER+0x100+8)
#define WM_DDRV_SUBCLASS           (WM_USER+0x100+9)
#define WM_DDRV_EXTDMODE           (WM_USER+0x100+10)
#define WM_DDRV_DEVCAPS            (WM_USER+0x100+11)
#define WM_DDRV_FREEFAXINFO16      (WM_USER+0x100+12)
#define WM_DDRV_UNLOAD             (WM_USER+0x100+20)
#define WM_DDRV_LAST               (WM_USER+0x100+20) // end DDRV range

#define CCHDOCNAME 128

#ifdef DEBUG
#ifdef DEFINE_DDRV_DEBUG_STRINGS
char *szWmDdrvDebugStrings[] =
{
    "WM_DDRV_LOAD",
    "WM_DDRV_ENABLE",
    "WM_DDRV_STARTDOC",
    "WM_DDRV_PRINTPAGE",
    "WM_DDRV_ESCAPE",
    "WM_DDRV_DISABLE",
    "WM_DDRV_INITFAXINFO16",
    "WM_DDRV_ENDDOC",
    "WM_DDRV_SUBCLASS",
    "WM_DDRV_EXTDMODE",
    "WM_DDRV_DEVCAPS",
    "WM_DDRV_FREEFAXINFO16",
    "UNKNOWN MESSAGE",
    "UNKNOWN MESSAGE",
    "UNKNOWN MESSAGE",
    "UNKNOWN MESSAGE",
    "UNKNOWN MESSAGE",
    "UNKNOWN MESSAGE",
    "UNKNOWN MESSAGE",
    "WM_DDRV_UNLOAD"
};
#endif
#endif // #define(DEBUG)

// WOWFAX component file names. Unicode and ANSII
#define WOWFAX_DLL_NAME L"WOWFAX.DLL"
#define WOWFAXUI_DLL_NAME L"WOWFAXUI.DLL"
#define WOWFAX_DLL_NAME_A "WOWFAX.DLL"
#define WOWFAXUI_DLL_NAME_A "WOWFAXUI.DLL"

//***************************************************************************
//     wow16 ie  wowfax.c defines WOWFAX16 to include 16bit print.h etc
//
//***************************************************************************

#if  defined(_WOWFAX16_)

#include "..\..\..\private\mvdm\wow16\inc\print.h"
#include "..\..\..\private\mvdm\wow16\inc\gdidefs.inc"

#define LPTSTR              LPSTR
#define TEXT(x)             x
#define SRCCOPY             0x00CC0020L

//
// the following UNALIGNED definition is required because this file
// in included in wow16\test\shell\wowfax.h
//

#ifndef UNALIGNED

#if defined(MIPS) || defined(_ALPHA_) // winnt
#define UNALIGNED __unaligned         // winnt
#else                                 // winnt
#define UNALIGNED                     // winnt
#endif                                // winnt

#endif


#endif       // defined(wowfax16)

//***************************************************************************
// WOWFAXINFO16  common to wow16 and wow32
//      Struct cannot be accessed from 32bit wowfax/wowfaxui dlls
//***************************************************************************

#define WFINFO16_ENABLED           0x01

/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _WOWFAXINFO16 {  /* winfo16 */
    WORD      hmem;
    WORD      flState;
    WORD      hInst;

    WORD (FAR PASCAL *lpEnable)(LPVOID,short,LPSTR,LPSTR,LPVOID);
    VOID (FAR PASCAL *lpDisable)(LPVOID);
    int (FAR PASCAL *lpControl)(LPVOID, short, LPVOID, LPVOID);
    BOOL (FAR PASCAL *lpBitblt)(LPVOID,WORD,WORD,LPVOID,
                              WORD,WORD,WORD,WORD,long,LPVOID,LPVOID);
    WORD (FAR PASCAL *lpExtDMode)(HWND, HANDLE, LPVOID, LPSTR, LPSTR,
                                               LPVOID, LPSTR , WORD);
    DWORD (FAR PASCAL *lpDevCaps)(LPSTR, LPSTR, WORD, LPSTR, LPVOID);

    WORD        hmemdevice;
    DWORD       cData;
    WORD        hwndui;
    DWORD       retvalue;
    WORD        wCmd;

    // The following pointers provide offsets into the mapped file
    // section used for inter process communication. They point to
    // objects which have a variable length.
    LPVOID      lpDevice;
    LPVOID      lpDriverName;
    LPVOID      lpPortName;
    LPVOID      lpIn;
    LPVOID      lpOut;

    // Since we have a max length (CCHDEVICENAME) we'll pass
    // the printer/device name in this fixed length buffer.
    char        szDeviceName[CCHDEVICENAME+1];
    char        szDocName[CCHDOCNAME+1];

} WOWFAXINFO16;

typedef WOWFAXINFO16 UNALIGNED FAR *LPWOWFAXINFO16;

//***************************************************************************
// GDIINFO16  common to wow16 and wow32
//        - this structure copied from wow16\inc\gdidefs.inc
//        - PTTYPE has been replaced with POINT
//
//      Struct cannot be accessed from 32bit wowfax/wowfaxui dlls. the
//      definition itself will be incorrect.
//
//***************************************************************************

#ifndef _DEF_WOW32_
#define POINT16             POINT
#endif

typedef struct _GDIINFO16{  /* gdii16 */
    short int dpVersion;
    short int dpTechnology;
    short int dpHorzSize;
    short int dpVertSize;
    short int dpHorzRes;
    short int dpVertRes;
    short int dpBitsPixel;
    short int dpPlanes;
    short int dpNumBrushes;
    short int dpNumPens;
    short int futureuse;
    short int dpNumFonts;
    short int dpNumColors;
    short int dpDEVICEsize;
    unsigned short int dpCurves;
    unsigned short int dpLines;
    unsigned short int dpPolygonals;
    unsigned short int dpText;
    unsigned short int dpClip;
    unsigned short int dpRaster;
    short int dpAspectX;
    short int dpAspectY;
    short int dpAspectXY;
    short int dpStyleLen;
    POINT16  dpMLoWin;
    POINT16  dpMLoVpt;
    POINT16  dpMHiWin;
    POINT16  dpMHiVpt;
    POINT16  dpELoWin;
    POINT16  dpELoVpt;
    POINT16  dpEHiWin;
    POINT16  dpEHiVpt;
    POINT16  dpTwpWin;
    POINT16  dpTwpVpt;
    short int dpLogPixelsX;
    short int dpLogPixelsY;
    short int dpDCManage;
    unsigned short int dpCaps1;
    short int futureuse4;
    short int futureuse5;
    short int futureuse6;
    short int futureuse7;
    WORD dpNumPalReg;
    WORD dpPalReserved;
    WORD dpColorRes;
} GDIINFO16;

typedef GDIINFO16 UNALIGNED FAR *LPGDIINFO16;


/* XLATOFF */
#pragma pack()
/* XLATON */

#ifndef _WOWFAX16_

//***************************************************************************
// WOWFAXINFO  - common to wow32,wowfax,wowfaxui. This defines the header of
//      shared memory section.
//
//***************************************************************************

typedef struct _WOWFAXINFO {   /* faxi */
    HWND    hwnd;
    DWORD   tid;
    WNDPROC proc16;
    LPBYTE  lpinfo16;

    UINT    msg;
    WPARAM  hdc;

    WORD    wCmd;
    DWORD   cData;
    HWND    hwndui;
    DWORD   retvalue;
    DWORD   status;

    // The following pointers provide offsets into the mapped file
    // section used for inter process communication. They point to
    // objects which have a variable length.
    LPVOID      lpDevice;
    LPVOID      lpDriverName;
    LPVOID      lpPortName;
    LPVOID      lpIn;
    LPVOID      lpOut;

    // Since we have a max length (CCHDEVICENAME) we'll pass
    // the printer/device name in this fixed length buffer.
    WCHAR       szDeviceName[CCHDEVICENAME+1];

    // EasyFax Ver2.0 support for JAPAN. 
    // Also needed for Procomm+ 3 cover sheets. Bug #305665
    WCHAR       szDocName[CCHDOCNAME+1];

    UINT    bmPixPerByte;
    UINT    bmWidthBytes;
    UINT    bmHeight;
    LPBYTE  lpbits;

} WOWFAXINFO, FAR *LPWOWFAXINFO;

#endif // _WOWFAX16_

#define WOWFAX_CLASS      TEXT("WOWFaxClass")

VOID GetFaxDataMapName(DWORD idMap, LPTSTR lpT);

//***************************************************************************
// Common functions for wow32, wowfax and wowfaxui.
//***************************************************************************

#ifdef WOWFAX_INC_COMMON_CODE

#define WOWFAX_MAPPREFIX  TEXT("wfm")
#define WOWFAX_HEXDIGITS  TEXT("0123456789abcdef")

//***************************************************************************
// GetFaxDataMapName - given idMap, generates the sharedmem Map Name.
//       A process can access the relevant data by opening the file
//       identified by idMap
//***************************************************************************

VOID GetFaxDataMapName(DWORD idMap, LPTSTR lpT)
{
    int i;
    int cb = lstrlen(WOWFAX_MAPPREFIX);
    LPBYTE lpid = (LPBYTE)&idMap;
    LPTSTR lphexT = WOWFAX_HEXDIGITS;

    lstrcpy(lpT, WOWFAX_MAPPREFIX);
    for (i = 0; i < sizeof(idMap)  ; i++) {
         lpT[(i * 2) + cb] = lphexT[lpid[i] & 0xf];
         lpT[(i * 2) + cb + 1] = lphexT[(lpid[i] & 0xf0) >> 4];
    }

    lpT[(i * 2) + cb] = 0;
}

#endif       // WOWFAX_INC_COMMON_CODE


