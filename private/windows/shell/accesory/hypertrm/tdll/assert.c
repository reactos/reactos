/*	File: D:\WACKER\tdll\assert.c (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

#include <windows.h>
#pragma hdrstop

#include <stdarg.h>
#include "assert.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoAssertDebug
 *
 * DESCRIPTION:
 *	Our own home-grown assert function.
 *
 * ARGUMENTS:
 *	file	- file where it happened
 *	line	- line where is happened
 *
 * RETURNS:
 *	void
 *
 */
void DoAssertDebug(TCHAR *file, int line)
	{
#if !defined(NDEBUG)
	int retval;
	TCHAR buffer[256];

	wsprintf(buffer,
			TEXT("Assert error in file %s on line %d.\n")
			TEXT("Press YES to continue, NO to call CVW, CANCEL to exit.\n"),
			file, line);

	retval = MessageBox((HWND)0, buffer, TEXT("Assert"),
		MB_ICONQUESTION | MB_YESNOCANCEL | MB_SETFOREGROUND);

	switch (retval)
		{
		case IDYES:
			return;

		case IDNO:
			DebugBreak();
			return;

		case IDCANCEL:
			MessageBeep(MB_ICONHAND);
			ExitProcess(1);
			break;

		default:
			break;
		}

	return;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoDbgOutStr
 *
 * DESCRIPTION:
 *	Used to output a string to a debug monitor.  Use the macros defined
 *	in ASSERT.H to access this function.
 *
 * ARGUMENTS:
 *	LPTSTR	achFmt	- printf style format string.
 *	... 			- arguments used in formating list.
 *
 * RETURNS:
 *	VOID
 *
 */
VOID __cdecl DoDbgOutStr(TCHAR *achFmt, ...)
	{
#if !defined(NDEBUG)
	va_list valist;
	TCHAR	achBuf[256];

	va_start(valist, achFmt);

	wvsprintf(achBuf, achFmt, valist);
	OutputDebugString(achBuf);

	va_end(valist);
	return;
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	DoShowLastError
 *
 * DESCRIPTION:
 * 	Does a GetLastError() and displays it.  Similar to assert.
 *
 * ARGUMENTS:
 *	file	- file where it happened
 *	line	- line where it happened
 *
 * RETURNS:
 *	void
 *
 */
void DoShowLastError(const TCHAR *file, const int line)
	{
#if !defined(NDEBUG)
	int retval;
	TCHAR ach[256];
	const DWORD dwErr = GetLastError();

	if (dwErr == 0)
		return;

	wsprintf(ach, TEXT("GetLastError=0x%x in file %s, on line %d\n")
				  TEXT("Press YES to continue, NO to call CVW, CANCEL to exit.\n"),
				  dwErr, file, line);

	retval = MessageBox((HWND)0, ach, TEXT("GetLastError"),
		MB_ICONQUESTION | MB_YESNOCANCEL | MB_SETFOREGROUND);

	switch (retval)
		{
		case IDYES:
			return;

		case IDNO:
			DebugBreak();
			return;

		case IDCANCEL:
			MessageBeep(MB_ICONHAND);
			ExitProcess(1);
			break;

		default:
			break;
		}

	return;
#endif
	}
