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

#define NDEBUG
#include <debug.h>

#undef ExAcquireFastMutex
#undef ExReleaseFastMutex
#undef ExTryToAcquireFastMutex

/* DATA **********************************************************************/

PUCHAR KdComPortInUse;

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
    ASSERT(FALSE);
    return;
}

/*
* @unimplemented
*/
VOID
NTAPI
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    ASSERT(FALSE);
    return;
}

/*
* @unimplemented
*/
ULONG_PTR
NTAPI
HalSetProfileInterval(IN ULONG_PTR Interval)
{
    ASSERT(FALSE);
    return Interval;
}

#ifndef _M_AMD64
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
#endif

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
HalInitializeProcessor(ULONG ProcessorNumber,
                       PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  UNIMPLEMENTED;
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
    KAFFINITY TargetSet)
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


#undef KeAcquireSpinLock
VOID
NTAPI
KeAcquireSpinLock(
  PKSPIN_LOCK SpinLock,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;
}


KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch(
  PKSPIN_LOCK SpinLock)
{
  UNIMPLEMENTED;

  return 0;
}

#ifndef _M_AMD64
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
  UNIMPLEMENTED;
}

VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
   UNIMPLEMENTED;
}

VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(
    IN PKLOCK_QUEUE_HANDLE LockHandle
    )
{
  UNIMPLEMENTED;
}
#endif

VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
  UNIMPLEMENTED;
}

#ifndef _M_AMD64
#undef KeGetCurrentIrql
KIRQL
NTAPI
KeGetCurrentIrql(VOID)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}

#undef KeLowerIrql
VOID
NTAPI
KeLowerIrql(
  KIRQL NewIrql)
{
  UNIMPLEMENTED;
}
#endif


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

#ifndef _M_AMD64
#undef KeRaiseIrql
VOID
NTAPI
KeRaiseIrql(
  KIRQL NewIrql,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;
}


KIRQL
NTAPI
KeRaiseIrqlToDpcLevel(VOID)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}


KIRQL
NTAPI
KeRaiseIrqlToSynchLevel(VOID)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}
#endif

#ifndef _M_AMD64
#undef KeReleaseSpinLock
VOID
NTAPI
KeReleaseSpinLock(
  PKSPIN_LOCK SpinLock,
  KIRQL NewIrql)
{
  UNIMPLEMENTED;
}
#endif

VOID
NTAPI
KeStallExecutionProcessor(
  ULONG Microseconds)
{
  UNIMPLEMENTED;
}


LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(
  KSPIN_LOCK_QUEUE_NUMBER LockNumber,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;

  return FALSE;
}


BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(
  KSPIN_LOCK_QUEUE_NUMBER LockNumber,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;

  return FALSE;
}

#if !defined(_M_AMD64)
KIRQL
FASTCALL
KfAcquireSpinLock(
  PKSPIN_LOCK SpinLock)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}


VOID
FASTCALL
KfLowerIrql(
  KIRQL NewIrql)
{
  UNIMPLEMENTED;
}


KIRQL
FASTCALL
KfRaiseIrql(
  KIRQL NewIrql)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}


VOID
FASTCALL
KfReleaseSpinLock(
  PKSPIN_LOCK SpinLock,
  KIRQL NewIrql)
{
  UNIMPLEMENTED;
}
#endif

#if !defined(_M_AMD64)
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
#endif

KIRQL
FASTCALL
KeAcquireQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
  UNIMPLEMENTED;
  return (KIRQL)0;
}

KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    UNIMPLEMENTED;
    return (KIRQL)0;
}

VOID
FASTCALL
KeReleaseQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle,
                        IN KIRQL OldIrql)
{
  UNIMPLEMENTED;
}

VOID
HalSweepDcache(VOID)
{
  UNIMPLEMENTED;
}

VOID
HalSweepIcache(VOID)
{
    UNIMPLEMENTED;
}

ULONG
HalGetInterruptSource(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
