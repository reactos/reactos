#ifndef __WIN32K_MENU_H
#define __WIN32K_MENU_H

#include <ddk/ntddk.h>
#include <napi/win32.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

typedef struct _MENU_OBJECT
{
  HANDLE Self;
  FAST_MUTEX MenuItemsLock;
  LIST_ENTRY MenuItemsHead;
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
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4);
  
BOOL
STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable);
  
/*DWORD
STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii);*/
  
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
  
int
STDCALL
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y);
  
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
  DWORD Unknown0,
  DWORD Unknown1);
  
DWORD
STDCALL
NtUserThunkedMenuItemInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5);
  
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
