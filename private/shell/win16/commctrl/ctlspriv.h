#define STRICT

/* disable "non-standard extension" warnings in our code
 */
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif

#ifdef WIN32
#define _COMCTL32_  // for DECLSPEC_IMPORT
#define _INC_OLE
#define CONST_VTABLE
#endif

#ifdef IEWIN31_25
#undef WINVER
#define WINVER 0x030a
#endif

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#ifdef IEWIN31
#include <comctlie.h>
#else
#include <commctrl.h>
#endif
#include <port32.h>
#include <debug.h>              // DebugMsg stuff.

#ifdef IEWIN31_25
#include "windowie.h"           // 4.0 features extracted from windows.h
#else
#include <winerror.h>
#endif

#ifdef  FE_IME
#include <imm.h>
#endif

#include "mem.h"
#include "rcids.h"
#include "cstrings.h"

#ifdef IEWIN31_25
#define RECTWIDTH(rc) (rc.right - rc.left)
#define RECTHEIGHT(rc) (rc.bottom - rc.top)

//
// inside comctlie we always call _TrackMouseEvent...
//
#ifndef TrackMouseEvent
#define TrackMouseEvent _TrackMouseEvent
#endif

//
// subclassing stuff
//
typedef LRESULT (CALLBACK FAR *SUBCLASSPROC)(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT uIdSubclass, DWORD dwRefData);

BOOL SetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT uIdSubclass,
    DWORD dwRefData);
BOOL GetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT uIdSubclass,
    DWORD FAR *pdwRefData);
BOOL RemoveWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass,
    UINT uIdSubclass);
LRESULT DefSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define MoveMemory hmemcpy

// common control info stuff

typedef struct tagControlInfo {
    HWND        hwnd;
    HWND        hwndParent;
    DWORD       style;
    DWORD       dwCustom;
    BOOL        bUnicode;
    UINT        uiCodePage;
} CONTROLINFO, FAR *LPCONTROLINFO;

void FAR PASCAL CIInitialize(LPCONTROLINFO lpci, HWND hwnd, LPCREATESTRUCT lpcs);
LRESULT FAR PASCAL CIHandleNotifyFormat(LPCONTROLINFO lpci, LPARAM lParam);
DWORD NEAR PASCAL CICustomDrawNotify(LPCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd);


#define SWAP(x,y, _type)  { _type i; i = x; x = y; y = i; }
#endif //IEWIN31_25

//
// This is for 3.1 property sheet emulation
//
#define DLGC_RECURSE 0x8000

//
// This is for widened dispatch loop stuff
//
#ifdef WIN32
typedef MSG MSG32;
typedef MSG32 FAR *     LPMSG32;

#define GetMessage32(lpmsg, hwnd, min, max, f32)        GetMessage(lpmsg, hwnd, min, max)
#define PeekMessage32(lpmsg, hwnd, min, max, flags, f32)       PeekMessage(lpmsg, hwnd, min, max, flags)
#define TranslateMessage32(lpmsg, f32)  TranslateMessage(lpmsg)
#define DispatchMessage32(lpmsg, f32)   DispatchMessage(lpmsg)
#define CallMsgFilter32(lpmsg, u, f32)  CallMsgFilter(lpmsg, u)
#define IsDialogMessage32(hwnd, lpmsg, f32)   IsDialogMessage(hwnd, lpmsg)
#else

#ifdef WIN31
typedef MSG MSG32;
typedef MSG32 FAR *     LPMSG32;

#define GetMessage32(lpmsg, hwnd, min, max, f32)        GetMessage(lpmsg, hwnd, min, max)
#define PeekMessage32(lpmsg, hwnd, min, max, flags, f32)       PeekMessage(lpmsg, hwnd, min, max, flags)
#define TranslateMessage32(lpmsg, f32)  TranslateMessage(lpmsg)
#define DispatchMessage32(lpmsg, f32)   DispatchMessage(lpmsg)
#define CallMsgFilter32(lpmsg, u, f32)  CallMsgFilter(lpmsg, u)
#define IsDialogMessage32(hwnd, lpmsg, f32)   IsDialogMessage(hwnd, lpmsg)
#else

// This comes from ..\..\inc\usercmn.h--but I can't get commctrl to compile
// when I include it and I don't have the time to mess with this right now.

// DWORD wParam MSG structure
typedef struct tagMSG32
{
    HWND    hwnd;
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
    DWORD   time;
    POINT   pt;

    WPARAM  wParamHi;
} MSG32, FAR* LPMSG32;

BOOL    WINAPI GetMessage32(LPMSG32, HWND, UINT, UINT, BOOL);
BOOL    WINAPI PeekMessage32(LPMSG32, HWND, UINT, UINT, UINT, BOOL);
BOOL    WINAPI TranslateMessage32(const MSG32 FAR*, BOOL);
LONG    WINAPI DispatchMessage32(const MSG32 FAR*, BOOL);
BOOL    WINAPI CallMsgFilter32(LPMSG32, int, BOOL);
BOOL    WINAPI IsDialogMessage32(HWND, LPMSG32, BOOL);

#endif // WIN31
#endif // WIN32


//
// This is a very important piece of performance hack for non-DBCS codepage.
//
#ifndef DBCS
// NB - These are already macros in Win32 land.
#ifdef WIN32
#undef AnsiNext
#undef AnsiPrev
#endif

#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) ((x), FALSE)
#endif // DBCS

#define CH_PREFIX '&'

void FAR PASCAL InitGlobalMetrics(WPARAM);
void FAR PASCAL InitGlobalColors();
void FAR PASCAL ReInitGlobalColors();
void FAR PASCAL DestroyGlobalColors();

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance);
#ifdef IEWIN31_25
BOOL FAR PASCAL InitReBarClass(HINSTANCE hInstance);
#endif //IEWIN31_25
BOOL FAR PASCAL InitToolTipsClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitHeaderClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitButtonListBoxClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance);
BOOL FAR PASCAL InitUpDownClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitProgressClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitHotKeyClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitToolTips(HINSTANCE hInstance);


/* cutils.c */
#ifdef IEWIN31_25
BOOL CCForwardEraseBackground(HWND hwnd, HDC hdc);
#endif  //IEWIN31_25
BOOL FAR PASCAL CheckForDragBegin(HWND hwnd, int x, int y);
int FAR PASCAL GetIncrementSearchString(LPSTR lpsz);
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height);
BOOL FAR PASCAL MGetTextExtent(HDC hdc, LPCSTR lpstr, int cnt, int FAR * pcx, int FAR * pcy);
void FAR PASCAL RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
#ifdef  FE_IME
BOOL FAR PASCAL IncrementSearchImeCompStr(BOOL fCompStr, LPSTR lpszCompChar, LPSTR FAR *lplpstr);
#endif
BOOL FAR PASCAL IncrementSearchString(UINT ch, LPSTR FAR *lplpstr);
void FAR PASCAL StripAccelerators(LPSTR lpszFrom, LPSTR lpszTo);
//
// Global variables
//
extern HINSTANCE g_hinst;
extern UINT uDragListMsg;
extern int g_iIncrSearchFailed;

// Globals from toolbar.c (Ugly!)
extern const int g_dxButtonSep;
extern int g_nSelectedBM;
extern HDC g_hdcGlyphs;
extern HDC g_hdcMono;
extern HBITMAP g_hbmMono;


#define HINST_THISDLL   g_hinst

#ifdef WIN32

void Controls_EnterCriticalSection(void);
void Controls_LeaveCriticalSection(void);

#define ENTERCRITICAL   Controls_EnterCriticalSection();
#define LEAVECRITICAL   Controls_LeaveCriticalSection();

#ifdef DEBUG
extern int   g_CriticalSectionCount;
extern DWORD g_CriticalSectionOwner;
#define ASSERTCRITICAL  Assert(g_CriticalSectionCount > 0 && GetCurrentThreadId() == g_CriticalSectionOwner);
#define ASSERTNONCRITICAL  Assert(GetCurrentThreadId() != g_CriticalSectionOwner);
#undef SendMessage
#define SendMessage  SendMessageD
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#else // DEBUG
#define ASSERTCRITICAL
#define ASSERTNONCRITICAL
#endif // DEBUG

#else // WIN32
#define ENTERCRITICAL
#define LEAVECRITICAL
#endif // WIN32

// REVIEW, should this be a function? (inline may generate a lot of code)
#define CBBITMAPBITS(cx, cy, cPlanes, cBitsPerPixel)    \
        (((((cx) * (cBitsPerPixel) + 15) & ~15) >> 3)   \
        * (cPlanes) * (cy))

#define WIDTHBYTES(cx, cBitsPerPixel)   \
        ((((cx) * (cBitsPerPixel) + 31) / 32) * 4)


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))                          /* ;Internal */

void FAR PASCAL ColorDitherBrush_OnSysColorChange();
extern HBRUSH g_hbrMonoDither;              // gray dither brush from image.c
void FAR PASCAL InitDitherBrush();
void FAR PASCAL TerminateDitherBrush();



#define SHDT_DRAWTEXT       0x0001
#define SHDT_ELLIPSES       0x0002
#define SHDT_CLIPPED        0x0004
#define SHDT_SELECTED       0x0008
#define SHDT_DESELECTED     0x0010
#define SHDT_DEPRESSED      0x0020
#define SHDT_EXTRAMARGIN    0x0040
#define SHDT_TRANSPARENT    0x0080

void WINAPI SHDrawText(HDC hdc, LPCSTR pszText, RECT FAR* prc,
        int fmt, UINT flags, int cyChar, int cxEllipses,
        COLORREF clrText, COLORREF clrTextBk);


#ifdef IEWIN31_25
// notify.c
LRESULT WINAPI CCSendNotify(CONTROLINFO FAR * pci, int code, LPNMHDR pnm);
#endif //IEWIN31_25


// Global System metrics.

extern int g_cxEdge;
extern int g_cyEdge;
extern int g_cxBorder;
extern int g_cyBorder;
extern int g_cxScreen;
extern int g_cyScreen;
//extern int g_cxSmIcon;
//extern int g_cySmIcon;
//extern int g_cxIcon;
//extern int g_cyIcon;
extern int g_cxFrame;
extern int g_cyFrame;
extern int g_cxIconSpacing, g_cyIconSpacing;
extern int g_cxScrollbar, g_cyScrollbar;
extern int g_cxIconMargin, g_cyIconMargin;
extern int g_cyLabelSpace;
extern int g_cxLabelMargin;
//extern int g_cxIconOffset, g_cyIconOffset;
extern int g_cxVScroll;
extern int g_cyHScroll;
extern int g_cxHScroll;
extern int g_cyVScroll;

extern COLORREF g_clrWindow;
extern COLORREF g_clrWindowText;
extern COLORREF g_clrWindowFrame;
extern COLORREF g_clrGrayText;
extern COLORREF g_clrBtnText;
extern COLORREF g_clrBtnFace;
extern COLORREF g_clrBtnShadow;
extern COLORREF g_clrBtnHighlight;
extern COLORREF g_clrHighlight;
extern COLORREF g_clrHighlightText;
extern COLORREF g_clrInfoText;
extern COLORREF g_clrInfoBk;

extern HBRUSH g_hbrGrayText;
extern HBRUSH g_hbrWindow;
extern HBRUSH g_hbrWindowText;
extern HBRUSH g_hbrWindowFrame;
extern HBRUSH g_hbrBtnFace;
extern HBRUSH g_hbrBtnHighlight;
extern HBRUSH g_hbrBtnShadow;
extern HBRUSH g_hbrHighlight;

#ifdef WIN31

extern HBRUSH g_hbr3DDkShadow;
extern HBRUSH g_hbr3DFace;
extern HBRUSH g_hbr3DHilight;
extern HBRUSH g_hbr3DLight;
extern HBRUSH g_hbr3DShadow;
extern HBRUSH g_hbrBtnText;
extern HBRUSH g_hbrWhite;
extern HBRUSH g_hbrGray;
extern HBRUSH g_hbrBlack;

extern int g_oemInfo_Planes;
extern int g_oemInfo_BitsPixel;
extern int g_oemInfo_BitCount;

#define CXEDGE          g_cxEdge
#define CXBORDER        g_cxBorder
#define CYBORDER        g_cyBorder

#define RGB_3DFACE      g_clrBtnFace
#define RGB_3DHILIGHT   g_clrBtnHighlight
#define RGB_3DDKSHADOW  RGB(  0,   0,   0)
#define RGB_3DLIGHT     RGB(223, 223, 223)
#define RGB_WINDOWFRAME g_clrWindowFrame
#define RGB_3DSHADOW    g_clrBtnShadow

#define HBR_3DDKSHADOW  g_hbr3DDkShadow
#define HBR_3DFACE      g_hbr3DFace
#define HBR_3DHILIGHT   g_hbr3DHilight
#define HBR_3DLIGHT     g_hbr3DLight
#define HBR_3DSHADOW    g_hbr3DShadow
#define HBR_WINDOW      g_hbrWindow
#define HBR_WINDOWFRAME g_hbrWindowFrame
#define HBR_BTNTEXT     g_hbrBtnText
#define HBR_WINDOWTEXT  g_hbrWindowText
#define HBR_GRAYTEXT    g_hbrGrayText
#define hbrGray         g_hbrGray
#define hbrWhite        g_hbrWhite
#define hbrBlack        g_hbrBlack


BOOL API DrawFrameControl(HDC hdc, LPRECT lprc, UINT wType, UINT wState);
void FAR DrawPushButton(HDC hdc, LPRECT lprc, UINT state, UINT flags);


#endif

extern HFONT g_hfontSystem;

//
// Defining FULL_DEBUG makes us debug memory problems.
//
#if defined(FULL_DEBUG) && defined(WIN32)
#include <deballoc.h>
#endif // defined(FULL_DEBUG) && defined(WIN32)

#ifdef IEWIN31
    #ifndef LPTSTR
    #define LPTSTR LPSTR
    #endif
    #ifndef LPCTSTR
    #define LPCTSTR LPCSTR
    #endif
    #ifndef TCHAR
    #define TCHAR char
    #endif
    #ifndef PTSTR
    #define PTSTR LPSTR
    #endif
    #define TEXT(psz)  psz
#endif

