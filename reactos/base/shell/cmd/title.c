/*
 *  title.c - title internal command.
 *
 *
 *  History:
 *	1999-02-11 Emanuele Aliberti
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_TITLE


INT cmd_title (LPTSTR cmd, LPTSTR param)
{

	/* Do nothing if no args */
	if (*param == _T('\0'))
		return 0;

	/* Asking help? */
	if (!_tcsncmp(param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_TITLE_HELP);
		return 0;
	}

	return SetConsoleTitle (param);
}

#endif /* def INCLUDE_CMD_TITLE */

/* EOF */
