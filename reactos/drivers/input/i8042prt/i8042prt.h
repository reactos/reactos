#ifndef _I8042DRV_H
#define _I8042DRV_H

#include <ntddk.h>
#include <kbdmou.h>
#include <ntdd8042.h>

#ifdef _MSC_VER
  #define STDCALL
  #define DDKAPI
#endif

#define KEYBOARD_IRQ       1
#define MOUSE_IRQ          12
#define KBD_BUFFER_SIZE    32

#define WHEEL_DELTA 120

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
	ULONG Headless;               /* done */
	ULONG CrashScroll;
	ULONG CrashSysRq;             /* done */
	ULONG ReportResetErrors;
	ULONG PollStatusIterations;   /* done */
	ULONG ResendIterations;       /* done */
	ULONG PollingIterations;
	ULONG PollingIterationsMaximum;
	ULONG OverrideKeyboardType;
	ULONG OverrideKeyboardSubtype;
	ULONG MouseResendStallTime;
	ULONG MouseSynchIn100ns;      /* done */
	ULONG MouseResolution;        /* done */
	ULONG NumberOfButtons;
	ULONG EnableWheelDetection;
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
	ULONG PacketResends;
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

#define I8042_DATA_PORT      ((PUCHAR)0x60)
#define I8042_CTRL_PORT      ((PUCHAR)0x64)


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
 * Controller command byte bits
 */
#define CCB_KBD_INT_ENAB   0x01
#define CCB_MOUSE_INT_ENAB 0x02
#define CCB_SYSTEM_FLAG    0x04
#define CCB_IGN_KEY_LOCK   0x08
#define CCB_KBD_DISAB      0x10
#define CCB_MOUSE_DISAB    0x20
#define CCB_TRANSLATE      0x40


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
extern UNICODE_STRING I8042RegistryPath;

NTSTATUS I8042ReadData(UCHAR *Data);

NTSTATUS I8042ReadStatus(UCHAR *Status);

NTSTATUS I8042ReadDataWait(PDEVICE_EXTENSION DevExt, UCHAR *Data);

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

IO_WORKITEM_ROUTINE I8042SendHookWorkItem;
VOID STDCALL I8042SendHookWorkItem(PDEVICE_OBJECT DeviceObject,
                                   PVOID Context);

BOOLEAN I8042Write(PDEVICE_EXTENSION DevExt, PUCHAR addr, UCHAR data);

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT DriverObject,
			     PUNICODE_STRING RegistryPath);

/* keyboard.c */
VOID STDCALL I8042IsrWritePortKbd(PVOID Context,
                                  UCHAR Value);

NTSTATUS STDCALL I8042SynchWritePortKbd(PVOID Context,
                                        UCHAR Value,
                                        BOOLEAN WaitForAck);

KSERVICE_ROUTINE I8042InterruptServiceKbd;
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

KSERVICE_ROUTINE I8042InterruptServiceMouse;
BOOLEAN STDCALL I8042InterruptServiceMouse(struct _KINTERRUPT *Interrupt,
                                           VOID *Context);

NTSTATUS STDCALL I8042InternalDeviceControlMouse(PDEVICE_OBJECT DeviceObject,
                                                 PIRP Irp);

VOID STDCALL I8042QueueMousePacket(PVOID Context);

VOID STDCALL I8042MouseHandleButtons(PDEVICE_EXTENSION DevExt,
                                     USHORT Mask);

VOID STDCALL I8042MouseHandle(PDEVICE_EXTENSION DevExt,
                              UCHAR Output);

BOOLEAN STDCALL I8042MouseEnable(PDEVICE_EXTENSION DevExt);
BOOLEAN STDCALL I8042MouseDisable(PDEVICE_EXTENSION DevExt);
BOOLEAN STDCALL I8042DetectMouse(PDEVICE_EXTENSION DevExt);

/* ps2pp.c */
VOID I8042MouseHandlePs2pp(PDEVICE_EXTENSION DevExt, UCHAR Input);

#endif // _KEYBOARD_H_
