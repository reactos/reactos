#ifndef _SERMOUSE_PCH_
#define _SERMOUSE_PCH_

#include <ntddk.h>
#include <ntddser.h>
#include <kbdmou.h>

#define SERMOUSE_TAG 'uoMS'

typedef enum
{
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} SERMOUSE_DEVICE_STATE;

typedef enum
{
	mtNone,      /* No Mouse */
	mtMicrosoft, /* Microsoft Mouse with 2 buttons */
	mtLogitech,  /* Logitech Mouse with 3 buttons */
	mtWheelZ     /* Microsoft Wheel Mouse (aka Z Mouse) */
} SERMOUSE_MOUSE_TYPE;

/* Size for packet buffer used in interrupt routine */
#define PACKET_BUFFER_SIZE  4

/* Hardware byte mask for left button */
#define LEFT_BUTTON_MASK     0x20
/* Hardware to Microsoft specific code byte shift for left button */
#define LEFT_BUTTON_SHIFT    5
/* Hardware byte mask for right button */
#define RIGHT_BUTTON_MASK    0x10
/* Hardware to Microsoft specific code byte shift for right button */
#define RIGHT_BUTTON_SHIFT   3
/* Hardware byte mask for middle button */
#define MIDDLE_BUTTON_MASK   0x20
/* Hardware to Microsoft specific code byte shift for middle button */
#define MIDDLE_BUTTON_SHIFT  3

/* Microsoft byte mask for left button */
#define MOUSE_BUTTON_LEFT    0x01
/* Microsoft byte mask for right button */
#define MOUSE_BUTTON_RIGHT   0x02
/* Microsoft byte mask for middle button */
#define MOUSE_BUTTON_MIDDLE  0x04

typedef struct _SERMOUSE_DRIVER_EXTENSION
{
	USHORT NumberOfButtons;
} SERMOUSE_DRIVER_EXTENSION, *PSERMOUSE_DRIVER_EXTENSION;

typedef struct _SERMOUSE_DEVICE_EXTENSION
{
	PDEVICE_OBJECT LowerDevice;
	SERMOUSE_DEVICE_STATE PnpState;
	SERMOUSE_MOUSE_TYPE MouseType;
	PSERMOUSE_DRIVER_EXTENSION DriverExtension;

	HANDLE WorkerThreadHandle;
	KEVENT StopWorkerThreadEvent;

	ULONG ActiveQueue;
	ULONG InputDataCount[2];
	CONNECT_DATA ConnectData;
	MOUSE_INPUT_DATA MouseInputData[2];
	UCHAR PacketBuffer[PACKET_BUFFER_SIZE];
	ULONG PacketBufferPosition;
	ULONG PreviousButtons;
	MOUSE_ATTRIBUTES AttributesInformation;
} SERMOUSE_DEVICE_EXTENSION, *PSERMOUSE_DEVICE_EXTENSION;

/************************************ createclose.c */

DRIVER_DISPATCH SermouseCreate;

DRIVER_DISPATCH SermouseClose;

DRIVER_DISPATCH SermouseCleanup;

/************************************ detect.c */

SERMOUSE_MOUSE_TYPE
SermouseDetectLegacyDevice(
	IN PDEVICE_OBJECT LowerDevice);

/************************************ fdo.c */

DRIVER_ADD_DEVICE SermouseAddDevice;

DRIVER_DISPATCH SermousePnp;

/************************************ internaldevctl.c */

DRIVER_DISPATCH SermouseInternalDeviceControl;

/************************************ misc.c */

NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ readmouse.c */

VOID NTAPI
SermouseDeviceWorker(
	PVOID Context);

#endif /* _SERMOUSE_PCH_ */
