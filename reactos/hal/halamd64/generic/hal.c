/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/hal.c
 * PURPOSE:         Hardware Abstraction Layer
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>
#include <ndk/inbvfuncs.h>

#undef ExAcquireFastMutex
#undef ExReleaseFastMutex
#undef ExTryToAcquireFastMutex
#undef KeAcquireSpinLock
#undef KeLowerIrql
#undef KeRaiseIrql
#undef KeReleaseSpinLock

#define READ_REGISTER_ULONG(r) (*(volatile ULONG * const)(r))
#define WRITE_REGISTER_ULONG(r, v) (*(volatile ULONG *)(r) = (v))

/* DATA **********************************************************************/

ULONG HalpCurrentTimeIncrement, HalpNextTimeIncrement, HalpNextIntervalCount;
ULONG _KdComPortInUse = 0;

ULONG HalpIrqlTable[HIGH_LEVEL + 1] =
{
    0xFFFFFFFF, // IRQL 0 PASSIVE_LEVEL
    0xFFFFFFFD, // IRQL 1 APC_LEVEL
    0xFFFFFFF9, // IRQL 2 DISPATCH_LEVEL
    0xFFFFFFD9, // IRQL 3
    0xFFFFFF99, // IRQL 4
    0xFFFFFF19, // IRQL 5
    0xFFFFFE19, // IRQL 6
    0xFFFFFC19, // IRQL 7
    0xFFFFF819, // IRQL 8
    0xFFFFF019, // IRQL 9
    0xFFFFE019, // IRQL 10
    0xFFFFC019, // IRQL 11
    0xFFFF8019, // IRQL 12
    0xFFFF0019, // IRQL 13
    0xFFFE0019, // IRQL 14
    0xFFFC0019, // IRQL 15
};

UCHAR HalpMaskTable[HIGH_LEVEL + 1] =
{
    PROFILE_LEVEL, // INT 0 WATCHDOG
    APC_LEVEL,     // INT 1 SOFTWARE INTERRUPT
    DISPATCH_LEVEL,// INT 2 COMM RX
    IPI_LEVEL,     // INT 3 COMM TX
    CLOCK_LEVEL,  // INT 4 TIMER 0
    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10,
    11,
    12,
    13
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
DriverEntry(
  PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}

/*
* @unimplemented
*/
VOID
NTAPI
HalStopProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    KEBUGCHECK(0);
    return;
}

/*
* @unimplemented
*/
VOID
NTAPI
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    KEBUGCHECK(0);
    return;
}

/*
* @unimplemented
*/
ULONG_PTR
NTAPI
HalSetProfileInterval(IN ULONG_PTR Interval)
{
    KEBUGCHECK(0);
    return Interval;
}

NTSTATUS
NTAPI
HalAdjustResourceList(
  PCM_RESOURCE_LIST Resources)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
HalAllProcessorsStarted(VOID)
{
    /* Do nothing */
    return TRUE;
}


NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  PADAPTER_OBJECT AdapterObject,
  PWAIT_CONTEXT_BLOCK WaitContextBlock,
  ULONG NumberOfMapRegisters,
  PDRIVER_CONTROL ExecutionRoutine)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}


PVOID
NTAPI
HalAllocateCommonBuffer(
  PADAPTER_OBJECT AdapterObject,
  ULONG Length,
  PPHYSICAL_ADDRESS LogicalAddress,
  BOOLEAN CacheEnabled)
{
  UNIMPLEMENTED;

  return NULL;
}


PVOID
NTAPI
HalAllocateCrashDumpRegisters(
  PADAPTER_OBJECT AdapterObject,
  PULONG NumberOfMapRegisters)
{
  UNIMPLEMENTED;
  return NULL;
}


NTSTATUS
NTAPI
HalAssignSlotResources(
  PUNICODE_STRING RegistryPath,
  PUNICODE_STRING DriverClassName,
  PDRIVER_OBJECT DriverObject,
  PDEVICE_OBJECT DeviceObject,
  INTERFACE_TYPE BusType,
  ULONG BusNumber,
  ULONG SlotNumber,
  PCM_RESOURCE_LIST *AllocatedResources)
{
  UNIMPLEMENTED;

  return TRUE;
}


BOOLEAN
NTAPI
HalBeginSystemInterrupt (KIRQL Irql,
			 ULONG Vector,
			 PKIRQL OldIrql)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
NTAPI
HalCalibratePerformanceCounter(
  volatile LONG *Count,
  ULONGLONG NewCount)
{
  UNIMPLEMENTED;
}


BOOLEAN
NTAPI
HalDisableSystemInterrupt(
  ULONG Vector,
  KIRQL Irql)
{
  UNIMPLEMENTED;

  return TRUE;
}

VOID
NTAPI
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
    //
    // Stub since Windows XP implemented Inbv
    //
    return;
}

VOID
NTAPI
HalDisplayString(IN PCH String)
{
    //
    // Call the Inbv driver
    //
    InbvDisplayString(String);
}

VOID
NTAPI
HalQueryDisplayParameters(OUT PULONG DispSizeX,
                          OUT PULONG DispSizeY,
                          OUT PULONG CursorPosX,
                          OUT PULONG CursorPosY)
{
    //
    // Stub since Windows XP implemented Inbv
    //
    return;
}

VOID
NTAPI
HalSetDisplayParameters(IN ULONG CursorPosX,
                        IN ULONG CursorPosY)
{
    //
    // Stub since Windows XP implemented Inbv
    //
    return;
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
  ULONG Vector,
  KIRQL Irql,
  KINTERRUPT_MODE InterruptMode)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
NTAPI
HalEndSystemInterrupt(
  KIRQL Irql,
  ULONG Unknown2)
{
  UNIMPLEMENTED;
}


BOOLEAN
NTAPI
HalFlushCommonBuffer(
  ULONG Unknown1,
  ULONG Unknown2,
  ULONG Unknown3,
  ULONG Unknown4,
  ULONG Unknown5)
{
  UNIMPLEMENTED;

   return TRUE;
}


VOID
NTAPI
HalFreeCommonBuffer(
  PADAPTER_OBJECT AdapterObject,
  ULONG Length,
  PHYSICAL_ADDRESS LogicalAddress,
  PVOID VirtualAddress,
  BOOLEAN CacheEnabled)
{
  UNIMPLEMENTED;
}


PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  PDEVICE_DESCRIPTION DeviceDescription,
  PULONG NumberOfMapRegisters)
{
  UNIMPLEMENTED;

  return (PADAPTER_OBJECT)NULL;
}


ULONG
NTAPI
HalGetBusData(
  BUS_DATA_TYPE BusDataType,
  ULONG BusNumber,
  ULONG SlotNumber,
  PVOID Buffer,
  ULONG Length)
{
  UNIMPLEMENTED;

  return 0;
}


ULONG
NTAPI
HalGetBusDataByOffset(
  BUS_DATA_TYPE BusDataType,
  ULONG BusNumber,
  ULONG SlotNumber,
  PVOID Buffer,
  ULONG Offset,
  ULONG Length)
{
  UNIMPLEMENTED;

  return 0;
}


ARC_STATUS
NTAPI
HalGetEnvironmentVariable(
  PCH Name,
  USHORT ValueLength,
  PCH Value)
{
  UNIMPLEMENTED;

  return ENOENT;
}


ULONG
NTAPI
HalGetInterruptVector(
  INTERFACE_TYPE InterfaceType,
  ULONG BusNumber,
  ULONG BusInterruptLevel,
  ULONG BusInterruptVector,
  PKIRQL Irql,
  PKAFFINITY Affinity)
{
  UNIMPLEMENTED;

  return 0;
}


VOID
NTAPI
HalHandleNMI(
  PVOID NmiData)
{
  UNIMPLEMENTED;
}

VOID
NTAPI
HalpGetParameters(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine;
    
    /* Make sure we have a loader block and command line */
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        /* Read the command line */
        CommandLine = LoaderBlock->LoadOptions;
        
        /* Check for initial breakpoint */
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

ULONG
HalGetInterruptSource(VOID)
{
    KEBUGCHECK(0);
    return 0;
}

VOID
HalpClockInterrupt(VOID)
{   
    KEBUGCHECK(0);
    return;
}

VOID
HalpStallInterrupt(VOID)
{   
    KEBUGCHECK(0);
    return;
}

VOID
HalpInitializeInterrupts(VOID)
{
    KEBUGCHECK(0);
    return;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalInitSystem(IN ULONG BootPhase,
              IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    KEBUGCHECK(0);
    return;
}


VOID
NTAPI
HalInitializeProcessor(IN ULONG ProcessorNumber,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Nothing to do
    //
    return;
}


BOOLEAN
NTAPI
HalMakeBeep(
  ULONG Frequency)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
NTAPI
HalProcessorIdle(VOID)
{
  UNIMPLEMENTED;
}


#define RTC_DATA   (PVOID)0xE00E8000

BOOLEAN
NTAPI
HalQueryRealTimeClock(IN PTIME_FIELDS Time)
{
    KEBUGCHECK(0);
    return TRUE;
}

ULONG
NTAPI
HalReadDmaCounter(
  PADAPTER_OBJECT AdapterObject)
{
  UNIMPLEMENTED;

  return 0;
}


VOID
NTAPI
HalReportResourceUsage(VOID)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
HalRequestIpi(
    KAFFINITY TargetSet)
{
  UNIMPLEMENTED;
}


VOID
FASTCALL
HalRequestSoftwareInterrupt(IN KIRQL Request)
{
    KEBUGCHECK(0);
    return;
}

VOID
FASTCALL
HalClearSoftwareInterrupt(IN KIRQL Request)
{    
    KEBUGCHECK(0);
    return;
}

VOID
NTAPI
HalReturnToFirmware(
  FIRMWARE_REENTRY Action)
{
  UNIMPLEMENTED;
}


ULONG
NTAPI
HalSetBusData(
  BUS_DATA_TYPE BusDataType,
  ULONG BusNumber,
  ULONG SlotNumber,
  PVOID Buffer,
  ULONG Length)
{
  UNIMPLEMENTED;

  return 0;
}


ULONG
NTAPI
HalSetBusDataByOffset(
  BUS_DATA_TYPE BusDataType,
  ULONG BusNumber,
  ULONG SlotNumber,
  PVOID Buffer,
  ULONG Offset,
  ULONG Length)
{
  UNIMPLEMENTED;

  return 0;
}


ARC_STATUS
NTAPI
HalSetEnvironmentVariable(
  PCH Name,
  PCH Value)
{
  UNIMPLEMENTED;

  return ESUCCESS;
}


BOOLEAN
NTAPI
HalSetRealTimeClock(
  PTIME_FIELDS Time)
{
  UNIMPLEMENTED;

  return TRUE;
}


ULONG
NTAPI
HalSetTimeIncrement(
  ULONG Increment)
{
  UNIMPLEMENTED;

  return Increment;
}


BOOLEAN
NTAPI
HalStartNextProcessor(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN PKPROCESSOR_STATE ProcessorState)
{
  UNIMPLEMENTED;

  return TRUE;
}


ULONG
FASTCALL
HalSystemVectorDispatchEntry(
  ULONG Unknown1,
  ULONG Unknown2,
  ULONG Unknown3)
{
  UNIMPLEMENTED;

  return 0;
}


BOOLEAN
NTAPI
HalTranslateBusAddress(
  INTERFACE_TYPE InterfaceType,
  ULONG BusNumber,
  PHYSICAL_ADDRESS BusAddress,
  PULONG AddressSpace,
  PPHYSICAL_ADDRESS TranslatedAddress)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
NTAPI
HalpAssignDriveLetters(IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
                       IN PSTRING NtDeviceName,
                       OUT PUCHAR NtSystemPath,
                       OUT PSTRING NtSystemPathString)
{
    /* Call the kernel */
    IoAssignDriveLetters(LoaderBlock,
                                NtDeviceName,
                                NtSystemPath,
                                NtSystemPathString);
}

NTSTATUS
NTAPI
HalpReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                       IN ULONG SectorSize,
                       IN BOOLEAN ReturnRecognizedPartitions,
                       IN OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    /* Call the kernel */
    return IoReadPartitionTable(DeviceObject,
                                SectorSize,
                                ReturnRecognizedPartitions,
                                PartitionBuffer);
}

NTSTATUS
NTAPI
HalpWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                        IN ULONG SectorSize,
                        IN ULONG SectorsPerTrack,
                        IN ULONG NumberOfHeads,
                        IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    /* Call the kernel */
    return IoWritePartitionTable(DeviceObject,
                                 SectorSize,
                                 SectorsPerTrack,
                                 NumberOfHeads,
                                 PartitionBuffer);
}

NTSTATUS
NTAPI
HalpSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG SectorSize,
                            IN ULONG PartitionNumber,
                            IN ULONG PartitionType)
{
    /* Call the kernel */
    return IoSetPartitionInformation(DeviceObject,
                                     SectorSize,
                                     PartitionNumber,
                                     PartitionType);
}


BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  PADAPTER_OBJECT AdapterObject,
  PMDL Mdl,
  PVOID MapRegisterBase,
  PVOID CurrentVa,
  ULONG Length,
  BOOLEAN WriteToDevice)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
NTAPI
IoFreeAdapterChannel(
  PADAPTER_OBJECT AdapterObject)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
IoFreeMapRegisters(
  PADAPTER_OBJECT AdapterObject,
  PVOID MapRegisterBase,
  ULONG NumberOfMapRegisters)
{
  UNIMPLEMENTED;
}


PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  PADAPTER_OBJECT AdapterObject,
  PMDL Mdl,
  PVOID MapRegisterBase,
  PVOID CurrentVa,
  PULONG Length,
  BOOLEAN WriteToDevice)
{
  PHYSICAL_ADDRESS Address;

  UNIMPLEMENTED;

  Address.QuadPart = 0;

  return Address;
}

VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
  UNIMPLEMENTED;
}

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
  PLARGE_INTEGER PerformanceFreq)
{
  LARGE_INTEGER Value;

  UNIMPLEMENTED;

  Value.QuadPart = 0;

  return Value;
}

VOID
NTAPI
KeStallExecutionProcessor(IN ULONG Microseconds)
{
  UNIMPLEMENTED;
  return;
}

VOID
FASTCALL
KfLowerIrql(IN KIRQL NewIrql)
{
  UNIMPLEMENTED;
  return;
}

KIRQL
FASTCALL
KfRaiseIrql(IN KIRQL NewIrql)
{
  UNIMPLEMENTED;
  return;
}



KIRQL
KeRaiseIrqlToDpcLevel(VOID)
{
    //
    // Call the generic routine
    //
    return KfRaiseIrql(DISPATCH_LEVEL);
}

KIRQL
KeRaiseIrqlToSynchLevel(VOID)
{
    //
    // Call the generic routine
    //
    return KfRaiseIrql(DISPATCH_LEVEL);
}

BOOLEAN HalpProcessorIdentified;
BOOLEAN HalpTestCleanSupported;

VOID
HalpIdentifyProcessor(VOID)
{
  UNIMPLEMENTED;
  return;
}

VOID
HalSweepDcache(VOID)
{
  UNIMPLEMENTED;
  return;
}

VOID
HalSweepIcache(VOID)
{
  UNIMPLEMENTED;
  return;
}

/*
 * @implemented
 */
#undef KeGetCurrentIrql
KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
  UNIMPLEMENTED;
  return;
}

/*
 * @implemented
 */
VOID
NTAPI
KeLowerIrql(KIRQL NewIrql)
{
    /* Call the fastcall function */
    KfLowerIrql(NewIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
KeRaiseIrql(KIRQL NewIrql,
            PKIRQL OldIrql)
{
    /* Call the fastcall function */
    *OldIrql = KfRaiseIrql(NewIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
KeAcquireSpinLock(PKSPIN_LOCK SpinLock,
                  PKIRQL OldIrql)
{
    /* Call the fastcall function */
    *OldIrql = KfAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch(PKSPIN_LOCK SpinLock)
{
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
NTAPI
KeReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL NewIrql)
{
    /* Call the fastcall function */
    KfReleaseSpinLock(SpinLock, NewIrql);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KfAcquireSpinLock(PKSPIN_LOCK SpinLock)
{
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KfReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL OldIrql)
{
    /* Simply lower IRQL back */
    KfLowerIrql(OldIrql);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(IN PKSPIN_LOCK SpinLock,
                               IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply raise to dispatch */
    LockHandle->OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(IN PKSPIN_LOCK SpinLock,
                                           IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply raise to synch */
    LockHandle->OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                        IN KIRQL OldIrql)
{
    /* Simply lower IRQL back */
    KfLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply lower IRQL back */
    KfLowerIrql(LockHandle->OldIrql);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                                         IN PKIRQL OldIrql)
{
    /* Simply raise to dispatch */
    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    
    /* Always return true on UP Machines */
    return TRUE;
}

/*
 * @implemented
 */
LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                             OUT PKIRQL OldIrql)
{
    /* Simply raise to dispatch */
    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    
    /* Always return true on UP Machines */
    return TRUE;
}

/* EOF */
