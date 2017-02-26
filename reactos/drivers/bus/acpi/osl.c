/*******************************************************************************
*                                                                              *
* ACPI Component Architecture Operating System Layer (OSL) for ReactOS         *
*                                                                              *
*******************************************************************************/

#include "precomp.h"

#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

static PKINTERRUPT AcpiInterrupt;
static BOOLEAN AcpiInterruptHandlerRegistered = FALSE;
static ACPI_OSD_HANDLER AcpiIrqHandler = NULL;
static PVOID AcpiIrqContext = NULL;
static ULONG AcpiIrqNumber = 0;

ACPI_STATUS
AcpiOsInitialize (void)
{
    DPRINT("AcpiOsInitialize called\n");

#ifndef NDEBUG
    /* Verboseness level of the acpica core */
    AcpiDbgLevel = 0x00FFFFFF;
    AcpiDbgLayer = 0xFFFFFFFF;
#endif

    return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate(void)
{
    DPRINT("AcpiOsTerminate() called\n");

    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
    ACPI_PHYSICAL_ADDRESS pa = 0;

    DPRINT("AcpiOsGetRootPointer\n");

    AcpiFindRootPointer(&pa);
    return pa;
}

ACPI_STATUS
AcpiOsPredefinedOverride(
    const ACPI_PREDEFINED_NAMES *PredefinedObject,
    ACPI_STRING                 *NewValue)
{
    if (!PredefinedObject || !NewValue)
    {
        DPRINT1("Invalid parameter\n");
        return AE_BAD_PARAMETER;
    }

    /* No override */
    *NewValue = NULL;

    return AE_OK;
}

ACPI_STATUS
AcpiOsTableOverride(
    ACPI_TABLE_HEADER *ExistingTable,
    ACPI_TABLE_HEADER **NewTable)
{
    if (!ExistingTable || !NewTable)
    {
        DPRINT1("Invalid parameter\n");
        return AE_BAD_PARAMETER;
    }

    /* No override */
    *NewTable = NULL;

    return AE_OK;
}

ACPI_STATUS
AcpiOsPhysicalTableOverride(
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{
    if (!ExistingTable || !NewAddress || !NewTableLength)
    {
        DPRINT1("Invalid parameter\n");
        return AE_BAD_PARAMETER;
    }

    /* No override */
    *NewAddress     = 0;
    *NewTableLength = 0;

    return AE_OK;
}

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   phys,
    ACPI_SIZE               length)
{
    PHYSICAL_ADDRESS Address;
    PVOID Ptr;

    DPRINT("AcpiOsMapMemory(phys 0x%p  size 0x%X)\n", phys, length);

    Address.QuadPart = (ULONG)phys;
    Ptr = MmMapIoSpace(Address, length, MmNonCached);
    if (!Ptr)
    {
        DPRINT1("Mapping failed\n");
    }

    return Ptr;
}

void
AcpiOsUnmapMemory (
    void                    *virt,
    ACPI_SIZE               length)
{
    DPRINT("AcpiOsMapMemory(phys 0x%p  size 0x%X)\n", virt, length);

    ASSERT(virt);

    MmUnmapIoSpace(virt, length);
}

ACPI_STATUS
AcpiOsGetPhysicalAddress(
    void *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
    PHYSICAL_ADDRESS PhysAddr;

    if (!LogicalAddress || !PhysicalAddress)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    PhysAddr = MmGetPhysicalAddress(LogicalAddress);

    *PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)PhysAddr.QuadPart;

    return AE_OK;
}

void *
AcpiOsAllocate (ACPI_SIZE size)
{
    DPRINT("AcpiOsAllocate size %d\n",size);
    return ExAllocatePoolWithTag(NonPagedPool, size, 'ipcA');
}

void
AcpiOsFree(void *ptr)
{
    if (!ptr)
        DPRINT1("Attempt to free null pointer!!!\n");
    ExFreePoolWithTag(ptr, 'ipcA');
}

BOOLEAN
AcpiOsReadable(
    void *Memory,
    ACPI_SIZE Length)
{
    BOOLEAN Ret = FALSE;

    _SEH2_TRY
    {
        ProbeForRead(Memory, Length, sizeof(UCHAR));
        Ret = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    return Ret;
}

BOOLEAN
AcpiOsWritable(
    void *Memory,
    ACPI_SIZE Length)
{
    BOOLEAN Ret = FALSE;

    _SEH2_TRY
    {
        ProbeForWrite(Memory, Length, sizeof(UCHAR));
        Ret = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    return Ret;
}

ACPI_THREAD_ID
AcpiOsGetThreadId (void)
{
    /* Thread ID must be non-zero */
    return (ULONG)PsGetCurrentThreadId() + 1;
}

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    DPRINT("AcpiOsExecute\n");

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  (PKSTART_ROUTINE)Function,
                                  Context);
    if (!NT_SUCCESS(Status))
        return AE_ERROR;

    ZwClose(ThreadHandle);

    return AE_OK;
}

void
AcpiOsSleep (UINT64 milliseconds)
{
    DPRINT("AcpiOsSleep %d\n", milliseconds);
    KeStallExecutionProcessor(milliseconds*1000);
}

void
AcpiOsStall (UINT32 microseconds)
{
    DPRINT("AcpiOsStall %d\n",microseconds);
    KeStallExecutionProcessor(microseconds);
}

ACPI_STATUS
AcpiOsCreateMutex(
    ACPI_MUTEX *OutHandle)
{
    PFAST_MUTEX Mutex;

    if (!OutHandle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    Mutex = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), 'LpcA');
    if (!Mutex) return AE_NO_MEMORY;

    ExInitializeFastMutex(Mutex);

    *OutHandle = (ACPI_MUTEX)Mutex;

    return AE_OK;
}

void
AcpiOsDeleteMutex(
    ACPI_MUTEX Handle)
{
    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return;
    }

    ExFreePoolWithTag(Handle, 'LpcA');
}

ACPI_STATUS
AcpiOsAcquireMutex(
    ACPI_MUTEX Handle,
    UINT16 Timeout)
{
    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    /* Check what the caller wants us to do */
    if (Timeout == ACPI_DO_NOT_WAIT)
    {
        /* Try to acquire without waiting */
        if (!ExTryToAcquireFastMutex((PFAST_MUTEX)Handle))
            return AE_TIME;
    }
    else
    {
        /* Block until we get it */
        ExAcquireFastMutex((PFAST_MUTEX)Handle);
    }

    return AE_OK;
}

void
AcpiOsReleaseMutex(
    ACPI_MUTEX Handle)
{
    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return;
    }

    ExReleaseFastMutex((PFAST_MUTEX)Handle);
}

typedef struct _ACPI_SEM {
    UINT32 CurrentUnits;
    KEVENT Event;
    KSPIN_LOCK Lock;
} ACPI_SEM, *PACPI_SEM;

ACPI_STATUS
AcpiOsCreateSemaphore(
    UINT32 MaxUnits,
    UINT32 InitialUnits,
    ACPI_SEMAPHORE *OutHandle)
{
    PACPI_SEM Sem;

    if (!OutHandle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    Sem = ExAllocatePoolWithTag(NonPagedPool, sizeof(ACPI_SEM), 'LpcA');
    if (!Sem) return AE_NO_MEMORY;

    Sem->CurrentUnits = InitialUnits;
    KeInitializeEvent(&Sem->Event, SynchronizationEvent, Sem->CurrentUnits != 0);
    KeInitializeSpinLock(&Sem->Lock);

    *OutHandle = (ACPI_SEMAPHORE)Sem;

    return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore(
    ACPI_SEMAPHORE Handle)
{
    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    ExFreePoolWithTag(Handle, 'LpcA');

    return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore(
    ACPI_SEMAPHORE Handle,
    UINT32 Units,
    UINT16 Timeout)
{
    PACPI_SEM Sem = Handle;
    KIRQL OldIrql;

    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    KeAcquireSpinLock(&Sem->Lock, &OldIrql);

    /* Make sure we can wait if we have fewer units than we need */
    if ((Timeout == ACPI_DO_NOT_WAIT) && (Sem->CurrentUnits < Units))
    {
        /* We can't so we must bail now */
        KeReleaseSpinLock(&Sem->Lock, OldIrql);
        return AE_TIME;
    }

    /* Time to block until we get enough units */
    while (Sem->CurrentUnits < Units)
    {
        KeReleaseSpinLock(&Sem->Lock, OldIrql);
        KeWaitForSingleObject(&Sem->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        KeAcquireSpinLock(&Sem->Lock, &OldIrql);
    }

    Sem->CurrentUnits -= Units;

    if (Sem->CurrentUnits != 0) KeSetEvent(&Sem->Event, IO_NO_INCREMENT, FALSE);

    KeReleaseSpinLock(&Sem->Lock, OldIrql);

    return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore(
    ACPI_SEMAPHORE Handle,
    UINT32 Units)
{
    PACPI_SEM Sem = Handle;
    KIRQL OldIrql;

    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    KeAcquireSpinLock(&Sem->Lock, &OldIrql);

    Sem->CurrentUnits += Units;
    KeSetEvent(&Sem->Event, IO_NO_INCREMENT, FALSE);

    KeReleaseSpinLock(&Sem->Lock, OldIrql);

    return AE_OK;
}

ACPI_STATUS
AcpiOsCreateLock(
    ACPI_SPINLOCK *OutHandle)
{
    PKSPIN_LOCK SpinLock;

    if (!OutHandle)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    SpinLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSPIN_LOCK), 'LpcA');
    if (!SpinLock) return AE_NO_MEMORY;

    KeInitializeSpinLock(SpinLock);

    *OutHandle = (ACPI_SPINLOCK)SpinLock;

    return AE_OK;
}

void
AcpiOsDeleteLock(
    ACPI_SPINLOCK Handle)
{
    if (!Handle)
    {
        DPRINT1("Bad parameter\n");
        return;
    }

    ExFreePoolWithTag(Handle, 'LpcA');
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock(
    ACPI_SPINLOCK Handle)
{
    KIRQL OldIrql;

    if ((OldIrql = KeGetCurrentIrql()) >= DISPATCH_LEVEL)
    {
        KeAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)Handle);
    }
    else
    {
        KeAcquireSpinLock((PKSPIN_LOCK)Handle, &OldIrql);
    }

    return (ACPI_CPU_FLAGS)OldIrql;
}

void
AcpiOsReleaseLock(
    ACPI_SPINLOCK Handle,
    ACPI_CPU_FLAGS Flags)
{
    KIRQL OldIrql = (KIRQL)Flags;

    if (OldIrql >= DISPATCH_LEVEL)
    {
        KeReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)Handle);
    }
    else
    {
        KeReleaseSpinLock((PKSPIN_LOCK)Handle, OldIrql);
    }
}

BOOLEAN NTAPI
OslIsrStub(
  PKINTERRUPT Interrupt,
  PVOID ServiceContext)
{
  INT32 Status;

  Status = (*AcpiIrqHandler)(AcpiIrqContext);

  if (Status == ACPI_INTERRUPT_HANDLED)
    return TRUE;
  else
    return FALSE;
}

UINT32
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
    ULONG Vector;
    KIRQL DIrql;
    KAFFINITY Affinity;
    NTSTATUS Status;

    if (AcpiInterruptHandlerRegistered)
    {
        DPRINT1("Reregister interrupt attempt failed\n");
        return AE_ALREADY_EXISTS;
    }

    if (!ServiceRoutine)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    DPRINT("AcpiOsInstallInterruptHandler()\n");
    Vector = HalGetInterruptVector(
        Internal,
        0,
        InterruptNumber,
        InterruptNumber,
        &DIrql,
        &Affinity);

    AcpiIrqNumber = InterruptNumber;
    AcpiIrqHandler = ServiceRoutine;
    AcpiIrqContext = Context;
    AcpiInterruptHandlerRegistered = TRUE;

    Status = IoConnectInterrupt(
        &AcpiInterrupt,
        OslIsrStub,
        NULL,
        NULL,
        Vector,
        DIrql,
        DIrql,
        LevelSensitive,
        TRUE,
        Affinity,
        FALSE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not connect to interrupt %d\n", Vector);
        return AE_ERROR;
    }
    return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
    DPRINT("AcpiOsRemoveInterruptHandler()\n");

    if (!ServiceRoutine)
    {
        DPRINT1("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    if (AcpiInterruptHandlerRegistered)
    {
        IoDisconnectInterrupt(AcpiInterrupt);
        AcpiInterrupt = NULL;
        AcpiInterruptHandlerRegistered = FALSE;
    }
    else
    {
        DPRINT1("Trying to remove non-existing interrupt handler\n");
        return AE_NOT_EXIST;
    }

    return AE_OK;
}

ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{
    DPRINT("AcpiOsReadMemory %p\n", Address);
    switch (Width)
    {
    case 8:
        *Value = (*(PUCHAR)(ULONG_PTR)Address);
        break;

    case 16:
        *Value = (*(PUSHORT)(ULONG_PTR)Address);
        break;

    case 32:
        *Value = (*(PULONG)(ULONG_PTR)Address);
        break;

    case 64:
        *Value = (*(PULONGLONG)(ULONG_PTR)Address);
        break;

    default:
        DPRINT1("AcpiOsReadMemory got bad width: %d\n",Width);
        return (AE_BAD_PARAMETER);
        break;
    }
    return (AE_OK);
}

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
    DPRINT("AcpiOsWriteMemory %p\n", Address);
    switch (Width)
    {
    case 8:
        *(PUCHAR)(ULONG_PTR)Address = Value;
        break;

    case 16:
        *(PUSHORT)(ULONG_PTR)Address = Value;
        break;

    case 32:
        *(PULONG)(ULONG_PTR)Address = Value;
        break;

    case 64:
        *(PULONGLONG)(ULONG_PTR)Address = Value;
        break;

    default:
        DPRINT1("AcpiOsWriteMemory got bad width: %d\n",Width);
        return (AE_BAD_PARAMETER);
        break;
    }

    return (AE_OK);
}

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
    DPRINT("AcpiOsReadPort %p, width %d\n",Address,Width);

    switch (Width)
    {
    case 8:
        *Value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Address);
        break;

    case 16:
        *Value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)Address);
        break;

    case 32:
        *Value = READ_PORT_ULONG((PULONG)(ULONG_PTR)Address);
        break;

    default:
        DPRINT1("AcpiOsReadPort got bad width: %d\n",Width);
        return (AE_BAD_PARAMETER);
        break;
    }
    return (AE_OK);
}

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{
    DPRINT("AcpiOsWritePort %p, width %d\n",Address,Width);
    switch (Width)
    {
    case 8:
        WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Address, Value);
        break;

    case 16:
        WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)Address, Value);
        break;

    case 32:
        WRITE_PORT_ULONG((PULONG)(ULONG_PTR)Address, Value);
        break;

    default:
        DPRINT1("AcpiOsWritePort got bad width: %d\n",Width);
        return (AE_BAD_PARAMETER);
        break;
    }
    return (AE_OK);
}

BOOLEAN
OslIsPciDevicePresent(ULONG BusNumber, ULONG SlotNumber)
{
    UINT32 ReadLength;
    PCI_COMMON_CONFIG PciConfig;

    /* Detect device presence by reading the PCI configuration space */

    ReadLength = HalGetBusDataByOffset(PCIConfiguration,
                                       BusNumber,
                                       SlotNumber,
                                       &PciConfig,
                                       0,
                                       sizeof(PciConfig));
    if (ReadLength == 0)
    {
        DPRINT("PCI device is not present\n");
        return FALSE;
    }

    ASSERT(ReadLength >= 2);

    if (PciConfig.VendorID == PCI_INVALID_VENDORID)
    {
        DPRINT("Invalid vendor ID in PCI configuration space\n");
        return FALSE;
    }

    DPRINT("PCI device is present\n");

    return TRUE;
}

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{
    PCI_SLOT_NUMBER slot;

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = PciId->Device;
    slot.u.bits.FunctionNumber = PciId->Function;

    DPRINT("AcpiOsReadPciConfiguration, slot=0x%X, func=0x%X\n", slot.u.AsULONG, Reg);

    if (!OslIsPciDevicePresent(PciId->Bus, slot.u.AsULONG))
        return AE_NOT_FOUND;

    /* Width is in BITS */
    HalGetBusDataByOffset(PCIConfiguration,
        PciId->Bus,
        slot.u.AsULONG,
        Value,
        Reg,
        (Width >> 3));

    return AE_OK;
}

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID              *PciId,
    UINT32                   Reg,
    UINT64                   Value,
    UINT32                   Width)
{
    ULONG buf = Value;
    PCI_SLOT_NUMBER slot;

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = PciId->Device;
    slot.u.bits.FunctionNumber = PciId->Function;

    DPRINT("AcpiOsWritePciConfiguration, slot=0x%x\n", slot.u.AsULONG);
    if (!OslIsPciDevicePresent(PciId->Bus, slot.u.AsULONG))
        return AE_NOT_FOUND;

    /* Width is in BITS */
    HalSetBusDataByOffset(PCIConfiguration,
        PciId->Bus,
        slot.u.AsULONG,
        &buf,
        Reg,
        (Width >> 3));

    return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Fmt,
    ...)
{
    va_list                 Args;
    va_start (Args, Fmt);

    AcpiOsVprintf (Fmt, Args);

    va_end (Args);
    return;
}

void
AcpiOsVprintf (
    const char              *Fmt,
    va_list                 Args)
{
#ifndef NDEBUG
    vDbgPrintEx (-1, DPFLTR_ERROR_LEVEL, Fmt, Args);
#endif
    return;
}

void
AcpiOsRedirectOutput(
    void *Destination)
{
    /* No-op */
    DPRINT1("Output redirection not supported\n");
}

UINT64
AcpiOsGetTimer(
    void)
{
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime(&CurrentTime);
    return CurrentTime.QuadPart;
}

void
AcpiOsWaitEventsComplete(void)
{
    /*
     * Wait for all asynchronous events to complete.
     * This implementation does nothing.
     */
    return;
}

ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
    ACPI_SIGNAL_FATAL_INFO *FatalInfo = Info;

    switch (Function)
    {
    case ACPI_SIGNAL_FATAL:
        if (Info)
            DPRINT1 ("AcpiOsBreakpoint: %d %d %d ****\n", FatalInfo->Type, FatalInfo->Code, FatalInfo->Argument);
        else
            DPRINT1 ("AcpiOsBreakpoint ****\n");
        break;
    case ACPI_SIGNAL_BREAKPOINT:
        if (Info)
            DPRINT1 ("AcpiOsBreakpoint: %s ****\n", Info);
        else
            DPRINT1 ("AcpiOsBreakpoint ****\n");
        break;
    }

    ASSERT(FALSE);

    return (AE_OK);
}

ACPI_STATUS
AcpiOsEnterSleep(
    UINT8 SleepState,
    UINT32 RegaValue,
    UINT32 RegbValue)
{
    DPRINT1("Entering sleep state S%u.\n", SleepState);
    return AE_OK;
}

ACPI_STATUS
AcpiOsGetLine(
    char *Buffer,
    UINT32 BufferLength,
    UINT32 *BytesRead)
{
    DPRINT1("File reading not supported\n");
    return AE_ERROR;
}
