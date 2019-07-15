/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for CcMapData function
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define IOCTL_START_TEST  1
#define IOCTL_FINISH_TEST 2

typedef struct _TEST_FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    FAST_MUTEX HeaderMutex;
} TEST_FCB, *PTEST_FCB;

typedef struct _TEST_CONTEXT
{
    PVOID Bcb;
    PVOID Buffer;
    ULONG Length;
} TEST_CONTEXT, *PTEST_CONTEXT;

static ULONG TestTestId = -1;
static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;
static KMT_IRP_HANDLER TestIrpHandler;
static KMT_MESSAGE_HANDLER TestMessageHandler;
static ULONGLONG Memory = 0;
static BOOLEAN TS = FALSE;

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    ULONG Length;
    SYSTEM_BASIC_INFORMATION SBI;
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_OSVERSIONINFOEXW VersionInfo;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    *DeviceName = L"CcMapData";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SBI,
                                      sizeof(SBI),
                                      &Length);
    if (NT_SUCCESS(Status))
    {
        Memory = (SBI.NumberOfPhysicalPages * SBI.PageSize) / 1024 / 1024;
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&VersionInfo);
    if (NT_SUCCESS(Status))
    {
        TS = BooleanFlagOn(VersionInfo.wSuiteMask, VER_SUITE_TERMINAL) &&
             !BooleanFlagOn(VersionInfo.wSuiteMask, VER_SUITE_SINGLEUSERTS);
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    trace("System with %I64dMb RAM and terminal services %S\n",  Memory, (TS ? L"enabled" : L"disabled"));

    return Status;
}

VOID
TestUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
}

BOOLEAN
NTAPI
AcquireForLazyWrite(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    return TRUE;
}

VOID
NTAPI
ReleaseFromLazyWrite(
    _In_ PVOID Context)
{
    return;
}

BOOLEAN
NTAPI
AcquireForReadAhead(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    return TRUE;
}

VOID
NTAPI
ReleaseFromReadAhead(
    _In_ PVOID Context)
{
    return;
}

static CACHE_MANAGER_CALLBACKS Callbacks = {
    AcquireForLazyWrite,
    ReleaseFromLazyWrite,
    AcquireForReadAhead,
    ReleaseFromReadAhead,
};

static CC_FILE_SIZES FileSizes = {
    RTL_CONSTANT_LARGE_INTEGER((LONGLONG)0x4000), // .AllocationSize
    RTL_CONSTANT_LARGE_INTEGER((LONGLONG)0x4000), // .FileSize
    RTL_CONSTANT_LARGE_INTEGER((LONGLONG)0x4000)  // .ValidDataLength
};

static
PVOID
MapAndLockUserBuffer(
    _In_ _Out_ PIRP Irp,
    _In_ ULONG BufferLength)
{
    PMDL Mdl;

    if (Irp->MdlAddress == NULL)
    {
        Mdl = IoAllocateMdl(Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp);
        if (Mdl == NULL)
        {
            return NULL;
        }

        _SEH2_TRY
        {
            MmProbeAndLockPages(Mdl, Irp->RequestorMode, IoWriteAccess);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            IoFreeMdl(Mdl);
            Irp->MdlAddress = NULL;
            _SEH2_YIELD(return NULL);
        }
        _SEH2_END;
    }

    return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
}

static
VOID
NTAPI
MapInAnotherThread(IN PVOID Context)
{
    BOOLEAN Ret;
    PULONG Buffer;
    PVOID Bcb;
    LARGE_INTEGER Offset;
    PTEST_CONTEXT TestContext;

    ok(TestFileObject != NULL, "Called in invalid context!\n");
    ok_eq_ulong(TestTestId, 3);

    TestContext = Context;
    ok(TestContext != NULL, "Called in invalid context!\n");
    ok(TestContext->Bcb != NULL, "Called in invalid context!\n");
    ok(TestContext->Buffer != NULL, "Called in invalid context!\n");
    ok(TestContext->Length != 0, "Called in invalid context!\n");

    Ret = FALSE;
    Offset.QuadPart = 0x1000;
    KmtStartSeh();
    Ret = CcMapData(TestFileObject, &Offset, TestContext->Length, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
    KmtEndSeh(STATUS_SUCCESS);

    if (!skip(Ret == TRUE, "CcMapData failed\n"))
    {
        ok_eq_pointer(Bcb, TestContext->Bcb);
        ok_eq_pointer(Buffer, TestContext->Buffer);

        CcUnpinData(Bcb);
    }

    KmtStartSeh();
    Ret = CcPinRead(TestFileObject, &Offset, TestContext->Length, 0, &Bcb, (PVOID *)&Buffer);
    KmtEndSeh(STATUS_SUCCESS);

    if (!skip(Ret == TRUE, "CcPinRead failed\n"))
    {
        ok(Bcb != TestContext->Bcb, "Returned same BCB!\n");
        ok_eq_pointer(Buffer, TestContext->Buffer);

        CcUnpinData(Bcb);
    }

    KmtStartSeh();
    Ret = CcPinRead(TestFileObject, &Offset, TestContext->Length, PIN_IF_BCB, &Bcb, (PVOID *)&Buffer);
    KmtEndSeh(STATUS_SUCCESS);
    ok(Ret == FALSE, "CcPinRead succeed\n");

    if (Ret)
    {
        CcUnpinData(Bcb);
    }

    KmtStartSeh();
    Ret = CcPinRead(TestFileObject, &Offset, TestContext->Length, PIN_EXCLUSIVE, &Bcb, (PVOID *)&Buffer);
    KmtEndSeh(STATUS_SUCCESS);

    if (!skip(Ret == TRUE, "CcPinRead failed\n"))
    {
        ok(Bcb != TestContext->Bcb, "Returned same BCB!\n");
        ok_eq_pointer(Buffer, TestContext->Buffer);

        CcUnpinData(Bcb);
    }

    Offset.QuadPart = 0x1500;
    TestContext->Length -= 0x500;

    KmtStartSeh();
    Ret = CcMapData(TestFileObject, &Offset, TestContext->Length, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
    KmtEndSeh(STATUS_SUCCESS);

    if (!skip(Ret == TRUE, "CcMapData failed\n"))
    {
        ok_eq_pointer(Bcb, TestContext->Bcb);
        ok_eq_pointer(Buffer, (PVOID)((ULONG_PTR)TestContext->Buffer + 0x500));

        CcUnpinData(Bcb);
    }

    return;
}

static
VOID
PerformTest(
    ULONG TestId,
    PDEVICE_OBJECT DeviceObject)
{
    PVOID Bcb;
    BOOLEAN Ret;
    PULONG Buffer;
    PTEST_FCB Fcb;
    LARGE_INTEGER Offset;

    ok_eq_pointer(TestFileObject, NULL);
    ok_eq_pointer(TestDeviceObject, NULL);
    ok_eq_ulong(TestTestId, -1);

    TestDeviceObject = DeviceObject;
    TestTestId = TestId;
    TestFileObject = IoCreateStreamFileObject(NULL, DeviceObject);
    if (!skip(TestFileObject != NULL, "Failed to allocate FO\n"))
    {
        Fcb = ExAllocatePool(NonPagedPool, sizeof(TEST_FCB));
        if (!skip(Fcb != NULL, "ExAllocatePool failed\n"))
        {
            RtlZeroMemory(Fcb, sizeof(TEST_FCB));
            ExInitializeFastMutex(&Fcb->HeaderMutex);
            FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);

            TestFileObject->FsContext = Fcb;
            TestFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

            KmtStartSeh();
            CcInitializeCacheMap(TestFileObject, &FileSizes, FALSE, &Callbacks, NULL);
            KmtEndSeh(STATUS_SUCCESS);

            if (!skip(CcIsFileCached(TestFileObject) == TRUE, "CcInitializeCacheMap failed\n"))
            {
                if (TestId < 3)
                {
                    Ret = FALSE;
                    Offset.QuadPart = TestId * 0x1000;
                    KmtStartSeh();
                    Ret = CcMapData(TestFileObject, &Offset, FileSizes.FileSize.QuadPart - Offset.QuadPart, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_SUCCESS);

                    if (!skip(Ret == TRUE, "CcMapData failed\n"))
                    {
                        ok_eq_ulong(Buffer[(0x3000 - TestId * 0x1000) / sizeof(ULONG)], 0xDEADBABE);

                        CcUnpinData(Bcb);
                    }
                }
                else if (TestId == 3)
                {
                    PTEST_CONTEXT TestContext;

                    TestContext = ExAllocatePool(NonPagedPool, sizeof(TEST_CONTEXT));
                    if (!skip(TestContext != NULL, "ExAllocatePool failed\n"))
                    {
                        Ret = FALSE;
                        Offset.QuadPart = 0x1000;
                        KmtStartSeh();
                        Ret = CcMapData(TestFileObject, &Offset, FileSizes.FileSize.QuadPart - Offset.QuadPart, MAP_WAIT, &TestContext->Bcb, &TestContext->Buffer);
                        KmtEndSeh(STATUS_SUCCESS);

                        if (!skip(Ret == TRUE, "CcMapData failed\n"))
                        {
                            PKTHREAD ThreadHandle;

                            /* That's a bit rough but should do the job */
#ifdef _X86_
                            if (Memory >= 2 * 1024)
                            {
                                ok((TestContext->Buffer >= (PVOID)0xC1000000 && TestContext->Buffer < (PVOID)0xE0FFFFFF) ||
                                   (TestContext->Buffer >= (PVOID)0xA4000000 && TestContext->Buffer < (PVOID)0xBFFFFFFF),
                                   "Buffer %p not mapped in system space\n", TestContext->Buffer);
                            }
                            else if (TS)
                            {
                                ok(TestContext->Buffer >= (PVOID)0xC1000000 && TestContext->Buffer < (PVOID)0xDCFFFFFF,
                                   "Buffer %p not mapped in system space\n", TestContext->Buffer);
                            }
                            else
                            {
                                ok(TestContext->Buffer >= (PVOID)0xC1000000 && TestContext->Buffer < (PVOID)0xDBFFFFFF,
                                   "Buffer %p not mapped in system space\n", TestContext->Buffer);
                            }
#elif defined(_M_AMD64)
                            ok(TestContext->Buffer >= (PVOID)0xFFFFF98000000000 && TestContext->Buffer < (PVOID)0xFFFFFA8000000000,
                               "Buffer %p not mapped in system space\n", TestContext->Buffer);
#else
                            skip(FALSE, "System space mapping not defined\n");
#endif

                            TestContext->Length = FileSizes.FileSize.QuadPart - Offset.QuadPart;
                            ThreadHandle = KmtStartThread(MapInAnotherThread, TestContext);
                            KmtFinishThread(ThreadHandle, NULL);

                            TestContext->Length = FileSizes.FileSize.QuadPart - 2 * Offset.QuadPart;
                            ThreadHandle = KmtStartThread(MapInAnotherThread, TestContext);
                            KmtFinishThread(ThreadHandle, NULL);

                            CcUnpinData(TestContext->Bcb);
                        }

                        ExFreePool(TestContext);
                    }
                }
                else if (TestId == 4)
                {
                    /* Map after EOF */
                    Ret = FALSE;
                    Offset.QuadPart = FileSizes.FileSize.QuadPart + 0x1000;

                    KmtStartSeh();
                    Ret = CcMapData(TestFileObject, &Offset, 0x1000, 0, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_SUCCESS);
                    ok(Ret == FALSE, "CcMapData succeed\n");

                    if (Ret)
                    {
                        CcUnpinData(Bcb);
                    }

                    /* Map a VACB after EOF */
                    Ret = FALSE;
                    Offset.QuadPart = FileSizes.FileSize.QuadPart + 0x1000 + VACB_MAPPING_GRANULARITY;

                    KmtStartSeh();
                    Ret = CcMapData(TestFileObject, &Offset, 0x1000, 0, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_ACCESS_VIOLATION);
                    ok(Ret == FALSE, "CcMapData succeed\n");

                    if (Ret)
                    {
                        CcUnpinData(Bcb);
                    }

                    /* Map more than a VACB */
                    Ret = FALSE;
                    Offset.QuadPart = 0x0;

                    KmtStartSeh();
                    Ret = CcMapData(TestFileObject, &Offset, 0x1000 + VACB_MAPPING_GRANULARITY, 0, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_SUCCESS);
                    ok(Ret == FALSE, "CcMapData succeed\n");

                    if (Ret)
                    {
                        CcUnpinData(Bcb);
                    }
                }
            }
        }
    }
}


static
VOID
CleanupTest(
    ULONG TestId,
    PDEVICE_OBJECT DeviceObject)
{
    LARGE_INTEGER Zero = RTL_CONSTANT_LARGE_INTEGER(0LL);
    CACHE_UNINITIALIZE_EVENT CacheUninitEvent;

    ok_eq_pointer(TestDeviceObject, DeviceObject);
    ok_eq_ulong(TestTestId, TestId);

    if (!skip(TestFileObject != NULL, "No test FO\n"))
    {
        if (CcIsFileCached(TestFileObject))
        {
            KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
            CcUninitializeCacheMap(TestFileObject, &Zero, &CacheUninitEvent);
            KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        }

        if (TestFileObject->FsContext != NULL)
        {
            ExFreePool(TestFileObject->FsContext);
            TestFileObject->FsContext = NULL;
            TestFileObject->SectionObjectPointer = NULL;
        }

        ObDereferenceObject(TestFileObject);
    }

    TestFileObject = NULL;
    TestDeviceObject = NULL;
    TestTestId = -1;
}


static
NTSTATUS
TestMessageHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ControlCode,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T InLength,
    _Inout_ PSIZE_T OutLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    switch (ControlCode)
    {
        case IOCTL_START_TEST:
            ok_eq_ulong((ULONG)InLength, sizeof(ULONG));
            PerformTest(*(PULONG)Buffer, DeviceObject);
            break;
            
        case IOCTL_FINISH_TEST:
            ok_eq_ulong((ULONG)InLength, sizeof(ULONG));
            CleanupTest(*(PULONG)Buffer, DeviceObject);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    FsRtlExitFileSystem();

    return Status;
}

static
NTSTATUS
TestIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStack->MajorFunction, IoStack->MinorFunction);
    ASSERT(IoStack->MajorFunction == IRP_MJ_READ);

    FsRtlEnterFileSystem();

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_READ)
    {
        PMDL Mdl;
        ULONG Length;
        PVOID Buffer;
        LARGE_INTEGER Offset;

        Offset = IoStack->Parameters.Read.ByteOffset;
        Length = IoStack->Parameters.Read.Length;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);

        ok(FlagOn(Irp->Flags, IRP_NOCACHE), "Not coming from Cc\n");

        ok_irql(APC_LEVEL);
        ok((Offset.QuadPart % PAGE_SIZE == 0 || Offset.QuadPart == 0), "Offset is not aligned: %I64i\n", Offset.QuadPart);
        ok(Length % PAGE_SIZE == 0, "Length is not aligned: %I64i\n", Length);

        ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
        Buffer = MapAndLockUserBuffer(Irp, Length);
        ok(Buffer != NULL, "Null pointer!\n");
        RtlFillMemory(Buffer, Length, 0xBA);

        Status = STATUS_SUCCESS;
        if (Offset.QuadPart <= 0x3000 && Offset.QuadPart + Length > 0x3000)
        {
            *(PULONG)((ULONG_PTR)Buffer + (ULONG_PTR)(0x3000 - Offset.QuadPart)) = 0xDEADBABE;
        }

        Mdl = Irp->MdlAddress;
        ok(Mdl != NULL, "Null pointer for MDL!\n");
        ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
        ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
        ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) != 0, "Non paging IO\n");
        ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");

        Irp->IoStatus.Information = Length;
    }

    if (Status == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        Status = STATUS_PENDING;
    }
    else
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    FsRtlExitFileSystem();

    return Status;
}
