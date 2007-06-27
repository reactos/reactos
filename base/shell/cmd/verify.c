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
#include "resource.h"

#ifdef INCLUDE_CMD_VERIFY


/* global verify flag */
static BOOL bVerify = FALSE;


INT cmd_verify (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_VERIFY_HELP1);
		return 0;
	}

  nErrorLevel = 0;

	if (!*param)
	{
		LoadString(CMD_ModuleHandle, STRING_VERIFY_HELP2, szMsg, RC_STRING_MAX_SIZE);
		ConOutPrintf(szMsg, bVerify ? D_ON : D_OFF);
	}
	else if (_tcsicmp (param, D_OFF) == 0)
		bVerify = FALSE;
	else if (_tcsicmp (param, D_ON) == 0)
		bVerify = TRUE;
	else
	{
		ConOutResPuts(STRING_VERIFY_HELP3);
	}

	return 0;
}

#endif
