/* PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/subst/subst.c
 * PURPOSE:         Associates a path with a drive letter
 * PROGRAMMERS:     Sam Arun Raj
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <stdlib.h>
#include <tchar.h>

#define NDEBUG
#include <debug.h>

#include "resource.h"

/* FUNCTIONS ****************************************************************/

void PrintError(DWORD ErrCode)
{
    TCHAR szFmtString[RC_STRING_MAX_SIZE] = {0};
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
        LoadString(GetModuleHandle(NULL),
                   IDS_FAILED_WITH_ERRORCODE,
                   szFmtString,
                   sizeof(szFmtString) / sizeof(szFmtString[0]));
        _sntprintf(buffer,
                   2048,
                   szFmtString,
                   ErrCode,
                   msg);
        _tprintf(_T("%s"),
                 buffer);
        if (msg)
            LocalFree(msg);
        free(buffer);
    }
}

void DisplaySubstUsage(void)
{
    TCHAR szHelp[RC_STRING_MAX_SIZE] = {0};

    LoadString(GetModuleHandle(NULL),
                IDS_USAGE,
                szHelp,
                sizeof(szHelp) / sizeof(szHelp[0]));
    _tprintf(_T("%s"), szHelp);
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

void DumpSubstedDrives(void)
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
    TCHAR szFmtString[RC_STRING_MAX_SIZE] = {0};

    LoadString(GetModuleHandle(NULL),
                IDS_INVALID_PARAMETER2,
                szFmtString,
                sizeof(szFmtString) / sizeof(szFmtString[0]));

    if (_tcslen(Drive) > 2)
    {
        _tprintf(szFmtString,
                 Drive);
        return 1;
    }

    if (! IsSubstedDrive(Drive))
    {
        _tprintf(szFmtString,
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
    TCHAR szFmtString[RC_STRING_MAX_SIZE] = {0};

    LoadString(GetModuleHandle(NULL),
                IDS_INVALID_PARAMETER2,
                szFmtString,
                sizeof(szFmtString) / sizeof(szFmtString[0]));
    if (_tcslen(Drive) != 2)
    {
        _tprintf(szFmtString,
                 Drive);
        return 1;
    }

    if (Drive[1] != _T(':'))
    {
        _tprintf(szFmtString,
                 Drive);
        return 1;
    }

    if (IsSubstedDrive(Drive))
    {
        LoadString(GetModuleHandle(NULL),
                   IDS_DRIVE_ALREADY_SUBSTED,
                   szFmtString,
                   sizeof(szFmtString) / sizeof(szFmtString[0]));
        _tprintf(szFmtString);
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
    TCHAR szFmtString[RC_STRING_MAX_SIZE] = {0};

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
            LoadString(GetModuleHandle(NULL),
                       IDS_INVALID_PARAMETER,
                       szFmtString,
                       sizeof(szFmtString) / sizeof(szFmtString[0]));
            _tprintf(szFmtString,
                     argv[1]);
            return 1;
        }
        DumpSubstedDrives();
        return 0;
    }

    if (argc > 3)
    {
        LoadString(GetModuleHandle(NULL),
                   IDS_INCORRECT_PARAMETER_COUNT,
                   szFmtString,
                   sizeof(szFmtString) / sizeof(szFmtString[0]));
        _tprintf(szFmtString,
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
