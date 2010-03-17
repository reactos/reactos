/*******************************************************************************
*                                                                              *
* ACPI Component Architecture Operating System Layer (OSL) for ReactOS         *
*                                                                              *
*******************************************************************************/
#include <ntddk.h>

#include <acpi.h>

#define NDEBUG
#include <debug.h>

#define NUM_SEMAPHORES      128

static PKINTERRUPT AcpiInterrupt;
static BOOLEAN AcpiInterruptHandlerRegistered = FALSE;
static ACPI_OSD_HANDLER AcpiIrqHandler = NULL;
static PVOID AcpiIrqContext = NULL;
static ULONG AcpiIrqNumber = 0;
static KDPC AcpiDpc;
static PVOID IVTVirtualAddress = NULL;


typedef struct semaphore_entry
{
    UINT16                  MaxUnits;
    UINT16                  CurrentUnits;
    void                    *OsHandle;
} SEMAPHORE_ENTRY;

static SEMAPHORE_ENTRY AcpiGbl_Semaphores[NUM_SEMAPHORES];

VOID NTAPI
OslDpcStub(
  IN PKDPC Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2)
{
    ACPI_OSD_EXEC_CALLBACK Routine = (ACPI_OSD_EXEC_CALLBACK)SystemArgument1;

    DPRINT("OslDpcStub()\n");
    DPRINT("Calling [%p]([%p])\n", Routine, SystemArgument2);
    (*Routine)(SystemArgument2);
}

BOOLEAN NTAPI
OslIsrStub(
  PKINTERRUPT Interrupt,
  PVOID ServiceContext)
{
  INT32 Status;

  Status = (*AcpiIrqHandler)(AcpiIrqContext);

  if (ACPI_SUCCESS(Status))
    return TRUE;
  else
    return FALSE;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine);

ACPI_STATUS
AcpiOsInitialize (void)
{
    DPRINT("AcpiOsInitialize called\n");

#ifndef NDEBUG
    /* Verboseness level of the acpica core */
    AcpiDbgLevel = 0x00FFFFFF;
    AcpiDbgLayer = 0xFFFFFFFF;
#endif

    UINT32 i;

    for (i = 0; i < NUM_SEMAPHORES; i++)
    {
        AcpiGbl_Semaphores[i].OsHandle = NULL;
    }

    KeInitializeDpc(&AcpiDpc, OslDpcStub, NULL);

    return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate(void)
{
    DPRINT("AcpiOsTerminate() called\n");

    if (AcpiInterruptHandlerRegistered)
        AcpiOsRemoveInterruptHandler(AcpiIrqNumber, AcpiIrqHandler);

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
    vDbgPrintEx (-1, DPFLTR_ERROR_LEVEL, Fmt, Args);
    return;
}

void *
AcpiOsAllocate (ACPI_SIZE size)
{
    DPRINT("AcpiOsAllocate size %d\n",size);
    return ExAllocatePool(NonPagedPool, size);
}

void *
AcpiOsCallocate(ACPI_SIZE size)
{
  PVOID ptr = ExAllocatePool(NonPagedPool, size);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

void
AcpiOsFree(void *ptr)
{
    if (!ptr)
        DPRINT1("Attempt to free null pointer!!!\n");	
    ExFreePool(ptr);
}

#ifndef ACPI_USE_LOCAL_CACHE

void*
AcpiOsAcquireObjectHelper (
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes,
    ULONG Tag)
{
    void* Alloc = ExAllocatePool(PoolType, NumberOfBytes);

    /* acpica expects memory allocated from cache to be zeroed */
    RtlZeroMemory(Alloc,NumberOfBytes);
    return Alloc;
}

ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache)
{
    PNPAGED_LOOKASIDE_LIST Lookaside = 
        ExAllocatePool(NonPagedPool,sizeof(NPAGED_LOOKASIDE_LIST));

    ExInitializeNPagedLookasideList(Lookaside,
        (PALLOCATE_FUNCTION)AcpiOsAcquireObjectHelper,// custom memory allocator
        NULL,
        0,
        ObjectSize,
        'IPCA',
        0);
    *ReturnCache = (ACPI_CACHE_T *)Lookaside;

    DPRINT("AcpiOsCreateCache %p\n", Lookaside);
    return (AE_OK);
}

ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T            *Cache)
{
    DPRINT("AcpiOsDeleteCache %p\n", Cache);
    ExDeleteNPagedLookasideList(
        (PNPAGED_LOOKASIDE_LIST) Cache);
    ExFreePool(Cache);
    return (AE_OK);
}

ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache)
{
    DPRINT("AcpiOsPurgeCache\n");
    /* No such functionality for LookAside lists */
    return (AE_OK);
}

void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache)
{
	PNPAGED_LOOKASIDE_LIST List = (PNPAGED_LOOKASIDE_LIST)Cache;
    DPRINT("AcpiOsAcquireObject from %p\n", Cache);
    void* ptr = 
        ExAllocateFromNPagedLookasideList(List);
    ASSERT(ptr);

	RtlZeroMemory(ptr,List->L.Size);
    return ptr;
}

ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{
    DPRINT("AcpiOsReleaseObject %p from %p\n",Object, Cache);
    ExFreeToNPagedLookasideList(
        (PNPAGED_LOOKASIDE_LIST)Cache,
        Object);
    return (AE_OK);
}

#endif

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   phys,
    ACPI_SIZE               length)
{
    PHYSICAL_ADDRESS Address;

    DPRINT("AcpiOsMapMemory(phys 0x%X  size 0x%X)\n", (ULONG)phys, length);
    if (phys == 0x0)
    {
        IVTVirtualAddress = ExAllocatePool(NonPagedPool, length);
        return IVTVirtualAddress;
    }

    Address.QuadPart = (ULONG)phys;
    return MmMapIoSpace(Address, length, MmNonCached);
}

void
AcpiOsUnmapMemory (
    void                    *virt,
    ACPI_SIZE               length)
{
    DPRINT("AcpiOsUnmapMemory()\n");

    if (virt == 0x0)
    {
        ExFreePool(IVTVirtualAddress);
        return;
    }
    MmUnmapIoSpace(virt, length);
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

    DPRINT("AcpiOsInstallInterruptHandler()\n");
    Vector = HalGetInterruptVector(
        Internal,
        0,
        InterruptNumber,
        0,
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
        LevelSensitive, /* FIXME: LevelSensitive or Latched? */
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
    if (AcpiInterruptHandlerRegistered)
    {
        IoDisconnectInterrupt(AcpiInterrupt);
        AcpiInterrupt = NULL;
        AcpiInterruptHandlerRegistered = FALSE;
    }

    return AE_OK;
}

void
AcpiOsStall (UINT32 microseconds)
{
    DPRINT("AcpiOsStall %d\n",microseconds);
    KeStallExecutionProcessor(microseconds);
    return;
}

void
AcpiOsSleep (ACPI_INTEGER milliseconds)
{
    DPRINT("AcpiOsSleep %d\n", milliseconds);
	KeStallExecutionProcessor(milliseconds*1000);
    return;
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
        *Value = READ_PORT_UCHAR((PUCHAR)Address);
        break;

    case 16:
        *Value = READ_PORT_USHORT((PUSHORT)Address);
        break;

    case 32:
        *Value = READ_PORT_ULONG((PULONG)Address);
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
        WRITE_PORT_UCHAR((PUCHAR)Address, Value);
        break;

    case 16:
        WRITE_PORT_USHORT((PUSHORT)Address, Value);
        break;

    case 32:
        WRITE_PORT_ULONG((PULONG)Address, Value);
        break;
    
    default:
        DPRINT1("AcpiOsWritePort got bad width: %d\n",Width);
        return (AE_BAD_PARAMETER);
        break;
    }
    return (AE_OK);
}

ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  *Value,
    UINT32                  Width)
{
    DPRINT("AcpiOsReadMemory %p\n", Address);
    switch (Width)
    {
    case 8:
        *Value = (*(PUCHAR)(ULONG)Address);
        break;
    case 16:
        *Value = (*(PUSHORT)(ULONG)Address);
        break;
    case 32:
        *Value = (*(PULONG)(ULONG)Address);
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
    UINT32                  Value,
    UINT32                  Width)
{
    DPRINT("AcpiOsWriteMemory %p\n", Address);
    switch (Width)
    {
    case 8:
        *(PUCHAR)(ULONG)Address = Value;
        break;
    case 16:
        *(PUSHORT)(ULONG)Address = Value;
        break;
    case 32:
        *(PULONG)(ULONG)Address = Value;
        break;

    default:
        DPRINT1("AcpiOsWriteMemory got bad width: %d\n",Width);
        return (AE_BAD_PARAMETER);
        break;
    }

    return (AE_OK);
}

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    void                    *Value,
    UINT32                  Width)
{
    NTSTATUS Status;
    PCI_SLOT_NUMBER slot;

    if (Register == 0)
        return AE_ERROR;

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = PciId->Device;
    slot.u.bits.FunctionNumber = PciId->Function;

    DPRINT("AcpiOsReadPciConfiguration, slot=0x%X, func=0x%X\n", slot.u.AsULONG, Register);
    Status = HalGetBusDataByOffset(PCIConfiguration,
        PciId->Bus,
        slot.u.AsULONG,
        Value,
        Register,
        Width);

    if (NT_SUCCESS(Status))
        return AE_OK;
    else
        return AE_ERROR;
}

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID              *PciId,
    UINT32                   Register,
    ACPI_INTEGER             Value,
    UINT32                   Width)
{
    NTSTATUS Status;
    ULONG buf = Value;
    PCI_SLOT_NUMBER slot;

    if (Register == 0)
        return AE_ERROR;

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = PciId->Device;
    slot.u.bits.FunctionNumber = PciId->Function;

    DPRINT("AcpiOsWritePciConfiguration, slot=0x%x\n", slot.u.AsULONG);
    Status = HalSetBusDataByOffset(PCIConfiguration,
        PciId->Bus,
        slot.u.AsULONG,
        &buf,
        Register,
        Width);

    if (NT_SUCCESS(Status))
        return AE_OK;
    else
        return AE_ERROR;
}

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32              MaxUnits,
    UINT32              InitialUnits,
    ACPI_SEMAPHORE      *OutHandle)
{
    PFAST_MUTEX Mutex;

    Mutex = ExAllocatePool(NonPagedPool, sizeof(FAST_MUTEX));
    if (!Mutex)
    return AE_NO_MEMORY;

    DPRINT("AcpiOsCreateSemaphore() at 0x%X\n", Mutex);

    ExInitializeFastMutex(Mutex);

    *OutHandle = Mutex;
    return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE      Handle)
{
    PFAST_MUTEX Mutex = (PFAST_MUTEX)Handle;

    DPRINT("AcpiOsDeleteSemaphore(handle 0x%X)\n", Handle);

    if (!Mutex)
    return AE_BAD_PARAMETER;

    ExFreePool(Mutex);
    return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore(
    ACPI_SEMAPHORE             Handle,
    UINT32                     units,
    UINT16                     timeout)
{
    PFAST_MUTEX Mutex = (PFAST_MUTEX)Handle;

    if (!Mutex || (units < 1))
    {
        DPRINT("AcpiOsWaitSemaphore(handle 0x%X, units %d) Bad parameters\n",
            Mutex, units);
        return AE_BAD_PARAMETER;
    }

    DPRINT("Waiting for semaphore %p\n", Handle);
    ASSERT(Mutex);

    ExAcquireFastMutex(Mutex);
    return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units)
{
    PFAST_MUTEX  Mutex = (PFAST_MUTEX)Handle;

    DPRINT("AcpiOsSignalSemaphore %p\n",Handle);
    ASSERT(Mutex);

    ExReleaseFastMutex(Mutex);
    return AE_OK;
}

ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
    DPRINT("AcpiOsCreateLock\n");
    return (AcpiOsCreateSemaphore (1, 1, OutHandle));
}

void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
    DPRINT("AcpiOsDeleteLock %p\n", Handle);
    AcpiOsDeleteSemaphore (Handle);
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_HANDLE             Handle)
{
    DPRINT("AcpiOsAcquireLock, %p\n", Handle);
    AcpiOsWaitSemaphore (Handle, 1, 0xFFFF);
    return (0);
}


void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
    DPRINT("AcpiOsReleaseLock %p\n",Handle);
    AcpiOsSignalSemaphore (Handle, 1);
}

ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{

    switch (Function)
    {
    case ACPI_SIGNAL_FATAL:
        if (Info)
            AcpiOsPrintf ("AcpiOsBreakpoint: %s ****\n", Info);
        else
            AcpiOsPrintf ("AcpiOsBreakpoint ****\n");
        break;
    case ACPI_SIGNAL_BREAKPOINT:
        if (Info)
            AcpiOsPrintf ("AcpiOsBreakpoint: %s ****\n", Info);
        else
            AcpiOsPrintf ("AcpiOsBreakpoint ****\n");
        break;
    }

    return (AE_OK);
}


ACPI_THREAD_ID
AcpiOsGetThreadId (void)
{
    return (ULONG)PsGetCurrentThreadId();
}

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
	DPRINT("AcpiOsExecute\n");

	KeInsertQueueDpc(&AcpiDpc, (PVOID)Function, (PVOID)Context);

#ifdef _MULTI_THREADED
    //_beginthread (Function, (unsigned) 0, Context);
#endif

    return 0;
}

UINT64
AcpiOsGetTimer (void)
{
    DPRINT("AcpiOsGetTimer\n");
    LARGE_INTEGER Timer;
    KeQueryTickCount(&Timer);

    return Timer.QuadPart;
}

void
AcpiOsDerivePciId(
    ACPI_HANDLE             rhandle,
    ACPI_HANDLE             chandle,
    ACPI_PCI_ID             **PciId)
{
    DPRINT("AcpiOsDerivePciId\n");
    return;
}

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
    if (!InitVal || !NewVal)
        return AE_BAD_PARAMETER;

    *NewVal = ACPI_OS_NAME;
    DPRINT("AcpiOsPredefinedOverride\n");
    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void);

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
    DPRINT("AcpiOsTableOverride\n");
    *NewTable = NULL;
    return (AE_OK);
}

ACPI_STATUS
AcpiOsValidateInterface (
    char                    *Interface)
{
    DPRINT("AcpiOsValidateInterface\n");
    return (AE_OK);
}

ACPI_STATUS
AcpiOsValidateAddress (
    UINT8                   SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_SIZE               Length)
{
    DPRINT("AcpiOsValidateAddress\n");
    return (AE_OK);
}

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
    DPRINT("AcpiOsGetRootPointer\n");
    ACPI_PHYSICAL_ADDRESS pa = 0;

    AcpiFindRootPointer(&pa);
    return pa;
}
