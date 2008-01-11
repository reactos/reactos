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
 *    04-Dec-1998 (Eric Kohl)
 *        Changed to Win32 console app.
 *
 *    08-Dec-1998 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    14-Jan-1998 (Eric Kohl)
 *        Unicode ready!
 *
 *    20-Jan-1998 (Eric Kohl)
 *        Redirection ready!
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_CLS

INT cmd_cls (LPTSTR cmd, LPTSTR param)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coPos;
	DWORD dwWritten;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CLS_HELP);
		return 0;
	}

	GetConsoleScreenBufferInfo(hConsole, &csbi);

	coPos.X = 0;
	coPos.Y = 0;
	FillConsoleOutputAttribute(hConsole, wColor,
	                           csbi.dwSize.X * csbi.dwSize.Y,
	                           coPos, &dwWritten);
	FillConsoleOutputCharacter(hConsole, _T(' '),
	                           csbi.dwSize.X * csbi.dwSize.Y,
	                           coPos, &dwWritten);
	SetConsoleCursorPosition(hConsole, coPos);

	bIgnoreEcho = TRUE;

	return 0;
}
#endif
