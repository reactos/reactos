/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Network configuration
 * FILE:              dll/win32/netcfgx/netinstall.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include <rpc.h>


typedef struct _COMPONENT_INFO
{
    PWSTR pszFullInfPath;
    PWSTR pszInfPath;
    PWSTR pszInfSection;
    PWSTR pszComponentId;
    PWSTR pszDescription;
    PWSTR pszClassGuid;
    DWORD dwCharacteristics;
} COMPONENT_INFO, *PCOMPONENT_INFO;


/* GLOBALS ******************************************************************/


/* FUNCTIONS ****************************************************************/

static
BOOL
InstallInfSections(
    _In_ HWND hWnd,
    _In_ HKEY hKey,
    _In_ LPCWSTR InfFile,
    _In_ LPCWSTR InfSection)
{
    WCHAR Buffer[MAX_PATH];
    HINF hInf = INVALID_HANDLE_VALUE;
    UINT BufferSize;
    PVOID Context = NULL;
    BOOL ret = FALSE;

    TRACE("InstallInfSections()\n");

    if (InfSection == NULL)
        return FALSE;

    /* Get Windows directory */
    BufferSize = ARRAYSIZE(Buffer) - 5 - wcslen(InfFile);
    if (GetWindowsDirectoryW(Buffer, BufferSize) > BufferSize)
    {
        /* Function failed */
        SetLastError(ERROR_GEN_FAILURE);
        goto cleanup;
    }

    /* We have enough space to add some information in the buffer */
    if (Buffer[wcslen(Buffer) - 1] != '\\')
        wcscat(Buffer, L"\\");
    wcscat(Buffer, L"Inf\\");
    wcscat(Buffer, InfFile);

    /* Install specified section */
    hInf = SetupOpenInfFileW(Buffer, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        goto cleanup;

    Context = SetupInitDefaultQueueCallback(hWnd);
    if (Context == NULL)
        goto cleanup;

    ret = SetupInstallFromInfSectionW(
                hWnd, hInf,
                InfSection, SPINST_ALL,
                hKey, NULL, SP_COPY_NEWER,
                SetupDefaultQueueCallbackW, Context,
                NULL, NULL);
    if (ret == FALSE)
    {
        ERR("SetupInstallFromInfSectionW(%S) failed (Error %lx)\n", InfSection, GetLastError());
        goto cleanup;
    }

    wcscpy(Buffer, InfSection);
    wcscat(Buffer, L".Services");

    ret = SetupInstallServicesFromInfSectionW(hInf, Buffer, 0);
    if (ret == FALSE)
    {
        ERR("SetupInstallServicesFromInfSectionW(%S) failed (Error %lx)\n", Buffer, GetLastError());
        goto cleanup;
    }

cleanup:
    if (Context)
        SetupTermDefaultQueueCallback(Context);

    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);

    TRACE("InstallInfSections() done %u\n", ret);

    return ret;
}


static
BOOL
CreateInstanceKey(
    _In_ PCOMPONENT_INFO pComponentInfo,
    _Out_ PHKEY pInstanceKey)
{
    WCHAR szKeyBuffer[128];
    LPWSTR UuidString = NULL;
    UUID Uuid;
    RPC_STATUS RpcStatus;
    HKEY hInstanceKey;
    DWORD rc;
    BOOL ret = FALSE;

    TRACE("CreateInstanceKey()\n");

    *pInstanceKey = NULL;

    wcscpy(szKeyBuffer, L"SYSTEM\\CurrentControlSet\\Control\\Network\\");
    wcscat(szKeyBuffer, pComponentInfo->pszClassGuid);
    wcscat(szKeyBuffer, L"\\{");

    /* Create a new UUID */
    RpcStatus = UuidCreate(&Uuid);
    if (RpcStatus != RPC_S_OK && RpcStatus != RPC_S_UUID_LOCAL_ONLY)
    {
        ERR("UuidCreate() failed with RPC status 0x%lx\n", RpcStatus);
        goto done;
    }

    RpcStatus = UuidToStringW(&Uuid, &UuidString);
    if (RpcStatus != RPC_S_OK)
    {
        ERR("UuidToStringW() failed with RPC status 0x%lx\n", RpcStatus);
        goto done;
    }

    wcscat(szKeyBuffer, UuidString);
    wcscat(szKeyBuffer, L"}");

    RpcStringFreeW(&UuidString);

    TRACE("szKeyBuffer %S\n", szKeyBuffer);

    rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                         szKeyBuffer,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                         NULL,
                         &hInstanceKey,
                         NULL);
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW() failed with error 0x%lx\n", rc);
        goto done;
    }

    rc = RegSetValueExW(hInstanceKey,
                        L"Characteristics",
                        0,
                        REG_DWORD,
                        (LPBYTE)&pComponentInfo->dwCharacteristics,
                        sizeof(DWORD));
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto done;
    }

    rc = RegSetValueExW(hInstanceKey,
                        L"ComponentId",
                        0,
                        REG_SZ,
                        (LPBYTE)pComponentInfo->pszComponentId,
                        (wcslen(pComponentInfo->pszComponentId) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto done;
    }

    rc = RegSetValueExW(hInstanceKey,
                        L"Description",
                        0,
                        REG_SZ,
                        (LPBYTE)pComponentInfo->pszDescription,
                        (wcslen(pComponentInfo->pszDescription) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto done;
    }

    rc = RegSetValueExW(hInstanceKey,
                        L"InfPath",
                        0,
                        REG_SZ,
                        (LPBYTE)pComponentInfo->pszInfPath,
                        (wcslen(pComponentInfo->pszInfPath) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto done;
    }

    rc = RegSetValueExW(hInstanceKey,
                        L"InfSection",
                        0,
                        REG_SZ,
                        (LPBYTE)pComponentInfo->pszInfSection,
                        (wcslen(pComponentInfo->pszInfSection) + 1) * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegSetValueExW() failed with error 0x%lx\n", rc);
        goto done;
    }

    *pInstanceKey = hInstanceKey;
    ret = TRUE;

done:
    if (ret == FALSE)
        RegCloseKey(hInstanceKey);

    TRACE("CreateInstanceKey() done %u\n", ret);

    return ret;
}


static
BOOL
CheckInfFile(
    _In_ PWSTR pszFullInfName,
    _In_ PCWSTR pszComponentId,
    _In_ PCOMPONENT_INFO pComponentInfo)
{
    WCHAR szLineBuffer[MAX_PATH];
    HINF hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT MfgContext, DevContext, MiscContext;
    DWORD dwLength;

    hInf = SetupOpenInfFileW(pszFullInfName,
                             NULL,
                             INF_STYLE_WIN4,
                             NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        ERR("\n");
        return FALSE;
    }

    if (!SetupFindFirstLineW(hInf,
                             L"Manufacturer",
                             NULL,
                             &MfgContext))
    {
        ERR("No Manufacurer section found!\n");
        goto done;
    }

    for (;;)
    {
        if (!SetupGetStringFieldW(&MfgContext,
                                  1,
                                  szLineBuffer,
                                  MAX_PATH,
                                  NULL))
            break;

        TRACE("Manufacturer: %S\n", szLineBuffer);
        if (!SetupFindFirstLineW(hInf,
                                 szLineBuffer,
                                 NULL,
                                 &DevContext))
            break;

        for (;;)
        {
            if (!SetupGetStringFieldW(&DevContext,
                                      2,
                                      szLineBuffer,
                                      MAX_PATH,
                                      NULL))
                break;

            TRACE("Device: %S\n", szLineBuffer);
            if (_wcsicmp(szLineBuffer, pszComponentId) == 0)
            {
                TRACE("Found it!\n");

                /* Get the section name*/
                SetupGetStringFieldW(&DevContext,
                                     1,
                                     NULL,
                                     0,
                                     &dwLength);

                pComponentInfo->pszInfSection = HeapAlloc(GetProcessHeap(),
                                                          0,
                                                          dwLength * sizeof(WCHAR));
                if (pComponentInfo->pszInfSection)
                {
                    SetupGetStringFieldW(&DevContext,
                                         1,
                                         pComponentInfo->pszInfSection,
                                         dwLength,
                                         &dwLength);
                }

                /* Get the description*/
                SetupGetStringFieldW(&DevContext,
                                     0,
                                     NULL,
                                     0,
                                     &dwLength);

                pComponentInfo->pszDescription = HeapAlloc(GetProcessHeap(),
                                                           0,
                                                           dwLength * sizeof(WCHAR));
                if (pComponentInfo->pszDescription)
                {
                    SetupGetStringFieldW(&DevContext,
                                         0,
                                         pComponentInfo->pszDescription,
                                         dwLength,
                                         &dwLength);
                }

                /* Get the class GUID */
                if (SetupFindFirstLineW(hInf,
                                        L"Version",
                                        L"ClassGuid",
                                        &MiscContext))
                {
                    SetupGetStringFieldW(&MiscContext,
                                         1,
                                         NULL,
                                         0,
                                         &dwLength);

                    pComponentInfo->pszClassGuid = HeapAlloc(GetProcessHeap(),
                                                             0,
                                                             dwLength * sizeof(WCHAR));
                    if (pComponentInfo->pszInfSection)
                    {
                        SetupGetStringFieldW(&MiscContext,
                                             1,
                                             pComponentInfo->pszClassGuid,
                                             dwLength,
                                             &dwLength);
                    }
                }

                /* Get the Characteristics value */
                if (SetupFindFirstLineW(hInf,
                                        pComponentInfo->pszInfSection,
                                        L"Characteristics",
                                        &MiscContext))
                {
                    SetupGetIntField(&MiscContext,
                                     1,
                                     (PINT)&pComponentInfo->dwCharacteristics);
                }

                SetupCloseInfFile(hInf);
                return TRUE;
            }

            if (!SetupFindNextLine(&DevContext, &DevContext))
                break;
        }

        if (!SetupFindNextLine(&MfgContext, &MfgContext))
            break;
    }

done:
    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);

    return FALSE;
}


static
BOOL
ScanForInfFile(
    _In_ PCWSTR pszComponentId,
    _In_ PCOMPONENT_INFO pComponentInfo)
{
    WCHAR szInfPath[MAX_PATH];
    WCHAR szFullInfName[MAX_PATH];
    WCHAR szPathBuffer[MAX_PATH];
    WIN32_FIND_DATAW fdw;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    BOOL bFound = FALSE;

    GetWindowsDirectoryW(szInfPath, MAX_PATH);
    wcscat(szInfPath, L"\\inf");

    wcscpy(szPathBuffer, szInfPath);
    wcscat(szPathBuffer, L"\\*.inf");

    hFindFile = FindFirstFileW(szPathBuffer, &fdw);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return FALSE;

    for (;;)
    {
        if (wcscmp(fdw.cFileName, L".") == 0 ||
            wcscmp(fdw.cFileName, L"..") == 0)
            continue;

        TRACE("FileName: %S\n", fdw.cFileName);

        wcscpy(szFullInfName, szInfPath);
        wcscat(szFullInfName, L"\\");
        wcscat(szFullInfName, fdw.cFileName);

        TRACE("Full Inf Name: %S\n", szFullInfName);
        if (CheckInfFile(szFullInfName,
                         pszComponentId,
                         pComponentInfo))
        {
            pComponentInfo->pszFullInfPath = HeapAlloc(GetProcessHeap(),
                                                       0,
                                                       (wcslen(szFullInfName) + 1) * sizeof(WCHAR));
            if (pComponentInfo->pszFullInfPath)
                wcscpy(pComponentInfo->pszFullInfPath, szFullInfName);

            pComponentInfo->pszInfPath = HeapAlloc(GetProcessHeap(),
                                                   0,
                                                   (wcslen(fdw.cFileName) + 1) * sizeof(WCHAR));
            if (pComponentInfo->pszInfPath)
                wcscpy(pComponentInfo->pszInfPath, fdw.cFileName);

            pComponentInfo->pszComponentId = HeapAlloc(GetProcessHeap(),
                                                       0,
                                                       (wcslen(pszComponentId) + 1) * sizeof(WCHAR));
            if (pComponentInfo->pszComponentId)
                wcscpy(pComponentInfo->pszComponentId, pszComponentId);

            bFound = TRUE;
            break;
        }

        if (!FindNextFileW(hFindFile, &fdw))
            break;
    }

    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);

    return bFound;
}

static
VOID
StartNetworkServices(
    _In_ PCOMPONENT_INFO pComponentInfo)
{
    WCHAR szServiceName[64];
    HINF hInf = INVALID_HANDLE_VALUE;
    INFCONTEXT Context;
    PWSTR pServiceSection = NULL;

    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;

    hManager = OpenSCManagerW(NULL,
                              NULL,
                              SC_MANAGER_ALL_ACCESS);
    if (hManager == NULL)
    {
        ERR("\n");
        goto done;
    }

    hInf = SetupOpenInfFileW(pComponentInfo->pszFullInfPath,
                             NULL,
                             INF_STYLE_WIN4,
                             NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        ERR("\n");
        goto done;
    }

    pServiceSection = HeapAlloc(GetProcessHeap(), 0,
                                (wcslen(pComponentInfo->pszInfSection) + wcslen(L".Service") + 1) * sizeof(WCHAR));
    if (pServiceSection == NULL)
    {
        ERR("\n");
        goto done;
    }

    wcscpy(pServiceSection, pComponentInfo->pszInfSection);
    wcscat(pServiceSection, L".Service");

    TRACE("ServiceSection: %S\n", pServiceSection);

    if (!SetupFindFirstLineW(hInf,
                             pServiceSection,
                             L"AddService",
                             &Context))
    {
        ERR("\n");
        goto done;
    }

    for (;;)
    {
        SetupGetStringFieldW(&Context,
                             1,
                             szServiceName,
                             64,
                             NULL);

        hService = OpenServiceW(hManager,
                                szServiceName,
                                SERVICE_START);
        if (hService)
        {
            if (!StartService(hService, 0, NULL))
            {
                ERR("StartService() %S failed %lu\n", szServiceName, GetLastError());
            }

            CloseServiceHandle(hService);
        }

        if (!SetupFindNextLine(&Context, &Context))
            break;
    }

done:
    if (pServiceSection)
        HeapFree(GetProcessHeap(), 0, pServiceSection);

    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);

    if (hManager != NULL)
        CloseServiceHandle(hManager);
}


BOOL
InstallNetworkComponent(
    _In_ PCWSTR pszComponentId)
{
    COMPONENT_INFO ComponentInfo;
    HKEY hInstanceKey = NULL;
    BOOL bResult = FALSE;

    TRACE("InstallNetworkComponent(%S)\n", pszComponentId);

    ZeroMemory(&ComponentInfo, sizeof(COMPONENT_INFO));

    if (!ScanForInfFile(pszComponentId, &ComponentInfo))
        goto done;

    TRACE("Characteristics: 0x%lx\n", ComponentInfo.dwCharacteristics);
    TRACE("ComponentId: %S\n", ComponentInfo.pszComponentId);
    TRACE("Description: %S\n", ComponentInfo.pszDescription);
    TRACE("InfPath: %S\n", ComponentInfo.pszInfPath);
    TRACE("InfSection: %S\n", ComponentInfo.pszInfSection);
    TRACE("ClassGuid: %S\n", ComponentInfo.pszClassGuid);

    if (!CreateInstanceKey(&ComponentInfo,
                           &hInstanceKey))
    {
        ERR("CreateInstanceKey() failed (Error %lx)\n", GetLastError());
        goto done;
    }

    if (!InstallInfSections(NULL,
                            hInstanceKey,
                            ComponentInfo.pszInfPath,
                            ComponentInfo.pszInfSection))
    {
        ERR("InstallInfSections() failed (Error %lx)\n", GetLastError());
        goto done;
    }

    bResult = TRUE;

    StartNetworkServices(&ComponentInfo);

done:
    if (hInstanceKey != NULL)
        RegCloseKey(hInstanceKey);

    if (ComponentInfo.pszFullInfPath)
        HeapFree(GetProcessHeap(), 0, ComponentInfo.pszFullInfPath);

    if (ComponentInfo.pszInfPath)
        HeapFree(GetProcessHeap(), 0, ComponentInfo.pszInfPath);

    if (ComponentInfo.pszInfSection)
        HeapFree(GetProcessHeap(), 0, ComponentInfo.pszInfSection);

    if (ComponentInfo.pszComponentId)
        HeapFree(GetProcessHeap(), 0, ComponentInfo.pszComponentId);

    if (ComponentInfo.pszDescription)
        HeapFree(GetProcessHeap(), 0, ComponentInfo.pszDescription);

    if (ComponentInfo.pszClassGuid)
        HeapFree(GetProcessHeap(), 0, ComponentInfo.pszClassGuid);

    return bResult;
}

/* EOF */
