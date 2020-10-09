/*
 *  INTERNAL.C - command.com internal commands.
 *
 *
 *  History:
 *
 *  17/08/94 (Tim Norman)
 *    started.
 *
 *  08/08/95 (Matt Rains)
 *    i have cleaned up the source code. changes now bring this source into
 *    guidelines for recommended programming practice.
 *
 *  cd()
 *    started.
 *
 *  dir()
 *    i have added support for file attributes to the DIR() function. the
 *    routine adds "d" (directory) and "r" (read only) output. files with the
 *    system attribute have the filename converted to lowercase. files with
 *    the hidden attribute are not displayed.
 *
 *    i have added support for directorys. now if the directory attribute is
 *    detected the file size if replaced with the string "<dir>".
 *
 *  ver()
 *    started.
 *
 *  md()
 *    started.
 *
 *  rd()
 *    started.
 *
 *  del()
 *    started.
 *
 *  does not support wildcard selection.
 *
 *  todo: add delete directory support.
 *        add recursive directory delete support.
 *
 *  ren()
 *    started.
 *
 *  does not support wildcard selection.
 *
 *    todo: add rename directory support.
 *
 *  a general structure has been used for the cd, rd and md commands. this
 *  will be better in the long run. it is too hard to maintain such diverse
 *  functions when you are involved in a group project like this.
 *
 *  12/14/95 (Tim Norman)
 *    fixed DIR so that it will stick \*.* if a directory is specified and
 *    that it will stick on .* if a file with no extension is specified or
 *    *.* if it ends in a \
 *
 *  1/6/96 (Tim Norman)
 *    added an isatty call to DIR so it won't prompt for keypresses unless
 *    stdin and stdout are the console.
 *
 *    changed parameters to be mutually consistent to make calling the
 *    functions easier
 *
 *  rem()
 *    started.
 *
 *  doskey()
 *    started.
 *
 *    01/22/96 (Oliver Mueller)
 *        error messages are now handled by perror.
 *
 *    02/05/96 (Tim Norman)
 *        converted all functions to accept first/rest parameters
 *
 *    07/26/96 (Tim Norman)
 *        changed return values to int instead of void
 *
 *        path() started.
 *
 *    12/23/96 (Aaron Kaufman)
 *        rewrote dir() to mimic MS-DOS's dir
 *
 *    01/28/97 (Tim Norman)
 *        cleaned up Aaron's DIR code
 *
 *    06/13/97 (Tim Norman)
 *        moved DIR code to dir.c
 *        re-implemented Aaron's DIR code
 *
 *    06/14/97 (Steffan Kaiser)
 *        ctrl-break handling
 *        bug fixes
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    03-Dec-1998 (Eric Kohl)
 *        Replaced DOS calls by Win32 calls.
 *
 *    08-Dec-1998 (Eric Kohl)
 *        Added help texts ("/?").
 *
 *    18-Dec-1998 (Eric Kohl)
 *        Added support for quoted arguments (cd "program files").
 *
 *    07-Jan-1999 (Eric Kohl)
 *        Clean up.
 *
 *    26-Jan-1999 (Eric Kohl)
 *        Replaced remaining CRT io functions by Win32 io functions.
 *        Unicode safe!
 *
 *    30-Jan-1999 (Eric Kohl)
 *        Added "cd -" feature. Changes to the previous directory.
 *
 *    15-Mar-1999 (Eric Kohl)
 *        Fixed bug in "cd -" feature. If the previous directory was a root
 *        directory, it was ignored.
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        Improved chdir/cd command.
 *
 *    02-Apr-2004 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hard code string so they can be
 *        translate to other langues.
 *
 *    19-Jul-2005 (Brandon Turner <turnerb7@msu.edu>)
 *        Rewrite the CD, it working as Windows 2000 CMD
 *
 *    19-Jul-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Add SetRootPath and GetRootPath
 *
 *    14-Jul-2007 (Pierre Schweitzer <heis_spiter@hotmail.com>)
 *        Added commands help display to help command (ex. : "help cmd")
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_CHDIR

/*
 * Helper function for getting the current path from drive
 * without changing the drive. Return code: 0 = ok, 1 = fail.
 * 'InPath' can have any size; if the two first letters are
 * not a drive with ':' it will get the current path on
 * the current drive exactly as GetCurrentDirectory() does.
 */
INT
GetRootPath(
    IN LPCTSTR InPath,
    OUT LPTSTR OutPath,
    IN INT size)
{
    if (InPath[0] && InPath[1] == _T(':'))
    {
        INT t = 0;

        if ((InPath[0] >= _T('0')) && (InPath[0] <= _T('9')))
        {
            t = (InPath[0] - _T('0')) + 28;
        }
        else if ((InPath[0] >= _T('a')) && (InPath[0] <= _T('z')))
        {
            t = (InPath[0] - _T('a')) + 1;
        }
        else if ((InPath[0] >= _T('A')) && (InPath[0] <= _T('Z')))
        {
            t = (InPath[0] - _T('A')) + 1;
        }

        return (_tgetdcwd(t, OutPath, size) == NULL);
    }

    /* Get current directory */
    return !GetCurrentDirectory(size, OutPath);
}


BOOL SetRootPath(TCHAR *oldpath, TCHAR *InPath)
{
    DWORD dwLastError;
    TCHAR OutPath[MAX_PATH];
    TCHAR OutPathTemp[MAX_PATH];

    StripQuotes(InPath);

    /* Retrieve the full path name from the (possibly relative) InPath */
    if (GetFullPathName(InPath, ARRAYSIZE(OutPathTemp), OutPathTemp, NULL) == 0)
    {
        dwLastError = GetLastError();
        goto Fail;
    }

    if (bEnableExtensions)
    {
        /*
         * Convert the full path to its correct case, and
         * resolve any wilcard present as well in the path
         * and retrieve the first result.
         * Example: c:\windows\SYSTEM32 => C:\WINDOWS\System32
         * Example: C:\WINDOWS\S* => C:\WINDOWS\System,
         * or C:\WINDOWS\System32, depending on the user's OS.
         */
        GetPathCase(OutPathTemp, OutPath);
    }
    else
    {
        _tcscpy(OutPath, OutPathTemp);
    }

    /* Use _tchdir(), since unlike SetCurrentDirectory() it updates
     * the current-directory-on-drive environment variables. */
    if (_tchdir(OutPath) != 0)
    {
        dwLastError = GetLastError();
        if (dwLastError == ERROR_FILE_NOT_FOUND)
            dwLastError = ERROR_PATH_NOT_FOUND;
        goto Fail;
    }

    /* Keep the original drive in ordinary CD/CHDIR (without /D switch) */
    if (oldpath != NULL && _tcsncicmp(OutPath, oldpath, 2) != 0)
        SetCurrentDirectory(oldpath);

    return TRUE;

Fail:
    ConErrFormatMessage(dwLastError);
    nErrorLevel = 1;
    return FALSE;
}


/*
 * CD / CHDIR
 */
INT cmd_chdir(LPTSTR param)
{
    BOOL bChangeDrive = FALSE;
    LPTSTR tmp;
    TCHAR szCurrent[MAX_PATH];

    /* Filter out special cases first */

    /* Print help */
    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_CD_HELP);
        return 0;
    }

    //
    // FIXME: Use the split() tokenizer if bEnableExtensions == FALSE,
    // so as to cut the parameter at the first separator (space, ',', ';'):
    // - When bEnableExtensions == FALSE, doing
    //   CD system32;winsxs
    //   will go into system32, (but: CD "system32;winsxs" will fail as below), while
    // - When bEnableExtensions == TRUE, it will fail because the "system32;winsxs"
    //   directory does not exist.
    //

    /* Remove extra quotes */
    StripQuotes(param);

    if (bEnableExtensions)
    {
        /* Strip trailing whitespace */
        tmp = param + _tcslen(param) - 1;
        while (tmp > param && _istspace(*tmp))
            --tmp;
        *(tmp + 1) = _T('\0');
    }

    /* Reset the error level */
    nErrorLevel = 0;

    /* Print the current directory on a disk */
    if (_tcslen(param) == 2 && param[1] == _T(':'))
    {
        if (GetRootPath(param, szCurrent, ARRAYSIZE(szCurrent)))
        {
            error_invalid_drive();
            return 1;
        }
        ConOutPrintf(_T("%s\n"), szCurrent);
        return 0;
    }

    /* Get the current directory */
    GetCurrentDirectory(ARRAYSIZE(szCurrent), szCurrent);
    if (param[0] == _T('\0'))
    {
        ConOutPrintf(_T("%s\n"), szCurrent);
        return 0;
    }

    /* If the input string is prefixed with the /D switch, change the drive */
    if (!_tcsncicmp(param, _T("/D"), 2))
    {
        bChangeDrive = TRUE;
        param += 2;
        while (_istspace(*param))
            ++param;
    }

    if (!SetRootPath(bChangeDrive ? NULL : szCurrent, param))
    {
        nErrorLevel = 1;
        return 1;
    }

    return 0;
}

#endif

#ifdef INCLUDE_CMD_MKDIR

/* Helper function for mkdir to make directories in a path.
Don't use the api to decrease dependence on libs */
BOOL
MakeFullPath(TCHAR * DirPath)
{
    TCHAR path[MAX_PATH];
    TCHAR *p = DirPath;
    INT_PTR  n;

    if (CreateDirectory(DirPath, NULL))
        return TRUE;
    else if (GetLastError() != ERROR_PATH_NOT_FOUND)
        return FALSE;

    /* got ERROR_PATH_NOT_FOUND, so try building it up one component at a time */
    if (p[0] && p[1] == _T(':'))
        p += 2;
    while (*p == _T('\\'))
        p++; /* skip drive root */
    do
    {
        p = _tcschr(p, _T('\\'));
        n = p ? p++ - DirPath : _tcslen(DirPath);
        _tcsncpy(path, DirPath, n);
        path[n] = _T('\0');
        if ( !CreateDirectory(path, NULL) &&
            (GetLastError() != ERROR_ALREADY_EXISTS))
        {
            return FALSE;
        }
    } while (p != NULL);

    return TRUE;
}

/*
 * MD / MKDIR
 */
INT cmd_mkdir (LPTSTR param)
{
    LPTSTR *p;
    INT argc, i;
    DWORD dwLastError;

    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_MKDIR_HELP);
        return 0;
    }

    p = split (param, &argc, FALSE, FALSE);
    if (argc == 0)
    {
        ConErrResPuts(STRING_ERROR_REQ_PARAM_MISSING);
        freep(p);
        nErrorLevel = 1;
        return 1;
    }

    nErrorLevel = 0;
    for (i = 0; i < argc; i++)
    {
        if (!MakeFullPath(p[i]))
        {
            dwLastError = GetLastError();
            switch (dwLastError)
            {
            case ERROR_PATH_NOT_FOUND:
                ConErrResPuts(STRING_MD_ERROR2);
                break;

            case ERROR_FILE_EXISTS:
            case ERROR_ALREADY_EXISTS:
                ConErrResPrintf(STRING_MD_ERROR, p[i]);
                break;

            default:
                ErrorMessage(GetLastError(), NULL);
            }
            nErrorLevel = 1;
        }
    }

    freep (p);
    return nErrorLevel;
}
#endif

#ifdef INCLUDE_CMD_RMDIR
/*
 * RD / RMDIR
 */
BOOL DeleteFolder(LPTSTR Directory)
{
    LPTSTR pFileName;
    HANDLE hFile;
    WIN32_FIND_DATA f;
    DWORD dwAttribs;
    TCHAR szFullPath[MAX_PATH];

    _tcscpy(szFullPath, Directory);
    pFileName = &szFullPath[_tcslen(szFullPath)];
    /*
     * Append a path separator if we don't have one already, and if this a drive root
     * path is not specified (paths like "C:" mean the current directory on drive C:).
     */
    if (*szFullPath && *(pFileName - 1) != _T(':') && *(pFileName - 1) != _T('\\'))
        *pFileName++ = _T('\\');
    _tcscpy(pFileName, _T("*"));

    hFile = FindFirstFile(szFullPath, &f);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            /* Check Breaker */
            if (bCtrlBreak)
                break;

            if (!_tcscmp(f.cFileName, _T(".")) ||
                !_tcscmp(f.cFileName, _T("..")))
            {
                continue;
            }

            _tcscpy(pFileName, f.cFileName);

            dwAttribs = f.dwFileAttributes;

            if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!DeleteFolder(szFullPath))
                {
                    /* Couldn't delete the file, print out the error */
                    ErrorMessage(GetLastError(), szFullPath);

                    /* Continue deleting files/subfolders */
                }
            }
            else
            {
                /* Force file deletion even if it's read-only */
                if (dwAttribs & FILE_ATTRIBUTE_READONLY)
                    SetFileAttributes(szFullPath, dwAttribs & ~FILE_ATTRIBUTE_READONLY);

                if (!DeleteFile(szFullPath))
                {
                    /* Couldn't delete the file, print out the error */
                    ErrorMessage(GetLastError(), szFullPath);

                    /* Restore file attributes */
                    SetFileAttributes(szFullPath, dwAttribs);

                    /* Continue deleting files/subfolders */
                }
            }

        } while (FindNextFile(hFile, &f));
        FindClose(hFile);
    }

    /* Ignore directory deletion if the user pressed Ctrl-C */
    if (bCtrlBreak)
        return TRUE;

    /*
     * Detect whether we are trying to delete a pure root drive (e.g. "C:\\", but not "C:");
     * if so, just return success. Otherwise the RemoveDirectory() call below would fail
     * and return ERROR_ACCESS_DENIED.
     */
    if (GetFullPathName(Directory, ARRAYSIZE(szFullPath), szFullPath, NULL) == 3 &&
        szFullPath[1] == _T(':') && szFullPath[2] == _T('\\'))
    {
        return TRUE;
    }

    /* First attempt to delete the directory */
    if (RemoveDirectory(Directory))
        return TRUE;

    /*
     * It failed; if it was due to an denied access, check whether it was
     * due to the directory being read-only. If so, remove its attribute
     * and retry deletion.
     */
    if (GetLastError() == ERROR_ACCESS_DENIED)
    {
        /* Force directory deletion even if it's read-only */
        dwAttribs = GetFileAttributes(Directory);
        if (dwAttribs & FILE_ATTRIBUTE_READONLY)
        {
            SetFileAttributes(Directory, dwAttribs & ~FILE_ATTRIBUTE_READONLY);
            return RemoveDirectory(Directory);
        }
    }

    return FALSE;
}

INT cmd_rmdir(LPTSTR param)
{
    INT nError = 0;
    INT res;
    LPTSTR *arg;
    INT args;
    INT dirCount;
    INT i;
    TCHAR ch;
    BOOL bRecurseDir = FALSE;
    BOOL bQuiet = FALSE;

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_RMDIR_HELP);
        return 0;
    }

    arg = split(param, &args, FALSE, FALSE);
    dirCount = 0;

    /* Check for options anywhere in command line */
    for (i = 0; i < args; i++)
    {
        if (*arg[i] == _T('/'))
        {
            /* Found an option, but check to make sure it has something after it */
            if (_tcslen(arg[i]) == 2)
            {
                ch = _totupper(arg[i][1]);

                if (ch == _T('S'))
                    bRecurseDir = TRUE;
                else if (ch == _T('Q'))
                    bQuiet = TRUE;
            }
        }
        else
        {
            dirCount++;
        }
    }

    if (dirCount == 0)
    {
        /* No folder to remove */
        error_req_param_missing();
        freep(arg);
        return 1;
    }

    for (i = 0; i < args; i++)
    {
        if (*arg[i] == _T('/'))
            continue;

        if (bRecurseDir)
        {
            /* Ask the user whether to delete everything in the folder */
            if (!bQuiet)
            {
                res = FilePromptYNA(STRING_DEL_HELP2);
                if (res == PROMPT_NO || res == PROMPT_BREAK)
                {
                    nError = 1;
                    continue;
                }
                if (res == PROMPT_ALL)
                    bQuiet = TRUE;
            }

            res = DeleteFolder(arg[i]);
        }
        else
        {
            /* Without /S, do not force directory deletion even if it's read-only */
            res = RemoveDirectory(arg[i]);
        }

        if (!res)
        {
            /* Couldn't delete the folder, print out the error */
            nError = GetLastError();
            ErrorMessage(nError, NULL);
        }
    }

    freep(arg);
    return nError;
}
#endif


/*
 * Either exits the command interpreter, or quits the current batch context.
 */

/* Enable this define for supporting EXIT /B even when extensions are disabled */
// #define SUPPORT_EXIT_B_NO_EXTENSIONS

INT CommandExit(LPTSTR param)
{
    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_EXIT_HELP);

        /* Just make sure we don't exit */
        bExit = FALSE;
        return 0;
    }

    if (_tcsnicmp(param, _T("/B"), 2) == 0)
    {
        param += 2;

        /*
         * If a current batch file is running, exit it,
         * otherwise exit this command interpreter instance.
         */
        if (bc)
        {
            /* Windows' CMD compatibility: Use GOTO :EOF */
            TCHAR EofLabel[] = _T(":EOF");

#ifdef SUPPORT_EXIT_B_NO_EXTENSIONS
            /*
             * Temporarily enable extensions so as to support :EOF.
             *
             * Our GOTO implementation ensures that, when extensions are
             * enabled and the label is ':EOF', no immediate change of batch
             * context (done e.g. via ExitBatch() calls) is performed.
             * This will therefore ensure that we do not spoil the extensions
             * state when we restore it below.
             */
            BOOL bOldEnableExtensions = bEnableExtensions;
            bEnableExtensions = TRUE;
#endif

            cmd_goto(EofLabel);

#ifdef SUPPORT_EXIT_B_NO_EXTENSIONS
            /* Restore the original state of the extensions */
            bEnableExtensions = bOldEnableExtensions;
#endif
        }
        else
        {
            bExit = TRUE;
        }
    }
    else
    {
        /* Exit this command interpreter instance */
        bExit = TRUE;
    }

    /* Search for an optional exit code */
    while (_istspace(*param))
        ++param;

    /* Set the errorlevel to the exit code */
    if (_istdigit(*param))
    {
        nErrorLevel = _ttoi(param);
        // if (fSingleCommand == 1) return nErrorLevel;
    }

    return (bExit ? nErrorLevel : 0);
}

#ifdef INCLUDE_CMD_REM
/*
 * does nothing
 */
INT CommandRem (LPTSTR param)
{
    if (_tcsstr(param, _T("/?")) == param)
    {
        ConOutResPaging(TRUE,STRING_REM_HELP);
    }

    return 0;
}
#endif /* INCLUDE_CMD_REM */


INT CommandShowCommands(LPTSTR param)
{
    PrintCommandList();
    return 0;
}

/* EOF */
