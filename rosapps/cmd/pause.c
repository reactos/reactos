/*
 *  PAUSE.C - pause internal command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Seperated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_PAUSE

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


/*
 * Perform PAUSE command.
 *
 * FREEDOS extension : If parameter is specified use that as the pause
 *   message.
 *
 * ?? Extend to include functionality of CHOICE if switch chars
 *     specified.
 */

INT cmd_pause (LPTSTR cmd, LPTSTR param)
{
#ifdef _DEBUG
	DebugPrintf ("cmd_pause: \'%s\' : \'%s\'\n", cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Stops the execution of a batch file and shows the following message:\n"
					   "\"Press any key to continue...\" or a user defined message.\n\n"
					   "PAUSE [message]"));
		return 0;
	}

	if (*param)
		ConOutPrintf (param);
	else
		msg_pause ();

	cgetchar ();

	return 0;
}

#endif
