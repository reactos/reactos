/*
 *  VERIFY.C - verify internal command.
 *
 *
 *  History:
 *
 *    31 Jul 1998 (John P Price)
 *        started.
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        VERIFY is just a dummy under Win32; it only exists
 *        for compatibility!!!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection ready!
 */

#include "config.h"

#ifdef INCLUDE_CMD_VERIFY

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"


/* global verify flag */
static BOOL bVerify = FALSE;


INT cmd_verify (LPTSTR cmd, LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("This command is just a dummy!!\n"
					   "Sets whether to verify that your files are written correctly to a\n"
					   "disk.\n\n"
					   "VERIFY [ON | OFF]\n\n"
					   "Type VERIFY without a parameter to display the current VERIFY setting."));
		return 0;
	}

	if (!*param)
		ConOutPrintf (_T("VERIFY is %s.\n"), bVerify ? D_ON : D_OFF);
	else if (_tcsicmp (param, D_OFF) == 0)
		bVerify = FALSE;
	else if (_tcsicmp (param, D_ON) == 0)
		bVerify = TRUE;
	else
		ConOutPuts (_T("Must specify ON or OFF."));

	return 0;
}

#endif
