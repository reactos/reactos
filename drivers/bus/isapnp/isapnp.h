#ifndef _ISAPNP_PCH_
#define _ISAPNP_PCH_

#include <wdm.h>
#include <ntstrsafe.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_ISAPNP 'PNPI'

typedef enum {
    dsStopped,
    dsStarted
} ISAPNP_DEVICE_STATE;

typedef struct _ISAPNP_LOGICAL_DEVICE {
    PDEVICE_OBJECT Pdo;
    UCHAR VendorId[3];
    USHORT ProdId;
    ULONG SerialNumber;
    USHORT IoAddr;
    UCHAR IrqNo;
    UCHAR CSN;
    UCHAR LDN;
    LIST_ENTRY ListEntry;
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;

typedef struct _ISAPNP_COMMON_EXTENSION {
    PDEVICE_OBJECT Self;
    BOOLEAN IsFdo;
    ISAPNP_DEVICE_STATE State;
} ISAPNP_COMMON_EXTENSION, *PISAPNP_COMMON_EXTENSION;

typedef struct _ISAPNP_FDO_EXTENSION {
    ISAPNP_COMMON_EXTENSION Common;
    PDEVICE_OBJECT Ldo;
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT DataPortPdo;
    LIST_ENTRY DeviceListHead;
    ULONG DeviceCount;
    PDRIVER_OBJECT DriverObject;
    PUCHAR ReadDataPort;
    KSPIN_LOCK Lock;
} ISAPNP_FDO_EXTENSION, *PISAPNP_FDO_EXTENSION;

typedef struct _ISAPNP_PDO_EXTENSION {
    ISAPNP_COMMON_EXTENSION Common;
    PISAPNP_LOGICAL_DEVICE IsaPnpDevice;
    UNICODE_STRING DeviceID;
    UNICODE_STRING HardwareIDs;
    UNICODE_STRING InstanceID;
} ISAPNP_PDO_EXTENSION, *PISAPNP_PDO_EXTENSION;

/* isapnp.c */

#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE         1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING   2

NTSTATUS
NTAPI
IsaPnpDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString);

NTSTATUS
NTAPI
IsaPnpFillDeviceRelations(
    IN PISAPNP_FDO_EXTENSION FdoExt,
    IN PIRP Irp,
    IN BOOLEAN IncludeDataPort);

DRIVER_INITIALIZE DriverEntry;

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
    IN PISAPNP_PDO_EXTENSION PdoDeviceExtension,
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

#endif /* _ISAPNP_PCH_ */
