
#define WIN32_NO_STATUS
#include <wine/test.h>
#include <ndk/ntndk.h>
#include <pseh/pseh2.h>

NTSYSAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    HANDLE SectionHandle,
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	ULONG_PTR ZeroBits,
	SIZE_T CommitSize,
	PLARGE_INTEGER SectionOffset,
	PSIZE_T ViewSize,
	SECTION_INHERIT InheritDisposition,
	ULONG AllocationType,
	ULONG Protect);

void
Test_PageFileSection(void)
{
    NTSTATUS Status;
    HANDLE SectionHandle;
    LARGE_INTEGER MaximumSize, SectionOffset;
    PVOID BaseAddress;
    SIZE_T ViewSize;

    /* Create a page file backed section */
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    ok(NT_SUCCESS(Status), "NtCreateSection failed with Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
        return;

    /* Try to map a page at an address that is not 64k aligned */
    BaseAddress = (PVOID)0x30001000;
    SectionOffset.QuadPart = 0;
    ViewSize = 0x1000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok(Status == STATUS_MAPPED_ALIGNMENT,
       "NtMapViewOfSection returned wrong Status %lx\n", Status);

    /* Try to map a page with execute rights */
    BaseAddress = (PVOID)0x30000000;
    SectionOffset.QuadPart = 0;
    ViewSize = 0x1000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_EXECUTE_READWRITE);
    ok(Status == STATUS_SECTION_PROTECTION,
       "NtMapViewOfSection returned wrong Status %lx\n", Status);

    /* Map 2 pages, not comitting them */
    BaseAddress = (PVOID)0x30000000;
    SectionOffset.QuadPart = 0;
    ViewSize = 0x2000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtMapViewOfSection failed with Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
        return;

    /* Commit a page in the section */
    BaseAddress = (PVOID)0x30000000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed with Status %lx\n", Status);

    /* Try to commit a range larger than the section */
    BaseAddress = (PVOID)0x30000000;
    ViewSize = 0x3000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(Status == STATUS_CONFLICTING_ADDRESSES,
       "NtAllocateVirtualMemory failed with wrong Status %lx\n", Status);

    /* Try to commit a page after the section */
    BaseAddress = (PVOID)0x30002000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(!NT_SUCCESS(Status), "NtAllocateVirtualMemory Should fail\n");

    /* Try to allocate a page after the section */
    BaseAddress = (PVOID)0x30002000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(!NT_SUCCESS(Status), "NtAllocateVirtualMemory should fail\n");

    /* Need to go to next 64k boundary */
    BaseAddress = (PVOID)0x30010000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed with Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
        return;

    /* Free the allocation */
    BaseAddress = (PVOID)0x30010000;
    ViewSize = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed with Status %lx\n", Status);

    /* Free the section mapping */
    BaseAddress = (PVOID)0x30000000;
    ViewSize = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE);
    ok(Status == STATUS_UNABLE_TO_DELETE_SECTION,
       "NtFreeVirtualMemory failed with wrong Status %lx\n", Status);

    /* Commit a page in the section */
    BaseAddress = (PVOID)0x30001000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed with Status %lx\n", Status);

    /* Try to decommit the page */
    BaseAddress = (PVOID)0x30001000;
    ViewSize = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_DECOMMIT);
    ok(Status == STATUS_UNABLE_TO_DELETE_SECTION,
       "NtFreeVirtualMemory failed with wrong Status %lx\n", Status);

    BaseAddress = (PVOID)0x40000000;
    SectionOffset.QuadPart = 0;
    ViewSize = 0x1000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtMapViewOfSection failed with Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
        return;

    ok(BaseAddress == (PVOID)0x40000000, "Invalid BaseAddress: %p", BaseAddress);

    BaseAddress = (PVOID)0x40080000;
    SectionOffset.QuadPart = 0x10000;
    ViewSize = 0x1000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtMapViewOfSection failed with Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
        return;

    ok(BaseAddress == (PVOID)0x40080000, "Invalid BaseAddress: %p", BaseAddress);

    /* Commit a page in the section */
    BaseAddress = (PVOID)0x40000000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed with Status %lx\n", Status);
    if (!NT_SUCCESS(Status))
        return;

}

void
Test_ImageSection(void)
{
    UNICODE_STRING FileName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES FileObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle, DataSectionHandle, ImageSectionHandle;
    PVOID DataBase, ImageBase;
    SIZE_T ViewSize;

    if (!RtlDosPathNameToNtPathName_U(L"testdata\\test.dll",
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        ok(0, "RtlDosPathNameToNtPathName_U failed\n");
        return;
    }

    InitializeObjectAttributes(&FileObjectAttributes,
                               &FileName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
                        &FileObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    ok(Status == STATUS_SUCCESS, "NtOpenFile failed, Status 0x%lx\n", Status);

    /* Create a data section with write access */
    Status = NtCreateSection(&DataSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_COMMIT, // AllocationAttributes
                             FileHandle);
    ok(Status == STATUS_SUCCESS, "NtCreateSection failed, Status 0x%lx\n", Status);

    /* Map the data section */
    DataBase = NULL;
    ViewSize = 0;
    Status = NtMapViewOfSection(DataSectionHandle,
                                NtCurrentProcess(),
                                &DataBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok(Status == STATUS_SUCCESS, "NtMapViewOfSection failed, Status 0x%lx\n", Status);

    /* Check the original data */
    ok(*(ULONG*)DataBase == 0x00905a4d, "Header not ok\n");

    /* Now modify the data in the data section */
    *(ULONG*)DataBase = 0xdeadbabe;

    /* Check the data */
    ok(*(ULONG*)DataBase == 0xdeadbabe, "Header not ok\n");

    /* Now try to create an image section */
    Status = NtCreateSection(&ImageSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_IMAGE, // AllocationAttributes
                             FileHandle);
    ok(Status == STATUS_INVALID_IMAGE_NOT_MZ, "NtCreateSection failed, Status 0x%lx\n", Status);

    /* Restore the original data */
    *(ULONG*)DataBase = 0x00905a4d;

    /* Try to create an image section again */
    Status = NtCreateSection(&ImageSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_IMAGE, // AllocationAttributes
                             FileHandle);
    ok(Status == STATUS_SUCCESS, "NtCreateSection failed, Status 0x%lx\n", Status);

    /* Map the image section */
    ImageBase = NULL;
    ViewSize = 0;
    Status = NtMapViewOfSection(ImageSectionHandle,
                                NtCurrentProcess(),
                                &ImageBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READONLY);
    ok(Status == STATUS_SUCCESS, "NtMapViewOfSection failed, Status 0x%lx\n", Status);

    /* Check the data */
    ok(*(ULONG*)DataBase == 0x00905a4d, "Header not ok\n");
    ok(*(ULONG*)ImageBase == 0x00905a4d, "Header not ok\n");

    /* Now modify the data again */
    *(ULONG*)DataBase = 0xdeadbabe;

    /* Check the data */
    ok(*(ULONG*)DataBase == 0xdeadbabe, "Header not ok\n");
    ok(*(ULONG*)ImageBase == 0x00905a4d, "Data should not be synced!\n");

    /* Flush the view */
    ViewSize = 0x1000;
    Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                  &DataBase,
                                  &ViewSize,
                                  &IoStatusBlock);
    ok(Status == STATUS_SUCCESS, "NtFlushVirtualMemory failed, Status 0x%lx\n", Status);

    /* Check the data again */
    ok(*(ULONG*)ImageBase == 0x00905a4d, "Data should not be synced!\n");

    /* Restore the original data */
    *(ULONG*)DataBase = 0x00905a4d;
    ok(*(ULONG*)DataBase == 0x00905a4d, "Header not ok\n");

    /* Cleanup */
    NtUnmapViewOfSection(NtCurrentProcess(), ImageBase);
    NtUnmapViewOfSection(NtCurrentProcess(), DataBase);
    NtClose(ImageSectionHandle);
    NtClose(DataSectionHandle);
    NtClose(FileHandle);
}

START_TEST(NtMapViewOfSection)
{
    //Test_PageFileSection();
    Test_ImageSection();
}
