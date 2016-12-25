/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         LGPL - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for SHIM engine caching.
 * PROGRAMMER:      Mark Jansen
 */

#include <apitest.h>

#include <windows.h>

#define WIN32_NO_STATUS
#include <ntndk.h>

enum ServiceCommands
{
    RegisterShimCacheWithHandle = 128,
    RegisterShimCacheWithoutHandle = 129,
};


NTSTATUS CallCacheControl(UNICODE_STRING* PathName, BOOLEAN WithMapping, APPHELPCACHESERVICECLASS Service)
{
    APPHELP_CACHE_SERVICE_LOOKUP CacheEntry = { {0} };
    NTSTATUS Status;
    CacheEntry.ImageName = *PathName;
    if (WithMapping)
    {
        OBJECT_ATTRIBUTES LocalObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        InitializeObjectAttributes(&LocalObjectAttributes, PathName,
            OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtOpenFile(&CacheEntry.ImageHandle,
                    SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_EXECUTE,
                    &LocalObjectAttributes, &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
    else
    {
        CacheEntry.ImageHandle = INVALID_HANDLE_VALUE;
    }
    Status = NtApphelpCacheControl(Service, &CacheEntry);
    if (CacheEntry.ImageHandle != INVALID_HANDLE_VALUE)
        NtClose(CacheEntry.ImageHandle);
    return Status;
}

int InitEnv(UNICODE_STRING* PathName)
{
    NTSTATUS Status = CallCacheControl(PathName, FALSE, ApphelpCacheServiceRemove);
    if (Status == STATUS_INVALID_PARAMETER)
    {
        /* Windows Vista+ has a different layout for APPHELP_CACHE_SERVICE_LOOKUP */
        return 0;
    }
    ok(Status == STATUS_SUCCESS || Status == STATUS_NOT_FOUND,
        "Wrong value for Status, expected: SUCCESS or NOT_FOUND, got: 0x%lx\n",
        Status);
    return 1;
}

void CheckValidation(UNICODE_STRING* PathName)
{
    APPHELP_CACHE_SERVICE_LOOKUP CacheEntry = { {0} };
    NTSTATUS Status;

    /* Validate the handling of a NULL pointer */
    Status = NtApphelpCacheControl(ApphelpCacheServiceRemove, NULL);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup, NULL);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    /* Validate the handling of a NULL pointer inside the struct */
    Status = NtApphelpCacheControl(ApphelpCacheServiceRemove, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    /* Just call the dump function */
    Status = NtApphelpCacheControl(ApphelpCacheServiceDump, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Validate the handling of an invalid handle inside the struct */
    CacheEntry.ImageName = *PathName;
    CacheEntry.ImageHandle = (HANDLE)2;
    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    /* Validate the handling of an invalid service number */
    Status = NtApphelpCacheControl(999, NULL);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = NtApphelpCacheControl(999, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
}

static BOOLEAN RequestAddition(SC_HANDLE service_handle, BOOLEAN WithMapping)
{
    SERVICE_STATUS Status;
    ControlService(service_handle, WithMapping ? RegisterShimCacheWithHandle :
                    RegisterShimCacheWithoutHandle, &Status);
    /* TODO: how to get a return code from the service? */
    return TRUE;
}

static void RunApphelpCacheControlTests(SC_HANDLE service_handle)
{
    WCHAR szPath[MAX_PATH];
    UNICODE_STRING ntPath;
    BOOLEAN Result;
    NTSTATUS Status;
    APPHELP_CACHE_SERVICE_LOOKUP CacheEntry;

    GetModuleFileNameW(NULL, szPath, sizeof(szPath) / sizeof(szPath[0]));
    Result = RtlDosPathNameToNtPathName_U(szPath, &ntPath, NULL, NULL);
    ok(Result == TRUE, "RtlDosPathNameToNtPathName_U\n");
    if (!InitEnv(&ntPath))
    {
        skip("NtApphelpCacheControl expects a different structure layout\n");
        return;
    }
    /* At this point we have made sure that our binary is not present in the cache,
        and that the NtApphelpCacheControl function expects the struct layout we use. */
    CheckValidation(&ntPath);

    /* We expect not to find it */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    /* First we add our process without a file handle (so it will be registered without file info) */
    RequestAddition(service_handle, FALSE);

    /* now we try to find it without validating file info */
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* when validating file info the cache notices the file is wrong, so it is dropped from the cache */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);
    /* making the second check without info also fail. */
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);


    /* Now we add the file with file info */
    RequestAddition(service_handle, TRUE);

    /* so both checks should succeed */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We know the file is in the cache now (assuming previous tests succeeded,
        let's test invalid handle behavior */
    CacheEntry.ImageName = ntPath;
    CacheEntry.ImageHandle = 0;
    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    /* re-add it for the next test */
    RequestAddition(service_handle, TRUE);
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);
    CacheEntry.ImageHandle = (HANDLE)1;
    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    /* and again */
    RequestAddition(service_handle, TRUE);
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);
    CacheEntry.ImageHandle = (HANDLE)0x80000000;
    Status = NtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    RtlFreeHeap(RtlGetProcessHeap(), 0, ntPath.Buffer);
}


/* Most service related code was taken from services_winetest:service and modified for usage here
    The rest came from MSDN */

static SERVICE_STATUS_HANDLE (WINAPI *pRegisterServiceCtrlHandlerExA)(LPCSTR,LPHANDLER_FUNCTION_EX,LPVOID);
static char service_name[100] = "apphelp_test_service";
static HANDLE service_stop_event;
static SERVICE_STATUS_HANDLE service_status;

static BOOLEAN RegisterInShimCache(BOOLEAN WithMapping)
{
    WCHAR szPath[MAX_PATH];
    UNICODE_STRING ntPath;
    BOOLEAN Result;
    NTSTATUS Status;
    GetModuleFileNameW(NULL, szPath, sizeof(szPath) / sizeof(szPath[0]));
    Result = RtlDosPathNameToNtPathName_U(szPath, &ntPath, NULL, NULL);
    if (!Result)
    {
        DbgPrint("RegisterInShimCache: RtlDosPathNameToNtPathName_U failed\n");
        return FALSE;
    }

    Status = CallCacheControl(&ntPath, WithMapping, ApphelpCacheServiceUpdate);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("RegisterInShimCache: CallCacheControl failed\n");
        RtlFreeHeap(RtlGetProcessHeap(), 0, ntPath.Buffer);
        return FALSE;
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, ntPath.Buffer);
    return TRUE;
}


static DWORD WINAPI service_handler(DWORD ctrl, DWORD event_type, void *event_data, void *context)
{
    SERVICE_STATUS status = {0};
    status.dwServiceType = SERVICE_WIN32;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus(service_status, &status);
        SetEvent(service_stop_event);
        return NO_ERROR;
    case RegisterShimCacheWithHandle:
        if (!RegisterInShimCache(TRUE))
        {
            /* TODO: how should we communicate a failure? */
        }
        break;
    case RegisterShimCacheWithoutHandle:
        if (!RegisterInShimCache(FALSE))
        {
            /* TODO: how should we communicate a failure? */
        }
        break;
    default:
        DbgPrint("Unhandled: %d\n", ctrl);
        break;
    }
    status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(service_status, &status);
    return NO_ERROR;
}

static void WINAPI service_main(DWORD argc, char **argv)
{
    SERVICE_STATUS status = {0};
    service_status = pRegisterServiceCtrlHandlerExA(service_name, service_handler, NULL);
    if(!service_status)
        return;

    status.dwServiceType = SERVICE_WIN32;
    status.dwCurrentState = SERVICE_RUNNING;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus(service_status, &status);

    WaitForSingleObject(service_stop_event, INFINITE);

    status.dwCurrentState = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus(service_status, &status);
}

static SC_HANDLE InstallService(SC_HANDLE scm_handle)
{
    char service_cmd[MAX_PATH+150], *ptr;
    SC_HANDLE service;

    ptr = service_cmd + GetModuleFileNameA(NULL, service_cmd, MAX_PATH);
    strcpy(ptr, " NtApphelpCacheControl service");
    ptr += strlen(ptr);

    service = CreateServiceA(scm_handle, service_name, service_name, GENERIC_ALL,
                             SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
                             service_cmd, NULL, NULL, NULL, NULL, NULL);
    if (!service)
    {
        skip("Could not create helper service\n");
        return NULL;
    }
    return service;
}

static void WaitService(SC_HANDLE service_handle, DWORD Status, SERVICE_STATUS_PROCESS* ssp)
{
    DWORD dwBytesNeeded;
    DWORD dwStartTime = GetTickCount();
    while (ssp->dwCurrentState != Status)
    {
        Sleep(40);
        if (!QueryServiceStatusEx(service_handle, SC_STATUS_PROCESS_INFO,
            (LPBYTE)ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ))
        {
            ok(0, "QueryServiceStatusEx failed waiting for %lu\n", Status);
            break;
        }
        if ((GetTickCount() - dwStartTime) > 1000)
        {
            ok(0, "Timeout waiting for (%lu) from service, is: %lu.\n",
                Status, ssp->dwCurrentState);
            break;
        }
    }
}

static void RunTest()
{
    SC_HANDLE scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE service_handle = InstallService(scm_handle);
    if (service_handle)
    {
        SERVICE_STATUS_PROCESS ssp = {0};
        BOOL res = StartServiceA(service_handle, 0, NULL);
        if (res)
        {
            WaitService(service_handle, SERVICE_RUNNING, &ssp);
            RunApphelpCacheControlTests(service_handle);
            ControlService(service_handle, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp);
            WaitService(service_handle, SERVICE_STOPPED, &ssp);
        }
        else
        {
            skip("Could not start helper service\n");
        }
        DeleteService(service_handle);
    }
    CloseServiceHandle(scm_handle);
}

START_TEST(NtApphelpCacheControl)
{
    char **argv;
    int argc;

    pRegisterServiceCtrlHandlerExA = (void*)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegisterServiceCtrlHandlerExA");
    if (!pRegisterServiceCtrlHandlerExA)
    {
        win_skip("RegisterServiceCtrlHandlerExA not available, skipping tests\n");
        return;
    }
    argc = winetest_get_mainargs(&argv);
    if(argc < 3)
    {
        RunTest();
    }
    else
    {
        SERVICE_TABLE_ENTRYA servtbl[] = {
            {service_name, service_main},
            {NULL, NULL}
        };
        service_stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);
        StartServiceCtrlDispatcherA(servtbl);
        Sleep(50);
        CloseHandle(service_stop_event);
    }
}


