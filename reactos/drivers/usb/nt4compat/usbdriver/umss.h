#ifndef __UMSS_H__
#define __UMSS_H__

#define MAX_BULK_TRANSFER_LENGTH		0x100000

#define PROTOCOL_CBI       				0x00
#define PROTOCOL_CB        				0x01
#define PROTOCOL_BULKONLY  				0x50

#define PROTOCOL_UNDEFINED 				0xFF  // Not in spec

#define UMSS_SUBCLASS_RBC				0x01
#define UMSS_SUBCLASS_SFF8020I			0X02
#define UMSS_SUBCLASS_QIC157			0x03
#define UMSS_SUBCLASS_UFI				0x04
#define UMSS_SUBCLASS_SFF8070I			0x05
#define UMSS_SUBCLASS_SCSI_TCS			0x06

#define ACCEPT_DEVICE_SPECIFIC_COMMAND 	0

#define BULK_ONLY_MASS_STORAGE_RESET 	0xFF
#define BULK_ONLY_GET_MAX_LUN 			0xFE

#define CBW_SIGNATURE 					0x43425355L
#define CSW_SIGNATURE 					0x53425355L
#define CSW_OLYMPUS_SIGNATURE			0x55425355L

#define CSW_STATUS_PASSED         		0x00
#define CSW_STATUS_FAILED         		0x01
#define CSW_STATUS_PHASE_ERROR    		0x02

#define IOCTL_UMSS_SUBMIT_CDB			CTL_CODE( FILE_USB_DEV_TYPE, 4200, METHOD_BUFFERED,		FILE_ANY_ACCESS )
// for request with no other input and output
// input buffer is a _USER_IO_PACKET and input_buffer_length is length of the _USER_IO_PACKET
// output_buffer is NULL, and output_buffer_length is zero

#define IOCTL_UMSS_SUBMIT_CDB_IN		CTL_CODE( FILE_USB_DEV_TYPE, 4201, METHOD_IN_DIRECT,	FILE_ANY_ACCESS )
// for request to read in data
// input_buffer is a _USER_IO_PACKET and input_buffer_length is length to the _USER_IO_PACKET
// output_buffer is a buffer to receive the data from dev, and
// output_buffer_length is the size of the buffer

#define IOCTL_UMSS_SUBMIT_CDB_OUT		CTL_CODE( FILE_USB_DEV_TYPE, 4202, METHOD_OUT_DIRECT,	FILE_ANY_ACCESS )
// for request to write data to device
// input_buffer is a _USER_IO_PACKET and input_buffer_length is length to the _USER_IO_PACKET
// output_buffer is data to send to the device, and
// output_buffer_length is the size of the buffer

#define IOCTL_REGISTER_DRIVER 			CTL_CODE( FILE_USB_DEV_TYPE, 4203, METHOD_BUFFERED, FILE_ANY_ACCESS )
// input_buffer is a CLASS_DRV_REG_INFO, and input_buffer_length is equal to or greater than 
// sizeof( CLASS_DRV_REG_INFO ); the output_buffer is null and no output_buffer_length,
// only the following fields in urb can be accessed, others must be zeroed.

#define IOCTL_REVOKE_DRIVER 			CTL_CODE( FILE_USB_DEV_TYPE, 4204, METHOD_BUFFERED, FILE_ANY_ACCESS )
// tell the umss driver to clear the information in the drivers registry
// no other parameters

#define IOCTL_UMSS_SUBMIT_SRB 			CTL_CODE( FILE_USB_DEV_TYPE, 4205, METHOD_BUFFERED, FILE_ANY_ACCESS )
// irpStack->Parameters.Scsi.Srb points to an Srb structure and all the data buffer and buffer
// size are stored in the srb


#define IOCTL_UMSS_SET_FDO			CTL_CODE( FILE_USB_DEV_TYPE, 4206, METHOD_BUFFERED,	FILE_ANY_ACCESS )
// input_buffer is a pointer to PDEVICE_OBJECT, and input_buffer_length should be
// no less than sizeof( PDEVICE_OBJECT )
// output buffer is NULL, and output_buffer_length is zero
// if the deivce is accessable, the fdo is set, else, the fdo is not set and return
// STATUS_DEVICE_DOES_NOT_EXIST

#define SFF_FORMAT_UNIT			0x04
#define SFF_INQUIRY				0x12
#define SFF_MODE_SELECT			0x55
#define SFF_MODE_SENSE			0x5a
#define SFF_ALLOW_REMOVE		0x1e
#define SFF_READ10				0x28
#define SFF_READ12				0xa8
#define SFF_READ_CAPACITY		0x25
#define	SFF_REQUEST_SENSEE		0x03
#define SFF_SEEK				0x2b
#define SFF_START_STOP			0x1b
#define SFF_TUR					0x00
#define SFF_VERIFY				0x2f
#define SFF_WRITE10				0x2a
#define SFF_WRITE12				0xaa
#define SFF_READ_FMT_CAPACITY	0x23
#define SFF_WRITE_VERIFY		0x2e

#define MAX_CDB_LENGTH			0x10

typedef struct _USER_IO_PACKET
{
	UCHAR					sub_class;
	UCHAR					lun;
	UCHAR   				cdb_length;
	UCHAR   				cdb[ MAX_CDB_LENGTH ];

} USER_IO_PACKET, *PUSER_IO_PACKET;

//flags for IO_PACKET::flags
#define IOP_FLAG_REQ_SENSE			0x80000000 		// sense data would be fetched if error occurs
#define IOP_FLAG_SRB_TRANSFER		0x40000000		// current tranfer is initiated by an srb request, the srb is held by the irp
#define IOP_FLAG_SCSI_CTRL_TRANSFER 0x20000000		// current transfer is initiated by an scsi ioctrl request

#define IOP_FLAG_DIR_IN			USB_DIR_IN
#define IOP_FLAG_STAGE_MASK		0x03
#define IOP_FLAG_STAGE_NORMAL	0x00
#define IOP_FLAG_STAGE_SENSE	0x01

typedef struct _IO_PACKET
{
        ULONG   			flags;
        UCHAR   			cdb_length;
        UCHAR   			cdb[ MAX_CDB_LENGTH ];
		UCHAR				lun;
        PVOID   			data_buffer;
        ULONG   			data_length;
		PVOID				sense_data;
		ULONG				sense_data_length;
		PIRP				pirp;

} IO_PACKET, *PIO_PACKET;

#pragma pack( 1 )

typedef struct _COMMAND_BLOCK_WRAPPER
{
        ULONG 				dCBWSignature;
        ULONG 				dCBWTag;
        ULONG 				dCBWDataTransferLength;
        UCHAR 				bmCBWFlags;
        UCHAR 				bCBWLun;
        UCHAR 				bCBWLength;
        UCHAR 				CBWCB[ MAX_CDB_LENGTH ];

} COMMAND_BLOCK_WRAPPER, *PCOMMAND_BLOCK_WRAPPER;

typedef struct _COMMAND_STATUS_WRAPPER
{
        ULONG 				dCSWSignature;
        ULONG 				dCSWTag;
        ULONG 				dCSWDataResidue;
        UCHAR 				bCSWStatus;

} COMMAND_STATUS_WRAPPER, *PCOMMAND_STATUS_WRAPPER;


typedef struct _INTERRUPT_DATA_BLOCK
{
        UCHAR 				bType;
        UCHAR 				bValue;

} INTERRUPT_DATA_BLOCK, *PINTERRUPT_DATA_BLOCK;

#pragma pack()

#define UMSS_PNPMSG_STOP			0x01
#define UMSS_PNPMSG_DISCONNECT 		0x02

typedef NTSTATUS ( *PCLASS_DRVR_PNP_DISP )( PDEVICE_OBJECT pdo, ULONG ctrl_code, PVOID context );
	// pdo is the device object umss created

typedef PDEVICE_OBJECT ( *PCLASS_DRIVER_ADD_DEV )( PDRIVER_OBJECT fdo_drvr, PDEVICE_OBJECT pdo );
	// if the return value is not zero, it is a pointer to the
	// fdo sitting over the pdo of this driver. if it is null,
	// the add_device failed, and initialization process should
	// stall.

typedef struct _CLASS_DRV_REGISTRY_INFO
{
	// class driver will pass this structure to umss port
	// driver after loaded
	PDRIVER_OBJECT			fdo_driver;
	PCLASS_DRIVER_ADD_DEV   add_device;
	PCLASS_DRVR_PNP_DISP	pnp_dispatch;

} CLASS_DRV_REG_INFO, *PCLASS_DRV_REG_INFO;

typedef struct _UMSS_PORT_DEVICE_EXTENSION
{
	// this structure is the device extension for port dev_obj
	// it is used to has class driver pass CLASS_DRV_REG_INFO
	// to our umss driver.
	DEVEXT_HEADER			dev_ext_hdr;
	PUSB_DRIVER				pdriver;

} UMSS_PORT_DEV_EXT, *PUMSS_PORT_DEV_EXT;

typedef struct _UMSS_DRVR_EXTENSION
{
	LIST_HEAD				dev_list;
	FAST_MUTEX				dev_list_mutex;
	UCHAR					dev_count;
	CLASS_DRV_REG_INFO		class_driver_info;
	PDEVICE_OBJECT			port_dev_obj;	// we use this obj as a connection point for class driver, its name usbPort0

} UMSS_DRVR_EXTENSION, *PUMSS_DRVR_EXTENSION;

#define UMSS_DEV_FLAG_IF_DEV	0x01
#define UMSS_DEV_FLAG_OLYMPUS_DEV	0x02

#define UMSS_OLYMPUS_VENDOR_ID	0x07b4

typedef struct _UMSS_DEVICE_EXTENSION
{
	//this structure is the device extension for dev_obj
	//created for the device.
	DEVEXT_HEADER			dev_ext_hdr;
	
	ULONG					flags;
	LIST_ENTRY				dev_obj_link;	// this link is used by the driver object to track the existing dev_objs

	PDEVICE_OBJECT  		pdo;			// this is the pdo
	PDEVICE_OBJECT			fdo;			// driver object for the dev_obj

	DEV_HANDLE				dev_handle;		// handle to the usb_dev under

	PUCHAR					desc_buf;
	UCHAR					umss_dev_id;	// used to build symbolic link

	PUSB_INTERFACE_DESC 	pif_desc;
	PUSB_ENDPOINT_DESC  	pout_endp_desc, pin_endp_desc, pint_endp_desc;
	UCHAR					if_idx, out_endp_idx, in_endp_idx, int_endp_idx;

	struct _USB_DEV_MANAGER	*dev_mgr;

	//working data
	COMMAND_BLOCK_WRAPPER	cbw;
	union
	{
		INTERRUPT_DATA_BLOCK	idb;
		COMMAND_STATUS_WRAPPER	csw;
	};

	KEVENT					sync_event; //for umss_sync_submit_urb
	KSPIN_LOCK				dev_lock;
	IO_PACKET				io_packet;
	BOOLEAN					retry;

	PUSB_DRIVER				pdriver;	//used by umss_delete_device
	NTSTATUS				reset_pipe_status;
} UMSS_DEVICE_EXTENSION, *PUMSS_DEVICE_EXTENSION;

// for device creation workitem
typedef struct _UMSS_CREATE_DATA
{
	DEV_HANDLE 				dev_handle;
	PUCHAR					desc_buf;
	PUSB_DEV_MANAGER 		dev_mgr;
	PUSB_DRIVER				pdriver;

} UMSS_CREATE_DATA, *PUMSS_CREATE_DATA;

// for reset pipe item
//typedef void ( _stdcall *COMPLETION_HANDLER )( PVOID );
typedef void ( *UMSS_WORKER_ROUTINE )( PVOID );

typedef struct _UMSS_WORKER_PACKET
{
    UMSS_WORKER_ROUTINE 	completion;
    PVOID 					context;
	PUSB_DEV_MANAGER		dev_mgr;
	PVOID					pdev;

} UMSS_WORKER_PACKET, *PUMSS_WORKER_PACKET;
    
BOOLEAN
umss_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
umss_if_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
umss_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

#define umss_if_driver_destroy umss_driver_destroy

BOOLEAN
umss_if_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

VOID
umss_complete_request(
PUMSS_DEVICE_EXTENSION pdev_ext,
NTSTATUS status
);

NTSTATUS
umss_reset_pipe(
PUMSS_DEVICE_EXTENSION pdev_ext,
DEV_HANDLE endp_handle
);

PVOID
umss_get_buffer(
PUMSS_DEVICE_EXTENSION  pdev_ext,
ULONG* buf_length
);

NTSTATUS
umss_bulk_transfer(
IN PUMSS_DEVICE_EXTENSION pdev_ext,
IN UCHAR trans_dir,
IN PVOID buf,
IN ULONG buf_length,
IN PURBCOMPLETION completion 
);

BOOLEAN
umss_schedule_workitem(
PVOID context,
UMSS_WORKER_ROUTINE completion,
PUSB_DEV_MANAGER dev_mgr,
DEV_HANDLE dev_handle
);

NTSTATUS
umss_bulkonly_startio(
IN PUMSS_DEVICE_EXTENSION pdev_ext,
IN PIO_PACKET io_packet
);

NTSTATUS
umss_cbi_startio(
IN PUMSS_DEVICE_EXTENSION pdev_ext,
IN PIO_PACKET io_packet
);

#define UMSS_FORGE_GOOD_SENSE( sense_BUF ) \
{\
    int i;\
	PUCHAR buf = ( PUCHAR )( sense_BUF ); \
    for( i = 0; i < 18; i++)\
    {\
        buf[i] = 0;\
    }\
    buf[7] = 10;\
}

#endif
