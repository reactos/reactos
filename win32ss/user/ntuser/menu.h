#pragma once

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#ifndef MF_END
#define MF_END             (0x0080)
#endif

typedef struct _MENU_ITEM
{
  struct _MENU_ITEM *Next;
  UINT fType;
  UINT fState;
  UINT wID;
  HMENU hSubMenu;
  HBITMAP hbmpChecked;
  HBITMAP hbmpUnchecked;
  ULONG_PTR dwItemData;
  UNICODE_STRING Text;
  HBITMAP hbmpItem;
  RECTL Rect;
  UINT dxTab;
} MENU_ITEM, *PMENU_ITEM;

typedef struct _MENU_OBJECT
{
  PROCDESKHEAD head;
  PEPROCESS Process;
  LIST_ENTRY ListEntry;
  PMENU_ITEM MenuItemList;
  ROSMENUINFO MenuInfo;
  BOOL RtoL;
} MENU_OBJECT, *PMENU_OBJECT;

typedef struct _SETMENUITEMRECT
{
  UINT uItem;
  BOOL fByPosition;
  RECTL rcRect;
} SETMENUITEMRECT, *PSETMENUITEMRECT;

PMENU_OBJECT FASTCALL
IntGetMenuObject(HMENU hMenu);

#define IntReleaseMenuObject(MenuObj) \
  UserDereferenceObject(MenuObj)

BOOL FASTCALL
IntDestroyMenuObject(PMENU_OBJECT MenuObject, BOOL bRecurse, BOOL RemoveFromProcess);

PMENU_OBJECT FASTCALL
IntCloneMenu(PMENU_OBJECT Source);

int FASTCALL
IntGetMenuItemByFlag(PMENU_OBJECT MenuObject, UINT uSearchBy, UINT fFlag,
                     PMENU_OBJECT *SubMenu, PMENU_ITEM *MenuItem,
                     PMENU_ITEM *PrevMenuItem);

BOOL FASTCALL
IntCleanupMenus(struct _EPROCESS *Process, PPROCESSINFO Win32Process);

BOOL FASTCALL
IntInsertMenuItem(_In_ PMENU_OBJECT MenuObject, UINT uItem, BOOL fByPosition,
                  PROSMENUITEMINFO ItemInfo);

PMENU_OBJECT FASTCALL
IntGetSystemMenu(PWND Window, BOOL bRevert, BOOL RetMenu);

UINT FASTCALL IntFindSubMenu(HMENU *hMenu, HMENU hSubTarget );
UINT FASTCALL IntGetMenuState( HMENU hMenu, UINT uId, UINT uFlags);

