/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_PRSHT_H
#define __WINE_PRSHT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINCOMMCTRLAPI
#ifdef _COMCTL32_
# define WINCOMMCTRLAPI
#else
# define WINCOMMCTRLAPI DECLSPEC_IMPORT
#endif
#endif

#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif /* ifndef SNDMSG */

/*
 * Property sheet support (callback procs)
 */


#define WC_PROPSHEETA      "SysPropertySheet"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_PROPSHEETW     L"SysPropertySheet"
#else
static const WCHAR WC_PROPSHEETW[] = { 'S','y','s',
  'P','r','o','p','e','r','t','y','S','h','e','e','t',0 };
#endif
#define WC_PROPSHEET         WINELIB_NAME_AW(WC_PROPSHEET)

struct _PROPSHEETPAGEA;  /** need to forward declare those structs **/
struct _PROPSHEETPAGEW;
struct _PSP;
#ifndef _HPROPSHEETPAGE_DEFINED
#define _HPROPSHEETPAGE_DEFINED
typedef struct _PSP *HPROPSHEETPAGE;
#endif /* _HPROPSHEETPAGE_DEFINED */


typedef UINT (CALLBACK *LPFNPSPCALLBACKA)(HWND, UINT, struct _PROPSHEETPAGEA*);
typedef UINT (CALLBACK *LPFNPSPCALLBACKW)(HWND, UINT, struct _PROPSHEETPAGEW*);
typedef INT  (CALLBACK *PFNPROPSHEETCALLBACK)(HWND, UINT, LPARAM);
typedef BOOL (CALLBACK *LPFNADDPROPSHEETPAGE)(HPROPSHEETPAGE, LPARAM);
typedef BOOL (CALLBACK *LPFNADDPROPSHEETPAGES)(LPVOID, LPFNADDPROPSHEETPAGE, LPARAM);

/*
 * Property sheet support (structures)
 */

typedef LPCDLGTEMPLATEA PROPSHEETPAGE_RESOURCEA;
typedef LPCDLGTEMPLATEW PROPSHEETPAGE_RESOURCEW;
DECL_WINELIB_TYPE_AW(PROPSHEETPAGE_RESOURCE)

#define PROPSHEETPAGEA_V1_FIELDS           \
    DWORD              dwSize;             \
    DWORD              dwFlags;            \
    HINSTANCE          hInstance;          \
    union                                  \
    {                                      \
        const char    *pszTemplate;        \
        PROPSHEETPAGE_RESOURCEA pResource; \
    } DUMMYUNIONNAME;                      \
    union                                  \
    {                                      \
        HICON          hIcon;              \
        const char    *pszIcon;            \
    } DUMMYUNIONNAME2;                     \
    const char        *pszTitle;           \
    DLGPROC            pfnDlgProc;         \
    LPARAM             lParam;             \
    LPFNPSPCALLBACKA   pfnCallback;        \
    UINT              *pcRefParent;

typedef struct _PROPSHEETPAGEA_V1
{
    PROPSHEETPAGEA_V1_FIELDS
} PROPSHEETPAGEA_V1, *LPPROPSHEETPAGEA_V1;

typedef struct _PROPSHEETPAGEA_V2
{
    PROPSHEETPAGEA_V1_FIELDS
    const char        *pszHeaderTitle;
    const char        *pszHeaderSubTitle;
} PROPSHEETPAGEA_V2, *LPPROPSHEETPAGEA_V2;

typedef struct _PROPSHEETPAGEA_V3
{
    PROPSHEETPAGEA_V1_FIELDS
    const char        *pszHeaderTitle;
    const char        *pszHeaderSubTitle;
    HANDLE             hActCtx;
} PROPSHEETPAGEA_V3, *LPPROPSHEETPAGEA_V3;

typedef struct _PROPSHEETPAGEA
{
    PROPSHEETPAGEA_V1_FIELDS
    const char        *pszHeaderTitle;
    const char        *pszHeaderSubTitle;
    HANDLE             hActCtx;
    union
    {
        HBITMAP        hbmHeader;
        const char    *pszbmHeader;
    } DUMMYUNIONNAME3;
} PROPSHEETPAGEA, *LPPROPSHEETPAGEA,
  PROPSHEETPAGEA_V4, *LPPROPSHEETPAGEA_V4,
  PROPSHEETPAGEA_LATEST, *LPPROPSHEETPAGEA_LATEST;

typedef const PROPSHEETPAGEA_V1 *LPCPROPSHEETPAGEA_V1;
typedef const PROPSHEETPAGEA_V2 *LPCPROPSHEETPAGEA_V2;
typedef const PROPSHEETPAGEA_V3 *LPCPROPSHEETPAGEA_V3;
typedef const PROPSHEETPAGEA_V4 *LPCPROPSHEETPAGEA, *LPCPROPSHEETPAGEA_V4, *LPCPROPSHEETPAGEA_LATEST;
#define PROPSHEETPAGEA_V1_SIZE sizeof(PROPSHEETPAGEA_V1)
#define PROPSHEETPAGEA_V2_SIZE sizeof(PROPSHEETPAGEA_V2)
#define PROPSHEETPAGEA_V3_SIZE sizeof(PROPSHEETPAGEA_V3)
#define PROPSHEETPAGEA_V4_SIZE sizeof(PROPSHEETPAGEA_V4)

#define PROPSHEETPAGEW_V1_FIELDS           \
    DWORD              dwSize;             \
    DWORD              dwFlags;            \
    HINSTANCE          hInstance;          \
    union                                  \
    {                                      \
        const WCHAR   *pszTemplate;        \
        PROPSHEETPAGE_RESOURCEW pResource; \
    } DUMMYUNIONNAME;                      \
    union                                  \
    {                                      \
        HICON          hIcon;              \
        const WCHAR   *pszIcon;            \
    } DUMMYUNIONNAME2;                     \
    const WCHAR       *pszTitle;           \
    DLGPROC            pfnDlgProc;         \
    LPARAM             lParam;             \
    LPFNPSPCALLBACKW   pfnCallback;        \
    UINT              *pcRefParent;

typedef struct _PROPSHEETPAGEW_V1
{
    PROPSHEETPAGEW_V1_FIELDS
} PROPSHEETPAGEW_V1, *LPPROPSHEETPAGEW_V1;

typedef struct _PROPSHEETPAGEW_V2
{
    PROPSHEETPAGEW_V1_FIELDS
    const WCHAR       *pszHeaderTitle;
    const WCHAR       *pszHeaderSubTitle;
} PROPSHEETPAGEW_V2, *LPPROPSHEETPAGEW_V2;

typedef struct _PROPSHEETPAGEW_V3
{
    PROPSHEETPAGEW_V1_FIELDS
    const WCHAR       *pszHeaderTitle;
    const WCHAR       *pszHeaderSubTitle;
    HANDLE             hActCtx;
} PROPSHEETPAGEW_V3, *LPPROPSHEETPAGEW_V3;

typedef struct _PROPSHEETPAGEW
{
    PROPSHEETPAGEW_V1_FIELDS
    const WCHAR       *pszHeaderTitle;
    const WCHAR       *pszHeaderSubTitle;
    HANDLE             hActCtx;
    union
    {
        HBITMAP        hbmHeader;
        const WCHAR   *pszbmHeader;
    } DUMMYUNIONNAME3;
} PROPSHEETPAGEW, *LPPROPSHEETPAGEW,
  PROPSHEETPAGEW_V4, *LPPROPSHEETPAGEW_V4,
  PROPSHEETPAGEW_LATEST, *LPPROPSHEETPAGEW_LATEST;

typedef const PROPSHEETPAGEW_V1 *LPCPROPSHEETPAGEW_V1;
typedef const PROPSHEETPAGEW_V2 *LPCPROPSHEETPAGEW_V2;
typedef const PROPSHEETPAGEW_V3 *LPCPROPSHEETPAGEW_V3;
typedef const PROPSHEETPAGEW *LPCPROPSHEETPAGEW, *LPCPROPSHEETPAGEW_V4, *LPCPROPSHEETPAGEW_LATEST;
#define PROPSHEETPAGEW_V1_SIZE sizeof(PROPSHEETPAGEW_V1)
#define PROPSHEETPAGEW_V2_SIZE sizeof(PROPSHEETPAGEW_V2)
#define PROPSHEETPAGEW_V3_SIZE sizeof(PROPSHEETPAGEW_V3)
#define PROPSHEETPAGEW_V4_SIZE sizeof(PROPSHEETPAGEW_V4)


typedef struct _PROPSHEETHEADERA
{
    DWORD                    dwSize;
    DWORD                    dwFlags;
    HWND                   hwndParent;
    HINSTANCE              hInstance;
    union
    {
      HICON                  hIcon;
      LPCSTR                   pszIcon;
    }DUMMYUNIONNAME;
    LPCSTR                   pszCaption;
    UINT                   nPages;
    union
    {
        UINT                 nStartPage;
        LPCSTR                 pStartPage;
    }DUMMYUNIONNAME2;
    union
    {
        LPCPROPSHEETPAGEA    ppsp;
        HPROPSHEETPAGE*        phpage;
    }DUMMYUNIONNAME3;
    PFNPROPSHEETCALLBACK   pfnCallback;
    union
    {
        HBITMAP              hbmWatermark;
        LPCSTR                 pszbmWatermark;
    }DUMMYUNIONNAME4;
    HPALETTE               hplWatermark;
    union
    {
        HBITMAP              hbmHeader;
        LPCSTR                 pszbmHeader;
    }DUMMYUNIONNAME5;
} PROPSHEETHEADERA, *LPPROPSHEETHEADERA;

typedef const PROPSHEETHEADERA *LPCPROPSHEETHEADERA;
#define PROPSHEETHEADERA_V1_SIZE CCSIZEOF_STRUCT(PROPSHEETHEADERA, pfnCallback)
#define PROPSHEETHEADERA_V2_SIZE sizeof(PROPSHEETHEADERA)

typedef struct _PROPSHEETHEADERW
{
    DWORD                    dwSize;
    DWORD                    dwFlags;
    HWND                   hwndParent;
    HINSTANCE              hInstance;
    union
    {
      HICON                  hIcon;
      LPCWSTR                   pszIcon;
    }DUMMYUNIONNAME;
    LPCWSTR                  pszCaption;
    UINT                   nPages;
    union
    {
        UINT                 nStartPage;
        LPCWSTR                pStartPage;
    }DUMMYUNIONNAME2;
    union
    {
        LPCPROPSHEETPAGEW    ppsp;
        HPROPSHEETPAGE*        phpage;
    }DUMMYUNIONNAME3;
    PFNPROPSHEETCALLBACK   pfnCallback;
    union
    {
        HBITMAP              hbmWatermark;
        LPCWSTR                pszbmWatermark;
    }DUMMYUNIONNAME4;
    HPALETTE               hplWatermark;
    union
    {
        HBITMAP              hbmHeader;
        LPCWSTR                pszbmHeader;
    }DUMMYUNIONNAME5;
} PROPSHEETHEADERW, *LPPROPSHEETHEADERW;

typedef const PROPSHEETHEADERW *LPCPROPSHEETHEADERW;
#define PROPSHEETHEADERW_V1_SIZE CCSIZEOF_STRUCT(PROPSHEETHEADERW, pfnCallback)
#define PROPSHEETHEADERW_V2_SIZE sizeof(PROPSHEETHEADERW)


/*
 * Property sheet support (methods)
 */
WINCOMMCTRLAPI INT_PTR WINAPI PropertySheetA(LPCPROPSHEETHEADERA);
WINCOMMCTRLAPI INT_PTR WINAPI PropertySheetW(LPCPROPSHEETHEADERW);
#define PropertySheet WINELIB_NAME_AW(PropertySheet)
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(LPCPROPSHEETPAGEA);
WINCOMMCTRLAPI HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW);
#define CreatePropertySheetPage WINELIB_NAME_AW(CreatePropertySheetPage)
WINCOMMCTRLAPI BOOL WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE hPropPage);

/*
 * Property sheet support (UNICODE-Winelib)
 */

DECL_WINELIB_TYPE_AW(PROPSHEETPAGE)
DECL_WINELIB_TYPE_AW(LPPROPSHEETPAGE)
DECL_WINELIB_TYPE_AW(LPCPROPSHEETPAGE)
DECL_WINELIB_TYPE_AW(PROPSHEETHEADER)
DECL_WINELIB_TYPE_AW(LPPROPSHEETHEADER)
DECL_WINELIB_TYPE_AW(LPCPROPSHEETHEADER)
DECL_WINELIB_TYPE_AW(LPFNPSPCALLBACK)

#ifdef WINE_NO_UNICODE_MACROS
# define PRSHT_NAME_AW(base, suffix)                  \
    base##_##suffix##_must_use_W_or_A_in_this_context \
    base##_##suffix##_must_use_W_or_A_in_this_context
# define DECL_PRSHT_TYPE_AW(base, suffix)  /* nothing */
#else  /* WINE_NO_UNICODE_MACROS */
# ifdef UNICODE
#  define PRSHT_NAME_AW(base, suffix) base##W_##suffix
# else
#  define PRSHT_NAME_AW(base, suffix) base##A_##suffix
# endif
# define DECL_PRSHT_TYPE_AW(base, suffix)  typedef PRSHT_NAME_AW(base, suffix) base##_##suffix;
#endif  /* WINE_NO_UNICODE_MACROS */

DECL_PRSHT_TYPE_AW(PROPSHEETPAGE, V1)
DECL_PRSHT_TYPE_AW(LPPROPSHEETPAGE, V1)
DECL_PRSHT_TYPE_AW(LPCPROPSHEETPAGE, V1)
DECL_PRSHT_TYPE_AW(PROPSHEETPAGE, V2)
DECL_PRSHT_TYPE_AW(LPPROPSHEETPAGE, V2)
DECL_PRSHT_TYPE_AW(LPCPROPSHEETPAGE, V2)
DECL_PRSHT_TYPE_AW(PROPSHEETPAGE, V3)
DECL_PRSHT_TYPE_AW(LPPROPSHEETPAGE, V3)
DECL_PRSHT_TYPE_AW(LPCPROPSHEETPAGE, V3)
DECL_PRSHT_TYPE_AW(PROPSHEETPAGE, V4)
DECL_PRSHT_TYPE_AW(LPPROPSHEETPAGE, V4)
DECL_PRSHT_TYPE_AW(LPCPROPSHEETPAGE, V4)
DECL_PRSHT_TYPE_AW(PROPSHEETPAGE, LATEST)
DECL_PRSHT_TYPE_AW(LPPROPSHEETPAGE, LATEST)
DECL_PRSHT_TYPE_AW(LPCPROPSHEETPAGE, LATEST)
#define PROPSHEETPAGE_V1_SIZE    PRSHT_NAME_AW(PROPSHEETPAGE, V1_SIZE)
#define PROPSHEETPAGE_V2_SIZE    PRSHT_NAME_AW(PROPSHEETPAGE, V2_SIZE)
#define PROPSHEETPAGE_V3_SIZE    PRSHT_NAME_AW(PROPSHEETPAGE, V3_SIZE)
#define PROPSHEETPAGE_V4_SIZE    PRSHT_NAME_AW(PROPSHEETPAGE, V4_SIZE)
#define PROPSHEETHEADER_V1_SIZE  PRSHT_NAME_AW(PROPSHEETHEADER, V1_SIZE)
#define PROPSHEETHEADER_V2_SIZE  PRSHT_NAME_AW(PROPSHEETHEADER, V2_SIZE)

#undef PRSHT_NAME_AW
#undef DECL_PRSHT_TYPE_AW

/*
 * Property sheet support (defines)
 */
#define PSP_DEFAULT             0x0000
#define PSP_DLGINDIRECT         0x0001
#define PSP_USEHICON            0x0002
#define PSP_USEICONID           0x0004
#define PSP_USETITLE            0x0008
#define PSP_RTLREADING          0x0010

#define PSP_HASHELP             0x0020
#define PSP_USEREFPARENT        0x0040
#define PSP_USECALLBACK         0x0080
#define PSP_PREMATURE           0x0400

#define PSP_HIDEHEADER          0x00000800
#define PSP_USEHEADERTITLE      0x00001000
#define PSP_USEHEADERSUBTITLE   0x00002000
#define PSP_USEFUSIONCONTEXT    0x00004000
#define PSP_COMMANDLINKS        0x00040000

#define PSPCB_ADDREF            0
#define PSPCB_RELEASE           1
#define PSPCB_CREATE            2

#define PSH_DEFAULT             0x0000
#define PSH_PROPTITLE           0x0001
#define PSH_USEHICON            0x0002
#define PSH_USEICONID           0x0004
#define PSH_PROPSHEETPAGE       0x0008
#define PSH_WIZARDHASFINISH     0x0010
#define PSH_WIZARD              0x0020
#define PSH_USEPSTARTPAGE       0x0040
#define PSH_NOAPPLYNOW          0x0080
#define PSH_USECALLBACK         0x0100
#define PSH_HASHELP             0x0200
#define PSH_MODELESS            0x0400
#define PSH_RTLREADING          0x0800
#define PSH_WIZARDCONTEXTHELP   0x00001000

#define PSH_WIZARD97_OLD        0x00002000 /* for IE < 5 */
#define PSH_AEROWIZARD          0x00004000
#define PSH_WATERMARK           0x00008000
#define PSH_USEHBMWATERMARK     0x00010000
#define PSH_USEHPLWATERMARK     0x00020000
#define PSH_STRETCHWATERMARK    0x00040000
#define PSH_HEADER              0x00080000
#define PSH_USEHBMHEADER        0x00100000
#define PSH_USEPAGELANG         0x00200000
#define PSH_WIZARD_LITE         0x00400000
#define PSH_WIZARD97_NEW        0x01000000 /* for IE >= 5 */
#define PSH_NOCONTEXTHELP       0x02000000
#define PSH_RESIZABLE           0x04000000
#define PSH_HEADERBITMAP        0x08000000
#define PSH_NOMARGIN            0x10000000
//#ifndef __WINESRC__
# if defined(_WIN32_IE) && (_WIN32_IE < 0x0500)
#  define PSH_WIZARD97          PSH_WIZARD97_OLD
# else
#  define PSH_WIZARD97          PSH_WIZARD97_NEW
# endif
//#endif

#define PSCB_INITIALIZED  1
#define PSCB_PRECREATE    2
#if (NTDDI_VERSION >= NTDDI_WINXP)
#define PSCB_BUTTONPRESSED 3
#endif

typedef struct _PSHNOTIFY
{
   NMHDR hdr;
   LPARAM lParam;
} PSHNOTIFY, *LPPSHNOTIFY;

#define PSN_FIRST               (0U-200U)
#define PSN_LAST                (0U-299U)


#define PSN_SETACTIVE           (PSN_FIRST-0)
#define PSN_KILLACTIVE          (PSN_FIRST-1)
/* #define PSN_VALIDATE            (PSN_FIRST-1) */
#define PSN_APPLY               (PSN_FIRST-2)
#define PSN_RESET               (PSN_FIRST-3)
/* #define PSN_CANCEL              (PSN_FIRST-3) */
#define PSN_HELP                (PSN_FIRST-5)
#define PSN_WIZBACK             (PSN_FIRST-6)
#define PSN_WIZNEXT             (PSN_FIRST-7)
#define PSN_WIZFINISH           (PSN_FIRST-8)
#define PSN_QUERYCANCEL         (PSN_FIRST-9)
#define PSN_GETOBJECT           (PSN_FIRST-10)
#define PSN_TRANSLATEACCELERATOR (PSN_FIRST-12)
#define PSN_QUERYINITIALFOCUS   (PSN_FIRST-13)

#define PSNRET_NOERROR              0
#define PSNRET_INVALID              1
#define PSNRET_INVALID_NOCHANGEPAGE 2


#define PSM_SETCURSEL           (WM_USER + 101)
#define PSM_REMOVEPAGE          (WM_USER + 102)
#define PSM_ADDPAGE             (WM_USER + 103)
#define PSM_CHANGED             (WM_USER + 104)
#define PSM_RESTARTWINDOWS      (WM_USER + 105)
#define PSM_REBOOTSYSTEM        (WM_USER + 106)
#define PSM_CANCELTOCLOSE       (WM_USER + 107)
#define PSM_QUERYSIBLINGS       (WM_USER + 108)
#define PSM_UNCHANGED           (WM_USER + 109)
#define PSM_APPLY               (WM_USER + 110)
#define PSM_SETTITLEA         (WM_USER + 111)
#define PSM_SETTITLEW         (WM_USER + 120)
#define PSM_SETTITLE WINELIB_NAME_AW(PSM_SETTITLE)
#define PSM_SETWIZBUTTONS       (WM_USER + 112)
#define PSM_PRESSBUTTON         (WM_USER + 113)
#define PSM_SETCURSELID         (WM_USER + 114)
#define PSM_SETFINISHTEXTA    (WM_USER + 115)
#define PSM_SETFINISHTEXTW    (WM_USER + 121)
#define PSM_SETFINISHTEXT WINELIB_NAME_AW(PSM_SETFINISHTEXT)
#define PSM_GETTABCONTROL       (WM_USER + 116)
#define PSM_ISDIALOGMESSAGE     (WM_USER + 117)
#define PSM_GETCURRENTPAGEHWND  (WM_USER + 118)
#define PSM_INSERTPAGE          (WM_USER + 119)
#define PSM_SETHEADERTITLEA     (WM_USER + 125)
#define PSM_SETHEADERTITLEW     (WM_USER + 126)
#define PSM_SETHEADERTITLE      WINELIB_NAME_AW(PSM_SETHEADERTITLE)
#define PSM_SETHEADERSUBTITLEA  (WM_USER + 127)
#define PSM_SETHEADERSUBTITLEW  (WM_USER + 128)
#define PSM_SETHEADERSUBTITLE   WINELIB_NAME_AW(PSM_SETHEADERSUBTITLE)
#define PSM_HWNDTOINDEX         (WM_USER + 129)
#define PSM_INDEXTOHWND         (WM_USER + 130)
#define PSM_PAGETOINDEX         (WM_USER + 131)
#define PSM_INDEXTOPAGE         (WM_USER + 132)
#define PSM_IDTOINDEX           (WM_USER + 133)
#define PSM_INDEXTOID           (WM_USER + 134)
#define PSM_GETRESULT           (WM_USER + 135)
#define PSM_RECALCPAGESIZES     (WM_USER + 136)

#define PSWIZB_BACK             0x00000001
#define PSWIZB_NEXT             0x00000002
#define PSWIZB_FINISH           0x00000004
#define PSWIZB_DISABLEDFINISH   0x00000008

#define PSBTN_BACK              0
#define PSBTN_NEXT              1
#define PSBTN_FINISH            2
#define PSBTN_OK                3
#define PSBTN_APPLYNOW          4
#define PSBTN_CANCEL            5
#define PSBTN_HELP              6
#define PSBTN_MAX               6

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

/*
 * Property sheet support (macros)
 */

#define PropSheet_SetCurSel(hDlg, hpage, index) \
	SendMessageA(hDlg, PSM_SETCURSEL, (WPARAM)index, (LPARAM)hpage)

#define PropSheet_RemovePage(hDlg, index, hpage) \
	SNDMSG(hDlg, PSM_REMOVEPAGE, index, (LPARAM)hpage)

#define PropSheet_AddPage(hDlg, hpage) \
	SNDMSG(hDlg, PSM_ADDPAGE, 0, (LPARAM)hpage)

#define PropSheet_Changed(hDlg, hwnd) \
        SNDMSG(hDlg, PSM_CHANGED, (WPARAM)hwnd, 0)

#define PropSheet_RestartWindows(hDlg) \
        SNDMSG(hDlg, PSM_RESTARTWINDOWS, 0, 0)

#define PropSheet_RebootSystem(hDlg) \
        SNDMSG(hDlg, PSM_REBOOTSYSTEM, 0, 0)

#define PropSheet_CancelToClose(hDlg) \
        PostMessage(hDlg, PSM_CANCELTOCLOSE, 0, 0)

#define PropSheet_QuerySiblings(hDlg, wParam, lParam) \
	SNDMSG(hDlg, PSM_QUERYSIBLINGS, wParam, lParam)

#define PropSheet_UnChanged(hDlg, hwnd) \
        SNDMSG(hDlg, PSM_UNCHANGED, (WPARAM)hwnd, 0)

#define PropSheet_Apply(hDlg) \
        SNDMSG(hDlg, PSM_APPLY, 0, 0)

#define PropSheet_SetTitle(hDlg, wStyle, lpszText)\
	SNDMSG(hDlg, PSM_SETTITLE, wStyle, (LPARAM)(LPCTSTR)lpszText)

#define PropSheet_SetWizButtons(hDlg, dwFlags) \
	PostMessage(hDlg, PSM_SETWIZBUTTONS, 0, (LPARAM)dwFlags)

#define PropSheet_PressButton(hDlg, iButton) \
	PostMessage(hDlg, PSM_PRESSBUTTON, (WPARAM)iButton, 0)

#define PropSheet_SetCurSelByID(hDlg, id) \
	SNDMSG(hDlg, PSM_SETCURSELID, 0, (LPARAM)id)

#define PropSheet_SetFinishText(hDlg, lpszText) \
	SNDMSG(hDlg, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText)

#define PropSheet_GetTabControl(hDlg) \
	(HWND)SNDMSG(hDlg, PSM_GETTABCONTROL, 0, 0)

#define PropSheet_IsDialogMessage(hDlg, pMsg) \
	(BOOL)SNDMSG(hDlg, PSM_ISDIALOGMESSAGE, 0, (LPARAM)pMsg)

#define PropSheet_GetCurrentPageHwnd(hDlg) \
        (HWND)SNDMSG(hDlg, PSM_GETCURRENTPAGEHWND, 0, 0)

#define PropSheet_InsertPage(hDlg, index, hpage) \
        SNDMSG(hDlg, PSM_INSERTPAGE, (WPARAM)(index), (LPARAM)(hpage))

#define PropSheet_SetHeaderTitle(hDlg, index, lpszText) \
        SNDMSG(hDlg, PSM_SETHEADERTITLE, (WPARAM)(index), (LPARAM)(lpszText))

#define PropSheet_SetHeaderSubTitle(hDlg, index, lpszText) \
        SNDMSG(hDlg, PSM_SETHEADERSUBTITLE, (WPARAM)(index), (LPARAM)(lpszText))

#define PropSheet_HwndToIndex(hDlg, hwnd) \
        (int)SNDMSG(hDlg, PSM_HWNDTOINDEX, (WPARAM)(hwnd), 0)

#define PropSheet_IndexToHwnd(hDlg, i) \
        (HWND)SNDMSG(hDlg, PSM_INDEXTOHWND, (WPARAM)(i), 0)

#define PropSheet_PageToIndex(hDlg, hpage) \
        (int)SNDMSG(hDlg, PSM_PAGETOINDEX, 0, (LPARAM)(hpage))

#define PropSheet_IndexToPage(hDlg, i) \
        (HPROPSHEETPAGE)SNDMSG(hDlg, PSM_INDEXTOPAGE, (WPARAM)(i), 0)

#define PropSheet_IdToIndex(hDlg, id) \
        (int)SNDMSG(hDlg, PSM_IDTOINDEX, 0, (LPARAM)(id))

#define PropSheet_IndexToId(hDlg, i) \
        SNDMSG(hDlg, PSM_INDEXTOID, (WPARAM)(i), 0)

#define PropSheet_GetResult(hDlg) \
        SNDMSG(hDlg, PSM_GETRESULT, 0, 0)

#define PropSheet_RecalcPageSizes(hDlg) \
        SNDMSG(hDlg, PSM_RECALCPAGESIZES, 0, 0)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_PRSHT_H */
