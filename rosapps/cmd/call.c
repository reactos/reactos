/*
 *  CALL.C - call internal batch command.
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
 *    04-Aug-1998 (Hans B Pufal)
 *        added lines to initialize for pointers (HBP004)  This fixed the
 *        lock-up that happened sometimes when calling a batch file from
 *        another batch file.
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("call /?") and cleaned up.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


/*
 * Perform CALL command.
 *
 * Allocate a new batch context and add it to the current chain.
 * Call parsecommandline passing in our param string
 * If No batch file was opened then remove our newly allocted
 * context block.
 */

INT cmd_call (LPTSTR cmd, LPTSTR param)
{
	LPBATCH_CONTEXT n = NULL;

#ifdef _DEBUG
	DebugPrintf ("cmd_call: (\'%s\',\'%s\')\n", cmd, param);
#endif
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Calls one batch program from another.\n\n"
					   "CALL [drive:][path]filename [batch-parameter]\n\n"
					   "  batch-parameter  Specifies any command-line information required by the\n"
					   "                   batch program."));
		return 0;
	}

	n = (LPBATCH_CONTEXT)malloc (sizeof (BATCH_CONTEXT));

	if (n == NULL)
	{
		error_out_of_memory ();
		return 1;
	}

	n->prev = bc;
	bc = n;

	bc->hBatchFile = INVALID_HANDLE_VALUE;
	bc->params = NULL;
	bc->shiftlevel = 0;
	bc->forvar = 0;        /* HBP004 */
	bc->forproto = NULL;   /* HBP004 */

	ParseCommandLine (param);

	/* Wasn't a batch file so remove conext */
	if (bc->hBatchFile == INVALID_HANDLE_VALUE)
	{
		bc = bc->prev;
		free (n);
	}

	return 0;
}
