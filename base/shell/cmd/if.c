/*
 *  IF.C - if internal batch command.
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
 *    07-Jan-1999 (Eric Kohl)
 *        Added help text ("if /?") and cleaned up.
 *
 *    21-Jan-1999 (Eric Kohl)
 *        Unicode and redirection ready!
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Fixed help text.
 *
 *    17-Feb-2001 (ea)
 *        IF DEFINED variable command
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>
 *        Remove all hardcoded strings in En.rc
 *
 */

#include "precomp.h"

static INT GenericCmp(INT (WINAPI *StringCmp)(LPCTSTR, LPCTSTR),
                      LPCTSTR Left, LPCTSTR Right)
{
    TCHAR *end;
    INT nLeft = _tcstol(Left, &end, 0);
    if (*end == _T('\0'))
    {
        INT nRight = _tcstol(Right, &end, 0);
        if (*end == _T('\0'))
        {
            /* both arguments are numeric */
            return (nLeft < nRight) ? -1 : (nLeft > nRight);
        }
    }
    return StringCmp(Left, Right);
}

INT cmd_if(LPTSTR param)
{
    TRACE("cmd_if(\'%s\')\n", debugstr_aw(param));

    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_IF_HELP1);
        return 0;
    }

    ParseErrorEx(param);
    return 1;
}

INT ExecuteIf(PARSED_COMMAND *Cmd)
{
    INT result = FALSE; /* when set cause 'then' clause to be executed */
    LPTSTR param;
    LPTSTR Left = NULL, Right;

    if (Cmd->If.LeftArg)
    {
        Left = DoDelayedExpansion(Cmd->If.LeftArg);
        if (!Left)
            return 1;
    }
    Right = DoDelayedExpansion(Cmd->If.RightArg);
    if (!Right)
    {
        if (Left) cmd_free(Left);
        return 1;
    }

    if (bEnableExtensions && (Cmd->If.Operator == IF_CMDEXTVERSION))
    {
        /* IF CMDEXTVERSION n: check if Command Extensions
         * version is greater or equal to n. */
        DWORD n = _tcstoul(Right, &param, 10);
        if (*param != _T('\0'))
        {
            error_syntax(Right);
            goto fail;
        }
        result = (CMDEXTVERSION >= n);
    }
    else if (bEnableExtensions && (Cmd->If.Operator == IF_DEFINED))
    {
        /* IF DEFINED var: check if environment variable exists */
        result = (GetEnvVarOrSpecial(Right) != NULL);
    }
    else if (Cmd->If.Operator == IF_ERRORLEVEL)
    {
        /* IF ERRORLEVEL n: check if last exit code is greater or equal to n */
        INT n = _tcstol(Right, &param, 10);
        if (*param != _T('\0'))
        {
            error_syntax(Right);
            goto fail;
        }
        result = (nErrorLevel >= n);
    }
    else if (Cmd->If.Operator == IF_EXIST)
    {
        BOOL IsDir;
        SIZE_T Size;
        WIN32_FIND_DATA f;
        HANDLE hFind;
        DWORD attrs;

        /* IF EXIST filename: check if file exists (wildcards allowed) */
        StripQuotes(Right);

        Size = _tcslen(Right);
        IsDir = (Right[Size - 1] == '\\');
        if (IsDir)
            Right[Size - 1] = 0;

        hFind = FindFirstFile(Right, &f);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            attrs = f.dwFileAttributes;
            FindClose(hFind);
        }
        else
        {
            /* FindFirstFile fails at the root directory. */
            attrs = GetFileAttributes(Right);
        }

        if (attrs == INVALID_FILE_ATTRIBUTES)
            result = FALSE;
        else if (IsDir)
            result = ((attrs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
        else
            result = TRUE;

        if (IsDir)
            Right[Size - 1] = '\\';
    }
    else
    {
        /*
         * Do case-insensitive string comparisons if /I specified.
         *
         * Since both strings are user-specific, use kernel32!lstrcmp(i)
         * instead of CRT!_tcs(i)cmp, so as to use the correct
         * current thread locale information.
         */
        INT (WINAPI *StringCmp)(LPCTSTR, LPCTSTR) =
            (Cmd->If.Flags & IFFLAG_IGNORECASE) ? lstrcmpi : lstrcmp;

        if (Cmd->If.Operator == IF_STRINGEQ)
        {
            /* IF str1 == str2 */
            result = (StringCmp(Left, Right) == 0);
        }
        else if (bEnableExtensions)
        {
            result = GenericCmp(StringCmp, Left, Right);
            switch (Cmd->If.Operator)
            {
            case IF_EQU: result = (result == 0); break;
            case IF_NEQ: result = (result != 0); break;
            case IF_LSS: result = (result <  0); break;
            case IF_LEQ: result = (result <= 0); break;
            case IF_GTR: result = (result >  0); break;
            case IF_GEQ: result = (result >= 0); break;
            default: goto unknownOp;
            }
        }
        else
        {
unknownOp:
            ERR("Unknown IF operator 0x%x\n", Cmd->If.Operator);
            ASSERT(FALSE);
            goto fail;
        }
    }

    if (Left) cmd_free(Left);
    cmd_free(Right);

    if (result ^ ((Cmd->If.Flags & IFFLAG_NEGATE) != 0))
    {
        /* Full condition was true, do the command */
        return ExecuteCommand(Cmd->Subcommands);
    }
    else
    {
        /* Full condition was false, do the "else" command if there is one */
        return ExecuteCommand(Cmd->Subcommands->Next);
    }

fail:
    if (Left) cmd_free(Left);
    cmd_free(Right);
    return 1;
}

/* EOF */
