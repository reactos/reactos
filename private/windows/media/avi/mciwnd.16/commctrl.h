/* commctrl.h : Interfaces for the Windows Common Controls.
 * Copyright (C) Microsoft 1991-1992
 */

/*REVIEW: this stuff needs Windows style in many places; find all REVIEWs. */

#ifndef _INC_COMMCTRL
#define _INC_COMMCTRL

#ifdef __cplusplus
extern "C" {
#endif

/* Users of this header may define any number of these constants to avoid
 * the definitions of each functional group.
 *    NOTOOLBAR    Customizable bitmap-button toolbar control.
 *    NOUPDOWN     Up and Down arrow increment/decrement control.
 *    NOSTATUSBAR  Status bar and header bar controls.
 *    NOMENUHELP   APIs to help manage menus, especially with a status bar.
 *    NOTRACKBAR   Customizable column-width tracking control.
 *    NOBTNLIST    A control which is a list of bitmap buttons.
 *    NODRAGLIST   APIs to make a listbox source and sink drag&drop actions.
 *    NOPROGRESS   Progress gas gauge.
 */

/*/////////////////////////////////////////////////////////////////////////*/

/* InitCommonControls:
 * Any application requiring the use of any common control should call this
 * API upon application startup.  There is no required shutdown.
 */
void WINAPI InitCommonControls();

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOTOOLBAR

#define TOOLBARCLASSNAME "MCIWndToolbar"


/* Note that LOWORD(dwData) is at the same offset as idsHelp in the old
** structure, since it was never used anyway.
*/
typedef struct tagTBBUTTON
{
/*REVIEW: index, command, flag words, resource ids should be UINT */
    int iBitmap;	/* index into bitmap of this button's picture */
    int idCommand;	/* WM_COMMAND menu ID that this button sends */
    BYTE fsState;	/* button's state */
    BYTE fsStyle;	/* button's style */
    DWORD dwData;	/* app defined data */
    int iString;	/* index into string list */
} TBBUTTON;
typedef TBBUTTON NEAR* PTBBUTTON;
typedef TBBUTTON FAR* LPTBBUTTON;
typedef const TBBUTTON FAR* LPCTBBUTTON;


/*REVIEW: is this internal? if not, call it TBADJUSTINFO, prefix tba */
typedef struct tagADJUSTINFO
{
    TBBUTTON tbButton;
    char szDescription[1];
} ADJUSTINFO;
typedef ADJUSTINFO NEAR* PADJUSTINFO;
typedef ADJUSTINFO FAR* LPADJUSTINFO;


/*REVIEW: is this internal? if not, call it TBCOLORMAP, prefix tbc */
typedef struct tagCOLORMAP
{
    COLORREF from;
    COLORREF to;
} COLORMAP;
typedef COLORMAP NEAR* PCOLORMAP;
typedef COLORMAP FAR* LPCOLORMAP;


/* This is likely to change several times in the near future. */
HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, WORD wID, int nBitmaps,
			HINSTANCE hBMInst, WORD wBMID, LPCTBBUTTON lpButtons, 
			int iNumButtons, int dxButton, int dyButton, 
			int dxBitmap, int dyBitmap, UINT uStructSize);

/*REVIEW: idBitmap, iNumMaps should be UINT */
HBITMAP WINAPI CreateMappedBitmap(HINSTANCE hInstance, int idBitmap,
                                  WORD wFlags, LPCOLORMAP lpColorMap,
				  int iNumMaps);

#define CMB_DISCARDABLE	0x01	/* create bitmap as discardable */
#define CMB_MASKED	0x02	/* create image/mask pair in bitmap */

/*REVIEW: TBSTATE_* should be TBF_* (for Flags) */
#define TBSTATE_CHECKED		0x01	/* radio button is checked */
#define TBSTATE_PRESSED		0x02	/* button is being depressed (any style) */
#define TBSTATE_ENABLED		0x04	/* button is enabled */
#define TBSTATE_HIDDEN		0x08	/* button is hidden */
#define TBSTATE_INDETERMINATE	0x10	/* button is indeterminate */
                                        /*  (needs to be endabled, too) */

/*REVIEW: TBSTYLE_* should be TBS_* (for Style) */
#define TBSTYLE_BUTTON		0x00	/* this entry is button */
#define TBSTYLE_SEP		0x01	/* this entry is a separator */
#define TBSTYLE_CHECK		0x02	/* this is a check button (it stays down) */
#define TBSTYLE_GROUP		0x04	/* this is a check button (it stays down) */
#define TBSTYLE_CHECKGROUP	(TBSTYLE_GROUP | TBSTYLE_CHECK)	/* this group is a member of a group radio group */

/*REVIEW: ifdef _INC_WINDOWSX, should we provide message crackers? */

#define TB_ENABLEBUTTON	(WM_USER + 1)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, enable if nonzero; HIWORD not used, 0
	** return: not used
	*/

#define TB_CHECKBUTTON	(WM_USER + 2)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, check if nonzero; HIWORD not used, 0
	** return: not used
	*/

#define TB_PRESSBUTTON	(WM_USER + 3)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, press if nonzero; HIWORD not used, 0
	** return: not used
	*/

#define TB_HIDEBUTTON	(WM_USER + 4)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, hide if nonzero; HIWORD not used, 0
	** return: not used
	*/
#define TB_INDETERMINATE	(WM_USER + 5)
	/* wParam: UINT, button ID
	** lParam: BOOL LOWORD, make indeterminate if nonzero; HIWORD not used, 0
	** return: not used
	*/

/*REVIEW: Messages up to WM_USER+8 are reserved until we define more state bits */

#define TB_ISBUTTONENABLED	(WM_USER + 9)
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, enabled if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONCHECKED	(WM_USER + 10)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, checked if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONPRESSED	(WM_USER + 11)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, pressed if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONHIDDEN	(WM_USER + 12)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, hidden if nonzero; HIWORD not used
	*/

#define TB_ISBUTTONINDETERMINATE	(WM_USER + 13)	
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: BOOL LOWORD, indeterminate if nonzero; HIWORD not used
	*/

/*REVIEW: Messages up to WM_USER+16 are reserved until we define more state bits */

#define TB_SETSTATE             (WM_USER + 17)
	/* wParam: UINT, button ID
	** lParam: UINT LOWORD, state bits; HIWORD not used, 0
	** return: not used
	*/

#define TB_GETSTATE             (WM_USER + 18)
	/* wParam: UINT, button ID
	** lParam: not used, 0
	** return: UINT LOWORD, state bits; HIWORD not used
	*/

#define TB_ADDBITMAP		(WM_USER + 19)
	/* wParam: UINT, number of button graphics in bitmap
	** lParam: one of:
	**         HINSTANCE LOWORD, module handle; UINT HIWORD, resource id
	**         HINSTANCE LOWORD, NULL; HBITMAP HIWORD, bitmap handle
	** return: one of:
	**         int LOWORD, index for first new button; HIWORD not used
	**         int LOWORD, -1 indicating error; HIWORD not used
	*/

#define TB_ADDBUTTONS		(WM_USER + 20)
	/* wParam: UINT, number of buttons to add
	** lParam: LPTBBUTTON, pointer to array of TBBUTTON structures
	** return: not used
	*/

#define TB_INSERTBUTTON		(WM_USER + 21)
	/* wParam: UINT, index for insertion (appended if index doesn't exist)
	** lParam: LPTBBUTTON, pointer to one TBBUTTON structure
	** return: not used
	*/

#define TB_DELETEBUTTON		(WM_USER + 22)
	/* wParam: UINT, index of button to delete
	** lParam: not used, 0
	** return: not used
	*/

#define TB_GETBUTTON		(WM_USER + 23)
	/* wParam: UINT, index of button to get
	** lParam: LPTBBUTTON, pointer to TBBUTTON buffer to receive button
	** return: not used
	*/

#define TB_BUTTONCOUNT		(WM_USER + 24)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: UINT LOWORD, number of buttons; HIWORD not used
	*/

#define TB_COMMANDTOINDEX	(WM_USER + 25)
	/* wParam: UINT, command id
	** lParam: not used, 0
	** return: UINT LOWORD, index of button (-1 if command not found);
	**         HIWORD not used
	**/

#define TB_SAVERESTORE		(WM_USER + 26)
	/* wParam: BOOL, save state if nonzero (otherwise restore)
	** lParam: LPSTR FAR*, pointer to two LPSTRs:
	**         (LPSTR FAR*)(lParam)[0]: ini section name
	**         (LPSTR FAR*)(lParam)[1]: ini file name or NULL for WIN.INI
	** return: not used
	*/

#define TB_CUSTOMIZE            (WM_USER + 27)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: not used
	*/

#define TB_ADDSTRING		(WM_USER + 28)
	/* wParam: UINT, 0 if no resource; HINSTANCE, module handle
	** lParam: LPSTR, null-terminated strings with double-null at end
	**         UINT LOWORD, resource id
	** return: one of:
	**         int LOWORD, index for first new string; HIWORD not used
	**         int LOWORD, -1 indicating error; HIWORD not used
	*/

#define TB_GETITEMRECT		(WM_USER + 29)
	/* wParam: UINT, index of toolbar item whose rect to retrieve
	** lParam: LPRECT, pointer to a RECT struct to fill
	** return: Non-zero, if the RECT is successfully filled
	**         Zero, otherwise (item did not exist or was hidden)
	*/

#define TB_BUTTONSTRUCTSIZE	(WM_USER + 30)
	/* wParam: UINT, size of the TBBUTTON structure.  This is used
	**         as a version check.
	** lParam: not used
	** return: not used
	**
	** This is required before any buttons are added to the toolbar if
	** the toolbar is created using CreateWindow, but is implied when
	** using CreateToolbar and is a parameter to CreateToolbarEx.
	*/

#define TB_SETBUTTONSIZE	(WM_USER + 31)
	/* wParam: not used, 0
	** lParam: UINT LOWORD, button width
	**         UINT HIWORD, button height
	** return: not used
	**
	** The button size can only be set before any buttons are
	** added.  A default size of 24x22 is assumed if the size
	** is not set explicitly.
	*/

#define TB_SETBITMAPSIZE	(WM_USER + 32)
	/* wParam: not used, 0
	** lParam: UINT LOWORD, bitmap width
	**         UINT HIWORD, bitmap height
	** return: not used
	**
	** The bitmap size can only be set before any bitmaps are
	** added.  A default size of 16x15 is assumed if the size
	** is not set explicitly.
	*/

#define TB_AUTOSIZE		(WM_USER + 33)
	/* wParam: not used, 0
	** lParam: not used, 0
	** return: not used
	**
	** Application should call this after causing the toolbar size
	** to change by either setting the button or bitmap size or
	** by adding strings for the first time.
	*/

#define TB_SETBUTTONTYPE	(WM_USER + 34)
	/* wParam: WORD, frame control style of button (DFC_*)
	** lParam: not used, 0
	** return: not used
	*/

#endif /* NOTOOLBAR */

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOSTATUSBAR

/*REVIEW: Here exists the only known documentation for status bars. */

/* DrawStatusText:
 * This is used if the app wants to draw status in its client rect,
 * instead of just creating a window.  Note that this same function is
 * used internally in the status bar window's WM_PAINT message.
 * hDC is the DC to draw to.  The font that is selected into hDC will
 * be used.  The RECT lprc is the only portion of hDC that will be drawn
 * to: the outer edge of lprc will have the highlights (the area outside
 * of the highlights will not be drawn in the BUTTONFACE color: the app
 * must handle that).  The area inside the highlights will be erased
 * properly when drawing the text.
 */
/*REVIEW: szText should be LPCSTR */
void WINAPI DrawStatusText(HDC hDC, LPRECT lprc, LPSTR szText, UINT uFlags);

/* CreateStatusWindow:
 * CreateHeaderWindow:
 * These create a "default" status or header window.  This will have the
 * default borders around the text, the default font, and only one pane.
 * It may also automatically resize and move itself (depending on the SBS_*
 * flags).
 *
 * The style should contain WS_CHILD, and can contain WS_BORDER and
 * WS_VISIBLE, plus any of the SBS_* styles described below.  I don't know
 * about other WS_* styles.
 *
 * The lpszText is the initial text for the first pane.
 * hwndParent is the window the status bar exists in, and should not be NULL.
 * wID is the child window ID of the window.
 * hInstance is the instance handle of the application using this.
 * Note that the app can also just call CreateWindow with
 * STATUSCLASSNAME/HEADERCLASSNAME to create a window of a specific size.
 *
 * Note the user can change the font used by setting win.ini [Desktop]:
 *   StatusBarFaceName=Arial
 *   StatusBarFaceHeight=10
 */
/*REVIEW: style should be DWORD, lpszText should be LPCSTR */
HWND WINAPI CreateStatusWindow(LONG style, LPSTR lpszText,
      HWND hwndParent, WORD wID);
HWND WINAPI CreateHeaderWindow(LONG style, LPSTR lpszText,
      HWND hwndParent, WORD wID);

/*REVIEW: should be STATUSBAR_CLASS, HEADERBAR_CLASS */
#define STATUSCLASSNAME "msctls_statusbar"
/* This is the name of the status bar class (it will probably change later
 * so use the #define here).
 */
#define HEADERCLASSNAME "msctls_headerbar"
/* This is the name of the status bar class (it will probably change later
 * so use the #define here).
 */


#define SB_SETTEXT		(WM_USER+1)
#define SB_GETTEXT		(WM_USER+2)
#define SB_GETTEXTLENGTH	(WM_USER+3)
/* Just like WM_?ETTEXT*, with wParam specifying the pane that is referenced
 * (at most 255).
 * Note that you can use the WM_* versions to reference the 0th pane (this
 * is useful if you want to treat a "default" status bar like a static text
 * control).
 * For SETTEXT, wParam is the pane or'ed with SBT_* style bits (defined below).
 * If the text is "normal" (not OWNERDRAW), then a single pane may have left,
 * center, and right justified text by separating the parts with a single tab,
 * plus if lParam is NULL, then the pane has no text.  The pane will be
 * invalidated, but not draw until the next PAINT message.
 * For GETTEXT and GETTEXTLENGTH, the LOWORD of the return will be the length,
 * and the HIWORD will be the SBT_* style bits.
 */
#define SB_SETPARTS		(WM_USER+4)
/* wParam is the number of panes, and lParam points to an array of points
 * specifying the right hand side of each pane.  A right hand side of -1 means
 * it goes all the way to the right side of the control minus the X border
 */
#define SB_SETBORDERS		(WM_USER+5)
/* lParam points to an array of 3 integers: X border, Y border, between pane
 * border.  If any is less than 0, the default will be used for that one.
 */
#define SB_GETPARTS		(WM_USER+6)
/* lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define SB_GETBORDERS		(WM_USER+7)
/* lParam is a pointer to an array of 3 integers that will get filled in with
 * the X border, the Y border, and the between pane border.
 */
#define SB_SETMINHEIGHT		(WM_USER+8)
/* wParam is the minimum height of the status bar "drawing" area.  This is
 * the area inside the highlights.  This is most useful if a pane is used
 * for an OWNERDRAW item, and is ignored if the SBS_NORESIZE flag is set.
 * Note that WM_SIZE (wParam=0, lParam=0L) must be sent to the control for
 * any size changes to take effect.
 */
#define SB_SIMPLE		(WM_USER+9)
/* wParam specifies whether to set (non-zero) or unset (zero) the "simple"
 * mode of the status bar.  In simple mode, only one pane is displayed, and
 * its text is set with LOWORD(wParam)==255 in the SETTEXT message.
 * OWNERDRAW is not allowed, but other styles are.
 * The pane gets invalidated, but not painted until the next PAINT message,
 * so you can set new text without flicker (I hope).
 * This can be used with the WM_INITMENU and WM_MENUSELECT messages to
 * implement help text when scrolling through a menu.
 */


#define HB_SAVERESTORE		(WM_USER+256)
/* This gets a header bar to read or write its state to or from an ini file.
 * wParam is 0 for reading, non-zero for writing.  lParam is a pointer to
 * an array of two LPSTR's: the section and file respectively.
 * Note that the correct number of partitions must be set before calling this.
 */
#define HB_ADJUST		(WM_USER+257)
/* This puts the header bar into "adjust" mode, for changing column widths
 * with the keyboard.
 */
#define HB_SETWIDTHS		SB_SETPARTS
/* Set the widths of the header columns.  Note that "springy" columns only
 * have a minumum width, and negative width are assumed to be hidden columns.
 * This works just like SB_SETPARTS.
 */
#define HB_GETWIDTHS		SB_GETPARTS
/* Get the widths of the header columns.  Note that "springy" columns only
 * have a minumum width.  This works just like SB_GETPARTS.
 */
#define HB_GETPARTS		(WM_USER+258)
/* Get a list of the right-hand sides of the columns, for use when drawing the
 * actual columns for which this is a header.  
 * lParam is a pointer to an array of integers that will get filled in with
 * the right hand side of each pane and wParam is the size (in integers)
 * of the lParam array (so we do not go off the end of it).
 * Returns the number of panes.
 */
#define HB_SHOWTOGGLE		(WM_USER+259)
/* Toggle the hidden state of a column.  wParam is the 0-based index of the
 * column to toggle.
 */


#define SBT_OWNERDRAW	0x1000
/* The lParam of the SB_SETTEXT message will be returned in the DRAWITEMSTRUCT
 * of the WM_DRAWITEM message.  Note that the fields CtlType, itemAction, and
 * itemState of the DRAWITEMSTRUCT are undefined for a status bar.
 * The return value for GETTEXT will be the itemData.
 */
#define SBT_NOBORDERS	0x0100
/* No borders will be drawn for the pane.
 */
#define SBT_POPOUT	0x0200
/* The text pops out instead of in
 */
#define HBT_SPRING	0x0400
/* this means that the item is "springy", meaning that it has a minimum
 * width, but will grow if there is extra room in the window.  Note that
 * multiple springs are allowed, and the extra room will be distributed
 * among them.
 */

/* Here's a simple dialog function that uses a default status bar to display
 * the mouse position in the given window.
 *
 * extern HINSTANCE hInst;
 *
 * BOOL CALLBACK MyWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
 * {
 *   switch (msg)
 *     {
 *       case WM_INITDIALOG:
 *         CreateStatusWindow(WS_CHILD|WS_BORDER|WS_VISIBLE, "", hDlg,
 *               IDC_STATUS, hInst);
 *         break;
 *
 *       case WM_SIZE:  //REVIEW: simulating fake WM_SIZE may not be wise
 *         SendDlgItemMessage(hDlg, IDC_STATUS, WM_SIZE, 0, 0L);
 *         break;
 *
 *       case WM_MOUSEMOVE:
 *         wsprintf(szBuf, "%d,%d", LOWORD(lParam), HIWORD(lParam));
 *         SendDlgItemMessage(hDlg, IDC_STATUS, SB_SETTEXT, 0,
 *               (LPARAM)(LPSTR)szBuf);
 *         break;
 *
 *       default:
 *         break;
 *     }
 *   return(FALSE);
 * }
 */

#endif /* NOSTATUSBAR */

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOMENUHELP

/*REVIEW: iMessage should be UINT */
void WINAPI MenuHelp(WORD iMessage, WPARAM wParam, LPARAM lParam,
      HMENU hMainMenu, HINSTANCE hInst, HWND hwndStatus, LPWORD lpwIDs);

BOOL WINAPI ShowHideMenuCtl(HWND hWnd, UINT uFlags, LPINT lpInfo);

void WINAPI GetEffectiveClientRect(HWND hWnd, LPRECT lprc, LPINT lpInfo);

/*REVIEW: is this internal? */
#define MINSYSCOMMAND	SC_SIZE

#endif /* NOMENUHELP */

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOBTNLIST
/*
 *  BUTTON LISTBOX CONTROL
 *
 *  The Button Listbox control creates an array of buttons that behaves
 *  similar to both a button and a listbox: the array may be scrollable
 *  like a listbox and each listbox item is behaves like a pushbutton
 *  control
 *
 *
 *  SPECIFYING A BUTTONLISTBOX IN THE DIALOG TEMPLATE
 *
 *  The CONTROL statement in the dialog template specifies the
 *  dimensions of each individual button in the x, y, width and height
 *  parameters. The low order byte in the style field specifies the
 *  number of buttons that will be displayed; the actual size of the
 *  displayed control is determined by the number of buttons specified.
 *
 *  For a standard control--no other style bits set--the width of the
 *  control in dialog base units will be
 *      CX = cx * (n + 2/3) + 2
 *  where cx is the width of the button and n is number of buttons
 *  specified. (The 2/3 is for displaying partially visible buttons for
 *  scrolling plus 2 for the control borders.) The control will also be
 *  augmented in the cy direction by the height of the horizontal scroll
 *  bar.
 *
 *  If the BLS_NOSCROLL style is set, no scroll bar will appear and the
 *  button listbox will be limited to displaying the number of buttons
 *  specified and no more. In this case, the width of the control will
 *  be
 *      CX = cx * n + 2
 *
 *  If the BLS_VERTICAL style is set, the entire control goes vertical
 *  and cy should be substituted in the above calculations to determine
 *  CY, the actual height of the displayed control.
 *
 *  The statement
 *
 *  CONTROL  "", IDD_BUTTONLIST, "buttonlistbox", 0x0005 | WS_TABSTOP,
 *           4, 128, 34, 24
 *
 *  creates a scrollable horizontal list of 5 buttons at the position
 *  (4,128) with each button having dimensions (34,24). The entire control
 *  has the tabstop style.
 *
 *
 *  ADDING BUTTONS TO A BUTTONLISTBOX CONTROL
 *
 *  Buttons are added to the listbox in the same manner that items are
 *  added to a standard listbox; however, the messages BL_ADDBUTTON and
 *  BL_INSERTBUTTON must be passed a pointer to a CREATELISTBUTTON
 *  structure in the lParam.
 *
 *  Example:
 *
 *  {
 *      CREATELISTBUTTON clb;
 *      const int numColors = 1;
 *      COLORMAP colorMap;
 *
 *      colorMap.from = BUTTON_MAP_COLOR;   // your background color
 *      colorMap.to   = GetSysColor(COLOR_BTNFACE);
 *
 *      clb.cbSize = sizeof(clb);
 *      clb.dwItemData = BUTTON_1;
 *      clb.hBitmap = CreateMappedBitmap(hInst,BMP_BUTTON,FALSE,
 *                      &colorMap,numColors);
 *      clb.lpszText = "Button 1";
 *      SendMessage(GetDlgItem(hDlg,IDD_BUTTONLIST),
 *                  BL_ADDBUTTON, 0,
 *                  (LPARAM)(CREATELISTBUTTON FAR*)&clb);
 *      DeleteObject(clb.hBitmap);
 *  }
 *
 *  Note that the caller must delete any memory for objects passed in
 *  the CREATELISTBUTTON structure. Also, the CreateMappedBitmap API is
 *  useful for mapping the background color of the button bitmap to the
 *  system color COLOR_BTNFACE for a cleaner visual appearance.
 *
 *  The BL_ADDBUTTON message causes the listbox to be sorted by the
 *  button text whereas the BL_INSERTBUTTON does not cause the list to
 *  be sorted.
 *
 *  The button listbox sends a WM_DELETEITEM message to the control parent
 *  when a button is deleted so that any item data can be cleaned up.
 *
 */

/*REVIEW: should be BUTTONLIST_CLASS */
#define BUTTONLISTBOX           "ButtonListBox"

/* Button List Box Styles */
#define BLS_NUMBUTTONS      0x00FF
#define BLS_VERTICAL        0x0100
#define BLS_NOSCROLL        0x0200

/* Button List Box Messages */
#define BL_ADDBUTTON        (WM_USER+1)
#define BL_DELETEBUTTON     (WM_USER+2)
#define BL_GETCARETINDEX    (WM_USER+3)
#define BL_GETCOUNT         (WM_USER+4)
#define BL_GETCURSEL        (WM_USER+5)
#define BL_GETITEMDATA      (WM_USER+6)
#define BL_GETITEMRECT      (WM_USER+7)
#define BL_GETTEXT          (WM_USER+8)
#define BL_GETTEXTLEN       (WM_USER+9)
#define BL_GETTOPINDEX      (WM_USER+10)
#define BL_INSERTBUTTON     (WM_USER+11)
#define BL_RESETCONTENT     (WM_USER+12)
#define BL_SETCARETINDEX    (WM_USER+13)
#define BL_SETCURSEL        (WM_USER+14)
#define BL_SETITEMDATA      (WM_USER+15)
#define BL_SETTOPINDEX      (WM_USER+16)
#define BL_MSGMAX           (WM_USER+17) /* ;Internal */

/* Button listbox notification codes send in WM_COMMAND */
#define BLN_ERRSPACE        (-2)
#define BLN_SELCHANGE       1
#define BLN_CLICKED         2
#define BLN_SELCANCEL       3
#define BLN_SETFOCUS        4
#define BLN_KILLFOCUS       5

/* Message return values */
#define BL_OKAY             0
#define BL_ERR              (-1)
#define BL_ERRSPACE         (-2)

/* Create structure for
 * BL_ADDBUTTON and
 * BL_INSERTBUTTON
 *   lpCLB = (LPCREATELISTBUTTON)lParam
 */
typedef struct tagCREATELISTBUTTON
{
    UINT        cbSize;     /* size of structure */
    DWORD       dwItemData; /* user defined item data */
                            /* for LB_GETITEMDATA and LB_SETITEMDATA */
    HBITMAP     hBitmap;    /* button bitmap */
    LPCSTR      lpszText;   /* button text */

} CREATELISTBUTTON;
typedef CREATELISTBUTTON FAR* LPCREATELISTBUTTON;

#endif /* NOBTNLIST */

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOTRACKBAR
/*
    This control keeps its ranges in LONGs.  but for
    convienence and symetry with scrollbars
    WORD parameters are are used for some messages.
    if you need a range in LONGs don't use any messages 
    that pack values into loword/hiword pairs

    The trackbar messages:
    message         wParam  lParam  return

    TBM_GETPOS      ------  ------  Current logical position of trackbar.
    TBM_GETRANGEMIN ------  ------  Current logical minimum position allowed.
    TBM_GETRANGEMAX ------  ------  Current logical maximum position allowed.
    TBM_SETTIC
    TBM_SETPOS
    TBM_SETRANGEMIN
    TBM_SETRANGEMAX
*/

#define TRACKBAR_CLASS          "MCIWndTrackbar"

/* Trackbar styles */

/* add ticks automatically on TBM_SETRANGE message */
#define TBS_AUTOTICKS           0x0001L


/* Trackbar messages */

/* returns current position (LONG) */
#define TBM_GETPOS              (WM_USER)

/* set the min of the range to LPARAM */
#define TBM_GETRANGEMIN         (WM_USER+1)

/* set the max of the range to LPARAM */
#define TBM_GETRANGEMAX         (WM_USER+2)

/* wParam is index of tick to get (ticks are in the range of min - max) */
#define TBM_GETTIC              (WM_USER+3)

/* wParam is index of tick to set */
#define TBM_SETTIC              (WM_USER+4)

/* set the position to the value of lParam (wParam is the redraw flag) */
#define TBM_SETPOS              (WM_USER+5)

/* LOWORD(lParam) = min, HIWORD(lParam) = max, wParam == fRepaint */
#define TBM_SETRANGE            (WM_USER+6)

/* lParam is range min (use this to keep LONG precision on range) */
#define TBM_SETRANGEMIN         (WM_USER+7)

/* lParam is range max (use this to keep LONG precision on range) */
#define TBM_SETRANGEMAX         (WM_USER+8)

/* remove the ticks */
#define TBM_CLEARTICS           (WM_USER+9)

/* select a range LOWORD(lParam) min, HIWORD(lParam) max */
#define TBM_SETSEL              (WM_USER+10)

/* set selection rang (LONG form) */
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)

// #define TBM_SETTICTOK           (WM_USER+13)

/* return a pointer to the list of tics (DWORDS) */
#define TBM_GETPTICS            (WM_USER+14)

/* get the pixel position of a given tick */
#define TBM_GETTICPOS           (WM_USER+15)
/* get the number of tics */
#define TBM_GETNUMTICS          (WM_USER+16)

/* get the selection range */
#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND  	        (WM_USER+18)

/* clear the selection */
#define TBM_CLEARSEL  	        (WM_USER+19)

/*REVIEW: these match the SB_ (scroll bar messages); define them that way? */

#define TB_LINEUP		0
#define TB_LINEDOWN		1
#define TB_PAGEUP		2
#define TB_PAGEDOWN		3
#define TB_THUMBPOSITION	4
#define TB_THUMBTRACK		5
#define TB_TOP			6
#define TB_BOTTOM		7
#define TB_ENDTRACK             8
#endif

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NODRAGLIST

typedef struct
  {
    UINT uNotification;
    HWND hWnd;
    POINT ptCursor;
  } DRAGLISTINFO, FAR *LPDRAGLISTINFO;

#define DL_BEGINDRAG	(LB_MSGMAX+100)
#define DL_DRAGGING	(LB_MSGMAX+101)
#define DL_DROPPED	(LB_MSGMAX+102)
#define DL_CANCELDRAG	(LB_MSGMAX+103)

#define DL_CURSORSET	0
#define DL_STOPCURSOR	1
#define DL_COPYCURSOR	2
#define DL_MOVECURSOR	3

#define DRAGLISTMSGSTRING "commctrl_DragListMsg"

BOOL WINAPI MakeDragList(HWND hLB);
int WINAPI LBItemFromPt(HWND hLB, POINT pt, BOOL bAutoScroll);
void WINAPI DrawInsert(HWND handParent, HWND hLB, int nItem);

#endif /* NODRAGLIST */

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOUPDOWN

/*
// OVERVIEW:
//
// The UpDown control is a simple pair of buttons which increment or
// decrement an integer value.  The operation is similar to a vertical
// scrollbar; except that the control only has line-up and line-down
// functionality, and changes the current position automatically.
//
// The control also can be linked with a companion control, usually an
// "edit" control, to simplify dialog-box management.  This companion is
// termed a "buddy" in this documentation.  Any sibling HWND may be
// assigned as the control's buddy, or the control may be allowed to
// choose one automatically.  Once chosen, the UpDown can size itself to
// match the buddy's right or left border, and/or automatically set the
// text of the buddy control to make the current position visible.
//
// ADDITIONAL NOTES:
//
// The "upper" and "lower" limits must not cover a range larger than 32,767
// positions.  It is acceptable to have the range inverted, i.e., to have
// (lower > upper).  The upper button always moves the current position
// towards the "upper" number, and the lower button always moves towards the
// "lower" number.  If the range is zero (lower == upper), or the control
// is disabled (EnableWindow(hCtrl, FALSE)), the control draws grayed
// arrows in both buttons.  The UDS_WRAP style makes the range cyclic; that
// is, the numbers will wrap once one end of the range is reached.
//
// The buddy window must have the same parent as the UpDown control.
//
// If the buddy window resizes, and the UDS_ALIGN* styles are used, it
// is necessary to send the UDM_SETBUDDY message to re-anchor the UpDown
// control on the appropriate border of the buddy window.
//
// The UDS_AUTOBUDDY style uses GetWindow(hCtrl, GW_HWNDPREV) to pick
// the best buddy window.  In the case of a DIALOG resource, this will
// choose the previous control listed in the resource script.  If the
// windows will change in Z-order, sending UDM_SETBUDDY with a NULL handle
// will pick a new buddy; otherwise the original auto-buddy choice is
// maintained.
//
// The UDS_SETBUDDYINT style uses its own SetDlgItemInt-style
// functionality to set the caption text of the buddy.  All WIN.INI [Intl]
// values are honored by this routine.
//
// The UDS_ARROWKEYS style will subclass the buddy window, in order to steal
// the VK_UP and VK_DOWN arrow key messages.
*/

/*/////////////////////////////////////////////////////////////////////////*/

/* Structures */

typedef struct tagUDACCEL
{
	UINT nSec;
	UINT nInc;
} UDACCEL, FAR *LPUDACCEL;

#define UD_MAXVAL	0x7fff
#define UD_MINVAL	(-UD_MAXVAL)


/* STYLE BITS */

#define UDS_WRAP		0x0001
#define UDS_SETBUDDYINT		0x0002
#define UDS_ALIGNRIGHT		0x0004
#define UDS_ALIGNLEFT		0x0008
#define UDS_AUTOBUDDY		0x0010
#define UDS_ARROWKEYS		0x0020


/* MESSAGES */

#define UDM_SETRANGE		(WM_USER+101)
	/* wParam: not used, 0
	// lParam: short LOWORD, new upper; short HIWORD, new lower limit
	// return: not used
	*/

#define UDM_GETRANGE		(WM_USER+102)
	/* wParam: not used, 0
	// lParam: not used, 0
	// return: short LOWORD, upper; short HIWORD, lower limit
	*/

#define UDM_SETPOS		(WM_USER+103)
	/* wParam: not used, 0
	// lParam: short LOWORD, new pos; HIWORD not used, 0
	// return: short LOWORD, old pos; HIWORD not used
	*/

#define UDM_GETPOS		(WM_USER+104)
	/* wParam: not used, 0
	// lParam: not used, 0
	// return: short LOWORD, current pos; HIWORD not used
	*/

#define UDM_SETBUDDY		(WM_USER+105)
	/* wParam: HWND, new buddy
	// lParam: not used, 0
	// return: HWND LOWORD, old buddy; HIWORD not used
	*/

#define UDM_GETBUDDY		(WM_USER+106)
	/* wParam: not used, 0
	// lParam: not used, 0
	// return: HWND LOWORD, current buddy; HIWORD not used
	*/

#define UDM_SETACCEL		(WM_USER+107)
	/* wParam: UINT, number of acceleration steps
	// lParam: LPUDACCEL, pointer to array of UDACCEL elements
	//         Elements should be sorted in increasing nSec order.
	// return: BOOL LOWORD, nonzero if successful; HIWORD not used
	*/

#define UDM_GETACCEL		(WM_USER+108)
	/* wParam: UINT, number of elements in the UDACCEL array
	// lParam: LPUDACCEL, pointer to UDACCEL buffer to receive array
	// return: UINT LOWORD, number of elements returned in buffer
	*/

#define UDM_SETBASE		(WM_USER+109)
	/* wParam: UINT, new radix base (10 for decimal, 16 for hex, etc.)
	// lParam: not used, 0
	// return: not used
	*/
#define UDM_GETBASE		(WM_USER+110)
	/* wParam: not used, 0
	// lParam: not used, 0
	// return: UINT LOWORD, current radix base; HIWORD not used
	*/

/* NOTIFICATIONS */

/* WM_VSCROLL
// Note that unlike a scrollbar, the position is automatically changed by
// the control, and the LOWORD(lParam) is always the new position.  Only
// SB_THUMBTRACK and SB_THUMBPOSITION scroll codes are sent in the wParam.
*/

/* HELPER APIs */

#define UPDOWN_CLASS "msctls_updown"
HWND WINAPI CreateUpDownControl(DWORD dwStyle, int x, int y, int cx, int cy,
                                HWND hParent, int nID, HINSTANCE hInst,
                                HWND hBuddy,
				int nUpper, int nLower, int nPos);
	/* Does the CreateWindow call followed by setting the various
	// state information:
	//	hBuddy	The companion control (usually an "edit").
	//	nUpper	The range limit corresponding to the upper button.
	//	nLower	The range limit corresponding to the lower button.
	//	nPos	The initial position.
	// Returns the handle to the control or NULL on failure.
	*/

#endif /* NOUPDOWN */

/*/////////////////////////////////////////////////////////////////////////*/

#ifndef NOPROGRESS

/*
// OVERVIEW:
//
// The progress bar control is a "gas gauge" that can be used to show the
// progress of a lengthy operation.
//
// The application sets the range and current position (similar to a
// scrollbar) and has the ability to advance the current position in
// a variety of ways.
//
// Text can be displayed in the progress bar as either a percentage
// of the entire range (using the PBS_SHOWPERCENT style) or as the 
// value of the current position (using the PBS_SHOWPOS style).  If
// neither bit is set, no text is shown in the bar.
//
// When PBM_STEPIT is used to advance the current position, the gauge
// will wrap when it reaches the end and start again at the start.
// The position is clamped at either end in other cases.
//
*/

/*/////////////////////////////////////////////////////////////////////////*/

/* STYLE BITS */

#define PBS_SHOWPERCENT		0x01
#define PBS_SHOWPOS		0x02

/* MESSAGES */

#define PBM_SETRANGE         (WM_USER+1)
	/* wParam: not used, 0
	// lParam: int LOWORD, bottom of range; int HIWORD top of range
	// return: int LOWORD, previous bottom; int HIWORD old top
	*/
#define PBM_SETPOS           (WM_USER+2)
	/* wParam: int new position
	// lParam: not used, 0
	// return: int LOWORD, previous position; HIWORD not used
	*/
#define PBM_DELTAPOS         (WM_USER+3)
	/* wParam: int amount to advance current position
	// lParam: not used, 0
	// return: int LOWORD, previous position; HIWORD not used
	*/
#define PBM_SETSTEP          (WM_USER+4)
	/* wParam: int new step
	// lParam: not used, 0
	// return: int LOWORD, previous step; HIWORD not used
	*/
#define PBM_STEPIT	     (WM_USER+5)
        /* advance current position by current step
	// wParam: not used 0
	// lParam: not used, 0
	// return: int LOWORD, previous position; HIWORD not used
	*/

#define PROGRESS_CLASS "msctls_progress"

#endif /* NOPROGRESS */

/*/////////////////////////////////////////////////////////////////////////*/

/*REVIEW: move these to their appropriate control sections. */

/* Note that the set of HBN_* and TBN_* defines must be a disjoint set so
 * that MenuHelp can tell them apart.
 */

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the mouse or keyboard.
 */
#define HBN_BEGINDRAG	0x0101
#define HBN_DRAGGING	0x0102
#define HBN_ENDDRAG	0x0103

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * header bar when the user adjusts the headers with the keyboard.
 */
#define HBN_BEGINADJUST	0x0111
#define HBN_ENDADJUST	0x0112

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * tool bar.  If the left button is pressed and then released in a single
 * "button" of a tool bar, then a WM_COMMAND message will be sent with wParam
 * being the id of the button.
 */
#define TBN_BEGINDRAG	0x0201
#define TBN_ENDDRAG	0x0203

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * tool bar.  The TBN_BEGINADJUST message is sent before the "insert"
 * dialog appears.  The app must return a handle (which will
 * NOT be freed by the toolbar) to an ADJUSTINFO struct for the TBN_ADJUSTINFO
 * message; the LOWORD of lParam is the index of the button whose info should
 * be retrieved.  The app can clean up in the TBN_ENDADJUST message.
 * The app should reset the toolbar on the TBN_RESET message.
 */
#define TBN_BEGINADJUST	0x0204
#define TBN_ADJUSTINFO	0x0205
#define TBN_ENDADJUST	0x0206
#define TBN_RESET	0x0207

/* These are in the HIWORD of lParam in WM_COMMAND messages sent from a
 * tool bar.  The LOWORD is the index where the button is or will be.
 * If the app returns FALSE from either of these during a button move, then
 * the button will not be moved.  If the app returns FALSE to the INSERT
 * when the toolbar tries to add buttons, then the insert dialog will not
 * come up.  TBN_TOOLBARCHANGE is sent whenever any button is added, moved,
 * or deleted from the toolbar by the user, so the app can do stuff.
 */
#define TBN_QUERYINSERT	0x0208
#define TBN_QUERYDELETE	0x0209
#define TBN_TOOLBARCHANGE	0x020a

/* This is in the HIWORD of lParam in a WM_COMMAND message.  It notifies the
 * parent of a toolbar that the HELP button was pressed in the toolbar
 * customize dialog.  The dialog window handle is in the LOWORD of lParam.
 */
#define TBN_CUSTHELP	0x020b

/* Note that the following flags are checked every time the window gets a
 * WM_SIZE message, so the style of the window can be changed "on-the-fly".
 * If NORESIZE is set, then the app is responsible for all control placement
 * and sizing.  If NOPARENTALIGN is set, then the app is responsible for
 * placement.  If neither is set, the app just needs to send a WM_SIZE
 * message for the window to be positioned and sized correctly whenever the
 * parent window size changes.
 * Note that for STATUS bars, CCS_BOTTOM is the default, for HEADER bars,
 * CCS_NOMOVEY is the default, and for TOOL bars, CCS_TOP is the default.
 */
#define CCS_TOP			0x00000001L
/* This flag means the status bar should be "top" aligned.  If the
 * NOPARENTALIGN flag is set, then the control keeps the same top, left, and
 * width measurements, but the height is adjusted to the default, otherwise
 * the status bar is positioned at the top of the parent window such that
 * its client area is as wide as the parent window and its client origin is
 * the same as its parent.
 * Similarly, if this flag is not set, the control is bottom-aligned, either
 * with its original rect or its parent rect, depending on the NOPARENTALIGN
 * flag.
 */
#define CCS_NOMOVEY		0x00000002L
/* This flag means the control may be resized and moved horizontally (if the
 * CCS_NORESIZE flag is not set), but it will not move vertically when a
 * WM_SIZE message comes through.
 */
#define CCS_BOTTOM		0x00000003L
/* Same as CCS_TOP, only on the bottom.
 */
#define CCS_NORESIZE		0x00000004L
/* This flag means that the size given when creating or resizing is exact,
 * and the control should not resize itself to the default height or width
 */
#define CCS_NOPARENTALIGN	0x00000008L
/* This flag means that the control should not "snap" to the top or bottom
 * or the parent window, but should keep the same placement it was given
 */
#define CCS_NOHILITE		0x00000010L
/* Don't draw the one pixel highlight at the top of the control
 */
#define CCS_ADJUSTABLE		0x00000020L
/* This allows a toolbar (header bar?) to be configured by the user.
 */
#define CCS_NODIVIDER		0x00000040L
/* Don't draw the 2 pixel highlight at top of control (toolbar)
 */

/*/////////////////////////////////////////////////////////////////////////*/

#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif

#endif /* _INC_COMMCTRL */
