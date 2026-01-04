/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtQuerySection
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

static
HANDLE
CreateSection(ULONG Size, ULONG Protection, ULONG Attributes, HANDLE hFile)
{
    NTSTATUS Status;
    HANDLE hSection = NULL;
    LARGE_INTEGER MaximumSize;

    MaximumSize.QuadPart = Size;
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL, // ObjectAttributes
                             &MaximumSize,
                             Protection,
                             Attributes,
                             hFile);
    ok_ntstatus(Status, STATUS_SUCCESS);
    return hSection;
}

static
VOID
QuerySbi(SECTION_BASIC_INFORMATION *SectionInfo, HANDLE hSection)
{
    NTSTATUS Status;
    SIZE_T ReturnLength;

    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            SectionInfo,
                            sizeof(*SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, sizeof(*SectionInfo));
}

void Test_SectionBasicInformation(void)
{
    NTSTATUS Status;
    HANDLE hSection, hFile;
    SECTION_BASIC_INFORMATION SectionInfo;
    SIZE_T ReturnLength;

    // Create a section with SEC_COMMIT
    hSection = CreateSection(0x123, PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);
    ok(hSection != NULL, "hSection is NULL\n");

    // Call NtQuerySection with SectionBasicInformation and a NULL handle
    Status = NtQuerySection(NULL,
                            SectionBasicInformation,
                            &SectionInfo,
                            sizeof(SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_HANDLE);

    // Call NtQuerySection with SectionBasicInformation and a NULL buffer
    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            NULL,
                            sizeof(SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    // Call NtQuerySection with SectionBasicInformation and a too small buffer
    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            &SectionInfo,
                            sizeof(SectionInfo) - 1,
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_INFO_LENGTH_MISMATCH);

    // Call NtQuerySection with SectionBasicInformation and proper parameters
    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            &SectionInfo,
                            sizeof(SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_eq_hex(ReturnLength, sizeof(SectionInfo));
    ok_eq_pointer(SectionInfo.BaseAddress, NULL);
    ok_eq_hex(SectionInfo.Attributes, SEC_COMMIT);
    ok_eq_hex64(SectionInfo.Size.QuadPart, PAGE_SIZE);
    NtClose(hSection);

    // Section with SEC_RESERVE
    hSection = CreateSection(PAGE_SIZE + 1, PAGE_EXECUTE_READWRITE, SEC_RESERVE, NULL);
    ok(hSection != NULL, "hSection is NULL\n");
    QuerySbi(&SectionInfo, hSection);
    ok_eq_pointer(SectionInfo.BaseAddress, NULL);
    ok_eq_hex(SectionInfo.Attributes, SEC_RESERVE);
    ok_eq_hex64(SectionInfo.Size.QuadPart, 2 * PAGE_SIZE);
    NtClose(hSection);

    // Section with SEC_BASED
    hSection = CreateSection(0x20000, PAGE_EXECUTE_READWRITE, SEC_BASED | SEC_COMMIT, NULL);
    ok(hSection != NULL, "hSection is NULL\n");
    QuerySbi(&SectionInfo, hSection);
    ok(SectionInfo.BaseAddress != NULL, "BaseAddress is NULL\n");
    ok_eq_hex(SectionInfo.Attributes, SEC_BASED | SEC_COMMIT);
    ok_eq_hex64(SectionInfo.Size.QuadPart, 0x20000);
    NtClose(hSection);

    // Open this executable file
    CHAR TestExecutableName[MAX_PATH];
    GetModuleFileNameA(NULL, TestExecutableName, sizeof(TestExecutableName));
    hFile = CreateFileA(TestExecutableName,
                        GENERIC_READ | GENERIC_EXECUTE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Failed to open file %s\n", TestExecutableName);

    // Section with SEC_IMAGE and 0 size
    hSection = CreateSection(0, PAGE_READONLY, SEC_IMAGE, hFile);
    ok(hSection != NULL, "hSection is NULL\n");
    QuerySbi(&SectionInfo, hSection);
    ok_eq_pointer(SectionInfo.BaseAddress, NULL);
    ok_eq_hex(SectionInfo.Attributes, SEC_FILE | SEC_IMAGE);
    ok(SectionInfo.Size.QuadPart >= 3 * PAGE_SIZE, "Unexpected size:%I64x\n", SectionInfo.Size.QuadPart);
    NtClose(hSection);

    // Section with SEC_IMAGE and specific size
    hSection = CreateSection(42, PAGE_EXECUTE_READ, SEC_IMAGE, hFile);
    ok(hSection != NULL, "hSection is NULL\n");
    QuerySbi(&SectionInfo, hSection);
    ok_eq_pointer(SectionInfo.BaseAddress, NULL);
    ok_eq_hex(SectionInfo.Attributes, SEC_FILE | SEC_IMAGE);
    ok_eq_hex64(SectionInfo.Size.QuadPart, 42);
    NtClose(hSection);

    // File backed section with SEC_RESERVE (ignored)
    hSection = CreateSection(1, PAGE_EXECUTE_READ, SEC_RESERVE, hFile);
    ok(hSection != NULL, "hSection is NULL\n");
    QuerySbi(&SectionInfo, hSection);
    ok_eq_pointer(SectionInfo.BaseAddress, NULL);
    ok_eq_hex(SectionInfo.Attributes, SEC_FILE);
    ok_eq_hex64(SectionInfo.Size.QuadPart, 1);
    NtClose(hSection);

    // Close the file handle
    NtClose(hFile);
}

START_TEST(NtQuerySection)
{
    Test_SectionBasicInformation();
}
