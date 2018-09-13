/*---------------------------------------------------------------------------
 |   WINVER.C - Windows Version program
 |
 |   History:
 |  03/08/89 Toddla     Created
 |
 *--------------------------------------------------------------------------*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <port1632.h>
#include <stdio.h>
#include "winverp.h"
#include <shellapi.h>

void FileTimeToDateTimeString(
    LPFILETIME pft,
    LPTSTR     pszBuf,
    UINT       cchBuf)
{
    SYSTEMTIME st;
    int cch;

    FileTimeToLocalFileTime(pft, pft);
    FileTimeToSystemTime(pft, &st);

    cch = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf);
    cchBuf -= cch;
    pszBuf += cch - 1;

    *pszBuf++ = TEXT(' ');
    *pszBuf = 0;          // (in case GetTimeFormat doesn't add anything)
    cchBuf--;

    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf);
}

/*----------------------------------------------------------------------------*\
|   WinMain( hInst, hPrev, lpszCmdLine, cmdShow )                              |
|                                                                              |
|   Description:                                                               |
|       The main procedure for the App.  After initializing, it just goes      |
|       into a message-processing loop until it gets a WM_QUIT message         |
|       (meaning the app was closed).                                          |
|                                                                              |
|   Arguments:                                                                 |
|   hInst       instance handle of this instance of the app                    |
|   hPrev       instance handle of previous instance, NULL if first            |
|       lpszCmdLine     ->null-terminated command line                         |
|       cmdShow         specifies how the window is initially displayed        |
|                                                                              |
|   Returns:                                                                   |
|       The exit code as specified in the WM_QUIT message.                     |
|                                                                              |
\*----------------------------------------------------------------------------*/
INT
__cdecl
ModuleEntry()
{
    TCHAR szTitle[32];
    LARGE_INTEGER Time = USER_SHARED_DATA->SystemExpirationDate;

    LoadString(GetModuleHandle(NULL), IDS_APPTITLE, szTitle, 32);

    if (Time.QuadPart) {
        TCHAR szExtra[128];
        TCHAR szTime[128];

        FileTimeToDateTimeString((PFILETIME)&Time, szTime, 128);

        LoadString(GetModuleHandle(NULL), IDS_EVALUATION, szExtra, 128);

        lstrcat(szExtra, szTime);

        ShellAbout(NULL, szTitle, szExtra, NULL);
    } else {
        ShellAbout(NULL, szTitle, NULL, NULL);
    }

    return 0;
}
