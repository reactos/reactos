/*-----------------------------------------------------------------------
|
|   CTL3D
|
|       Copyright Microsoft Corporation 1992.  All Rights Reserved.
|
|
|   This module contains the functions to give windows controls a 3d effect
|
|   This source is made public for your edification and debugging pleasure
|
|   PLEASE do not make any changes or release private versions of this DLL
|       send e-mail to me (wesc) if you have feature requests or bug fixes.
|
|   Thanks -- Wes.
|
|
|   History:
|       1-Jan-92 :  Added OOM handling on GetDC (not really necessary but
|                       XL4 OOM failure testing made GetDC return NULL)
|
|       1-Jan-92    :   Check wasn't getting redrawn when state changed in
|                       the default button proc.
|
|       29-Jan-92:  If button has the focus and app is switched in, we weren't
|                       redrawing the entire button check & text.  Force redraw
|                       of these on WM_SETFOCUS message.
|
|        3-Feb-92:  Fixed switch in via task manager by erasing the buttons
|                       backgound on WM_SETFOCUS (detect this when wParam == NULL)
|
|        4-Apr-92:  Make it work with OWNERDRAW buttons
|
|       22-Apr-92:  Removed Excel specific code
|
|       19-May-92:  Turn it into a DLL
|
|       May-Jun92:  Lots o' fixes & enhancements
|
|       23-Jun-92:  Added support for hiding, sizing & moving
|
|       24-Jun-92:  Make checks & radio button circles draw w/ window
|                       text color 'cause they are drawn on window bkgnd
|
|       30-Jun-92:  (0.984) Fix bug where EnableWindow of StaticText doesn't 
|                       redraw properly.  Also disable ctl3d when verWindows > 3.1
|
|      1-Jul-92:  Added WIN32 support (davegi) (not in this source)
|
|       2-Jul-92:  (0.984) Disable when verWindows >= 4.0
|
|       20-Jul-92:  (0.985) Draw focus rects of checks/radios properly on non
|                       default sized controls.
|
|       21-Jul-92:  (0.990) Ctl3dAutoSubclass
|
|       21-Jul-92:  (0.991) ported DaveGi's WIN32 support
|
|       22-Jul-92:  (0.991) fixed Ctl3dCtlColor returning fFalse bug
|
|        4-Aug-92:  (0.992) Graphic designers bug fixes...Now subclass
|                       regular buttons + disabled states for checks & radios
|
|        6-Aug-92:  (0.993) Fix bug where activate via taskman & group
|                       box has focus, & not centering text in buttons
|
|        6-Aug-92:  (0.993) Tweek drawing next to scroll bars. 
|
|       13-Aug-92:  (0.994) Fix button focus rect bug drawing due to 
|                       Win 3.0 DrawText bug.
|
|       14-Aug-92:  (1.0) Release of version 1.0
|                       Don't draw default button border on BS_DEFPUSHBUTTON
|                       pushbuttons
|                       Fix bogus bug where Windows hangs when in a AUTORADIOBUTTON
|                       hold down space bar and hit arrow key. 
|
|       23-Sep-92:  (1.01) Made Ctl3dCtlColor call DefWindowProc so it works when
|                       called in a windproc.
|
|       28-Sep-92:  (1.02) Added MyGetTextExtent so '&''s not considered in 
|                       text extents.
|
|       08-Dec-92:  (1.03) minor tweeks to the button text centering code
|                       for Publisher
|
|       11-Dec-92:  (1.04) added 3d frames to dialogs
|
|       15-Dec-92:  (1.05) fixed bug where group boxes redraw wrong when
|                       Window text is changed to something shorter
|
|       ??-Dec-92:  (1.06) added 3d borders
|
|       21-Dec-92:  (1.07) added WM_DLGBORDER to disable borders
|
|      4-Jan-93:  (1.08) fixed WM_SETTEXT bug w/ DLG frames & checks/checkboxes
|                       Also, WM_DLGSUBCLASS
|
|       22-Feb-93:  (1.12) disabled it under Chicago
|
|       25-Feb-93:  (1.13) re-add fix which allows dialog procs to
|                       handle WM_CTLCOLOR messages
|
|       26-April-93 (2.0) Changed to allow for second subclass. Now uses class instead of
|                         wndproc for subclass determination.
|                         store next wndproc in properties with global atoms                         
|
|       06-Jun-93  (2.0) Make a static linked library version.
|
|
-----------------------------------------------------------------------*/
#include "ctlspriv.h"
#define _hModule HINST_THISDLL
//#define NO_STRICT
//#include <windows.h>

//#ifdef _BORLAND
//#include <mem.h>
//#else
//#include <memory.h>
//#endif

//#include <malloc.h>
#include "ctl3d.h"

//#include "stdio.h"
#define SDLL

/*-----------------------------------------------------------------------
|CTL3D Types
-----------------------------------------------------------------------*/
#ifdef WIN32

#define Win32Only(e) e
#define Win16Only(e)
#define Win32Or16(e32, e16) e32
#define Win16Or32(e16, e32) e32

#define _loadds
#define __export

#define FValidLibHandle(hlib) ((hlib) != NULL)

//
// No concept of far in Win32.
//

#define MEMCMP  memcmp
#define NPTSTR  LPTSTR

//
// Control IDs are LONG in Win32.
//

typedef LONG CTLID;
#define GetControlId(hwnd) GetWindowLong(hwnd, GWL_ID)

//
// Send a color button message.
//

#define SEND_COLOR_BUTTON_MESSAGE( hwndParent, hwnd, hdc )      \
    ((HBRUSH) SendMessage(hwndParent, WM_CTLCOLORBTN, (WPARAM) hdc, (LPARAM) hwnd))

//
// Send a color static message.
//

#define SEND_COLOR_STATIC_MESSAGE( hwndParent, hwnd, hdc )      \
    ((HBRUSH) SendMessage(hwndParent, WM_CTLCOLORSTATIC, (WPARAM) hdc, (LPARAM) hwnd))

#else

#define CallWindowProcA  CallWindowProc
#define DefWindowProcA  DefWindowProc
#define MessageBoxA MessageBox

#define TEXT(a)  a
#define TCHAR    char

#ifndef LPTSTR
#define LPTSTR   LPSTR
#endif
#define LPCTSTR  LPCSTR
#define NPTSTR   NPSTR

#define Win32Only(e)
#define Win16Only(e) e
#define Win32Or16(e32, e16) e16
#define Win16Or32(e16, e32) e16


#define FValidLibHandle(hlib) (( hlib ) > 32 )

#define MEMCMP _fmemcmp

typedef WORD CTLID;
#define GetControlId(h) GetWindowWord(h, GWW_ID)

#define SEND_COLOR_BUTTON_MESSAGE( hwndParent, hwnd, hdc )      \
    ((HBRUSH) SendMessage(hwndParent, WM_CTLCOLOR, (WORD) hdc, MAKELONG(hwnd, CTLCOLOR_BTN)))

#define SEND_COLOR_STATIC_MESSAGE( hwndParent, hwnd, hdc )      \
    ((HBRUSH) SendMessage(hwndParent, WM_CTLCOLOR, (WORD) hdc, MAKELONG(hwnd, CTLCOLOR_STATIC)))


typedef struct
    {
    LPARAM lParam;
    WPARAM wParam;
    UINT   message;
    HWND   hwnd;
} CWPSTRUCT;

#endif // WIN32

// DBCS far east short cut key support
#define cchShortCutModeMax 10
#define chShortCutSbcsPrefix '\036'
#define chShortCutDbcsPrefix '\037'

#define cchClassMax 16  // max class is "combolbox"+NUL rounded up to 16


//#define Assert(f)

#define PUBLIC
#define PRIVATE static

#define fFalse 0
#define fTrue 1

#define INCBTHOOK     1
#define OUTCBTHOOK    0

#ifdef _BORLAND
#define CSCONST(type) type const
#define CodeLpszDecl(lpszVar, szLit) TCHAR *lpszVar = szLit
#define CodeLpszDeclA(lpszVar, szLit) char *lpszVar = szLit
#define _alloca alloca
#define _memcmp memcmp
#else
#ifdef WIN32
#define CSCONST(type) type const
#define CodeLpszDecl(lpszVar, szLit) TCHAR *lpszVar = szLit
#define CodeLpszDeclA(lpszVar, szLit) char *lpszVar = szLit
#else
#define CSCONST(type) type _based(_segname("_CODE")) const
#define CodeLpszDecl(lpszVar, szLit) \
    static CSCONST(char) lpszVar##Code[] = szLit; \
    char far *lpszVar = (char far *)lpszVar##Code
#define CodeLpszDeclA(lpszVar, szLit) \
    static CSCONST(char) lpszVar##Code[] = szLit; \
    char far *lpszVar = (char far *)lpszVar##Code
#endif
#endif


// isomorphic to windows RECT
typedef struct
    {
    int xLeft;
    int yTop;
    int xRight;
    int yBot;
    } RC;


// Windows Versions (Byte order flipped from GetWindowsVersion)
#define ver30  0x0300
#define ver31  0x030a
#define ver40  0x035F

// Border widths
#define dxBorder 1
#define dyBorder 1


// Index Color Table
// WARNING: change mpicvSysColors if you change the icv order
typedef WORD ICV;
#define icvBtnHilite 0
#define icvBtnFace 1
#define icvBtnShadow 2

#define icvBrushMax 3

#define icvBtnText 3
#define icvWindow 4
#define icvWindowText 5
#define icvGrayText 6
#define icvWindowFrame 7
#define icvMax 8

typedef COLORREF CV;

// CoLoR Table
typedef struct
    {
    CV rgcv[icvMax];
    } CLRT;


// BRush Table
typedef struct
    {
    HBRUSH mpicvhbr[icvBrushMax];
    } BRT;


// DrawRec3d flags
#define dr3Left  0x0001
#define dr3Top   0x0002
#define dr3Right 0x0004
#define dr3Bot   0x0008

#define dr3HackBotRight 0x1000  // code size is more important than aesthetics
#define dr3All    0x000f
typedef WORD DR3;


// Control Types
// Commdlg types are necessary because commdlg.dll subclasses certain 
// controls before the app can call Ctl3dSubclassDlg.
#define ctButton            0
#define ctList              1
#define ctEdit              2
#define ctCombo             3
#define ctStatic            4
#define ctComboLBox         5
#define ctMax               6

// ConTroL 
typedef struct 
    {
    WNDPROC lpfn;
    WNDPROC lpfnDefProc;
    TCHAR   szClassName[cchClassMax];
    } CTL;

// Control DEFinition
typedef struct 
    {
    TCHAR sz[20];
    WNDPROC lpfnWndProc;
    BOOL (* lpfnFCanSubclass)(HWND, LONG, WORD, WORD, HWND);
    WORD msk;
    } CDEF;

// CLIent HooK
typedef struct
    {
    HINSTANCE hinstApp;
    HANDLE htask;
    HHOOK  hhook;
    int    iCount;
    DWORD  dwFlags;

    } CLIHK;

#ifdef WIN32
#define iclihkMaxBig    1024
#define iclihkMaxSmall  128
#else
#define iclihkMaxBig    32
#define iclihkMaxSmall  4
#endif

#ifdef DLL
#define iclihkMax iclihkMaxBig
#else
#ifdef SDLL
#define iclihkMax iclihkMaxBig
#else
#define iclihkMax iclihkMaxSmall
#define _loadds
#endif
#endif

#ifdef SDLL
extern const HINSTANCE _hModule;
#endif

// special styles
// #define bitFCoolButtons 0x0001

/*-----------------------------------------------------------------------
|CTL3D Function Prototypes
-----------------------------------------------------------------------*/
PRIVATE VOID End3dDialogs(VOID);
PRIVATE BOOL FAR FInit3dDialogs(VOID);
PRIVATE BOOL DoSubclassCtl(HWND hwnd, WORD grbit, WORD wCallFlags, HWND hwndParent);
PRIVATE BOOL InternalCtl3dColorChange(BOOL fForce);
PRIVATE VOID DeleteObjectNull(HANDLE FAR *ph);
PRIVATE VOID DeleteObjects(VOID);
PRIVATE int  IclihkFromHinst(HANDLE hinst);

LRESULT __export _loadds WINAPI Ctl3dHook(int code, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI BtnWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI EditWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI ListWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI ComboWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI StaticWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI CDListWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
LRESULT __export _loadds WINAPI CDEditWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
WORD    __export _loadds WINAPI Ctl3dSetStyle(HINSTANCE hinst, LPTSTR lpszName, WORD grbit);

LRESULT __export _loadds WINAPI Ctl3dDlgProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);

BOOL FBtn(HWND, LONG, WORD, WORD, HWND);
BOOL FEdit(HWND, LONG, WORD, WORD, HWND);
BOOL FList(HWND, LONG, WORD, WORD, HWND);
BOOL FComboList(HWND, LONG, WORD, WORD, HWND);
BOOL FCombo(HWND, LONG, WORD, WORD, HWND);
BOOL FStatic(HWND, LONG, WORD, WORD, HWND);

HBITMAP PASCAL LoadUIBitmap(HANDLE, LPCTSTR, COLORREF, COLORREF, COLORREF, COLORREF, COLORREF, COLORREF);

#ifdef WIN32
#ifdef DLL
BOOL CALLBACK LibMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved);
#else
#ifdef SDLL
FAR BOOL Ctl3dLibMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved);
#else
FAR BOOL LibMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved);
#endif
#endif
#else
#ifdef DLL
int WINAPI LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine);
#else
#ifdef SDLL
int FAR Ctl3dLibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine);
#else
#ifdef _BORLAND
int FAR PASCAL LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine);
#else
int CALLBACK LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine);
#endif
#endif
#endif
#endif

/*
#ifndef _BORLAND
#ifndef WIN32
#pragma alloc_text(INIT_TEXT, Ctl3dSetStyle)
#pragma alloc_text(INIT_TEXT, Ctl3dColorChange)
#pragma alloc_text(INIT_TEXT, Ctl3dGetVer)
#pragma alloc_text(INIT_TEXT, Ctl3dRegister)
#pragma alloc_text(INIT_TEXT, Ctl3dUnregister)
#pragma alloc_text(INIT_TEXT, Ctl3dAutoSubclass)
#pragma alloc_text(INIT_TEXT, Ctl3dEnabled)
#pragma alloc_text(INIT_TEXT, Ctl3dWinIniChange)
#pragma alloc_text(INIT_TEXT, DeleteObjects)
#pragma alloc_text(INIT_TEXT, DeleteObjectNull)
#pragma alloc_text(INIT_TEXT, InternalCtl3dColorChange)
#ifdef SDLL
#pragma alloc_text(INIT_TEXT, Ctl3dLibMain)
#else
#pragma alloc_text(INIT_TEXT, LibMain)
#endif
#pragma alloc_text(INIT_TEXT, FInit3dDialogs)
#pragma alloc_text(INIT_TEXT, End3dDialogs)
#pragma alloc_text(INIT_TEXT, LoadUIBitmap)
#pragma alloc_text(INIT_TEXT, IclihkFromHinst)
#endif
#endif
*/

#ifndef WIN32
#ifdef DLL
int FAR PASCAL WEP(int);
#pragma alloc_text(WEP_TEXT, WEP)
#endif
#endif

/*-----------------------------------------------------------------------
|CTL3D Globals
-----------------------------------------------------------------------*/
//These static varables are only access when running 16 bit Windows or Win32s
//Since this is single threaded access they are OK to be statics and not protected.
//
static HHOOK    hhookCallWndProcFilterProc;
static WNDPROC  lpfnSubclassByHook;
static HWND     SubclasshWnd;

#ifdef WIN32
CRITICAL_SECTION g_CriticalSection;
#endif

typedef struct _g3d
    {
    BOOL f3dDialogs;
    int cInited;
    ATOM aCtl3dOld;
    ATOM aCtl3dHighOld;
    ATOM aCtl3dLowOld;
    ATOM aCtl3d;
    ATOM aCtl3dHigh;
    ATOM aCtl3dLow;

    ATOM aCtl3dDisable;
    // module & windows stuff
    HINSTANCE hinstLib;
    HANDLE hmodLib;
    WORD   verWindows;
    WORD   verBase;

    // drawing globals
    CLRT clrt;
    BRT brt;
    HBITMAP hbmpCheckboxes;

    // Hook cache
    HANDLE htaskCache;
    int iclihkCache;
    int iclihkMac;
    CLIHK rgclihk[iclihkMax];

    // Control info
    CTL mpctctl[ctMax];
    WNDPROC lpfnDefDlgWndProc;

    // System Metrics
    int dxFrame;
    int dyFrame;
    int dyCaption;
    int dxSysMenu;

    // Windows functions
#ifndef WIN32
#ifdef DLL
    HHOOK (FAR PASCAL *lpfnSetWindowsHookEx)(int, HOOKPROC, HINSTANCE, HANDLE);
    LRESULT (FAR PASCAL *lpfnCallNextHookEx)(HHOOK, int, WPARAM, LPARAM);
    BOOL (FAR PASCAL *lpfnUnhookWindowsHookEx)(HHOOK);
#endif
#endif

    // DBCS stuff
    char chShortCutPrefix;
    char fDBCS;

    } G3D;

G3D g3d;


CSCONST(CDEF) mpctcdef[ctMax] =
{
    { TEXT("Button"), BtnWndProc3d, FBtn, CTL3D_BUTTONS },
    { TEXT("ListBox"), ListWndProc3d, FList, CTL3D_LISTBOXES },
    { TEXT("Edit"), EditWndProc3d, FEdit, CTL3D_EDITS },
    { TEXT("ComboBox"), ComboWndProc3d, FCombo, CTL3D_COMBOS},
    { TEXT("Static"), StaticWndProc3d, FStatic, CTL3D_STATICTEXTS|CTL3D_STATICFRAMES },
    { TEXT("ComboLBox"), ListWndProc3d, FComboList, CTL3D_LISTBOXES },
};


CSCONST (WORD) mpicvSysColor[] =
    {
    COLOR_BTNHIGHLIGHT,
    COLOR_BTNFACE,
    COLOR_BTNSHADOW,
    COLOR_BTNTEXT,
    COLOR_WINDOW,
    COLOR_WINDOWTEXT,
    COLOR_GRAYTEXT,
    COLOR_WINDOWFRAME
    };

#define WM_CHECKSUBCLASS_OLD (WM_USER+5443)
#define WM_CHECKSUBCLASS (WM_USER+5444)

/*-----------------------------------------------------------------------
|   CTL3D Utility routines
-----------------------------------------------------------------------*/

PRIVATE WNDPROC LpfnGetDefWndProcNull(HWND hwnd)
    {                                
    if ( hwnd == NULL ) 
        return NULL;
        
    Win32Only(return (WNDPROC) GetProp(hwnd, (LPCTSTR) g3d.aCtl3d));
    Win16Only(return (WNDPROC) MAKELONG((UINT) GetProp(hwnd, (LPCSTR) g3d.aCtl3dLow),
        GetProp(hwnd, (LPCSTR) g3d.aCtl3dHigh)));
    }

PRIVATE WNDPROC LpfnGetDefWndProc(HWND hwnd, int ct)
        {
        WNDPROC lpfnWndProc;

        lpfnWndProc = LpfnGetDefWndProcNull(hwnd);
        if ( lpfnWndProc == NULL ) {
            if ( ct == ctMax )
                {
                lpfnWndProc = (WNDPROC) g3d.lpfnDefDlgWndProc;
                }
            else
                {
                lpfnWndProc = (WNDPROC) g3d.mpctctl[ct].lpfnDefProc;
                }

            Win32Only(SetProp(hwnd, (LPCTSTR) g3d.aCtl3d, (HANDLE)(DWORD)lpfnWndProc));
            Win16Only(SetProp(hwnd, (LPCTSTR) g3d.aCtl3dLow,  (HANDLE)LOWORD(lpfnWndProc)));
            Win16Only(SetProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh, (HANDLE)HIWORD(lpfnWndProc)));
        }
        return lpfnWndProc;

        }

PRIVATE VOID SubclassTheWindow(HWND hwnd, WNDPROC lpfnSubclassProc)
    {
    WNDPROC lpfnWndProc;

    // Make sure we don't double subclass (16 | 32 bit subclass??)
    if (GetProp(hwnd, (LPCTSTR) g3d.aCtl3dOld) ||
        GetProp(hwnd, (LPCTSTR) g3d.aCtl3d) ||
        GetProp(hwnd, (LPCTSTR) g3d.aCtl3dLow) ||
        GetProp(hwnd, (LPCTSTR) g3d.aCtl3dLowOld) ||
        GetProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh) ||
        GetProp(hwnd, (LPCTSTR) g3d.aCtl3dHighOld))
    {
        return;
    }

    // Is this already subclassed by CTL3D?
    if (LpfnGetDefWndProcNull(hwnd) == (WNDPROC) NULL)
        {
#ifdef WIN32
          if (g3d.fDBCS && !IsWindowUnicode(hwnd)) 
          {
            TCHAR szClass[cchClassMax];
            GetClassName(hwnd, szClass, cchClassMax);
            if (lstrcmpi(szClass, TEXT("edit")) == 0)
            {
                lpfnWndProc = (WNDPROC)SetWindowLongA(hwnd, GWL_WNDPROC,(LONG)lpfnSubclassProc);
                goto SetProps;
            }
          }
#endif

         lpfnWndProc = (WNDPROC)SetWindowLong((HWND) hwnd, GWL_WNDPROC, (LONG) lpfnSubclassProc);
#ifdef WIN32
SetProps:
#endif
         Win32Only(SetProp(hwnd, (LPCTSTR) g3d.aCtl3d, (HANDLE)(DWORD)lpfnWndProc));
         Win16Only(SetProp(hwnd, (LPCTSTR) g3d.aCtl3dLow, (HANDLE)LOWORD(lpfnWndProc)));
         Win16Only(SetProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh, (HANDLE)HIWORD(lpfnWndProc)));
        }
    }

LRESULT __export _loadds WINAPI CallWndProcFilterProc(int code, WPARAM wParam, LPARAM lParam)
{
    CWPSTRUCT FAR *cwpStruct;
    LONG l;

    cwpStruct = (CWPSTRUCT FAR *) lParam;

    l = CallNextHookEx(hhookCallWndProcFilterProc, code, wParam, lParam);

    if ( cwpStruct->hwnd == SubclasshWnd )
        {
        BOOL fSubclass;
        UnhookWindowsHookEx(hhookCallWndProcFilterProc);

        if (g3d.verWindows >= ver40 && (GetWindowLong(cwpStruct->hwnd, GWL_STYLE) & 0x04))
            fSubclass = fFalse;
        else
            fSubclass = fTrue;
        SendMessage(cwpStruct->hwnd, WM_DLGSUBCLASS, 0, (LPARAM)(int FAR *)&fSubclass);
        if (fSubclass)
            SubclassTheWindow(cwpStruct->hwnd, lpfnSubclassByHook);

        hhookCallWndProcFilterProc = 0L;
        lpfnSubclassByHook = NULL;
        SubclasshWnd = NULL;
        }

    return l;
}


PRIVATE VOID HookSubclassWindow(HWND hWnd, WNDPROC lpfnSubclass)
{
    //
    // Windows 3.1 ( 16 bit ) and Win32s can't sublcass in
    // WH_CBT hook. Must set up a MSG hook and subclasss at
    // WM_GETMINMAXINFO ( for dialogs ) or WM_NCCREATE ( for controls )
    // Any other message and we are out of here.
    //
    // Notes from the inside:
    //
    // The only reason not to get the WM_GETMINMAXINFO/WM_NCCREATE message
    // is if another CBT hook did not allow the window create.
    // This code only runs/works on non multithreaded systems. Thus the global
    // to hold the Hook Proc and subclass proc is OK.
    //

    lpfnSubclassByHook = lpfnSubclass;
    SubclasshWnd = hWnd;

    Win32Only(hhookCallWndProcFilterProc = SetWindowsHookEx(WH_CALLWNDPROC, (WNDPROC)CallWndProcFilterProc, g3d.hmodLib, GetCurrentThreadId()));
    Win16Only(hhookCallWndProcFilterProc = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)CallWndProcFilterProc, g3d.hmodLib, GetCurrentTask()));
}

PRIVATE LRESULT CleanupSubclass(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam, int ct)
    {
    WNDPROC lpfnWinProc;
    LRESULT lRet;

    lpfnWinProc = LpfnGetDefWndProc(hwnd, ct);
    lRet = CallWindowProc(lpfnWinProc, hwnd, wm, wParam, lParam);
    Win32Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3d));
    Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dLow));
    Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh));
    RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dDisable);
    return lRet;
    }


PRIVATE VOID DeleteObjectNull(HANDLE FAR *ph)
    {
    if (*ph != NULL)
        {
        DeleteObject(*ph);
        *ph = NULL;
        }
    }

PRIVATE VOID DeleteObjects(VOID)
    {
    int icv;
    
    for(icv = 0; icv < icvBrushMax; icv++)
        DeleteObjectNull(&g3d.brt.mpicvhbr[icv]);
    DeleteObjectNull(&g3d.hbmpCheckboxes);
    }


PRIVATE VOID PatFill(HDC hdc, RC FAR *lprc)
    {
    PatBlt(hdc, lprc->xLeft, lprc->yTop, lprc->xRight-lprc->xLeft, lprc->yBot-lprc->yTop, PATCOPY);
    }


/*-----------------------------------------------------------------------
|   DrawRec3d
|   
|   
|   Arguments:
|       HDC hdc:    
|       RC FAR *lprc:   
|       LONG cvUL:  
|       LONG cvLR:  
|       WORD grbit;
|       
|   Returns:
|       
-----------------------------------------------------------------------*/
PRIVATE VOID DrawRec3d(HDC hdc, RC FAR *lprc, ICV icvUL, ICV icvLR, DR3 dr3)
    {
    COLORREF cvSav;
    RC rc;

    cvSav = SetBkColor(hdc, g3d.clrt.rgcv[icvUL]);

    // top
    rc = *lprc;
    rc.yBot = rc.yTop+1;
    if (dr3 & dr3Top)
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, (LPRECT) &rc, 
            (LPCTSTR) NULL, 0, (int far *) NULL);

    // left
    rc.yBot = lprc->yBot;
    rc.xRight = rc.xLeft+1;
    if (dr3 & dr3Left)
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, (LPRECT) &rc, 
        (LPCTSTR) NULL, 0, (int far *) NULL);

    if (icvUL != icvLR)
        SetBkColor(hdc, g3d.clrt.rgcv[icvLR]);

    // right
    rc.xRight = lprc->xRight;
    rc.xLeft = rc.xRight-1;
    if (dr3 & dr3Right)
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, (LPRECT) &rc, 
            (LPCTSTR) NULL, 0, (int far *) NULL);

    // bot
    if (dr3 & dr3Bot)
        {
        rc.xLeft = lprc->xLeft;
        rc.yTop = rc.yBot-1;
        if (dr3 & dr3HackBotRight)
            rc.xRight -=2;
        ExtTextOut(hdc, 0, 0, ETO_OPAQUE, (LPRECT) &rc, 
            (LPCTSTR) NULL, 0, (int far *) NULL);
        }

    SetBkColor(hdc, cvSav);

    }

#ifdef CANTUSE
// Windows forces dialog fonts to be BOLD...URRRGH
PRIVATE VOID MyDrawText(HWND hwnd, HDC hdc, LPSTR lpch, int cch, RC FAR *lprc, int dt)
    {
    TEXTMETRIC tm;
    BOOL fChisled;

    fChisled = fFalse;
    if (!IsWindowEnabled(hwnd))
        {
        GetTextMetrics(hdc, &tm);
        if (tm.tmWeight > 400)
            SetTextColor(hdc, g3d.clrt.rgcv[icvGrayText]);
        else
            {
            fChisled = fTrue;
            SetTextColor(hdc, g3d.clrt.rgcv[icvBtnHilite]);
            OffsetRect((LPRECT) lprc, -1, -1);
            }
        }
    DrawText(hdc, lpch, cch, (LPRECT) lprc, dt);
    if (fChisled)
        {
        SetTextColor(hdc, g3d.clrt.rgcv[icvBtnHilite]);
        OffsetRect((LPRECT) lprc, 1, 1);
        DrawText(hdc, lpch, cch, (LPRECT) lprc, dt);
        }
    }
#endif


PRIVATE VOID DrawInsetRect3d(HDC hdc, RC FAR *prc, DR3 dr3)
    {
    RC rc;

    rc = *prc;
    DrawRec3d(hdc, &rc, icvWindowFrame, icvBtnFace, (WORD)(dr3 & dr3All));
    rc.xLeft--;
    rc.yTop--;
    rc.xRight++;
    rc.yBot++;
    DrawRec3d(hdc, &rc, icvBtnShadow, icvBtnHilite, dr3);
    }


PRIVATE VOID ClipCtlDc(HWND hwnd, HDC hdc)
    {
    RC rc;

    GetClientRect(hwnd, (LPRECT) &rc);
    IntersectClipRect(hdc, rc.xLeft, rc.yTop, rc.xRight, rc.yBot);
    }


PRIVATE int IclihkFromHinst(HANDLE hinst)
    {
    int iclihk;

    for (iclihk = 0; iclihk < g3d.iclihkMac; iclihk++)
        if (g3d.rgclihk[iclihk].hinstApp == hinst)
            return iclihk;
    return -1;
    }


PRIVATE VOID MyGetTextExtent(HDC hdc, LPTSTR lpsz, int FAR *lpdx, int FAR *lpdy)
    {
    LPTSTR lpch;
    TCHAR  szT[256];

    lpch = szT;
    while(*lpsz != '\000')
        {
        if (*lpsz == '&')
            {
            lpsz++;
            if (*lpsz == '\000')
                break;
            }
//begin DBCS: far east short cut key support
        else if (g3d.fDBCS)
            {
            if (*lpsz == g3d.chShortCutPrefix)
                { // skip only prefix
                lpsz++;
                if (*lpsz == '\000')
                    break;
                }
            else if (*lpsz == chShortCutSbcsPrefix || *lpsz == chShortCutDbcsPrefix)
                { // skip both prefix and short cut key
                lpsz++;
                if (*lpsz == '\000')
                    break;
                lpsz = Win32Or16(CharNext(lpsz),AnsiNext(lpsz));
                continue;
                }
            }
//end DBCS
        *lpch++ = *lpsz++;
        }
    *lpch = '\000';
#ifdef WIN32
    {
    SIZE    pt;

    GetTextExtentPoint(hdc, szT, lstrlen(szT), &pt);
    *lpdx = pt.cx;
    *lpdy = pt.cy;
    }
#else
    {
    long dwExt;

    dwExt = GetTextExtent(hdc, szT, lpch-(char far *)szT);
    *lpdx = LOWORD(dwExt);
    // Check for Hangeul Windows - JeeP 011194
    if ( (g3d.verWindows >= ver31 && GetSystemMetrics(SM_DBCSENABLED)) ||
         (IsDBCSLeadByte(0xa1) && !IsDBCSLeadByte(0xa0)) )
        *lpdy = HIWORD(dwExt)+1;
    else
        *lpdy = HIWORD(dwExt);
    }
#endif
    }
    

/*-----------------------------------------------------------------------
|   CTL3D Publics
-----------------------------------------------------------------------*/


PUBLIC BOOL WINAPI Ctl3dRegister(HINSTANCE hinstApp)
    {

#ifdef WIN32
#ifndef DLL
    InitializeCriticalSection(&g_CriticalSection);
#endif
    EnterCriticalSection(&g_CriticalSection);
#endif

    g3d.cInited++;

    Win32Only(LeaveCriticalSection(&g_CriticalSection));

    if (g3d.cInited == 1)
        {
#ifndef DLL
#ifdef SDLL
        Win32Only(Ctl3dLibMain(hinstApp, DLL_PROCESS_ATTACH, (LPVOID) NULL));
        Win16Only(Ctl3dLibMain(hinstApp, 0, 0, (LPSTR) NULL));
#else
        Win32Only(LibMain(hinstApp, DLL_PROCESS_ATTACH, (LPVOID) NULL));
        Win16Only(LibMain(hinstApp, 0, 0, (LPSTR) NULL));
#endif
#endif
        FInit3dDialogs();
        }

    if (Ctl3dIsAutoSubclass())
        Ctl3dAutoSubclass(hinstApp);

    return g3d.f3dDialogs;
    }


PUBLIC BOOL WINAPI Ctl3dUnregister(HINSTANCE hinstApp)
    {
    int iclihk;
    HANDLE hTask;

    //
    // Find the task's hook
    //
    Win32Only(hTask = (HANDLE)GetCurrentThreadId());
    Win16Only(hTask = GetCurrentTask());

    Win32Only(EnterCriticalSection(&g_CriticalSection));

    for (iclihk = 0; iclihk < g3d.iclihkMac; iclihk++)
        {
        if (g3d.rgclihk[iclihk].htask == hTask)
            {
            g3d.rgclihk[iclihk].iCount--;
            if ( g3d.rgclihk[iclihk].iCount == 0 || hinstApp == g3d.rgclihk[iclihk].hinstApp)
                {
                Win32Only(UnhookWindowsHookEx(g3d.rgclihk[iclihk].hhook));
#ifdef DLL
                Win16Only((*g3d.lpfnUnhookWindowsHookEx)(g3d.rgclihk[iclihk].hhook));
#else
                Win16Only(UnhookWindowsHookEx(g3d.rgclihk[iclihk].hhook));
#endif
                g3d.iclihkMac--;
                while(iclihk < g3d.iclihkMac)
                    {
                    g3d.rgclihk[iclihk] = g3d.rgclihk[iclihk+1];
                    iclihk++;
                    }
                }
            }
        }

    g3d.cInited--;

    Win32Only(LeaveCriticalSection(&g_CriticalSection));
        
    if (g3d.cInited == 0)
        {
        End3dDialogs();
        }
    return fTrue;
    }




/*-----------------------------------------------------------------------
|   Ctl3dAutoSubclass
|   
|      Automatically subclasses all dialogs of the client app.
|
|   Note: Due to bugs in Commdlg, an app should still call Ctl3dSubclassDlg 
|   for the Commdlg OpenFile and PageSetup dialogs.
|   
|   Arguments:
|      HINSTANCE hinstApp:  
|      
|   Returns:
|      
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dAutoSubclass(HINSTANCE hinstApp)
{
    return Ctl3dAutoSubclassEx(hinstApp, 0);
}

PUBLIC BOOL WINAPI Ctl3dAutoSubclassEx(HINSTANCE hinstApp, DWORD dwFlags)
    {
    HHOOK  hhook;
    HANDLE htask;
    int    iclihk;

    if (g3d.verWindows < ver31)
        return fFalse;
    if (!g3d.f3dDialogs)
        return fFalse;

#ifdef WIN32
    // CTL3D_SUBCLASS_DYNCREATE is considered default in Win32, but
    // not Win16 for backward compatibility reasons.
    dwFlags |= CTL3D_SUBCLASS_DYNCREATE;
#endif
    // CTL3D_NOSUBCLASS_DYNCREATE always overrides CTL3D_SUBCLASS_DYNCREATE
    if (dwFlags & CTL3D_NOSUBCLASS_DYNCREATE)
        dwFlags &= ~(CTL3D_NOSUBCLASS_DYNCREATE|CTL3D_SUBCLASS_DYNCREATE);

    Win32Only(EnterCriticalSection(&g_CriticalSection));

    if (g3d.iclihkMac == iclihkMax)
        goto Fail;

    Win32Only(htask = (HANDLE)GetCurrentThreadId());
    Win16Only(htask = GetCurrentTask());
    //
    // Don't set the hook twice for the same task....
    //
    for (iclihk = 0; iclihk < g3d.iclihkMac; iclihk++)
        {
        if (g3d.rgclihk[iclihk].htask == htask)
            {
            g3d.rgclihk[iclihk].iCount++;
            goto Success;
            }
        }

    Win32Only(hhook = SetWindowsHookEx(WH_CBT, (HOOKPROC)Ctl3dHook, g3d.hmodLib, (DWORD)htask));
#ifdef DLL
    Win16Only(hhook = (*g3d.lpfnSetWindowsHookEx)(WH_CBT, (HOOKPROC) Ctl3dHook, g3d.hmodLib, hinstApp == NULL ? NULL : htask));
#else
    Win16Only(hhook = SetWindowsHookEx(WH_CBT, (HOOKPROC) Ctl3dHook, g3d.hmodLib, hinstApp == NULL ? NULL : htask));
#endif
    if (hhook != NULL)
        {
        g3d.rgclihk[g3d.iclihkMac].hinstApp = hinstApp;
        g3d.rgclihk[g3d.iclihkMac].htask    = htask;
        g3d.rgclihk[g3d.iclihkMac].hhook    = hhook;
        g3d.rgclihk[g3d.iclihkMac].iCount   = 1;
        g3d.rgclihk[g3d.iclihkMac].dwFlags  = dwFlags;
        g3d.htaskCache = htask;
        g3d.iclihkCache = g3d.iclihkMac;
        g3d.iclihkMac++;
Success:
        Win32Only(LeaveCriticalSection(&g_CriticalSection));
        return fTrue;
        }
Fail:
    Win32Only(LeaveCriticalSection(&g_CriticalSection));
    return fFalse;
    }

/*-----------------------------------------------------------------------
|   Ctl3dIsAutoSubclass
|   
|   Returns:
|       Whether this task has Automatic Subclassing Enabled
|       
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dIsAutoSubclass()
    {
        int iclihk;
        HANDLE hTask;

        Win32Only(hTask = (HANDLE)GetCurrentThreadId());
        Win16Only(hTask = GetCurrentTask());

        for (iclihk = 0; iclihk < g3d.iclihkMac; iclihk++)
            {
            if (g3d.rgclihk[iclihk].htask == hTask)
                {
                return TRUE;
                }
            }
        // didn't find task in hook table.
        return FALSE;
    }

/*-----------------------------------------------------------------------
|   Ctl3dUnAutoSubclass
|   
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dUnAutoSubclass()
    {
    int iclihk;
    HANDLE hTask;

    // Find the task's hook
    //
    //
    Win32Only(hTask = (HANDLE)GetCurrentThreadId());
    Win16Only(hTask = GetCurrentTask());
    Win32Only(EnterCriticalSection(&g_CriticalSection));
    for (iclihk = 0; iclihk < g3d.iclihkMac; iclihk++)
        {
        if (g3d.rgclihk[iclihk].htask == hTask)
            {
            g3d.rgclihk[iclihk].iCount--;
            if ( g3d.rgclihk[iclihk].iCount == 0 )
                {
                Win32Only(UnhookWindowsHookEx(g3d.rgclihk[iclihk].hhook));
#ifdef DLL
                Win16Only((*g3d.lpfnUnhookWindowsHookEx)(g3d.rgclihk[iclihk].hhook));
#else
                Win16Only(UnhookWindowsHookEx(g3d.rgclihk[iclihk].hhook));
#endif
                g3d.iclihkMac--;
                while(iclihk < g3d.iclihkMac)
                    {
                    g3d.rgclihk[iclihk] = g3d.rgclihk[iclihk+1];
                    iclihk++;
                    }
                }
            }
        }
    Win32Only(LeaveCriticalSection(&g_CriticalSection));
    return TRUE;
    }

WORD __export _loadds WINAPI Ctl3dSetStyle(HINSTANCE hinst, LPTSTR lpszName, WORD grbit)
    {
#ifdef OLD
    WORD grbitOld;

    if (!g3d.f3dDialogs)
        return fFalse;

    grbitOld = grbitStyle;
    if (grbit != 0)
        grbitStyle = grbit;

    if (hinst != NULL && lpszName != NULL)
        {
        HBITMAP hbmpCheckboxesNew;

        hbmpCheckboxesNew = LoadUIBitmap(hinst, (LPCSTR) lpszName,
            g3d.clrt.rgcv[icvWindowText],
            g3d.clrt.rgcv[icvBtnFace],
            g3d.clrt.rgcv[icvBtnShadow],
            g3d.clrt.rgcv[icvBtnHilite],
            g3d.clrt.rgcv[icvWindow],
            g3d.clrt.rgcv[icvWindowFrame]);
        if (hbmpCheckboxesNew != NULL)
            {
            DeleteObjectNull(&g3d.hbmpCheckboxes);
            g3d.hbmpCheckboxes = hbmpCheckboxesNew;
            }
        }
    
    return grbitOld;
#endif
    return 0;
    }


/*-----------------------------------------------------------------------
|   Ctl3dGetVer
|   
|       Returns version of CTL3D library
|   
|   Returns:
|       Major version # in hibyte, minor version # in lobyte
|       
-----------------------------------------------------------------------*/
PUBLIC WORD WINAPI Ctl3dGetVer(void)
    {
    return 0x0231;
    }


/*-----------------------------------------------------------------------
|   Ctl3dEnabled
|   
|   Returns:
|       Whether or not controls will be draw with 3d effects
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dEnabled(void)
    {
    return g3d.f3dDialogs;
    }



/*-----------------------------------------------------------------------
|   Ctl3dSubclassCtl
|   
|       Subclasses an individual control
|   
|   Arguments:
|       HWND hwnd:  
|       
|   Returns:
|       fTrue if control was successfully subclassed
|       
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dSubclassCtl(HWND hwnd)
    {
    if (!g3d.f3dDialogs)
        return fFalse;
    return DoSubclassCtl(hwnd, CTL3D_ALL, OUTCBTHOOK, NULL);
    }

/*-----------------------------------------------------------------------
|   Ctl3dUnsubclassCtl
|   
|       Un-Subclasses an individual control
|   
|   Arguments:
|       HWND hwnd:
|       
|   Returns:
|       fTrue if control was successfully subclassed
|       
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dUnsubclassCtl(HWND hwnd)
    {
    WNDPROC lpfnWinProc;
    HWND hwndKids;
    int ct;

    if (!g3d.f3dDialogs)
        return fFalse;

    lpfnWinProc = (WNDPROC) GetWindowLong(hwnd, GWL_WNDPROC);

    // Is it a control
    for (ct = 0; ct < ctMax; ct++)
        {
        if ( lpfnWinProc == g3d.mpctctl[ct].lpfn )
            {
             lpfnWinProc = LpfnGetDefWndProc(hwnd, ct);
             Win32Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3d));
             Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dLow));
             Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh));
             SetWindowLong(hwnd, GWL_WNDPROC, (LONG) lpfnWinProc );
             lpfnWinProc = NULL;
             ct = ctMax+10;
            }
        }

    // How about a dlg ?
    if ( ct == ctMax )
        {
         if ( lpfnWinProc == (WNDPROC) Ctl3dDlgProc )
            {
             lpfnWinProc = LpfnGetDefWndProc(hwnd, ct);
             Win32Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3d));
             Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dLow));
             Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh));
             SetWindowLong(hwnd, GWL_WNDPROC, (LONG) lpfnWinProc );
             lpfnWinProc = NULL;
            }
         else
            {
               // None of the above, add disable property
               if (GetProp(hwnd, (LPCTSTR) g3d.aCtl3d) ||
                   GetProp(hwnd, (LPCTSTR) g3d.aCtl3dLow) ||
                   GetProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh))
               {
                    SetProp(hwnd,(LPCTSTR) g3d.aCtl3dDisable, (HANDLE) 1);
               }
            }
        }

    //
    // Now unsubclass all the kids
    //
    for (hwndKids = GetWindow(hwnd, GW_CHILD); hwndKids != NULL;
                hwndKids = GetWindow(hwndKids, GW_HWNDNEXT))
        {
            Ctl3dUnsubclassCtl(hwndKids);
        }

    return fTrue;

    }


/*-----------------------------------------------------------------------
|   Ctl3dSubclassCtlEx
|   
|      Actually subclass the control
|   
|      
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dSubclassCtlEx(HWND hwnd, int ct)
    {
    LONG style;
    BOOL fCan;

    if (!g3d.f3dDialogs)
        return fFalse;

    if (ct < 0 || ct > ctMax)
        return fFalse;

    // Is this already subclassed by CTL3D?
    if (LpfnGetDefWndProcNull(hwnd) != (WNDPROC) NULL)
       return fFalse;

    // Only subclass it if it is something that we'd normally subclass
    style = GetWindowLong(hwnd, GWL_STYLE);
    fCan = mpctcdef[ct].lpfnFCanSubclass(hwnd, style, CTL3D_ALL,
        OUTCBTHOOK, GetParent(hwnd));
    if (fCan == fTrue)
        SubclassTheWindow(hwnd, g3d.mpctctl[ct].lpfn);

    return fTrue;
    }

/*-----------------------------------------------------------------------
|   Ctl3dSubclassDlg
|
|      Call this during WM_INITDIALOG processing.
|
|   Arguments:
|      hwndDlg:
|
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dSubclassDlg(HWND hwndDlg, WORD grbit)
    {
    HWND hwnd;

    if (!g3d.f3dDialogs)
        return fFalse;

    for(hwnd = GetWindow(hwndDlg, GW_CHILD); hwnd != NULL;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT))
        {
        DoSubclassCtl(hwnd, grbit, OUTCBTHOOK, NULL);
        }
    return fTrue;
    }

/*-----------------------------------------------------------------------
|   Ctl3dCheckSubclassDlg
|
|      Call this during WM_INITDIALOG processing.
|
|   Arguments:
|      hwndDlg:
|
-----------------------------------------------------------------------*/
PRIVATE void CheckChildSubclass(HWND hwnd, WORD grbit, HWND hwndParent)
{
    // Is this already subclassed by CTL3D?
    // Is our property there ?
    if (LpfnGetDefWndProcNull(hwnd) == (WNDPROC) NULL)
        {
        // No, how did this slip by, try a subclass again.
        DoSubclassCtl(hwnd, grbit, OUTCBTHOOK, hwndParent);
        }
    else
        {
        // Yes, we have subclassed this control.
        // Is our subclass still on the chain ?
        BOOL fSubclass;

        // Make sure subclassing isn't disabled...
        if (GetProp(hwnd, (LPCTSTR)g3d.aCtl3dDisable))
            return;

        fSubclass = 666;
        SendMessage((HWND) hwnd, WM_CHECKSUBCLASS, 0, (LPARAM)(int FAR *)&fSubclass);
        if ( fSubclass == 666 )
            SendMessage((HWND) hwnd, WM_CHECKSUBCLASS_OLD, 0, (LPARAM)(int FAR *)&fSubclass);

        if ( fSubclass == 666 )  // Evil
            {
            // We have been un-subclassed by some bad app ( common dialogs in Win16 )
            // Remove the Prop, and subclass again, take that.
            Win32Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3d));
            Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dLow));
            Win16Only(RemoveProp(hwnd, (LPCTSTR) g3d.aCtl3dHigh));
            DoSubclassCtl(hwnd, grbit, OUTCBTHOOK, hwndParent);
            }
        }
}

PUBLIC BOOL WINAPI Ctl3dCheckSubclassDlg(HWND hwndDlg, WORD grbit)
    {
    HWND hwnd, hwnd2;

    if (!g3d.f3dDialogs)
        return fFalse;

    for (hwnd = GetWindow(hwndDlg, GW_CHILD); hwnd != NULL;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT))
        {
            CheckChildSubclass(hwnd, grbit, NULL);
            for (hwnd2 = GetWindow(hwnd, GW_CHILD); hwnd2 != NULL;
                hwnd2 = GetWindow(hwnd2, GW_HWNDNEXT))
                {
                    CheckChildSubclass(hwnd2, grbit, hwnd);
                }
        }

    return fTrue;
    }

/*-----------------------------------------------------------------------
|   Ctl3dSubclassDlgEx
|
|      Call this during WM_INITDIALOG processing. This is like
|    Ctl3dSubclassDlg but it also subclasses the dialog window itself
|    so the app doesn't need to.
|
|   Arguments:
|      hwndDlg:
|
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dSubclassDlgEx(HWND hwndDlg, DWORD grbit)
    {
    HWND hwnd;

    if (!g3d.f3dDialogs)
        return fFalse;

    for(hwnd = GetWindow(hwndDlg, GW_CHILD); hwnd != NULL;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT))
        {
        DoSubclassCtl(hwnd, LOWORD(grbit), OUTCBTHOOK, NULL);
        }

    //
    // Now Subclass the dialog window as well
    //
    SubclassTheWindow((HWND) hwndDlg, (WNDPROC)Ctl3dDlgProc);

    return fTrue;
    }


/*-----------------------------------------------------------------------
|   Ctl3dCtlColor
|
|       Common CTL_COLOR processor for 3d UITF dialogs & alerts.
|
|   Arguments:
|       hdc:
|       lParam:
|
|   Returns:
|       appropriate brush if g3d.f3dDialogs.  Returns fFalse otherwise
|
-----------------------------------------------------------------------*/
PUBLIC HBRUSH WINAPI Ctl3dCtlColor(HDC hdc, LPARAM lParam)
    {
#ifdef WIN32
    return (HBRUSH) fFalse;
#else
    HWND hwndParent;

    Assert(CTLCOLOR_MSGBOX < CTLCOLOR_BTN);
    Assert(CTLCOLOR_EDIT < CTLCOLOR_BTN);
    Assert(CTLCOLOR_LISTBOX < CTLCOLOR_BTN);
    if(g3d.f3dDialogs)
        {
        if (HIWORD(lParam) >= CTLCOLOR_LISTBOX)
            {
            if (HIWORD(lParam) == CTLCOLOR_LISTBOX &&
                (g3d.verWindows >= ver40 ||
                ((GetWindow((HWND)LOWORD(lParam), GW_CHILD) == NULL ||
                (GetWindowLong((HWND)LOWORD(lParam), GWL_STYLE) & 0x03) == CBS_DROPDOWNLIST))))
                {
                // if it doesn't have a child then it must be a list box
                // don't do brush stuff for drop down lists or else
                // it draws funny grey inside the edit rect
                goto DefWP;
                }
            SetTextColor(hdc, g3d.clrt.rgcv[icvBtnText]);
            SetBkColor(hdc, g3d.clrt.rgcv[icvBtnFace]);
            return g3d.brt.mpicvhbr[icvBtnFace];
            }
        }
DefWP:
    hwndParent = GetParent((HWND)LOWORD(lParam));
    if (hwndParent == NULL)
        return fFalse;
    return (HBRUSH) DefWindowProc(hwndParent, WM_CTLCOLOR, (WPARAM) hdc, (LONG) lParam);
#endif
    }



/*-----------------------------------------------------------------------
|   Ctl3dCtlColorEx
|
|       Common CTL_COLOR processor for 3d UITF dialogs & alerts.
|
|   Arguments:
|
|   Returns:
|       appropriate brush if g3d.f3dDialogs.  Returns fFalse otherwise
|
-----------------------------------------------------------------------*/
PUBLIC HBRUSH WINAPI Ctl3dCtlColorEx(UINT wm, WPARAM wParam, LPARAM lParam)
    {
#ifdef WIN32
    Assert(WM_CTLCOLORMSGBOX < WM_CTLCOLORBTN);
    Assert(WM_CTLCOLOREDIT < WM_CTLCOLORBTN);
    Assert(WM_CTLCOLORLISTBOX < WM_CTLCOLORBTN);
    if(g3d.f3dDialogs)
        {
        if (wm >= WM_CTLCOLORLISTBOX && wm != WM_CTLCOLORSCROLLBAR)
            {
            if (wm == WM_CTLCOLORLISTBOX &&
                (g3d.verWindows >= ver40 ||
                ((GetWindow((HWND) lParam, GW_CHILD) == NULL ||
                (GetWindowLong((HWND) lParam, GWL_STYLE) & 0x03) == CBS_DROPDOWNLIST))))
                {
                // if it doesn't have a child then it must be a list box
                // don't do brush stuff for drop down lists or else
                // it draws funny grey inside the edit rect
                return (HBRUSH) fFalse;
                }
            SetTextColor((HDC) wParam, g3d.clrt.rgcv[icvBtnText]);
            SetBkColor((HDC) wParam, g3d.clrt.rgcv[icvBtnFace]);
            return g3d.brt.mpicvhbr[icvBtnFace];
            }
        }
    return (HBRUSH) fFalse;
#else
    return Ctl3dCtlColor((HDC)wParam, lParam);
#endif
    }


/*-----------------------------------------------------------------------
|   Ctl3dColorChange
|   
|      App calls this when it gets a WM_SYSCOLORCHANGE message
|      
|   Returns:
|      TRUE if successful.
|      
-----------------------------------------------------------------------*/
PUBLIC BOOL WINAPI Ctl3dColorChange(VOID)
    {
    BOOL bResult;
    Win32Only(EnterCriticalSection(&g_CriticalSection));
    bResult = InternalCtl3dColorChange(fFalse);
    Win32Only(LeaveCriticalSection(&g_CriticalSection));
    return bResult;
    }

PRIVATE LONG WINAPI
Ctl3dDlgFramePaintI(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam, BOOL fDefWP);

/*-----------------------------------------------------------------------
|   Ctl3dDlgFramePaint
|   
|      App calls this when it gets a NC_PAINT message
|      
|   Returns:
|      TRUE if successful.
|      
-----------------------------------------------------------------------*/
PUBLIC LONG WINAPI Ctl3dDlgFramePaint(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    return Ctl3dDlgFramePaintI(hwnd, wm, wParam, lParam, TRUE);
    }

// Ctl3dDlgFramePaintI used only internally by Ctl3d
PRIVATE LONG WINAPI 
Ctl3dDlgFramePaintI(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam, BOOL fDefWP)
    {
    LONG lResult;
    LONG lStyle;
    BOOL fBorder;

    WNDPROC defProc = fDefWP ? NULL : (WNDPROC) LpfnGetDefWndProc(hwnd, ctMax);

    if (defProc != NULL)
        lResult = CallWindowProc((WNDPROC)defProc, hwnd, wm, wParam, lParam);
    else
        lResult = DefWindowProc(hwnd, wm, wParam, lParam);

    if (!g3d.f3dDialogs)
        return lResult;

    if ( IsIconic(hwnd) )
        return lResult;

    fBorder = CTL3D_BORDER;
    SendMessage(hwnd, WM_DLGBORDER, 0, (LPARAM)(int FAR *)&fBorder);
    lStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (fBorder != CTL3D_NOBORDER && (lStyle & (WS_VISIBLE|WS_DLGFRAME|DS_MODALFRAME)) == (WS_VISIBLE|WS_DLGFRAME|DS_MODALFRAME))
        {
        BOOL fCaption;
        HBRUSH hbrSav;
        HDC hdc;
        RC rc;
        RC rcFill;
        int dyFrameTop;

        fCaption = (lStyle & WS_CAPTION) == WS_CAPTION;
        dyFrameTop = g3d.dyFrame - (fCaption ? dyBorder : 0);

        hdc = GetWindowDC(hwnd);
        GetWindowRect(hwnd, (LPRECT) &rc);
        rc.xRight = rc.xRight-rc.xLeft;
        rc.yBot = rc.yBot-rc.yTop;
        rc.xLeft = rc.yTop = 0;

        DrawRec3d(hdc, &rc, icvBtnShadow, icvWindowFrame, dr3All);
        InflateRect((LPRECT) &rc, -dxBorder, -dyBorder);
        DrawRec3d(hdc, &rc, icvBtnHilite, icvBtnShadow, dr3All);
        InflateRect((LPRECT) &rc, -dxBorder, -dyBorder);
        
        hbrSav = SelectObject(hdc, g3d.brt.mpicvhbr[icvBtnFace]);
        rcFill = rc;
        // Left
        rcFill.xRight = rcFill.xLeft+g3d.dxFrame;
        PatFill(hdc, &rcFill);
        // Right
        OffsetRect((LPRECT) &rcFill, rc.xRight-rc.xLeft-g3d.dxFrame, 0);
        PatFill(hdc, &rcFill);
        // Top
        rcFill.xLeft = rc.xLeft + g3d.dxFrame;
        rcFill.xRight = rc.xRight - g3d.dxFrame;
        rcFill.yBot = rcFill.yTop+dyFrameTop;
        PatFill(hdc, &rcFill);
        if (fCaption)
            {
            RC rcT;

            rcT = rcFill;
            rcT.yTop += dyFrameTop;
            rcT.yBot = rcT.yTop + g3d.dyCaption;
            DrawRec3d(hdc, &rcT, icvBtnShadow, icvBtnHilite, dr3All);
            }

        // Bottom
        rcFill.yTop += rc.yBot-rc.yTop-g3d.dxFrame;
        rcFill.yBot = rcFill.yTop + g3d.dyFrame;
        PatFill(hdc, &rcFill);
#ifdef CHISLEBORDER
        if (fBorder == CTL3D_CHISLEBORDER)
            {
            // This code doesn't work because it draws in the client area
            GetClientRect(hwnd, (LPRECT) &rc);
            OffsetRect((LPRECT) &rc, g3d.dxFrame+2*dxBorder, fCaption ? g3d.dyFrame+g3d.dyCaption : g3d.dyFrame+dyBorder);
            DrawRec3d(hdc, &rc, icvBtnShadow, icvBtnHilite, dr3Bot|dr3Left|dr3Right);
            rc.xLeft++;
            rc.xRight--;
            rc.yBot--;
            DrawRec3d(hdc, &rc, icvBtnHilite, icvBtnShadow, dr3Bot|dr3Left|dr3Right);
            }
#endif
        SelectObject(hdc, hbrSav);
        ReleaseDC(hwnd, hdc);
        }
    return lResult;
    }


//begin DBCS: far east short cut key support
/*-----------------------------------------------------------------------
|   CTL3D Far East Support
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|   Ctl3dWinIniChange
|   
|       App calls this when it gets a WM_WININICHANGE message
|       
|   Returns:
|       none
|       
-----------------------------------------------------------------------*/
PUBLIC VOID WINAPI Ctl3dWinIniChange(void)
    {
    TCHAR szShortCutMode[cchShortCutModeMax];
    CodeLpszDecl(szSectionWindows, TEXT("windows"));
    CodeLpszDecl(szEntryShortCutKK, TEXT("kanjimenu"));
    CodeLpszDecl(szEntryShortCutCH, TEXT("hangeulmenu"));
    CodeLpszDecl(szShortCutSbcsKK, TEXT("roman"));
    CodeLpszDecl(szShortCutSbcsCH, TEXT("english"));
    CodeLpszDecl(szShortCutDbcsKK, TEXT("kanji"));
    CodeLpszDecl(szShortCutDbcsCH, TEXT("hangeul"));

    if (!g3d.fDBCS)
        return;

    Win32Only(EnterCriticalSection(&g_CriticalSection));

    g3d.chShortCutPrefix = chShortCutSbcsPrefix;
    GetProfileString(szSectionWindows, szEntryShortCutKK, szShortCutSbcsKK, szShortCutMode, cchShortCutModeMax - 1);
    if (!lstrcmpi(szShortCutMode, szShortCutDbcsKK))
        g3d.chShortCutPrefix = chShortCutDbcsPrefix;
    GetProfileString(szSectionWindows, szEntryShortCutCH, szShortCutSbcsCH, szShortCutMode, cchShortCutModeMax - 1);
    if (!lstrcmpi(szShortCutMode, szShortCutDbcsCH))
        g3d.chShortCutPrefix = chShortCutDbcsPrefix;

    Win32Only(LeaveCriticalSection(&g_CriticalSection));
    }
//end DBCS



/*-----------------------------------------------------------------------
|   CTL3D Internal Routines
-----------------------------------------------------------------------*/


/*-----------------------------------------------------------------------
|   FInit3dDialogs
|
|      Initialized 3d stuff
|
-----------------------------------------------------------------------*/
PRIVATE BOOL FAR FInit3dDialogs(VOID)
    {
    HDC hdc;
    WNDCLASS wc;

#ifdef DLL
#ifdef V2
    int nChars;
    LPTSTR pCh;
    static TCHAR MyDirectory[260];
    TCHAR OkDirectory[260];
#endif
#endif

    //if (g3d.verWindows >= ver40)
    //    {
    //    g3d.f3dDialogs = fFalse;
    //    return fFalse;
    //    }

    Win32Only(EnterCriticalSection(&g_CriticalSection));

#ifdef DLL
#ifdef V2

#ifdef WIN32
    {
        TCHAR szT[2];
        CodeLpszDecl(szSpecial, TEXT("Ctl3d_RunAlways"));
        if (GetEnvironmentVariable(szSpecial, szT, 2) != 0 && szT[0] == '1')
        {
            goto AllowBadInstall;
        }
    }
#endif

#ifdef WIN32
#ifdef UNICODE
    if (GetVersion() & 0x80000000)
    {
        Win16Or32(
            CodeLpszDeclA(lpszCtl3d, "CTL3DV2.DLL"),
            CodeLpszDeclA(lpszCtl3d, "CTL3D32.DLL"));
        CodeLpszDeclA(lpszBadInstMsg,
            "This application uses CTL3D32.DLL, which is not the correct version.  "
            "This version of CTL3D32.DLL is designed only for Windows NT systems.");
        MessageBoxA(NULL, lpszBadInstMsg, lpszCtl3d, MB_ICONSTOP | MB_OK);
        g3d.f3dDialogs = fFalse;
        goto Return;
    }
#else
    if (!(GetVersion() & 0x80000000))
    {
        Win16Or32(
            CodeLpszDeclA(lpszCtl3d, "CTL3DV2.DLL"),
            CodeLpszDeclA(lpszCtl3d, "CTL3D32.DLL"));
        CodeLpszDeclA(lpszBadInstMsg,
            "This application uses CTL3D32.DLL, which is not the correct version.  "
            "This version of CTL3D32.DLL is designed only for Win32s or Windows 95 systems.");
        MessageBoxA(NULL, lpszBadInstMsg, lpszCtl3d, MB_ICONSTOP | MB_OK);
        g3d.f3dDialogs = fFalse;
        goto Return;
    }
#endif
#endif
    nChars = GetModuleFileName(g3d.hinstLib, MyDirectory, sizeof(MyDirectory)Win32Only(/sizeof(TCHAR)));
    for (pCh = (LPTSTR)(MyDirectory+nChars-1);
         pCh >= (LPTSTR)MyDirectory;
         pCh = Win32Or16(CharPrev(MyDirectory, pCh),AnsiPrev(MyDirectory, pCh)))
        {
        if (  *pCh == '\\' )
            {
            if ( *(pCh-1) != ':' )
                *pCh = 0;
            else
                *(pCh+1) = 0;
            break;
            }
        }

    nChars = GetSystemDirectory(OkDirectory, sizeof(OkDirectory)Win32Only(/sizeof(TCHAR)));
    if ( lstrcmpi(MyDirectory,OkDirectory ) )
        {
        nChars = GetWindowsDirectory(OkDirectory, sizeof(OkDirectory)Win32Only(/sizeof(TCHAR)));
        if ( lstrcmpi(MyDirectory,OkDirectory ) )
            {
            Win16Or32(
                CodeLpszDeclA(lpszCtl3d, "CTL3DV2.DLL"),
                CodeLpszDeclA(lpszCtl3d, "CTL3D32.DLL"));
            Win16Or32(
            CodeLpszDeclA(lpszBadInstMsg,
                "This application uses CTL3DV2.DLL, which has not been correctly installed.  "
                "CTL3DV2.DLL must be installed in the Windows system directory."),
            CodeLpszDeclA(lpszBadInstMsg,
                "This application uses CTL3D32.DLL, which has not been correctly installed.  "
                "CTL3D32.DLL must be installed in the Windows system directory."));
            Win32Only(LeaveCriticalSection(&g_CriticalSection));
            MessageBoxA(NULL, lpszBadInstMsg, lpszCtl3d, MB_ICONSTOP | MB_OK );
            g3d.f3dDialogs = fFalse;
            goto Return;
            }
        }

Win32Only(AllowBadInstall:;)
#endif
#endif

    hdc = GetDC(NULL);
    g3d.f3dDialogs = GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES) >= 4;
    // Win 3.1 EGA lies to us...
    if(GetSystemMetrics(SM_CYSCREEN) == 350 && GetSystemMetrics(SM_CXSCREEN) == 640)
        g3d.f3dDialogs = fFalse;
    ReleaseDC(NULL, hdc);
    if (g3d.f3dDialogs)
        {
        int ct; 
        CodeLpszDecl(lpszC3dD, TEXT("C3dD"));
        
        CodeLpszDecl(lpszC3dOld, TEXT("C3d"));
        CodeLpszDecl(lpszC3dLOld, TEXT("C3dL"));
        CodeLpszDecl(lpszC3dHOld, TEXT("C3dH"));
        CodeLpszDecl(lpszC3d, TEXT("C3dNew"));
        CodeLpszDecl(lpszC3dL, TEXT("C3dLNew"));
        CodeLpszDecl(lpszC3dH, TEXT("C3dHNew"));

        g3d.aCtl3dOld = GlobalAddAtom(lpszC3dOld);
        if (g3d.aCtl3dOld == 0)
            {
            g3d.f3dDialogs = fFalse;
            goto Return;
            }
        g3d.aCtl3d  = GlobalAddAtom(lpszC3d);
        if (g3d.aCtl3d == 0)
            {
            g3d.f3dDialogs = fFalse;
            goto Return;
            }

        g3d.aCtl3dLowOld = GlobalAddAtom(lpszC3dLOld);
        g3d.aCtl3dHighOld = GlobalAddAtom(lpszC3dHOld);
        if (g3d.aCtl3dLowOld == 0 || g3d.aCtl3dHighOld == 0)
            {
            g3d.f3dDialogs = fFalse;
            return fFalse;
            }

        g3d.aCtl3dLow  = GlobalAddAtom(lpszC3dL);
        g3d.aCtl3dHigh = GlobalAddAtom(lpszC3dH);
        if (g3d.aCtl3dLow == 0 || g3d.aCtl3dHigh == 0)
            {
            g3d.f3dDialogs = fFalse;
            return fFalse;
            }

        g3d.aCtl3dDisable = GlobalAddAtom(lpszC3dD);
        if (g3d.aCtl3dDisable == 0)
            {
            g3d.f3dDialogs = fFalse;
            goto Return;
            }

        // DBCS
        g3d.fDBCS = GetSystemMetrics(SM_DBCSENABLED);
        Ctl3dWinIniChange();
                                                     
        if (InternalCtl3dColorChange(fTrue))        // load bitmap & brushes
            {
            for (ct = 0; ct < ctMax; ct++)
                {
                g3d.mpctctl[ct].lpfn = (WNDPROC)mpctcdef[ct].lpfnWndProc;
                Assert(g3d.mpctctl[ct].lpfn != NULL);
                GetClassInfo(NULL, mpctcdef[ct].sz, (LPWNDCLASS) &wc);
                g3d.mpctctl[ct].lpfnDefProc = wc.lpfnWndProc;
                }
            if (GetClassInfo(NULL, WC_DIALOG, &wc))
                g3d.lpfnDefDlgWndProc = (WNDPROC) wc.lpfnWndProc;
            else
                g3d.lpfnDefDlgWndProc = (WNDPROC) DefDlgProc;
            }
        else
            {
            g3d.f3dDialogs = fFalse;
            }
        }
Return:
    Win32Only(LeaveCriticalSection(&g_CriticalSection));
    return g3d.f3dDialogs;
    }



/*-----------------------------------------------------------------------
|   End3dDialogs
|
|       Called at DLL termination to free 3d dialog stuff
-----------------------------------------------------------------------*/
PRIVATE VOID End3dDialogs(VOID)
    {
    int ct;

    Win32Only(EnterCriticalSection(&g_CriticalSection));

    for (ct = 0; ct < ctMax; ct++)
        {                                              
        if(g3d.mpctctl[ct].lpfn != NULL)
            {
            FreeProcInstance((FARPROC)g3d.mpctctl[ct].lpfn);
            g3d.mpctctl[ct].lpfn = NULL;
            }
        }
    DeleteObjects();
    g3d.aCtl3dOld ? GlobalDeleteAtom(g3d.aCtl3dOld) : 0;
    g3d.aCtl3d ? GlobalDeleteAtom(g3d.aCtl3d) : 0;
    g3d.aCtl3dLowOld ? GlobalDeleteAtom(g3d.aCtl3dLowOld) : 0;
    g3d.aCtl3dHighOld ? GlobalDeleteAtom(g3d.aCtl3dHighOld) : 0;
    g3d.aCtl3dLow ? GlobalDeleteAtom(g3d.aCtl3dLow) : 0;
    g3d.aCtl3dHigh ? GlobalDeleteAtom(g3d.aCtl3dHigh) : 0;
    g3d.aCtl3dDisable ? GlobalDeleteAtom(g3d.aCtl3dDisable) : 0;

    g3d.f3dDialogs = fFalse;

    Win32Only(LeaveCriticalSection(&g_CriticalSection));

    }


PRIVATE BOOL InternalCtl3dColorChange(BOOL fForce)
    {
    ICV icv;
    CLRT clrtNew;
    HBITMAP hbmpCheckboxesNew;
    BRT brtNew;

    if (!g3d.f3dDialogs)
        return fFalse;

    for (icv = 0; icv < icvMax; icv++)
        clrtNew.rgcv[icv] = GetSysColor(mpicvSysColor[icv]);

    if (g3d.verWindows == ver30)
        clrtNew.rgcv[icvBtnHilite] = RGB(0xff, 0xff, 0xff);

    if (clrtNew.rgcv[icvGrayText] == 0L || clrtNew.rgcv[icvGrayText] == clrtNew.rgcv[icvBtnFace])
        {
        if (clrtNew.rgcv[icvBtnFace] == RGB(0x80, 0x80, 0x80))
            clrtNew.rgcv[icvGrayText] = RGB(0xc0, 0xc0, 0xc0);
        else
            clrtNew.rgcv[icvGrayText] = RGB(0x80, 0x80, 0x80);
        }

    if (fForce || MEMCMP(&g3d.clrt, &clrtNew, sizeof(CLRT)))
        {
        hbmpCheckboxesNew = LoadUIBitmap(g3d.hinstLib, MAKEINTRESOURCE(CTL3D_3DCHECK),
            clrtNew.rgcv[icvWindowText],
            clrtNew.rgcv[icvBtnFace],
            clrtNew.rgcv[icvBtnShadow],
            clrtNew.rgcv[icvBtnHilite],
            clrtNew.rgcv[icvWindow],
            clrtNew.rgcv[icvWindowFrame]);

        for (icv = 0; icv < icvBrushMax; icv++)
            brtNew.mpicvhbr[icv] = CreateSolidBrush(clrtNew.rgcv[icv]);

        for (icv = 0; icv < icvBrushMax; icv++)
            if (brtNew.mpicvhbr[icv] == NULL)
                goto OOM;

        if(hbmpCheckboxesNew != NULL)
            {
            DeleteObjects();
            g3d.brt = brtNew;
            g3d.clrt = clrtNew;
            g3d.hbmpCheckboxes = hbmpCheckboxesNew;
            return fTrue;
            }
        else
            {
OOM:
            for (icv = 0; icv < icvBrushMax; icv++)
                DeleteObjectNull(&brtNew.mpicvhbr[icv]);
            DeleteObjectNull(&hbmpCheckboxesNew);
            return fFalse;
            }
        }
    return fTrue;
    }


/*-----------------------------------------------------------------------
|   Ctl3dDlgProc
|   
|       Subclass DlgProc for use w/ Ctl3dAutoSubclass
|   
|   
|   Arguments:
|       HWND hwnd:  
|       int wm: 
|       WORD wParam:    
|       LPARAM lParam:  
|       
|   Returns:
|       
-----------------------------------------------------------------------*/
LRESULT __export _loadds WINAPI Ctl3dDlgProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    HBRUSH hbrush;
    WNDPROC lpfnDlgProc;
    TCHAR szClass[cchClassMax];

    if ( wm == WM_NCDESTROY )
    return CleanupSubclass(hwnd, wm, wParam, lParam, ctMax);

    if ( GetProp(hwnd,(LPCTSTR) g3d.aCtl3dDisable) )
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd, wm, wParam, lParam);

    switch (wm)
        {
    case WM_CHECKSUBCLASS_OLD:
    case WM_CHECKSUBCLASS:
        *(int FAR *)lParam = fTrue;
        return ctMax+1000;

    case WM_INITDIALOG:
      {
        long l;
        BOOL fSubclass;
        WNDPROC lpfnWinProc;

        lpfnWinProc = LpfnGetDefWndProc(hwnd, ctMax);

        if (g3d.verWindows >= ver40 && (GetWindowLong(hwnd, GWL_STYLE) & 0x04))
            fSubclass = fFalse;
        else
            fSubclass = fTrue;
        SendMessage(hwnd, WM_DLGSUBCLASS, 0, (LPARAM)(int FAR *)&fSubclass);

        if (!fSubclass)
            {
            Ctl3dUnsubclassCtl(hwnd);
            return CallWindowProc(lpfnWinProc, hwnd, wm, wParam, lParam);
            }

        l = CallWindowProc(lpfnWinProc, hwnd, wm, wParam, lParam);

        if (g3d.verWindows < ver40 || !(GetWindowLong(hwnd, GWL_STYLE) & 0x04))
            Ctl3dCheckSubclassDlg(hwnd, CTL3D_ALL);

        return l;
      }

    case WM_NCPAINT:
    case WM_NCACTIVATE:
    case WM_SETTEXT:
        if (g3d.verWindows >= ver40 || IsIconic(hwnd) )
            return CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd, wm, wParam, lParam);
        else
            return Ctl3dDlgFramePaintI(hwnd, wm, wParam, lParam, FALSE);

#ifdef WIN32
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSTATIC:
#else
    case WM_CTLCOLOR:
#endif
    // Is this really a dialog
    GetClassName(hwnd, szClass, sizeof(szClass)Win32Only(/sizeof(TCHAR)));
    if (lstrcmp(TEXT("#32770"),szClass) != 0 )
       {
#ifdef WIN32
          hbrush = (HBRUSH) CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd,
                           wm-WM_CTLCOLORMSGBOX+CTLMSGOFFSET, wParam, lParam);
#else
          hbrush = (HBRUSH) CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd,
                           CTL3D_CTLCOLOR, wParam, lParam);
#endif
          if (hbrush == (HBRUSH) fFalse || hbrush == (HBRUSH)1)
            hbrush = Ctl3dCtlColorEx(wm, wParam, lParam);
       }
    else
     {
        lpfnDlgProc = (WNDPROC) GetWindowLong(hwnd, DWL_DLGPROC);

        if (lpfnDlgProc == NULL )
            {
            hbrush = Ctl3dCtlColorEx(wm, wParam, lParam);
            }
        else
            {
#ifdef WIN32
            if ( (LONG)lpfnDlgProc > 0xFFFF0000 && g3d.verWindows <= ver31)
                {
                // We have a Uni-code / non Unicode issue.
                // If this is before Daytona, then I CAN NOT call because it may be NULL, but
                // the returned value is not-null. NT Bug.
                // So Just send our own message to the window proc instead
                hbrush = (HBRUSH) CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd,
                                                wm-WM_CTLCOLORMSGBOX+CTLMSGOFFSET, wParam, lParam);
                if (hbrush == (HBRUSH) fFalse || hbrush == (HBRUSH)1)
                    hbrush = Ctl3dCtlColorEx(wm, wParam, lParam);
                }
            else
                {
#endif
                hbrush = (HBRUSH) CallWindowProc(lpfnDlgProc, hwnd, wm, wParam, lParam);
                if (hbrush == (HBRUSH) fFalse || hbrush == (HBRUSH)1)
                    {
#ifdef WIN32
                    hbrush = (HBRUSH) CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd,
                                                wm-WM_CTLCOLORMSGBOX+CTLMSGOFFSET, wParam, lParam);
#else
                    hbrush = (HBRUSH) CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd,
                                                CTL3D_CTLCOLOR, wParam, lParam);
#endif
                    if (hbrush == (HBRUSH) fFalse || hbrush == (HBRUSH)1)
                        hbrush = Ctl3dCtlColorEx(wm, wParam, lParam);
                    }
                }
#ifdef WIN32
            }
#endif
    }
        if (hbrush != (HBRUSH) fFalse)
            return  (LRESULT)hbrush;
        break;
        }                         
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ctMax), hwnd, wm, wParam, lParam);
    }

PRIVATE BOOL NEAR DoesChildNeedSubclass(HWND hwnd)
    {
        if (!LpfnGetDefWndProcNull(hwnd))
            return fFalse;
        if (g3d.verWindows >= ver40 && GetWindowLong(hwnd, GWL_STYLE) & 0x04)
            return fFalse;
        return fTrue;
    }

/*-----------------------------------------------------------------------
|   Ctl3dHook
|   
|      CBT Hook to watch for window creation.  Automatically subclasses all
|   dialogs w/ Ctl3dDlgProc
|   
|   Arguments:
|      int code:   
|      WORD wParam: 
|      LPARAM lParam:  
|      
|   Returns:
|      
-----------------------------------------------------------------------*/
LRESULT __export _loadds WINAPI Ctl3dHook(int code, WPARAM wParam, LPARAM lParam)
    {
    int iclihk;
    HANDLE htask;

    htask = Win32Or16((HANDLE)GetCurrentThreadId(), GetCurrentTask());
    Win32Only(EnterCriticalSection(&g_CriticalSection));
    if (htask != g3d.htaskCache)
        {
        for (iclihk = 0; iclihk < g3d.iclihkMac; iclihk++)
            {
            if (g3d.rgclihk[iclihk].htask == htask)
                {
                g3d.iclihkCache = iclihk;
                g3d.htaskCache = htask;
                break;
                }
        }
        if ( iclihk == g3d.iclihkMac )
            {
            // didn't find task in hook table.  This could be bad, but
            // returning 0L is about all we can doo.
            //
            // Actually not. The hhook isn't used anyway just set it to NULL.
            // and call the next hook..... KGM
            Win32Only(LeaveCriticalSection(&g_CriticalSection));
            return CallNextHookEx((HHOOK)0L, code, wParam, lParam);
            }
        }
    iclihk = g3d.iclihkCache;
    Win32Only(LeaveCriticalSection(&g_CriticalSection));

    if (code == HCBT_CREATEWND)
        {
        LPCREATESTRUCT lpcs;
        lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;

          if (lpcs->lpszClass == WC_DIALOG)
            {
            if (g3d.verBase == 32)
               {
                BOOL fSubclass;
                if (g3d.verWindows >= ver40 && (GetWindowLong((HWND)wParam, GWL_STYLE) & 0x04))
                    fSubclass = fFalse;
                else
                    fSubclass = fTrue;
                SendMessage((HWND)wParam, WM_DLGSUBCLASS, 0, (LPARAM)(int FAR *)&fSubclass);
                if (fSubclass)
                    SubclassTheWindow((HWND)wParam, (WNDPROC) Ctl3dDlgProc);
               }
            else
               {
               HookSubclassWindow((HWND)wParam, (WNDPROC) Ctl3dDlgProc);
               }
            goto Zing;
            }
          if (!(g3d.rgclihk[iclihk].dwFlags & CTL3D_SUBCLASS_DYNCREATE))
            goto Zing;

          if (DoesChildNeedSubclass(lpcs->hwndParent) ||
             (lpcs->hwndParent && g3d.verBase != 24 &&
                DoesChildNeedSubclass(GetParent(lpcs->hwndParent))))
            {
            DoSubclassCtl((HWND)wParam, CTL3D_ALL, INCBTHOOK, lpcs->hwndParent);
            }
        }

Zing:;
    Win32Only(return CallNextHookEx(g3d.rgclihk[iclihk].hhook, code, wParam, lParam));
#ifdef DLL
    Win16Only(return (*g3d.lpfnCallNextHookEx)(g3d.rgclihk[iclihk].hhook, code, wParam, lParam));
#else
    Win16Only(return (CallNextHookEx(g3d.rgclihk[iclihk].hhook, code, wParam, lParam)));
#endif
    }




/*-----------------------------------------------------------------------
|   CTL3D F* routines
|   
|   These routines determine whether or not the given control may be
|      subclassed.  They may recursively call DoSubclassCtl in the
|      case of multi-control controls
|
|   Returns:
|      fTrue if can subclass the given control.
-----------------------------------------------------------------------*/


PRIVATE BOOL FBtn(HWND hwnd, LONG style, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {
    if (g3d.verWindows >= ver40)
       {
        return fFalse;
       }
    style &= ~(BS_LEFTTEXT);
    return ( LOWORD(style) >= BS_PUSHBUTTON && LOWORD(style) <= BS_AUTORADIOBUTTON);
    }

PRIVATE BOOL FEdit(HWND hwnd, LONG style, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {
    if (g3d.verWindows >= ver40 && hwndParent)
        {
         TCHAR szClass[cchClassMax];
         GetClassName(hwndParent, szClass, sizeof(szClass)Win32Only(/sizeof(TCHAR)));
         if (lstrcmp(szClass, mpctcdef[ctCombo].sz) == 0 )
            return fFalse;
         else
            return fTrue;
        }
    else
    return fTrue;
    }

PRIVATE BOOL FList(HWND hwnd, LONG style, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {
    if (g3d.verWindows >= ver40 && hwndParent)
        {
         TCHAR szClass[cchClassMax];
         GetClassName(hwndParent, szClass, sizeof(szClass)Win32Only(/sizeof(TCHAR)));
         if (lstrcmp(szClass, mpctcdef[ctCombo].sz) == 0 )
            return fFalse;
         else
            return fTrue;
        }
    else
       return fTrue;
    }

PRIVATE BOOL FComboList(HWND hwnd, LONG style, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {

    if (g3d.verWindows >= ver40)
       return fFalse;

    if ( wCallFlags == INCBTHOOK )
        {
        LONG style;
        style = GetWindowLong(hwndParent, GWL_STYLE);
        if (!(((style & 0x0003) == CBS_DROPDOWN) || ((style & 0x0003) == CBS_DROPDOWNLIST)))
            return fTrue;
        else
            return fFalse;
        }

    return fTrue;
    }

PRIVATE BOOL FCombo(HWND hwnd, LONG style, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {
    HWND hwndEdit;
    HWND hwndList;

    if (g3d.verWindows >= ver40)
       return fFalse;

    if ((style & 0x0003) == CBS_DROPDOWN)
        {
        if ( wCallFlags == INCBTHOOK )
            {
            return fFalse;
            }
        // Subclass edit so bottom border of the edit draws properly...This case
        // is specially handled in ListEditPaint3d
        hwndEdit = GetWindow(hwnd, GW_CHILD);
        if (hwndEdit != NULL)
            DoSubclassCtl(hwndEdit, CTL3D_EDITS, wCallFlags, hwnd);
        return fTrue;
        }
    else if ((style & 0x0003) == CBS_DROPDOWNLIST )
        {
        return fTrue;
        }
    else // assume simple // if ((style & 0x0003) == CBS_SIMPLE)
        {
        if ( wCallFlags == INCBTHOOK )
            {
                return fTrue;
            }
        hwndList = GetWindow(hwnd, GW_CHILD);
        if (hwndList != NULL)
            {
            // Subclass list & edit box so they draw properly.  We also
            // subclass the combo so we can hide/show/move it and the
            // 3d effects outside the client area get erased
            DoSubclassCtl(hwndList, CTL3D_LISTBOXES, wCallFlags, hwnd);

            hwndEdit = GetWindow(hwndList, GW_HWNDNEXT);
            if (hwndEdit != NULL)
                DoSubclassCtl(hwndEdit, CTL3D_EDITS, wCallFlags, hwnd);
            return fTrue;
            }
        return fFalse;  
        }
    }

PRIVATE BOOL FStatic(HWND hwnd, LONG style, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {
    int wStyle;

    wStyle = LOWORD(style) & 0x1f;
    return (wStyle != SS_ICON &&
        ((grbit & CTL3D_STATICTEXTS) && 
        (wStyle <= SS_RIGHT || wStyle == SS_LEFTNOWORDWRAP) ||
        ((grbit & CTL3D_STATICFRAMES) &&
        ((wStyle >= SS_BLACKRECT && wStyle <= SS_WHITEFRAME) ||
         (g3d.verWindows < ver40 && wStyle >= 0x10 && wStyle <= 0x12)))));
    }



/*-----------------------------------------------------------------------
|   DoSubclassCtl
|   
|      Actually subclass the control
|   
|   
|   Arguments:
|      HWND hwnd:  
|      WORD grbit: 
|      WORD wCallFlags
|   Returns:
|      
-----------------------------------------------------------------------*/
PRIVATE BOOL DoSubclassCtl(HWND hwnd, WORD grbit, WORD wCallFlags, HWND hwndParent)
    {
    LONG style;
    int ct;
    BOOL fCan;
    TCHAR szClass[cchClassMax];

    // Is this already subclassed by CTL3D?
    if (LpfnGetDefWndProcNull(hwnd) != (WNDPROC) NULL)
       return fFalse;

    GetClassName(hwnd, szClass, sizeof(szClass)Win32Only(/sizeof(TCHAR)));

    for (ct = 0; ct < ctMax; ct++)
        {
        if ((mpctcdef[ct].msk & grbit) &&
            (lstrcmp(mpctcdef[ct].sz,szClass) == 0))
            {
            style = GetWindowLong(hwnd, GWL_STYLE);
            fCan = mpctcdef[ct].lpfnFCanSubclass(hwnd, style, grbit, wCallFlags, hwndParent);
            if (fCan == fTrue)
                {
                if ( wCallFlags == INCBTHOOK && g3d.verBase == 16 )
                    HookSubclassWindow(hwnd, g3d.mpctctl[ct].lpfn);
                else
                    SubclassTheWindow(hwnd, g3d.mpctctl[ct].lpfn);
                }
            return fCan != fFalse;
            }
        }

    return fFalse;
    }



/*-----------------------------------------------------------------------
|   Inval3dCtl
|   
|      Invalidate the controls rect in response to a WM_SHOWWINDOW or
|   WM_WINDOWPOSCHANGING message.  This is necessary because ctl3d draws
|   the 3d effects of listboxes, combos & edits outside the controls client
|   rect.
|   
|   Arguments:
|      HWND hwnd:  
|      WINDOWPOS FAR *lpwp: 
|      
|   Returns:
|      
-----------------------------------------------------------------------*/
PRIVATE VOID Inval3dCtl(HWND hwnd, WINDOWPOS FAR *lpwp)
    {
    RC rc;
    HWND hwndParent;
    LONG lStyle;
    unsigned flags;

    GetWindowRect(hwnd, (LPRECT) &rc);
    lStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (lStyle & WS_VISIBLE)
        {
        if (lpwp != NULL)
            {
            flags = lpwp->flags;

            //
            // Is all this necessary ? Are we moving or sizing ?
            //
            if ( !((flags & SWP_HIDEWINDOW) || (flags & SWP_SHOWWINDOW)) &&
                (flags & SWP_NOMOVE) && (flags & SWP_NOSIZE) )
               // Nope
               return;

            // handle integral height listboxes (or any other control which
            // shrinks from the bottom)
            if ((flags & (SWP_NOMOVE|SWP_NOSIZE)) == SWP_NOMOVE &&
                (lpwp->cx == (rc.xRight-rc.xLeft) && lpwp->cy <= (rc.yBot-rc.yTop)))
                rc.yTop = rc.yTop+lpwp->cy+1;      // +1 to offset InflateRect
            }
        InflateRect((LPRECT) &rc, 1, 1);
        hwndParent = GetParent(hwnd);
        ScreenToClient(hwndParent, (LPPOINT) &rc);
        ScreenToClient(hwndParent, ((LPPOINT) &rc)+1);
        if(lStyle & WS_VSCROLL)
            rc.xRight ++;
        InvalidateRect(hwndParent, (LPRECT) &rc, fFalse);
        }
    }

/*-----------------------------------------------------------------------
|   Val3dCtl
|      
-----------------------------------------------------------------------*/
PRIVATE VOID Val3dCtl(HWND hwnd)
    {
    RC rc;
    HWND hwndParent;
    LONG lStyle;

    lStyle = GetWindowLong(hwnd, GWL_STYLE);
    GetWindowRect(hwnd, (LPRECT) &rc);
    InflateRect((LPRECT) &rc, 1, 1);
    hwndParent = GetParent(hwnd);
    ScreenToClient(hwndParent, (LPPOINT) &rc);
    ScreenToClient(hwndParent, ((LPPOINT) &rc)+1);
    if(lStyle & WS_VSCROLL)
        rc.xRight ++;
    ValidateRect(hwndParent, (LPRECT) &rc);
    }

/*-----------------------------------------------------------------------
|   CTL3D Subclass Wndprocs
-----------------------------------------------------------------------*/

/* These values are assumed for bit shifting operations */
#define BFCHECK  0x0003
#define BFSTATE  0x0004
#define BFFOCUS  0x0008
#define BFINCLICK   0x0010  /* Inside click code */
#define BFCAPTURED  0x0020  /* We have mouse capture */
#define BFMOUSE  0x0040  /* Mouse-initiated */
#define BFDONTCLICK 0x0080  /* Don't check on get focus */

#define bpText  0x0002
#define bpCheck 0x0004
#define bpFocus 0x0008  // must be same as BFFOCUS
#define bpBkgnd 0x0010
#define bpEraseGroupText 0x0020

PRIVATE VOID Ctl3dDrawPushButton(HWND hwnd, HDC hdc, RC FAR *lprc, LPTSTR lpch, int cch, WORD bs, BOOL fDown)
    {
    // int dxyBrdr;
    int dxyShadow;
    HBRUSH hbrSav;
    RC rcInside;
    rcInside = *lprc;

//  if (!(grbitStyle & bitFCoolButtons))
        {
        DrawRec3d(hdc, lprc, icvWindowFrame, icvWindowFrame, dr3All);
        InflateRect((LPRECT) &rcInside, -1, -1);
        if (bs == LOWORD(BS_DEFPUSHBUTTON) && IsWindowEnabled(hwnd))
            {
            // dxyBrdr = 2;
            DrawRec3d(hdc, &rcInside, icvWindowFrame, icvWindowFrame, dr3All);
            InflateRect((LPRECT) &rcInside, -1, -1);
            }
        // else
            // dxyBrdr = 1;

        // Notch the corners
        PatBlt(hdc, lprc->xLeft, lprc->yTop, dxBorder, dyBorder, PATCOPY);
        /* Top xRight corner */
        PatBlt(hdc, lprc->xRight - dxBorder, lprc->yTop, dxBorder, dyBorder, PATCOPY);
        /* yBot xLeft corner */
        PatBlt(hdc, lprc->xLeft, lprc->yBot - dyBorder, dxBorder, dyBorder, PATCOPY);
        /* yBot xRight corner */
        PatBlt(hdc, lprc->xRight - dxBorder, lprc->yBot - dyBorder, dxBorder, dyBorder, PATCOPY);
        dxyShadow = 1 + !fDown;
        }
//  else
//      dxyShadow = 1;

    // draw upper left hilite/shadow

    if (fDown)
        hbrSav = SelectObject(hdc, g3d.brt.mpicvhbr[icvBtnShadow]);
    else
        hbrSav = SelectObject(hdc, g3d.brt.mpicvhbr[icvBtnHilite]);

    PatBlt(hdc, rcInside.xLeft, rcInside.yTop, dxyShadow,
        (rcInside.yBot - rcInside.yTop), PATCOPY);
    PatBlt(hdc, rcInside.xLeft, rcInside.yTop,
        (rcInside.xRight - rcInside.xLeft), dxyShadow, PATCOPY);

    // draw lower right shadow (only if not down)
    if (!fDown) // || (grbitStyle & bitFCoolButtons))
        {
        int i;

        if (fDown)
            SelectObject(hdc, g3d.brt.mpicvhbr[icvBtnHilite]);
        else
            SelectObject(hdc, g3d.brt.mpicvhbr[icvBtnShadow]);

        rcInside.yBot--;
        rcInside.xRight--;

        for (i = 0; i < dxyShadow; i++)
            {
         PatBlt(hdc, rcInside.xLeft, rcInside.yBot,
                rcInside.xRight - rcInside.xLeft + dxBorder, dyBorder, 
                PATCOPY);
            PatBlt(hdc, rcInside.xRight, rcInside.yTop, dxBorder,
                rcInside.yBot - rcInside.yTop, PATCOPY);
            if (i < dxyShadow-1)
                InflateRect((LPRECT) &rcInside, -dxBorder, -dyBorder);
            }
        }
    // draw the button face

    rcInside.xLeft++;
    rcInside.yTop++;

    SelectObject(hdc, g3d.brt.mpicvhbr[icvBtnFace]);
    PatBlt(hdc, rcInside.xLeft, rcInside.yTop, rcInside.xRight-rcInside.xLeft,
        rcInside.yBot - rcInside.yTop, PATCOPY);

    // Draw the durned text

    if(!IsWindowEnabled(hwnd))
        SetTextColor(hdc, g3d.clrt.rgcv[icvGrayText]);
    
    {
    int dy;
    int dx;

    MyGetTextExtent(hdc, lpch, &dx, &dy);
    rcInside.yTop += (rcInside.yBot-rcInside.yTop-dy)/2;
    rcInside.xLeft += (rcInside.xRight-rcInside.xLeft-dx)/2;
    rcInside.yBot = min(rcInside.yTop+dy, rcInside.yBot);
    rcInside.xRight = min(rcInside.xLeft+dx, rcInside.xRight);
    }

    if (fDown)
        {
        OffsetRect((LPRECT) &rcInside, 1, 1);
        rcInside.xRight = min(rcInside.xRight, lprc->xRight-3);
        rcInside.yBot = min(rcInside.yBot, lprc->yBot-3);
        }

    DrawText(hdc, lpch, cch, (LPRECT) &rcInside, DT_LEFT|DT_SINGLELINE);
    
    if (hwnd == GetFocus())
        {
        InflateRect((LPRECT) &rcInside, 1, 1);
        IntersectRect((LPRECT) &rcInside, (LPRECT) &rcInside, (LPRECT) lprc);
        DrawFocusRect(hdc, (LPRECT) &rcInside);
        }

    if (hbrSav)
        SelectObject(hdc, hbrSav);
    }


/*-----------------------------------------------------------------------
|   BtnPaint
|   
|      Paint a button
|   
|   Arguments:
|      HWND hwnd:  
|      HDC hdc: 
|      int bp: 
|      
|   Returns:
|      
-----------------------------------------------------------------------*/
PRIVATE VOID BtnPaint(HWND hwnd, HDC hdc, int bp)
    {
    RC rc;
    RC rcClient;
    HFONT hfont;
    int bs;
    int bf;
    HBRUSH hbrBtn;
    HWND hwndParent;
    int xBtnBmp;
    int yBtnBmp;
    HBITMAP hbmpSav;
    HDC hdcMem;
    TCHAR szTitle[256];
    int cch;
    BOOL fEnabled;
    BOOL fLeftText;

    bs = (int) GetWindowLong(hwnd, GWL_STYLE);
    fLeftText = (bs & Win32Or16(0x00000020, 0x0020));
    bs &= Win32Or16(0x0000001F, 0x001F);
    hwndParent = GetParent(hwnd);
    SetBkMode(hdc, OPAQUE);
    GetClientRect(hwnd, (LPRECT)&rcClient);
    rc = rcClient;
    if((hfont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0L)) != NULL)
        hfont = SelectObject(hdc, hfont);

    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    hbrBtn = SEND_COLOR_BUTTON_MESSAGE(hwndParent, hwnd, hdc);
    hbrBtn = SelectObject(hdc, hbrBtn);
    IntersectClipRect(hdc, rc.xLeft, rc.yTop, rc.xRight, rc.yBot);
    if(bp & bpBkgnd && (bs != BS_GROUPBOX))
        PatBlt(hdc, rc.xLeft, rc.yTop, rc.xRight-rc.xLeft, rc.yBot-rc.yTop, PATCOPY);

    fEnabled = IsWindowEnabled(hwnd);
    bf = (int) SendMessage(hwnd, BM_GETSTATE, 0, 0L);
    yBtnBmp = 0;
    xBtnBmp = (((bf&BFCHECK) != 0) | ((bf&BFSTATE) >> 1)) * 14;
    if (!fEnabled)
        xBtnBmp += 14*(2+((bf&BFCHECK) != 0));
    if(bp & (bpText|bpFocus) || 
            bs == BS_PUSHBUTTON || bs == BS_DEFPUSHBUTTON)
        cch = GetWindowText(hwnd, szTitle, sizeof(szTitle)Win32Only(/sizeof(TCHAR)));
    switch(bs)
        {
#ifdef DEBUG
        default:
            Assert(fFalse);
            break;
#endif
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
            Ctl3dDrawPushButton(hwnd, hdc, &rcClient, szTitle, cch, LOWORD(bs), bf & BFSTATE);
            break;

        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            yBtnBmp = 13;
            goto DrawBtn;
        case BS_3STATE:
        case BS_AUTO3STATE:
            Assert((BFSTATE >> 1) == 2);
            if((bf & BFCHECK) == 2)
                yBtnBmp = 26;
            // fall through
        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
DrawBtn:
            if(bp & bpCheck)
                {
                hdcMem = CreateCompatibleDC(hdc);
                if(hdcMem != NULL)
                    {
                    hbmpSav = SelectObject(hdcMem, g3d.hbmpCheckboxes);
                    if(hbmpSav != NULL)
                        {
                        if (fLeftText)
                            BitBlt(hdc, rc.xRight - 14, rc.yTop+(rc.yBot-rc.yTop-13)/2,
                                14, 13, hdcMem, xBtnBmp, yBtnBmp, SRCCOPY);
                        else
                            BitBlt(hdc, rc.xLeft, rc.yTop+(rc.yBot-rc.yTop-13)/2,
                                14, 13, hdcMem, xBtnBmp, yBtnBmp, SRCCOPY);
                        SelectObject(hdcMem, hbmpSav);
                        }
                    DeleteDC(hdcMem);
                    }
                }
            if(bp & bpText)
                {
                // BUG! this assumes we have only 1 hbm3dCheck type
                if (fLeftText)
                    rc.xRight = rcClient.xRight - (14+4);
                else
                    rc.xLeft = rcClient.xLeft + 14+4;
                if(!fEnabled)
                    SetTextColor(hdc, g3d.clrt.rgcv[icvGrayText]);
                DrawText(hdc, szTitle, cch, (LPRECT) &rc, DT_VCENTER|DT_LEFT|DT_SINGLELINE);
                }
            if(bp & bpFocus)
                {
                int dx;
                int dy;

                MyGetTextExtent(hdc, szTitle, &dx, &dy);
                rc.yTop = (rc.yBot-rc.yTop-dy)/2;
                rc.yBot = rc.yTop+dy;
                rc.xLeft = rcClient.xLeft;
                if (fLeftText)
                {
                    rc.xLeft = rcClient.xLeft;
                    rcClient.xRight -= (14+4);
                }
                else
                    rc.xLeft = rcClient.xLeft + (14+4);
                rc.xRight = rc.xLeft + dx;
                InflateRect((LPRECT) &rc, 1, 1);
                IntersectRect((LPRECT) &rc, (LPRECT) &rc, (LPRECT) &rcClient);
                DrawFocusRect(hdc, (LPRECT) &rc);
                }
            break;
        case BS_GROUPBOX:
            if(bp & (bpText|bpCheck))
                {
                int dy;
                int dx;

                MyGetTextExtent(hdc, szTitle, &dx, &dy);
                if (dy == 0)
                    {
                    int dxT;
                    MyGetTextExtent(hdc, TEXT("X"), &dxT, &dy);
                    }
                    
                rc.xLeft += 4;
                rc.xRight = rc.xLeft + dx + 4;
                rc.yBot = rc.yTop + dy;

                if (bp & bpEraseGroupText)
                    {
                    RC rcT;

                    rcT = rc;
                    rcT.xRight = rcClient.xRight;
                    // Hack!
                    ClientToScreen(hwnd, (LPPOINT) &rcT);
                    ClientToScreen(hwnd, ((LPPOINT) &rcT)+1);
                    ScreenToClient(hwndParent, (LPPOINT) &rcT);
                    ScreenToClient(hwndParent, ((LPPOINT) &rcT)+1);
                    InvalidateRect(hwndParent, (LPRECT) &rcT, fTrue);
                    return;
                    }

                rcClient.yTop += dy/2;
                rcClient.xRight--;
                rcClient.yBot--;
                DrawRec3d(hdc, &rcClient, icvBtnShadow, icvBtnShadow, dr3All);
                OffsetRect((LPRECT) &rcClient, 1, 1);
                DrawRec3d(hdc, &rcClient, icvBtnHilite, icvBtnHilite, dr3All);

                if(!fEnabled)
                    SetTextColor(hdc, g3d.clrt.rgcv[icvGrayText]);
                DrawText(hdc, szTitle, cch, (LPRECT) &rc, DT_LEFT|DT_SINGLELINE);
                }
            break;
        }

    SelectObject(hdc, hbrBtn);
    if(hfont != NULL)
        SelectObject(hdc, hfont);
    }

LRESULT __export _loadds WINAPI BtnWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    LONG lRet;
    LONG lStyle;
    PAINTSTRUCT ps;
    HDC hdc;
    int bf;
    int bfNew;
    int bp;

    if ( wm == WM_NCDESTROY )
    return CleanupSubclass(hwnd, wm, wParam, lParam, ctButton);

    if ( GetProp(hwnd,(LPCTSTR) g3d.aCtl3dDisable) )
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ctButton), hwnd, wm, wParam, lParam);

    switch(wm)
        {
    case WM_CHECKSUBCLASS_OLD:
    case WM_CHECKSUBCLASS:
        *(int FAR *)lParam = fTrue;
        return ctButton+1000;

    case WM_SETTEXT:
        lStyle = GetWindowLong(hwnd, GWL_STYLE);
        if ((lStyle & WS_VISIBLE) && (LOWORD(lStyle) & 0x1f) == BS_GROUPBOX)
            {
            // total hack -- if group box text length shortens then
            // we have to erase the old text.  BtnPaint will Invalidate
            // the rect of the text so everything will redraw.
            bp = bpText | bpEraseGroupText;
            }
        else
            {
            bp = bpText|bpCheck|bpBkgnd;
            }
        goto DoIt;

    case BM_SETSTATE:
    case BM_SETCHECK:
        bp = bpCheck;
        goto DoIt;
    case WM_KILLFOCUS:
        // HACK! Windows will go into an infinite loop trying to sync the
        // states of the AUTO_RADIOBUTTON in this group.  (we turn off the
        // visible bit so it gets skipped in the enumeration)
        // Disable this code by clearing the STATE bit
        if ((LOWORD(GetWindowLong(hwnd, GWL_STYLE)) & 0x1F) == BS_AUTORADIOBUTTON)
            SendMessage(hwnd, BM_SETSTATE, 0, 0L);
        bp = 0;
        goto DoIt;
    case WM_ENABLE:
        bp = bpCheck | bpText;
        goto DoIt;
    case WM_SETFOCUS:
        // HACK! if wParam == NULL we may be activated via the task manager
        // Erase background of control because a WM_ERASEBKGND messsage has not
        // arrived yet for the dialog
        // bp = wParam == (WPARAM)NULL ? (bpCheck | bpText | bpBkgnd) : (bpCheck | bpText);
        bp = bpCheck | bpText | bpBkgnd;
DoIt:
        bf = (int) SendMessage(hwnd, BM_GETSTATE, 0, 0L);
        if((lStyle = GetWindowLong(hwnd, GWL_STYLE)) & WS_VISIBLE)
            {
            if ( wm != WM_SETFOCUS )
               SetWindowLong(hwnd, GWL_STYLE, lStyle & ~(WS_VISIBLE));
            lRet = CallWindowProc(LpfnGetDefWndProc(hwnd, ctButton), hwnd, wm, wParam, lParam);

            if ( wm != WM_SETFOCUS )
               SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE)|WS_VISIBLE);
            bfNew = (int) SendMessage(hwnd, BM_GETSTATE, 0, 0L);
            if((wm != BM_SETSTATE && wm != BM_SETCHECK) ||
                bf != bfNew)
                {
                hdc = GetDC(hwnd);
                if (hdc != NULL)
                    {
                    Assert(BFFOCUS == bpFocus);
                    /* If the check state changed, redraw no matter what,
                        because it won't have during the above call to the def
                        wnd proc */
                    if ((bf & BFCHECK) != (bfNew & BFCHECK))
                        bp |= bpCheck;
                    ExcludeUpdateRgn(hdc, hwnd);
                    BtnPaint(hwnd, hdc, bp|((bf^bfNew)&BFFOCUS));
                    ReleaseDC(hwnd, hdc);
                    }
                }
            return lRet;
            }
        break;
    case WM_PAINT:
        bf = (int) SendMessage(hwnd, BM_GETSTATE, 0, 0L);
        if ((hdc = (HDC) wParam) == NULL)
            hdc = BeginPaint(hwnd, &ps);
        if(GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)
            BtnPaint(hwnd, hdc, bpText|bpCheck|(bf&BFFOCUS));
        if (wParam == (WPARAM)NULL)
            EndPaint(hwnd, &ps);
        return 0L;
        }
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ctButton), hwnd, wm, wParam, lParam);
    }


void ListEditPaint3d(HWND hwnd, BOOL fEdit, int ct)
{
    CTLID id;
    RC rc;
    HDC hdc;
    HWND hwndParent;
    LONG lStyle;
    DR3 dr3;

    if(!((lStyle = GetWindowLong(hwnd, GWL_STYLE)) & WS_VISIBLE))
        return;  
    
#ifdef IEWIN31_25
    // Don't draw the 3D border for read-only edit controls with no scroll bars and no border!
    // This case hides the edit controls borders on the IE property page.
//    if ((lStyle & ES_READONLY) && !(lStyle & (WS_BORDER | WS_VSCROLL | WS_HSCROLL)))
//        return;
#endif  //IEWIN31_25
    
    if ((ct == ctCombo && (lStyle & 0x003) == CBS_DROPDOWNLIST))
        {
        if ( SendMessage(hwnd, CB_GETDROPPEDSTATE,0,0L) )
            return;
        }

    if (fEdit)
        HideCaret(hwnd);

    GetWindowRect(hwnd, (LPRECT) &rc);        

    ScreenToClient(hwndParent = GetParent(hwnd), (LPPOINT) &rc);
    ScreenToClient(hwndParent, ((LPPOINT) &rc)+1);

    hdc = GetDC(hwndParent);

    dr3 = dr3All;

    if(lStyle & WS_HSCROLL)
        dr3 = dr3 & ~dr3Bot;

    if(lStyle & WS_VSCROLL)
        dr3 = dr3 & ~dr3Right;

    // don't draw the top if it's a listbox of a simple combo
    id = GetControlId(hwnd);
    if (id == (CTLID) (1000 + fEdit))
        {
        TCHAR szClass[cchClassMax];
        BOOL fSubclass = 666;
        int ctParent;

        // could be superclassed!
        fSubclass = 666;
        ctParent = (int)SendMessage(hwndParent, WM_CHECKSUBCLASS, 0, (LPARAM)(int FAR *)&fSubclass);
        if (fSubclass == 666)
            ctParent = (int)SendMessage(hwndParent, WM_CHECKSUBCLASS_OLD, 0, (LPARAM)(int FAR *)&fSubclass);


        // could be subclassed!
        GetClassName(hwndParent, szClass, sizeof(szClass)Win32Only(/sizeof(TCHAR)));
        if (lstrcmp(szClass, mpctcdef[ctCombo].sz) == 0 ||
            (fSubclass == fTrue && ctParent == ctCombo+1000))
            {
            HWND hwndComboParent;

            hwndComboParent = GetParent(hwndParent);

            Win16Only(GetWindowRect(hwnd, (LPRECT) &rc));
            Win16Only(ScreenToClient(hwndComboParent, (LPPOINT) &rc));
            Win16Only(ScreenToClient(hwndComboParent, ((LPPOINT) &rc)+1));

            Win32Only(MapWindowPoints(hwndParent, hwndComboParent, (POINT*)&rc, 2));

            ReleaseDC(hwndParent, hdc);
            hdc = GetDC(hwndComboParent);

            if (fEdit)
                {
                RC rcList;
                HWND hwndList;
                long style;

                style = GetWindowLong(hwndParent, GWL_STYLE);
                if (!(((style & 0x0003) == CBS_DROPDOWN)
                   || ((style & 0x0003) == CBS_DROPDOWNLIST)))
                    {
                    dr3 &= ~dr3Bot;
            
                    hwndList = GetWindow(hwndParent, GW_CHILD);
                    GetWindowRect(hwndList, (LPRECT) &rcList);        
            
                    // Some ugly shit goin' on here!
                    rc.xRight -= rcList.xRight-rcList.xLeft;
                    DrawInsetRect3d(hdc, &rc, dr3Bot|dr3HackBotRight);
                    rc.xRight += rcList.xRight-rcList.xLeft;        
                    }
                else
                    {
                    //
                    // Is the drop down on the parent down ? if so don't paint.
                    //
                    if ( SendMessage(hwndParent, CB_GETDROPPEDSTATE,0,0L) )
                        {
                        ReleaseDC(hwndComboParent, hdc);
                        ShowCaret(hwnd);
                        return;
                        }
                    }
                }
            else
                {
                rc.yTop++;
                dr3 &= ~dr3Top;
                }

            hwndParent = hwndComboParent;

            }
        }

    DrawInsetRect3d(hdc, &rc, dr3);

    if ((ct == ctCombo && (lStyle & 0x003) == CBS_DROPDOWNLIST))
        {
        rc.xLeft = rc.xRight - GetSystemMetrics(SM_CXVSCROLL);
        DrawRec3d(hdc, &rc, icvWindowFrame, icvWindowFrame, dr3Right|dr3Bot);
        Val3dCtl(hwnd);
        }
    else {
    if (lStyle & WS_VSCROLL)
        {
        int SaveLeft;

        rc.xRight++;
        DrawRec3d(hdc, &rc, icvBtnHilite, icvBtnHilite, dr3Right);
        rc.xRight--;
        SaveLeft = rc.xLeft;
        rc.xLeft = rc.xRight - GetSystemMetrics(SM_CXVSCROLL);
        DrawRec3d(hdc, &rc, icvWindowFrame, icvWindowFrame, dr3Bot);
        rc.xLeft = SaveLeft;
        }
    if (lStyle & WS_HSCROLL)
        {
        rc.yBot++;
        DrawRec3d(hdc, &rc, icvBtnHilite, icvBtnHilite, dr3Bot);
        rc.yBot--;
        rc.yTop = rc.yBot - GetSystemMetrics(SM_CXHSCROLL);
        DrawRec3d(hdc, &rc, icvWindowFrame, icvWindowFrame, dr3Right);
        }
    }
    ReleaseDC(hwndParent, hdc);
    if (fEdit)
        ShowCaret(hwnd);

#ifdef IEWIN31_25

    // The following hack draws a disabled edit control using COLOR_BTNSHADOW
    // for the text and COLOR_BTNFACE for the background.  By default windows 3.1
    // draws using gray text on the usual window color so if there is no text
    // you cannot tell that the control is disabled. You can change the background
    // color via WM_SYSCOLOR, but the text color seems fixed to gray!
    // 
    if (!IsWindowEnabled(hwnd))
    {   
        RECT rc;
        char szText[100];
        HFONT hfontOld;
        HBRUSH hBrush;
        HWND hwndCtl = hwnd;

        // For combo boxes, get the child edit control
        if (ct == ctCombo)
        {
            hwndCtl = GetWindow(hwnd, GW_CHILD);
            if (NULL == hwndCtl)
            {
                return;
            }
        }
        GetClientRect(hwndCtl, &rc);
        GetWindowText(hwndCtl, szText, sizeof(szText)-1);     

        hdc = GetDC(hwndCtl);
        SetTextColor(hdc, GetSysColor(COLOR_BTNSHADOW));
        SetBkMode(hdc, TRANSPARENT);
        hfontOld = SelectObject(hdc, GetWindowFont(hwndCtl));
        hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

        // Inset the rect to avoid overwriting the 3d border
        rc.left += 1;
        rc.top += 1;
        rc.right -= 1;
        rc.bottom -= 1;
        FillRect(hdc, &rc, hBrush);
        rc.left += 2;
        DrawText(hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, hfontOld);
        ReleaseDC(hwndCtl, hdc);
        DeleteObject(hBrush);
    }
#endif  //IEWIN31_25    

}


LONG ShareEditComboWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam, int ct)
    {
    LONG l;
    LONG style;

    if ( wm == WM_NCDESTROY )
    return CleanupSubclass(hwnd, wm, wParam, lParam, ct);

    if ( GetProp(hwnd,(LPCTSTR) g3d.aCtl3dDisable) )
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ct), hwnd, wm, wParam, lParam);

    l = CallWindowProc(LpfnGetDefWndProc(hwnd,ct), hwnd, wm, wParam, lParam);
    if (ct == ctCombo)
    {
        style = GetWindowLong(hwnd, GWL_STYLE);
        if ((style & 0x0003) == CBS_DROPDOWN)
        {
#ifdef IEWIN31_25    
            // If the combobox is moved while dropped down, hide the dropdown.
            // Otherwise things don't repaint properly Win3.1.
            if ((wm == WM_WINDOWPOSCHANGING) &&
                SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0L))
            {
                SendMessage(hwnd, CB_SHOWDROPDOWN, FALSE, 0);
            }
#endif  //IEWIN31_25    
            return l;
        }
    }

    switch(wm)
        {
    case WM_CHECKSUBCLASS_OLD:
    case WM_CHECKSUBCLASS:
        *(int FAR *)lParam = fTrue;
        return ctEdit+1000;

    case WM_SHOWWINDOW:
        if (g3d.verWindows < ver31 && wParam == 0)
            Inval3dCtl(hwnd, (WINDOWPOS FAR *) NULL);
        break;
    case WM_WINDOWPOSCHANGING:
        if (g3d.verWindows >= ver31)
            Inval3dCtl(hwnd, (WINDOWPOS FAR *) lParam);
        break;

    case WM_PAINT:
        {
        if (ct != ctCombo ||
           (((style & 0x0003) == CBS_DROPDOWN) || ((style & 0x0003) == CBS_DROPDOWNLIST)))
            ListEditPaint3d(hwnd, TRUE, ct);
        }
        break;
        }
    return l;
    }


LRESULT __export _loadds WINAPI EditWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    return ShareEditComboWndProc3d(hwnd, wm, wParam, lParam, ctEdit);
    }


LONG SharedListWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam, unsigned ct)
    {
    LONG l;

    if ( wm == WM_NCDESTROY )
    return CleanupSubclass(hwnd, wm, wParam, lParam, ct);

    if ( GetProp(hwnd,(LPCTSTR) g3d.aCtl3dDisable) )
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ct), hwnd, wm, wParam, lParam);

    switch(wm)
        {
    case WM_CHECKSUBCLASS_OLD:
    case WM_CHECKSUBCLASS:
        *(int FAR *)lParam = fTrue;
        return ctList+1000;

    case WM_SHOWWINDOW:
        if (g3d.verWindows < ver31 && wParam == 0)
            Inval3dCtl(hwnd, (WINDOWPOS FAR *) NULL);
        break;
    case WM_WINDOWPOSCHANGING:
        if (g3d.verWindows >= ver31)
            Inval3dCtl(hwnd, (WINDOWPOS FAR *) lParam);
        break;
    case WM_PAINT:
        l = CallWindowProc(LpfnGetDefWndProc(hwnd, ct), hwnd, wm, wParam, lParam);
        ListEditPaint3d(hwnd, FALSE, ct);
        return l;
    case WM_NCCALCSIZE:
        {
        RC rc;         
        RC rcNew;
        HWND hwndParent;

        // Inval3dCtl handles this case under Win 3.1
        if (g3d.verWindows >= ver31)
            break;

        GetWindowRect(hwnd, (LPRECT) &rc);        
#ifdef UNREACHABLE
        if (g3d.verWindows >= ver31)
            {
            hwndParent = GetParent(hwnd);
            ScreenToClient(hwndParent, (LPPOINT) &rc);
            ScreenToClient(hwndParent, ((LPPOINT) &rc)+1);
            }
#endif

        l = CallWindowProc(LpfnGetDefWndProc(hwnd, ct), hwnd, wm, wParam, lParam);

        rcNew = *(RC FAR *)lParam;
        InflateRect((LPRECT) &rcNew, 2, 1); // +1 for border (Should use AdjustWindowRect)
        if (rcNew.yBot < rc.yBot)
            {
            rcNew.yTop = rcNew.yBot+1;
            rcNew.yBot = rc.yBot+1;

#ifdef ALWAYS
            if (g3d.verWindows < ver31)
#endif
                {
                hwndParent = GetParent(hwnd);
                ScreenToClient(hwndParent, (LPPOINT) &rcNew);
                ScreenToClient(hwndParent, ((LPPOINT) &rcNew)+1);
                }

            InvalidateRect(hwndParent, (LPRECT) &rcNew, TRUE);
            }
        return l;
        }
        }
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ct), hwnd, wm, wParam, lParam);
    }

LRESULT __export _loadds WINAPI ListWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    return SharedListWndProc(hwnd, wm, wParam, lParam, ctList); 
    }



LRESULT __export _loadds WINAPI ComboWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    switch(wm)
        {
    case WM_CHECKSUBCLASS_OLD:
    case WM_CHECKSUBCLASS:
        *(int FAR *)lParam = fTrue;
        return ctCombo+1000;
        }

    return ShareEditComboWndProc3d(hwnd, wm, wParam, lParam, ctCombo);
    }

void StaticPrint(HWND hwnd, HDC hdc, RC FAR *lprc, LONG style)
    {
    WORD dt;
    LONG cv;
    Win16Or32(HANDLE , LPTSTR )hText;
    int TextLen;

    PatBlt(hdc, lprc->xLeft, lprc->yTop, lprc->xRight-lprc->xLeft, lprc->yBot-lprc->yTop, PATCOPY);

    TextLen = GetWindowTextLength(hwnd);

#ifndef WIN32
    hText = LocalAlloc(LPTR|LMEM_NODISCARD,(TextLen+5)*sizeof(TCHAR));
#else
    hText = _alloca((TextLen+5)*sizeof(TCHAR));
#endif
    if (hText == NULL)
        return;

    if (GetWindowText(hwnd, (NPTSTR)hText, TextLen+2*sizeof(TCHAR)) == 0)
    {
#ifndef WIN32
        LocalFree(hText);
#endif
        return;
    }
    
    if ((style & 0x000f) == SS_LEFTNOWORDWRAP)
        dt = DT_NOCLIP | DT_EXPANDTABS;
    else
        {
        dt = LOWORD(DT_NOCLIP | DT_EXPANDTABS | DT_WORDBREAK | ((style & 0x0000000f)-SS_LEFT));
        }

    if (style & SS_NOPREFIX)
        dt |= DT_NOPREFIX;

    if (style & WS_DISABLED)
        cv = SetTextColor(hdc, g3d.clrt.rgcv[icvGrayText]);

    DrawText(hdc, (NPTSTR)hText, -1, (LPRECT) lprc, dt);

#ifndef WIN32
    LocalFree(hText);
#endif

    if (style & WS_DISABLED)
        cv = SetTextColor(hdc, cv);
    }

void StaticPaint(HWND hwnd, HDC hdc)
    {
    LONG style;
    RC rc;

    style = GetWindowLong(hwnd, GWL_STYLE);
    if(!(style & WS_VISIBLE))
        return;

    GetClientRect(hwnd, (LPRECT) &rc);
    switch(style & 0x1f)
        {
    case SS_BLACKRECT:
    case SS_BLACKFRAME:  // Inset rect
        DrawRec3d(hdc, &rc, icvBtnShadow, icvBtnHilite, dr3All);
        break;
    case SS_GRAYRECT:
    case SS_GRAYFRAME:
    case 0x10:
    case 0x11:
    case 0x12:
        rc.xLeft++;
        rc.yTop++;
        DrawRec3d(hdc, &rc, icvBtnHilite, icvBtnHilite, dr3All);
        OffsetRect((LPRECT) &rc, -1, -1);
        DrawRec3d(hdc, &rc, icvBtnShadow, icvBtnShadow, dr3All);
        break;
    case SS_WHITERECT:            // outset rect
    case SS_WHITEFRAME:
        DrawRec3d(hdc, &rc, icvBtnHilite, icvBtnShadow, dr3All);
        break;
    case SS_LEFT:
    case SS_CENTER:
    case SS_RIGHT:
    case SS_LEFTNOWORDWRAP:
        {
        HANDLE hfont;
        HBRUSH hbr;

        if((hfont = (HANDLE)SendMessage(hwnd, WM_GETFONT, 0, 0L)) != NULL)
            hfont = SelectObject(hdc, hfont);
        SetBkMode(hdc, OPAQUE);

        if(( hbr = SEND_COLOR_STATIC_MESSAGE(GetParent(hwnd), hwnd, hdc)) != NULL)
            hbr = SelectObject(hdc, hbr);

        StaticPrint(hwnd, hdc, (RC FAR *)&rc, style);

        if (hfont != NULL)
            SelectObject(hdc, hfont);

        if (hbr != NULL)
            SelectObject(hdc, hbr);
        }
        break;
        }
    }


LRESULT __export _loadds WINAPI StaticWndProc3d(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
    {
    HDC hdc;
    PAINTSTRUCT ps;

    if ( wm == WM_NCDESTROY )
    return CleanupSubclass(hwnd, wm, wParam, lParam, ctStatic);

    if ( GetProp(hwnd,(LPCTSTR) g3d.aCtl3dDisable) )
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ctStatic), hwnd, wm, wParam, lParam);

    switch (wm)
        {
    case WM_CHECKSUBCLASS_OLD:
    case WM_CHECKSUBCLASS:
        *(int FAR *)lParam = fTrue;
        return ctStatic+1000;

    case WM_PAINT:
        if ((hdc = (HDC) wParam) == NULL)
            {
            hdc = BeginPaint(hwnd, &ps);
            ClipCtlDc(hwnd, hdc);
            }
        StaticPaint(hwnd, hdc);
        if (wParam == (WPARAM)NULL)
            EndPaint(hwnd, &ps);
        return 0L;
        
    case WM_ENABLE:
        hdc = GetDC(hwnd);
        ClipCtlDc(hwnd, hdc);
        StaticPaint(hwnd, hdc);
        ReleaseDC(hwnd, hdc);
        return 0L;
        }
    return CallWindowProc(LpfnGetDefWndProc(hwnd, ctStatic), hwnd, wm, wParam, lParam);
    }


/*-----------------------------------------------------------------------
|   LibMain
-----------------------------------------------------------------------*/
#ifdef WIN32
#ifdef DLL
BOOL CALLBACK LibMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
#else
#ifdef SDLL
BOOL FAR Ctl3dLibMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
#else
BOOL FAR LibMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
#endif
#endif
    {
    WORD wT;
    DWORD dwVersion;
    BOOL (WINAPI* pfnDisableThreadLibraryCalls)(HMODULE);
    HMODULE hKernel;

    switch(dwReason)
        {
    case DLL_PROCESS_ATTACH:
        // call DisableThreadLibraryCalls if available
        hKernel = GetModuleHandleA("KERNEL32.DLL");
        *(WNDPROC*)&pfnDisableThreadLibraryCalls =
            GetProcAddress(hKernel, "DisableThreadLibraryCalls");
        if (pfnDisableThreadLibraryCalls != NULL)
            (*pfnDisableThreadLibraryCalls)(hModule);

#ifdef DLL
        InitializeCriticalSection(&g_CriticalSection);
#endif
        EnterCriticalSection(&g_CriticalSection);
#ifdef SDLL
        g3d.hinstLib = g3d.hmodLib = _hModule;
#else
        g3d.hinstLib = g3d.hmodLib = hModule;
#endif

        dwVersion = (DWORD)GetVersion();
        wT = LOWORD(dwVersion);
        // get adjusted windows version
        g3d.verWindows = (LOBYTE(wT) << 8) | HIBYTE(wT);
        // Win32s or Win32 for real (Chicago reports Win32s)
        g3d.verBase =
            (dwVersion & 0x80000000) && g3d.verWindows < ver40 ? 16 : 32;

        g3d.dxFrame = GetSystemMetrics(SM_CXDLGFRAME)-dxBorder;
        g3d.dyFrame = GetSystemMetrics(SM_CYDLGFRAME)-dyBorder;
        g3d.dyCaption = GetSystemMetrics(SM_CYCAPTION);
        g3d.dxSysMenu = GetSystemMetrics(SM_CXSIZE);

        LeaveCriticalSection(&g_CriticalSection);
        }
    return  TRUE;
    }
#else
#ifdef DLL
int WINAPI LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
#else
#ifdef SDLL
int FAR Ctl3dLibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
#else
#ifdef _BORLAND
BOOL FAR PASCAL LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
#else
BOOL FAR LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
#endif
#endif
#endif
    {
    WORD wT;
#ifdef DLL

#ifdef V2
    CodeLpszDeclA(lpszCtl3d, "CTL3DV2");
#else
    CodeLpszDeclA(lpszCtl3d, "CTL3D");
#endif

    CodeLpszDeclA(lpszUser, "user.exe");
    CodeLpszDeclA(lpszSetWindowsHookEx, "SETWINDOWSHOOKEX");
    CodeLpszDeclA(lpszUnhookWindowsHookEx, "UNHOOKWINDOWSHOOKEX");
    CodeLpszDeclA(lpszCallNextHookEx, "CALLNEXTHOOKEX");
#endif

    g3d.hinstLib = hModule;
#ifdef DLL
    g3d.hmodLib = GetModuleHandle(lpszCtl3d);
#else
#ifdef SDLL
    g3d.hinstLib = _hModule;
    g3d.hmodLib = GetModuleHandle(MAKELP(0,_hModule));
#else
    g3d.hmodLib = hModule;
#endif
#endif
    wT = LOWORD( GetVersion() );
    g3d.verWindows = (LOBYTE(wT) << 8) | HIBYTE(wT);

    if ( GetWinFlags() & 0x4000 )
        g3d.verBase = 24;        // More then 16, not yet 32....WOW box on NT
    else
        g3d.verBase = 16;        // Regular old 3.1

    g3d.dxFrame = GetSystemMetrics(SM_CXDLGFRAME)-dxBorder;
    g3d.dyFrame = GetSystemMetrics(SM_CYDLGFRAME)-dyBorder;
    g3d.dyCaption = GetSystemMetrics(SM_CYCAPTION);
    g3d.dxSysMenu = GetSystemMetrics(SM_CXSIZE);

#ifdef DLL
    if (g3d.verWindows >= ver31)
        {
        HANDLE hlib;

        hlib = LoadLibrary(lpszUser);
        if (FValidLibHandle(hlib))
            {
            (WNDPROC) g3d.lpfnSetWindowsHookEx = GetProcAddress(hlib, lpszSetWindowsHookEx);
            (WNDPROC) g3d.lpfnUnhookWindowsHookEx = GetProcAddress(hlib, lpszUnhookWindowsHookEx);
            (WNDPROC) g3d.lpfnCallNextHookEx = GetProcAddress(hlib, lpszCallNextHookEx);
            FreeLibrary(hlib);
            }
        }
#endif
   return 1;
    }
#endif  // win32

// convert a RGB into a RGBQ
#define RGBQ(dw) RGB(GetBValue(dw),GetGValue(dw),GetRValue(dw))

//
//  LoadUIBitmap() - load a bitmap resource
//
//   load a bitmap resource from a resource file, converting all
//   the standard UI colors to the current user specifed ones.
//
//   this code is designed to load bitmaps used in "gray ui" or
//   "toolbar" code.
//
//   the bitmap must be a 4bpp windows 3.0 DIB, with the standard
//   VGA 16 colors.
//
//   the bitmap must be authored with the following colors
//
//      Window Text      Black     (index 0)
//      Button Shadow     gray      (index 7)
//      Button Face      lt gray   (index 8)
//      Button Highlight white     (index 15)
//      Window Color      yellow       (index 11)
//      Window Frame      green    (index 10)
//
//   Example:
//
//      hbm = LoadUIBitmap(hInstance, "TestBmp",
//          GetSysColor(COLOR_WINDOWTEXT),
//          GetSysColor(COLOR_BTNFACE),
//          GetSysColor(COLOR_BTNSHADOW),
//          GetSysColor(COLOR_BTNHIGHLIGHT),
//          GetSysColor(COLOR_WINDOW),
//          GetSysColor(COLOR_WINDOWFRAME));
//
//   Author:    JimBov, ToddLa
//
//
#ifdef WIN32

HBITMAP PASCAL  LoadUIBitmap(
    HINSTANCE      hInstance,          // EXE file to load resource from
    LPCTSTR      szName,             // name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame)           // color to use for "Window Frame"
{
    HBITMAP             hbm;
    LPBITMAPINFO        lpbi;
    HRSRC               hrsrc;
    HGLOBAL             h;
    HDC                 hdc;
    DWORD               size;

    //
    // Load the bitmap resource and make a writable copy.
    //

    hrsrc = FindResource(hInstance, szName, RT_BITMAP);
    if (!hrsrc)
        return(NULL);
    size = SizeofResource( hInstance, hrsrc );
    h = LoadResource(hInstance,hrsrc);
    if (!h)
        return(NULL);

    lpbi = ( LPBITMAPINFO ) GlobalAlloc( GPTR, size );

    if (!lpbi)
        return(NULL);

    CopyMemory( lpbi, h, size );

    *( LPCOLORREF ) &lpbi->bmiColors[0]  = RGBQ(rgbText);           // Black
    *( LPCOLORREF ) &lpbi->bmiColors[7]  = RGBQ(rgbShadow);        // gray
    *( LPCOLORREF ) &lpbi->bmiColors[8]  = RGBQ(rgbFace);          // lt gray
    *( LPCOLORREF ) &lpbi->bmiColors[15] = RGBQ(rgbHighlight);     // white
    *( LPCOLORREF ) &lpbi->bmiColors[11] = RGBQ(rgbWindow);        // yellow
    *( LPCOLORREF ) &lpbi->bmiColors[10] = RGBQ(rgbFrame);         // green

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap(hdc, &lpbi->bmiHeader, CBM_INIT, (LPBYTE)(&lpbi->bmiColors[ 16 ]),
            lpbi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);
    GlobalFree( lpbi );

    return(hbm);
}

#else

HBITMAP PASCAL  LoadUIBitmap(
        HINSTANCE       hInstance,      // EXE file to load resource from
        LPCTSTR     szName,        // name of bitmap resource
        COLORREF    rgbText,          // color to use for "Button Text"
        COLORREF    rgbFace,          // color to use for "Button Face"
        COLORREF    rgbShadow,      // color to use for "Button Shadow"
        COLORREF    rgbHighlight,     // color to use for "Button Hilight"
        COLORREF    rgbWindow,      // color to use for "Window Color"
        COLORREF    rgbFrame)        // color to use for "Window Frame"
    {
   LPBYTE           lpb;
    HBITMAP        hbm;
    LPBITMAPINFOHEADER  lpbi;
    HANDLE          h;
    HDC           hdc;
    LPDWORD        lprgb;

    h = LoadResource(hInstance,FindResource(hInstance, szName, RT_BITMAP));

    lpbi = (LPBITMAPINFOHEADER)LockResource(h);

    if (!lpbi)
       return(NULL);

#ifdef NOTNEEDEDFORCTL3D
    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
       return NULL;

    if (lpbi->biBitCount != 4)
       return NULL;
#endif

    lprgb = (LPDWORD)((LPBYTE)lpbi + (int)lpbi->biSize);
    lpb   = (LPBYTE)(lprgb + 16);

    lprgb[0]  = RGBQ(rgbText);      // Black

//  lprgb[7]  = RGBQ(rgbFace);      // lt gray
//  lprgb[8]  = RGBQ(rgbShadow);    // gray

    lprgb[7]  = RGBQ(rgbShadow);    // gray
    lprgb[8]  = RGBQ(rgbFace);      // lt gray

    lprgb[15] = RGBQ(rgbHighlight); // white
    lprgb[11] = RGBQ(rgbWindow);    // yellow
    lprgb[10] = RGBQ(rgbFrame);     // green

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap(hdc, lpbi, CBM_INIT, (LPVOID)lpb,
       (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);
    UnlockResource(h);
    FreeResource(h);

    return(hbm);
    }


#endif  // win32


/*-----------------------------------------------------------------------
|   
| DLL specific routines
|   
---------------------------------------------------------------WESC----*/

#ifndef WIN32
#ifdef DLL
/*-----------------------------------------------------------------------
|   WEP
-----------------------------------------------------------------------*/
int FAR PASCAL WEP (int wSystemExit)
    {
   return 1;
    }
#endif
#endif
