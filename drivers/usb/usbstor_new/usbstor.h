#ifndef _USBSTOR_H_
#define _USBSTOR_H_

#include <wdm.h>
#include <usbdi.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <classpnp.h>

#define USB_STOR_TAG 'sbsu'
#define USB_MAXCHILDREN              (16)

#define HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

#define HTONL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))


#define NTOHL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))

#define USB_RECOVERABLE_ERRORS (USBD_STATUS_STALL_PID | USBD_STATUS_DEV_NOT_RESPONDING \
	| USBD_STATUS_ENDPOINT_HALTED | USBD_STATUS_NO_BANDWIDTH)

typedef struct __COMMON_DEVICE_EXTENSION__
{
    BOOLEAN IsFDO;

}USBSTOR_COMMON_DEVICE_EXTENSION, *PUSBSTOR_COMMON_DEVICE_EXTENSION;

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
    PDEVICE_OBJECT ChildPDO[16];                                                         // max 16 child pdo devices
    KSPIN_LOCK IrpListLock;                                                              // irp list lock
    LIST_ENTRY IrpListHead;                                                              // irp list head
    BOOLEAN IrpListFreeze;                                                               // if true the irp list is freezed
    BOOLEAN ResetInProgress;                                                             // if hard reset is in progress
    ULONG IrpPendingCount;                                                               // count of irp pending
    PSCSI_REQUEST_BLOCK ActiveSrb;                                                       // stores the current active SRB
    KEVENT NoPendingRequests;                                                            // set if no pending or in progress requests
    PSCSI_REQUEST_BLOCK LastTimerActiveSrb;                                              // last timer tick active srb
    ULONG SrbErrorHandlingActive;                                                        // error handling of srb is activated
    ULONG TimerWorkQueueEnabled;                                                         // timer work queue enabled
    ULONG InstanceCount;                                                                 // pdo instance count
}FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct
{
    USBSTOR_COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT LowerDeviceObject;                                                    // points to FDO
    UCHAR LUN;                                                                           // lun id
    PVOID InquiryData;                                                                   // USB SCSI inquiry data
    PUCHAR FormatData;                                                                   // USB SCSI Read Format Capacity Data
    UCHAR Claimed;                                                                       // indicating if it has been claimed by upper driver
    ULONG BlockLength;                                                                   // length of block
    ULONG LastLogicBlockAddress;                                                         // last block address
    PDEVICE_OBJECT *PDODeviceObject;                                                     // entry in pdo list
    PDEVICE_OBJECT Self;                                                                 // self
    UCHAR MediumTypeCode;                                                                // floppy medium type code
    UCHAR IsFloppy;                                                                      // is device floppy
}PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

//
// max lun command identifier
//
#define USB_BULK_GET_MAX_LUN             0xFE
#define USB_BULK_RESET_DEVICE             0xFF

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
}CBW, *PCBW;

C_ASSERT(sizeof(CBW) == 31);


#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

#define MAX_LUN 0xF

typedef struct
{
    ULONG Signature;                                                 // CSW signature
    ULONG Tag;                                                       // CSW tag
    ULONG DataResidue;                                               // CSW data transfer diff
    UCHAR Status;                                                    // CSW status
}CSW, *PCSW;

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI INQUIRY command
//
typedef struct
{
    UCHAR Code;                                                      // operation code 0x12
    UCHAR LUN;                                                       // lun address
    UCHAR PageCode;                                                  // product data information, always 0x00
    UCHAR Reserved;                                                  // reserved 0x00
    UCHAR AllocationLength;                                          // length of inquiry data to be returned, default 36 bytes
    UCHAR Reserved1[7];                                              //reserved bytes 0x00
}UFI_INQUIRY_CMD, *PUFI_INQUIRY_CMD;

C_ASSERT(sizeof(UFI_INQUIRY_CMD) == 12);

#define UFI_INQUIRY_CMD_LEN 0x6

//
// UFI INQUIRY command response
//
typedef struct
{
    UCHAR DeviceType;                                                // device type
    UCHAR RMB;                                                       // removable media bit
    UCHAR Version;                                                   // contains version 0x00
    UCHAR Format;                                                    // response format
    UCHAR Length;                                                    // additional length
    UCHAR Reserved[3];                                               // reserved
    UCHAR Vendor[8];                                                 // vendor identification string
    UCHAR Product[16];                                               // product identification string
    UCHAR Revision[4];                                               // product revision code
}UFI_INQUIRY_RESPONSE, *PUFI_INQUIRY_RESPONSE;

C_ASSERT(sizeof(UFI_INQUIRY_RESPONSE) == 36);

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI read cmd
//
typedef struct
{
    UCHAR Code;                                                      // operation code
    UCHAR LUN;                                                       // lun
    UCHAR LogicalBlockByte0;                                         // lba byte 0
    UCHAR LogicalBlockByte1;                                         // lba byte 1
    UCHAR LogicalBlockByte2;                                         // lba byte 2
    UCHAR LogicalBlockByte3;                                         // lba byte 3
    UCHAR Reserved;                                                  // reserved 0x00
    UCHAR ContiguousLogicBlocksByte0;                                // msb contiguous logic blocks byte
    UCHAR ContiguousLogicBlocksByte1;                                // msb contiguous logic blocks
    UCHAR Reserved1[3];                                              // reserved 0x00
}UFI_READ_WRITE_CMD;

C_ASSERT(sizeof(UFI_READ_WRITE_CMD) == 12);

#define UFI_READ_WRITE_CMD_LEN (0xA)

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI read capacity cmd
//
typedef struct
{
    UCHAR Code;                                                      // operation code 0x25
    UCHAR LUN;                                                       // lun address
    UCHAR LBA[4];                                                   // logical block address, should be zero
    UCHAR Reserved1[2];                                              // reserved 0x00
    UCHAR PMI;                                                       // PMI = 0x00
    UCHAR Reserved2[3];                                              // reserved 0x00
}UFI_CAPACITY_CMD, *PUFI_CAPACITY_CMD;

C_ASSERT(sizeof(UFI_CAPACITY_CMD) == 12);

#define UFI_CAPACITY_CMD_LEN 0xA //FIXME support length 16 too if requested

//
// UFI Read Capacity command response
//
typedef struct
{
    ULONG LastLogicalBlockAddress;                                   // last logical block address
    ULONG BlockLength;                                               // block length in bytes
}UFI_CAPACITY_RESPONSE, *PUFI_CAPACITY_RESPONSE;

#define UFI_READ_CAPACITY_CMD_LEN 0xA
C_ASSERT(sizeof(UFI_CAPACITY_RESPONSE) == 8);

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI sense mode cmd
//
typedef struct
{
    UCHAR Code;                                                      // operation code
    UCHAR LUN;                                                       // lun address
    UCHAR PageCode:6;                                                // page code selector
    UCHAR PC:2;                                                      // type of parameters to be returned
    UCHAR Reserved[4];                                               // reserved 0x00
    USHORT AllocationLength;                                         // parameters length
    UCHAR Reserved1[3];
}UFI_SENSE_CMD, *PUFI_SENSE_CMD;

C_ASSERT(sizeof(UFI_SENSE_CMD) == 12);

#define UFI_SENSE_CMD_LEN (6)

typedef struct
{
    USHORT ModeDataLength;                                           // length of parameters for sense cmd
    UCHAR MediumTypeCode;                                            // 00 for mass storage, 0x94 for floppy
    UCHAR WP:1;                                                      // write protect bit
    UCHAR Reserved1:2;                                                // reserved 00
    UCHAR DPOFUA:1;                                                  // should be zero
    UCHAR Reserved2:4;                                               // reserved
    UCHAR Reserved[4];                                               // reserved
}UFI_MODE_PARAMETER_HEADER, *PUFI_MODE_PARAMETER_HEADER;


C_ASSERT(sizeof(UFI_MODE_PARAMETER_HEADER) == 8);

typedef struct
{
    UCHAR PC;
    UCHAR PageLength;
    UCHAR Reserved1;
    UCHAR ITM;
    UCHAR Flags;
    UCHAR Reserved[3];
}UFI_TIMER_PROTECT_PAGE, *PUFI_TIMER_PROTECT_PAGE;
C_ASSERT(sizeof(UFI_TIMER_PROTECT_PAGE) == 8);

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI read capacity cmd
//

typedef struct
{
    UCHAR Code;
    UCHAR LUN;
    UCHAR Reserved[5];
    UCHAR AllocationLengthMsb;
    UCHAR AllocationLengthLsb;
    UCHAR Reserved1[3];
}UFI_READ_FORMAT_CAPACITY, *PUFI_READ_FORMAT_CAPACITY;

C_ASSERT(sizeof(UFI_READ_FORMAT_CAPACITY) == 12);

#define UFI_READ_FORMAT_CAPACITY_CMD_LEN (10)

typedef struct
{
    UCHAR Reserved1;
    UCHAR Reserved2;
    UCHAR Reserved3;
    UCHAR CapacityLength;
}UFI_CAPACITY_FORMAT_HEADER, *PUFI_CAPACITY_FORMAT_HEADER;

C_ASSERT(sizeof(UFI_CAPACITY_FORMAT_HEADER) == 4);

typedef struct
{
    ULONG BlockCount;
    UCHAR Code;
    UCHAR BlockLengthByte0;
    UCHAR BlockLengthByte1;
    UCHAR BlockLengthByte2;
}UFI_CAPACITY_DESCRIPTOR, *PUFI_CAPACITY_DESCRIPTOR;

#define UNFORMATTED_MEDIA_CODE_DESCRIPTORY_TYPE (1)
#define FORMAT_MEDIA_CODE_DESCRIPTOR_TYPE      (2)
#define CARTRIDGE_MEDIA_CODE_DESCRIPTOR_TYPE   (3)




//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI test unit command
//

typedef struct
{
    UCHAR Code;                                                       // operation code 0x00
    UCHAR LUN;                                                        // lun
    UCHAR Reserved[10];                                               // reserved 0x00
}UFI_TEST_UNIT_CMD, *PUFI_TEST_UNIT_CMD;

C_ASSERT(sizeof(UFI_TEST_UNIT_CMD) == 12);

#define UFI_TEST_UNIT_CMD_LEN (6)

//-------------------------------------------------------------------------------------------------------------------------------------------
typedef struct
{
    UCHAR Bytes[16];
}UFI_UNKNOWN_CMD, *PUFI_UNKNOWN_CMD;

#include <poppack.h>

typedef struct
{
    union
    {
        PCBW cbw;
        PCSW csw;
    };
    URB Urb;
    PIRP Irp;
    ULONG TransferDataLength;
    PUCHAR TransferData;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PMDL TransferBufferMDL;
    ULONG ErrorIndex;
    ULONG RetryCount;
}IRP_CONTEXT, *PIRP_CONTEXT;

typedef struct _ERRORHANDLER_WORKITEM_DATA
{
    PDEVICE_OBJECT DeviceObject;
    PIRP_CONTEXT Context;
    WORK_QUEUE_ITEM WorkQueueItem;
    PIRP Irp;
} ERRORHANDLER_WORKITEM_DATA, *PERRORHANDLER_WORKITEM_DATA;


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
USBSTOR_SyncForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

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
NTAPI
USBSTOR_SyncForwardIrpCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context);

NTSTATUS
USBSTOR_ResetDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

BOOLEAN
USBSTOR_IsFloppy(
    IN PUCHAR Buffer,
    IN ULONG BufferLength,
    OUT PUCHAR MediumTypeCode);

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
    IN PIRP Irp,
    IN ULONG RetryCount);

NTSTATUS
NTAPI
USBSTOR_CSWCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Ctx);

NTSTATUS
USBSTOR_SendCBW(
    PIRP_CONTEXT Context,
    PIRP Irp);

VOID
USBSTOR_SendCSW(
    PIRP_CONTEXT Context,
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
NTAPI
ErrorHandlerWorkItemRoutine(
	PVOID Context);

VOID
NTAPI
ResetHandlerWorkItemRoutine(
    PVOID Context);



VOID
USBSTOR_QueueNextRequest(
    IN PDEVICE_OBJECT DeviceObject);

VOID
USBSTOR_QueueTerminateRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* error.c */
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

#endif /* _USBSTOR_H_ */
