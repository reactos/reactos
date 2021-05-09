/*
 *  GOTO.C - goto internal batch command.
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
 *    28 Jul 1998 (Hans B Pufal) [HBP_003]
 *        Terminate label on first space character, use only first 8 chars of
 *        label string
 *
 *    24-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    27-Jan-1999 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    28-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

/*
 * Perform GOTO command.
 *
 * Only valid when a batch context is active.
 */
INT cmd_goto(LPTSTR param)
{
    LPTSTR label, tmp;
    DWORD dwCurrPos;
    BOOL bRetry;

    TRACE("cmd_goto(\'%s\')\n", debugstr_aw(param));

    /*
     * Keep the help message handling here too.
     * This allows us to reproduce the Windows' CMD "bug"
     * (from a batch script):
     *
     * SET label=/?
     * CALL :%%label%%
     *
     * calls GOTO help, due to how CALL :label functionality
     * is internally implemented.
     *
     * See https://stackoverflow.com/q/31987023/13530036
     * for more details.
     *
     * Note that the choice of help parsing forbids
     * any label containing '/?' in it.
     */
    if (_tcsstr(param, _T("/?")))
    {
        ConOutResPaging(TRUE, STRING_GOTO_HELP1);
        return 0;
    }

    /* If not in batch, fail */
    if (bc == NULL)
        return 1;

    /* Fail if no label has been provided */
    if (*param == _T('\0'))
    {
        ConErrResPrintf(STRING_GOTO_ERROR1);
        ExitBatch();
        return 1;
    }

    /* Strip leading whitespace */
    while (_istspace(*param))
        ++param;

    /* Support jumping to the end of the file, only if extensions are enabled */
    if (bEnableExtensions &&
        (_tcsnicmp(param, _T(":EOF"), 4) == 0) &&
        (!param[4] || _istspace(param[4])))
    {
        /* Position at the end of the batch file */
        bc->mempos = bc->memsize;

        /* Do not process any more parts of a compound command */
        bc->current = NULL;
        return 0;
    }

    /* Skip the first colon or plus sign */
    if (*param == _T(':') || *param == _T('+'))
        ++param;
    /* Terminate the label at the first delimiter character */
    tmp = param;
    while (!_istcntrl(*tmp) && !_istspace(*tmp) &&
           !_tcschr(_T(":+"), *tmp) && !_tcschr(STANDARD_SEPS, *tmp) /* &&
           !_tcschr(_T("&|<>"), *tmp) */)
    {
        ++tmp;
    }
    *tmp = _T('\0');

    /* If we don't have any label, bail out */
    if (!*param)
        goto NotFound;

    /*
     * Search the next label starting our position, until the end of the file.
     * If none has been found, restart at the beginning of the file, and continue
     * until reaching back our old current position.
     */
    bRetry = FALSE;
    dwCurrPos = bc->mempos;

retry:
    while (BatchGetString(textline, ARRAYSIZE(textline)))
    {
        if (bRetry && (bc->mempos >= dwCurrPos))
            break;

#if 0
        /* If this is not a label, continue searching */
        if (!_tcschr(textline, _T(':')))
            continue;
#endif

        label = textline;

        /* A bug in Windows' CMD makes it always ignore the
         * first character of the line, unless it's a colon. */
        if (*label != _T(':'))
            ++label;

        /* Strip any leading whitespace */
        while (_istspace(*label))
            ++label;

        /* If this is not a label, continue searching */
        if (*label != _T(':'))
            continue;

        /* Skip the first colon or plus sign */
#if 0
        if (*label == _T(':') || *label == _T('+'))
            ++label;
#endif
        ++label;
        /* Strip any whitespace between the colon and the label */
        while (_istspace(*label))
            ++label;
        /* Terminate the label at the first delimiter character */
        tmp = label;
        while (!_istcntrl(*tmp) && !_istspace(*tmp) &&
               !_tcschr(_T(":+"), *tmp) && !_tcschr(STANDARD_SEPS, *tmp) &&
               !_tcschr(_T("&|<>"), *tmp))
        {
            /* Support the escape caret */
            if (*tmp == _T('^'))
            {
                /* Move the buffer back one character */
                memmove(tmp, tmp + 1, (_tcslen(tmp + 1) + 1) * sizeof(TCHAR));
                /* We will ignore the new character */
            }

            ++tmp;
        }
        *tmp = _T('\0');

        /* Jump if the labels are identical */
        if (_tcsicmp(label, param) == 0)
        {
            /* Do not process any more parts of a compound command */
            bc->current = NULL;
            return 0;
        }
    }
    if (!bRetry && (bc->mempos >= bc->memsize))
    {
        bRetry = TRUE;
        bc->mempos = 0;
        goto retry;
    }

NotFound:
    ConErrResPrintf(STRING_GOTO_ERROR2, param);
    ExitBatch();
    return 1;
}

/* EOF */
