#include "sc.h"

extern SC_HANDLE hSCManager; // declared in sc.c

BOOL Start(INT ArgCount, TCHAR **Args)
{
    SC_HANDLE hSc;
    LPCTSTR ServiceName = *Args++;
    LPCTSTR *ServiceArgs = &*Args;

    hSc = OpenService(hSCManager, ServiceName, SERVICE_ALL_ACCESS);

    if (hSc == NULL)
    {
        dprintf("openService failed\n");
        ReportLastError();
        return FALSE;
    }

    if (! StartService(hSc, ArgCount, ServiceArgs))
    {
        dprintf("DeleteService failed\n");
        ReportLastError();
        return FALSE;
    }

    CloseServiceHandle(hSc);
    return TRUE;

}


BOOL Create(TCHAR **Args)
{
    SC_HANDLE hSc;
    LPCTSTR Name = *Args;
    LPCTSTR BinaryPathName = *++Args;
    

    hSc = CreateService(hSCManager,
                        Name,
                        Name,
                        SERVICE_ALL_ACCESS,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_DEMAND_START,
                        SERVICE_ERROR_NORMAL,
                        BinaryPathName,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
                        
    if (hSc == NULL)
    {
        dprintf("CreateService failed (%d)\n");
        ReportLastError();
        return FALSE;
    }
    else
    {
        CloseServiceHandle(hSc);
        return TRUE;
    }
}

BOOL Delete(TCHAR **Args)
{
    SC_HANDLE hSc;
    LPCTSTR ServiceName = *Args;
    
    hSc = OpenService(hSCManager, ServiceName, DELETE);
    
    if (hSc == NULL)
    {
        dprintf("openService failed\n");
        ReportLastError();
        return FALSE;
    }
    
    if (! DeleteService(hSc))
    {
        dprintf("DeleteService failed\n");
        ReportLastError();
        return FALSE;
    }

    CloseServiceHandle(hSc);
    return TRUE;
}


BOOL Control(DWORD Control, TCHAR **Args)
{
    SC_HANDLE hSc;
    SERVICE_STATUS Status;
    LPCTSTR ServiceName = *Args;
    

    hSc = OpenService(hSCManager, ServiceName, DELETE);

    if (hSc == NULL)
    {
        dprintf("openService failed\n");
        ReportLastError();
        return FALSE;
    }

    if (! ControlService(hSc, Control, &Status))
    {
        dprintf("controlService failed\n");
        ReportLastError();
        return FALSE;
    }

    CloseServiceHandle(hSc);
    return TRUE;

}


