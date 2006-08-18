
#ifndef __HUB_H__
#define __HUB_H__

#include "td.h"
//#include "hcd.h"
#include "ntddk.h"

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

//event definitions

#define MAX_EVENTS          128
#define MAX_TIMER_SVCS      24

#define USB_EVENT_FLAG_ACTIVE       0x80000000

#define USB_EVENT_FLAG_QUE_TYPE     0x000000FF    
#define USB_EVENT_FLAG_QUE_RESET  	0x01
#define USB_EVENT_FLAG_NOQUE        0x00

#define USB_EVENT_DEFAULT			0x00		//as a placeholder
#define USB_EVENT_INIT_DEV_MGR		0x01
#define USB_EVENT_HUB_POLL          0x02
#define USB_EVENT_WAIT_RESET_PORT   0x03
#define USB_EVENT_CLEAR_TT_BUFFER	0x04

typedef VOID ( *PROCESS_QUEUE )(
PLIST_HEAD event_list,
struct _USB_EVENT_POOL *event_pool,
struct _USB_EVENT *usb_event,
struct _USB_EVENT *out_event
);

typedef VOID ( *PROCESS_EVENT )(
PUSB_DEV dev,
ULONG event,
ULONG context,
ULONG param
);

typedef struct _USB_EVENT
{
    LIST_ENTRY          event_link;
    ULONG               flags;
    ULONG               event;
    PUSB_DEV            pdev;
    ULONG               context;
	ULONG        		param;
    struct _USB_EVENT   *pnext;         //vertical queue for serialized operation
    PROCESS_EVENT       process_event;
    PROCESS_QUEUE       process_queue;
    
} USB_EVENT, *PUSB_EVENT;

typedef struct _USB_EVENT_POOL
{
    PUSB_EVENT          event_array;
    LIST_HEAD           free_que;
    LONG                free_count;
    LONG                total_count;
    KSPIN_LOCK          pool_lock;

} USB_EVENT_POOL, *PUSB_EVENT_POOL;

BOOL
init_event_pool(
PUSB_EVENT_POOL pool
);

BOOL
free_event(
PUSB_EVENT_POOL pool,
PUSB_EVENT pevent
); //add qhs till pnext == NULL

PUSB_EVENT
alloc_event(
PUSB_EVENT_POOL pool,
LONG count
);  //null if failed

BOOL
destroy_event_pool(
PUSB_EVENT_POOL pool
);

VOID
lock_event_pool(
PUSB_EVENT_POOL pool
);

VOID
unlock_event_pool(
PUSB_EVENT_POOL pool
);

#define DEV_MGR_TIMER_INTERVAL_NS  ( 10 * 1000 * 10 ) //unit 100 ns
#define DEV_MGR_TIMER_INTERVAL_MS  10

typedef VOID ( *TIMER_SVC_HANDLER )(PUSB_DEV dev, PVOID context);

typedef struct _TIMER_SVC
{
	LIST_ENTRY       	timer_svc_link;
    ULONG               counter;
    ULONG               threshold;
    ULONG               context;
    PUSB_DEV            pdev;
    TIMER_SVC_HANDLER   func;
    
} TIMER_SVC, *PTIMER_SVC;

typedef struct _TIMER_SVC_POOL
{
    PTIMER_SVC          timer_svc_array;
    LIST_HEAD           free_que;
    LONG                free_count;
    LONG                total_count;
    KSPIN_LOCK          pool_lock;

} TIMER_SVC_POOL, *PTIMER_SVC_POOL;

BOOL
init_timer_svc_pool(
PTIMER_SVC_POOL pool
);
BOOL
free_timer_svc(
PTIMER_SVC_POOL pool,
PTIMER_SVC ptimer
);

PTIMER_SVC
alloc_timer_svc(
PTIMER_SVC_POOL pool,
LONG count
);  //null if failed

BOOL
destroy_timer_svc_pool(
PTIMER_SVC_POOL pool
);

VOID
lock_timer_svc_pool(
PTIMER_SVC_POOL pool
);

VOID
unlock_timer_svc_pool(
PTIMER_SVC_POOL pool
);

#define MAX_IRP_LIST_SIZE	32

typedef struct _IRP_LIST_ELEMENT
{
	LIST_ENTRY  irp_link;
	PIRP		pirp;
	struct _URB *purb;

} IRP_LIST_ELEMENT, *PIRP_LIST_ELEMENT;

typedef struct _IRP_LIST
{
	KSPIN_LOCK 			irp_list_lock;
	LIST_HEAD			irp_busy_list;
	LONG				irp_free_list_count;
	LIST_HEAD			irp_free_list;
	PIRP_LIST_ELEMENT 	irp_list_element_array;

} IRP_LIST, *PIRP_LIST;

BOOL
init_irp_list( 
PIRP_LIST irp_list
);

VOID
destroy_irp_list(
PIRP_LIST irp_list
);

BOOL
add_irp_to_list(
PIRP_LIST irp_list,
PIRP pirp,
PURB purb
);

PURB
remove_irp_from_list(
PIRP_LIST irp_list,
PIRP pirp,
struct _USB_DEV_MANAGER *dev_mgr
);

BOOL
irp_list_empty(
PIRP_LIST irp_list
);

BOOL
irp_list_full(
PIRP_LIST irp_list
);

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

BOOL
psq_enqueue(
PPORT_STATUS_QUEUE psq,
ULONG status
);
ULONG
psq_outqueue(
PPORT_STATUS_QUEUE psq
);			//return 0xffffffff if no element

BOOL
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

typedef struct _CONNECT_DATA
{
	DEV_HANDLE 	dev_handle;
	struct _USB_DRIVER *pdriver;
	struct _USB_DEV_MANAGER *dev_mgr;

} CONNECT_DATA, *PCONNECT_DATA;

typedef BOOL ( *PDEV_CONNECT_EX )( PCONNECT_DATA init_param, DEV_HANDLE dev_handle );
typedef BOOL ( *PDEV_CONNECT )( struct _USB_DEV_MANAGER *dev_mgr, DEV_HANDLE dev_handle );
typedef BOOL ( *PDRVR_INIT )( struct _USB_DEV_MANAGER *dev_mgr, struct _USB_DRIVER *pdriver );

typedef struct _PNP_DISPATCH
{
	ULONG				version;
	PDEV_CONNECT_EX		dev_connect;
	PDEV_CONNECT 		dev_reserved;		//currently we do not use this entry
	PDEV_CONNECT 		dev_stop;
	PDEV_CONNECT 		dev_disconnect;

}PNP_DISPATCH, *PPNP_DISPATCH;

#define USB_DRIVER_FLAG_IF_CAPABLE 		0x80000000
#define USB_DRIVER_FLAG_DEV_CAPABLE		0x40000000

typedef struct _USB_DRIVER_DESCRIPTION
{
	// Device Info
	DWORD flags;
	WORD vendor_id; 							// USB Vendor ID
	WORD product_id;							// USB Product ID.
	WORD release_num;							// Release Number of Device
	
	// Interface Info
	BYTE config_val;							// Configuration Value
	BYTE if_num;								// Interface Number
	BYTE if_class;								// Interface Class
	BYTE if_sub_class; 							// Interface SubClass
	BYTE if_protocol;							// Interface Protocol
	
	// Driver Info	
	PBYTE driver_name;							// Driver name for Name Registry
	BYTE dev_class;								// Device Class (from SampleStorageDeviceID.h)
	BYTE dev_sub_class;							// Device Subclass
	BYTE dev_protocol;							// Protocol Info.

} USB_DRIVER_DESCRIPTION,*PUSB_DRIVER_DESCRIPTION;

#define DEVMGR_MAX_DRIVERS 	6
#define RH_DRIVER_IDX  		0
#define HUB_DRIVER_IDX		1
#define UMSS_DRIVER_IDX		2
#define COMP_DRIVER_IDX		3
#define GEN_DRIVER_IDX		4
#define GEN_IF_DRIVER_IDX	5

typedef struct _USB_DRIVER
{
	USB_DRIVER_DESCRIPTION 	driver_desc;
	PNP_DISPATCH 			disp_tbl;

	PBYTE					driver_ext;
	LONG					driver_ext_size;

	PDRVR_INIT				driver_init;
	PDRVR_INIT				driver_destroy;

} USB_DRIVER, *PUSB_DRIVER;

extern USB_DRIVER g_driver_list[ DEVMGR_MAX_DRIVERS ];

#define MAX_HCDS 		8
#define dev_mgr_from_hcd( hCD ) ( ( hCD )->hcd_get_dev_mgr( hCD ) )
#define dev_mgr_from_dev( pdEV ) ( dev_mgr_from_hcd( pdEV->hcd ) ) 

typedef struct _USB_DEV_MANAGER
{
    //BYTE                dev_addr_map[ MAX_DEVS / 8 ];     //one bit per dev
	struct _HCD			*hcd_array[ MAX_HCDS ];
   	unsigned char		hcd_count; 
	
	KSPIN_LOCK          dev_list_lock;
    LIST_HEAD           dev_list;

	//PDEVICE_EXTENSION 	pdev_ext;
	
	PVOID				pthread;	
	BOOL    			term_flag;
	KEVENT       		wake_up_event;

    KSPIN_LOCK          event_list_lock;
    LIST_HEAD           event_list;
	USB_EVENT_POOL   	event_pool;

	KTIMER   			dev_mgr_timer;
	KDPC   				dev_mgr_timer_dpc;
    KSPIN_LOCK          timer_svc_list_lock;
    LIST_HEAD           timer_svc_list;
	TIMER_SVC_POOL	    timer_svc_pool;
	LONG 				timer_click;
	IRP_LIST			irp_list;

	PUSB_DRIVER			driver_list;
	LONG                hub_count;
	LIST_HEAD			hub_list;			//for reference only

        //statistics
    LONG                conn_count;			//will also be used to assign device id
	PDRIVER_OBJECT		usb_driver_obj;		//this driver object
	LONG				open_count;			//increment when IRP_MJ_CREATE arrives
											//and decrement when IRP_MJ_CLOSE arrives

} USB_DEV_MANAGER, *PUSB_DEV_MANAGER;

BOOL
dev_mgr_post_event(
PUSB_DEV_MANAGER dev_mgr,
PUSB_EVENT event
);

BOOL
dev_mgr_init(
PUSB_DEV dev,		//always null. we do not use this param
ULONG event,
ULONG dev_mgr
);

VOID
dev_mgr_destroy(
PUSB_DEV_MANAGER dev_mgr
);

VOID
dev_mgr_thread(
PVOID dev_mgr
);

VOID
dev_mgr_timer_dpc_callback(
PKDPC Dpc,
PVOID DeferredContext,
PVOID SystemArgument1,
PVOID SystemArgument2
);

BOOL
dev_mgr_request_timer_svc(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DEV pdev,
ULONG context,
ULONG due_time,
TIMER_SVC_HANDLER handler
);

BYTE
dev_mgr_alloc_addr(
PUSB_DEV_MANAGER dev_mgr,
PHCD hcd
);

BOOL
dev_mgr_free_addr(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DEV pdev,
BYTE addr
);

PUSB_DEV
dev_mgr_alloc_device(
PUSB_DEV_MANAGER dev_mgr,
PHCD hcd
);

VOID
dev_mgr_free_device(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DEV pdev
);

VOID
dev_mgr_disconnect_dev(
PUSB_DEV pdev
);

BOOL
dev_mgr_strobe(
PUSB_DEV_MANAGER dev_mgr
);

NTSTATUS
dev_mgr_build_usb_config(
PUSB_DEV pdev,
PBYTE pbuf,
ULONG config_val,
LONG config_count
);

UCHAR
dev_mgr_register_hcd(
PUSB_DEV_MANAGER dev_mgr,
PHCD hcd
);

NTSTATUS
dev_mgr_dispatch(
IN PUSB_DEV_MANAGER dev_mgr,
IN PIRP           	irp
);

BOOL
dev_mgr_register_irp(
PUSB_DEV_MANAGER dev_mgr,
PIRP pirp,
PURB purb
);

PURB
dev_mgr_remove_irp(
PUSB_DEV_MANAGER dev_mgr,
PIRP pirp
);

LONG
dev_mgr_score_driver_for_if(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver,
PUSB_INTERFACE_DESC pif_desc
);

BOOL
dev_mgr_set_driver(
PUSB_DEV_MANAGER dev_mgr,
DEV_HANDLE dev_handle,
PUSB_DRIVER pdriver,
PUSB_DEV pdev	//if pdev != NULL, we use pdev instead if_handle
);

BOOL
dev_mgr_set_if_driver(
PUSB_DEV_MANAGER dev_mgr,
DEV_HANDLE if_handle,
PUSB_DRIVER pdriver,
PUSB_DEV pdev	//if pdev != NULL, we use pdev instead if_handle
);

VOID
dev_mgr_release_hcd(
PUSB_DEV_MANAGER dev_mgr
);

VOID
dev_mgr_start_hcd(
PUSB_DEV_MANAGER dev_mgr
);

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

BOOL
rh_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
rh_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
rh_destroy(
PUSB_DEV pdev
);

VOID
rh_timer_svc(
PUSB_DEV dev,
PVOID context
);

BOOL
rh_submit_urb(
PUSB_DEV pdev,
PURB urb
);

BOOL
hub_init(
PUSB_DEV pdev
);

BOOL
hub_destroy(
PUSB_DEV pdev
);

BOOL
hub_lock_tt(
PUSB_DEV pdev,
UCHAR port_idx,
UCHAR type   // transfer type
);

BOOL
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

BOOL
compdev_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
compdev_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
gendrv_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
gendrv_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
gendrv_if_driver_init(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);

BOOL
gendrv_if_driver_destroy(
PUSB_DEV_MANAGER dev_mgr,
PUSB_DRIVER pdriver
);
#endif


