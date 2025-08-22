/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for GetFinalPathNameByHandleW
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <lm.h>
#include <ndk/obfuncs.h>
#include <ndk/iofuncs.h>

typedef
DWORD
WINAPI
FN_GetFinalPathNameByHandleW(
    _In_ HANDLE hFile,
    _Out_writes_(cchFilePath) LPWSTR lpszFilePath,
    _In_ DWORD cchFilePath,
    _In_ DWORD dwFlags);
FN_GetFinalPathNameByHandleW* pGetFinalPathNameByHandleW = NULL;

#define VOLUME_NAME_DOS 0x0
#define VOLUME_NAME_GUID 0x1
#define VOLUME_NAME_NT 0x2
#define VOLUME_NAME_NONE 0x4
#define FILE_NAME_NORMALIZED 0x0
#define FILE_NAME_OPENED 0x8

static void Test_File(void)
{
    WCHAR FilePath[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    WCHAR VolumeBuffer[4];
    PWSTR VolumeRelativePath;
    WCHAR ExpectedString[MAX_PATH];
    DWORD Result;
    SIZE_T ExpectedStringLength;
    HANDLE hFile;
    BOOL Success;

    /* Get the full path name of the current executable */
    Result = GetModuleFileNameW(NULL, FilePath, ARRAYSIZE(FilePath));
    ok(Result != 0, "GetModuleFileNameW failed: %ld\n", GetLastError());
    if (Result == 0)
    {
        skip("GetModuleFileNameW failed\n");
        return;
    }

    /* Open the executable file */
    hFile = CreateFileW(FilePath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "File '%S': Opening failed\n", FilePath);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        skip("File '%S': Opening failed\n", FilePath);
        return;
    }

    /* Expected string for VOLUME_NAME_DOS:
       L"\\\\?\\C:\\ReactOS\\bin\\kernel32_apitest.exe" */
    wcscpy(ExpectedString, L"\\\\?\\");
    wcscat(ExpectedString, FilePath);
    ExpectedStringLength = wcslen(ExpectedString);

    SetLastError(0xdeadbeef);
    StartSeh()
        Result = pGetFinalPathNameByHandleW(hFile, NULL, ARRAYSIZE(Buffer), VOLUME_NAME_DOS);
    EndSeh(STATUS_ACCESS_VIOLATION)

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, 0, VOLUME_NAME_DOS);
    ok_eq_ulong(Result, ExpectedStringLength + 1);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_NOT_ENOUGH_MEMORY);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, 9, VOLUME_NAME_DOS);
    ok_eq_ulong(Result, ExpectedStringLength + 1);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_NOT_ENOUGH_MEMORY);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, NULL, 0, VOLUME_NAME_DOS);
    ok_eq_ulong(Result, ExpectedStringLength + 1);
    ok_err(ERROR_NOT_ENOUGH_MEMORY);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), 0x1000);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), 7);
    ok_eq_ulong(Result, 0ul);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_DOS);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_DOS | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0);

    /* Query the GUID based volume name */
    memcpy(VolumeBuffer, FilePath, 3 * sizeof(WCHAR));
    VolumeBuffer[3] = UNICODE_NULL;
    Success = GetVolumeNameForVolumeMountPointW(VolumeBuffer,
                                                ExpectedString,
                                                ARRAYSIZE(ExpectedString));
    if (!Success)
    {
        skip("GetVolumeNameForVolumeMountPointW failed: %ld\n", GetLastError());
        return;
    }

    /* Expected string for VOLUME_NAME_GUID:
       L"\\\\?\\Volume{cd4317d4-A62f-53d7-b36c-73f935c37280}\\ReactOS\\bin\\kernel32_apitest.exe" */
    VolumeRelativePath = &FilePath[wcslen(L"C:\\")];
    wcscat(ExpectedString, VolumeRelativePath);
    ExpectedStringLength = wcslen(ExpectedString);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_GUID);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_GUID | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0);

    /* Expected string for VOLUME_NAME_NT (2):
       L"\\Device\\HarddiskVolume1\\ReactOS\\bin\\kernel32_apitest.exe" */
    ExpectedStringLength = wcslen(L"\\Device\\HarddiskVolume*\\") +
                           wcslen(VolumeRelativePath);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_int(wcsncmp(Buffer, L"\\Device\\HarddiskVolume", 22), 0);
    ok_int(wcscmp(Buffer + 24, VolumeRelativePath), 0);
    ok_err(0xdeadbeef);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_int(wcsncmp(Buffer, L"\\Device\\HarddiskVolume", 22), 0);
    ok_int(wcscmp(Buffer + 24, VolumeRelativePath), 0);
    ok_err(0xdeadbeef);

    /* Expected string for VOLUME_NAME_NONE:
       L"\\ReactOS\\bin\\kernel32_apitest.exe" */
    ExpectedStringLength = wcslen(VolumeRelativePath - 1);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NONE);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, VolumeRelativePath - 1);
    ok_err(0xdeadbeef);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NONE | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, VolumeRelativePath - 1);
    ok_err(0xdeadbeef);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, 1, VOLUME_NAME_NONE | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength + 1);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_NOT_ENOUGH_MEMORY);

    CloseHandle(hFile);
}

static void Test_NetworkShare(void)
{
    WCHAR PathBuffer[MAX_PATH];
    WCHAR RemotePathBuffer[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    WCHAR ExpectedString[MAX_PATH];
    PWSTR FileName;
    DWORD Result;
    SIZE_T ExpectedStringLength;
    DWORD dwError = 0;
    HANDLE hFile;
    NET_API_STATUS Status;

    /* Get the full path name of the current executable */
    Result = GetModuleFileNameW(NULL, PathBuffer, ARRAYSIZE(PathBuffer));
    ok(Result != 0, "GetModuleFileNameW failed: %ld\n", GetLastError());
    if (Result == 0)
    {
        skip("GetModuleFileNameW failed: %ld\n", GetLastError());
        return;
    }

    /* Reduce to the containing folder */
    FileName = wcsrchr(PathBuffer, L'\\');
    *FileName = L'\0';
    FileName++;

    // Define the share parameters
    static WCHAR ShareName[] = L"TestShare"; // Name of the share

    NetShareDel(L"", ShareName, 0);

    // Set up the SHARE_INFO_2 structure
    SHARE_INFO_2 shareInfo = { 0 };
    shareInfo.shi2_netname = ShareName;
    shareInfo.shi2_type = STYPE_DISKTREE; // Disk directory share
    shareInfo.shi2_remark = L"";
    shareInfo.shi2_permissions = ACCESS_ALL; // Full access (adjust as needed)
    shareInfo.shi2_max_uses = -1; // Unlimited connections
    shareInfo.shi2_current_uses = 0; // 0 for new share
    shareInfo.shi2_path = PathBuffer;
    shareInfo.shi2_passwd = NULL; // No password

    // Call NetShareAdd to create the share
    Status = NetShareAdd(L"", // Empty string for local machine
                         2,             // Level 2 for SHARE_INFO_2
                         (LPBYTE)&shareInfo, // Share info structure
                         &dwError);       // Error code if applicable
    if (Status != NERR_Success)
    {
        skip("Failed to create share. Error code: %ld\n", Status);
        if (Status == ERROR_ACCESS_DENIED)
        {
            wprintf(L"Error: Access denied. Ensure you have administrative privileges.\n");
        }

        return;
    }

    swprintf(RemotePathBuffer, L"\\\\localhost\\%s\\%s", ShareName, FileName);
    hFile = CreateFileW(RemotePathBuffer,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Failed to open file '%S'. Error: %ld\n",
       RemotePathBuffer, GetLastError());
    if (hFile == INVALID_HANDLE_VALUE)
    {
        skip("Failed to open file '%S'. Error: %ld\n", RemotePathBuffer, GetLastError());
        /* Clean up by deleting the share */
        NetShareDel(L"", ShareName, 0);
        return;
    }

    /* Expected string for VOLUME_NAME_DOS:
       L"\\\\?\\UNC\\localhost\\TestShare\\kernel32_apitest.exe" */
    swprintf(ExpectedString, L"\\\\?\\UNC\\localhost\\%s\\%s", ShareName, FileName);
    ExpectedStringLength = wcslen(ExpectedString);
    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_DOS);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0xdeadbeef);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_DOS | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0xdeadbeef);

    // VOLUME_NAME_GUID doesn't work for network shares
    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_GUID);
    ok_eq_ulong(Result, 0ul);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_PATH_NOT_FOUND);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_GUID | FILE_NAME_OPENED);
    ok_eq_ulong(Result, 0ul);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_PATH_NOT_FOUND);

    /* Expected string for VOLUME_NAME_NT (2):
       L"\\Device\\Mup\\localhost\\TestShare\\kernel32_apitest.exe" */
    swprintf(ExpectedString, L"\\Device\\Mup\\localhost\\%s\\%s", ShareName, FileName);
    ExpectedStringLength = wcslen(ExpectedString);
    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0xdeadbeef);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0xdeadbeef);

    /* Expected string for VOLUME_NAME_NONE:
       L"\\localhost\\TestShare\\kernel32_apitest.exe" */
    swprintf(ExpectedString, L"\\localhost\\%s\\%s", ShareName,  FileName);
    ExpectedStringLength = wcslen(ExpectedString);
    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NONE);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0xdeadbeef);

    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(hFile, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NONE | FILE_NAME_OPENED);
    ok_eq_ulong(Result, ExpectedStringLength);
    ok_eq_wstr(Buffer, ExpectedString);
    ok_err(0xdeadbeef);

    /* Clean up */
    CloseHandle(hFile);
    Status = NetShareDel(L"", ShareName, 0);
    ok_ntstatus(Status, NERR_Success);
}

static void Test_Other(void)
{
    WCHAR Buffer[MAX_PATH];
    DWORD Result;
    HANDLE Handle;
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Test NULL handle */
    SetLastError(0xdeadbeef);
    Result = pGetFinalPathNameByHandleW(NULL, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_DOS);
    ok_eq_ulong(Result, 0ul);
    ok_err(ERROR_INVALID_HANDLE);

    /* Test NULL handle and NULL buffer */
    SetLastError(0xdeadbeef);
    Result = pGetFinalPathNameByHandleW(NULL, NULL, ARRAYSIZE(Buffer), VOLUME_NAME_DOS);
    ok_eq_ulong(Result, 0ul);
    ok_err(ERROR_INVALID_HANDLE);

    /* Test NULL handle and invalid volume type */
    SetLastError(0xdeadbeef);
    Result = pGetFinalPathNameByHandleW(NULL, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT | VOLUME_NAME_GUID);
    ok_eq_ulong(Result, 0ul);
    ok_err(ERROR_INVALID_PARAMETER);

    /* Test INVALID_HANDLE_VALUE */
    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(INVALID_HANDLE_VALUE, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_DOS);
    ok_eq_ulong(Result, 0ul);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_INVALID_HANDLE);

    /* Test NtCurrentProcess */
    SetLastError(0xdeadbeef);
    memset(Buffer, 0xCC, sizeof(Buffer));
    Result = pGetFinalPathNameByHandleW(NtCurrentProcess(), Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT);
    ok_eq_ulong(Result, 0ul);
    ok_eq_wchar(Buffer[0], 0xCCCC);
    ok_err(ERROR_INVALID_HANDLE);

    /* Open a handle to the Beep device */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Beep");
    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_READ_ATTRIBUTES, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Opening Beep device failed\n");
    }
    else
    {
        SetLastError(0xdeadbeef);
        memset(Buffer, 0xCC, sizeof(Buffer));
        Result = pGetFinalPathNameByHandleW(Handle, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT);
        ok_eq_ulong(Result, 0ul);
        ok_eq_wchar(Buffer[0], 0xCCCC);
        ok_err(ERROR_INVALID_FUNCTION);
        NtClose(Handle);
    }

    /* Open a handle to the Null device */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Null");
    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&Handle, FILE_READ_ATTRIBUTES, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Opening Null device failed\n");
    }
    else
    {
        SetLastError(0xdeadbeef);
        memset(Buffer, 0xCC, sizeof(Buffer));
        Result = pGetFinalPathNameByHandleW(Handle, Buffer, ARRAYSIZE(Buffer), VOLUME_NAME_NT);
        ok_eq_ulong(Result, 0ul);
        ok_eq_wchar(Buffer[0], 0xCCCC);
        ok_err(ERROR_INVALID_PARAMETER);
        NtClose(Handle);
    }
}


START_TEST(GetFinalPathNameByHandle)
{
    HMODULE hmodKernel32 = GetModuleHandleW(L"kernel32.dll");
    pGetFinalPathNameByHandleW = (FN_GetFinalPathNameByHandleW*)
        GetProcAddress(hmodKernel32, "GetFinalPathNameByHandleW");
    if (pGetFinalPathNameByHandleW == NULL)
    {
        hmodKernel32 = GetModuleHandleW(L"kernel32_vista.dll");
        pGetFinalPathNameByHandleW = (FN_GetFinalPathNameByHandleW*)
            GetProcAddress(hmodKernel32, "GetFinalPathNameByHandleW");
        if (pGetFinalPathNameByHandleW == NULL)
        {
            skip("GetFinalPathNameByHandleW not available\n");
            return;
        }
    }

    Test_File();
    Test_NetworkShare();
    Test_Other();
}
