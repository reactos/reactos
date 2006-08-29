#ifndef __HCD_H__
#define __HCD_H__

#define HCD_TYPE_MASK 	0xf0
#define HCD_TYPE_UHCI	0x10
#define HCD_TYPE_OHCI	0x20
#define HCD_TYPE_EHCI	0x30

#define hcd_type( hCD ) ( ( ( hCD )->flags ) & HCD_TYPE_MASK )
#define usb2( hCD ) ( hcd_type( hCD ) == HCD_TYPE_EHCI )

#define	HCD_ID_MASK		0xf

#define HCD_DISP_READ_PORT_COUNT	1		// the param is a pointer to UCHAR
#define HCD_DISP_READ_RH_DEV_CHANGE	2		// the param is a buffer to hold conn change on all the port 
											// must have the rh dev_lock acquired

struct _HCD;
struct _USB_DEV_MANAGER;
struct _USB_DEV;
struct _USB_ENDPOINT;
struct _URB;

typedef	VOID ( *PHCD_SET_DEV_MGR )( struct _HCD* hcd, struct _USB_DEV_MANAGER  *dev_mgr );
typedef	struct _USB_DEV_MANAGER* ( *PHCD_GET_DEV_MGR )( struct _HCD* hcd );
typedef ULONG ( *PHCD_GET_TYPE )( struct _HCD* hcd );	
typedef VOID ( *PHCD_SET_ID )(  struct _HCD* hcd, UCHAR id );
typedef UCHAR ( *PHCD_GET_ID )( struct _HCD* hcd );
typedef UCHAR ( *PHCD_ALLOC_ADDR )( struct _HCD* hcd );
typedef VOID ( *PHCD_FREE_ADDR )(  struct _HCD* hcd, UCHAR addr );
typedef NTSTATUS ( *PHCD_SUBMIT_URB )( struct _HCD* hcd, struct _USB_DEV *pdev, struct _USB_ENDPOINT *pendp, struct _URB *purb );	
typedef VOID ( *PHCD_GENERIC_URB_COMPLETION )(  struct _URB *purb, PVOID context ); //we can get te hcd from purb
typedef struct _USB_DEV* ( *PHCD_GET_ROOT_HUB )(  struct _HCD* hcd );
typedef VOID ( *PHCD_SET_ROOT_HUB )(  struct _HCD* hcd, struct _USB_DEV *root_hub );
typedef BOOLEAN ( *PHCD_REMOVE_DEVICE )( struct _HCD* hcd, struct _USB_DEV *pdev );
typedef BOOLEAN ( *PHCD_RH_RESET_PORT )( struct _HCD* hcd, UCHAR port_idx );   //must have the rh dev_lock acquired
typedef BOOLEAN ( *PHCD_RELEASE )( struct _HCD* hcd );   //must have the rh dev_lock acquired
typedef NTSTATUS( *PHCD_CANCEL_URB)( struct _HCD* hcd, struct _USB_DEV *pdev, struct _USB_ENDPOINT* pendp, struct _URB *purb );
typedef BOOLEAN ( *PHCD_START )( struct _HCD* hcd );   //must have the rh dev_lock acquired
typedef NTSTATUS ( *PHCD_DISPATCH )( struct _HCD* hcd, LONG disp_code, PVOID param ); // locking depends on type of code

typedef struct _HCD
{
	PHCD_SET_DEV_MGR 			hcd_set_dev_mgr;
	PHCD_GET_DEV_MGR			hcd_get_dev_mgr;
	PHCD_GET_TYPE				hcd_get_type;
	PHCD_SET_ID					hcd_set_id;
	PHCD_GET_ID					hcd_get_id;
	PHCD_ALLOC_ADDR				hcd_alloc_addr;
	PHCD_FREE_ADDR				hcd_free_addr;
	PHCD_SUBMIT_URB				hcd_submit_urb;
	PHCD_GENERIC_URB_COMPLETION hcd_generic_urb_completion;
	PHCD_GET_ROOT_HUB 			hcd_get_root_hub;
	PHCD_SET_ROOT_HUB			hcd_set_root_hub;
	PHCD_REMOVE_DEVICE			hcd_remove_device; 
	PHCD_RH_RESET_PORT			hcd_rh_reset_port;
	PHCD_RELEASE				hcd_release;
	PHCD_CANCEL_URB				hcd_cancel_urb;
	PHCD_START					hcd_start;
	PHCD_DISPATCH				hcd_dispatch;

	//interfaces for all the host controller
	ULONG 						flags; 				//hcd types | hcd id
	ULONG 						conn_count;			//statics for connection activities
	struct _USB_DEV_MANAGER 	*dev_mgr;  			//pointer manager
	UCHAR						dev_addr_map[ 128 / 8 ]; //bitmap for the device addrs
	struct _DEVICE_EXTENSION 	*pdev_ext;

} HCD, *PHCD;

#endif
