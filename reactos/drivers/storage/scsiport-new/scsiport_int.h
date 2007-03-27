/*
 * SCSI_PORT_TIMER_STATES
 *
 * DESCRIPTION
 *	An enumeration containing the states in the timer DFA
 */

#define VERSION "0.0.3"

#ifndef PAGE_ROUND_UP
#define PAGE_ROUND_UP(x) ( (((ULONG_PTR)x)%PAGE_SIZE) ? ((((ULONG_PTR)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG_PTR)x) )
#endif
#ifndef ROUND_UP
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#endif

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

typedef enum _SCSI_PORT_TIMER_STATES
{
  IDETimerIdle,
  IDETimerCmdWait,
  IDETimerResetWaitForBusyNegate,
  IDETimerResetWaitForDrdyAssert
} SCSI_PORT_TIMER_STATES;


typedef struct _SCSI_PORT_DEVICE_BASE
{
  LIST_ENTRY List;

  PVOID MappedAddress;
  ULONG NumberOfBytes;
  SCSI_PHYSICAL_ADDRESS IoAddress;
  ULONG SystemIoBusNumber;
} SCSI_PORT_DEVICE_BASE, *PSCSI_PORT_DEVICE_BASE;


typedef struct _SCSI_PORT_LUN_EXTENSION
{
  LIST_ENTRY List;

  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;

  BOOLEAN DeviceClaimed;
  PDEVICE_OBJECT DeviceObject;

  INQUIRYDATA InquiryData;

  KDEVICE_QUEUE DeviceQueue;

  /* More data? */

  UCHAR MiniportLunExtension[1]; /* must be the last entry */
} SCSI_PORT_LUN_EXTENSION, *PSCSI_PORT_LUN_EXTENSION;


/*
 * SCSI_PORT_DEVICE_EXTENSION
 *
 * DESCRIPTION
 *	First part of the port objects device extension. The second
 *	part is the miniport-specific device extension.
 */

typedef struct _SCSI_PORT_DEVICE_EXTENSION
{
  ULONG Length;
  ULONG MiniPortExtensionSize;
  PPORT_CONFIGURATION_INFORMATION PortConfig;
  ULONG PortNumber;

  KSPIN_LOCK IrpLock;
  KSPIN_LOCK SpinLock;
  PKINTERRUPT Interrupt;
  PIRP                   CurrentIrp;
  ULONG IrpFlags;

  SCSI_PORT_TIMER_STATES TimerState;
  LONG                   TimerCount;

  LIST_ENTRY DeviceBaseListHead;

  ULONG LunExtensionSize;
  LIST_ENTRY LunExtensionListHead;

  ULONG SrbExtensionSize;

  PIO_SCSI_CAPABILITIES PortCapabilities;

  PDEVICE_OBJECT DeviceObject;
  PCONTROLLER_OBJECT ControllerObject;

  PHW_STARTIO HwStartIo;
  PHW_INTERRUPT HwInterrupt;

  PSCSI_REQUEST_BLOCK OriginalSrb;
  SCSI_REQUEST_BLOCK InternalSrb;
  SENSE_DATA InternalSenseData;

  /* DMA related stuff */
  PADAPTER_OBJECT AdapterObject;
  ULONG MapRegisterCount;

  PHYSICAL_ADDRESS PhysicalAddress;
  PVOID VirtualAddress;
  ULONG CommonBufferLength;

  UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;
