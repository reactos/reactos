#if defined(__GNUC__)
  #include <ddk/ntddk.h>
  #include <ddk/ntddser.h>
  #include <stdio.h>
  
  #include <debug.h>
  
  /* FIXME: these prototypes MUST NOT be here! */
  NTSTATUS STDCALL
  IoAttachDeviceToDeviceStackSafe(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject);
  
#elif defined(_MSC_VER)
  #include <ntddk.h>
  #include <ntddser.h>
  #include <stdio.h>
  
  #define STDCALL
  
  #define DPRINT1 DbgPrint("(%s:%d) ", __FILE__, __LINE__), DbgPrint
  #define CHECKPOINT1 DbgPrint("(%s:%d)\n", __FILE__, __LINE__)
  
  #define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
  
  NTSTATUS STDCALL
  IoAttachDeviceToDeviceStackSafe(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PDEVICE_OBJECT *AttachedToDeviceObject);
  
  #ifdef NDEBUG2
    #define DPRINT
    #define CHECKPOINT
  #else
    #define DPRINT DPRINT1
    #define CHECKPOINT CHECKPOINT1
    #undef NDEBUG
  #endif
#else
  #error Unknown compiler!
#endif

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
	
	SERIAL_LINE_CONTROL SerialLineControl;
	UART_TYPE UartType;
	ULONG WaitMask;
	
	SERIALPERF_STATS SerialPerfStats;
	SERIAL_TIMEOUTS SerialTimeOuts;
	BOOLEAN IsOpened;
	KEVENT InputBufferNotEmpty;
	CIRCULAR_BUFFER InputBuffer;
	KSPIN_LOCK InputBufferLock;
	CIRCULAR_BUFFER OutputBuffer;
	KSPIN_LOCK OutputBufferLock;
	
	/* Current values */
	UCHAR MCR; /* Base+4, Modem Control Register */
	UCHAR MSR; /* Base+6, Modem Status Register */
} SERIAL_DEVICE_EXTENSION, *PSERIAL_DEVICE_EXTENSION;

typedef struct _WORKITEM_DATA
{
	PIRP Irp;
	
	BOOLEAN UseIntervalTimeout;
	BOOLEAN UseTotalTimeout;
	LARGE_INTEGER IntervalTimeout;
	LARGE_INTEGER TotalTimeoutTime;
	BOOLEAN DontWait;
	BOOLEAN ReadAtLeastOneByte;
} WORKITEM_DATA, *PWORKITEM_DATA;

#define SERIAL_TAG TAG('S', 'e', 'r', 'l')

#define INFINITE ((ULONG)-1)

/* Baud master clock */
#define BAUD_CLOCK      1843200
#define CLOCKS_PER_BIT  16

/* UART registers and bits */
#define   SER_RBR(x)   ((x)+0) /* Receive Register */
#define   SER_THR(x)   ((x)+0) /* Transmit Register */
#define   SER_DLL(x)   ((x)+0) /* Baud Rate Divisor LSB */
#define   SER_IER(x)   ((x)+1) /* Interrupt Enable Register */
#define     SR_IER_DATA_RECEIVED 0x01
#define     SR_IER_THR_EMPTY     0x02
#define     SR_IER_LSR_CHANGE    0x04
#define     SR_IER_MSR_CHANGE    0x08
#define     SR_IER_SLEEP_MODE    0x10 /* Uart >= 16750 */
#define     SR_IER_LOW_POWER     0x20 /* Uart >= 16750 */
#define   SER_DLM(x)   ((x)+1) /* Baud Rate Divisor MSB */
#define   SER_IIR(x)   ((x)+2) /* Interrupt Identification Register */
#define     SR_IIR_SELF          0x00
#define     SR_IIR_ID_MASK       0x07
#define     SR_IIR_MSR_CHANGE    SR_IIR_SELF
#define     SR_IIR_THR_EMPTY     (SR_IIR_SELF | 2)
#define     SR_IIR_DATA_RECEIVED (SR_IIR_SELF | 4)
#define     SR_IIR_ERROR         (SR_IIR_SELF | 6)
#define   SER_FCR(x)   ((x)+2) /* FIFO Control Register (Uart >= 16550A) */
#define     SR_FCR_ENABLE_FIFO 0x01
#define     SR_FCR_CLEAR_RCVR  (0x02 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_CLEAR_XMIT  (0x04 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_1_BYTE      (0x00 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_4_BYTES     (0x40 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_8_BYTES     (0x80 | SR_FCR_ENABLE_FIFO)
#define     SR_FCR_14_BYTES    (0xC0 | SR_FCR_ENABLE_FIFO)
#define   SER_LCR(x)   ((x)+3) /* Line Control Register */
#define     SR_LCR_CS5 0x00
#define     SR_LCR_CS6 0x01
#define     SR_LCR_CS7 0x02
#define     SR_LCR_CS8 0x03
#define     SR_LCR_ST1 0x00
#define     SR_LCR_ST2 0x04
#define     SR_LCR_PNO 0x00
#define     SR_LCR_POD 0x08
#define     SR_LCR_PEV 0x18
#define     SR_LCR_PMK 0x28
#define     SR_LCR_PSP 0x38
#define     SR_LCR_BRK 0x40
#define     SR_LCR_DLAB 0x80
#define   SER_MCR(x)   ((x)+4) /* Modem Control Register */
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define   SER_LSR(x)   ((x)+5) /* Line Status Register */
#define     SR_LSR_DR  0x01
#define     SR_LSR_TBE 0x20
#define   SER_MSR(x)   ((x)+6) /* Modem Status Register */
#define     SR_MSR_CTS 0x10
#define     SR_MSR_DSR 0x20
#define   SER_SCR(x)   ((x)+7) /* Scratch Pad Register */

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

NTSTATUS STDCALL
SerialCleanup(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ close.c */

NTSTATUS STDCALL
SerialClose(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ create.c */

NTSTATUS STDCALL
SerialCreate(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ devctrl.c */

NTSTATUS STDCALL
SerialDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
SerialSetBaudRate(
	IN PSERIAL_DEVICE_EXTENSION DeviceExtension,
	IN ULONG NewBaudRate);

NTSTATUS STDCALL
SerialSetLineControl(
	IN PSERIAL_DEVICE_EXTENSION DeviceExtension,
	IN PSERIAL_LINE_CONTROL NewSettings);

/************************************ info.c */

NTSTATUS STDCALL
SerialQueryInformation(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ legacy.c */

UART_TYPE
SerialDetectUartType(
	IN PUCHAR ComPortBase);

NTSTATUS
DetectLegacyDevices(
	IN PDRIVER_OBJECT DriverObject);

/************************************ misc.c */

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

VOID STDCALL
SerialReceiveByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID pByte,            // real type UCHAR
	IN PVOID Unused);

VOID STDCALL
SerialSendByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID Unused1,
	IN PVOID Unused2);

BOOLEAN STDCALL
SerialInterruptService(
	IN PKINTERRUPT Interrupt,
	IN OUT PVOID ServiceContext);

/************************************ pnp.c */

NTSTATUS STDCALL
SerialAddDeviceInternal(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo,
	IN UART_TYPE UartType,
	OUT PDEVICE_OBJECT* pFdo OPTIONAL);

NTSTATUS STDCALL
SerialAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

NTSTATUS STDCALL
SerialPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCM_RESOURCE_LIST ResourceList);

NTSTATUS STDCALL
SerialPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ power.c */

NTSTATUS STDCALL
SerialPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

/************************************ rw.c */

NTSTATUS STDCALL
SerialRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

NTSTATUS STDCALL
SerialWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

