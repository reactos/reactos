/*
 *  CLS.C - clear screen internal command.
 *
 *
 *  History:
 *
 *    07/27/1998 (John P. Price)
 *        started.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    04-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Changed to Win32 console app.
 *
 *    08-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    14-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 *
 *    20-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_CLS

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"


INT cmd_cls (LPTSTR cmd, LPTSTR param)
{
	DWORD dwWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coPos;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Clears the screen.\n\nCLS"));
		return 0;
	}

	GetConsoleScreenBufferInfo (hOut, &csbi);

	coPos.X = 0;
	coPos.Y = 0;
	FillConsoleOutputAttribute (hOut, wColor,
								(csbi.dwSize.X)*(csbi.dwSize.Y),
								coPos, &dwWritten);
	FillConsoleOutputCharacter (hOut, _T(' '),
								(csbi.dwSize.X)*(csbi.dwSize.Y),
								coPos, &dwWritten);
	SetConsoleCursorPosition (hOut, coPos);

	bIgnoreEcho = TRUE;

	return 0;
}
#endif
