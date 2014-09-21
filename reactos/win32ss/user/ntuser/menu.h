#pragma once

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#ifndef MF_END
#define MF_END             (0x0080)
#endif

typedef struct _SETMENUITEMRECT
{
  UINT uItem;
  BOOL fByPosition;
  RECTL rcRect;
} SETMENUITEMRECT, *PSETMENUITEMRECT;

PMENU FASTCALL
IntGetMenuObject(HMENU hMenu);

#define IntReleaseMenuObject(MenuObj) \
  UserDereferenceObject(MenuObj)

BOOLEAN
UserDestroyMenuObject(PVOID Object);

BOOL FASTCALL
IntDestroyMenuObject(PMENU MenuObject, BOOL bRecurse);

PMENU FASTCALL
IntCloneMenu(PMENU Source);

int FASTCALL
IntGetMenuItemByFlag(PMENU MenuObject, UINT uSearchBy, UINT fFlag,
                     PMENU *SubMenu, PITEM *MenuItem,
                     PITEM *PrevMenuItem);

BOOL FASTCALL
IntCleanupMenus(struct _EPROCESS *Process, PPROCESSINFO Win32Process);

BOOL FASTCALL
IntInsertMenuItem(_In_ PMENU MenuObject, UINT uItem, BOOL fByPosition, PROSMENUITEMINFO ItemInfo, PUNICODE_STRING lpstr);

PMENU FASTCALL
IntGetSystemMenu(PWND Window, BOOL bRevert);

UINT FASTCALL IntFindSubMenu(HMENU *hMenu, HMENU hSubTarget );
UINT FASTCALL IntGetMenuState( HMENU hMenu, UINT uId, UINT uFlags);
BOOL FASTCALL IntRemoveMenuItem(PMENU Menu, UINT uPosition, UINT uFlags, BOOL bRecurse);
PITEM FASTCALL MENU_FindItem( PMENU *pmenu, UINT *nPos, UINT wFlags );
BOOL FASTCALL IntMenuItemInfo(PMENU Menu, UINT Item, BOOL ByPosition, PROSMENUITEMINFO UnsafeItemInfo, BOOL SetOrGet, PUNICODE_STRING lpstr);
