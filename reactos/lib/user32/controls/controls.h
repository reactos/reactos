#ifndef _ROS_CONTROLS_H
#define _ROS_CONTROLS_H

/* Missing from Winuser.h */
#define ES_COMBO        0x00000200   /* Undocumented. Parent is a combobox */
#ifndef MAKEINTATOMA
#define MAKEINTATOMA(atom)  ((LPCSTR)((ULONG_PTR)((WORD)(atom))))
#endif

#ifndef WM_ISACTIVEICON 
#define WM_ISACTIVEICON         0x0035
#endif 
/* winuser.h */


// I dont know where this goes

#define LB_CARETON             0x01a3
#define LB_CARETOFF            0x01a4

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
} HEADCOMBO,*LPHEADCOMBO;

/* Note, that CBS_DROPDOWNLIST style is actually (CBS_SIMPLE | CBS_DROPDOWN) */
#define CB_GETTYPE( lphc )    ((lphc)->dwStyle & (CBS_DROPDOWNLIST))

extern BOOL COMBO_FlipListbox( LPHEADCOMBO, BOOL, BOOL );

#endif /* _ROS_CONTROLS_H */
