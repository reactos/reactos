#ifndef __WIN32K_MENU_H
#define __WIN32K_MENU_H

#include <ddk/ntddk.h>
#include <napi/win32.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))
  
#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))
  
#ifndef MF_END
#define MF_END             (0x0080)
#endif

#ifndef MIIM_STRING
#define MIIM_STRING      (0x00000040)
#endif
  
typedef struct _MENU_ITEM
{
  struct _MENU_ITEM *Next;
  MENUITEMINFOW MenuItem;
} MENU_ITEM, *PMENU_ITEM;

typedef struct _MENU_OBJECT
{
  HANDLE Self;
  int MenuItemCount;
  FAST_MUTEX MenuItemsLock;
  PMENU_ITEM MenuItemList;
  BOOL RtoL;
  DWORD dwStyle;
  UINT cyMax;
  HBRUSH hbrBack;
  DWORD dwContextHelpID;
  ULONG_PTR dwMenuData;
} MENU_OBJECT, *PMENU_OBJECT;

NTSTATUS FASTCALL
InitMenuImpl(VOID);

NTSTATUS FASTCALL
CleanupMenuImpl(VOID);

DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 LPCMENUITEMINFOW* lpmiil,
 ULONG nBufSize,
 DWORD Reserved);

DWORD
STDCALL
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck);
  
HMENU
STDCALL
NtUserCreateMenu(VOID);

BOOL
STDCALL
NtUserDeleteMenu(
HMENU hMenu,
  UINT uPosition,
  UINT uFlags);
  
BOOL
STDCALL
NtUserDestroyMenu(
  HMENU hMenu);
  
DWORD
STDCALL
NtUserDrawMenuBarTemp(
  HWND hWnd,
  HDC hDC,
  PRECT hRect,
  HMENU hMenu,
  DWORD Unknown4);
  
UINT
STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable);
  
DWORD
STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFOW lpmii);

  
BOOL
STDCALL
NtUserEndMenu(VOID);

BOOL
STDCALL
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi);
  
DWORD
STDCALL
NtUserGetMenuIndex(
  DWORD Unknown0,
  DWORD Unknown1);
  
BOOL
STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem);
  
BOOL
STDCALL
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite);

BOOL
STDCALL
NtUserMenuInfo(
 HMENU hmenu,
 LPMENUINFO lpmi,
 BOOL fsog);
  
int
STDCALL
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y);

BOOL
STDCALL
NtUserMenuItemInfo(
 HMENU hMenu,
 UINT uItem,
 BOOL fByPosition,
 LPMENUITEMINFOW lpmii,
 BOOL fsog);
  
BOOL
STDCALL
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags);
  
BOOL
STDCALL
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId);
  
BOOL
STDCALL
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos);
  
BOOL
STDCALL
NtUserSetMenuFlagRtoL(
  HMENU hMenu);
  
DWORD
STDCALL
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi);
  
DWORD
STDCALL
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption);
  
BOOL
STDCALL
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm);

#endif /* __WIN32K_MENU_H */

/* EOF */