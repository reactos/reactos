/*
* PROJECT:         ReactOS kernel-mode tests
* LICENSE:         GPLv2+ - See COPYING in the top level directory
* PURPOSE:         Kernel-Mode Test Suite ZwMapViewOfSection
* PROGRAMMER:      Nikolay Borisov <nib9@aber.ac.uk>
*/

#include <kmt_test.h>

#define IGNORE -99
#define NEW_CONTENT "NewContent"
#define NEW_CONTENT_LEN sizeof(NEW_CONTENT)

static UNICODE_STRING FileReadOnlyPath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\ntdll.dll");
static UNICODE_STRING NtosImgPath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\ntoskrnl.exe");
static UNICODE_STRING WritableFilePath = RTL_CONSTANT_STRING(L"\\SystemRoot\\kmtest-MmSection.txt");
static UNICODE_STRING SharedSectionName = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\kmtest-SharedSection");
extern const char TestString[];
extern const ULONG TestStringSize;
static OBJECT_ATTRIBUTES NtdllObject;
static OBJECT_ATTRIBUTES KmtestFileObject;
static OBJECT_ATTRIBUTES NtoskrnlFileObject;

#define TestMapView(SectionHandle, ProcessHandle, BaseAddress2, ZeroBits, CommitSize, SectionOffset, ViewSize2, InheritDisposition, AllocationType, Win32Protect, MapStatus, UnmapStatus) do    \
    {                                                                                                                                                                                           \
        Status = ZwMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress2, ZeroBits, CommitSize, SectionOffset, ViewSize2, InheritDisposition, AllocationType, Win32Protect);              \
        ok_eq_hex(Status, MapStatus);                                               \
        if (NT_SUCCESS(Status))                                                     \
        {                                                                           \
            Status = ZwUnmapViewOfSection(ProcessHandle, BaseAddress);              \
            if (UnmapStatus != IGNORE) ok_eq_hex(Status, UnmapStatus);              \
            *BaseAddress2 = NULL;                                                   \
            *ViewSize2 = 0;                                                         \
        }                                                                           \
    } while (0)                                                                     \

#define MmTestMapView(Object, ProcessHandle, BaseAddress2, ZeroBits, CommitSize, SectionOffset, ViewSize2, InheritDisposition, AllocationType, Win32Protect, MapStatus, UnmapStatus) do    \
    {                                                                                                                                                                                      \
        Status = MmMapViewOfSection(Object, ProcessHandle, BaseAddress2, ZeroBits, CommitSize, SectionOffset, ViewSize2, InheritDisposition, AllocationType, Win32Protect);                \
        ok_eq_hex(Status, MapStatus);                                               \
        if (NT_SUCCESS(Status))                                                     \
        {                                                                           \
            Status = MmUnmapViewOfSection(ProcessHandle, BaseAddress);              \
            if (UnmapStatus != IGNORE) ok_eq_hex(Status, UnmapStatus);              \
            *BaseAddress2 = NULL;                                                   \
            *ViewSize2 = 0;                                                         \
        }                                                                           \
    } while (0)                                                                     \

#define CheckObject(Handle, Pointers, Handles) do                   \
{                                                                   \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                     \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,          \
    &ObjectInfo, sizeof ObjectInfo, NULL);                          \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    ok_eq_ulong(ObjectInfo.PointerCount, Pointers);                 \
    ok_eq_ulong(ObjectInfo.HandleCount, Handles);                   \
} while (0)                                                         \

static
VOID
KmtInitTestFiles(PHANDLE ReadOnlyFile, PHANDLE WriteOnlyFile, PHANDLE ExecutableFile)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    //INIT THE READ-ONLY FILE
    Status = ZwCreateFile(ReadOnlyFile, GENERIC_READ, &NtdllObject, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(*ReadOnlyFile != NULL, "Couldn't acquire READONLY handle\n");

    //INIT THE EXECUTABLE FILE
    Status = ZwCreateFile(ExecutableFile, GENERIC_EXECUTE, &NtdllObject, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(*ExecutableFile != NULL, "Couldn't acquire EXECUTE handle\n");

    //INIT THE WRITE-ONLY FILE
    //TODO: Delete the file when the tests are all executed
    Status = ZwCreateFile(WriteOnlyFile, (GENERIC_WRITE | SYNCHRONIZE), &KmtestFileObject, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE, FILE_SUPERSEDE, (FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE), NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulongptr(IoStatusBlock.Information, FILE_CREATED);
    ok(*WriteOnlyFile != NULL, "WriteOnlyFile is NULL\n");
    if (!skip(*WriteOnlyFile != NULL, "No WriteOnlyFile\n"))
    {
        FileOffset.QuadPart = 0;
        Status = ZwWriteFile(*WriteOnlyFile, NULL, NULL, NULL, &IoStatusBlock, (PVOID)TestString, TestStringSize, &FileOffset, NULL);
        ok(Status == STATUS_SUCCESS || Status == STATUS_PENDING, "Status = 0x%08lx\n", Status);
        Status = ZwWaitForSingleObject(*WriteOnlyFile, FALSE, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_ulongptr(IoStatusBlock.Information, TestStringSize);
    }
}

static
VOID
SimpleErrorChecks(HANDLE FileHandleReadOnly, HANDLE FileHandleWriteOnly, HANDLE ExecutableImg)
{
    NTSTATUS Status;
    HANDLE WriteSectionHandle;
    HANDLE ReadOnlySection;
    HANDLE PageFileSectionHandle;
    LARGE_INTEGER MaximumSize;
    LARGE_INTEGER SectionOffset;
    SIZE_T AllocSize = TestStringSize;
    SIZE_T ViewSize = 0;
    PVOID BaseAddress = NULL;
    PVOID AllocBase = NULL;
    MaximumSize.QuadPart = TestStringSize;

    //Used for parameters working on file-based section
    Status = ZwCreateSection(&WriteSectionHandle, SECTION_ALL_ACCESS, NULL, &MaximumSize, PAGE_READWRITE, SEC_COMMIT, FileHandleWriteOnly);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = ZwCreateSection(&ReadOnlySection, SECTION_ALL_ACCESS, NULL, &MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleReadOnly);
    ok_eq_hex(Status, STATUS_SUCCESS);

    //Used for parameters taking effect only on page-file backed section
    MaximumSize.QuadPart = 5 * MM_ALLOCATION_GRANULARITY;
    Status = ZwCreateSection(&PageFileSectionHandle, SECTION_ALL_ACCESS, NULL, &MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    MaximumSize.QuadPart = TestStringSize;

    //section handle
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView((HANDLE)(ULONG_PTR)0xDEADBEEFDEADBEEFull, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_HANDLE, IGNORE);
    TestMapView(INVALID_HANDLE_VALUE, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_OBJECT_TYPE_MISMATCH, IGNORE);
    TestMapView(NULL, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_HANDLE, IGNORE);

    //process handle
    TestMapView(WriteSectionHandle, (HANDLE)(ULONG_PTR)0xDEADBEEFDEADBEEFull, &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_HANDLE, IGNORE);
    TestMapView(WriteSectionHandle, (HANDLE)NULL, &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_HANDLE, IGNORE);

    //base address
    BaseAddress = (PVOID)(ULONG_PTR)0x00567A20;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_MAPPED_ALIGNMENT, IGNORE);

    BaseAddress = (PVOID)(ULONG_PTR)0x60000000;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    BaseAddress = (PVOID)((char *)MmSystemRangeStart + 200);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_3, IGNORE);

    //invalid section handle AND unaligned base address
    BaseAddress = (PVOID)(ULONG_PTR)0x00567A20;
    TestMapView((HANDLE)(ULONG_PTR)0xDEADBEEFDEADBEEFull, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_HANDLE, IGNORE);

    //invalid process handle AND unaligned base address
    BaseAddress = (PVOID)(ULONG_PTR)0x00567A20;
    TestMapView(WriteSectionHandle, (HANDLE)(ULONG_PTR)0xDEADBEEFDEADBEEFull, &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_HANDLE, IGNORE);

    //try mapping section to an already mapped address
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &AllocBase, 0, &AllocSize, MEM_COMMIT, PAGE_READWRITE);
    if (!skip(NT_SUCCESS(Status), "Cannot allocate memory\n"))
    {
        BaseAddress = AllocBase;
        TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_CONFLICTING_ADDRESSES, IGNORE);
        Status = ZwFreeVirtualMemory(NtCurrentProcess(), &AllocBase, &AllocSize, MEM_RELEASE);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    //zero bits
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 1, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 5, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, -1, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_4, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 20, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_NO_MEMORY, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 21, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_NO_MEMORY, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 22, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_4, IGNORE);

    //commit size
    TestMapView(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 500, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, -1, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, IGNORE);
    TestMapView(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0x10000000, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 500, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 500, NULL, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    //section offset
    SectionOffset.QuadPart = 0;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    ok_eq_ulonglong(SectionOffset.QuadPart, 0);

    SectionOffset.QuadPart = 0x00040211; //MSDN is wrong, in w2k3 the ZwMapViewOfSection doesn't align offsets automatically
    TestMapView(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 500, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_MAPPED_ALIGNMENT, IGNORE);

    SectionOffset.QuadPart = -1;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_MAPPED_ALIGNMENT, IGNORE);

    //View Size
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    ViewSize = -1;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_3, IGNORE);

    ViewSize = TestStringSize+1;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_VIEW_SIZE, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    ViewSize = TestStringSize;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    ViewSize = TestStringSize-1;
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    //allocation type
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE, STATUS_INVALID_PARAMETER_9, STATUS_SUCCESS);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE, STATUS_INVALID_PARAMETER_9, IGNORE);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, (MEM_LARGE_PAGES | MEM_RESERVE), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    //win32protect
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_NOACCESS, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE_WRITECOPY, STATUS_SECTION_PROTECTION, IGNORE);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, IGNORE);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_WRITECOPY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE_READ, STATUS_SECTION_PROTECTION, IGNORE);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, IGNORE);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_NOACCESS, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, (PAGE_READWRITE | PAGE_READONLY), STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    TestMapView(ReadOnlySection, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READONLY, STATUS_SECTION_PROTECTION, IGNORE);

    ZwClose(WriteSectionHandle);
    ZwClose(PageFileSectionHandle);
    ZwClose(ReadOnlySection);
}


static
VOID
AdvancedErrorChecks(HANDLE FileHandleReadOnly, HANDLE FileHandleWriteOnly)
{
    NTSTATUS Status;
    PVOID BaseAddress;
    HANDLE FileSectionHandle;
    LARGE_INTEGER SectionOffset;
    LARGE_INTEGER MaximumSize;
    SIZE_T ViewSize = 0;
    PVOID SectionObject;

    MaximumSize.QuadPart = TestStringSize;
    //Used for parameters working on file-based section
    Status = ZwCreateSection(&FileSectionHandle, SECTION_ALL_ACCESS, NULL, &MaximumSize, PAGE_READWRITE, SEC_COMMIT, FileHandleWriteOnly);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = ObReferenceObjectByHandle(FileSectionHandle,
        STANDARD_RIGHTS_ALL,
        NULL,
        KernelMode,
        &SectionObject,
        NULL);

    ok_eq_hex(Status, STATUS_SUCCESS);

    //Bypassing Zw function calls mean bypassing the alignment checks which are not crucial for the branches being tested here

    //test first conditional branch
    ViewSize = -1;
    MmTestMapView(SectionObject, PsGetCurrentProcess(), &BaseAddress, 0, TestStringSize, &SectionOffset, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE, STATUS_INVALID_VIEW_SIZE, IGNORE);

    //test second conditional branch
    ViewSize = 1;
    SectionOffset.QuadPart = TestStringSize;
    MmTestMapView(SectionObject, PsGetCurrentProcess(), &BaseAddress, 0, TestStringSize, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_INVALID_VIEW_SIZE, IGNORE);

    ObDereferenceObject(SectionObject);
    ZwClose(FileSectionHandle);
}

static
SIZE_T
CompareFileContents(HANDLE FileHandle, ULONG BufferLength, PVOID Buffer)
{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    PVOID FileContent;
    SIZE_T Match;

    Match = 0;
    ByteOffset.QuadPart = 0;

    FileContent = ExAllocatePoolWithTag(PagedPool, BufferLength, 'Test');
    if (!skip((FileContent != NULL), "Error allocating memory for FileContent\n"))
    {
        Status = ZwReadFile(FileHandle, NULL, NULL, NULL, &IoStatusBlock, FileContent, BufferLength, &ByteOffset, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_ulongptr(IoStatusBlock.Information, BufferLength);

        Match = 0;
        Match = RtlCompareMemory(FileContent, Buffer, BufferLength);
        ExFreePoolWithTag(FileContent, 'Test');
    }

    return Match;
}


static
VOID
NTAPI
SystemProcessWorker(PVOID StartContext)
{
    NTSTATUS Status;
    PVOID BaseAddress;
    HANDLE SectionHandle;
    SIZE_T ViewSize;
    SIZE_T Match;
    LARGE_INTEGER SectionOffset;
    OBJECT_ATTRIBUTES ObjectAttributes;

    UNREFERENCED_PARAMETER(StartContext);

    BaseAddress = NULL;
    ViewSize = TestStringSize;
    SectionOffset.QuadPart = 0;

    InitializeObjectAttributes(&ObjectAttributes, &SharedSectionName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ZwOpenSection(&SectionHandle, SECTION_ALL_ACCESS, &ObjectAttributes);
    if (!skip(NT_SUCCESS(Status), "Error acquiring handle to section. Error = %p\n", Status))
    {
        CheckObject(SectionHandle, 4, 2);
        Status = ZwMapViewOfSection(SectionHandle, NtCurrentProcess(), &BaseAddress, 0, TestStringSize, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE);

        //make sure ZwMapViewofSection doesn't touch the section ref counts.
        CheckObject(SectionHandle, 4, 2);

        if (!skip(NT_SUCCESS(Status), "Error mapping page file view in system process. Error = %p\n", Status))
        {
            Match = RtlCompareMemory(BaseAddress, TestString, TestStringSize);
            ok_eq_size(Match, TestStringSize);

            RtlCopyMemory(BaseAddress, NEW_CONTENT, NEW_CONTENT_LEN);
            ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

            //make sure ZwMapViewofSection doesn't touch the section ref counts.
            CheckObject(SectionHandle, 4, 2);
        }

        ZwClose(SectionHandle);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}


static
VOID
BehaviorChecks(HANDLE FileHandleReadOnly, HANDLE FileHandleWriteOnly)
{
    NTSTATUS Status;
    PVOID BaseAddress = NULL;
    PVOID ThreadObject;
    HANDLE WriteSectionHandle;
    HANDLE SysThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LARGE_INTEGER SectionOffset;
    LARGE_INTEGER MaximumSize;
    SIZE_T Match;
    SIZE_T ViewSize = 0;

    InitializeObjectAttributes(&ObjectAttributes, &SharedSectionName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    MaximumSize.QuadPart = TestStringSize;
    SectionOffset.QuadPart = 0;

    Status = ZwCreateSection(&WriteSectionHandle, SECTION_ALL_ACCESS, &ObjectAttributes, &MaximumSize, PAGE_READWRITE, SEC_COMMIT, FileHandleWriteOnly);
    CheckObject(WriteSectionHandle, 3, 1);
    ok(NT_SUCCESS(Status), "Error creating write section from file. Error = %p\n", Status);

    //check for section reading/writing by comparing section content to a well-known value.
    Status = ZwMapViewOfSection(WriteSectionHandle, NtCurrentProcess() ,&BaseAddress, 0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE);
    CheckObject(WriteSectionHandle, 3, 1);
    if (!skip(NT_SUCCESS(Status), "Error mapping view with READ/WRITE priv. Error = %p\n", Status))
    {
        Match = RtlCompareMemory(BaseAddress, TestString, TestStringSize);
        ok_eq_size(Match, TestStringSize);

        //now check writing to section
        RtlCopyMemory(BaseAddress, NEW_CONTENT, NEW_CONTENT_LEN);

        Match = RtlCompareMemory(BaseAddress, NEW_CONTENT, NEW_CONTENT_LEN);
        ok_eq_size(Match, NEW_CONTENT_LEN);

        //check to see if the contents have been flushed to the actual file on disk.
        Match = CompareFileContents(FileHandleWriteOnly, NEW_CONTENT_LEN, NEW_CONTENT);
        ok_eq_size(Match, NEW_CONTENT_LEN);

        //bring everything back to normal
        RtlCopyMemory(BaseAddress, TestString, TestStringSize);

        //Initiate an external thread to modify the file
        InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
        Status = PsCreateSystemThread(&SysThreadHandle, STANDARD_RIGHTS_ALL, &ObjectAttributes, NULL, NULL, SystemProcessWorker, NULL);
        if (!skip(NT_SUCCESS(Status), "Error creating System thread. Error = %p\n", Status))
        {
            Status = ObReferenceObjectByHandle(SysThreadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &ThreadObject, NULL);
            if (!skip(NT_SUCCESS(Status), "Error getting reference to System thread when testing file-backed section\n"))
            {
                //wait until the system thread actually terminates
                KeWaitForSingleObject(ThreadObject, Executive, KernelMode, FALSE, NULL);

                //no longer need the thread object
                ObDereferenceObject(ThreadObject);

                //test for bi-directional access to the shared page file
                Match = RtlCompareMemory(BaseAddress, NEW_CONTENT, NEW_CONTENT_LEN);
                ok_eq_size(Match, NEW_CONTENT_LEN);

                //bring everything back to normal, again
                RtlCopyMemory(BaseAddress, TestString, TestStringSize);
            }
        }

        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }

    //Try to write to read-only mapped view
    BaseAddress = NULL;
    ViewSize = 0;
    SectionOffset.QuadPart = 0;
    Status = ZwMapViewOfSection(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READONLY);
    if (!skip(NT_SUCCESS(Status), "Error mapping view with READ priv. Error = %p\n", Status))
    {
        Match = RtlCompareMemory(BaseAddress, TestString, TestStringSize);
        ok_eq_size(Match, TestStringSize);

        KmtStartSeh()
            RtlCopyMemory(BaseAddress, NEW_CONTENT, NEW_CONTENT_LEN);
        KmtEndSeh(STATUS_ACCESS_VIOLATION);

        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }

    //try to access forbidden memory
    BaseAddress = NULL;
    ViewSize = 0;
    SectionOffset.QuadPart = 0;
    Status = ZwMapViewOfSection(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_NOACCESS);
    if (!skip(NT_SUCCESS(Status), "Error mapping view with PAGE_NOACCESS priv. Error = %p\n", Status))
    {
        KmtStartSeh()
            RtlCompareMemory(BaseAddress, TestString, TestStringSize);
        KmtEndSeh(STATUS_ACCESS_VIOLATION);

        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }

    //try to access guarded memory
    BaseAddress = NULL;
    ViewSize = 0;
    SectionOffset.QuadPart = 0;
    Status = ZwMapViewOfSection(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_GUARD | PAGE_READWRITE);
    if (!skip(NT_SUCCESS(Status), "Error mapping view with PAGE_GUARD priv. Error = %p\n", Status))
    {
        KmtStartSeh()
            RtlCompareMemory(BaseAddress, TestString, TestStringSize);
        KmtEndSeh(STATUS_GUARD_PAGE_VIOLATION);

        KmtStartSeh()
            RtlCompareMemory(BaseAddress, TestString, TestStringSize);
        KmtEndSeh(STATUS_SUCCESS);

        ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }

    ZwClose(WriteSectionHandle);

    //section created with sec_reserve should not be commited.
    BaseAddress = NULL;
    ViewSize = 0;
    SectionOffset.QuadPart = 0;
    Status = ZwCreateSection(&WriteSectionHandle, SECTION_ALL_ACCESS, &ObjectAttributes, &MaximumSize, PAGE_READWRITE, SEC_RESERVE, FileHandleWriteOnly);
    if (!skip(NT_SUCCESS(Status), "Error creating page file section. Error = %p\n", Status))
    {
        Status = ZwMapViewOfSection(WriteSectionHandle, NtCurrentProcess(), &BaseAddress, 0, TestStringSize, &SectionOffset, &ViewSize, ViewUnmap, MEM_RESERVE, PAGE_READWRITE);
        if (!skip(NT_SUCCESS(Status), "Error mapping page file view. Error = %p\n", Status))
        {
            //check also the SEC_COMMIT flag
            /* This test proves that MSDN is once again wrong
             *  https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw states that SEC_RESERVE
             *  should cause the allocated memory for the view to be reserved but in fact it is always committed.
             *  It fails also on windows.
             */
            Test_NtQueryVirtualMemory(BaseAddress, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
            ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
        }

        ZwClose(WriteSectionHandle);
    }
}


static
VOID
PageFileBehaviorChecks()
{
    NTSTATUS Status;
    LARGE_INTEGER MaxSectionSize;
    LARGE_INTEGER SectionOffset;
    HANDLE PageFileSectionHandle;
    PVOID BaseAddress;
    SIZE_T ViewSize;
    SIZE_T Match;
    PVOID ThreadObject;
    OBJECT_ATTRIBUTES ObjectAttributes;

    MaxSectionSize.QuadPart = TestStringSize;
    SectionOffset.QuadPart = 0;
    PageFileSectionHandle = INVALID_HANDLE_VALUE;
    BaseAddress = NULL;
    ViewSize = TestStringSize;
    InitializeObjectAttributes(&ObjectAttributes, &SharedSectionName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    //test memory sharing between 2 different processes
    Status = ZwCreateSection(&PageFileSectionHandle, SECTION_ALL_ACCESS, &ObjectAttributes, &MaxSectionSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!skip(NT_SUCCESS(Status), "Error creating page file section. Error = %p\n", Status))
    {
        CheckObject(PageFileSectionHandle, 3, 1);
        Status = ZwMapViewOfSection(PageFileSectionHandle, NtCurrentProcess(), &BaseAddress, 0, TestStringSize, &SectionOffset, &ViewSize, ViewUnmap, 0, PAGE_READWRITE);
        if (!skip(NT_SUCCESS(Status), "Error mapping page file view. Error = %p\n", Status))
        {
            HANDLE SysThreadHandle;

            CheckObject(PageFileSectionHandle, 3, 1);

            //check also the SEC_COMMIT flag
            Test_NtQueryVirtualMemory(BaseAddress, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

            RtlCopyMemory(BaseAddress, TestString, TestStringSize);

            InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
            Status = PsCreateSystemThread(&SysThreadHandle, STANDARD_RIGHTS_ALL, &ObjectAttributes, NULL, NULL, SystemProcessWorker, NULL);

            if (!skip(NT_SUCCESS(Status), "Error creating System thread. Error = %p\n", Status))
            {
                Status = ObReferenceObjectByHandle(SysThreadHandle, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &ThreadObject, NULL);
                if (!skip(NT_SUCCESS(Status), "Error getting reference to System thread when testing pagefile-backed section\n"))
                {
                    //wait until the system thread actually terminates
                    KeWaitForSingleObject(ThreadObject, Executive, KernelMode, FALSE, NULL);

                    //no longer need the thread object
                    ObDereferenceObject(ThreadObject);

                    //test for bi-directional access to the shared page file
                    Match = RtlCompareMemory(BaseAddress, NEW_CONTENT, NEW_CONTENT_LEN);
                    ok_eq_size(Match, NEW_CONTENT_LEN);
                }
            }
            ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
        }
        ZwClose(PageFileSectionHandle);
    }
}


START_TEST(ZwMapViewOfSection)
{
    HANDLE FileHandleReadOnly = NULL;
    HANDLE FileHandleWriteOnly = NULL;
    HANDLE ExecutableFileHandle = NULL;

    InitializeObjectAttributes(&NtdllObject, &FileReadOnlyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    InitializeObjectAttributes(&KmtestFileObject, &WritableFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    InitializeObjectAttributes(&NtoskrnlFileObject, &NtosImgPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    KmtInitTestFiles(&FileHandleReadOnly, &FileHandleWriteOnly, &ExecutableFileHandle);

    SimpleErrorChecks(FileHandleReadOnly, FileHandleWriteOnly, ExecutableFileHandle);
    AdvancedErrorChecks(FileHandleReadOnly, FileHandleWriteOnly);
    BehaviorChecks(FileHandleReadOnly, FileHandleWriteOnly);
    PageFileBehaviorChecks();

    if (FileHandleReadOnly)
        ZwClose(FileHandleReadOnly);

    if (FileHandleWriteOnly)
        ZwClose(FileHandleWriteOnly);

    if (ExecutableFileHandle)
        ZwClose(ExecutableFileHandle);
}
