/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/bus/serenum/serenum.h
 * PURPOSE:         Serial enumerator driver header
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#include <ntddk.h>
#include <ntddser.h>
#include <stdio.h>

#if defined(__GNUC__)
  #include <debug.h>
#elif defined(_MSC_VER)
  #define STDCALL

  #define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
  #define CHECKPOINT1 DbgPrint("(%s:%d)\n")
  #define DPRINT DPRINT1
  #define CHECKPOINT CHECKPOINT1
#else
  #error Unknown compiler!
#endif

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

/* FIXME: I don't know why it is not defined anywhere... */
NTSTATUS STDCALL
IoAttachDeviceToDeviceStackSafe(
  IN PDEVICE_OBJECT SourceDevice,
  IN PDEVICE_OBJECT TargetDevice,
  OUT PDEVICE_OBJECT *AttachedToDeviceObject);

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

NTSTATUS STDCALL
SerenumAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

NTSTATUS
SerenumFdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ misc.c */

NTSTATUS
SerenumDuplicateUnicodeString(
	OUT PUNICODE_STRING Destination,
	IN PUNICODE_STRING Source,
	IN POOL_TYPE PoolType);

NTSTATUS
SerenumInitMultiSzString(
	OUT PUNICODE_STRING Destination,
	... /* list of PCSZ */);

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpToLowerDeviceAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpToAttachedFdoAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ pdo.c */

NTSTATUS
SerenumPdoPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
