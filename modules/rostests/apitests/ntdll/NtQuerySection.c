/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtQuerySection
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

START_TEST(NtQuerySection)
{
    NTSTATUS Status;
    HANDLE hSection;
    LARGE_INTEGER MaximumSize;
    SECTION_BASIC_INFORMATION SectionInfo;
    SIZE_T ReturnLength;

    // Create a section with SEC_COMMIT
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL, // ObjectAttributes,
                             &MaximumSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
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

    // Call NtQuerySection with SectionBasicInformation
    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            &SectionInfo,
                            sizeof(SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(ReturnLength == sizeof(SectionInfo), "ReturnLength is %lu, expected %u\n", ReturnLength, sizeof(SectionInfo));
    ok(SectionInfo.BaseAddress == NULL, "BaseAddress is %p, expected NULL\n", SectionInfo.BaseAddress);
    ok(SectionInfo.Attributes == SEC_COMMIT, "Attributes is %lx, expected %x\n", SectionInfo.Attributes, SEC_COMMIT);
    ok(SectionInfo.Size.QuadPart == 0x20000, "Size is %I64x, expected %I64x\n", SectionInfo.Size.QuadPart, 0x20000ull);

    // Close the section handle
    NtClose(hSection);

    // Create a section with SEC_RESERVE
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL, // ObjectAttributes,
                             &MaximumSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_RESERVE,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(hSection != NULL, "hSection is NULL\n");

    // Call NtQuerySection with SectionBasicInformation
    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            &SectionInfo,
                            sizeof(SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(ReturnLength == sizeof(SectionInfo), "ReturnLength is %lu, expected %u\n", ReturnLength, sizeof(SectionInfo));
    ok(SectionInfo.BaseAddress == NULL, "BaseAddress is %p, expected NULL\n", SectionInfo.BaseAddress);
    ok(SectionInfo.Attributes == SEC_RESERVE, "Attributes is %lx, expected %x\n", SectionInfo.Attributes, SEC_RESERVE);
    ok(SectionInfo.Size.QuadPart == 0x20000, "Size is %I64x, expected %I64x\n", SectionInfo.Size.QuadPart, 0x20000ull);

    // Close the section handle
    NtClose(hSection);

    // Create a section with SEC_BASED
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL, // ObjectAttributes,
                             &MaximumSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_BASED | SEC_COMMIT,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(hSection != NULL, "hSection is NULL\n");

    // Call NtQuerySection with SectionBasicInformation
    Status = NtQuerySection(hSection,
                            SectionBasicInformation,
                            &SectionInfo,
                            sizeof(SectionInfo),
                            &ReturnLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(ReturnLength == sizeof(SectionInfo), "ReturnLength is %lu, expected %u\n", ReturnLength, sizeof(SectionInfo));
    ok(SectionInfo.BaseAddress != NULL, "BaseAddress is NULL\n");
    ok(SectionInfo.Attributes == (SEC_BASED | SEC_COMMIT), "Attributes is %lx, expected %x\n", SectionInfo.Attributes, SEC_BASED | SEC_COMMIT);
    ok(SectionInfo.Size.QuadPart == 0x20000, "Size is %I64x, expected %I64x\n", SectionInfo.Size.QuadPart, 0x20000ull);

    // Close the section handle
    NtClose(hSection);
}
