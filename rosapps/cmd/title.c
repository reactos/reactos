/*
 *  title.c - title internal command.
 *
 *
 *  History:
 *	1999-02-11 Emanuele Aliberti
 */

#include "config.h"

#ifdef INCLUDE_CMD_TITLE

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"


INT cmd_title (LPTSTR cmd, LPTSTR param)
{
	/* Do nothing if no args */
	if (*param == _T('\0'))
		return 0;

	/* Asking help? */
	if (!_tcsncmp(param, _T("/?"), 2))
	{
		ConOutPuts (_T("Sets the window title for the command prompt window.\n"
					   "\n"
					   "TITLE [string]\n"
					   "\n"
					   "  string       Specifies the title for the command prompt window."));
		return 0;
	}

	return SetConsoleTitle (param);
}

#endif /* def INCLUDE_CMD_TITLE */

/* EOF */