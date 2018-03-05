/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite ZwAllocateVirtualMemory/ZwFreeVirtualMemory
 * PROGRAMMER:      Nikolay Borisov <nib9@aber.ac.uk>
 */

#include <kmt_test.h>

#define ROUND_DOWN(n,align) (((ULONG_PTR)n) & ~((align) - 1l))
#define DEFAULT_ALLOC_SIZE 200
#define IGNORE -1
#define PAGE_NOPROT 0x0 //MEM_RESERVE has this type of "protection"

/* These are being used in ZwMapViewOfSection as well */
const char TestString[] = "TheLongBrownFoxJumpedTheWhiteRabbitTheLongBrownFoxJumpedTheWhiteRabbitTheLongBrownFoxJumpedTheWhiteRabbitTheLongBrownFoxJumpedTheWhiteRabbitTheLongBrownFoxJumpedTheWhiteRabbitTheLongBrownFoxJumpedThe";
const ULONG TestStringSize = sizeof(TestString);

VOID Test_ZwAllocateVirtualMemory(VOID);

typedef struct _TEST_CONTEXT
{
    HANDLE ProcessHandle;
    SIZE_T RegionSize;
    ULONG AllocationType;
    ULONG Protect;
    PVOID Bases[1024];
    SHORT ThreadId;
} TEST_CONTEXT, *PTEST_CONTEXT;


#define ALLOC_MEMORY_WITH_FREE(ProcessHandle, BaseAddress, ZeroBits, RegionSize, AllocationType, Protect, RetStatus, FreeStatus)   \
    do {                                                                                                                   \
        PVOID __BaseSave = BaseAddress;                                                                                    \
        Status = ZwAllocateVirtualMemory(ProcessHandle, &BaseAddress, ZeroBits, &RegionSize, AllocationType, Protect);     \
        ok_eq_hex(Status, RetStatus);                                                                                      \
        if (__BaseSave != NULL)                                                                                            \
            ok_eq_pointer(BaseAddress, __BaseSave);                                                                        \
        else if (!NT_SUCCESS(Status))                                                                                      \
            ok_eq_pointer(BaseAddress, NULL);                                                                              \
        RegionSize = 0;                                                                                                    \
        Status = ZwFreeVirtualMemory(ProcessHandle, &BaseAddress, &RegionSize, MEM_RELEASE);                               \
        if (FreeStatus != IGNORE) ok_eq_hex(Status, FreeStatus);                                                           \
        BaseAddress = NULL;                                                                                                \
        RegionSize = DEFAULT_ALLOC_SIZE;                                                                                   \
    } while (0)                                                                                                            \



static
BOOLEAN
CheckBuffer(PVOID Buffer, SIZE_T Size, UCHAR Value)
{
    PUCHAR Array = Buffer;
    SIZE_T i;

    for (i = 0; i < Size; i++)
    {
        if (Array[i] != Value)
        {
            trace("Expected %x, found %x at offset %lu\n", Value, Array[i], (ULONG)i);
            return FALSE;
        }
    }
    return TRUE;
}

static
SIZE_T
CheckBufferRead(CONST VOID *Source, CONST VOID *Destination, SIZE_T Length, NTSTATUS ExpectedStatus)
{
    SIZE_T Match = 0;

    KmtStartSeh()
        Match = RtlCompareMemory(Source, Destination, Length);
    KmtEndSeh(ExpectedStatus);

    return Match;
}

static
VOID
CheckBufferReadWrite(PVOID Destination, CONST VOID *Source, SIZE_T Length, NTSTATUS ExpectedStatus)
{
    //do a little bit of writing/reading to memory
    SIZE_T Match = 0;

    KmtStartSeh()
        RtlCopyMemory(Destination, Source, Length);
    KmtEndSeh(ExpectedStatus);

    Match = CheckBufferRead(Source, Destination, Length, ExpectedStatus);
    if (ExpectedStatus == STATUS_SUCCESS) ok_eq_int(Match, Length);
}


static
VOID
SimpleErrorChecks(VOID)
{
    NTSTATUS Status;
    PVOID Base = NULL;
    SIZE_T RegionSize = DEFAULT_ALLOC_SIZE;

    //HANDLE TESTS
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    ALLOC_MEMORY_WITH_FREE(NULL, Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_HANDLE, STATUS_INVALID_HANDLE);
    ALLOC_MEMORY_WITH_FREE((HANDLE)(ULONG_PTR)0xDEADBEEFDEADBEEFull, Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_HANDLE, STATUS_INVALID_HANDLE);

    //BASE ADDRESS TESTS
    Base = (PVOID)0x00567A20;
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_CONFLICTING_ADDRESSES, STATUS_UNABLE_TO_DELETE_SECTION);

    Base = (PVOID) 0x60000000;
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    Base = (PVOID)((char *)MmSystemRangeStart + 200);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_2, STATUS_INVALID_PARAMETER_2);

    /* http://jira.reactos.org/browse/CORE-6814 */
    RegionSize = 0x1000;
    Base = Test_ZwAllocateVirtualMemory;
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, MEM_COMMIT, PAGE_READWRITE, STATUS_INVALID_PARAMETER_2, STATUS_INVALID_PARAMETER_2);

    //ZERO BITS TESTS
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 21, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_NO_MEMORY, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 22, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_3, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, -1, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_3, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 3, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);

    //REGION SIZE TESTS
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    RegionSize = -1;
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_4, STATUS_MEMORY_NOT_ALLOCATED);
    RegionSize = 0;
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_4, STATUS_MEMORY_NOT_ALLOCATED);
    RegionSize = 0xFFFFFFFF; // 4 gb  is invalid
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_4, STATUS_MEMORY_NOT_ALLOCATED);

    //Allocation type tests
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, MEM_PHYSICAL, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESET), PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, 0, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, MEM_TOP_DOWN, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_TOP_DOWN | MEM_RESET), PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_TOP_DOWN | MEM_COMMIT), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_PHYSICAL | MEM_RESERVE), PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_PHYSICAL | MEM_COMMIT), PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_RESET | MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, -1, PAGE_READWRITE, STATUS_INVALID_PARAMETER_5, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize,  MEM_COMMIT, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize,  MEM_RESERVE, PAGE_READWRITE, STATUS_SUCCESS, STATUS_SUCCESS);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize,  MEM_RESERVE, PAGE_WRITECOPY, STATUS_INVALID_PAGE_PROTECTION, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize,  MEM_RESERVE, PAGE_EXECUTE_WRITECOPY, STATUS_INVALID_PAGE_PROTECTION, STATUS_MEMORY_NOT_ALLOCATED);

    //Memory protection tests
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), 0, STATUS_INVALID_PAGE_PROTECTION, STATUS_MEMORY_NOT_ALLOCATED);
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), -1, STATUS_INVALID_PAGE_PROTECTION, STATUS_MEMORY_NOT_ALLOCATED);
    if (!KmtIsCheckedBuild)
    {
        ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), (PAGE_NOACCESS | PAGE_GUARD), STATUS_INVALID_PAGE_PROTECTION, STATUS_MEMORY_NOT_ALLOCATED);
        ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), (PAGE_NOACCESS | PAGE_WRITECOMBINE), STATUS_INVALID_PAGE_PROTECTION, STATUS_MEMORY_NOT_ALLOCATED);
    }
    ALLOC_MEMORY_WITH_FREE(NtCurrentProcess(), Base, 0, RegionSize, (MEM_COMMIT | MEM_RESERVE), (PAGE_READONLY | PAGE_WRITECOMBINE), STATUS_SUCCESS, STATUS_SUCCESS);
}


static
NTSTATUS
SimpleAllocation(VOID)
{
    NTSTATUS Status;
    PVOID Base = NULL;
    SIZE_T RegionSize = DEFAULT_ALLOC_SIZE;

    //////////////////////////////////////////////////////////////////////////
    //Normal operation
    //////////////////////////////////////////////////////////////////////////
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, MEM_COMMIT, PAGE_READWRITE);
    ok_eq_size(RegionSize, 4096);

    //check for the zero-filled pages
    ok_bool_true(CheckBuffer(Base, RegionSize, 0), "The buffer is not zero-filled");

    CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_SUCCESS);

    // try freeing
    RegionSize = 0;
    Status = ZwFreeVirtualMemory(NtCurrentProcess(), &Base, &RegionSize, MEM_RELEASE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(RegionSize, PAGE_SIZE);

    //////////////////////////////////////////////////////////////////////////
    // COMMIT AND RESERVE SCENARIO AND STATE CHANGE
    //////////////////////////////////////////////////////////////////////////
    //reserve and then commit
    Base = NULL;
    RegionSize = DEFAULT_ALLOC_SIZE;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, MEM_RESERVE, PAGE_READWRITE);
    Test_NtQueryVirtualMemory(Base, RegionSize, MEM_RESERVE, PAGE_NOPROT);
    CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_ACCESS_VIOLATION);


    Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, MEM_COMMIT, PAGE_READWRITE);
    CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_SUCCESS);
    Test_NtQueryVirtualMemory(Base, RegionSize, MEM_COMMIT, PAGE_READWRITE);

    RegionSize = 0;
    ZwFreeVirtualMemory(NtCurrentProcess(), &Base, &RegionSize, MEM_RELEASE);

    //////////////////////////////////////////////////////////////////////////
    // TRY READING/WRITING TO INVALID PROTECTION PAGES
    //////////////////////////////////////////////////////////////////////////
    RegionSize = DEFAULT_ALLOC_SIZE;
    Base = NULL;
    ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_NOACCESS);

    KmtStartSeh()
        RtlCopyMemory(Base, TestString, TestStringSize);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    Test_NtQueryVirtualMemory(Base, RegionSize, MEM_COMMIT, PAGE_NOACCESS);
    CheckBufferRead(Base, TestString, TestStringSize, STATUS_ACCESS_VIOLATION);

    RegionSize = 0;
    ZwFreeVirtualMemory(NtCurrentProcess(), &Base, &RegionSize, MEM_RELEASE);

    ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READONLY);
    KmtStartSeh()
        RtlCopyMemory(Base, TestString, TestStringSize);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    Test_NtQueryVirtualMemory(Base, RegionSize, MEM_COMMIT, PAGE_READONLY);

    ok_bool_true(CheckBuffer(Base, TestStringSize, 0), "Couldn't read a read-only buffer");

    RegionSize = 0;
    ZwFreeVirtualMemory(NtCurrentProcess(), &Base, &RegionSize, MEM_RELEASE);

    //////////////////////////////////////////////////////////////////////////
    // GUARD PAGES
    //////////////////////////////////////////////////////////////////////////

    RegionSize = 1000;
    Base = NULL;
    ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, (MEM_COMMIT | MEM_RESERVE), (PAGE_GUARD | PAGE_READWRITE));

    Test_NtQueryVirtualMemory(Base, RegionSize, MEM_COMMIT, (PAGE_GUARD | PAGE_READWRITE));
    KmtStartSeh()
        RtlCopyMemory(Base, TestString, TestStringSize);
    KmtEndSeh(STATUS_GUARD_PAGE_VIOLATION);

    Test_NtQueryVirtualMemory(Base, RegionSize, MEM_COMMIT, PAGE_READWRITE);

    KmtStartSeh()
        RtlCopyMemory(Base, TestString, TestStringSize);
    KmtEndSeh(STATUS_SUCCESS);

    RegionSize = 0;
    ZwFreeVirtualMemory(NtCurrentProcess(), &Base, &RegionSize, MEM_RELEASE);

    return Status;
}


static
VOID
CustomBaseAllocation(VOID)
{
    NTSTATUS Status;
    SIZE_T RegionSize = 200;
    PVOID Base =  (PVOID) 0x60025000;
    PVOID ActualStartingAddress = (PVOID)ROUND_DOWN(Base, MM_ALLOCATION_GRANULARITY); //it is rounded down to the nearest allocation granularity (64k) address
    PVOID EndingAddress = (PVOID)(((ULONG_PTR)Base + RegionSize - 1) | (PAGE_SIZE - 1));
    SIZE_T ActualSize = BYTES_TO_PAGES((ULONG_PTR)EndingAddress - (ULONG_PTR)ActualStartingAddress) * PAGE_SIZE; //calculates the actual size based on the required pages

    // allocate the memory
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(), (PVOID *)&Base, 0, &RegionSize, (MEM_COMMIT | MEM_RESERVE), PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(RegionSize, ActualSize);
    ok_eq_ulong(Base, ActualStartingAddress);
    Test_NtQueryVirtualMemory(ActualStartingAddress, ActualSize, MEM_COMMIT, PAGE_READWRITE);

    // try freeing
    RegionSize = 0;
    Status = ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *)&Base, &RegionSize, MEM_RELEASE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RegionSize, ActualSize);
}


static
NTSTATUS
StressTesting(ULONG AllocationType)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    static PVOID bases[1024]; //assume we are going to allocate only 5 gigs. static here means the arrays is not allocated on the stack but in the BSS segment of the driver
    ULONG Index = 0;
    PVOID Base = NULL;
    SIZE_T RegionSize = 5 * 1024 * 1024; // 5 megabytes;

    RtlZeroMemory(bases, sizeof(bases));

    for (Index = 0; Index < RTL_NUMBER_OF(bases) && NT_SUCCESS(Status); Index++)
    {
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &RegionSize, AllocationType, PAGE_READWRITE);

        bases[Index] = Base;
        if ((Index % 10) == 0)
        {
            if (AllocationType == MEM_COMMIT && NT_SUCCESS(Status))
            {
                CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_SUCCESS);
            }
            else
            {
                CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_ACCESS_VIOLATION);
            }
        }

        Base = NULL;
    }

    trace("Finished reserving. Error code %x. Chunks allocated: %d\n", Status, Index );

    ReturnStatus = Status;

    //free the allocated memory so that we can continue with the tests
    Status = STATUS_SUCCESS;
    Index = 0;
    while (NT_SUCCESS(Status) && Index < RTL_NUMBER_OF(bases))
    {
        RegionSize = 0;
        Status = ZwFreeVirtualMemory(NtCurrentProcess(), &bases[Index], &RegionSize, MEM_RELEASE);
        bases[Index++] = NULL;
    }

    return ReturnStatus;
}


static
VOID
NTAPI
SystemProcessTestWorker(PVOID StartContext)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PTEST_CONTEXT Context = (PTEST_CONTEXT)StartContext;
   ULONG Index = 0;
   PVOID Base = NULL;

   PAGED_CODE();

   RtlZeroMemory(Context->Bases, sizeof(Context->Bases));

   Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &Context->RegionSize, Context->AllocationType, Context->Protect);
   ZwFreeVirtualMemory(NtCurrentProcess(), &Base, &Context->RegionSize, MEM_RELEASE);
   Base = NULL;

    //if the previous allocation has failed there is no need to do the loop
    while (NT_SUCCESS(Status) && Index < RTL_NUMBER_OF(Context->Bases))
    {
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &Base, 0, &Context->RegionSize, Context->AllocationType, Context->Protect);

        Context->Bases[Index] = Base;
        if ((Index % 10) == 0)
        {
            if (Context->AllocationType == MEM_COMMIT)
            {
                CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_SUCCESS);
            }
            else
            {
                CheckBufferReadWrite(Base, TestString, TestStringSize, STATUS_ACCESS_VIOLATION);
            }
        }

        Base = NULL;
        Index++;
    }

    trace("[SYSTEM THREAD %d]. Error code %x. Chunks allocated: %d\n", Context->ThreadId, Status, Index);

    //free the allocated memory so that we can continue with the tests
    Status = STATUS_SUCCESS;
    Index = 0;
    while (NT_SUCCESS(Status) && Index < RTL_NUMBER_OF(Context->Bases))
    {
        Context->RegionSize = 0;
        Status = ZwFreeVirtualMemory(NtCurrentProcess(), &Context->Bases[Index], &Context->RegionSize, MEM_RELEASE);
        Context->Bases[Index++] = NULL;
    }

    PsTerminateSystemThread(Status);
}


static
VOID
KmtInitTestContext(PTEST_CONTEXT Ctx, SHORT ThreadId, ULONG RegionSize, ULONG AllocationType, ULONG Protect)
{
    PAGED_CODE();

    Ctx->AllocationType = AllocationType;
    Ctx->Protect = Protect;
    Ctx->RegionSize = RegionSize;
    Ctx->ThreadId = ThreadId;
}


static
VOID
SystemProcessTest(VOID)
{
    NTSTATUS Status;
    HANDLE Thread1 = INVALID_HANDLE_VALUE;
    HANDLE Thread2 = INVALID_HANDLE_VALUE;
    PVOID ThreadObjects[2] = { NULL };
    OBJECT_ATTRIBUTES ObjectAttributes;
    PTEST_CONTEXT StartContext1;
    PTEST_CONTEXT StartContext2;

    PAGED_CODE();

    StartContext1 = ExAllocatePoolWithTag(PagedPool, sizeof(TEST_CONTEXT), 'tXTC');
    StartContext2 = ExAllocatePoolWithTag(PagedPool, sizeof(TEST_CONTEXT), 'tXTC');
    if (StartContext1 == NULL || StartContext2 == NULL)
    {
        trace("Error allocating space for context structs\n");
        goto cleanup;
    }

    KmtInitTestContext(StartContext1, 1, 1 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
    KmtInitTestContext(StartContext2, 2, 3 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);
    InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = PsCreateSystemThread(&Thread1, THREAD_ALL_ACCESS, &ObjectAttributes, NULL, NULL, SystemProcessTestWorker, StartContext1);
    if (!NT_SUCCESS(Status))
    {
        trace("Error creating thread1\n");
        goto cleanup;
    }

    Status = ObReferenceObjectByHandle(Thread1, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &ThreadObjects[0], NULL);
    if (!NT_SUCCESS(Status))
    {
        trace("error referencing thread1\n");
        goto cleanup;
    }

    Status = PsCreateSystemThread(&Thread2, THREAD_ALL_ACCESS, &ObjectAttributes, NULL, NULL, SystemProcessTestWorker, StartContext2);
    if (!NT_SUCCESS(Status))
    {
        trace("Error creating thread2\n");
        goto cleanup;
    }

    Status = ObReferenceObjectByHandle(Thread2, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &ThreadObjects[1], NULL);
    if (!NT_SUCCESS(Status))
    {
        trace("error referencing thread2\n");
        goto cleanup;
    }

cleanup:

    if (ThreadObjects[0])
        Status = KeWaitForSingleObject(ThreadObjects[0], Executive, KernelMode, FALSE, NULL);

    if (StartContext1 != NULL)
        ExFreePoolWithTag(StartContext1, 'tXTC');

    if (ThreadObjects[1])
        Status = KeWaitForSingleObject(ThreadObjects[1], Executive, KernelMode, FALSE, NULL);

    if (StartContext2 != NULL)
        ExFreePoolWithTag(StartContext2, 'tXTC');

    if (ThreadObjects[0] != NULL)
        ObDereferenceObject(ThreadObjects[0]);

    if (ThreadObjects[1] != NULL)
        ObDereferenceObject(ThreadObjects[1]);

    if (Thread1 != INVALID_HANDLE_VALUE)
        ZwClose(Thread1);

    if (Thread2 != INVALID_HANDLE_VALUE)
        ZwClose(Thread2);
}


START_TEST(ZwAllocateVirtualMemory)
{
    NTSTATUS Status;

    SimpleErrorChecks();

    SimpleAllocation();

    CustomBaseAllocation();

    Status = StressTesting(MEM_RESERVE);
    ok_eq_hex(Status, STATUS_NO_MEMORY);

    Status = STATUS_SUCCESS;
    Status = StressTesting(MEM_COMMIT);
    ok_eq_hex(Status, STATUS_NO_MEMORY);

    SystemProcessTest();
}
