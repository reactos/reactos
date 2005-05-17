/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnpdma.c
 * PURPOSE:         PnP manager DMA routines
 *
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
#include <ddk/wdmguid.h>

typedef struct _DMA_ADAPTER_INTERNAL {
  USHORT Version;
  USHORT Size;
  PDMA_OPERATIONS DmaOperations;
  PADAPTER_OBJECT HalAdapter;
} DMA_ADAPTER_INTERNAL, *PDMA_ADAPTER_INTERNAL;

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
IopPutDmaAdapter(
  PDMA_ADAPTER DmaAdapter)
{
  DPRINT("IopPutDmaAdapter\n");
  ExFreePool(DmaAdapter);
}


PVOID
STDCALL
IopAllocateCommonBuffer(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled)
{
  DPRINT("IopAllocateCommonBuffer\n");
  return HalAllocateCommonBuffer(
    ((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter,
    Length, LogicalAddress, CacheEnabled);
}


VOID
STDCALL
IopFreeCommonBuffer(
  IN PDMA_ADAPTER DmaAdapter,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled)
{
  DPRINT("IopFreeCommonBuffer\n");
  HalFreeCommonBuffer(
    ((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter,
    Length, LogicalAddress, VirtualAddress, CacheEnabled);
}


NTSTATUS
STDCALL
IopAllocateAdapterChannel(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG NumberOfMapRegisters,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context)
{
  DPRINT("IopAllocateAdapterChannel\n");
  return IoAllocateAdapterChannel(
    ((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter,
    DeviceObject, NumberOfMapRegisters, ExecutionRoutine, Context);
}


BOOLEAN
STDCALL
IopFlushAdapterBuffers(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice)
{
  DPRINT("IopFlushAdapterBuffers\n");
  return IoFlushAdapterBuffers(
    ((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter,
    Mdl, MapRegisterBase, CurrentVa, Length, WriteToDevice);
}


VOID
STDCALL
IopFreeAdapterChannel(
  IN PDMA_ADAPTER DmaAdapter)
{
  DPRINT("IopFreeAdapterChannel\n");
  IoFreeAdapterChannel(((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter);
}


VOID
STDCALL
IopFreeMapRegisters(
  IN PDMA_ADAPTER DmaAdapter,
  PVOID MapRegisterBase,
  ULONG NumberOfMapRegisters)
{
  DPRINT("IopFreeMapRegisters\n");
  IoFreeMapRegisters(
    ((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter,
    MapRegisterBase, NumberOfMapRegisters);
}


PHYSICAL_ADDRESS
STDCALL
IopMapTransfer(
  IN PDMA_ADAPTER DmaAdapter,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN OUT PULONG Length,
  IN BOOLEAN WriteToDevice)
{
  DPRINT("IopMapTransfer\n");
  return IoMapTransfer(
    ((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter,
    Mdl, MapRegisterBase, CurrentVa, Length, WriteToDevice);
}


ULONG
STDCALL
IopGetDmaAlignment(
  IN PDMA_ADAPTER DmaAdapter)
{
  DPRINT("IopGetDmaAlignment\n");
  /* FIXME: This is actually true only on i386 and Amd64 */
  return 1L;
}


ULONG
STDCALL
IopReadDmaCounter(
  IN PDMA_ADAPTER DmaAdapter)
{
  DPRINT("IopReadDmaCounter\n");
  return HalReadDmaCounter(((PDMA_ADAPTER_INTERNAL)DmaAdapter)->HalAdapter);
}


NTSTATUS
STDCALL
IopGetScatterGatherList(
  IN PDMA_ADAPTER DmaAdapter,
  IN PDEVICE_OBJECT DeviceObject,
  IN PMDL Mdl,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN PDRIVER_LIST_CONTROL ExecutionRoutine,
  IN PVOID Context,
  IN BOOLEAN WriteToDevice)
{
  DPRINT("IopGetScatterGatherList\n");
  /* FIXME */
  return STATUS_UNSUCCESSFUL;
}


VOID
STDCALL
IopPutScatterGatherList(
  IN PDMA_ADAPTER DmaAdapter,
  IN PSCATTER_GATHER_LIST ScatterGather,
  IN BOOLEAN WriteToDevice)
{
  DPRINT("IopPutScatterGatherList\n");
  /* FIXME */
}

/*
 * @implemented
 */
PDMA_ADAPTER
STDCALL
IoGetDmaAdapter(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN PDEVICE_DESCRIPTION DeviceDescription,
  IN OUT PULONG NumberOfMapRegisters)
{
  static DMA_OPERATIONS HalDmaOperations = {
    sizeof(DMA_OPERATIONS),
    IopPutDmaAdapter,
    IopAllocateCommonBuffer,
    IopFreeCommonBuffer,
    IopAllocateAdapterChannel,
    IopFlushAdapterBuffers,
    IopFreeAdapterChannel,
    IopFreeMapRegisters,
    IopMapTransfer,
    IopGetDmaAlignment,
    IopReadDmaCounter,
    IopGetScatterGatherList,
    IopPutScatterGatherList
  };
  NTSTATUS Status;
  ULONG ResultLength;
  BUS_INTERFACE_STANDARD BusInterface;
  IO_STATUS_BLOCK IoStatusBlock;
  IO_STACK_LOCATION Stack;
  DEVICE_DESCRIPTION PrivateDeviceDescription;
  PDMA_ADAPTER Result = NULL;
  PDMA_ADAPTER_INTERNAL ResultInternal = NULL;
  PADAPTER_OBJECT HalAdapter;

  DPRINT("IoGetDmaAdapter called\n");

  /*
   * Try to create DMA adapter through bus driver
   */
  if (PhysicalDeviceObject != NULL)
  {
    if (DeviceDescription->InterfaceType == 0x0F /*PNPBus*/ ||
        DeviceDescription->InterfaceType == 0xFFFFFFFF)
    {
      RtlCopyMemory(&PrivateDeviceDescription, DeviceDescription,
        sizeof(DEVICE_DESCRIPTION));
      Status = IoGetDeviceProperty(PhysicalDeviceObject,
         DevicePropertyLegacyBusType, sizeof(INTERFACE_TYPE),
         &PrivateDeviceDescription.InterfaceType, &ResultLength);
      if (!NT_SUCCESS(Status))
      {
        PrivateDeviceDescription.InterfaceType = Internal;
      }
      DeviceDescription = &PrivateDeviceDescription;
    }

    Stack.Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack.Parameters.QueryInterface.Version = 1;
    Stack.Parameters.QueryInterface.Interface = (PINTERFACE)&BusInterface;
    Stack.Parameters.QueryInterface.InterfaceType =
      &GUID_BUS_INTERFACE_STANDARD;
    Status = IopInitiatePnpIrp(PhysicalDeviceObject, &IoStatusBlock,
      IRP_MN_QUERY_INTERFACE, &Stack);
    if (NT_SUCCESS(Status))
    {
      Result = BusInterface.GetDmaAdapter(BusInterface.Context,
        DeviceDescription, NumberOfMapRegisters);
      BusInterface.InterfaceDereference(BusInterface.Context);
      if (Result != NULL)
        return Result;
    }
  }

  /*
   * Fallback to HAL
   */

  HalAdapter = HalGetAdapter(DeviceDescription, NumberOfMapRegisters);
  if (HalAdapter == NULL)
    return NULL;

  ResultInternal = ExAllocatePool(PagedPool, sizeof(DMA_ADAPTER_INTERNAL));
  if (ResultInternal == NULL)
    return NULL;

  ResultInternal->Version = DEVICE_DESCRIPTION_VERSION;
  ResultInternal->Size = sizeof(DMA_ADAPTER);
  ResultInternal->DmaOperations = &HalDmaOperations;
  ResultInternal->HalAdapter = HalAdapter;

  return (PDMA_ADAPTER)ResultInternal;
}

/* EOF */
