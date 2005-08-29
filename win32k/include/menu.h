#ifndef _WIN32K_MENU_H
#define _WIN32K_MENU_H

#include <win32k/menu.h>

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
  RECT Rect;
  UINT XTab;
} MENU_ITEM, *PMENU_ITEM;

typedef struct _MENU_OBJECT
{
/*---------- USER_OBJECT_HDR --------------*/
  HMENU hSelf;
  LONG refs;
  BYTE flags;
  
/*---------- USER_OBJECT_HDR --------------*/
   
  PEPROCESS Process;
  LIST_ENTRY ListEntry;
  PMENU_ITEM MenuItemList;
  ROSMENUINFO MenuInfo;
  BOOL RtoL;
} MENU_OBJECT, *PMENU_OBJECT;




#endif /* _WIN32K_MENU_H */
