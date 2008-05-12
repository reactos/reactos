/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/bus/serenum/serenum.h
 * PURPOSE:         Serial enumerator driver header
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include <ntifs.h>
#include <ntddk.h>
#include <ntddser.h>
#include <stdio.h>
#include <debug.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

typedef enum
{
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} SERENUM_DEVICE_STATE;

typedef struct _COMMON_DEVICE_EXTENSION
{
	BOOLEAN IsFDO;
	SERENUM_DEVICE_STATE PnpState;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	PDEVICE_OBJECT LowerDevice;
	PDEVICE_OBJECT Pdo;
	IO_REMOVE_LOCK RemoveLock;

	UNICODE_STRING SerenumInterfaceName;

	PDEVICE_OBJECT AttachedPdo;
	ULONG Flags;
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	PDEVICE_OBJECT AttachedFdo;

	UNICODE_STRING DeviceDescription; // REG_SZ
	UNICODE_STRING DeviceId;          // REG_SZ
	UNICODE_STRING InstanceId;        // REG_SZ
	UNICODE_STRING HardwareIds;       // REG_MULTI_SZ
	UNICODE_STRING CompatibleIds;     // REG_MULTI_SZ
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

#define SERENUM_TAG TAG('S', 'e', 'r', 'e')

/* Flags */
#define FLAG_ENUMERATION_DONE    0x01

/************************************ detect.c */

NTSTATUS
SerenumDetectPnpDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PDEVICE_OBJECT LowerDevice);

NTSTATUS
SerenumDetectLegacyDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PDEVICE_OBJECT LowerDevice);

/************************************ fdo.c */

DRIVER_ADD_DEVICE SerenumAddDevice;

NTSTATUS
SerenumFdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ misc.c */

NTSTATUS
SerenumInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */);

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
ForwardIrpToLowerDeviceAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
ForwardIrpToAttachedFdoAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
DuplicateUnicodeString(
	IN ULONG Flags,
	IN PCUNICODE_STRING SourceString,
	OUT PUNICODE_STRING DestinationString);

/************************************ pdo.c */

NTSTATUS
SerenumPdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ serenum.c */

NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegPath);
