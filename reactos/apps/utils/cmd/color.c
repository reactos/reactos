/*
 *  COLOR.C - color internal command.
 *
 *
 *  History:
 *
 *    13-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_COLOR
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


VOID SetScreenColor (WORD wColor)
{
	DWORD dwWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coPos;

	GetConsoleScreenBufferInfo (hOut, &csbi);

	coPos.X = 0;
	coPos.Y = 0;
	FillConsoleOutputAttribute (hOut, wColor,
								(csbi.dwSize.X)*(csbi.dwSize.Y),
								coPos, &dwWritten);
	SetConsoleTextAttribute (hOut, wColor);
}


/*
 * color
 *
 * internal dir command
 */
INT cmd_color (LPTSTR first, LPTSTR rest)
{
	if (_tcsncmp (rest, _T("/?"), 2) == 0)
	{
		ConOutPuts (_T("Sets the default foreground and background colors.\n\n"
				   "COLOR [attr]\n\n"
				   "  attr        Specifies color attribute of console output\n\n"
				   "Color attributes are specified by TWO hex digits -- the first\n"
				   "corresponds to the background; the second to the foreground. Each digit\n"
				   "can be one of the following:\n"));

		ConOutPuts (_T("    0 = Black       8 = Gray\n"
				   "    1 = Blue        9 = Light Blue\n"
				   "    2 = Green       A = Light Green\n"
				   "    3 = Aqua        B = Light Aqua\n"
				   "    4 = Red         C = Light Red\n"
				   "    5 = Purple      D = Light Purple\n"
				   "    6 = Yellow      E = Light Yellow\n"
				   "    7 = White       F = Bright White\n"));
		return 0;
	}

	if (rest[0] == _T('\0'))
	{
		/* set default color */
		wColor = wDefColor;
		SetScreenColor (wColor);
		return 0;
	}

	if (_tcslen (rest) != 2)
	{
		ConErrPuts (_T("parameter error!"));
		return 1;
	}

	wColor = (WORD)_tcstoul (rest, NULL, 16);

	if ((wColor & 0xF) == (wColor &0xF0) >> 4)
	{
		ConErrPuts (_T("same colors error!"));
		return 1;
	}

	/* set color */
	SetScreenColor (wColor);

	return 0;
}

#endif
