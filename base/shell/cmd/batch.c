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
 *        Separated commands into individual files.
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
 *    02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

/* The stack of current batch contexts.
 * NULL when no batch is active.
 */
BATCH_TYPE BatType = NONE;
PBATCH_CONTEXT bc = NULL;

#ifdef MSCMD_BATCH_ECHO
BOOL bBcEcho = TRUE;
#endif

BOOL bEcho = TRUE;  /* The echo flag */

/* Buffer for reading Batch file lines */
TCHAR textline[BATCH_BUFFSIZE];

/*
 * Returns a pointer to the n'th parameter of the current batch file.
 * If no such parameter exists returns pointer to empty string.
 * If no batch file is current, returns NULL.
 */
BOOL
FindArg(
    IN TCHAR Char,
    OUT PCTSTR* ArgPtr,
    OUT BOOL* IsParam0)
{
    PCTSTR pp;
    INT n = Char - _T('0');

    TRACE("FindArg: (%d)\n", n);

    *ArgPtr = NULL;

    if (n < 0 || n > 9)
        return FALSE;

    n = bc->shiftlevel[n];
    *IsParam0 = (n == 0);
    pp = bc->params;

    /* Step up the strings till we reach
     * the end or the one we want. */
    while (*pp && n--)
        pp += _tcslen(pp) + 1;

    *ArgPtr = pp;
    return TRUE;
}


/*
 * Builds the batch parameter list in newly allocated memory.
 * The parameters consist of NULL terminated strings with a
 * final NULL character signalling the end of the parameters.
 */
static BOOL
BatchParams(
    IN PCTSTR Arg0,
    IN PCTSTR Args,
    OUT PTSTR* RawParams,
    OUT PTSTR* ParamList)
{
    PTSTR dp;
    SIZE_T len;

    *RawParams = NULL;
    *ParamList = NULL;

    /* Make a raw copy of the parameters, but trim any leading and trailing whitespace */
    // Args += _tcsspn(Args, _T(" \t"));
    while (_istspace(*Args))
        ++Args;
    dp = (PTSTR)Args + _tcslen(Args);
    while ((dp > Args) && _istspace(*(dp - 1)))
        --dp;
    len = dp - Args;
    *RawParams = (PTSTR)cmd_alloc((len + 1)* sizeof(TCHAR));
    if (!*RawParams)
    {
        WARN("Cannot allocate memory for RawParams!\n");
        error_out_of_memory();
        return FALSE;
    }
    _tcsncpy(*RawParams, Args, len);
    (*RawParams)[len] = _T('\0');

    /* Parse the parameters as well */
    Args = *RawParams;

    *ParamList = (PTSTR)cmd_alloc((_tcslen(Arg0) + _tcslen(Args) + 3) * sizeof(TCHAR));
    if (!*ParamList)
    {
        WARN("Cannot allocate memory for ParamList!\n");
        error_out_of_memory();
        cmd_free(*RawParams);
        *RawParams = NULL;
        return FALSE;
    }

    dp = *ParamList;

    if (Arg0 && *Arg0)
    {
        dp = _stpcpy(dp, Arg0);
        *dp++ = _T('\0');
    }

    while (*Args)
    {
        BOOL inquotes = FALSE;

        /* Find next parameter */
        while (_istspace(*Args) || (*Args && _tcschr(STANDARD_SEPS, *Args)))
            ++Args;
        if (!*Args)
            break;

        /* Copy it */
        do
        {
            if (!inquotes && (_istspace(*Args) || _tcschr(STANDARD_SEPS, *Args)))
                break;
            inquotes ^= (*Args == _T('"'));
            *dp++ = *Args++;
        } while (*Args);
        *dp++ = _T('\0');
    }
    *dp = _T('\0');

    return TRUE;
}

/*
 * Free the allocated memory of a batch file.
 */
static VOID ClearBatch(VOID)
{
    TRACE("ClearBatch  mem = %08x ; free = %d\n", bc->mem, bc->memfree);

    if (bc->mem && bc->memfree)
        cmd_free(bc->mem);

    if (bc->raw_params)
        cmd_free(bc->raw_params);

    if (bc->params)
        cmd_free(bc->params);
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

VOID ExitBatch(VOID)
{
    ClearBatch();

    TRACE("ExitBatch\n");

    UndoRedirection(bc->RedirList, NULL);
    FreeRedirection(bc->RedirList);

#ifndef MSCMD_BATCH_ECHO
    /* Preserve echo state across batch calls */
    bEcho = bc->bEcho;
#endif

    while (bc->setlocal)
        cmd_endlocal(_T(""));

    bc = bc->prev;

#if 0
    /* Do not process any more parts of a compound command */
    bc->current = NULL;
#endif

    /* If there is no more batch contexts, notify the signal handler */
    if (!bc)
    {
        CheckCtrlBreak(BREAK_OUTOFBATCH);
        BatType = NONE;

#ifdef MSCMD_BATCH_ECHO
        bEcho = bBcEcho;
#endif
    }
}

/*
 * Exit all the nested batch calls.
 */
VOID ExitAllBatches(VOID)
{
    while (bc)
        ExitBatch();
}

/*
 * Load batch file into memory.
 */
static void BatchFile2Mem(HANDLE hBatchFile)
{
    TRACE("BatchFile2Mem()\n");

    bc->memsize = GetFileSize(hBatchFile, NULL);
    bc->mem     = (char *)cmd_alloc(bc->memsize+1);     /* 1 extra for '\0' */

    /* if memory is available, read it in and close the file */
    if (bc->mem != NULL)
    {
        TRACE ("BatchFile2Mem memory %08x - %08x\n",bc->mem,bc->memsize);
        SetFilePointer (hBatchFile, 0, NULL, FILE_BEGIN);
        ReadFile(hBatchFile, (LPVOID)bc->mem, bc->memsize,  &bc->memsize, NULL);
        bc->mem[bc->memsize]='\0';  /* end this, so you can dump it as a string */
        bc->memfree=TRUE;           /* this one needs to be freed */
    }
    else
    {
        bc->memsize=0;              /* this will prevent mem being accessed */
        bc->memfree=FALSE;
    }
    bc->mempos = 0;                 /* set position to the start */
}

/*
 * Start batch file execution.
 *
 * The firstword parameter is the full filename of the batch file.
 */
INT Batch(LPTSTR fullname, LPTSTR firstword, LPTSTR param, PARSED_COMMAND *Cmd)
{
    INT ret = 0;
    INT i;
    HANDLE hFile = NULL;
    BOOLEAN bSameFn = FALSE;
    BOOLEAN bTopLevel;
    BATCH_CONTEXT new;
    PFOR_CONTEXT saved_fc;

    SetLastError(0);
    if (bc && bc->mem)
    {
        TCHAR fpname[MAX_PATH];
        GetFullPathName(fullname, ARRAYSIZE(fpname), fpname, NULL);
        if (_tcsicmp(bc->BatchFilePath, fpname) == 0)
            bSameFn = TRUE;
    }
    TRACE("Batch(\'%s\', \'%s\', \'%s\')  bSameFn = %d\n",
        debugstr_aw(fullname), debugstr_aw(firstword), debugstr_aw(param), bSameFn);

    if (!bSameFn)
    {
        hFile = CreateFile(fullname, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
                           FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            ConErrResPuts(STRING_BATCH_ERROR);
            return 1;
        }
    }

    /*
     * Remember whether this is a top-level batch context, i.e. if there is
     * no batch context existing prior (bc == NULL originally), and we are
     * going to create one below.
     */
    bTopLevel = !bc;

    if (bc != NULL && Cmd == bc->current)
    {
        /* Then we are transferring to another batch */
        ClearBatch();
        AddBatchRedirection(&Cmd->Redirections);
    }
    else
    {
        struct _SETLOCAL *setlocal = NULL;

        if (Cmd == NULL)
        {
            /* This is a CALL. CALL will set errorlevel to our return value, so
             * in order to keep the value of errorlevel unchanged in the case
             * of calling an empty batch file, we must return that same value. */
            ret = nErrorLevel;
        }
        else if (bc)
        {
            /* If a batch file runs another batch file as part of a compound command
             * (e.g. "x.bat & somethingelse") then the first file gets terminated. */

            /* Get its SETLOCAL stack so it can be migrated to the new context */
            setlocal = bc->setlocal;
            bc->setlocal = NULL;
            ExitBatch();
        }

        /* Create a new context. This function will not
         * return until this context has been exited */
        new.prev = bc;
        /* copy some fields in the new structure if it is the same file */
        if (bSameFn)
        {
            new.mem     = bc->mem;
            new.memsize = bc->memsize;
            new.mempos  = 0;
            new.memfree = FALSE;    /* don't free this, being used before this */
        }
        bc = &new;
        bc->RedirList = NULL;
        bc->setlocal = setlocal;
    }

    GetFullPathName(fullname, ARRAYSIZE(bc->BatchFilePath), bc->BatchFilePath, NULL);

    /* If a new batch file, load it into memory and close the file */
    if (!bSameFn)
    {
        BatchFile2Mem(hFile);
        CloseHandle(hFile);
    }

    bc->mempos = 0;    /* Go to the beginning of the batch file */
#ifndef MSCMD_BATCH_ECHO
    bc->bEcho = bEcho; /* Preserve echo across batch calls */
#endif
    for (i = 0; i < 10; i++)
        bc->shiftlevel[i] = i;

    /* Parse the batch parameters */
    if (!BatchParams(firstword, param, &bc->raw_params, &bc->params))
        return 1;

    /* If we are calling from inside a FOR, hide the FOR variables */
    saved_fc = fc;
    fc = NULL;

    /* Perform top-level batch initialization */
    if (bTopLevel)
    {
        TCHAR *dot;

        /* Default the top-level batch context type to .BAT */
        BatType = BAT_TYPE;

        /* If this is a .CMD file, adjust the type */
        dot = _tcsrchr(bc->BatchFilePath, _T('.'));
        if (dot && (!_tcsicmp(dot, _T(".cmd"))))
        {
            BatType = CMD_TYPE;
        }

#ifdef MSCMD_BATCH_ECHO
        bBcEcho = bEcho;
#endif
    }

    /* If this is a "CALL :label args ...", call a subroutine of
     * the current batch file, only if extensions are enabled. */
    if (bEnableExtensions && (*firstword == _T(':')))
    {
        LPTSTR expLabel;

        /* Position at the place of the parent file (which is the same as the caller) */
        bc->mempos = (bc->prev ? bc->prev->mempos : 0);

        /*
         * Jump to the label. Strip the label's colon; as a side-effect
         * this will forbid "CALL :EOF"; however "CALL ::EOF" will work!
         */
        bc->current = Cmd;
        ++firstword;

        /* Expand the label only! (simulate a GOTO command as in Windows' CMD) */
        expLabel = DoDelayedExpansion(firstword);
        ret = cmd_goto(expLabel ? expLabel : firstword);
        if (expLabel)
            cmd_free(expLabel);
    }

    /* If we have created a new context, don't return
     * until this batch file has completed. */
    while (bc == &new && !bExit)
    {
        Cmd = ParseCommand(NULL);
        if (!Cmd)
        {
            if (!bParseError)
                continue;

            /* Echo the pre-parsed batch file line on error */
            if (bEcho && !bDisableBatchEcho)
            {
                if (!bIgnoreEcho)
                    ConOutChar(_T('\n'));
                PrintPrompt();
                ConOutPuts(ParseLine);
                ConOutChar(_T('\n'));
            }
            /* Stop all execution */
            ExitAllBatches();
            ret = 1;
            break;
        }

        /* JPP 19980807 */
        /* Echo the command and execute it */
        bc->current = Cmd;
        ret = ExecuteCommandWithEcho(Cmd);
        FreeCommand(Cmd);
    }
    if (bExit)
    {
        /* Stop all execution */
        ExitAllBatches();
    }

    /* Perform top-level batch cleanup */
    if (!bc || bTopLevel)
    {
        /* Reset the top-level batch context type */
        BatType = NONE;

#ifdef MSCMD_BATCH_ECHO
        bEcho = bBcEcho;
#endif
    }

    /* Restore the FOR variables */
    fc = saved_fc;

    /* Always return the last command's return code */
    TRACE("Batch: returns %d\n", ret);
    return ret;
}

VOID AddBatchRedirection(REDIRECTION **RedirList)
{
    REDIRECTION **ListEnd;

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
 *   Read a single line from the batch file from the current batch/memory position.
 *   Almost a copy of FileGetString with same UNICODE handling
 */
BOOL BatchGetString(LPTSTR lpBuffer, INT nBufferLength)
{
    INT len = 0;

    /* read all chars from memory until a '\n' is encountered */
    if (bc->mem)
    {
        for (; ((bc->mempos + len) < bc->memsize  &&  len < (nBufferLength-1)); len++)
        {
#ifndef _UNICODE
            lpBuffer[len] = bc->mem[bc->mempos + len];
#endif
            if (bc->mem[bc->mempos + len] == '\n')
            {
                len++;
                break;
            }
        }
#ifdef _UNICODE
        nBufferLength = MultiByteToWideChar(OutputCodePage, 0, &bc->mem[bc->mempos], len, lpBuffer, nBufferLength);
        lpBuffer[nBufferLength] = L'\0';
        lpBuffer[len] = '\0';
#endif
        bc->mempos += len;
    }

    return len != 0;
}

/*
 * Read and return the next executable line form the current batch file
 *
 * If no batch file is current or no further executable lines are found
 * return NULL.
 *
 * Set eflag to 0 if line is not to be echoed else 1
 */
LPTSTR ReadBatchLine(VOID)
{
    TRACE("ReadBatchLine()\n");

    /* User halt */
    if (CheckCtrlBreak(BREAK_BATCHFILE))
    {
        ExitAllBatches();
        return NULL;
    }

    if (!BatchGetString(textline, ARRAYSIZE(textline) - 1))
    {
        TRACE("ReadBatchLine(): Reached EOF!\n");
        /* End of file */
        ExitBatch();
        return NULL;
    }

    TRACE("ReadBatchLine(): textline: \'%s\'\n", debugstr_aw(textline));

#if 1
    //
    // FIXME: This is redundant, but keep it for the moment until we correctly
    // hande the end-of-file situation here, in ReadLine() and in the parser.
    // (In an EOF, the previous BatchGetString() call will return FALSE but
    // we want not to run the ExitBatch() at first, but wait later to do it.)
    //
    if (textline[_tcslen(textline) - 1] != _T('\n'))
        _tcscat(textline, _T("\n"));
#endif

    return textline;
}

/* EOF */
