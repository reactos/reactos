/*
 * Combo box definitions
 */

#ifndef __WINE_COMBO_H
#define __WINE_COMBO_H

#define ID_CB_LISTBOX           1000
#define ID_CB_EDIT              1001

/* Internal flags */

#define CBF_DROPPED             0x0001
#define CBF_BUTTONDOWN          0x0002
#define CBF_NOROLLUP            0x0004
#define CBF_MEASUREITEM		0x0008
#define CBF_FOCUSED             0x0010
#define CBF_CAPTURE             0x0020
#define CBF_EDIT                0x0040
#define CBF_NORESIZE		0x0080
#define CBF_NOTIFY		0x0100
#define CBF_NOREDRAW            0x0200
#define CBF_SELCHANGE		0x0400
#define CBF_EUI                 0x8000


#define LB_CARETON             0x01a3
#define LB_CARETOFF            0x01a4


/* Combo state struct */

typedef struct
{
   WND*    	self;
   HWND  	owner;
   UINT  	dwStyle;
   HWND  	hWndEdit;
   HWND  	hWndLBox;
   UINT  	wState;
   HFONT 	hFont;
   RECT  	RectCombo;
   RECT  	RectEdit;
   RECT  	RectButton;
   INT   	droppedWidth;		/* last two are not used unless set */
   INT   	editHeight;		/* explicitly */
} HEADCOMBO,*LPHEADCOMBO;

typedef DELETEITEMSTRUCT* LPDELETEITEMSTRUCT;

typedef COMPAREITEMSTRUCT* LPCOMPAREITEMSTRUCT;

/* Combo box message return values */
#define CB_OKAY             0
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)


/*
 * Note, that CBS_DROPDOWNLIST style is actually (CBS_SIMPLE | CBS_DROPDOWN)!
 */

#define CB_GETTYPE( lphc )    ((lphc)->dwStyle & (CBS_DROPDOWNLIST))
#define CB_DISABLED( lphc )   ((lphc)->self->dwStyle & WS_DISABLED)
#define CB_OWNERDRAWN( lphc ) ((lphc)->dwStyle & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
#define CB_HASSTRINGS( lphc ) ((lphc)->dwStyle & CBS_HASSTRINGS)
#define CB_HWND( lphc )       ((lphc)->self->hwndSelf)

WINBOOL 	COMBO_FlipListbox( LPHEADCOMBO, WINBOOL );
HWND 	COMBO_GetLBWindow( WND* );
LRESULT COMBO_Directory( LPHEADCOMBO, UINT, LPSTR, WINBOOL );

#endif /* __WINE_COMBO_H */

