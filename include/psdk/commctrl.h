/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#ifndef _INC_COMMCTRL
#define _INC_COMMCTRL

#include <_mingw_unicode.h>

#ifndef _WINRESRC_
#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#else
/* FIXME: This really must be 0x0501 !!! */
#if (_WIN32_IE < 0x0500)
#error _WIN32_IE setting conflicts
#endif
#endif /* !_WIN32_IE */
#endif /* !_WINRESRC_ */

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef _Return_type_success_(return >= 0) LONG HRESULT;
#endif

#ifndef NOUSER

#ifndef WINCOMMCTRLAPI
#ifndef _COMCTL32_
#define WINCOMMCTRLAPI DECLSPEC_IMPORT
#else
#define WINCOMMCTRLAPI
#endif
#endif /* !WINCOMMCTRLAPI */

#ifdef __cplusplus
extern "C" {
#endif

#include <prsht.h>

#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif /* !SNDMSG */

  WINCOMMCTRLAPI void WINAPI InitCommonControls(void);

  typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;
    DWORD dwICC;
  } INITCOMMONCONTROLSEX,*LPINITCOMMONCONTROLSEX;
#define ICC_LISTVIEW_CLASSES 0x1
#define ICC_TREEVIEW_CLASSES 0x2
#define ICC_BAR_CLASSES 0x4
#define ICC_TAB_CLASSES 0x8
#define ICC_UPDOWN_CLASS 0x10
#define ICC_PROGRESS_CLASS 0x20
#define ICC_HOTKEY_CLASS 0x40
#define ICC_ANIMATE_CLASS 0x80
#define ICC_WIN95_CLASSES 0xff
#define ICC_DATE_CLASSES 0x100
#define ICC_USEREX_CLASSES 0x200
#define ICC_COOL_CLASSES 0x400
#define ICC_INTERNET_CLASSES 0x800
#define ICC_PAGESCROLLER_CLASS 0x1000
#define ICC_NATIVEFNTCTL_CLASS 0x2000
#define ICC_STANDARD_CLASSES 0x4000
#define ICC_LINK_CLASS 0x8000

  WINCOMMCTRLAPI BOOL WINAPI InitCommonControlsEx(_In_ const INITCOMMONCONTROLSEX *);

#define ODT_HEADER 100
#define ODT_TAB 101
#define ODT_LISTVIEW 102

#define LVM_FIRST 0x1000
#define TV_FIRST 0x1100
#define HDM_FIRST 0x1200
#define TCM_FIRST 0x1300

#define PGM_FIRST 0x1400
#define ECM_FIRST 0x1500
#define BCM_FIRST 0x1600
#define CBM_FIRST 0x1700
#define CCM_FIRST 0x2000
#define CCM_LAST (CCM_FIRST+0x200)
#define CCM_SETBKCOLOR (CCM_FIRST+1)
#define CCM_SETCOLORSCHEME (CCM_FIRST+2)
#define CCM_GETCOLORSCHEME (CCM_FIRST+3)
#define CCM_GETDROPTARGET (CCM_FIRST+4)
#define CCM_SETUNICODEFORMAT (CCM_FIRST+5)
#define CCM_GETUNICODEFORMAT (CCM_FIRST+6)

  typedef struct tagCOLORSCHEME {
    DWORD dwSize;
    COLORREF clrBtnHighlight;
    COLORREF clrBtnShadow;
  } COLORSCHEME,*LPCOLORSCHEME;

#if (_WIN32_IE >= 0x0500)

#if (_WIN32_WINNT >= 0x0501)
#define COMCTL32_VERSION  6
#else
#define COMCTL32_VERSION  5
#endif

#define CCM_SETVERSION (CCM_FIRST+0x7)
#define CCM_GETVERSION (CCM_FIRST+0x8)
#define CCM_SETNOTIFYWINDOW (CCM_FIRST+0x9)
#if (_WIN32_WINNT >= 0x0501)
#define CCM_SETWINDOWTHEME (CCM_FIRST+0xb)
#define CCM_DPISCALE (CCM_FIRST+0xc)
#endif

#endif /* _WIN32_IE >= 0x0500 */

#define INFOTIPSIZE 1024

#define HANDLE_WM_NOTIFY(hwnd,wParam,lParam,fn) (fn)((hwnd),(int)(wParam),(NMHDR *)(lParam))
#define FORWARD_WM_NOTIFY(hwnd,idFrom,pnmhdr,fn) (LRESULT)(fn)((hwnd),WM_NOTIFY,(WPARAM)(int)(idFrom),(LPARAM)(NMHDR *)(pnmhdr))

#define NM_OUTOFMEMORY (NM_FIRST-1)
#define NM_CLICK (NM_FIRST-2)
#define NM_DBLCLK (NM_FIRST-3)
#define NM_RETURN (NM_FIRST-4)
#define NM_RCLICK (NM_FIRST-5)
#define NM_RDBLCLK (NM_FIRST-6)
#define NM_SETFOCUS (NM_FIRST-7)
#define NM_KILLFOCUS (NM_FIRST-8)
#define NM_CUSTOMDRAW (NM_FIRST-12)
#define NM_HOVER (NM_FIRST-13)
#define NM_NCHITTEST (NM_FIRST-14)
#define NM_KEYDOWN (NM_FIRST-15)
#define NM_RELEASEDCAPTURE (NM_FIRST-16)
#define NM_SETCURSOR (NM_FIRST-17)
#define NM_CHAR (NM_FIRST-18)
#define NM_TOOLTIPSCREATED (NM_FIRST-19)
#define NM_LDOWN (NM_FIRST-20)
#define NM_RDOWN (NM_FIRST-21)
#define NM_THEMECHANGED (NM_FIRST-22)

#ifndef CCSIZEOF_STRUCT
#define CCSIZEOF_STRUCT(structname,member) (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0))))+sizeof(((structname*)0)->member))
#endif

  typedef struct tagNMTOOLTIPSCREATED {
    NMHDR hdr;
    HWND hwndToolTips;
  } NMTOOLTIPSCREATED,*LPNMTOOLTIPSCREATED;

  typedef struct tagNMMOUSE {
    NMHDR hdr;
    DWORD_PTR dwItemSpec;
    DWORD_PTR dwItemData;
    POINT pt;
    LPARAM dwHitInfo;
  } NMMOUSE,*LPNMMOUSE;

  typedef NMMOUSE NMCLICK;
  typedef LPNMMOUSE LPNMCLICK;

  typedef struct tagNMOBJECTNOTIFY {
    NMHDR hdr;
    int iItem;
#ifdef __IID_DEFINED__
    const IID *piid;
#else
    const void *piid;
#endif
    void *pObject;
    HRESULT hResult;
    DWORD dwFlags;
  } NMOBJECTNOTIFY,*LPNMOBJECTNOTIFY;

  typedef struct tagNMKEY {
    NMHDR hdr;
    UINT nVKey;
    UINT uFlags;
  } NMKEY,*LPNMKEY;

  typedef struct tagNMCHAR {
    NMHDR hdr;
    UINT ch;
    DWORD dwItemPrev;
    DWORD dwItemNext;
  } NMCHAR,*LPNMCHAR;

#define NM_FIRST (0U- 0U)
#define NM_LAST (0U- 99U)

#define LVN_FIRST (0U-100U)
#define LVN_LAST (0U-199U)

#define HDN_FIRST (0U-300U)
#define HDN_LAST (0U-399U)

#define TVN_FIRST (0U-400U)
#define TVN_LAST (0U-499U)

#define TTN_FIRST (0U-520U)
#define TTN_LAST (0U-549U)

#define TCN_FIRST (0U-550U)
#define TCN_LAST (0U-580U)

#ifndef CDN_FIRST
#define CDN_FIRST (0U-601U)
#define CDN_LAST (0U-699U)
#endif

#define TBN_FIRST (0U-700U)
#define TBN_LAST (0U-720U)

#define UDN_FIRST (0U-721)
#define UDN_LAST (0U-740)

#define MCN_FIRST (0U-750U)
#define MCN_LAST (0U-759U)
#define DTN_FIRST (0U-760U)
#define DTN_LAST (0U-799U)
#define CBEN_FIRST (0U-800U)
#define CBEN_LAST (0U-830U)
#define RBN_FIRST (0U-831U)
#define RBN_LAST (0U-859U)

#define IPN_FIRST (0U-860U)
#define IPN_LAST (0U-879U)
#define SBN_FIRST (0U-880U)
#define SBN_LAST (0U-899U)
#define PGN_FIRST (0U-900U)
#define PGN_LAST (0U-950U)

#ifndef WMN_FIRST
#define WMN_FIRST (0U-1000U)
#define WMN_LAST (0U-1200U)
#endif

#define BCN_FIRST (0U-1250U)
#define BCN_LAST (0U-1350U)

#define MSGF_COMMCTRL_BEGINDRAG 0x4200
#define MSGF_COMMCTRL_SIZEHEADER 0x4201
#define MSGF_COMMCTRL_DRAGSELECT 0x4202
#define MSGF_COMMCTRL_TOOLBARCUST 0x4203

#define CDRF_DODEFAULT 0x0
#define CDRF_NEWFONT 0x2
#define CDRF_SKIPDEFAULT 0x4

#define CDRF_NOTIFYPOSTPAINT 0x10
#define CDRF_NOTIFYITEMDRAW 0x20
#define CDRF_NOTIFYSUBITEMDRAW 0x20
#define CDRF_NOTIFYPOSTERASE 0x40
#define CDRF_NOTIFYITEMERASE 0x00000080 /* obsolete ??? */

#define CDDS_PREPAINT 0x1
#define CDDS_POSTPAINT 0x2
#define CDDS_PREERASE 0x3
#define CDDS_POSTERASE 0x4
#define CDDS_ITEM 0x10000
#define CDDS_ITEMPREPAINT (CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT (CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_ITEMPREERASE (CDDS_ITEM | CDDS_PREERASE)
#define CDDS_ITEMPOSTERASE (CDDS_ITEM | CDDS_POSTERASE)
#define CDDS_SUBITEM 0x20000

#define CDIS_SELECTED 0x1
#define CDIS_GRAYED 0x2
#define CDIS_DISABLED 0x4
#define CDIS_CHECKED 0x8
#define CDIS_FOCUS 0x10
#define CDIS_DEFAULT 0x20
#define CDIS_HOT 0x40
#define CDIS_MARKED 0x80
#define CDIS_INDETERMINATE 0x100
#define CDIS_SHOWKEYBOARDCUES 0x200

  typedef struct tagNMCUSTOMDRAWINFO {
    NMHDR hdr;
    DWORD dwDrawStage;
    HDC hdc;
    RECT rc;
    DWORD_PTR dwItemSpec;
    UINT uItemState;
    LPARAM lItemlParam;
  } NMCUSTOMDRAW,*LPNMCUSTOMDRAW;

  typedef struct tagNMTTCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    UINT uDrawFlags;
  } NMTTCUSTOMDRAW,*LPNMTTCUSTOMDRAW;

#ifndef NOIMAGEAPIS

#define CLR_NONE 0xffffffffL
#define CLR_DEFAULT 0xFF000000L

#ifndef HIMAGELIST
  struct _IMAGELIST;
  typedef struct _IMAGELIST *HIMAGELIST;
#endif

#ifndef IMAGELISTDRAWPARAMS
  typedef struct _IMAGELISTDRAWPARAMS {
    DWORD cbSize;
    HIMAGELIST himl;
    int i;
    HDC hdcDst;
    int x;
    int y;
    int cx;
    int cy;
    int xBitmap;
    int yBitmap;
    COLORREF rgbBk;
    COLORREF rgbFg;
    UINT fStyle;
    DWORD dwRop;
    DWORD fState;
    DWORD Frame;
    COLORREF crEffect;
  } IMAGELISTDRAWPARAMS,*LPIMAGELISTDRAWPARAMS;

#define IMAGELISTDRAWPARAMS_V3_SIZE CCSIZEOF_STRUCT(IMAGELISTDRAWPARAMS,dwRop)
#endif

#define ILC_MASK 0x1
#define ILC_COLOR 0x0
#define ILC_COLORDDB 0xfe
#define ILC_COLOR4 0x4
#define ILC_COLOR8 0x8
#define ILC_COLOR16 0x10
#define ILC_COLOR24 0x18
#define ILC_COLOR32 0x20
#define ILC_PALETTE 0x800
#define ILC_MIRROR 0x2000
#define ILC_PERITEMMIRROR 0x8000

  WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Create(int cx,int cy,UINT flags,int cInitial,int cGrow);
  WINCOMMCTRLAPI BOOL WINAPI ImageList_Destroy(_In_opt_ HIMAGELIST himl);
  WINCOMMCTRLAPI int WINAPI ImageList_GetImageCount(_In_ HIMAGELIST himl);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_SetImageCount(
    _In_ HIMAGELIST himl,
    _In_ UINT uNewCount);

  WINCOMMCTRLAPI
  int
  WINAPI
  ImageList_Add(
    _In_ HIMAGELIST himl,
    _In_ HBITMAP hbmImage,
    _In_opt_ HBITMAP hbmMask);

  WINCOMMCTRLAPI
  int
  WINAPI
  ImageList_ReplaceIcon(
    _In_ HIMAGELIST himl,
    _In_ int i,
    _In_ HICON hicon);

  WINCOMMCTRLAPI
  COLORREF
  WINAPI
  ImageList_SetBkColor(
    _In_ HIMAGELIST himl,
    _In_ COLORREF clrBk);

  WINCOMMCTRLAPI COLORREF WINAPI ImageList_GetBkColor(_In_ HIMAGELIST himl);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_SetOverlayImage(
    _In_ HIMAGELIST himl,
    _In_ int iImage,
    _In_ int iOverlay);

  WINCOMMCTRLAPI
  HRESULT
  WINAPI
  HIMAGELIST_QueryInterface(
    _In_ HIMAGELIST,
    _In_ REFIID,
    _Outptr_ void **);

#define ImageList_AddIcon(himl,hicon) ImageList_ReplaceIcon(himl,-1,hicon)

#define ILD_NORMAL 0x0
#define ILD_TRANSPARENT 0x1
#define ILD_MASK 0x10
#define ILD_IMAGE 0x20
#define ILD_ROP 0x40
#define ILD_BLEND25 0x2
#define ILD_BLEND50 0x4
#define ILD_OVERLAYMASK 0xf00
#define INDEXTOOVERLAYMASK(i) ((i) << 8)
#define ILD_PRESERVEALPHA 0x1000
#define ILD_SCALE 0x2000
#define ILD_DPISCALE 0x4000

#define ILD_SELECTED ILD_BLEND50
#define ILD_FOCUS ILD_BLEND25
#define ILD_BLEND ILD_BLEND50
#define CLR_HILIGHT CLR_DEFAULT

#define ILS_NORMAL 0x0
#define ILS_GLOW 0x1
#define ILS_SHADOW 0x2
#define ILS_SATURATE 0x4
#define ILS_ALPHA 0x8

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_Draw(
    _In_ HIMAGELIST himl,
    _In_ int i,
    _In_ HDC hdcDst,
    _In_ int x,
    _In_ int y,
    _In_ UINT fStyle);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_Replace(
    _In_ HIMAGELIST himl,
    _In_ int i,
    _In_ HBITMAP hbmImage,
    _In_opt_ HBITMAP hbmMask);

  WINCOMMCTRLAPI
  int
  WINAPI
  ImageList_AddMasked(
    _In_ HIMAGELIST himl,
    _In_ HBITMAP hbmImage,
    _In_ COLORREF crMask);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_DrawEx(
    _In_ HIMAGELIST himl,
    _In_ int i,
    _In_ HDC hdcDst,
    _In_ int x,
    _In_ int y,
    _In_ int dx,
    _In_ int dy,
    _In_ COLORREF rgbBk,
    _In_ COLORREF rgbFg,
    _In_ UINT fStyle);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_DrawIndirect(
    _In_ IMAGELISTDRAWPARAMS *pimldp);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_Remove(
    _In_ HIMAGELIST himl,
    _In_ int i);

  WINCOMMCTRLAPI
  HICON
  WINAPI
  ImageList_GetIcon(
    _In_ HIMAGELIST himl,
    _In_ int i,
    _In_ UINT flags);

  WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_LoadImageA(HINSTANCE hi,LPCSTR lpbmp,int cx,int cGrow,COLORREF crMask,UINT uType,UINT uFlags);
  WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_LoadImageW(HINSTANCE hi,LPCWSTR lpbmp,int cx,int cGrow,COLORREF crMask,UINT uType,UINT uFlags);

#define ImageList_LoadImage __MINGW_NAME_AW(ImageList_LoadImage)

#define ILCF_MOVE 0x0
#define ILCF_SWAP 0x1

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_Copy(
    _In_ HIMAGELIST himlDst,
    _In_ int iDst,
    _In_ HIMAGELIST himlSrc,
    _In_ int iSrc,
    _In_ UINT uFlags);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_BeginDrag(
    _In_ HIMAGELIST himlTrack,
    _In_ int iTrack,
    _In_ int dxHotspot,
    _In_ int dyHotspot);

  WINCOMMCTRLAPI void WINAPI ImageList_EndDrag(void);
  WINCOMMCTRLAPI BOOL WINAPI ImageList_DragEnter(HWND hwndLock,int x,int y);
  WINCOMMCTRLAPI BOOL WINAPI ImageList_DragLeave(HWND hwndLock);
  WINCOMMCTRLAPI BOOL WINAPI ImageList_DragMove(int x,int y);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_SetDragCursorImage(
    _In_ HIMAGELIST himlDrag,
    _In_ int iDrag,
    _In_ int dxHotspot,
    _In_ int dyHotspot);

  WINCOMMCTRLAPI BOOL WINAPI ImageList_DragShowNolock(BOOL fShow);

  _Success_(return != NULL)
  WINCOMMCTRLAPI
  HIMAGELIST
  WINAPI
  ImageList_GetDragImage(
    _Out_opt_ POINT *ppt,
    _Out_opt_ POINT *pptHotspot);

#define ImageList_RemoveAll(himl) ImageList_Remove(himl,-1)
#define ImageList_ExtractIcon(hi,himl,i) ImageList_GetIcon(himl,i,0)
#define ImageList_LoadBitmap(hi,lpbmp,cx,cGrow,crMask) ImageList_LoadImage(hi,lpbmp,cx,cGrow,crMask,IMAGE_BITMAP,0)

#ifdef __IStream_INTERFACE_DEFINED__
  WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Read(_In_ LPSTREAM pstm);
  WINCOMMCTRLAPI BOOL WINAPI ImageList_Write(_In_ HIMAGELIST himl, _In_ LPSTREAM pstm);
#define ILP_NORMAL 0
#define ILP_DOWNLEVEL 1
  WINCOMMCTRLAPI HRESULT WINAPI ImageList_ReadEx(_In_ DWORD dwFlags, _In_ LPSTREAM pstm, _In_ REFIID riid, _Outptr_ PVOID *ppv);
  WINCOMMCTRLAPI HRESULT WINAPI ImageList_WriteEx(_In_ HIMAGELIST himl, _In_ DWORD dwFlags, _In_ LPSTREAM pstm);
#endif

#ifndef IMAGEINFO
  typedef struct _IMAGEINFO {
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    int Unused1;
    int Unused2;
    RECT rcImage;
  } IMAGEINFO,*LPIMAGEINFO;
#endif

  _Success_(return != 0)
  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_GetIconSize(
    _In_ HIMAGELIST himl,
    _Out_opt_ int *cx,
    _Out_opt_ int *cy);

  _Success_(return != 0)
  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_SetIconSize(
    _In_ HIMAGELIST himl,
    _In_ int cx,
    _In_ int cy);

  _Success_(return != 0)
  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ImageList_GetImageInfo(
    _In_ HIMAGELIST himl,
    _In_ int i,
    _Out_ IMAGEINFO *pImageInfo);

  WINCOMMCTRLAPI
  HIMAGELIST
  WINAPI
  ImageList_Merge(
    _In_ HIMAGELIST himl1,
    _In_ int i1,
    _In_ HIMAGELIST himl2,
    _In_ int i2,
    _In_ int dx,
    _In_ int dy);

  WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Duplicate(_In_ HIMAGELIST himl);

#endif /* !NOIMAGEAPIS */

#ifndef NOHEADER

#define WC_HEADERA "SysHeader32"
#define WC_HEADERW L"SysHeader32"

#define WC_HEADER __MINGW_NAME_AW(WC_HEADER)

#define HDS_HORZ 0x0
#define HDS_BUTTONS 0x2
#define HDS_HOTTRACK 0x4
#define HDS_HIDDEN 0x8
#define HDS_DRAGDROP 0x40
#define HDS_FULLDRAG 0x80
#define HDS_FILTERBAR 0x100
#define HDS_FLAT 0x200
#if (_WIN32_WINNT >= 0x0600)
#define HDS_CHECKBOXES 0x400
#define HDS_NOSIZING 0x800
#define HDS_OVERFLOW 0x1000
#endif

#define HDFT_ISSTRING 0x0
#define HDFT_ISNUMBER 0x1

#define HDFT_HASNOVALUE 0x8000

#define HD_TEXTFILTER __MINGW_NAME_AW(HD_TEXTFILTER)
#define HDTEXTFILTER __MINGW_NAME_AW(HD_TEXTFILTER)
#define LPHD_TEXTFILTER __MINGW_NAME_AW(LPHD_TEXTFILTER)
#define LPHDTEXTFILTER __MINGW_NAME_AW(LPHD_TEXTFILTER)

  typedef struct _HD_TEXTFILTERA {
    LPSTR pszText;
    INT cchTextMax;
  } HD_TEXTFILTERA,*LPHD_TEXTFILTERA;

  typedef struct _HD_TEXTFILTERW {
    LPWSTR pszText;
    INT cchTextMax;
  } HD_TEXTFILTERW,*LPHD_TEXTFILTERW;

#define HD_ITEMA HDITEMA
#define HD_ITEMW HDITEMW
#define HD_ITEM HDITEM

  typedef struct _HD_ITEMA {
    UINT mask;
    int cxy;
    LPSTR pszText;
    HBITMAP hbm;
    int cchTextMax;
    int fmt;
    LPARAM lParam;
    int iImage;
    int iOrder;
    UINT type;
    void  *pvFilter;
  } HDITEMA,*LPHDITEMA;

#define HDITEMA_V1_SIZE CCSIZEOF_STRUCT(HDITEMA,lParam)
#define HDITEMW_V1_SIZE CCSIZEOF_STRUCT(HDITEMW,lParam)

  typedef struct _HD_ITEMW {
    UINT mask;
    int cxy;
    LPWSTR pszText;
    HBITMAP hbm;
    int cchTextMax;
    int fmt;
    LPARAM lParam;
    int iImage;
    int iOrder;
    UINT type;
    void *pvFilter;
  } HDITEMW,*LPHDITEMW;

#define HDITEM __MINGW_NAME_AW(HDITEM)
#define LPHDITEM __MINGW_NAME_AW(LPHDITEM)

#define HDITEM_V1_SIZE __MINGW_NAME_AW_EXT(HDITEM,_V1_SIZE)

#define HDI_WIDTH 0x1
#define HDI_HEIGHT HDI_WIDTH
#define HDI_TEXT 0x2
#define HDI_FORMAT 0x4
#define HDI_LPARAM 0x8
#define HDI_BITMAP 0x10
#define HDI_IMAGE 0x20
#define HDI_DI_SETITEM 0x40
#define HDI_ORDER 0x80
#define HDI_FILTER 0x100

#define HDF_LEFT 0x0
#define HDF_RIGHT 0x1
#define HDF_CENTER 0x2
#define HDF_JUSTIFYMASK 0x3
#define HDF_RTLREADING 0x4

#define HDF_OWNERDRAW 0x8000
#define HDF_STRING 0x4000
#define HDF_BITMAP 0x2000
#define HDF_BITMAP_ON_RIGHT 0x1000
#define HDF_IMAGE 0x800
#define HDF_SORTUP 0x400
#define HDF_SORTDOWN 0x200
#if (_WIN32_WINNT >= 0x0600)
#define HDF_CHECKBOX 0x40
#define HDF_CHECKED 0x80
#define HDF_FIXEDWIDTH 0x100
#define HDF_SPLITBUTTON 0x1000000
#endif

#define HDM_GETITEMCOUNT (HDM_FIRST+0)
#define Header_GetItemCount(hwndHD) (int)SNDMSG((hwndHD),HDM_GETITEMCOUNT,0,0L)

#define HDM_INSERTITEMA (HDM_FIRST+1)
#define HDM_INSERTITEMW (HDM_FIRST+10)

#define HDM_INSERTITEM __MINGW_NAME_AW(HDM_INSERTITEM)

#define Header_InsertItem(hwndHD,i,phdi) (int)SNDMSG((hwndHD),HDM_INSERTITEM,(WPARAM)(int)(i),(LPARAM)(const HD_ITEM *)(phdi))

#define HDM_DELETEITEM (HDM_FIRST+2)
#define Header_DeleteItem(hwndHD,i) (BOOL)SNDMSG((hwndHD),HDM_DELETEITEM,(WPARAM)(int)(i),0L)

#define HDM_GETITEMA (HDM_FIRST+3)
#define HDM_GETITEMW (HDM_FIRST+11)

#define HDM_GETITEM __MINGW_NAME_AW(HDM_GETITEM)

#define Header_GetItem(hwndHD,i,phdi) (BOOL)SNDMSG((hwndHD),HDM_GETITEM,(WPARAM)(int)(i),(LPARAM)(HD_ITEM *)(phdi))

#define HDM_SETITEMA (HDM_FIRST+4)
#define HDM_SETITEMW (HDM_FIRST+12)

#define HDM_SETITEM __MINGW_NAME_AW(HDM_SETITEM)

#define Header_SetItem(hwndHD,i,phdi) (BOOL)SNDMSG((hwndHD),HDM_SETITEM,(WPARAM)(int)(i),(LPARAM)(const HD_ITEM *)(phdi))

#define HD_LAYOUT HDLAYOUT

  typedef struct _HD_LAYOUT {
    RECT *prc;
    WINDOWPOS *pwpos;
  } HDLAYOUT,*LPHDLAYOUT;

#define HDM_LAYOUT (HDM_FIRST+5)
#define Header_Layout(hwndHD,playout) (BOOL)SNDMSG((hwndHD),HDM_LAYOUT,0,(LPARAM)(HD_LAYOUT *)(playout))

#define HHT_NOWHERE 0x1
#define HHT_ONHEADER 0x2
#define HHT_ONDIVIDER 0x4
#define HHT_ONDIVOPEN 0x8
#define HHT_ONFILTER 0x10
#define HHT_ONFILTERBUTTON 0x20
#define HHT_ABOVE 0x100
#define HHT_BELOW 0x200
#define HHT_TORIGHT 0x400
#define HHT_TOLEFT 0x800

#define HD_HITTESTINFO HDHITTESTINFO

  typedef struct _HD_HITTESTINFO {
    POINT pt;
    UINT flags;
    int iItem;
  } HDHITTESTINFO,*LPHDHITTESTINFO;

#define HDM_HITTEST (HDM_FIRST+6)

#define HDM_GETITEMRECT (HDM_FIRST+7)
#define Header_GetItemRect(hwnd,iItem,lprc) (BOOL)SNDMSG((hwnd),HDM_GETITEMRECT,(WPARAM)(iItem),(LPARAM)(lprc))

#define HDM_SETIMAGELIST (HDM_FIRST+8)
#define Header_SetImageList(hwnd,himl) (HIMAGELIST)SNDMSG((hwnd),HDM_SETIMAGELIST,0,(LPARAM)(himl))

#define HDM_GETIMAGELIST (HDM_FIRST+9)
#define Header_GetImageList(hwnd) (HIMAGELIST)SNDMSG((hwnd),HDM_GETIMAGELIST,0,0)

#define HDM_ORDERTOINDEX (HDM_FIRST+15)
#define Header_OrderToIndex(hwnd,i) (int)SNDMSG((hwnd),HDM_ORDERTOINDEX,(WPARAM)(i),0)

#define HDM_CREATEDRAGIMAGE (HDM_FIRST+16)
#define Header_CreateDragImage(hwnd,i) (HIMAGELIST)SNDMSG((hwnd),HDM_CREATEDRAGIMAGE,(WPARAM)(i),0)

#define HDM_GETORDERARRAY (HDM_FIRST+17)
#define Header_GetOrderArray(hwnd,iCount,lpi) (BOOL)SNDMSG((hwnd),HDM_GETORDERARRAY,(WPARAM)(iCount),(LPARAM)(lpi))

#define HDM_SETORDERARRAY (HDM_FIRST+18)
#define Header_SetOrderArray(hwnd,iCount,lpi) (BOOL)SNDMSG((hwnd),HDM_SETORDERARRAY,(WPARAM)(iCount),(LPARAM)(lpi))

#define HDM_SETHOTDIVIDER (HDM_FIRST+19)
#define Header_SetHotDivider(hwnd,fPos,dw) (int)SNDMSG((hwnd),HDM_SETHOTDIVIDER,(WPARAM)(fPos),(LPARAM)(dw))

#define HDM_SETBITMAPMARGIN (HDM_FIRST+20)
#define Header_SetBitmapMargin(hwnd,iWidth) (int)SNDMSG((hwnd),HDM_SETBITMAPMARGIN,(WPARAM)(iWidth),0)

#define HDM_GETBITMAPMARGIN (HDM_FIRST+21)
#define Header_GetBitmapMargin(hwnd) (int)SNDMSG((hwnd),HDM_GETBITMAPMARGIN,0,0)

#define HDM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define Header_SetUnicodeFormat(hwnd,fUnicode) (BOOL)SNDMSG((hwnd),HDM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)

#define HDM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define Header_GetUnicodeFormat(hwnd) (BOOL)SNDMSG((hwnd),HDM_GETUNICODEFORMAT,0,0)

#define HDM_SETFILTERCHANGETIMEOUT (HDM_FIRST+22)
#define Header_SetFilterChangeTimeout(hwnd,i) (int)SNDMSG((hwnd),HDM_SETFILTERCHANGETIMEOUT,0,(LPARAM)(i))

#define HDM_EDITFILTER (HDM_FIRST+23)
#define Header_EditFilter(hwnd,i,fDiscardChanges) (int)SNDMSG((hwnd),HDM_EDITFILTER,(WPARAM)(i),MAKELPARAM(fDiscardChanges,0))

#define HDM_CLEARFILTER (HDM_FIRST+24)
#define Header_ClearFilter(hwnd,i) (int)SNDMSG((hwnd),HDM_CLEARFILTER,(WPARAM)(i),0)
#define Header_ClearAllFilters(hwnd) (int)SNDMSG((hwnd),HDM_CLEARFILTER,(WPARAM)-1,0)

#define HDN_ITEMCHANGINGA (HDN_FIRST-0)
#define HDN_ITEMCHANGINGW (HDN_FIRST-20)
#define HDN_ITEMCHANGEDA (HDN_FIRST-1)
#define HDN_ITEMCHANGEDW (HDN_FIRST-21)
#define HDN_ITEMCLICKA (HDN_FIRST-2)
#define HDN_ITEMCLICKW (HDN_FIRST-22)
#define HDN_ITEMDBLCLICKA (HDN_FIRST-3)
#define HDN_ITEMDBLCLICKW (HDN_FIRST-23)
#define HDN_DIVIDERDBLCLICKA (HDN_FIRST-5)
#define HDN_DIVIDERDBLCLICKW (HDN_FIRST-25)
#define HDN_BEGINTRACKA (HDN_FIRST-6)
#define HDN_BEGINTRACKW (HDN_FIRST-26)
#define HDN_ENDTRACKA (HDN_FIRST-7)
#define HDN_ENDTRACKW (HDN_FIRST-27)
#define HDN_TRACKA (HDN_FIRST-8)
#define HDN_TRACKW (HDN_FIRST-28)
#define HDN_GETDISPINFOA (HDN_FIRST-9)
#define HDN_GETDISPINFOW (HDN_FIRST-29)
#define HDN_BEGINDRAG (HDN_FIRST-10)
#define HDN_ENDDRAG (HDN_FIRST-11)
#define HDN_FILTERCHANGE (HDN_FIRST-12)
#define HDN_FILTERBTNCLICK (HDN_FIRST-13)
#define HDN_BEGINFILTEREDIT (HDN_FIRST-14)
#define HDN_ENDFILTEREDIT (HDN_FIRST-15)
#define HDN_ITEMSTATEICONCLICK (HDN_FIRST-16)
#define HDN_ITEMKEYDOWN (HDN_FIRST-17)

#define HDN_ITEMCHANGING __MINGW_NAME_AW(HDN_ITEMCHANGING)
#define HDN_ITEMCHANGED __MINGW_NAME_AW(HDN_ITEMCHANGED)
#define HDN_ITEMCLICK __MINGW_NAME_AW(HDN_ITEMCLICK)
#define HDN_ITEMDBLCLICK __MINGW_NAME_AW(HDN_ITEMDBLCLICK)
#define HDN_DIVIDERDBLCLICK __MINGW_NAME_AW(HDN_DIVIDERDBLCLICK)
#define HDN_BEGINTRACK __MINGW_NAME_AW(HDN_BEGINTRACK)
#define HDN_ENDTRACK __MINGW_NAME_AW(HDN_ENDTRACK)
#define HDN_TRACK __MINGW_NAME_AW(HDN_TRACK)
#define HDN_GETDISPINFO __MINGW_NAME_AW(HDN_GETDISPINFO)

#define HD_NOTIFYA NMHEADERA
#define HD_NOTIFYW NMHEADERW
#define HD_NOTIFY NMHEADER

  typedef struct tagNMHEADERA {
    NMHDR hdr;
    int iItem;
    int iButton;
    HDITEMA *pitem;
  } NMHEADERA,*LPNMHEADERA;

  typedef struct tagNMHEADERW {
    NMHDR hdr;
    int iItem;
    int iButton;
    HDITEMW *pitem;
  } NMHEADERW,*LPNMHEADERW;

#define NMHEADER __MINGW_NAME_AW(NMHEADER)
#define LPNMHEADER __MINGW_NAME_AW(LPNMHEADER)

  typedef struct tagNMHDDISPINFOW {
    NMHDR hdr;
    int iItem;
    UINT mask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
  } NMHDDISPINFOW,*LPNMHDDISPINFOW;

  typedef struct tagNMHDDISPINFOA {
    NMHDR hdr;
    int iItem;
    UINT mask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
  } NMHDDISPINFOA,*LPNMHDDISPINFOA;

#define NMHDDISPINFO __MINGW_NAME_AW(NMHDDISPINFO)
#define LPNMHDDISPINFO __MINGW_NAME_AW(LPNMHDDISPINFO)

  typedef struct tagNMHDFILTERBTNCLICK {
    NMHDR hdr;
    INT iItem;
    RECT rc;
  } NMHDFILTERBTNCLICK,*LPNMHDFILTERBTNCLICK;

#endif /* !NOHEADER */

#ifndef NOTOOLBAR

#define TOOLBARCLASSNAMEW L"ToolbarWindow32"
#define TOOLBARCLASSNAMEA "ToolbarWindow32"

#define TOOLBARCLASSNAME __MINGW_NAME_AW(TOOLBARCLASSNAME)

  typedef struct _TBBUTTON {
    int iBitmap;
    int idCommand;
    BYTE fsState;
    BYTE fsStyle;
#ifdef _WIN64
    BYTE bReserved[6];
#else
    BYTE bReserved[2];
#endif
    DWORD_PTR dwData;
    INT_PTR iString;
  } TBBUTTON,NEAR *PTBBUTTON,*LPTBBUTTON;
  typedef const TBBUTTON *LPCTBBUTTON;

  typedef struct _COLORMAP {
    COLORREF from;
    COLORREF to;
  } COLORMAP,*LPCOLORMAP;

  WINCOMMCTRLAPI HWND WINAPI CreateToolbarEx(HWND hwnd,DWORD ws,UINT wID,int nBitmaps,HINSTANCE hBMInst,UINT_PTR wBMID,LPCTBBUTTON lpButtons,int iNumButtons,int dxButton,int dyButton,int dxBitmap,int dyBitmap,UINT uStructSize);
  WINCOMMCTRLAPI HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance,INT_PTR idBitmap,UINT wFlags,LPCOLORMAP lpColorMap,int iNumMaps);

#define CMB_MASKED 0x2
#define TBSTATE_CHECKED 0x1
#define TBSTATE_PRESSED 0x2
#define TBSTATE_ENABLED 0x4
#define TBSTATE_HIDDEN 0x8
#define TBSTATE_INDETERMINATE 0x10
#define TBSTATE_WRAP 0x20
#define TBSTATE_ELLIPSES 0x40
#define TBSTATE_MARKED 0x80

#define TBSTYLE_BUTTON 0x0
#define TBSTYLE_SEP 0x1
#define TBSTYLE_CHECK 0x2
#define TBSTYLE_GROUP 0x4
#define TBSTYLE_CHECKGROUP (TBSTYLE_GROUP | TBSTYLE_CHECK)
#define TBSTYLE_DROPDOWN 0x8
#define TBSTYLE_AUTOSIZE 0x10
#define TBSTYLE_NOPREFIX 0x20
#define TBSTYLE_TOOLTIPS 0x100
#define TBSTYLE_WRAPABLE 0x200
#define TBSTYLE_ALTDRAG 0x400
#define TBSTYLE_FLAT 0x800
#define TBSTYLE_LIST 0x1000
#define TBSTYLE_CUSTOMERASE 0x2000
#define TBSTYLE_REGISTERDROP 0x4000
#define TBSTYLE_TRANSPARENT 0x8000

#define BTNS_BUTTON TBSTYLE_BUTTON
#define BTNS_SEP TBSTYLE_SEP
#define BTNS_CHECK TBSTYLE_CHECK
#define BTNS_GROUP TBSTYLE_GROUP
#define BTNS_CHECKGROUP TBSTYLE_CHECKGROUP
#define BTNS_DROPDOWN TBSTYLE_DROPDOWN
#define BTNS_AUTOSIZE TBSTYLE_AUTOSIZE
#define BTNS_NOPREFIX TBSTYLE_NOPREFIX
#define BTNS_SHOWTEXT 0x40
#define BTNS_WHOLEDROPDOWN 0x80

#define TBSTYLE_EX_DRAWDDARROWS         0x00000001
#define TBSTYLE_EX_MULTICOLUMN          0x00000002
#define TBSTYLE_EX_VERTICAL             0x00000004
#define TBSTYLE_EX_MIXEDBUTTONS         0x00000008
#define TBSTYLE_EX_HIDECLIPPEDBUTTONS   0x00000010 /* don't show partially obscured buttons */
#define TBSTYLE_EX_DOUBLEBUFFER         0x00000080 /* Double Buffer the toolbar */

  typedef struct _NMTBCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    HBRUSH hbrMonoDither;
    HBRUSH hbrLines;
    HPEN hpenLines;
    COLORREF clrText;
    COLORREF clrMark;
    COLORREF clrTextHighlight;
    COLORREF clrBtnFace;
    COLORREF clrBtnHighlight;
    COLORREF clrHighlightHotTrack;
    RECT rcText;
    int nStringBkMode;
    int nHLStringBkMode;
    int iListGap;
  } NMTBCUSTOMDRAW,*LPNMTBCUSTOMDRAW;

#define TBCDRF_NOEDGES 0x10000
#define TBCDRF_HILITEHOTTRACK 0x20000
#define TBCDRF_NOOFFSET 0x40000
#define TBCDRF_NOMARK 0x80000
#define TBCDRF_NOETCHEDEFFECT 0x100000

#define TBCDRF_BLENDICON 0x200000
#define TBCDRF_NOBACKGROUND 0x400000

#define TB_ENABLEBUTTON (WM_USER+1)
#define TB_CHECKBUTTON (WM_USER+2)
#define TB_PRESSBUTTON (WM_USER+3)
#define TB_HIDEBUTTON (WM_USER+4)
#define TB_INDETERMINATE (WM_USER+5)
#define TB_MARKBUTTON (WM_USER+6)
#define TB_ISBUTTONENABLED (WM_USER+9)
#define TB_ISBUTTONCHECKED (WM_USER+10)
#define TB_ISBUTTONPRESSED (WM_USER+11)
#define TB_ISBUTTONHIDDEN (WM_USER+12)
#define TB_ISBUTTONINDETERMINATE (WM_USER+13)
#define TB_ISBUTTONHIGHLIGHTED (WM_USER+14)
#define TB_SETSTATE (WM_USER+17)
#define TB_GETSTATE (WM_USER+18)
#define TB_ADDBITMAP (WM_USER+19)

  typedef struct tagTBADDBITMAP {
    HINSTANCE hInst;
    UINT_PTR nID;
  } TBADDBITMAP,*LPTBADDBITMAP;

#define HINST_COMMCTRL ((HINSTANCE)-1)
#define IDB_STD_SMALL_COLOR 0
#define IDB_STD_LARGE_COLOR 1
#define IDB_VIEW_SMALL_COLOR 4
#define IDB_VIEW_LARGE_COLOR 5
#define IDB_HIST_SMALL_COLOR 8
#define IDB_HIST_LARGE_COLOR 9

#define STD_CUT 0
#define STD_COPY 1
#define STD_PASTE 2
#define STD_UNDO 3
#define STD_REDOW 4
#define STD_DELETE 5
#define STD_FILENEW 6
#define STD_FILEOPEN 7
#define STD_FILESAVE 8
#define STD_PRINTPRE 9
#define STD_PROPERTIES 10
#define STD_HELP 11
#define STD_FIND 12
#define STD_REPLACE 13
#define STD_PRINT 14

#define VIEW_LARGEICONS 0
#define VIEW_SMALLICONS 1
#define VIEW_LIST 2
#define VIEW_DETAILS 3
#define VIEW_SORTNAME 4
#define VIEW_SORTSIZE 5
#define VIEW_SORTDATE 6
#define VIEW_SORTTYPE 7
#define VIEW_PARENTFOLDER 8
#define VIEW_NETCONNECT 9
#define VIEW_NETDISCONNECT 10
#define VIEW_NEWFOLDER 11
#define VIEW_VIEWMENU 12
#define HIST_BACK 0
#define HIST_FORWARD 1
#define HIST_FAVORITES 2
#define HIST_ADDTOFAVORITES 3
#define HIST_VIEWTREE 4

#define TB_ADDBUTTONSA (WM_USER+20)
#define TB_INSERTBUTTONA (WM_USER+21)
#define TB_DELETEBUTTON (WM_USER+22)
#define TB_GETBUTTON (WM_USER+23)
#define TB_BUTTONCOUNT (WM_USER+24)
#define TB_COMMANDTOINDEX (WM_USER+25)

  typedef struct tagTBSAVEPARAMSA {
    HKEY hkr;
    LPCSTR pszSubKey;
    LPCSTR pszValueName;
  } TBSAVEPARAMSA,*LPTBSAVEPARAMSA;

  typedef struct tagTBSAVEPARAMSW {
    HKEY hkr;
    LPCWSTR pszSubKey;
    LPCWSTR pszValueName;
  } TBSAVEPARAMSW,*LPTBSAVEPARAMW;

#define TBSAVEPARAMS __MINGW_NAME_AW(TBSAVEPARAMS)
#define LPTBSAVEPARAMS __MINGW_NAME_AW(LPTBSAVEPARAMS)

#define TB_SAVERESTOREA (WM_USER+26)
#define TB_SAVERESTOREW (WM_USER+76)
#define TB_CUSTOMIZE (WM_USER+27)
#define TB_ADDSTRINGA (WM_USER+28)
#define TB_ADDSTRINGW (WM_USER+77)
#define TB_GETITEMRECT (WM_USER+29)
#define TB_BUTTONSTRUCTSIZE (WM_USER+30)
#define TB_SETBUTTONSIZE (WM_USER+31)
#define TB_SETBITMAPSIZE (WM_USER+32)
#define TB_AUTOSIZE (WM_USER+33)
#define TB_GETTOOLTIPS (WM_USER+35)
#define TB_SETTOOLTIPS (WM_USER+36)
#define TB_SETPARENT (WM_USER+37)
#define TB_SETROWS (WM_USER+39)
#define TB_GETROWS (WM_USER+40)
#define TB_SETCMDID (WM_USER+42)
#define TB_CHANGEBITMAP (WM_USER+43)
#define TB_GETBITMAP (WM_USER+44)
#define TB_GETBUTTONTEXTA (WM_USER+45)
#define TB_GETBUTTONTEXTW (WM_USER+75)
#define TB_REPLACEBITMAP (WM_USER+46)
#define TB_SETINDENT (WM_USER+47)
#define TB_SETIMAGELIST (WM_USER+48)
#define TB_GETIMAGELIST (WM_USER+49)
#define TB_LOADIMAGES (WM_USER+50)
#define TB_GETRECT (WM_USER+51)
#define TB_SETHOTIMAGELIST (WM_USER+52)
#define TB_GETHOTIMAGELIST (WM_USER+53)
#define TB_SETDISABLEDIMAGELIST (WM_USER+54)
#define TB_GETDISABLEDIMAGELIST (WM_USER+55)
#define TB_SETSTYLE (WM_USER+56)
#define TB_GETSTYLE (WM_USER+57)
#define TB_GETBUTTONSIZE (WM_USER+58)
#define TB_SETBUTTONWIDTH (WM_USER+59)
#define TB_SETMAXTEXTROWS (WM_USER+60)
#define TB_GETTEXTROWS (WM_USER+61)

#define TB_GETBUTTONTEXT __MINGW_NAME_AW(TB_GETBUTTONTEXT)
#define TB_SAVERESTORE __MINGW_NAME_AW(TB_SAVERESTORE)
#define TB_ADDSTRING __MINGW_NAME_AW(TB_ADDSTRING)

#define TB_GETOBJECT (WM_USER+62)
#define TB_GETHOTITEM (WM_USER+71)
#define TB_SETHOTITEM (WM_USER+72)
#define TB_SETANCHORHIGHLIGHT (WM_USER+73)
#define TB_GETANCHORHIGHLIGHT (WM_USER+74)
#define TB_MAPACCELERATORA (WM_USER+78)

#define TB_SETBOUNDINGSIZE (WM_USER+93)

  typedef struct {
    int iButton;
    DWORD dwFlags;
  } TBINSERTMARK,*LPTBINSERTMARK;
#define TBIMHT_AFTER 0x1
#define TBIMHT_BACKGROUND 0x2

#define TB_GETINSERTMARK (WM_USER+79)
#define TB_SETINSERTMARK (WM_USER+80)
#define TB_INSERTMARKHITTEST (WM_USER+81)
#define TB_MOVEBUTTON (WM_USER+82)
#define TB_GETMAXSIZE (WM_USER+83)
#define TB_SETEXTENDEDSTYLE (WM_USER+84)
#define TB_GETEXTENDEDSTYLE (WM_USER+85)
#define TB_GETPADDING (WM_USER+86)
#define TB_SETPADDING (WM_USER+87)
#define TB_SETINSERTMARKCOLOR (WM_USER+88)
#define TB_GETINSERTMARKCOLOR (WM_USER+89)

#define TB_SETCOLORSCHEME CCM_SETCOLORSCHEME
#define TB_GETCOLORSCHEME CCM_GETCOLORSCHEME

#define TB_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define TB_GETUNICODEFORMAT CCM_GETUNICODEFORMAT

#define TB_MAPACCELERATORW (WM_USER+90)

#define TB_MAPACCELERATOR __MINGW_NAME_AW(TB_MAPACCELERATOR)

  typedef struct {
    HINSTANCE hInstOld;
    UINT_PTR nIDOld;
    HINSTANCE hInstNew;
    UINT_PTR nIDNew;
    int nButtons;
  } TBREPLACEBITMAP,*LPTBREPLACEBITMAP;

#define TBBF_LARGE 0x1

#define TB_GETBITMAPFLAGS (WM_USER+41)

#define TBIF_IMAGE 0x1
#define TBIF_TEXT 0x2
#define TBIF_STATE 0x4
#define TBIF_STYLE 0x8
#define TBIF_LPARAM 0x10
#define TBIF_COMMAND 0x20
#define TBIF_SIZE 0x40
#define TBIF_BYINDEX 0x80000000

  typedef struct {
    UINT cbSize;
    DWORD dwMask;
    int idCommand;
    int iImage;
    BYTE fsState;
    BYTE fsStyle;
    WORD cx;
    DWORD_PTR lParam;
    LPSTR pszText;
    int cchText;
  } TBBUTTONINFOA,*LPTBBUTTONINFOA;

  typedef struct {
    UINT cbSize;
    DWORD dwMask;
    int idCommand;
    int iImage;
    BYTE fsState;
    BYTE fsStyle;
    WORD cx;
    DWORD_PTR lParam;
    LPWSTR pszText;
    int cchText;
  } TBBUTTONINFOW,*LPTBBUTTONINFOW;

#define TBBUTTONINFO __MINGW_NAME_AW(TBBUTTONINFO)
#define LPTBBUTTONINFO __MINGW_NAME_AW(LPTBBUTTONINFO)

#define TB_GETBUTTONINFOW (WM_USER+63)
#define TB_SETBUTTONINFOW (WM_USER+64)
#define TB_GETBUTTONINFOA (WM_USER+65)
#define TB_SETBUTTONINFOA (WM_USER+66)

#define TB_GETBUTTONINFO __MINGW_NAME_AW(TB_GETBUTTONINFO)
#define TB_SETBUTTONINFO __MINGW_NAME_AW(TB_SETBUTTONINFO)

#define TB_INSERTBUTTONW (WM_USER+67)
#define TB_ADDBUTTONSW (WM_USER+68)

#define TB_HITTEST (WM_USER+69)

#define TB_INSERTBUTTON __MINGW_NAME_AW(TB_INSERTBUTTON)
#define TB_ADDBUTTONS __MINGW_NAME_AW(TB_ADDBUTTONS)

#define TB_SETDRAWTEXTFLAGS (WM_USER+70)

#define TB_GETSTRINGW (WM_USER+91)
#define TB_GETSTRINGA (WM_USER+92)

#define TB_GETSTRING __MINGW_NAME_AW(TB_GETSTRING)

#define TB_SETHOTITEM2 (WM_USER+94)
#define TB_HASACCELERATOR (WM_USER+95)
#define TB_SETLISTGAP (WM_USER+96)
#define TB_GETIMAGELISTCOUNT (WM_USER+98)
#define TB_GETIDEALSIZE (WM_USER+99)
#define TB_TRANSLATEACCELERATOR CCM_TRANSLATEACCELERATOR

#define TBMF_PAD 0x1
#define TBMF_BARPAD 0x2
#define TBMF_BUTTONSPACING 0x4

  typedef struct {
    UINT cbSize;
    DWORD dwMask;
    int cxPad;
    int cyPad;
    int cxBarPad;
    int cyBarPad;
    int cxButtonSpacing;
    int cyButtonSpacing;
  } TBMETRICS,*LPTBMETRICS;

#define TB_GETMETRICS (WM_USER+101)
#define TB_SETMETRICS (WM_USER+102)
#define TB_SETWINDOWTHEME CCM_SETWINDOWTHEME

#define TBN_GETBUTTONINFOA (TBN_FIRST-0)
#define TBN_BEGINDRAG (TBN_FIRST-1)
#define TBN_ENDDRAG (TBN_FIRST-2)
#define TBN_BEGINADJUST (TBN_FIRST-3)
#define TBN_ENDADJUST (TBN_FIRST-4)
#define TBN_RESET (TBN_FIRST-5)
#define TBN_QUERYINSERT (TBN_FIRST-6)
#define TBN_QUERYDELETE (TBN_FIRST-7)
#define TBN_TOOLBARCHANGE (TBN_FIRST-8)
#define TBN_CUSTHELP (TBN_FIRST-9)
#define TBN_DROPDOWN (TBN_FIRST - 10)
#define TBN_GETOBJECT (TBN_FIRST - 12)
  typedef struct tagNMTBHOTITEM {
    NMHDR hdr;
    int idOld;
    int idNew;
    DWORD dwFlags;
  } NMTBHOTITEM,*LPNMTBHOTITEM;

#define HICF_OTHER 0x0
#define HICF_MOUSE 0x1
#define HICF_ARROWKEYS 0x2
#define HICF_ACCELERATOR 0x4
#define HICF_DUPACCEL 0x8
#define HICF_ENTERING 0x10
#define HICF_LEAVING 0x20
#define HICF_RESELECT 0x40
#define HICF_LMOUSE 0x80
#define HICF_TOGGLEDROPDOWN 0x100

#define TBN_HOTITEMCHANGE (TBN_FIRST - 13)
#define TBN_DRAGOUT (TBN_FIRST - 14)
#define TBN_DELETINGBUTTON (TBN_FIRST - 15)
#define TBN_GETDISPINFOA (TBN_FIRST - 16)
#define TBN_GETDISPINFOW (TBN_FIRST - 17)
#define TBN_GETINFOTIPA (TBN_FIRST - 18)
#define TBN_GETINFOTIPW (TBN_FIRST - 19)
#define TBN_GETBUTTONINFOW (TBN_FIRST - 20)
#define TBN_RESTORE (TBN_FIRST - 21)
#define TBN_SAVE (TBN_FIRST - 22)
#define TBN_INITCUSTOMIZE (TBN_FIRST - 23)
#define TBN_WRAPHOTITEM (TBN_FIRST - 24)
#define TBN_DUPACCELERATOR (TBN_FIRST - 25)
#define TBN_WRAPACCELERATOR (TBN_FIRST - 26)
#define TBN_DRAGOVER (TBN_FIRST - 27)
#define TBN_MAPACCELERATOR (TBN_FIRST - 28)
#define TBNRF_HIDEHELP 0x1
#define TBNRF_ENDCUSTOMIZE 0x2

  typedef struct tagNMTBSAVE {
    NMHDR hdr;
    DWORD *pData;
    DWORD *pCurrent;
    UINT cbData;
    int iItem;
    int cButtons;
    TBBUTTON tbButton;
  } NMTBSAVE,*LPNMTBSAVE;

  typedef struct tagNMTBRESTORE {
    NMHDR hdr;
    DWORD *pData;
    DWORD *pCurrent;
    UINT cbData;
    int iItem;
    int cButtons;
    int cbBytesPerRecord;
    TBBUTTON tbButton;
  } NMTBRESTORE,*LPNMTBRESTORE;

  typedef struct tagNMTBGETINFOTIPA {
    NMHDR hdr;
    LPSTR pszText;
    int cchTextMax;
    int iItem;
    LPARAM lParam;
  } NMTBGETINFOTIPA,*LPNMTBGETINFOTIPA;

  typedef struct tagNMTBGETINFOTIPW {
    NMHDR hdr;
    LPWSTR pszText;
    int cchTextMax;
    int iItem;
    LPARAM lParam;
  } NMTBGETINFOTIPW,*LPNMTBGETINFOTIPW;

#define TBN_GETINFOTIP __MINGW_NAME_AW(TBN_GETINFOTIP)
#define NMTBGETINFOTIP __MINGW_NAME_AW(NMTBGETINFOTIP)
#define LPNMTBGETINFOTIP __MINGW_NAME_AW(LPNMTBGETINFOTIP)

#define TBNF_IMAGE 0x1
#define TBNF_TEXT 0x2
#define TBNF_DI_SETITEM 0x10000000

  typedef struct {
    NMHDR hdr;
    DWORD dwMask;
    int idCommand;
    DWORD_PTR lParam;
    int iImage;
    LPSTR pszText;
    int cchText;
  } NMTBDISPINFOA,*LPNMTBDISPINFOA;

  typedef struct {
    NMHDR hdr;
    DWORD dwMask;
    int idCommand;
    DWORD_PTR lParam;
    int iImage;
    LPWSTR pszText;
    int cchText;
  } NMTBDISPINFOW,*LPNMTBDISPINFOW;

#define TBN_GETDISPINFO __MINGW_NAME_AW(TBN_GETDISPINFO)
#define NMTBDISPINFO __MINGW_NAME_AW(NMTBDISPINFO)
#define LPNMTBDISPINFO __MINGW_NAME_AW(LPNMTBDISPINFO)

#define TBDDRET_DEFAULT 0
#define TBDDRET_NODEFAULT 1
#define TBDDRET_TREATPRESSED 2

#define TBN_GETBUTTONINFO __MINGW_NAME_AW(TBN_GETBUTTONINFO)

#define TBNOTIFYA NMTOOLBARA
#define TBNOTIFYW NMTOOLBARW
#define LPTBNOTIFYA LPNMTOOLBARA
#define LPTBNOTIFYW LPNMTOOLBARW

#define TBNOTIFY NMTOOLBAR
#define LPTBNOTIFY LPNMTOOLBAR

  typedef struct tagNMTOOLBARA {
    NMHDR hdr;
    int iItem;
    TBBUTTON tbButton;
    int cchText;
    LPSTR pszText;
    RECT rcButton;
  } NMTOOLBARA,*LPNMTOOLBARA;

  typedef struct tagNMTOOLBARW {
    NMHDR hdr;
    int iItem;
    TBBUTTON tbButton;
    int cchText;
    LPWSTR pszText;
    RECT rcButton;
  } NMTOOLBARW,*LPNMTOOLBARW;

#define NMTOOLBAR __MINGW_NAME_AW(NMTOOLBAR)
#define LPNMTOOLBAR __MINGW_NAME_AW(LPNMTOOLBAR)

#endif /* !NOTOOLBAR */

#ifndef NOREBAR

#define REBARCLASSNAMEW L"ReBarWindow32"
#define REBARCLASSNAMEA "ReBarWindow32"

#define REBARCLASSNAME __MINGW_NAME_AW(REBARCLASSNAME)

#define RBIM_IMAGELIST 0x1

#define RBS_TOOLTIPS 0x100
#define RBS_VARHEIGHT 0x200
#define RBS_BANDBORDERS 0x400
#define RBS_FIXEDORDER 0x800
#define RBS_REGISTERDROP 0x1000
#define RBS_AUTOSIZE 0x2000
#define RBS_VERTICALGRIPPER 0x4000
#define RBS_DBLCLKTOGGLE 0x8000

  typedef struct tagREBARINFO {
    UINT cbSize;
    UINT fMask;
#ifndef NOIMAGEAPIS
    HIMAGELIST himl;
#else
    HANDLE himl;
#endif
  } REBARINFO,*LPREBARINFO;

#define RBBS_BREAK 0x1
#define RBBS_FIXEDSIZE 0x2
#define RBBS_CHILDEDGE 0x4
#define RBBS_HIDDEN 0x8
#define RBBS_NOVERT 0x10
#define RBBS_FIXEDBMP 0x20
#define RBBS_VARIABLEHEIGHT 0x40
#define RBBS_GRIPPERALWAYS 0x80
#define RBBS_NOGRIPPER 0x100
#define RBBS_USECHEVRON 0x200
#define RBBS_HIDETITLE 0x400
#define RBBS_TOPALIGN 0x800

#define RBBIM_STYLE 0x1
#define RBBIM_COLORS 0x2
#define RBBIM_TEXT 0x4
#define RBBIM_IMAGE 0x8
#define RBBIM_CHILD 0x10
#define RBBIM_CHILDSIZE 0x20
#define RBBIM_SIZE 0x40
#define RBBIM_BACKGROUND 0x80
#define RBBIM_ID 0x100
#define RBBIM_IDEALSIZE 0x200
#define RBBIM_LPARAM 0x400
#define RBBIM_HEADERSIZE 0x800

  typedef struct tagREBARBANDINFOA {
    UINT cbSize;
    UINT fMask;
    UINT fStyle;
    COLORREF clrFore;
    COLORREF clrBack;
    LPSTR lpText;
    UINT cch;
    int iImage;
    HWND hwndChild;
    UINT cxMinChild;
    UINT cyMinChild;
    UINT cx;
    HBITMAP hbmBack;
    UINT wID;
    UINT cyChild;
    UINT cyMaxChild;
    UINT cyIntegral;
    UINT cxIdeal;
    LPARAM lParam;
    UINT cxHeader;
  } REBARBANDINFOA,*LPREBARBANDINFOA;
  typedef REBARBANDINFOA CONST *LPCREBARBANDINFOA;

#define REBARBANDINFOA_V3_SIZE CCSIZEOF_STRUCT(REBARBANDINFOA,wID)
#define REBARBANDINFOW_V3_SIZE CCSIZEOF_STRUCT(REBARBANDINFOW,wID)
#define REBARBANDINFOA_V6_SIZE CCSIZEOF_STRUCT(REBARBANDINFOA,cxHeader)
#define REBARBANDINFOW_V6_SIZE CCSIZEOF_STRUCT(REBARBANDINFOW,cxHeader)

  typedef struct tagREBARBANDINFOW {
    UINT cbSize;
    UINT fMask;
    UINT fStyle;
    COLORREF clrFore;
    COLORREF clrBack;
    LPWSTR lpText;
    UINT cch;
    int iImage;
    HWND hwndChild;
    UINT cxMinChild;
    UINT cyMinChild;
    UINT cx;
    HBITMAP hbmBack;
    UINT wID;
    UINT cyChild;
    UINT cyMaxChild;
    UINT cyIntegral;
    UINT cxIdeal;
    LPARAM lParam;
    UINT cxHeader;
  } REBARBANDINFOW,*LPREBARBANDINFOW;

  typedef REBARBANDINFOW CONST *LPCREBARBANDINFOW;

#define REBARBANDINFO __MINGW_NAME_AW(REBARBANDINFO)
#define LPREBARBANDINFO __MINGW_NAME_AW(LPREBARBANDINFO)
#define LPCREBARBANDINFO __MINGW_NAME_AW(LPCREBARBANDINFO)

#define REBARBANDINFO_V3_SIZE __MINGW_NAME_AW_EXT(REBARBANDINFO,_V3_SIZE)

#define RB_INSERTBANDA (WM_USER+1)
#define RB_DELETEBAND (WM_USER+2)
#define RB_GETBARINFO (WM_USER+3)
#define RB_SETBARINFO (WM_USER+4)
#define RB_SETBANDINFOA (WM_USER+6)
#define RB_SETPARENT (WM_USER+7)
#define RB_HITTEST (WM_USER+8)
#define RB_GETRECT (WM_USER+9)
#define RB_INSERTBANDW (WM_USER+10)
#define RB_SETBANDINFOW (WM_USER+11)
#define RB_GETBANDCOUNT (WM_USER+12)
#define RB_GETROWCOUNT (WM_USER+13)
#define RB_GETROWHEIGHT (WM_USER+14)
#define RB_IDTOINDEX (WM_USER+16)
#define RB_GETTOOLTIPS (WM_USER+17)
#define RB_SETTOOLTIPS (WM_USER+18)
#define RB_SETBKCOLOR (WM_USER+19)
#define RB_GETBKCOLOR (WM_USER+20)
#define RB_SETTEXTCOLOR (WM_USER+21)
#define RB_GETTEXTCOLOR (WM_USER+22)

#define RBSTR_CHANGERECT 0x1

#define RB_SIZETORECT (WM_USER+23)
#define RB_SETCOLORSCHEME CCM_SETCOLORSCHEME
#define RB_GETCOLORSCHEME CCM_GETCOLORSCHEME

#define RB_INSERTBAND __MINGW_NAME_AW(RB_INSERTBAND)
#define RB_SETBANDINFO __MINGW_NAME_AW(RB_SETBANDINFO)

#define RB_BEGINDRAG (WM_USER+24)
#define RB_ENDDRAG (WM_USER+25)
#define RB_DRAGMOVE (WM_USER+26)
#define RB_GETBARHEIGHT (WM_USER+27)
#define RB_GETBANDINFOW (WM_USER+28)
#define RB_GETBANDINFOA (WM_USER+29)

#define RB_GETBANDINFO __MINGW_NAME_AW(RB_GETBANDINFO)

#define RB_MINIMIZEBAND (WM_USER+30)
#define RB_MAXIMIZEBAND (WM_USER+31)
#define RB_GETDROPTARGET (CCM_GETDROPTARGET)
#define RB_GETBANDBORDERS (WM_USER+34)
#define RB_SHOWBAND (WM_USER+35)
#define RB_SETPALETTE (WM_USER+37)
#define RB_GETPALETTE (WM_USER+38)
#define RB_MOVEBAND (WM_USER+39)
#define RB_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define RB_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define RB_GETBANDMARGINS (WM_USER+40)
#define RB_SETWINDOWTHEME CCM_SETWINDOWTHEME
#define RB_PUSHCHEVRON (WM_USER+43)

#define RBN_HEIGHTCHANGE (RBN_FIRST - 0)
#define RBN_GETOBJECT (RBN_FIRST - 1)
#define RBN_LAYOUTCHANGED (RBN_FIRST - 2)
#define RBN_AUTOSIZE (RBN_FIRST - 3)
#define RBN_BEGINDRAG (RBN_FIRST - 4)
#define RBN_ENDDRAG (RBN_FIRST - 5)
#define RBN_DELETINGBAND (RBN_FIRST - 6)
#define RBN_DELETEDBAND (RBN_FIRST - 7)
#define RBN_CHILDSIZE (RBN_FIRST - 8)
#define RBN_CHEVRONPUSHED (RBN_FIRST - 10)
#define RBN_MINMAX (RBN_FIRST - 21)
#define RBN_AUTOBREAK (RBN_FIRST - 22)

  typedef struct tagNMREBARCHILDSIZE {
    NMHDR hdr;
    UINT uBand;
    UINT wID;
    RECT rcChild;
    RECT rcBand;
  } NMREBARCHILDSIZE,*LPNMREBARCHILDSIZE;

  typedef struct tagNMREBAR {
    NMHDR hdr;
    DWORD dwMask;
    UINT uBand;
    UINT fStyle;
    UINT wID;
    LPARAM lParam;
  } NMREBAR,*LPNMREBAR;

#define RBNM_ID 0x1
#define RBNM_STYLE 0x2
#define RBNM_LPARAM 0x4

  typedef struct tagNMRBAUTOSIZE {
    NMHDR hdr;
    BOOL fChanged;
    RECT rcTarget;
    RECT rcActual;
  } NMRBAUTOSIZE,*LPNMRBAUTOSIZE;

  typedef struct tagNMREBARCHEVRON {
    NMHDR hdr;
    UINT uBand;
    UINT wID;
    LPARAM lParam;
    RECT rc;
    LPARAM lParamNM;
  } NMREBARCHEVRON,*LPNMREBARCHEVRON;

#define RBAB_AUTOSIZE 0x1
#define RBAB_ADDBAND 0x2

  typedef struct tagNMREBARAUTOBREAK {
    NMHDR hdr;
    UINT uBand;
    UINT wID;
    LPARAM lParam;
    UINT uMsg;
    UINT fStyleCurrent;
    BOOL fAutoBreak;
  } NMREBARAUTOBREAK,*LPNMREBARAUTOBREAK;

#define RBHT_NOWHERE 0x1
#define RBHT_CAPTION 0x2
#define RBHT_CLIENT 0x3
#define RBHT_GRABBER 0x4
#define RBHT_CHEVRON 0x8

  typedef struct _RB_HITTESTINFO {
    POINT pt;
    UINT flags;
    int iBand;
  } RBHITTESTINFO,*LPRBHITTESTINFO;

#endif /* !NOREBAR */

#ifndef NOTOOLTIPS

#define TOOLTIPS_CLASSW L"tooltips_class32"
#define TOOLTIPS_CLASSA "tooltips_class32"

#define TOOLTIPS_CLASS __MINGW_NAME_AW(TOOLTIPS_CLASS)

#define LPTOOLINFOA LPTTTOOLINFOA
#define LPTOOLINFOW LPTTTOOLINFOW
#define TOOLINFOA TTTOOLINFOA
#define TOOLINFOW TTTOOLINFOW

#define LPTOOLINFO LPTTTOOLINFO
#define TOOLINFO TTTOOLINFO

#define TTTOOLINFOA_V1_SIZE CCSIZEOF_STRUCT(TTTOOLINFOA,lpszText)
#define TTTOOLINFOW_V1_SIZE CCSIZEOF_STRUCT(TTTOOLINFOW,lpszText)
#define TTTOOLINFOA_V2_SIZE CCSIZEOF_STRUCT(TTTOOLINFOA,lParam)
#define TTTOOLINFOW_V2_SIZE CCSIZEOF_STRUCT(TTTOOLINFOW,lParam)
#define TTTOOLINFOA_V3_SIZE CCSIZEOF_STRUCT(TTTOOLINFOA,lpReserved)
#define TTTOOLINFOW_V3_SIZE CCSIZEOF_STRUCT(TTTOOLINFOW,lpReserved)

  typedef struct tagTOOLINFOA {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT_PTR uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
    LPARAM lParam;
    void *lpReserved;
  } TTTOOLINFOA,NEAR *PTOOLINFOA,*LPTTTOOLINFOA;

  typedef struct tagTOOLINFOW {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT_PTR uId;
    RECT rect;
    HINSTANCE hinst;
    LPWSTR lpszText;
    LPARAM lParam;
    void *lpReserved;
  } TTTOOLINFOW,NEAR *PTOOLINFOW,*LPTTTOOLINFOW;

#define TTTOOLINFO __MINGW_NAME_AW(TTTOOLINFO)
#define PTOOLINFO __MINGW_NAME_AW(PTOOLINFO)
#define LPTTTOOLINFO __MINGW_NAME_AW(LPTTTOOLINFO)

#define TTTOOLINFO_V1_SIZE __MINGW_NAME_AW_EXT(TTTOOLINFO,_V1_SIZE)

#define TTS_ALWAYSTIP 0x1
#define TTS_NOPREFIX 0x2
#define TTS_NOANIMATE 0x10
#define TTS_NOFADE 0x20
#define TTS_BALLOON 0x40
#define TTS_CLOSE 0x80

#define TTF_IDISHWND 0x1
#define TTF_CENTERTIP 0x2
#define TTF_RTLREADING 0x4
#define TTF_SUBCLASS 0x10
#define TTF_TRACK 0x20
#define TTF_ABSOLUTE 0x80
#define TTF_TRANSPARENT 0x100
#define TTF_PARSELINKS 0x1000
#define TTF_DI_SETITEM 0x8000

#define TTDT_AUTOMATIC 0
#define TTDT_RESHOW 1
#define TTDT_AUTOPOP 2
#define TTDT_INITIAL 3

#define TTI_NONE 0
#define TTI_INFO 1
#define TTI_WARNING 2
#define TTI_ERROR 3

#define TTM_ACTIVATE (WM_USER+1)
#define TTM_SETDELAYTIME (WM_USER+3)
#define TTM_ADDTOOLA (WM_USER+4)
#define TTM_ADDTOOLW (WM_USER+50)
#define TTM_DELTOOLA (WM_USER+5)
#define TTM_DELTOOLW (WM_USER+51)
#define TTM_NEWTOOLRECTA (WM_USER+6)
#define TTM_NEWTOOLRECTW (WM_USER+52)
#define TTM_RELAYEVENT (WM_USER+7)

#define TTM_GETTOOLINFOA (WM_USER+8)
#define TTM_GETTOOLINFOW (WM_USER+53)

#define TTM_SETTOOLINFOA (WM_USER+9)
#define TTM_SETTOOLINFOW (WM_USER+54)

#define TTM_HITTESTA (WM_USER +10)
#define TTM_HITTESTW (WM_USER +55)
#define TTM_GETTEXTA (WM_USER +11)
#define TTM_GETTEXTW (WM_USER +56)
#define TTM_UPDATETIPTEXTA (WM_USER +12)
#define TTM_UPDATETIPTEXTW (WM_USER +57)
#define TTM_GETTOOLCOUNT (WM_USER +13)
#define TTM_ENUMTOOLSA (WM_USER +14)
#define TTM_ENUMTOOLSW (WM_USER +58)
#define TTM_GETCURRENTTOOLA (WM_USER+15)
#define TTM_GETCURRENTTOOLW (WM_USER+59)
#define TTM_WINDOWFROMPOINT (WM_USER+16)
#define TTM_TRACKACTIVATE (WM_USER+17)
#define TTM_TRACKPOSITION (WM_USER+18)
#define TTM_SETTIPBKCOLOR (WM_USER+19)
#define TTM_SETTIPTEXTCOLOR (WM_USER+20)
#define TTM_GETDELAYTIME (WM_USER+21)
#define TTM_GETTIPBKCOLOR (WM_USER+22)
#define TTM_GETTIPTEXTCOLOR (WM_USER+23)
#define TTM_SETMAXTIPWIDTH (WM_USER+24)
#define TTM_GETMAXTIPWIDTH (WM_USER+25)
#define TTM_SETMARGIN (WM_USER+26)
#define TTM_GETMARGIN (WM_USER+27)
#define TTM_POP (WM_USER+28)
#define TTM_UPDATE (WM_USER+29)
#define TTM_GETBUBBLESIZE (WM_USER+30)
#define TTM_ADJUSTRECT (WM_USER+31)
#define TTM_SETTITLEA (WM_USER+32)
#define TTM_SETTITLEW (WM_USER+33)

#define TTM_POPUP (WM_USER+34)
#define TTM_GETTITLE (WM_USER+35)
  typedef struct _TTGETTITLE {
    DWORD dwSize;
    UINT uTitleBitmap;
    UINT cch;
    WCHAR *pszTitle;
  } TTGETTITLE,*PTTGETTITLE;

#define TTM_ADDTOOL __MINGW_NAME_AW(TTM_ADDTOOL)
#define TTM_DELTOOL __MINGW_NAME_AW(TTM_DELTOOL)
#define TTM_NEWTOOLRECT __MINGW_NAME_AW(TTM_NEWTOOLRECT)
#define TTM_GETTOOLINFO __MINGW_NAME_AW(TTM_GETTOOLINFO)
#define TTM_SETTOOLINFO __MINGW_NAME_AW(TTM_SETTOOLINFO)
#define TTM_HITTEST __MINGW_NAME_AW(TTM_HITTEST)
#define TTM_GETTEXT __MINGW_NAME_AW(TTM_GETTEXT)
#define TTM_UPDATETIPTEXT __MINGW_NAME_AW(TTM_UPDATETIPTEXT)
#define TTM_ENUMTOOLS __MINGW_NAME_AW(TTM_ENUMTOOLS)
#define TTM_GETCURRENTTOOL __MINGW_NAME_AW(TTM_GETCURRENTTOOL)
#define TTM_SETTITLE __MINGW_NAME_AW(TTM_SETTITLE)

#define TTM_SETWINDOWTHEME CCM_SETWINDOWTHEME

#define LPHITTESTINFOW LPTTHITTESTINFOW
#define LPHITTESTINFOA LPTTHITTESTINFOA
#define LPHITTESTINFO LPTTHITTESTINFO

  typedef struct _TT_HITTESTINFOA {
    HWND hwnd;
    POINT pt;
    TTTOOLINFOA ti;
  } TTHITTESTINFOA,*LPTTHITTESTINFOA;

  typedef struct _TT_HITTESTINFOW {
    HWND hwnd;
    POINT pt;
    TTTOOLINFOW ti;
  } TTHITTESTINFOW,*LPTTHITTESTINFOW;

#define TTHITTESTINFO __MINGW_NAME_AW(TTHITTESTINFO)
#define LPTTHITTESTINFO __MINGW_NAME_AW(LPTTHITTESTINFO)

#define TTN_GETDISPINFOA (TTN_FIRST - 0)
#define TTN_GETDISPINFOW (TTN_FIRST - 10)
#define TTN_SHOW (TTN_FIRST - 1)
#define TTN_POP (TTN_FIRST - 2)
#define TTN_LINKCLICK (TTN_FIRST - 3)

#define TTN_GETDISPINFO __MINGW_NAME_AW(TTN_GETDISPINFO)

#define TTN_NEEDTEXT TTN_GETDISPINFO
#define TTN_NEEDTEXTA TTN_GETDISPINFOA
#define TTN_NEEDTEXTW TTN_GETDISPINFOW

#define TOOLTIPTEXTW NMTTDISPINFOW
#define TOOLTIPTEXTA NMTTDISPINFOA
#define LPTOOLTIPTEXTA LPNMTTDISPINFOA
#define LPTOOLTIPTEXTW LPNMTTDISPINFOW

#define TOOLTIPTEXT NMTTDISPINFO
#define LPTOOLTIPTEXT LPNMTTDISPINFO

#define NMTTDISPINFOA_V1_SIZE CCSIZEOF_STRUCT(NMTTDISPINFOA,uFlags)
#define NMTTDISPINFOW_V1_SIZE CCSIZEOF_STRUCT(NMTTDISPINFOW,uFlags)

  typedef struct tagNMTTDISPINFOA {
    NMHDR hdr;
    LPSTR lpszText;
    char szText[80];
    HINSTANCE hinst;
    UINT uFlags;
    LPARAM lParam;
  } NMTTDISPINFOA,*LPNMTTDISPINFOA;

  typedef struct tagNMTTDISPINFOW {
    NMHDR hdr;
    LPWSTR lpszText;
    WCHAR szText[80];
    HINSTANCE hinst;
    UINT uFlags;
    LPARAM lParam;
  } NMTTDISPINFOW,*LPNMTTDISPINFOW;


#define NMTTDISPINFO __MINGW_NAME_AW(NMTTDISPINFO)
#define LPNMTTDISPINFO __MINGW_NAME_AW(LPNMTTDISPINFO)

#define NMTTDISPINFO_V1_SIZE __MINGW_NAME_AW_EXT(NMTTDISPINFO,_V1_SIZE)

#endif /* !NOTOOLTIPS */

#ifndef NOSTATUSBAR

#define SBARS_SIZEGRIP 0x100
#define SBARS_TOOLTIPS 0x800
#define SBT_TOOLTIPS 0x800

  WINCOMMCTRLAPI void WINAPI DrawStatusTextA(HDC hDC,LPCRECT lprc,LPCSTR pszText,UINT uFlags);
  WINCOMMCTRLAPI void WINAPI DrawStatusTextW(HDC hDC,LPCRECT lprc,LPCWSTR pszText,UINT uFlags);

  WINCOMMCTRLAPI HWND WINAPI CreateStatusWindowA(LONG style,LPCSTR lpszText,HWND hwndParent,UINT wID);
  WINCOMMCTRLAPI HWND WINAPI CreateStatusWindowW(LONG style,LPCWSTR lpszText,HWND hwndParent,UINT wID);

#define CreateStatusWindow __MINGW_NAME_AW(CreateStatusWindow)
#define DrawStatusText __MINGW_NAME_AW(DrawStatusText)

#define STATUSCLASSNAMEW L"msctls_statusbar32"
#define STATUSCLASSNAMEA "msctls_statusbar32"

#define STATUSCLASSNAME __MINGW_NAME_AW(STATUSCLASSNAME)

#define SB_SETTEXTA (WM_USER+1)
#define SB_SETTEXTW (WM_USER+11)
#define SB_GETTEXTA (WM_USER+2)
#define SB_GETTEXTW (WM_USER+13)
#define SB_GETTEXTLENGTHA (WM_USER+3)
#define SB_GETTEXTLENGTHW (WM_USER+12)

#define SB_GETTEXT __MINGW_NAME_AW(SB_GETTEXT)
#define SB_SETTEXT __MINGW_NAME_AW(SB_SETTEXT)
#define SB_GETTEXTLENGTH __MINGW_NAME_AW(SB_GETTEXTLENGTH)
#define SB_SETTIPTEXT __MINGW_NAME_AW(SB_SETTIPTEXT)
#define SB_GETTIPTEXT __MINGW_NAME_AW(SB_GETTIPTEXT)

#define SB_SETPARTS (WM_USER+4)
#define SB_GETPARTS (WM_USER+6)
#define SB_GETBORDERS (WM_USER+7)
#define SB_SETMINHEIGHT (WM_USER+8)
#define SB_SIMPLE (WM_USER+9)
#define SB_GETRECT (WM_USER+10)
#define SB_ISSIMPLE (WM_USER+14)
#define SB_SETICON (WM_USER+15)
#define SB_SETTIPTEXTA (WM_USER+16)
#define SB_SETTIPTEXTW (WM_USER+17)
#define SB_GETTIPTEXTA (WM_USER+18)
#define SB_GETTIPTEXTW (WM_USER+19)
#define SB_GETICON (WM_USER+20)
#define SB_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define SB_GETUNICODEFORMAT CCM_GETUNICODEFORMAT

#define SBT_OWNERDRAW 0x1000
#define SBT_NOBORDERS 0x100
#define SBT_POPOUT 0x200
#define SBT_RTLREADING 0x400
#define SBT_NOTABPARSING 0x800

#define SB_SETBKCOLOR CCM_SETBKCOLOR

#define SBN_SIMPLEMODECHANGE (SBN_FIRST - 0)

#define SB_SIMPLEID 0xff

#endif /* !NOSTATUSBAR */

#ifndef NOMENUHELP

  WINCOMMCTRLAPI void WINAPI MenuHelp(UINT uMsg,WPARAM wParam,LPARAM lParam,HMENU hMainMenu,HINSTANCE hInst,HWND hwndStatus,UINT *lpwIDs);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  ShowHideMenuCtl(
    _In_ HWND hWnd,
    _In_ UINT_PTR uFlags,
    _In_z_ LPINT lpInfo);

  WINCOMMCTRLAPI
  void
  WINAPI
  GetEffectiveClientRect(
    _In_ HWND hWnd,
    _Out_ LPRECT lprc,
    _In_z_ const INT *lpInfo);

#define MINSYSCOMMAND SC_SIZE

#endif /* !NOMENUHELP */

#ifndef NOTRACKBAR

#define TRACKBAR_CLASSA "msctls_trackbar32"
#define TRACKBAR_CLASSW L"msctls_trackbar32"

#define TRACKBAR_CLASS __MINGW_NAME_AW(TRACKBAR_CLASS)

#define TBS_AUTOTICKS 0x1
#define TBS_VERT 0x2
#define TBS_HORZ 0x0
#define TBS_TOP 0x4
#define TBS_BOTTOM 0x0
#define TBS_LEFT 0x4
#define TBS_RIGHT 0x0
#define TBS_BOTH 0x8
#define TBS_NOTICKS 0x10
#define TBS_ENABLESELRANGE 0x20
#define TBS_FIXEDLENGTH 0x40
#define TBS_NOTHUMB 0x80
#define TBS_TOOLTIPS 0x100
#define TBS_REVERSED 0x200
#define TBS_DOWNISLEFT 0x400

#define TBM_GETPOS (WM_USER)
#define TBM_GETRANGEMIN (WM_USER+1)
#define TBM_GETRANGEMAX (WM_USER+2)
#define TBM_GETTIC (WM_USER+3)
#define TBM_SETTIC (WM_USER+4)
#define TBM_SETPOS (WM_USER+5)
#define TBM_SETRANGE (WM_USER+6)
#define TBM_SETRANGEMIN (WM_USER+7)
#define TBM_SETRANGEMAX (WM_USER+8)
#define TBM_CLEARTICS (WM_USER+9)
#define TBM_SETSEL (WM_USER+10)
#define TBM_SETSELSTART (WM_USER+11)
#define TBM_SETSELEND (WM_USER+12)
#define TBM_GETPTICS (WM_USER+14)
#define TBM_GETTICPOS (WM_USER+15)
#define TBM_GETNUMTICS (WM_USER+16)
#define TBM_GETSELSTART (WM_USER+17)
#define TBM_GETSELEND (WM_USER+18)
#define TBM_CLEARSEL (WM_USER+19)
#define TBM_SETTICFREQ (WM_USER+20)
#define TBM_SETPAGESIZE (WM_USER+21)
#define TBM_GETPAGESIZE (WM_USER+22)
#define TBM_SETLINESIZE (WM_USER+23)
#define TBM_GETLINESIZE (WM_USER+24)
#define TBM_GETTHUMBRECT (WM_USER+25)
#define TBM_GETCHANNELRECT (WM_USER+26)
#define TBM_SETTHUMBLENGTH (WM_USER+27)
#define TBM_GETTHUMBLENGTH (WM_USER+28)
#define TBM_SETTOOLTIPS (WM_USER+29)
#define TBM_GETTOOLTIPS (WM_USER+30)
#define TBM_SETTIPSIDE (WM_USER+31)

#define TBTS_TOP 0
#define TBTS_LEFT 1
#define TBTS_BOTTOM 2
#define TBTS_RIGHT 3

#define TBM_SETBUDDY (WM_USER+32)
#define TBM_GETBUDDY (WM_USER+33)
#define TBM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define TBM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT

#define TB_LINEUP 0
#define TB_LINEDOWN 1
#define TB_PAGEUP 2
#define TB_PAGEDOWN 3
#define TB_THUMBPOSITION 4
#define TB_THUMBTRACK 5
#define TB_TOP 6
#define TB_BOTTOM 7
#define TB_ENDTRACK 8

#define TBCD_TICS 0x1
#define TBCD_THUMB 0x2
#define TBCD_CHANNEL 0x3

#endif /* !NOTRACKBAR */

#ifndef NODRAGLIST

  typedef struct tagDRAGLISTINFO {
    UINT uNotification;
    HWND hWnd;
    POINT ptCursor;
  } DRAGLISTINFO,*LPDRAGLISTINFO;

#define DL_BEGINDRAG (WM_USER+133)
#define DL_DRAGGING (WM_USER+134)
#define DL_DROPPED (WM_USER+135)
#define DL_CANCELDRAG (WM_USER+136)

#define DL_CURSORSET 0
#define DL_STOPCURSOR 1
#define DL_COPYCURSOR 2
#define DL_MOVECURSOR 3

#define DRAGLISTMSGSTRING TEXT("commctrl_DragListMsg")

  WINCOMMCTRLAPI BOOL WINAPI MakeDragList(HWND hLB);
  WINCOMMCTRLAPI void WINAPI DrawInsert(HWND handParent,HWND hLB,int nItem);

  WINCOMMCTRLAPI int WINAPI LBItemFromPt(HWND hLB,POINT pt,BOOL bAutoScroll);

#endif /* !NODRAGLIST */

#ifndef NOUPDOWN

#define UPDOWN_CLASSA "msctls_updown32"
#define UPDOWN_CLASSW L"msctls_updown32"

#define UPDOWN_CLASS __MINGW_NAME_AW(UPDOWN_CLASS)

  typedef struct _UDACCEL {
    UINT nSec;
    UINT nInc;
  } UDACCEL,*LPUDACCEL;

#define UD_MAXVAL 0x7fff
#define UD_MINVAL (-UD_MAXVAL)

#define UDS_WRAP 0x1
#define UDS_SETBUDDYINT 0x2
#define UDS_ALIGNRIGHT 0x4
#define UDS_ALIGNLEFT 0x8
#define UDS_AUTOBUDDY 0x10
#define UDS_ARROWKEYS 0x20
#define UDS_HORZ 0x40
#define UDS_NOTHOUSANDS 0x80
#define UDS_HOTTRACK 0x100

#define UDM_SETRANGE (WM_USER+101)
#define UDM_GETRANGE (WM_USER+102)
#define UDM_SETPOS (WM_USER+103)
#define UDM_GETPOS (WM_USER+104)
#define UDM_SETBUDDY (WM_USER+105)
#define UDM_GETBUDDY (WM_USER+106)
#define UDM_SETACCEL (WM_USER+107)
#define UDM_GETACCEL (WM_USER+108)
#define UDM_SETBASE (WM_USER+109)
#define UDM_GETBASE (WM_USER+110)
#define UDM_SETRANGE32 (WM_USER+111)
#define UDM_GETRANGE32 (WM_USER+112)
#define UDM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define UDM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define UDM_SETPOS32 (WM_USER+113)
#define UDM_GETPOS32 (WM_USER+114)

  WINCOMMCTRLAPI HWND WINAPI CreateUpDownControl(DWORD dwStyle,int x,int y,int cx,int cy,HWND hParent,int nID,HINSTANCE hInst,HWND hBuddy,int nUpper,int nLower,int nPos);

#define NM_UPDOWN NMUPDOWN
#define LPNM_UPDOWN LPNMUPDOWN

  typedef struct _NM_UPDOWN {
    NMHDR hdr;
    int iPos;
    int iDelta;
  } NMUPDOWN,*LPNMUPDOWN;

#define UDN_DELTAPOS (UDN_FIRST - 1)

#endif /* !NOUPDOWN */

#ifndef NOPROGRESS

#define PROGRESS_CLASSA "msctls_progress32"
#define PROGRESS_CLASSW L"msctls_progress32"

#define PROGRESS_CLASS __MINGW_NAME_AW(PROGRESS_CLASS)

#define PBS_SMOOTH 0x1
#define PBS_VERTICAL 0x4

#define PBM_SETRANGE (WM_USER+1)
#define PBM_SETPOS (WM_USER+2)
#define PBM_DELTAPOS (WM_USER+3)
#define PBM_SETSTEP (WM_USER+4)
#define PBM_STEPIT (WM_USER+5)
#define PBM_SETRANGE32 (WM_USER+6)
  typedef struct {
    int iLow;
    int iHigh;
  } PBRANGE,*PPBRANGE;
#define PBM_GETRANGE (WM_USER+7)
#define PBM_GETPOS (WM_USER+8)
#define PBM_SETBARCOLOR (WM_USER+9)
#define PBM_SETBKCOLOR CCM_SETBKCOLOR

#define PBS_MARQUEE 0x8
#define PBM_SETMARQUEE (WM_USER+10)

#if (_WIN32_WINNT >= 0x0600)
#define PBM_GETSTEP (WM_USER+13)
#define PBM_GETBKCOLOR (WM_USER+14)
#define PBM_GETBARCOLOR (WM_USER+15)
#define PBM_SETSTATE (WM_USER+16)
#define PBM_GETSTATE (WM_USER+17)
#define PBS_SMOOTHREVERSE 0x10
#define PBST_NORMAL 1
#define PBST_ERROR 2
#define PBST_PAUSED 3
#endif /* (_WIN32_WINNT >= 0x0600) */

#endif /* !NOPROGRESS */


#ifndef NOHOTKEY

#define HOTKEYF_SHIFT 0x1
#define HOTKEYF_CONTROL 0x2
#define HOTKEYF_ALT 0x4
#define HOTKEYF_EXT 0x8
#define HKCOMB_NONE 0x1
#define HKCOMB_S 0x2
#define HKCOMB_C 0x4
#define HKCOMB_A 0x8
#define HKCOMB_SC 0x10
#define HKCOMB_SA 0x20
#define HKCOMB_CA 0x40
#define HKCOMB_SCA 0x80

#define HKM_SETHOTKEY (WM_USER+1)
#define HKM_GETHOTKEY (WM_USER+2)
#define HKM_SETRULES (WM_USER+3)

#define HOTKEY_CLASSA "msctls_hotkey32"
#define HOTKEY_CLASSW L"msctls_hotkey32"

#define HOTKEY_CLASS __MINGW_NAME_AW(HOTKEY_CLASS)

#endif /* !NOHOTKEY */

#define CCS_TOP 0x1L
#define CCS_NOMOVEY 0x2L
#define CCS_BOTTOM 0x3L
#define CCS_NORESIZE 0x4L
#define CCS_NOPARENTALIGN 0x8L
#define CCS_ADJUSTABLE 0x20L
#define CCS_NODIVIDER 0x40L
#define CCS_VERT 0x80L
#define CCS_LEFT (CCS_VERT | CCS_TOP)
#define CCS_RIGHT (CCS_VERT | CCS_BOTTOM)
#define CCS_NOMOVEX (CCS_VERT | CCS_NOMOVEY)

#ifndef NOLISTVIEW

#define WC_LISTVIEWA "SysListView32"
#define WC_LISTVIEWW L"SysListView32"

#define WC_LISTVIEW __MINGW_NAME_AW(WC_LISTVIEW)

#define LVS_ICON 0x0
#define LVS_REPORT 0x1
#define LVS_SMALLICON 0x2
#define LVS_LIST 0x3
#define LVS_TYPEMASK 0x3
#define LVS_SINGLESEL 0x4
#define LVS_SHOWSELALWAYS 0x8
#define LVS_SORTASCENDING 0x10
#define LVS_SORTDESCENDING 0x20
#define LVS_SHAREIMAGELISTS 0x40
#define LVS_NOLABELWRAP 0x80
#define LVS_AUTOARRANGE 0x100
#define LVS_EDITLABELS 0x200
#define LVS_OWNERDATA 0x1000
#define LVS_NOSCROLL 0x2000

#define LVS_TYPESTYLEMASK 0xfc00

#define LVS_ALIGNTOP 0x0
#define LVS_ALIGNLEFT 0x800
#define LVS_ALIGNMASK 0xc00

#define LVS_OWNERDRAWFIXED 0x400
#define LVS_NOCOLUMNHEADER 0x4000
#define LVS_NOSORTHEADER 0x8000

#define LVM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define ListView_SetUnicodeFormat(hwnd,fUnicode) (BOOL)SNDMSG((hwnd),LVM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)
#define LVM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define ListView_GetUnicodeFormat(hwnd) (BOOL)SNDMSG((hwnd),LVM_GETUNICODEFORMAT,0,0)
#define LVM_GETBKCOLOR (LVM_FIRST+0)
#define ListView_GetBkColor(hwnd) (COLORREF)SNDMSG((hwnd),LVM_GETBKCOLOR,0,0L)
#define LVM_SETBKCOLOR (LVM_FIRST+1)
#define ListView_SetBkColor(hwnd,clrBk) (BOOL)SNDMSG((hwnd),LVM_SETBKCOLOR,0,(LPARAM)(COLORREF)(clrBk))
#define LVM_GETIMAGELIST (LVM_FIRST+2)
#define ListView_GetImageList(hwnd,iImageList) (HIMAGELIST)SNDMSG((hwnd),LVM_GETIMAGELIST,(WPARAM)(INT)(iImageList),0L)

#define LVSIL_NORMAL            0
#define LVSIL_SMALL             1
#define LVSIL_STATE             2
#define LVSIL_GROUPHEADER       3

#define LVM_SETIMAGELIST (LVM_FIRST+3)
#define ListView_SetImageList(hwnd,himl,iImageList) (HIMAGELIST)SNDMSG((hwnd),LVM_SETIMAGELIST,(WPARAM)(iImageList),(LPARAM)(HIMAGELIST)(himl))

#define LVM_GETITEMCOUNT (LVM_FIRST+4)
#define ListView_GetItemCount(hwnd) (int)SNDMSG((hwnd),LVM_GETITEMCOUNT,0,0L)

#define LVIF_TEXT 0x1
#define LVIF_IMAGE 0x2
#define LVIF_PARAM 0x4
#define LVIF_STATE 0x8
#define LVIF_INDENT 0x10
#define LVIF_NORECOMPUTE 0x800
#define LVIF_GROUPID 0x100
#define LVIF_COLUMNS 0x200

#define LVIS_FOCUSED 0x1
#define LVIS_SELECTED 0x2
#define LVIS_CUT 0x4
#define LVIS_DROPHILITED 0x8
#define LVIS_GLOW 0x10
#define LVIS_ACTIVATING 0x20

#define LVIS_OVERLAYMASK 0xf00
#define LVIS_STATEIMAGEMASK 0xF000

#define INDEXTOSTATEIMAGEMASK(i) ((i) << 12)

#define I_INDENTCALLBACK (-1)
#define LV_ITEMA LVITEMA
#define LV_ITEMW LVITEMW

#define I_GROUPIDCALLBACK (-1)
#define I_GROUPIDNONE (-2)

#define LV_ITEM LVITEM

#define LVITEMA_V1_SIZE CCSIZEOF_STRUCT(LVITEMA,lParam)
#define LVITEMW_V1_SIZE CCSIZEOF_STRUCT(LVITEMW,lParam)

  typedef struct tagLVITEMA {
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;
    int iGroupId;
    UINT cColumns;
    PUINT puColumns;
  } LVITEMA,*LPLVITEMA;

  typedef struct tagLVITEMW
  {
    UINT mask;
    int iItem;
    int iSubItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
    int iIndent;
    int iGroupId;
    UINT cColumns;
    PUINT puColumns;
  } LVITEMW,*LPLVITEMW;

#define LVITEM __MINGW_NAME_AW(LVITEM)
#define LPLVITEM __MINGW_NAME_AW(LPLVITEM)

#define LVITEM_V1_SIZE __MINGW_NAME_AW_EXT(LVITEM,_V1_SIZE)

#define LPSTR_TEXTCALLBACKW ((LPWSTR)-1L)
#define LPSTR_TEXTCALLBACKA ((LPSTR)-1L)

#define LPSTR_TEXTCALLBACK __MINGW_NAME_AW(LPSTR_TEXTCALLBACK)

#define I_IMAGECALLBACK (-1)
#define I_IMAGENONE (-2)
#define I_COLUMNSCALLBACK ((UINT)-1)

#define LVM_GETITEMA (LVM_FIRST+5)
#define LVM_GETITEMW (LVM_FIRST+75)

#define LVM_GETITEM __MINGW_NAME_AW(LVM_GETITEM)

#define ListView_GetItem(hwnd,pitem) (BOOL)SNDMSG((hwnd),LVM_GETITEM,0,(LPARAM)(LV_ITEM *)(pitem))

#define LVM_SETITEMA (LVM_FIRST+6)
#define LVM_SETITEMW (LVM_FIRST+76)

#define LVM_SETITEM __MINGW_NAME_AW(LVM_SETITEM)

#define ListView_SetItem(hwnd,pitem) (BOOL)SNDMSG((hwnd),LVM_SETITEM,0,(LPARAM)(const LV_ITEM *)(pitem))

#define LVM_INSERTITEMA (LVM_FIRST+7)
#define LVM_INSERTITEMW (LVM_FIRST+77)

#define LVM_INSERTITEM __MINGW_NAME_AW(LVM_INSERTITEM)

#define ListView_InsertItem(hwnd,pitem) (int)SNDMSG((hwnd),LVM_INSERTITEM,0,(LPARAM)(const LV_ITEM *)(pitem))

#define LVM_DELETEITEM (LVM_FIRST+8)
#define ListView_DeleteItem(hwnd,i) (BOOL)SNDMSG((hwnd),LVM_DELETEITEM,(WPARAM)(int)(i),0L)

#define LVM_DELETEALLITEMS (LVM_FIRST+9)
#define ListView_DeleteAllItems(hwnd) (BOOL)SNDMSG((hwnd),LVM_DELETEALLITEMS,0,0L)

#define LVM_GETCALLBACKMASK (LVM_FIRST+10)
#define ListView_GetCallbackMask(hwnd) (BOOL)SNDMSG((hwnd),LVM_GETCALLBACKMASK,0,0)

#define LVM_SETCALLBACKMASK (LVM_FIRST+11)
#define ListView_SetCallbackMask(hwnd,mask) (BOOL)SNDMSG((hwnd),LVM_SETCALLBACKMASK,(WPARAM)(UINT)(mask),0)

#define LVNI_ALL 0x0
#define LVNI_FOCUSED 0x1
#define LVNI_SELECTED 0x2
#define LVNI_CUT 0x4
#define LVNI_DROPHILITED 0x8

#define LVNI_ABOVE 0x100
#define LVNI_BELOW 0x200
#define LVNI_TOLEFT 0x400
#define LVNI_TORIGHT 0x800

#define LVM_GETNEXTITEM (LVM_FIRST+12)
#define ListView_GetNextItem(hwnd,i,flags) (int)SNDMSG((hwnd),LVM_GETNEXTITEM,(WPARAM)(int)(i),MAKELPARAM((flags),0))

#define LVFI_PARAM              0x0001
#define LVFI_STRING             0x0002
#define LVFI_SUBSTRING          0x0004
#define LVFI_PARTIAL            0x0008
#define LVFI_WRAP               0x0020
#define LVFI_NEARESTXY          0x0040

#define LV_FINDINFOA LVFINDINFOA
#define LV_FINDINFOW LVFINDINFOW
#define LV_FINDINFO LVFINDINFO

  typedef struct tagLVFINDINFOA {
    UINT flags;
    LPCSTR psz;
    LPARAM lParam;
    POINT pt;
    UINT vkDirection;
  } LVFINDINFOA,*LPFINDINFOA;

  typedef struct tagLVFINDINFOW {
    UINT flags;
    LPCWSTR psz;
    LPARAM lParam;
    POINT pt;
    UINT vkDirection;
  } LVFINDINFOW,*LPFINDINFOW;

#define LVFINDINFO __MINGW_NAME_AW(LVFINDINFO)

#define LVM_FINDITEMA (LVM_FIRST+13)
#define LVM_FINDITEMW (LVM_FIRST+83)

#define LVM_FINDITEM __MINGW_NAME_AW(LVM_FINDITEM)

#define ListView_FindItem(hwnd,iStart,plvfi) (int)SNDMSG((hwnd),LVM_FINDITEM,(WPARAM)(int)(iStart),(LPARAM)(const LV_FINDINFO *)(plvfi))

#define LVIR_BOUNDS 0
#define LVIR_ICON 1
#define LVIR_LABEL 2
#define LVIR_SELECTBOUNDS 3

#define LVM_GETITEMRECT (LVM_FIRST+14)
#define ListView_GetItemRect(hwnd,i,prc,code) (BOOL)SNDMSG((hwnd),LVM_GETITEMRECT,(WPARAM)(int)(i),((prc) ? (((RECT *)(prc))->left = (code),(LPARAM)(RECT *)(prc)) : (LPARAM)(RECT *)NULL))

#define LVM_SETITEMPOSITION (LVM_FIRST+15)
#define ListView_SetItemPosition(hwndLV,i,x,y) (BOOL)SNDMSG((hwndLV),LVM_SETITEMPOSITION,(WPARAM)(int)(i),MAKELPARAM((x),(y)))

#define LVM_GETITEMPOSITION (LVM_FIRST+16)
#define ListView_GetItemPosition(hwndLV,i,ppt) (BOOL)SNDMSG((hwndLV),LVM_GETITEMPOSITION,(WPARAM)(int)(i),(LPARAM)(POINT *)(ppt))

#define LVM_GETSTRINGWIDTHA (LVM_FIRST+17)
#define LVM_GETSTRINGWIDTHW (LVM_FIRST+87)

#define LVM_GETSTRINGWIDTH __MINGW_NAME_AW(LVM_GETSTRINGWIDTH)

#define ListView_GetStringWidth(hwndLV,psz) (int)SNDMSG((hwndLV),LVM_GETSTRINGWIDTH,0,(LPARAM)(LPCTSTR)(psz))

#define LVHT_NOWHERE 0x1
#define LVHT_ONITEMICON 0x2
#define LVHT_ONITEMLABEL 0x4
#define LVHT_ONITEMSTATEICON 0x8
#define LVHT_ONITEM (LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON)

#define LVHT_ABOVE 0x8
#define LVHT_BELOW 0x10
#define LVHT_TORIGHT 0x20
#define LVHT_TOLEFT 0x40

#define LV_HITTESTINFO LVHITTESTINFO

#define LVHITTESTINFO_V1_SIZE CCSIZEOF_STRUCT(LVHITTESTINFO,iItem)

  typedef struct tagLVHITTESTINFO {
    POINT pt;
    UINT flags;
    int iItem;
    int iSubItem;
  } LVHITTESTINFO,*LPLVHITTESTINFO;

#define LVM_HITTEST (LVM_FIRST+18)
#define ListView_HitTest(hwndLV,pinfo) (int)SNDMSG((hwndLV),LVM_HITTEST,0,(LPARAM)(LV_HITTESTINFO *)(pinfo))

#define LVM_ENSUREVISIBLE (LVM_FIRST+19)
#define ListView_EnsureVisible(hwndLV,i,fPartialOK) (BOOL)SNDMSG((hwndLV),LVM_ENSUREVISIBLE,(WPARAM)(int)(i),MAKELPARAM((fPartialOK),0))

#define LVM_SCROLL (LVM_FIRST+20)
#define ListView_Scroll(hwndLV,dx,dy) (BOOL)SNDMSG((hwndLV),LVM_SCROLL,(WPARAM)(int)(dx),(LPARAM)(int)(dy))

#define LVM_REDRAWITEMS (LVM_FIRST+21)
#define ListView_RedrawItems(hwndLV,iFirst,iLast) (BOOL)SNDMSG((hwndLV),LVM_REDRAWITEMS,(WPARAM)(int)(iFirst),(LPARAM)(int)(iLast))

#define LVA_DEFAULT 0x0
#define LVA_ALIGNLEFT 0x1
#define LVA_ALIGNTOP 0x2
#define LVA_SNAPTOGRID 0x5

#define LVM_ARRANGE (LVM_FIRST+22)
#define ListView_Arrange(hwndLV,code) (BOOL)SNDMSG((hwndLV),LVM_ARRANGE,(WPARAM)(UINT)(code),0L)

#define LVM_EDITLABELA (LVM_FIRST+23)
#define LVM_EDITLABELW (LVM_FIRST+118)

#define LVM_EDITLABEL __MINGW_NAME_AW(LVM_EDITLABEL)

#define ListView_EditLabel(hwndLV,i) (HWND)SNDMSG((hwndLV),LVM_EDITLABEL,(WPARAM)(int)(i),0L)

#define LVM_GETEDITCONTROL (LVM_FIRST+24)
#define ListView_GetEditControl(hwndLV) (HWND)SNDMSG((hwndLV),LVM_GETEDITCONTROL,0,0L)

#define LV_COLUMNA LVCOLUMNA
#define LV_COLUMNW LVCOLUMNW
#define LV_COLUMN LVCOLUMN

#define LVCOLUMNA_V1_SIZE CCSIZEOF_STRUCT(LVCOLUMNA,iSubItem)
#define LVCOLUMNW_V1_SIZE CCSIZEOF_STRUCT(LVCOLUMNW,iSubItem)

  typedef struct tagLVCOLUMNA {
    UINT mask;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
    int iImage;
    int iOrder;
  } LVCOLUMNA,*LPLVCOLUMNA;

  typedef struct tagLVCOLUMNW {
    UINT mask;
    int fmt;
    int cx;
    LPWSTR pszText;
    int cchTextMax;
    int iSubItem;
# if (_WIN32_IE >= 0x0300)
    int iImage;
    int iOrder;
# endif
# if (_WIN32_WINNT >= 0x0600)
    int cxMin;
    int cxDefault;
    int cxIdeal;
# endif
  } LVCOLUMNW,*LPLVCOLUMNW;

#define LVCOLUMN __MINGW_NAME_AW(LVCOLUMN)
#define LPLVCOLUMN __MINGW_NAME_AW(LPLVCOLUMN)

#define LVCOLUMN_V1_SIZE __MINGW_NAME_AW_EXT(LVCOLUMN,_V1_SIZE)

#define LVCF_FMT 0x1
#define LVCF_WIDTH 0x2
#define LVCF_TEXT 0x4
#define LVCF_SUBITEM 0x8
#define LVCF_IMAGE 0x10
#define LVCF_ORDER 0x20
#if (_WIN32_WINNT >= 0x0600)
#define LVCF_MINWIDTH 0x40
#define LVCF_DEFAULTWIDTH 0x80
#define LVCF_IDEALWIDTH 0x100
#endif /* (_WIN32_WINNT >= 0x0600) */

#define LVCFMT_LEFT 0x0
#define LVCFMT_RIGHT 0x1
#define LVCFMT_CENTER 0x2
#define LVCFMT_JUSTIFYMASK 0x3
#define LVCFMT_IMAGE 0x800
#define LVCFMT_BITMAP_ON_RIGHT 0x1000
#define LVCFMT_COL_HAS_IMAGES 0x8000
#if (_WIN32_WINNT >= 0x0600)
#define LVCFMT_FIXED_WIDTH 0x100
#define LVCFMT_NO_DPI_SCALE 0x40000
#define LVCFMT_FIXED_RATIO 0x80000
#define LVCFMT_LINE_BREAK 0x100000
#define LVCFMT_FILL 0x200000
#define LVCFMT_WRAP 0x400000
#define LVCFMT_NO_TITLE 0x800000
#define LVCFMT_SPLITBUTTON 0x1000000
#define LVCFMT_TILE_PLACEMENTMASK (LVCFMT_LINE_BREAK|LVCFMT_FILL)
#endif /* (_WIN32_WINNT >= 0x0600) */

#define LVM_GETCOLUMNA (LVM_FIRST+25)
#define LVM_GETCOLUMNW (LVM_FIRST+95)

#define LVM_GETCOLUMN __MINGW_NAME_AW(LVM_GETCOLUMN)

#define ListView_GetColumn(hwnd,iCol,pcol) (BOOL)SNDMSG((hwnd),LVM_GETCOLUMN,(WPARAM)(int)(iCol),(LPARAM)(LV_COLUMN *)(pcol))

#define LVM_SETCOLUMNA (LVM_FIRST+26)
#define LVM_SETCOLUMNW (LVM_FIRST+96)

#define LVM_SETCOLUMN __MINGW_NAME_AW(LVM_SETCOLUMN)

#define ListView_SetColumn(hwnd,iCol,pcol) (BOOL)SNDMSG((hwnd),LVM_SETCOLUMN,(WPARAM)(int)(iCol),(LPARAM)(const LV_COLUMN *)(pcol))

#define LVM_INSERTCOLUMNA (LVM_FIRST+27)
#define LVM_INSERTCOLUMNW (LVM_FIRST+97)

#define LVM_INSERTCOLUMN __MINGW_NAME_AW(LVM_INSERTCOLUMN)

#define ListView_InsertColumn(hwnd,iCol,pcol) (int)SNDMSG((hwnd),LVM_INSERTCOLUMN,(WPARAM)(int)(iCol),(LPARAM)(const LV_COLUMN *)(pcol))

#define LVM_DELETECOLUMN (LVM_FIRST+28)
#define ListView_DeleteColumn(hwnd,iCol) (BOOL)SNDMSG((hwnd),LVM_DELETECOLUMN,(WPARAM)(int)(iCol),0)

#define LVM_GETCOLUMNWIDTH (LVM_FIRST+29)
#define ListView_GetColumnWidth(hwnd,iCol) (int)SNDMSG((hwnd),LVM_GETCOLUMNWIDTH,(WPARAM)(int)(iCol),0)

#define LVSCW_AUTOSIZE -1
#define LVSCW_AUTOSIZE_USEHEADER -2
#define LVM_SETCOLUMNWIDTH (LVM_FIRST+30)

#define ListView_SetColumnWidth(hwnd,iCol,cx) (BOOL)SNDMSG((hwnd),LVM_SETCOLUMNWIDTH,(WPARAM)(int)(iCol),MAKELPARAM((cx),0))

#define LVM_GETHEADER (LVM_FIRST+31)
#define ListView_GetHeader(hwnd) (HWND)SNDMSG((hwnd),LVM_GETHEADER,0,0L)
#define LVM_CREATEDRAGIMAGE (LVM_FIRST+33)
#define ListView_CreateDragImage(hwnd,i,lpptUpLeft) (HIMAGELIST)SNDMSG((hwnd),LVM_CREATEDRAGIMAGE,(WPARAM)(int)(i),(LPARAM)(LPPOINT)(lpptUpLeft))
#define LVM_GETVIEWRECT (LVM_FIRST+34)
#define ListView_GetViewRect(hwnd,prc) (BOOL)SNDMSG((hwnd),LVM_GETVIEWRECT,0,(LPARAM)(RECT *)(prc))
#define LVM_GETTEXTCOLOR (LVM_FIRST+35)
#define ListView_GetTextColor(hwnd) (COLORREF)SNDMSG((hwnd),LVM_GETTEXTCOLOR,0,0L)
#define LVM_SETTEXTCOLOR (LVM_FIRST+36)
#define ListView_SetTextColor(hwnd,clrText) (BOOL)SNDMSG((hwnd),LVM_SETTEXTCOLOR,0,(LPARAM)(COLORREF)(clrText))
#define LVM_GETTEXTBKCOLOR (LVM_FIRST+37)
#define ListView_GetTextBkColor(hwnd) (COLORREF)SNDMSG((hwnd),LVM_GETTEXTBKCOLOR,0,0L)
#define LVM_SETTEXTBKCOLOR (LVM_FIRST+38)
#define ListView_SetTextBkColor(hwnd,clrTextBk) (BOOL)SNDMSG((hwnd),LVM_SETTEXTBKCOLOR,0,(LPARAM)(COLORREF)(clrTextBk))
#define LVM_GETTOPINDEX (LVM_FIRST+39)
#define ListView_GetTopIndex(hwndLV) (int)SNDMSG((hwndLV),LVM_GETTOPINDEX,0,0)
#define LVM_GETCOUNTPERPAGE (LVM_FIRST+40)
#define ListView_GetCountPerPage(hwndLV) (int)SNDMSG((hwndLV),LVM_GETCOUNTPERPAGE,0,0)
#define LVM_GETORIGIN (LVM_FIRST+41)
#define ListView_GetOrigin(hwndLV,ppt) (BOOL)SNDMSG((hwndLV),LVM_GETORIGIN,(WPARAM)0,(LPARAM)(POINT *)(ppt))
#define LVM_UPDATE (LVM_FIRST+42)
#define ListView_Update(hwndLV,i) (BOOL)SNDMSG((hwndLV),LVM_UPDATE,(WPARAM)(i),0L)
#define LVM_SETITEMSTATE (LVM_FIRST+43)
#define ListView_SetItemState(hwndLV,i,data,mask) { LV_ITEM _ms_lvi; _ms_lvi.stateMask = mask; _ms_lvi.state = data; SNDMSG((hwndLV),LVM_SETITEMSTATE,(WPARAM)(i),(LPARAM)(LV_ITEM *)&_ms_lvi);}
#define ListView_SetCheckState(hwndLV,i,fCheck) ListView_SetItemState(hwndLV,i,INDEXTOSTATEIMAGEMASK((fCheck)?2:1),LVIS_STATEIMAGEMASK)
#define LVM_GETITEMSTATE (LVM_FIRST+44)
#define ListView_GetItemState(hwndLV,i,mask) (UINT)SNDMSG((hwndLV),LVM_GETITEMSTATE,(WPARAM)(i),(LPARAM)(mask))
#define ListView_GetCheckState(hwndLV,i) ((((UINT)(SNDMSG((hwndLV),LVM_GETITEMSTATE,(WPARAM)(i),LVIS_STATEIMAGEMASK))) >> 12) -1)

#define LVM_GETITEMTEXTA (LVM_FIRST+45)
#define LVM_GETITEMTEXTW (LVM_FIRST+115)

#define LVM_GETITEMTEXT __MINGW_NAME_AW(LVM_GETITEMTEXT)

#define ListView_GetItemText(hwndLV,i,iSubItem_,pszText_,cchTextMax_) { LV_ITEM _ms_lvi; _ms_lvi.iSubItem = iSubItem_; _ms_lvi.cchTextMax = cchTextMax_; _ms_lvi.pszText = pszText_; SNDMSG((hwndLV),LVM_GETITEMTEXT,(WPARAM)(i),(LPARAM)(LV_ITEM *)&_ms_lvi);}

#define LVM_SETITEMTEXTA (LVM_FIRST+46)
#define LVM_SETITEMTEXTW (LVM_FIRST+116)

#define LVM_SETITEMTEXT __MINGW_NAME_AW(LVM_SETITEMTEXT)

#define ListView_SetItemText(hwndLV,i,iSubItem_,pszText_) { LV_ITEM _ms_lvi; _ms_lvi.iSubItem = iSubItem_; _ms_lvi.pszText = pszText_; SNDMSG((hwndLV),LVM_SETITEMTEXT,(WPARAM)(i),(LPARAM)(LV_ITEM *)&_ms_lvi);}

#define LVSICF_NOINVALIDATEALL 0x1
#define LVSICF_NOSCROLL 0x2

#define LVM_SETITEMCOUNT (LVM_FIRST+47)
#define ListView_SetItemCount(hwndLV,cItems) SNDMSG((hwndLV),LVM_SETITEMCOUNT,(WPARAM)(cItems),0)
#define ListView_SetItemCountEx(hwndLV,cItems,dwFlags) SNDMSG((hwndLV),LVM_SETITEMCOUNT,(WPARAM)(cItems),(LPARAM)(dwFlags))

  typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM,LPARAM,LPARAM);

#define LVM_SORTITEMS (LVM_FIRST+48)
#define ListView_SortItems(hwndLV,_pfnCompare,_lPrm) (BOOL)SNDMSG((hwndLV),LVM_SORTITEMS,(WPARAM)(LPARAM)(_lPrm),(LPARAM)(PFNLVCOMPARE)(_pfnCompare))

#define LVM_SETITEMPOSITION32 (LVM_FIRST+49)
#define ListView_SetItemPosition32(hwndLV,i,x0,y0) { POINT ptNewPos; ptNewPos.x = x0; ptNewPos.y = y0; SNDMSG((hwndLV),LVM_SETITEMPOSITION32,(WPARAM)(int)(i),(LPARAM)&ptNewPos); }

#define LVM_GETSELECTEDCOUNT (LVM_FIRST+50)
#define ListView_GetSelectedCount(hwndLV) (UINT)SNDMSG((hwndLV),LVM_GETSELECTEDCOUNT,0,0L)

#define LVM_GETITEMSPACING (LVM_FIRST+51)
#define ListView_GetItemSpacing(hwndLV,fSmall) (DWORD)SNDMSG((hwndLV),LVM_GETITEMSPACING,fSmall,0L)

#define LVM_GETISEARCHSTRINGA (LVM_FIRST+52)
#define LVM_GETISEARCHSTRINGW (LVM_FIRST+117)

#define LVM_GETISEARCHSTRING __MINGW_NAME_AW(LVM_GETISEARCHSTRING)

#define ListView_GetISearchString(hwndLV,lpsz) (BOOL)SNDMSG((hwndLV),LVM_GETISEARCHSTRING,0,(LPARAM)(LPTSTR)(lpsz))

#define LVM_SETICONSPACING (LVM_FIRST+53)

#define ListView_SetIconSpacing(hwndLV,cx,cy) (DWORD)SNDMSG((hwndLV),LVM_SETICONSPACING,0,MAKELONG(cx,cy))
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+54)
#define ListView_SetExtendedListViewStyle(hwndLV,dw) (DWORD)SNDMSG((hwndLV),LVM_SETEXTENDEDLISTVIEWSTYLE,0,dw)
#define ListView_SetExtendedListViewStyleEx(hwndLV,dwMask,dw) (DWORD)SNDMSG((hwndLV),LVM_SETEXTENDEDLISTVIEWSTYLE,dwMask,dw)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+55)
#define ListView_GetExtendedListViewStyle(hwndLV) (DWORD)SNDMSG((hwndLV),LVM_GETEXTENDEDLISTVIEWSTYLE,0,0)
#define LVS_EX_GRIDLINES 0x1
#define LVS_EX_SUBITEMIMAGES 0x2
#define LVS_EX_CHECKBOXES 0x4
#define LVS_EX_TRACKSELECT 0x8
#define LVS_EX_HEADERDRAGDROP 0x10
#define LVS_EX_FULLROWSELECT 0x20
#define LVS_EX_ONECLICKACTIVATE 0x40
#define LVS_EX_TWOCLICKACTIVATE 0x80
#define LVS_EX_FLATSB 0x100
#define LVS_EX_REGIONAL 0x200
#define LVS_EX_INFOTIP 0x400
#define LVS_EX_UNDERLINEHOT 0x800
#define LVS_EX_UNDERLINECOLD 0x1000
#define LVS_EX_MULTIWORKAREAS 0x2000
#define LVS_EX_LABELTIP 0x4000
#define LVS_EX_BORDERSELECT 0x8000
#define LVS_EX_DOUBLEBUFFER 0x10000
#define LVS_EX_HIDELABELS 0x20000
#define LVS_EX_SINGLEROW 0x40000
#define LVS_EX_SNAPTOGRID 0x80000
#define LVS_EX_SIMPLESELECT 0x100000
#if _WIN32_WINNT >= 0x0600
#define LVS_EX_JUSTIFYCOLUMNS 0x200000
#define LVS_EX_TRANSPARENTBKGND 0x400000
#define LVS_EX_TRANSPARENTSHADOWTEXT 0x800000
#define LVS_EX_AUTOAUTOARRANGE 0x1000000
#define LVS_EX_HEADERINALLVIEWS 0x2000000
#define LVS_EX_AUTOCHECKSELECT 0x8000000
#define LVS_EX_AUTOSIZECOLUMNS 0x10000000
#define LVS_EX_COLUMNSNAPPOINTS 0x40000000
#define LVS_EX_COLUMNOVERFLOW 0x80000000
#endif

#define LVM_GETSUBITEMRECT (LVM_FIRST+56)
#define ListView_GetSubItemRect(hwnd,iItem,iSubItem,code,prc) (BOOL)SNDMSG((hwnd),LVM_GETSUBITEMRECT,(WPARAM)(int)(iItem),((prc) ? ((((LPRECT)(prc))->top = iSubItem),(((LPRECT)(prc))->left = code),(LPARAM)(prc)) : (LPARAM)(LPRECT)NULL))
#define LVM_SUBITEMHITTEST (LVM_FIRST+57)
#define ListView_SubItemHitTest(hwnd,plvhti) (int)SNDMSG((hwnd),LVM_SUBITEMHITTEST,0,(LPARAM)(LPLVHITTESTINFO)(plvhti))
#define LVM_SETCOLUMNORDERARRAY (LVM_FIRST+58)
#define ListView_SetColumnOrderArray(hwnd,iCount,pi) (BOOL)SNDMSG((hwnd),LVM_SETCOLUMNORDERARRAY,(WPARAM)(iCount),(LPARAM)(LPINT)(pi))
#define LVM_GETCOLUMNORDERARRAY (LVM_FIRST+59)
#define ListView_GetColumnOrderArray(hwnd,iCount,pi) (BOOL)SNDMSG((hwnd),LVM_GETCOLUMNORDERARRAY,(WPARAM)(iCount),(LPARAM)(LPINT)(pi))
#define LVM_SETHOTITEM (LVM_FIRST+60)
#define ListView_SetHotItem(hwnd,i) (int)SNDMSG((hwnd),LVM_SETHOTITEM,(WPARAM)(i),0)
#define LVM_GETHOTITEM (LVM_FIRST+61)
#define ListView_GetHotItem(hwnd) (int)SNDMSG((hwnd),LVM_GETHOTITEM,0,0)
#define LVM_SETHOTCURSOR (LVM_FIRST+62)
#define ListView_SetHotCursor(hwnd,hcur) (HCURSOR)SNDMSG((hwnd),LVM_SETHOTCURSOR,0,(LPARAM)(hcur))
#define LVM_GETHOTCURSOR (LVM_FIRST+63)
#define ListView_GetHotCursor(hwnd) (HCURSOR)SNDMSG((hwnd),LVM_GETHOTCURSOR,0,0)
#define LVM_APPROXIMATEVIEWRECT (LVM_FIRST+64)
#define ListView_ApproximateViewRect(hwnd,iWidth,iHeight,iCount) (DWORD)SNDMSG((hwnd),LVM_APPROXIMATEVIEWRECT,iCount,MAKELPARAM(iWidth,iHeight))

#define LV_MAX_WORKAREAS 16
#define LVM_SETWORKAREAS (LVM_FIRST+65)
#define ListView_SetWorkAreas(hwnd,nWorkAreas,prc) (BOOL)SNDMSG((hwnd),LVM_SETWORKAREAS,(WPARAM)(int)(nWorkAreas),(LPARAM)(RECT *)(prc))
#define LVM_GETWORKAREAS (LVM_FIRST+70)
#define ListView_GetWorkAreas(hwnd,nWorkAreas,prc) (BOOL)SNDMSG((hwnd),LVM_GETWORKAREAS,(WPARAM)(int)(nWorkAreas),(LPARAM)(RECT *)(prc))
#define LVM_GETNUMBEROFWORKAREAS (LVM_FIRST+73)
#define ListView_GetNumberOfWorkAreas(hwnd,pnWorkAreas) (BOOL)SNDMSG((hwnd),LVM_GETNUMBEROFWORKAREAS,0,(LPARAM)(UINT *)(pnWorkAreas))
#define LVM_GETSELECTIONMARK (LVM_FIRST+66)
#define ListView_GetSelectionMark(hwnd) (int)SNDMSG((hwnd),LVM_GETSELECTIONMARK,0,0)
#define LVM_SETSELECTIONMARK (LVM_FIRST+67)
#define ListView_SetSelectionMark(hwnd,i) (int)SNDMSG((hwnd),LVM_SETSELECTIONMARK,0,(LPARAM)(i))
#define LVM_SETHOVERTIME (LVM_FIRST+71)
#define ListView_SetHoverTime(hwndLV,dwHoverTimeMs) (DWORD)SNDMSG((hwndLV),LVM_SETHOVERTIME,0,(LPARAM)(dwHoverTimeMs))
#define LVM_GETHOVERTIME (LVM_FIRST+72)
#define ListView_GetHoverTime(hwndLV) (DWORD)SNDMSG((hwndLV),LVM_GETHOVERTIME,0,0)
#define LVM_SETTOOLTIPS (LVM_FIRST+74)
#define ListView_SetToolTips(hwndLV,hwndNewHwnd) (HWND)SNDMSG((hwndLV),LVM_SETTOOLTIPS,(WPARAM)(hwndNewHwnd),0)
#define LVM_GETTOOLTIPS (LVM_FIRST+78)
#define ListView_GetToolTips(hwndLV) (HWND)SNDMSG((hwndLV),LVM_GETTOOLTIPS,0,0)
#define LVM_SORTITEMSEX (LVM_FIRST+81)
#define ListView_SortItemsEx(hwndLV,_pfnCompare,_lPrm) (BOOL)SNDMSG((hwndLV),LVM_SORTITEMSEX,(WPARAM)(LPARAM)(_lPrm),(LPARAM)(PFNLVCOMPARE)(_pfnCompare))

  typedef struct tagLVBKIMAGEA {
    ULONG ulFlags;
    HBITMAP hbm;
    LPSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
  } LVBKIMAGEA,*LPLVBKIMAGEA;

  typedef struct tagLVBKIMAGEW {
    ULONG ulFlags;
    HBITMAP hbm;
    LPWSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
  } LVBKIMAGEW,*LPLVBKIMAGEW;

#define LVBKIF_SOURCE_NONE 0x0
#define LVBKIF_SOURCE_HBITMAP 0x1
#define LVBKIF_SOURCE_URL 0x2
#define LVBKIF_SOURCE_MASK 0x3
#define LVBKIF_STYLE_NORMAL 0x0
#define LVBKIF_STYLE_TILE 0x10
#define LVBKIF_STYLE_MASK 0x10
#define LVBKIF_FLAG_TILEOFFSET 0x100
#define LVBKIF_TYPE_WATERMARK 0x10000000

#define LVM_SETBKIMAGEA (LVM_FIRST+68)
#define LVM_SETBKIMAGEW (LVM_FIRST+138)
#define LVM_GETBKIMAGEA (LVM_FIRST+69)
#define LVM_GETBKIMAGEW (LVM_FIRST+139)

#define LVM_SETSELECTEDCOLUMN (LVM_FIRST+140)
#define ListView_SetSelectedColumn(hwnd,iCol) SNDMSG((hwnd),LVM_SETSELECTEDCOLUMN,(WPARAM)iCol,0)
#define LVM_SETTILEWIDTH (LVM_FIRST+141)
#define ListView_SetTileWidth(hwnd,cpWidth) SNDMSG((hwnd),LVM_SETTILEWIDTH,(WPARAM)cpWidth,0)
#define LV_VIEW_ICON 0x0
#define LV_VIEW_DETAILS 0x1
#define LV_VIEW_SMALLICON 0x2
#define LV_VIEW_LIST 0x3
#define LV_VIEW_TILE 0x4
#define LV_VIEW_MAX 0x4
#define LVM_SETVIEW (LVM_FIRST+142)
#define ListView_SetView(hwnd,iView) (DWORD)SNDMSG((hwnd),LVM_SETVIEW,(WPARAM)(DWORD)iView,0)
#define LVM_GETVIEW (LVM_FIRST+143)
#define ListView_GetView(hwnd) (DWORD)SNDMSG((hwnd),LVM_GETVIEW,0,0)
#define LVGF_NONE 0x0
#define LVGF_HEADER 0x1
#define LVGF_FOOTER 0x2
#define LVGF_STATE 0x4
#define LVGF_ALIGN 0x8
#define LVGF_GROUPID 0x10

#define LVGS_NORMAL 0x0
#define LVGS_COLLAPSED 0x1
#define LVGS_HIDDEN 0x2

#define LVGA_HEADER_LEFT 0x1
#define LVGA_HEADER_CENTER 0x2
#define LVGA_HEADER_RIGHT 0x4
#define LVGA_FOOTER_LEFT 0x8
#define LVGA_FOOTER_CENTER 0x10
#define LVGA_FOOTER_RIGHT 0x20

  typedef struct tagLVGROUP {
    UINT cbSize;
    UINT mask;
    LPWSTR pszHeader;
    int cchHeader;
    LPWSTR pszFooter;
    int cchFooter;
    int iGroupId;
    UINT stateMask;
    UINT state;
    UINT uAlign;
  } LVGROUP,*PLVGROUP;

#define LVM_INSERTGROUP (LVM_FIRST+145)
#define ListView_InsertGroup(hwnd,index,pgrp) SNDMSG((hwnd),LVM_INSERTGROUP,(WPARAM)index,(LPARAM)pgrp)
#define LVM_SETGROUPINFO (LVM_FIRST+147)
#define ListView_SetGroupInfo(hwnd,iGroupId,pgrp) SNDMSG((hwnd),LVM_SETGROUPINFO,(WPARAM)iGroupId,(LPARAM)pgrp)
#define LVM_GETGROUPINFO (LVM_FIRST+149)
#define ListView_GetGroupInfo(hwnd,iGroupId,pgrp) SNDMSG((hwnd),LVM_GETGROUPINFO,(WPARAM)iGroupId,(LPARAM)pgrp)
#define LVM_REMOVEGROUP (LVM_FIRST+150)
#define ListView_RemoveGroup(hwnd,iGroupId) SNDMSG((hwnd),LVM_REMOVEGROUP,(WPARAM)iGroupId,0)
#define LVM_MOVEGROUP (LVM_FIRST+151)
#define ListView_MoveGroup(hwnd,iGroupId,toIndex) SNDMSG((hwnd),LVM_MOVEGROUP,(WPARAM)iGroupId,(LPARAM)toIndex)
#define LVM_MOVEITEMTOGROUP (LVM_FIRST+154)
#define ListView_MoveItemToGroup(hwnd,idItemFrom,idGroupTo) SNDMSG((hwnd),LVM_MOVEITEMTOGROUP,(WPARAM)idItemFrom,(LPARAM)idGroupTo)
#define LVGMF_NONE 0x0
#define LVGMF_BORDERSIZE 0x1
#define LVGMF_BORDERCOLOR 0x2
#define LVGMF_TEXTCOLOR 0x4

  typedef struct tagLVGROUPMETRICS {
    UINT cbSize;
    UINT mask;
    UINT Left;
    UINT Top;
    UINT Right;
    UINT Bottom;
    COLORREF crLeft;
    COLORREF crTop;
    COLORREF crRight;
    COLORREF crBottom;
    COLORREF crHeader;
    COLORREF crFooter;
  } LVGROUPMETRICS,*PLVGROUPMETRICS;

#define LVM_SETGROUPMETRICS (LVM_FIRST+155)
#define ListView_SetGroupMetrics(hwnd,pGroupMetrics) SNDMSG((hwnd),LVM_SETGROUPMETRICS,0,(LPARAM)pGroupMetrics)
#define LVM_GETGROUPMETRICS (LVM_FIRST+156)
#define ListView_GetGroupMetrics(hwnd,pGroupMetrics) SNDMSG((hwnd),LVM_GETGROUPMETRICS,0,(LPARAM)pGroupMetrics)
#define LVM_ENABLEGROUPVIEW (LVM_FIRST+157)
#define ListView_EnableGroupView(hwnd,fEnable) SNDMSG((hwnd),LVM_ENABLEGROUPVIEW,(WPARAM)fEnable,0)

  typedef int (CALLBACK *PFNLVGROUPCOMPARE)(int,int,void *);

#define LVM_SORTGROUPS (LVM_FIRST+158)
#define ListView_SortGroups(hwnd,_pfnGroupCompate,_plv) SNDMSG((hwnd),LVM_SORTGROUPS,(WPARAM)_pfnGroupCompate,(LPARAM)_plv)

  typedef struct tagLVINSERTGROUPSORTED {
    PFNLVGROUPCOMPARE pfnGroupCompare;
    void *pvData;
    LVGROUP lvGroup;
  } LVINSERTGROUPSORTED,*PLVINSERTGROUPSORTED;

#define LVM_INSERTGROUPSORTED (LVM_FIRST+159)
#define ListView_InsertGroupSorted(hwnd,structInsert) SNDMSG((hwnd),LVM_INSERTGROUPSORTED,(WPARAM)structInsert,0)
#define LVM_REMOVEALLGROUPS (LVM_FIRST+160)
#define ListView_RemoveAllGroups(hwnd) SNDMSG((hwnd),LVM_REMOVEALLGROUPS,0,0)
#define LVM_HASGROUP (LVM_FIRST+161)
#define ListView_HasGroup(hwnd,dwGroupId) SNDMSG((hwnd),LVM_HASGROUP,dwGroupId,0)

#define LVTVIF_AUTOSIZE 0x0
#define LVTVIF_FIXEDWIDTH 0x1
#define LVTVIF_FIXEDHEIGHT 0x2
#define LVTVIF_FIXEDSIZE 0x3

#define LVTVIM_TILESIZE 0x1
#define LVTVIM_COLUMNS 0x2
#define LVTVIM_LABELMARGIN 0x4

  typedef struct tagLVTILEVIEWINFO {
    UINT cbSize;
    DWORD dwMask;
    DWORD dwFlags;
    SIZE sizeTile;
    int cLines;
    RECT rcLabelMargin;
  } LVTILEVIEWINFO,*PLVTILEVIEWINFO;

  typedef struct tagLVTILEINFO {
    UINT cbSize;
    int iItem;
    UINT cColumns;
    PUINT puColumns;
  } LVTILEINFO,*PLVTILEINFO;

#define LVM_SETTILEVIEWINFO (LVM_FIRST+162)
#define ListView_SetTileViewInfo(hwnd,ptvi) SNDMSG((hwnd),LVM_SETTILEVIEWINFO,0,(LPARAM)ptvi)
#define LVM_GETTILEVIEWINFO (LVM_FIRST+163)
#define ListView_GetTileViewInfo(hwnd,ptvi) SNDMSG((hwnd),LVM_GETTILEVIEWINFO,0,(LPARAM)ptvi)
#define LVM_SETTILEINFO (LVM_FIRST+164)
#define ListView_SetTileInfo(hwnd,pti) SNDMSG((hwnd),LVM_SETTILEINFO,0,(LPARAM)pti)
#define LVM_GETTILEINFO (LVM_FIRST+165)
#define ListView_GetTileInfo(hwnd,pti) SNDMSG((hwnd),LVM_GETTILEINFO,0,(LPARAM)pti)

  typedef struct {
    UINT cbSize;
    DWORD dwFlags;
    int iItem;
    DWORD dwReserved;
  } LVINSERTMARK,*LPLVINSERTMARK;

#define LVIM_AFTER 0x1

#define LVM_SETINSERTMARK (LVM_FIRST+166)
#define ListView_SetInsertMark(hwnd,lvim) (BOOL)SNDMSG((hwnd),LVM_SETINSERTMARK,(WPARAM) 0,(LPARAM) (lvim))
#define LVM_GETINSERTMARK (LVM_FIRST+167)
#define ListView_GetInsertMark(hwnd,lvim) (BOOL)SNDMSG((hwnd),LVM_GETINSERTMARK,(WPARAM) 0,(LPARAM) (lvim))
#define LVM_INSERTMARKHITTEST (LVM_FIRST+168)
#define ListView_InsertMarkHitTest(hwnd,point,lvim) (int)SNDMSG((hwnd),LVM_INSERTMARKHITTEST,(WPARAM)(LPPOINT)(point),(LPARAM)(LPLVINSERTMARK)(lvim))
#define LVM_GETINSERTMARKRECT (LVM_FIRST+169)
#define ListView_GetInsertMarkRect(hwnd,rc) (int)SNDMSG((hwnd),LVM_GETINSERTMARKRECT,(WPARAM)0,(LPARAM)(LPRECT)(rc))
#define LVM_SETINSERTMARKCOLOR (LVM_FIRST+170)
#define ListView_SetInsertMarkColor(hwnd,color) (COLORREF)SNDMSG((hwnd),LVM_SETINSERTMARKCOLOR,(WPARAM)0,(LPARAM)(COLORREF)(color))
#define LVM_GETINSERTMARKCOLOR (LVM_FIRST+171)
#define ListView_GetInsertMarkColor(hwnd) (COLORREF)SNDMSG((hwnd),LVM_GETINSERTMARKCOLOR,(WPARAM)0,(LPARAM)0)

  typedef struct tagLVSETINFOTIP {
    UINT cbSize;
    DWORD dwFlags;
    LPWSTR pszText;
    int iItem;
    int iSubItem;
  } LVSETINFOTIP,*PLVSETINFOTIP;

#define LVM_SETINFOTIP (LVM_FIRST+173)
#define ListView_SetInfoTip(hwndLV,plvInfoTip) (BOOL)SNDMSG((hwndLV),LVM_SETINFOTIP,(WPARAM)0,(LPARAM)plvInfoTip)
#define LVM_GETSELECTEDCOLUMN (LVM_FIRST+174)
#define ListView_GetSelectedColumn(hwnd) (UINT)SNDMSG((hwnd),LVM_GETSELECTEDCOLUMN,0,0)
#define LVM_ISGROUPVIEWENABLED (LVM_FIRST+175)
#define ListView_IsGroupViewEnabled(hwnd) (BOOL)SNDMSG((hwnd),LVM_ISGROUPVIEWENABLED,0,0)
#define LVM_GETOUTLINECOLOR (LVM_FIRST+176)
#define ListView_GetOutlineColor(hwnd) (COLORREF)SNDMSG((hwnd),LVM_GETOUTLINECOLOR,0,0)
#define LVM_SETOUTLINECOLOR (LVM_FIRST+177)
#define ListView_SetOutlineColor(hwnd,color) (COLORREF)SNDMSG((hwnd),LVM_SETOUTLINECOLOR,(WPARAM)0,(LPARAM)(COLORREF)(color))
#define LVM_CANCELEDITLABEL (LVM_FIRST+179)
#define ListView_CancelEditLabel(hwnd) (VOID)SNDMSG((hwnd),LVM_CANCELEDITLABEL,(WPARAM)0,(LPARAM)0)
#define LVM_MAPINDEXTOID (LVM_FIRST+180)
#define ListView_MapIndexToID(hwnd,index) (UINT)SNDMSG((hwnd),LVM_MAPINDEXTOID,(WPARAM)index,(LPARAM)0)
#define LVM_MAPIDTOINDEX (LVM_FIRST+181)
#define ListView_MapIDToIndex(hwnd,id) (UINT)SNDMSG((hwnd),LVM_MAPIDTOINDEX,(WPARAM)id,(LPARAM)0)
#define LVM_ISITEMVISIBLE (LVM_FIRST+182)
#define ListView_IsItemVisible(hwnd,index) (UINT)SNDMSG((hwnd),LVM_ISITEMVISIBLE,(WPARAM)(index),(LPARAM)0)

#define LVBKIMAGE __MINGW_NAME_AW(LVBKIMAGE)
#define LPLVBKIMAGE __MINGW_NAME_AW(LPLVBKIMAGE)
#define LVM_SETBKIMAGE __MINGW_NAME_AW(LVM_SETBKIMAGE)
#define LVM_GETBKIMAGE __MINGW_NAME_AW(LVM_GETBKIMAGE)

#define ListView_SetBkImage(hwnd,plvbki) (BOOL)SNDMSG((hwnd),LVM_SETBKIMAGE,0,(LPARAM)(plvbki))
#define ListView_GetBkImage(hwnd,plvbki) (BOOL)SNDMSG((hwnd),LVM_GETBKIMAGE,0,(LPARAM)(plvbki))

#define LPNM_LISTVIEW LPNMLISTVIEW
#define NM_LISTVIEW NMLISTVIEW

  typedef struct tagNMLISTVIEW {
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
  } NMLISTVIEW,*LPNMLISTVIEW;

  typedef struct tagNMITEMACTIVATE {
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
    UINT uKeyFlags;
  } NMITEMACTIVATE,*LPNMITEMACTIVATE;

#define LVKF_ALT 0x1
#define LVKF_CONTROL 0x2
#define LVKF_SHIFT 0x4

#define NMLVCUSTOMDRAW_V3_SIZE CCSIZEOF_STRUCT(NMLVCUSTOMDRW,clrTextBk)

  typedef struct tagNMLVCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
    int iSubItem;
    DWORD dwItemType;
    COLORREF clrFace;
    int iIconEffect;
    int iIconPhase;
    int iPartId;
    int iStateId;
    RECT rcText;
    UINT uAlign;
  } NMLVCUSTOMDRAW,*LPNMLVCUSTOMDRAW;

#define LVCDI_ITEM 0x0
#define LVCDI_GROUP 0x1

#define LVCDRF_NOSELECT 0x10000
#define LVCDRF_NOGROUPFRAME 0x20000

  typedef struct tagNMLVCACHEHINT {
    NMHDR hdr;
    int iFrom;
    int iTo;
  } NMLVCACHEHINT,*LPNMLVCACHEHINT;

#define LPNM_CACHEHINT LPNMLVCACHEHINT
#define PNM_CACHEHINT LPNMLVCACHEHINT
#define NM_CACHEHINT NMLVCACHEHINT

  typedef struct tagNMLVFINDITEMA {
    NMHDR hdr;
    int iStart;
    LVFINDINFOA lvfi;
  } NMLVFINDITEMA,*LPNMLVFINDITEMA;

  typedef struct tagNMLVFINDITEMW {
    NMHDR hdr;
    int iStart;
    LVFINDINFOW lvfi;
  } NMLVFINDITEMW,*LPNMLVFINDITEMW;

#define PNM_FINDITEMA LPNMLVFINDITEMA
#define LPNM_FINDITEMA LPNMLVFINDITEMA
#define NM_FINDITEMA NMLVFINDITEMA

#define PNM_FINDITEMW LPNMLVFINDITEMW
#define LPNM_FINDITEMW LPNMLVFINDITEMW
#define NM_FINDITEMW NMLVFINDITEMW

#define PNM_FINDITEM __MINGW_NAME_AW(PNM_FINDITEM)
#define LPNM_FINDITEM __MINGW_NAME_AW(LPNM_FINDITEM)
#define NM_FINDITEM __MINGW_NAME_AW(NM_FINDITEM)
#define NMLVFINDITEM __MINGW_NAME_AW(NMLVFINDITEM)
#define LPNMLVFINDITEM __MINGW_NAME_AW(LPNMLVFINDITEM)

  typedef struct tagNMLVODSTATECHANGE {
    NMHDR hdr;
    int iFrom;
    int iTo;
    UINT uNewState;
    UINT uOldState;
  } NMLVODSTATECHANGE,*LPNMLVODSTATECHANGE;

#define PNM_ODSTATECHANGE LPNMLVODSTATECHANGE
#define LPNM_ODSTATECHANGE LPNMLVODSTATECHANGE
#define NM_ODSTATECHANGE NMLVODSTATECHANGE

#define LVN_ITEMCHANGING (LVN_FIRST-0)
#define LVN_ITEMCHANGED (LVN_FIRST-1)
#define LVN_INSERTITEM (LVN_FIRST-2)
#define LVN_DELETEITEM (LVN_FIRST-3)
#define LVN_DELETEALLITEMS (LVN_FIRST-4)
#define LVN_BEGINLABELEDITA (LVN_FIRST-5)
#define LVN_BEGINLABELEDITW (LVN_FIRST-75)
#define LVN_ENDLABELEDITA (LVN_FIRST-6)
#define LVN_ENDLABELEDITW (LVN_FIRST-76)
#define LVN_COLUMNCLICK (LVN_FIRST-8)
#define LVN_BEGINDRAG (LVN_FIRST-9)
#define LVN_BEGINRDRAG (LVN_FIRST-11)

#define LVN_ODCACHEHINT (LVN_FIRST-13)
#define LVN_ODFINDITEMA (LVN_FIRST-52)
#define LVN_ODFINDITEMW (LVN_FIRST-79)

#define LVN_ITEMACTIVATE (LVN_FIRST-14)
#define LVN_ODSTATECHANGED (LVN_FIRST-15)

#define LVN_ODFINDITEM __MINGW_NAME_AW(LVN_ODFINDITEM)

#define LVN_HOTTRACK (LVN_FIRST-21)
#define LVN_GETDISPINFOA (LVN_FIRST-50)
#define LVN_GETDISPINFOW (LVN_FIRST-77)
#define LVN_SETDISPINFOA (LVN_FIRST-51)
#define LVN_SETDISPINFOW (LVN_FIRST-78)

#define LVN_BEGINLABELEDIT __MINGW_NAME_AW(LVN_BEGINLABELEDIT)
#define LVN_ENDLABELEDIT __MINGW_NAME_AW(LVN_ENDLABELEDIT)
#define LVN_GETDISPINFO __MINGW_NAME_AW(LVN_GETDISPINFO)
#define LVN_SETDISPINFO __MINGW_NAME_AW(LVN_SETDISPINFO)

#define LVIF_DI_SETITEM 0x1000

#define LV_DISPINFOA NMLVDISPINFOA
#define LV_DISPINFOW NMLVDISPINFOW
#define LV_DISPINFO NMLVDISPINFO

  typedef struct tagLVDISPINFO {
    NMHDR hdr;
    LVITEMA item;
  } NMLVDISPINFOA,*LPNMLVDISPINFOA;

  typedef struct tagLVDISPINFOW {
    NMHDR hdr;
    LVITEMW item;
  } NMLVDISPINFOW,*LPNMLVDISPINFOW;

#define NMLVDISPINFO __MINGW_NAME_AW(NMLVDISPINFO)

#define LVN_KEYDOWN (LVN_FIRST-55)

#define LV_KEYDOWN NMLVKEYDOWN

#include <pshpack1.h>

  typedef struct tagLVKEYDOWN {
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
  } NMLVKEYDOWN,*LPNMLVKEYDOWN;

#include <poppack.h>

#define LVN_MARQUEEBEGIN (LVN_FIRST-56)

  typedef struct tagNMLVGETINFOTIPA {
    NMHDR hdr;
    DWORD dwFlags;
    LPSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
  } NMLVGETINFOTIPA,*LPNMLVGETINFOTIPA;

  typedef struct tagNMLVGETINFOTIPW {
    NMHDR hdr;
    DWORD dwFlags;
    LPWSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
  } NMLVGETINFOTIPW,*LPNMLVGETINFOTIPW;

#define LVGIT_UNFOLDED 0x1

#define LVN_GETINFOTIPA (LVN_FIRST-57)
#define LVN_GETINFOTIPW (LVN_FIRST-58)

#define LVN_GETINFOTIP __MINGW_NAME_AW(LVN_GETINFOTIP)
#define NMLVGETINFOTIP __MINGW_NAME_AW(NMLVGETINFOTIP)
#define LPNMLVGETINFOTIP __MINGW_NAME_AW(LPNMLVGETINFOTIP)

  typedef struct tagNMLVSCROLL {
    NMHDR hdr;
    int dx;
    int dy;
  } NMLVSCROLL,*LPNMLVSCROLL;

#define LVN_BEGINSCROLL (LVN_FIRST-80)
#define LVN_ENDSCROLL (LVN_FIRST-81)

#endif /* !NOLISTVIEW */

#ifndef NOTREEVIEW

#define WC_TREEVIEWA "SysTreeView32"
#define WC_TREEVIEWW L"SysTreeView32"

#define WC_TREEVIEW __MINGW_NAME_AW(WC_TREEVIEW)

#define TVS_HASBUTTONS 0x1
#define TVS_HASLINES 0x2
#define TVS_LINESATROOT 0x4
#define TVS_EDITLABELS 0x8
#define TVS_DISABLEDRAGDROP 0x10
#define TVS_SHOWSELALWAYS 0x20
#define TVS_RTLREADING 0x40
#define TVS_NOTOOLTIPS 0x80
#define TVS_CHECKBOXES 0x100
#define TVS_TRACKSELECT 0x200
#define TVS_SINGLEEXPAND 0x400
#define TVS_INFOTIP 0x800
#define TVS_FULLROWSELECT 0x1000
#define TVS_NOSCROLL 0x2000
#define TVS_NONEVENHEIGHT 0x4000
#define TVS_NOHSCROLL 0x8000

  typedef struct _TREEITEM *HTREEITEM;

#define TVIF_TEXT 0x1
#define TVIF_IMAGE 0x2
#define TVIF_PARAM 0x4
#define TVIF_STATE 0x8
#define TVIF_HANDLE 0x10
#define TVIF_SELECTEDIMAGE 0x20
#define TVIF_CHILDREN 0x40
#if(_WIN32_IE >= 0x0400)
#define TVIF_INTEGRAL 0x80
#endif
#if(_WIN32_IE >= 0x0600)
#define TVIF_STATEEX 0x100
#endif
#define TVIF_EXPANDEDIMAGE 0x200
#define TVIS_SELECTED 0x2
#define TVIS_CUT 0x4
#define TVIS_DROPHILITED 0x8
#define TVIS_BOLD 0x10
#define TVIS_EXPANDED 0x20
#define TVIS_EXPANDEDONCE 0x40
#define TVIS_EXPANDPARTIAL 0x80
#define TVIS_OVERLAYMASK 0xf00
#define TVIS_STATEIMAGEMASK 0xF000
#define TVIS_USERMASK 0xF000

#define TVIS_FOCUSED 0x0001

#define I_CHILDRENCALLBACK (-1)

#define LPTV_ITEMW LPTVITEMW
#define LPTV_ITEMA LPTVITEMA
#define TV_ITEMW TVITEMW
#define TV_ITEMA TVITEMA
#define LPTV_ITEM LPTVITEM
#define TV_ITEM TVITEM

  typedef struct tagTVITEMA {
    UINT mask;
    HTREEITEM hItem;
    UINT state;
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
  } TVITEMA,*LPTVITEMA;

  typedef struct tagTVITEMW {
    UINT mask;
    HTREEITEM hItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
  } TVITEMW,*LPTVITEMW;

  typedef struct tagTVITEMEXA {
    UINT mask;
    HTREEITEM hItem;
    UINT state;
    UINT stateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
    int iIntegral;
    UINT uStateEx;        /* _WIN32_IE >= 0x600 */
    HWND hwnd;            /* _WIN32_IE >= 0x600 */
    int iExpandedImage;   /* _WIN32_IE >= 0x600 */
  } TVITEMEXA, *LPTVITEMEXA;

  typedef struct tagTVITEMEXW {
    UINT mask;
    HTREEITEM hItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int cChildren;
    LPARAM lParam;
    int iIntegral;
    UINT uStateEx;        /* _WIN32_IE >= 0x600 */
    HWND hwnd;            /* _WIN32_IE >= 0x600 */
    int iExpandedImage;   /* _WIN32_IE >= 0x600 */
  } TVITEMEXW, *LPTVITEMEXW;

  __MINGW_TYPEDEF_AW(TVITEMEX)
  __MINGW_TYPEDEF_AW(LPTVITEMEX)

#define TVITEM __MINGW_NAME_AW(TVITEM)
#define LPTVITEM __MINGW_NAME_AW(LPTVITEM)

#define TVI_ROOT ((HTREEITEM)(ULONG_PTR)-0x10000)
#define TVI_FIRST ((HTREEITEM)(ULONG_PTR)-0xffff)
#define TVI_LAST ((HTREEITEM)(ULONG_PTR)-0xfffe)
#define TVI_SORT ((HTREEITEM)(ULONG_PTR)-0xfffd)

#define LPTV_INSERTSTRUCTA LPTVINSERTSTRUCTA
#define LPTV_INSERTSTRUCTW LPTVINSERTSTRUCTW
#define TV_INSERTSTRUCTA TVINSERTSTRUCTA
#define TV_INSERTSTRUCTW TVINSERTSTRUCTW
#define TV_INSERTSTRUCT TVINSERTSTRUCT
#define LPTV_INSERTSTRUCT LPTVINSERTSTRUCT

#define TVINSERTSTRUCTA_V1_SIZE CCSIZEOF_STRUCT(TVINSERTSTRUCTA,item)
#define TVINSERTSTRUCTW_V1_SIZE CCSIZEOF_STRUCT(TVINSERTSTRUCTW,item)

  typedef struct tagTVINSERTSTRUCTA {
    HTREEITEM hParent;
    HTREEITEM hInsertAfter;
    __C89_NAMELESS union {
      TVITEMEXA itemex;
      TV_ITEMA item;
    } DUMMYUNIONNAME;
  } TVINSERTSTRUCTA,*LPTVINSERTSTRUCTA;

  typedef struct tagTVINSERTSTRUCTW {
    HTREEITEM hParent;
    HTREEITEM hInsertAfter;
    __C89_NAMELESS union {
      TVITEMEXW itemex;
      TV_ITEMW item;
    } DUMMYUNIONNAME;
  } TVINSERTSTRUCTW,*LPTVINSERTSTRUCTW;


#define TVINSERTSTRUCT __MINGW_NAME_AW(TVINSERTSTRUCT)
#define LPTVINSERTSTRUCT __MINGW_NAME_AW(LPTVINSERTSTRUCT)

#define TVINSERTSTRUCT_V1_SIZE __MINGW_NAME_AW_EXT(TVINSERTSTRUCT,_V1_SIZE)

#define TVM_INSERTITEMA (TV_FIRST+0)
#define TVM_INSERTITEMW (TV_FIRST+50)

#define TVM_INSERTITEM __MINGW_NAME_AW(TVM_INSERTITEM)

#define TreeView_InsertItem(hwnd,lpis) (HTREEITEM)SNDMSG((hwnd),TVM_INSERTITEM,0,(LPARAM)(LPTV_INSERTSTRUCT)(lpis))

#define TVM_DELETEITEM (TV_FIRST+1)
#define TreeView_DeleteItem(hwnd,hitem) (BOOL)SNDMSG((hwnd),TVM_DELETEITEM,0,(LPARAM)(HTREEITEM)(hitem))

#define TreeView_DeleteAllItems(hwnd) (BOOL)SNDMSG((hwnd),TVM_DELETEITEM,0,(LPARAM)TVI_ROOT)

#define TVM_EXPAND (TV_FIRST+2)
#define TreeView_Expand(hwnd,hitem,code) (BOOL)SNDMSG((hwnd),TVM_EXPAND,(WPARAM)(code),(LPARAM)(HTREEITEM)(hitem))

#define TVE_COLLAPSE 0x1
#define TVE_EXPAND 0x2
#define TVE_TOGGLE 0x3
#define TVE_EXPANDPARTIAL 0x4000
#define TVE_COLLAPSERESET 0x8000

#define TVM_GETITEMRECT (TV_FIRST+4)
#define TreeView_GetItemRect(hwnd,hitem,prc,code) (*(HTREEITEM *)prc = (hitem),(BOOL)SNDMSG((hwnd),TVM_GETITEMRECT,(WPARAM)(code),(LPARAM)(RECT *)(prc)))

#define TVM_GETCOUNT (TV_FIRST+5)
#define TreeView_GetCount(hwnd) (UINT)SNDMSG((hwnd),TVM_GETCOUNT,0,0)

#define TVM_GETINDENT (TV_FIRST+6)
#define TreeView_GetIndent(hwnd) (UINT)SNDMSG((hwnd),TVM_GETINDENT,0,0)

#define TVM_SETINDENT (TV_FIRST+7)
#define TreeView_SetIndent(hwnd,indent) (BOOL)SNDMSG((hwnd),TVM_SETINDENT,(WPARAM)(indent),0)

#define TVM_GETIMAGELIST (TV_FIRST+8)
#define TreeView_GetImageList(hwnd,iImage) (HIMAGELIST)SNDMSG((hwnd),TVM_GETIMAGELIST,iImage,0)

#define TVSIL_NORMAL 0
#define TVSIL_STATE 2

#define TVM_SETIMAGELIST (TV_FIRST+9)
#define TreeView_SetImageList(hwnd,himl,iImage) (HIMAGELIST)SNDMSG((hwnd),TVM_SETIMAGELIST,iImage,(LPARAM)(HIMAGELIST)(himl))

#define TVM_GETNEXTITEM (TV_FIRST+10)
#define TreeView_GetNextItem(hwnd,hitem,code) (HTREEITEM)SNDMSG((hwnd),TVM_GETNEXTITEM,(WPARAM)(code),(LPARAM)(HTREEITEM)(hitem))

#define TVGN_ROOT 0x0
#define TVGN_NEXT 0x1
#define TVGN_PREVIOUS 0x2
#define TVGN_PARENT 0x3
#define TVGN_CHILD 0x4
#define TVGN_FIRSTVISIBLE 0x5
#define TVGN_NEXTVISIBLE 0x6
#define TVGN_PREVIOUSVISIBLE 0x7
#define TVGN_DROPHILITE 0x8
#define TVGN_CARET 0x9
#define TVGN_LASTVISIBLE 0xa

#define TVSI_NOSINGLEEXPAND 0x8000

#define TreeView_GetChild(hwnd,hitem) TreeView_GetNextItem(hwnd,hitem,TVGN_CHILD)
#define TreeView_GetNextSibling(hwnd,hitem) TreeView_GetNextItem(hwnd,hitem,TVGN_NEXT)
#define TreeView_GetPrevSibling(hwnd,hitem) TreeView_GetNextItem(hwnd,hitem,TVGN_PREVIOUS)
#define TreeView_GetParent(hwnd,hitem) TreeView_GetNextItem(hwnd,hitem,TVGN_PARENT)
#define TreeView_GetFirstVisible(hwnd) TreeView_GetNextItem(hwnd,NULL,TVGN_FIRSTVISIBLE)
#define TreeView_GetNextVisible(hwnd,hitem) TreeView_GetNextItem(hwnd,hitem,TVGN_NEXTVISIBLE)
#define TreeView_GetPrevVisible(hwnd,hitem) TreeView_GetNextItem(hwnd,hitem,TVGN_PREVIOUSVISIBLE)
#define TreeView_GetSelection(hwnd) TreeView_GetNextItem(hwnd,NULL,TVGN_CARET)
#define TreeView_GetDropHilight(hwnd) TreeView_GetNextItem(hwnd,NULL,TVGN_DROPHILITE)
#define TreeView_GetRoot(hwnd) TreeView_GetNextItem(hwnd,NULL,TVGN_ROOT)
#define TreeView_GetLastVisible(hwnd) TreeView_GetNextItem(hwnd,NULL,TVGN_LASTVISIBLE)

#define TVM_SELECTITEM (TV_FIRST+11)
#define TreeView_Select(hwnd,hitem,code) (BOOL)SNDMSG((hwnd),TVM_SELECTITEM,(WPARAM)(code),(LPARAM)(HTREEITEM)(hitem))

#define TreeView_SelectItem(hwnd,hitem) TreeView_Select(hwnd,hitem,TVGN_CARET)
#define TreeView_SelectDropTarget(hwnd,hitem) TreeView_Select(hwnd,hitem,TVGN_DROPHILITE)
#define TreeView_SelectSetFirstVisible(hwnd,hitem) TreeView_Select(hwnd,hitem,TVGN_FIRSTVISIBLE)

#define TVM_GETITEMA (TV_FIRST+12)
#define TVM_GETITEMW (TV_FIRST+62)

#define TVM_GETITEM __MINGW_NAME_AW(TVM_GETITEM)

#define TreeView_GetItem(hwnd,pitem) (BOOL)SNDMSG((hwnd),TVM_GETITEM,0,(LPARAM)(TV_ITEM *)(pitem))

#define TVM_SETITEMA (TV_FIRST+13)
#define TVM_SETITEMW (TV_FIRST+63)

#define TVM_SETITEM __MINGW_NAME_AW(TVM_SETITEM)

#define TreeView_SetItem(hwnd,pitem) (BOOL)SNDMSG((hwnd),TVM_SETITEM,0,(LPARAM)(const TV_ITEM *)(pitem))

#define TVM_EDITLABELA (TV_FIRST+14)
#define TVM_EDITLABELW (TV_FIRST+65)

#define TVM_EDITLABEL __MINGW_NAME_AW(TVM_EDITLABEL)

#define TreeView_EditLabel(hwnd,hitem) (HWND)SNDMSG((hwnd),TVM_EDITLABEL,0,(LPARAM)(HTREEITEM)(hitem))

#define TVM_GETEDITCONTROL (TV_FIRST+15)
#define TreeView_GetEditControl(hwnd) (HWND)SNDMSG((hwnd),TVM_GETEDITCONTROL,0,0)

#define TVM_GETVISIBLECOUNT (TV_FIRST+16)
#define TreeView_GetVisibleCount(hwnd) (UINT)SNDMSG((hwnd),TVM_GETVISIBLECOUNT,0,0)

#define TVM_HITTEST (TV_FIRST+17)
#define TreeView_HitTest(hwnd,lpht) (HTREEITEM)SNDMSG((hwnd),TVM_HITTEST,0,(LPARAM)(LPTV_HITTESTINFO)(lpht))

#define LPTV_HITTESTINFO LPTVHITTESTINFO
#define TV_HITTESTINFO TVHITTESTINFO

  typedef struct tagTVHITTESTINFO {
    POINT pt;
    UINT flags;
    HTREEITEM hItem;
  } TVHITTESTINFO,*LPTVHITTESTINFO;

#define TVHT_NOWHERE 0x1
#define TVHT_ONITEMICON 0x2
#define TVHT_ONITEMLABEL 0x4
#define TVHT_ONITEM (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)
#define TVHT_ONITEMINDENT 0x8
#define TVHT_ONITEMBUTTON 0x10
#define TVHT_ONITEMRIGHT 0x20
#define TVHT_ONITEMSTATEICON 0x40

#define TVHT_ABOVE 0x100
#define TVHT_BELOW 0x200
#define TVHT_TORIGHT 0x400
#define TVHT_TOLEFT 0x800

#define TVM_CREATEDRAGIMAGE (TV_FIRST+18)
#define TreeView_CreateDragImage(hwnd,hitem) (HIMAGELIST)SNDMSG((hwnd),TVM_CREATEDRAGIMAGE,0,(LPARAM)(HTREEITEM)(hitem))

#define TVM_SORTCHILDREN (TV_FIRST+19)
#define TreeView_SortChildren(hwnd,hitem,recurse) (BOOL)SNDMSG((hwnd),TVM_SORTCHILDREN,(WPARAM)(recurse),(LPARAM)(HTREEITEM)(hitem))

#define TVM_ENSUREVISIBLE (TV_FIRST+20)
#define TreeView_EnsureVisible(hwnd,hitem) (BOOL)SNDMSG((hwnd),TVM_ENSUREVISIBLE,0,(LPARAM)(HTREEITEM)(hitem))

#define TVM_SORTCHILDRENCB (TV_FIRST+21)
#define TreeView_SortChildrenCB(hwnd,psort,recurse) (BOOL)SNDMSG((hwnd),TVM_SORTCHILDRENCB,(WPARAM)(recurse),(LPARAM)(LPTV_SORTCB)(psort))

#define TVM_ENDEDITLABELNOW (TV_FIRST+22)
#define TreeView_EndEditLabelNow(hwnd,fCancel) (BOOL)SNDMSG((hwnd),TVM_ENDEDITLABELNOW,(WPARAM)(fCancel),0)

#define TVM_GETISEARCHSTRINGA (TV_FIRST+23)
#define TVM_GETISEARCHSTRINGW (TV_FIRST+64)

#define TVM_GETISEARCHSTRING __MINGW_NAME_AW(TVM_GETISEARCHSTRING)

#define TVM_SETTOOLTIPS (TV_FIRST+24)
#define TreeView_SetToolTips(hwnd,hwndTT) (HWND)SNDMSG((hwnd),TVM_SETTOOLTIPS,(WPARAM)(hwndTT),0)
#define TVM_GETTOOLTIPS (TV_FIRST+25)
#define TreeView_GetToolTips(hwnd) (HWND)SNDMSG((hwnd),TVM_GETTOOLTIPS,0,0)
#define TreeView_GetISearchString(hwndTV,lpsz) (BOOL)SNDMSG((hwndTV),TVM_GETISEARCHSTRING,0,(LPARAM)(LPTSTR)(lpsz))

#define TVM_SETINSERTMARK (TV_FIRST+26)
#define TreeView_SetInsertMark(hwnd,hItem,fAfter) (BOOL)SNDMSG((hwnd),TVM_SETINSERTMARK,(WPARAM) (fAfter),(LPARAM) (hItem))
#define TVM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define TreeView_SetUnicodeFormat(hwnd,fUnicode) (BOOL)SNDMSG((hwnd),TVM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)
#define TVM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define TreeView_GetUnicodeFormat(hwnd) (BOOL)SNDMSG((hwnd),TVM_GETUNICODEFORMAT,0,0)

#define TVM_SETITEMHEIGHT (TV_FIRST+27)
#define TreeView_SetItemHeight(hwnd,iHeight) (int)SNDMSG((hwnd),TVM_SETITEMHEIGHT,(WPARAM)(iHeight),0)
#define TVM_GETITEMHEIGHT (TV_FIRST+28)
#define TreeView_GetItemHeight(hwnd) (int)SNDMSG((hwnd),TVM_GETITEMHEIGHT,0,0)
#define TVM_SETBKCOLOR (TV_FIRST+29)
#define TreeView_SetBkColor(hwnd,clr) (COLORREF)SNDMSG((hwnd),TVM_SETBKCOLOR,0,(LPARAM)(clr))
#define TVM_SETTEXTCOLOR (TV_FIRST+30)
#define TreeView_SetTextColor(hwnd,clr) (COLORREF)SNDMSG((hwnd),TVM_SETTEXTCOLOR,0,(LPARAM)(clr))
#define TVM_GETBKCOLOR (TV_FIRST+31)
#define TreeView_GetBkColor(hwnd) (COLORREF)SNDMSG((hwnd),TVM_GETBKCOLOR,0,0)
#define TVM_GETTEXTCOLOR (TV_FIRST+32)
#define TreeView_GetTextColor(hwnd) (COLORREF)SNDMSG((hwnd),TVM_GETTEXTCOLOR,0,0)
#define TVM_SETSCROLLTIME (TV_FIRST+33)
#define TreeView_SetScrollTime(hwnd,uTime) (UINT)SNDMSG((hwnd),TVM_SETSCROLLTIME,uTime,0)
#define TVM_GETSCROLLTIME (TV_FIRST+34)
#define TreeView_GetScrollTime(hwnd) (UINT)SNDMSG((hwnd),TVM_GETSCROLLTIME,0,0)
#define TVM_SETINSERTMARKCOLOR (TV_FIRST+37)
#define TreeView_SetInsertMarkColor(hwnd,clr) (COLORREF)SNDMSG((hwnd),TVM_SETINSERTMARKCOLOR,0,(LPARAM)(clr))
#define TVM_GETINSERTMARKCOLOR (TV_FIRST+38)
#define TreeView_GetInsertMarkColor(hwnd) (COLORREF)SNDMSG((hwnd),TVM_GETINSERTMARKCOLOR,0,0)

#define TreeView_SetItemState(hwndTV,hti,data,_mask) { TVITEM _ms_TVi; _ms_TVi.mask = TVIF_STATE; _ms_TVi.hItem = hti; _ms_TVi.stateMask = _mask; _ms_TVi.state = data; SNDMSG((hwndTV),TVM_SETITEM,0,(LPARAM)(TV_ITEM *)&_ms_TVi);}
#define TreeView_SetCheckState(hwndTV,hti,fCheck) TreeView_SetItemState(hwndTV,hti,INDEXTOSTATEIMAGEMASK((fCheck)?2:1),TVIS_STATEIMAGEMASK)
#define TVM_GETITEMSTATE (TV_FIRST+39)
#define TreeView_GetItemState(hwndTV,hti,mask) (UINT)SNDMSG((hwndTV),TVM_GETITEMSTATE,(WPARAM)(hti),(LPARAM)(mask))
#define TreeView_GetCheckState(hwndTV,hti) ((((UINT)(SNDMSG((hwndTV),TVM_GETITEMSTATE,(WPARAM)(hti),TVIS_STATEIMAGEMASK))) >> 12) -1)
#define TVM_SETLINECOLOR (TV_FIRST+40)
#define TreeView_SetLineColor(hwnd,clr) (COLORREF)SNDMSG((hwnd),TVM_SETLINECOLOR,0,(LPARAM)(clr))
#define TVM_GETLINECOLOR (TV_FIRST+41)
#define TreeView_GetLineColor(hwnd) (COLORREF)SNDMSG((hwnd),TVM_GETLINECOLOR,0,0)

#define TVM_MAPACCIDTOHTREEITEM (TV_FIRST+42)
#define TreeView_MapAccIDToHTREEITEM(hwnd,id) (HTREEITEM)SNDMSG((hwnd),TVM_MAPACCIDTOHTREEITEM,id,0)

#define TVM_MAPHTREEITEMTOACCID (TV_FIRST+43)
#define TreeView_MapHTREEITEMToAccID(hwnd,htreeitem) (UINT)SNDMSG((hwnd),TVM_MAPHTREEITEMTOACCID,(WPARAM)htreeitem,0)

  typedef int (CALLBACK *PFNTVCOMPARE)(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

#define LPTV_SORTCB LPTVSORTCB
#define TV_SORTCB TVSORTCB

  typedef struct tagTVSORTCB {
    HTREEITEM hParent;
    PFNTVCOMPARE lpfnCompare;
    LPARAM lParam;
  } TVSORTCB,*LPTVSORTCB;

#define LPNM_TREEVIEWA LPNMTREEVIEWA
#define LPNM_TREEVIEWW LPNMTREEVIEWW
#define NM_TREEVIEWW NMTREEVIEWW
#define NM_TREEVIEWA NMTREEVIEWA
#define LPNM_TREEVIEW LPNMTREEVIEW
#define NM_TREEVIEW NMTREEVIEW

  typedef struct tagNMTREEVIEWA {
    NMHDR hdr;
    UINT action;
    TVITEMA itemOld;
    TVITEMA itemNew;
    POINT ptDrag;
  } NMTREEVIEWA,*LPNMTREEVIEWA;

  typedef struct tagNMTREEVIEWW {
    NMHDR hdr;
    UINT action;
    TVITEMW itemOld;
    TVITEMW itemNew;
    POINT ptDrag;
  } NMTREEVIEWW,*LPNMTREEVIEWW;

#define NMTREEVIEW __MINGW_NAME_AW(NMTREEVIEW)
#define LPNMTREEVIEW __MINGW_NAME_AW(LPNMTREEVIEW)

#define TVN_SELCHANGINGA (TVN_FIRST-1)
#define TVN_SELCHANGINGW (TVN_FIRST-50)
#define TVN_SELCHANGEDA (TVN_FIRST-2)
#define TVN_SELCHANGEDW (TVN_FIRST-51)

#define TVC_UNKNOWN 0x0
#define TVC_BYMOUSE 0x1
#define TVC_BYKEYBOARD 0x2

#define TVN_GETDISPINFOA (TVN_FIRST-3)
#define TVN_GETDISPINFOW (TVN_FIRST-52)
#define TVN_SETDISPINFOA (TVN_FIRST-4)
#define TVN_SETDISPINFOW (TVN_FIRST-53)

#define TVIF_DI_SETITEM 0x1000

#define TV_DISPINFOA NMTVDISPINFOA
#define TV_DISPINFOW NMTVDISPINFOW
#define TV_DISPINFO NMTVDISPINFO

  typedef struct tagTVDISPINFOA {
    NMHDR hdr;
    TVITEMA item;
  } NMTVDISPINFOA,*LPNMTVDISPINFOA;

  typedef struct tagTVDISPINFOW {
    NMHDR hdr;
    TVITEMW item;
  } NMTVDISPINFOW,*LPNMTVDISPINFOW;

#define NMTVDISPINFO __MINGW_NAME_AW(NMTVDISPINFO)
#define LPNMTVDISPINFO __MINGW_NAME_AW(LPNMTVDISPINFO)

#if (_WIN32_IE >= 0x0600)

typedef struct tagTVDISPINFOEXA {
    NMHDR hdr;
    TVITEMEXA item;
} NMTVDISPINFOEXA, *LPNMTVDISPINFOEXA;

typedef struct tagTVDISPINFOEXW {
    NMHDR hdr;
    TVITEMEXW item;
} NMTVDISPINFOEXW, *LPNMTVDISPINFOEXW;

#define NMTVDISPINFOEX   __MINGW_NAME_AW(NMTVDISPINFOEX)
#define LPNMTVDISPINFOEX __MINGW_NAME_AW(LPNMTVDISPINFOEX)

#define TV_DISPINFOEXA NMTVDISPINFOEXA
#define TV_DISPINFOEXW NMTVDISPINFOEXW
#define TV_DISPINFOEX  NMTVDISPINFOEX

#endif /* (_WIN32_IE >= 0x0600) */

#define TVN_ITEMEXPANDINGA (TVN_FIRST-5)
#define TVN_ITEMEXPANDINGW (TVN_FIRST-54)
#define TVN_ITEMEXPANDEDA (TVN_FIRST-6)
#define TVN_ITEMEXPANDEDW (TVN_FIRST-55)
#define TVN_BEGINDRAGA (TVN_FIRST-7)
#define TVN_BEGINDRAGW (TVN_FIRST-56)
#define TVN_BEGINRDRAGA (TVN_FIRST-8)
#define TVN_BEGINRDRAGW (TVN_FIRST-57)
#define TVN_DELETEITEMA (TVN_FIRST-9)
#define TVN_DELETEITEMW (TVN_FIRST-58)
#define TVN_BEGINLABELEDITA (TVN_FIRST-10)
#define TVN_BEGINLABELEDITW (TVN_FIRST-59)
#define TVN_ENDLABELEDITA (TVN_FIRST-11)
#define TVN_ENDLABELEDITW (TVN_FIRST-60)
#define TVN_KEYDOWN (TVN_FIRST-12)
#define TVN_GETINFOTIPA (TVN_FIRST-13)
#define TVN_GETINFOTIPW (TVN_FIRST-14)
#define TVN_SINGLEEXPAND (TVN_FIRST-15)

#define TVNRET_DEFAULT 0
#define TVNRET_SKIPOLD 1
#define TVNRET_SKIPNEW 2

#define TV_KEYDOWN NMTVKEYDOWN

#include <pshpack1.h>

  typedef struct tagTVKEYDOWN {
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
  } NMTVKEYDOWN,*LPNMTVKEYDOWN;

#include <poppack.h>

#define TVN_SELCHANGING __MINGW_NAME_AW(TVN_SELCHANGING)
#define TVN_SELCHANGED __MINGW_NAME_AW(TVN_SELCHANGED)
#define TVN_GETDISPINFO __MINGW_NAME_AW(TVN_GETDISPINFO)
#define TVN_SETDISPINFO __MINGW_NAME_AW(TVN_SETDISPINFO)
#define TVN_ITEMEXPANDING __MINGW_NAME_AW(TVN_ITEMEXPANDING)
#define TVN_ITEMEXPANDED __MINGW_NAME_AW(TVN_ITEMEXPANDED)
#define TVN_BEGINDRAG __MINGW_NAME_AW(TVN_BEGINDRAG)
#define TVN_BEGINRDRAG __MINGW_NAME_AW(TVN_BEGINRDRAG)
#define TVN_DELETEITEM __MINGW_NAME_AW(TVN_DELETEITEM)
#define TVN_BEGINLABELEDIT __MINGW_NAME_AW(TVN_BEGINLABELEDIT)
#define TVN_ENDLABELEDIT __MINGW_NAME_AW(TVN_ENDLABELEDIT)

#define NMTVCUSTOMDRAW_V3_SIZE CCSIZEOF_STRUCT(NMTVCUSTOMDRAW,clrTextBk)

  typedef struct tagNMTVCUSTOMDRAW {
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
    int iLevel;
  } NMTVCUSTOMDRAW,*LPNMTVCUSTOMDRAW;

  typedef struct tagNMTVGETINFOTIPA {
    NMHDR hdr;
    LPSTR pszText;
    int cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
  } NMTVGETINFOTIPA,*LPNMTVGETINFOTIPA;

  typedef struct tagNMTVGETINFOTIPW {
    NMHDR hdr;
    LPWSTR pszText;
    int cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
  } NMTVGETINFOTIPW,*LPNMTVGETINFOTIPW;

#define TVN_GETINFOTIP __MINGW_NAME_AW(TVN_GETINFOTIP)
#define NMTVGETINFOTIP __MINGW_NAME_AW(NMTVGETINFOTIP)
#define LPNMTVGETINFOTIP __MINGW_NAME_AW(LPNMTVGETINFOTIP)

#define TVCDRF_NOIMAGES 0x10000

#endif /* !NOTREEVIEW */

#ifndef NOUSEREXCONTROLS

#define WC_COMBOBOXEXW L"ComboBoxEx32"
#define WC_COMBOBOXEXA "ComboBoxEx32"

#define WC_COMBOBOXEX __MINGW_NAME_AW(WC_COMBOBOXEX)

#define CBEIF_TEXT 0x1
#define CBEIF_IMAGE 0x2
#define CBEIF_SELECTEDIMAGE 0x4
#define CBEIF_OVERLAY 0x8
#define CBEIF_INDENT 0x10
#define CBEIF_LPARAM 0x20

#define CBEIF_DI_SETITEM 0x10000000

  typedef struct tagCOMBOBOXEXITEMA {
    UINT mask;
    INT_PTR iItem;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
  } COMBOBOXEXITEMA,*PCOMBOBOXEXITEMA;
  typedef COMBOBOXEXITEMA CONST *PCCOMBOEXITEMA;

  typedef struct tagCOMBOBOXEXITEMW
  {
    UINT mask;
    INT_PTR iItem;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
  } COMBOBOXEXITEMW,*PCOMBOBOXEXITEMW;
  typedef COMBOBOXEXITEMW CONST *PCCOMBOEXITEMW;

#define COMBOBOXEXITEM __MINGW_NAME_AW(COMBOBOXEXITEM)
#define PCOMBOBOXEXITEM __MINGW_NAME_AW(PCOMBOBOXEXITEM)
#define PCCOMBOBOXEXITEM __MINGW_NAME_AW(PCCOMBOBOXEXITEM)

#define CBEM_INSERTITEMA (WM_USER+1)
#define CBEM_SETIMAGELIST (WM_USER+2)
#define CBEM_GETIMAGELIST (WM_USER+3)
#define CBEM_GETITEMA (WM_USER+4)
#define CBEM_SETITEMA (WM_USER+5)
#define CBEM_DELETEITEM CB_DELETESTRING
#define CBEM_GETCOMBOCONTROL (WM_USER+6)
#define CBEM_GETEDITCONTROL (WM_USER+7)
#define CBEM_SETEXSTYLE (WM_USER+8)
#define CBEM_SETEXTENDEDSTYLE (WM_USER+14)
#define CBEM_GETEXSTYLE (WM_USER+9)
#define CBEM_GETEXTENDEDSTYLE (WM_USER+9)
#define CBEM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define CBEM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define CBEM_HASEDITCHANGED (WM_USER+10)
#define CBEM_INSERTITEMW (WM_USER+11)
#define CBEM_SETITEMW (WM_USER+12)
#define CBEM_GETITEMW (WM_USER+13)

#define CBEM_INSERTITEM __MINGW_NAME_AW(CBEM_INSERTITEM)
#define CBEM_SETITEM __MINGW_NAME_AW(CBEM_SETITEM)
#define CBEM_GETITEM __MINGW_NAME_AW(CBEM_GETITEM)

#define CBEM_SETWINDOWTHEME CCM_SETWINDOWTHEME

#define CBES_EX_NOEDITIMAGE 0x1
#define CBES_EX_NOEDITIMAGEINDENT 0x2
#define CBES_EX_PATHWORDBREAKPROC 0x4
#define CBES_EX_NOSIZELIMIT 0x8
#define CBES_EX_CASESENSITIVE 0x10

  typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEMA ceItem;
  } NMCOMBOBOXEXA,*PNMCOMBOBOXEXA;

  typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEMW ceItem;
  } NMCOMBOBOXEXW,*PNMCOMBOBOXEXW;

#define NMCOMBOBOXEX __MINGW_NAME_AW(NMCOMBOBOXEX)
#define PNMCOMBOBOXEX __MINGW_NAME_AW(PNMCOMBOBOXEX)
#define CBEN_GETDISPINFO __MINGW_NAME_AW(CBEN_GETDISPINFO)

#define CBEN_GETDISPINFOA (CBEN_FIRST - 0)
#define CBEN_INSERTITEM (CBEN_FIRST - 1)
#define CBEN_DELETEITEM (CBEN_FIRST - 2)
#define CBEN_BEGINEDIT (CBEN_FIRST - 4)
#define CBEN_ENDEDITA (CBEN_FIRST - 5)
#define CBEN_ENDEDITW (CBEN_FIRST - 6)

#define CBEN_GETDISPINFOW (CBEN_FIRST - 7)

#define CBEN_DRAGBEGINA (CBEN_FIRST - 8)
#define CBEN_DRAGBEGINW (CBEN_FIRST - 9)

#define CBEN_DRAGBEGIN __MINGW_NAME_AW(CBEN_DRAGBEGIN)

#define CBEN_ENDEDIT __MINGW_NAME_AW(CBEN_ENDEDIT)

#define CBENF_KILLFOCUS 1
#define CBENF_RETURN 2
#define CBENF_ESCAPE 3
#define CBENF_DROPDOWN 4

#define CBEMAXSTRLEN 260

  typedef struct {
    NMHDR hdr;
    int iItemid;
    WCHAR szText[CBEMAXSTRLEN];
  }NMCBEDRAGBEGINW,*LPNMCBEDRAGBEGINW,*PNMCBEDRAGBEGINW;

  typedef struct {
    NMHDR hdr;
    int iItemid;
    char szText[CBEMAXSTRLEN];
  }NMCBEDRAGBEGINA,*LPNMCBEDRAGBEGINA,*PNMCBEDRAGBEGINA;

#define NMCBEDRAGBEGIN __MINGW_NAME_AW(NMCBEDRAGBEGIN)
#define LPNMCBEDRAGBEGIN __MINGW_NAME_AW(LPNMCBEDRAGBEGIN)
#define PNMCBEDRAGBEGIN __MINGW_NAME_AW(PNMCBEDRAGBEGIN)

  typedef struct {
    NMHDR hdr;
    BOOL fChanged;
    int iNewSelection;
    WCHAR szText[CBEMAXSTRLEN];
    int iWhy;
  } NMCBEENDEDITW,*LPNMCBEENDEDITW,*PNMCBEENDEDITW;

  typedef struct {
    NMHDR hdr;
    BOOL fChanged;
    int iNewSelection;
    char szText[CBEMAXSTRLEN];
    int iWhy;
  } NMCBEENDEDITA,*LPNMCBEENDEDITA,*PNMCBEENDEDITA;

#define NMCBEENDEDIT __MINGW_NAME_AW(NMCBEENDEDIT)
#define LPNMCBEENDEDIT __MINGW_NAME_AW(LPNMCBEENDEDIT)
#define PNMCBEENDEDIT __MINGW_NAME_AW(PNMCBEENDEDIT)

#endif /* !NOUSEREXCONTROLS */

#ifndef NOTABCONTROL

#define WC_TABCONTROLA "SysTabControl32"
#define WC_TABCONTROLW L"SysTabControl32"
#define WC_TABCONTROL __MINGW_NAME_AW(WC_TABCONTROL)

#define TCS_SCROLLOPPOSITE 0x1
#define TCS_BOTTOM 0x2
#define TCS_RIGHT 0x2
#define TCS_MULTISELECT 0x4
#define TCS_FLATBUTTONS 0x8
#define TCS_FORCEICONLEFT 0x10
#define TCS_FORCELABELLEFT 0x20
#define TCS_HOTTRACK 0x40
#define TCS_VERTICAL 0x80
#define TCS_TABS 0x0
#define TCS_BUTTONS 0x100
#define TCS_SINGLELINE 0x0
#define TCS_MULTILINE 0x200
#define TCS_RIGHTJUSTIFY 0x0
#define TCS_FIXEDWIDTH 0x400
#define TCS_RAGGEDRIGHT 0x800
#define TCS_FOCUSONBUTTONDOWN 0x1000
#define TCS_OWNERDRAWFIXED 0x2000
#define TCS_TOOLTIPS 0x4000
#define TCS_FOCUSNEVER 0x8000

#define TCS_EX_FLATSEPARATORS 0x1
#define TCS_EX_REGISTERDROP 0x2

#define TCM_GETIMAGELIST (TCM_FIRST+2)
#define TabCtrl_GetImageList(hwnd) (HIMAGELIST)SNDMSG((hwnd),TCM_GETIMAGELIST,0,0L)

#define TCM_SETIMAGELIST (TCM_FIRST+3)
#define TabCtrl_SetImageList(hwnd,himl) (HIMAGELIST)SNDMSG((hwnd),TCM_SETIMAGELIST,0,(LPARAM)(HIMAGELIST)(himl))

#define TCM_GETITEMCOUNT (TCM_FIRST+4)
#define TabCtrl_GetItemCount(hwnd) (int)SNDMSG((hwnd),TCM_GETITEMCOUNT,0,0L)

#define TCIF_TEXT 0x1
#define TCIF_IMAGE 0x2
#define TCIF_RTLREADING 0x4
#define TCIF_PARAM 0x8
#define TCIF_STATE 0x10

#define TCIS_BUTTONPRESSED 0x1
#define TCIS_HIGHLIGHTED 0x2

#define TC_ITEMHEADERA TCITEMHEADERA
#define TC_ITEMHEADERW TCITEMHEADERW
#define TC_ITEMHEADER TCITEMHEADER

  typedef struct tagTCITEMHEADERA {
    UINT mask;
    UINT lpReserved1;
    UINT lpReserved2;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
  } TCITEMHEADERA,*LPTCITEMHEADERA;

  typedef struct tagTCITEMHEADERW {
    UINT mask;
    UINT lpReserved1;
    UINT lpReserved2;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
  } TCITEMHEADERW,*LPTCITEMHEADERW;

#define TCITEMHEADER __MINGW_NAME_AW(TCITEMHEADER)
#define LPTCITEMHEADER __MINGW_NAME_AW(LPTCITEMHEADER)

#define TC_ITEMA TCITEMA
#define TC_ITEMW TCITEMW
#define TC_ITEM TCITEM

  typedef struct tagTCITEMA {
    UINT mask;
    DWORD dwState;
    DWORD dwStateMask;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
  } TCITEMA,*LPTCITEMA;

  typedef struct tagTCITEMW {
    UINT mask;
    DWORD dwState;
    DWORD dwStateMask;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    LPARAM lParam;
  } TCITEMW,*LPTCITEMW;

#define TCITEM __MINGW_NAME_AW(TCITEM)
#define LPTCITEM __MINGW_NAME_AW(LPTCITEM)

#define TCM_GETITEMA (TCM_FIRST+5)
#define TCM_GETITEMW (TCM_FIRST+60)

#define TCM_GETITEM __MINGW_NAME_AW(TCM_GETITEM)

#define TabCtrl_GetItem(hwnd,iItem,pitem) (BOOL)SNDMSG((hwnd),TCM_GETITEM,(WPARAM)(int)(iItem),(LPARAM)(TC_ITEM *)(pitem))

#define TCM_SETITEMA (TCM_FIRST+6)
#define TCM_SETITEMW (TCM_FIRST+61)

#define TCM_SETITEM __MINGW_NAME_AW(TCM_SETITEM)

#define TabCtrl_SetItem(hwnd,iItem,pitem) (BOOL)SNDMSG((hwnd),TCM_SETITEM,(WPARAM)(int)(iItem),(LPARAM)(TC_ITEM *)(pitem))

#define TCM_INSERTITEMA (TCM_FIRST+7)
#define TCM_INSERTITEMW (TCM_FIRST+62)

#define TCM_INSERTITEM __MINGW_NAME_AW(TCM_INSERTITEM)

#define TabCtrl_InsertItem(hwnd,iItem,pitem) (int)SNDMSG((hwnd),TCM_INSERTITEM,(WPARAM)(int)(iItem),(LPARAM)(const TC_ITEM *)(pitem))

#define TCM_DELETEITEM (TCM_FIRST+8)
#define TabCtrl_DeleteItem(hwnd,i) (BOOL)SNDMSG((hwnd),TCM_DELETEITEM,(WPARAM)(int)(i),0L)

#define TCM_DELETEALLITEMS (TCM_FIRST+9)
#define TabCtrl_DeleteAllItems(hwnd) (BOOL)SNDMSG((hwnd),TCM_DELETEALLITEMS,0,0L)

#define TCM_GETITEMRECT (TCM_FIRST+10)
#define TabCtrl_GetItemRect(hwnd,i,prc) (BOOL)SNDMSG((hwnd),TCM_GETITEMRECT,(WPARAM)(int)(i),(LPARAM)(RECT *)(prc))

#define TCM_GETCURSEL (TCM_FIRST+11)
#define TabCtrl_GetCurSel(hwnd) (int)SNDMSG((hwnd),TCM_GETCURSEL,0,0)

#define TCM_SETCURSEL (TCM_FIRST+12)
#define TabCtrl_SetCurSel(hwnd,i) (int)SNDMSG((hwnd),TCM_SETCURSEL,(WPARAM)(i),0)

#define TCHT_NOWHERE 0x1
#define TCHT_ONITEMICON 0x2
#define TCHT_ONITEMLABEL 0x4
#define TCHT_ONITEM (TCHT_ONITEMICON | TCHT_ONITEMLABEL)

#define LPTC_HITTESTINFO LPTCHITTESTINFO
#define TC_HITTESTINFO TCHITTESTINFO

  typedef struct tagTCHITTESTINFO {
    POINT pt;
    UINT flags;
  } TCHITTESTINFO,*LPTCHITTESTINFO;

#define TCM_HITTEST (TCM_FIRST+13)
#define TabCtrl_HitTest(hwndTC,pinfo) (int)SNDMSG((hwndTC),TCM_HITTEST,0,(LPARAM)(TC_HITTESTINFO *)(pinfo))
#define TCM_SETITEMEXTRA (TCM_FIRST+14)
#define TabCtrl_SetItemExtra(hwndTC,cb) (BOOL)SNDMSG((hwndTC),TCM_SETITEMEXTRA,(WPARAM)(cb),0L)
#define TCM_ADJUSTRECT (TCM_FIRST+40)
#define TabCtrl_AdjustRect(hwnd,bLarger,prc) (int)SNDMSG(hwnd,TCM_ADJUSTRECT,(WPARAM)(BOOL)(bLarger),(LPARAM)(RECT *)prc)
#define TCM_SETITEMSIZE (TCM_FIRST+41)
#define TabCtrl_SetItemSize(hwnd,x,y) (DWORD)SNDMSG((hwnd),TCM_SETITEMSIZE,0,MAKELPARAM(x,y))
#define TCM_REMOVEIMAGE (TCM_FIRST+42)
#define TabCtrl_RemoveImage(hwnd,i) (void)SNDMSG((hwnd),TCM_REMOVEIMAGE,i,0L)
#define TCM_SETPADDING (TCM_FIRST+43)
#define TabCtrl_SetPadding(hwnd,cx,cy) (void)SNDMSG((hwnd),TCM_SETPADDING,0,MAKELPARAM(cx,cy))
#define TCM_GETROWCOUNT (TCM_FIRST+44)
#define TabCtrl_GetRowCount(hwnd) (int)SNDMSG((hwnd),TCM_GETROWCOUNT,0,0L)
#define TCM_GETTOOLTIPS (TCM_FIRST+45)
#define TabCtrl_GetToolTips(hwnd) (HWND)SNDMSG((hwnd),TCM_GETTOOLTIPS,0,0L)
#define TCM_SETTOOLTIPS (TCM_FIRST+46)
#define TabCtrl_SetToolTips(hwnd,hwndTT) (void)SNDMSG((hwnd),TCM_SETTOOLTIPS,(WPARAM)(hwndTT),0L)
#define TCM_GETCURFOCUS (TCM_FIRST+47)
#define TabCtrl_GetCurFocus(hwnd) (int)SNDMSG((hwnd),TCM_GETCURFOCUS,0,0)
#define TCM_SETCURFOCUS (TCM_FIRST+48)
#define TabCtrl_SetCurFocus(hwnd,i) SNDMSG((hwnd),TCM_SETCURFOCUS,i,0)
#define TCM_SETMINTABWIDTH (TCM_FIRST+49)
#define TabCtrl_SetMinTabWidth(hwnd,x) (int)SNDMSG((hwnd),TCM_SETMINTABWIDTH,0,x)
#define TCM_DESELECTALL (TCM_FIRST+50)
#define TabCtrl_DeselectAll(hwnd,fExcludeFocus) (void)SNDMSG((hwnd),TCM_DESELECTALL,fExcludeFocus,0)
#define TCM_HIGHLIGHTITEM (TCM_FIRST+51)
#define TabCtrl_HighlightItem(hwnd,i,fHighlight) (BOOL)SNDMSG((hwnd),TCM_HIGHLIGHTITEM,(WPARAM)(i),(LPARAM)MAKELONG (fHighlight,0))
#define TCM_SETEXTENDEDSTYLE (TCM_FIRST+52)
#define TabCtrl_SetExtendedStyle(hwnd,dw) (DWORD)SNDMSG((hwnd),TCM_SETEXTENDEDSTYLE,0,dw)
#define TCM_GETEXTENDEDSTYLE (TCM_FIRST+53)
#define TabCtrl_GetExtendedStyle(hwnd) (DWORD)SNDMSG((hwnd),TCM_GETEXTENDEDSTYLE,0,0)
#define TCM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define TabCtrl_SetUnicodeFormat(hwnd,fUnicode) (BOOL)SNDMSG((hwnd),TCM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)
#define TCM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define TabCtrl_GetUnicodeFormat(hwnd) (BOOL)SNDMSG((hwnd),TCM_GETUNICODEFORMAT,0,0)

#define TCN_KEYDOWN (TCN_FIRST - 0)

#define TC_KEYDOWN NMTCKEYDOWN

#include <pshpack1.h>

  typedef struct tagTCKEYDOWN {
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
  } NMTCKEYDOWN;

#include <poppack.h>

#define TCN_SELCHANGE (TCN_FIRST - 1)
#define TCN_SELCHANGING (TCN_FIRST - 2)
#define TCN_GETOBJECT (TCN_FIRST - 3)
#define TCN_FOCUSCHANGE (TCN_FIRST - 4)

#endif /* !NOTABCONTROL */

#ifndef NOANIMATE

#define ANIMATE_CLASSW L"SysAnimate32"
#define ANIMATE_CLASSA "SysAnimate32"

#define ANIMATE_CLASS __MINGW_NAME_AW(ANIMATE_CLASS)

#define ACS_CENTER 0x1
#define ACS_TRANSPARENT 0x2
#define ACS_AUTOPLAY 0x4
#define ACS_TIMER 0x8

#define ACM_OPENA (WM_USER+100)
#define ACM_OPENW (WM_USER+103)

#define ACM_OPEN __MINGW_NAME_AW(ACM_OPEN)

#define ACM_PLAY (WM_USER+101)
#define ACM_STOP (WM_USER+102)

#define ACN_START 1
#define ACN_STOP 2

#define Animate_Create(hwndP,id,dwStyle,hInstance) CreateWindow(ANIMATE_CLASS,NULL,dwStyle,0,0,0,0,hwndP,(HMENU)(id),hInstance,NULL)

#define Animate_Open(hwnd,szName) (BOOL)SNDMSG(hwnd,ACM_OPEN,0,(LPARAM)(LPTSTR)(szName))
#define Animate_OpenEx(hwnd,hInst,szName) (BOOL)SNDMSG(hwnd,ACM_OPEN,(WPARAM)(hInst),(LPARAM)(LPTSTR)(szName))
#define Animate_Play(hwnd,from,to,rep) (BOOL)SNDMSG(hwnd,ACM_PLAY,(WPARAM)(rep),(LPARAM)MAKELONG(from,to))
#define Animate_Stop(hwnd) (BOOL)SNDMSG(hwnd,ACM_STOP,0,0)
#define Animate_Close(hwnd) Animate_Open(hwnd,NULL)
#define Animate_Seek(hwnd,frame) Animate_Play(hwnd,frame,frame,1)

#endif /* !NOANIMATE */

#ifndef NOMONTHCAL

#define MONTHCAL_CLASSW L"SysMonthCal32"
#define MONTHCAL_CLASSA "SysMonthCal32"

#define MONTHCAL_CLASS __MINGW_NAME_AW(MONTHCAL_CLASS)

  typedef DWORD MONTHDAYSTATE,*LPMONTHDAYSTATE;

#define MCM_FIRST 0x1000

#define MCM_GETCURSEL (MCM_FIRST+1)
#define MonthCal_GetCurSel(hmc,pst) (BOOL)SNDMSG(hmc,MCM_GETCURSEL,0,(LPARAM)(pst))
#define MCM_SETCURSEL (MCM_FIRST+2)
#define MonthCal_SetCurSel(hmc,pst) (BOOL)SNDMSG(hmc,MCM_SETCURSEL,0,(LPARAM)(pst))
#define MCM_GETMAXSELCOUNT (MCM_FIRST+3)
#define MonthCal_GetMaxSelCount(hmc) (DWORD)SNDMSG(hmc,MCM_GETMAXSELCOUNT,0,0L)
#define MCM_SETMAXSELCOUNT (MCM_FIRST+4)
#define MonthCal_SetMaxSelCount(hmc,n) (BOOL)SNDMSG(hmc,MCM_SETMAXSELCOUNT,(WPARAM)(n),0L)
#define MCM_GETSELRANGE (MCM_FIRST+5)
#define MonthCal_GetSelRange(hmc,rgst) SNDMSG(hmc,MCM_GETSELRANGE,0,(LPARAM)(rgst))
#define MCM_SETSELRANGE (MCM_FIRST+6)
#define MonthCal_SetSelRange(hmc,rgst) SNDMSG(hmc,MCM_SETSELRANGE,0,(LPARAM)(rgst))
#define MCM_GETMONTHRANGE (MCM_FIRST+7)
#define MonthCal_GetMonthRange(hmc,gmr,rgst) (DWORD)SNDMSG(hmc,MCM_GETMONTHRANGE,(WPARAM)(gmr),(LPARAM)(rgst))
#define MCM_SETDAYSTATE (MCM_FIRST+8)
#define MonthCal_SetDayState(hmc,cbds,rgds) SNDMSG(hmc,MCM_SETDAYSTATE,(WPARAM)(cbds),(LPARAM)(rgds))
#define MCM_GETMINREQRECT (MCM_FIRST+9)
#define MonthCal_GetMinReqRect(hmc,prc) SNDMSG(hmc,MCM_GETMINREQRECT,0,(LPARAM)(prc))
#define MCM_SETCOLOR (MCM_FIRST+10)
#define MonthCal_SetColor(hmc,iColor,clr) SNDMSG(hmc,MCM_SETCOLOR,iColor,clr)
#define MCM_GETCOLOR (MCM_FIRST+11)
#define MonthCal_GetColor(hmc,iColor) SNDMSG(hmc,MCM_GETCOLOR,iColor,0)

#define MCSC_BACKGROUND 0
#define MCSC_TEXT 1
#define MCSC_TITLEBK 2
#define MCSC_TITLETEXT 3
#define MCSC_MONTHBK 4
#define MCSC_TRAILINGTEXT 5

#define MCM_SETTODAY (MCM_FIRST+12)
#define MonthCal_SetToday(hmc,pst) SNDMSG(hmc,MCM_SETTODAY,0,(LPARAM)(pst))
#define MCM_GETTODAY (MCM_FIRST+13)
#define MonthCal_GetToday(hmc,pst) (BOOL)SNDMSG(hmc,MCM_GETTODAY,0,(LPARAM)(pst))
#define MCM_HITTEST (MCM_FIRST+14)
#define MonthCal_HitTest(hmc,pinfo) SNDMSG(hmc,MCM_HITTEST,0,(LPARAM)(PMCHITTESTINFO)(pinfo))

typedef struct {
  UINT cbSize;
  POINT pt;
  UINT uHit;
  SYSTEMTIME st;
#if(NTDDI_VERSION >= NTDDI_VISTA)
  RECT rc;
  int iOffset;
  int iRow;
  int iCol;
#endif
} MCHITTESTINFO, *PMCHITTESTINFO;

#define MCHITTESTINFO_V1_SIZE CCSIZEOF_STRUCT(MCHITTESTINFO, st)

#define MCHT_TITLE 0x10000
#define MCHT_CALENDAR 0x20000
#define MCHT_TODAYLINK 0x30000

#define MCHT_NEXT 0x1000000
#define MCHT_PREV 0x2000000

#define MCHT_NOWHERE 0x0

#define MCHT_TITLEBK (MCHT_TITLE)
#define MCHT_TITLEMONTH (MCHT_TITLE | 0x1)
#define MCHT_TITLEYEAR (MCHT_TITLE | 0x2)
#define MCHT_TITLEBTNNEXT (MCHT_TITLE | MCHT_NEXT | 0x3)
#define MCHT_TITLEBTNPREV (MCHT_TITLE | MCHT_PREV | 0x3)

#define MCHT_CALENDARBK (MCHT_CALENDAR)
#define MCHT_CALENDARDATE (MCHT_CALENDAR | 0x1)
#define MCHT_CALENDARDATENEXT (MCHT_CALENDARDATE | MCHT_NEXT)
#define MCHT_CALENDARDATEPREV (MCHT_CALENDARDATE | MCHT_PREV)
#define MCHT_CALENDARDAY (MCHT_CALENDAR | 0x2)
#define MCHT_CALENDARWEEKNUM (MCHT_CALENDAR | 0x3)

#define MCM_SETFIRSTDAYOFWEEK (MCM_FIRST+15)
#define MonthCal_SetFirstDayOfWeek(hmc,iDay) SNDMSG(hmc,MCM_SETFIRSTDAYOFWEEK,0,iDay)
#define MCM_GETFIRSTDAYOFWEEK (MCM_FIRST+16)
#define MonthCal_GetFirstDayOfWeek(hmc) (DWORD)SNDMSG(hmc,MCM_GETFIRSTDAYOFWEEK,0,0)
#define MCM_GETRANGE (MCM_FIRST+17)
#define MonthCal_GetRange(hmc,rgst) (DWORD)SNDMSG(hmc,MCM_GETRANGE,0,(LPARAM)(rgst))
#define MCM_SETRANGE (MCM_FIRST+18)
#define MonthCal_SetRange(hmc,gd,rgst) (BOOL)SNDMSG(hmc,MCM_SETRANGE,(WPARAM)(gd),(LPARAM)(rgst))
#define MCM_GETMONTHDELTA (MCM_FIRST+19)
#define MonthCal_GetMonthDelta(hmc) (int)SNDMSG(hmc,MCM_GETMONTHDELTA,0,0)
#define MCM_SETMONTHDELTA (MCM_FIRST+20)
#define MonthCal_SetMonthDelta(hmc,n) (int)SNDMSG(hmc,MCM_SETMONTHDELTA,n,0)
#define MCM_GETMAXTODAYWIDTH (MCM_FIRST+21)
#define MonthCal_GetMaxTodayWidth(hmc) (DWORD)SNDMSG(hmc,MCM_GETMAXTODAYWIDTH,0,0)
#define MCM_GETCURRENTVIEW (MCM_FIRST + 22)
#define MonthCal_GetCurrentView(hmc) (DWORD)SNDMSG(hmc, MCM_GETCURRENTVIEW, 0, 0)
#define MCM_GETCALENDARCOUNT (MCM_FIRST + 23)
#define MonthCal_GetCalendarCount(hmc) (DWORD)SNDMSG(hmc, MCM_GETCALENDARCOUNT, 0, 0)
#define MCM_SETUNICODEFORMAT CCM_SETUNICODEFORMAT
#define MonthCal_SetUnicodeFormat(hwnd,fUnicode) (BOOL)SNDMSG((hwnd),MCM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)
#define MCM_GETUNICODEFORMAT CCM_GETUNICODEFORMAT
#define MonthCal_GetUnicodeFormat(hwnd) (BOOL)SNDMSG((hwnd),MCM_GETUNICODEFORMAT,0,0)
#define MCM_SIZERECTTOMIN (MCM_FIRST + 29)
#define MonthCal_SizeRectToMin(hmc, prc) SNDMSG(hmc, MCM_SIZERECTTOMIN, 0, (LPARAM)(prc))
#define MCM_SETCALENDARBORDER (MCM_FIRST + 30)
#define MCM_GETCALENDARBORDER (MCM_FIRST + 31)

  typedef struct tagNMSELCHANGE {
    NMHDR nmhdr;
    SYSTEMTIME stSelStart;
    SYSTEMTIME stSelEnd;
  } NMSELCHANGE,*LPNMSELCHANGE;

#define MCN_SELCHANGE (MCN_FIRST+1)

  typedef struct tagNMDAYSTATE {
    NMHDR nmhdr;
    SYSTEMTIME stStart;
    int cDayState;

    LPMONTHDAYSTATE prgDayState;
  } NMDAYSTATE,*LPNMDAYSTATE;

#define MCN_GETDAYSTATE (MCN_FIRST+3)

  typedef NMSELCHANGE NMSELECT,*LPNMSELECT;

#define MCN_SELECT (MCN_FIRST+4)

#define MCS_DAYSTATE           0x0001
#define MCS_MULTISELECT        0x0002
#define MCS_WEEKNUMBERS        0x0004
#define MCS_NOTODAYCIRCLE      0x0008
#define MCS_NOTODAY            0x0010
#define MCS_NOTRAILINGDATES    0x0040

#define GMR_VISIBLE 0
#define GMR_DAYSTATE 1

#endif /* !NOMONTHCAL */

#ifndef NODATETIMEPICK

#define DATETIMEPICK_CLASSW L"SysDateTimePick32"
#define DATETIMEPICK_CLASSA "SysDateTimePick32"

#define DATETIMEPICK_CLASS __MINGW_NAME_AW(DATETIMEPICK_CLASS)

#define DTM_FIRST 0x1000

#define DTM_GETSYSTEMTIME (DTM_FIRST+1)
#define DateTime_GetSystemtime(hdp,pst) (DWORD)SNDMSG(hdp,DTM_GETSYSTEMTIME,0,(LPARAM)(pst))
#define DTM_SETSYSTEMTIME (DTM_FIRST+2)
#define DateTime_SetSystemtime(hdp,gd,pst) (BOOL)SNDMSG(hdp,DTM_SETSYSTEMTIME,(WPARAM)(gd),(LPARAM)(pst))
#define DTM_GETRANGE (DTM_FIRST+3)
#define DateTime_GetRange(hdp,rgst) (DWORD)SNDMSG(hdp,DTM_GETRANGE,0,(LPARAM)(rgst))
#define DTM_SETRANGE (DTM_FIRST+4)
#define DateTime_SetRange(hdp,gd,rgst) (BOOL)SNDMSG(hdp,DTM_SETRANGE,(WPARAM)(gd),(LPARAM)(rgst))
#define DTM_SETFORMATA (DTM_FIRST+5)
#define DTM_SETFORMATW (DTM_FIRST+50)

#define DTM_SETFORMAT __MINGW_NAME_AW(DTM_SETFORMAT)

#define DateTime_SetFormat(hdp,sz) (BOOL)SNDMSG(hdp,DTM_SETFORMAT,0,(LPARAM)(sz))

#define DTM_SETMCCOLOR (DTM_FIRST+6)
#define DateTime_SetMonthCalColor(hdp,iColor,clr) SNDMSG(hdp,DTM_SETMCCOLOR,iColor,clr)
#define DTM_GETMCCOLOR (DTM_FIRST+7)
#define DateTime_GetMonthCalColor(hdp,iColor) SNDMSG(hdp,DTM_GETMCCOLOR,iColor,0)
#define DTM_GETMONTHCAL (DTM_FIRST+8)
#define DateTime_GetMonthCal(hdp) (HWND)SNDMSG(hdp,DTM_GETMONTHCAL,0,0)
#define DTM_SETMCFONT (DTM_FIRST+9)
#define DateTime_SetMonthCalFont(hdp,hfont,fRedraw) SNDMSG(hdp,DTM_SETMCFONT,(WPARAM)(hfont),(LPARAM)(fRedraw))
#define DTM_GETMCFONT (DTM_FIRST+10)
#define DateTime_GetMonthCalFont(hdp) SNDMSG(hdp,DTM_GETMCFONT,0,0)

#define DTS_UPDOWN 0x1
#define DTS_SHOWNONE 0x2
#define DTS_SHORTDATEFORMAT 0x0
#define DTS_LONGDATEFORMAT 0x4
#define DTS_SHORTDATECENTURYFORMAT 0xc
#define DTS_TIMEFORMAT 0x9
#define DTS_APPCANPARSE 0x10
#define DTS_RIGHTALIGN 0x20

#define DTN_DATETIMECHANGE (DTN_FIRST+1)
  typedef struct tagNMDATETIMECHANGE {
    NMHDR nmhdr;
    DWORD dwFlags;
    SYSTEMTIME st;
  } NMDATETIMECHANGE,*LPNMDATETIMECHANGE;

#define DTN_USERSTRINGA (DTN_FIRST+2)
#define DTN_USERSTRINGW (DTN_FIRST+15)
  typedef struct tagNMDATETIMESTRINGA {
    NMHDR nmhdr;
    LPCSTR pszUserString;
    SYSTEMTIME st;
    DWORD dwFlags;
  } NMDATETIMESTRINGA,*LPNMDATETIMESTRINGA;

  typedef struct tagNMDATETIMESTRINGW {
    NMHDR nmhdr;
    LPCWSTR pszUserString;
    SYSTEMTIME st;
    DWORD dwFlags;
  } NMDATETIMESTRINGW,*LPNMDATETIMESTRINGW;

#define DTN_USERSTRING __MINGW_NAME_AW(DTN_USERSTRING)
#define NMDATETIMESTRING __MINGW_NAME_AW(NMDATETIMESTRING)
#define LPNMDATETIMESTRING __MINGW_NAME_AW(LPNMDATETIMESTRING)

#define DTN_WMKEYDOWNA (DTN_FIRST+3)
#define DTN_WMKEYDOWNW (DTN_FIRST+16)
  typedef struct tagNMDATETIMEWMKEYDOWNA {
    NMHDR nmhdr;
    int nVirtKey;
    LPCSTR pszFormat;
    SYSTEMTIME st;
  } NMDATETIMEWMKEYDOWNA,*LPNMDATETIMEWMKEYDOWNA;

  typedef struct tagNMDATETIMEWMKEYDOWNW {
    NMHDR nmhdr;
    int nVirtKey;
    LPCWSTR pszFormat;
    SYSTEMTIME st;
  } NMDATETIMEWMKEYDOWNW,*LPNMDATETIMEWMKEYDOWNW;

#define DTN_WMKEYDOWN __MINGW_NAME_AW(DTN_WMKEYDOWN)
#define NMDATETIMEWMKEYDOWN __MINGW_NAME_AW(NMDATETIMEWMKEYDOWN)
#define LPNMDATETIMEWMKEYDOWN __MINGW_NAME_AW(LPNMDATETIMEWMKEYDOWN)

#define DTN_FORMATA (DTN_FIRST+4)
#define DTN_FORMATW (DTN_FIRST+17)
  typedef struct tagNMDATETIMEFORMATA {
    NMHDR nmhdr;
    LPCSTR pszFormat;
    SYSTEMTIME st;
    LPCSTR pszDisplay;
    CHAR szDisplay[64];
  } NMDATETIMEFORMATA,*LPNMDATETIMEFORMATA;

  typedef struct tagNMDATETIMEFORMATW {
    NMHDR nmhdr;
    LPCWSTR pszFormat;
    SYSTEMTIME st;
    LPCWSTR pszDisplay;
    WCHAR szDisplay[64];
  } NMDATETIMEFORMATW,*LPNMDATETIMEFORMATW;

#define DTN_FORMAT __MINGW_NAME_AW(DTN_FORMAT)
#define NMDATETIMEFORMAT __MINGW_NAME_AW(NMDATETIMEFORMAT)
#define LPNMDATETIMEFORMAT __MINGW_NAME_AW(LPNMDATETIMEFORMAT)

#define DTN_FORMATQUERYA (DTN_FIRST+5)
#define DTN_FORMATQUERYW (DTN_FIRST+18)
  typedef struct tagNMDATETIMEFORMATQUERYA {
    NMHDR nmhdr;
    LPCSTR pszFormat;
    SIZE szMax;
  } NMDATETIMEFORMATQUERYA,*LPNMDATETIMEFORMATQUERYA;

  typedef struct tagNMDATETIMEFORMATQUERYW {
    NMHDR nmhdr;
    LPCWSTR pszFormat;
    SIZE szMax;
  } NMDATETIMEFORMATQUERYW,*LPNMDATETIMEFORMATQUERYW;

#define DTN_FORMATQUERY __MINGW_NAME_AW(DTN_FORMATQUERY)
#define NMDATETIMEFORMATQUERY __MINGW_NAME_AW(NMDATETIMEFORMATQUERY)
#define LPNMDATETIMEFORMATQUERY __MINGW_NAME_AW(LPNMDATETIMEFORMATQUERY)

#define DTN_DROPDOWN (DTN_FIRST+6)
#define DTN_CLOSEUP (DTN_FIRST+7)

#define GDTR_MIN 0x1
#define GDTR_MAX 0x2

#define GDT_ERROR -1
#define GDT_VALID 0
#define GDT_NONE 1

#ifndef NOIPADDRESS

#define IPM_CLEARADDRESS (WM_USER+100)
#define IPM_SETADDRESS (WM_USER+101)
#define IPM_GETADDRESS (WM_USER+102)
#define IPM_SETRANGE (WM_USER+103)
#define IPM_SETFOCUS (WM_USER+104)
#define IPM_ISBLANK (WM_USER+105)

#define WC_IPADDRESSW L"SysIPAddress32"
#define WC_IPADDRESSA "SysIPAddress32"

#define WC_IPADDRESS __MINGW_NAME_AW(WC_IPADDRESS)

#define IPN_FIELDCHANGED (IPN_FIRST - 0)
  typedef struct tagNMIPADDRESS {
    NMHDR hdr;
    int iField;
    int iValue;
  } NMIPADDRESS,*LPNMIPADDRESS;

#define MAKEIPRANGE(low,high) ((LPARAM)(WORD)(((BYTE)(high) << 8)+(BYTE)(low)))

#define MAKEIPADDRESS(b1,b2,b3,b4) ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

#define FIRST_IPADDRESS(x) ((x>>24) & 0xff)
#define SECOND_IPADDRESS(x) ((x>>16) & 0xff)
#define THIRD_IPADDRESS(x) ((x>>8) & 0xff)
#define FOURTH_IPADDRESS(x) (x & 0xff)

#endif /* !NOIPADDRESS */

#ifndef NOPAGESCROLLER

#define WC_PAGESCROLLERW L"SysPager"
#define WC_PAGESCROLLERA "SysPager"

#define WC_PAGESCROLLER __MINGW_NAME_AW(WC_PAGESCROLLER)

#define PGS_VERT 0x0
#define PGS_HORZ 0x1
#define PGS_AUTOSCROLL 0x2
#define PGS_DRAGNDROP 0x4

#define PGF_INVISIBLE 0
#define PGF_NORMAL 1
#define PGF_GRAYED 2
#define PGF_DEPRESSED 4
#define PGF_HOT 8

#define PGB_TOPORLEFT 0
#define PGB_BOTTOMORRIGHT 1

#define PGM_SETCHILD (PGM_FIRST+1)
#define Pager_SetChild(hwnd,hwndChild) (void)SNDMSG((hwnd),PGM_SETCHILD,0,(LPARAM)(hwndChild))

#define PGM_RECALCSIZE (PGM_FIRST+2)
#define Pager_RecalcSize(hwnd) (void)SNDMSG((hwnd),PGM_RECALCSIZE,0,0)

#define PGM_FORWARDMOUSE (PGM_FIRST+3)
#define Pager_ForwardMouse(hwnd,bForward) (void)SNDMSG((hwnd),PGM_FORWARDMOUSE,(WPARAM)(bForward),0)

#define PGM_SETBKCOLOR (PGM_FIRST+4)
#define Pager_SetBkColor(hwnd,clr) (COLORREF)SNDMSG((hwnd),PGM_SETBKCOLOR,0,(LPARAM)(clr))

#define PGM_GETBKCOLOR (PGM_FIRST+5)
#define Pager_GetBkColor(hwnd) (COLORREF)SNDMSG((hwnd),PGM_GETBKCOLOR,0,0)

#define PGM_SETBORDER (PGM_FIRST+6)
#define Pager_SetBorder(hwnd,iBorder) (int)SNDMSG((hwnd),PGM_SETBORDER,0,(LPARAM)(iBorder))

#define PGM_GETBORDER (PGM_FIRST+7)
#define Pager_GetBorder(hwnd) (int)SNDMSG((hwnd),PGM_GETBORDER,0,0)

#define PGM_SETPOS (PGM_FIRST+8)
#define Pager_SetPos(hwnd,iPos) (int)SNDMSG((hwnd),PGM_SETPOS,0,(LPARAM)(iPos))

#define PGM_GETPOS (PGM_FIRST+9)
#define Pager_GetPos(hwnd) (int)SNDMSG((hwnd),PGM_GETPOS,0,0)

#define PGM_SETBUTTONSIZE (PGM_FIRST+10)
#define Pager_SetButtonSize(hwnd,iSize) (int)SNDMSG((hwnd),PGM_SETBUTTONSIZE,0,(LPARAM)(iSize))

#define PGM_GETBUTTONSIZE (PGM_FIRST+11)
#define Pager_GetButtonSize(hwnd) (int)SNDMSG((hwnd),PGM_GETBUTTONSIZE,0,0)

#define PGM_GETBUTTONSTATE (PGM_FIRST+12)
#define Pager_GetButtonState(hwnd,iButton) (DWORD)SNDMSG((hwnd),PGM_GETBUTTONSTATE,0,(LPARAM)(iButton))

#define PGM_GETDROPTARGET CCM_GETDROPTARGET
#define Pager_GetDropTarget(hwnd,ppdt) (void)SNDMSG((hwnd),PGM_GETDROPTARGET,0,(LPARAM)(ppdt))

#define PGN_SCROLL (PGN_FIRST-1)

#define PGF_SCROLLUP 1
#define PGF_SCROLLDOWN 2
#define PGF_SCROLLLEFT 4
#define PGF_SCROLLRIGHT 8

#define PGK_SHIFT 1
#define PGK_CONTROL 2
#define PGK_MENU 4

#include <pshpack1.h>

  typedef struct {
    NMHDR hdr;
    WORD fwKeys;
    RECT rcParent;
    int iDir;
    int iXpos;
    int iYpos;
    int iScroll;
  }NMPGSCROLL,*LPNMPGSCROLL;

#include <poppack.h>

#define PGN_CALCSIZE (PGN_FIRST-2)

#define PGF_CALCWIDTH 1
#define PGF_CALCHEIGHT 2

  typedef struct {
    NMHDR hdr;
    DWORD dwFlag;
    int iWidth;
    int iHeight;
  }NMPGCALCSIZE,*LPNMPGCALCSIZE;

#define PGN_HOTITEMCHANGE (PGN_FIRST-3)

  typedef struct tagNMPGHOTITEM
  {
    NMHDR hdr;
    int idOld;
    int idNew;
    DWORD dwFlags;
  } NMPGHOTITEM,*LPNMPGHOTITEM;

#endif /* !NOPAGESCROLLER */

#ifndef NONATIVEFONTCTL

#define WC_NATIVEFONTCTLW L"NativeFontCtl"
#define WC_NATIVEFONTCTLA "NativeFontCtl"

#define WC_NATIVEFONTCTL __MINGW_NAME_AW(WC_NATIVEFONTCTL)

#define NFS_EDIT 0x1
#define NFS_STATIC 0x2
#define NFS_LISTCOMBO 0x4
#define NFS_BUTTON 0x8
#define NFS_ALL 0x10
#define NFS_USEFONTASSOC 0x20

#endif /* !NONATIVEFONTCTL */

#ifndef NOBUTTON

#define WC_BUTTONA "Button"
#define WC_BUTTONW L"Button"

#define WC_BUTTON __MINGW_NAME_AW(WC_BUTTON)

#define BUTTON_IMAGELIST_ALIGN_LEFT 0
#define BUTTON_IMAGELIST_ALIGN_RIGHT 1
#define BUTTON_IMAGELIST_ALIGN_TOP 2
#define BUTTON_IMAGELIST_ALIGN_BOTTOM 3
#define BUTTON_IMAGELIST_ALIGN_CENTER 4

  typedef struct {
    HIMAGELIST himl;
    RECT margin;
    UINT uAlign;
  } BUTTON_IMAGELIST,*PBUTTON_IMAGELIST;

#define BCM_GETIDEALSIZE (BCM_FIRST+0x1)
#define Button_GetIdealSize(hwnd,psize) (BOOL)SNDMSG((hwnd),BCM_GETIDEALSIZE,0,(LPARAM)(psize))

#define BCM_SETIMAGELIST (BCM_FIRST+0x2)
#define Button_SetImageList(hwnd,pbuttonImagelist) (BOOL)SNDMSG((hwnd),BCM_SETIMAGELIST,0,(LPARAM)(pbuttonImagelist))

#define BCM_GETIMAGELIST (BCM_FIRST+0x3)
#define Button_GetImageList(hwnd,pbuttonImagelist) (BOOL)SNDMSG((hwnd),BCM_GETIMAGELIST,0,(LPARAM)(pbuttonImagelist))

#define BCM_SETTEXTMARGIN (BCM_FIRST+0x4)
#define Button_SetTextMargin(hwnd,pmargin) (BOOL)SNDMSG((hwnd),BCM_SETTEXTMARGIN,0,(LPARAM)(pmargin))
#define BCM_GETTEXTMARGIN (BCM_FIRST+0x5)
#define Button_GetTextMargin(hwnd,pmargin) (BOOL)SNDMSG((hwnd),BCM_GETTEXTMARGIN,0,(LPARAM)(pmargin))

  typedef struct tagNMBCHOTITEM {
    NMHDR hdr;
    DWORD dwFlags;
  } NMBCHOTITEM,*LPNMBCHOTITEM;

#define BCN_HOTITEMCHANGE (BCN_FIRST+0x1)

#define BST_HOT 0x200

#endif /* !NOBUTTON */

#ifndef NOSTATIC

#define WC_STATICA "Static"
#define WC_STATICW L"Static"

#define WC_STATIC __MINGW_NAME_AW(WC_STATIC)

#ifndef NOEDIT

#define WC_EDITA "Edit"
#define WC_EDITW L"Edit"

#define WC_EDIT __MINGW_NAME_AW(WC_EDIT)

#define EM_SETCUEBANNER (ECM_FIRST+1)
#define Edit_SetCueBannerText(hwnd,lpcwText) (BOOL)SNDMSG((hwnd),EM_SETCUEBANNER,0,(LPARAM)(lpcwText))
#define EM_GETCUEBANNER (ECM_FIRST+2)
#define Edit_GetCueBannerText(hwnd,lpwText,cchText) (BOOL)SNDMSG((hwnd),EM_GETCUEBANNER,(WPARAM)(lpwText),(LPARAM)(cchText))

  typedef struct _tagEDITBALLOONTIP {
    DWORD cbStruct;
    LPCWSTR pszTitle;
    LPCWSTR pszText;
    INT ttiIcon;
  } EDITBALLOONTIP,*PEDITBALLOONTIP;
#define EM_SHOWBALLOONTIP (ECM_FIRST+3)
#define Edit_ShowBalloonTip(hwnd,peditballoontip) (BOOL)SNDMSG((hwnd),EM_SHOWBALLOONTIP,0,(LPARAM)(peditballoontip))
#define EM_HIDEBALLOONTIP (ECM_FIRST+4)
#define Edit_HideBalloonTip(hwnd) (BOOL)SNDMSG((hwnd),EM_HIDEBALLOONTIP,0,0)

#endif /* !NOEDIT */

#ifndef NOLISTBOX
#define WC_LISTBOXA "ListBox"
#define WC_LISTBOXW L"ListBox"
#define WC_LISTBOX __MINGW_NAME_AW(WC_LISTBOX)
#endif /* !NOLISTBOX */

#ifndef NOCOMBOBOX
#define WC_COMBOBOXA "ComboBox"
#define WC_COMBOBOXW L"ComboBox"
#define WC_COMBOBOX __MINGW_NAME_AW(WC_COMBOBOX)
#endif /* !NOCOMBOBOX */

#define CB_SETMINVISIBLE (CBM_FIRST+1)
#define CB_GETMINVISIBLE (CBM_FIRST+2)

#define ComboBox_SetMinVisible(hwnd,iMinVisible) (BOOL)SNDMSG((hwnd),CB_SETMINVISIBLE,(WPARAM)iMinVisible,0)
#define ComboBox_GetMinVisible(hwnd) (int)SNDMSG((hwnd),CB_GETMINVISIBLE,0,0)

#ifndef NOSCROLLBAR
#define WC_SCROLLBARA "ScrollBar"
#define WC_SCROLLBARW L"ScrollBar"
#define WC_SCROLLBAR __MINGW_NAME_AW(WC_SCROLLBAR)
#endif /* !NOSCROLLBAR */

#define INVALID_LINK_INDEX (-1)
#define MAX_LINKID_TEXT 48
#define L_MAX_URL_LENGTH (2048+32+sizeof("://"))

#define WC_LINK L"SysLink"

#define LWS_TRANSPARENT 0x1
#define LWS_IGNORERETURN 0x2

#define LIF_ITEMINDEX 0x1
#define LIF_STATE 0x2
#define LIF_ITEMID 0x4
#define LIF_URL 0x8

#define LIS_FOCUSED 0x1
#define LIS_ENABLED 0x2
#define LIS_VISITED 0x4

  typedef struct tagLITEM {
    UINT mask;
    int iLink;
    UINT state;
    UINT stateMask;
    WCHAR szID[MAX_LINKID_TEXT];
    WCHAR szUrl[L_MAX_URL_LENGTH];
  } LITEM,*PLITEM;

  typedef struct tagLHITTESTINFO {
    POINT pt;
    LITEM item;
  } LHITTESTINFO,*PLHITTESTINFO;

  typedef struct tagNMLINK {
    NMHDR hdr;
    LITEM item;
  } NMLINK,*PNMLINK;

#define LM_HITTEST (WM_USER+0x300)
#define LM_GETIDEALHEIGHT (WM_USER+0x301)
#define LM_SETITEM (WM_USER+0x302)
#define LM_GETITEM (WM_USER+0x303)

#ifndef NOMUI
  void WINAPI InitMUILanguage(LANGID uiLang);
  LANGID WINAPI GetMUILanguage(void);
#endif

#endif /* !NOSTATIC */

#define DA_LAST (0x7fffffff)
#define DPA_APPEND (0x7fffffff)
#define DPA_ERR (-1)

#define DSA_APPEND (0x7fffffff)
#define DSA_ERR (-1)

  typedef struct _DSA *HDSA;

  typedef int (CALLBACK *PFNDPAENUMCALLBACK)(void *p,void *pData);
  typedef int (CALLBACK *PFNDSAENUMCALLBACK)(void *p,void *pData);

  WINCOMMCTRLAPI HDSA WINAPI DSA_Create(int cbItem,int cItemGrow);
  WINCOMMCTRLAPI BOOL WINAPI DSA_Destroy(_Inout_opt_ HDSA hdsa);

  WINCOMMCTRLAPI
  void
  WINAPI
  DSA_DestroyCallback(
    _Inout_opt_ HDSA hdsa,
    _In_ PFNDSAENUMCALLBACK pfnCB,
    _In_opt_ void *pData);

  WINCOMMCTRLAPI
  VOID
  WINAPI
  DSA_EnumCallback(
    _In_ HDSA hdsa,
    _In_ PFNDSAENUMCALLBACK enumProc,
    _In_opt_ LPVOID lParam);

  WINCOMMCTRLAPI PVOID WINAPI DSA_GetItemPtr(_In_ HDSA hdsa, int i);

  WINCOMMCTRLAPI BOOL WINAPI DSA_DeleteAllItems(_In_ HDSA hdsa);

  WINCOMMCTRLAPI
  int
  WINAPI
  DSA_InsertItem(
    _Inout_ HDSA hdsa,
    _In_ int i,
    _In_ void *pitem);

  typedef struct _DPA *HDPA;

  WINCOMMCTRLAPI HDPA WINAPI DPA_Create(int cItemGrow);
  WINCOMMCTRLAPI BOOL WINAPI DPA_Destroy(_Inout_opt_ HDPA hdpa);
  WINCOMMCTRLAPI PVOID WINAPI DPA_DeletePtr(_Inout_ HDPA hdpa, _In_ int i);
  WINCOMMCTRLAPI BOOL WINAPI DPA_DeleteAllPtrs(_Inout_ HDPA hdpa);

  WINCOMMCTRLAPI
  void
  WINAPI
  DPA_EnumCallback(
    _In_opt_ HDPA hdpa,
    _In_opt_ PFNDPAENUMCALLBACK pfnCB,
    _In_opt_ void *pData);

  WINCOMMCTRLAPI
  void
  WINAPI
  DPA_DestroyCallback(
    _Inout_opt_ HDPA hdpa,
    _In_ PFNDPAENUMCALLBACK pfnCB,
    _In_opt_ void *pData);

  typedef int (CALLBACK *PFNDPACOMPARE)(void *p1,void *p2,LPARAM lParam);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  DPA_Sort(
    _Inout_ HDPA hdpa,
    _In_ PFNDPACOMPARE pfnCompare,
    _In_ LPARAM lParam);

#define DPAS_SORTED 0x1
#define DPAS_INSERTBEFORE 0x2
#define DPAS_INSERTAFTER 0x4

  WINCOMMCTRLAPI
  int
  WINAPI
  DPA_Search(
    _In_ HDPA hdpa,
    _In_opt_ void *pFind,
    _In_ int iStart,
    _In_ PFNDPACOMPARE pfnCompare,
    _In_ LPARAM lParam,
    _In_ UINT options);

  WINCOMMCTRLAPI
  BOOL
  WINAPI Str_SetPtrW(
    _Inout_ LPWSTR *ppsz,
    _In_opt_ LPCWSTR psz);

typedef struct _DPASTREAMINFO {
  int iPos;
  void *pvItem;
} DPASTREAMINFO;

struct IStream;

typedef HRESULT
(CALLBACK *PFNDPASTREAM)(
  _In_ DPASTREAMINFO*,
  _In_ struct IStream*,
  _In_opt_ void*);

typedef void*
(CALLBACK *PFNDPAMERGE)(
  _In_ UINT,
  _In_ void*,
  _In_ void*,
  _In_ LPARAM);

typedef const void*
(CALLBACK *PFNDPAMERGECONST)(
  _In_ UINT,
  _In_ const void*,
  _In_ const void*,
  _In_ LPARAM);

  WINCOMMCTRLAPI
  HRESULT
  WINAPI
  DPA_LoadStream(
    _Outptr_ HDPA * phdpa,
    _In_ PFNDPASTREAM pfn,
    _In_ struct IStream * pstream,
    _In_opt_ void *pvInstData);

  WINCOMMCTRLAPI
  HRESULT
  WINAPI
  DPA_SaveStream(
    _In_ HDPA hdpa,
    _In_ PFNDPASTREAM pfn,
    _In_ struct IStream * pstream,
    _In_opt_ void *pvInstData);

  WINCOMMCTRLAPI BOOL WINAPI DPA_Grow(_Inout_ HDPA pdpa, _In_ int cp);

  WINCOMMCTRLAPI
  int
  WINAPI
  DPA_InsertPtr(
    _Inout_ HDPA hdpa,
    _In_ int i,
    _In_opt_ void *p);

  WINCOMMCTRLAPI PVOID WINAPI DPA_GetPtr(_In_ HDPA hdpa, _In_ INT_PTR i);

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  DPA_SetPtr(
    _Inout_ HDPA hdpa,
    _In_ int i,
    _In_opt_ void *p);

  WINCOMMCTRLAPI
  int
  WINAPI
  DPA_GetPtrIndex(
    _In_ HDPA hdpa,
    _In_opt_ const void *p);

#define DPA_GetPtrCount(hdpa) (*(int *)(hdpa))
#define DPA_SetPtrCount(hdpa, cItems) (*(int *)(hdpa) = (cItems))
#define DPA_GetPtrPtr(hdpa) (*((void * **)((BYTE *)(hdpa) + sizeof(void *))))
#define DPA_AppendPtr(hdpa, pitem) DPA_InsertPtr(hdpa, DA_LAST, pitem)
#define DPA_FastDeleteLastPtr(hdpa) (--*(int *)(hdpa))
#define DPA_FastGetPtr(hdpa, i) (DPA_GetPtrPtr(hdpa)[i])

#define DPAM_SORTED    1
#define DPAM_NORMAL    2
#define DPAM_UNION     4
#define DPAM_INTERSECT 8

#define DPAMM_MERGE    1
#define DPAMM_DELETE   2
#define DPAMM_INSERT   3

#ifndef NOTRACKMOUSEEVENT

#ifndef WM_MOUSEHOVER
#define WM_MOUSEHOVER 0x2a1
#define WM_MOUSELEAVE 0x2a3
#endif

#ifndef TME_HOVER

#define TME_HOVER 0x1
#define TME_LEAVE 0x2
#define TME_NONCLIENT 0x10
#define TME_QUERY 0x40000000
#define TME_CANCEL 0x80000000

#define HOVER_DEFAULT 0xffffffff

  typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize;
    DWORD dwFlags;
    HWND hwndTrack;
    DWORD dwHoverTime;
  } TRACKMOUSEEVENT,*LPTRACKMOUSEEVENT;

#endif /* !TME_HOVER */

  WINCOMMCTRLAPI
  BOOL
  WINAPI
  _TrackMouseEvent(
    _Inout_ LPTRACKMOUSEEVENT lpEventTrack);

#endif /* !NOTRACKMOUSEEVENT */

#ifndef NOFLATSBAPIS

#define WSB_PROP_CYVSCROLL 0x1L
#define WSB_PROP_CXHSCROLL 0x2L
#define WSB_PROP_CYHSCROLL 0x4L
#define WSB_PROP_CXVSCROLL 0x8L
#define WSB_PROP_CXHTHUMB 0x10L
#define WSB_PROP_CYVTHUMB 0x20L
#define WSB_PROP_VBKGCOLOR 0x40L
#define WSB_PROP_HBKGCOLOR 0x80L
#define WSB_PROP_VSTYLE 0x100L
#define WSB_PROP_HSTYLE 0x200L
#define WSB_PROP_WINSTYLE 0x400L
#define WSB_PROP_PALETTE 0x800L
#define WSB_PROP_MASK 0xfffL

#define FSB_FLAT_MODE 2
#define FSB_ENCARTA_MODE 1
#define FSB_REGULAR_MODE 0

  WINCOMMCTRLAPI BOOL WINAPI FlatSB_EnableScrollBar(HWND,int,UINT);
  WINCOMMCTRLAPI BOOL WINAPI FlatSB_ShowScrollBar(HWND,int code,BOOL);
  WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollRange(HWND,int code,LPINT,LPINT);
  WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollInfo(HWND,int code,LPSCROLLINFO);
  WINCOMMCTRLAPI int WINAPI FlatSB_GetScrollPos(HWND,int code);
  WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollProp(HWND,int propIndex,LPINT);
#ifdef _WIN64
  WINCOMMCTRLAPI BOOL WINAPI FlatSB_GetScrollPropPtr(HWND,int propIndex,PINT_PTR);
#else
#define FlatSB_GetScrollPropPtr FlatSB_GetScrollProp
#endif

  WINCOMMCTRLAPI int WINAPI FlatSB_SetScrollPos(HWND,int code,int pos,BOOL fRedraw);
  WINCOMMCTRLAPI int WINAPI FlatSB_SetScrollInfo(HWND,int code,LPSCROLLINFO,BOOL fRedraw);
  WINCOMMCTRLAPI int WINAPI FlatSB_SetScrollRange(HWND,int code,int min,int max,BOOL fRedraw);
  WINCOMMCTRLAPI BOOL WINAPI FlatSB_SetScrollProp(HWND,UINT index,INT_PTR newValue,BOOL);
#define FlatSB_SetScrollPropPtr FlatSB_SetScrollProp
  WINCOMMCTRLAPI BOOL WINAPI InitializeFlatSB(HWND);
  WINCOMMCTRLAPI HRESULT WINAPI UninitializeFlatSB(HWND);

#endif /* !NOFLATSBAPIS */

#endif /* !NODATETIMEPICK */

  typedef LRESULT (CALLBACK *SUBCLASSPROC)(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

  _Success_(return != 0)
  BOOL
  WINAPI
  SetWindowSubclass(
    _In_ HWND hWnd,
    _In_ SUBCLASSPROC pfnSubclass,
    _In_ UINT_PTR uIdSubclass,
    _In_ DWORD_PTR dwRefData);

  _Success_(return != 0)
  BOOL
  WINAPI
  GetWindowSubclass(
    _In_ HWND hWnd,
    _In_ SUBCLASSPROC pfnSubclass,
    _In_ UINT_PTR uIdSubclass,
    _Out_opt_ DWORD_PTR *pdwRefData);

  _Success_(return != 0)
  BOOL
  WINAPI
  RemoveWindowSubclass(
    _In_ HWND hWnd,
    _In_ SUBCLASSPROC pfnSubclass,
    _In_ UINT_PTR uIdSubclass);

  LRESULT WINAPI DefSubclassProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

  int
  WINAPI
  DrawShadowText(
    _In_ HDC hdc,
    _In_reads_(cch) LPCWSTR pszText,
    _In_ UINT cch,
    _In_ RECT *prc,
    _In_ DWORD dwFlags,
    _In_ COLORREF crText,
    _In_ COLORREF crShadow,
    _In_ int ixOffset,
    _In_ int iyOffset);

#ifndef NOTASKDIALOG

#include <pshpack1.h>

enum _TASKDIALOG_FLAGS
{
    TDF_ENABLE_HYPERLINKS           = 0x0001,
    TDF_USE_HICON_MAIN              = 0x0002,
    TDF_USE_HICON_FOOTER            = 0x0004,
    TDF_ALLOW_DIALOG_CANCELLATION   = 0x0008,
    TDF_USE_COMMAND_LINKS           = 0x0010,
    TDF_USE_COMMAND_LINKS_NO_ICON   = 0x0020,
    TDF_EXPAND_FOOTER_AREA          = 0x0040,
    TDF_EXPANDED_BY_DEFAULT         = 0x0080,
    TDF_VERIFICATION_FLAG_CHECKED   = 0x0100,
    TDF_SHOW_PROGRESS_BAR           = 0x0200,
    TDF_SHOW_MARQUEE_PROGRESS_BAR   = 0x0400,
    TDF_CALLBACK_TIMER              = 0x0800,
    TDF_POSITION_RELATIVE_TO_WINDOW = 0x1000,
    TDF_RTL_LAYOUT                  = 0x2000,
    TDF_NO_DEFAULT_RADIO_BUTTON     = 0x4000,
    TDF_CAN_BE_MINIMIZED            = 0x8000
};
typedef int TASKDIALOG_FLAGS;

enum _TASKDIALOG_COMMON_BUTTON_FLAGS
{
    TDCBF_OK_BUTTON     = 0x0001,
    TDCBF_YES_BUTTON    = 0x0002,
    TDCBF_NO_BUTTON     = 0x0004,
    TDCBF_CANCEL_BUTTON = 0x0008,
    TDCBF_RETRY_BUTTON  = 0x0010,
    TDCBF_CLOSE_BUTTON  = 0x0020
};
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;

typedef struct _TASKDIALOG_BUTTON
{
    int     nButtonID;
    PCWSTR  pszButtonText;
} TASKDIALOG_BUTTON;

typedef HRESULT
(CALLBACK *PFTASKDIALOGCALLBACK)(
  _In_ HWND,
  _In_ UINT,
  _In_ WPARAM,
  _In_ LPARAM,
  _In_ LONG_PTR);

typedef struct _TASKDIALOGCONFIG
{
    UINT        cbSize;
    HWND        hwndParent;
    HINSTANCE   hInstance;
    TASKDIALOG_FLAGS    dwFlags;
    TASKDIALOG_COMMON_BUTTON_FLAGS  dwCommonButtons;
    PCWSTR      pszWindowTitle;
    union
    {
        HICON   hMainIcon;
        PCWSTR  pszMainIcon;
    } DUMMYUNIONNAME;
    PCWSTR      pszMainInstruction;
    PCWSTR      pszContent;
    UINT        cButtons;
    const TASKDIALOG_BUTTON *pButtons;
    int         nDefaultButton;
    UINT        cRadioButtons;
    const TASKDIALOG_BUTTON *pRadioButtons;
    int         nDefaultRadioButton;
    PCWSTR      pszVerificationText;
    PCWSTR      pszExpandedInformation;
    PCWSTR      pszExpandedControlText;
    PCWSTR      pszCollapsedControlText;
    union
    {
        HICON   hFooterIcon;
        PCWSTR  pszFooterIcon;
    } DUMMYUNIONNAME2;
    PCWSTR      pszFooter;
    PFTASKDIALOGCALLBACK pfCallback;
    LONG_PTR    lpCallbackData;
    UINT        cxWidth;
} TASKDIALOGCONFIG;

HRESULT
WINAPI
TaskDialogIndirect(
  _In_ const TASKDIALOGCONFIG *,
  _Out_opt_ int *,
  _Out_opt_ int *,
  _Out_opt_ BOOL *);

#include <poppack.h>

#endif /* !NOTASKDIALOG */

#if (NTDDI_VERSION >= NTDDI_VISTA)
enum _LI_METRIC
{
   LIM_SMALL,
   LIM_LARGE,
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* !NOUSER */

#endif /* _INC_COMMCTRL */
