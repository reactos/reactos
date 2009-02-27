#ifndef __DEVMGR_H__
#define __DEVMGR_H__

typedef struct _DEV_CONNECT_DATA
{
	DEV_HANDLE 	dev_handle;
	struct _USB_DRIVER *pdriver;
	struct _USB_DEV_MANAGER *dev_mgr;
	PUSB_INTERFACE_DESC if_desc;

} DEV_CONNECT_DATA, *PDEV_CONNECT_DATA;

typedef BOOLEAN ( *PDEV_CONNECT_EX )( PDEV_CONNECT_DATA init_param, DEV_HANDLE dev_handle );
typedef BOOLEAN ( *PDEV_CONNECT )( struct _USB_DEV_MANAGER *dev_mgr, DEV_HANDLE dev_handle );
typedef BOOLEAN ( *PDRVR_INIT )( struct _USB_DEV_MANAGER *dev_mgr, struct _USB_DRIVER *pdriver );

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
	const char *driver_name;					// Driver name for Name Registry
	BYTE dev_class;								// Device Class (from SampleStorageDeviceID.h)
	BYTE dev_sub_class;							// Device Subclass
	BYTE dev_protocol;							// Protocol Info.

} USB_DRIVER_DESCRIPTION,*PUSB_DRIVER_DESCRIPTION;

#define DEVMGR_MAX_DRIVERS 	7//8
#define RH_DRIVER_IDX  		0
#define HUB_DRIVER_IDX		1
#define UMSS_DRIVER_IDX		2
#define COMP_DRIVER_IDX		3
#define GEN_DRIVER_IDX		4
#define GEN_IF_DRIVER_IDX	5
#define MOUSE_DRIVER_IDX	6
#define KEYBOARD_DRIVER_IDX	7//temp disabled

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
	BOOLEAN    			term_flag;
	KEVENT       		wake_up_event;

    KSPIN_LOCK          event_list_lock;
    LIST_HEAD           event_list;
	USB_EVENT_POOL   	event_pool;

    KEVENT              drivers_inited;
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

BOOLEAN
dev_mgr_post_event(
PUSB_DEV_MANAGER dev_mgr,
PUSB_EVENT event
);

BOOLEAN
dev_mgr_init(
PUSB_DEV dev,		//always null. we do not use this param
ULONG event,
ULONG dev_mgr
);

VOID
dev_mgr_destroy(
PUSB_DEV_MANAGER dev_mgr
);

VOID NTAPI
dev_mgr_thread(
PVOID dev_mgr
);

VOID NTAPI
dev_mgr_timer_dpc_callback(
PKDPC Dpc,
PVOID DeferredContext,
PVOID SystemArgument1,
PVOID SystemArgument2
);

BOOLEAN
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

BOOLEAN
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

BOOLEAN
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

BOOLEAN
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

BOOLEAN
dev_mgr_set_driver(
PUSB_DEV_MANAGER dev_mgr,
DEV_HANDLE dev_handle,
PUSB_DRIVER pdriver,
PUSB_DEV pdev	//if pdev != NULL, we use pdev instead if_handle
);

BOOLEAN
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

BOOLEAN dev_mgr_start_config_dev(PUSB_DEV pdev);
BOOLEAN dev_mgr_event_init(PUSB_DEV dev,   //always null. we do not use this param
                        ULONG event,
                        ULONG context,
                        ULONG param);
VOID dev_mgr_get_desc_completion(PURB purb, PVOID context);
VOID dev_mgr_event_select_driver(PUSB_DEV pdev, ULONG event, ULONG context, ULONG param);
LONG dev_mgr_score_driver_for_dev(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver, PUSB_DEVICE_DESC pdev_desc);
NTSTATUS dev_mgr_destroy_usb_config(PUSB_CONFIGURATION pcfg);
BOOLEAN dev_mgr_start_select_driver(PUSB_DEV pdev);
VOID dev_mgr_cancel_irp(PDEVICE_OBJECT pdev_obj, PIRP pirp);

#endif
