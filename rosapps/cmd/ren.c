/*
 *  REN.C - rename internal command.
 *
 *
 *  History:
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *        Added support for quoted long file names with spaces.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *        Unicode and redirection safe!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef INCLUDE_CMD_RENAME

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"

/*
 *  simple file rename internal command.
 *
 */
INT cmd_rename (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg;
	INT    argc;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Renames a file/directory or files/directories.\n"
					   "\n"
					   "RENAME [drive:][path][directoryname1 | filename1] [directoryname2 | filename2]\n"
					   "REN [drive:][path][directoryname1 | filename1] [directoryname2 | filename2]\n"
					   "\n"
					   "Note that you cannot specify a new drive or path for your destination. Use\n"
					   "the MOVE command for that purpose."));
		return 0;
	}

	/* split the argument list */
	arg = split (param, &argc);

	if (argc != 2)
	{
		freep (arg);
		error_too_many_parameters (param);
		return 1;
	}

	if (!MoveFile (arg[0], arg[1]))
	{
		ConErrPuts (_T("rename"));
		freep (arg);
		return 1;
	}

	freep (arg);

	return 0;
}

#endif
