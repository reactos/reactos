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

#define INFINITE -1
#define KEYBOARD_BUFFER_SIZE 100

typedef enum
{
	Green,
	Screen,
	Keyboard
} GREEN_DEVICE_TYPE;

typedef struct _COMMON_DEVICE_EXTENSION
{
	GREEN_DEVICE_TYPE Type;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _KEYBOARD_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;
	PDEVICE_OBJECT Green;

	CONNECT_DATA ClassInformation;
	HANDLE WorkerThreadHandle;
	KDPC KeyboardDpc;

	ULONG ActiveQueue;
	ULONG InputDataCount[2];
	KEYBOARD_INPUT_DATA KeyboardInputData[2][KEYBOARD_BUFFER_SIZE];
} KEYBOARD_DEVICE_EXTENSION, *PKEYBOARD_DEVICE_EXTENSION;

typedef struct _SCREEN_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;
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

typedef struct _GREEN_DEVICE_EXTENSION
{
	COMMON_DEVICE_EXTENSION Common;
	PDEVICE_OBJECT Serial;

	PDEVICE_OBJECT LowerDevice;
	ULONG BaudRate;
	SERIAL_LINE_CONTROL LineControl;
	SERIAL_TIMEOUTS Timeouts;

	PDEVICE_OBJECT Keyboard;
	PDEVICE_OBJECT Screen;
} GREEN_DEVICE_EXTENSION, *PGREEN_DEVICE_EXTENSION;

/************************************ createclose.c */

NTSTATUS NTAPI
GreenCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS NTAPI
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
KeyboardInitialize(
	IN PDRIVER_OBJECT DriverObject,
	OUT PDEVICE_OBJECT* KeyboardFdo);

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

/************************************ pnp.c */

NTSTATUS NTAPI
GreenAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

/************************************ screen.c */

NTSTATUS
ScreenInitialize(
	IN PDRIVER_OBJECT DriverObject,
	OUT PDEVICE_OBJECT* ScreenFdo);

NTSTATUS
ScreenWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS
ScreenDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);
