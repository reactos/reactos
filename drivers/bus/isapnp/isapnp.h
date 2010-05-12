#pragma once

#include <wdm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_ISAPNP 'PNPI'

typedef enum {
  dsStopped,
  dsStarted
} ISAPNP_DEVICE_STATE;

typedef struct _ISAPNP_COMMON_EXTENSION {
  PDEVICE_OBJECT Self;
  BOOLEAN IsFdo;
  ISAPNP_DEVICE_STATE State;
} ISAPNP_COMMON_EXTENSION, *PISAPNP_COMMON_EXTENSION;

typedef struct _ISAPNP_FDO_EXTENSION {
  ISAPNP_COMMON_EXTENSION Common;
  PDEVICE_OBJECT Ldo;
  PDEVICE_OBJECT Pdo;
  LIST_ENTRY DeviceListHead;
  ULONG DeviceCount;
  PUCHAR ReadDataPort;
  KSPIN_LOCK Lock;
} ISAPNP_FDO_EXTENSION, *PISAPNP_FDO_EXTENSION;

typedef struct _ISAPNP_LOGICAL_DEVICE {
  ISAPNP_COMMON_EXTENSION Common;
  USHORT VendorId;
  USHORT ProdId;
  USHORT IoAddr;
  UCHAR IrqNo;
  UCHAR CSN;
  UCHAR LDN;
  LIST_ENTRY ListEntry;
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;

/* isapnp.c */
NTSTATUS
NTAPI
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath);

NTSTATUS
NTAPI
IsaForwardIrpSynchronous(
	IN PISAPNP_FDO_EXTENSION FdoExt,
	IN PIRP Irp);

/* fdo.c */
NTSTATUS
NTAPI
IsaFdoPnp(
  IN PISAPNP_FDO_EXTENSION FdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp);

/* pdo.c */
NTSTATUS
NTAPI
IsaPdoPnp(
  IN PISAPNP_LOGICAL_DEVICE LogDev,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp);

/* hardware.c */
NTSTATUS
NTAPI
IsaHwDetectReadDataPort(
  IN PISAPNP_FDO_EXTENSION FdoExt);

NTSTATUS
NTAPI
IsaHwFillDeviceList(
  IN PISAPNP_FDO_EXTENSION FdoExt);

NTSTATUS
NTAPI
IsaHwDeactivateDevice(
  IN PISAPNP_LOGICAL_DEVICE LogicalDevice);

NTSTATUS
NTAPI
IsaHwActivateDevice(
  IN PISAPNP_LOGICAL_DEVICE LogicalDevice);

#ifdef __cplusplus
}
#endif
