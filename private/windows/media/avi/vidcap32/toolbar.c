/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   toolbar.c: Toolbar control window
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

#include <string.h>

#include <windows.h>
#include <windowsx.h>
//#include <win32.h>
#include "toolbar.h"		// use this for generic app
/************************************************************************/

/* work for win3.0 */
#ifndef COLOR_BTNHIGHLIGHT
#define COLOR_BTNHIGHLIGHT 20
#endif

TCHAR    szToolBarClass[] = "ToolBarClass";
HBRUSH	ghbrToolbar;		// brush for toolbar background

//
// Window proc for buttons, THIS FUNCTION MUST BE EXPORTED
//
LRESULT FAR PASCAL toolbarWndProc(HWND, unsigned, WPARAM, LPARAM);

typedef long (FAR PASCAL *LPWNDPROC)();

/*
	Defines
*/

#ifdef _WIN32

#define GETARRAYBUTT(hwnd)	((HANDLE)GetWindowLongPtr(hwnd,GWLP_ARRAYBUTT))
#define GETNUMBUTTONS(hwnd)	((int)GetWindowLong(hwnd,GWL_NUMBUTTONS))
#define GETPRESSED(hwnd)	((BOOL)GetWindowLong(hwnd,GWL_PRESSED))
#define GETKEYPRESSED(hwnd)	((BOOL)GetWindowLong(hwnd,GWL_KEYPRESSED))
#define GETWHICH(hwnd)		((int)GetWindowLong(hwnd,GWL_WHICH))
#define GETSHIFTED(hwnd)	((BOOL)GetWindowLong(hwnd,GWL_SHIFTED))
#define GETBMPHANDLE(hwnd)	((HANDLE)GetWindowLongPtr(hwnd,GWLP_BMPHANDLE))
#define GETBMPINT(hwnd)		((int)GetWindowLong(hwnd,GWL_BMPINT))
#define GETBUTTONSIZE(hwnd)	GetWindowLong(hwnd,GWL_BUTTONSIZE)
#define GETHINST(hwnd)		((HANDLE)GetWindowLongPtr(hwnd,GWLP_HINST))


#define SETARRAYBUTT(hwnd, h) SetWindowLongPtr(hwnd, GWLP_ARRAYBUTT, (UINT_PTR)h)
#define SETNUMBUTTONS(hwnd, wNumButtons) \
			SetWindowLong(hwnd, GWL_NUMBUTTONS, wNumButtons)
#define SETPRESSED(hwnd, f)	SetWindowLong(hwnd, GWL_PRESSED, (UINT)f)
#define SETKEYPRESSED(hwnd, f)	SetWindowLong(hwnd, GWL_KEYPRESSED, (UINT)f)
#define SETWHICH(hwnd, i)	SetWindowLong(hwnd, GWL_WHICH, (UINT)i)
#define SETSHIFTED(hwnd, i)	SetWindowLong(hwnd, GWL_SHIFTED, (UINT)i)
#define SETBMPHANDLE(hwnd, h)	SetWindowLongPtr(hwnd, GWLP_BMPHANDLE, (UINT_PTR)h)
#define SETBMPINT(hwnd, i)	SetWindowLong(hwnd, GWL_BMPINT, (UINT)i)
#define SETBUTTONSIZE(hwnd, l)	SetWindowLong(hwnd, GWL_BUTTONSIZE, l)
#define SETHINST(hwnd, h)	SetWindowLongPtr(hwnd, GWLP_HINST, (UINT_PTR)h)

#else

#define GETARRAYBUTT(hwnd)	((HANDLE)GetWindowWord(hwnd,GWW_ARRAYBUTT))
#define GETNUMBUTTONS(hwnd)	((int)GetWindowWord(hwnd,GWW_NUMBUTTONS))
#define GETPRESSED(hwnd)	((BOOL)GetWindowWord(hwnd,GWW_PRESSED))
#define GETKEYPRESSED(hwnd)	((BOOL)GetWindowWord(hwnd,GWW_KEYPRESSED))
#define GETWHICH(hwnd)		((int)GetWindowWord(hwnd,GWW_WHICH))
#define GETSHIFTED(hwnd)	((BOOL)GetWindowWord(hwnd,GWW_SHIFTED))
#define GETBMPHANDLE(hwnd)	((HANDLE)GetWindowWord(hwnd,GWW_BMPHANDLE))
#define GETBMPINT(hwnd)		((int)GetWindowWord(hwnd,GWW_BMPINT))
#define GETBUTTONSIZE(hwnd)	GetWindowLong(hwnd,GWL_BUTTONSIZE)
#define GETHINST(hwnd)		((HANDLE)GetWindowWord(hwnd,GWW_HINST))


#define SETARRAYBUTT(hwnd, h) SetWindowWord(hwnd, GWW_ARRAYBUTT, (WORD)h)
#define SETNUMBUTTONS(hwnd, wNumButtons) \
			SetWindowWord(hwnd, GWW_NUMBUTTONS, wNumButtons)
#define SETPRESSED(hwnd, f)	SetWindowWord(hwnd, GWW_PRESSED, (WORD)f)
#define SETKEYPRESSED(hwnd, f)	SetWindowWord(hwnd, GWW_KEYPRESSED, (WORD)f)
#define SETWHICH(hwnd, i)	SetWindowWord(hwnd, GWW_WHICH, (WORD)i)
#define SETSHIFTED(hwnd, i)	SetWindowWord(hwnd, GWW_SHIFTED, (WORD)i)
#define SETBMPHANDLE(hwnd, h)	SetWindowWord(hwnd, GWW_BMPHANDLE, (WORD)h)
#define SETBMPINT(hwnd, i)	SetWindowWord(hwnd, GWW_BMPINT, (WORD)i)
#define SETBUTTONSIZE(hwnd, l)	SetWindowLong(hwnd, GWL_BUTTONSIZE, l)
#define SETHINST(hwnd, h)	SetWindowWord(hwnd, GWW_HINST, (WORD)h)

#endif

#define lpCreate ((LPCREATESTRUCT)lParam)

/* Prototypes */

static void NEAR PASCAL NotifyParent(HWND, int);



/**************************************************************************
	toolbarInit( hInst, hPrev )

	Call this routine to initialize the toolbar code.

	Arguments:
		hPrev	instance handle of previous instance
		hInst	instance handle of current instance

	Returns:
		TRUE if successful, FALSE if not
***************************************************************************/

BOOL FAR PASCAL toolbarInit(HANDLE hInst, HANDLE hPrev)
{
	WNDCLASS	cls;
	
	/* Register the tool bar window class */
	if (!hPrev) {

	    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
	    cls.hIcon          = NULL;
	    cls.lpszMenuName   = NULL;
	    cls.lpszClassName  = (LPSTR)szToolBarClass;
	    cls.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
	    cls.hInstance      = hInst;
	    cls.style          = CS_DBLCLKS;
	    cls.lpfnWndProc    = toolbarWndProc;
	    cls.cbClsExtra     = 0;
	    cls.cbWndExtra     = TOOLBAR_EXTRABYTES;
	    if (!RegisterClass(&cls))
		return FALSE;
	}

	return TRUE;
}


/***************************************************************************/
/* toolbarSetBitmap:  takes a resource ID and associates that bitmap with  */
/*                    a given toolbar.  Also takes the instance handle and */
/*                    the size of the buttons on the toolbar.              */
/***************************************************************************/
BOOL FAR PASCAL toolbarSetBitmap(HWND hwnd, HANDLE hInst, int ibmp, POINT ptSize)
{
	SETHINST(hwnd, hInst);
	SETBMPHANDLE(hwnd, NULL);
	SETBMPINT(hwnd, ibmp);
	SETBUTTONSIZE(hwnd, MAKELONG(ptSize.y, ptSize.x));
	return (BOOL)SendMessage(hwnd, WM_SYSCOLORCHANGE, 0, 0L); // do the work
}

/***************************************************************************/
/* toolbarGetNumButtons:  return the number of buttons registered on a     */
/*                        given toolbar window.                            */
/***************************************************************************/
int FAR PASCAL toolbarGetNumButtons(HWND hwnd)
{
    return GETNUMBUTTONS(hwnd);
}


/***************************************************************************/
/* toolbarButtonFromIndex:  Given an index into the array of buttons on    */
/*                          this toolbar, return which button is there.    */
/*                          Returns -1 for an error code.                  */
/***************************************************************************/
int FAR PASCAL toolbarButtonFromIndex(HWND hwnd, int iBtnPos)
{
	int		iButton;
	HANDLE		h;
	TOOLBUTTON	far *lpaButtons;

	/* Get the array of buttons on this toolbar */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;
	
	/* Validate the index passed in */
	if (iBtnPos > GETNUMBUTTONS(hwnd) || iBtnPos < 0)
		return -1;

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Read off the answer */
	iButton = lpaButtons[iBtnPos].iButton;

	GlobalUnlock(h);
	return iButton;
}


/***************************************************************************/
/* toolbarIndexFromButton:  Given a button ID, return the position in the  */
/*                          array that it appears at.                      */
/*                          Returns -1 for an error code.                  */
/***************************************************************************/
int FAR PASCAL toolbarIndexFromButton(HWND hwnd, int iButton)
{
	int		i, iBtnPos = -1;
	HANDLE		h;
	TOOLBUTTON	far *lpButton;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* loop through until you find it */
	for(i = 0; i < GETNUMBUTTONS(hwnd); i++, lpButton++)
		if (lpButton->iButton == iButton) {
			iBtnPos = i;
			break;
		}

	GlobalUnlock(h);
	return iBtnPos;
}



/***************************************************************************/
/* toolbarPrevStateFromButton:  Given a button ID, return the state that   */
/*                              the button was in before it was pressed    */
/*                              all the way down (for non-push buttons).   */
/*                              Return -1 for an error code.               */
/***************************************************************************/
int FAR PASCAL toolbarPrevStateFromButton(HWND hwnd, int iButton)
{
	int		i, iPrevState = -1;
	HANDLE		h;
	TOOLBUTTON	far *lpButton;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* look for what we need */
	for(i = 0; i < GETNUMBUTTONS(hwnd); i++, lpButton++)
		if (lpButton->iButton == iButton) {
			iPrevState = lpButton->iPrevState;
			break;
		}

	GlobalUnlock(h);
	return iPrevState;
}



/***************************************************************************/
/* toolbarActivityFromButton:   Given a button ID, return the most recent  */
/*                              activity that happened to it. (eg DBLCLK)  */
/*                              Return -1 for an error code.               */
/***************************************************************************/
int FAR PASCAL toolbarActivityFromButton(HWND hwnd, int iButton)
{
	int		i, iActivity = -1;
	HANDLE		h;
	TOOLBUTTON	far *lpButton;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* loop through until you find it */
	for(i = 0; i < GETNUMBUTTONS(hwnd); i++, lpButton++)
		if (lpButton->iButton == iButton)
			iActivity = lpButton->iActivity;

	GlobalUnlock(h);
	return iActivity;
}



/***************************************************************************/
/* toolbarIndexFromPoint:  Given a point in the toolbar window, return the */
/*                         index of the button beneath that point.         */
/*                         Return -1 for an error code.                    */
/***************************************************************************/
int FAR PASCAL toolbarIndexFromPoint(HWND hwnd, POINT pt)
{
	int		i, iBtnPos = -1;
	HANDLE		h;
	TOOLBUTTON	far *lpButton;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* loop through until we find an intersection */
	for(i = 0; i < GETNUMBUTTONS(hwnd); i++, lpButton++)
		if (PtInRect(&lpButton->rc, pt)) {
			iBtnPos = i;
			break;
		}

	GlobalUnlock(h);
	return iBtnPos;
}



/***************************************************************************/
/* toolbarRectFromIndex:   Given an index into our array of buttons, return*/
/*                         the rect occupied by that button.               */
/*                         Return a NULL rect for an error.                */
/***************************************************************************/
BOOL FAR PASCAL toolbarRectFromIndex(HWND hwnd, int iBtnPos, LPRECT lprc)
{
	HANDLE		h;
	TOOLBUTTON	far *lpaButtons;
	
	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
	    return FALSE;

	/* Validate the index passed in */
	if (iBtnPos > GETNUMBUTTONS(hwnd) || iBtnPos < 0)
	    return FALSE;

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Read off the rect */
	*lprc = lpaButtons[iBtnPos].rc;

	GlobalUnlock(h);
        return TRUE;
}



/***************************************************************************/
/* toolbarFullStateFromButton: Given a button in our array of buttons,     */
/*                             return the state of that button.            */
/*                             (including the wierd state FULLDOWN). For   */
/*                             just UP or DOWN or GRAYED,                  */
/*                             call toolbarStateFromButton.		   */
/*                             Return -1 for an error.                     */
/***************************************************************************/
int FAR PASCAL toolbarFullStateFromButton(HWND hwnd, int iButton)
{
	int		iState, iBtnPos;
	HANDLE		h;
	TOOLBUTTON	far *lpaButtons;

	iBtnPos = toolbarIndexFromButton(hwnd, iButton);
	if (iBtnPos == -1)
		return -1;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Read off the state */
	iState = lpaButtons[iBtnPos].iState;

	GlobalUnlock(h);
	return iState;
}	



/***************************************************************************/
/* toolbarStateFromButton: This fn is called by the parent application     */
/*                         to get the state of a button.  It will only     */
/*                         return DOWN, or UP or GRAYED as opposed to      */
/*                         toolbarFullStateFromButton which could return   */
/*                         FULLDOWN.                                       */
/***************************************************************************/
int FAR PASCAL toolbarStateFromButton(HWND hwnd, int iButton)
{
	int	iState;

	/* If a checkbox button is all the way down, it's previous state is */
	/* the one we want.						    */
	if ((iState = toolbarFullStateFromButton(hwnd, iButton))
							== BTNST_FULLDOWN) {
	    iState = toolbarPrevStateFromButton(hwnd, iButton);
	    return iState;
	} else
	    return iState;
}



/***************************************************************************/
/* toolbarStringFromIndex: Given an index into our array of buttons, return*/
/*                         the string resource associated with it.         */
/*                         Return -1 for an error.                         */
/***************************************************************************/
int FAR PASCAL toolbarStringFromIndex(HWND hwnd, int iBtnPos)
{
	int		iString;
	HANDLE		h;
	TOOLBUTTON	far *lpaButtons;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;

	/* Validate the index passed in */
	if (iBtnPos > GETNUMBUTTONS(hwnd) || iBtnPos < 0)
		return -1;

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Read off the ID */
	iString = lpaButtons[iBtnPos].iString;	

	GlobalUnlock(h);
	return iString;
}



/***************************************************************************/
/* toolbarTypeFromIndex:   Given an index into our array of buttons, return*/
/*                         the type of button it is (PUSH, RADIO, etc.)    */
/*                         Return -1 for an error.                         */
/***************************************************************************/
int FAR PASCAL toolbarTypeFromIndex(HWND hwnd, int iBtnPos)
{
	int		iType;
	HANDLE		h;
	TOOLBUTTON	far *lpaButtons;

	/* Get the Array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return -1;

	/* Validate the index passed in */
	if (iBtnPos > GETNUMBUTTONS(hwnd) || iBtnPos < 0)
		return -1;

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Read off the type */
	iType = lpaButtons[iBtnPos].iType;

	GlobalUnlock(h);
	return iType;
}


/***************************************************************************/
/* toolbarAddTool:  Add a button to this toolbar.  Sort them by leftmost   */
/*                  position in the window (for tabbing order).            */
/*                  Return FALSE for an error.                             */
/***************************************************************************/
BOOL FAR PASCAL toolbarAddTool(HWND hwnd, TOOLBUTTON tb)
{
	HANDLE		h;
	TOOLBUTTON far  *lpaButtons;
	int		cButtons, i, j;
	BOOL		fInsert = FALSE;

	/* We better not have this button on the toolbar already */
	if (toolbarIndexFromButton(hwnd, tb.iButton) != -1)
		return FALSE;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;

	/* How many buttons are there already? */
	cButtons = GETNUMBUTTONS(hwnd);

	/* If we have filled our alloced memory for this array already, we */
	/* need to re-alloc some more memory				   */
	if ( ((cButtons & (TOOLGROW - 1)) == 0) && (cButtons > 0) ) {

		/* Re-alloc it bigger */
		h = GlobalReAlloc(h,
			GlobalSize(h) + TOOLGROW * sizeof(TOOLBUTTON),
			GMEM_MOVEABLE | GMEM_SHARE);
		if (!h)
		    return FALSE;
	}

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Look for the spot we need to insert this new guy at.	*/
 	/* Remember, we sort by left x position	breaking ties   */
 	/* with top y position.					*/
	for (i = 0; i < cButtons; i++) {
						// Here it goes
 	    if (lpaButtons[i].rc.left > tb.rc.left ||
 			(lpaButtons[i].rc.left == tb.rc.left &&
 				lpaButtons[i].rc.top > tb.rc.top)) {
		fInsert = TRUE;
		/* Open up a spot in the array */
		for (j = cButtons; j > i; j--)
		    lpaButtons[j] = lpaButtons[j-1];
		/* Add our new guy */
		lpaButtons[i] = tb;		// redraw now
		InvalidateRect(hwnd, &(lpaButtons[i].rc), FALSE);
		break;
	    }
	}

	/* If our loop didn't insert it, we need to add it to the end */
	if (!fInsert)
	    lpaButtons[i] = tb;

	/* If we are told that this button has the focus, we better	*/
	/* change the focus to it.  Then use the normal state.          */
	if (tb.iState == BTNST_FOCUSUP) {
	    tb.iState = BTNST_UP;
	    SETWHICH(hwnd, i);
	} else if (tb.iState == BTNST_FOCUSDOWN || tb.iState == BTNST_FULLDOWN){
	    tb.iState = BTNST_DOWN;	// nonsense to init to FULLDOWN
	    SETWHICH(hwnd, i);
	}

	cButtons++;		// one more button now.
	GlobalUnlock(h);

	SETNUMBUTTONS(hwnd, cButtons);	// new count
	SETARRAYBUTT(hwnd, h);		// re-alloc might have changed it

	/* Just in case no one else makes this new button draw */
	InvalidateRect(hwnd, &(tb.rc), FALSE);

	return TRUE;
}


 /***************************************************************************/
 /* toolbarRetrieveTool:  Get the TOOLBUTTON struct for the given button.   */
 /*                       Return FALSE for an error.                        */
 /***************************************************************************/
 BOOL FAR PASCAL toolbarRetrieveTool(HWND hwnd, int iButton, LPTOOLBUTTON lptb)
 {
 	int		i;
 	HANDLE		h;
 	TOOLBUTTON	far *lpButton;
 	BOOL		fFound = FALSE;
 	
 	/* Get the array of buttons */
 	h = GETARRAYBUTT(hwnd);
 	if (!h)
 		return FALSE;
 	lpButton = (TOOLBUTTON far *)GlobalLock(h);

 	/* look for what we need */
 	for(i = 0; i < GETNUMBUTTONS(hwnd); i++, lpButton++)
 		if (lpButton->iButton == iButton) {
 			*lptb = *lpButton;
 			fFound = TRUE;
 			break;
 		}

 	GlobalUnlock(h);
 	return fFound;
 }



/***************************************************************************/
/* toolbarRemoveTool:  Remove this button ID from our array of buttons on  */
/*                    the toolbar.  (only 1 of each button ID allowed).   */
/*                     Return FALSE for an error.                          */
/***************************************************************************/
BOOL FAR PASCAL toolbarRemoveTool(HWND hwnd, int iButton)
{
	HANDLE		h;
	TOOLBUTTON far  *lpaButtons;
	int		cButtons, i, j;
	BOOL		fFound = FALSE;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;

	/* How many buttons are on there now? */
	cButtons = GETNUMBUTTONS(hwnd);

	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

	/* Find a match, remove it, and close the array around it. */
	for (i = 0; i < cButtons; i++)
		if (lpaButtons[i].iButton == iButton) {	
			fFound = TRUE;
						// redraw now
			InvalidateRect(hwnd, &(lpaButtons[i].rc), FALSE);
			if (i != cButtons - 1)	// Last button? Don't bother!
				for (j = i; j < cButtons; j++)
					lpaButtons[j] = lpaButtons[j + 1];
			break;
		}

	GlobalUnlock(h);

	/* Didn't find it! */
	if (!fFound)
	    return FALSE;

	/* One less button */
	cButtons--;

	/* Every once in a while, re-alloc a smaller array chunk to	*/
	/* save memory.							*/
	if ( ((cButtons & (TOOLGROW - 1)) == 0) && (cButtons > 0) ) {

		/* Re-alloc it smaller */
		h = GlobalReAlloc(h,
			GlobalSize(h) - TOOLGROW * sizeof(TOOLBUTTON),
			GMEM_MOVEABLE | GMEM_SHARE);
		if (!h)
		    return FALSE;
	}

	SETNUMBUTTONS(hwnd, cButtons);	// new count
	SETARRAYBUTT(hwnd, h);		// re-alloc could have changed it

	return TRUE;
}

/***************************************************************************/
/* toolbarModifyString: Given a button ID on the toolbar, change it's      */
/*                      string resource associated with it.                */
/*                      returns FALSE for an error or if no such button    */
/***************************************************************************/
BOOL FAR PASCAL toolbarModifyString(HWND hwnd, int iButton, int iString)
{
	HANDLE		h;
	TOOLBUTTON far  *lpButton;
	int		cButtons, i;
	BOOL		fFound = FALSE;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;

	/* How many buttons? */
	cButtons = GETNUMBUTTONS(hwnd);
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* Find that button, and change it's state */
	for (i = 0; i < cButtons; i++, lpButton++)
		if (lpButton->iButton == iButton) {
			lpButton->iString = iString;
			fFound = TRUE;			// redraw now
			break;
		}

	GlobalUnlock(h);
	return fFound;
}

/***************************************************************************/
/* toolbarModifyState:  Given a button ID on the toolbar, change it's      */
/*                      state.                                             */
/*                      returns FALSE for an error or if no such button    */
/***************************************************************************/
BOOL FAR PASCAL toolbarModifyState(HWND hwnd, int iButton, int iState)
{
	HANDLE		h;
	TOOLBUTTON far  *lpButton;
	int		cButtons, i;
	BOOL		fFound = FALSE;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;

	/* How many buttons? */
	cButtons = GETNUMBUTTONS(hwnd);
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* Find that button, and change it's state */
	for (i = 0; i < cButtons; i++, lpButton++)
		if (lpButton->iButton == iButton) {
			if (lpButton->iState != iState) {
				lpButton->iState = iState;
				InvalidateRect(hwnd, &(lpButton->rc), FALSE);
			}
			fFound = TRUE;			// redraw now

			/* if we're pushing a radio button down, bring */
			/* all others in its group up */
			if (lpButton->iType >= BTNTYPE_RADIO &&
					iState == BTNST_DOWN)
			    toolbarExclusiveRadio(hwnd, lpButton->iType,
								iButton);
			break;
		}

	GlobalUnlock(h);
	return fFound;
}


/***************************************************************************/
/* toolbarModifyPrevState: Given a button on the toolbar, change it's prev-*/
/*                      ious state. Used for non-PUSH buttons to remember  */
/*                      what state a button was in before pressed all the  */
/*                      way down, so that when you let go, you know what   */
/*                      state to set it to (the opposite of what it was).  */
/*                      returns FALSE for an error (no button array)       */
/***************************************************************************/
BOOL FAR PASCAL toolbarModifyPrevState(HWND hwnd, int iButton, int iPrevState)
{
	HANDLE		h;
	TOOLBUTTON far  *lpButton;
	int		cButtons, i;

	/* Get button array */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;

	/* How many buttons? */
	cButtons = GETNUMBUTTONS(hwnd);

	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* Find the button, change the state */
	for (i = 0; i < cButtons; i++, lpButton++)
		if (lpButton->iButton == iButton) {
			lpButton->iPrevState = iPrevState;
			break;
		}

	GlobalUnlock(h);
	return TRUE;
}


/***************************************************************************/
/* toolbarModifyActivity: Given a button ID on the toolbar, change it's    */
/*                        activity.  This tells the app what just happened */
/*                        to the button (ie. KEYUP, MOUSEDBLCLK, etc.)     */
/*                        returns FALSE for an error or if no such button  */
/***************************************************************************/
BOOL FAR PASCAL toolbarModifyActivity(HWND hwnd, int iButton, int iActivity)
{
	HANDLE		h;
	TOOLBUTTON far  *lpButton;
	int		cButtons, i;

	/* Get the button array */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;

	/* How many buttons */
	cButtons = GETNUMBUTTONS(hwnd);

	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* loop through and change the right one */
	for (i = 0; i < cButtons; i++, lpButton++)
		if (lpButton->iButton == iButton) {
			lpButton->iActivity = iActivity;
			break;
		}

	GlobalUnlock(h);
	return TRUE;
}



/***************************************************************************/
/* toolbarFixFocus:  SETWHICH() has been called to tell us which button    */
/*                   has the focus, but the states of all the buttons are  */
/*                   not updated (ie. take focus away from the old button) */
/*                   This routine is called from the Paint routine to fix  */
/*                   the states of all the buttons before drawing them.    */
/*                   Returns FALSE for an error.                           */
/***************************************************************************/
BOOL FAR PASCAL toolbarFixFocus(HWND hwnd)
{
	int		iFocus;
	HANDLE		h;
	TOOLBUTTON	far *lpaButtons;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;
	lpaButtons = (TOOLBUTTON far *)GlobalLock(h);

        /* if focus is on an illegal button, default to the first one */
	iFocus = GETWHICH(hwnd);
	if (iFocus < 0 || iFocus >= GETNUMBUTTONS(hwnd))
	    SETWHICH(hwnd, 0);

	/* First of all, make sure that the focus in not on a grayed button. */
	/* if so, we advance focus.  If it runs out of buttons without       */
	/* finding a non-gray one, we start back at the beginning and start  */
	/* looking for a non-gray one from there.  If every button is grayed,*/
	/* we leave no focus anywhere.					     */
	if (lpaButtons[GETWHICH(hwnd)].iState == BTNST_GRAYED) {
	    if (!toolbarMoveFocus(hwnd, FALSE)) {
		SETWHICH(hwnd, -1);
		toolbarMoveFocus(hwnd, FALSE);
	    }
	}

	GlobalUnlock(h);
	return TRUE;
}



/***************************************************************************/
/* toolbarExclusiveRadio:  For radio buttons, we need to pop all others    */
/*                         in the group up when one goes down.  Pass the   */
/*                         button that is going down, and its group, and   */
/*                         this routine will pop all others up.            */
/*                         Returns FALSE for an error.                     */
/***************************************************************************/
BOOL FAR PASCAL toolbarExclusiveRadio(HWND hwnd, int iType, int iButton)
{
	int		i;
	HANDLE		h;
	TOOLBUTTON	far *lpButton;

	/* Get the array of buttons */
	h = GETARRAYBUTT(hwnd);
	if (!h)
		return FALSE;
	lpButton = (TOOLBUTTON far *)GlobalLock(h);

	/* all buttons with this type that aren't this button come up	*/
	/* if they are not grayed					*/
	for(i = 0; i < GETNUMBUTTONS(hwnd); i++, lpButton++)
	    if (lpButton->iType == iType)
		if (lpButton->iButton != iButton &&
				lpButton->iState != BTNST_GRAYED) {
		    toolbarModifyState(hwnd, lpButton->iButton,	BTNST_UP);
		}

	GlobalUnlock(h);
	return TRUE;
}


/*	NotifyParent()  of activity to a button  */

static void NEAR PASCAL NotifyParent(HWND hwnd, int iButton)
{
#ifdef _WIN32
        PostMessage(
            GetParent(hwnd),
            WM_COMMAND,
            GET_WM_COMMAND_MPS(GetWindowLong(hwnd, GWL_ID), hwnd, iButton));
#else
	PostMessage(GetParent(hwnd),WM_COMMAND,
			GetWindowWord(hwnd,GWW_ID),MAKELONG(hwnd,iButton));
#endif
}


/***************************************************************************/
/* toolbarPaintControl:  Handles paint messages by blitting each bitmap    */
/*                       that is on the toolbar to its rect.               */
/*                       First, it fixes the states of the buttons to give */
/*                       the focus to the proper button.                   */
/*                       Returns FALSE for an error.                       */
/***************************************************************************/
static BOOL NEAR PASCAL toolbarPaintControl(HWND hwnd, HDC hdc)
{
    int		iBtnPos;	/* 0 to toolbarGetNumButtons inclusive	*/
    int		iButton;	/* 0 to NUMBUTTONS-1 inclusive		*/
    int		iState;		/* 0 to NUMSTATES-1 inclusive		*/
    HDC		hdcBtn;		/* DC onto button bitmap		*/

    RECT	rcDest;
    POINT	pt;
    long	l;
    HANDLE	hbm;

    /* Make a source HDC for the button pictures, and select the button */
    /* bitmap into it.							*/
    hdcBtn = CreateCompatibleDC(hdc);
    if (!hdcBtn)
	return FALSE;
    hbm = GETBMPHANDLE(hwnd);
    if (hbm) {
	if (!SelectObject(hdcBtn, GETBMPHANDLE(hwnd))) {
	    DeleteDC(hdcBtn);
	    return FALSE;
	}
    }

    toolbarFixFocus(hwnd);	// set the focus field correctly

    /* Go through all buttons on the toolbar */
    for (iBtnPos = 0; iBtnPos < toolbarGetNumButtons(hwnd); iBtnPos++) {

	iButton = toolbarButtonFromIndex(hwnd, iBtnPos);	// button
	iState = toolbarFullStateFromButton(hwnd, iButton);	// state
	toolbarRectFromIndex(hwnd, iBtnPos, &rcDest);		// Dest Rect
	
	/* If we have the focus, we should draw it that way */
        if (GetFocus() == hwnd && GETWHICH(hwnd) == iBtnPos
						&& iState == BTNST_UP)
	    iState = BTNST_FOCUSUP;
        if (GetFocus() == hwnd && GETWHICH(hwnd) == iBtnPos
						&& iState == BTNST_DOWN)
	    iState = BTNST_FOCUSDOWN;

	/* If we don't have the focus, we should take it away */
        if ((GetFocus() != hwnd || GETWHICH(hwnd) != iBtnPos)
						&& iState == BTNST_FOCUSUP)
	    iState = BTNST_UP;
        if ((GetFocus() != hwnd || GETWHICH(hwnd) == iBtnPos)
						&& iState == BTNST_FOCUSDOWN)
	    iState = BTNST_DOWN;

	/* The size of each button */
	l = GETBUTTONSIZE(hwnd);
	pt.x = HIWORD(l);
	pt.y = LOWORD(l);

	/* Blit from the button picture to the toolbar window */
	BitBlt(hdc, rcDest.left, rcDest.top,
	    rcDest.right - rcDest.left, rcDest.bottom - rcDest.top,
	    hdcBtn, pt.x * iButton, pt.y * iState,
	    SRCCOPY);
    }

    DeleteDC(hdcBtn);

    return TRUE;
}




/***************************************************************************/
/* toolbarMoveFocus:  Move Focus forward or backward one button.  You give */
/*                    it the direction to move the focus.  The routine will*/
/*                    stop at the end of the button list without wrapping  */
/*                    around.                                              */
/*                    Returns TRUE if focus moved, or FALSE if it ran out  */
/*                    of buttons before finding a non-grayed one.          */
/***************************************************************************/
BOOL FAR PASCAL toolbarMoveFocus(HWND hwnd, BOOL fBackward)
{
	int 	iBtnPos, iButton, nOffset, nStopAt;
	RECT	rc;
	int iPrevPos = GETWHICH(hwnd); 	/* Who used to have focus? */

	/* Fix illegal value.  It's OK to be one less or greater than range */
	if (iPrevPos < -1 || iPrevPos > GETNUMBUTTONS(hwnd))
	    SETWHICH(hwnd, 0);	// good a default as any

	if (fBackward) {
	    nOffset = -1;
	    nStopAt = -1;
	} else {
	    nOffset = 1;
	    nStopAt = GETNUMBUTTONS(hwnd);
	}
			
	/* look for next button that isn't grayed    */
	/* DON'T wrap around - future code will pass */
	/* the focus to another window (???)         */
	for (iBtnPos = GETWHICH(hwnd) + nOffset;
		    iBtnPos != nStopAt;
		    iBtnPos += nOffset) {
	    iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
	    if (toolbarStateFromButton(hwnd, iButton) !=
				    BTNST_GRAYED) {
		SETWHICH(hwnd, iBtnPos);	// set focus

		/* Redraw both old and new focused button */
		toolbarRectFromIndex(hwnd, iPrevPos, &rc);
		InvalidateRect(hwnd, &rc, FALSE);
		toolbarRectFromIndex(hwnd, iBtnPos, &rc);
		InvalidateRect(hwnd, &rc, FALSE);
		break;

	    }
	}

	if (GETWHICH(hwnd) != iPrevPos)
	    return TRUE;
	else
	    return FALSE;
}

/***************************************************************************/
/* toolbarSetFocus :  Set the focus in the toolbar to the specified button.*/
/*                    If it's gray, it'll set focus to next ungrayed btn.  */
/*                    Returns TRUE if focus set, or FALSE if the button    */
/*                    doesn't exist or if it and all buttons after it were */
/*                    grayed...       You can use TB_FIRST or TB_LAST in   */
/*                    place of a button ID.  This uses the first or last   */
/*                    un-grayed button.                                    */
/***************************************************************************/
BOOL FAR PASCAL toolbarSetFocus(HWND hwnd, int iButton)
{
    int iBtnPos;
    RECT rc;

    /* Don't move focus while a button is down */
    if (GetCapture() != hwnd && !GETKEYPRESSED(hwnd)) {

	/* redraw button with focus in case focus moves */
	toolbarRectFromIndex(hwnd, GETWHICH(hwnd), &rc);
	InvalidateRect(hwnd, &rc, FALSE);

	if (iButton == TB_FIRST) {
	    SETWHICH(hwnd, -1); // move forward to 1st button
	    return toolbarMoveFocus(hwnd, FALSE);
	} else if (iButton == TB_LAST) {
	    SETWHICH(hwnd, GETNUMBUTTONS(hwnd));
	    return toolbarMoveFocus(hwnd, TRUE);
	} else {
	    iBtnPos = toolbarIndexFromButton(hwnd, iButton);
	    if (iBtnPos != -1) {
		SETWHICH(hwnd, --iBtnPos);
		return toolbarMoveFocus(hwnd, FALSE);
	    } else
		return FALSE;
	}
	return TRUE;

    } else
	return FALSE;
}

//
//  LoadUIBitmap() - load a bitmap resource
//
//      load a bitmap resource from a resource file, converting all
//      the standard UI colors to the current user specifed ones.
//
//      this code is designed to load bitmaps used in "gray ui" or
//      "toolbar" code.
//
//      the bitmap must be a 4bpp windows 3.0 DIB, with the standard
//      VGA 16 colors.
//
//      the bitmap must be authored with the following colors
//
//          Button Text        Black        (index 0)
//          Button Face        lt gray      (index 7)
//          Button Shadow      gray         (index 8)
//          Button Highlight   white        (index 15)
//          Window Color       yellow       (index 11)
//          Window Frame       green        (index 10)
//
//      Example:
//
//          hbm = LoadUIBitmap(hInstance, "TestBmp",
//              GetSysColor(COLOR_BTNTEXT),
//              GetSysColor(COLOR_BTNFACE),
//              GetSysColor(COLOR_BTNSHADOW),
//              GetSysColor(COLOR_BTNHIGHLIGHT),
//              GetSysColor(COLOR_WINDOW),
//              GetSysColor(COLOR_WINDOWFRAME));
//
//      Author:     JimBov, ToddLa
//
//

HBITMAP FAR PASCAL  LoadUIBitmap(
    HANDLE      hInstance,          // EXE file to load resource from
    LPCSTR      szName,             // name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame)           // color to use for "Window Frame"
{
    LPBYTE              lpb;
    HBITMAP             hbm;
    LPBITMAPINFOHEADER  lpbi;
    HANDLE              h;
    HDC                 hdc;
    LPDWORD             lprgb;
    int isize;
    HANDLE hmem;
    LPBYTE lpCopy;

    // convert a RGB into a RGBQ
    #define RGBQ(dw) RGB(GetBValue(dw),GetGValue(dw),GetRValue(dw))

    h = LoadResource (hInstance,FindResource(hInstance, szName, RT_BITMAP));

    lpbi = (LPBITMAPINFOHEADER)LockResource(h);

    if (!lpbi)
        return(NULL);

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
        return NULL;

    if (lpbi->biBitCount != 4)
        return NULL;

    /*
     * copy the resource since they are now loaded read-only
     */
#ifdef _WIN32
    isize = lpbi->biSize + lpbi->biSizeImage +
            ((int)lpbi->biClrUsed ?
                    (int)lpbi->biClrUsed :
                    (1 << (int)lpbi->biBitCount))
            * sizeof(RGBQUAD);
    hmem = GlobalAlloc(GHND, isize);
    lpCopy = GlobalLock(hmem);
    if ((hmem == NULL) || (lpCopy == NULL)) {
        UnlockResource(h);
        FreeResource(h);
        return(NULL);
    }


    CopyMemory(lpCopy, lpbi, isize);
    UnlockResource(h);
    FreeResource(h);

    lpbi = (LPBITMAPINFOHEADER)lpCopy;
#endif

    /* Calcluate the pointer to the Bits information */
    /* First skip over the header structure */

    lprgb = (LPDWORD)((LPBYTE)(lpbi) + lpbi->biSize);

    /* Skip the color table entries, if any */
    lpb = (LPBYTE)lprgb + ((int)lpbi->biClrUsed ? (int)lpbi->biClrUsed :
        (1 << (int)lpbi->biBitCount)) * sizeof(RGBQUAD);

    lprgb[0]  = RGBQ(rgbText);          // Black
    lprgb[7]  = RGBQ(rgbFace);          // lt gray
    lprgb[8]  = RGBQ(rgbShadow);        // gray
    lprgb[15] = RGBQ(rgbHighlight);     // white
    lprgb[11] = RGBQ(rgbWindow);        // yellow
    lprgb[10] = RGBQ(rgbFrame);         // green

    hdc = GetDC(NULL);

    hbm = CreateDIBitmap (hdc, lpbi, CBM_INIT, (LPVOID)lpb,
        (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hdc);
    UnlockResource(h);
    FreeResource(h);

    return(hbm);
}

/****************************************************************************
	toolbarWndProc()

	Window proc for toolbar.

	Arguments:
		Standard window proc
****************************************************************************/

LRESULT FAR PASCAL toolbarWndProc(HWND hwnd, unsigned message,
						WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT	ps;
    POINT		pt;
    RECT		rc;
    int			iBtnPos, iButton, ibmp;
    HANDLE		lpaButtons, hbm, hInst;

    switch (message) {

        case WM_CREATE:			// do all initialization
		
		/* What do these do? */
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
		    			SWP_NOZORDER | SWP_NOSIZE |
					SWP_NOMOVE | SWP_NOACTIVATE);
		SetWindowLong(hwnd,GWL_STYLE,lpCreate->style & 0xFFFF00FF);
		
		/* Alloc some space for the array of buttons on this bar */
		lpaButtons = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
					TOOLGROW * sizeof(TOOLBUTTON));

		SETARRAYBUTT(hwnd, lpaButtons);	// list of buttons on toolbar
		SETNUMBUTTONS(hwnd, 0);		// # buttons in toolbar
		SETPRESSED(hwnd, FALSE);	// mouse button being pressed?
		SETKEYPRESSED(hwnd, FALSE);	// is a key being pressed?
		SETWHICH(hwnd, -1);		// which button has the focus?
		SETSHIFTED(hwnd, FALSE);	// shift-click or right-click?

		/* This wParam will be sent to the parent window to indentify */
		/* that the toolbar sent the WM_COMMAND msg.  The hwnd of the */
		/* toolbar that sent the msg will be in the lParam.	      */
#ifdef _WIN32
		SetWindowLong(hwnd, GWL_ID, IDC_TOOLBAR);
#else
		SetWindowWord(hwnd, GWW_ID, (WORD)IDC_TOOLBAR);
#endif

		/* later on, someone will set the bmp handle of the buttons */
		SETBMPHANDLE(hwnd, NULL);

		break;

        case WM_LBUTTONDOWN:	// button goes down on a toolbar button
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:

		/* If we don't give ourself focus, we'll never get KEYDOWN */
		/* or KEYUP messages.					   */
		/* Get the focus only if we're a TABSTOP and the app wants */
		/* us to take focus.					   */
		if ( (GetWindowLong(hwnd, GWL_STYLE) & WS_TABSTOP)
						&& GetFocus() != hwnd)
		    SetFocus(hwnd);

		/* ignore messages if window is disabled */
		if (!IsWindowEnabled(hwnd))
		    return 0L;

		/* ignore multiple down messages (we set Capture here) */
		/* also ignore if a key is down                        */
		if (GetCapture() == hwnd || GETPRESSED(hwnd))
		    return 0L;
		
		/* Where did the mouse go down? */
                pt.x = (short)LOWORD(lParam);
                pt.y = (short)HIWORD(lParam);

		/* which button was pressed? */
		iBtnPos = toolbarIndexFromPoint(hwnd, pt);

		/* If it was a valid button... */
		if (iBtnPos >= 0) {
		    int		iOldPos;
		    int		iState, iType, iButton;

		    /* Everything you wanted to know about this button */
		    iType = toolbarTypeFromIndex(hwnd, iBtnPos);
		    iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
		    iState = toolbarFullStateFromButton(hwnd, iButton);

		    /* ignore downs on a grayed button, unless it's a	*/
		    /* custom button, then tell them anyway		*/
		    if (iType != BTNTYPE_CUSTOM && iState == BTNST_GRAYED)
			return 0;

		    /* We better get all mouse messages from now on */
		    SetCapture(hwnd);

		    /* Shift key or right button indicates a SHIFT down */
		    SETSHIFTED(hwnd, (message == WM_RBUTTONDOWN) ||
						    (wParam & MK_SHIFT));

		    /* Yes, we've pressed the button down */
		    SETPRESSED(hwnd, TRUE);

		    /* Remember who used to have the focus, and we get it now */
		    iOldPos = GETWHICH(hwnd);
		    SETWHICH(hwnd, iBtnPos);

		    /* For a push button, send it down */
		    if (iType == BTNTYPE_PUSH)
			toolbarModifyState(hwnd, iButton, BTNST_DOWN);

		    /* for a checkbox or radio button (of any group),       */
		    /* remember what state it was in, and send it FULL down */
		    /* (with focus).					    */
		    if (iType == BTNTYPE_CHECKBOX || iType >= BTNTYPE_RADIO) {
			toolbarModifyPrevState(hwnd, iButton, iState);
			toolbarModifyState(hwnd,iButton,BTNST_FULLDOWN);
		    }

		    toolbarModifyActivity(hwnd, iButton, BTNACT_MOUSEDOWN);

		    /* Set Double click flag appropriately */
		    if (message == WM_LBUTTONDBLCLK ||
						message == WM_RBUTTONDBLCLK)
			NotifyParent(hwnd, (GETSHIFTED(hwnd) ? BTN_SHIFT : 0)
						 + BTN_DBLCLICK + iButton);
		    else
			NotifyParent(hwnd, (GETSHIFTED(hwnd) ? BTN_SHIFT : 0)
						 + iButton);

		    /* Invalidate the Rect of the button being pressed */
		    toolbarRectFromIndex(hwnd, iBtnPos, &rc);
		    InvalidateRect(hwnd, &rc, FALSE);

		    /* Invalidate the Rect of the button losing focus */
		    toolbarRectFromIndex(hwnd, iOldPos, &rc);
		    InvalidateRect(hwnd, &rc, FALSE);

		    /* Force re-paint now */
		    UpdateWindow(hwnd);

		    /* Set a timer for repeated mouse downs */
		    SetTimer(hwnd, TIMER_BUTTONREPEAT,
				 MSEC_BUTTONREPEAT, NULL);
		}
		
		return 0L;

        case WM_MOUSEMOVE:

#if 0
		/* This should be impossible - it means that the system lost */
		/* a mouse up (maybe codeview is up?) We need to force a     */
		/* mouse up at this point.				     */
		if (GetCapture() == hwnd &&
			(wParam & (MK_LBUTTON | MK_RBUTTON) == 0))
		    SendMessage(hwnd, WM_LBUTTONUP, 0, lParam);
#endif

		/* Mouse moving while pressing a button?  If not, ignore. */
		if (GetCapture() == hwnd) {
		    int		iPrevState, iState, iButton, iType;
		    BOOL	fPressed;
		
		    /* Which button is being pressed down? */
		    iBtnPos = GETWHICH(hwnd);

		    /* Where is mouse cursor now? */
                    pt.x = (short)LOWORD(lParam);
                    pt.y = (short)HIWORD(lParam);

		    /* where is button being pressed? Are we still on */
		    /* top of that button or have we moved?	      */
		    toolbarRectFromIndex(hwnd, iBtnPos, &rc);
		    fPressed = PtInRect(&rc, pt);

		    /* Let go if we move off of the button, but don't */
		    /* act like it was pressed.                       */
		    /* Also, push it back down if we move back on top */
		    /* of it (while the mouse button is STILL down).  */
		    if (fPressed != GETPRESSED(hwnd)) {

			/* update: is this button pressed anymore? */
			SETPRESSED(hwnd, fPressed);

			iType = toolbarTypeFromIndex(hwnd, iBtnPos);
			iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
			iState = toolbarFullStateFromButton(hwnd, iButton);

			/* The mouse moved back onto the button while */
			/* the mouse button was still pressed.	      */
			if (fPressed) {

			    /* Push the push button back down again */
	 		    if (iType == BTNTYPE_PUSH)
				toolbarModifyState(hwnd, iButton,
							BTNST_DOWN);

			    /* Push the radio or checkbox button ALL the */
			    /* way down again.				 */
			    if (iType >= BTNTYPE_RADIO ||
						iType == BTNTYPE_CHECKBOX)
				toolbarModifyState(hwnd, iButton,
							BTNST_FULLDOWN);

			    toolbarModifyActivity(hwnd, iButton,
							BTNACT_MOUSEMOVEON);
			    NotifyParent(hwnd,
					(GETSHIFTED(hwnd) ? BTN_SHIFT : 0) +
					iButton);

			/* We moved the mouse off of the toolbar button */
			/* while still holding the mouse button down.   */
			} else {

			    /* lift the push button up */
	 		    if (iType == BTNTYPE_PUSH)
				toolbarModifyState(hwnd, iButton,
							BTNST_UP);

			    /* Restore radio button or checkbox button to */
			    /* where it was before pressed		  */
			    if (iType >= BTNTYPE_RADIO ||
						iType == BTNTYPE_CHECKBOX) {
				iPrevState = toolbarPrevStateFromButton(hwnd,
							iButton);
				toolbarModifyState(hwnd, iButton, iPrevState);
			    }

			    toolbarModifyActivity(hwnd, iButton,
							BTNACT_MOUSEMOVEOFF);
			    NotifyParent(hwnd,
					(GETSHIFTED(hwnd) ? BTN_SHIFT : 0) +
					toolbarButtonFromIndex(hwnd, iBtnPos));
			}
		    }
		}
		return 0L;

        case WM_LBUTTONUP:	
        case WM_RBUTTONUP:

		/* If we don't have capture, we aren't expecting this. Ignore */
		if (GetCapture() == hwnd) {
		    int		iPrevState, iState, iButton, iType;
		
		    /* Who has the focus? */
		    iBtnPos = GETWHICH(hwnd);

		    /* Release the mouse */
		    ReleaseCapture();
		
		    /* No more repeats of the mouse button downs */
		    KillTimer(hwnd, TIMER_BUTTONREPEAT);
		
		    /* Everything you wanted to know about the button */
		    toolbarRectFromIndex(hwnd, iBtnPos, &rc);
		    iType = toolbarTypeFromIndex(hwnd, iBtnPos);
		    iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
		    iState = toolbarFullStateFromButton(hwnd, iButton);

		    /* Don't do anything if we've moved off the button */
		    if (GETPRESSED(hwnd)) {

			/* No longer down */
			SETPRESSED(hwnd, FALSE);

			/* Bring the push button up */
			if (iType == BTNTYPE_PUSH)
			    toolbarModifyState(hwnd, iButton, BTNST_UP);

			/* Bring the checkbox to the opposite state it was in */
			if (iType == BTNTYPE_CHECKBOX) {
			    iPrevState = toolbarPrevStateFromButton(hwnd,
							iButton);
			    if (iPrevState == BTNST_DOWN)
				toolbarModifyState(hwnd, iButton, BTNST_UP);
			    if (iPrevState == BTNST_UP)
				toolbarModifyState(hwnd, iButton, BTNST_DOWN);
			}

			/* Force a radio button down, and bring all   */
			/* other radio buttons of this type up	      */
			if (iType >= BTNTYPE_RADIO) {
			    toolbarModifyState(hwnd, iButton, BTNST_DOWN);
			    toolbarExclusiveRadio(hwnd, iType, iButton);
			}

			/* Notify the parent that the mouse button came up */
			/* on this button so the app can do something.     */
			/* Every button should notify the app, not just a  */
			/* custom button.				   */
			toolbarModifyActivity(hwnd, iButton, BTNACT_MOUSEUP);
			NotifyParent(hwnd,
			    (GETSHIFTED(hwnd) ? BTN_SHIFT : 0) + iButton);
		    }
		}

		return 0L;

		
	case WM_TIMER:

		/* If we have a tool button down, send a repeat message */
		if (GETPRESSED(hwnd)) {
		    int		iButton, iType;

		    iBtnPos = GETWHICH(hwnd);
		    iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
		    iType = toolbarTypeFromIndex(hwnd, iBtnPos);

		    NotifyParent(hwnd, BTN_REPEAT +
					(GETSHIFTED(hwnd) ? BTN_SHIFT : 0) +
					toolbarButtonFromIndex(hwnd, iBtnPos));
		}
		break;
		

        case WM_DESTROY:
		if (GETBMPHANDLE(hwnd))
		    DeleteObject(GETBMPHANDLE(hwnd));
		SETBMPHANDLE(hwnd, NULL);
		if (GETARRAYBUTT(hwnd))
		    GlobalFree(GETARRAYBUTT(hwnd));
		SETARRAYBUTT(hwnd, NULL);
		break;

        case WM_SETTEXT:
		break;
		
/* MANY, MANY cases deleted */

	case WM_SETFOCUS:		// focus comes to toolbar window
	    {
		/* Remember who had the focus and give it back.  Of course, */
		/* if by some wierdness that button is now grayed, give it  */
		/* to the next person in line.				    */
		iBtnPos = GETWHICH(hwnd);
		if (iBtnPos < 0 || iBtnPos >= toolbarGetNumButtons(hwnd)) {
		    iBtnPos = 0;
		    SETWHICH(hwnd, 0);
		}

		do {
		    iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
		    if (toolbarFullStateFromButton(hwnd, iButton)
							!= BTNST_GRAYED)
			break;			// give it here
		    iBtnPos++;
		    if (iBtnPos >= toolbarGetNumButtons(hwnd))
			iBtnPos = 0;		// wrap around
		    if (iBtnPos == GETWHICH(hwnd))
			return 0L;		// uh-oh! They're all gray!
		} while (iBtnPos != GETWHICH(hwnd));
			
		SETWHICH(hwnd, iBtnPos);	// give focus here
		
		/* And redraw! */
		toolbarRectFromIndex(hwnd, iBtnPos, &rc);
		InvalidateRect(hwnd, &rc, FALSE);
		UpdateWindow(hwnd);
		return 0;
	    }
	
	case WM_KILLFOCUS:

		/* Send a KEYUP if one is pending */
		if (GETKEYPRESSED(hwnd))
		    SendMessage(hwnd, WM_KEYUP, VK_SPACE, 0L);

		/* Redraw the focused button, because now that focus is gone */
		/* from our toolbar window, the focused button won't be      */
		/* focused anymore, although we remember which one it was.   */
		toolbarRectFromIndex(hwnd, GETWHICH(hwnd), &rc);
		InvalidateRect(hwnd, &rc, FALSE);
		UpdateWindow(hwnd);
		return 0;

	case WM_SYSKEYDOWN:
		/* Send a KEYUP if one is pending */
		if (GETKEYPRESSED(hwnd))
		    SendMessage(hwnd, WM_KEYUP, VK_SPACE, 0L);
		break;	// MUST LET DEFWNDPROC RUN!!! (to handle the key)

        case WM_GETDLGCODE:
		return DLGC_WANTARROWS | DLGC_WANTTAB;

	case WM_KEYDOWN:

		/* Window disabled or a key is already down */
		if (IsWindowEnabled(hwnd) && !GETPRESSED(hwnd)) {

		    /* Tab forward to next button and move focus there */
		    if (wParam == VK_TAB && GetKeyState(VK_SHIFT) >= 0 ) {

			/* Move Focus forward one.  If */
			/* we've tabbed off of the toolbar, it's time */
			/* to go on to the next control. We need to invldte */
			/* because we might be the only control and we need */
			/* to repaint to show the new button with highlight */
			/* after it wrapped around the end of the toolbar.  */
			if (!toolbarMoveFocus(hwnd, FALSE)) {
			    PostMessage(GetParent(hwnd), WM_NEXTDLGCTL, 0, 0L);
			    toolbarRectFromIndex(hwnd, GETWHICH(hwnd), &rc);
			    InvalidateRect(hwnd, &rc, FALSE);
			}

			return 0L;
		    }
		    if (wParam == VK_TAB && GetKeyState(VK_SHIFT) < 0 ) {

			/* Move focus backward one.  If */
			/* We've tabbed off of the toolbar, it's time    */
			/* to go on to the next control. We need to invldte */
			/* because we might be the only control and we need */
			/* to repaint to show the new button with highlight */
			/* after it wrapped around the end of the toolbar.  */
			if (!toolbarMoveFocus(hwnd, TRUE)) {
			    PostMessage(GetParent(hwnd), WM_NEXTDLGCTL, 1, 0L);
			    toolbarRectFromIndex(hwnd, GETWHICH(hwnd), &rc);
			    InvalidateRect(hwnd, &rc, FALSE);
			}

			return 0L;
		    }
		    if ((wParam == VK_SPACE) && (GetCapture() != hwnd)) {

			int	iButton, iType, iState;

			/* Same as mouse button down -- Press the button! */
			iBtnPos = GETWHICH(hwnd);
			iType = toolbarTypeFromIndex(hwnd, iBtnPos);
			iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
			iState = toolbarFullStateFromButton(hwnd, iButton);

			/* ignore multiple key downs */
			if (!GETKEYPRESSED(hwnd)) {

			    SETKEYPRESSED(hwnd, TRUE);	// a key is pressed

			    SETSHIFTED(hwnd, FALSE);	// NEVER shifted
			    SETPRESSED(hwnd, TRUE);	// a button is pressed

			    /* Push button goes down - with focus */
			    if (iType == BTNTYPE_PUSH)
				toolbarModifyState(hwnd, iButton, BTNST_DOWN);

			    /* Radio or checkbox button goes full down */
			    /* with focus - and remember previous state*/
			    if (iType >= BTNTYPE_RADIO ||
						iType == BTNTYPE_CHECKBOX) {
				toolbarModifyPrevState(hwnd, iButton, iState);
				toolbarModifyState(hwnd, iButton,
							BTNST_FULLDOWN);
			    }

			    toolbarModifyActivity(hwnd, iButton,
								BTNACT_KEYDOWN);
			    NotifyParent(hwnd, (GETSHIFTED(hwnd)
						? BTN_SHIFT : 0) + iButton);

			    return 0L;
			}
		
			/* If this is another KEYDOWN msg, it's a REPEAT */
			/* Notify parent.                                */
			NotifyParent(hwnd, BTN_REPEAT +
					(GETSHIFTED(hwnd) ? BTN_SHIFT : 0) +
					toolbarButtonFromIndex(hwnd,
							GETWHICH(hwnd)));
		    }
		}
		break;
	
	case WM_KEYUP:

		/* A button was pressed and should come up now */
		if ((wParam == VK_SPACE) && (GETKEYPRESSED(hwnd))) {
		    int		iButton, iState, iType, iPrevState;

		    iBtnPos = GETWHICH(hwnd);		// which button?
		    SETKEYPRESSED(hwnd, FALSE);		// let go
		    SETPRESSED(hwnd, FALSE);

		    /* Everything about this button */
		    toolbarRectFromIndex(hwnd, iBtnPos, &rc);
		    iType = toolbarTypeFromIndex(hwnd, iBtnPos);
		    iButton = toolbarButtonFromIndex(hwnd, iBtnPos);
		    iState = toolbarFullStateFromButton(hwnd, iButton);

		    /* Bring a push button up */
		    if (iType == BTNTYPE_PUSH)
			toolbarModifyState(hwnd, iButton, BTNST_UP);

		    /* Bring a checkbox to the opposite state it was in */
		    if (iType == BTNTYPE_CHECKBOX) {
			iPrevState = toolbarPrevStateFromButton(hwnd, iButton);
			if (iPrevState == BTNST_DOWN)
			    toolbarModifyState(hwnd, iButton, BTNST_UP);
			if (iPrevState == BTNST_UP)
			    toolbarModifyState(hwnd, iButton, BTNST_DOWN);
		    }

		    /* Bring a radio button down, and bring all others in */
		    /* its group up.					  */
		    if (iType >= BTNTYPE_RADIO) {
			toolbarModifyState(hwnd, iButton, BTNST_DOWN);
			toolbarExclusiveRadio(hwnd, iType, iButton);
		    }

		    toolbarModifyActivity(hwnd, iButton, BTNACT_KEYUP);
		    NotifyParent(hwnd, toolbarButtonFromIndex(hwnd,
					(GETSHIFTED(hwnd) ? BTN_SHIFT : 0) +
					GETWHICH(hwnd)));
		}
		break;
	
	case WM_SYSCOLORCHANGE:
		/* load the bitmap of what all the buttons look like */
		/* and change the colours to the system colours.     */
		hInst = GETHINST(hwnd);
		ibmp = GETBMPINT(hwnd);
		hbm = GETBMPHANDLE(hwnd);
		if (hbm)
		    DeleteObject(hbm);
		hbm = LoadUIBitmap(hInst, MAKEINTRESOURCE(ibmp),
		    GetSysColor(COLOR_BTNTEXT),
		    GetSysColor(COLOR_BTNFACE),
		    GetSysColor(COLOR_BTNSHADOW),
		    GetSysColor(COLOR_BTNHIGHLIGHT),
		    GetSysColor(COLOR_BTNFACE),
		    GetSysColor(COLOR_WINDOWFRAME));
		SETBMPHANDLE(hwnd, hbm);
#ifdef _WIN32
		return (LONG_PTR) hbm;
#else
		return MAKELONG(hbm, 0);
#endif

        case WM_ERASEBKGND:
		break;


        case WM_PAINT:

		/* Call our paint code */
		BeginPaint(hwnd, &ps);
		toolbarPaintControl(hwnd, ps.hdc);
		EndPaint(hwnd, &ps);

		return 0L;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);

}
