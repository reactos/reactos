#undef STRICT
#define STRICT

/* disable "non-standard extension" warnings in our code
 */
#ifndef RC_INVOKED
#pragma warning(disable:4001)
#endif

#ifdef WIN32
#define _COMCTL32_
#define _INC_OLE
#define _SHLWAPI_
#define CONST_VTABLE
#endif


#ifndef UNIX   // IEUNIX - build process on UNIX doen't define WINVER
#ifndef WINVER
// This stuff must run on Win95
// The NT build process already have these set as 0x0400
#define WINVER              0x0400
#endif
#endif // UNIX

#define CC_INTERNAL

#include <windows.h>

#if defined(WINNT_ENV) && !defined(WINNT) && defined(_X86)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// If we are building Win95 binaries from a NT build environment,
// then we need to special case RtlMoveMemory (hmemcpy is defined to be
// RtlMoveMemory).  On Win95, RtlMoveMemory is exported from kernel32.dll
// but on NT, RtlMoveMemory is implemented as memmove exported from
// ntdll.dll.  So, NT's winnt.h defines RtlMoveMemory as memmove,
// but Win95's winnt.h doesn't.
//
// Since we are building with NT's winnt.h, but targeting Win95,
// undefine RtlMoveMemory and offer the function proto-type.
//

#undef RtlMoveMemory

NTSYSAPI
VOID
NTAPI
RtlMoveMemory (
   VOID UNALIGNED *Destination,
   CONST VOID UNALIGNED *Source,
   DWORD Length
   );

#ifdef __cplusplus
}
#endif // __cplusplus
#endif

#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#include <commctrl.h>
#define NO_SHLWAPI_UNITHUNK     // We have our own private thunks
#include <shlwapi.h>
#include <port32.h>

#if defined(UNICODE) && !defined(WINNT)
#define UNICODE_WIN9x
#endif

#include "wrapfns.h"            // This should be first than ccstock.h
                                // unless real W api is called on Win9x
#define DISALLOW_Assert
#include <debug.h>
#include <winerror.h>
#include <ccstock.h>
#if defined(FE_IME) || !defined(WINNT)
#include <imm.h>
#endif

#include "multimon.h"   // support for multiple monitor APIs on non-mm OSes
#include "thunk.h"      // Ansi / Wide string conversions
#include "apithk.h"     // Run-time thunks for different major revs of OSes
#include "mem.h"
#include "rcids.h"
#include "cstrings.h"
#include <crtfree.h>

#ifdef MAINWIN
#include <mainwin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef DS_BIDI_RTL
#define DS_BIDI_RTL  0x8000
#endif

#ifdef FONT_LINK
//
//  CodePages
//
#define CP_OEM_437       437
#define CP_IBM852        852
#define CP_IBM866        866
#define CP_THAI          874
#define CP_JAPAN         932
#define CP_CHINA         936
#define CP_KOREA         949
#define CP_TAIWAN        950
#define CP_EASTEUROPE    1250
#define CP_RUSSIAN       1251
#define CP_WESTEUROPE    1252
#define CP_GREEK         1253
#define CP_TURKISH       1254
#define CP_HEBREW        1255
#define CP_ARABIC        1256
#define CP_BALTIC        1257
#define CP_VIETNAMESE    1258
#define CP_RUSSIANKOI8R  20866
#define CP_RUSSIANKOI8RU 21866
#define CP_ISOEASTEUROPE 28592
#define CP_ISOTURKISH    28593
#define CP_ISOBALTIC     28594
#define CP_ISORUSSIAN    28595
#define CP_ISOARABIC     28596
#define CP_ISOGREEK      28597
#define CP_JAPANNHK      50220
#define CP_JAPANESC      50221
#define CP_JAPANSIO      50222
#define CP_KOREAISO      50225
#define CP_JAPANEUC      51932
#define CP_CHINAHZ       52936
#define CP_MAC_ROMAN     10000
#define CP_MAC_JAPAN     10001
#define CP_MAC_GREEK     10006
#define CP_MAC_CYRILLIC  10007
#define CP_MAC_LATIN2    10029
#define CP_MAC_TURKISH   10081
#define CP_DEFAULT       CP_ACP
#define CP_GETDEFAULT    GetACP()
#define CP_JOHAB         1361
#define CP_SYMBOL        42
#define CP_UTF8          65001
#define CP_UTF7          65000
#define CP_UNICODELITTLE 1200
#define CP_UNICODEBIG    1201

#define OEM437_CHARSET   254
#endif   //FONT_LINK

//
// inside comctl32 we always call _TrackMouseEvent...
//
#ifndef TrackMouseEvent
#define TrackMouseEvent _TrackMouseEvent
#endif

#define DCHF_LARGE          0x00000001  // default is small
#define DCHF_TOPALIGN       0x00000002  // default is center-align
#define DCHF_HORIZONTAL     0x00000004  // default is vertical
#define DCHF_HOT            0x00000008  // default is flat
#define DCHF_PUSHED         0x00000010  // default is flat
#define DCHF_FLIPPED        0x00000020  // if horiz, default is pointing right
                                        // if vert, default is pointing up
#define DCHF_TRANSPARENT    0x00000040
#define DCHF_INACTIVE       0x00000080
#define DCHF_NOBORDER       0x00000100

extern void DrawCharButton(HDC hdc, LPRECT lprc, UINT wControlState, TCHAR ch);
extern void DrawScrollArrow(HDC hdc, LPRECT lprc, UINT wControlState);
extern void DrawChevron(HDC hdc, LPRECT lprc, DWORD dwState);

//
// BOGUS -- This are all in \win\core\access\inc32\winable.h, but it's too
// tricky to mess with the build process.  The IE guys are not enlisted in
// core, just shell, so they won't be able to build COMCTL32 if we include
// that file.
//
extern void MyNotifyWinEvent(UINT, HWND, LONG, LONG_PTR);

#define     OBJID_WINDOW        0x00000000
#define     OBJID_SYSMENU       0xFFFFFFFF
#define     OBJID_TITLEBAR      0xFFFFFFFE
#define     OBJID_MENU          0xFFFFFFFD
#define     OBJID_CLIENT        0xFFFFFFFC
#define     OBJID_VSCROLL       0xFFFFFFFB
#define     OBJID_HSCROLL       0xFFFFFFFA
#define     OBJID_SIZEGRIP      0xFFFFFFF9
#define     OBJID_CARET         0xFFFFFFF8
#define     OBJID_CURSOR        0xFFFFFFF7
#define     OBJID_ALERT         0xFFFFFFF6
#define     OBJID_SOUND         0xFFFFFFF5

#define EVENT_OBJECT_CREATE             0x8000
#define EVENT_OBJECT_DESTROY            0x8001
#define EVENT_OBJECT_SHOW               0x8002
#define EVENT_OBJECT_HIDE               0x8003
#define EVENT_OBJECT_REORDER            0x8004
#define EVENT_OBJECT_FOCUS              0x8005
#define EVENT_OBJECT_SELECTION          0x8006
#define EVENT_OBJECT_SELECTIONADD       0x8007
#define EVENT_OBJECT_SELECTIONREMOVE    0x8008
#define EVENT_OBJECT_SELECTIONWITHIN    0x8009
#define EVENT_OBJECT_STATECHANGE        0x800A
#define EVENT_OBJECT_LOCATIONCHANGE     0x800B
#define EVENT_OBJECT_NAMECHANGE         0x800C
#define EVENT_OBJECT_DESCRIPTIONCHANGE  0x800D
#define EVENT_OBJECT_VALUECHANGE        0x800E

#define EVENT_SYSTEM_SOUND              0x0001
#define EVENT_SYSTEM_ALERT              0x0002
#define EVENT_SYSTEM_SCROLLINGSTART     0x0012
#define EVENT_SYSTEM_SCROLLINGEND       0x0013

// Secret SCROLLBAR index values
#define INDEX_SCROLLBAR_SELF            0
#define INDEX_SCROLLBAR_UP              1
#define INDEX_SCROLLBAR_UPPAGE          2
#define INDEX_SCROLLBAR_THUMB           3
#define INDEX_SCROLLBAR_DOWNPAGE        4
#define INDEX_SCROLLBAR_DOWN            5

#define INDEX_SCROLLBAR_MIC             1
#define INDEX_SCROLLBAR_MAC             5

#define INDEX_SCROLLBAR_LEFT            7
#define INDEX_SCROLLBAR_LEFTPAGE        8
#define INDEX_SCROLLBAR_HORZTHUMB       9
#define INDEX_SCROLLBAR_RIGHTPAGE       10
#define INDEX_SCROLLBAR_RIGHT           11

#define INDEX_SCROLLBAR_HORIZONTAL      6
#define INDEX_SCROLLBAR_GRIP            12

#define CHILDID_SELF                    0
#define INDEXID_OBJECT                  0
#define INDEXID_CONTAINER               0

#ifndef WM_GETOBJECT
#define WM_GETOBJECT                    0x003D
#endif

#define OBJID_QUERYCLASSNAMEIDX 0xFFFFFFF4

#define MSAA_CLASSNAMEIDX_BASE 65536L

#define MSAA_CLASSNAMEIDX_STATUS     (MSAA_CLASSNAMEIDX_BASE+11)
#define MSAA_CLASSNAMEIDX_TOOLBAR    (MSAA_CLASSNAMEIDX_BASE+12)
#define MSAA_CLASSNAMEIDX_PROGRESS   (MSAA_CLASSNAMEIDX_BASE+13)
#define MSAA_CLASSNAMEIDX_ANIMATE    (MSAA_CLASSNAMEIDX_BASE+14)
#define MSAA_CLASSNAMEIDX_TAB        (MSAA_CLASSNAMEIDX_BASE+15)
#define MSAA_CLASSNAMEIDX_HOTKEY     (MSAA_CLASSNAMEIDX_BASE+16)
#define MSAA_CLASSNAMEIDX_HEADER     (MSAA_CLASSNAMEIDX_BASE+17)
#define MSAA_CLASSNAMEIDX_TRACKBAR   (MSAA_CLASSNAMEIDX_BASE+18)
#define MSAA_CLASSNAMEIDX_LISTVIEW   (MSAA_CLASSNAMEIDX_BASE+19)
#define MSAA_CLASSNAMEIDX_UPDOWN     (MSAA_CLASSNAMEIDX_BASE+22)
#define MSAA_CLASSNAMEIDX_TOOLTIPS   (MSAA_CLASSNAMEIDX_BASE+24)
#define MSAA_CLASSNAMEIDX_TREEVIEW   (MSAA_CLASSNAMEIDX_BASE+25)
//
// End BOGUS insertion from \win\core\access\inc32\winable.h
//

#ifdef MAXINT
#undef MAXINT
#endif
#define MAXINT  (int)0x7FFFFFFF
// special value for pt.y or cyLabel indicating recomputation needed
// NOTE: icon ordering code considers (RECOMPUTE, RECOMPUTE) at end
// of all icons
//
#define RECOMPUTE  (DWORD)MAXINT
#define SRECOMPUTE ((short)0x7FFF)

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
#define ABS(i)  (((i) < 0) ? -(i) : (i))
#define BOUND(x,low,high)   max(min(x, high),low)

#define LPARAM_TO_POINT(lParam, pt)       ((pt).x = LOWORD(lParam), \
                                           (pt).y = HIWORD(lParam))

// common control info stuff

typedef struct tagControlInfo {
    HWND        hwnd;
    HWND        hwndParent;
    DWORD       style;
    DWORD       dwCustom;
    BITBOOL     bUnicode : 1;
    BITBOOL     bInFakeCustomDraw:1;
    UINT        uiCodePage;
    DWORD       dwExStyle;
    LRESULT     iVersion;
#ifdef KEYBOARDCUES
    WORD        wUIState;
#endif
} CONTROLINFO, FAR *LPCONTROLINFO;

#ifdef KEYBOARDCUES
BOOL CCGetUIState(LPCONTROLINFO pControlInfo);

BOOL CCNotifyNavigationKeyUsage(LPCONTROLINFO pControlInfo, WORD wFlag);

BOOL NEAR PASCAL CCOnUIState(LPCONTROLINFO pCI, UINT uMessage, WPARAM wParam, LPARAM lParam);
#endif

BOOL CCWndProc(CONTROLINFO* pci, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
void FAR PASCAL CIInitialize(LPCONTROLINFO lpci, HWND hwnd, LPCREATESTRUCT lpcs);
LRESULT FAR PASCAL CIHandleNotifyFormat(LPCONTROLINFO lpci, LPARAM lParam);
DWORD NEAR PASCAL CICustomDrawNotify(LPCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd);
DWORD CIFakeCustomDrawNotify(LPCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd);
UINT RTLSwapLeftRightArrows(CONTROLINFO *pci, WPARAM wParam);
UINT CCSwapKeys(WPARAM wParam, UINT vk1, UINT vk2);
LPTSTR CCReturnDispInfoText(LPTSTR pszSrc, LPTSTR pszDest, UINT cchDest);

void FillRectClr(HDC hdc, LPRECT prc, COLORREF clr);


//
// helpers for drag-drop enabled controls
//
typedef LRESULT (*PFNDRAGCB)(HWND hwnd, UINT code, WPARAM wp, LPARAM lp);
#define DPX_DRAGHIT   (0)  // WP = (unused)  LP = POINTL*         ret = item id
#define DPX_GETOBJECT (1)  // LP = nmobjectnotify   ret = HRESULT
#define DPX_SELECT    (2)  // WP = item id   LP = DROPEFFECT_     ret = (unused)
#define DPX_ENTER     (3)  // WP = (unused)  LP = (unused)        ret = BOOL
#define DPX_LEAVE     (4)  // WP = (unused)  LP = (unused)        ret = (unused)


// ddproxy.cpp

DECLARE_HANDLE(HDRAGPROXY);

STDAPI_(HDRAGPROXY) CreateDragProxy(HWND hwnd, PFNDRAGCB pfn, BOOL bRegister);
STDAPI_(void) DestroyDragProxy(HDRAGPROXY hdp);
STDAPI GetDragProxyTarget(HDRAGPROXY hdp, IDropTarget **ppdtgt);
STDAPI GetItemObject(CONTROLINFO *, UINT, const IID *, LPNMOBJECTNOTIFY);


#define SWAP(x,y, _type)  { _type i; i = x; x = y; y = i; }

#if !defined(WIN32) && (defined(MAINWIN))
//
// This is for 3.1 property sheet emulation
//
#define DLGC_RECURSE 0x8000
#endif

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

#endif // WIN32


//
// This is a very important piece of performance hack for non-DBCS codepage.
//
// was !defined(DBCS) || defined(UNICODE)
#if defined(WINNT)
// NB - These are already macros in Win32 land.
#ifdef WIN32
#undef AnsiNext
#undef AnsiPrev
#endif

#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) ((x), FALSE)
#endif

// FastCharNext and FastCharPrev are like CharNext and CharPrev except that
// they don't check if you are at the beginning/end of the string.

#ifdef UNICODE
#define FastCharNext(pch) ((pch)+1)
#define FastCharPrev(pchStart, pch) ((pch)-1)
#else
#define FastCharNext        CharNext
#define FastCharPrev        CharPrev
#endif

#define CH_PREFIX TEXT('&')


#ifdef UNICODE
#define lstrfns_StrEndN         lstrfns_StrEndNW
#define ChrCmp                  ChrCmpW
#define ChrCmpI                 ChrCmpIW

#else
#define lstrfns_StrEndN         lstrfns_StrEndNA
#define ChrCmp                  ChrCmpA
#define ChrCmpI                 ChrCmpIA

#endif
BOOL ChrCmpIA(WORD w1, WORD wMatch);
BOOL ChrCmpIW(WCHAR w1, WCHAR wMatch);
void  TruncateString(char *sz, int cch); // from strings.c

void FAR PASCAL InitGlobalMetrics(WPARAM);
void FAR PASCAL InitGlobalColors();

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitReBarClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitToolTipsClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitHeaderClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitButtonListBoxClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance);
BOOL FAR PASCAL InitUpDownClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitProgressClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitHotKeyClass(HINSTANCE hInstance);
BOOL FAR PASCAL InitToolTips(HINSTANCE hInstance);
BOOL FAR PASCAL InitDateClasses(HINSTANCE hinst);

BOOL NEAR PASCAL ChildOfActiveWindow(HWND hwnd);

/* cutils.c */
HFONT CCGetHotFont(HFONT hFont, HFONT *phFontHot);
HFONT CCCreateStatusFont(void);
BOOL CCForwardEraseBackground(HWND hwnd, HDC hdc);
void CCPlaySound(LPCTSTR lpszName);
BOOL FAR PASCAL CheckForDragBegin(HWND hwnd, int x, int y);
void FAR PASCAL NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height);
BOOL FAR PASCAL MGetTextExtent(HDC hdc, LPCTSTR lpstr, int cnt, int FAR * pcx, int FAR * pcy);
void FAR PASCAL RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
void FAR PASCAL StripAccelerators(LPTSTR lpszFrom, LPTSTR lpszTo, BOOL fAmpOnly);
UINT GetCodePageForFont (HFONT hFont);
void* CCLocalReAlloc(void* p, UINT uBytes);
LONG GetMessagePosClient(HWND hwnd, LPPOINT ppt);
void FAR PASCAL FlipRect(LPRECT prc);
DWORD SetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue);
BOOL CCDrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags, LPCOLORSCHEME lpclrsc);
void CCInvalidateFrame(HWND hwnd);
void FlipPoint(LPPOINT lppt);
void CCSetInfoTipWidth(HWND hwndOwner, HWND hwndToolTips);
#define CCResetInfoTipWidth(hwndOwner, hwndToolTips) \
    SendMessage(hwndToolTips, TTM_SETMAXTIPWIDTH, 0, -1)

// Incremental search
typedef struct ISEARCHINFO {
    int iIncrSearchFailed;
    LPTSTR pszCharBuf;                  // isearch string lives here
    int cbCharBuf;                      // allocated size of pszCharBuf
    int ichCharBuf;                     // number of live chars in pszCharBuf
    DWORD timeLast;                     // time of last input event
#if defined(FE_IME) || !defined(WINNT)
    BOOL fReplaceCompChar;
#endif

} ISEARCHINFO, *PISEARCHINFO;

#if defined(FE_IME) || !defined(WINNT)
BOOL FAR PASCAL IncrementSearchImeCompStr(PISEARCHINFO pis, BOOL fCompStr, LPTSTR lpszCompChar, LPTSTR FAR *lplpstr);
#endif
BOOL FAR PASCAL IncrementSearchString(PISEARCHINFO pis, UINT ch, LPTSTR FAR *lplpstr);
int FAR PASCAL GetIncrementSearchString(PISEARCHINFO pis, LPTSTR lpsz);
int FAR PASCAL GetIncrementSearchStringA(PISEARCHINFO pis, UINT uiCodePage, LPSTR lpsz);
void FAR PASCAL IncrementSearchBeep(PISEARCHINFO pis);

#define IncrementSearchFree(pis) ((pis)->pszCharBuf ? Free((pis)->pszCharBuf) : 0)

// For RTL mirroring use
void MirrorBitmapInDC( HDC hdc , HBITMAP hbmOrig );

// Locale manipulation (prsht.c)
//
//  The "proper thread locale" is the thread locale we should
//  be using for our UI elements.
//
//  If you need to change the thread locale temporarily
//  to the proper thread locale, use
//
//  LCID lcidPrev;
//  CCSetProperThreadLocale(&lcidPrev);
//  munge munge munge
//  CCRestoreThreadLocale(lcidPrev);
//
//  If you just want to retrieve the proper thread locale,
//  call CCGetProperThreadLocale(NULL).
//
//
LCID CCGetProperThreadLocale(OPTIONAL LCID *plcidPrev);

__inline void CCSetProperThreadLocale(LCID *plcidPrev) {
    SetThreadLocale(CCGetProperThreadLocale(plcidPrev));
}

#define CCRestoreThreadLocale(lcid) SetThreadLocale(lcid)

int CCLoadStringExInternal(HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLang);
int CCLoadStringEx(UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLang);
int LocalizedLoadString(UINT uID, LPWSTR lpBuffer, int nBufferMax);
HRSRC FindResourceExRetry(HMODULE hmod, LPCTSTR lpType, LPCTSTR lpName, WORD wLang);

// assign most unlikely used value for the fake sublang id
#define SUBLANG_JAPANESE_ALTFONT 0x3f // max within 6bit

// used to get resource lang of shell32
#define DLG_EXITWINDOWS         1064

//
// Plug UI Setting funcions (commctrl.c)
//
LANGID WINAPI GetMUILanguage(void);

#ifdef UNICODE
//
// Tooltip thunking api's
//

BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW, UINT uiCodePage);

#endif

HWND GetDlgItemRect(HWND hDlg, int nIDItem, LPRECT prc);

//
// Global variables
//
extern HINSTANCE g_hinst;
extern UINT uDragListMsg;
extern int g_iIncrSearchFailed;

#ifdef WINNT
#define g_bRunOnNT TRUE
#define g_bRunOnMemphis FALSE
extern BOOL g_bRunOnNT5;
extern BOOL g_bRemoteSession;
#else
#define g_bRunOnNT FALSE
#define g_bRunOnNT5 FALSE
#define g_bRemoteSession FALSE
extern BOOL g_bRunOnMemphis;
extern BOOL g_bRunOnBiDiWin95Loc;
#endif
extern UINT g_uiACP;

//
// Is Mirroring APIs enabled (BiDi Memphis and NT5 only)
//
extern BOOL g_bMirroredOS;

#ifdef FONT_LINK
extern BOOL g_bComplexPlatform;
#endif

//
// Icon mirroring stuff
//
extern HDC g_hdc;
extern HDC g_hdcMask;


#define HINST_THISDLL   g_hinst

#ifdef WIN32

#ifdef DEBUG
#undef SendMessage
#define SendMessage  SendMessageD
#ifdef __cplusplus
extern "C"
{
#endif
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI Str_GetPtr0(LPCTSTR pszCurrent, LPTSTR pszBuf, int cchBuf);
#ifdef __cplusplus
}
#endif
#else  // !DEBUG
#define Str_GetPtr0     Str_GetPtr
#endif // DEBUG / !DEBUG

#endif // WIN32

// REVIEW, should this be a function? (inline may generate a lot of code)
#define CBBITMAPBITS(cx, cy, cPlanes, cBitsPerPixel)    \
        (((((cx) * (cBitsPerPixel) + 15) & ~15) >> 3)   \
        * (cPlanes) * (cy))

#define WIDTHBYTES(cx, cBitsPerPixel)   \
        ((((cx) * (cBitsPerPixel) + 31) / 32) * 4)

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))                          /* ;Internal */

#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))

void FAR PASCAL ColorDitherBrush_OnSysColorChange();
extern HBRUSH g_hbrMonoDither;              // gray dither brush from image.c
void FAR PASCAL InitDitherBrush();
void FAR PASCAL TerminateDitherBrush();


#ifndef DT_NOFULLWIDTHCHARBREAK
#define DT_NOFULLWIDTHCHARBREAK     0x00080000
#endif  // DT_NOFULLWIDTHCHARBREAK

#define SHDT_DRAWTEXT       0x0001
#define SHDT_ELLIPSES       0x0002
#define SHDT_CLIPPED        0x0004
#define SHDT_SELECTED       0x0008
#define SHDT_DESELECTED     0x0010
#define SHDT_DEPRESSED      0x0020
#define SHDT_EXTRAMARGIN    0x0040
#define SHDT_TRANSPARENT    0x0080
#define SHDT_SELECTNOFOCUS  0x0100
#define SHDT_HOTSELECTED    0x0200
#define SHDT_DTELLIPSIS     0x0400
#ifdef WINDOWS_ME
#define SHDT_RTLREADING     0x0800
#endif
#define SHDT_NODBCSBREAK    0x1000

void WINAPI SHDrawText(HDC hdc, LPCTSTR pszText, RECT FAR* prc,
        int fmt, UINT flags, int cyChar, int cxEllipses,
        COLORREF clrText, COLORREF clrTextBk);


// notify.c
LRESULT WINAPI CCSendNotify(CONTROLINFO * pci, int code, LPNMHDR pnm);
BOOL CCReleaseCapture(CONTROLINFO * pci);
void CCSetCapture(CONTROLINFO * pci, HWND hwndSet);


// treeview.c, listview.c for FE_IME code
LPTSTR GET_COMP_STRING(HIMC hImc, DWORD dwFlags);

// lvicon.c in-place editing
#define SEIPS_WRAP          0x0001
#ifdef DEBUG
#define SEIPS_NOSCROLL      0x0002      // Flag is used only in DEBUG
#endif
void FAR PASCAL SetEditInPlaceSize(HWND hwndEdit, RECT FAR *prc, HFONT hFont, UINT seips);
HWND FAR PASCAL CreateEditInPlaceWindow(HWND hwnd, LPCTSTR lpText, int cbText, LONG style, HFONT hFont);
void RescrollEditWindow(HWND hwndEdit);

// Global System metrics.

extern int g_cxEdge;
extern int g_cyEdge;
extern int g_cxBorder;
extern int g_cyBorder;
extern int g_cxScreen;
extern int g_cyScreen;
extern int g_cxDoubleClk;
extern int g_cyDoubleClk;

extern int g_cxSmIcon;
extern int g_cySmIcon;
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
extern int g_fDragFullWindows;
extern int g_fDBCSEnabled;
extern int g_fMEEnabled;
extern int g_fThaiEnabled;
extern int g_fDBCSInputEnabled;

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
extern COLORREF g_clr3DDkShadow;
extern COLORREF g_clr3DLight;

extern HBRUSH g_hbrGrayText;
extern HBRUSH g_hbrWindow;
extern HBRUSH g_hbrWindowText;
extern HBRUSH g_hbrWindowFrame;
extern HBRUSH g_hbrBtnFace;
extern HBRUSH g_hbrBtnHighlight;
extern HBRUSH g_hbrBtnShadow;
extern HBRUSH g_hbrHighlight;

extern HFONT g_hfontSystem;
#define WHEEL_DELTA     120
extern UINT g_msgMSWheel;
extern UINT g_ucScrollLines;
extern int  gcWheelDelta;
extern UINT g_uDragImages;

#ifdef __cplusplus
}
#endif // __cplusplus
//
// Defining FULL_DEBUG makes us debug memory problems.
//
#if defined(FULL_DEBUG) && defined(WIN32)
#include "../inc/deballoc.h"
#endif // defined(FULL_DEBUG) && defined(WIN32)

// TRACE FLAGS
//
#define TF_MONTHCAL     0x00000100  // MonthCal and DateTimePick
#define TF_BKIMAGE      0x00000200  // ListView background image
#define TF_TOOLBAR      0x00000400  // Toolbar stuff
#define TF_PAGER        0x00000800  // Pager  Stuff
#define TF_REBAR        0x00001000  // Rebar
#define TF_LISTVIEW     0x00002000  // Listview
#define TF_TREEVIEW     0x00004000  // Treeview
#define TF_STATUS       0x00008000  // Status bar

// Prototype flags
#define PTF_FLATLOOK    0x00000001  // Overall flatlook
#define PTF_NOISEARCHTO 0x00000002  // No incremental search timeout

#include <platform.h>

// Dummy union macros for code compilation on platforms not
// supporting nameless stuct/union

#ifdef NONAMELESSUNION
#define DUMMYUNION_MEMBER(member)   DUMMYUNIONNAME.member
#define DUMMYUNION2_MEMBER(member)  DUMMYUNIONNAME2.member
#define DUMMYUNION3_MEMBER(member)  DUMMYUNIONNAME3.member
#define DUMMYUNION4_MEMBER(member)  DUMMYUNIONNAME4.member
#define DUMMYUNION5_MEMBER(member)  DUMMYUNIONNAME5.member
#else
#define DUMMYUNION_MEMBER(member)    member
#define DUMMYUNION2_MEMBER(member)   member
#define DUMMYUNION3_MEMBER(member)   member
#define DUMMYUNION4_MEMBER(member)   member
#define DUMMYUNION5_MEMBER(member)   member
#endif

#ifdef  UNIX
#define ALLOC_NULLHEAP(heap, size) ControlAlloc( heap, size )
#define COLOR_STRUCT RGBQUAD
typedef struct tagRGBQUAD_COLORMAP {
    RGBQUAD from;
    RGBQUAD to;
} RGBQUAD_COLORMAP;
#else
#define ALLOC_NULLHEAP(heap, size) Alloc( size )
#define COLOR_STRUCT DWORD
#define QUAD_PART(a) ((a)##.QuadPart)
#endif
