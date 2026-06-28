/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/database.c
 * PURPOSE:     Database control interface
 * COPYRIGHT:   Copyright 2002-2006 Eric Kohl
 *              Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *                             Gregor Brunmar <gregor.brunmar@home.se>
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#include <winbase_undoc.h>
#include <userenv.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>

#define SCM_TRACE_SERVICE_ENTRY(TraceService, ...) \
    do {                                           \
        if (TraceService)                          \
            DPRINT1(__VA_ARGS__);                  \
    } while (0)


/* GLOBALS *******************************************************************/

LIST_ENTRY ImageListHead;
LIST_ENTRY ServiceListHead;

static RTL_RESOURCE DatabaseLock;
static DWORD ResumeCount = 1;
static DWORD NoInteractiveServices = 0;
static DWORD ServiceTag = 0;

/* The critical section synchronizes service control requests */
static CRITICAL_SECTION ControlServiceCriticalSection;
static DWORD PipeTimeout = 30000; /* 30 Seconds */


/* FUNCTIONS *****************************************************************/

static
BOOL
ScmIsGuestStateCollectorService(
    _In_opt_ LPCWSTR lpServiceName)
{
    return lpServiceName != NULL &&
           _wcsicmp(lpServiceName, L"ReactOSGuestStateCollector") == 0;
}


static
BOOL
ScmIsGuestStateTraceService(
    _In_opt_ LPCWSTR lpServiceName)
{
    return ScmIsGuestStateCollectorService(lpServiceName) ||
           (lpServiceName != NULL &&
            _wcsicmp(lpServiceName, L"VMware Physical Disk Helper Service") == 0);
}


static
BOOL
ScmShouldTraceServiceListEntry(
    _In_opt_ LPCWSTR lpServiceName)
{
    if (ScmIsGuestStateTraceService(lpServiceName))
        return TRUE;

    return lpServiceName != NULL &&
           (_wcsicmp(lpServiceName, L"vga") == 0 ||
            _wcsicmp(lpServiceName, L"wuauserv") == 0);
}


static
VOID
ScmTraceAutoStartScan(
    _In_ PCSTR PassName,
    _In_ PSERVICE Service)
{
    DPRINT1("ROSGUESTSTATE autostart-scan pass=%s service='%S' group='%S' start=%lu visited=%u tag=%lu state=%lu\n",
            PassName,
            Service->lpServiceName,
            Service->lpGroup ? Service->lpGroup->lpGroupName : L"(null)",
            Service->dwStartType,
            Service->ServiceVisited,
            Service->dwTag,
            Service->Status.dwCurrentState);
}


static
VOID
ScmTraceAutoStartService(
    _In_ PCSTR PassName,
    _In_ PSERVICE Service)
{
    DPRINT1("ROSGUESTSTATE autostart-load pass=%s service='%S' group='%S' start=%lu visited=%u tag=%lu state=%lu\n",
            PassName,
            Service->lpServiceName,
            Service->lpGroup ? Service->lpGroup->lpGroupName : L"(null)",
            Service->dwStartType,
            Service->ServiceVisited,
            Service->dwTag,
            Service->Status.dwCurrentState);
}


static
VOID
ScmTraceAutoStartServiceResult(
    _In_ PCSTR PassName,
    _In_ PSERVICE Service,
    _In_ DWORD Error)
{
    DPRINT1("ROSGUESTSTATE autostart-result pass=%s service='%S' error=%lu state=%lu win32=%lu visited=%u image=%p\n",
            PassName,
            Service->lpServiceName,
            Error,
            Service->Status.dwCurrentState,
            Service->Status.dwWin32ExitCode,
            Service->ServiceVisited,
            Service->lpImage);
}


static
VOID
ScmTraceGuestStateAutoStartTarget(
    _In_ PCSTR PointName,
    _In_opt_ PSERVICE Service)
{
    DPRINT1("ROSGUESTSTATE autostart-target point=%s service=%p group=%p groupname='%S' start=%lu visited=%u tag=%lu state=%lu image=%p\n",
            PointName,
            Service,
            Service ? Service->lpGroup : NULL,
            (Service && Service->lpGroup) ? Service->lpGroup->lpGroupName : L"(null)",
            Service ? Service->dwStartType : 0,
            Service ? Service->ServiceVisited : 0,
            Service ? Service->dwTag : 0,
            Service ? Service->Status.dwCurrentState : 0,
            Service ? Service->lpImage : NULL);
}


static
BOOL
ScmIsSecurityService(
    _In_ PSERVICE_IMAGE pServiceImage)
{
    return (wcsstr(pServiceImage->pszImagePath, L"\\system32\\lsass.exe") != NULL);
}


static DWORD
ScmCreateNewControlPipe(
    _In_ PSERVICE_IMAGE pServiceImage,
    _In_ BOOL bSecurityServiceProcess)
{
    WCHAR szControlPipeName[MAX_PATH + 1];
    SECURITY_ATTRIBUTES SecurityAttributes;
    HKEY hServiceCurrentKey = INVALID_HANDLE_VALUE;
    DWORD dwServiceCurrent = 1;
    DWORD dwKeyDisposition;
    DWORD dwKeySize;
    DWORD dwError;

    /* Get the service number */
    if (bSecurityServiceProcess == FALSE)
    {
        /* TODO: Create registry entry with correct write access */
        dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                  L"SYSTEM\\CurrentControlSet\\Control\\ServiceCurrent",
                                  0,
                                  NULL,
                                  REG_OPTION_VOLATILE,
                                  KEY_WRITE | KEY_READ,
                                  NULL,
                                  &hServiceCurrentKey,
                                  &dwKeyDisposition);
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegCreateKeyEx() failed with error %lu\n", dwError);
            return dwError;
        }

        if (dwKeyDisposition == REG_OPENED_EXISTING_KEY)
        {
            dwKeySize = sizeof(DWORD);
            dwError = RegQueryValueExW(hServiceCurrentKey,
                                       L"",
                                       0,
                                       NULL,
                                       (BYTE*)&dwServiceCurrent,
                                       &dwKeySize);
            if (dwError != ERROR_SUCCESS)
            {
                RegCloseKey(hServiceCurrentKey);
                DPRINT1("RegQueryValueEx() failed with error %lu\n", dwError);
                return dwError;
            }

            dwServiceCurrent++;
        }

        dwError = RegSetValueExW(hServiceCurrentKey,
                                 L"",
                                 0,
                                 REG_DWORD,
                                 (BYTE*)&dwServiceCurrent,
                                 sizeof(dwServiceCurrent));

        RegCloseKey(hServiceCurrentKey);

        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegSetValueExW() failed (Error %lu)\n", dwError);
            return dwError;
        }
    }
    else
    {
        dwServiceCurrent = 0;
    }

    /* Create '\\.\pipe\net\NtControlPipeXXX' instance */
    StringCchPrintfW(szControlPipeName, ARRAYSIZE(szControlPipeName),
                     L"\\\\.\\pipe\\net\\NtControlPipe%lu", dwServiceCurrent);

    DPRINT("PipeName: %S\n", szControlPipeName);

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = pPipeSD;
    SecurityAttributes.bInheritHandle = FALSE;

    pServiceImage->hControlPipe = CreateNamedPipeW(szControlPipeName,
                                                   PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                                   PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                                   100,
                                                   8000,
                                                   4,
                                                   PipeTimeout,
                                                   &SecurityAttributes);
    DPRINT("CreateNamedPipeW(%S) done\n", szControlPipeName);
    if (pServiceImage->hControlPipe == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Failed to create control pipe\n");
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


static PSERVICE_IMAGE
ScmGetServiceImageByImagePath(LPWSTR lpImagePath)
{
    PLIST_ENTRY ImageEntry;
    PSERVICE_IMAGE CurrentImage;

    DPRINT("ScmGetServiceImageByImagePath(%S) called\n", lpImagePath);

    ImageEntry = ImageListHead.Flink;
    while (ImageEntry != &ImageListHead)
    {
        CurrentImage = CONTAINING_RECORD(ImageEntry,
                                         SERVICE_IMAGE,
                                         ImageListEntry);
        if (_wcsicmp(CurrentImage->pszImagePath, lpImagePath) == 0)
        {
            DPRINT("Found image: '%S'\n", CurrentImage->pszImagePath);
            return CurrentImage;
        }

        ImageEntry = ImageEntry->Flink;
    }

    DPRINT("Couldn't find a matching image\n");

    return NULL;

}


DWORD
ScmGetServiceNameFromTag(IN PTAG_INFO_NAME_FROM_TAG_IN_PARAMS InParams,
                         OUT PTAG_INFO_NAME_FROM_TAG_OUT_PARAMS *OutParams)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;
    PSERVICE_IMAGE CurrentImage;
    PTAG_INFO_NAME_FROM_TAG_OUT_PARAMS OutBuffer = NULL;
    DWORD dwError;

    /* Lock the database */
    ScmLockDatabaseExclusive();

    /* Find the matching service */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);

        /* We must match the tag */
        if (CurrentService->dwServiceTag == InParams->dwTag &&
            CurrentService->lpImage != NULL)
        {
            CurrentImage = CurrentService->lpImage;
            /* And matching the PID */
            if (CurrentImage->dwProcessId == InParams->dwPid)
            {
                break;
            }
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    /* No match! */
    if (ServiceEntry == &ServiceListHead)
    {
        dwError = ERROR_RETRY;
        goto Cleanup;
    }

    /* Allocate the output buffer */
    OutBuffer = MIDL_user_allocate(sizeof(TAG_INFO_NAME_FROM_TAG_OUT_PARAMS));
    if (OutBuffer == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    /* And the buffer for the name */
    OutBuffer->pszName = MIDL_user_allocate(wcslen(CurrentService->lpServiceName) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (OutBuffer->pszName == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    /* Fill in output data */
    wcscpy(OutBuffer->pszName, CurrentService->lpServiceName);
    OutBuffer->TagType = TagTypeService;

    /* And return */
    *OutParams = OutBuffer;
    dwError = ERROR_SUCCESS;

Cleanup:

    /* Unlock database */
    ScmUnlockDatabase();

    /* If failure, free allocated memory */
    if (dwError != ERROR_SUCCESS)
    {
        if (OutBuffer != NULL)
        {
            MIDL_user_free(OutBuffer);
        }
    }

    /* Return error/success */
    return dwError;
}


static
BOOL
ScmIsSameServiceAccount(
    _In_ PCWSTR pszAccountName1,
    _In_ PCWSTR pszAccountName2)
{
    if (pszAccountName1 == NULL &&
        pszAccountName2 == NULL)
        return TRUE;

    if (pszAccountName1 == NULL &&
        pszAccountName2 != NULL &&
        _wcsicmp(pszAccountName2, L"LocalSystem") == 0)
        return TRUE;

    if (pszAccountName1 != NULL &&
        pszAccountName2 == NULL &&
        _wcsicmp(pszAccountName1, L"LocalSystem") == 0)
        return TRUE;

    if (pszAccountName1 != NULL &&
        pszAccountName2 != NULL &&
        _wcsicmp(pszAccountName1, pszAccountName2) == 0)
        return TRUE;

    return FALSE;
}


static
BOOL
ScmIsLocalSystemAccount(
    _In_ PCWSTR pszAccountName)
{
    if (pszAccountName == NULL ||
        _wcsicmp(pszAccountName, L"LocalSystem") == 0)
        return TRUE;

    return FALSE;
}


static
BOOL
ScmEnableBackupRestorePrivileges(
    _In_ HANDLE hToken,
    _In_ BOOL bEnable)
{
    PTOKEN_PRIVILEGES pTokenPrivileges = NULL;
    DWORD dwSize;
    BOOL bRet = FALSE;

    DPRINT("ScmEnableBackupRestorePrivileges(%p %d)\n", hToken, bEnable);

    dwSize = sizeof(TOKEN_PRIVILEGES) + 2 * sizeof(LUID_AND_ATTRIBUTES);
    pTokenPrivileges = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
    if (pTokenPrivileges == NULL)
    {
        DPRINT1("Failed to allocate privilege buffer\n");
        goto done;
    }

    pTokenPrivileges->PrivilegeCount = 2;
    pTokenPrivileges->Privileges[0].Luid.LowPart = SE_BACKUP_PRIVILEGE;
    pTokenPrivileges->Privileges[0].Luid.HighPart = 0;
    pTokenPrivileges->Privileges[0].Attributes = (bEnable ? SE_PRIVILEGE_ENABLED : 0);
    pTokenPrivileges->Privileges[1].Luid.LowPart = SE_RESTORE_PRIVILEGE;
    pTokenPrivileges->Privileges[1].Luid.HighPart = 0;
    pTokenPrivileges->Privileges[1].Attributes = (bEnable ? SE_PRIVILEGE_ENABLED : 0);

    bRet = AdjustTokenPrivileges(hToken, FALSE, pTokenPrivileges, 0, NULL, NULL);
    if (!bRet)
    {
        DPRINT1("AdjustTokenPrivileges() failed with error %lu\n", GetLastError());
    }
    else if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        DPRINT1("AdjustTokenPrivileges() succeeded, but with not all privileges assigned\n");
        bRet = FALSE;
    }

done:
    if (pTokenPrivileges != NULL)
        HeapFree(GetProcessHeap(), 0, pTokenPrivileges);

    return bRet;
}


static
DWORD
ScmLogonService(
    IN PSERVICE pService,
    IN PSERVICE_IMAGE pImage)
{
    PROFILEINFOW ProfileInfo;
    PWSTR pszUserName = NULL;
    PWSTR pszDomainName = NULL;
    PWSTR pszPassword = NULL;
    PWSTR ptr;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("ScmLogonService(%p %p)\n", pService, pImage);
    DPRINT("Service %S\n", pService->lpServiceName);

    if (ScmIsLocalSystemAccount(pImage->pszAccountName) || ScmLiveSetup || ScmSetupInProgress)
        return ERROR_SUCCESS;

    /* Get the user and domain names */
    ptr = wcschr(pImage->pszAccountName, L'\\');
    if (ptr != NULL)
    {
        *ptr = L'\0';
        pszUserName = ptr + 1;
        pszDomainName = pImage->pszAccountName;
    }
    else
    {
        // ERROR_INVALID_SERVICE_ACCOUNT
        pszUserName = pImage->pszAccountName;
        pszDomainName = NULL;
    }

    /* Build the service 'password' */
    pszPassword = HeapAlloc(GetProcessHeap(),
                            HEAP_ZERO_MEMORY,
                            (wcslen(pService->lpServiceName) + 5) * sizeof(WCHAR));
    if (pszPassword == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    wcscpy(pszPassword, L"_SC_");
    wcscat(pszPassword, pService->lpServiceName);

    DPRINT("Domain: %S  User: %S  Password: %S\n", pszDomainName, pszUserName, pszPassword);

    /* Do the service logon */
    if (!LogonUserW(pszUserName,
                    pszDomainName,
                    pszPassword,
                    LOGON32_LOGON_SERVICE,
                    LOGON32_PROVIDER_DEFAULT,
                    &pImage->hToken))
    {
        dwError = GetLastError();
        DPRINT1("LogonUserW() failed (Error %lu)\n", dwError);

        /* Normalize the returned error */
        dwError = ERROR_SERVICE_LOGON_FAILED;
        goto done;
    }

    /* Load the user profile; the per-user environment variables are thus correctly initialized */
    ZeroMemory(&ProfileInfo, sizeof(ProfileInfo));
    ProfileInfo.dwSize = sizeof(ProfileInfo);
    ProfileInfo.dwFlags = PI_NOUI;
    ProfileInfo.lpUserName = pszUserName;
    // ProfileInfo.lpProfilePath = NULL;
    // ProfileInfo.lpDefaultPath = NULL;
    // ProfileInfo.lpServerName = NULL;
    // ProfileInfo.lpPolicyPath = NULL;
    // ProfileInfo.hProfile = NULL;

    ScmEnableBackupRestorePrivileges(pImage->hToken, TRUE);
    if (!LoadUserProfileW(pImage->hToken, &ProfileInfo))
        dwError = GetLastError();
    ScmEnableBackupRestorePrivileges(pImage->hToken, FALSE);

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("LoadUserProfileW() failed (Error %lu)\n", dwError);
        goto done;
    }

    pImage->hProfile = ProfileInfo.hProfile;

done:
    if (pszPassword != NULL)
        HeapFree(GetProcessHeap(), 0, pszPassword);

    if (ptr != NULL)
        *ptr = L'\\';

    return dwError;
}


static DWORD
ScmCreateOrReferenceServiceImage(PSERVICE pService)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    UNICODE_STRING ImagePath;
    UNICODE_STRING ObjectName;
    PSERVICE_IMAGE pServiceImage = NULL;
    NTSTATUS Status;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwRecordSize;
    LPWSTR pString;
    BOOL bSecurityService;

    DPRINT("ScmCreateOrReferenceServiceImage(%p)\n", pService);

    RtlInitUnicodeString(&ImagePath, NULL);
    RtlInitUnicodeString(&ObjectName, NULL);

    /* Get service data */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"ImagePath";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].EntryContext = &ImagePath;
    QueryTable[1].Name = L"ObjectName";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].EntryContext = &ObjectName;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    pService->lpServiceName,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        if (ScmIsGuestStateTraceService(pService->lpServiceName))
        {
            DPRINT1("ROSGUESTSTATE image-query-failed status=%lx\n", Status);
        }

        DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    if (ScmIsGuestStateTraceService(pService->lpServiceName))
    {
        DPRINT1("ROSGUESTSTATE image-query image='%S' object='%S'\n",
                ImagePath.Buffer ? ImagePath.Buffer : L"(null)",
                ObjectName.Buffer ? ObjectName.Buffer : L"(null)");
    }

    DPRINT("ImagePath: '%wZ'\n", &ImagePath);
    DPRINT("ObjectName: '%wZ'\n", &ObjectName);

    pServiceImage = ScmGetServiceImageByImagePath(ImagePath.Buffer);
    if (pServiceImage == NULL)
    {
        dwRecordSize = sizeof(SERVICE_IMAGE) +
                       ImagePath.Length + sizeof(WCHAR) +
                       ((ObjectName.Length != 0) ? (ObjectName.Length + sizeof(WCHAR)) : 0);

        /* Create a new service image */
        pServiceImage = HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  dwRecordSize);
        if (pServiceImage == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        pServiceImage->dwImageRunCount = 1;
        pServiceImage->hControlPipe = INVALID_HANDLE_VALUE;
        pServiceImage->hProcess = INVALID_HANDLE_VALUE;

        pString = (PWSTR)((INT_PTR)pServiceImage + sizeof(SERVICE_IMAGE));

        /* Set the image path */
        pServiceImage->pszImagePath = pString;
        wcscpy(pServiceImage->pszImagePath,
               ImagePath.Buffer);

        /* Set the account name */
        if (ObjectName.Length > 0)
        {
            pString = pString + wcslen(pString) + 1;

            pServiceImage->pszAccountName = pString;
            wcscpy(pServiceImage->pszAccountName,
                   ObjectName.Buffer);
        }

        /* Service logon */
        dwError = ScmLogonService(pService, pServiceImage);
        if (dwError != ERROR_SUCCESS)
        {
            if (ScmIsGuestStateTraceService(pService->lpServiceName))
            {
                DPRINT1("ROSGUESTSTATE logon-failed error=%lu\n", dwError);
            }

            DPRINT1("ScmLogonService() failed (Error %lu)\n", dwError);

            /* Release the service image */
            HeapFree(GetProcessHeap(), 0, pServiceImage);

            goto done;
        }

        bSecurityService = ScmIsSecurityService(pServiceImage);

        /* Create the control pipe */
        dwError = ScmCreateNewControlPipe(pServiceImage,
                                          bSecurityService);
        if (dwError != ERROR_SUCCESS)
        {
            if (ScmIsGuestStateTraceService(pService->lpServiceName))
            {
                DPRINT1("ROSGUESTSTATE control-pipe-failed error=%lu\n", dwError);
            }

            DPRINT1("ScmCreateNewControlPipe() failed (Error %lu)\n", dwError);

            /* Unload the user profile */
            if (pServiceImage->hProfile != NULL)
            {
                ScmEnableBackupRestorePrivileges(pServiceImage->hToken, TRUE);
                UnloadUserProfile(pServiceImage->hToken, pServiceImage->hProfile);
                ScmEnableBackupRestorePrivileges(pServiceImage->hToken, FALSE);
            }

            /* Close the logon token */
            if (pServiceImage->hToken != NULL)
                CloseHandle(pServiceImage->hToken);

            /* Release the service image */
            HeapFree(GetProcessHeap(), 0, pServiceImage);

            goto done;
        }

        if (bSecurityService)
        {
            SetSecurityServicesEvent();
        }

        /* FIXME: Add more initialization code here */


        /* Append service record */
        InsertTailList(&ImageListHead,
                       &pServiceImage->ImageListEntry);
    }
    else
    {
//        if ((lpService->Status.dwServiceType & SERVICE_WIN32_SHARE_PROCESS) == 0)

        /* Fail if services in an image use different accounts */
        if (!ScmIsSameServiceAccount(pServiceImage->pszAccountName, ObjectName.Buffer))
        {
            dwError = ERROR_DIFFERENT_SERVICE_ACCOUNT;
            goto done;
        }

        /* Increment the run counter */
        pServiceImage->dwImageRunCount++;
    }

    DPRINT("pServiceImage->pszImagePath: %S\n", pServiceImage->pszImagePath);
    DPRINT("pServiceImage->pszAccountName: %S\n", pServiceImage->pszAccountName);
    DPRINT("pServiceImage->dwImageRunCount: %lu\n", pServiceImage->dwImageRunCount);

    /* Link the service image to the service */
    pService->lpImage = pServiceImage;

done:
    RtlFreeUnicodeString(&ObjectName);
    RtlFreeUnicodeString(&ImagePath);

    DPRINT("ScmCreateOrReferenceServiceImage() done (Error: %lu)\n", dwError);

    return dwError;
}


VOID
ScmRemoveServiceImage(PSERVICE_IMAGE pServiceImage)
{
    DPRINT1("ScmRemoveServiceImage() called\n");

    /* FIXME: Terminate the process */

    /* Remove the service image from the list */
    RemoveEntryList(&pServiceImage->ImageListEntry);

    /* Close the process handle */
    if (pServiceImage->hProcess != INVALID_HANDLE_VALUE)
        CloseHandle(pServiceImage->hProcess);

    /* Close the control pipe */
    if (pServiceImage->hControlPipe != INVALID_HANDLE_VALUE)
        CloseHandle(pServiceImage->hControlPipe);

    /* Unload the user profile */
    if (pServiceImage->hProfile != NULL)
    {
        ScmEnableBackupRestorePrivileges(pServiceImage->hToken, TRUE);
        UnloadUserProfile(pServiceImage->hToken, pServiceImage->hProfile);
        ScmEnableBackupRestorePrivileges(pServiceImage->hToken, FALSE);
    }

    /* Close the logon token */
    if (pServiceImage->hToken != NULL)
        CloseHandle(pServiceImage->hToken);

    /* Release the service image */
    HeapFree(GetProcessHeap(), 0, pServiceImage);
}


PSERVICE
ScmGetServiceEntryByName(LPCWSTR lpServiceName)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByName() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (_wcsicmp(CurrentService->lpServiceName, lpServiceName) == 0)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpServiceName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


PSERVICE
ScmGetServiceEntryByDisplayName(LPCWSTR lpDisplayName)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByDisplayName() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (_wcsicmp(CurrentService->lpDisplayName, lpDisplayName) == 0)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpDisplayName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


PSERVICE
ScmGetServiceEntryByResumeCount(DWORD dwResumeCount)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetServiceEntryByResumeCount() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry,
                                           SERVICE,
                                           ServiceListEntry);
        if (CurrentService->dwResumeCount > dwResumeCount)
        {
            DPRINT("Found service: '%S'\n", CurrentService->lpDisplayName);
            return CurrentService;
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("Couldn't find a matching service\n");

    return NULL;
}


DWORD
ScmGenerateServiceTag(PSERVICE lpServiceRecord)
{
    /* Check for an overflow */
    if (ServiceTag == -1)
    {
        return ERROR_INVALID_DATA;
    }

    /* This is only valid for Win32 services */
    if (!(lpServiceRecord->Status.dwServiceType & SERVICE_WIN32))
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Increment the tag counter and set it */
    ServiceTag = ServiceTag % 0xFFFFFFFF + 1;
    lpServiceRecord->dwServiceTag = ServiceTag;

    return ERROR_SUCCESS;
}


DWORD
ScmCreateNewServiceRecord(LPCWSTR lpServiceName,
                          PSERVICE *lpServiceRecord,
                          DWORD dwServiceType,
                          DWORD dwStartType)
{
    PSERVICE lpService = NULL;

    DPRINT("Service: '%S'\n", lpServiceName);

    /* Allocate service entry */
    lpService = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          FIELD_OFFSET(SERVICE, szServiceName[wcslen(lpServiceName) + 1]));
    if (lpService == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    *lpServiceRecord = lpService;

    /* Copy service name */
    wcscpy(lpService->szServiceName, lpServiceName);
    lpService->lpServiceName = lpService->szServiceName;
    lpService->lpDisplayName = lpService->lpServiceName;

    /* Set the start type */
    lpService->dwStartType = dwStartType;

    /* Set the resume count */
    lpService->dwResumeCount = ResumeCount++;

    /* Append service record */
    InsertTailList(&ServiceListHead,
                   &lpService->ServiceListEntry);

    /* Initialize the service status */
    lpService->Status.dwServiceType = dwServiceType;
    lpService->Status.dwCurrentState = SERVICE_STOPPED;
    lpService->Status.dwControlsAccepted = 0;
    lpService->Status.dwWin32ExitCode =
        (dwStartType == SERVICE_DISABLED) ? ERROR_SERVICE_DISABLED : ERROR_SERVICE_NEVER_STARTED;
    lpService->Status.dwServiceSpecificExitCode = 0;
    lpService->Status.dwCheckPoint = 0;
    lpService->Status.dwWaitHint =
        (dwServiceType & SERVICE_DRIVER) ? 0 : 2000; /* 2 seconds */

    return ERROR_SUCCESS;
}


VOID
ScmDeleteServiceRecord(PSERVICE lpService)
{
    DPRINT("Deleting Service %S\n", lpService->lpServiceName);

    /* Delete the display name */
    if (lpService->lpDisplayName != NULL &&
        lpService->lpDisplayName != lpService->lpServiceName)
        HeapFree(GetProcessHeap(), 0, lpService->lpDisplayName);

    /* Dereference the service image */
    if (lpService->lpImage)
    {
        lpService->lpImage->dwImageRunCount--;

        if (lpService->lpImage->dwImageRunCount == 0)
        {
            ScmRemoveServiceImage(lpService->lpImage);
            lpService->lpImage = NULL;
        }
    }

    /* Decrement the group reference counter */
    ScmSetServiceGroup(lpService, NULL);

    /* Release the SecurityDescriptor */
    if (lpService->pSecurityDescriptor != NULL)
        HeapFree(GetProcessHeap(), 0, lpService->pSecurityDescriptor);

    /* Remove the Service from the List */
    RemoveEntryList(&lpService->ServiceListEntry);

    DPRINT("Deleted Service %S\n", lpService->lpServiceName);

    /* Delete the service record */
    HeapFree(GetProcessHeap(), 0, lpService);

    DPRINT("Done\n");
}

DWORD
Int_EnumDependentServicesW(HKEY hServicesKey,
                           PSERVICE lpService,
                           DWORD dwServiceState,
                           PSERVICE *lpServices,
                           LPDWORD pcbBytesNeeded,
                           LPDWORD lpServicesReturned);

DWORD ScmDeleteService(PSERVICE lpService)
{
    DWORD dwError;
    DWORD pcbBytesNeeded = 0;
    DWORD dwServicesReturned = 0;
    HKEY hServicesKey;

    ASSERT(lpService->RefCount == 0);

    /* Open the Services Reg key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_SET_VALUE | KEY_READ,
                            &hServicesKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Failed to open services key\n");
        return dwError;
    }

    /* Call the function with NULL, just to get bytes we need */
    Int_EnumDependentServicesW(hServicesKey,
                               lpService,
                               SERVICE_ACTIVE,
                               NULL,
                               &pcbBytesNeeded,
                               &dwServicesReturned);

    /* If pcbBytesNeeded returned a value then there are services running that are dependent on this service */
    if (pcbBytesNeeded)
    {
        DPRINT1("Deletion failed due to running dependencies\n");
        RegCloseKey(hServicesKey);
        return ERROR_DEPENDENT_SERVICES_RUNNING;
    }

    /* There are no references and no running dependencies,
       it is now safe to delete the service */

    /* Delete the Service Key */
    dwError = ScmDeleteRegKey(hServicesKey, lpService->lpServiceName);

    RegCloseKey(hServicesKey);

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Failed to delete the Service Registry key\n");
        return dwError;
    }

    /* Delete the Service */
    ScmDeleteServiceRecord(lpService);

    return ERROR_SUCCESS;
}

/*
 * This function allows the caller to be sure that the service won't be freed unexpectedly.
 * In order to be sure that lpService will be valid until the reference is added
 * the caller needs to hold the database lock.
 * A running service will keep a reference for the whole time it is not SERVICE_STOPPED.
 * A service handle will also keep a reference to a service. Keeping a reference is
 * really needed so that ScmControlService can be called without keeping the database locked.
 * This means that eventually the correct order of operations to send a control message to
 * a service looks like: lock, reference, unlock, send control, lock, dereference, unlock.
 */
DWORD
ScmReferenceService(PSERVICE lpService)
{
    return InterlockedIncrement(&lpService->RefCount);
}

/* This function must be called with the database lock held exclusively as
   it can end up deleting the service */
DWORD
ScmDereferenceService(PSERVICE lpService)
{
    DWORD ref;

    ASSERT(lpService->RefCount > 0);

    ref = InterlockedDecrement(&lpService->RefCount);

    if (ref == 0 && lpService->bDeleted &&
        lpService->Status.dwCurrentState == SERVICE_STOPPED)
    {
        ScmDeleteService(lpService);
    }
    return ref;
}

static DWORD
CreateServiceListEntry(LPCWSTR lpServiceName,
                       HKEY hServiceKey)
{
    PSERVICE lpService = NULL;
    LPWSTR lpDisplayName = NULL;
    LPWSTR lpGroup = NULL;
    BOOL bTraceService;
    DWORD dwSize;
    DWORD dwError;
    DWORD dwServiceType = 0;
    DWORD dwStartType = 0;
    DWORD dwErrorControl = 0;
    DWORD dwTagId = 0;

    DPRINT("Service: '%S'\n", lpServiceName);
    bTraceService = ScmShouldTraceServiceListEntry(lpServiceName);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-enter service='%S' key=%p\n",
                            lpServiceName,
                            hServiceKey);

    if (*lpServiceName == L'{')
        return ERROR_SUCCESS;

    dwSize = sizeof(DWORD);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query service='%S' value=Type\n",
                            lpServiceName);

    dwError = RegQueryValueExW(hServiceKey,
                               L"Type",
                               NULL,
                               NULL,
                               (LPBYTE)&dwServiceType,
                               &dwSize);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query-result service='%S' value=Type error=%lu size=%lu data=%lx\n",
                            lpServiceName,
                            dwError,
                            dwSize,
                            dwServiceType);

    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    if (((dwServiceType & ~SERVICE_INTERACTIVE_PROCESS) != SERVICE_WIN32_OWN_PROCESS) &&
        ((dwServiceType & ~SERVICE_INTERACTIVE_PROCESS) != SERVICE_WIN32_SHARE_PROCESS) &&
        (dwServiceType != SERVICE_KERNEL_DRIVER) &&
        (dwServiceType != SERVICE_FILE_SYSTEM_DRIVER))
        return ERROR_SUCCESS;

    DPRINT("Service type: %lx\n", dwServiceType);

    dwSize = sizeof(DWORD);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query service='%S' value=Start\n",
                            lpServiceName);

    dwError = RegQueryValueExW(hServiceKey,
                               L"Start",
                               NULL,
                               NULL,
                               (LPBYTE)&dwStartType,
                               &dwSize);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query-result service='%S' value=Start error=%lu size=%lu data=%lx\n",
                            lpServiceName,
                            dwError,
                            dwSize,
                            dwStartType);

    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    DPRINT("Start type: %lx\n", dwStartType);

    dwSize = sizeof(DWORD);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query service='%S' value=ErrorControl\n",
                            lpServiceName);

    dwError = RegQueryValueExW(hServiceKey,
                               L"ErrorControl",
                               NULL,
                               NULL,
                               (LPBYTE)&dwErrorControl,
                               &dwSize);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query-result service='%S' value=ErrorControl error=%lu size=%lu data=%lx\n",
                            lpServiceName,
                            dwError,
                            dwSize,
                            dwErrorControl);

    if (dwError != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    DPRINT("Error control: %lx\n", dwErrorControl);

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query service='%S' value=Tag\n",
                            lpServiceName);

    dwError = RegQueryValueExW(hServiceKey,
                               L"Tag",
                               NULL,
                               NULL,
                               (LPBYTE)&dwTagId,
                               &dwSize);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-query-result service='%S' value=Tag error=%lu size=%lu data=%lx\n",
                            lpServiceName,
                            dwError,
                            dwSize,
                            dwTagId);

    if (dwError != ERROR_SUCCESS)
        dwTagId = 0;

    DPRINT("Tag: %lx\n", dwTagId);

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-read-string service='%S' value=Group\n",
                            lpServiceName);

    dwError = ScmReadString(hServiceKey,
                            L"Group",
                            &lpGroup);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-read-string-result service='%S' value=Group error=%lu data='%S'\n",
                            lpServiceName,
                            dwError,
                            (dwError == ERROR_SUCCESS && lpGroup != NULL) ? lpGroup : L"(null)");

    if (dwError != ERROR_SUCCESS)
        lpGroup = NULL;

    DPRINT("Group: %S\n", lpGroup);

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-read-string service='%S' value=DisplayName\n",
                            lpServiceName);

    dwError = ScmReadString(hServiceKey,
                            L"DisplayName",
                            &lpDisplayName);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-read-string-result service='%S' value=DisplayName error=%lu data='%S'\n",
                            lpServiceName,
                            dwError,
                            (dwError == ERROR_SUCCESS && lpDisplayName != NULL) ? lpDisplayName : L"(null)");

    if (dwError != ERROR_SUCCESS)
        lpDisplayName = NULL;

    DPRINT("Display name: %S\n", lpDisplayName);

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry values type=%lx start=%lx error=%lx tag=%lx group='%S' display='%S'\n",
                            dwServiceType,
                            dwStartType,
                            dwErrorControl,
                            dwTagId,
                            lpGroup ? lpGroup : L"(null)",
                            lpDisplayName ? lpDisplayName : L"(null)");

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-new-record service='%S'\n",
                            lpServiceName);

    dwError = ScmCreateNewServiceRecord(lpServiceName,
                                        &lpService,
                                        dwServiceType,
                                        dwStartType);
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-new-record-result service='%S' error=%lu record=%p\n",
                            lpServiceName,
                            dwError,
                            lpService);

    if (dwError != ERROR_SUCCESS)
        goto done;

    lpService->dwErrorControl = dwErrorControl;
    lpService->dwTag = dwTagId;

    if (lpGroup != NULL)
    {
        SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                "ROSGUESTSTATE create-entry-set-group service='%S' group='%S'\n",
                                lpServiceName,
                                lpGroup);

        dwError = ScmSetServiceGroup(lpService, lpGroup);
        SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                "ROSGUESTSTATE create-entry-set-group-result service='%S' error=%lu group-record=%p\n",
                                lpServiceName,
                                dwError,
                                lpService->lpGroup);

        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    if (lpDisplayName != NULL)
    {
        lpService->lpDisplayName = lpDisplayName;
        lpDisplayName = NULL;
    }

    DPRINT("ServiceName: '%S'\n", lpService->lpServiceName);
    if (lpService->lpGroup != NULL)
    {
        DPRINT("Group: '%S'\n", lpService->lpGroup->lpGroupName);
    }
    DPRINT("Start %lx  Type %lx  Tag %lx  ErrorControl %lx\n",
           lpService->dwStartType,
           lpService->Status.dwServiceType,
           lpService->dwTag,
           lpService->dwErrorControl);

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-delete-flag service='%S'\n",
                            lpServiceName);

    if (ScmIsDeleteFlagSet(hServiceKey))
        lpService->bDeleted = TRUE;
    else
        ScmGenerateServiceTag(lpService);

    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-delete-flag-done service='%S' deleted=%u tag=%lu\n",
                            lpServiceName,
                            lpService->bDeleted,
                            lpService->dwTag);

    if (lpService->Status.dwServiceType & SERVICE_WIN32)
    {
        SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                "ROSGUESTSTATE create-entry-read-sd service='%S'\n",
                                lpServiceName);

        dwError = ScmReadSecurityDescriptor(hServiceKey,
                                            &lpService->pSecurityDescriptor);
        SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                "ROSGUESTSTATE create-entry-read-sd-result service='%S' error=%lu sd=%p\n",
                                lpServiceName,
                                dwError,
                                lpService->pSecurityDescriptor);

        if (dwError != ERROR_SUCCESS)
            goto done;

        /* Assing the default security descriptor if the security descriptor cannot be read */
        if (lpService->pSecurityDescriptor == NULL)
        {
            SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                    "ROSGUESTSTATE create-entry-default-sd service='%S'\n",
                                    lpServiceName);

            DPRINT("No security descriptor found! Assign default security descriptor\n");
            dwError = ScmCreateDefaultServiceSD(&lpService->pSecurityDescriptor);
            SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                    "ROSGUESTSTATE create-entry-default-sd-result service='%S' error=%lu sd=%p\n",
                                    lpServiceName,
                                    dwError,
                                    lpService->pSecurityDescriptor);

            if (dwError != ERROR_SUCCESS)
                goto done;

            SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                    "ROSGUESTSTATE create-entry-write-sd service='%S'\n",
                                    lpServiceName);

            dwError = ScmWriteSecurityDescriptor(hServiceKey,
                                                 lpService->pSecurityDescriptor);
            SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                    "ROSGUESTSTATE create-entry-write-sd-result service='%S' error=%lu\n",
                                    lpServiceName,
                                    dwError);

            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }

done:
    SCM_TRACE_SERVICE_ENTRY(bTraceService,
                            "ROSGUESTSTATE create-entry-done error=%lu service=%p deleted=%u sd=%p\n",
                            dwError,
                            lpService,
                            (lpService != NULL) ? lpService->bDeleted : 0,
                            (lpService != NULL) ? lpService->pSecurityDescriptor : NULL);

    if (lpGroup != NULL)
        HeapFree(GetProcessHeap(), 0, lpGroup);

    if (lpDisplayName != NULL)
        HeapFree(GetProcessHeap(), 0, lpDisplayName);

    if (lpService != NULL)
    {
        ASSERT(lpService->lpImage == NULL);
    }

    return dwError;
}


VOID
ScmDeleteMarkedServices(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;
    HKEY hServicesKey;
    DWORD dwError;

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        ServiceEntry = ServiceEntry->Flink;

        if (CurrentService->bDeleted != FALSE)
        {
            dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                    L"System\\CurrentControlSet\\Services",
                                    0,
                                    DELETE,
                                    &hServicesKey);
            if (dwError == ERROR_SUCCESS)
            {
                dwError = ScmDeleteRegKey(hServicesKey, CurrentService->lpServiceName);
                RegCloseKey(hServicesKey);
                if (dwError == ERROR_SUCCESS)
                {
                    RemoveEntryList(&CurrentService->ServiceListEntry);
                    HeapFree(GetProcessHeap(), 0, CurrentService);
                }
            }

            if (dwError != ERROR_SUCCESS)
                DPRINT1("Delete service failed: %S\n", CurrentService->lpServiceName);
        }
    }
}


static
VOID
ScmGetNoInteractiveServicesValue(VOID)
{
    HKEY hKey;
    DWORD dwKeySize;
    LONG lError;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\Windows",
                           0,
                           KEY_READ,
                           &hKey);
    if (lError == ERROR_SUCCESS)
    {
        dwKeySize = sizeof(NoInteractiveServices);
        lError = RegQueryValueExW(hKey,
                                  L"NoInteractiveServices",
                                  0,
                                  NULL,
                                  (LPBYTE)&NoInteractiveServices,
                                  &dwKeySize);
        RegCloseKey(hKey);
    }
}


DWORD
ScmCreateServiceDatabase(VOID)
{
    WCHAR szSubKey[MAX_PATH];
    HKEY hServicesKey;
    HKEY hServiceKey;
    BOOL bTraceService = FALSE;
    DWORD dwSubKey;
    DWORD dwSubKeyLength;
    FILETIME ftLastChanged;
    DWORD dwError;

    DPRINT("ScmCreateServiceDatabase() called\n");
    DPRINT1("ROSGUESTSTATE database-enter\n");

    /* Retrieve the NoInteractiveServies value */
    ScmGetNoInteractiveServicesValue();
    DPRINT1("ROSGUESTSTATE database-nointeractive value=%lu\n",
            NoInteractiveServices);

    /* Create the service group list */
    dwError = ScmCreateGroupList();
    DPRINT1("ROSGUESTSTATE database-group-list-result error=%lu\n",
            dwError);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Initialize image and service lists */
    InitializeListHead(&ImageListHead);
    InitializeListHead(&ServiceListHead);

    /* Initialize the database lock */
    RtlInitializeResource(&DatabaseLock);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hServicesKey);
    DPRINT1("ROSGUESTSTATE database-open-services-result error=%lu key=%p\n",
            dwError,
            (dwError == ERROR_SUCCESS) ? hServicesKey : NULL);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwSubKey = 0;
    for (;;)
    {
        dwSubKeyLength = MAX_PATH;
        dwError = RegEnumKeyExW(hServicesKey,
                                dwSubKey,
                                szSubKey,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                &ftLastChanged);
        if (dwError == ERROR_SUCCESS)
            bTraceService = ScmShouldTraceServiceListEntry(szSubKey);
        else
            bTraceService = FALSE;

        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("ROSGUESTSTATE database-enum index=%lu error=%lu length=%lu name=''\n",
                    dwSubKey,
                    dwError,
                    dwSubKeyLength);
        }

        SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                "ROSGUESTSTATE database-enum index=%lu error=%lu length=%lu name='%S'\n",
                                dwSubKey,
                                dwError,
                                dwSubKeyLength,
                                szSubKey);
        if (dwError == ERROR_SUCCESS &&
            szSubKey[0] != L'{')
        {
            DPRINT("SubKeyName: '%S'\n", szSubKey);
            SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                    "ROSGUESTSTATE database-open-service index=%lu name='%S'\n",
                                    dwSubKey,
                                    szSubKey);

            dwError = RegOpenKeyExW(hServicesKey,
                                    szSubKey,
                                    0,
                                    KEY_READ,
                                    &hServiceKey);
            SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                    "ROSGUESTSTATE database-open-service-result index=%lu name='%S' error=%lu key=%p\n",
                                    dwSubKey,
                                    szSubKey,
                                    dwError,
                                    (dwError == ERROR_SUCCESS) ? hServiceKey : NULL);
            if (dwError == ERROR_SUCCESS)
            {
                dwError = CreateServiceListEntry(szSubKey,
                                                 hServiceKey);
                SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                        "ROSGUESTSTATE database-create-service-result index=%lu name='%S' error=%lu\n",
                                        dwSubKey,
                                        szSubKey,
                                        dwError);

                RegCloseKey(hServiceKey);
                SCM_TRACE_SERVICE_ENTRY(bTraceService,
                                        "ROSGUESTSTATE database-close-service index=%lu name='%S'\n",
                                        dwSubKey,
                                        szSubKey);
            }
        }

        if (dwError != ERROR_SUCCESS)
            break;

        dwSubKey++;
    }

    DPRINT1("ROSGUESTSTATE database-enum-done index=%lu error=%lu\n",
            dwSubKey,
            dwError);

    RegCloseKey(hServicesKey);
    DPRINT1("ROSGUESTSTATE database-close-services\n");

    /* Wait for the LSA server */
    DPRINT1("ROSGUESTSTATE database-wait-lsa-enter\n");
    ScmWaitForLsa();
    DPRINT1("ROSGUESTSTATE database-wait-lsa-done\n");

    /* Delete services that are marked for delete */
    DPRINT1("ROSGUESTSTATE database-delete-marked-enter\n");
    ScmDeleteMarkedServices();
    DPRINT1("ROSGUESTSTATE database-delete-marked-done\n");

    DPRINT("ScmCreateServiceDatabase() done\n");
    DPRINT1("ROSGUESTSTATE database-done\n");

    return ERROR_SUCCESS;
}


VOID
ScmShutdownServiceDatabase(VOID)
{
    DPRINT("ScmShutdownServiceDatabase() called\n");

    ScmDeleteMarkedServices();
    RtlDeleteResource(&DatabaseLock);

    DPRINT("ScmShutdownServiceDatabase() done\n");
}


static NTSTATUS
ScmCheckDriver(PSERVICE Service)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirName;
    HANDLE DirHandle;
    NTSTATUS Status;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG BufferLength;
    ULONG DataLength;
    ULONG Index;

    DPRINT("ScmCheckDriver(%S) called\n", Service->lpServiceName);

    if (Service->Status.dwServiceType == SERVICE_KERNEL_DRIVER)
    {
        RtlInitUnicodeString(&DirName, L"\\Driver");
    }
    else // if (Service->Status.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER)
    {
        ASSERT(Service->Status.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER);
        RtlInitUnicodeString(&DirName, L"\\FileSystem");
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenDirectoryObject(&DirHandle,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    BufferLength = sizeof(OBJECT_DIRECTORY_INFORMATION) +
                       2 * MAX_PATH * sizeof(WCHAR);
    DirInfo = HeapAlloc(GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        BufferLength);

    Index = 0;
    while (TRUE)
    {
        Status = NtQueryDirectoryObject(DirHandle,
                                        DirInfo,
                                        BufferLength,
                                        TRUE,
                                        FALSE,
                                        &Index,
                                        &DataLength);
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            /* FIXME: Add current service to 'failed service' list */
            DPRINT("Service '%S' failed\n", Service->lpServiceName);
            break;
        }

        if (!NT_SUCCESS(Status))
            break;

        DPRINT("Comparing: '%S'  '%wZ'\n", Service->lpServiceName, &DirInfo->Name);

        if (_wcsicmp(Service->lpServiceName, DirInfo->Name.Buffer) == 0)
        {
            DPRINT("Found: '%S'  '%wZ'\n",
                   Service->lpServiceName, &DirInfo->Name);

            /* Mark service as 'running' */
            Service->Status.dwCurrentState = SERVICE_RUNNING;
            Service->Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
            Service->Status.dwWin32ExitCode = ERROR_SUCCESS;
            Service->Status.dwServiceSpecificExitCode = 0;
            Service->Status.dwCheckPoint = 0;
            Service->Status.dwWaitHint = 0;

            /* Mark the service group as 'running' */
            if (Service->lpGroup != NULL)
            {
                Service->lpGroup->ServicesRunning = TRUE;
            }

            break;
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             DirInfo);
    NtClose(DirHandle);

    return STATUS_SUCCESS;
}


VOID
ScmGetBootAndSystemDriverState(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmGetBootAndSystemDriverState() called\n");

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if (CurrentService->dwStartType == SERVICE_BOOT_START ||
            CurrentService->dwStartType == SERVICE_SYSTEM_START)
        {
            /* Check driver */
            DPRINT("  Checking service: %S\n", CurrentService->lpServiceName);

            ScmCheckDriver(CurrentService);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    DPRINT("ScmGetBootAndSystemDriverState() done\n");
}


/*
 * ScmSendControlPacket must never be called with the database lock being held.
 * The service passed must always be referenced instead.
 */
DWORD
ScmSendControlPacket(
    _In_ HANDLE hControlPipe,
    _In_ PCWSTR pServiceName,
    _In_ DWORD dwControl,
    _In_ DWORD dwControlPacketSize,
    _In_ PVOID pControlPacket)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL bResult;
    SCM_REPLY_PACKET ReplyPacket;
    DWORD dwReadCount = 0;
    OVERLAPPED Overlapped = {0};

    DPRINT("ScmSendControlPacket(%p, %lu, %p) called\n",
           hControlPipe, dwControlPacketSize, pControlPacket);

    /* Acquire the service control critical section, to synchronize requests */
    EnterCriticalSection(&ControlServiceCriticalSection);

    bResult = TransactNamedPipe(hControlPipe,
                                pControlPacket,
                                dwControlPacketSize,
                                &ReplyPacket,
                                sizeof(ReplyPacket),
                                &dwReadCount,
                                &Overlapped);
    if (!bResult)
    {
        /* Fail for any error other than pending IO */
        dwError = GetLastError();
        if (dwError != ERROR_IO_PENDING)
        {
            DPRINT1("TransactNamedPipe(%S, %d) failed (Error %lu)\n", pServiceName, dwControl, dwError);
            goto Done;
        }

        DPRINT("TransactNamedPipe(%S, %d) returned ERROR_IO_PENDING\n", pServiceName, dwControl);

        dwError = WaitForSingleObject(hControlPipe, PipeTimeout);
        DPRINT("WaitForSingleObject(%S, %d) returned %lu\n", pServiceName, dwControl, dwError);

        if (dwError == WAIT_TIMEOUT)
        {
            DPRINT1("WaitForSingleObject(%S, %d) timed out\n", pServiceName, dwControl);
            bResult = CancelIo(hControlPipe);
            if (!bResult)
                DPRINT1("CancelIo(%S, %d) failed (Error %lu)\n", pServiceName, dwControl, GetLastError());

            dwError = ERROR_SERVICE_REQUEST_TIMEOUT;
        }
        else if (dwError == WAIT_OBJECT_0)
        {
            bResult = GetOverlappedResult(hControlPipe,
                                          &Overlapped,
                                          &dwReadCount,
                                          TRUE);
            if (!bResult)
            {
                dwError = GetLastError();
                DPRINT1("GetOverlappedResult(%S, %d) failed (Error %lu)\n", pServiceName, dwControl, dwError);
            }
        }
    }

Done:
    /* Release the service control critical section */
    LeaveCriticalSection(&ControlServiceCriticalSection);

    if (dwReadCount == sizeof(ReplyPacket))
        dwError = ReplyPacket.dwError;

    return dwError;
}


DWORD
ScmControlServiceEx(
    _In_ HANDLE hControlPipe,
    _In_ PCWSTR pServiceName,
    _In_ DWORD dwControl,
    _In_ SERVICE_STATUS_HANDLE hServiceStatus,
    _In_opt_ DWORD dwServiceTag,
    _In_opt_ DWORD argc,
    _In_reads_opt_(argc) const PCWSTR* argv)
{
    DWORD dwError = ERROR_SUCCESS;
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD PacketSize;
    DWORD i;
    PWSTR Ptr;

    DPRINT("ScmControlServiceEx(%S, %d) called\n", pServiceName, dwControl);

    /* Calculate the total size of the control packet:
     * initial structure, the start command line, and the argument vector */
    PacketSize = sizeof(SCM_CONTROL_PACKET);
    PacketSize += (DWORD)((wcslen(pServiceName) + 1) * sizeof(WCHAR));

    /*
     * Calculate the required packet size for the start argument vector 'argv',
     * composed of the pointer offsets list, followed by UNICODE strings.
     * The strings are stored successively after the offsets vector, with
     * the offsets being relative to the beginning of the vector, as in the
     * following layout (with N == argc):
     *     [argOff(0)]...[argOff(N-1)][str(0)]...[str(N-1)] .
     */
    if (argc > 0 && argv != NULL)
    {
        PacketSize = ALIGN_UP(PacketSize, PWSTR);
        PacketSize += (argc * sizeof(PWSTR));

        DPRINT("Argc: %lu\n", argc);
        for (i = 0; i < argc; i++)
        {
            DPRINT("Argv[%lu]: %S\n", i, argv[i]);
            PacketSize += (DWORD)((wcslen(argv[i]) + 1) * sizeof(WCHAR));
        }
    }

    /* Allocate the control packet */
    ControlPacket = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PacketSize);
    if (!ControlPacket)
        return ERROR_NOT_ENOUGH_MEMORY;

    ControlPacket->dwSize = PacketSize;
    ControlPacket->dwControl = dwControl;
    ControlPacket->hServiceStatus = hServiceStatus;
    ControlPacket->dwServiceTag = dwServiceTag;

    /* Copy the start command line */
    ControlPacket->dwServiceNameOffset = sizeof(SCM_CONTROL_PACKET);
    Ptr = (PWSTR)((ULONG_PTR)ControlPacket + ControlPacket->dwServiceNameOffset);
    wcscpy(Ptr, pServiceName);

    ControlPacket->dwArgumentsCount  = 0;
    ControlPacket->dwArgumentsOffset = 0;

    /* Copy the argument vector */
    if (argc > 0 && argv != NULL)
    {
        PWSTR *pOffPtr, pArgPtr;

        Ptr += wcslen(pServiceName) + 1;
        pOffPtr = (PWSTR*)ALIGN_UP_POINTER(Ptr, PWSTR);
        pArgPtr = (PWSTR)((ULONG_PTR)pOffPtr + argc * sizeof(PWSTR));

        ControlPacket->dwArgumentsCount  = argc;
        ControlPacket->dwArgumentsOffset = (DWORD)((ULONG_PTR)pOffPtr - (ULONG_PTR)ControlPacket);

        DPRINT("dwArgumentsCount: %lu\n", ControlPacket->dwArgumentsCount);
        DPRINT("dwArgumentsOffset: %lu\n", ControlPacket->dwArgumentsOffset);

        for (i = 0; i < argc; i++)
        {
            wcscpy(pArgPtr, argv[i]);
            pOffPtr[i] = (PWSTR)((ULONG_PTR)pArgPtr - (ULONG_PTR)pOffPtr);
            DPRINT("offset[%lu]: %p\n", i, pOffPtr[i]);
            pArgPtr += wcslen(argv[i]) + 1;
        }
    }

    dwError = ScmSendControlPacket(hControlPipe,
                                   pServiceName,
                                   dwControl,
                                   PacketSize,
                                   ControlPacket);

    /* Free the control packet */
    HeapFree(GetProcessHeap(), 0, ControlPacket);

    DPRINT("ScmControlServiceEx(%S, %d) done (Error %lu)\n", pServiceName, dwControl, dwError);
    return dwError;
}


DWORD
ScmControlService(
    _In_ HANDLE hControlPipe,
    _In_ PCWSTR pServiceName,
    _In_ DWORD dwControl,
    _In_ SERVICE_STATUS_HANDLE hServiceStatus)
{
    DWORD dwError = ERROR_SUCCESS;
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD PacketSize;
    PWSTR Ptr;

    DPRINT("ScmControlService(%S, %d) called\n", pServiceName, dwControl);

    /* Calculate the total size of the control packet:
     * initial structure, the start command line, and the argument vector */
    PacketSize = sizeof(SCM_CONTROL_PACKET);
    PacketSize += (DWORD)((wcslen(pServiceName) + 1) * sizeof(WCHAR));

    /* Allocate the control packet */
    ControlPacket = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PacketSize);
    if (!ControlPacket)
        return ERROR_NOT_ENOUGH_MEMORY;

    ControlPacket->dwSize = PacketSize;
    ControlPacket->dwControl = dwControl;
    ControlPacket->hServiceStatus = hServiceStatus;

    /* Copy the service name */
    ControlPacket->dwServiceNameOffset = sizeof(SCM_CONTROL_PACKET);
    Ptr = (PWSTR)((ULONG_PTR)ControlPacket + ControlPacket->dwServiceNameOffset);
    wcscpy(Ptr, pServiceName);

    /* Send the control packet */
    dwError = ScmSendControlPacket(hControlPipe,
                                   pServiceName,
                                   dwControl,
                                   PacketSize,
                                   ControlPacket);

    /* Free the control packet */
    HeapFree(GetProcessHeap(), 0, ControlPacket);

    DPRINT("ScmControlService(%S, %d) done (Error %lu)\n", pServiceName, dwControl, dwError);
    return dwError;
}


static DWORD
ScmWaitForTracedPipeOperation(
    _In_ PSERVICE Service,
    _In_ PCSTR OperationName,
    _Inout_ LPOVERLAPPED Overlapped,
    _Out_ LPDWORD BytesTransferred)
{
    HANDLE WaitHandles[2];
    DWORD WaitHandleCount = 1;
    DWORD StartTick;
    DWORD dwError;
    DWORD ExitCode = 0;

    WaitHandles[0] = Overlapped->hEvent;
    if (Service->lpImage->hProcess != NULL)
    {
        WaitHandles[WaitHandleCount] = Service->lpImage->hProcess;
        WaitHandleCount++;
    }

    StartTick = GetTickCount();

    for (;;)
    {
        DWORD Elapsed = GetTickCount() - StartTick;
        if (Elapsed >= PipeTimeout)
        {
            DPRINT1("ROSGUESTSTATE %s-timeout elapsed=%lu timeout=%lu\n",
                    OperationName,
                    Elapsed,
                    PipeTimeout);

            if (!CancelIo(Service->lpImage->hControlPipe))
            {
                DPRINT1("ROSGUESTSTATE %s-cancel-failed error=%lu\n",
                        OperationName,
                        GetLastError());
            }

            return ERROR_SERVICE_REQUEST_TIMEOUT;
        }

        DWORD Remaining = PipeTimeout - Elapsed;
        DWORD WaitTime = (Remaining > 1000) ? 1000 : Remaining;

        DWORD WaitResult = WaitForMultipleObjects(WaitHandleCount,
                                                  WaitHandles,
                                                  FALSE,
                                                  WaitTime);
        if (WaitResult == WAIT_OBJECT_0)
        {
            if (!GetOverlappedResult(Service->lpImage->hControlPipe,
                                     Overlapped,
                                     BytesTransferred,
                                     FALSE))
            {
                dwError = GetLastError();
                DPRINT1("ROSGUESTSTATE %s-result-failed error=%lu\n",
                        OperationName,
                        dwError);
                return dwError;
            }

            DPRINT1("ROSGUESTSTATE %s-complete bytes=%lu elapsed=%lu\n",
                    OperationName,
                    *BytesTransferred,
                    GetTickCount() - StartTick);
            return ERROR_SUCCESS;
        }

        if ((WaitHandleCount > 1) && (WaitResult == WAIT_OBJECT_0 + 1))
        {
            if (!GetExitCodeProcess(Service->lpImage->hProcess, &ExitCode))
                ExitCode = GetLastError();

            DPRINT1("ROSGUESTSTATE %s-process-exit exit=%lu elapsed=%lu\n",
                    OperationName,
                    ExitCode,
                    GetTickCount() - StartTick);

            if (!CancelIo(Service->lpImage->hControlPipe))
            {
                DPRINT1("ROSGUESTSTATE %s-cancel-after-exit-failed error=%lu\n",
                        OperationName,
                        GetLastError());
            }

            return ERROR_PROCESS_ABORTED;
        }

        if (WaitResult == WAIT_TIMEOUT)
        {
            DPRINT1("ROSGUESTSTATE %s-pending elapsed=%lu remaining=%lu\n",
                    OperationName,
                    Elapsed + WaitTime,
                    Remaining - WaitTime);
            continue;
        }

        dwError = GetLastError();
        if (dwError == ERROR_SUCCESS)
            dwError = ERROR_GEN_FAILURE;

        DPRINT1("ROSGUESTSTATE %s-wait-failed wait=%lu error=%lu\n",
                OperationName,
                WaitResult,
                dwError);
        return dwError;
    }
}


static DWORD
ScmWaitForTracedServiceConnect(
    _In_ PSERVICE Service)
{
    OVERLAPPED Overlapped = {0};
    HANDLE PipeEvent;
    DWORD dwRead = 0;
    DWORD dwProcessId = 0;
    DWORD dwError;
    BOOL bResult;

    DPRINT1("ROSGUESTSTATE wait-connect pipe=%p process=%lu timeout=%lu\n",
            Service->lpImage->hControlPipe,
            Service->lpImage->dwProcessId,
            PipeTimeout);

    PipeEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (PipeEvent == NULL)
    {
        dwError = GetLastError();
        DPRINT1("ROSGUESTSTATE connect-event-failed error=%lu\n", dwError);
        return dwError;
    }

    Overlapped.hEvent = PipeEvent;
    bResult = ConnectNamedPipe(Service->lpImage->hControlPipe, &Overlapped);
    if (!bResult)
    {
        dwError = GetLastError();
        DPRINT1("ROSGUESTSTATE connect-result result=0 error=%lu\n", dwError);

        if (dwError == ERROR_IO_PENDING)
        {
            dwError = ScmWaitForTracedPipeOperation(Service,
                                                    "connect",
                                                    &Overlapped,
                                                    &dwRead);
        }
        else if (dwError == ERROR_PIPE_CONNECTED)
        {
            dwError = ERROR_SUCCESS;
        }

        CloseHandle(PipeEvent);
        if (dwError != ERROR_SUCCESS)
            return dwError;
    }
    else
    {
        DPRINT1("ROSGUESTSTATE connect-result result=1 error=0\n");
        CloseHandle(PipeEvent);
    }

    DPRINT1("ROSGUESTSTATE control-pipe-connected\n");

    PipeEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (PipeEvent == NULL)
    {
        dwError = GetLastError();
        DPRINT1("ROSGUESTSTATE read-event-failed error=%lu\n", dwError);
        return dwError;
    }

    ZeroMemory(&Overlapped, sizeof(Overlapped));
    Overlapped.hEvent = PipeEvent;
    bResult = ReadFile(Service->lpImage->hControlPipe,
                       (LPVOID)&dwProcessId,
                       sizeof(DWORD),
                       &dwRead,
                       &Overlapped);
    if (!bResult)
    {
        dwError = GetLastError();
        DPRINT1("ROSGUESTSTATE read-result result=0 error=%lu\n", dwError);

        if (dwError == ERROR_IO_PENDING)
        {
            dwError = ScmWaitForTracedPipeOperation(Service,
                                                    "read",
                                                    &Overlapped,
                                                    &dwRead);
        }

        CloseHandle(PipeEvent);
        if (dwError != ERROR_SUCCESS)
            return dwError;
    }
    else
    {
        DPRINT1("ROSGUESTSTATE read-result result=1 error=0 bytes=%lu pid=%lu\n",
                dwRead,
                dwProcessId);
        CloseHandle(PipeEvent);
    }

    if ((ScmIsSecurityService(Service->lpImage) == FALSE) &&
        (dwProcessId != Service->lpImage->dwProcessId))
    {
        DPRINT1("Log EVENT_SERVICE_DIFFERENT_PID_CONNECTED by %S\n",
                Service->lpDisplayName);
    }

    DPRINT1("ROSGUESTSTATE wait-connect-done pid=%lu\n", dwProcessId);
    return ERROR_SUCCESS;
}


static DWORD
ScmWaitForServiceConnect(PSERVICE Service)
{
    DWORD dwRead = 0;
    DWORD dwProcessId = 0;
    DWORD dwError = ERROR_SUCCESS;
    BOOL bResult;
    OVERLAPPED Overlapped = {0};
#if 0
    LPCWSTR lpLogStrings[3];
    WCHAR szBuffer1[20];
    WCHAR szBuffer2[20];
#endif

    DPRINT("ScmWaitForServiceConnect()\n");

    if (ScmIsGuestStateTraceService(Service->lpServiceName))
        return ScmWaitForTracedServiceConnect(Service);

    bResult = ConnectNamedPipe(Service->lpImage->hControlPipe,
                               &Overlapped);
    if (bResult == FALSE)
    {
        DPRINT("ConnectNamedPipe() returned FALSE\n");

        dwError = GetLastError();
        if (dwError == ERROR_IO_PENDING)
        {
            DPRINT("dwError: ERROR_IO_PENDING\n");

            dwError = WaitForSingleObject(Service->lpImage->hControlPipe,
                                          PipeTimeout);
            DPRINT("WaitForSingleObject() returned %lu\n", dwError);

            if (dwError == WAIT_TIMEOUT)
            {
                DPRINT("WaitForSingleObject() returned WAIT_TIMEOUT\n");

                bResult = CancelIo(Service->lpImage->hControlPipe);
                if (bResult == FALSE)
                {
                    DPRINT1("CancelIo() failed (Error: %lu)\n", GetLastError());
                }

#if 0
                _ultow(PipeTimeout, szBuffer1, 10);
                lpLogStrings[0] = Service->lpDisplayName;
                lpLogStrings[1] = szBuffer1;

                ScmLogEvent(EVENT_CONNECTION_TIMEOUT,
                            EVENTLOG_ERROR_TYPE,
                            2,
                            lpLogStrings);
#endif
                DPRINT1("Log EVENT_CONNECTION_TIMEOUT by %S\n", Service->lpDisplayName);

                return ERROR_SERVICE_REQUEST_TIMEOUT;
            }
            else if (dwError == WAIT_OBJECT_0)
            {
                bResult = GetOverlappedResult(Service->lpImage->hControlPipe,
                                              &Overlapped,
                                              &dwRead,
                                              TRUE);
                if (bResult == FALSE)
                {
                    dwError = GetLastError();
                    DPRINT1("GetOverlappedResult failed (Error %lu)\n", dwError);

                    return dwError;
                }
            }
        }
        else if (dwError != ERROR_PIPE_CONNECTED)
        {
            DPRINT1("ConnectNamedPipe failed (Error %lu)\n", dwError);
            return dwError;
        }
    }

    DPRINT("Control pipe connected\n");

    Overlapped.hEvent = NULL;

    /* Read the process id from pipe */
    bResult = ReadFile(Service->lpImage->hControlPipe,
                       (LPVOID)&dwProcessId,
                       sizeof(DWORD),
                       &dwRead,
                       &Overlapped);
    if (bResult == FALSE)
    {
        DPRINT("ReadFile() returned FALSE\n");

        dwError = GetLastError();
        if (dwError == ERROR_IO_PENDING)
        {
            DPRINT("dwError: ERROR_IO_PENDING\n");

            dwError = WaitForSingleObject(Service->lpImage->hControlPipe,
                                          PipeTimeout);
            if (dwError == WAIT_TIMEOUT)
            {
                DPRINT("WaitForSingleObject() returned WAIT_TIMEOUT\n");

                bResult = CancelIo(Service->lpImage->hControlPipe);
                if (bResult == FALSE)
                {
                    DPRINT1("CancelIo() failed (Error: %lu)\n", GetLastError());
                }

#if 0
                _ultow(PipeTimeout, szBuffer1, 10);
                lpLogStrings[0] = szBuffer1;

                ScmLogEvent(EVENT_READFILE_TIMEOUT,
                            EVENTLOG_ERROR_TYPE,
                            1,
                            lpLogStrings);
#endif
                DPRINT1("Log EVENT_READFILE_TIMEOUT by %S\n", Service->lpDisplayName);

                return ERROR_SERVICE_REQUEST_TIMEOUT;
            }
            else if (dwError == WAIT_OBJECT_0)
            {
                DPRINT("WaitForSingleObject() returned WAIT_OBJECT_0\n");

                DPRINT("Process Id: %lu\n", dwProcessId);

                bResult = GetOverlappedResult(Service->lpImage->hControlPipe,
                                              &Overlapped,
                                              &dwRead,
                                              TRUE);
                if (bResult == FALSE)
                {
                    dwError = GetLastError();
                    DPRINT1("GetOverlappedResult() failed (Error %lu)\n", dwError);

                    return dwError;
                }
            }
            else
            {
                DPRINT1("WaitForSingleObject() returned %lu\n", dwError);
            }
        }
        else
        {
            DPRINT1("ReadFile() failed (Error %lu)\n", dwError);
            return dwError;
        }
    }

    if ((ScmIsSecurityService(Service->lpImage) == FALSE)&&
        (dwProcessId != Service->lpImage->dwProcessId))
    {
#if 0
        _ultow(Service->lpImage->dwProcessId, szBuffer1, 10);
        _ultow(dwProcessId, szBuffer2, 10);

        lpLogStrings[0] = Service->lpDisplayName;
        lpLogStrings[1] = szBuffer1;
        lpLogStrings[2] = szBuffer2;

        ScmLogEvent(EVENT_SERVICE_DIFFERENT_PID_CONNECTED,
                    EVENTLOG_WARNING_TYPE,
                    3,
                    lpLogStrings);
#endif

        DPRINT1("Log EVENT_SERVICE_DIFFERENT_PID_CONNECTED by %S\n", Service->lpDisplayName);
    }

    DPRINT("ScmWaitForServiceConnect() done\n");

    if (ScmIsGuestStateTraceService(Service->lpServiceName))
    {
        DPRINT1("ROSGUESTSTATE wait-connect-done pid=%lu\n", dwProcessId);
    }

    return ERROR_SUCCESS;
}


static DWORD
ScmStartUserModeService(PSERVICE Service,
                        DWORD argc,
                        const PCWSTR* argv)
{
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFOW StartupInfo;
    LPVOID lpEnvironment;
    BOOL Result;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("ScmStartUserModeService(%p)\n", Service);

    if (ScmIsGuestStateTraceService(Service->lpServiceName))
    {
        DPRINT1("ROSGUESTSTATE start-user enter image='%S' account='%S' run-count=%lu token=%p\n",
                Service->lpImage->pszImagePath,
                Service->lpImage->pszAccountName ? Service->lpImage->pszAccountName : L"(null)",
                Service->lpImage->dwImageRunCount,
                Service->lpImage->hToken);
    }

    /* If the image is already running, just send a start command */
    if (Service->lpImage->dwImageRunCount > 1)
        goto Quit;

    /* Otherwise start its process */
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

    if (Service->lpImage->hToken)
    {
        /* User token: Run the service under the user account */

        if (!CreateEnvironmentBlock(&lpEnvironment, Service->lpImage->hToken, FALSE))
        {
            /* We failed, run the service with the current environment */
            DPRINT1("CreateEnvironmentBlock() failed with error %d; service '%S' will run with current environment\n",
                    GetLastError(), Service->lpServiceName);
            lpEnvironment = NULL;
        }

        /* Impersonate the new user */
        Result = ImpersonateLoggedOnUser(Service->lpImage->hToken);
        if (Result)
        {
            /* Launch the process in the user's logon session */
            Result = CreateProcessAsUserW(Service->lpImage->hToken,
                                          NULL,
                                          Service->lpImage->pszImagePath,
                                          NULL,
                                          NULL,
                                          FALSE,
                                          CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS | CREATE_SUSPENDED,
                                          lpEnvironment,
                                          NULL,
                                          &StartupInfo,
                                          &ProcessInformation);
            if (!Result)
                dwError = GetLastError();

            /* Revert the impersonation */
            RevertToSelf();
        }
        else
        {
            dwError = GetLastError();
            DPRINT1("ImpersonateLoggedOnUser() failed with error %d\n", dwError);
        }
    }
    else
    {
        /* No user token: Run the service under the LocalSystem account */

        if (!CreateEnvironmentBlock(&lpEnvironment, NULL, TRUE))
        {
            /* We failed, run the service with the current environment */
            DPRINT1("CreateEnvironmentBlock() failed with error %d; service '%S' will run with current environment\n",
                    GetLastError(), Service->lpServiceName);
            lpEnvironment = NULL;
        }

        /* Use the interactive desktop if the service is interactive */
        if ((NoInteractiveServices == 0) &&
            (Service->Status.dwServiceType & SERVICE_INTERACTIVE_PROCESS))
        {
            StartupInfo.dwFlags |= STARTF_INHERITDESKTOP;
            StartupInfo.lpDesktop = L"WinSta0\\Default";
        }

        if (!ScmIsSecurityService(Service->lpImage))
        {
            Result = CreateProcessW(NULL,
                                    Service->lpImage->pszImagePath,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS | CREATE_SUSPENDED,
                                    lpEnvironment,
                                    NULL,
                                    &StartupInfo,
                                    &ProcessInformation);
            if (!Result)
                dwError = GetLastError();
        }
        else
        {
            Result = TRUE;
            dwError = ERROR_SUCCESS;
        }
    }

    if (lpEnvironment)
        DestroyEnvironmentBlock(lpEnvironment);

    if (!Result)
    {
        if (ScmIsGuestStateTraceService(Service->lpServiceName))
        {
            DPRINT1("ROSGUESTSTATE createprocess-failed error=%lu image='%S'\n",
                    dwError,
                    Service->lpImage->pszImagePath);
        }

        DPRINT1("Starting '%S' failed with error %d\n",
                Service->lpServiceName, dwError);
        return dwError;
    }

    DPRINT("Process Id: %lu  Handle %p\n",
           ProcessInformation.dwProcessId,
           ProcessInformation.hProcess);
    DPRINT("Thread Id: %lu  Handle %p\n",
           ProcessInformation.dwThreadId,
           ProcessInformation.hThread);

    if (ScmIsGuestStateTraceService(Service->lpServiceName))
    {
        DPRINT1("ROSGUESTSTATE createprocess-ok pid=%lu tid=%lu process=%p thread=%p\n",
                ProcessInformation.dwProcessId,
                ProcessInformation.dwThreadId,
                ProcessInformation.hProcess,
                ProcessInformation.hThread);
    }

    /* Get the process handle and ID */
    Service->lpImage->hProcess = ProcessInformation.hProcess;
    Service->lpImage->dwProcessId = ProcessInformation.dwProcessId;

    /* Resume the main thread and close its handle */
    ResumeThread(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hThread);

    /* Connect control pipe */
    dwError = ScmWaitForServiceConnect(Service);
    if (dwError != ERROR_SUCCESS)
    {
        if (ScmIsGuestStateTraceService(Service->lpServiceName))
        {
            DPRINT1("ROSGUESTSTATE wait-connect-failed error=%lu\n", dwError);
        }

        DPRINT1("Connecting control pipe failed! (Error %lu)\n", dwError);
        Service->lpImage->dwProcessId = 0;
        return dwError;
    }

Quit:
    /* Send the start command and return */
    return ScmControlServiceEx(Service->lpImage->hControlPipe,
                               Service->lpServiceName,
                               (Service->Status.dwServiceType & SERVICE_WIN32_OWN_PROCESS)
                                  ? SERVICE_CONTROL_START_OWN : SERVICE_CONTROL_START_SHARE,
                               (SERVICE_STATUS_HANDLE)Service,
                               Service->dwServiceTag,
                               argc, argv);
}


static DWORD
ScmLoadService(PSERVICE Service,
               DWORD argc,
               const PCWSTR* argv)
{
    PSERVICE_GROUP Group = Service->lpGroup;
    DWORD dwError = ERROR_SUCCESS;
    LPCWSTR lpLogStrings[2];
    WCHAR szLogBuffer[80];

    DPRINT("ScmLoadService() called\n");
    DPRINT("Start Service %p (%S)\n", Service, Service->lpServiceName);

    if (ScmIsGuestStateTraceService(Service->lpServiceName))
    {
        DPRINT1("ROSGUESTSTATE load-enter state=%lu type=%lu start=%lu visited=%u image=%p\n",
                Service->Status.dwCurrentState,
                Service->Status.dwServiceType,
                Service->dwStartType,
                Service->ServiceVisited,
                Service->lpImage);
    }

    if (Service->Status.dwCurrentState != SERVICE_STOPPED)
    {
        DPRINT("Service %S is already running\n", Service->lpServiceName);
        return ERROR_SERVICE_ALREADY_RUNNING;
    }

    DPRINT("Service->Type: %lu\n", Service->Status.dwServiceType);

    if (Service->Status.dwServiceType & SERVICE_DRIVER)
    {
        /* Start the driver */
        dwError = ScmStartDriver(Service);
    }
    else // if (Service->Status.dwServiceType & (SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS))
    {
        /* Start user-mode service */
        dwError = ScmCreateOrReferenceServiceImage(Service);
        if (ScmIsGuestStateTraceService(Service->lpServiceName))
        {
            DPRINT1("ROSGUESTSTATE create-image-result error=%lu image=%p\n",
                    dwError,
                    Service->lpImage);
        }

        if (dwError == ERROR_SUCCESS)
        {
            dwError = ScmStartUserModeService(Service, argc, argv);
            if (ScmIsGuestStateTraceService(Service->lpServiceName))
            {
                DPRINT1("ROSGUESTSTATE start-user-result error=%lu image=%p pid=%lu\n",
                        dwError,
                        Service->lpImage,
                        Service->lpImage ? Service->lpImage->dwProcessId : 0);
            }

            if (dwError == ERROR_SUCCESS)
            {
                Service->Status.dwCurrentState = SERVICE_START_PENDING;
                Service->Status.dwControlsAccepted = 0;
                ScmReferenceService(Service);
            }
            else
            {
                Service->lpImage->dwImageRunCount--;
                if (Service->lpImage->dwImageRunCount == 0)
                {
                    ScmRemoveServiceImage(Service->lpImage);
                    Service->lpImage = NULL;
                }
            }
        }
    }

    DPRINT("ScmLoadService() done (Error %lu)\n", dwError);

    if (ScmIsGuestStateTraceService(Service->lpServiceName))
    {
        DPRINT1("ROSGUESTSTATE load-done error=%lu state=%lu win32=%lu\n",
                dwError,
                Service->Status.dwCurrentState,
                Service->Status.dwWin32ExitCode);
    }

    if (dwError == ERROR_SUCCESS)
    {
        if (Group != NULL)
        {
            Group->ServicesRunning = TRUE;
        }

        /* Log a successful service start */
        LoadStringW(GetModuleHandle(NULL), IDS_SERVICE_START, szLogBuffer, 80);
        lpLogStrings[0] = Service->lpDisplayName;
        lpLogStrings[1] = szLogBuffer;

        ScmLogEvent(EVENT_SERVICE_CONTROL_SUCCESS,
                    EVENTLOG_INFORMATION_TYPE,
                    2,
                    lpLogStrings);
    }
    else
    {
        if (Service->dwErrorControl != SERVICE_ERROR_IGNORE)
        {
            /* Log a failed service start */
            StringCchPrintfW(szLogBuffer, ARRAYSIZE(szLogBuffer),
                             L"%lu", dwError);
            lpLogStrings[0] = Service->lpServiceName;
            lpLogStrings[1] = szLogBuffer;
            ScmLogEvent(EVENT_SERVICE_START_FAILED,
                        EVENTLOG_ERROR_TYPE,
                        2,
                        lpLogStrings);
        }

#if 0
        switch (Service->dwErrorControl)
        {
            case SERVICE_ERROR_SEVERE:
                if (IsLastKnownGood == FALSE)
                {
                    /* FIXME: Boot last known good configuration */
                }
                break;

            case SERVICE_ERROR_CRITICAL:
                if (IsLastKnownGood == FALSE)
                {
                    /* FIXME: Boot last known good configuration */
                }
                else
                {
                    /* FIXME: BSOD! */
                }
                break;
        }
#endif
    }

    return dwError;
}


DWORD
ScmStartService(PSERVICE Service,
                DWORD argc,
                const PCWSTR* argv)
{
    DWORD dwError = ERROR_SUCCESS;
    SC_RPC_LOCK Lock = NULL;

    DPRINT("ScmStartService() called\n");
    DPRINT("Start Service %p (%S)\n", Service, Service->lpServiceName);

    /* Acquire the service control critical section, to synchronize starts */
    EnterCriticalSection(&ControlServiceCriticalSection);

    /*
     * Acquire the user service start lock while the service is starting, if
     * needed (i.e. if we are not starting it during the initialization phase).
     * If we don't success, bail out.
     */
    if (!ScmInitialize)
    {
        dwError = ScmAcquireServiceStartLock(TRUE, &Lock);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    /* Really start the service */
    dwError = ScmLoadService(Service, argc, argv);

    /* Release the service start lock, if needed, and the critical section */
    if (Lock) ScmReleaseServiceStartLock(&Lock);

done:
    LeaveCriticalSection(&ControlServiceCriticalSection);

    DPRINT("ScmStartService() done (Error %lu)\n", dwError);

    return dwError;
}


VOID
ScmAutoStartServices(VOID)
{
    DWORD dwError;
    PLIST_ENTRY GroupEntry;
    PLIST_ENTRY ServiceEntry;
    PSERVICE_GROUP CurrentGroup;
    PSERVICE CurrentService;
    PSERVICE GuestStateCollectorService = NULL;
    WCHAR szSafeBootServicePath[MAX_PATH];
    DWORD SafeBootEnabled;
    HKEY hKey;
    DWORD dwKeySize;
    ULONG i;

    /*
     * This function MUST be called ONLY at initialization time.
     * Therefore, no need to acquire the user service start lock.
     */
    ASSERT(ScmInitialize);

    /* Retrieve the SafeBoot parameter */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option",
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        dwKeySize = sizeof(SafeBootEnabled);
        dwError = RegQueryValueExW(hKey,
                                   L"OptionValue",
                                   0,
                                   NULL,
                                   (LPBYTE)&SafeBootEnabled,
                                   &dwKeySize);
        RegCloseKey(hKey);
    }

    /* Default to Normal boot if the value doesn't exist */
    if (dwError != ERROR_SUCCESS)
        SafeBootEnabled = 0;

    DPRINT1("ROSGUESTSTATE autostart-safe-boot=%lu\n", SafeBootEnabled);

    /* Acquire the service control critical section, to synchronize starts */
    EnterCriticalSection(&ControlServiceCriticalSection);

    /* Clear 'ServiceVisited' flag (or set if not to start in Safe Mode) */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if (ScmIsGuestStateCollectorService(CurrentService->lpServiceName))
        {
            GuestStateCollectorService = CurrentService;
            ScmTraceGuestStateAutoStartTarget("precheck", CurrentService);
        }

        /* Build the safe boot path */
        StringCchCopyW(szSafeBootServicePath, ARRAYSIZE(szSafeBootServicePath),
                       L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot");

        switch (SafeBootEnabled)
        {
            /* NOTE: Assumes MINIMAL (1) and DSREPAIR (3) load same items */
            case 1:
            case 3:
                StringCchCatW(szSafeBootServicePath, ARRAYSIZE(szSafeBootServicePath),
                              L"\\Minimal\\");
                break;

            case 2:
                StringCchCatW(szSafeBootServicePath, ARRAYSIZE(szSafeBootServicePath),
                              L"\\Network\\");
                break;
        }

        if (SafeBootEnabled != 0)
        {
            /* If key does not exist then do not assume safe mode */
            dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                    szSafeBootServicePath,
                                    0,
                                    KEY_READ,
                                    &hKey);
            if (dwError == ERROR_SUCCESS)
            {
                RegCloseKey(hKey);

                /* Finish Safe Boot path off */
                StringCchCatW(szSafeBootServicePath, ARRAYSIZE(szSafeBootServicePath),
                              CurrentService->lpServiceName);

                /* Check that the key is in the Safe Boot path */
                dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                        szSafeBootServicePath,
                                        0,
                                        KEY_READ,
                                        &hKey);
                if (dwError != ERROR_SUCCESS)
                {
                    /* Mark service as visited so it is not auto-started */
                    CurrentService->ServiceVisited = TRUE;
                }
                else
                {
                    /* Must be auto-started in safe mode - mark as unvisited */
                    RegCloseKey(hKey);
                    CurrentService->ServiceVisited = FALSE;
                }
            }
            else
            {
                DPRINT1("WARNING: Could not open the associated Safe Boot key\n");
                CurrentService->ServiceVisited = FALSE;
            }
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    ScmTraceGuestStateAutoStartTarget("after-precheck", GuestStateCollectorService);
    DPRINT1("ROSGUESTSTATE autostart-pass-enter pass=grouped target=%p\n",
            GuestStateCollectorService);

    /* Start all services which are members of an existing group */
    GroupEntry = GroupListHead.Flink;
    while (GroupEntry != &GroupListHead)
    {
        CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

        DPRINT("Group '%S'\n", CurrentGroup->lpGroupName);

        /* Start all services witch have a valid tag */
        for (i = 0; i < CurrentGroup->TagCount; i++)
        {
            ServiceEntry = ServiceListHead.Flink;
            while (ServiceEntry != &ServiceListHead)
            {
                CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

                if ((CurrentService->lpGroup == CurrentGroup) &&
                    (CurrentService->dwStartType == SERVICE_AUTO_START) &&
                    (CurrentService->ServiceVisited == FALSE) &&
                    (CurrentService->dwTag == CurrentGroup->TagArray[i]))
                {
                    if (ScmIsGuestStateCollectorService(CurrentService->lpServiceName))
                    {
                        DPRINT1("ROSGUESTSTATE autostart-group-tag group='%S' tag=%lu\n",
                                CurrentGroup->lpGroupName,
                                CurrentService->dwTag);
                    }

                    CurrentService->ServiceVisited = TRUE;
                    ScmTraceAutoStartService("group-tag", CurrentService);
                    dwError = ScmLoadService(CurrentService, 0, NULL);
                    ScmTraceAutoStartServiceResult("group-tag", CurrentService, dwError);
                }

                ServiceEntry = ServiceEntry->Flink;
             }
        }

        /* Start all services which have an invalid tag or which do not have a tag */
        ServiceEntry = ServiceListHead.Flink;
        while (ServiceEntry != &ServiceListHead)
        {
            CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

            if ((CurrentService->lpGroup == CurrentGroup) &&
                (CurrentService->dwStartType == SERVICE_AUTO_START) &&
                (CurrentService->ServiceVisited == FALSE))
            {
                if (ScmIsGuestStateCollectorService(CurrentService->lpServiceName))
                {
                    DPRINT1("ROSGUESTSTATE autostart-group group='%S' tag=%lu\n",
                            CurrentGroup->lpGroupName,
                            CurrentService->dwTag);
                }

                CurrentService->ServiceVisited = TRUE;
                ScmTraceAutoStartService("group", CurrentService);
                dwError = ScmLoadService(CurrentService, 0, NULL);
                ScmTraceAutoStartServiceResult("group", CurrentService, dwError);
            }

            ServiceEntry = ServiceEntry->Flink;
        }

        GroupEntry = GroupEntry->Flink;
    }

    ScmTraceGuestStateAutoStartTarget("after-grouped", GuestStateCollectorService);
    DPRINT1("ROSGUESTSTATE autostart-pass-enter pass=missing-group target=%p\n",
            GuestStateCollectorService);

    /* Start all services which are members of any non-existing group */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
        ScmTraceAutoStartScan("missing-group", CurrentService);

        if ((CurrentService->lpGroup != NULL) &&
            (CurrentService->dwStartType == SERVICE_AUTO_START) &&
            (CurrentService->ServiceVisited == FALSE))
        {
            if (ScmIsGuestStateCollectorService(CurrentService->lpServiceName))
            {
                DPRINT1("ROSGUESTSTATE autostart-missing-group group='%S' tag=%lu\n",
                        CurrentService->lpGroup->lpGroupName,
                        CurrentService->dwTag);
            }

            CurrentService->ServiceVisited = TRUE;
            ScmTraceAutoStartService("missing-group", CurrentService);
            dwError = ScmLoadService(CurrentService, 0, NULL);
            ScmTraceAutoStartServiceResult("missing-group", CurrentService, dwError);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    ScmTraceGuestStateAutoStartTarget("after-missing-group", GuestStateCollectorService);
    DPRINT1("ROSGUESTSTATE autostart-pass-enter pass=no-group target=%p\n",
            GuestStateCollectorService);

    /* Start all services which are not a member of any group */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
        ScmTraceAutoStartScan("no-group", CurrentService);

        if ((CurrentService->lpGroup == NULL) &&
            (CurrentService->dwStartType == SERVICE_AUTO_START) &&
            (CurrentService->ServiceVisited == FALSE))
        {
            if (ScmIsGuestStateCollectorService(CurrentService->lpServiceName))
            {
                DPRINT1("ROSGUESTSTATE autostart-no-group tag=%lu\n",
                        CurrentService->dwTag);
            }

            CurrentService->ServiceVisited = TRUE;
            ScmTraceAutoStartService("no-group", CurrentService);
            dwError = ScmLoadService(CurrentService, 0, NULL);
            ScmTraceAutoStartServiceResult("no-group", CurrentService, dwError);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    ScmTraceGuestStateAutoStartTarget("after-no-group", GuestStateCollectorService);
    DPRINT1("ROSGUESTSTATE autostart-pass-enter pass=clear-visited target=%p\n",
            GuestStateCollectorService);

    /* Clear 'ServiceVisited' flag again */
    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
        CurrentService->ServiceVisited = FALSE;
        ServiceEntry = ServiceEntry->Flink;
    }

    ScmTraceGuestStateAutoStartTarget("after-clear-visited", GuestStateCollectorService);

    /* Release the critical section */
    LeaveCriticalSection(&ControlServiceCriticalSection);
}


VOID
ScmAutoShutdownServices(VOID)
{
    PLIST_ENTRY ServiceEntry;
    PSERVICE CurrentService;

    DPRINT("ScmAutoShutdownServices() called\n");

    /* Lock the service database exclusively */
    ScmLockDatabaseExclusive();

    ServiceEntry = ServiceListHead.Flink;
    while (ServiceEntry != &ServiceListHead)
    {
        CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

        if ((CurrentService->Status.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) &&
            (CurrentService->Status.dwCurrentState == SERVICE_RUNNING ||
             CurrentService->Status.dwCurrentState == SERVICE_START_PENDING))
        {
            /* Send the shutdown notification */
            DPRINT("Shutdown service: %S\n", CurrentService->lpServiceName);
            ScmControlService(CurrentService->lpImage->hControlPipe,
                              CurrentService->lpServiceName,
                              SERVICE_CONTROL_SHUTDOWN,
                              (SERVICE_STATUS_HANDLE)CurrentService);
        }

        ServiceEntry = ServiceEntry->Flink;
    }

    /* Unlock the service database */
    ScmUnlockDatabase();

    DPRINT("ScmAutoShutdownServices() done\n");
}


BOOL
ScmLockDatabaseExclusive(VOID)
{
    return RtlAcquireResourceExclusive(&DatabaseLock, TRUE);
}


BOOL
ScmLockDatabaseShared(VOID)
{
    return RtlAcquireResourceShared(&DatabaseLock, TRUE);
}


VOID
ScmUnlockDatabase(VOID)
{
    RtlReleaseResource(&DatabaseLock);
}


VOID
ScmInitNamedPipeCriticalSection(VOID)
{
    HKEY hKey;
    DWORD dwKeySize;
    DWORD dwError;

    InitializeCriticalSection(&ControlServiceCriticalSection);

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Control",
                            0,
                            KEY_READ,
                            &hKey);
   if (dwError == ERROR_SUCCESS)
   {
        dwKeySize = sizeof(PipeTimeout);
        RegQueryValueExW(hKey,
                         L"ServicesPipeTimeout",
                         0,
                         NULL,
                         (LPBYTE)&PipeTimeout,
                         &dwKeySize);
       RegCloseKey(hKey);
   }
}


VOID
ScmDeleteNamedPipeCriticalSection(VOID)
{
    DeleteCriticalSection(&ControlServiceCriticalSection);
}

/* EOF */
