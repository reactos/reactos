#ifndef _I8042DRV_H
#define _I8042DRV_H
#include <ddk/ntddk.h>
#include <ddk/ntddkbd.h>
#include <ddk/ntdd8042.h>

#define KEYBOARD_IRQ       1
#define MOUSE_IRQ          12
#define KBD_BUFFER_SIZE    32

// should be in ntdd8042.h

typedef VOID DDKAPI
(*KEYBOARD_CLASS_SERVICE_CALLBACK) (
	IN PDEVICE_OBJECT DeviceObject,
	IN PKEYBOARD_INPUT_DATA InputDataStart,
	IN PKEYBOARD_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed
);

/* I'm not actually sure if this is in the ddk, would seem logical */
typedef VOID DDKAPI
(*MOUSE_CLASS_SERVICE_CALLBACK) (
	IN PDEVICE_OBJECT DeviceObject,
	IN PMOUSE_INPUT_DATA InputDataStart,
	IN PMOUSE_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed
);

typedef struct _CONNECT_DATA {
	PDEVICE_OBJECT ClassDeviceObject;
	PVOID ClassService;
} CONNECT_DATA, *PCONNECT_DATA;

#define IOCTL_INTERNAL_KEYBOARD_CONNECT \
   CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_MOUSE_CONNECT \
   CTL_CODE(FILE_DEVICE_MOUSE, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)

/* For some bizarre reason, these are different from the defines in
 * w32api. I'm quite sure these are correct though, needs to be checked
 * against the ddk
 */
#define KEYBOARD_SCROLL_LOCK_ON 0x01
#define KEYBOARD_NUM_LOCK_ON 0x02
#define KEYBOARD_CAPS_LOCK_ON 0x04

/*-----------------------------------------------------
 *  DeviceExtension
 * --------------------------------------------------*/
typedef struct _COMMAND_CONTEXT
{
	int NumInput;
	int CurInput;
	UCHAR * Input;
	int NumOutput;
	int CurOutput;
	UCHAR * Output;
	NTSTATUS Status;

	BOOLEAN GotAck;
	KEVENT Event;

	PVOID DevExt;
} COMMAND_CONTEXT, *PCOMMAND_CONTEXT;

typedef enum _MOUSE_TIMEOUT_STATE
{
	NoChange,
	TimeoutStart,
	TimeoutCancel
} MOUSE_TIMEOUT_STATE, *PMOUSE_TIMEOUT_STATE;

/* TODO: part of this should be in the _ATTRIBUTES structs instead */
typedef struct _I8042_SETTINGS
{
	DWORD Headless;               /* done */
	DWORD CrashScroll;
	DWORD CrashSysRq;             /* done */
	DWORD ReportResetErrors;
	DWORD PollStatusIterations;   /* done */
	DWORD ResendIterations;       /* done */
	DWORD PollingIterations;
	DWORD PollingIterationsMaximum;
	DWORD OverrideKeyboardType;
	DWORD OverrideKeyboardSubtype;
	DWORD MouseResendStallTime;
	DWORD MouseSynchIn100ns;      /* done */
	DWORD MouseResolution;        /* done */
	DWORD NumberOfButtons;
	DWORD EnableWheelDetection;
} I8042_SETTINGS, *PI8042_SETTINGS;

typedef enum _I8042_MOUSE_TYPE
{
	GenericPS2,
	Intellimouse,
	IntellimouseExplorer,
	Ps2pp
} I8042_MOUSE_TYPE, *PI8042_MOUSE_TYPE;

typedef enum _I8042_DEVICE_TYPE
{
	Keyboard,
	Mouse
} I8042_DEVICE_TYPE, *PI8042_DEVICE_TYPE;

typedef struct _I8042_DEVICE
{
	LIST_ENTRY ListEntry;
	PDEVICE_OBJECT Pdo;
} I8042_DEVICE, *PI8042_DEVICE;

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT KeyboardObject;
	PDEVICE_OBJECT MouseObject;

	CONNECT_DATA KeyboardData;
	CONNECT_DATA MouseData;

	BOOLEAN KeyboardExists;
	BOOLEAN KeyboardIsAT;
	BOOLEAN MouseExists;

	BOOLEAN KeyboardClaimed;
	BOOLEAN MouseClaimed;

	ULONG BusNumber;
	LIST_ENTRY BusDevices;

	INTERNAL_I8042_START_INFORMATION KeyboardStartInformation;
	INTERNAL_I8042_START_INFORMATION MouseStartInformation;

	INTERNAL_I8042_HOOK_KEYBOARD KeyboardHook;
	INTERNAL_I8042_HOOK_MOUSE MouseHook;

	PKINTERRUPT KeyboardInterruptObject;
	PKINTERRUPT MouseInterruptObject;
	PKINTERRUPT HighestDIRQLInterrupt;
	KSPIN_LOCK SpinLock;
	KDPC DpcKbd;
	KDPC DpcMouse;

	KTIMER TimerMouseTimeout;
	KDPC DpcMouseTimeout;
	MOUSE_TIMEOUT_STATE MouseTimeoutState;
	BOOLEAN MouseTimeoutActive;

	KEYBOARD_ATTRIBUTES KeyboardAttributes;
	KEYBOARD_INDICATOR_PARAMETERS KeyboardIndicators;
	KEYBOARD_TYPEMATIC_PARAMETERS KeyboardTypematic;

	BOOLEAN WantAck;
	BOOLEAN WantOutput;
	BOOLEAN SignalEvent;

	KEYBOARD_SCAN_STATE KeyboardScanState;
	BOOLEAN KeyComplete;
	KEYBOARD_INPUT_DATA *KeyboardBuffer;
	ULONG KeysInBuffer;

	MOUSE_ATTRIBUTES MouseAttributes;

	MOUSE_STATE MouseState;
	BOOLEAN MouseComplete;
	MOUSE_RESET_SUBSTATE MouseResetState;
	MOUSE_INPUT_DATA *MouseBuffer;
	ULONG MouseInBuffer;
	USHORT MouseButtonState;
	ULARGE_INTEGER MousePacketStartTime;

	UCHAR MouseLogiBuffer[3];
	UCHAR MouseLogitechID;
	I8042_MOUSE_TYPE MouseType;

	OUTPUT_PACKET Packet;
	UINT PacketResends;
	BOOLEAN PacketComplete;
	NTSTATUS PacketResult;
	UCHAR PacketBuffer[16];
	UCHAR PacketPort;

	PIRP CurrentIrp;
	PDEVICE_OBJECT CurrentIrpDevice;

	/* registry config values */
	I8042_SETTINGS Settings;

	/* Debugger stuff */
	BOOLEAN TabPressed;
	ULONG DebugKey;
	PIO_WORKITEM DebugWorkItem;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
	PDEVICE_EXTENSION PortDevExt;
	I8042_DEVICE_TYPE Type;
	PDEVICE_OBJECT DeviceObject;

	LIST_ENTRY BusDevices;
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _I8042_HOOK_WORKITEM
{
	PIO_WORKITEM WorkItem;
	PDEVICE_OBJECT Target;
	PIRP Irp;
} I8042_HOOK_WORKITEM, *PI8042_HOOK_WORKITEM;

/*
 * Some defines
 */
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_I8042 TAG('8', '0', '4', '2')

#define KBD_WRAP_MASK      0x1F

#define ALT_PRESSED			(LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)
#define CTRL_PRESSED			(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)


/*
 * Keyboard controller ports
 */

#define I8042_DATA_PORT      0x60
#define I8042_CTRL_PORT      0x64


/*
 * Controller commands
 */

#define KBD_READ_MODE      0x20
#define KBD_WRITE_MODE     0x60
#define KBD_SELF_TEST      0xAA
#define KBD_LINE_TEST      0xAB
#define KBD_CTRL_ENABLE    0xAE

#define MOUSE_LINE_TEST    0xA9
#define MOUSE_CTRL_ENABLE  0xA8

#define KBD_READ_OUTPUT_PORT 0xD0
#define KBD_WRITE_OUTPUT_PORT 0xD1

/*
 * Keyboard commands
 */

#define KBD_SET_LEDS       0xED
#define KBD_GET_ID         0xF2
#define KBD_ENABLE         0xF4
#define KBD_DISABLE        0xF5
#define KBD_RESET          0xFF


/*
 * Keyboard responces
 */

#define KBD_BATCC          0xAA
#define KBD_ACK            0xFA
#define KBD_NACK           0xFC
#define KBD_RESEND         0xFE

/*
 * Controller status register bits
 */

#define KBD_OBF            0x01
#define KBD_IBF            0x02
#define KBD_AUX            0x10
#define KBD_GTO            0x40
#define KBD_PERR           0x80


/*
 * LED bits
 */

#define KBD_LED_SCROLL     0x01
#define KBD_LED_NUM        0x02
#define KBD_LED_CAPS       0x04

/*
 * Mouse responses
 */
#define MOUSE_ACK          0xFA
#define MOUSE_ERROR        0xFC
#define MOUSE_NACK         0xFE

/* i8042prt.c */
NTSTATUS I8042ReadData(BYTE *Data);

NTSTATUS I8042ReadStatus(BYTE *Status);

NTSTATUS I8042ReadDataWait(PDEVICE_EXTENSION DevExt, BYTE *Data);

VOID I8042Flush();

VOID STDCALL I8042IsrWritePort(PDEVICE_EXTENSION DevExt,
                               UCHAR Value,
                               UCHAR SelectCmd);

NTSTATUS STDCALL I8042SynchWritePort(PDEVICE_EXTENSION DevExt,
                                     UCHAR Port,
                                     UCHAR Value,
                                     BOOLEAN WaitForAck);

NTSTATUS STDCALL I8042StartPacket(PDEVICE_EXTENSION DevExt,
                                  PDEVICE_OBJECT Device,
                                  PUCHAR Bytes,
                                  ULONG ByteCount,
                                  PIRP Irp);

BOOLEAN STDCALL I8042PacketIsr(PDEVICE_EXTENSION DevExt,
                            UCHAR Output);

VOID I8042PacketDpc(PDEVICE_EXTENSION DevExt);

VOID STDCALL I8042SendHookWorkItem(PDEVICE_OBJECT DeviceObject,
                                   PVOID Context);

BOOLEAN I8042Write(PDEVICE_EXTENSION DevExt, int addr, BYTE data);

/* keyboard.c */
VOID STDCALL I8042IsrWritePortKbd(PVOID Context,
                                  UCHAR Value);

NTSTATUS STDCALL I8042SynchWritePortKbd(PVOID Context,
                                        UCHAR Value,
                                        BOOLEAN WaitForAck);

BOOLEAN STDCALL I8042InterruptServiceKbd(struct _KINTERRUPT *Interrupt,
                                         VOID * Context);

VOID STDCALL I8042DpcRoutineKbd(PKDPC Dpc,
                                PVOID DeferredContext,
                                PVOID SystemArgument1,
                                PVOID SystemArgument2);

BOOLEAN STDCALL I8042StartIoKbd(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS STDCALL I8042InternalDeviceControlKbd(PDEVICE_OBJECT DeviceObject,
                                               PIRP Irp);

BOOLEAN STDCALL I8042KeyboardEnable(PDEVICE_EXTENSION DevExt);

BOOLEAN STDCALL I8042KeyboardEnableInterrupt(PDEVICE_EXTENSION DevExt);

BOOLEAN STDCALL I8042DetectKeyboard(PDEVICE_EXTENSION DevExt);

/* registry.c */
VOID STDCALL I8042ReadRegistry(PDRIVER_OBJECT DriverObject,
                               PDEVICE_EXTENSION DevExt);

/* mouse.c */
VOID STDCALL I8042DpcRoutineMouse(PKDPC Dpc,
                                  PVOID DeferredContext,
                                  PVOID SystemArgument1,
                                  PVOID SystemArgument2);

VOID STDCALL I8042DpcRoutineMouseTimeout(PKDPC Dpc,
                                         PVOID DeferredContext,
                                         PVOID SystemArgument1,
                                         PVOID SystemArgument2);

BOOLEAN STDCALL I8042InterruptServiceMouse(struct _KINTERRUPT *Interrupt,
                                           VOID *Context);

NTSTATUS STDCALL I8042InternalDeviceControlMouse(PDEVICE_OBJECT DeviceObject,
                                                 PIRP Irp);

VOID STDCALL I8042QueueMousePacket(PVOID Context);

VOID STDCALL I8042MouseHandleButtons(PDEVICE_EXTENSION DevExt,
                                     USHORT Mask);

VOID STDCALL I8042MouseHandle(PDEVICE_EXTENSION DevExt,
                              BYTE Output);

BOOLEAN STDCALL I8042MouseEnable(PDEVICE_EXTENSION DevExt);
BOOLEAN STDCALL I8042MouseDisable(PDEVICE_EXTENSION DevExt);

/* ps2pp.c */
VOID I8042MouseHandlePs2pp(PDEVICE_EXTENSION DevExt, BYTE Input);

#endif // _KEYBOARD_H_
