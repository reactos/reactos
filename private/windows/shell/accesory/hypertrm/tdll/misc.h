/*	File: D:\WACKER\tdll\misc.h (Created: 27-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

#if !defined(INCL_MISC)
#define INCL_MISC

BOOL 	mscCenterWindowOnWindow(const HWND hwndChild, const HWND hwndParent);

LPTSTR 	mscStripPath	(LPTSTR pszStr);
LPTSTR 	mscStripName	(LPTSTR pszStr);
LPTSTR 	mscStripExt		(LPTSTR pszStr);
LPTSTR	mscModifyToFit	(HWND hwnd, LPTSTR pszStr);

int 	mscCreatePath(const TCHAR *pszPath);
int 	mscIsDirectory(LPCTSTR pszName);
int 	mscAskWizardQuestionAgain(void);
void	mscUpdateRegistryValue(void);

HICON	extLoadIcon(LPCSTR);
#endif
