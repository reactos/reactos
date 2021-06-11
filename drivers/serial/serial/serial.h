/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial driver
 * FILE:            drivers/dd/serial/serial.h
 * PURPOSE:         Serial driver header
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#ifndef _SERIAL_PCH_
#define _SERIAL_PCH_

#include <ntddk.h>
#include <ntddser.h>

/* See winbase.h */
#define PST_RS232 1
#define COMMPROP_INITIALIZED 0xE73CF52E

typedef enum
{
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} SERIAL_DEVICE_STATE;

typedef enum
{
	UartUnknown,
	Uart8250,  /* initial version */
	Uart16450, /* + 38.4 Kbps */
	Uart16550, /* + 115 Kbps */
	Uart16550A,/* + FIFO 16 bytes */
	Uart16650, /* + FIFO 32 bytes, 230 Kbps, power management, auto-flow */
	Uart16750  /* + FIFO 64 bytes, 460 Kbps */
} UART_TYPE;

typedef struct _CIRCULAR_BUFFER
{
	PUCHAR Buffer;
	ULONG Length;
	ULONG ReadPosition;
	ULONG WritePosition;
} CIRCULAR_BUFFER, *PCIRCULAR_BUFFER;

typedef struct _SERIAL_DEVICE_EXTENSION
{
	PDEVICE_OBJECT Pdo;
	PDEVICE_OBJECT LowerDevice;
	SERIAL_DEVICE_STATE PnpState;
	IO_REMOVE_LOCK RemoveLock;

	ULONG SerialPortNumber;

	ULONG ComPort;
	ULONG BaudRate;
	ULONG BaseAddress;
	PKINTERRUPT Interrupt;
	KDPC ReceivedByteDpc;
	KDPC SendByteDpc;
	KDPC CompleteIrpDpc;

	SERIAL_LINE_CONTROL SerialLineControl;
	UART_TYPE UartType;
	ULONG WaitMask;
	PIRP WaitOnMaskIrp;

	ULONG BreakInterruptErrorCount;
	SERIALPERF_STATS SerialPerfStats;
	SERIAL_TIMEOUTS SerialTimeOuts;
	BOOLEAN IsOpened;
	KEVENT InputBufferNotEmpty;
	CIRCULAR_BUFFER InputBuffer;
	KSPIN_LOCK InputBufferLock;
	CIRCULAR_BUFFER OutputBuffer;
	KSPIN_LOCK OutputBufferLock;

	UNICODE_STRING SerialInterfaceName;

	/* Current values */
	UCHAR MCR; /* Base+4, Modem Control Register */
	UCHAR MSR; /* Base+6, Modem Status Register */
} SERIAL_DEVICE_EXTENSION, *PSERIAL_DEVICE_EXTENSION;

typedef struct _WORKITEM_DATA
{
	PIRP Irp;
	PIO_WORKITEM IoWorkItem;

	BOOLEAN UseIntervalTimeout;
	BOOLEAN UseTotalTimeout;
	LARGE_INTEGER IntervalTimeout;
	LARGE_INTEGER TotalTimeoutTime;
	BOOLEAN DontWait;
	BOOLEAN ReadAtLeastOneByte;
} WORKITEM_DATA, *PWORKITEM_DATA;

#define SERIAL_TAG 'lreS'

#define INFINITE MAXULONG

/* Baud master clock */
#define BAUD_CLOCK      1843200
#define CLOCKS_PER_BIT  16

/* UART registers and bits */
#define   SER_RBR(x)   ((PUCHAR)(x)+0) /* Receive Register */
#define   SER_THR(x)   ((PUCHAR)(x)+0) /* Transmit Register */
#define   SER_DLL(x)   ((PUCHAR)(x)+0) /* Baud Rate Divisor LSB */
#define   SER_IER(x)   ((PUCHAR)(x)+1) /* Interrupt Enable Register */
#define     SR_IER_DATA_RECEIVED  0x01
#define     SR_IER_THR_EMPTY      0x02
#define     SR_IER_LSR_CHANGE     0x04
#define     SR_IER_MSR_CHANGE     0x08
#define     SR_IER_SLEEP_MODE     0x10 /* Uart >= 16750 */
#define     SR_IER_LOW_POWER      0x20 /* Uart >= 16750 */
#define   SER_DLM(x)   ((PUCHAR)(x)+1) /* Baud Rate Divisor MSB */
#define   SER_IIR(x)   ((PUCHAR)(x)+2) /* Interrupt Identification Register */
#define     SR_IIR_SELF           0x00
#define     SR_IIR_ID_MASK        0x07
#define     SR_IIR_MSR_CHANGE     SR_IIR_SELF
#define     SR_IIR_THR_EMPTY     (SR_IIR_SELF | 2)
#define     SR_IIR_DATA_RECEIVED (SR_IIR_SELF | 4)
#define     SR_IIR_ERROR         (SR_IIR_SELF | 6)
#define   SER_FCR(x)   ((PUCHAR)(x)+2) /* FIFO Control Register (Uart >= 16550A) */
#define     SR_FCR_ENABLE_FIFO    0x01
#define     SR_FCR_CLEAR_RCVR    (0x02 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_CLEAR_XMIT    (0x04 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_1_BYTE        (0x00 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_4_BYTES       (0x40 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_8_BYTES       (0x80 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_14_BYTES      (0xC0 | SR_FCR_ENABLE_FIFO)
#define   SER_LCR(x)   ((PUCHAR)(x)+3) /* Line Control Register */
#define     SR_LCR_CS5            0x00
#define     SR_LCR_CS6            0x01
#define     SR_LCR_CS7            0x02
#define     SR_LCR_CS8            0x03
#define     SR_LCR_ST1            0x00
#define     SR_LCR_ST2            0x04
#define     SR_LCR_PNO            0x00
#define     SR_LCR_POD            0x08
#define     SR_LCR_PEV            0x18
#define     SR_LCR_PMK            0x28
#define     SR_LCR_PSP            0x38
#define     SR_LCR_BRK            0x40
#define     SR_LCR_DLAB           0x80
#define   SER_MCR(x)   ((PUCHAR)(x)+4) /* Modem Control Register */
#define     SR_MCR_DTR            SERIAL_DTR_STATE
#define     SR_MCR_RTS            SERIAL_RTS_STATE
#define   SER_LSR(x)   ((PUCHAR)(x)+5) /* Line Status Register */
#define     SR_LSR_DATA_RECEIVED  0x01
#define     SR_LSR_OVERRUN_ERROR  0x02
#define     SR_LSR_PARITY_ERROR   0x04
#define     SR_LSR_FRAMING_ERROR  0x08
#define     SR_LSR_BREAK_INT      0x10
#define     SR_LSR_THR_EMPTY      0x20
#define     SR_LSR_TSR_EMPTY      0x40
#define     SR_LSR_ERROR_IN_FIFO  0x80 /* Uart >= 16550A */
#define   SER_MSR(x)   ((PUCHAR)(x)+6) /* Modem Status Register */
#define     SR_MSR_CTS_CHANGED    0x01
#define     SR_MSR_DSR_CHANGED    0x02
#define     SR_MSR_RI_CHANGED     0x04
#define     SR_MSR_DCD_CHANGED    0x08
#define     SR_MSR_CTS            SERIAL_CTS_STATE /* Clear To Send */
#define     SR_MSR_DSR            SERIAL_DSR_STATE /* Data Set Ready */
#define     SI_MSR_RI             SERIAL_RI_STATE  /* Ring Indicator */
#define     SR_MSR_DCD            SERIAL_DCD_STATE /* Data Carrier Detect */
#define   SER_SCR(x)   ((PUCHAR)(x)+7) /* Scratch Pad Register, Uart >= Uart16450 */

/************************************ circularbuffer.c */

/* FIXME: transform these functions into #define? */
NTSTATUS
InitializeCircularBuffer(
	IN PCIRCULAR_BUFFER pBuffer,
	IN ULONG BufferSize);

NTSTATUS
FreeCircularBuffer(
	IN PCIRCULAR_BUFFER pBuffer);

BOOLEAN
IsCircularBufferEmpty(
	IN PCIRCULAR_BUFFER pBuffer);

ULONG
GetNumberOfElementsInCircularBuffer(
	IN PCIRCULAR_BUFFER pBuffer);

NTSTATUS
PushCircularBufferEntry(
	IN PCIRCULAR_BUFFER pBuffer,
	IN UCHAR Entry);

NTSTATUS
PopCircularBufferEntry(
	IN PCIRCULAR_BUFFER pBuffer,
	OUT PUCHAR Entry);

NTSTATUS
IncreaseCircularBufferSize(
	IN PCIRCULAR_BUFFER pBuffer,
	IN ULONG NewBufferSize);

/************************************ cleanup.c */

DRIVER_DISPATCH SerialCleanup;

/************************************ close.c */

DRIVER_DISPATCH SerialClose;

/************************************ create.c */

DRIVER_DISPATCH SerialCreate;

/************************************ devctrl.c */

DRIVER_DISPATCH SerialDeviceControl;

NTSTATUS NTAPI
SerialSetBaudRate(
	IN PSERIAL_DEVICE_EXTENSION DeviceExtension,
	IN ULONG NewBaudRate);

NTSTATUS NTAPI
SerialSetLineControl(
	IN PSERIAL_DEVICE_EXTENSION DeviceExtension,
	IN PSERIAL_LINE_CONTROL NewSettings);

/************************************ info.c */

DRIVER_DISPATCH SerialQueryInformation;

/************************************ legacy.c */

UART_TYPE
SerialDetectUartType(
	IN PUCHAR ComPortBase);

/************************************ misc.c */

DRIVER_DISPATCH ForwardIrpAndForget;

VOID NTAPI
SerialReceiveByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID Unused1,
	IN PVOID Unused2);

VOID NTAPI
SerialSendByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID Unused1,
	IN PVOID Unused2);

VOID NTAPI
SerialCompleteIrp(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID pIrp, // real type PIRP
	IN PVOID Unused);

KSERVICE_ROUTINE SerialInterruptService;

/************************************ pnp.c */

NTSTATUS NTAPI
SerialAddDeviceInternal(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo,
	IN UART_TYPE UartType,
	IN PULONG pComPortNumber OPTIONAL,
	OUT PDEVICE_OBJECT* pFdo OPTIONAL);

DRIVER_ADD_DEVICE SerialAddDevice;

NTSTATUS NTAPI
SerialPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCM_RESOURCE_LIST ResourceList,
	IN PCM_RESOURCE_LIST ResourceListTranslated);

DRIVER_DISPATCH SerialPnp;

/************************************ power.c */

DRIVER_DISPATCH SerialPower;

/************************************ rw.c */

DRIVER_DISPATCH SerialRead;
DRIVER_DISPATCH SerialWrite;

#endif /* _SERIAL_PCH_ */
