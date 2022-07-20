#ifndef _USBSTOR_H_
#define _USBSTOR_H_

#include <wdm.h>
#include <usbdi.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <classpnp.h>

#define USB_STOR_TAG 'sbsu'

#ifndef BooleanFlagOn
#define BooleanFlagOn(Flags, SingleFlag) ((BOOLEAN)((((Flags) & (SingleFlag)) != 0)))
#endif

#ifndef SrbGetCdb
#define SrbGetCdb(srb) ((PCDB)(srb->Cdb))
#endif

// Storage subclass codes

#define USB_SUBCLASS_RBC        0x01    // Typically, flash devices
#define USB_SUBCLASS_8020       0x02    // CD-ROM
#define USB_SUBCLASS_QIC        0x03    // QIC-157 Tapes
#define USB_SUBCLASS_UFI        0x04    // Floppy
#define USB_SUBCLASS_8070       0x05    // Removable media
#define USB_SUBCLASS_SCSI       0x06    // Transparent
#define USB_SUBCLASS_LOCKABLE   0x07    // Password-protected

#define USB_SUBCLASS_ISD200     0xF0    // ISD200 ATA
#define USB_SUBCLASS_CYP_ATACB  0xF1    // Cypress ATACB
#define USB_SUBCLASS_VENDOR     0xFF    // Use vendor specific

// Storage protocol codes

#define USB_PROTOCOL_CBI          0x00    // Control/Bulk/Interrupt
#define USB_PROTOCOL_CB           0x01    // Control/Bulk w/o interrupt
#define USB_PROTOCOL_BULK         0x50    // bulk only
#define USB_PROTOCOL_UAS          0x62    // USB Attached SCSI
#define USB_PROTOCOL_USBAT        0x80    // SCM-ATAPI bridge
#define USB_PROTOCOL_EUSB_SDDR09  0x81    // SCM-SCSI bridge for SDDR-09
#define USB_PROTOCOL_SDDR55       0x82    // SDDR-55 (made up)

#define USB_PROTOCOL_DPCM_USB     0xF0    // Combination CB/SDDR09
#define USB_PROTOCOL_FREECOM      0xF1    // Freecom
#define USB_PROTOCOL_DATAFAB      0xF2    // Datafab chipsets
#define USB_PROTOCOL_JUMPSHOT     0xF3    // Lexar Jumpshot
#define USB_PROTOCOL_ALAUDA       0xF4    // Alauda chipsets
#define USB_PROTOCOL_KARMA        0xF5    // Rio Karma
#define USB_PROTOCOL_VENDOR       0xFF    // Use vendor specific

// Mass storage class-specific commands

#define USB_BULK_GET_MAX_LUN   0xFE
#define USB_BULK_RESET_DEVICE  0xFF

#define USB_RECOVERABLE_ERRORS (USBD_STATUS_STALL_PID | USBD_STATUS_DEV_NOT_RESPONDING \
	| USBD_STATUS_ENDPOINT_HALTED | USBD_STATUS_NO_BANDWIDTH)

#define USB_MAXCHILDREN 16
#define MAX_LUN 0xF
#define USBSTOR_DEFAULT_MAX_TRANSFER_LENGTH 0x10000

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

#include <pshpack1.h>

typedef struct
{
    ULONG Signature;                                                 // CBW signature
    ULONG Tag;                                                       // CBW Tag of operation
    ULONG DataTransferLength;                                        // data transfer length
    UCHAR Flags;                                                     // CBW Flags endpoint direction
    UCHAR LUN;                                                       // lun unit
    UCHAR CommandBlockLength;                                        // Command block length
    UCHAR CommandBlock[16];
} CBW, *PCBW;

C_ASSERT(sizeof(CBW) == 31);

#define CSW_STATUS_COMMAND_PASSED 0x00
#define CSW_STATUS_COMMAND_FAILED 0x01
#define CSW_STATUS_PHASE_ERROR    0x02

typedef struct
{
    ULONG Signature;                                                 // CSW signature
    ULONG Tag;                                                       // CSW tag
    ULONG DataResidue;                                               // CSW data transfer diff
    UCHAR Status;                                                    // CSW status
} CSW, *PCSW;

#include <poppack.h>

typedef struct
{
    PIRP Irp;
    ULONG ErrorIndex;
    ULONG StallRetryCount;                                            // the number of retries after receiving USBD_STATUS_STALL_PID status
    union
    {
        CBW cbw;
        CSW csw;
    };
    URB Urb;
    SCSI_REQUEST_BLOCK SenseSrb;
} IRP_CONTEXT, *PIRP_CONTEXT;

typedef struct __COMMON_DEVICE_EXTENSION__
{
    BOOLEAN IsFDO;

}USBSTOR_COMMON_DEVICE_EXTENSION, *PUSBSTOR_COMMON_DEVICE_EXTENSION;

#define USBSTOR_FDO_FLAGS_DEVICE_RESETTING   0x00000001 // hard reset is in progress
#define USBSTOR_FDO_FLAGS_IRP_LIST_FREEZE    0x00000002 // the irp list is freezed

typedef struct
{
    USBSTOR_COMMON_DEVICE_EXTENSION Common;                                                      // common device extension

    PDEVICE_OBJECT FunctionalDeviceObject;                                               // functional device object
    PDEVICE_OBJECT PhysicalDeviceObject;                                                 // physical device object
    PDEVICE_OBJECT LowerDeviceObject;                                                    // lower device object
    USB_BUS_INTERFACE_USBDI_V2 BusInterface;                                             // bus interface of device
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                                             // usb device descriptor
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;                               // usb configuration descriptor
    PUSB_STRING_DESCRIPTOR SerialNumber;                                                 // usb serial number
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;                                    // usb interface information
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;                                       // usb configuration handle
    UCHAR BulkInPipeIndex;                                                               // bulk in pipe index
    UCHAR BulkOutPipeIndex;                                                              // bulk out pipe index
    UCHAR MaxLUN;                                                                        // max lun for device
    PDEVICE_OBJECT ChildPDO[USB_MAXCHILDREN];                                            // max 16 child pdo devices
    KSPIN_LOCK IrpListLock;                                                              // irp list lock
    LIST_ENTRY IrpListHead;                                                              // irp list head
    ULONG IrpPendingCount;                                                               // count of irp pending
    PSCSI_REQUEST_BLOCK ActiveSrb;                                                       // stores the current active SRB
    KEVENT NoPendingRequests;                                                            // set if no pending or in progress requests
    PSCSI_REQUEST_BLOCK LastTimerActiveSrb;                                              // last timer tick active srb
    ULONG SrbErrorHandlingActive;                                                        // error handling of srb is activated
    ULONG TimerWorkQueueEnabled;                                                         // timer work queue enabled
    ULONG InstanceCount;                                                                 // pdo instance count
    KSPIN_LOCK CommonLock;
    PIO_WORKITEM ResetDeviceWorkItem;
    ULONG Flags;
    IRP_CONTEXT CurrentIrpContext;
}FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct
{
    USBSTOR_COMMON_DEVICE_EXTENSION Common;

    UCHAR LUN;                                                                           // lun id
    BOOLEAN Claimed;                                                                     // indicating if it has been claimed by upper driver
    PDEVICE_OBJECT LowerDeviceObject;                                                    // points to FDO
    PDEVICE_OBJECT *PDODeviceObject;                                                     // entry in pdo list
    PDEVICE_OBJECT Self;                                                                 // self
    // the whole structure is not stored
    UCHAR InquiryData[INQUIRYDATABUFFERSIZE];                                            // USB SCSI inquiry data
}PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

typedef struct _ERRORHANDLER_WORKITEM_DATA
{
    PDEVICE_OBJECT DeviceObject;
    PIRP_CONTEXT Context;
    WORK_QUEUE_ITEM WorkQueueItem;
    PIRP Irp;
} ERRORHANDLER_WORKITEM_DATA, *PERRORHANDLER_WORKITEM_DATA;

// we need this to be compatible with ReactOS' classpnp (which is compiled with NTDDI_WIN8)
typedef struct _STORAGE_ADAPTER_DESCRIPTOR_WIN8 {
    ULONG Version;
    ULONG Size;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG AlignmentMask;
    BOOLEAN AdapterUsesPio;
    BOOLEAN AdapterScansDown;
    BOOLEAN CommandQueueing;
    BOOLEAN AcceleratedTransfer;
    UCHAR BusType;
    USHORT BusMajorVersion;
    USHORT BusMinorVersion;
    UCHAR SrbType;
    UCHAR AddressType;
} STORAGE_ADAPTER_DESCRIPTOR_WIN8, *PSTORAGE_ADAPTER_DESCRIPTOR_WIN8;


//---------------------------------------------------------------------
//
// fdo.c routines
//
NTSTATUS
USBSTOR_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

//---------------------------------------------------------------------
//
// pdo.c routines
//
NTSTATUS
USBSTOR_PdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

NTSTATUS
USBSTOR_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR LUN);

//---------------------------------------------------------------------
//
// misc.c routines
//

NTSTATUS
NTAPI
USBSTOR_GetBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSB_BUS_INTERFACE_USBDI_V2 BusInterface);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN ULONG ItemSize);

VOID
FreeItem(
    IN PVOID Item);

NTSTATUS
USBSTOR_SyncUrbRequest(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PURB UrbRequest);

NTSTATUS
USBSTOR_GetMaxLUN(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
USBSTOR_ResetDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

//---------------------------------------------------------------------
//
// descriptor.c routines
//

NTSTATUS
USBSTOR_GetDescriptors(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
USBSTOR_SelectConfigurationAndInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
USBSTOR_GetPipeHandles(
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

//---------------------------------------------------------------------
//
// scsi.c routines
//
NTSTATUS
USBSTOR_HandleExecuteSCSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
USBSTOR_SendCSWRequest(
    PFDO_DEVICE_EXTENSION FDODeviceExtension,
    PIRP Irp);


//---------------------------------------------------------------------
//
// disk.c routines
//
NTSTATUS
USBSTOR_HandleInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
USBSTOR_HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

//---------------------------------------------------------------------
//
// queue.c routines
//
VOID
NTAPI
USBSTOR_StartIo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

VOID
USBSTOR_QueueWaitForPendingRequests(
    IN PDEVICE_OBJECT DeviceObject);

VOID
USBSTOR_QueueRelease(
    IN PDEVICE_OBJECT DeviceObject);

BOOLEAN
USBSTOR_QueueAddIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

VOID
NTAPI
USBSTOR_CancelIo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

VOID
USBSTOR_QueueInitialize(
    PFDO_DEVICE_EXTENSION FDODeviceExtension);

VOID
USBSTOR_QueueNextRequest(
    IN PDEVICE_OBJECT DeviceObject);

VOID
USBSTOR_QueueTerminateRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

//---------------------------------------------------------------------
//
// error.c routines
//
NTSTATUS
USBSTOR_GetEndpointStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR bEndpointAddress,
    OUT PUSHORT Value);

NTSTATUS
USBSTOR_ResetPipeWithHandle(
    IN PDEVICE_OBJECT DeviceObject,
    IN USBD_PIPE_HANDLE PipeHandle);

VOID
NTAPI
USBSTOR_TimerRoutine(
    PDEVICE_OBJECT DeviceObject,
     PVOID Context);

VOID
NTAPI
USBSTOR_QueueResetPipe(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension);

VOID
NTAPI
USBSTOR_QueueResetDevice(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension);

#endif // _USBSTOR_H_
