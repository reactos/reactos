;begin_both
/*****************************************************************************\
*                                                                             *
* prsht.h - - Interface for the Windows Property Sheet Pages                  *
*                                                                             *
* Version 1.0                                                                 *
*                                                                             *
* Copyright (c) 1991-1998, Microsoft Corp.      All rights reserved.          *
*                                                                             *
\*****************************************************************************/

;end_both
#ifndef _PRSHT_H_
#define _PRSHT_H_
#ifndef _PRSHTP_H_                                                   ;internal
#define _PRSHTP_H_                                                   ;internal

#ifndef _WINRESRC_
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#else
#if (_WIN32_IE < 0x0400) && defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
#error _WIN32_IE setting conflicts with _WIN32_WINNT setting
#endif
#endif
#endif

//
// Define API decoration for direct importing of DLL references.
//  BUGBUG: Exact same block is in commctrl.h   /* ;Internal */
//
#ifndef WINCOMMCTRLAPI
#if !defined(_COMCTL32_) && defined(_WIN32)
#define WINCOMMCTRLAPI DECLSPEC_IMPORT
#else
#define WINCOMMCTRLAPI
#endif
#endif // WINCOMMCTRLAPI

#ifndef CCSIZEOF_STRUCT
#define CCSIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif

//
// For compilers that don't support nameless unions
//  BUGBUG: Exact same block is in commctrl.h   /* ;Internal */
//
#ifndef DUMMYUNIONNAME
#ifdef NONAMELESSUNION
#define DUMMYUNIONNAME   u
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#define DUMMYUNIONNAME4  u4
#define DUMMYUNIONNAME5  u5
#else
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#endif
#endif // DUMMYUNIONNAME
;begin_both

#ifdef _WIN64
#include <pshpack8.h>
#else
#include <pshpack4.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

;end_both

#ifndef SNDMSG
#ifdef __cplusplus
#ifndef _MAC
#define SNDMSG ::SendMessage
#else
#define SNDMSG ::AfxSendMessage
#endif
#else
#ifndef _MAC
#define SNDMSG SendMessage
#else
#define SNDMSG AfxSendMessage
#endif
#endif
#endif // ifndef SNDMSG

#define MAXPROPPAGES            100

struct _PSP;
typedef struct _PSP FAR* HPROPSHEETPAGE;

#ifndef MIDL_PASS
struct _PROPSHEETPAGEA;
struct _PROPSHEETPAGEW;
#endif

typedef UINT (CALLBACK FAR * LPFNPSPCALLBACKA)(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEA FAR *ppsp);
typedef UINT (CALLBACK FAR * LPFNPSPCALLBACKW)(HWND hwnd, UINT uMsg, struct _PROPSHEETPAGEW FAR *ppsp);

#ifdef UNICODE
#define LPFNPSPCALLBACK         LPFNPSPCALLBACKW
#else
#define LPFNPSPCALLBACK         LPFNPSPCALLBACKA
#endif

#define PSP_DEFAULT                0x00000000
#define PSP_DLGINDIRECT            0x00000001
#define PSP_USEHICON               0x00000002
#define PSP_USEICONID              0x00000004
#define PSP_USETITLE               0x00000008
#define PSP_RTLREADING             0x00000010

#define PSP_HASHELP                0x00000020
#define PSP_USEREFPARENT           0x00000040
#define PSP_USECALLBACK            0x00000080
#define PSP_PREMATURE              0x00000400

#if (_WIN32_IE >= 0x0400)
//----- New flags for wizard97 --------------
#define PSP_HIDEHEADER             0x00000800
#define PSP_USEHEADERTITLE         0x00001000
#define PSP_USEHEADERSUBTITLE      0x00002000
//-------------------------------------------
#endif

;begin_internal
#define PSP_SHPAGE                 0x00000200  // BUGBUG raymondc remove once docfind2.c is fixed
#define PSP_DONOTUSE               0x00000200  // Dead flag - do not recycle
#define PSP_ALL                    0x0000FFFF
#define PSP_IS16                   0x00008000
;end_internal

#if (_WIN32_IE >= 0x0500)
#define PSPCB_ADDREF            0
#endif
#define PSPCB_RELEASE           1
#define PSPCB_CREATE            2

#define PROPSHEETPAGEA_V1_SIZE CCSIZEOF_STRUCT(PROPSHEETPAGEA, pcRefParent)
#define PROPSHEETPAGEW_V1_SIZE CCSIZEOF_STRUCT(PROPSHEETPAGEW, pcRefParent)

typedef struct _PROPSHEETPAGEA {
        DWORD           dwSize;
        DWORD           dwFlags;
        HINSTANCE       hInstance;
        union {
            LPCSTR          pszTemplate;
#ifdef _WIN32
            LPCDLGTEMPLATE  pResource;
#else
            const VOID FAR *pResource;
#endif
        } DUMMYUNIONNAME;
        union {
            HICON       hIcon;
            LPCSTR      pszIcon;
        } DUMMYUNIONNAME2;
        LPCSTR          pszTitle;
        DLGPROC         pfnDlgProc;
        LPARAM          lParam;
        LPFNPSPCALLBACKA pfnCallback;
        UINT FAR * pcRefParent;

#if (_WIN32_IE >= 0x0400)
        LPCSTR pszHeaderTitle;    // this is displayed in the header
        LPCSTR pszHeaderSubTitle; //
#endif
} PROPSHEETPAGEA, FAR *LPPROPSHEETPAGEA;
typedef const PROPSHEETPAGEA FAR *LPCPROPSHEETPAGEA;

typedef struct _PROPSHEETPAGEW {
        DWORD           dwSize;
        DWORD           dwFlags;
        HINSTANCE       hInstance;
        union {
            LPCWSTR          pszTemplate;
#ifdef _WIN32
            LPCDLGTEMPLATE  pResource;
#else
            const VOID FAR *pResource;
#endif
        }DUMMYUNIONNAME;
        union {
            HICON       hIcon;
            LPCWSTR      pszIcon;
        }DUMMYUNIONNAME2;
        LPCWSTR          pszTitle;
        DLGPROC         pfnDlgProc;
        LPARAM          lParam;
        LPFNPSPCALLBACKW pfnCallback;
        UINT FAR * pcRefParent;

#if (_WIN32_IE >= 0x0400)
        LPCWSTR pszHeaderTitle;    // this is displayed in the header
        LPCWSTR pszHeaderSubTitle; ///
#endif
} PROPSHEETPAGEW, FAR *LPPROPSHEETPAGEW;
typedef const PROPSHEETPAGEW FAR *LPCPROPSHEETPAGEW;

#if 0 // IEUNIX reserved.
/* Macros for the missing definitions.  */
/* Not all, because mostly used in C++. */
#if __STDC__ || defined (NONAMELESSUNION)
#   define PSP_pszTemplate(X) ((X).u.pszTemplate)
#else
#   define PSP_pszTemplate(X) ((X).pszTemplate)
#endif
#endif

#ifdef UNICODE
#define PROPSHEETPAGE           PROPSHEETPAGEW
#define LPPROPSHEETPAGE         LPPROPSHEETPAGEW
#define LPCPROPSHEETPAGE        LPCPROPSHEETPAGEW
#define PROPSHEETPAGE_V1_SIZE PROPSHEETPAGEW_V1_SIZE
#else
#define PROPSHEETPAGE           PROPSHEETPAGEA
#define LPPROPSHEETPAGE         LPPROPSHEETPAGEA
#define LPCPROPSHEETPAGE        LPCPROPSHEETPAGEA
#define PROPSHEETPAGE_V1_SIZE PROPSHEETPAGEA_V1_SIZE
#endif


#define PSH_DEFAULT             0x00000000
#define PSH_PROPTITLE           0x00000001
#define PSH_USEHICON            0x00000002
#define PSH_USEICONID           0x00000004
#define PSH_PROPSHEETPAGE       0x00000008
#define PSH_WIZARDHASFINISH     0x00000010
#define PSH_WIZARD              0x00000020
#define PSH_USEPSTARTPAGE       0x00000040
#define PSH_NOAPPLYNOW          0x00000080
#define PSH_USECALLBACK         0x00000100
#define PSH_HASHELP             0x00000200
#define PSH_MODELESS            0x00000400
#define PSH_RTLREADING          0x00000800
#define PSH_WIZARDCONTEXTHELP   0x00001000

#if (_WIN32_IE >= 0x0400)
//----- New flags for wizard97 -----------
;begin_internal
// we are such morons.  Wiz97 underwent a redesign between IE4 and IE5
// so we have to treat them as two unrelated wizard styles that happen to
// have frighteningly similar names.
#define PSH_WIZARD97IE4         0x00002000
#define PSH_WIZARD97IE5         0x01000000
;end_internal
#if (_WIN32_IE < 0x0500)
#define PSH_WIZARD97            0x00002000
#else
#define PSH_WIZARD97            0x01000000
#endif
// 0x00004000 was not used by any previous release
#define PSH_WATERMARK           0x00008000
#define PSH_USEHBMWATERMARK     0x00010000  // user pass in a hbmWatermark instead of pszbmWatermark
#define PSH_USEHPLWATERMARK     0x00020000  //
#define PSH_STRETCHWATERMARK    0x00040000  // stretchwatermark also applies for the header
#define PSH_HEADER              0x00080000
#define PSH_USEHBMHEADER        0x00100000
#define PSH_USEPAGELANG         0x00200000  // use frame dialog template matched to page
//----------------------------------------
#endif

#if (_WIN32_IE >= 0x0500)
//----- New flags for wizard-lite --------
#define PSH_WIZARD_LITE         0x00400000
#define PSH_NOCONTEXTHELP       0x02000000
//----------------------------------------
#endif

#define PSH_THUNKED             0x00800000                               ;Internal
#define PSH_ALL                 0x03FFFFFF                               ;Internal

typedef int (CALLBACK *PFNPROPSHEETCALLBACK)(HWND, UINT, LPARAM);

#define PROPSHEETHEADERA_V1_SIZE CCSIZEOF_STRUCT(PROPSHEETHEADERA, pfnCallback)
#define PROPSHEETHEADERW_V1_SIZE CCSIZEOF_STRUCT(PROPSHEETHEADERW, pfnCallback)

typedef struct _PROPSHEETHEADERA {
        DWORD           dwSize;
        DWORD           dwFlags;
        HWND            hwndParent;
        HINSTANCE       hInstance;
        union {
            HICON       hIcon;
            LPCSTR      pszIcon;
        }DUMMYUNIONNAME;
        LPCSTR          pszCaption;

        UINT            nPages;
        union {
            UINT        nStartPage;
            LPCSTR      pStartPage;
        }DUMMYUNIONNAME2;
        union {
            LPCPROPSHEETPAGEA ppsp;
            HPROPSHEETPAGE FAR *phpage;
        }DUMMYUNIONNAME3;
        PFNPROPSHEETCALLBACK pfnCallback;

#if (_WIN32_IE >= 0x0400)
        union {
            HBITMAP hbmWatermark;
            LPCSTR pszbmWatermark;
        } DUMMYUNIONNAME4;
        HPALETTE hplWatermark;
        union {
            HBITMAP hbmHeader;     // Header  bitmap shares the palette with watermark
            LPCSTR pszbmHeader;
        } DUMMYUNIONNAME5;
#endif
} PROPSHEETHEADERA, FAR *LPPROPSHEETHEADERA;

typedef const PROPSHEETHEADERA FAR *LPCPROPSHEETHEADERA;

typedef struct _PROPSHEETHEADERW {
        DWORD           dwSize;
        DWORD           dwFlags;
        HWND            hwndParent;
        HINSTANCE       hInstance;
        union {
            HICON       hIcon;
            LPCWSTR     pszIcon;
        }DUMMYUNIONNAME;
        LPCWSTR         pszCaption;


        UINT            nPages;
        union {
            UINT        nStartPage;
            LPCWSTR     pStartPage;
        }DUMMYUNIONNAME2;
        union {
            LPCPROPSHEETPAGEW ppsp;
            HPROPSHEETPAGE FAR *phpage;
        }DUMMYUNIONNAME3;
        PFNPROPSHEETCALLBACK pfnCallback;

#if (_WIN32_IE >= 0x0400)
        union {
            HBITMAP hbmWatermark;
            LPCWSTR pszbmWatermark;
        } DUMMYUNIONNAME4;
        HPALETTE hplWatermark;
        union {
            HBITMAP hbmHeader;
            LPCWSTR pszbmHeader;
        } DUMMYUNIONNAME5;
#endif
} PROPSHEETHEADERW, FAR *LPPROPSHEETHEADERW;
typedef const PROPSHEETHEADERW FAR *LPCPROPSHEETHEADERW;

#if 0 //IEUNIX reserved.
/* Macros for the missing definitions.  */
/* Not all, because mostly used in C++. */
#if __STDC__ || defined (NONAMELESSUNION)
#   define PSH_nStartPage(X) ((X).u2.nStartPage)
#   define PSH_ppsp(X)       ((X).u3.ppsp)
#else
#   define PSH_nStartPage(X) ((X).nStartPage)
#   define PSH_ppsp(X)       ((X).ppsp)
#endif
#endif

#ifdef UNICODE
#define PROPSHEETHEADER         PROPSHEETHEADERW
#define LPPROPSHEETHEADER       LPPROPSHEETHEADERW
#define LPCPROPSHEETHEADER      LPCPROPSHEETHEADERW
#define PROPSHEETHEADER_V1_SIZE PROPSHEETHEADERW_V1_SIZE
#else
#define PROPSHEETHEADER         PROPSHEETHEADERA
#define LPPROPSHEETHEADER       LPPROPSHEETHEADERA
#define LPCPROPSHEETHEADER      LPCPROPSHEETHEADERA
#define PROPSHEETHEADER_V1_SIZE PROPSHEETHEADERA_V1_SIZE
#endif


#define PSCB_INITIALIZED  1
#define PSCB_PRECREATE    2

WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(LPCPROPSHEETPAGEA);
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW);
WINCOMMCTRLAPI BOOL           WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE);
WINCOMMCTRLAPI INT_PTR        WINAPI PropertySheetA(LPCPROPSHEETHEADERA);
WINCOMMCTRLAPI INT_PTR        WINAPI PropertySheetW(LPCPROPSHEETHEADERW);

#ifdef UNICODE
#define CreatePropertySheetPage  CreatePropertySheetPageW
#define PropertySheet            PropertySheetW
#else
#define CreatePropertySheetPage  CreatePropertySheetPageA
#define PropertySheet            PropertySheetA
#endif


#ifdef _WIN32                                                                            ;Internal
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreateProxyPage32Ex(HPROPSHEETPAGE hpage16, HINSTANCE hinst16);    ;Internal
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreateProxyPage(HPROPSHEETPAGE hpage16, HINSTANCE hinst16);        ;Internal
#endif                                                                                   ;Internal

typedef BOOL (CALLBACK FAR * LPFNADDPROPSHEETPAGE)(HPROPSHEETPAGE, LPARAM);
typedef BOOL (CALLBACK FAR * LPFNADDPROPSHEETPAGES)(LPVOID, LPFNADDPROPSHEETPAGE, LPARAM);


typedef struct _PSHNOTIFY
{
    NMHDR hdr;
    LPARAM lParam;
} PSHNOTIFY, FAR *LPPSHNOTIFY;

// these need to match shell.h's ranges                              ;Internal
#define PSN_FIRST               (0U-200U)
#define PSN_LAST                (0U-299U)


#define PSN_SETACTIVE           (PSN_FIRST-0)
#define PSN_KILLACTIVE          (PSN_FIRST-1)
// #define PSN_VALIDATE            (PSN_FIRST-1)
#define PSN_APPLY               (PSN_FIRST-2)
#define PSN_RESET               (PSN_FIRST-3)
// #define PSN_CANCEL              (PSN_FIRST-3)
#define PSN_HASHELP             (PSN_FIRST-4)                        ;Internal
#define PSN_HELP                (PSN_FIRST-5)
#define PSN_WIZBACK             (PSN_FIRST-6)
#define PSN_WIZNEXT             (PSN_FIRST-7)
#define PSN_WIZFINISH           (PSN_FIRST-8)
#define PSN_QUERYCANCEL         (PSN_FIRST-9)
#if (_WIN32_IE >= 0x0400)
#define PSN_GETOBJECT           (PSN_FIRST-10)
#endif // 0x0400
#if (_WIN32_IE >= 0x0500)
#define PSN_LASTCHANCEAPPLY     (PSN_FIRST-11)                       ;Internal
#define PSN_TRANSLATEACCELERATOR (PSN_FIRST-12)
#define PSN_QUERYINITIALFOCUS   (PSN_FIRST-13)
// Note!  If you add a new PSN_*, make sure to tell the WOW people  ;Internal
#endif // 0x0500

// Do not rely on PSNRET_INVALID because some apps return 1 for     ;Internal
// all WM_NOTIFY messages, even if they weren't handled.            ;Internal
#define PSNRET_NOERROR              0
#define PSNRET_INVALID              1
#define PSNRET_INVALID_NOCHANGEPAGE 2
#define PSNRET_MESSAGEHANDLED       3

#define PSM_SETCURSEL           (WM_USER + 101)
#define PropSheet_SetCurSel(hDlg, hpage, index) \
        SNDMSG(hDlg, PSM_SETCURSEL, (WPARAM)index, (LPARAM)hpage)


#define PSM_REMOVEPAGE          (WM_USER + 102)
#define PropSheet_RemovePage(hDlg, index, hpage) \
        SNDMSG(hDlg, PSM_REMOVEPAGE, index, (LPARAM)hpage)


#define PSM_ADDPAGE             (WM_USER + 103)
#define PropSheet_AddPage(hDlg, hpage) \
        SNDMSG(hDlg, PSM_ADDPAGE, 0, (LPARAM)hpage)


#define PSM_CHANGED             (WM_USER + 104)
#define PropSheet_Changed(hDlg, hwnd) \
        SNDMSG(hDlg, PSM_CHANGED, (WPARAM)hwnd, 0L)


#define PSM_RESTARTWINDOWS      (WM_USER + 105)
#define PropSheet_RestartWindows(hDlg) \
        SNDMSG(hDlg, PSM_RESTARTWINDOWS, 0, 0L)


#define PSM_REBOOTSYSTEM        (WM_USER + 106)
#define PropSheet_RebootSystem(hDlg) \
        SNDMSG(hDlg, PSM_REBOOTSYSTEM, 0, 0L)


#define PSM_CANCELTOCLOSE       (WM_USER + 107)
#define PropSheet_CancelToClose(hDlg) \
        PostMessage(hDlg, PSM_CANCELTOCLOSE, 0, 0L)


#define PSM_QUERYSIBLINGS       (WM_USER + 108)
#define PropSheet_QuerySiblings(hDlg, wParam, lParam) \
        SNDMSG(hDlg, PSM_QUERYSIBLINGS, wParam, lParam)


#define PSM_UNCHANGED           (WM_USER + 109)
#define PropSheet_UnChanged(hDlg, hwnd) \
        SNDMSG(hDlg, PSM_UNCHANGED, (WPARAM)hwnd, 0L)


#define PSM_APPLY               (WM_USER + 110)
#define PropSheet_Apply(hDlg) \
        SNDMSG(hDlg, PSM_APPLY, 0, 0L)


#define PSM_SETTITLEA           (WM_USER + 111)
#define PSM_SETTITLEW           (WM_USER + 120)

;begin_internal
//
// we keep PSM_DISABLEAPPLY / PSM_ENABLEAPPLY messages private,
// because we dont want random prop sheets screwing with this.
//
#define PSM_DISABLEAPPLY        (WM_USER + 122)
#define PropSheet_DisableApply(hDlg) \
        SendMessage(hDlg, PSM_DISABLEAPPLY, 0, 0L)

#define PSM_ENABLEAPPLY         (WM_USER + 123)
#define PropSheet_EnableApply(hDlg) \
        SendMessage(hDlg, PSM_ENABLEAPPLY, 0, 0L)
;end_internal

#ifdef UNICODE
#define PSM_SETTITLE            PSM_SETTITLEW
#else
#define PSM_SETTITLE            PSM_SETTITLEA
#endif

#define PropSheet_SetTitle(hDlg, wStyle, lpszText)\
        SNDMSG(hDlg, PSM_SETTITLE, wStyle, (LPARAM)(LPCTSTR)lpszText)


#define PSM_SETWIZBUTTONS       (WM_USER + 112)
#define PropSheet_SetWizButtons(hDlg, dwFlags) \
        PostMessage(hDlg, PSM_SETWIZBUTTONS, 0, (LPARAM)dwFlags)

#define PropSheet_SetWizButtonsNow(hDlg, dwFlags) PropSheet_SetWizButtons(hDlg, dwFlags)  ;Internal


#define PSWIZB_BACK             0x00000001
#define PSWIZB_NEXT             0x00000002
#define PSWIZB_FINISH           0x00000004
#define PSWIZB_DISABLEDFINISH   0x00000008


#define PSM_PRESSBUTTON         (WM_USER + 113)
#define PropSheet_PressButton(hDlg, iButton) \
        PostMessage(hDlg, PSM_PRESSBUTTON, (WPARAM)iButton, 0)


#define PSBTN_BACK              0
#define PSBTN_NEXT              1
#define PSBTN_FINISH            2
#define PSBTN_OK                3
#define PSBTN_APPLYNOW          4
#define PSBTN_CANCEL            5
#define PSBTN_HELP              6
#define PSBTN_MAX               6



#define PSM_SETCURSELID         (WM_USER + 114)
#define PropSheet_SetCurSelByID(hDlg, id) \
        SNDMSG(hDlg, PSM_SETCURSELID, 0, (LPARAM)id)


#define PSM_SETFINISHTEXTA      (WM_USER + 115)
#define PSM_SETFINISHTEXTW      (WM_USER + 121)

#ifdef UNICODE
#define PSM_SETFINISHTEXT       PSM_SETFINISHTEXTW
#else
#define PSM_SETFINISHTEXT       PSM_SETFINISHTEXTA
#endif

#define PropSheet_SetFinishText(hDlg, lpszText) \
        SNDMSG(hDlg, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText)


#define PSM_GETTABCONTROL       (WM_USER + 116)
#define PropSheet_GetTabControl(hDlg) \
        (HWND)SNDMSG(hDlg, PSM_GETTABCONTROL, 0, 0)

#define PSM_ISDIALOGMESSAGE     (WM_USER + 117)
#define PropSheet_IsDialogMessage(hDlg, pMsg) \
        (BOOL)SNDMSG(hDlg, PSM_ISDIALOGMESSAGE, 0, (LPARAM)pMsg)

#define PSM_GETCURRENTPAGEHWND  (WM_USER + 118)
#define PropSheet_GetCurrentPageHwnd(hDlg) \
        (HWND)SNDMSG(hDlg, PSM_GETCURRENTPAGEHWND, 0, 0L)

#define PSM_INSERTPAGE          (WM_USER + 119)
#define PropSheet_InsertPage(hDlg, index, hpage) \
        SNDMSG(hDlg, PSM_INSERTPAGE, (WPARAM)(index), (LPARAM)(hpage))


#if (_WIN32_IE >= 0x0500)
#define PSM_SETHEADERTITLEA     (WM_USER + 125)
#define PSM_SETHEADERTITLEW     (WM_USER + 126)

#ifdef UNICODE
#define PSM_SETHEADERTITLE      PSM_SETHEADERTITLEW
#else
#define PSM_SETHEADERTITLE      PSM_SETHEADERTITLEA
#endif

#define PropSheet_SetHeaderTitle(hDlg, index, lpszText) \
        SNDMSG(hDlg, PSM_SETHEADERTITLE, (WPARAM)(index), (LPARAM)(lpszText))


#define PSM_SETHEADERSUBTITLEA     (WM_USER + 127)
#define PSM_SETHEADERSUBTITLEW     (WM_USER + 128)

#ifdef UNICODE
#define PSM_SETHEADERSUBTITLE      PSM_SETHEADERSUBTITLEW
#else
#define PSM_SETHEADERSUBTITLE      PSM_SETHEADERSUBTITLEA
#endif

#define PropSheet_SetHeaderSubTitle(hDlg, index, lpszText) \
        SNDMSG(hDlg, PSM_SETHEADERSUBTITLE, (WPARAM)(index), (LPARAM)(lpszText))

#define PSM_HWNDTOINDEX            (WM_USER + 129)
#define PropSheet_HwndToIndex(hDlg, hwnd) \
        (int)SNDMSG(hDlg, PSM_HWNDTOINDEX, (WPARAM)(hwnd), 0)

#define PSM_INDEXTOHWND            (WM_USER + 130)
#define PropSheet_IndexToHwnd(hDlg, i) \
        (HWND)SNDMSG(hDlg, PSM_INDEXTOHWND, (WPARAM)(i), 0)

#define PSM_PAGETOINDEX            (WM_USER + 131)
#define PropSheet_PageToIndex(hDlg, hpage) \
        (int)SNDMSG(hDlg, PSM_PAGETOINDEX, 0, (LPARAM)(hpage))

#define PSM_INDEXTOPAGE            (WM_USER + 132)
#define PropSheet_IndexToPage(hDlg, i) \
        (HPROPSHEETPAGE)SNDMSG(hDlg, PSM_INDEXTOPAGE, (WPARAM)(i), 0)

#define PSM_IDTOINDEX              (WM_USER + 133)
#define PropSheet_IdToIndex(hDlg, id) \
        (int)SNDMSG(hDlg, PSM_IDTOINDEX, 0, (LPARAM)(id))

#define PSM_INDEXTOID              (WM_USER + 134)
#define PropSheet_IndexToId(hDlg, i) \
        SNDMSG(hDlg, PSM_INDEXTOID, (WPARAM)(i), 0)

#define PSM_GETRESULT              (WM_USER + 135)
#define PropSheet_GetResult(hDlg) \
        SNDMSG(hDlg, PSM_GETRESULT, 0, 0)

#define PSM_RECALCPAGESIZES        (WM_USER + 136)
#define PropSheet_RecalcPageSizes(hDlg) \
        SNDMSG(hDlg, PSM_RECALCPAGESIZES, 0, 0)
#endif // 0x0500

#define ID_PSRESTARTWINDOWS     0x2
#define ID_PSREBOOTSYSTEM       (ID_PSRESTARTWINDOWS | 0x1)


#define WIZ_CXDLG               276
#define WIZ_CYDLG               140

#define WIZ_CXBMP               80

#define WIZ_BODYX               92
#define WIZ_BODYCX              184

#define PROP_SM_CXDLG           212
#define PROP_SM_CYDLG           188

#define PROP_MED_CXDLG          227
#define PROP_MED_CYDLG          215

#define PROP_LG_CXDLG           252
#define PROP_LG_CYDLG           218

;begin_both

#ifdef __cplusplus
}
#endif

#include <poppack.h>

;end_both

#endif // _PRSHTP_H_     // ;internal
#endif  // _PRSHT_H_
