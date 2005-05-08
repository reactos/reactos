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

#pragma pack(push,4)

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

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
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

NTOSAPI
NTSTATUS
DDKAPI
ExInterlockedExtendZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Segment,
  IN ULONG  SegmentSize,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
PVOID
DDKAPI
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

NTOSAPI
VOID
DDKAPI
ExQueueWorkItem(
  IN PWORK_QUEUE_ITEM  WorkItem,
  IN WORK_QUEUE_TYPE  QueueType);

NTOSAPI
BOOLEAN
DDKAPI
ExIsObjectInFirstZoneSegment(
  IN PZONE_HEADER  Zone,
  IN PVOID  Object);

NTOSAPI
VOID
DDKAPI
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

NTOSAPI
INTERLOCKED_RESULT
DDKAPI
ExInterlockedDecrementLong(
  IN PLONG  Addend,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
ULONG
DDKAPI
ExInterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
INTERLOCKED_RESULT
DDKAPI
ExInterlockedIncrementLong(
  IN PLONG  Addend,
  IN PKSPIN_LOCK  Lock);

NTOSAPI
PVOID
DDKAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN ULONG  Length,
  OUT PPHYSICAL_ADDRESS  LogicalAddress,
  IN BOOLEAN  CacheEnabled);

NTOSAPI
NTSTATUS
DDKAPI
HalAssignSlotResources(
  IN PUNICODE_STRING  RegistryPath,
  IN PUNICODE_STRING  DriverClassName,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN INTERFACE_TYPE  BusType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources);

NTOSAPI
VOID
DDKAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN ULONG  Length,
  IN PHYSICAL_ADDRESS  LogicalAddress,
  IN PVOID  VirtualAddress,
  IN BOOLEAN  CacheEnabled);

NTOSAPI
PADAPTER_OBJECT
DDKAPI
HalGetAdapter(
  IN PDEVICE_DESCRIPTION  DeviceDescription,
  IN OUT PULONG  NumberOfMapRegisters);

NTOSAPI
ULONG
DDKAPI
HalGetBusData(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Length);

NTOSAPI
ULONG
DDKAPI
HalGetBusDataByOffset(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

NTOSAPI
ULONG
DDKAPI
HalGetDmaAlignmentRequirement( 
  VOID);

NTOSAPI
ULONG
DDKAPI
HalGetInterruptVector(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber,
  IN ULONG  BusInterruptLevel,
  IN ULONG  BusInterruptVector,
  OUT PKIRQL  Irql,
  OUT PKAFFINITY  Affinity);

NTOSAPI
ULONG
DDKAPI
HalReadDmaCounter(
  IN PADAPTER_OBJECT  AdapterObject);

NTOSAPI
ULONG
DDKAPI
HalSetBusData(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Length);

NTOSAPI
ULONG
DDKAPI
HalSetBusDataByOffset(
  IN BUS_DATA_TYPE  BusDataType,
  IN ULONG  BusNumber,
  IN ULONG  SlotNumber,
  IN PVOID  Buffer,
  IN ULONG  Offset,
  IN ULONG  Length);

NTOSAPI
BOOLEAN
DDKAPI
HalTranslateBusAddress(
  IN INTERFACE_TYPE  InterfaceType,
  IN ULONG  BusNumber,
  IN PHYSICAL_ADDRESS  BusAddress,
  IN OUT PULONG  AddressSpace,
  OUT PPHYSICAL_ADDRESS  TranslatedAddress);

NTOSAPI
NTSTATUS
DDKAPI
IoAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PDEVICE_OBJECT  DeviceObject,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine,
  IN PVOID  Context);

NTOSAPI
NTSTATUS
DDKAPI
IoAssignResources(
  IN PUNICODE_STRING  RegistryPath,
  IN PUNICODE_STRING  DriverClassName  OPTIONAL,
  IN PDRIVER_OBJECT  DriverObject,
  IN PDEVICE_OBJECT  DeviceObject  OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST  RequestedResources,
  IN OUT PCM_RESOURCE_LIST  *AllocatedResources);

NTOSAPI
NTSTATUS
DDKAPI
IoAttachDeviceByPointer(
  IN PDEVICE_OBJECT  SourceDevice,
  IN PDEVICE_OBJECT  TargetDevice);

NTOSAPI
BOOLEAN
DDKAPI
IoFlushAdapterBuffers(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN ULONG  Length,
  IN BOOLEAN  WriteToDevice);

NTOSAPI
VOID
DDKAPI
IoFreeAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject);

NTOSAPI
VOID
DDKAPI
IoFreeMapRegisters(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PVOID  MapRegisterBase,
  IN ULONG  NumberOfMapRegisters);

NTOSAPI
PHYSICAL_ADDRESS
DDKAPI
IoMapTransfer(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PMDL  Mdl,
  IN PVOID  MapRegisterBase,
  IN PVOID  CurrentVa,
  IN OUT PULONG  Length,
  IN BOOLEAN  WriteToDevice);

NTOSAPI
PMDL
DDKAPI
MmCreateMdl(
  IN PMDL  MemoryDescriptorList  OPTIONAL,
  IN PVOID  Base,
  IN SIZE_T  Length);

NTOSAPI
BOOLEAN
DDKAPI
MmIsNonPagedSystemAddressValid(
  IN PVOID  VirtualAddress);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlEnlargedIntegerMultiply(
  IN LONG  Multiplicand,
  IN LONG  Multiplier);

NTOSAPI
ULONG
DDKAPI
RtlEnlargedUnsignedDivide(
  IN ULARGE_INTEGER  Dividend,
  IN ULONG  Divisor,
  IN OUT PULONG  Remainder);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlEnlargedUnsignedMultiply(
  IN ULONG  Multiplicand,
  IN ULONG  Multiplier);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlExtendedIntegerMultiply(
  IN LARGE_INTEGER  Multiplicand,
  IN LONG  Multiplier);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlExtendedLargeIntegerDivide(
  IN LARGE_INTEGER  Dividend,
  IN ULONG  Divisor,
  IN OUT PULONG  Remainder);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlExtendedMagicDivide(
  IN LARGE_INTEGER  Dividend,
  IN LARGE_INTEGER  MagicDivisor,
  IN CCHAR  ShiftCount);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlLargeIntegerAdd(
  IN LARGE_INTEGER  Addend1,
  IN LARGE_INTEGER  Addend2);

NTOSAPI
VOID
DDKAPI
RtlLargeIntegerAnd(
  IN OUT LARGE_INTEGER  Result,
  IN LARGE_INTEGER  Source,
  IN LARGE_INTEGER  Mask);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlLargeIntegerArithmeticShift(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlLargeIntegerDivide(
  IN LARGE_INTEGER  Dividend,
  IN LARGE_INTEGER  Divisor,
  IN OUT PLARGE_INTEGER  Remainder);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerEqualToZero(
  IN LARGE_INTEGER  Operand);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerGreaterOrEqualToZero(
  IN LARGE_INTEGER  Operand);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerGreaterThan(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerGreaterThanOrEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerGreaterThanZero(
  IN LARGE_INTEGER  Operand);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerLessOrEqualToZero(
  IN LARGE_INTEGER  Operand);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerLessThan(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerLessThanOrEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerLessThanZero(
  IN LARGE_INTEGER  Operand);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlLargeIntegerNegate(
  IN LARGE_INTEGER  Subtrahend);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerNotEqualTo(
  IN LARGE_INTEGER  Operand1,
  IN LARGE_INTEGER  Operand2);

NTOSAPI
BOOLEAN
DDKAPI
RtlLargeIntegerNotEqualToZero(
  IN LARGE_INTEGER  Operand);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlLargeIntegerShiftLeft(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTOSAPI
LARGE_INTEGER
DDKAPI
RtlLargeIntegerShiftRight(
  IN LARGE_INTEGER  LargeInteger,
  IN CCHAR  ShiftCount);

NTOSAPI
LARGE_INTEGER
DDKAPI
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

NTOSAPI
INTERLOCKED_RESULT
DDKFASTAPI
Exfi386InterlockedIncrementLong(
  IN PLONG  Addend);

NTOSAPI
INTERLOCKED_RESULT
DDKFASTAPI
Exfi386InterlockedDecrementLong(
  IN PLONG  Addend);

NTOSAPI
ULONG
DDKFASTAPI
Exfi386InterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value);

#define ExInterlockedIncrementLong(Addend,Lock) Exfi386InterlockedIncrementLong(Addend)
#define ExInterlockedDecrementLong(Addend,Lock) Exfi386InterlockedDecrementLong(Addend)
#define ExInterlockedExchangeUlong(Target, Value, Lock) Exfi386InterlockedExchangeUlong(Target, Value)

#endif /* _X86_ */

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* __WINNT4_H */
