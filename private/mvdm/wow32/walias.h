/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WALIAS.H
 *  WOW32 16-bit handle alias support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Modified 12-May-1992 by Mike Tricker (miketri) to add MultiMedia support
--*/

typedef HANDLE HAND32;

#define _WALIAS_
#include "wspool.h"
#include "wowuserp.h"


//
//
// The WC structure is present in every CLS structure in the system,
// although USER32 defines it as an array of 2 DWORDs.  FindPWC(hwnd)
// returns a read-only pointer to the WC structure for a given window's
// class.  Note that only classes registered by Win16 applications will
// have meaningful values in the structure.  To change elements of the
// structure, use SETWC (== SetClassLong) with the appropriate GCL_WOW*
// offset defined below.
//

#define SETWC(hwnd, nIndex, l)  SetClassLong(hwnd, nIndex, l)

#define SETWL(hwnd, nIndex, l)          SetWindowLong(hwnd, nIndex, l)

typedef struct _HDW {
    struct _HDW *Next;      // pointer to next hDDE alias
    HANDLE  hdwp32;         // handle of WOW allocated 32 bit object
} HDW, *PHDW;


/* Handle mapping macros
 */

//
//  The 32-bit hInstance for a 16-bit task will be hMod / hInst.
//  The hModule/hInstnace for a 32-bit entity will be xxxx / 0000.
//
//    FritzS  8/13/92
//

#define HINSTRES32(h16)            ((h16)?HMODINST32(h16):(HANDLE)NULL)

//
// The THREADID32 and HTASK32 macros are nearly equivalent, but the
// WOWHandle mapping uses the one which will detect aliases (see WOLE2.C).
// Most other functions don't need alias detection and it is too late
// to test with the more general.
//

#ifdef DEBUG

//
// Check for task aliases that will cause us to fault if we dereference the NULL
// pointer returned by SEGPTR(htask16,0).
//

#define THREADID32(htask16)                                                   \
        ((htask16)                                                            \
             ? (ISTASKALIAS(htask16)                                          \
                    ? (WOW32ASSERTMSGF(FALSE,                                 \
                           ("WOW32 ERROR %s line %d Task alias "              \
                            "to THREADID32, use HTASK32 instead.\n",          \
                            szModule, __LINE__)), 0)                          \
                    : ((PTDB)SEGPTR((htask16),0))->TDB_ThreadID)              \
             : 0)
#else
#define THREADID32(htask16)  ((htask16)                                       \
                                  ? ((PTDB)SEGPTR((htask16),0))->TDB_ThreadID \
                                  : 0)
#endif

#define HTASK32(htask16)           (Htask16toThreadID32(htask16))
#define GETHTASK16(htask32)        (ThreadID32toHtask16((DWORD)htask32))

#define ISINST16(h32)              (((INT)(h32) & 0x0000ffff) != 0)
#define HMODINST32(h16)            ((HANDLE) MAKELONG(h16, GetExePtr16(h16)))
#define GETHINST16(h32)            ((HAND16)(INT)(h32))
#define GETHMOD16(h32)             ((HAND16)(INT)(HIWORD(h32)))

#define ISMEM16(h32)               (((INT)(h32) & 0xFFFF0000) == 0)
#define HMEM32(h16)                ((HANDLE)(INT)(h16))
#define GETHMEM16(h32)             ((HMEM16)(INT)(h32))

#define ISRES16(h32)               ((INT)(h32)&1)
#define HRES32(p)                  ((p)?(HANDLE)((INT)(p)|1):(HANDLE)NULL)
#define GETHRES16(h32)             ((PRES)((INT)(h32)&~1))

#define USER32(h16)                ((HAND32)(INT)(SHORT)(h16))
#define USER16(h32)                ((HAND16)h32)

#define HWND32(h16)                USER32(h16)
#define FULLHWND32(h16)            (pfnOut.pfnGetFullUserHandle)(h16)
#define GETHWND16(h32)             USER16(h32)
#define GETHWNDIA16(h32)           GETHWND16(h32)
#define HWNDIA32(h16)              HWND32(h16)

#define HMENU32(h16)               USER32(h16)
#define GETHMENU16(h32)            USER16(h32)


#define SERVERHANDLE(h)            (HIWORD(h))

#define GDI32(h16)                 (HANDLE) hConvert16to32(h16)
#define GDI16(h32)                 (HAND16) (((DWORD) (h32)) << 2)

#define HGDI16(hobj32)             GDI16((HAND32)(hobj32))

#define HDC32(hdc16)               GDI32((HAND16)(hdc16))
#define GETHDC16(hdc32)            GDI16((HAND32)(hdc32))

#define HFONT32(hobj16)            GDI32((HAND16)(hobj16))
#define GETHFONT16(hobj32)         GDI16((HAND32)(hobj32))

#define HMETA32(hobj16)            ((HANDLE)HMFFromWinMetaFile((HAND16)(hobj16),FALSE))
#define GETHMETA16(hobj32)         ((HAND16)WinMetaFileFromHMF((HMETAFILE)(hobj32),FALSE))

#define HRGN32(hobj16)             GDI32((HAND16)(hobj16))
#define GETHRGN16(hobj32)          GDI16((HAND32)(hobj32))

#define HBITMAP32(hobj16)          GDI32((HAND16)(hobj16))
#define GETHBITMAP16(hobj32)       GDI16((HAND32)(hobj32))

#define HBRUSH32(hobj16)           GDI32((HAND16)(hobj16))
#define GETHBRUSH16(hobj32)        GDI16((HAND32)(hobj32))

#define HPALETTE32(hobj16)         GDI32((HAND16)(hobj16))
#define GETHPALETTE16(hobj32)      GDI16((HAND32)(hobj32))

#define HPEN32(hobj16)             GDI32((HAND16)(hobj16))
#define GETHPEN16(hobj32)          GDI16((HAND32)(hobj32))

#define HOBJ32(hobj16)             GDI32((HAND16)(hobj16))
#define GETHOBJ16(hobj32)          GDI16((HAND32)(hobj32))

#define HDROP32(hobj16)            (HDROP)DropFilesHandler((HAND16)(hobj16), 0, HDROP_H16 | HDROP_ALLOCALIAS)
#define GETHDROP16(hobj32)         (HAND16)DropFilesHandler(0, (HAND32)(hobj32), HDROP_H32 | HDROP_ALLOCALIAS)
#define FREEHDROP16(hobj16)        (HDROP)DropFilesHandler((HAND16)(hobj16), 0,  HDROP_H16 | HDROP_FREEALIAS)

#define HMODULE32(h16)             ((HANDLE)(h16))     // bogus
#define GETHMODULE16(h32)          ((HAND16)(h32))     // bogus

#define HLOCAL32(h16)              ((HANDLE)(h16))     // bogus
#define GETHLOCAL16(h32)           ((HAND16)(h32))     // bogus

#define HANDLE32(h16)              ((HANDLE)(h16))     // bogus (used in wucomm.c)
#define GETHANDLE16(h32)           ((HAND16)(h32))     // bogus (used in wucomm.c)

#define BOGUSHANDLE32(h16)         ((DWORD)(h16))      // bogus

#define HDWP32(hdwp16)             Prn32((HAND16)(hdwp16))
#define GETHDWP16(hdwp32)          GetPrn16((HAND32)(hdwp32))
#define FREEHDWP16(h16)            FreePrn((HAND16)(h16))

#define COLOR32(clr)               (COLORREF)( ( ((DWORD)(clr) >= 0x03000000) &&  \
                                                 (HIWORD(clr) != 0x10ff) )        \
                                               ? ((clr) & 0xffffff) : (clr) )

/*
 * MultiMedia handle mappings - MikeTri 12-May-1992
 *
 * change WOWCLASS_UNKNOWN to WOWCLASS_WIN16 MikeTri 210292
 */

#define HDRVR32(hdrvr16)           GetMMedia32((HAND16)(hdrvr16))
#define GETHDRVR16(hdrvr32)        GetMMedia16((HAND32)(hdrvr32), WOWCLASS_WIN16)
#define FREEHDRVR16(hdrvr16)       FreeMMedia16((HAND16)(hdrvr16))

#define HMMIO32(hmmio16)           GetMMedia32((HAND16)(hmmio16))
#define GETHMMIO16(hmmio32)        GetMMedia16((HAND32)(hmmio32), WOWCLASS_WIN16)
#define FREEHMMIO16(hmmio16)       FreeMMedia16((HAND16)(hmmio16))

#define HMIDIIN32(hmidiin16)       GetMMedia32((HAND16)(hmidiin16))
#define GETHMIDIIN16(hmidiin32)    GetMMedia16((HAND32)(hmidiin32), WOWCLASS_WIN16)
#define FREEHMIDIIN16(hmidiin16)   FreeMMedia16((HAND16)(hmidiin16))

#define HMIDIOUT32(hmidiout16)     GetMMedia32((HAND16)(hmidiout16))
#define GETHMIDIOUT16(hmidiout32)  GetMMedia16((HAND32)(hmidiout32), WOWCLASS_WIN16)
#define FREEHMIDIOUT16(hmidiout16) FreeMMedia16((HAND16)(hmidiout16))

#define HWAVEIN32(hwavein16)       GetMMedia32((HAND16)(hwavein16))
#define GETHWAVEIN16(hwavein32)    GetMMedia16((HAND32)(hwavein32), WOWCLASS_WIN16)
#define FREEHWAVEIN16(hwavein16)   FreeMMedia16((HAND16)(hwavein16))

#define HWAVEOUT32(hwaveout16)     GetMMedia32((HAND16)(hwaveout16))
#define GETHWAVEOUT16(hwaveout32)  GetMMedia16((HAND32)(hwaveout32), WOWCLASS_WIN16)
#define FREEHWAVEOUT16(hwaveout16) FreeMMedia16((HAND16)(hwaveout16))

/* Function prototypes
 */

INT     GetStdClassNumber(PSZ pszClass);
WNDPROC GetStdClassWndProc(DWORD iClass);
DWORD   GetStdClassThunkProc(INT iClass);

PWC     FindClass16 (LPCSTR pszClass, HINST16 hInst16);
#define FindPWC(h32) (PWC) GetClassLong((h32), GCL_WOWWORDS)
#define FindPWW(h32) (PWW) GetWindowLong((h32), GWL_WOWWORDS)

HAND16  GetMMedia16 (HANDLE h32, INT iClass);  //MultiMedia additions - MikeTri 12-May-1992
HANDLE  GetMMedia32 (HAND16 h16);
VOID    FreeMMedia16 (HAND16 h16);

HAND16  GetWinsock16 (INT h32, INT iClass);  //Winsock additions - DavidTr 4-Oct-1992
DWORD   GetWinsock32 (HAND16 h16);
VOID    FreeWinsock16 (HAND16 h16);

BOOL    MessageNeedsThunking (UINT uMsg);

DWORD   Htask16toThreadID32(HTASK16 htask16);

/* Data structure used in thunking LB_GETTEXT special case
 */

typedef struct _THUNKTEXTDWORD  {
    BOOL               fDWORD;     // dword used or text
    DWORD              dwDataItem; // dword 
} THUNKTEXTDWORD, *PTHUNKTEXTDWORD;

typedef union _MSGTHUNKBUFFER {
    MSG                msg;
    DRAWITEMSTRUCT     ditem;
    MEASUREITEMSTRUCT  mitem;
    DELETEITEMSTRUCT   delitem;
    COMPAREITEMSTRUCT  cmpitem;
    RECT               rect;
    CREATESTRUCT       cstruct;
    WINDOWPOS          winpos;
    CLIENTCREATESTRUCT clcstruct;
    MDINEXTMENU        mnm;
    MDICREATESTRUCT    mdis;
    DROPSTRUCT         dps;
    POINT              pt[5];                               // WM_GETMINMAXINFO
    UINT               uinteger[2];                         // SBM_GETRANGE
    BYTE               cmdichild[sizeof(CREATESTRUCT) +
                                  sizeof(MDICREATESTRUCT)]; // FinishThunking...
    BYTE               cmdiclient[sizeof(CREATESTRUCT) +
                               sizeof(CLIENTCREATESTRUCT)]; // FinishThunking...
    BYTE               calcsz[sizeof(NCCALCSIZE_PARAMS) +
                                        sizeof(WINDOWPOS)];
    THUNKTEXTDWORD     thkdword;                            // LB_GETTEXT w/no HASSTRINGS
} MSGTHUNKBUFFER, *LPMSGTHUNKBUFFER;

typedef struct _MSGPARAMEX *LPMSGPARAMEX;
typedef BOOL   (FASTCALL *LPFNTHUNKMSG16)(LPMSGPARAMEX lpmpex);
typedef VOID   (FASTCALL *LPFNUNTHUNKMSG16)(LPMSGPARAMEX lpmpex);

typedef struct _MSGPARAMEX {
    PARM16 Parm16;
    HWND hwnd;
    UINT uMsg;
    UINT uParam;
    LONG lParam;
    LONG   lReturn;
    LPFNUNTHUNKMSG16 lpfnUnThunk16;
    PWW            pww;
    INT            iMsgThunkClass;          // thunking aid
    INT            iClass;
    MSGTHUNKBUFFER MsgBuffer[1];
} MSGPARAMEX;

#define MSG16NEEDSTHUNKING(lpmpex) ((lpmpex)->iClass != WOWCLASS_NOTHUNK)

// Used for compatibility sake. If app gets The hInstance of a 32bit window
// (the loword of 32bit hinstance is zero) then return a bogus gdt.
//
// Subsequently, if the app does a getmodulefilename on it we will return a
// a fake 32bit modulename.
//
// This is required for a couple of HDC apps and 16bit recorder.
//
//                                                       - Nanduri
//

#define BOGUSGDT 0xfff0
#define VALIDHMOD(h32) (((h32) && !(WORD)(h32)) ? BOGUSGDT : (WORD)(h32))



// For DEVMODE struct handling
// We add a little extra to devmode sizes that we return to 16-bit apps
// including a signature "DM31" at the end of the driver extra stuff
// See notes in wstruc.c

typedef struct _WOWDM31 {
    DWORD dwWOWSig;
    WORD  dmSpecVersion;
    WORD  dmSize;
    WORD  dmDriverExtra;
    WORD  reserved;        // pad to even DWORD (required for ptr arithmetic)
} WOWDM31;
typedef WOWDM31 UNALIGNED *PWOWDM31;

// WOW DEVMODE magic signature
#define WOW_DEVMODE31SIG 0x444d3331   // "DM31"

// Win3.1 DEVMODE spec
#define WOW_DEVMODE31SPEC  0x30A

// Constant we add to Win3.1 DevMode->DriverExtra to account for the NT Devmode
// fields not in the Win3.1 devmode & the WOW thunk info we add to the end
#define WOW_DEVMODEEXTRA  ((sizeof(DEVMODE)-sizeof(DEVMODE31))+sizeof(WOWDM31))



extern WORD gUser16hInstance;
ULONG GetGCL_HMODULE(HWND hwnd);

#define ISFUNCID(dwcallid)  (!((DWORD)(dwcallid) & 0xffff0000))
#define POSTMSG(dwLocal)    (ISFUNCID(dwLocal =  \
                                      FRAMEPTR(CURRENTPTD()->vpStack)->wCallID) ?         \
                                        (aw32WOW[dwLocal].lpfnW32 == WU32PostMessage) :   \
                                        (dwLocal == (DWORD) WU32PostMessage))

ULONG WOW32FaxHandler(UINT iFun, LPSTR lpIn);


INT GetiClassTheHardWay(HWND hwnd);
INT GetIClass(PWW pww, HWND hwnd);

/*
//
// if it's a standard class (fast method) ? 
//    return it :
// else if the window is initialized ?
//    we know it's a private app class : else get the class the hard way
//
// Note: GetiClassTheHardWay() may stiil return a standard class.  See walias.c
//

#define GETICLASS(pww, hwnd) (                                                 \
(((((PWW)pww)->fnid & 0xfff) >= FNID_START) &&                                 \
                                   ((((PWW)pww)->fnid & 0xfff) <= FNID_END)) ? \
    (pfnOut.aiWowClass[(((PWW)pww)->fnid & 0xfff) - FNID_START]) :             \
((((PWW)pww)->state2 & WINDOW_IS_INITIALIZED) ?                                \
    WOWCLASS_WIN16 : GetiClassTheHardWay(hwnd)) )
*/
#define GETICLASS(pww, hwnd) GetIClass(pww, hwnd)
