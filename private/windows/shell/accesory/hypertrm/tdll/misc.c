/*	File: D:\WACKER\tdll\misc.c (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 3 $
 *	$Date: 7/30/99 4:27p $
 */

#include <windows.h>
#pragma hdrstop

#include "stdtyp.h"
#include "misc.h"
#include "tdll.h"
#include "tchar.h"
#include "globals.h"
#include "assert.h"
#include <term\res.h>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mscCenterWindowOnWindow
 *
 * DESCRIPTION:
 *	Center's first window on the second window.  Assumes hwndChild is
 *	a direct descendant of hwndParent
 *
 * ARGUMENTS:
 *	hwndChild	- window to center
 *	hwndParent	- window to center on
 *
 * RETURNS:
 *	BOOL
 *
 */
BOOL mscCenterWindowOnWindow(const HWND hwndChild, const HWND hwndParent)
	{
	RECT	rChild, rParent;
	int 	wChild, hChild, wParent, hParent;
	int 	xNew, yNew;
	int 	iMaxPos;

	if (!IsWindow(hwndParent))
		return FALSE;

	if (!IsWindow(hwndChild))
		return FALSE;

	/* --- Get the Height and Width of the child window --- */

	GetWindowRect(hwndChild, &rChild);
	wChild = rChild.right - rChild.left;
	hChild = rChild.bottom - rChild.top;

	/* --- Get the Height and Width of the parent window --- */

	GetWindowRect(hwndParent, &rParent);
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	/* --- Calculate new X position, then adjust for screen --- */

	xNew = rParent.left + ((wParent - wChild) / 2);

	/* --- Calculate new Y position, then adjust for screen --- */

	// Let's display the dialog so that the title bar is visible.
	//
	iMaxPos = GetSystemMetrics(SM_CYSCREEN);
	yNew = min(iMaxPos, rParent.top + ((hParent - hChild) / 2));

	//mpt:3-13-98 Need to make sure dialog is not off the screen
    if (yNew < 0)
        {
        yNew = 0;
        }

    if (xNew < 0)
        {
        xNew = 0;
        }

    // Set it, and return
	//
	return SetWindowPos(hwndChild, 0, xNew, yNew, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscStripPath
 *
 * DESCRIPTION:
 *	Strip off the path from the file name.
 *
 * ARGUMENTS:
 * 	pszStr - pointer to a null terminated string.
 *
 * RETURNS:
 *  void.
 */
LPTSTR mscStripPath(LPTSTR pszStr)
	{
	LPTSTR pszStart, psz;

	if (pszStr == 0)
		return 0;

	for (psz = pszStart = pszStr; *psz ; psz = StrCharNext(psz))
		{
		if (*psz == TEXT('\\') || *psz == TEXT(':'))
			pszStart = StrCharNext(psz);
		}

	StrCharCopy(pszStr, pszStart);
	return pszStr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscStripName
 *
 * DESCRIPTION:
 *	Strip off the name of the file, leave just the path.
 *
 * ARGUMENTS:
 * 	pszStr - pointer to a null terminated string.
 *
 * RETURNS:
 *  void.
 */
LPTSTR mscStripName(LPTSTR pszStr)
	{
	LPTSTR pszEnd, pszStart = pszStr;

	if (pszStr == 0)
		return 0;

	for (pszEnd = pszStr; *pszStr; pszStr = StrCharNext(pszStr))
		{
		if (*pszStr == TEXT('\\') || *pszStr == TEXT(':'))
			pszEnd = StrCharNext(pszStr);
		}

	*pszEnd = TEXT('\0');
	return (pszStart);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscStripExt
 *
 * DESCRIPTION:
 *	Strip off the file extension.  The parameter string can be a full-path
 *	or just a file name.
 *
 * ARGUMENTS:
 * 	pszStr - pointer to a null terminated string.
 *
 * RETURNS:
 *  void.
 */
LPTSTR mscStripExt(LPTSTR pszStr)
	{
	LPTSTR pszEnd, pszStart = pszStr;

	for (pszEnd = pszStr; *pszStr; pszStr = StrCharNext(pszStr))
		{
		// Need to check for both '.' and '\\' because directory names
		// can have extensions as well.
		//
		if (*pszStr == TEXT('.') || *pszStr == TEXT('\\'))
			pszEnd = pszStr;
		}

	if (*pszEnd == TEXT('.'))
		*pszEnd = TEXT('\0');

	return pszStart;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  mscModifyToFit
 *
 * DESCRIPTION:
 *  If a string won't fit in a given window then chop-off as much as possible
 *  to be able to display a part of the string with ellipsis concatanated to
 *  the end of it.
 *
 *  NOTE: I've attempted to make this code DBCS aware.
 *
 * ARGUMENTS:
 *  hwnd 	- control window, where the text is to be displayed.
 *  pszStr 	- pointer to the string to be displayed.
 *
 * RETURNS:
 *  lpszStr - pointer to the modified string.
 *
 */
LPTSTR mscModifyToFit(HWND hwnd, LPTSTR pszStr)
	{
	HDC	 	hDC;
	SIZE	sz;
	HFONT	hFontSave, hFont;
	RECT	rc;
	TCHAR	ach[512], achEllipsis[10];
	int		nWidth = 0, i = 0;

	TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
	memset(&hFont, 0, sizeof(HFONT));
	memset(&hFontSave, 0, sizeof(HFONT));
	memset(&rc, 0, sizeof(RECT));

	GetWindowRect(hwnd, &rc);
	nWidth = rc.right - rc.left;

	hDC = GetDC(hwnd);

	hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
	if (hFont)
		hFontSave = SelectObject(hDC, hFont);

	// TODO: I think here the string pszStr would have to be "deflated"
	// before we continue.  The rest of the code should stay the same.
	//
	GetTextExtentPoint(hDC, (LPCTSTR)pszStr, StrCharGetStrLength(pszStr), &sz);
	if (sz.cx > nWidth)
		{
		LoadString(glblQueryDllHinst(), IDS_GNRL_ELLIPSIS,
			achEllipsis, sizeof(achEllipsis) / sizeof(TCHAR));

		StrCharCopy(ach, achEllipsis);
		StrCharCat(ach, pszStr);
		i = StrCharGetStrLength(ach);

		while ((i > 0) && (sz.cx > nWidth))
			{
			GetTextExtentPoint(hDC, (LPCTSTR)ach, i, &sz);
			i -= 1;
			}
		TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
		StrCharCopyN(ach, pszStr, i);
		StrCharCat(ach, achEllipsis);
		StrCharCopy(pszStr, ach);
		}

	// Select the previously selected font, release DC.
	//
	if (hFontSave)
		SelectObject(hDC, hFontSave);
	ReleaseDC(hwnd, hDC);

	return pszStr;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	extLoadIcon
 *
 * DESCRIPTION:
 *	Gets the icon from the hticons.dll.  The extension handlers use
 *	this dll for icons and need to not link load anything more than
 *	absolutely necessary, otherwise, this function would go in the
 *	icon handler code.
 *
 * ARGUMENTS:
 *	id	- string id of resource (can be MAKEINTRESOURCE)
 *
 * RETURNS:
 *	HICON or zero on error.
 *
 */
HICON extLoadIcon(LPCSTR id)
	{
	static HINSTANCE hInstance;

	if (hInstance == 0)
		{
		if ((hInstance = LoadLibrary("hticons")) == 0)
			{
			assert(FALSE);
			return 0;
			}
		}

	return LoadIcon(hInstance, id);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	mscCreatePath
 *
 * DESCRIPTION:
 *	Creates the given path.  This function is somewhat tricky so study
 *	it carefully before modifying it.  Despite it's simplicity, it
 *	accounts for all boundary conditions. - mrw
 *
 * ARGUMENTS:
 *	pszPath - path to create
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
int mscCreatePath(const TCHAR *pszPath)
	{
	TCHAR ach[512];
	TCHAR *pachTok;

	if (pszPath == 0)
		return -1;

	StrCharCopy(ach, pszPath);
	pachTok = ach;

	// Basicly, we march along the string until we encounter a '\', flip
	// it to a NULL and try to create the path up to that point.
	// It would have been nice if CreateDirectory() could
	// create sub/sub directories, but it don't. - mrw
	//
	while (1)
		{
		if ((pachTok = StrCharFindFirst(pachTok, TEXT('\\'))) == 0)
			{
			if (!mscIsDirectory(ach) && !CreateDirectory(ach, 0))
				return -2;

			break;
			}

		if (pachTok != ach)
			{
			*pachTok = TEXT('\0');

			if (!mscIsDirectory(ach) && !CreateDirectory(ach, 0))
				return -3;

			*pachTok = TEXT('\\');
			}

		pachTok = StrCharNext(pachTok);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	mscIsDirectory
 *
 * DESCRIPTION:
 *	Checks to see if a string is a valid directory or not.
 *
 * PARAMETERS:
 *	pszName   -- the string to test
 *
 * RETURNS:
 *	TRUE if the string is a valid directory, otherwise FALSE.
 *
 */
int mscIsDirectory(LPCTSTR pszName)
	{
	DWORD dw;

	dw = GetFileAttributes(pszName);

	if ((dw != (DWORD)-1) && (dw & FILE_ATTRIBUTE_DIRECTORY))
		return TRUE;

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	mscAskWizardQuestionAgain
 *
 * DESCRIPTION:
 *	Reads a value from the Registry.  This value represents how many times
 *	the user responded "NO" to the question:  "Do you want to run the
 *	New Modem Wizard?".  We won't ask this question any more if the
 *	user responded no, twice.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	TRUE if the modem wizard question should be asked again, otherwize
 *	FALSE.
 *
 */
int mscAskWizardQuestionAgain(void)
	{
	long	lResult;
	DWORD	dwKeyValue = 0;
	DWORD	dwSize;
	DWORD	dwType;
	TCHAR	*pszAppKey = "HYPERTERMINAL";

	dwSize = sizeof(DWORD);

	lResult = RegQueryValueEx(HKEY_CLASSES_ROOT, (LPTSTR)pszAppKey, 0,
		&dwType, (LPBYTE)&dwKeyValue, &dwSize);

	// If we are able to read a value from the registry and that value
	// is 1, there is no need to ask the question again, so return
	// a false value.
	//
	if ( (lResult == ERROR_SUCCESS) && (dwKeyValue >= 1) )
		return (FALSE);

	return (TRUE);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	mscUpdateRegistryValue
 *
 * DESCRIPTION:
 *	See mscAskWizardQuestionAgain.	If the user responds "NO" to this
 *	question, we update a counter in the registry.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	void
 *
 */
void mscUpdateRegistryValue(void)
	{
	long	lResult;
	DWORD	dwKeyValue = 0;
	DWORD	dwSize;
	DWORD	dwType;
	TCHAR	*pszAppKey = "HYPERTERMINAL";

	dwSize = sizeof(DWORD);

	lResult = RegQueryValueEx(HKEY_CLASSES_ROOT, (LPTSTR)pszAppKey, 0,
		&dwType, (LPBYTE)&dwKeyValue, &dwSize);

	dwKeyValue += 1;

	lResult = RegSetValueEx(HKEY_CLASSES_ROOT, pszAppKey, 0,
		REG_BINARY, (LPBYTE)&dwKeyValue, dwSize);

	assert(lResult == ERROR_SUCCESS);

	return;
	}
