/*
 * Menu definitions
 */

#ifndef __WINE_MENU_H
#define __WINE_MENU_H

#include <user32/win.h>
#include <user32/queue.h>
#include <user32/nc.h>

#define PUT_WORD(ptr,w)   (*(WORD *)(ptr) = (w))
#define GET_WORD(ptr)     (*(WORD *)(ptr))
#define PUT_DWORD(ptr,dw) (*(DWORD *)(ptr) = (dw))
#define GET_DWORD(ptr)    (*(DWORD *)(ptr))


#define WM_NEXTMENU	    0x0213

#define MF_INSERT          0x0000
#define MF_CHANGE          0x0080
#define MF_APPEND          0x0100
#define MF_DELETE          0x0200
#define MF_REMOVE          0x1000
#define MF_END             0x0080

/* internal popup menu window messages */

#define MM_SETMENUHANDLE	(WM_USER + 0)
#define MM_GETMENUHANDLE	(WM_USER + 1)

extern WND* pTopPopupWnd;
extern UINT uSubPWndLevel;


/* Menu item structure */
typedef struct {
    /* ----------- MENUITEMINFO Stuff ----------- */
    UINT fType;		/* Item type. */
    UINT fState;		/* Item state.  */
    UINT wID;			/* Item id.  */
    HMENU hSubMenu;		/* Pop-up menu.  */
    HBITMAP hCheckBit;	/* Bitmap when checked.  */
    HBITMAP hUnCheckBit;	/* Bitmap when unchecked.  */
    LPWSTR text;			/* Item text or bitmap handle.  */
    DWORD dwItemData;		/* Application defined.  */
    /* ----------- Wine stuff ----------- */
    RECT      rect;          /* Item area (relative to menu window) */
    UINT      xTab;          /* X position of text after Tab */
} MENUITEM;

/* Popup menu structure */
typedef struct {
    WORD        wFlags;       /* Menu flags (MF_POPUP, MF_SYSMENU) */
    WORD        wMagic;       /* Magic number */
    HQUEUE    hTaskQ;         /* Task queue for this menu */
    WORD	Width;        /* Width of the whole menu */
    WORD	Height;       /* Height of the whole menu */
    WORD	nItems;       /* Number of items in the menu */
    HWND      hWnd;           /* Window containing the menu */
    MENUITEM   *items;        /* Array of menu items */
    UINT      FocusedItem;    /* Currently focused item */
    WORD      defitem;        /* default item position.  */
    DWORD     dwContextHelpId; /* Context help id */
} POPUPMENU, *LPPOPUPMENU;

/* internal flags for menu tracking */

#define TF_ENDMENU              0x0001
#define TF_SUSPENDPOPUP         0x0002
#define TF_SKIPREMOVE		0x0004

typedef struct
{
    UINT	trackFlags;
    HMENU	hCurrentMenu; /* current submenu (can be equal to hTopMenu)*/
    HMENU	hTopMenu;     /* initial menu */
    HWND	hOwnerWnd;    /* where notifications are sent */
    POINT	pt;
} MTRACKER;

#define MENU_MAGIC   0x554d  /* 'MU' */
#define IS_A_MENU(pmenu) ((pmenu) && (pmenu)->wMagic == MENU_MAGIC)

#define ITEM_PREV		-1
#define ITEM_NEXT		 1

  /* Internal MENU_TrackMenu() flags */
#define TPM_INTERNAL		0xF0000000
#define TPM_ENTERIDLEEX	 	0x80000000		/* set owner window for WM_ENTERIDLE */
#define TPM_BUTTONDOWN		0x40000000		/* menu was clicked before tracking */

  /* popup menu shade thickness */
#define POPUP_XSHADE		4
#define POPUP_YSHADE		4

  /* Space between 2 menu bar items */
#define MENU_BAR_ITEMS_SPACE 12

  /* Minimum width of a tab character */
#define MENU_TAB_SPACE 8

  /* Height of a separator item */
#define SEPARATOR_HEIGHT 5

  /* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#define IS_STRING_ITEM(flags) (MENU_ITEM_TYPE ((flags)) == MF_STRING)

#define IS_SYSTEM_MENU(menu)  \
	(!((menu)->wFlags & MF_POPUP) && (menu)->wFlags & MF_SYSMENU)
#define IS_SYSTEM_POPUP(menu) \
	((menu)->wFlags & MF_POPUP && (menu)->wFlags & MF_SYSMENU)

#define TYPE_MASK (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
		   MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
		   MFT_RIGHTORDER | MFT_RIGHTJUSTIFY | \
		   MF_POPUP | MF_SYSMENU | MF_HELP)
#define STATE_MASK (~TYPE_MASK)

extern  HMENU MENU_DefSysPopup;

WINBOOL MENU_Init(void);
HMENU MENU_GetSysMenu(HWND hWndOwner, HMENU hSysPopup);
UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                                     INT orgX, INT orgY );
BOOL MENU_PatchResidentPopup( HQUEUE, WND* );
void MENU_TrackMouseMenuBar( WND *wnd, INT ht, POINT pt );
void MENU_TrackKbdMenuBar( WND *wnd, UINT wParam, INT vkey );
UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect,
                                HWND hwnd, WINBOOL suppress_draw );
WINBOOL MENU_TrackMenu( HMENU hmenu, UINT wFlags, INT x, INT y,
                              HWND hwnd, const RECT *lprect );

WINBOOL MENU_ButtonUp( MTRACKER* pmt, HMENU hPtMenu );
WINBOOL MENU_MouseMove( MTRACKER* pmt, HMENU hPtMenu );
LRESULT MENU_DoNextMenu( MTRACKER* pmt, UINT vk );
void MENU_KeyLeft( MTRACKER* pmt );
void MENU_KeyRight( MTRACKER* pmt );
WINBOOL MENU_InitTracking(HWND hWnd, HMENU hMenu);
void MENU_TrackMouseMenuBar( WND* wndPtr, INT ht, POINT pt );

WINBOOL MENU_ShowPopup( HWND hwndOwner, HMENU hmenu, UINT id,
                              INT x, INT y, INT xanchor, INT yanchor );
void MENU_DrawPopupMenu( HWND hwnd, HDC hdc, HMENU hmenu );
UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect, HWND hwnd,
                         WINBOOL suppress_draw);
MENUITEM *MENU_FindItem( HMENU *hmenu, UINT *nPos, UINT wFlags );

void MENU_MenuBarCalcSize( HDC hdc, LPRECT lprect,
                                  LPPOPUPMENU lppop, HWND hwndOwner );

void MENU_CalcItemSize( HDC hdc, MENUITEM *lpitem, HWND hwndOwner,
			       INT orgX, INT orgY, WINBOOL menuBar );

void MENU_HideSubPopups( HWND hwndOwner, HMENU hmenu,
                                WINBOOL sendMenuSelect );

void MENU_SelectItem( HWND hwndOwner, HMENU hmenu, UINT wIndex,
                             WINBOOL sendMenuSelect );

MENUITEM *MENU_InsertItem( HMENU hMenu, UINT pos, UINT flags );

WINBOOL MENU_SetItemData( MENUITEM *item, UINT flags, UINT id,
                                LPCWSTR str );

void MENU_FreeItemData( MENUITEM* item );

HMENU MENU_CopySysPopup(void);


#endif /* __WINE_MENU_H */
