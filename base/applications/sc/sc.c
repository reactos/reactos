/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/sc/sc.c
 * PURPOSE:     parse command line
 * COPYRIGHT:   Copyright 2005 - 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "sc.h"

SC_HANDLE hSCManager;

VOID
ReportLastError(VOID)
{
    LPVOID lpMsgBuf;
    DWORD RetVal;

    DWORD ErrorCode = GetLastError();
    if (ErrorCode != ERROR_SUCCESS)
    {
        RetVal = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               ErrorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                               (LPTSTR) &lpMsgBuf,
                               0,
                               NULL );

        if (RetVal != 0)
        {
            _tprintf(_T("%s"), (LPTSTR)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
    }
}


static INT
ScControl(LPCTSTR Server,       // remote machine name
          LPCTSTR Command,      // sc command
          LPCTSTR *ServiceArgs, // any options
          DWORD ArgCount)       // argument counter
{
    LPCTSTR ServiceName = NULL;

    if (Server)
    {
        _tprintf(_T("Remote service control is not yet implemented\n"));
        return 2;
    }

    if (!lstrcmpi(Command, _T("query")))
    {
        Query(ServiceArgs,
              ArgCount,
              FALSE);
    }
    else if (!lstrcmpi(Command, _T("queryex")))
    {
        Query(ServiceArgs,
              ArgCount,
              TRUE);
    }
    else if (!lstrcmpi(Command, _T("start")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (ServiceName)
        {
            Start(ServiceName,
                  ServiceArgs,
                  ArgCount);
        }
        else
            StartUsage();
    }
    else if (!lstrcmpi(Command, _T("pause")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (ServiceName)
        {
            Control(SERVICE_CONTROL_PAUSE,
                    ServiceName,
                    ServiceArgs,
                    ArgCount);
        }
        else
            PauseUsage();
    }
    else if (!lstrcmpi(Command, _T("interrogate")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (ServiceName)
        {
            Control(SERVICE_CONTROL_INTERROGATE,
                    ServiceName,
                    ServiceArgs,
                    ArgCount);
        }
        else
            InterrogateUsage();
    }
    else if (!lstrcmpi(Command, _T("stop")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (ServiceName)
        {
            Control(SERVICE_CONTROL_STOP,
                    ServiceName,
                    ServiceArgs,
                    ArgCount);
        }
        else
            StopUsage();
    }
    else if (!lstrcmpi(Command, _T("continue")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (ServiceName)
        {
            Control(SERVICE_CONTROL_CONTINUE,
                    ServiceName,
                    ServiceArgs,
                    ArgCount);
        }
        else
            ContinueUsage();
    }
    else if (!lstrcmpi(Command, _T("delete")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (ServiceName)
            Delete(ServiceName);
        else
            DeleteUsage();
    }
    else if (!lstrcmpi(Command, _T("create")))
    {
        ServiceName = *ServiceArgs++;
        ArgCount--;

        if (*ServiceArgs)
            Create(ServiceName,
                   ServiceArgs);
        else
            CreateUsage();
    }
    else if (!lstrcmpi(Command, _T("control")))
    {
        INT CtlValue;

        ServiceName = *ServiceArgs++;
        ArgCount--;

        CtlValue = _ttoi(ServiceArgs[0]);
        ServiceArgs++;
        ArgCount--;

        if (ServiceName)
        {
            if ((CtlValue >=128) && CtlValue <= 255)
            {
                Control(CtlValue,
                        ServiceName,
                        ServiceArgs,
                        ArgCount);

                return 0;
            }
        }

        ContinueUsage();
    }
    return 0;
}

#if defined(_UNICODE) && defined(__GNUC__)
static
#endif

int _tmain(int argc, LPCTSTR argv[])
{
    LPCTSTR Server = NULL;   // remote machine
    LPCTSTR Command = NULL;  // sc command
    LPCTSTR *Args = NULL;    // Any remaining args

    if (argc < 2)
    {
        MainUsage();
        return -1;
    }

    /* get server name */
    if ((argv[1][0] == '\\') && (argv[1][1] == '\\'))
    {
        if (argc < 3)
        {
            MainUsage();
            return -1;
        }

        Server = argv[1];
        Command = argv[2];
        if (argc > 3)
            Args = &argv[3];

        return ScControl(Server,
                         Command,
                         Args,
                         argc-3);
    }
    else
    {
        Command = argv[1];
        if (argc > 2)
            Args = &argv[2];

        return ScControl(Server,
                         Command,
                         Args,
                         argc-2);
    }
}


#if defined(_UNICODE) && defined(__GNUC__)
/* HACK - MINGW HAS NO OFFICIAL SUPPORT FOR wmain()!!! */
int main( int argc, char **argv )
{
    WCHAR **argvW;
    int i, j, Ret = 1;

    if ((argvW = (WCHAR**) malloc(argc * sizeof(WCHAR*))))
    {
        /* convert the arguments */
        for (i = 0, j = 0; i < argc; i++)
        {
            if (!(argvW[i] = (WCHAR*) malloc((strlen(argv[i]) + 1) * sizeof(WCHAR))))
            {
                j++;
            }
            swprintf(argvW[i], L"%hs", argv[i]);
        }

        if (j == 0)
        {
            /* no error converting the parameters, call wmain() */
            Ret = wmain(argc, (LPCTSTR *)argvW);
        }

        /* free the arguments */
        for (i = 0; i < argc; i++)
        {
            if (argvW[i])
                free(argvW[i]);
        }
        free(argvW);
    }

    return Ret;
}
#endif
