#pragma once

#ifndef HBMMENU_CALLBACK
#define HBMMENU_CALLBACK	((HBITMAP) -1)
#endif
#ifndef HBMMENU_SYSTEM
#define HBMMENU_SYSTEM		((HBITMAP)  1)
#endif
#ifndef HBMMENU_MBAR_RESTORE
#define HBMMENU_MBAR_RESTORE	((HBITMAP)  2)
#endif
#ifndef HBMMENU_MBAR_MINIMIZE
#define HBMMENU_MBAR_MINIMIZE	((HBITMAP)  3)
#endif
#ifndef HBMMENU_MBAR_CLOSE
#define HBMMENU_MBAR_CLOSE	((HBITMAP)  5)
#endif
#ifndef HBMMENU_MBAR_CLOSE_D
#define HBMMENU_MBAR_CLOSE_D	((HBITMAP)  6)
#endif
#ifndef HBMMENU_MBAR_MINIMIZE_D
#define HBMMENU_MBAR_MINIMIZE_D	((HBITMAP)  7)
#endif
#ifndef HBMMENU_POPUP_CLOSE
#define HBMMENU_POPUP_CLOSE	((HBITMAP)  8)
#endif
#ifndef HBMMENU_POPUP_RESTORE
#define HBMMENU_POPUP_RESTORE	((HBITMAP)  9)
#endif
#ifndef HBMMENU_POPUP_MAXIMIZE
#define HBMMENU_POPUP_MAXIMIZE	((HBITMAP) 10)
#endif
#ifndef HBMMENU_POPUP_MINIMIZE
#define HBMMENU_POPUP_MINIMIZE	((HBITMAP) 11)
#endif

/* combo box */
#define ID_CB_LISTBOX           1000
#define ID_CB_EDIT              1001

/* Combo box message return values */
#define CB_OKAY             0

/* internal flags */
#define CBF_DROPPED             0x0001
#define CBF_BUTTONDOWN          0x0002
#define CBF_NOROLLUP            0x0004
#define CBF_MEASUREITEM         0x0008
#define CBF_FOCUSED             0x0010
#define CBF_CAPTURE             0x0020
#define CBF_EDIT                0x0040
#define CBF_NORESIZE            0x0080
#define CBF_NOTIFY              0x0100
#define CBF_NOREDRAW            0x0200
#define CBF_SELCHANGE           0x0400
#define CBF_NOEDITNOTIFY        0x1000
#define CBF_NOLBSELECT          0x2000  /* do not change current selection */
#define CBF_BEENFOCUSED         0x4000  /* has it ever had focus           */
#define CBF_EUI                 0x8000

/* combo state struct */
typedef struct
{
   HWND           self;
   HWND           owner;
   UINT           dwStyle;
   HWND           hWndEdit;
   HWND           hWndLBox;
   UINT           wState;
   HFONT          hFont;
   RECT           textRect;
   RECT           buttonRect;
   RECT           droppedRect;
   INT            droppedIndex;
   INT            fixedOwnerDrawHeight;
   INT            droppedWidth;   /* last two are not used unless set */
   INT            editHeight;     /* explicitly */
   LONG           UIState;
} HEADCOMBO,*LPHEADCOMBO;

/* Note, that CBS_DROPDOWNLIST style is actually (CBS_SIMPLE | CBS_DROPDOWN) */
#define CB_GETTYPE( lphc )    ((lphc)->dwStyle & (CBS_DROPDOWNLIST))

extern BOOL COMBO_FlipListbox( LPHEADCOMBO, BOOL, BOOL );

#define LB_INSERTSTRING_UPPER     0x1AA
#define LB_INSERTSTRING_LOWER     0x1AB
#define LB_ADDSTRING_UPPER        0x1AC
#define LB_ADDSTRING_LOWER        0x1AD

HRGN set_control_clipping( HDC hdc, const RECT *rect );

LRESULT WINAPI DesktopWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI DesktopWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI User32DefWindowProc(HWND,UINT,WPARAM,LPARAM,BOOL);
BOOL WINAPI RegisterClientPFN(VOID);
LRESULT WINAPI IconTitleWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ButtonWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ButtonWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ButtonWndProc_common(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode);
LRESULT WINAPI ComboWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ComboWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ComboWndProc_common( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL unicode);
LRESULT WINAPI EditWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI EditWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI EditWndProc_common( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode);
LRESULT WINAPI ListBoxWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ListBoxWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI ListBoxWndProc_common( HWND hwnd, UINT msg,WPARAM wParam, LPARAM lParam, BOOL unicode);
LRESULT WINAPI MDIClientWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI MDIClientWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI MDIClientWndProc_common( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL unicode);
LRESULT WINAPI PopupMenuWndProcA(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI PopupMenuWndProcW(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI ScrollBarWndProcW( HWND hwnd, UINT uMsg, WPARAM wParam,LPARAM lParam );
LRESULT WINAPI ScrollBarWndProcA( HWND hwnd, UINT uMsg, WPARAM wParam,LPARAM lParam );
LRESULT WINAPI StaticWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI StaticWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI StaticWndProc_common( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode);
LRESULT WINAPI SwitchWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI SwitchWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
