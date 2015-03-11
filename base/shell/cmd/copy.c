/*
 *  COPY.C -- copy internal command.
 *
 *
 *  History:
 *
 *    01-Aug-98 (Rob Lake z63rrl@morgan.ucs.mun.ca)
 *        started
 *
 *    13-Aug-1998 (John P. Price)
 *        fixed memory leak problem in copy function.
 *        fixed copy function so it would work with wildcards in the source
 *
 *    13-Dec-1998 (Eric Kohl)
 *        Added COPY command to CMD.
 *
 *    26-Jan-1998 (Eric Kohl)
 *        Replaced CRT io functions by Win32 io functions.
 *
 *    27-Oct-1998 (Eric Kohl)
 *        Disabled prompting when used in batch mode.
 *
 *    03-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 *
 *    13-Jul-2005 (Brandon Turner <turnerb7@msu.edu>)
 *        Rewrite to clean up copy and support wildcard.
 *
 *    20-Jul-2005 (Brandon Turner <turnerb7@msu.edu>)
 *        Add touch syntax.  "copy arp.exe+,,"
 *        Copy command is now completed.
 */

#include "precomp.h"

#ifdef INCLUDE_CMD_COPY

enum
{
    COPY_ASCII       = 0x001,   /* /A  */
    COPY_DECRYPT     = 0x004,   /* /D  */
    COPY_VERIFY      = 0x008,   /* /V  : Dummy, Never will be Impleneted */
    COPY_SHORTNAME   = 0x010,   /* /N  : Dummy, Never will be Impleneted */
    COPY_NO_PROMPT   = 0x020,   /* /Y  */
    COPY_PROMPT      = 0x040,   /* /-Y */
    COPY_RESTART     = 0x080,   /* /Z  */
    COPY_BINARY      = 0x100,   /* /B  */
};

INT
copy(TCHAR source[MAX_PATH],
     TCHAR dest[MAX_PATH],
     INT append,
     DWORD lpdwFlags,
     BOOL bTouch)
{
    FILETIME srctime,NewFileTime;
    HANDLE hFileSrc;
    HANDLE hFileDest;
    LPBYTE buffer;
    DWORD  dwAttrib;
    DWORD  dwRead;
    DWORD  dwWritten;
    BOOL   bEof = FALSE;
    TCHAR TrueDest[MAX_PATH];
    TCHAR TempSrc[MAX_PATH];
    TCHAR * FileName;
    SYSTEMTIME CurrentTime;

    /* Check Breaker */
    if (CheckCtrlBreak(BREAK_INPUT))
        return 0;

    TRACE ("checking mode\n");

    if (bTouch)
    {
        hFileSrc = CreateFile (source, GENERIC_WRITE, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, 0, NULL);
        if (hFileSrc == INVALID_HANDLE_VALUE)
        {
            ConOutResPrintf(STRING_COPY_ERROR1, source);
            nErrorLevel = 1;
            return 0;
        }

        GetSystemTime(&CurrentTime);
        SystemTimeToFileTime(&CurrentTime, &NewFileTime);
        if (SetFileTime(hFileSrc,(LPFILETIME) NULL, (LPFILETIME) NULL, &NewFileTime))
        {
            CloseHandle(hFileSrc);
            nErrorLevel = 1;
            return 1;

        }
        else
        {
            CloseHandle(hFileSrc);
            return 0;
        }
    }

    dwAttrib = GetFileAttributes (source);

    hFileSrc = CreateFile (source, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hFileSrc == INVALID_HANDLE_VALUE)
    {
        ConOutResPrintf(STRING_COPY_ERROR1, source);
        nErrorLevel = 1;
        return 0;
    }

    TRACE ("getting time\n");

    GetFileTime (hFileSrc, &srctime, NULL, NULL);

    TRACE ("copy: flags has %s\n",
        lpdwFlags & COPY_ASCII ? "ASCII" : "BINARY");

    /* Check to see if /D or /Z are true, if so we need a middle
       man to copy the file too to allow us to use CopyFileEx later */
    if (lpdwFlags & COPY_DECRYPT)
    {
        GetEnvironmentVariable(_T("TEMP"),TempSrc,MAX_PATH);
        _tcscat(TempSrc,_T("\\"));
        FileName = _tcsrchr(source,_T('\\'));
        FileName++;
        _tcscat(TempSrc,FileName);
        /* This is needed to be on the end to prevent an error
           if the user did "copy /D /Z foo bar then it would be copied
           too %TEMP%\foo here and when %TEMP%\foo when it sets it up
           for COPY_RESTART, this would mean it is copying to itself
           which would error when it tried to open the handles for ReadFile
           and WriteFile */
        _tcscat(TempSrc,_T(".decrypt"));
        if (!CopyFileEx(source, TempSrc, NULL, NULL, FALSE, COPY_FILE_ALLOW_DECRYPTED_DESTINATION))
        {
            CloseHandle (hFileSrc);
            nErrorLevel = 1;
            return 0;
        }
        _tcscpy(source, TempSrc);
    }


    if (lpdwFlags & COPY_RESTART)
    {
        _tcscpy(TrueDest, dest);
        GetEnvironmentVariable(_T("TEMP"),dest,MAX_PATH);
        _tcscat(dest,_T("\\"));
        FileName = _tcsrchr(TrueDest,_T('\\'));
        FileName++;
        _tcscat(dest,FileName);
    }


    if (!IsExistingFile (dest))
    {
        TRACE ("opening/creating\n");
        hFileDest =
            CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    }
    else if (!append)
    {
        TRACE ("SetFileAttributes (%s, FILE_ATTRIBUTE_NORMAL);\n", debugstr_aw(dest));
        SetFileAttributes (dest, FILE_ATTRIBUTE_NORMAL);

        TRACE ("DeleteFile (%s);\n", debugstr_aw(dest));
        DeleteFile (dest);

        hFileDest =	CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    }
    else
    {
        LONG lFilePosHigh = 0;

        if (!_tcscmp (dest, source))
        {
            CloseHandle (hFileSrc);
            return 0;
        }

        TRACE ("opening/appending\n");
        SetFileAttributes (dest, FILE_ATTRIBUTE_NORMAL);

        hFileDest =
            CreateFile (dest, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        /* Move to end of file to start writing */
        SetFilePointer (hFileDest, 0, &lFilePosHigh,FILE_END);
    }


    if (hFileDest == INVALID_HANDLE_VALUE)
    {
        CloseHandle (hFileSrc);
        ConOutResPuts(STRING_ERROR_PATH_NOT_FOUND);
        nErrorLevel = 1;
        return 0;
    }

    /* A page-aligned buffer usually give more speed */
    buffer = VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (buffer == NULL)
    {
        CloseHandle (hFileDest);
        CloseHandle (hFileSrc);
        ConOutResPuts(STRING_ERROR_OUT_OF_MEMORY);
        nErrorLevel = 1;
        return 0;
    }

    do
    {
        ReadFile (hFileSrc, buffer, BUFF_SIZE, &dwRead, NULL);
        if (lpdwFlags & COPY_ASCII)
        {
            LPBYTE pEof = memchr(buffer, 0x1A, dwRead);
            if (pEof != NULL)
            {
                bEof = TRUE;
                dwRead = pEof-buffer+1;
                break;
            }
        }

        if (dwRead == 0)
            break;

        WriteFile (hFileDest, buffer, dwRead, &dwWritten, NULL);
        if (dwWritten != dwRead || CheckCtrlBreak(BREAK_INPUT))
        {
            ConOutResPuts(STRING_COPY_ERROR3);

            VirtualFree (buffer, 0, MEM_RELEASE);
            CloseHandle (hFileDest);
            CloseHandle (hFileSrc);
            nErrorLevel = 1;
            return 0;
        }
    }
    while (!bEof);

    TRACE ("setting time\n");
    SetFileTime (hFileDest, &srctime, NULL, NULL);

    if ((lpdwFlags & COPY_ASCII) && !bEof)
    {
        /* we're dealing with ASCII files! */
        buffer[0] = 0x1A;
        TRACE ("appending ^Z\n");
        WriteFile (hFileDest, buffer, sizeof(CHAR), &dwWritten, NULL);
    }

    VirtualFree (buffer, 0, MEM_RELEASE);
    CloseHandle (hFileDest);
    CloseHandle (hFileSrc);

    TRACE ("setting mode\n");
    SetFileAttributes (dest, dwAttrib);

    /* Now finish off the copy if needed with CopyFileEx */
    if (lpdwFlags & COPY_RESTART)
    {
        if (!CopyFileEx(dest, TrueDest, NULL, NULL, FALSE, COPY_FILE_RESTARTABLE))
        {
            nErrorLevel = 1;
            DeleteFile(dest);
            return 0;
        }
        /* Take care of file in the temp folder */
        DeleteFile(dest);

    }

    if (lpdwFlags & COPY_DECRYPT)
        DeleteFile(TempSrc);

    return 1;
}


static INT CopyOverwrite (LPTSTR fn)
{
    /*ask the user if they want to override*/
    INT res;
    ConOutResPrintf(STRING_COPY_HELP1, fn);
    res = FilePromptYNA (0);
    return res;
}

/* The following lines of copy were written by someone else
(most likely Eric Kohl) and it was taken from ren.c */
static void
BuildFileName(
    LPTSTR pszSource,
    LPTSTR pszTarget,
    LPTSTR pszOutput)
{
    /* build destination file name */
    while (*pszTarget != 0)
    {
        if (*pszTarget == _T('*'))
        {
            pszTarget++;
            while ((*pszSource != 0) && (*pszSource != *pszTarget))
            {
                *pszOutput++ = *pszSource++;
            }
        }
        else if (*pszTarget == _T('?'))
        {
            pszTarget++;
            if (*pszSource != 0)
            {
                *pszOutput++ = *pszSource++;
            }
        }
        else
        {
            *pszOutput++ = *pszTarget++;
            if (*pszSource != 0)
                pszSource++;
        }
    }

    *pszOutput = 0;
}

INT cmd_copy(LPTSTR param)
{
    LPTSTR *arg;
    INT argc, i, nFiles, nOverwrite = 0, nSrc = -1, nDes = -1;
    /* this is the path up to the folder of the src and dest ie C:\windows\ */
    TCHAR szDestPath[MAX_PATH];
    TCHAR szSrcPath[MAX_PATH];
    DWORD dwFlags = 0;
    /* If this is the type of copy where we are adding files */
    BOOL bAppend = FALSE;
    WIN32_FIND_DATA findBuffer;
    HANDLE hFile = NULL;
    BOOL bTouch = FALSE;
    /* Used when something like "copy c*.exe d*.exe" during the process of
       figuring out the new name */
    /* Pointer to keep track of how far through the append input(file1+file2+file3) we are */
    TCHAR  * appendPointer = _T("\0");
    /* The full path to src and dest.  This has drive letter, folders, and filename */
    TCHAR tmpDestPath[MAX_PATH];
    TCHAR tmpSrcPath[MAX_PATH];
    /* A bool on weather or not the destination name will be taking from the input */
    BOOL bSrcName = FALSE;
    /* Seems like a waste but it is a pointer used to copy from input to PreserveName */
    TCHAR * UseThisName;
    /* for CMDCOPY env */
    TCHAR *evar;
    int size;
    TCHAR * szTouch;
    BOOL bHasWildcard, bDone = FALSE, bMoreFiles = FALSE;
    BOOL bMultipleSource = FALSE, bMultipleDest = FALSE;


    /* Show help/usage info */
    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_COPY_HELP2);
        return 0;
    }

    nErrorLevel = 0;

    /* Get the envor value if it exists */
    evar = cmd_alloc(512 * sizeof(TCHAR));
    if (evar == NULL)
        size = 0;
    else
        size = GetEnvironmentVariable (_T("COPYCMD"), evar, 512);

    if (size > 512)
    {
        TCHAR *old_evar = evar;
        evar = cmd_realloc(evar,size * sizeof(TCHAR) );
        if (evar!=NULL)
            size = GetEnvironmentVariable (_T("COPYCMD"), evar, size);
        else
        {
            size=0;
            evar = old_evar;
        }
    }

    /* check see if we did get any env variable */
    if (size != 0)
    {
        int t = 0;

        /* scan and set the flags */
        for (t = 0; t < size; t++)
        {
            if (_tcsncicmp(_T("/A"),&evar[t],2) == 0)
            {
                dwFlags |=COPY_ASCII;
                t++;
            }
            else if (_tcsncicmp(_T("/B"),&evar[t],2) == 0)
            {
                dwFlags |= COPY_BINARY;
                t++;
            }
            else if (_tcsncicmp(_T("/D"),&evar[t],2) == 0)
            {
                dwFlags |= COPY_DECRYPT;
                t++;
            }
            else if (_tcsncicmp(_T("/V"),&evar[t],2) == 0)
            {
                dwFlags |= COPY_VERIFY;
                t++;
            }
            else if (_tcsncicmp(_T("/N"),&evar[t],2) == 0)
            {
                dwFlags |= COPY_SHORTNAME;
                t++;
            }
            else if (_tcsncicmp(_T("/Y"),&evar[t],2) == 0)
            {
                dwFlags |= COPY_NO_PROMPT;
                t++;
            }
            else if (_tcsncicmp(_T("/-Y"),&evar[t],3) == 0)
            {
                dwFlags |= COPY_PROMPT;
                t+=2;
            }
            else if (_tcsncicmp(_T("/Z"),&evar[t],2) == 0)
            {
                dwFlags |= COPY_PROMPT;
                t++;
            }
        }
    }
    cmd_free(evar);


    /* Split the user input into array */
    arg = split(param, &argc, FALSE, TRUE);
    nFiles = argc;

    /* Read switches and count files */
    for (i = 0; i < argc; i++)
    {
        if (*arg[i] == _T('/'))
        {
            if (_tcslen(arg[i]) >= 2)
            {
                switch (_totupper(arg[i][1]))
                {
                    case _T('A'):
                        dwFlags |= COPY_ASCII;
                        break;

                    case _T('B'):
                        dwFlags |= COPY_BINARY;
                        break;

                    case _T('D'):
                        dwFlags |= COPY_DECRYPT;
                        break;

                    case _T('V'):
                        dwFlags |= COPY_VERIFY;
                        break;

                    case _T('N'):
                        dwFlags |= COPY_SHORTNAME;
                        break;

                    case _T('Y'):
                        dwFlags |= COPY_NO_PROMPT;
                        dwFlags &= ~COPY_PROMPT;
                        break;

                    case _T('-'):
                        if (_tcslen(arg[i]) >= 3)
                            if (_totupper(arg[i][2]) == _T('Y'))
                            {
                                dwFlags &= ~COPY_NO_PROMPT;
                                dwFlags |= COPY_PROMPT;
                            }

                            break;

                    case _T('Z'):
                        dwFlags |= COPY_RESTART;
                        break;

                    default:
                        /* Invalid switch */
                        ConOutResPrintf(STRING_ERROR_INVALID_SWITCH, _totupper(arg[i][1]));
                        nErrorLevel = 1;
                        freep (arg);
                        return 1;
                        break;
                }
            }
            /* If it was a switch, subtract from total arguments */
            nFiles--;
        }
        else
        {
            /* If it isn't a switch then it is the source or destination */
            if (nSrc == -1)
            {
                nSrc = i;
            }
            else if (*arg[i] == _T('+'))
            {
                /* Next file should be appended */
                bMoreFiles = TRUE;
                nFiles -= 1;
            }
            else if (bMoreFiles)
            {
                /* Add this file to the source string
                   this way we can do all checks
                    directly on source string later on */
                TCHAR * ptr;
                int length = (_tcslen(arg[nSrc]) + _tcslen(arg[i]) + 2) * sizeof(TCHAR);
                ptr = cmd_alloc(length);
                if (ptr)
                {
                    _tcscpy(ptr, arg[nSrc]);
                    _tcscat(ptr, _T("|"));
                    _tcscat(ptr, arg[i]);
                    cmd_free(arg[nSrc]);
                    arg[nSrc] = ptr;
                    nFiles -= 1;
                }

                bMoreFiles = FALSE;
            }
            else if (nDes == -1)
            {
                nDes = i;
            }
        }
    }

    /* keep quiet within batch files */
    if (bc != NULL)
    {
        dwFlags |= COPY_NO_PROMPT;
        dwFlags &= ~COPY_PROMPT;
    }

    if (nFiles < 1)
    {
        /* There are not enough files, there has to be at least 1 */
        ConOutResPuts(STRING_ERROR_REQ_PARAM_MISSING);
        freep(arg);
        return 1;
    }

    if (nFiles > 2)
    {
        /* There are too many file names in command */
        ConErrResPrintf(STRING_ERROR_TOO_MANY_PARAMETERS,_T(""));
        nErrorLevel = 1;
        freep(arg);
        return 1;
    }

    if ((_tcschr(arg[nSrc], _T('|')) != NULL) ||
        (_tcschr(arg[nSrc], _T('*')) != NULL) ||
        (_tcschr(arg[nSrc], _T('?')) != NULL) ||
        IsExistingDirectory(arg[nSrc]))
    {
        bMultipleSource = TRUE;
    }

    /* Reusing the number of files variable */
    nFiles = 0;

    /* Check if no destination argument is passed */
    if (nDes == -1)
    {
        /* If no destination was entered then just use
        the current directory as the destination */
        GetCurrentDirectory(MAX_PATH, szDestPath);
    }
    else
    {
        /* Check if the destination is 'x:' */
        if ((arg[nDes][1] == _T(':')) && (arg[nDes][2] == _T('\0')))
        {
            GetRootPath(arg[nDes], szDestPath, MAX_PATH);
        }
        else
        {
            /* If the user entered two file names then form the full string path */
            GetFullPathName(arg[nDes], MAX_PATH, szDestPath, NULL);
        }

        /* Make sure there is an ending slash to the path if the dest is a folder */
        if ((_tcschr(szDestPath, _T('*')) == NULL) &&
            IsExistingDirectory(szDestPath))
        {
            bMultipleDest = TRUE;
            if (szDestPath[_tcslen(szDestPath) -  1] != _T('\\'))
                _tcscat(szDestPath, _T("\\"));
        }

        /* Check if the destination uses wildcards */
        if ((_tcschr(arg[nDes], _T('*')) != NULL) ||
            (_tcschr(arg[nDes], _T('?')) != NULL))
        {
            bMultipleDest = TRUE;
        }
    }

    if (nDes != -1) /* you can only append files when there is a destination */
    {
        if (bMultipleSource && !bMultipleDest)
        {
            /* We have multiple source files, but not multiple destination
               files. This means we are appending the soruce files. */
            bAppend = TRUE;
            if (_tcschr(arg[nSrc], _T('|')) != NULL)
                appendPointer = arg[nSrc];
        }
    }

    /* Save the name the user entered */
    UseThisName = _tcsrchr(szDestPath,_T('\\'));
    if (UseThisName)
    {
        /* Split the name from the path */
        *UseThisName++ = _T('\0');

        /* Check if the dest path ends with '\*' or '\' */
        if (((UseThisName[0] == _T('*')) && (UseThisName[1] == _T('\0'))) ||
            (UseThisName[0] == _T('\0')))
        {
            /* In this case we will be using the same name as the source file
            for the destination file because destination is a folder */
            bSrcName = TRUE;
            UseThisName = NULL;
        }
    }
    else
    {
        /* Something's seriously wrong! */
        UseThisName = szDestPath;
    }

    do
    {
        /* Get the full string of the path to the source file */
        if (_tcschr(arg[nSrc], _T('|')) != NULL)
        {
            /* Reset the source path */
            szSrcPath[0] = _T('\0');

            /* Loop through the source file name and copy all
            the chars one at a time until it gets too + */
            while(TRUE)
            {
                if (appendPointer[0] == _T('|'))
                {
                    /* Skip the | and go to the next file name */
                    appendPointer++;
                    break;
                }
                else if (appendPointer[0] == _T('\0'))
                {
                    bDone = TRUE;
                    break;
                }

                _tcsncat(szSrcPath, appendPointer, 1);
                appendPointer++;
            }

            if (_tcschr(arg[nSrc], _T(',')) != NULL)
            {
                /* Only time there is a , in the source is when they are using touch
                   Cant have a destination and can only have on ,, at the end of the string
                    Cant have more then one file name */
                szTouch = _tcsstr(arg[nSrc], _T("|"));
                if (_tcsncmp(szTouch,_T("|,,\0"), 4) || (nDes != -1))
                {
                    ConErrResPrintf(STRING_ERROR_INVALID_PARAM_FORMAT,arg[nSrc]);
                    nErrorLevel = 1;
                    freep (arg);
                    return 1;
                }
                bTouch = TRUE;
                bDone = TRUE;
            }
        }
        else
        {
            bDone = TRUE;
            _tcscpy(szSrcPath, arg[nSrc]);
        }

        /* "x:" is not a valid source path format. */
        if ((szSrcPath[1] == _T(':')) && (szSrcPath[2] == _T('\0')))
        {
            ConOutPrintf(_T("%s\n"), szSrcPath);
            ConOutFormatMessage(ERROR_FILE_NOT_FOUND, szSrcPath);
            nErrorLevel = 1;
            break;
        }


        /* From this point on, we can assume that the shortest path is 3 letters long
        and that would be [DriveLetter]:\ */

        /* Check if the path has a wildcard */
        bHasWildcard = (_tcschr(szSrcPath, _T('*')) != NULL);

        /* If there is no * in the path name and it is a folder then we will
           need to add a wildcard to the pathname so FindFirstFile comes up
           with all the files in that folder */
        if (!bHasWildcard && IsExistingDirectory(szSrcPath))
        {
            /* If it doesnt have a \ at the end already then on needs to be added */
            if (szSrcPath[_tcslen(szSrcPath) -  1] != _T('\\'))
                _tcscat(szSrcPath, _T("\\"));
            _tcscat(szSrcPath, _T("*"));
            bHasWildcard = TRUE;
        }

        /* If the path ends with '\' add a wildcard at the end */
        if (szSrcPath[_tcslen(szSrcPath) -  1] == _T('\\'))
        {
            _tcscat(szSrcPath, _T("*"));
            bHasWildcard = TRUE;
        }

        /* Get a list of all the files */
        hFile = FindFirstFile(szSrcPath, &findBuffer);

        /* If it couldnt open the file handle, print out the error */
        if (hFile == INVALID_HANDLE_VALUE)
        {
            /* only print source name when more then one file */
            if (bMultipleSource)
                ConOutPrintf(_T("%s\n"), szSrcPath);

            ConOutFormatMessage(GetLastError(), szSrcPath);
            freep(arg);
            nErrorLevel = 1;
            return 1;
        }

        /* Strip the paths back to the folder they are in */
        for (i = (_tcslen(szSrcPath) -  1); i > -1; i--)
            if (szSrcPath[i] != _T('\\'))
                szSrcPath[i] = _T('\0');
            else
                break;

        do
        {
            /* Check Breaker */
            if (CheckCtrlBreak(BREAK_INPUT))
            {
                FindClose(hFile);
                freep(arg);
                return 1;
            }

            /* Set the override to yes each new file */
            nOverwrite = 1;

            /* Ignore the . and .. files */
            if (!_tcscmp(findBuffer.cFileName, _T("."))  ||
                !_tcscmp(findBuffer.cFileName, _T("..")) ||
                findBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            /* Copy the base folder over to a tmp string */
            _tcscpy(tmpDestPath, szDestPath);
            _tcscat(tmpDestPath, _T("\\"));

            /* Can't put a file into a folder that isn't there */
            if (_tcscmp(tmpDestPath, _T("\\\\.\\")) &&
                !IsExistingDirectory(tmpDestPath))
            {
                FindClose(hFile);
                ConOutFormatMessage(GetLastError(), szSrcPath);
                freep(arg);
                nErrorLevel = 1;
                return 1;
            }

            /* Copy over the destination path name */
            if (bSrcName)
                _tcscat(tmpDestPath, findBuffer.cFileName);
            else
            {
                /* If there is no wildcard you can use the name the user entered */
                if ((_tcschr(UseThisName, _T('*')) == NULL) &&
                    (_tcschr(UseThisName, _T('?')) == NULL))
                {
                    _tcscat(tmpDestPath, UseThisName);
                }
                else
                {
                    TCHAR DoneFile[MAX_PATH];

                    BuildFileName(findBuffer.cFileName,
                                  UseThisName,
                                  DoneFile);


                    /* Add the filename to the tmp string path */
                    _tcscat(tmpDestPath, DoneFile);
                }
            }

            /* Build the string path to the source file */
            _tcscpy(tmpSrcPath,szSrcPath);
            _tcscat (tmpSrcPath, findBuffer.cFileName);

            /* Check to see if the file is the same file */
            if (!bTouch && !_tcscmp(tmpSrcPath, tmpDestPath))
            {
                ConOutResPrintf(STRING_COPY_ERROR2);

                nErrorLevel = 1;
                break;
            }

            /* only print source name when more then one file */
            if (bMultipleSource)
                ConOutPrintf(_T("%s\n"), tmpSrcPath);

            /* Handle any overriding / prompting that needs to be done */
            if (((!(dwFlags & COPY_NO_PROMPT) && IsExistingFile (tmpDestPath)) || dwFlags & COPY_PROMPT) && !bTouch)
                nOverwrite = CopyOverwrite(tmpDestPath);
            if (nOverwrite == PROMPT_NO || nOverwrite == PROMPT_BREAK)
                continue;
            if (nOverwrite == PROMPT_ALL || (nOverwrite == PROMPT_YES && bAppend))
                dwFlags |= COPY_NO_PROMPT;

            /* Tell weather the copy was successful or not */
            if (copy(tmpSrcPath,tmpDestPath, bAppend, dwFlags, bTouch))
            {
                nFiles++;
                //LoadString(CMD_ModuleHandle, STRING_MOVE_ERROR1, szMsg, ARRAYSIZE(szMsg));
            }
            else
            {
                /* print out the error message */
                ConOutResPrintf(STRING_COPY_ERROR3);
                ConOutFormatMessage (GetLastError(), szSrcPath);
                nErrorLevel = 1;
            }

        /* Loop through all wildcard files */
        } while (FindNextFile(hFile, &findBuffer));

    /* Loop through all files in src string with a + */
    } while(!bDone);

    /* print out the number of files copied */
    ConOutResPrintf(STRING_COPY_FILE, bAppend ? 1 : nFiles);

    if (hFile) FindClose(hFile);

    if (arg != NULL)
        freep(arg);

    return 0;
}

#endif /* INCLUDE_CMD_COPY */
