/* $Id: menu.h,v 1.1 2004/02/15 07:39:12 gvg Exp $ */

#ifndef WIN32K_MENU_H_INCLUDED
#define WIN32K_MENU_H_INCLUDED

typedef struct tagROSMENUINFO {
  /* ----------- MENUINFO ----------- */
  DWORD   cbSize;
  DWORD   fMask;
  DWORD   dwStyle;
  UINT    cyMax;
  HBRUSH  hbrBack;
  DWORD   dwContextHelpID;
  ULONG_PTR  dwMenuData;
  /* ----------- Extra ----------- */
  HMENU Self;         /* Handle of this menu */
  WORD Flags;         /* Menu flags (MF_POPUP, MF_SYSMENU) */
  UINT FocusedItem;   /* Currently focused item */
  UINT MenuItemCount; /* Number of items in the menu */
  HWND Wnd;           /* Window containing the menu */
  WORD Width;         /* Width of the whole menu */
  WORD Height;        /* Height of the whole menu */
  HWND WndOwner;     /* window receiving the messages for ownerdraw */
  BOOL TimeToHide;   /* Request hiding when receiving a second click in the top-level menu item */
} ROSMENUINFO, *PROSMENUINFO;

/* (other FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

typedef struct tagROSMENUITEMINFO
{
  /* ----------- MENUITEMINFOW ----------- */
  UINT    cbSize;
  UINT    fMask;
  UINT    fType;
  UINT    fState;
  UINT    wID;
  HMENU   hSubMenu;
  HBITMAP hbmpChecked;
  HBITMAP hbmpUnchecked;
  DWORD   dwItemData;
  LPWSTR  dwTypeData;
  UINT    cch;
  HBITMAP hbmpItem;
  /* ----------- Extra ----------- */
  RECT    Rect;	/* Item area (relative to menu window) */
  UINT    XTab; /* X position of text after Tab */
} ROSMENUITEMINFO, *PROSMENUITEMINFO;

typedef struct _SETMENUITEMRECT
{
  UINT uItem;
  BOOL fByPosition;
  RECT rcRect;
} SETMENUITEMRECT, *PSETMENUITEMRECT;

DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 PVOID Buffer,
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
NtUserCreateMenu(BOOL PopupMenu);

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
  HFONT hFont);

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
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii);

BOOL
STDCALL
NtUserEndMenu(VOID);

UINT STDCALL
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags);

BOOL
STDCALL
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi);

UINT
STDCALL
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID);

BOOL
STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem);

HMENU
STDCALL
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert);

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
 PROSMENUINFO lpmi,
 BOOL fsog
);

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
 PROSMENUITEMINFO lpmii,
 BOOL fsog
);

BOOL
STDCALL
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags);

BOOL
STDCALL
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint);

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

BOOL
STDCALL
NtUserSetSystemMenu(
  HWND hWnd,
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

#endif /* WIN32K_MENU_H_INCLUDED */

