/* $Id: hal.c,v 1.5 2002/09/08 10:22:24 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             hal/hal.c
 * PURPOSE:          Hardware Abstraction Layer DLL
 * PROGRAMMER:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *    01-08-2001 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

ULONG EXPORTED KdComPortInUse = 0;

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
DriverEntry(
  PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
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
STDCALL
HalAcquireDisplayOwnership(
  PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
  UNIMPLEMENTED;
}


NTSTATUS
STDCALL
HalAdjustResourceList(
  PCM_RESOURCE_LIST Resources)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}


BOOLEAN
STDCALL
HalAllProcessorsStarted(VOID)
{
  UNIMPLEMENTED;

  return TRUE;
}


NTSTATUS
STDCALL
HalAllocateAdapterChannel(
  PADAPTER_OBJECT AdapterObject,
  PDEVICE_OBJECT DeviceObject,
  ULONG NumberOfMapRegisters,
  PDRIVER_CONTROL ExecutionRoutine,
  PVOID Context)
{
  UNIMPLEMENTED;

  return STATUS_SUCCESS;
}


PVOID
STDCALL
HalAllocateCommonBuffer(
  PADAPTER_OBJECT AdapterObject,
  ULONG Length,
  PPHYSICAL_ADDRESS LogicalAddress,
  BOOLEAN CacheEnabled)
{
  UNIMPLEMENTED;

  return NULL;
}


NTSTATUS
STDCALL
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
STDCALL 
HalBeginSystemInterrupt(
  ULONG Vector,
  KIRQL Irql,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
HalCalibratePerformanceCounter(
  ULONG Count)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalDisableSystemInterrupt(
  ULONG Vector,
  ULONG Unknown2)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
HalDisplayString(
  PCH String)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalEnableSystemInterrupt(
  ULONG Vector,
  ULONG Unknown2,
  ULONG Unknown3)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
HalEndSystemInterrupt(
  KIRQL Irql,
  ULONG Unknown2)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalFlushCommonBuffer(
  ULONG Unknown1,
  ULONG Unknown2,
  ULONG Unknown3,
  ULONG Unknown4,
  ULONG Unknown5,
  ULONG Unknown6,
  ULONG Unknown7,
  ULONG Unknown8)
{
  UNIMPLEMENTED;

   return TRUE;
}


VOID
STDCALL
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
STDCALL
HalGetAdapter(
  PDEVICE_DESCRIPTION DeviceDescription,
  PULONG NumberOfMapRegisters)
{
  UNIMPLEMENTED;

  return (PADAPTER_OBJECT)NULL;
}


ULONG
STDCALL
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
STDCALL
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


BOOLEAN
STDCALL
HalGetEnvironmentVariable(
  PCH Name,
  PCH Value,
  USHORT ValueLength)
{
  UNIMPLEMENTED;

  return FALSE;
}


ULONG
STDCALL
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
STDCALL
HalHandleNMI(
  ULONG Unused)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalInitSystem(
  ULONG BootPhase,
  PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
HalInitializeProcessor(
  ULONG ProcessorNumber,
  PVOID ProcessorStack)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalMakeBeep(
  ULONG Frequency)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
HalProcessorIdle(VOID)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
HalQueryDisplayParameters(
  PULONG DispSizeX,
  PULONG DispSizeY,
  PULONG CursorPosX,
  PULONG CursorPosY)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
HalQueryRealTimeClock(
  PTIME_FIELDS Time)
{
  UNIMPLEMENTED;
}


ULONG
STDCALL
HalReadDmaCounter(
  PADAPTER_OBJECT AdapterObject)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
HalReportResourceUsage(VOID)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
HalRequestIpi(
  ULONG Unknown)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
HalReturnToFirmware(
  ULONG Action)
{
  UNIMPLEMENTED;
}


ULONG
STDCALL
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
STDCALL
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
STDCALL
HalSetDisplayParameters(
  ULONG CursorPosX,
  ULONG CursorPosY)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalSetEnvironmentVariable(
  PCH Name,
  PCH Value)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
HalSetRealTimeClock(
  PTIME_FIELDS Time)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
HalStartNextProcessor(
  ULONG Unknown1,
  ULONG Unknown2)
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
STDCALL
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
STDCALL
IoAssignDriveLetters(
  PLOADER_PARAMETER_BLOCK LoaderBlock,
  PSTRING NtDeviceName,
  PUCHAR NtSystemPath,
  PSTRING NtSystemPathString)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
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
STDCALL
IoFreeAdapterChannel(
  PADAPTER_OBJECT AdapterObject)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
IoFreeMapRegisters(
  PADAPTER_OBJECT AdapterObject,
  PVOID MapRegisterBase,
  ULONG NumberOfMapRegisters)
{
  UNIMPLEMENTED;
}


PHYSICAL_ADDRESS
STDCALL
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


BOOLEAN
STDCALL
KdPortGetByte(
  PUCHAR  ByteRecieved)
{
  UNIMPLEMENTED;

  return TRUE;
}


BOOLEAN
STDCALL
KdPortGetByteEx(
  PKD_PORT_INFORMATION PortInformation,
  PUCHAR  ByteRecieved)
{
  UNIMPLEMENTED;

  return TRUE;
}


BOOLEAN
STDCALL
KdPortInitialize(
  PKD_PORT_INFORMATION PortInformation,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED;

  return TRUE;
}


BOOLEAN
STDCALL
KdPortPollByte(
  PUCHAR  ByteRecieved)
{
  UNIMPLEMENTED;

  return TRUE;
}


BOOLEAN
STDCALL
KdPortPollByteEx(
  PKD_PORT_INFORMATION PortInformation,
  PUCHAR  ByteRecieved)
{
  UNIMPLEMENTED;

  return TRUE;
}


VOID
STDCALL
KdPortPutByte(
  UCHAR ByteToSend)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
KdPortPutByteEx(
  PKD_PORT_INFORMATION PortInformation,
  UCHAR ByteToSend)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
KdPortRestore(VOID)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
KdPortSave(VOID)
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
KdPortDisableInterrupts()
{
  UNIMPLEMENTED;
}


BOOLEAN
STDCALL
KdPortEnableInterrupts()
{
  UNIMPLEMENTED;
}


VOID
STDCALL
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
}


VOID
STDCALL
KeFlushWriteBuffer(VOID)
{
  UNIMPLEMENTED;
}


KIRQL
STDCALL 
KeGetCurrentIrql(VOID)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}


VOID
STDCALL
KeLowerIrql(
  KIRQL NewIrql)
{
  UNIMPLEMENTED;
}


LARGE_INTEGER
STDCALL
KeQueryPerformanceCounter(
  PLARGE_INTEGER PerformanceFreq)
{
  LARGE_INTEGER Value;

  UNIMPLEMENTED;

  Value.QuadPart = 0;

  return Value;
}


VOID
STDCALL
KeRaiseIrql(
  KIRQL NewIrql,
  PKIRQL OldIrql)
{
  UNIMPLEMENTED;
}


KIRQL
STDCALL
KeRaiseIrqlToDpcLevel(VOID)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}


KIRQL
STDCALL
KeRaiseIrqlToSynchLevel(VOID)
{
  UNIMPLEMENTED;

  return (KIRQL)0;
}


VOID
STDCALL
KeReleaseSpinLock(
  PKSPIN_LOCK SpinLock,
  KIRQL NewIrql)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
KeStallExecutionProcessor(
  ULONG Microseconds)
{
  UNIMPLEMENTED;
}


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


VOID
STDCALL
READ_PORT_BUFFER_UCHAR(
  PUCHAR Port,
  PUCHAR Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
READ_PORT_BUFFER_ULONG(
  PULONG Port,
  PULONG Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
READ_PORT_BUFFER_USHORT(
  PUSHORT Port,
  PUSHORT Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


UCHAR
STDCALL
READ_PORT_UCHAR(
  PUCHAR Port)
{
  UNIMPLEMENTED;

  return 0;
}


ULONG
STDCALL
READ_PORT_ULONG(
  PULONG Port)
{
  UNIMPLEMENTED;

  return 0;
}


USHORT
STDCALL
READ_PORT_USHORT(
  PUSHORT Port)
{
  UNIMPLEMENTED;

  return 0;
}


VOID
STDCALL
WRITE_PORT_BUFFER_UCHAR(
  PUCHAR Port,
  PUCHAR Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
WRITE_PORT_BUFFER_USHORT(
  PUSHORT Port,
  PUSHORT Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
WRITE_PORT_BUFFER_ULONG(
  PULONG Port,
  PULONG Buffer,
  ULONG Count)
{
  UNIMPLEMENTED;
}


VOID
STDCALL
WRITE_PORT_UCHAR(
  PUCHAR Port,
  UCHAR Value)
{
  UNIMPLEMENTED;
}

VOID
STDCALL
WRITE_PORT_ULONG(
  PULONG Port,
  ULONG Value)
{
  UNIMPLEMENTED;
}

VOID
STDCALL
WRITE_PORT_USHORT(
  PUSHORT Port,
  USHORT Value)
{
  UNIMPLEMENTED;
}

/* EOF */
