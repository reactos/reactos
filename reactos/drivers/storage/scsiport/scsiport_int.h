/*
 * SCSI_PORT_TIMER_STATES
 *
 * DESCRIPTION
 *	An enumeration containing the states in the timer DFA
 */

#define VERSION "0.0.2"

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

  ULONG PendingIrpCount;
  ULONG ActiveIrpCount;
  ULONG NextLuRequestCount;

  PIRP NextIrp;

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

  KSPIN_LOCK Lock;
  ULONG Flags;

  PKINTERRUPT Interrupt;

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


  /* DMA related stuff */
  PADAPTER_OBJECT AdapterObject;
  ULONG MapRegisterCount;

  PHYSICAL_ADDRESS PhysicalAddress;
  PVOID VirtualAddress;
  ULONG VirtualAddressMap;
  ULONG CommonBufferLength;

  LIST_ENTRY PendingIrpListHead;
  PIRP NextIrp;
  ULONG PendingIrpCount;
  ULONG ActiveIrpCount;

  ULONG CompleteRequestCount;
  ULONG NextRequestCount;
  ULONG NextLuRequestCount;

  UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;

typedef struct _SCSI_PORT_SCAN_ADAPTER
{
  KEVENT Event;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  ULONG Lun;
  ULONG Bus;
  ULONG Target;
  SCSI_REQUEST_BLOCK Srb;
  UCHAR DataBuffer[256];
  BOOL Active;
} SCSI_PORT_SCAN_ADAPTER, *PSCSI_PORT_SCAN_ADAPTER;


