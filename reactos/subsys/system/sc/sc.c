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


INT ScControl(LPTSTR MachineName,   // remote machine name
              LPCTSTR Command,      // sc command
              LPCTSTR ServiceName,  // name of service
              LPCTSTR *ServiceArgs, // any options
              DWORD ArgCount)       // argument counter
{
    /* count trailing arguments */
    ArgCount -= 3;

    if (MachineName)
    {
        _tprintf(_T("Remote service control is not yet implemented\n"));
        return 2;
    }

    hSCManager = OpenSCManager(MachineName, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        _tprintf(_T("[SC] OpenSCManager FAILED %lu:\n\n"), GetLastError());
        ReportLastError();
        return -1;
    }

    /* emurate command */
    if (_tcsicmp(Command, _T("query")) == 0)
        Query(ServiceName, ServiceArgs, FALSE);
        
    else if (_tcsicmp(Command, _T("queryex")) == 0)
        Query(ServiceName, ServiceArgs, TRUE);
        
    else if (_tcsicmp(Command, _T("start")) == 0)
    {
        if (ServiceName)
            Start(ServiceName, ServiceArgs, ArgCount);
        else
            StartUsage();
    }
    else if (_tcsicmp(Command, _T("pause")) == 0)
    {
        if (ServiceName)
            Control(SERVICE_CONTROL_PAUSE, ServiceName, ServiceArgs);
        else
            PauseUsage();
    }
    else if (_tcsicmp(Command, _T("interrogate")) == 0)
    {
        if (ServiceName)
            Control(SERVICE_CONTROL_INTERROGATE, ServiceName, ServiceArgs);
        else
            InterrogateUsage();
    }
    else if (_tcsicmp(Command, _T("stop")) == 0)
    {
        if (ServiceName)
            Control(SERVICE_CONTROL_STOP, ServiceName, ServiceArgs);
        else
            StopUsage();
    }
    else if (_tcsicmp(Command, _T("continue")) == 0)
    {
        if (ServiceName)
            Control(SERVICE_CONTROL_CONTINUE, ServiceName, ServiceArgs);
        else
            ContinueUsage();
    }
    else if (_tcsicmp(Command, _T("delete")) == 0)
    {
        if (ServiceName)
            Delete(ServiceName);
        else
            DeleteUsage();
    }
    else if (_tcsicmp(Command, _T("create")) == 0)
    {
        if (*ServiceArgs)
            Create(ServiceName, ServiceArgs);
        else
            CreateUsage();
    }
    else if (_tcsicmp(Command, _T("control")) == 0)
    {
        if (ServiceName)
            Control((DWORD)NULL, ServiceName, ServiceArgs);
        else
            ContinueUsage();
    }
    return 0;
}


int _tmain(int argc, LPCTSTR argv[])
{
    LPTSTR MachineName = NULL;   // remote machine
    LPCTSTR Command = NULL;      // sc command
    LPCTSTR ServiceName = NULL;  // Name of service

    if (argc < 2)
        return MainUsage();

    /* get server name */
    if ((argv[1][0] == '\\') && (argv[1][1] == '\\'))
    {
        if (argc < 3)
            return MainUsage();

        _tcscpy(MachineName, argv[1]);
        Command = argv[2];
        if (argc > 3)
            ServiceName = argv[3];
        return ScControl(MachineName, Command, ServiceName,  &argv[4], argc);
    }
    else
    {
        Command = argv[1];
        if (argc > 2)
            ServiceName = argv[2];
        return ScControl(MachineName, Command, ServiceName, &argv[3], argc);
    }

    return MainUsage();
}
