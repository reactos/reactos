/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NtCreateSection test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

START_TEST(NtCreateSection)
{
    PVOID Buffer;
    SIZE_T FileSize;
    NTSTATUS Status;
    LARGE_INTEGER MaxFileSize;
    HANDLE Handle, SectionHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING InitOnCreate = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-NtCreateSection\\InitOnCreate");
    UNICODE_STRING InitOnRW = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-NtCreateSection\\InitOnRW");
    UNICODE_STRING InvalidInit = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-NtCreateSection\\InvalidInit");

    KmtLoadDriver(L"NtCreateSection", FALSE);
    KmtOpenDriver();

    /* Test 0 */
    InitializeObjectAttributes(&ObjectAttributes, &InvalidInit, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 512;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_INVALID_FILE_FOR_SECTION);
    if (NT_SUCCESS(Status)) NtClose(SectionHandle);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 1 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnCreate, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 512;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 512);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 2 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnCreate, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 4096;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 4096);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 3 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnRW, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 512;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 512);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 4 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnRW, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 4096;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 4096);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 10 */
    InitializeObjectAttributes(&ObjectAttributes, &InvalidInit, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 512;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_INVALID_FILE_FOR_SECTION);
    if (NT_SUCCESS(Status)) NtClose(SectionHandle);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 11 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnCreate, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 512;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 512);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 12 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnCreate, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 4096;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 4096);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 13 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnRW, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 512;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 512);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Test 14 */
    InitializeObjectAttributes(&ObjectAttributes, &InitOnRW, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaxFileSize.QuadPart = 4096;
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtStartSeh();
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    memset(Buffer, 0xBA, 4096);
    KmtEndSeh(STATUS_SUCCESS);

    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtCloseDriver();
    KmtUnloadDriver();
}
