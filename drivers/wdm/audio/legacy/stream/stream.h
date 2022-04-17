#pragma once

#include <strmini.h>
#define YDEBUG
#include <debug.h>

#define STREAMDEBUG_LEVEL DebugLevelMaximum

typedef BOOLEAN (NTAPI *SYNCHRONIZE_FUNC) (IN PKINTERRUPT  Interrupt, IN PKSYNCHRONIZE_ROUTINE  SynchronizeRoutine, IN PVOID  SynchronizeContext);

typedef struct
{
    HW_INITIALIZATION_DATA Data;

}STREAM_CLASS_DRIVER_EXTENSION, *PSTREAM_CLASS_DRIVER_EXTENSION;

typedef struct
{
    LIST_ENTRY Entry;
    PVOID Start;
    ULONG Length;
}MEMORY_RESOURCE_LIST, *PMEMORY_RESOURCE_LIST;

typedef struct
{
    KSDEVICE_HEADER Header;
    PDEVICE_OBJECT LowerDeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;

    SYNCHRONIZE_FUNC SynchronizeFunction;

    ULONG MapRegisters;
    PDMA_ADAPTER DmaAdapter;
    PVOID DmaCommonBuffer;
    PHYSICAL_ADDRESS DmaPhysicalAddress;

    PKINTERRUPT Interrupt;
    KDPC InterruptDpc;

    LIST_ENTRY MemoryResourceList;

    ULONG StreamDescriptorSize;
    PHW_STREAM_DESCRIPTOR StreamDescriptor;
    PSTREAM_CLASS_DRIVER_EXTENSION DriverExtension;

    PVOID DeviceExtension;
    LONG InstanceCount;

}STREAM_DEVICE_EXTENSION, *PSTREAM_DEVICE_EXTENSION;

typedef struct
{
    HW_STREAM_REQUEST_BLOCK Block;
    KEVENT Event;
}HW_STREAM_REQUEST_BLOCK_EXT, *PHW_STREAM_REQUEST_BLOCK_EXT;

NTSTATUS
NTAPI
StreamClassCreateFilter(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassSystemControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassCleanup(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassFlushBuffers(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
StreamClassAddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject);

BOOLEAN
NTAPI
StreamClassSynchronize(
    IN PKINTERRUPT  Interrupt,
    IN PKSYNCHRONIZE_ROUTINE  SynchronizeRoutine,
    IN PVOID  SynchronizeContext);

BOOLEAN
NTAPI
StreamClassInterruptRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext);

VOID
NTAPI
StreamClassInterruptDpc(
    IN PKDPC Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2);

VOID
CompleteIrp(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG_PTR Information);
