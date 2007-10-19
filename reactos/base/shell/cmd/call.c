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
 *    07-Jan-1999 (Eric Kohl)
 *        Added help text ("call /?") and cleaned up.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>


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
	DebugPrintf (_T("cmd_call: (\'%s\',\'%s\')\n"), cmd, param);
#endif
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_CALL_HELP);
		return 0;
	}

    nErrorLevel = 0;

	n = (LPBATCH_CONTEXT)cmd_alloc (sizeof (BATCH_CONTEXT));

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
	if (bc->prev)
	{
		_tcscpy(bc->In, bc->prev->In);
		_tcscpy(bc->Out, bc->prev->Out);
		_tcscpy(bc->Err, bc->prev->Err);
	}
	else
	{
		bc->In[0] = _T('\0');
		bc->Out[0] = _T('\0');
		bc->Err[0] = _T('\0');
	}


	/* Wasn't a batch file so remove conext */
	if (bc->hBatchFile == INVALID_HANDLE_VALUE)
	{
		bc = bc->prev;
		cmd_free (n);
	}

	return 0;
}

/* EOF */
