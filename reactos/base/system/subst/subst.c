/* PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/subst/subst.c
 * PURPOSE:         Associates a path with a drive letter
 * PROGRAMMERS:     Sam Arun Raj
 */

/* INCLUDES *****************************************************************/

#define LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

void PrintError(DWORD ErrCode)
{
    TCHAR *buffer = (TCHAR*) calloc(2048,
                                    sizeof(TCHAR));
    TCHAR *msg = NULL;

    if (buffer)
    {
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      ErrCode,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (TCHAR*)&msg,
                      0,
                      NULL);
        _sntprintf(buffer,
                   2048,
                   _T("Failed with error code 0x%x: %s\n"),
                   ErrCode,
                   msg);
        _tprintf(_T("%s"),
                 buffer);
        if (msg)
            LocalFree(msg);
        free(buffer);
    }
}

void DisplaySubstUsage()
{
    _tprintf(_T("Associates a path with a drive letter.\n\n"));
    _tprintf(_T("SUBST [drive1: [drive2:]path]\n"));
    _tprintf(_T("SUBST drive1: /D\n\n"));
    _tprintf(_T("   drive1:        Specifies a virtual drive to which you want to assign a path.\n"));
    _tprintf(_T("   [drive2:]path  Specifies a physical drive and path you want to assign to\n"));
    _tprintf(_T("                  a virtual drive.\n"));
    _tprintf(_T("   /D             Deletes a substituted (virtual) drive.\n\n"));
    _tprintf(_T("Type SUBST with no parameters to display a list of current virtual drives.\n"));
}

BOOLEAN IsSubstedDrive(TCHAR *Drive)
{
    BOOLEAN Result = FALSE;
    LPTSTR lpTargetPath = NULL;
    DWORD CharCount, dwSize;

    if (_tcslen(Drive) > 2)
        return FALSE;

    dwSize = sizeof(TCHAR) * MAX_PATH;
    lpTargetPath = (LPTSTR) malloc(sizeof(TCHAR) * MAX_PATH);
    if ( lpTargetPath)
    {
        CharCount = QueryDosDevice(Drive,
                                   lpTargetPath,
                                   dwSize / sizeof(TCHAR));
        while (! CharCount &&
               GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            free(lpTargetPath);
            dwSize *= 2;
            lpTargetPath = (LPTSTR) malloc(dwSize);
            if (lpTargetPath)
            {
                CharCount = QueryDosDevice(Drive,
                                           lpTargetPath,
                                           dwSize / sizeof(TCHAR));
            }
        }

        if (CharCount)
        {
            if ( _tcsncmp(lpTargetPath, _T("\\??\\"), 4) == 0 &&
                ( (lpTargetPath[4] >= _T('A') &&
                lpTargetPath[4] <= _T('Z')) ||
                 (lpTargetPath[4] >= _T('a') &&
                lpTargetPath[4] <= _T('z')) ) )
            {
                Result = TRUE;
            }
        }
        free(lpTargetPath);
    }
    return Result;
}

void DumpSubstedDrives()
{
    TCHAR Drive[3] = _T("A:");
    LPTSTR lpTargetPath = NULL;
    DWORD CharCount, dwSize;
    INT i = 0;

    dwSize = sizeof(TCHAR) * MAX_PATH;
    lpTargetPath = (LPTSTR) malloc(sizeof(TCHAR) * MAX_PATH);
    if (! lpTargetPath)
        return;

    while (i < 26)
    {
        Drive[0] = _T('A') + i;
        CharCount = QueryDosDevice(Drive,
                                   lpTargetPath,
                                   dwSize / sizeof(TCHAR));
        while (! CharCount &&
               GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            free(lpTargetPath);
            dwSize *= 2;
            lpTargetPath = (LPTSTR) malloc(dwSize);
            if (lpTargetPath)
            {
                CharCount = QueryDosDevice(Drive,
                                           lpTargetPath,
                                           dwSize / sizeof(TCHAR));
            }
        }

        if (! CharCount)
        {
            i++;
            continue;
        }
        else
        {
            if ( _tcsncmp(lpTargetPath, _T("\\??\\"), 4) == 0 &&
                ( (lpTargetPath[4] >= _T('A') &&
                lpTargetPath[4] <= _T('Z')) ||
                 (lpTargetPath[4] >= _T('a') &&
                lpTargetPath[4] <= _T('z')) ) )
            {
                _tprintf(_T("%s\\: => %s\n"),
                         Drive,
                         lpTargetPath + 4);
            }
        }
        i++;
    }
    free(lpTargetPath);
}

int DeleteSubst(TCHAR* Drive)
{
    BOOL Result;

    if (_tcslen(Drive) > 2)
    {
        _tprintf(_T("Invalid parameter - %s\n"),
                 Drive);
        return 1;
    }

    if (! IsSubstedDrive(Drive))
    {
        _tprintf(_T("Invalid Parameter - %s\n"),
                Drive);
        return 1;
    }

    Result = DefineDosDevice(DDD_REMOVE_DEFINITION,
                             Drive,
                             NULL);
    if (! Result)
    {
        PrintError(GetLastError());
        return 1;
    }
    return 0;
}

int AddSubst(TCHAR* Drive, TCHAR *Path)
{
    BOOL Result;

    if (_tcslen(Drive) > 2)
    {
        _tprintf(_T("Invalid parameter - %s\n"),
                 Drive);
        return 1;
    }

    if (IsSubstedDrive(Drive))
    {
        _tprintf(_T("Drive already SUBSTed\n"));
        return 1;
    }

    Result = DefineDosDevice(0,
                             Drive,
                             Path);
    if (! Result)
    {
        PrintError(GetLastError());
        return 1;
    }
    return 0;
}

int _tmain(int argc, TCHAR* argv[])
{
    INT i;

    for (i = 0; i < argc; i++)
    {
        if (!_tcsicmp(argv[i], _T("/?")))
        {
            DisplaySubstUsage();
            return 0;
        }
    }

    if (argc < 3)
    {
        if (argc >= 2)
        {
            _tprintf(_T("Invalid parameter - %s\n"),
                     argv[1]);
            return 1;
        }
        DumpSubstedDrives();
        return 0;
    }

    if (argc > 3)
    {
        _tprintf(_T("Incorrect number of parameters - %s\n"),
                 argv[3]);
        return 1;
    }

    if (! _tcsicmp(argv[1], _T("/D")))
        return DeleteSubst(argv[2]);
    if (! _tcsicmp(argv[2], _T("/D")))
        return DeleteSubst(argv[1]);
    return AddSubst(argv[1], argv[2]);
}

/* EOF */
