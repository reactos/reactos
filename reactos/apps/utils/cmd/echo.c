/*
 *  ECHO.C - echo internal command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        Started.
 *
 *    16 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added config.h include
 *
 *    08-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection ready!
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


INT cmd_echo (LPTSTR cmd, LPTSTR param)
{
#ifdef _DEBUG
	DebugPrintf ("cmd_echo '%s' : '%s'\n", cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts ("Displays messages or switches command echoing on or off.\n\n"
				   "ECHO [ON | OFF]\nECHO [message]\n\n"
				   "Type ECHO without a parameter to display the current ECHO setting.");
		return 0;
	}

	if (_tcsicmp (param, D_OFF) == 0)
		bEcho = FALSE;
	else if (_tcsicmp (param, D_ON) == 0)
		bEcho = TRUE;
	else if (*param)
		ConOutPuts (param);
	else
		ConOutPrintf (_T("ECHO is %s\n"), bEcho ? D_ON : D_OFF);

	return 0;
}
