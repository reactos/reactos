/*
* PROJECT:         ReactOS kernel-mode tests
* LICENSE:         GPLv2+ - See COPYING in the top level directory
* PURPOSE:         Kernel-Mode Test Suite ZwCreateSection
* PROGRAMMER:      Nikolay Borisov <nib9@aber.ac.uk>
*/

#include <kmt_test.h>

#define IGNORE -999
#define NO_HANDLE_CLOSE -998
#define _4mb 4194304
extern const char TestString[];
extern const ULONG TestStringSize;
static UNICODE_STRING FileReadOnlyPath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\ntdll.dll");
static UNICODE_STRING WritableFilePath = RTL_CONSTANT_STRING(L"\\SystemRoot\\kmtest-MmSection.txt");
static UNICODE_STRING CalcImgPath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\calc.exe");
static OBJECT_ATTRIBUTES NtdllObject;
static OBJECT_ATTRIBUTES KmtestFileObject;
static OBJECT_ATTRIBUTES CalcFileObject;

#define CREATE_SECTION(Handle, DesiredAccess, Attributes, Size, SectionPageProtection, AllocationAttributes, FileHandle,  RetStatus, CloseRetStatus)  do  \
{                                                                                                                                                         \
    Status = ZwCreateSection(&Handle, DesiredAccess, Attributes, &Size, SectionPageProtection, AllocationAttributes, FileHandle);                         \
    ok_eq_hex(Status, RetStatus);                                                                                                                         \
    if (NT_SUCCESS(Status))                                                                                                                               \
    {                                                                                                                                                     \
        if (CloseRetStatus != NO_HANDLE_CLOSE)                                                                                                            \
        {                                                                                                                                                 \
            Status = ZwClose(Handle);                                                                                                                     \
            Handle = NULL;                                                                                                                                \
            if (CloseRetStatus != IGNORE) ok_eq_hex(Status, CloseRetStatus);                                                                              \
        }                                                                                                                                                 \
    }                                                                                                                                                     \
} while (0)

#define TestMapView(SectionHandle, ProcessHandle, BaseAddress2, ZeroBits, CommitSize, SectionOffset, ViewSize2, InheritDisposition, AllocationType, Win32Protect, MapStatus, UnmapStatus) do   \
{                                                                                                                                                                                              \
    Status = ZwMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress2, ZeroBits, CommitSize, SectionOffset, ViewSize2, InheritDisposition, AllocationType, Win32Protect);                 \
    ok_eq_hex(Status, MapStatus);                                                                                                                                                              \
    if (NT_SUCCESS(Status))                                                                                                                                                                    \
    {                                                                                                                                                                                          \
        Status = ZwUnmapViewOfSection(ProcessHandle, BaseAddress);                                                                                                                             \
        if (UnmapStatus != IGNORE) ok_eq_hex(Status, UnmapStatus);                                                                                                                             \
        *BaseAddress2 = NULL;                                                                                                                                                                  \
        *ViewSize2 = 0;                                                                                                                                                                        \
    }                                                                                                                                                                                          \
} while (0)

#define CheckObject(Handle, Pointers, Handles) do                   \
{                                                                   \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                     \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,          \
    &ObjectInfo, sizeof ObjectInfo, NULL);                          \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    ok_eq_ulong(ObjectInfo.PointerCount, Pointers);                 \
    ok_eq_ulong(ObjectInfo.HandleCount, Handles);                   \
} while (0)


#define CheckSection(SectionHandle, SectionFlag, SectionSize, RetStatus) do \
{                                                                           \
    SECTION_BASIC_INFORMATION Sbi;                                          \
    NTSTATUS Status;                                                        \
    Status = ZwQuerySection(SectionHandle, SectionBasicInformation,         \
    &Sbi, sizeof Sbi, NULL);                                                \
    ok_eq_hex(Status, RetStatus);                                           \
    if (RetStatus == STATUS_SUCCESS && NT_SUCCESS(Status))                  \
    {                                                                       \
        ok_eq_pointer(Sbi.BaseAddress, NULL);                               \
        ok_eq_longlong(Sbi.Size.QuadPart, SectionSize);                     \
        ok_eq_hex(Sbi.Attributes, SectionFlag | SEC_FILE);                  \
    }                                                                       \
} while (0)

static
VOID
FileSectionViewPermissionCheck(HANDLE ReadOnlyFile, HANDLE WriteOnlyFile, HANDLE ExecutableFile)
{
    NTSTATUS Status;
    HANDLE SectionHandle = NULL;
    PVOID BaseAddress = NULL;
    SIZE_T ViewSize = 0;
    LARGE_INTEGER MaximumSize;

    MaximumSize.QuadPart = TestStringSize;

    //READ-ONLY FILE COMBINATIONS
    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SUCCESS, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SUCCESS, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SUCCESS, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, ReadOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    //WRITE-ONLY FILE COMBINATIONS
    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SUCCESS, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SUCCESS, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SUCCESS, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, WriteOnlyFile, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READWRITE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_READONLY, STATUS_SUCCESS, STATUS_SUCCESS);
    TestMapView(SectionHandle, ZwCurrentProcess(), &BaseAddress, 0, 0, NULL, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE, STATUS_SECTION_PROTECTION, STATUS_SUCCESS);
    ZwClose(SectionHandle);

    //EXECUTE ONLY FILE
    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_READ, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);

    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_WRITE, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);

    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_EXECUTE, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(SectionHandle, SECTION_MAP_EXECUTE, NULL, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, ExecutableFile, STATUS_SUCCESS, STATUS_SUCCESS);
}

static
VOID
KmtInitTestFiles(PHANDLE ReadOnlyFile, PHANDLE WriteOnlyFile, PHANDLE ExecutableFile)
{
    NTSTATUS Status;
    LARGE_INTEGER FileOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    //INIT THE READ-ONLY FILE
    Status = ZwCreateFile(ReadOnlyFile, ( GENERIC_READ | GENERIC_EXECUTE ), &NtdllObject, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(*ReadOnlyFile != NULL, "Couldn't acquire READONLY handle\n");

    //INIT THE EXECUTABLE FILE
    Status = ZwCreateFile(ExecutableFile, GENERIC_EXECUTE, &CalcFileObject, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(*ExecutableFile != NULL, "Couldn't acquire EXECUTE handle\n");

    //INIT THE WRITE-ONLY FILE
    //NB: this file is deleted at the end of basic behavior checks
    Status = ZwCreateFile(WriteOnlyFile, (GENERIC_WRITE | SYNCHRONIZE), &KmtestFileObject, &IoStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE, FILE_SUPERSEDE, FILE_NON_DIRECTORY_FILE, NULL, 0);
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
SimpleErrorChecks(HANDLE FileHandleReadOnly, HANDLE FileHandleWriteOnly, HANDLE FileHandleExecuteOnly)
{
    NTSTATUS Status;
    HANDLE Section = NULL;
    OBJECT_ATTRIBUTES ObjectAttributesReadOnly;
    OBJECT_ATTRIBUTES ObjectAttributesWriteOnly;
    OBJECT_ATTRIBUTES InvalidObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStandardInfo;
    LARGE_INTEGER MaximumSize;
    UNICODE_STRING SectReadOnly = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\KmtTestReadSect");
    UNICODE_STRING SectWriteOnly = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\KmtTestWriteSect");
    UNICODE_STRING InvalidObjectString = RTL_CONSTANT_STRING(L"THIS/IS/INVALID");

    MaximumSize.QuadPart = TestStringSize;

    InitializeObjectAttributes(&ObjectAttributesReadOnly, &SectReadOnly, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    InitializeObjectAttributes(&ObjectAttributesWriteOnly, &SectWriteOnly, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    InitializeObjectAttributes(&InvalidObjectAttributes, &InvalidObjectString, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    ///////////////////////////////////////////////////////////////////////////////////////
    //PAGE FILE BACKED SECTION
    //DESIRED ACCESS TESTS
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, 0, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, -1, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);

    //OBJECT ATTRIBUTES
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &ObjectAttributesReadOnly, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &InvalidObjectAttributes, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_OBJECT_PATH_SYNTAX_BAD, STATUS_SUCCESS);

    //MAXIMUM SIZE
    MaximumSize.QuadPart = -1;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_SECTION_TOO_BIG, IGNORE);

    MaximumSize.QuadPart = 0;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL, STATUS_INVALID_PARAMETER_4, IGNORE);

    //division by zero in ROS
    if (!skip(SharedUserData->LargePageMinimum > 0, "LargePageMinimum is 0\n"))
    {
        MaximumSize.QuadPart = (_4mb / SharedUserData->LargePageMinimum) * SharedUserData->LargePageMinimum; //4mb
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_LARGE_PAGES | SEC_COMMIT), NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    }

    MaximumSize.QuadPart = TestStringSize;

    //SECTION PAGE PROTECTION
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_WRITECOPY, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, (PAGE_EXECUTE_READ | PAGE_READWRITE), SEC_COMMIT, NULL, STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, (PAGE_READONLY | PAGE_READWRITE), SEC_COMMIT, NULL, STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, (PAGE_WRITECOPY | PAGE_READONLY), SEC_COMMIT, NULL, STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, 0, SEC_COMMIT, NULL, STATUS_INVALID_PAGE_PROTECTION, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, -1, SEC_COMMIT, NULL, STATUS_INVALID_PAGE_PROTECTION, STATUS_SUCCESS);

    //ALLOCATION ATTRIBUTES
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, 0, NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_COMMIT | SEC_RESERVE), NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_RESERVE, NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_IMAGE, NULL, STATUS_INVALID_FILE_FOR_SECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_IMAGE | SEC_COMMIT), NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, -1, NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_LARGE_PAGES, NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_LARGE_PAGES | SEC_COMMIT), NULL, STATUS_INVALID_PARAMETER_4, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_NOCACHE, NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_NOCACHE | SEC_RESERVE | SEC_COMMIT), NULL, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_NOCACHE | SEC_COMMIT), NULL, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_NOCACHE | SEC_RESERVE), NULL, STATUS_SUCCESS, STATUS_SUCCESS);

    /////////////////////////////////////////////////////////////////////////////////////////////
    //NORMAL FILE-BACKED SECTION

    //DESIRED ACCESS TESTS
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &ObjectAttributesReadOnly, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleReadOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &ObjectAttributesWriteOnly, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_MAP_WRITE, &ObjectAttributesReadOnly, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleReadOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_MAP_READ, &ObjectAttributesWriteOnly, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

    //Object Attributes
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleReadOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &InvalidObjectAttributes, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleReadOnly, STATUS_OBJECT_PATH_SYNTAX_BAD, IGNORE);

    //MAXIMUM SIZE
    MaximumSize.QuadPart = TestStringSize - 100;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

    MaximumSize.QuadPart = -1;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SECTION_TOO_BIG, IGNORE);

    MaximumSize.QuadPart = TestStringSize + 1;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SECTION_TOO_BIG, IGNORE);

    MaximumSize.QuadPart = 0;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

    //SECTION PAGE PROTECTION
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READWRITE, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_WRITECOPY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, (PAGE_EXECUTE_READ | PAGE_READWRITE), SEC_COMMIT, FileHandleWriteOnly, STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, (PAGE_READONLY | PAGE_READWRITE), SEC_COMMIT, FileHandleWriteOnly, STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, (PAGE_WRITECOPY | PAGE_READONLY), SEC_COMMIT, FileHandleWriteOnly, STATUS_INVALID_PAGE_PROTECTION, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, 0, SEC_COMMIT, FileHandleWriteOnly, STATUS_INVALID_PAGE_PROTECTION, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, -1, SEC_COMMIT, FileHandleWriteOnly, STATUS_INVALID_PAGE_PROTECTION, STATUS_SUCCESS);

    //allocation type
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, 0, FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_COMMIT | SEC_RESERVE), FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_RESERVE, FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_IMAGE | SEC_COMMIT), FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, -1, FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_LARGE_PAGES, FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_LARGE_PAGES | SEC_COMMIT), FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_NOCACHE, FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_NOCACHE | SEC_RESERVE | SEC_COMMIT), FileHandleWriteOnly, STATUS_INVALID_PARAMETER_6, IGNORE);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_NOCACHE | SEC_COMMIT), FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, (SEC_NOCACHE | SEC_RESERVE), FileHandleWriteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READWRITE, SEC_IMAGE, FileHandleWriteOnly, STATUS_INVALID_IMAGE_NOT_MZ, IGNORE);

    //////////////////////////////////////////////////
    //EXECUTABLE IMAGE
    CREATE_SECTION(Section, SECTION_MAP_READ, &ObjectAttributesWriteOnly, MaximumSize, PAGE_WRITECOPY, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_MAP_EXECUTE, &ObjectAttributesWriteOnly, MaximumSize, PAGE_WRITECOPY, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

    //DESIRED ACCESS TESTS
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &ObjectAttributesReadOnly, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &ObjectAttributesWriteOnly, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_MAP_WRITE, &ObjectAttributesReadOnly, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_MAP_READ, &ObjectAttributesWriteOnly, MaximumSize, PAGE_WRITECOPY, SEC_COMMIT, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

    //Object Attributes
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, &InvalidObjectAttributes, MaximumSize, PAGE_READONLY, SEC_COMMIT, FileHandleExecuteOnly, STATUS_OBJECT_PATH_SYNTAX_BAD, IGNORE);

    //MaximumSize
    Status = ZwQueryInformationFile(FileHandleExecuteOnly, &IoStatusBlock, &FileStandardInfo, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
    if (!skip(NT_SUCCESS(Status), "Cannot query file information\n"))
    {
        //as big as file
        MaximumSize = FileStandardInfo.EndOfFile;
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

        //less than file
        MaximumSize.QuadPart = FileStandardInfo.EndOfFile.QuadPart - 2;
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

        //larger than file
        MaximumSize.QuadPart = FileStandardInfo.EndOfFile.QuadPart + 2;
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

        //0
        MaximumSize.QuadPart = 0;
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SUCCESS, STATUS_SUCCESS);

        //-1 (very big number)
        MaximumSize.QuadPart = -1;
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, MaximumSize, PAGE_EXECUTE_READ, SEC_IMAGE, FileHandleExecuteOnly, STATUS_SECTION_TOO_BIG, IGNORE);
    }
}

static
VOID
BasicBehaviorChecks(HANDLE FileHandle)
{
    NTSTATUS Status;
    HANDLE Section = NULL;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER Length;
    Length.QuadPart = TestStringSize;

    //mimic lack of section support for a particular file as well.
    Status = ObReferenceObjectByHandle(FileHandle, STANDARD_RIGHTS_ALL, *IoFileObjectType, KernelMode, (PVOID *)&FileObject, NULL);
    if (!skip(NT_SUCCESS(Status), "Cannot reference object by handle\n"))
    {
        PSECTION_OBJECT_POINTERS Pointers = FileObject->SectionObjectPointer;

        FileObject->SectionObjectPointer = NULL;
        CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, Length, PAGE_READONLY, SEC_COMMIT, FileHandle, STATUS_INVALID_FILE_FOR_SECTION, IGNORE);
        FileObject->SectionObjectPointer = Pointers;
        ObDereferenceObject(FileObject);
    }

    Length.QuadPart = TestStringSize;
    CREATE_SECTION(Section, (SECTION_ALL_ACCESS), NULL, Length, PAGE_READONLY, SEC_COMMIT, FileHandle, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    CheckObject(Section, 2, 1);
    CheckSection(Section, SEC_FILE, Length.QuadPart, STATUS_SUCCESS);
    ZwClose(Section); //manually close it due to NO_HANDLE_CLOSE in CREATE_SECTION

    //section length should be set to that of file
    Length.QuadPart = 0;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, Length, PAGE_READONLY, SEC_COMMIT, FileHandle, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    CheckSection(Section, SEC_FILE, TestStringSize, STATUS_SUCCESS);
    ZwClose(Section);

    //create a smaller section than file
    Length.QuadPart = TestStringSize - 100;
    CREATE_SECTION(Section, SECTION_ALL_ACCESS, NULL, Length, PAGE_READONLY, SEC_COMMIT, FileHandle, STATUS_SUCCESS, NO_HANDLE_CLOSE);
    CheckSection(Section, SEC_FILE, TestStringSize - 100, STATUS_SUCCESS);
    ZwClose(Section);
}


START_TEST(ZwCreateSection)
{
    HANDLE FileHandleReadOnly = NULL;
    HANDLE FileHandleWriteOnly = NULL;
    HANDLE FileHandleExecuteOnly = NULL;

    InitializeObjectAttributes(&NtdllObject, &FileReadOnlyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    InitializeObjectAttributes(&KmtestFileObject, &WritableFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    InitializeObjectAttributes(&CalcFileObject, &CalcImgPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    KmtInitTestFiles(&FileHandleReadOnly, &FileHandleWriteOnly, &FileHandleExecuteOnly);

    if (!skip(FileHandleReadOnly && FileHandleWriteOnly && FileHandleExecuteOnly, "Missing one or more file handles\n"))
    {
        FileSectionViewPermissionCheck(FileHandleReadOnly, FileHandleWriteOnly, FileHandleExecuteOnly);
        SimpleErrorChecks(FileHandleReadOnly, FileHandleWriteOnly, FileHandleExecuteOnly);
        BasicBehaviorChecks(FileHandleWriteOnly);
    }

    if (FileHandleReadOnly)
        ZwClose(FileHandleReadOnly);

    if (FileHandleWriteOnly)
    {
        ZwClose(FileHandleWriteOnly);
        ZwDeleteFile(&KmtestFileObject);
    }

    if (FileHandleExecuteOnly)
        ZwClose(FileHandleExecuteOnly);
}
