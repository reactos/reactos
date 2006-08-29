
#ifndef __HUB_H__
#define __HUB_H__

/*
 * Hub Class feature numbers
 */
#define C_HUB_LOCAL_POWER	0
#define C_HUB_OVER_CURRENT	1

/*
 * Port feature numbers
 */
#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET			4
#define USB_PORT_FEAT_POWER			8
#define USB_PORT_FEAT_LOWSPEED		9
#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20

/* wPortStatus bits */
#define USB_PORT_STAT_CONNECTION	0x0001
#define USB_PORT_STAT_ENABLE		0x0002
#define USB_PORT_STAT_SUSPEND		0x0004
#define USB_PORT_STAT_OVERCURRENT	0x0008
#define USB_PORT_STAT_RESET			0x0010
#define USB_PORT_STAT_POWER			0x0100
#define USB_PORT_STAT_LOW_SPEED		0x0200

/* usb 2.0 features */
#define USB_PORT_STAT_HIGH_SPEED	0x0400
#define USB_PORT_STAT_PORT_TEST		0x0800
#define USB_PORT_STAT_PORT_INDICATOR 0x1000

/* wPortChange bits */
#define USB_PORT_STAT_C_CONNECTION	0x0001
#define USB_PORT_STAT_C_ENABLE		0x0002
#define USB_PORT_STAT_C_SUSPEND		0x0004
#define USB_PORT_STAT_C_OVERCURRENT	0x0008
#define USB_PORT_STAT_C_RESET		0x0010

/* wHubCharacteristics (masks) */
#define HUB_CHAR_LPSM		0x0003
#define HUB_CHAR_COMPOUND	0x0004
#define HUB_CHAR_OCPM		0x0018

/*
 *Hub Status & Hub Change bit masks
 */
#define HUB_STATUS_LOCAL_POWER	0x0001
#define HUB_STATUS_OVERCURRENT	0x0002

#define HUB_CHANGE_LOCAL_POWER	0x0001
#define HUB_CHANGE_OVERCURRENT	0x0002

#define HUB_DESCRIPTOR_MAX_SIZE	39	/* enough for 127 ports on a hub */

#define MAX_HUB_PORTS  					8
#define USB_HUB_INTERVAL 				0xff

#define USB_PORT_FLAG_STATE_MASK		( 0xf << 16 )
#define USB_PORT_FLAG_ENABLE            ( 0x1 << 16 )
#define USB_PORT_FLAG_DISABLE           ( 0x2 << 16 )
#define USB_PORT_FLAG_DISCONNECT        ( 0x3 << 16 )

#define USB_PORT_QUE_STATE_MASK   		0xff	// for detail, refer to document.txt
#define STATE_IDLE						0x00
#define STATE_EXAMINE_STATUS_QUE		0x01	// set when post a event to examine the status queue
#define STATE_WAIT_STABLE				0x02	// set when a new connection comes and about to wait some ms
#define STATE_WAIT_RESET				0x03	// set when dev stable and about to reset, may pending in the event queue
#define STATE_WAIT_RESET_COMPLETE       0x04	// set when reset signal is about to assert on the port
#define STATE_WAIT_ADDRESSED			0x05	// set when reset complete and before address is assigned

#define port_state( port_FLAG ) \
( port_FLAG & USB_PORT_QUE_STATE_MASK )

#define set_port_state( port_FLAG, stATE ) \
{ port_FLAG = ( port_FLAG & (~USB_PORT_QUE_STATE_MASK ) ) | ( stATE & USB_PORT_QUE_STATE_MASK ) ;}

#define default_endp_handle( endp_HANDLE ) \
( ( endp_HANDLE & 0xffff ) == 0xffff )

#define is_if_dev( pdev ) ( pdev->flags & USB_DEV_FLAG_IF_DEV )

#define is_composite_dev( pdev ) \
( is_if_dev( pdev ) == FALSE \
&& pdev->pusb_dev_desc->bDeviceClass == 0 \
&& pdev->pusb_dev_desc->bDeviceSubClass == 0 )

#define dev_handle_from_dev( pdev ) ( pdev->dev_id << 16 )
#define hub_ext_from_dev( pdEV ) ( ( PHUB2_EXTENSION )pdEV->dev_ext )

#pragma pack( push, hub_align, 1 )
typedef struct _USB_PORT_STATUS
{
	USHORT 	wPortStatus;
	USHORT	wPortChange;

} USB_PORT_STATUS, *PUSB_PORT_STATUS;


typedef struct _USB_HUB_STATUS
{
	USHORT wHubStatus;
	USHORT wHubChange;

} USB_HUB_STATUS, *PUSB_HUB_STATUS;


typedef struct _USB_HUB_DESCRIPTOR
{
	BYTE 	bLength;
	BYTE 	bDescriptorType;
	BYTE 	bNbrPorts;
	USHORT	wHubCharacteristics;
	BYTE 	bPwrOn2PwrGood;
	BYTE 	bHubContrCurrent;

	/* DeviceRemovable and PortPwrCtrlMask want to be variable-length 
	   bitmaps that hold max 256 entries, but for now they're ignored */
	BYTE 	bitmap[0];
} USB_HUB_DESCRIPTOR, *PUSB_HUB_DESCRIPTOR;
#pragma pack( pop, hub_align )

typedef struct _PORT_STATUS_QUEUE
{
	USB_PORT_STATUS				port_status[ 4 ];
	BYTE						status_count;
	ULONG 						port_flags;

} PORT_STATUS_QUEUE, *PPORT_STATUS_QUEUE;

VOID
psq_init(
PPORT_STATUS_QUEUE psq
);

BOOLEAN
psq_enqueue(
PPORT_STATUS_QUEUE psq,
ULONG status
);
ULONG
psq_outqueue(
PPORT_STATUS_QUEUE psq
);			//return 0xffffffff if no element

BOOLEAN
psq_push(
PPORT_STATUS_QUEUE psq,
ULONG status
);

#define psq_is_empty( pSQ ) ( ( pSQ )->status_count == 0 )
#define psq_is_full( pSQ ) ( ( pSQ )->status_count == 4 )
#define psq_count( psq ) ( ( psq )->status_count )
#define psq_peek( psq, i ) \
( ( ( ULONG )i ) < 3 ? ( psq )->port_status[ i ] : ( psq )->port_status[ 3 ] )

#define MAX_DEVS        		127
#define EHCI_MAX_ROOT_PORTS		15

typedef struct _HUB_EXTENSION
{

	LONG 						port_count;
	PUSB_DEV  					child_dev[ MAX_HUB_PORTS + 1 ];

	PORT_STATUS_QUEUE			port_status_queue[ MAX_HUB_PORTS + 1];
	PUSB_DEV   					pdev;
	PUSB_INTERFACE				pif;
	BYTE 						int_data_buf[ 64 ];

	USB_HUB_STATUS   			hub_status;
	USB_PORT_STATUS				port_status;		//working data buf for get port feature
	
	USB_PORT_STATUS				rh_port1_status; 	//working buf for get rh port1 feature
	USB_PORT_STATUS				rh_port2_status; 	//working buf for get rh port2 feature
	
	USB_HUB_DESCRIPTOR			hub_desc;

} HUB_EXTENSION, *PHUB_EXTENSION;

typedef struct _HUB2_PORT_TT
{
	ULONG tt_busy : 1;

} HUB2_PORT_TT, *PHUB2_PORT_TT;

typedef struct _HUB2_EXTENSION
{

	LONG 						port_count;
	PUSB_DEV  					child_dev[ MAX_HUB_PORTS + 1 ];

	PORT_STATUS_QUEUE			port_status_queue[ MAX_HUB_PORTS + 1];

	PUSB_DEV   					pdev;
	PUSB_INTERFACE				pif;
	BYTE 						int_data_buf[ 32 ];	// for ports up to 127

	USB_HUB_STATUS   			hub_status;
	USB_PORT_STATUS				port_status;		//working data buf for get port feature
	
	USB_PORT_STATUS				rh_port_status[ EHCI_MAX_ROOT_PORTS ]; 	//working buf for get rh ports feature

	UCHAR						multiple_tt;			// boolean
	ULONG						tt_status_map[ 4 ];		// bit map to indicate the indexed tt's periodic buffer busy or not
	ULONG						tt_bulk_map[ 4 ];		// bit map to indicate the indexed tt's bulk/control buffer busy or not
	USB_HUB_DESCRIPTOR			hub_desc;

} HUB2_EXTENSION, *PHUB2_EXTENSION;


VOID
event_list_default_process_queue(
PLIST_HEAD event_list,
PUSB_EVENT_POOL event_pool,
PUSB_EVENT usb_event,
PUSB_EVENT out_event
);

VOID
event_list_default_process_event(
PUSB_DEV dev,
ULONG event,
ULONG context,
ULONG param
);

// root hub routines and definitions

#define RH_INTERVAL   ( USB_HUB_INTERVAL / DEV_MGR_TIMER_INTERVAL_MS )

BOOLEAN
rh_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
rh_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
rh_destroy(
PUSB_DEV pdev
);

VOID
rh_timer_svc(
PUSB_DEV dev,
PVOID context
);

BOOLEAN
rh_submit_urb(
PUSB_DEV pdev,
PURB urb
);

BOOLEAN
hub_init(
PUSB_DEV pdev
);

BOOLEAN
hub_destroy(
PUSB_DEV pdev
);

BOOLEAN
hub_lock_tt(
PUSB_DEV pdev,
UCHAR port_idx,
UCHAR type   // transfer type
);

BOOLEAN
hub_unlock_tt(
PUSB_DEV pdev,
UCHAR port_idx,
UCHAR type
);


VOID
hub_post_clear_tt_event(
PUSB_DEV pdev,
BYTE port_idx,
ULONG pipe
);

BOOLEAN
compdev_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
compdev_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
gendrv_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
gendrv_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
gendrv_if_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN
gendrv_if_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOLEAN hub_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);
BOOLEAN hub_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);
BOOLEAN hub_remove_reset_event(PUSB_DEV pdev, ULONG port_idx, BOOLEAN from_dpc);
BOOLEAN hub_start_next_reset_port(PUSB_DEV_MANAGER dev_mgr, BOOLEAN from_dpc);
NTSTATUS hub_start_int_request(PUSB_DEV pdev);

#endif


