/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         LGPL - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for SHIM engine caching.
 * PROGRAMMER:      Mark Jansen
 */

// NB:
// UNICODE_STRING paths are assigned (shared buffer), not duplicated (deep-copied) :-|
// IsWindows7OrGreater()s are always false, as test is skipped earlier on NT6+.

#include "precomp.h"

#include <winsvc.h>
#include <versionhelpers.h>

enum ServiceCommands
{
    RegisterShimCacheWithHandle = 128,
    RegisterShimCacheWithoutHandle = 129,
};

static NTSTATUS (NTAPI *pNtApphelpCacheControl)(APPHELPCACHESERVICECLASS, PAPPHELP_CACHE_SERVICE_LOOKUP);

/* Check that NULL handle fails. (INVALID_HANDLE_VALUE is expected.) */
void CallCacheControl_WithNullHandle(UNICODE_STRING* PathName, APPHELPCACHESERVICECLASS Service)
{
    NTSTATUS Status;
    APPHELP_CACHE_SERVICE_LOOKUP CacheEntry;

    CacheEntry.ImageName = *PathName;
    CacheEntry.ImageHandle = NULL;
    Status = pNtApphelpCacheControl(Service, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, PathName, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, NULL);

    if (CacheEntry.ImageHandle != NULL)
        NtClose(CacheEntry.ImageHandle);
}

NTSTATUS CallCacheControl(UNICODE_STRING* PathName, BOOLEAN WithMapping, APPHELPCACHESERVICECLASS Service)
{
    APPHELP_CACHE_SERVICE_LOOKUP CacheEntry;
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

    Status = pNtApphelpCacheControl(Service, &CacheEntry);
    ok(!memcmp(&CacheEntry.ImageName, PathName, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    if (WithMapping)
    {
        ok(CacheEntry.ImageHandle != NULL, "CacheEntry.ImageHandle is NULL\n");
        ok(CacheEntry.ImageHandle != INVALID_HANDLE_VALUE, "CacheEntry.ImageHandle is INVALID_HANDLE_VALUE\n");
    }
    else
    {
        ok_hdl(CacheEntry.ImageHandle, INVALID_HANDLE_VALUE);;
    }

    if (CacheEntry.ImageHandle != INVALID_HANDLE_VALUE)
        NtClose(CacheEntry.ImageHandle);

    return Status;
}

int InitEnv(UNICODE_STRING* PathName)
{
    NTSTATUS Status;

    CallCacheControl_WithNullHandle(PathName, ApphelpCacheServiceRemove);

    Status = CallCacheControl(PathName, FALSE, ApphelpCacheServiceRemove);

    if (Status == STATUS_INVALID_PARAMETER)
    {
        /* NT6+ has a different layout for APPHELP_CACHE_SERVICE_LOOKUP */
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
    Status = pNtApphelpCacheControl(ApphelpCacheServiceRemove, NULL);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, NULL);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    /* Validate the handling of a NULL pointer inside the struct, with a NULL handle */
    Status = pNtApphelpCacheControl(ApphelpCacheServiceRemove, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(CacheEntry.ImageName.Length == 0 &&
       CacheEntry.ImageName.MaximumLength == 0 &&
       CacheEntry.ImageName.Buffer == NULL,
       "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, NULL);
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(CacheEntry.ImageName.Length == 0 &&
       CacheEntry.ImageName.MaximumLength == 0 &&
       CacheEntry.ImageName.Buffer == NULL,
       "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, NULL);

    /* Validate the handling of a NULL pointer inside the struct, without file info */
    CacheEntry.ImageHandle = INVALID_HANDLE_VALUE;
    Status = pNtApphelpCacheControl(ApphelpCacheServiceRemove, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(CacheEntry.ImageName.Length == 0 &&
       CacheEntry.ImageName.MaximumLength == 0 &&
       CacheEntry.ImageName.Buffer == NULL,
       "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, INVALID_HANDLE_VALUE);
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(CacheEntry.ImageName.Length == 0 &&
       CacheEntry.ImageName.MaximumLength == 0 &&
       CacheEntry.ImageName.Buffer == NULL,
       "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, INVALID_HANDLE_VALUE);

    /* Just call the dump function */
    Status = pNtApphelpCacheControl(ApphelpCacheServiceDump, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Validate the handling of an invalid handle inside the struct */
    CacheEntry.ImageName = *PathName;
    CacheEntry.ImageHandle = (HANDLE)2;
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, IsWindows7OrGreater() ? STATUS_NOT_FOUND : STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, PathName, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, (HANDLE)2);

    /* Validate the handling of an invalid service number */
    Status = pNtApphelpCacheControl(999, NULL);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pNtApphelpCacheControl(999, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, PathName, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, (HANDLE)2);
    CacheEntry.ImageHandle = INVALID_HANDLE_VALUE;
    Status = pNtApphelpCacheControl(999, &CacheEntry);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, PathName, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, INVALID_HANDLE_VALUE);
    // TODO: Test with actual handle too?
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

    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    Result = RtlDosPathNameToNtPathName_U(szPath, &ntPath, NULL, NULL);
    ok(Result, "RtlDosPathNameToNtPathName_U failed\n");

    if (!InitEnv(&ntPath))
    {
        skip("NtApphelpCacheControl expects a different structure layout (as on NT6+)\n");
        return;
    }

    /* At this point we have made sure that our binary is not present in the cache,
        and that the NtApphelpCacheControl function expects the struct layout we use */
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
    /* when validating file info, the cache removes the entry without file info */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);
    /* making a second check without file info fails then */
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    /* Now we add the file with file info */
    RequestAddition(service_handle, TRUE);

    /* so both checks should succeed */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We know the file is in the cache now (assuming previous tests succeeded),
        let's test NULL handle behavior */
    CacheEntry.ImageName = ntPath;
    CacheEntry.ImageHandle = NULL;
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, IsWindows7OrGreater() ? STATUS_NOT_FOUND : STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, &ntPath, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, NULL);

    /* Valid entry is still cached */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We know the file is in the cache now (assuming previous tests succeeded),
        let's test invalid handle behavior */
    // INVALID_HANDLE_VALUE is already tested by CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup).
    CacheEntry.ImageHandle = (HANDLE)1;
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, IsWindows7OrGreater() ? STATUS_NOT_FOUND : STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, &ntPath, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
    ok_hdl(CacheEntry.ImageHandle, (HANDLE)1);

    /* Valid entry is still cached */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* and again */
#ifdef _WIN64
    CacheEntry.ImageHandle = (HANDLE)0x8000000000000000ULL;
#else
    CacheEntry.ImageHandle = (HANDLE)0x80000000UL;
#endif
    Status = pNtApphelpCacheControl(ApphelpCacheServiceLookup, &CacheEntry);
    ok_ntstatus(Status, IsWindows7OrGreater() ? STATUS_NOT_FOUND : STATUS_INVALID_PARAMETER);
    ok(!memcmp(&CacheEntry.ImageName, &ntPath, sizeof(CacheEntry.ImageName)), "CacheEntry.ImageName was modified\n");
#ifdef _WIN64
    ok_hdl(CacheEntry.ImageHandle, (HANDLE)0x8000000000000000ULL);
#else
    ok_hdl(CacheEntry.ImageHandle, (HANDLE)0x80000000UL);
#endif

    /* Valid entry is still cached */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Remove the entry, without file info */
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceRemove);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Valid entry was removed from the cache */
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    RequestAddition(service_handle, TRUE);

    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Remove the entry, with file info */
    Status = CallCacheControl(&ntPath, TRUE, ApphelpCacheServiceRemove);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Valid entry was removed from the cache */
    Status = CallCacheControl(&ntPath, FALSE, ApphelpCacheServiceLookup);
    ok_ntstatus(Status, STATUS_NOT_FOUND);

    RtlFreeHeap(RtlGetProcessHeap(), 0, ntPath.Buffer);
}


/* Most service related code was taken from services_winetest:service and modified for usage here
    The rest came from MSDN */

static char service_name[100] = "apphelp_test_service";
static HANDLE service_stop_event;
static SERVICE_STATUS_HANDLE service_status;

static BOOLEAN RegisterInShimCache(BOOLEAN WithMapping)
{
    WCHAR szPath[MAX_PATH];
    UNICODE_STRING ntPath;
    BOOLEAN Result;
    NTSTATUS Status;
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
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
    service_status = RegisterServiceCtrlHandlerExA(service_name, service_handler, NULL);

    if (!service_status)
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

    pNtApphelpCacheControl = (void*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtApphelpCacheControl");
    if (!pNtApphelpCacheControl)
    {
        win_skip("NtApphelpCacheControl not available (as on NT5.1)\n");
        return;
    }

    argc = winetest_get_mainargs(&argv);
    if (argc < 3)
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
