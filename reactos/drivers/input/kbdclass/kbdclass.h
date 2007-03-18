#include <ntifs.h>
#include <kbdmou.h>
#include <ntddkbd.h>
#include <stdio.h>

#if defined(__GNUC__)
  #include <pseh/pseh.h>
  #include <debug.h>
#elif defined(_MSC_VER)
  #define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
  #define CHECKPOINT1 DbgPrint("(%s:%d)\n", __FILE__, __LINE__)
  #define DPRINT
  #define CHECKPOINT
  #define _SEH_TRY __try
  #define _SEH_HANDLE __except(1)
  #define _SEH_END
  #define _SEH_GetExceptionCode() GetExceptionCode()
#else
  #error Unknown compiler!
#endif

#define MAX_PATH 260

#define MIN(a, b) ((a) < (b) ? (a) : (b))

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
	UNICODE_STRING InterfaceName;
} PORT_DEVICE_EXTENSION, *PPORT_DEVICE_EXTENSION;

typedef struct _CLASS_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;

	PCLASS_DRIVER_EXTENSION DriverExtension;

	LIST_ENTRY ListHead;
	KSPIN_LOCK ListSpinLock;
	KSPIN_LOCK SpinLock;
	BOOLEAN ReadIsPending;
	SIZE_T InputCount;
	PKEYBOARD_INPUT_DATA PortData;
	LPCWSTR DeviceName;
} CLASS_DEVICE_EXTENSION, *PCLASS_DEVICE_EXTENSION;

/* misc.c */

NTSTATUS
ForwardIrpAndWait(
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
