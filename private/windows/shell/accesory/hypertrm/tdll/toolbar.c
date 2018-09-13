/*	File: D:\WACKER\tdll\toolbar.c (Created: 02-Dec-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 6/25/99 9:11a $
 */

#define OEMRESOURCE 	// Need for OBM bitmaps...

#include <windows.h>
#include <commctrl.h>

#include "assert.h"
#include "stdtyp.h"
#include "globals.h"
#include "session.h"
#include <term\res.h>

#define BTN_CNT 7

struct stToolbarStuff
	{
	int nSpacer;			/* Number of spaces to insert before button */
	int nBmpNum;			/* Index of the bitmap for this button */
	int nCmd;				/* Command to associate with the button */
	int nStrRes;			/* String resource ID number */
	};

static struct stToolbarStuff stTS[] = {
	{1, 0, IDM_NEW,				IDS_TTT_NEW},			/* New button */
	{0, 1, IDM_OPEN,			IDS_TTT_OPEN},			/* Open button */
	{1, 2, IDM_ACTIONS_DIAL,	IDS_TTT_DIAL},			/* Dial button */
	{0, 3, IDM_ACTIONS_HANGUP,	IDS_TTT_HANGUP},		/* Hangup button */
	{1, 4, IDM_ACTIONS_SEND,	IDS_TTT_SEND},			/* Send button */
	{0, 5, IDM_ACTIONS_RCV,		IDS_TTT_RECEIVE},		/* Receive button */
	{1, 6, IDM_PROPERTIES,		IDS_TTT_PROPERTY},		/* Properties button */
  //{1, 7, IDM_HELPTOPICS,		IDS_TTT_HELP}			/* Help button */
	};

static void AddMinitelButtons(const HWND hwnd);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateSessionToolbar
 *
 * DESCRIPTION:
 *	Creates a toolbar intended for use with the a session
 *
 * ARGUMENTS:
 *	hwndSession - session window handle
 *
 * RETURNS:
 *	Window handle to toolbar or zero on error.
 *
 */
/* ARGSUSED */
HWND CreateSessionToolbar(const HSESSION hSession, const HWND hwndSession)
	{
	HWND	 hWnd = 0;
	int		 iLoop = 0;
	DWORD	 lTBStyle = TBSTYLE_TOOLTIPS | WS_CHILD | WS_VISIBLE;
	TBBUTTON lButton;

	//
    // Changed to use Flat style in version 4.0 - mpt 06-10-98
	//

#if defined( EXTENDED_FEATURES )
	lTBStyle |= TBSTYLE_FLAT;
#endif

	//
	// Create a toolbar with no buttons.
	//

	hWnd = CreateToolbarEx( hwndSession,
						    lTBStyle,
						    IDC_TOOLBAR_WIN,
						    BTN_CNT,
							glblQueryDllHinst(),
							IDB_BUTTONS_SMALL,
							NULL,				 // Array of buttons
							0,					 // Number of buttons in array
							16, 16,				 // button size
							16, 16,	 			 // bitmap size
							sizeof( TBBUTTON ) );

	assert( hWnd );

	if ( IsWindow( hWnd ) )
		{
		//
		// Add some buttons.
		//

		for ( iLoop = 0; iLoop < BTN_CNT; iLoop++ )
			{
			int iIndex;

			for ( iIndex = 0; iIndex < stTS[ iLoop ].nSpacer; iIndex++ )
				{
				//
				// Don't add seperator at beginning of toolbar.
				//

				if ( iLoop != 0 )
					{
					//
					// Just insert space between two buttons.
					//

					lButton.iBitmap   = 0;
					lButton.idCommand = 0;
					lButton.fsState   = TBSTATE_ENABLED;
					lButton.fsStyle   = TBSTYLE_SEP;
					lButton.dwData    = 0;
					lButton.iString   = 0;
            
					SendMessage( hWnd, TB_ADDBUTTONS, 1, (LPARAM)&lButton );
					}
				}

			lButton.iBitmap   = stTS[ iLoop ].nBmpNum;
			lButton.idCommand = stTS[ iLoop ].nCmd;
			lButton.fsState   = TBSTATE_ENABLED;
			lButton.fsStyle   = TBSTYLE_BUTTON;
			lButton.dwData    = 0;
			lButton.iString   = 0;

			SendMessage( hWnd, TB_ADDBUTTONS, 1, (LPARAM)&lButton );
			}
		}

	return hWnd;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ToolbarNotification
 *
 * DESCRIPTION:
 *	This function is called when the toolbar sends a notification message to
 *	the session window.
 *
 * ARGUMENTS:
 *	hwnd     -- the window handle of the session window
 *	nId      -- the control ID (the tool bar in this case)
 *	nNotify  -- the notification code
 *	hwndCtrl -- the window handle of the tool bar
 *
 * RETURNS:
 *	Whatever the notification requires.  See individual code below.
 *
 */
/* ARGSUSED */
LRESULT ToolbarNotification(const HWND hwnd,
						const int nId,
						const int nNotify,
						const HWND hwndCtrl)
	{
	//lint -e648	TBN constants overflow
	LRESULT lRet = 0;
	static int nCount;

	switch ((UINT)nNotify)
		{
		case TBN_BEGINADJUST:
			/*
			 * No return value.
			 */
			nCount = 1;
			break;

		case TBN_QUERYDELETE:
			/*
			 * Return TRUE to delete the button or FALSE to prevent the button
			 * from being deleted.
			 */
			lRet = FALSE;
			break;

		case TBN_QUERYINSERT:
			/*
			 * Return TRUE to insert the new button in front of the given
			 * button, or FALSE to prevent the button from being inserted.
			 */
			if (nCount > 0)
				{
				nCount -= 1;
				lRet = TRUE;
				}
			break;

		default:
			break;
		}

	return lRet;
	//lint +e648
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ToolbarNeedsText
 *
 * DESCRIPTION:
 *	This function is called when the toolbar sends a notification message to
 *	the session window saying that it needs text for the ToolTips window.
 *
 * ARGUMENTS:
 *	hwnd     -- the window handle of the session window
 *	lPar     -- the lPar that the session window got
 *
 * RETURNS:
 *
 */
/* ARGSUSED */
void ToolbarNeedsText(HSESSION hSession, long lPar)
	{
	unsigned int nLoop;
	LPTOOLTIPTEXT pText = (LPTOOLTIPTEXT)lPar;

	for (nLoop = 0 ; nLoop < DIM(stTS) ; nLoop += 1)
		{
		if ((int)pText->hdr.idFrom == stTS[nLoop].nCmd)
			{
			LoadString(glblQueryDllHinst(), (UINT)stTS[nLoop].nStrRes,
						pText->szText,
						sizeof(pText->szText) / sizeof(TCHAR));
			return;
			}
		}
	}
