/*
 *  SHIFT.C - shift internal batch command
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
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("shift /?") and cleaned up.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


/*
 *  Perform the SHIFT command.
 *
 * Only valid inside batch files.
 *
 * FREEDOS extension : optional parameter DOWN to allow shifting
 *   parameters backwards.
 */

INT cmd_shift (LPTSTR cmd, LPTSTR param)
{
#ifdef _DEBUG
	DebugPrintf ("cmd_shift: (\'%s\', \'%s\'\n", cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Changes the position of replaceable parameters in a batch file.\n\n"
					   "SHIFT [DOWN]"));
		return 0;
	}

	if (bc == NULL)
	{
		/* not in batch - error!! */
		return 1;
	}

	if (!_tcsicmp (param, _T("down")))
	{
		if (bc->shiftlevel)
			bc->shiftlevel--;
	}
	else /* shift up */
		bc->shiftlevel++;

	return 0;
}
