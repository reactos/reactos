/*
 * Compatibility header
 *
 * This header is wrapper to allow compilation of Wine DLLs under ReactOS
 * build system. It contains definitions commonly refered to as Wineisms
 * and definitions that are missing in w32api.
 */

#include_next <commctrl.h>

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

typedef LPFINDINFOA LPLVFINDINFOA;
typedef LPFINDINFOW LPLVFINDINFOW;

#undef RB_GETBANDINFO
#define RB_GETBANDINFO (WM_USER+5)   /* just for compatibility */

#define LVIS_ACTIVATING         0x0020

#define ListView_FindItemA(hwnd,nItem,plvfi) \
    (INT)SendMessageA((hwnd),LVM_FINDITEMA,(WPARAM)(INT)(nItem),(LPARAM)(LVFINDINFOA*)(plvfi))
#define ListView_FindItemW(hwnd,nItem,plvfi) \
    (INT)SendMessageW((hwnd),LVM_FINDITEMW,(WPARAM)(INT)(nItem),(LPARAM)(LVFINDINFOW*)(plvfi))
#define ListView_InsertColumnA(hwnd,iCol,pcol) \
    (INT)SendMessageA((hwnd),LVM_INSERTCOLUMNA,(WPARAM)(INT)(iCol),(LPARAM)(const LVCOLUMNA *)(pcol))
#define ListView_GetItemA(hwnd,pitem) \
    (BOOL)SendMessageA((hwnd),LVM_GETITEMA,0,(LPARAM)(LVITEMA *)(pitem))
#define ListView_SetItemA(hwnd,pitem) \
    (INT)SendMessageA((hwnd),LVM_SETITEMA,0,(LPARAM)(const LVITEMA *)(pitem))
#define ListView_InsertItemA(hwnd,pitem) \
    (INT)SendMessageA((hwnd),LVM_INSERTITEMA,0,(LPARAM)(const LVITEMA *)(pitem))
#define ListView_EditLabelA(hwndLV, i) \
    (HWND)SendMessageA((hwndLV),LVM_EDITLABELA,(WPARAM)(int)(i), 0L)
#define TreeView_InsertItemW(hwnd,phdi) \
  (HTREEITEM)SendMessageW((hwnd), TVM_INSERTITEMW, 0, (LPARAM)(LPTVINSERTSTRUCTW)(phdi))
#define Header_SetItemW(hwndHD,i,phdi) \
  (BOOL)SendMessageW((hwndHD),HDM_SETITEMW,(WPARAM)(INT)(i),(LPARAM)(const HDITEMW*)(phdi))
#define Header_GetItemW(hwndHD,i,phdi) \
  (BOOL)SendMessageW((hwndHD),HDM_GETITEMW,(WPARAM)(INT)(i),(LPARAM)(HDITEMW*)(phdi))

#define TBSTYLE_EX_UNDOC1               0x00000004 /* similar to TBSTYLE_WRAPABLE */

/* undocumented messages in Toolbar */
#define TB_UNKWN45D              (WM_USER+93)
#define TB_UNKWN45E              (WM_USER+94)
#define TB_UNKWN460              (WM_USER+96)
#define TB_UNKWN462              (WM_USER+98)
#define TB_UNKWN463              (WM_USER+99)
#define TB_UNKWN464              (WM_USER+100)

#define RBBS_USECHEVRON         0x00000200
#define RBHT_CHEVRON            0x0008
#define RBN_CHEVRONPUSHED       (RBN_FIRST-10)
#define RB_PUSHCHEVRON          (WM_USER+43)

#define HDM_SETBITMAPMARGIN     (HDM_FIRST+20)
#define HDM_GETBITMAPMARGIN     (HDM_FIRST+21)

#define SB_SETBORDERS		(WM_USER+5)

#define FLATSB_CLASSA           "flatsb_class32"
#if defined(__GNUC__)
# define FLATSB_CLASSW (const WCHAR []){ 'f','l','a','t','s','b','_', \
  'c','l','a','s','s','3','2',0 }
#elif defined(_MSC_VER)
# define FLATSB_CLASSW        L"flatsb_class32"
#else
static const WCHAR FLATSB_CLASSW[] = { 'f','l','a','t','s','b','_',
  'c','l','a','s','s','3','2',0 };
#endif

#define DRAGLISTMSGSTRINGA      "commctrl_DragListMsg"
#if defined(__GNUC__)
# define DRAGLISTMSGSTRINGW (const WCHAR []){ 'c','o','m','m','c','t','r','l', \
  '_','D','r','a','g','L','i','s','t','M','s','g',0 }
#elif defined(_MSC_VER)
# define DRAGLISTMSGSTRINGW     L"commctrl_DragListMsg"
#else
static const WCHAR DRAGLISTMSGSTRINGW[] = { 'c','o','m','m','c','t','r','l', \
  '_','D','r','a','g','L','i','s','t','M','s','g',0 };
#endif

#endif /* __WINE_COMMCTRL_H */
