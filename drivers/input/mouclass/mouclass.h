#ifndef _MOUCLASS_PCH_
#define _MOUCLASS_PCH_

#include <ntifs.h>
#include <ntddmou.h>

#define MAX_PATH 260

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLASS_TAG 'CuoM'
#define DPFLTR_CLASS_NAME_ID DPFLTR_MOUCLASS_ID

typedef enum
{
	dsStopped,
	dsStarted,
	dsPaused,
	dsRemoved,
	dsSurpriseRemoved
} PORT_DEVICE_STATE;

typedef struct _CLASS_DRIVER_EXTENSION
{
	UNICODE_STRING RegistryPath;

	/* Registry settings */
	ULONG ConnectMultiplePorts;
	ULONG DataQueueSize;
	UNICODE_STRING DeviceBaseName;

	PDEVICE_OBJECT MainClassDeviceObject;
} CLASS_DRIVER_EXTENSION, *PCLASS_DRIVER_EXTENSION;

typedef struct _COMMON_DEVICE_EXTENSION
{
	BOOLEAN IsClassDO;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _PORT_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	LIST_ENTRY ListEntry;
	PDEVICE_OBJECT DeviceObject;
	PORT_DEVICE_STATE PnpState;
	PDEVICE_OBJECT LowerDevice;
	PDEVICE_OBJECT ClassDO;
	HANDLE FileHandle;
	UNICODE_STRING InterfaceName;
} PORT_DEVICE_EXTENSION, *PPORT_DEVICE_EXTENSION;

typedef struct _CLASS_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	PCLASS_DRIVER_EXTENSION DriverExtension;

	LIST_ENTRY ListHead;
	KSPIN_LOCK ListSpinLock;
	KSPIN_LOCK SpinLock;
	PIRP PendingIrp;
	SIZE_T InputCount;
	PMOUSE_INPUT_DATA PortData;
	LPCWSTR DeviceName;
} CLASS_DEVICE_EXTENSION, *PCLASS_DEVICE_EXTENSION;

/* misc.c */

DRIVER_DISPATCH ForwardIrpAndForget;

NTSTATUS
DuplicateUnicodeString(
	IN ULONG Flags,
	IN PCUNICODE_STRING SourceString,
	OUT PUNICODE_STRING DestinationString);

#endif /* _MOUCLASS_PCH_ */
