/* $Id: color.c,v 1.1 2003/03/20 19:19:22 rcampbell Exp $
 *
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
 *
 *    14-Oct-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        4nt's syntax implemented
 */

#include "config.h"

#ifdef INCLUDE_CMD_COLOR
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"

static VOID ColorHelp (VOID)
{
		ConOutPuts (_T(
			"Sets the default foreground and background colors.\n"
			"\n"
			"COLOR [attr [/F]] \n\n"
			"  attr        Specifies color attribute of console output\n"
			"  /F          fill the console with color attribute\n"
			"\n"			
			"There are three ways to specify the colors:"
			));

		ConOutPuts (_T(
			"\n"
			"1) [bright] name on [bright] name  (only the first three letters are required)\n"
			"2) decimal on decimal\n"
			"3) two hex digits\n"
			"\n"
			"Colors are:"
			));

		ConOutPuts (_T(
			"dec  hex  name       dec  hex  name\n"
			"0    0    Black       8   8    Gray(Bright black)\n"
			"1    1    Blue        9   9    Bright Blue\n"
			"2    2    Green      10   A    Bright Green\n"
			"3    3    Cyan       11   B    Bright Cyan\n"
			"4    4    Red        12   C    Bright Red\n"
			"5    5    Magenta    13   D    Bright Magenta\n"
			"6    6    Yellow     14   E    Bright Yellow\n"
			"7    7    White      15   F    Bright White"));
}


VOID SetScreenColor (WORD wColor, BOOL bFill)
{
	DWORD dwWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coPos;

	if (bFill == TRUE)
	{
		GetConsoleScreenBufferInfo (hOut, &csbi);

		coPos.X = 0;
		coPos.Y = 0;
		FillConsoleOutputAttribute (hOut,
		                            (WORD)(wColor & 0x00FF),
		                            (csbi.dwSize.X)*(csbi.dwSize.Y),
		                            coPos,
		                            &dwWritten);
	}
	SetConsoleTextAttribute (hOut, (WORD)(wColor & 0x00FF));
}


/*
 * color
 *
 * internal dir command
 */
INT CommandColor (LPTSTR first, LPTSTR rest)
{
	if (_tcsncmp (rest, _T("/?"), 2) == 0)
	{
		ColorHelp ();
		return 0;
	}

	if (rest[0] == _T('\0'))
	{
		/* set default color */
		wColor = wDefColor;
		SetScreenColor (wColor, TRUE);
		return 0;
	}

	if (StringToColor (&wColor, &rest) == FALSE)
	{
		ConErrPuts("error in color specification");
		return 1;
	}

	ConErrPrintf ("Color %x\n", wColor);

	if ((wColor & 0xF) == (wColor &0xF0) >> 4)
	{
		ConErrPuts (_T("same colors error!"));
		return 1;
	}

	/* set color */
	SetScreenColor (wColor,
	                (_tcsstr (rest,"/F") || _tcsstr (rest,"/f")));

	return 0;
}

#endif /* INCLUDE_CMD_COLOR */

/* EOF */
