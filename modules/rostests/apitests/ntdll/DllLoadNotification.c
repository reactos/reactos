/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for DLL Load Notification API
 * COPYRIGHT:   Copyright 2024 Ratin Gao <ratin@knsoft.org>
 */

#define UNICODE

#include "precomp.h"

#include <winuser.h>

static WCHAR g_szDllPath[MAX_PATH];
static UNICODE_STRING g_usDllPath;
static UNICODE_STRING g_usDllName;
static volatile LONG g_lDllLoadCount = 0;

typedef
NTSTATUS
NTAPI
FN_LdrRegisterDllNotification(
    _In_ ULONG Flags,
    _In_ PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID Context,
    _Out_ PVOID* Cookie);

typedef
NTSTATUS
NTAPI
FN_LdrUnregisterDllNotification(
    _In_ PVOID Cookie);

static BOOL ExtractResource(
    _In_z_ PCWSTR SavePath,
    _In_ PCWSTR ResourceType,
    _In_ PCWSTR ResourceName)
{
    BOOL bSuccess;
    DWORD dwWritten, dwSize;
    HGLOBAL hGlobal;
    LPVOID pData;
    HANDLE hFile;
    HRSRC hRsrc;

    /* Load resource */
    if ((hRsrc = FindResourceW(NULL, ResourceName, ResourceType)) == NULL ||
        (dwSize = SizeofResource(NULL, hRsrc)) == 0 ||
        (hGlobal = LoadResource(NULL, hRsrc)) == NULL ||
        (pData = LockResource(hGlobal)) == NULL)
    {
        return FALSE;
    }

    /* Save to file */
    hFile = CreateFileW(SavePath,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    bSuccess = WriteFile(hFile, pData, dwSize, &dwWritten, NULL);
    CloseHandle(hFile);
    if (!bSuccess)
    {
        return FALSE;
    }
    else if (dwWritten != dwSize)
    {
        trace("Extract resource failed, written size (%lu) is not actual size (%lu)\n", dwWritten, dwSize);
        DeleteFileW(SavePath);
        SetLastError(ERROR_INCORRECT_SIZE);
        return FALSE;
    }
    return TRUE;
}

static VOID NTAPI DllLoadCallback(
    _In_ ULONG NotificationReason,
    _In_ PCLDR_DLL_NOTIFICATION_DATA NotificationData,
    _In_opt_ PVOID Context)
{
    LONG lRet;
    HMODULE* phNotifiedDllBase = Context;

    /*
     * Verify the data,
     * NotificationData->Loaded and NotificationData->Unloaded currently are the same.
     */

    /* Verify the FullDllName and BaseDllName */
    ok_eq_ulong(NotificationData->Loaded.Flags, 0UL);
    lRet = RtlCompareUnicodeString(NotificationData->Loaded.FullDllName,
                                   (PCUNICODE_STRING)&g_usDllPath,
                                   TRUE);
    ok_eq_long(lRet, 0L);
    lRet = RtlCompareUnicodeString(NotificationData->Loaded.BaseDllName,
                                   (PCUNICODE_STRING)&g_usDllName,
                                   TRUE);
    ok_eq_long(lRet, 0L);

    /*
     * Verify SizeOfImage and read SizeOfImage from PE header,
     * make sure the DLL is not unmapped, the memory is still accessible.
     */
    ok_eq_ulong(NotificationData->Loaded.SizeOfImage,
                RtlImageNtHeader(NotificationData->Loaded.DllBase)->OptionalHeader.SizeOfImage);

    /* Reason can be load or unload */
    ok(NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED ||
       NotificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED, "Incorrect NotificationReason\n");
    if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
    {
        *phNotifiedDllBase = NotificationData->Loaded.DllBase;
        InterlockedIncrement(&g_lDllLoadCount);
    }
    else if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED)
    {
        InterlockedDecrement(&g_lDllLoadCount);
    }
}

START_TEST(DllLoadNotification)
{
    WCHAR szTempPath[MAX_PATH];
    PCWSTR pszDllName;
    HMODULE hNtDll, hTestDll, hNotifiedDllBase;
    FN_LdrRegisterDllNotification* pfnLdrRegisterDllNotification;
    FN_LdrUnregisterDllNotification* pfnLdrUnregisterDllNotification;
    NTSTATUS Status;
    PVOID Cookie1, Cookie2;

    /* Load functions */
    hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (hNtDll == NULL)
    {
        skip("GetModuleHandleW for ntdll failed with 0x%08lX\n", GetLastError());
        return;
    }
    pfnLdrRegisterDllNotification = (FN_LdrRegisterDllNotification*)GetProcAddress(hNtDll, "LdrRegisterDllNotification");
    pfnLdrUnregisterDllNotification = (FN_LdrUnregisterDllNotification*)GetProcAddress(hNtDll, "LdrUnregisterDllNotification");
    if (!pfnLdrRegisterDllNotification || !pfnLdrUnregisterDllNotification)
    {
        skip("ntdll.dll!Ldr[Un]RegisterDllNotification not found\n");
        return;
    }

    /* Extract DLL to temp directory */
    if (!GetTempPathW(ARRAYSIZE(szTempPath), szTempPath))
    {
        skip("GetTempPathW failed with 0x%08lX\n", GetLastError());
        return;
    }
    if (GetTempFileNameW(szTempPath, L"DLN", 0, g_szDllPath) == 0)
    {
        skip("GetTempFileNameW failed with 0x%08lX\n", GetLastError());
        return;
    }
    RtlInitUnicodeString(&g_usDllPath, g_szDllPath);
    pszDllName = wcsrchr(g_szDllPath, L'\\') + 1;
    if (pszDllName == NULL)
    {
        skip("Find file name of %ls failed\n", g_szDllPath);
        return;
    }
    RtlInitUnicodeString(&g_usDllName, pszDllName);
    if (!ExtractResource(g_szDllPath, RT_RCDATA, MAKEINTRESOURCEW(102)))
    {
        skip("ExtractResource failed with 0x%08lX\n", GetLastError());
        return;
    }

    /* Register DLL load notification callback */
    hNotifiedDllBase = NULL;
    Cookie1 = NULL;
    Cookie2 = NULL;
    Status = pfnLdrRegisterDllNotification(0, DllLoadCallback, &hNotifiedDllBase, &Cookie1);
    ok_eq_bool(NT_SUCCESS(Status), TRUE);
    ok(Cookie1 != NULL, "Cookie1 is NULL\n");

    /* Register the callback again is valid */
    Status = pfnLdrRegisterDllNotification(0, DllLoadCallback, &hNotifiedDllBase, &Cookie2);
    ok_eq_bool(NT_SUCCESS(Status), TRUE);
    ok(Cookie2 != NULL, "Cookie2 is NULL\n");

    /* Load the test DLL */
    hTestDll = LoadLibraryW(g_szDllPath);
    if (!hTestDll)
    {
        skip("LoadLibraryW failed with 0x%08lX\n", GetLastError());
        goto _exit;
    }

    /* Verify the Dll base received in callback and returned via context */
    ok_eq_pointer(hNotifiedDllBase, hTestDll);

    /* The count should be 2 because the callback was registered twice */
    ok_eq_long(g_lDllLoadCount, 2L);

    /*
     * Callback will not be triggered because following
     * load and unload actions change the DLL reference count only
     */
    LoadLibraryW(g_szDllPath);
    ok_eq_long(g_lDllLoadCount, 2L);
    FreeLibrary(hTestDll);
    ok_eq_long(g_lDllLoadCount, 2L);

    /* Unregister the callback once */
    Status = pfnLdrUnregisterDllNotification(Cookie1);
    ok_eq_bool(NT_SUCCESS(Status), TRUE);

    /* Unload the test DLL */
    if (FreeLibrary(hTestDll))
    {
        /* The count will decrease 1 because the last callback still there */
        ok_eq_long(g_lDllLoadCount, 1L);
    }
    else
    {
        skip("FreeLibrary failed with 0x%08lX\n", GetLastError());
    }

    /* Unregister the last callback */
    Status = pfnLdrUnregisterDllNotification(Cookie2);
    ok_eq_bool(NT_SUCCESS(Status), TRUE);

_exit:
    DeleteFileW(g_szDllPath);
}
