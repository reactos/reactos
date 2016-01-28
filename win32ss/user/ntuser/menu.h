#pragma once

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#ifndef MF_END
#define MF_END             (0x0080)
#endif

typedef struct tagMENUSTATE
{
  PPOPUPMENU  pGlobalPopupMenu;
  struct
  {
  ULONG       fMenuStarted:1;
  ULONG       fIsSysMenu:1;
  ULONG       fInsideMenuLoop:1;
  ULONG       fButtonDown:1;
  ULONG       fInEndMenu:1;
  ULONG       fUnderline:1;
  ULONG       fButtonAlwaysDown:1;
  ULONG       fDragging:1;
  ULONG       fModelessMenu:1;
  ULONG       fInCallHandleMenuMessages:1;
  ULONG       fDragAndDrop:1;
  ULONG       fAutoDismiss:1;
  ULONG       fAboutToAutoDismiss:1;
  ULONG       fIgnoreButtonUp:1;
  ULONG       fMouseOffMenu:1;
  ULONG       fInDoDragDrop:1;
  ULONG       fActiveNoForeground:1;
  ULONG       fNotifyByPos:1;
  ULONG       fSetCapture:1;
  ULONG       iAniDropDir:5;
  };
  POINT       ptMouseLast; 
  INT         mnFocus;
  INT         cmdLast;
  PTHREADINFO ptiMenuStateOwner;
  DWORD       dwLockCount;
  struct tagMENUSTATE* pmnsPrev;
  POINT       ptButtonDown;
  ULONG_PTR   uButtonDownHitArea;
  UINT        uButtonDownIndex;
  INT         vkButtonDown;
  ULONG_PTR   uDraggingHitArea;
  UINT        uDraggingIndex;
  UINT        uDraggingFlags;
  HDC         hdcWndAni;
  DWORD       dwAniStartTime;
  INT         ixAni;
  INT         iyAni;
  INT         cxAni;
  INT         cyAni;
  HBITMAP     hbmAni;
  HDC         hdcAni;
} MENUSTATE, *PMENUSTATE;

typedef struct _SETMENUITEMRECT
{
  UINT uItem;
  BOOL fByPosition;
  RECTL rcRect;
} SETMENUITEMRECT, *PSETMENUITEMRECT;


//
// Legacy ReactOS Menu transfer structures.
//
typedef struct tagROSMENUINFO
{
    /* ----------- MENUINFO ----------- */
    DWORD cbSize;
    DWORD fMask;
    DWORD dwStyle;
    UINT cyMax;
    HBRUSH  hbrBack;
    DWORD dwContextHelpID;
    ULONG_PTR dwMenuData;
    /* ----------- Extra ----------- */
    ULONG fFlags; /* Menu flags (MF_POPUP, MF_SYSMENU) */
    UINT iItem; /* Currently focused item */
    UINT cItems; /* Number of items in the menu */
    WORD cxMenu; /* Width of the whole menu */
    WORD cyMenu; /* Height of the whole menu */
    ULONG cxTextAlign;
    PWND spwndNotify; /* window receiving the messages for ownerdraw */
    INT iTop;
    INT iMaxTop;
    DWORD dwArrowsOn:2;

    HMENU Self; /* Handle of this menu */
    HWND Wnd; /* Window containing the menu */
    BOOL TimeToHide; /* Request hiding when receiving a second click in the top-level menu item */
} ROSMENUINFO, *PROSMENUINFO;

typedef struct tagROSMENUITEMINFO
{
    /* ----------- MENUITEMINFOW ----------- */
    UINT cbSize;
    UINT fMask;
    UINT fType;
    UINT fState;
    UINT wID;
    HMENU hSubMenu;
    HBITMAP hbmpChecked;
    HBITMAP hbmpUnchecked;
    DWORD dwItemData;
    LPWSTR dwTypeData;
    UINT cch;
    HBITMAP hbmpItem;
    /* ----------- Extra ----------- */
    RECT Rect; /* Item area (relative to menu window) */
    UINT dxTab; /* X position of text after Tab */
    LPWSTR lpstr; /* Copy of the text pointer in MenuItem->Text */
    SIZE maxBmpSize; /* Maximum size of the bitmap items in MIIM_BITMAP state */
} ROSMENUITEMINFO, *PROSMENUITEMINFO;
//
//
//

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
BOOL FASTCALL IntSetMenu(PWND Wnd,HMENU Menu,BOOL *Changed);
UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect, PWND pWnd, BOOL suppress_draw );
BOOL MenuInit(VOID);
VOID MENU_TrackKbdMenuBar(PWND pwnd, UINT wParam, WCHAR wChar);
VOID MENU_TrackMouseMenuBar( PWND pWnd, ULONG ht, POINT pt);
BOOL WINAPI PopupMenuWndProc(PWND Wnd,UINT Message,WPARAM wParam,LPARAM lParam,LRESULT *lResult);
BOOL FASTCALL IntSetMenuItemInfo(PMENU, PITEM, PROSMENUITEMINFO, PUNICODE_STRING);
PWND MENU_IsMenuActive(VOID);
void MENU_EndMenu( PWND pwnd );
void FASTCALL MENU_InitSysMenuPopup(PMENU menu, DWORD style, DWORD clsStyle, LONG HitTest );
INT FASTCALL IntMenuItemFromPoint(PWND pWnd, HMENU hMenu, POINT ptScreen);
BOOL WINAPI IntTrackPopupMenuEx( PMENU menu, UINT wFlags, int x, int y, PWND pWnd, LPTPMPARAMS lpTpm);
