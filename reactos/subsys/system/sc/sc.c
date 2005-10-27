#include "sc.h"

HANDLE OutputHandle;
HANDLE InputHandle;

SC_HANDLE hSCManager;

VOID dprintf(TCHAR* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args, fmt);
   wvsprintfA(buffer, fmt, args);
   WriteConsole(OutputHandle, buffer, lstrlenA(buffer), NULL, NULL);
   va_end(args);
}

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
        dprintf("Remote service control is not yet implemented\n");
        return 2;
    }

    hSCManager = OpenSCManager(MachineName, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        dprintf("[SC] OpenSCManager FAILED \n");
        ReportLastError();
        return -1;
    }


    if (_tcsicmp(Command, _T("query")) == 0)
        Query(Args, FALSE);
        
    else if (_tcsicmp(Command, _T("queryex")) == 0)
        Query(Args, TRUE);
        
    else if (_tcsicmp(Command, _T("start")) == 0)
    {
        /*if (! **Args)
            StartUsage();
        else
            Start(Args);*/
    }
    else if (_tcsicmp(Command, _T("pause")) == 0)
        Control(SERVICE_CONTROL_PAUSE, ++Args);

    else if (_tcsicmp(Command, _T("interrogate")) == 0)
        Control(SERVICE_CONTROL_INTERROGATE, ++Args);
        
    else if (_tcsicmp(Command, _T("interrogate")) == 0)
        Control(SERVICE_CONTROL_INTERROGATE, ++Args);
        
    else if (_tcsicmp(Command, _T("continue")) == 0)
        Control(SERVICE_CONTROL_CONTINUE, ++Args);
        
    else if (_tcsicmp(Command, _T("delete")) == 0)
        Delete(Args);
        
    else if (_tcsicmp(Command, _T("create")) == 0)
        Create(Args);
        
    else if (_tcsicmp(Command, _T("control")) == 0)
        Control((DWORD)NULL, Args);
    
    return 0;
}


int main(int argc, char* argv[])
{
    LPTSTR MachineName = NULL;  // remote machine
    LPCTSTR Command = argv[1];  // sc command
    TCHAR **Args = NULL;        // rest of args
    
     /*  initialize standard input / output and get handles */
    AllocConsole();
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

    if (argc < 2)
        return MainUsage();

    /* get server name */
    if ((argv[1][0] == '\\') && (argv[1][1] == '\\'))
    {
        if (argc < 3)
            return MainUsage();

        _tcscpy(MachineName, argv[1]);
        Command = argv[2];
        Args = &argv[3];
        return ScControl(MachineName, Command, Args);
    }
    else
    {
        Args = &argv[2];
        return ScControl(MachineName, Command, Args);
    }

    return MainUsage();
}
