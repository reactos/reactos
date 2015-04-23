
#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/ntndk.h>

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
    PVOID BaseAddress, BaseAddress2;
    SIZE_T ViewSize;
    ULONG OldProtect;

    /* Create a page file backed section with SEC_COMMIT */
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
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
    ok_ntstatus(Status, STATUS_MAPPED_ALIGNMENT);

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
    ok_ntstatus(Status, STATUS_SECTION_PROTECTION);

    /* Try to map 2 pages with MEM_COMMIT */
    BaseAddress = (PVOID)0x30000000;
    SectionOffset.QuadPart = 0;
    ViewSize = 0x2000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                PAGE_SIZE,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                MEM_COMMIT,
                                PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_9);

    /* Map 2 pages, without MEM_COMMIT */
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
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We must be able to access the memory */
    _SEH2_TRY
    {
        *(PULONG)BaseAddress = 1;
    }
    _SEH2_EXCEPT(1)
    {
        ok(FALSE, "Got an exception\n");
    }
    _SEH2_END;

    /* Commit a page in the section */
    BaseAddress = (PVOID)0x30000000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to decommit a page in the section */
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_DECOMMIT);
    ok_ntstatus(Status, STATUS_UNABLE_TO_DELETE_SECTION);

    /* Try to commit a range larger than the section */
    BaseAddress = (PVOID)0x30000000;
    ViewSize = 0x3000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Try to commit a page after the section */
    BaseAddress = (PVOID)0x30002000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Try to allocate a page after the section */
    BaseAddress = (PVOID)0x30002000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Need to go to next 64k boundary */
    BaseAddress = (PVOID)0x30010000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
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

    /* Try to release the section mapping with NtFreeVirtualMemory */
    BaseAddress = (PVOID)0x30000000;
    ViewSize = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE);
    ok_ntstatus(Status, STATUS_UNABLE_TO_DELETE_SECTION);

    /* Commit a page in the section */
    BaseAddress = (PVOID)0x30001000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to decommit the page */
    BaseAddress = (PVOID)0x30001000;
    ViewSize = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_DECOMMIT);
    ok_ntstatus(Status, STATUS_UNABLE_TO_DELETE_SECTION);

    BaseAddress = UlongToPtr(0x40000000);
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
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    ok(BaseAddress == UlongToPtr(0x40000000), "Invalid BaseAddress: %p\n", BaseAddress);

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
    ok_ntstatus(Status, STATUS_SUCCESS);

    ok(BaseAddress == (PVOID)0x40080000, "Invalid BaseAddress: %p\n", BaseAddress);

    /* Commit a page in the section */
    BaseAddress = (PVOID)0x40000000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close the mapping */
    NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    NtClose(SectionHandle);

    /* Create a page file backed section, but only reserved */
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_RESERVE,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to map 1 page, passing MEM_RESERVE */
    BaseAddress = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = PAGE_SIZE;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                PAGE_SIZE,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                MEM_RESERVE,
                                PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_9);

    /* Try to map 1 page using MEM_COMMIT */
    BaseAddress = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = PAGE_SIZE;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                PAGE_SIZE,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                MEM_COMMIT,
                                PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_9);

    /* Map 2 pages, but commit 1 */
    BaseAddress = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = 2 * PAGE_SIZE;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                PAGE_SIZE,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We must be able to access the 1st page */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        *(PUCHAR)BaseAddress = 1;
    }
    _SEH2_EXCEPT(1)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We must not be able to access the 2nd page */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        *((PUCHAR)BaseAddress + PAGE_SIZE) = 1;
    }
    _SEH2_EXCEPT(1)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Map the 2 pages again into a different memory location */
    BaseAddress2 = NULL;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress2,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Commit a the 2nd page in the 2nd memory location */
    BaseAddress2 = (PUCHAR)BaseAddress2 + PAGE_SIZE;
    ViewSize = PAGE_SIZE;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress2,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READONLY);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to commit again (the already committed page) */
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress2,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_READONLY);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* We must be able to access the memory in the 2nd page of the 1st memory location */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        *((PUCHAR)BaseAddress + PAGE_SIZE) = 2;
    }
    _SEH2_EXCEPT(1)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_SUCCESS);

    ok(*(PULONG)BaseAddress2 == 2, "Value in memory was wrong\n");

    /* Close the mapping */
    NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    NtUnmapViewOfSection(NtCurrentProcess(), (PUCHAR)BaseAddress2 - PAGE_SIZE);
    NtClose(SectionHandle);

    /* Try to create a 512 GB page file backed section with committed pages */
    MaximumSize.QuadPart = 0x8000000000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    ok_ntstatus(Status, STATUS_COMMITMENT_LIMIT);

    /* Try to create a huge page file backed section with PAGE_NOACCESS protection */
    MaximumSize.QuadPart = 0x8000000000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_NOACCESS,
                             SEC_COMMIT,
                             NULL);
    ok_ntstatus(Status, STATUS_INVALID_PAGE_PROTECTION);

    /* Try to create a very huge page file backed section, but only reserved */
    MaximumSize.QuadPart = 0x80000000000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_RESERVE,
                             NULL);
#ifdef _WIN64
    ok_ntstatus(Status, STATUS_INSUFFICIENT_RESOURCES);
#else
    /* WoW64 returns STATUS_INSUFFICIENT_RESOURCES */
    ok((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_SECTION_TOO_BIG),
       "got wrong Status: 0x%lx\n", Status);
#endif

    /* Try to create a even huger page file backed section, but only reserved */
    MaximumSize.QuadPart = 0x800000000000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_RESERVE,
                             NULL);
    ok_ntstatus(Status, STATUS_SECTION_TOO_BIG);

    /* Create a 8 GB page file backed section, but only reserved */
    MaximumSize.QuadPart = 0x200000000;
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_RESERVE,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Pass a too large region size */
    BaseAddress = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = MAXULONG_PTR;
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
#ifdef _WIN64
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_3);
#else
    /* WoW64 returns STATUS_INVALID_PARAMETER_4 */
    ok((Status == STATUS_INVALID_PARAMETER_4) || (Status == STATUS_INVALID_PARAMETER_3),
       "got wrong Status: 0x%lx\n", Status);
#endif

    /* Pass 0 region size */
    BaseAddress = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = 0;
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
#ifdef _WIN64
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(ViewSize == 0x200000000, "wrong ViewSize: 0x%Ix\n", ViewSize);
#else
    /* WoW64 returns STATUS_NO_MEMORY */
    ok((Status == STATUS_NO_MEMORY) || (Status == STATUS_INVALID_VIEW_SIZE),
       "got wrong Status: 0x%lx\n", Status);
    ok(ViewSize == 0, "wrong ViewSize: 0x%Ix\n", ViewSize);
#endif

    /* Map with PAGE_NOACCESS */
    BaseAddress = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = 0x20000000;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to change protection to read/write */
    ViewSize = 0x1000;
    OldProtect = -1;
    BaseAddress2 = BaseAddress;
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress2, &ViewSize, PAGE_READWRITE, &OldProtect);
    ok_ntstatus(Status, STATUS_SECTION_PROTECTION);
    // Windows 2003 returns bogus
    //ok(OldProtect == PAGE_READWRITE, "Wrong protection returned: %u\n", OldProtect);

    /* Test read access */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        (void)(*(volatile char*)BaseAddress2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Try to change protection to read/write */
    ViewSize = 0x1000;
    OldProtect = -1;
    BaseAddress2 = BaseAddress;
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress2, &ViewSize, PAGE_READWRITE, &OldProtect);
    ok_ntstatus(Status, STATUS_SECTION_PROTECTION);
#ifdef _WIN64
    ok(OldProtect == 0, "Wrong protection returned: 0x%lx\n", OldProtect);
#else
    // Windows 2003 returns bogus
#endif

    /* Try to change protection to readonly */
    ViewSize = 0x1000;
    OldProtect = -1;
    BaseAddress2 = BaseAddress;
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress2, &ViewSize, PAGE_READONLY, &OldProtect);
    ok_ntstatus(Status, STATUS_SECTION_PROTECTION);
#ifdef _WIN64
    //ok(OldProtect == 0, "Wrong protection returned: 0x%lx\n", OldProtect);
#else
    // Windows 2003 returns bogus
#endif

    /* Commit a page */
    ViewSize = 0x1000;
    BaseAddress2 = BaseAddress;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress2, 0, &ViewSize, MEM_COMMIT, PAGE_READONLY);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress2 == BaseAddress, "Invalid base address: %p\n", BaseAddress2);

    /* Commit the page again */
    ViewSize = 0x1000;
    BaseAddress2 = BaseAddress;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress2, 0, &ViewSize, MEM_COMMIT, PAGE_READONLY);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress2 == BaseAddress, "Invalid base address: %p\n", BaseAddress2);

    /* Test read access */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        (void)(*(volatile char*)BaseAddress2);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Test write access */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        *(char*)BaseAddress2 = 1;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Update protection to PAGE_READWRITE */
    ViewSize = 0x1000;
    BaseAddress2 = BaseAddress;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress2, 0, &ViewSize, MEM_COMMIT, PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Test write access */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        *(char*)BaseAddress2 = 1;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Update protection to PAGE_EXECUTE_READWRITE (1 page) */
    ViewSize = 0x1000;
    BaseAddress2 = BaseAddress;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress2, 0, &ViewSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

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
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to open file\n");
        return;
    }

    /* Create a data section with write access */
    Status = NtCreateSection(&DataSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_COMMIT, // AllocationAttributes
                             FileHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create data section\n");
        NtClose(FileHandle);
        return;
    }

    /* Map the data section as flat mapping */
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
    ok_ntstatus(Status, STATUS_SUCCESS);
    //ok(ViewSize == 0x3f95cc48, "ViewSize wrong: 0x%lx\n");
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to map view of data section\n");
        NtClose(DataSectionHandle);
        NtClose(FileHandle);
        return;
    }

    /* Check the original data */
    ok(*(ULONG*)DataBase == 0x00905a4d, "Header not ok\n");

    /* Modify the PE header (but do not flush!) */
    *(ULONG*)DataBase = 0xdeadbabe;
    ok(*(ULONG*)DataBase == 0xdeadbabe, "Header not ok\n");

    /* Modify data in the .data section (but do not flush!) */
    ok(*(ULONG*)((PUCHAR)DataBase + 0x800) == 0x12345678,
       "Data in .data section invalid: 0x%lx!\n", *(ULONG*)((PUCHAR)DataBase + 0x800));
    *(ULONG*)((PUCHAR)DataBase + 0x800) = 0x87654321;

    /* Now try to create an image section (should fail) */
    Status = NtCreateSection(&ImageSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_IMAGE, // AllocationAttributes
                             FileHandle);
    ok_ntstatus(Status, STATUS_INVALID_IMAGE_NOT_MZ);
    if (NT_SUCCESS(Status)) NtClose(ImageSectionHandle);

    /* Restore the original header */
    *(ULONG*)DataBase = 0x00905a4d;

    /* Modify data in the .data section (but do not flush!) */
    ok_hex(*(ULONG*)((PUCHAR)DataBase + 0x800), 0x87654321);
    *(ULONG*)((PUCHAR)DataBase + 0x800) = 0xdeadbabe;

    /* Try to create an image section again */
    Status = NtCreateSection(&ImageSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_IMAGE, // AllocationAttributes
                             FileHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create image section\n");
        NtClose(DataSectionHandle);
        NtClose(FileHandle);
        return;
    }

    /* Map the image section */
    ImageBase = NULL;
    ViewSize = 0;
    Status = NtMapViewOfSection(ImageSectionHandle,
                                NtCurrentProcess(),
                                &ImageBase,
                                0, // ZeroBits
                                0, // CommitSize
                                NULL, // SectionOffset
                                &ViewSize,
                                ViewShare,
                                0, // AllocationType
                                PAGE_READONLY);
#ifdef _M_IX86
    ok_ntstatus(Status, STATUS_IMAGE_NOT_AT_BASE);
#else
    ok_ntstatus(Status, STATUS_IMAGE_MACHINE_TYPE_MISMATCH);
#endif
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to map view of image section\n");
        NtClose(ImageSectionHandle);
        NtClose(DataSectionHandle);
        NtClose(FileHandle);
        return;
    }

    /* Check the header */
    ok(*(ULONG*)DataBase == 0x00905a4d, "Header not ok\n");
    ok(*(ULONG*)ImageBase == 0x00905a4d, "Header not ok\n");

    /* Check the data section. Either of these can be present! */
    ok((*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0x87654321) ||
       (*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0x12345678),
       "Wrong value in data section: 0x%lx!\n", *(ULONG*)((PUCHAR)ImageBase + 0x80000));

    /* Now modify the data again */
    *(ULONG*)DataBase = 0xdeadbabe;
    *(ULONG*)((PUCHAR)DataBase + 0x800) = 0xf00dada;

    /* Check the data */
    ok(*(ULONG*)DataBase == 0xdeadbabe, "Header not ok\n");
    ok(*(ULONG*)ImageBase == 0x00905a4d, "Data should not be synced!\n");
    ok((*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0x87654321) ||
       (*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0x12345678),
       "Wrong value in data section: 0x%lx!\n", *(ULONG*)((PUCHAR)ImageBase + 0x80000));

    /* Flush the view */
    ViewSize = 0x1000;
    Status = NtFlushVirtualMemory(NtCurrentProcess(),
                                  &DataBase,
                                  &ViewSize,
                                  &IoStatusBlock);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Check the data again */
    ok(*(ULONG*)ImageBase == 0x00905a4d, "Data should not be synced!\n");
    ok((*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0x87654321) ||
       (*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0x12345678),
       "Wrong value in data section: 0x%lx!\n", *(ULONG*)((PUCHAR)ImageBase + 0x80000));

    /* Restore the original header */
    *(ULONG*)DataBase = 0x00905a4d;
    ok(*(ULONG*)DataBase == 0x00905a4d, "Header not ok\n");

    /* Close the image mapping */
    NtUnmapViewOfSection(NtCurrentProcess(), ImageBase);
    NtClose(ImageSectionHandle);

    /* Create an image section again */
    Status = NtCreateSection(&ImageSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             NULL, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_IMAGE, // AllocationAttributes
                             FileHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create image section\n");
        NtClose(DataSectionHandle);
        NtClose(FileHandle);
        return;
    }

    /* Map the image section again */
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
#ifdef _M_IX86
    ok_ntstatus(Status, STATUS_IMAGE_NOT_AT_BASE);
#else
    ok_ntstatus(Status, STATUS_IMAGE_MACHINE_TYPE_MISMATCH);
#endif
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to map view of image section\n");
        NtClose(ImageSectionHandle);
        NtClose(DataSectionHandle);
        NtClose(FileHandle);
        return;
    }

    // This one doesn't always work, needs investigation
    /* Check the .data section again */
    //ok(*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0xf00dada,
    //   "Data should be synced: 0x%lx!\n", *(ULONG*)((PUCHAR)ImageBase + 0x80000));

    /* Restore the original data */
    *(ULONG*)((PUCHAR)DataBase + 0x800) = 0x12345678;

    /* Close the data mapping */
    NtUnmapViewOfSection(NtCurrentProcess(), DataBase);

    NtClose(DataSectionHandle);

    /* Try to allocate memory inside the image mapping */
    DataBase = (PUCHAR)ImageBase + 0x20000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &DataBase, 0, &ViewSize, MEM_RESERVE, PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Cleanup */
    NtClose(FileHandle);
    NtClose(ImageSectionHandle);
    NtUnmapViewOfSection(NtCurrentProcess(), ImageBase);
}

void
Test_ImageSection2(void)
{
    UNICODE_STRING FileName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES FileObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle, ImageSectionHandle;
    PVOID ImageBase, BaseAddress;
    SIZE_T ViewSize;
    LARGE_INTEGER MaximumSize, SectionOffset;

    if (!RtlDosPathNameToNtPathName_U(L"testdata\\nvoglv32.dll",
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
    ok_ntstatus(Status, STATUS_SUCCESS);
    printf("Opened file with handle %p\n", FileHandle);

    /* Create a data section with write access */
    MaximumSize.QuadPart = 0x20000;
    Status = NtCreateSection(&ImageSectionHandle,
                             SECTION_ALL_ACCESS, // DesiredAccess
                             NULL, // ObjectAttributes
                             &MaximumSize, // MaximumSize
                             PAGE_READWRITE, // SectionPageProtection
                             SEC_IMAGE, // AllocationAttributes
                             FileHandle);
    ok_ntstatus(Status, STATUS_SUCCESS);

    printf("Created image section with handle %p\n", ImageSectionHandle);
    //system("PAUSE");

    /* Map the image section */
    ImageBase = NULL;
    ViewSize = 0x0000;
    SectionOffset.QuadPart = 0x00000;
    Status = NtMapViewOfSection(ImageSectionHandle,
                                NtCurrentProcess(),
                                &ImageBase,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    printf("Mapped image section at %p, value in text section: %lx\n",
           ImageBase, *((ULONG*)((PCHAR)ImageBase + 0x1196)));
    system("PAUSE");

    /* Try to allocate a page after the section */
    BaseAddress = (PUCHAR)ImageBase + 0x10000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &ViewSize,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    printf("allocation status: %lx\n", Status);
    system("PAUSE");

}

// doesn't work with WoW64!
void
Test_BasedSection(void)
{
    NTSTATUS Status;
    HANDLE SectionHandle1, SectionHandle2;
    LARGE_INTEGER MaximumSize, SectionOffset;
    PVOID BaseAddress1, BaseAddress2;
    SIZE_T ViewSize;

    /* Create a based section with SEC_COMMIT */
    MaximumSize.QuadPart = 0x1000;
    Status = NtCreateSection(&SectionHandle1,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT | SEC_BASED,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Map the 1st section */
    BaseAddress1 = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = 0;
    Status = NtMapViewOfSection(SectionHandle1,
                                NtCurrentProcess(),
                                &BaseAddress1,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
#if 0 // WOW64?
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
#else
    ok_ntstatus(Status, STATUS_SUCCESS);
#endif

    /* Create a 2nd based section with SEC_COMMIT */
    MaximumSize.QuadPart = 0x1000;
    Status = NtCreateSection(&SectionHandle2,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT | SEC_BASED,
                             NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);//

    /* Map the 2nd section */
    BaseAddress2 = NULL;
    SectionOffset.QuadPart = 0;
    ViewSize = 0;
    Status = NtMapViewOfSection(SectionHandle2,
                                NtCurrentProcess(),
                                &BaseAddress2,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);
#if 0 // WOW64?
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
#else
    ok_ntstatus(Status, STATUS_SUCCESS);
#endif

    ok_ntstatus(Status, STATUS_SUCCESS);
    ok((ULONG_PTR)BaseAddress2 < (ULONG_PTR)BaseAddress1,
       "Invalid addresses: BaseAddress1=%p, BaseAddress2=%p\n", BaseAddress1, BaseAddress2);
    ok(((ULONG_PTR)BaseAddress1 - (ULONG_PTR)BaseAddress2) == 0x10000,
       "Invalid addresses: BaseAddress1=%p, BaseAddress2=%p\n", BaseAddress1, BaseAddress2);

}

START_TEST(NtMapViewOfSection)
{
    Test_PageFileSection();
    Test_ImageSection();
    Test_BasedSection();
}

