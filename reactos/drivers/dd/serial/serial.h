#if defined(__GNUC__)
  #include <ddk/ntddk.h>
  #include <ddk/ntddser.h>
  #include <stdio.h>
  
  #include <debug.h>
  
  /* FIXME: this prototype MUST NOT be here! */
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
  #define CHECKPOINT1 DbgPrint("(%s:%d)\n")
  
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

typedef enum {
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} SERIAL_DEVICE_STATE;

typedef struct _SERIAL_DEVICE_EXTENSION
{
	PDEVICE_OBJECT Pdo;
	PDEVICE_OBJECT LowerDevice;
	SERIAL_DEVICE_STATE PnpState;
	IO_REMOVE_LOCK RemoveLock;
	
	ULONG SerialPortNumber;
	
	ULONG ComPort; /* FIXME: move to serenum */
	ULONG BaudRate;
	ULONG BaseAddress;
	ULONG Irq;
	PKINTERRUPT Interrupt;
	
	SERIAL_LINE_CONTROL SerialLineControl;
	ULONG WaitMask;
	
	/* Current values */
	UCHAR IER; /* Base+1, Interrupt Enable Register */
	UCHAR MCR; /* Base+4, Modem Control Register */
	UCHAR MSR; /* Base+6, Modem Status Register */
} SERIAL_DEVICE_EXTENSION, *PSERIAL_DEVICE_EXTENSION;

#define SERIAL_TAG TAG('S', 'e', 'r', 'l')

/* Baud master clock */
#define BAUD_CLOCK      1843200
#define CLOCKS_PER_BIT  16 

/* UART registers and bits */
#define   SER_RBR(x)   ((x)+0)
#define   SER_THR(x)   ((x)+0)
#define   SER_DLL(x)   ((x)+0)
#define   SER_IER(x)   ((x)+1)
#define   SER_DLM(x)   ((x)+1)
#define   SER_FCR(x)   ((x)+1)
#define   SER_IIR(x)   ((x)+2)
#define     SR_IIR_SELF          0x01
#define     SR_IIR_ID_MASK       0x07
#define     SR_IIR_MSR_CHANGE    SR_IIR_SELF
#define     SR_IIR_THR_EMPTY     (SR_IIR_SELF | 2)
#define     SR_IIR_DATA_RECEIVED (SR_IIR_SELF | 4)
#define     SR_IIR_ERROR         (SR_IIR_SELF | 6)
#define   SER_LCR(x)   ((x)+3)
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
#define   SER_MCR(x)   ((x)+4)
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define   SER_LSR(x)   ((x)+5)
#define     SR_LSR_DR  0x01
#define     SR_LSR_TBE 0x20
#define   SER_MSR(x)   ((x)+6)
#define     SR_MSR_CTS 0x10
#define     SR_MSR_DSR 0x20
#define   SER_SCR(x)   ((x)+7)

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

/************************************ misc.c */

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp);

BOOLEAN STDCALL
SerialInterruptService(
	IN PKINTERRUPT Interrupt,
	IN OUT PVOID ServiceContext);

/************************************ pnp.c */

NTSTATUS STDCALL
SerialAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo);

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

