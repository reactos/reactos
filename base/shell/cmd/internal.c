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

/* helper functions for getting current path from drive
   without changing drive. Return code 0 = ok, 1 = fail.
   INT GetRootPath("C:",outbuffer,chater size of outbuffer);
   the first param can have any size, if the the two frist
   letter are not a drive with : it will get Currentpath on
   current drive exacly as GetCurrentDirectory does.
   */

INT GetRootPath(TCHAR *InPath,TCHAR *OutPath,INT size)
{
    if (InPath[0] && InPath[1] == _T(':'))
    {
        INT t=0;

        if ((InPath[0] >= _T('0')) && (InPath[0] <= _T('9')))
        {
            t = (InPath[0] - _T('0')) +28;
        }

        if ((InPath[0] >= _T('a')) && (InPath[0] <= _T('z')))
        {
            t = (InPath[0] - _T('a')) +1;
            InPath[0] = t + _T('A') - 1;
        }

        if ((InPath[0] >= _T('A')) && (InPath[0] <= _T('Z')))
        {
            t = (InPath[0] - _T('A')) +1;
        }

        return _tgetdcwd(t,OutPath,size) == NULL;
    }

    /* Get current directory */
    return !GetCurrentDirectory(size,OutPath);
}


BOOL SetRootPath(TCHAR *oldpath, TCHAR *InPath)
{
    TCHAR OutPath[MAX_PATH];
    TCHAR OutPathTemp[MAX_PATH];

    /* Retrieve the full path name from the (possibly relative) InPath */
    if (GetFullPathName(InPath, MAX_PATH, OutPathTemp, NULL) == 0)
        goto Fail;

    /* Convert the full path to its correct case.
     * Example: c:\windows\SYSTEM32 => C:\WINDOWS\System32 */
    GetPathCase(OutPathTemp, OutPath);

    /* Use _tchdir, since unlike SetCurrentDirectory it updates
     * the current-directory-on-drive environment variables. */
    if (_tchdir(OutPath) != 0)
        goto Fail;

    /* Keep original drive in ordinary CD/CHDIR (without /D switch). */
    if (oldpath != NULL && _tcsncicmp(OutPath, oldpath, 2) != 0)
        SetCurrentDirectory(oldpath);

    return TRUE;

Fail:
    ConErrFormatMessage(GetLastError());
    nErrorLevel = 1;
    return FALSE;
}


/*
 * CD / CHDIR
 *
 */
INT cmd_chdir (LPTSTR param)
{
    BOOL bChangeDrive = FALSE;
    LPTSTR tmp;
    TCHAR szCurrent[MAX_PATH];

    /* Filter out special cases first */

    /* Print Help */
    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_CD_HELP);
        return 0;
    }

    /* Remove extra quotes and strip trailing whitespace */
    StripQuotes(param);
    tmp = param + _tcslen(param) - 1;
    while (tmp > param && _istspace(*tmp))
        tmp--;
    *(tmp + 1) = _T('\0');

    /* Set Error Level to Success */
    nErrorLevel = 0;

    /* Print Current Directory on a disk */
    if (_tcslen(param) == 2 && param[1] == _T(':'))
    {
        if (GetRootPath(param, szCurrent, MAX_PATH))
        {
            error_invalid_drive();
            return 1;
        }
        ConOutPrintf(_T("%s\n"), szCurrent);
        return 0;
    }

    /* Get Current Directory */
    GetCurrentDirectory(MAX_PATH, szCurrent);
    if (param[0] == _T('\0'))
    {
        ConOutPrintf(_T("%s\n"), szCurrent);
        return 0;
    }

    /* Input String Contains /D Switch */
    if (!_tcsncicmp(param, _T("/D"), 2))
    {
        bChangeDrive = TRUE;
        param += 2;
        while (_istspace(*param))
            param++;
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

/* Helper funtion for mkdir to make directories in a path.
Dont use the api to decrease depence on libs */
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
            if (GetLastError() == ERROR_PATH_NOT_FOUND)
            {
                ConErrResPuts(STRING_MD_ERROR2);
            }
            else
            {
                ErrorMessage (GetLastError(), _T("MD"));
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
BOOL DeleteFolder(LPTSTR FileName)
{
    TCHAR Base[MAX_PATH];
    TCHAR TempFileName[MAX_PATH];
    HANDLE hFile;
    WIN32_FIND_DATA f;
    _tcscpy(Base,FileName);
    _tcscat(Base,_T("\\*"));
    hFile = FindFirstFile(Base, &f);
    Base[_tcslen(Base) - 1] = _T('\0');
    if (hFile != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!_tcscmp(f.cFileName, _T(".")) ||
                !_tcscmp(f.cFileName, _T("..")))
                continue;
            _tcscpy(TempFileName,Base);
            _tcscat(TempFileName,f.cFileName);

            if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                DeleteFolder(TempFileName);
            else
            {
                SetFileAttributes(TempFileName,FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFile(TempFileName))
                {
                    FindClose (hFile);
                    return 0;
                }
            }

        }while (FindNextFile (hFile, &f));
        FindClose (hFile);
    }
    return RemoveDirectory(FileName);
}

INT cmd_rmdir (LPTSTR param)
{
    TCHAR ch;
    INT args;
    INT dirCount;
    LPTSTR *arg;
    INT i;
    BOOL RD_SUB = FALSE;
    BOOL RD_QUIET = FALSE;
    INT res;
    INT nError = 0;
    TCHAR szFullPath[MAX_PATH];

    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_RMDIR_HELP);
        return 0;
    }

    arg = split (param, &args, FALSE, FALSE);
    dirCount = 0;

    /* check for options anywhere in command line */
    for (i = 0; i < args; i++)
    {
        if (*arg[i] == _T('/'))
        {
            /*found a command, but check to make sure it has something after it*/
            if (_tcslen (arg[i]) == 2)
            {
                ch = _totupper (arg[i][1]);

                if (ch == _T('S'))
                {
                    RD_SUB = TRUE;
                }
                else if (ch == _T('Q'))
                {
                    RD_QUIET = TRUE;
                }
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

        if (RD_SUB)
        {
            /* ask if they want to delete evrything in the folder */
            if (!RD_QUIET)
            {
                res = FilePromptYNA (STRING_DEL_HELP2);
                if (res == PROMPT_NO || res == PROMPT_BREAK)
                {
                    nError = 1;
                    continue;
                }
                if (res == PROMPT_ALL)
                    RD_QUIET = TRUE;
            }
            /* get the folder name */
            GetFullPathName(arg[i],MAX_PATH,szFullPath,NULL);

            /* remove trailing \ if any, but ONLY if dir is not the root dir */
            if (_tcslen (szFullPath) >= 2 && szFullPath[_tcslen (szFullPath) - 1] == _T('\\'))
                szFullPath[_tcslen(szFullPath) - 1] = _T('\0');

            res = DeleteFolder(szFullPath);
        }
        else
        {
            res = RemoveDirectory(arg[i]);
        }

        if (!res)
        {
            /* Couldn't delete the folder, print out the error */
            nError = GetLastError();
            ErrorMessage(nError, _T("RD"));
        }
    }

    freep (arg);
    return nError;
}
#endif


/*
 * set the exitflag to true
 */
INT CommandExit (LPTSTR param)
{
    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_EXIT_HELP);
        /* Just make sure */
        bExit = FALSE;
        /* Dont exit */
        return 0;
    }

    if (bc != NULL && _tcsnicmp(param,_T("/b"),2) == 0)
    {
        param += 2;
        while (_istspace (*param))
            param++;
        if (_istdigit(*param))
            nErrorLevel = _ttoi(param);
        ExitBatch();
    }
    else
    {
        bExit = TRUE;
    }

    return 0;
}

#ifdef INCLUDE_CMD_REM
/*
 * does nothing
 */
INT CommandRem (LPTSTR param)
{
    if (!_tcsncmp (param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE,STRING_REM_HELP);
    }

    return 0;
}
#endif /* INCLUDE_CMD_REM */


INT CommandShowCommands (LPTSTR param)
{
    PrintCommandList();
    return 0;
}

/* EOF */
