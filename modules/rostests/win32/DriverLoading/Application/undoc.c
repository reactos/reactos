#include "DriverTester.h"

static BOOL
SetPrivilege(BOOL bSet)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES,
                          &hToken))
    {
        return FALSE;
    }

    if(!LookupPrivilegeValue(NULL,
                             SE_LOAD_DRIVER_NAME,
                             &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;

    if (bSet)
    {
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }
    else
    {
        tp.Privileges[0].Attributes = 0;
    }

    AdjustTokenPrivileges(hToken,
                          FALSE,
                          &tp,
                          sizeof(TOKEN_PRIVILEGES),
                          NULL,
                          NULL);
    if (GetLastError() != ERROR_SUCCESS)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    return TRUE;
}


BOOL
ConvertPath(LPCWSTR lpPath,
            LPWSTR lpDevice)
{
    LPWSTR lpFullPath = NULL;
    DWORD size;

    if (lpPath)
    {
        size = GetLongPathNameW(lpPath,
                                0,
                                0);
        if (!size)
            return FALSE;

        size = (size + 1) * sizeof(WCHAR);

        lpFullPath = HeapAlloc(GetProcessHeap(),
                               0,
                               size);
        if (!lpFullPath)
            return FALSE;

        if (GetLongPathNameW(lpPath,
                             lpFullPath,
                             size))
        {
            HANDLE hDevice;
            POBJECT_NAME_INFORMATION pObjName;
            NTSTATUS Status;
            DWORD len;

            hDevice = CreateFileW(lpFullPath,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

            HeapFree(GetProcessHeap(), 0, lpFullPath);

            if(hDevice == INVALID_HANDLE_VALUE)
            {
                wprintf(L"[%x] Failed to open %s\n", GetLastError(), DRIVER_NAME);
                return FALSE;
            }

            size = MAX_PATH * sizeof(WCHAR);
            pObjName = HeapAlloc(GetProcessHeap(), 0, size);
            if (!pObjName)
                return FALSE;

            Status = NtQueryObject(hDevice,
                                   ObjectNameInformation,
                                   pObjName,
                                   size,
                                   &size);
            if (Status == STATUS_SUCCESS)
            {
                len = pObjName->Name.Length / sizeof(WCHAR);
                wcsncpy(lpDevice, pObjName->Name.Buffer, len);
                lpDevice[len] = UNICODE_NULL;

                HeapFree(GetProcessHeap(), 0, pObjName);

                return TRUE;
            }

            HeapFree(GetProcessHeap(), 0, pObjName);
        }
    }

    return FALSE;
}


BOOL
NtStartDriver(LPCWSTR lpService)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status = -1;

    wcscpy(szDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(szDriverPath,
           lpService);

    RtlInitUnicodeString(&DriverPath,
                         szDriverPath);

    if (SetPrivilege(TRUE))
    {
        Status = NtLoadDriver(&DriverPath);
        if (Status != STATUS_SUCCESS)
        {
            DWORD err = RtlNtStatusToDosError(Status);
            wprintf(L"NtUnloadDriver failed [%lu]\n", err);
        }

        SetPrivilege(FALSE);
    }

    return (Status == STATUS_SUCCESS);
}


BOOL
NtStopDriver(LPCWSTR lpService)
{
    WCHAR szDriverPath[MAX_PATH];
    UNICODE_STRING DriverPath;
    NTSTATUS Status = -1;

    wcscpy(szDriverPath,
           L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    wcscat(szDriverPath,
           lpService);

    RtlInitUnicodeString(&DriverPath,
                         szDriverPath);

    if (SetPrivilege(TRUE))
    {
        Status = NtUnloadDriver(&DriverPath);
        if (Status != STATUS_SUCCESS)
        {
            DWORD err = RtlNtStatusToDosError(Status);
            wprintf(L"NtUnloadDriver failed [%lu]\n", err);
        }

        SetPrivilege(FALSE);
    }

    return (Status == STATUS_SUCCESS);
}


//
// We shouldn't be able to call this from umode.
// Returns true if
//
BOOL
LoadVia_SystemLoadGdiDriverInformation(LPWSTR lpDriverPath)
{
    NTSTATUS Status;
    SYSTEM_GDI_DRIVER_INFORMATION Buffer;
    DWORD bufSize;

    bufSize = sizeof(SYSTEM_GDI_DRIVER_INFORMATION);

    ZeroMemory(&Buffer, bufSize);
    RtlInitUnicodeString(&Buffer.DriverName, lpDriverPath);

    if (SetPrivilege(TRUE))
    {
        Status = NtSetSystemInformation(SystemLoadGdiDriverInformation,
                                        &Buffer,
                                        bufSize);
        if (Status == STATUS_PRIVILEGE_NOT_HELD)
        {
            wprintf(L"SystemLoadGdiDriverInformation can only be used in kmode.\n");
        }
        else if (Status == STATUS_SUCCESS)
        {
            wprintf(L"SystemLoadGdiDriverInformation incorrectly loaded the driver\n");
            NtUnloadDriver(&Buffer.DriverName);

            return TRUE;
        }
        else
        {
            DWORD err = RtlNtStatusToDosError(Status);
            wprintf(L"LoadVia_SystemLoadGdiDriverInformation failed [%lu]\n", err);
        }

        SetPrivilege(FALSE);
    }

    return FALSE;
}


BOOL
LoadVia_SystemExtendServiceTableInformation(LPWSTR lpDriverPath)
{
    NTSTATUS Status;
    UNICODE_STRING Buffer;
    DWORD bufSize;

    RtlInitUnicodeString(&Buffer, lpDriverPath);
    bufSize = sizeof(UNICODE_STRING);

    if (SetPrivilege(TRUE))
    {
        Status = NtSetSystemInformation(SystemExtendServiceTableInformation,
                                        &Buffer,
                                        bufSize);
        if (Status == STATUS_PRIVILEGE_NOT_HELD)
        {
            wprintf(L"SystemExtendServiceTableInformation can only be used in kmode.\n");
        }
        else if (Status == STATUS_SUCCESS)
        {
            wprintf(L"SystemExtendServiceTableInformation incorrectly loaded the driver\n");
            NtUnloadDriver(&Buffer);

            return TRUE;
        }
        else
        {
            DWORD err = RtlNtStatusToDosError(Status);
            wprintf(L"LoadVia_SystemExtendServiceTableInformation failed [%lu] - 0x%x\n", err, Status);
        }

        SetPrivilege(FALSE);
    }

    return FALSE;
}

