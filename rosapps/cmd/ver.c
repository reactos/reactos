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
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"

#define VER_HELP     "display shell version info   VER [/C/R/W/?]"


VOID ShortVersion (VOID)
{
	ConOutPuts (_T("\n" SHELLINFO " " SHELLVER "\n"));
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

	/* JPP 07/08/1998 clean up and shortened info. */

	ConOutPuts (_T("\n" SHELLINFO " " SHELLVER "\nCopyright (C) 1994-1998 Tim Norman and others."));
	ConOutPuts (_T("Copyright (C) 1998,1999 Eric Kohl."));

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
		/* MS-DOS ver prints just help if /? is alone or not */
		if (_tcsstr (param, _T("/?")) != NULL)
		{
			ConOutPuts (_T(USAGE ": " VER_HELP));
			return 0;
		}

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
				/* JPP 07/08/1998 removed extra printf calls */
				ConOutPuts (_T("\n This program is distributed in the hope that it will be useful,\n"
							   " but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
							   " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
							   " GNU General Public License for more details."));
			}
			else if (_totupper (param[i]) == _T('R'))
			{
				/* Redistribution notice */
				/* JPP 07/08/1998 removed extra printf calls */
				ConOutPuts (_T("\n This program is free software; you can redistribute it and/or modify\n"
							   " it under the terms of the GNU General Public License as published by\n"
							   " the Free Software Foundation; either version 2 of the License, or\n"
							   " (at your option) any later version."));
			}
			else if (_totupper (param[i]) == _T('C'))
			{
				/* Developer listing */
				/* JPP 07/08/1998 removed extra printf calls; rearranged names */
				ConOutPuts (_T("\ndeveloped by:\n"
							   "    Tim Norman      Matt Rains\n"
							   "    Evan Jeffrey    Steffen Kaiser\n"
							   "    Svante Frey     Oliver Mueller\n"
							   "    Aaron Kaufman   Marc Desrochers\n"
							   "    Rob Lake        John P Price\n"
							   "    Hans B Pufal\n"
							   "\nconverted to Win32 by:\n"
							   "    Eric Kohl\n"));
			}
			else
			{
				error_invalid_switch ((TCHAR)_totupper (param[i]));
				return 1;
			}
		}
	}

	ConOutPuts (_T("\nSend bug reports to <ekohl@abo.rhein-zeitung.de>.\n"
/*
				   "Updates are available at ftp://www.sid-dis.com/..."
*/
			   ));
	return 0;
}

#endif
