/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS SC utility
 * FILE:        subsys/system/sc/sc.c
 * PURPOSE:     control ReactOS services
 * PROGRAMMERS: Ged Murphy (gedmurphy@gmail.com)
 * REVISIONS:
 *           Ged Murphy 20/10/05 Created
 *
 */

#include "sc.h"

SC_HANDLE hSCManager;

DWORD ReportLastError(VOID)
{
    LPVOID lpMsgBuf;
    DWORD RetVal;

    DWORD ErrorCode = GetLastError();
    if (ErrorCode != ERROR_SUCCESS) 
    {
        if ((RetVal = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                ErrorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                (LPTSTR) &lpMsgBuf,
                0,
                NULL )))
        {
            _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);

            LocalFree(lpMsgBuf);
            /* return number of TCHAR's stored in output buffer
             * excluding '\0' - as FormatMessage does*/
            return RetVal;
        }
    }
    return 0;
}


INT ScControl(LPTSTR MachineName, LPCTSTR Command, TCHAR **Args)
{

    if (MachineName)
    {
        _tprintf(_T("Remote service control is not yet implemented\n"));
        return 2;
    }
/*
    hSCManager = OpenSCManager(MachineName, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %d:\n\n"), GetLastError());
        ReportLastError();
        return -1;
    }
*/

    if (_tcsicmp(Command, _T("query")) == 0)
        Query(Args, FALSE);
        
    else if (_tcsicmp(Command, _T("queryex")) == 0)
        Query(Args, TRUE);
        
    else if (_tcsicmp(Command, _T("start")) == 0)
    {
        if (*Args)
            Start(3, Args);
        else
            StartUsage();
    }
    else if (_tcsicmp(Command, _T("pause")) == 0)
    {
        if (*Args)
            Control(SERVICE_CONTROL_PAUSE, Args);
        else
            PauseUsage();
    }
    else if (_tcsicmp(Command, _T("interrogate")) == 0)
    {
        if (*Args)
            Control(SERVICE_CONTROL_INTERROGATE, Args);
        else
            InterrogateUsage();
    }
    else if (_tcsicmp(Command, _T("stop")) == 0)
    {
        if (*Args)
            Control(SERVICE_CONTROL_STOP, Args);
        else
            StopUsage();
    }
    else if (_tcsicmp(Command, _T("continue")) == 0)
    {
        if (*Args)
            Control(SERVICE_CONTROL_CONTINUE, Args);
        else
            ContinueUsage();
    }
    else if (_tcsicmp(Command, _T("delete")) == 0)
    {
        if (*Args)
            Delete(Args);
        else
            DeleteUsage();
    }
    else if (_tcsicmp(Command, _T("create")) == 0)
    {
        if (*Args)
            Create(Args);
        else
            CreateUsage();
    }
    else if (_tcsicmp(Command, _T("control")) == 0)
    {
        if (*Args)
            Control((DWORD)NULL, ++Args);
        else
            ContinueUsage();
    }
    return 0;
}


int _tmain(DWORD argc, LPCTSTR argv[])
{
    LPTSTR MachineName = NULL;  // remote machine
    LPCTSTR Command = argv[1];  // sc command
    TCHAR **Args = NULL;        // rest of args
    

    if (argc < 2)
        return MainUsage();

    /* get server name */
    if ((argv[1][0] == '\\') && (argv[1][1] == '\\'))
    {
        if (argc < 3)
            return MainUsage();

        _tcscpy(MachineName, argv[1]);
        Command = argv[2];
        Args = (TCHAR **)&argv[3];
        return ScControl(MachineName, Command, Args);
    }
    else
    {
        Args = (TCHAR **)&argv[2];
        return ScControl(MachineName, Command, Args);
    }

    return MainUsage();
}
