#ifndef __USBD_H__
#define __USBD_H__
/*
 * Some USB bandwidth allocation constants.
 */

#define USB2_HOST_DELAY	5			/* nsec, guess */
#define BW_HOST_DELAY	1000L		/* nanoseconds */
#define BW_HUB_LS_SETUP	333L		/* nanoseconds */
                        /* 4 full-speed bit times (est.) */

#define FRAME_TIME_BITS         12000L		/* frame = 1 millisecond */
#define FRAME_TIME_MAX_BITS_ALLOC	(90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_USECS	1000L
#define FRAME_TIME_MAX_USECS_ALLOC	(90L * FRAME_TIME_USECS / 100L)

#define bit_time(bytecount)  (7 * 8 * bytecount / 6)  /* with integer truncation */
		/* Trying not to use worst-case bit-stuffing
                   of (7/6 * 8 * bytecount) = 9.33 * bytecount */
		/* bytecount = data payload byte count */

#define ns_to_us(ns)	((ns + 500L) / 1000L)
			/* convert & round nanoseconds to microseconds */

#define usb_make_handle( dev_Id, if_iDx, endp_iDx) \
( ( DEV_HANDLE )( ( ( ( ( ULONG )dev_Id ) << 16 ) | ( ( ( ULONG )if_iDx ) << 8 ) ) | ( ( ULONG ) endp_iDx ) ) )

#define usb_make_ref( poinTER ) \
( poinTER ^ 0xffffffff )

#define ptr_from_ref uhci_make_ref

#define dev_id_from_handle( hanDLE ) ( ( ( ULONG ) ( hanDLE ) ) >> 16 )
#define if_idx_from_handle( hanDLE ) ( ( ( ( ULONG ) ( hanDLE ) ) << 16 ) >> 24 )
#define endp_idx_from_handle( hanDLE ) ( ( ( ULONG ) ( hanDLE ) ) & 0xff )

#define endp_from_handle( pDEV, hanDLE, peNDP ) \
{\
    LONG if_idx, endp_idx;\
	BOOLEAN def_endp; \
    endp_idx = endp_idx_from_handle( hanDLE );\
    if_idx = if_idx_from_handle( hanDLE );\
    def_endp = ( ( hanDLE & 0xffff ) == 0xffff ); \
	if( def_endp ) \
		peNDP = &pdev->default_endp; \
	else \
	{ \
		if( if_idx >= pdev->usb_config->if_count ) \
			peNDP = NULL; \
		else if( endp_idx >= pdev->usb_config->interf[ if_idx ].endp_count ) \
			peNDP = NULL; \
		else \
			peNDP = &( pDEV )->usb_config->interf[ if_idx ].endp[ endp_idx ]; \
	} \
}

#define endp_type( enDP ) \
( ( enDP->flags & USB_ENDP_FLAG_DEFAULT_ENDP ) \
  ? USB_ENDPOINT_XFER_CONTROL\
  : ( ( enDP )->pusb_endp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK ) )


//init work data for urb
#define urb_init( uRb ) \
{\
	RtlZeroMemory( ( uRb ), sizeof( URB ) ); \
	InitializeListHead( &( uRb )->trasac_list ); \
}

#define UsbBuildInterruptOrBulkTransferRequest(uRb, \
                                               endp_hanDle, \
                                               data_Buf, \
                                               data_sIze, \
											   completIon, \
											   contExt, \
											   refereNce ) \
{ \
	urb_init( ( uRb ) );\
	( uRb )->endp_handle = endp_hanDle;\
	( uRb )->data_buffer = data_Buf;\
	( uRb )->data_length = data_sIze;\
	( uRb )->completion = completIon;\
	( uRb )->context = contExt; \
	( uRb )->reference = refereNce; \
}



#define UsbBuildGetDescriptorRequest(uRb, \
									 endp_hAndle, \
                                     descriPtorType, \
                                     descriPtorIndex, \
                                     languaGeId, \
                                     data_bUffer, \
                                     data_sIze, \
									 compleTion, \
									 contexT, \
									 refereNce ) \
{ \
	PUSB_CTRL_SETUP_PACKET pseTup;\
	pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle = ( endp_hAndle );\
	( uRb )->data_length = ( data_sIze ); \
	( uRb )->data_buffer = ( ( PUCHAR )data_bUffer ); \
	( uRb )->completion = ( compleTion );\
	( uRb )->context = ( ( PVOID )contexT ); \
	( uRb )->reference = ( ULONG )refereNce; \
	pseTup->wValue = ( ( descriPtorType ) << 8 )| ( descriPtorIndex ); \
	pseTup->wLength = ( data_sIze ); \
	pseTup->wIndex = ( languaGeId );\
    pseTup->bRequest = USB_REQ_GET_DESCRIPTOR;\
    pseTup->bmRequestType = 0x80;\
}



#define UsbBuildGetStatusRequest(uRb, \
								 endp_hanDle, \
                                 recipiEnt, \
                                 inDex, \
                                 transferBufFer, \
							     completIon, \
							     contExt, \
							     refereNce ) \
{ \
	PUSB_CTRL_SETUP_PACKET pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle =  ( endp_hanDle ); \
	( uRb )->data_buffer = ( transferBufFer ); \
	( uRb )->data_length = sizeof(USHORT); \
    ( uRb )->completion = ( completIon );\
    ( uRb )->context = ( contExt );\
    ( uRb )->reference = ( refereNce );\
	pseTup->bmRequestType = ( 0x80 | recipiEnt );\
	pseTup->bRequest = USB_REQ_GET_STATUS;\
	pseTup->wIndex = ( inDex ); \
    pseTup->wValue = 0;\
	pseTup->wLength = sizeof( USHORT );\
}


#define UsbBuildFeatureRequest(uRb, \
							   endp_hanDle,\
                               recipiEnt, \
                               featureSelecTor, \
                               inDex, \
							   completIon, \
							   contExt, \
							   refereNce ) \
 { \
	PUSB_CTRL_SETUP_PACKET pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle =  ( endp_hanDle ); \
	( uRb )->data_buffer = NULL;\
	( uRb )->data_length = ( 0 );\
	( uRb )->completion = ( completIon );\
	( uRb )->context = ( contExt ); \
	( uRb )->reference = ( refereNce ); \
	pseTup->bmRequestType = recipiEnt; \
	pseTup->bRequest = USB_REQ_SET_FEATURE;\
	pseTup->wValue = ( featureSelecTor );\
	pseTup->wIndex = ( inDex );\
	pseTup->wLength = 0;\
}

#define UsbBuildSelectConfigurationRequest(uRb, \
										   endp_hanDle,\
										   config_Val,\
										   completIon, \
										   contExt, \
										   refereNce ) \
 { \
	PUSB_CTRL_SETUP_PACKET pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle =  ( endp_hanDle ); \
	( uRb )->data_buffer = NULL;\
	( uRb )->data_length = 0;\
	( uRb )->completion = ( completIon );\
	( uRb )->context = ( contExt ); \
	( uRb )->reference = ( refereNce ); \
	pseTup->bmRequestType = 0;\
	pseTup->bRequest = USB_REQ_SET_CONFIGURATION;\
	pseTup->wValue = ( config_Val );\
	pseTup->wIndex = 0;\
	pseTup->wLength = 0;\
}

#define UsbBuildSelectInterfaceRequest(uRb, \
    								   endp_hanDle,\
                                       if_Num, \
                                       alt_Num,\
									   completIon, \
									   contExt, \
									   refereNce ) \
 { \
	PUSB_CTRL_SETUP_PACKET pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle =  ( endp_hanDle ); \
	( uRb )->data_buffer = NULL;\
	( uRb )->data_length = 0;\
	( uRb )->completion = ( completIon );\
	( uRb )->context = ( contExt ); \
	( uRb )->reference = ( refereNce ); \
	pseTup->bmRequestType = 1;\
	pseTup->bRequest = USB_REQ_SET_INERFACE;\
	pseTup->wValue = ( alt_Num );\
	pseTup->wIndex = ( if_Num );\
	pseTup->wLength = 0;\
}


#define UsbBuildVendorRequest(uRb, \
							  endp_hanDle,\
                              data_bufFer, \
                              data_sIze, \
                              request_tYpe, \
                              requEst, \
                              vaLue, \
                              inDex, \
							  completIon, \
							  contExt, \
							  refereNce ) \
 { \
	PUSB_CTRL_SETUP_PACKET pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle =  ( endp_hanDle ); \
	( uRb )->data_buffer = data_bufFer;\
	( uRb )->data_length = data_sIze;\
	( uRb )->completion = ( completIon );\
	( uRb )->context = ( contExt ); \
	( uRb )->reference = ( refereNce ); \
	pseTup->bmRequestType = request_tYpe;\
	pseTup->bRequest = requEst;\
	pseTup->wValue = vaLue;\
	pseTup->wIndex = inDex;\
	pseTup->wLength = ( USHORT )data_sIze;\
}

#define UsbBuildResetPipeRequest(uRb, \
								 dev_hanDle, \
								 endp_aDdr, \
								 completIon, \
								 contExt, \
								 refereNce ) \
{\
	PUSB_CTRL_SETUP_PACKET pseTup = ( PUSB_CTRL_SETUP_PACKET )( uRb )->setup_packet;\
	urb_init( ( uRb ) );\
	( uRb )->endp_handle =  ( dev_hanDle | 0xffff ); \
	( uRb )->completion = ( completIon );\
	( uRb )->context = ( contExt ); \
	( uRb )->reference = ( refereNce ); \
	pseTup->bmRequestType = 0x02;\
	pseTup->bRequest = USB_REQ_CLEAR_FEATURE;\
	pseTup->wIndex = endp_aDdr;\
}

// Forward structs declarations
struct _URB;
struct _HCD;
struct _USB_DEV_MANAGER;
struct _USB_DEV;
struct _USB_ENDPOINT;
struct _USB_EVENT;
struct _USB_EVENT_POOL;
struct _USB_DRIVER;

/* USB constants */

#define USB_SPEED_FULL			0x00
#define USB_SPEED_LOW			0x01
#define USB_SPEED_HIGH 			0x02

/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE	0	/* for DeviceClass */
#define USB_CLASS_AUDIO			1
#define USB_CLASS_COMM			2
#define USB_CLASS_HID			3
#define USB_CLASS_PHYSICAL		5
#define USB_CLASS_PRINTER		7
#define USB_CLASS_MASS_STORAGE	8
#define USB_CLASS_HUB			9
#define USB_CLASS_DATA			10
#define USB_CLASS_APP_SPEC		0xfe
#define USB_CLASS_VENDOR_SPEC	0xff

/*
 * USB types
 */
#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

/*
 * USB recipients
 */
#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

/*
 * USB directions
 */
#define USB_DIR_OUT			0
#define USB_DIR_IN			0x80

/*
 * Descriptor types
 */
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05

#define USB_DT_HID			(USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT		(USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL		(USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB			(USB_TYPE_CLASS | 0x09)

/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE		18
#define USB_DT_CONFIG_SIZE		9
#define USB_DT_INTERFACE_SIZE		9
#define USB_DT_ENDPOINT_SIZE		7
#define USB_DT_ENDPOINT_AUDIO_SIZE	9	/* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE		7
#define USB_DT_HID_SIZE			9

/*
 * Endpoints
 */
#define USB_ENDPOINT_NUMBER_MASK	0x0f	/* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		0x80

#define USB_ENDPOINT_XFERTYPE_MASK	0x03	/* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL	0
#define USB_ENDPOINT_XFER_ISOC		1
#define USB_ENDPOINT_XFER_BULK		2
#define USB_ENDPOINT_XFER_INT		3

/*
 * USB Packet IDs (PIDs)
 */
#define USB_PID_UNDEF_0                        0xf0
#define USB_PID_OUT                            0xe1
#define USB_PID_ACK                            0xd2
#define USB_PID_DATA0                          0xc3
#define USB_PID_PING                           0xb4	/* USB 2.0 */
#define USB_PID_SOF                            0xa5
#define USB_PID_NYET                           0x96	/* USB 2.0 */
#define USB_PID_DATA2                          0x87	/* USB 2.0 */
#define USB_PID_SPLIT                          0x78	/* USB 2.0 */
#define USB_PID_IN                             0x69
#define USB_PID_NAK                            0x5a
#define USB_PID_DATA1                          0x4b
#define USB_PID_PREAMBLE                       0x3c	/* Token mode */
#define USB_PID_ERR                            0x3c	/* USB 2.0: handshake mode */
#define USB_PID_SETUP                          0x2d
#define USB_PID_STALL                          0x1e
#define USB_PID_MDATA                          0x0f	/* USB 2.0 */

/*
 * Standard requests
 */
#define USB_REQ_GET_STATUS			0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE			0x03
#define USB_REQ_SET_ADDRESS			0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME			0x0C

/*
 * HID requests
 */
#define USB_REQ_GET_REPORT			0x01
#define USB_REQ_GET_IDLE			0x02
#define USB_REQ_GET_PROTOCOL		0x03
#define USB_REQ_SET_REPORT			0x09
#define USB_REQ_SET_IDLE			0x0A
#define USB_REQ_SET_PROTOCOL		0x0B

// HUB request
#define	HUB_REQ_GET_STATE			0x02

// usb2.0 hub
#define HUB_REQ_CLEAR_TT_BUFFER		0x08


typedef LONG USBD_STATUS;

//
// USBD status codes
//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+---------------------------+-------------------------------+
//  | S |               Status Code                                 |
//  +---+---------------------------+-------------------------------+
//
//  where
//
//      S - is the state code
//
//          00 - completed with success
//          01 - request is pending
//          10 - completed with error, endpoint not stalled
//          11 - completed with error, endpoint stalled
//
//
//      Code - is the status code
//

//
// Generic test for success on any status value (non-negative numbers
// indicate success).
//

#define usb_success(Status) ((USBD_STATUS)(Status) >= 0)

//
// Generic test for pending status value.
//

#define usb_pending(Status) ((ULONG)(Status) >> 30 == 1)

//
// Generic test for error on any status value.
//

#define usb_error(Status) ((USBD_STATUS)(Status) < 0)

//
// Generic test for stall on any status value.
//

#define usb_halted(Status) ((ULONG)(Status) >> 30 == 3)

//
// Macro to check the status code only
//

#define usb_status(Status) ((ULONG)(Status) & 0x0FFFFFFFL)


#define USB_STATUS_SUCCESS                  ((USBD_STATUS)0x00000000L)
#define USB_STATUS_PENDING                  ((USBD_STATUS)0x40000000L)
#define USB_STATUS_HALTED                   ((USBD_STATUS)0xC0000000L)
#define USB_STATUS_ERROR                    ((USBD_STATUS)0x80000000L)

//
// HC status codes
// Note: these status codes have both the error and the stall bit set.
//
#define USB_STATUS_CRC                      ((USBD_STATUS)0xC0000401L)
#define USB_STATUS_BTSTUFF                  ((USBD_STATUS)0xC0000402L)
#define USB_STATUS_DATA_TOGGLE_MISMATCH     ((USBD_STATUS)0xC0000403L)
#define USB_STATUS_STALL_PID                ((USBD_STATUS)0xC0000404L)
#define USB_STATUS_DEV_NOT_RESPONDING       ((USBD_STATUS)0xC0000405L)
#define USB_STATUS_PID_CHECK_FAILURE        ((USBD_STATUS)0xC0000406L)
#define USB_STATUS_UNEXPECTED_PID           ((USBD_STATUS)0xC0000407L)
#define USB_STATUS_DATA_OVERRUN             ((USBD_STATUS)0xC0000408L)
#define USB_STATUS_DATA_UNDERRUN            ((USBD_STATUS)0xC0000409L)
#define USB_STATUS_RESERVED1                ((USBD_STATUS)0xC000040AL)
#define USB_STATUS_RESERVED2                ((USBD_STATUS)0xC000040BL)
#define USB_STATUS_BUFFER_OVERRUN           ((USBD_STATUS)0xC000040CL)
#define USB_STATUS_BUFFER_UNDERRUN          ((USBD_STATUS)0xC000040DL)
#define USB_STATUS_NOT_ACCESSED             ((USBD_STATUS)0xC000040FL)
#define USB_STATUS_FIFO                     ((USBD_STATUS)0xC0000410L)
#define USB_STATUS_BABBLE_DETECTED			((USBD_STATUS)0xC0000408L)

//
// returned by HCD if a transfer is submitted to an endpoint that is
// stalled
//
#define USB_STATUS_ENDPOINT_HALTED         ((USBD_STATUS)0xC0000430L)

//
// Software status codes
// Note: the following status codes have only the error bit set
//
#define USB_STATUS_NO_MEMORY                ((USBD_STATUS)0x80000100L)
#define USB_STATUS_INVALID_URB_FUNCTION     ((USBD_STATUS)0x80000200L)
#define USB_STATUS_INVALID_PARAMETER        ((USBD_STATUS)0x80000300L)

//
// returned if client driver attempts to close an endpoint/interface
// or configuration with outstanding transfers.
//
#define USB_STATUS_ERROR_BUSY               ((USBD_STATUS)0x80000400L)
//
// returned by USBD if it cannot complete a URB request, typically this
// will be returned in the URB status field when the Irp is completed
// with a more specific NT error code in the irp.status field.
//
#define USB_STATUS_REQUEST_FAILED           ((USBD_STATUS)0x80000500L)

#define USB_STATUS_INVALID_PIPE_HANDLE      ((USBD_STATUS)0x80000600L)

// returned when there is not enough bandwidth avialable
// to open a requested endpoint
#define USB_STATUS_NO_BANDWIDTH             ((USBD_STATUS)0x80000700L)
//
// generic HC error
//
#define USB_STATUS_INTERNAL_HC_ERROR        ((USBD_STATUS)0x80000800L)
//
// returned when a short packet terminates the transfer
// ie USBD_SHORT_TRANSFER_OK bit not set
//
#define USB_STATUS_ERROR_SHORT_TRANSFER     ((USBD_STATUS)0x80000900L)
//
// returned if the requested start frame is not within
// USBD_ISO_START_FRAME_RANGE of the current USB frame,
// note that the stall bit is set
//
#define USB_STATUS_BAD_START_FRAME          ((USBD_STATUS)0xC0000A00L)
//
// returned by HCD if all packets in an iso transfer complete with an error
//
#define USB_STATUS_ISOCH_REQUEST_FAILED     ((USBD_STATUS)0xC0000B00L)
//
// returned by USBD if the frame length control for a given
// HC is already taken by anothe driver
//
#define USB_STATUS_FRAME_CONTROL_OWNED      ((USBD_STATUS)0xC0000C00L)
//
// returned by USBD if the caller does not own frame length control and
// attempts to release or modify the HC frame length
//
#define USB_STATUS_FRAME_CONTROL_NOT_OWNED  ((USBD_STATUS)0xC0000D00L)

//
// set when a transfers is completed due to an AbortPipe request from
// the client driver
//
// Note: no error or stall bit is set for these status codes
//
#define USB_STATUS_CANCELED                 ((USBD_STATUS)0x00010000L)

#define USB_STATUS_CANCELING                ((USBD_STATUS)0x00020000L)

// Device type           -- in the "User Defined" range."
#define FILE_HCD_DEV_TYPE	45000
#define FILE_UHCI_DEV_TYPE 	( FILE_HCD_DEV_TYPE + 1 )
#define FILE_OHCI_DEV_TYPE 	( FILE_HCD_DEV_TYPE + 2 )
#define FILE_EHCI_DEV_TYPE	( FILE_HCD_DEV_TYPE + 3 )
#define FILE_USB_DEV_TYPE	( FILE_HCD_DEV_TYPE + 8 )

#define IOCTL_GET_DEV_COUNT		CTL_CODE( FILE_HCD_DEV_TYPE, 4093, METHOD_BUFFERED, FILE_ANY_ACCESS )
//input_buffer and input_buffer_length is zero, output_buffer is to receive a dword value of the
//dev count, output_buffer_length must be no less than sizeof( unsigned long ).

#define IOCTL_ENUM_DEVICES 		CTL_CODE( FILE_HCD_DEV_TYPE, 4094, METHOD_BUFFERED, FILE_ANY_ACCESS )
//input_buffer is a dword value to indicate the count of elements in the array
//input_buffer_length is sizeof( unsigned long ), output_buffer is to receive a
//structure ENUM_DEV_ARRAY where dev_count is the elements hold in this array.

#define IOCTL_GET_DEV_DESC		CTL_CODE( FILE_HCD_DEV_TYPE, 4095, METHOD_BUFFERED, FILE_ANY_ACCESS )
//input_buffer is a structure GET_DEV_DESC_REQ, and the input_buffer_length is
//no less than sizeof( input_buffer ), output_buffer is a buffer to receive the
//requested dev's desc, and output_buffer_length specifies the length of the
//buffer

#define IOCTL_SUBMIT_URB_RD		CTL_CODE( FILE_HCD_DEV_TYPE, 4096, METHOD_IN_DIRECT, FILE_ANY_ACCESS )
#define IOCTL_SUBMIT_URB_WR 	CTL_CODE( FILE_HCD_DEV_TYPE, 4097, METHOD_OUT_DIRECT, FILE_ANY_ACCESS )
// if the major_function is IRP_MJ_DEVICE_CONTROL
// input_buffer is a URB, and input_buffer_length is equal to or greater than
// sizeof( URB ); the output_buffer is a buffer to receive data from or send data
// to device. only the following urb fields can be accessed, others must be zeroed.
//  DEV_HANDLE 			endp_handle;
//	UCHAR             	setup_packet[8];   	// for control pipe
// the choosing of IOCTL_SUBMIT_URB_RD or IOCTL_SUBMIT_URB_WR should be determined
// by the current URB, for example, a request string from device will use XXX_RD,
// and a write to the bulk endpoint will use XXX_WR
// if the major_function is IRP_MJ_INTERNAL_DEVICE_CONTROL
// input_buffer is a URB, and input_buffer_length is equal to or greater than
// sizeof( URB );
// only the following urb fields can be accessed, others must be zeroed.
//  DEV_HANDLE			endp_handle;
//	UCHAR             	setup_packet[8];   	// for control pipe, or zeroed
//	PUCHAR				data_buffer;		// buffer for READ/WRITE
//	ULONG				data_length;		// buffer size in bytes

#define IOCTL_SUBMIT_URB_NOIO	CTL_CODE( FILE_HCD_DEV_TYPE, 4098, METHOD_BUFFERED,	FILE_ANY_ACCESS )
// input_buffer is a URB, and input_buffer_length is equal to or greater than
// sizeof( URB ); the output_buffer is null and no output_buffer_length,
// only the following fields in urb can be accessed, others must be zeroed.
//  DEV_HANDLE 			endp_handle;
//	UCHAR             	setup_packet[8];   	//for control pipe
//	there is no difference between IRP_MJ_DEVICE_CONTROL and IRP_MJ_INTERNAL_DEVICE_CONTROL
#define IOCTL_GET_DEV_HANDLE 		CTL_CODE( FILE_HCD_DEV_TYPE, 4099, METHOD_BUFFERED, FILE_ANY_ACCESS )
// input_buffer is null ,and input_buffer_length is zero.
// output_buffer will hold the handle to this dev, output_buffer_length is 4
// or bigger

typedef ULONG 	DEV_HANDLE, ENDP_HANDLE, IF_HANDLE;

struct URB;
#pragma pack( push, usb_align, 1 )

//structures for DeviceIoControl
typedef struct _ENUM_DEV_ELEMENT
{
	DEV_HANDLE 	dev_handle;
	USHORT		product_id;
	USHORT		vendor_id;
	UCHAR		dev_addr;

} ENUM_DEV_ELEMENT, *PENUM_DEV_ELEMENT;

typedef struct _ENUM_DEV_ARRAY
{
	UCHAR				dev_count;
	ENUM_DEV_ELEMENT 	dev_arr[ 1 ];

} ENUM_DEV_ARRAY, *PENUM_DEV_ARRAY;

typedef struct _GET_DEV_DESC_REQ
{
	DEV_HANDLE dev_handle;
	UCHAR	desc_type;
	UCHAR	desc_idx;

} GET_DEV_DESC_REQ, *PGET_DEV_DESC_REQ;

//usb definitions
typedef struct _USB_CTRL_SETUP_PACKET
{
	UCHAR 				bmRequestType;
	UCHAR 				bRequest;
	USHORT 				wValue;
	USHORT			 	wIndex;
	USHORT 				wLength;

}USB_CTRL_SETUP_PACKET, *PUSB_CTRL_SETUP_PACKET;

typedef struct _USB_STRING_DESCRIPTOR
{
    UCHAR               bLength;
    UCHAR               bDescriptorType;
    USHORT              wData[1];

} USB_STRING_DESCRIPTOR, *PUSB_STRING_DESCRIPTOR;

typedef struct _USB_DESC_HEADER
{
    UCHAR               bLength;
    UCHAR               bDescriptorType;

} USB_DESC_HEADER, *PUSB_DESC_HEADER;

typedef struct _USB_ENDPOINT_DESC
{
    UCHAR                bLength;
    UCHAR                bDescriptorType;
    UCHAR                bEndpointAddress;
    UCHAR                bmAttributes;
    USHORT               wMaxPacketSize;
    UCHAR                bInterval;

} USB_ENDPOINT_DESC, *PUSB_ENDPOINT_DESC;

typedef struct _USB_INTERFACE_DESC
{
    UCHAR               bLength;
    UCHAR               bDescriptorType;
    UCHAR               bInterfaceNumber;
    UCHAR               bAlternateSetting;
    UCHAR               bNumEndpoints;
    UCHAR               bInterfaceClass;
    UCHAR               bInterfaceSubClass;
    UCHAR               bInterfaceProtocol;
    UCHAR               iInterface;

} USB_INTERFACE_DESC, *PUSB_INTERFACE_DESC;

typedef struct _USB_CONFIGURATION_DESC
{
    UCHAR               bLength;
    UCHAR               bDescriptorType;
    USHORT              wTotalLength;
    UCHAR               bNumInterfaces;
    UCHAR               bConfigurationValue;
    UCHAR               iConfiguration;
    UCHAR               bmAttributes;
    UCHAR               MaxPower;

} USB_CONFIGURATION_DESC, *PUSB_CONFIGURATION_DESC;

typedef struct _USB_DEVICE_DESC
{
    UCHAR               bLength;
    UCHAR               bDescriptorType;
    USHORT              bcdUSB;
    UCHAR               bDeviceClass;
    UCHAR               bDeviceSubClass;
    UCHAR               bDeviceProtocol;
    UCHAR               bMaxPacketSize0;
    USHORT              idVendor;
    USHORT              idProduct;
    USHORT              bcdDevice;
    UCHAR               iManufacturer;
    UCHAR               iProduct;
    UCHAR               iSerialNumber;
    UCHAR               bNumConfigurations;

} USB_DEVICE_DESC, *PUSB_DEVICE_DESC;


#define URB_FLAG_STATE_MASK  		0x0f
#define URB_FLAG_STATE_PENDING		0x00
#define URB_FLAG_STATE_IN_PROCESS   0x01
#define URB_FLAG_STATE_FINISHED     0x02

// USB2.0 state
#define URB_FLAG_STATE_DOORBELL		0x03	// for async request removal
#define URB_FLAG_STATE_WAIT_FRAME	0x04	// for sync request removal( for cancel only )
#define URB_FLAG_STATE_ERROR		0x05

#define URB_FLAG_IN_SCHEDULE        0x10
#define URB_FLAG_FORCE_CANCEL       0x20
#define URB_FLAG_SHORT_PACKET       0x80000000

typedef struct _SPLIT_ISO_BUS_TIME
{
	USHORT 				bus_time;
	USHORT				start_uframe;

} SPLIT_ISO_BUS_TIME, *PSPLIT_ISO_BUS_TIME;

typedef struct _ISO_PACKET_DESC
{
	LONG 				offset;
	LONG 				length;		// expected length
	LONG 				actual_length;
	LONG 				status;
	union
	{
		LONG  				bus_time;   //opaque for client request of split iso, the bus_time is the start_uframe for each transaction
		SPLIT_ISO_BUS_TIME 	params;
	};

} ISO_PACKET_DESC, *PISO_PACKET_DESC;

#define CTRL_PARENT_URB_VALID	1

typedef void ( *PURBCOMPLETION )( struct _URB *purb, PVOID pcontext);

typedef struct _CTRL_REQ_STACK
{
	PURBCOMPLETION 		urb_completion;	// the last step of the urb_completion is to call the
										// the prevoius stack's callback if has, and the last
										// one is purb->completion
	PVOID  				context;
	ULONG 				params[ 3 ];

} CTRL_REQ_STACK, *PCTRL_REQ_STACK;

#pragma pack( pop, usb_align )

typedef struct _URB_HS_PIPE_CONTENT
{
	ULONG trans_type : 2;	// bit 0-1
	ULONG mult_count : 2;	// bit 2-3, used in high speed int and iso requests
	ULONG reserved : 1;		// bit 1
	ULONG speed_high : 1;	// bit 5
	ULONG speed_low : 1;	// bit 6
	ULONG trans_dir : 1;	// bit 7
	ULONG dev_addr : 7;		// bit 8-14
	ULONG endp_addr : 4;		// bit 15-18
	ULONG data_toggle : 1;	// bit 19
	ULONG max_packet_size : 4; // bit 20-23 log( max_packet_size )
	ULONG interval : 4;		// bit 24-27 the same definition in USB2.0, for high or full/low speed
	ULONG start_uframe : 3; // bit 28-30
	ULONG reserved1 : 1;    //

} URB_HS_PIPE_CONTENT, *PURB_HS_PIPE_CONTENT;

typedef struct _URB_HS_CONTEXT_CONTENT
{
	ULONG hub_addr : 7;	// high speed hub addr for split transfer
	ULONG port_idx : 7;
	ULONG reserved : 18;

} URB_HS_CONTEXT_CONTENT, *PURB_HS_CONTEXT_CONTENT;

typedef struct _URB
{
    LIST_ENTRY          urb_link;
    ULONG               flags;
	DEV_HANDLE 			endp_handle;
	LONG 				status;
		//record info for isr use, similar to td.status
		//int pipe has different content in the 8 msb
        //the eight bits contain interrupt interval.
        //and max packet length is encoded in 3 bits from 23-21
	    //that means 2^(x) bytes in the packet.
	ULONG 				pipe;               // bit0-1: endp type, bit 6: ls or fs. bit 7: dir

	union
	{
		UCHAR             	setup_packet[8];   	// for control
		LONG				params[ 2 ];		// params[ 0 ] is used in iso transfer as max_packet_size
	};

    PUCHAR              data_buffer;       	//user data
    LONG                data_length;       	//user data length

	struct _USB_DEV 	*pdev;
    struct _USB_ENDPOINT *pendp;             //pipe for current transfer

    PURBCOMPLETION      completion;
    PVOID               context;           	//parameter of completion

	PVOID				urb_ext;			//for high speed hcd use
    ULONG               hs_context;         //for high speed hcd use

	PIRP                pirp;           	//irp from client driver
	LONG  	            reference;          //for caller private use

	LONG        		td_count;           //for any kinds of transfer
    LONG                rest_bytes;         //for split bulk transfer involving more than 1024 tds
	LONG 				bytes_to_transfer;
	LONG          		bytes_transfered;	//( bulk transfer )accumulate one split-transfer by xfered bytes of the executed transactions
	PLIST_ENTRY         last_finished_td;   //last inactive td useful for large amount transfer
    LIST_ENTRY          trasac_list;        //list of tds or qhs

	union
	{
		LONG                iso_start_frame;	// for high speed endp, this is uframe index, and not used for full/low speed endp, instead,
												// iso_packet_desc.param.start_uframe is used.
		LONG				int_start_frame;	// frame( ms ) index for high/full/low speed endp
		struct
		{
			UCHAR			ctrl_req_flags;
			UCHAR			ctrl_stack_count;
			UCHAR			ctrl_cur_stack;		// the receiver uses this by increment the stack pointer first. if the requester
			UCHAR			ctrl_reserved;		// send it down with ctrl_stack_count zero, that means the stack is not initialized,
												// and can be initialized by receiver to 1 and only 1.
												// If the initializer found the stack size won't meet the number down the drivers, it must
												// reallocate one urb with the required stack size. and store the previous urb in
												// ctrl_parent_urb
		} ctrl_req_context;
	};

	union
	{
		LONG 				iso_frame_count;	// uframe for high speed and frame for full/low speed
		struct _URB*		ctrl_parent_urb;
	};

	union
	{
		ISO_PACKET_DESC		iso_packet_desc[ 1 ]; 	//used to build up trasac_list for iso transfer and claim bandwidth
		CTRL_REQ_STACK		ctrl_req_stack[ 1 ];
	};

} URB, *PURB;


NTSTATUS
usb_set_dev_ext(
ULONG dev_ref,
PVOID dev_ext,
LONG size
);

NTSTATUS
usb_set_if_ext(
ULONG dev_ref,
ULONG if_ref,
PVOID if_ext,
LONG size
);

PVOID
usb_get_dev_ext(
ULONG dev_ref
);

PVOID
usb_get_if_ext(
ULONG dev_ref,
ULONG if_ref
);

NTSTATUS
usb_claim_interface(
ULONG dev_ref,
ULONG if_ref,
struct _USB_DRIVER *usb_drvr
);

//return reference to the endp
ULONG
usb_get_endp(
ULONG dev_ref,
LONG endp_addr
);

//return reference to the interface
ULONG
usb_get_interface(
ULONG dev_ref,
LONG if_idx
);

NTSTATUS
usb_set_configuration(
ULONG dev_ref,
LONG config_value
);

// each call will return full size of the config desc and
// its if, endp descs.
// return value is the bytes actually returned.
// if the return value is equal to wTotalLength, all the descs of the
// configuration returned.
NTSTATUS
usb_get_config_desc(
ULONG dev_ref,
LONG config_idx,
PCHAR buffer,
PLONG psize
);

NTSTATUS
usb_submit_urb(
struct _USB_DEV_MANAGER* dev_mgr,
PURB purb
);

NTSTATUS
usb_cancel_urb(
ULONG dev_ref,
ULONG endp_ref,
PURB purb
);

void usb_fill_int_urb(PURB urb,
    struct _USB_DEV *dev,
    ULONG pipe,
    PVOID transfer_buffer,
    LONG buffer_length,
    PURBCOMPLETION complete,
    PVOID context,
    int interval);

LONG
usb_calc_bus_time(
LONG low_speed,
LONG input_dir,
LONG isoc,
LONG bytecount
);

//increment the dev->ref_count to lock the dev
NTSTATUS
usb_query_and_lock_dev(
struct _USB_DEV_MANAGER* dev_mgr,
DEV_HANDLE dev_handle,
struct _USB_DEV** ppdev
);

//decrement the dev->ref_count
NTSTATUS
usb_unlock_dev(
struct _USB_DEV *dev
);

NTSTATUS
usb_reset_pipe(
struct _USB_DEV *pdev,
struct _USB_ENDPOINT *pendp,
PURBCOMPLETION client_completion,
PVOID param			//parameter for client_completion
);

VOID
usb_reset_pipe_completion(
PURB purb,
PVOID pcontext
);

PVOID
usb_alloc_mem(
POOL_TYPE pool_type,
LONG size
);

VOID
usb_free_mem(
PVOID pbuf
);

PUSB_CONFIGURATION_DESC
usb_find_config_desc_by_val(
PUCHAR pbuf,
LONG val,
LONG cfg_count
);

PUSB_CONFIGURATION_DESC
usb_find_config_desc_by_idx(
PUCHAR pbuf,
LONG idx,
LONG cfg_count
);

BOOLEAN
usb_skip_if_and_altif(
PUCHAR* pdesc_BUF
);

BOOLEAN
usb_skip_one_config(
PUCHAR* pconfig_desc_BUF
);

VOID
usb_wait_ms_dpc(
ULONG ms
);

VOID
usb_wait_us_dpc(
ULONG us
);

BOOLEAN
usb_query_clicks(
PLARGE_INTEGER	clicks
);

VOID
usb_cal_cpu_freq();

NTSTATUS
usb_reset_pipe_ex(
struct _USB_DEV_MANAGER *dev_mgr,
DEV_HANDLE endp_handle,
PURBCOMPLETION reset_completion,
PVOID param
);

VOID
usb_call_ctrl_completion(
PURB purb
);

BOOLEAN
is_header_match(
PUCHAR pbuf,
ULONG type
); //used to check descriptor validity
#endif
