/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for PrivMoveFileIdentityW
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <ndk/iofuncs.h>
#include <ndk/rtltypes.h>

static const WCHAR FileName[] = L"TestFile.xxx";
static const WCHAR Self[] = L"kernel32_apitest.exe";

static BOOL (WINAPI * pPrivMoveFileIdentityW)(LPCWSTR, LPCWSTR, DWORD);

static
BOOL
QueryFileInfo(
    LPCWSTR File,
    PFILE_BASIC_INFORMATION FileBasicInfo,
    PFILE_STANDARD_INFORMATION FileStandardInfo)
{
    HANDLE hFile;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    hFile = CreateFileW(File, FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    Status = NtQueryInformationFile(hFile, &IoStatusBlock, FileBasicInfo,
                                    sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    Status = NtQueryInformationFile(hFile, &IoStatusBlock, FileStandardInfo,
                                    sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);

    CloseHandle(hFile);
    return NT_SUCCESS(Status);
}

static
VOID
TestPrivMoveFileIdentityW(VOID)
{
    FILE_BASIC_INFORMATION FileBasicInfo;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    LARGE_INTEGER CreationTime, EndOfFile;
    HANDLE hDest;
    NTSTATUS Status;

    DeleteFileW(FileName);

    if (!QueryFileInfo(Self, &FileBasicInfo, &FileStandardInfo))
    {
        win_skip("Failed querying self\n");
        return;
    }

    CreationTime = FileBasicInfo.CreationTime;
    EndOfFile = FileStandardInfo.EndOfFile;

    Sleep(150);

    hDest = CreateFileW(FileName, GENERIC_WRITE | SYNCHRONIZE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL);
    if (hDest == INVALID_HANDLE_VALUE)
    {
        win_skip("Failed creating new\n");
        return;
    }

    CloseHandle(hDest);

    ok(QueryFileInfo(FileName, &FileBasicInfo, &FileStandardInfo) == TRUE, "QueryFileInfo returned FALSE\n");
    ok(FileBasicInfo.CreationTime.QuadPart != CreationTime.QuadPart, "Equal creation times\n");
    ok(FileStandardInfo.EndOfFile.QuadPart == 0LL, "File wasn't created empty: %I64d\n", FileStandardInfo.EndOfFile.QuadPart);
    Status = (NTSTATUS)pPrivMoveFileIdentityW(Self, FileName, 0);
    ok(Status == STATUS_SUCCESS, "PrivMoveFileIdentityW failed with %lx\n", Status);
    ok(QueryFileInfo(FileName, &FileBasicInfo, &FileStandardInfo) == TRUE, "QueryFileInfo returned FALSE\n");
    ok(FileBasicInfo.CreationTime.QuadPart == CreationTime.QuadPart, "Creation time didn't change\n");
    ok(FileStandardInfo.EndOfFile.QuadPart == 0LL, "File not empty anymore: %I64d\n", FileStandardInfo.EndOfFile.QuadPart);
    ok(QueryFileInfo(Self, &FileBasicInfo, &FileStandardInfo) == TRUE, "QueryFileInfo returned FALSE\n");
    ok(FileBasicInfo.CreationTime.QuadPart == CreationTime.QuadPart, "Creation time changed\n");
    ok(FileStandardInfo.EndOfFile.QuadPart == EndOfFile.QuadPart, "File size changed: %I64d\n", FileStandardInfo.EndOfFile.QuadPart);

    DeleteFileW(FileName);
}

START_TEST(PrivMoveFileIdentityW)
{
    HMODULE hKern = GetModuleHandleA("kernel32.dll");
    pPrivMoveFileIdentityW = (void *)GetProcAddress(hKern, "PrivMoveFileIdentityW");

    TestPrivMoveFileIdentityW();
}
