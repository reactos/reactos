/*
 * winnt4.h
 *
 * Definitions only used in Windows NT 4.0 and earlier versions
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __WINNT4_H
#define __WINNT4_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ZONE_SEGMENT_HEADER {
  SINGLE_LIST_ENTRY  SegmentList;
  PVOID  Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
  SINGLE_LIST_ENTRY  FreeList;
  SINGLE_LIST_ENTRY  SegmentList;
  ULONG  BlockSize;
  ULONG  TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

static __inline PVOID
ExAllocateFromZone(
  IN PZONE_HEADER  Zone)
{
  if (Zone->FreeList.Next)
    Zone->FreeList.Next = Zone->FreeList.Next->Next;
  return (PVOID) Zone->FreeList.Next;
}

NTKERNELAPI
NTSTATUS
NTAPI
ExExtendZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Segment,
  IN ULONG  SegmentSize);

static __inline PVOID
ExFreeToZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Block)
{
  ((PSINGLE_LIST_ENTRY) Block)->Next = Zone->FreeList.Next;
  Zone->FreeList.Next = ((PSINGLE_LIST_ENTRY) Block);
  return ((PSINGLE_LIST_ENTRY) Block)->Next;
}

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeZone(
  IN PZONE_HEADER  Zone,
  IN ULONG  BlockSize,
  IN PVOID  InitialSegment,
  IN ULONG  InitialSegmentSize);

/*
 * PVOID
 * ExInterlockedAllocateFromZone(
 *   IN PZONE_HEADER  Zone,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedAllocateFromZone(Zone, \
                              Lock) \
  ((PVOID) ExInterlockedPopEntryList(&Zone->FreeList, Lock))

NTKERNELAPI
NTSTATUS
NTAPI
ExInterlockedExtendZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Segment,
  IN ULONG  SegmentSize,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
PVOID
NTAPI
ExInterlockedFreeToZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Block,
  IN PKSPIN_LOCK  Lock);

/*
 * VOID
 * ExInitializeWorkItem(
 *   IN PWORK_QUEUE_ITEM  Item,
 *   IN PWORKER_THREAD_ROUTINE  Routine,
 *   IN PVOID  Context)
 */
#define ExInitializeWorkItem(Item, \
                             Routine, \
                             Context) \
{ \
  (Item)->WorkerRoutine = Routine; \
  (Item)->Parameter = Context; \
  (Item)->List.Flink = NULL; \
}

/*
 * BOOLEAN
 * ExIsFullZone(
 *  IN PZONE_HEADER  Zone)
 */
#define ExIsFullZone(Zone) \
  ((Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY) NULL)

NTKERNELAPI
VOID
NTAPI
ExQueueWorkItem(
  IN PWORK_QUEUE_ITEM  WorkItem,
  IN WORK_QUEUE_TYPE  QueueType);

NTKERNELAPI
BOOLEAN
NTAPI
ExIsObjectInFirstZoneSegment(
  IN PZONE_HEADER  Zone,
  IN PVOID  Object);

NTKERNELAPI
VOID
NTAPI
ExReleaseResource(
  IN PERESOURCE  Resource);

#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExInitializeResource ExInitializeResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite

NTKERNELAPI
INTERLOCKED_RESULT
NTAPI
ExInterlockedDecrementLong(
  IN PLONG  Addend,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
ULONG
NTAPI
ExInterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value,
  IN PKSPIN_LOCK  Lock);

NTKERNELAPI
INTERLOCKED_RESULT
NTAPI
ExInterlockedIncrementLong(
  IN PLONG  Addend,
  IN PKSPIN_LOCK  Lock);

NTHALAPI
VOID
NTAPI
HalAcquireDisplayOwnership(
  IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters);

NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PWAIT_CONTEXT_BLOCK  Wcb,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine);

NTHALAPI
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN ULONG  Length,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled);

NTHALAPI
NTSTATUS
NTAPI
HalAssignSlotResources(
  IN PUNICODE_STRING  RegistryPath,
  IN PUNICODE_STRING  DriverClassName,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN INTERFACE_TYPE  BusType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources);

NTHALAPI
VOID
NTAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN ULONG  Length,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN PVOID  VirtualAddress,
  IN BOOLEAN  CacheEnabled);

NTHALAPI
PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  IN PDEVICE_DESCRIPTION  DeviceDescription,
  IN OUT PULONG  NumberOfMapRegisters);

NTHALAPI
ULONG
NTAPI
HalGetBusData(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Length);

NTHALAPI
ULONG
NTAPI
HalGetBusDataByOffset(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

NTHALAPI
ULONG
NTAPI
HalGetDmaAlignmentRequirement(
  VOID);

NTHALAPI
ULONG
NTAPI
HalGetInterruptVector(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber,
  IN ULONG  BusInterruptLevel,
  IN ULONG  BusInterruptVector,
  OUT PKIRQL  Irql,
  OUT PKAFFINITY  Affinity);

NTHALAPI
ULONG
NTAPI
HalReadDmaCounter(
  IN PADAPTER_OBJECT  AdapterObject);

NTHALAPI
ULONG
NTAPI
HalSetBusData(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Length);

NTHALAPI
ULONG
NTAPI
HalSetBusDataByOffset(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

NTHALAPI
BOOLEAN
NTAPI
HalTranslateBusAddress(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber,
  IN PHYSICAL_ADDRESS  BusAddress,
  IN OUT PULONG  AddressSpace,
  OUT PPHYSICAL_ADDRESS  TranslatedAddress);

NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine,
  IN PVOID  Context);

NTKERNELAPI
NTSTATUS
NTAPI
IoAssignResources(
  IN PUNICODE_STRING  RegistryPath,
  IN PUNICODE_STRING  DriverClassName  OPTIONAL,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDEVICE_OBJECT  DeviceObject  OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST  RequestedResources,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources);

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDeviceByPointer(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PDEVICE_OBJECT  TargetDevice);

NTHALAPI
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN BOOLEAN  WriteToDevice);

NTHALAPI
VOID
NTAPI
IoFreeAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject);

NTHALAPI
VOID
NTAPI
IoFreeMapRegisters(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PVOID  MapRegisterBase,
  IN ULONG  NumberOfMapRegisters);

NTHALAPI
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN OUT PULONG  Length,
  IN BOOLEAN  WriteToDevice);

NTKERNELAPI
PMDL
NTAPI
MmCreateMdl(
  IN PMDL  MemoryDescriptorList  OPTIONAL,
  IN PVOID  Base,
  IN SIZE_T  Length);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(
  IN PVOID  VirtualAddress);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlEnlargedIntegerMultiply(
  IN LONG  Multiplicand,
  IN LONG  Multiplier);

NTSYSAPI
ULONG
NTAPI
RtlEnlargedUnsignedDivide(
  IN ULARGE_INTEGER  Dividend,
  IN ULONG  Divisor,
  IN OUT PULONG  Remainder);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlEnlargedUnsignedMultiply(
  IN ULONG  Multiplicand,
  IN ULONG  Multiplier);

#ifndef _M_AMD64
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply(
  IN LARGE_INTEGER  Multiplicand,
  IN LONG  Multiplier);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide(
  IN LARGE_INTEGER  Dividend,
  IN ULONG  Divisor,
  IN OUT PULONG  Remainder);
#endif

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedMagicDivide(
  IN LARGE_INTEGER  Dividend,
  IN LARGE_INTEGER  MagicDivisor,
  IN CCHAR  ShiftCount);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerAdd(
  IN LARGE_INTEGER  Addend1,
  IN LARGE_INTEGER  Addend2);

NTSYSAPI
VOID
NTAPI
RtlLargeIntegerAnd(
  IN OUT LARGE_INTEGER  Result,
  IN LARGE_INTEGER  Source,
  IN LARGE_INTEGER  Mask);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerArithmeticShift(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide(
  IN LARGE_INTEGER  Dividend,
  IN LARGE_INTEGER  Divisor,
  IN OUT PLARGE_INTEGER  Remainder);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterOrEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterThan(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterThanOrEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerGreaterThanZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessOrEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessThan(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessThanOrEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerLessThanZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerNegate(
  IN LARGE_INTEGER  Subtrahend);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerNotEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTSYSAPI
BOOLEAN
NTAPI
RtlLargeIntegerNotEqualToZero(
  IN LARGE_INTEGER  Operand);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerShiftLeft(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerShiftRight(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerSubtract(
  IN LARGE_INTEGER  Minuend,
  IN LARGE_INTEGER  Subtrahend);


/*
 * ULONG
 * COMPUTE_PAGES_SPANNED(
 *   IN PVOID  Va,
 *   IN ULONG  Size)
 */
#define COMPUTE_PAGES_SPANNED(Va, \
                              Size) \
  (ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Size))


/*
** Architecture specific structures
*/

#ifdef _X86_

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
  IN PLONG  Addend);

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
  IN PLONG  Addend);

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value);

#define ExInterlockedIncrementLong(Addend,Lock) Exfi386InterlockedIncrementLong(Addend)
#define ExInterlockedDecrementLong(Addend,Lock) Exfi386InterlockedDecrementLong(Addend)
#define ExInterlockedExchangeUlong(Target, Value, Lock) Exfi386InterlockedExchangeUlong(Target, Value)

#endif /* _X86_ */

#ifdef __cplusplus
}
#endif

#endif /* __WINNT4_H */
