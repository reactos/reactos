/* $Id $
 *
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

#define FLATSB_CLASSA         "flatsb_class32"

#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)

#if (_WIN32_IE >= 0x0500)
#if (_WIN32_WINNT >= 0x0501)
#define COMCTL32_VERSION 6
#else
#define COMCTL32_VERSION 5
#endif
#endif

#define DPAS_SORTED	1
#define DPAS_INSERTBEFORE	2
#define DPAS_INSERTAFTER	4

#if (_WIN32_IE >= 0x0400)
#define TCIS_HIGHLIGHTED 2
#endif

typedef struct _DSA *HDSA;
typedef struct _DPA *HDPA;
typedef INT (CALLBACK *PFNDPAENUMCALLBACK)(PVOID,PVOID);
typedef INT (CALLBACK *PFNDSAENUMCALLBACK)(PVOID,PVOID);
typedef INT (CALLBACK *PFNDPACOMPARE)(PVOID,PVOID,LPARAM);
#if (_WIN32_WINNT >= 0x0501)
typedef LRESULT (CALLBACK *SUBCLASSPROC)(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
#endif /* _WIN32_WINNT >= 0x0501 */

HDSA WINAPI DSA_Create(INT,INT);
BOOL WINAPI DSA_Destroy(HDSA);
VOID WINAPI DSA_DestroyCallback(HDSA,PFNDSAENUMCALLBACK,PVOID);
PVOID WINAPI DSA_GetItemPtr(HDSA,INT);
INT WINAPI DSA_InsertItem(HDSA,INT,PVOID);

HDPA WINAPI DPA_Create(INT);
BOOL WINAPI DPA_Destroy(HDPA);
PVOID WINAPI DPA_DeletePtr(HDPA,INT);
BOOL WINAPI DPA_DeleteAllPtrs(HDPA);
VOID WINAPI DPA_EnumCallback(HDPA,PFNDPAENUMCALLBACK,PVOID);
VOID WINAPI DPA_DestroyCallback(HDPA,PFNDPAENUMCALLBACK,PVOID);
BOOL WINAPI DPA_SetPtr(HDPA,INT,PVOID);
INT WINAPI DPA_InsertPtr(HDPA,INT,PVOID);
PVOID WINAPI DPA_GetPtr(HDPA,INT_PTR);
BOOL WINAPI DPA_Sort(HDPA,PFNDPACOMPARE,LPARAM);
INT WINAPI DPA_Search(HDPA,PVOID,INT,PFNDPACOMPARE,LPARAM,UINT);
BOOL WINAPI Str_SetPtrW(LPWSTR*,LPCWSTR);

#if (_WIN32_WINNT >= 0x0501)
BOOL WINAPI SetWindowSubclass(HWND,SUBCLASSPROC,UINT_PTR,DWORD_PTR);
BOOL WINAPI RemoveWindowSubclass(HWND,SUBCLASSPROC,UINT_PTR);
LRESULT WINAPI DefSubclassProc(HWND,UINT,WPARAM,LPARAM);
#endif /* _WIN32_WINNT >= 0x0501 */

#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

#endif /* __WINE_COMMCTRL_H */
