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
 *        Separated commands into individual files.
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
 *    02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

/* Enable this define for "buggy" Windows' CMD CALL-command compatibility */
#define MSCMD_CALL_QUIRKS


/*
 * Perform CALL command.
 */
INT cmd_call(LPTSTR param)
{
    PARSED_COMMAND* Cmd = NULL;
    BOOL bOldIgnoreParserComments;
#ifndef MSCMD_CALL_QUIRKS
    BOOL bOldHandleContinuations;
#else
    SIZE_T nNumCarets;
#endif
    PTSTR first;

    TRACE("cmd_call(\'%s\')\n", debugstr_aw(param));

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_CALL_HELP);
        return 0;
    }

    /* Fail if no command or label has been provided */
    if (*param == _T('\0'))
        return (nErrorLevel = 1);

    /* Ignore parser comments (starting with a colon) */
    bOldIgnoreParserComments = bIgnoreParserComments;
    bIgnoreParserComments = FALSE;

#ifndef MSCMD_CALL_QUIRKS
    /* Disable parsing of escape carets */
    bOldHandleContinuations = bHandleContinuations;
    bHandleContinuations = FALSE;
    first = param;
#else
    /*
     * As the original escape carets have been dealt with during the first
     * command parsing step, the remaining ones need to be doubled so that
     * they can again survive the new parsing step done below.
     * But do it the Windows' CMD "buggy" way: **all** carets are doubled,
     * even those inside quotes. However, this causes carets inside quotes
     * to remain doubled after the parsing step...
     */

    /* Count all the carets */
    nNumCarets = 0;
    first = param;
    while (first)
    {
        first = _tcschr(first, _T('^'));
        if (first)
        {
            ++nNumCarets;
            ++first;
        }
    }

    /* Re-allocate a large enough parameter string if needed */
    if (nNumCarets > 0)
    {
        PTCHAR Src, Dest, End;

        // TODO: Improvement: Use the scratch TempBuf if the string is not too long.
        first = cmd_alloc((_tcslen(param) + nNumCarets + 1) * sizeof(TCHAR));
        if (!first)
        {
            WARN("Cannot allocate memory for new CALL parameter string!\n");
            error_out_of_memory();
            return (nErrorLevel = 1);
        }

        /* Copy the parameter string and double the escape carets */
        Src = param;
        Dest = first;
        while (*Src)
        {
            if (*Src != _T('^'))
            {
                /* Copy everything before the next caret (or the end of the string) */
                End = _tcschr(Src, _T('^'));
                if (!End)
                    End = Src + _tcslen(Src);
                memcpy(Dest, Src, (End - Src) * sizeof(TCHAR));
                Dest += End - Src;
                Src = End;
                continue;
            }

            /* Copy the original caret and double it */
            *Dest++ = *Src;
            *Dest++ = *Src++;
        }
        *Dest = _T('\0');
    }
    else
    {
        first = param;
    }
#endif

    /*
     * Reparse the CALL parameter string as a command.
     * Note that this will trigger a second round of %-variable substitutions.
     */
    Cmd = ParseCommand(first);

    /* Restore the global parsing state */
#ifndef MSCMD_CALL_QUIRKS
    bHandleContinuations = bOldHandleContinuations;
#endif
    bIgnoreParserComments = bOldIgnoreParserComments;

    /*
     * If no command is there, yet no error occurred, this means that
     * a whitespace label was given. Do not consider this as a failure.
     */
    if (!Cmd && !bParseError)
    {
#ifdef MSCMD_CALL_QUIRKS
        if (first != param)
            cmd_free(first);
#endif
        return (nErrorLevel = 0);
    }

    /* Reset bParseError so as to continue running afterwards */
    bParseError = FALSE;

    /*
     * Otherwise, if no command is there because a parse error occurred,
     * or if this an unsupported command: not a standard one, including
     * FOR and IF, fail and bail out.
     */
    if (!Cmd || (Cmd->Type == C_FOR) || (Cmd->Type == C_IF) ||
        ((Cmd->Type != C_COMMAND) && (Cmd->Type != C_REM)))
    {
        // FIXME: Localize
        ConErrPrintf(_T("%s was unexpected.\n"), first);

#ifdef MSCMD_CALL_QUIRKS
        if (first != param)
            cmd_free(first);
#endif
        if (Cmd) FreeCommand(Cmd);
        return (nErrorLevel = 1);
    }

#ifdef MSCMD_CALL_QUIRKS
    if (first != param)
        cmd_free(first);
#endif

    first = Cmd->Command.First;
    param = Cmd->Command.Rest;

    /* "CALL :label args ..." - Call a subroutine of the current batch file, only if extensions are enabled */
    if (bEnableExtensions && (*first == _T(':')))
    {
        INT ret;

        /* A batch context must be present */
        if (!bc)
        {
            // FIXME: Localize
            ConErrPuts(_T("Invalid attempt to call batch label outside of batch script.\n"));
            FreeCommand(Cmd);
            return (nErrorLevel = 1);
        }

        ret = Batch(bc->BatchFilePath, first, param, NULL);
        nErrorLevel = (ret != 0 ? ret : nErrorLevel);
    }
    else
    {
        nErrorLevel = DoCommand(first, param, NULL);
    }

    FreeCommand(Cmd);
    return nErrorLevel;
}

/* EOF */
