/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             hal/hal.c
 * PURPOSE:          Hardware Abstraction Layer DLL
 * PROGRAMMER:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *    01-08-2001 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <ntddk.h>
#include <ntdddisk.h>
#include <arc/arc.h>
#include <intrin.h>
#include <ndk/halfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kdfuncs.h>
#include <internal/arm/ke.h>
#include <internal/arm/intrin_i.h>

#define NDEBUG
#include <debug.h>

#undef ExAcquireFastMutex
#undef ExReleaseFastMutex
#undef ExTryToAcquireFastMutex
#undef KeAcquireSpinLock
#undef KeGetCurrentIrql
#undef KeLowerIrql
#undef KeRaiseIrql
#undef KeReleaseSpinLock

/* DATA **********************************************************************/

ULONG _KdComPortInUse = 0;

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

VOID
FASTCALL
ExAcquireFastMutex(
  PFAST_MUTEX FastMutex)
{
  UNIMPLEMENTED;
}


VOID
FASTCALL
ExReleaseFastMutex(
  PFAST_MUTEX FastMutex)
{
  UNIMPLEMENTED;
}


BOOLEAN FASTCALL
ExTryToAcquireFastMutex(
  PFAST_MUTEX FastMutex)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
NTAPI
HalAcquireDisplayOwnership(
  PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
  UNIMPLEMENTED;
}


NTSTATUS
NTAPI
HalAdjustResourceList(
  PCM_RESOURCE_LIST Resources)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}


BOOLEAN
NTAPI
HalAllProcessorsStarted(VOID)
{
  UNIMPLEMENTED;

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
HalDisplayString(
  PCH String)
{
  UNIMPLEMENTED;
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


BOOLEAN
NTAPI
HalInitSystem(
  ULONG BootPhase,
  PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  UNIMPLEMENTED;

  return TRUE;
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


BOOLEAN
NTAPI
HalQueryDisplayOwnership(VOID)
{
  UNIMPLEMENTED;

  return FALSE;
}


VOID
NTAPI
HalQueryDisplayParameters(
  OUT PULONG DispSizeX,
  OUT PULONG DispSizeY,
  OUT PULONG CursorPosX,
  OUT PULONG CursorPosY)
{
  UNIMPLEMENTED;
}


BOOLEAN
NTAPI
HalQueryRealTimeClock(
  PTIME_FIELDS Time)
{
  UNIMPLEMENTED;
  return FALSE;
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
HalReleaseDisplayOwnership(VOID)
{
  UNIMPLEMENTED;
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
  ULONG Unknown)
{
  UNIMPLEMENTED;
}


VOID
FASTCALL
HalRequestSoftwareInterrupt(
  KIRQL Request)
{
  UNIMPLEMENTED;
}

VOID FASTCALL
HalClearSoftwareInterrupt(
  IN KIRQL Request)
{
  UNIMPLEMENTED;
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


VOID
NTAPI
HalSetDisplayParameters(
  ULONG CursorPosX,
  ULONG CursorPosY)
{
  UNIMPLEMENTED;
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
KeStallExecutionProcessor(
  ULONG Microseconds)
{
  UNIMPLEMENTED;
}

VOID
FASTCALL
KfLowerIrql(
  KIRQL NewIrql)
{
    KeGetPcr()->CurrentIrql = NewIrql;
    UNIMPLEMENTED;
}


KIRQL
FASTCALL
KfRaiseIrql(
  KIRQL NewIrql)
{
    KIRQL OldIrql = KeGetPcr()->CurrentIrql;
    KeGetPcr()->CurrentIrql = NewIrql;
    UNIMPLEMENTED;

    return OldIrql;
}

VOID
NTAPI
READ_PORT_BUFFER_UCHAR(
  PUCHAR Port,
  PUCHAR Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
READ_PORT_BUFFER_ULONG(
  PULONG Port,
  PULONG Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
READ_PORT_BUFFER_USHORT(
  PUSHORT Port,
  PUSHORT Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


UCHAR
NTAPI
READ_PORT_UCHAR(
  PUCHAR Port)
{
  UNIMPLEMENTED;

  return 0;
}


ULONG
NTAPI
READ_PORT_ULONG(
  PULONG Port)
{
  UNIMPLEMENTED;

  return 0;
}


USHORT
NTAPI
READ_PORT_USHORT(
  PUSHORT Port)
{
  UNIMPLEMENTED;

  return 0;
}


VOID
NTAPI
WRITE_PORT_BUFFER_UCHAR(
  PUCHAR Port,
  PUCHAR Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
WRITE_PORT_BUFFER_USHORT(
  PUSHORT Port,
  PUSHORT Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
WRITE_PORT_BUFFER_ULONG(
  PULONG Port,
  PULONG Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
NTAPI
WRITE_PORT_UCHAR(
  PUCHAR Port,
  UCHAR Value)
{
  UNIMPLEMENTED;
}

VOID
NTAPI
WRITE_PORT_ULONG(
  PULONG Port,
  ULONG Value)
{
  UNIMPLEMENTED;
}

VOID
NTAPI
WRITE_PORT_USHORT(
  PUSHORT Port,
  USHORT Value)
{
  UNIMPLEMENTED;
}

KIRQL
KeSwapIrql(IN KIRQL Irql)
{
    KIRQL OldIrql = KeGetPcr()->CurrentIrql;
    KeGetPcr()->CurrentIrql = Irql;
    UNIMPLEMENTED;
    return OldIrql;
}

KIRQL
KeRaiseIrqlToDpcLevel(VOID)
{
    KIRQL OldIrql = KeGetPcr()->CurrentIrql;
    KeGetPcr()->CurrentIrql = SYNCH_LEVEL;
    UNIMPLEMENTED;
    return OldIrql;
}

KIRQL
KeRaiseIrqlToSynchLevel(VOID)
{
    KIRQL OldIrql = KeGetPcr()->CurrentIrql;
    KeGetPcr()->CurrentIrql = SYNCH_LEVEL;
    UNIMPLEMENTED;
    return OldIrql;
}

BOOLEAN HalpProcessorIdentified;
BOOLEAN HalpTestCleanSupported;

VOID
HalpIdentifyProcessor(VOID)
{
    ARM_ID_CODE_REGISTER IdRegister;

    //
    // Don't do it again
    //
    HalpProcessorIdentified = TRUE;
    
    //
    // Read the ID Code
    //
    IdRegister = KeArmIdCodeRegisterGet();
    
    //
    // Architecture "6" CPUs support test-and-clean (926EJ-S and 1026EJ-S)
    //
    HalpTestCleanSupported = (IdRegister.Architecture == 6);
}


VOID
HalSweepDcache(VOID)
{
    //
    // We get called very early on, before HalInitSystem or any of the Hal*
    // processor routines, so we need to figure out what CPU we're on.
    //
    if (!HalpProcessorIdentified) HalpIdentifyProcessor();
    
    //
    // Check if we can do it the ARMv5TE-J way
    //
    if (HalpTestCleanSupported)
    {
        //
        // Test, clean, flush D-Cache
        //
        __asm__ __volatile__ ("1: mrc p15, 0, pc, c7, c14, 3; bne 1b");
    }
    else
    {
        //
        // We need to do it it by set/way
        //
        UNIMPLEMENTED;
    }
}

VOID
HalSweepIcache(VOID)
{
    //
    // All ARM cores support the same Icache flush command, no need for HAL work
    //
    KeArmFlushIcache();
}

/*
 * @implemented
 */
KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
    /* Return IRQL */
    return PCR->CurrentIrql;
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
    LockHandle->OldIrql = KfRaiseIrql(SYNCH_LEVEL);
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
