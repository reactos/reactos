/*
 *  BATCH.C - batch file processor for CMD.EXE.
 *
 *
 *  History:
 *
 *    ??/??/?? (Evan Jeffrey)
 *        started.
 *
 *    15 Jul 1995 (Tim Norman)
 *        modes and bugfixes.
 *
 *    08 Aug 1995 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this
 *        source into guidelines for recommended programming practice.
 *
 *        i have added some constants to help making changes easier.
 *
 *    29 Jan 1996 (Steffan Kaiser)
 *        made a few cosmetic changes
 *
 *    05 Feb 1996 (Tim Norman)
 *        changed to comply with new first/rest calling scheme
 *
 *    14 Jun 1997 (Steffen Kaiser)
 *        bug fixes.  added error level expansion %?.  ctrl-break handling
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        Totally reorganised in conjunction with COMMAND.C (cf) to
 *        implement proper BATCH file nesting and other improvements.
 *
 *    16 Jul 1998 (John P Price <linux-guru@gcfl.net>)
 *        Seperated commands into individual files.
 *
 *    19 Jul 1998 (Hans B Pufal) [HBP_001]
 *        Preserve state of echo flag across batch calls.
 *
 *    19 Jul 1998 (Hans B Pufal) [HBP_002]
 *        Implementation of FOR command
 *
 *    20-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added error checking after cmd_alloc calls
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    02-Aug-1998 (Hans B Pufal) [HBP_003]
 *        Fixed bug in ECHO flag restoration at exit from batch file
 *
 *    26-Jan-1999 Eric Kohl
 *        Replaced CRT io functions by Win32 io functions.
 *        Unicode safe!
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.es>)
 *        Fixes made to get "for" working.
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>


/* The stack of current batch contexts.
 * NULL when no batch is active
 */
LPBATCH_CONTEXT bc = NULL;

BOOL bEcho = TRUE;          /* The echo flag */



/* Buffer for reading Batch file lines */
TCHAR textline[BATCH_BUFFSIZE];


/*
 * Returns a pointer to the n'th parameter of the current batch file.
 * If no such parameter exists returns pointer to empty string.
 * If no batch file is current, returns NULL
 *
 */

LPTSTR FindArg (INT n)
{
	LPTSTR pp;

	TRACE ("FindArg: (%d)\n", n);

	if (bc == NULL)
		return NULL;

	n += bc->shiftlevel;
	pp = bc->params;

	/* Step up the strings till we reach the end */
	/* or the one we want */
	while (*pp && n--)
		pp += _tcslen (pp) + 1;

	return pp;
}


/*
 * Batch_params builds a parameter list in newlay allocated memory.
 * The parameters consist of null terminated strings with a final
 * NULL character signalling the end of the parameters.
 *
*/

LPTSTR BatchParams (LPTSTR s1, LPTSTR s2)
{
	LPTSTR dp = (LPTSTR)cmd_alloc ((_tcslen(s1) + _tcslen(s2) + 3) * sizeof (TCHAR));

	/* JPP 20-Jul-1998 added error checking */
	if (dp == NULL)
	{
		error_out_of_memory();
		return NULL;
	}

	if (s1 && *s1)
	{
		s1 = _stpcpy (dp, s1);
		*s1++ = _T('\0');
	}
	else
		s1 = dp;

	while (*s2)
	{
		BOOL inquotes = FALSE;

		/* Find next parameter */
		while (_istspace(*s2) || (*s2 && _tcschr(_T(",;="), *s2)))
			s2++;
		if (!*s2)
			break;

		/* Copy it */
		do
		{
			if (!inquotes && (_istspace(*s2) || _tcschr(_T(",;="), *s2)))
				break;
			inquotes ^= (*s2 == _T('"'));
			*s1++ = *s2++;
		} while (*s2);
		*s1++ = _T('\0');
	}

	*s1 = _T('\0');

	return dp;
}


/*
 * If a batch file is current, exits it, freeing the context block and
 * chaining back to the previous one.
 *
 * If no new batch context is found, sets ECHO back ON.
 *
 * If the parameter is non-null or not empty, it is printed as an exit
 * message
 */

VOID ExitBatch (LPTSTR msg)
{
	TRACE ("ExitBatch: (\'%s\')\n", debugstr_aw(msg));

	if (bc != NULL)
	{
		if (bc->hBatchFile)
		{
			CloseHandle (bc->hBatchFile);
			bc->hBatchFile = INVALID_HANDLE_VALUE;
		}

		if (bc->raw_params)
			cmd_free(bc->raw_params);

		if (bc->params)
			cmd_free(bc->params);

		UndoRedirection(bc->RedirList, NULL);
		FreeRedirection(bc->RedirList);

		/* Preserve echo state across batch calls */
		bEcho = bc->bEcho;

		bc = bc->prev;
	}

	if (msg && *msg)
		ConOutPrintf (_T("%s\n"), msg);
}


/*
 * Start batch file execution
 *
 * The firstword parameter is the full filename of the batch file.
 *
 */

BOOL Batch (LPTSTR fullname, LPTSTR firstword, LPTSTR param, PARSED_COMMAND *Cmd)
{
	BATCH_CONTEXT new;

	HANDLE hFile;
	SetLastError(0);
	hFile = CreateFile (fullname, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
			    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
				 FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	TRACE ("Batch: (\'%s\', \'%s\', \'%s\')  hFile = %x\n",
		debugstr_aw(fullname), debugstr_aw(firstword), debugstr_aw(param), hFile);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		ConErrResPuts(STRING_BATCH_ERROR);
		return FALSE;
	}

	/* Kill any and all FOR contexts */
	fc = NULL;

	if (bc != NULL && Cmd == bc->current)
	{
		/* Then we are transferring to another batch */
		CloseHandle (bc->hBatchFile);
		bc->hBatchFile = INVALID_HANDLE_VALUE;
		if (bc->params)
			cmd_free (bc->params);
		if (bc->raw_params)
			cmd_free (bc->raw_params);
		AddBatchRedirection(&Cmd->Redirections);
	}
	else
	{
		/* If a batch file runs another batch file as part of a compound command
		 * (e.g. "x.bat & somethingelse") then the first file gets terminated. */
		if (Cmd != NULL)
			ExitBatch(NULL);

		/* Create a new context. This function will not
		 * return until this context has been exited */
		new.prev = bc;
		bc = &new;
		bc->RedirList = NULL;
	}

	GetFullPathName(fullname, sizeof(bc->BatchFilePath) / sizeof(TCHAR), bc->BatchFilePath, NULL);

	bc->hBatchFile = hFile;
	SetFilePointer (bc->hBatchFile, 0, NULL, FILE_BEGIN);
	bc->bEcho = bEcho; /* Preserve echo across batch calls */
	bc->shiftlevel = 0;
	
	bc->params = BatchParams (firstword, param);
    //
    // Allocate enough memory to hold the params and copy them over without modifications
    //
    bc->raw_params = cmd_dup(param);
    if (bc->raw_params == NULL)
    {
        error_out_of_memory();
        return FALSE;
    }

	/* Check if this is a "CALL :label" */
	if (*firstword == _T(':'))
		cmd_goto(firstword);

	/* If we have created a new context, don't return
	 * until this batch file has completed. */
	while (bc == &new && !bExit)
	{
		Cmd = ParseCommand(NULL);
		if (!Cmd)
			continue;

		/* JPP 19980807 */
		/* Echo batch file line */
		if (bEcho && Cmd->Type != C_QUIET)
		{
			ConOutChar(_T('\n'));
			PrintPrompt();
			EchoCommand(Cmd);
			ConOutChar(_T('\n'));
		}

		bc->current = Cmd;
		ExecuteCommand(Cmd);
		FreeCommand(Cmd);
	}

	TRACE ("Batch: returns TRUE\n");

	return TRUE;
}

VOID AddBatchRedirection(REDIRECTION **RedirList)
{
	REDIRECTION **ListEnd;

	if(!bc)
		return;

	/* Prepend the list to the batch context's list */
	ListEnd = RedirList;
	while (*ListEnd)
		ListEnd = &(*ListEnd)->Next;
	*ListEnd = bc->RedirList;
	bc->RedirList = *RedirList;

	/* Null out the pointer so that the list will not be cleared prematurely.
	 * These redirections should persist until the batch file exits. */
	*RedirList = NULL;
}

/*
 * Read and return the next executable line form the current batch file
 *
 * If no batch file is current or no further executable lines are found
 * return NULL.
 *
 * Set eflag to 0 if line is not to be echoed else 1
 */

LPTSTR ReadBatchLine ()
{
	/* No batch */
	if (bc == NULL)
		return NULL;

	TRACE ("ReadBatchLine ()\n");

	/* User halt */
	if (CheckCtrlBreak (BREAK_BATCHFILE))
	{
		while (bc)
			ExitBatch (NULL);
		return NULL;
	}

	if (!FileGetString (bc->hBatchFile, textline, sizeof (textline) / sizeof (textline[0]) - 1))
	{
		TRACE ("ReadBatchLine(): Reached EOF!\n");
		/* End of file.... */
		ExitBatch (NULL);
		return NULL;
	}

	TRACE ("ReadBatchLine(): textline: \'%s\'\n", debugstr_aw(textline));

	if (textline[_tcslen(textline) - 1] != _T('\n'))
		_tcscat(textline, _T("\n"));

	return textline;
}

/* EOF */
