/*
 *  VER.C - ver internal command.
 *
 *
 *  History:
 *
 *    06/30/98 (Rob Lake)
 *        rewrote ver command to accept switches, now ver alone prints
 *        copyright notice only.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added text about where to send bug reports and get updates.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *
 *    26-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        New version info and some output changes.
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"



VOID ShortVersion (VOID)
{
	ConOutPuts (_T("\n"
	               SHELLINFO "\n"
	               SHELLVER "\n"));
}


#ifdef INCLUDE_CMD_VER

/*
 *  display shell version info internal command.
 *
 *
 */
INT cmd_ver (LPTSTR cmd, LPTSTR param)
{
	INT i;

	if (_tcsstr (param, _T("/?")) != NULL)
	{
		ConOutPuts (_T("Displays shell version information\n"
		               "\n"
		               "VER [/C][/R][/W]\n"
		               "\n"
		               "  /C  Displays credits.\n"
		               "  /R  Displays redistribution information.\n"
		               "  /W  Displays warranty information."));
		return 0;
	}

	ConOutPuts (_T("\n"
	               SHELLINFO "\n"
	               SHELLVER "\n"
	               "\n"
	               "Copyright (C) 1994-1998 Tim Norman and others."));
	ConOutPuts (_T("Copyright (C) 1998-2001 Eric Kohl and others."));

	/* Basic copyright notice */
	if (param[0] == _T('\0'))
	{
		ConOutPuts (_T("\n"SHELLINFO
		               " comes with ABSOLUTELY NO WARRANTY; for details\n"
		               "type: `ver /w'. This is free software, and you are welcome to redistribute\n"
		               "it under certain conditions; type `ver /r' for details. Type `ver /c' for a\n"
		               "listing of credits."));
	}
	else
	{
		for (i = 0; param[i]; i++)
		{
			/* skip spaces */
			if (param[i] == _T(' '))
				continue;

			if (param[i] == _T('/'))
			{
				/* is this a lone '/' ? */
				if (param[i + 1] == 0)
				{
					error_invalid_switch (_T(' '));
					return 1;
				}
				continue;
			}

			if (_totupper (param[i]) == _T('W'))
			{
				/* Warranty notice */
				ConOutPuts (_T("\n This program is distributed in the hope that it will be useful,\n"
				               " but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
				               " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
				               " GNU General Public License for more details."));
			}
			else if (_totupper (param[i]) == _T('R'))
			{
				/* Redistribution notice */
				ConOutPuts (_T("\n This program is free software; you can redistribute it and/or modify\n"
				               " it under the terms of the GNU General Public License as published by\n"
				               " the Free Software Foundation; either version 2 of the License, or\n"
				               " (at your option) any later version."));
			}
			else if (_totupper (param[i]) == _T('C'))
			{
				/* Developer listing */
				ConOutPuts (_T("\n"
				               "FreeDOS version written by:\n"
				               "    Tim Norman      Matt Rains\n"
				               "    Evan Jeffrey    Steffen Kaiser\n"
				               "    Svante Frey     Oliver Mueller\n"
				               "    Aaron Kaufman   Marc Desrochers\n"
				               "    Rob Lake        John P Price\n"
				               "    Hans B Pufal\n"
				               "\n"
				               "ReactOS version written by:\n"
				               "    Eric Kohl       Emanuele Aliberti\n"
				               "    Paolo Pantaleo  Phillip Susi\n"));
			}
			else
			{
				error_invalid_switch ((TCHAR)_totupper (param[i]));
				return 1;
			}
		}
	}

	ConOutPuts (_T("\n"
	               "Send bug reports to <ekohl@rz-online.de>.\n"
	               "Updates are available at: http://www.reactos.com"));
	return 0;
}

#endif
