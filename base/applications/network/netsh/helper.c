/*
 * PROJECT:    ReactOS NetSh
 * LICENSE:    GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:    Network Shell helper dll management and support functions
 * COPYRIGHT:  Copyright 2023 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PDLL_LIST_ENTRY pDllListHead = NULL;
PDLL_LIST_ENTRY pDllListTail = NULL;

PHELPER_ENTRY pHelperListHead = NULL;
PHELPER_ENTRY pHelperListTail = NULL;

PDLL_LIST_ENTRY pCurrentDll = NULL;

/* FUNCTIONS ******************************************************************/

static
VOID
StartHelpers(VOID)
{
    PHELPER_ENTRY pHelper;
    DWORD dwError;

    pHelper = pHelperListHead;
    while (pHelper != NULL)
    {
        if (pHelper->bStarted == FALSE)
        {
            if (pHelper->Attributes.pfnStart)
            {
                dwError = pHelper->Attributes.pfnStart(NULL, 0);
                if (dwError == ERROR_SUCCESS)
                    pHelper->bStarted = TRUE;
            }
        }

        pHelper = pHelper->pNext;
    }
}


static
VOID
RegisterHelperDll(
    _In_ PDLL_LIST_ENTRY pEntry)
{
    PWSTR pszValueName = NULL;
    HKEY hKey;
    DWORD dwError;

    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              REG_NETSH_PATH,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
    if (dwError == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey,
                       pEntry->pszValueName,
                       0,
                       REG_SZ,
                       (PBYTE)pEntry->pszDllName,
                       (wcslen(pEntry->pszDllName) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }

    HeapFree(GetProcessHeap(), 0, pszValueName);
}


static
VOID
FreeHelperDll(
    _In_ PDLL_LIST_ENTRY pEntry)
{
    if (pEntry->hModule)
        FreeLibrary(pEntry->hModule);

    if (pEntry->pszValueName)
        HeapFree(GetProcessHeap(), 0, pEntry->pszValueName);

    if (pEntry->pszShortName)
        HeapFree(GetProcessHeap(), 0, pEntry->pszShortName);

    if (pEntry->pszDllName)
        HeapFree(GetProcessHeap(), 0, pEntry->pszDllName);

    HeapFree(GetProcessHeap(), 0, pEntry);
}


static
DWORD
LoadHelperDll(
    _In_ PWSTR pszDllName,
    _In_ BOOL bRegister)
{
    PNS_DLL_INIT_FN pInitHelperDll;
    PDLL_LIST_ENTRY pEntry;
    PWSTR pszStart, pszEnd;
    BOOL bInserted = FALSE;
    DWORD dwError;

    pEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DLL_LIST_ENTRY));
    if (pEntry == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    pEntry->pszDllName = HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   (wcslen(pszDllName) + 1) * sizeof(WCHAR));
    if (pEntry->pszDllName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    wcscpy(pEntry->pszDllName, pszDllName);

    pszStart = wcsrchr(pszDllName, L'\\');
    if (pszStart == NULL)
        pszStart = pszDllName;

    pEntry->pszShortName = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (wcslen(pszStart) + 1) * sizeof(WCHAR));
    if (pEntry->pszShortName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    wcscpy(pEntry->pszShortName, pszStart);

    pEntry->pszValueName = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (wcslen(pEntry->pszShortName) + 1) * sizeof(WCHAR));
    if (pEntry->pszValueName == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    wcscpy(pEntry->pszValueName, pEntry->pszShortName);

    pszEnd = wcsrchr(pEntry->pszValueName, L'.');
    if (pszEnd != NULL)
        *pszEnd = UNICODE_NULL;

    if (pDllListTail == NULL)
    {
        pEntry->pPrev = NULL;
        pEntry->pNext = NULL;
        pDllListHead = pEntry;
        pDllListTail = pEntry;
    }
    else
    {
        pEntry->pPrev = NULL;
        pEntry->pNext = pDllListHead;
        pDllListHead->pPrev = pEntry;
        pDllListHead = pEntry;
    }

    bInserted = TRUE;

    pEntry->hModule = LoadLibraryW(pEntry->pszDllName);
    if (pEntry->hModule == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not load the helper dll %S (Error: %lu)\n", pEntry->pszDllName, dwError);
        goto done;
    }

    pInitHelperDll = (PNS_DLL_INIT_FN)GetProcAddress(pEntry->hModule, "InitHelperDll");
    if (pInitHelperDll == NULL)
    {
        dwError = GetLastError();
        DPRINT1("Could not find 'InitHelperDll' (Error: %lu)\n", dwError);
        goto done;
    }

    pCurrentDll = pEntry;
    dwError = pInitHelperDll(5, NULL);
    pCurrentDll = NULL;

    DPRINT1("InitHelperDll returned %lu\n", dwError);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("Call to InitHelperDll failed (Error: %lu)\n", dwError);
        goto done;
    }

//    if (pEntry->Attributes.pfnStart)
//        pEntry->Attributes.pfnStart(NULL, 0);

    if (bRegister)
        RegisterHelperDll(pEntry);

done:
    if (dwError != ERROR_SUCCESS)
    {
        if (bInserted)
        {
            if (pEntry->pPrev != NULL)
                pEntry->pPrev->pNext = pEntry->pNext;
            if (pEntry->pNext != NULL)
                pEntry->pNext->pPrev = pEntry->pPrev;
            if (pDllListTail == pEntry)
                pDllListTail = pEntry->pPrev;
            if (pDllListHead == pEntry)
                pDllListHead = pEntry->pNext;
            pEntry->pPrev = NULL;
            pEntry->pNext = NULL;
        }

        FreeHelperDll(pEntry);
    }

    return dwError;
}


VOID
LoadHelpers(VOID)
{
    PWSTR pszNameBuffer = NULL;
    PWSTR pszValueBuffer = NULL;
    HKEY hKey;
    DWORD dwValueCount, dwMaxNameLength, dwMaxValueLength;
    DWORD dwNameLength, dwValueLength, dwType;
    DWORD dwIndex, dwError;

    DPRINT1("LoadHelpers()\n");

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            REG_NETSH_PATH,
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return;

    dwError = RegQueryInfoKeyW(hKey,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               &dwValueCount,
                               &dwMaxNameLength,
                               &dwMaxValueLength,
                               NULL,
                               NULL);
    if (dwError != ERROR_SUCCESS)
        goto done;

    pszNameBuffer = HeapAlloc(GetProcessHeap(), 0,
                              (dwMaxNameLength + 1) * sizeof(WCHAR));
    if (pszNameBuffer == NULL)
        goto done;

    pszValueBuffer = HeapAlloc(GetProcessHeap(), 0,
                               dwMaxValueLength + sizeof(WCHAR));
    if (pszValueBuffer == NULL)
        goto done;

    for (dwIndex = 0; dwIndex < dwValueCount; dwIndex++)
    {
        dwNameLength = dwMaxNameLength + 1;
        dwValueLength = dwMaxValueLength + sizeof(WCHAR);
        dwError = RegEnumValueW(hKey,
                                dwIndex,
                                pszNameBuffer,
                                &dwNameLength,
                                NULL,
                                &dwType,
                                (PBYTE)pszValueBuffer,
                                &dwValueLength);
        if (dwError != ERROR_SUCCESS)
            break;

        DPRINT1("Dll: %S --> %S  %lu\n", pszNameBuffer, pszValueBuffer, dwError);
        LoadHelperDll(pszValueBuffer, FALSE);
    }

done:
    if (pszValueBuffer)
        HeapFree(GetProcessHeap(), 0, pszValueBuffer);

    if (pszNameBuffer)
        HeapFree(GetProcessHeap(), 0, pszNameBuffer);

    RegCloseKey(hKey);

    StartHelpers();
}


VOID
UnloadHelpers(VOID)
{
    PDLL_LIST_ENTRY pEntry;

    while (pDllListHead != NULL)
    {
        pEntry = pDllListHead;
        pDllListHead = pEntry->pNext;

//        if (pEntry->Attributes.pfnStop)
//            pEntry->Attributes.pfnStop(0);

        FreeHelperDll(pEntry);
    }

    pDllListTail = NULL;
}


PHELPER_ENTRY
FindHelper(
    _In_ const GUID *pguidHelper)
{
    PHELPER_ENTRY pHelper;

    pHelper = pHelperListHead;
    while (pHelper != NULL)
    {
        if (IsEqualGUID(pguidHelper, &pHelper->Attributes.guidHelper))
            return pHelper;

        pHelper = pHelper->pNext;
    }

    return NULL;
}


DWORD
WINAPI
RegisterHelper(
    _In_ const GUID *pguidParentHelper,
    _In_ const NS_HELPER_ATTRIBUTES *pHelperAttributes)
{
    PHELPER_ENTRY pHelper = NULL, pParentHelper;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("RegisterHelper(%p %p)\n", pguidParentHelper, pHelperAttributes);

    if (FindHelper(&pHelperAttributes->guidHelper) != NULL)
    {
        DPRINT1("The Helper has already been registered!\n");
        return 1;
    }

    pHelper = (PHELPER_ENTRY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HELPER_ENTRY));
    if (pHelper == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    CopyMemory(&pHelper->Attributes, pHelperAttributes, sizeof(NS_HELPER_ATTRIBUTES));
    pHelper->pDllEntry = pCurrentDll;
    DPRINT("pHelper->pDllEntry: %p\n", pHelper->pDllEntry);

    if (pguidParentHelper == NULL)
    {
        if (pHelperListTail == NULL)
        {
            pHelperListHead = pHelper;
            pHelperListTail = pHelper;
        }
        else
        {
            pHelper->pNext = pHelperListHead;
            pHelperListHead->pPrev = pHelper;
            pHelperListHead = pHelper;
        }
    }
    else
    {
        pParentHelper = FindHelper(&pHelperAttributes->guidHelper);
        if (pParentHelper == NULL)
            return ERROR_INVALID_PARAMETER;

        if (pParentHelper->pSubHelperHead == NULL && pParentHelper->pSubHelperTail == NULL)
        {
            pParentHelper->pSubHelperHead = pHelper;
            pParentHelper->pSubHelperTail = pHelper;
        }
        else
        {
            pHelper->pPrev = pParentHelper->pSubHelperTail;
            pParentHelper->pSubHelperTail->pNext = pHelper;
            pParentHelper->pSubHelperTail = pHelper;
        }
    }

done:

    return dwError;
}


DWORD
WINAPI
AddHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("AddHelperCommand()\n");

    if (dwArgCount == 2)
    {
//        ConResPrintf(StdErr, IDS_INVALID_SYNTAX);
//        ConResPrintf(StdErr, IDS_HLP_ADD_HELPER_EX);
        return 1;
    }

    dwError = LoadHelperDll(ppwcArguments[2], TRUE);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    StartHelpers();

    return ERROR_SUCCESS;
}


DWORD
WINAPI
DeleteHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    PDLL_LIST_ENTRY pEntry;
    HKEY hKey;
    DWORD dwError;

    DPRINT("DeleteHelper()\n");

    if (dwArgCount == 2)
    {
//        ConResPrintf(StdErr, IDS_INVALID_SYNTAX);
//        ConResPrintf(StdErr, IDS_HLP_DEL_HELPER_EX);
        return 1;
    }

    pEntry = pDllListHead;
    while (pEntry != NULL)
    {
        if (wcscmp(pEntry->pszShortName, ppwcArguments[2]) == 0)
        {
            DPRINT1("remove %S\n", pEntry->pszShortName);

            if (pEntry->pPrev != NULL)
                pEntry->pPrev->pNext = pEntry->pNext;
            if (pEntry->pNext != NULL)
                pEntry->pNext->pPrev = pEntry->pPrev;
            if (pDllListTail == pEntry)
                pDllListTail = pEntry->pPrev;
            if (pDllListHead == pEntry)
                pDllListHead = pEntry->pNext;
            pEntry->pPrev = NULL;
            pEntry->pNext = NULL;

            dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                    REG_NETSH_PATH,
                                    0,
                                    KEY_WRITE,
                                    &hKey);
            if (dwError == ERROR_SUCCESS)
            {
                RegDeleteValue(hKey, pEntry->pszValueName);
                RegCloseKey(hKey);
            }

            FreeHelperDll(pEntry);

            return 1;
        }

        pEntry = pEntry->pNext;
    }

    return ERROR_SUCCESS;
}


static
VOID
PrintSubContext(
    _In_ PCONTEXT_ENTRY pParentContext,
    _In_ DWORD dwLevel)
{
    PCONTEXT_ENTRY pContext;
    PHELPER_ENTRY pHelper;
    WCHAR szPrefix[22];
    DWORD i;

    if (pParentContext == NULL)
        return;

    pContext = pParentContext->pSubContextHead;
    while (pContext != NULL)
    {
        pHelper = FindHelper(&pContext->Guid);
        if (pHelper != NULL)
        {
            if (dwLevel > 10)
                dwLevel = 10;

            for (i = 0; i < dwLevel * 2; i++)
                szPrefix[i] = L' ';
            szPrefix[i] = UNICODE_NULL;

            ConPrintf(StdOut, L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}  %-16s  %s%s\n",
                pHelper->Attributes.guidHelper.Data1,
                pHelper->Attributes.guidHelper.Data2,
                pHelper->Attributes.guidHelper.Data3,
                pHelper->Attributes.guidHelper.Data4[0],
                pHelper->Attributes.guidHelper.Data4[1],
                pHelper->Attributes.guidHelper.Data4[2],
                pHelper->Attributes.guidHelper.Data4[3],
                pHelper->Attributes.guidHelper.Data4[4],
                pHelper->Attributes.guidHelper.Data4[5],
                pHelper->Attributes.guidHelper.Data4[6],
                pHelper->Attributes.guidHelper.Data4[7],
                pHelper->pDllEntry->pszShortName,
                szPrefix,
                pContext->pszContextName);
        }

        PrintSubContext(pContext, dwLevel + 1);

        pContext = pContext->pNext;
    }
}


DWORD
WINAPI
ShowHelperCommand(
    LPCWSTR pwszMachine,
    LPWSTR *ppwcArguments,
    DWORD dwCurrentIndex,
    DWORD dwArgCount,
    DWORD dwFlags,
    LPCVOID pvData,
    BOOL *pbDone)
{
    DPRINT("ShowHelperCommand()\n");

    ConPrintf(StdOut, L"Helper GUID                             DLL Name          Command\n");
    ConPrintf(StdOut, L"--------------------------------------  ----------------  --------\n");

    if (pRootContext == NULL)
        return ERROR_SUCCESS;

    PrintSubContext(pRootContext, 0);

    return ERROR_SUCCESS;
}
