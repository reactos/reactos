#include <stdarg.h>
#include <ntddk.h>
#include <ndk/iotypes.h>
#include <windef.h>
#define WINBASEAPI
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;
#include <ntddser.h>
#include <kbdmou.h>
#include <wincon.h>
#include <drivers/blue/ntddblue.h>

NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString
);
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE         1

#define INFINITE -1
#define KEYBOARD_BUFFER_SIZE 100

typedef enum
{
	GreenPDO,
	ScreenPDO,
	KeyboardPDO,
	GreenFDO,
	ScreenFDO,
	KeyboardFDO,
	PassThroughFDO,
} GREEN_DEVICE_TYPE;

typedef struct _COMMON_DEVICE_EXTENSION
{
	GREEN_DEVICE_TYPE Type;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

/* For PassThroughFDO devices */
typedef struct _COMMON_FDO_DEVICE_EXTENSION
{
	GREEN_DEVICE_TYPE Type;
	PDEVICE_OBJECT LowerDevice;
} COMMON_FDO_DEVICE_EXTENSION, *PCOMMON_FDO_DEVICE_EXTENSION;

/* For KeyboardFDO devices */
typedef struct _KEYBOARD_DEVICE_EXTENSION
{
	COMMON_FDO_DEVICE_EXTENSION Common;
	PDEVICE_OBJECT Green;

	CONNECT_DATA ClassInformation;
	HANDLE WorkerThreadHandle;
	KDPC KeyboardDpc;

	ULONG ActiveQueue;
	ULONG InputDataCount[2];
	KEYBOARD_INPUT_DATA KeyboardInputData[2][KEYBOARD_BUFFER_SIZE];
} KEYBOARD_DEVICE_EXTENSION, *PKEYBOARD_DEVICE_EXTENSION;

/* For ScreenFDO devices */
typedef struct _SCREEN_DEVICE_EXTENSION
{
	COMMON_FDO_DEVICE_EXTENSION Common;
	PDEVICE_OBJECT Green;

	PUCHAR VideoMemory;   /* Pointer to video memory */
	USHORT CharAttribute; /* Current color attribute */
	ULONG  Mode;
	UCHAR  ScanLines;     /* Height of a text line   */
	UCHAR  Rows;          /* Number of rows          */
	UCHAR  Columns;       /* Number of columns       */
	UCHAR  TabWidth;

	ULONG LogicalOffset;  /* Position of the cursor  */

	UCHAR SendBuffer[1024];
	ULONG SendBufferPosition;
	PDEVICE_OBJECT PreviousBlue;
} SCREEN_DEVICE_EXTENSION, *PSCREEN_DEVICE_EXTENSION;

/* For GreenFDO devices */
typedef struct _GREEN_DEVICE_EXTENSION
{
	COMMON_FDO_DEVICE_EXTENSION Common;
	PDEVICE_OBJECT Serial;

	SERIAL_LINE_CONTROL LineControl;
	SERIAL_TIMEOUTS Timeouts;

	PDEVICE_OBJECT KeyboardPdo;
	PDEVICE_OBJECT ScreenPdo;
	PDEVICE_OBJECT KeyboardFdo;
	PDEVICE_OBJECT ScreenFdo;
} GREEN_DEVICE_EXTENSION, *PGREEN_DEVICE_EXTENSION;

typedef struct _GREEN_DRIVER_EXTENSION
{
	UNICODE_STRING RegistryPath;

	UNICODE_STRING AttachedDeviceName;
	ULONG DeviceReported;
	ULONG SampleRate;

	PDEVICE_OBJECT GreenMainDO;
	PDEVICE_OBJECT LowerDevice;
} GREEN_DRIVER_EXTENSION, *PGREEN_DRIVER_EXTENSION;

/************************************ createclose.c */

NTSTATUS
GreenCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
GreenClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ dispatch.c */

NTSTATUS NTAPI
GreenDispatch(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ keyboard.c */

NTSTATUS
KeyboardAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

NTSTATUS
KeyboardInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ misc.c */

NTSTATUS
GreenDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG CtlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferSize,
	IN OUT PVOID OutputBuffer OPTIONAL,
	IN OUT PULONG OutputBufferSize);

NTSTATUS
ReadRegistryEntries(
	IN PUNICODE_STRING RegistryPath,
	IN PGREEN_DRIVER_EXTENSION DriverExtension);

/************************************ pnp.c */

NTSTATUS NTAPI
GreenAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

NTSTATUS
GreenPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ power.c */

NTSTATUS
GreenPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ screen.c */

NTSTATUS
ScreenAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

NTSTATUS
ScreenWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
ScreenDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
