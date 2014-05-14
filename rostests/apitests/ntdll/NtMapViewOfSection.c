
#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/ntndk.h>

BOOL WINAPI SetInformationJobObject(
  _In_  HANDLE hJob,
  _In_  JOBOBJECTINFOCLASS JobObjectInfoClass,
  _In_  LPVOID lpJobObjectInfo,
  _In_  DWORD cbJobObjectInfoLength
);

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
    //ULONG OldProtect;
    QUOTA_LIMITS QuotaLimits;

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

    ok(BaseAddress == UlongToPtr(0x40000000), "Invalid BaseAddress: %p", BaseAddress);

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

    ok(BaseAddress == (PVOID)0x40080000, "Invalid BaseAddress: %p", BaseAddress);

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

    ok(*(PULONG)BaseAddress2 == 2, "Value in memory was wrong");

    /* Close the mapping */
    NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    NtUnmapViewOfSection(NtCurrentProcess(), (PUCHAR)BaseAddress2 - PAGE_SIZE);
    NtClose(SectionHandle);

#if 0
    {
        HANDLE Job;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION JobLimitInformation;

        Job = CreateJobObject(NULL, NULL);
        ok_ntstatus(Status, STATUS_SUCCESS);

        ok(AssignProcessToJobObject(Job, NtCurrentProcess()), "");

        RtlZeroMemory(&JobLimitInformation, sizeof(JobLimitInformation));
        JobLimitInformation.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
        JobLimitInformation.JobMemoryLimit = 100 * 1024;
        JobLimitInformation.PeakProcessMemoryUsed = JobLimitInformation.JobMemoryLimit;
        JobLimitInformation.PeakJobMemoryUsed = JobLimitInformation.JobMemoryLimit;

        ok(SetInformationJobObject(Job,
                                         JobObjectExtendedLimitInformation,
                                         &JobLimitInformation,
                                         sizeof(JobLimitInformation)), "");
    }
#endif

    QuotaLimits.PagedPoolLimit = 0;
    QuotaLimits.NonPagedPoolLimit = 0;
    QuotaLimits.TimeLimit.QuadPart = 0;
    QuotaLimits.PagefileLimit = 0;
    QuotaLimits.MinimumWorkingSetSize = 90000;
    QuotaLimits.MaximumWorkingSetSize = 90000;
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QuotaLimits));
    ok_ntstatus(Status, STATUS_SUCCESS);

    QuotaLimits.MinimumWorkingSetSize = 0;
    QuotaLimits.MaximumWorkingSetSize = 0;
    QuotaLimits.PagefileLimit = 1;
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QuotaLimits));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to create a huge page file backed section */
    MaximumSize.QuadPart = 0x800000000;
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

    /* Create a huge page file backed section, but only reserved */
    MaximumSize.QuadPart = 0x8000000000;
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
    ViewSize = 0x80000000;
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
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_4);

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

    /* Update protection to PAGE_READWRITE */
    ViewSize = 0x20000000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress, 0, &ViewSize, MEM_COMMIT, PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    //Status = NtProtectVirtualMemory(NtCurrentProcess(), &BaseAddress, &ViewSize, PAGE_READWRITE, &OldProtect);
    //ok_ntstatus(Status, STATUS_SUCCESS);
    //ok(OldProtect == PAGE_NOACCESS, "Wrong protection returned: %u\n", OldProtect);

    /* Update protection to PAGE_READWRITE (1 page) */
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &BaseAddress, 0, &ViewSize, MEM_COMMIT, PAGE_READWRITE);
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
    ok(Status == STATUS_SUCCESS, "NtMapViewOfSection failed, Status 0x%lx\n", Status);

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
    ok(Status == STATUS_INVALID_IMAGE_NOT_MZ, "NtCreateSection failed, Status 0x%lx\n", Status);

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
    ok(Status == STATUS_SUCCESS, "NtCreateSection failed, Status 0x%lx\n", Status);

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
    ok(Status == STATUS_SUCCESS, "NtMapViewOfSection failed, Status 0x%lx\n", Status);
#else
    ok(Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH, "NtMapViewOfSection failed, Status 0x%lx\n", Status);
#endif

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
    ok(Status == STATUS_SUCCESS, "NtFlushVirtualMemory failed, Status 0x%lx\n", Status);

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
    ok(Status == STATUS_SUCCESS, "NtCreateSection failed, Status 0x%lx\n", Status);

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
    ok(Status == STATUS_SUCCESS, "NtMapViewOfSection failed, Status 0x%lx\n", Status);
#else
    ok(Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH, "NtMapViewOfSection failed, Status 0x%lx\n", Status);
#endif

    /* Check the .data section again */
    ok(*(ULONG*)((PUCHAR)ImageBase + 0x80000) == 0xf00dada,
       "Data should be synced: 0x%lx!\n", *(ULONG*)((PUCHAR)ImageBase + 0x80000));

    /* Restore the original data */
    *(ULONG*)((PUCHAR)DataBase + 0x800) = 0x12345678;

    /* Close the data mapping */
    NtUnmapViewOfSection(NtCurrentProcess(), DataBase);

    NtClose(DataSectionHandle);

    /* Try to allocate memory inside the image mapping */
    DataBase = (PUCHAR)ImageBase + 0x20000;
    ViewSize = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &DataBase, 0, &ViewSize, MEM_RESERVE, PAGE_NOACCESS);
    ok(Status ==  STATUS_CONFLICTING_ADDRESSES, "Wrong Status: 0x%lx\n", Status);

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
    ok(Status == STATUS_SUCCESS, "NtOpenFile failed, Status 0x%lx\n", Status);
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
    ok(Status == STATUS_SUCCESS, "NtCreateSection failed, Status 0x%lx\n", Status);

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
    ok(Status == STATUS_SUCCESS, "NtMapViewOfSection failed, Status 0x%lx\n", Status);

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

START_TEST(NtMapViewOfSection)
{
    Test_PageFileSection();
    //Test_ImageSection2();
}

