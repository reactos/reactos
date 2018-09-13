/*	File: D:\WACKER\tdll\globals.c (Created: 26-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 8/18/99 10:52a $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "globals.h"
#include "assert.h"

#include <term\res.h>

static TCHAR szHelpFileName[FNAME_LEN];

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	glblQueryHelpFileName
 *
 * DESCRIPTION:
 *  Return the name of the help file.
 *
 * RETURNS:
 *	LPTSTR - pointer to the help file name.
 */
LPTSTR glblQueryHelpFileName(void)
	{
	return ((LPTSTR)szHelpFileName);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	glblSetHelpFileName
 *
 * DESCRIPTION:
 *  Load the help file name from the registry or resources.
 *
 * RETURNS:
 */
void glblSetHelpFileName(void)
	{
    DWORD dwSize = sizeof(szHelpFileName);
    HKEY  hKey;

	memset(szHelpFileName, 0, dwSize);
	LoadString(glblQueryDllHinst(), IDS_GNRL_HELPFILE, szHelpFileName,
               dwSize / sizeof(TCHAR));

    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\HyperTrm.exe"),
                       0,
                       KEY_QUERY_VALUE,
                       &hKey) == ERROR_SUCCESS)
        {

	    memset(szHelpFileName, 0, dwSize);
        if (RegQueryValueEx(hKey, TEXT("HelpFileName"), 0, 0,
                            szHelpFileName, &dwSize) != ERROR_SUCCESS)
            {
	        LoadString(glblQueryDllHinst(), IDS_GNRL_HELPFILE, szHelpFileName,
                       dwSize / sizeof(TCHAR));
            }

        RegCloseKey(hKey);
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * Set and Query the program's instance handle.  Only the Query function
 * is exported.
 */

static HINSTANCE hInstance;

HINSTANCE  glblQueryHinst(void)
	{
	return hInstance;
	}

void  glblSetHinst(const HINSTANCE hInst)
	{
	hInstance = hInst;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * Set and Query the DLL's instance handle.  Only the Query function is
 * exported.
 */

static HINSTANCE hDllInstance;

HINSTANCE glblQueryDllHinst(void)
	{
	return hDllInstance;
	}

void glblSetDllHinst(const HINSTANCE hInst)
	{
	hDllInstance = hInst;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * Set and Query the program's accelerator handle.
 */

static HACCEL hAccel;

void glblSetAccelHdl(const HACCEL hAccelerator)
	{
	hAccel = hAccelerator;
	}

HACCEL glblQueryAccelHdl(void)
	{
	return hAccel;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * Set and Query the program's frame window handle.  Currently the frame
 * window is also the session window, but don't count on it necessarily
 * staying that way.  Upper wacker may change.	I strongly discourge the
 * use of the glbl????HwndFrame functions.	Why are they here?	The message
 * loop (ie. TranslateAccelerator()) needs the handle of the window that
 * owns the menus in order to process accelerators. - mrw
 */

static HWND hwndFrame;

void glblSetHwndFrame(const HWND hwnd)
	{
	hwndFrame = hwnd;
	}

HWND glblQueryHwndFrame(void)
	{
	return hwndFrame;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * DESCRIPTION:
 *	This block of global data is used to manage modeless dialogs.  It consists
 *	of two items.  The first is a counter to determin how many modeless dialogs
 *	are currently registered.  The second is an array of window handles that
 *	are for the modeless dialogs.  At the present time the array is staticly
 *	allocated but could be made dynamic if it ever becomes a problem.
 *
 */
static int glblModelessDlgCount = 0;
static HWND glblModelessDlgArray[64];			/* Think that's enough ? */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	glblAddModelessDlgHwnd
 *
 * DESCRIPTION:
 *	This function adds a window handle to our list of modeless dialog windows.
 *
 * PARAMETERS:
 *	hwnd -- the window handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code of some sort.
 *
 */
int glblAddModelessDlgHwnd(const HWND hwnd)
	{
	int nIndx;

	if (!IsWindow(hwnd))
		{
		assert(FALSE);
		return 1;
		}

	if (glblModelessDlgCount >= 62)
		{
		assert(FALSE);
		return 2;
		}

	for (nIndx = 0; nIndx < glblModelessDlgCount; nIndx += 1)
		{
		if (hwnd == glblModelessDlgArray[nIndx])
			{
			assert(FALSE);
			return 3;
			}
		}

	/*
	 * "It must be safe", he said foolishly.
	 */
	glblModelessDlgArray[glblModelessDlgCount++] = hwnd;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	glblDeleteModelessDlgHwnd
 *
 * DESCRIPTION:
 *	This function takes a modeless dialog box window handle out of the list
 *	that the previous function put it into.
 *
 * PARAMETERS:
 *	hwnd -- the very window handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise an error code of some sort.
 *
 */
int glblDeleteModelessDlgHwnd(const HWND hwnd)
	{
	int nIndx;

	if (glblModelessDlgCount == 0)
		{
		assert(FALSE);
		return 2;
		}

	for (nIndx = 0; nIndx < glblModelessDlgCount; nIndx += 1)
		{
		if (hwnd == glblModelessDlgArray[nIndx])
			{
			/* remove and adjust the array */
			while (nIndx < 62)
				{
				glblModelessDlgArray[nIndx] = glblModelessDlgArray[nIndx + 1];
				nIndx += 1;
				}

			glblModelessDlgCount -= 1;

			return 0;
			}
		}

	/* Never found the puppy */
	assert(FALSE);
	return 3;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CheckModelessMessage
 *
 * DESCRIPTION:
 *	This function is called to check and see if there are any modeless dialog
 *	boxes awaiting input and feeds them the messages.
 *
 * PARAMETERS:
 *	pmsg -- pointer to the message structure
 *
 * RETURNS:
 *	TRUE if it has already processed the message, otherwise FALSE
 *
 */
int CheckModelessMessage(MSG *pmsg)
	{
	int nIndx;

	/* Avoid unnecessary effort */
	if (glblModelessDlgCount == 0)
		return FALSE;

	for (nIndx = 0; nIndx < glblModelessDlgCount; nIndx += 1)
		{
#if !defined(NDEBUG)
		assert(IsWindow(glblModelessDlgArray[nIndx]));
#endif
		if (IsDialogMessage(glblModelessDlgArray[nIndx], pmsg))
			return TRUE;
		}
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * It is possible, under some conditions, that the program will shut down
 * because of an error.  Under some of these conditions, there may not be
 * a valid session handle, so it becomes necessary to store the status of
 * the shutdown as a static variable.
 */

static int nShutdownStatus;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	glblQueryProgramStatus
 *
 * DESCRIPTION:
 *	Returns the startup/shutdown status of the program.
 *
 * ARGUMENTS:
 *	None.
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise a shutdown status code.
 *
 */
int glblQueryProgramStatus()
	{
	return nShutdownStatus;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	glblSetProgramStatus
 *
 * DESCRIPTION:
 *	Changes the startup/shutdown status of the program.
 *
 * ARGUMENTS:
 *	nStatus -- the new status.
 *
 * RETURNS:
 *	The previous status.
 *
 */
int glblSetProgramStatus(int nStatus)
	{
	int nRet;

	nRet = nShutdownStatus;
	nShutdownStatus = nStatus;

	return nShutdownStatus;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * Banner is displayed at program startup
 */

static HWND hwndBanner;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	glblQueryHwndBanner
 *
 * DESCRIPTION:
 *	Returns window handle of banner
 *
 * ARGUMENTS:
 *	void
 *
 * RETURNS:
 *	HWND
 *
 */
HWND glblQueryHwndBanner(void)
	{
	return hwndBanner;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	glblSetHwndBanner
 *
 * DESCRIPTION:
 *	Sets the value of hwndBanner for later reference
 *
 * ARGUMENTS:
 *	hwnd	- window handle of banner
 *
 * RETURNS:
 *	void
 *
 */
void glblSetHwndBanner(const HWND hwnd)
	{
	hwndBanner = hwnd;
	return;
	}
