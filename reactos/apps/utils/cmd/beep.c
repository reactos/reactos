/*
 *  BEEP.C - beep internal command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    14-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("beep /?").
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_BEEP

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


INT cmd_beep (LPTSTR cmd, LPTSTR param)
{
	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
		ConOutPuts (_T("Beep the speaker.\n\nBEEP"));
		return 0;
	}

#if 0
	/* check if run in batch mode */
	if (bc == NULL)
		return 1;
#endif

	MessageBeep (-1);

	return 0;
}
#endif
