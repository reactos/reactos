/*
 *  VERIFY.C - verify internal command.
 *
 *
 *  History:
 *
 *    31 Jul 1998 (John P Price)
 *        started.
 *
 *    18-Jan-1999 (Eric Kohl)
 *        VERIFY is just a dummy under Win32; it only exists
 *        for compatibility!!!
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection ready!
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_VERIFY


/* global verify flag */
static BOOL bVerify = FALSE;


INT cmd_verify (LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_VERIFY_HELP1);
		return 0;
	}

	if (!OnOffCommand(param, &bVerify, STRING_VERIFY_HELP2))
	{
		ConErrResPuts(STRING_VERIFY_HELP3);
		return nErrorLevel = 1;
	}

	return nErrorLevel = 0;
}

#endif
