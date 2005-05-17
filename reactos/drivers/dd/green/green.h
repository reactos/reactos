#if defined(__GNUC__)
  #include <ddk/ntddk.h>
  #include <ddk/ntddser.h>
  #include <ntos/halfuncs.h>
  #include <ddk/ntddblue.h>
  #include <ddk/ntddkbd.h> /* should be in kbdmou.h */

  #include <debug.h>

  /* FIXME: should be in kbdmou.h */
  typedef struct _CONNECT_DATA {
    PDEVICE_OBJECT ClassDeviceObject;
    PVOID ClassService;
  } CONNECT_DATA, *PCONNECT_DATA;

  /* FIXME: should be in kbdmou.h */
  #define IOCTL_INTERNAL_KEYBOARD_CONNECT \
    CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)

  NTSTATUS STDCALL
  ObReferenceObjectByName(PUNICODE_STRING ObjectPath,
    ULONG Attributes,
    PACCESS_STATE PassedAccessState,
    ACCESS_MASK DesiredAccess,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID ParseContext,
    PVOID* ObjectPtr);

  /* FIXME: should be in kbdmou.h */
  typedef VOID (*PSERVICE_CALLBACK_ROUTINE)(PDEVICE_OBJECT, PKEYBOARD_INPUT_DATA, PKEYBOARD_INPUT_DATA, PULONG);

  typedef struct _CLASS_INFORMATION
  {
    PDEVICE_OBJECT DeviceObject;
    PVOID CallBack;
  } CLASS_INFORMATION, *PCLASS_INFORMATION;

  #define KEYBOARD_BUFFER_SIZE 100

#elif defined(_MSC_VER)
  #include <ntddk.h>
  #include <ntddser.h>
  #include <kbdmou.h>

  #define STDCALL

  #define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
  #define CHECKPOINT1 DbgPrint("(%s:%d)\n", __FILE__, __LINE__)
  #define DPRINT DPRINT1
  #define CHECKPOINT CHECKPOINT1
#else
  #error Unknown compiler!
#endif

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

	CLASS_INFORMATION ClassInformation;
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

NTSTATUS STDCALL
GreenCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
GreenClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ dispatch.c */

NTSTATUS STDCALL
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

NTSTATUS STDCALL
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
