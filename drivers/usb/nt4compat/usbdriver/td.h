/*
 *  NOTE!!!: part of the code is pasted from linux/driver/usb/uhci.h
 */

#ifndef __UHCI_H__
#define __UHCI_H__

#define LIST_HEAD                   LIST_ENTRY
#define PLIST_HEAD                  PLIST_ENTRY
#define BYTE                        UCHAR
#define PBYTE                       PUCHAR
#define WORD                        USHORT
#define DWORD						ULONG

#ifndef LOWORD
#define LOWORD(l)					( (WORD) ( ( l ) & 0xffff ) )
#endif
#ifndef HIWORD
#define HIWORD(l)					( (WORD) ( ( l ) >> 16 ) )
#endif

#define PCI_MAX_FUNCTIONS           8


#define UHCI_MAX_POOL_TDS             1024    	//8 pages of 4k
#define UHCI_MAX_POOL_QHS             256		//1 page of 4k
#define UHCI_MAX_ANT_TDS 			  0x20e82

#define UHCI_MAX_TD_POOLS    8
#define UHCI_MAX_TDS_PER_TRANSFER     UHCI_MAX_POOL_TDS

#define UHCI_FULL_SPEED_BANDWIDHT     ( 12 * 1000 * 1000 )
#define UHCI_LOW_SPEED_BANDWIDHT      ( 15 * 100 * 1000 )
/*
 * Universal Host Controller Interface data structures and defines
 */

/* Command register */
#define USBCMD      		0
#define   USBCMD_RS     	0x0001  /* Run/Stop */
#define   USBCMD_HCRESET    0x0002  /* Host reset */
#define   USBCMD_GRESET     0x0004  /* Global reset */
#define   USBCMD_EGSM       0x0008  /* Global Suspend Mode */
#define   USBCMD_FGR        0x0010  /* Force Global Resume */
#define   USBCMD_SWDBG      0x0020  /* SW Debug mode */
#define   USBCMD_CF     	0x0040  /* Config Flag (sw only) */
#define   USBCMD_MAXP       0x0080  /* Max Packet (0 = 32, 1 = 64) */

/* Status register */
#define USBSTS      		2
#define   USBSTS_USBINT     0x0001  /* Interrupt due to IOC */
#define   USBSTS_ERROR      0x0002  /* Interrupt due to error */
#define   USBSTS_RD     	0x0004  /* Resume Detect */
#define   USBSTS_HSE        0x0008  /* Host System Error - basically PCI problems */
#define   USBSTS_HCPE       0x0010  /* Host Controller Process Error - the scripts were buggy */
#define   USBSTS_HCH        0x0020  /* HC Halted */

/* Interrupt enable register */
#define USBINTR     		4
#define   USBINTR_TIMEOUT   0x0001  /* Timeout/CRC error enable */
#define   USBINTR_RESUME    0x0002  /* Resume interrupt enable */
#define   USBINTR_IOC       0x0004  /* Interrupt On Complete enable */
#define   USBINTR_SP        0x0008  /* Short packet interrupt enable */

#define USBFRNUM    		6
#define USBFLBASEADD    	8
#define USBSOF      		12

/* USB port status and control registers */
#define USBPORTSC1  		16
#define USBPORTSC2  		18
#define   USBPORTSC_CCS     0x0001  /* Current Connect Status ("device present") */
#define   USBPORTSC_CSC     0x0002  /* Connect Status Change */
#define   USBPORTSC_PE      0x0004  /* Port Enable */
#define   USBPORTSC_PEC     0x0008  /* Port Enable Change */
#define   USBPORTSC_LS      0x0020  /* Line Status */
#define   USBPORTSC_RD      0x0040  /* Resume Detect */
#define   USBPORTSC_LSDA    0x0100  /* Low Speed Device Attached */
#define   USBPORTSC_PR      0x0200  /* Port Reset */
#define   USBPORTSC_SUSP    0x1000  /* Suspend */

/* Legacy support register */
#define   USBLEGSUP		0xc0
#define   USBLEGSUP_DEFAULT	0x2000	/* only PIRQ enable set */
#define   USBLEGSUP_RWC		0x8f00	/* the R/WC bits */
#define   USBLEGSUP_RO		0x5040	/* R/O and reserved bits */


#define UHCI_NULL_DATA_SIZE 0x7FF   /* for UHCI controller TD */

#define UHCI_PTR_BITS       0x000F
#define UHCI_PTR_TERM       0x0001
#define UHCI_PTR_QH     	0x0002
#define UHCI_PTR_DEPTH      0x0004

#define UHCI_MAX_FRAMES      1024    /* in the frame list [array] */
#define UHCI_MAX_SOF_NUMBER  2047    /* in an SOF packet */
#define CAN_SCHEDULE_FRAMES  1000    /* how far future frames can be scheduled */


//from linux's uhci.h
#define UHCI_MAX_SKELTDS    10
#define skel_int1_td        skel_td[0]
#define skel_int2_td        skel_td[1]
#define skel_int4_td        skel_td[2]
#define skel_int8_td        skel_td[3]
#define skel_int16_td       skel_td[4]
#define skel_int32_td       skel_td[5]
#define skel_int64_td       skel_td[6]
#define skel_int128_td      skel_td[7]
#define skel_int256_td      skel_td[8]
#define skel_term_td        skel_td[9]   /* To work around PIIX UHCI bug */

#define UHCI_MAX_SKELQHS    4
#define skel_ls_control_qh  skel_qh[0]
#define skel_hs_control_qh  skel_qh[1]
#define skel_bulk_qh        skel_qh[2]
#define skel_term_qh        skel_qh[3]



struct URB;

/*
 * for TD <status>:
 */
#define TD_CTRL_SPD     	(1 << 29)   /* Short Packet Detect */
#define TD_CTRL_C_ERR_MASK  (3 << 27)   /* Error Counter bits */
#define TD_CTRL_C_ERR_SHIFT 27
#define TD_CTRL_LS      	(1 << 26)   /* Low Speed Device */
#define TD_CTRL_IOS     	(1 << 25)   /* Isochronous Select */
#define TD_CTRL_IOC     	(1 << 24)   /* Interrupt on Complete */
#define TD_CTRL_ACTIVE      (1 << 23)   /* TD Active */
#define TD_CTRL_STALLED     (1 << 22)   /* TD Stalled */
#define TD_CTRL_DBUFERR     (1 << 21)   /* Data Buffer Error */
#define TD_CTRL_BABBLE      (1 << 20)   /* Babble Detected */
#define TD_CTRL_NAK     	(1 << 19)   /* NAK Received */
#define TD_CTRL_CRCTIMEO    (1 << 18)   /* CRC/Time Out Error */
#define TD_CTRL_BITSTUFF    (1 << 17)   /* Bit Stuff Error */
#define TD_CTRL_ACTLEN_MASK 0x7FF       /* actual length, encoded as n - 1 */

#define TD_CTRL_ANY_ERROR   (TD_CTRL_STALLED | TD_CTRL_DBUFERR | \
                 TD_CTRL_BABBLE | TD_CTRL_CRCTIMEO | TD_CTRL_BITSTUFF)

#define uhci_status_bits(ctrl_sts)  (ctrl_sts & 0xFE0000)
#define uhci_actual_length(ctrl_sts)    ((ctrl_sts + 1) & TD_CTRL_ACTLEN_MASK) /* 1-based */

/*
 * for TD <info>: (a.k.a. Token)
 */
#define TD_TOKEN_TOGGLE     19
#define TD_PID          	0xFF

#define uhci_maxlen(token)  ((token) >> 21)
#define uhci_expected_length(info) (((info >> 21) + 1) & TD_CTRL_ACTLEN_MASK) /* 1-based */
#define uhci_toggle(token)  (((token) >> TD_TOKEN_TOGGLE) & 1)
#define uhci_endpoint(token)    (((token) >> 15) & 0xf)
#define uhci_devaddr(token) (((token) >> 8) & 0x7f)
#define uhci_devep(token)   (((token) >> 8) & 0x7ff)
#define uhci_packetid(token)    ((token) & 0xff)
#define uhci_packetout(token)   (uhci_packetid(token) != USB_PID_IN)
#define uhci_packetin(token)    (uhci_packetid(token) == USB_PID_IN)

/*
 * The documentation says "4 dwords for hardware, 4 dwords for software".
 *
 * That's silly, the hardware doesn't care. The hardware only cares that
 * the hardware words are 16-byte aligned, and we can have any amount of
 * sw space after the TD entry as far as I can tell.
 *
 * But let's just go with the documentation, at least for 32-bit machines.
 * On 64-bit machines we probably want to take advantage of the fact that
 * hw doesn't really care about the size of the sw-only area.
 *
 * Alas, not anymore, we have more than 4 dwords for software, woops.
 * Everything still works tho, surprise! -jerdfelt
 */

#define UHCI_ITEM_FLAG_TYPE       0x3
#define UHCI_ITEM_FLAG_TD     	  0x0
#define UHCI_ITEM_FLAG_QH         0x1

#ifndef offsetof
#define offsetof( s, m ) ( ( ULONG )( &( ( s* )0 )->m  ) )
#endif

#define struct_ptr( meMBER_ptr, stRUCT_name, meMBER_name ) \
( ( stRUCT_name* ) ( ( (CHAR*)( meMBER_ptr ) ) - offsetof( stRUCT_name, meMBER_name ) ) )

#define dev_state( pdEV ) ( ( pdEV )->flags & USB_DEV_STATE_MASK )

#define dev_class( pdev ) ( pdev->flags & USB_DEV_CLASS_MASK )

#define uhci_from_hcd( hCD ) ( struct_ptr( ( hCD ), UHCI_DEV, hcd_interf ) )
#define uhci_from_dev( dEV ) ( struct_ptr( ( dEV->hcd ), UHCI_DEV, hcd_interf ) )

typedef struct _TD_EXTENSION
{
    LIST_ENTRY          vert_link;          // urb will use this to link qh and tds
    LIST_ENTRY          hori_link;
	ULONG 				flags;
    struct _UHCI_TD     *ptd;                //link to is companion td, constant since initialized

} TD_EXTENSION, *PTD_EXTENSION;

typedef struct _UHCI_TD
{
    /* Hardware fields */
    ULONG               link;
    ULONG               status;
    ULONG               info;
    ULONG               buffer;

    /* Software fields */
    ULONG                   phy_addr;
    PTD_EXTENSION           ptde;
    struct _URB             *purb;
    struct _UHCI_TD_POOL    *pool;              //pool this td belongs to

} UHCI_TD, *PUHCI_TD;

typedef struct _UHCI_TD_POOL
{

	LIST_ENTRY          pool_link;			//link to next and prev td pool
    PUHCI_TD            td_array[ sizeof( UHCI_TD ) * UHCI_MAX_POOL_TDS / PAGE_SIZE ];		//would be allcated in common buffer
	PHYSICAL_ADDRESS	logic_addr[ sizeof( UHCI_TD ) * UHCI_MAX_POOL_TDS / PAGE_SIZE ];	//logical addr of the array

    PTD_EXTENSION       tde_array;
    LIST_ENTRY          free_que;			//list of free tds
    LONG                free_count;			//free tds in this pool
    LONG                total_count;		//total count of the tds
    PADAPTER_OBJECT     padapter;

} UHCI_TD_POOL, *PUHCI_TD_POOL;

BOOLEAN
init_td_pool(
PUHCI_TD_POOL pool
);

BOOLEAN
free_td_to_pool(
PUHCI_TD_POOL pool,
PUHCI_TD ptd
); //add tds till pnext == NULL

PUHCI_TD
alloc_td_from_pool(
PUHCI_TD_POOL ptd_pool
);   //null if failed]

BOOLEAN
is_pool_free(
PUHCI_TD_POOL pool
);	//test whether the pool is all free

BOOLEAN
is_pool_full(
PUHCI_TD_POOL pool
);

BOOLEAN
destroy_td_pool(
PUHCI_TD_POOL pool
);

typedef struct _UHCI_TD_POOL_LIST
{
	LIST_ENTRY			busy_pools;
	KSPIN_LOCK     		pool_lock;
    LONG                free_tds;				//free tds in all the pool
	LONG                free_count;				//free pool count
	LIST_ENTRY          free_pools;
	UHCI_TD_POOL        pool_array[UHCI_MAX_TD_POOLS];			//max transfer is 640k

} UHCI_TD_POOL_LIST, *PUHCI_TD_POOL_LIST;

BOOLEAN
init_td_pool_list(
PUHCI_TD_POOL_LIST pool_list,
PADAPTER_OBJECT padapter
);

BOOLEAN
destroy_td_pool_list(
PUHCI_TD_POOL_LIST pool_list
);

BOOLEAN
expand_pool_list(
PUHCI_TD_POOL_LIST pool_list,
LONG pool_count
);      	//private

BOOLEAN
collect_garbage(
PUHCI_TD_POOL_LIST pool_list
);

LONG
get_free_tds(
PUHCI_TD_POOL_LIST pool_list
);			//private

LONG
get_max_free_tds(
PUHCI_TD_POOL_LIST pool_list
);		//private

BOOLEAN
can_transfer(
PUHCI_TD_POOL_LIST pool_list,
LONG td_count);

BOOLEAN
free_td(
PUHCI_TD_POOL_LIST pool_list,
PUHCI_TD ptd
); 	//add tds till pnext == NULL

PUHCI_TD
alloc_td(
PUHCI_TD_POOL_LIST pool_list
);  				//null if failed

VOID
lock_td_pool(
PUHCI_TD_POOL_LIST pool_list,
BOOLEAN at_dpc
);

VOID
unlock_td_pool(
PUHCI_TD_POOL_LIST pool_list,
BOOLEAN at_dpc
);

typedef struct _UHCI_QH
{
    /* Hardware fields */
    ULONG               link;           // Next queue
    ULONG               element;        // Queue element pointer

    /* Software fields */
    ULONG               phy_addr;       //constant since initialized
    struct _QH_EXTENSION *pqhe;

} UHCI_QH, *PUHCI_QH;

typedef struct _QH_EXTENSION
{

    LIST_ENTRY          vert_link;
    LIST_ENTRY          hori_link;
	ULONG 				flags;
    PUHCI_QH            pqh;            //constant since initialized
    struct _URB          *purb;

} QH_EXTENSION, *PQH_EXTENSION;

typedef struct _UHCI_QH_POOL
{

    PUHCI_QH            qh_array;
	PHYSICAL_ADDRESS	logic_addr;			//logical addr of the array

	PQH_EXTENSION       qhe_array;
    LIST_ENTRY          free_que;
    LONG                free_count;
    LONG                total_count;
    KSPIN_LOCK          pool_lock;
	PADAPTER_OBJECT     padapter;			//we need this garbage for allocation

} UHCI_QH_POOL, *PUHCI_QH_POOL;


BOOLEAN
init_qh_pool(
PUHCI_QH_POOL pool,
PADAPTER_OBJECT padapter
);

BOOLEAN
free_qh(
PUHCI_QH_POOL pool,
PUHCI_QH ptd
); //add qhs till pnext == NULL

PUHCI_QH
alloc_qh(
PUHCI_QH_POOL pool
);  //null if failed

BOOLEAN
destroy_qh_pool(
PUHCI_QH_POOL pool
);

VOID
lock_qh_pool(
PUHCI_QH_POOL pool,
BOOLEAN at_dpc
);

VOID
unlock_qh_pool(
PUHCI_QH_POOL pool,
BOOLEAN at_dpc
);

/*
 * Search tree for determining where <interval> fits in the
 * skelqh[] skeleton.
 *
 * An interrupt request should be placed into the slowest skelqh[]
 * which meets the interval/period/frequency requirement.
 * An interrupt request is allowed to be faster than <interval> but not slower.
 *
 * For a given <interval>, this function returns the appropriate/matching
 * skelqh[] index value.
 *
 * NOTE: For UHCI, we don't really need int256_qh since the maximum interval
 * is 255 ms.  However, we do need an int1_qh since 1 is a valid interval
 * and we should meet that frequency when requested to do so.
 * This will require some change(s) to the UHCI skeleton.
 */
#if 0
static int __interval_to_skel(int interval)
{
    if (interval < 16) {
        if (interval < 4) {
            if (interval < 2)
                return 0;   /* int1 for 0-1 ms */
            return 1;       /* int2 for 2-3 ms */
        }
        if (interval < 8)
            return 2;       /* int4 for 4-7 ms */
        return 3;           /* int8 for 8-15 ms */
    }
    if (interval < 64) {
        if (interval < 32)
            return 4;       /* int16 for 16-31 ms */
        return 5;           /* int32 for 32-63 ms */
    }
    if (interval < 128)
        return 6;           /* int64 for 64-127 ms */
    return 7;               /* int128 for 128-255 ms (Max.) */
}
#endif

#define USB_ENDP_FLAG_BUSY_MASK         0x0000ff00
#define USB_ENDP_FLAG_STAT_MASK         0xff
#define USB_ENDP_FLAG_STALL            	0x01
#define USB_ENDP_FLAG_DATATOGGLE        0x80000000
#define USB_ENDP_FLAG_DEFAULT_ENDP      0x40000000

#define usb_endp_busy_count( peNDP ) \
( ( ( peNDP )->flags & USB_ENDP_FLAG_BUSY_MASK ) >> 8 )

#define usb_endp_busy_count_inc( peNDP ) \
( peNDP->flags = ( ( ( ( usb_endp_busy_count( peNDP ) + 1 ) << 8 ) & USB_ENDP_FLAG_BUSY_MASK ) | \
( ( peNDP )->flags & ~USB_ENDP_FLAG_BUSY_MASK ) ) )

#define usb_endp_busy_count_dec( peNDP ) \
( peNDP->flags = ( ( ( ( usb_endp_busy_count( peNDP ) - 1 ) << 8 ) & USB_ENDP_FLAG_BUSY_MASK )| \
( ( peNDP )->flags & ~USB_ENDP_FLAG_BUSY_MASK ) ) )

typedef struct _USB_ENDPOINT
{
    ULONG						flags;			//toggle | busy-count | stall | default-endp; busy count( usually 1 when busy, may be greater if iso endp )
    LIST_ENTRY					urb_list;       //pending urb queue

	struct _USB_INTERFACE       *pusb_if;
    struct _USB_ENDPOINT_DESC   *pusb_endp_desc;

} USB_ENDPOINT, *PUSB_ENDPOINT;

#define MAX_ENDPS_PER_IF                10

typedef struct _USB_INTERFACE
{
    UCHAR               		endp_count;
    USB_ENDPOINT        		endp[MAX_ENDPS_PER_IF];


	struct _USB_CONFIGURATION  	*pusb_config;
    PUSB_INTERFACE_DESC 		pusb_if_desc;

	struct _USB_DRIVER  		*pif_drv;			//for hub_dev use
    PVOID               		if_ext;
    LONG               			if_ext_size;
	UCHAR						altif_count;
	LIST_ENTRY					altif_list;


} USB_INTERFACE, *PUSB_INTERFACE;

#define MAX_INTERFACES_PER_CONFIG          4
#define MAX_CONFIGS_PER_DEV				   4

typedef struct _USB_CONFIGURATION
{
        //only for active configuration
    struct _USB_CONFIGURATION_DESC  *pusb_config_desc;
    UCHAR               if_count;
    USB_INTERFACE       interf[MAX_INTERFACES_PER_CONFIG];
    struct _USB_DEV     *pusb_dev;

} USB_CONFIGURATION, *PUSB_CONFIGURATION;

#define USE_IRQL \
KIRQL _pending_endp_lock_old_irql=0, _pending_endp_list_lock_old_irql=0, _dev_lock_old_irql=0, old_irql=0;

#define USE_BASIC_IRQL \
KIRQL _pending_endp_list_lock_old_irql=0, _dev_lock_old_irql=0;

#define USE_NON_PENDING_IRQL \
KIRQL _dev_lock_old_irql=0, old_irql=0;

#define USE_BASIC_NON_PENDING_IRQL \
KIRQL _dev_lock_old_irql=0;


#define USB_DEV_STATE_MASK          ( 0xff << 8 )
#define USB_DEV_STATE_POWERED       ( 0x01 << 8 )
#define USB_DEV_STATE_RESET         ( 0x02 << 8 )
#define USB_DEV_STATE_ADDRESSED     ( 0x03 << 8 )
#define USB_DEV_STATE_FIRST_CONFIG	( 0x04 << 8 )
#define USB_DEV_STATE_RECONFIG		( 0x05 << 8 )
#define USB_DEV_STATE_CONFIGURED    ( 0x06 << 8 )
#define USB_DEV_STATE_SUSPENDED     ( 0x07 << 8 )
#define USB_DEV_STATE_BEFORE_ZOMB	( 0x08 << 8 )
#define USB_DEV_STATE_ZOMB          ( 0x09 << 8 )

#define USB_DEV_CLASS_MASK          ( 0xff << 16 )
#define USB_DEV_CLASS_HUB           ( USB_CLASS_HUB << 16 )
#define USB_DEV_CLASS_MASSSTOR      ( USB_CLASS_MASS_STORAGE << 16 )
#define USB_DEV_CLASS_ROOT_HUB      ( ( USB_CLASS_VENDOR_SPEC - 2 ) << 16 )
#define USB_DEV_CLASS_SCANNER		( ( USB_DEV_CLASS_ROOT_HUB - 1 ) << 16 )

#define USB_DEV_FLAG_HIGH_SPEED		0x20	// high speed dev for usb2.0
#define USB_DEV_FLAG_LOW_SPEED      0x40  	// note: this bit is shared in urb->pipe
#define USB_DEV_FLAG_IF_DEV			0x01    // this dev is a virtual dev, that is a interface

#define lock_dev( pdev, at_dpc ) \
{\
	KIRQL cur_irql;\
	cur_irql = KeGetCurrentIrql();\
	if( cur_irql == DISPATCH_LEVEL )\
	{\
		KeAcquireSpinLockAtDpcLevel( &pdev->dev_lock );\
		_dev_lock_old_irql = DISPATCH_LEVEL;\
	}\
	else if( cur_irql < DISPATCH_LEVEL )\
		KeAcquireSpinLock( &pdev->dev_lock, &_dev_lock_old_irql );\
	else\
		TRAP();\
}

#define unlock_dev( pdev, from_dpc ) \
{\
	if( _dev_lock_old_irql == DISPATCH_LEVEL )\
		KeReleaseSpinLockFromDpcLevel( &pdev->dev_lock );\
	else if( _dev_lock_old_irql < DISPATCH_LEVEL )\
		KeReleaseSpinLock( &pdev->dev_lock, _dev_lock_old_irql );\
	else\
		TRAP();\
}

typedef struct _USB_DEV
{
	LIST_ENTRY          dev_link;         	//for dev-list

    KSPIN_LOCK          dev_lock;
    PDEVICE_OBJECT      dev_obj;
    ULONG               flags;          	//class | cur_state | low speed
    LONG               	ref_count;       	//client count

    UCHAR               dev_addr;       	//usb addr
    ULONG               dev_id;         	//will be used to compose dev handle

    struct _USB_DEV     *parent_dev;
	UCHAR 				port_idx;			//parent hub's port idx, to which the dev attached

	struct _HCD			*hcd;				//point to the hcd the dev belongs to

	USB_ENDPOINT       	default_endp;  		//control endp. its interfac pointer is to the first interface

    LONG               	desc_buf_size;
    PUCHAR              desc_buf;
	struct _USB_DEVICE_DESC *pusb_dev_desc;

    UCHAR               active_config_idx;
    PUSB_CONFIGURATION  usb_config;			//the active configuration

	struct _USB_DRIVER   *dev_driver;
	PVOID				dev_ext;
	LONG				dev_ext_size;

	LONG 				time_out_count;		//default pipe error counter, three time-outs will cause the dev not function
	LONG               	error_count;        //usb transfer error counter for statics only

} USB_DEV, *PUSB_DEV;

// pending endpoint pool definitions

#define  UHCI_MAX_PENDING_ENDPS   32

typedef struct _UHCI_PENDING_ENDP
{
	LIST_ENTRY   		endp_link;
	PUSB_ENDPOINT      	pendp;

} UHCI_PENDING_ENDP, *PUHCI_PENDING_ENDP;

typedef struct _UHCI_PENDING_ENDP_POOL
{
    PUHCI_PENDING_ENDP  pending_endp_array;
    LIST_ENTRY          free_que;
    LONG                free_count;
    LONG                total_count;
    KSPIN_LOCK          pool_lock;

} UHCI_PENDING_ENDP_POOL, *PUHCI_PENDING_ENDP_POOL;

BOOLEAN
init_pending_endp_pool(
PUHCI_PENDING_ENDP_POOL pool
);

BOOLEAN
free_pending_endp(
PUHCI_PENDING_ENDP_POOL pool,
PUHCI_PENDING_ENDP pending_endp
);

PUHCI_PENDING_ENDP
alloc_pending_endp(
PUHCI_PENDING_ENDP_POOL pool,
LONG count
);

BOOLEAN
destroy_pending_endp_pool(
PUHCI_PENDING_ENDP_POOL pool
);

// pool type is PUHCI_PENDING_ENDP_POOL
#define lock_pending_endp_pool( pool ) \
{\
    KeAcquireSpinLock( &pool->pool_lock, &_pending_endp_lock_old_irql );\
}

#define unlock_pending_endp_pool( pool ) \
{\
	KeReleaseSpinLock( &pool->pool_lock, &_pending_endp_lock_old_irql );\
}


// end of pending endpoint pool
typedef struct _FRAME_LIST_CPU_ENTRY
{
	LIST_ENTRY td_link;

} FRAME_LIST_CPU_ENTRY, *PFRAME_LIST_CPU_ENTRY;

#define uhci_public_res_lock  	pending_endp_list_lock
#define uhci_status( _uhci_ )	( READ_PORT_USHORT( ( PUSHORT )( ( _uhci_ )->port_base + USBSTS ) ) )

typedef struct _UHCI
{
	PHYSICAL_ADDRESS   	uhci_reg_base;					// io space
	BOOLEAN				port_mapped;
	PBYTE				port_base;

	PHYSICAL_ADDRESS	io_buf_logic_addr;
	PBYTE				io_buf;

	PHYSICAL_ADDRESS	frame_list_logic_addr;

	KSPIN_LOCK          frame_list_lock;    			//run at DIRQL
    PULONG              frame_list;
	PFRAME_LIST_CPU_ENTRY	frame_list_cpu;
    LIST_HEAD           urb_list;                   	//active urb-list
    PUHCI_TD            skel_td[UHCI_MAX_SKELTDS];
    PUHCI_QH            skel_qh[UHCI_MAX_SKELQHS];  	//skeltons



	UHCI_TD_POOL_LIST   td_pool;
	UHCI_QH_POOL 		qh_pool;


    //for iso and int bandwidth claim, bandwidth schedule
	KSPIN_LOCK 			pending_endp_list_lock;			//lock to access the following two
	LIST_HEAD 			pending_endp_list;
	UHCI_PENDING_ENDP_POOL  pending_endp_pool;
	PLONG 	            frame_bw;
    LONG               	fsbr_cnt;           			//used to record number of fsbr users

	KTIMER				reset_timer;					//used to reset the host controller

	//struct _USB_DEV_MANAGER		dev_mgr;			//it is in hcd_interf
	struct _DEVICE_EXTENSION    *pdev_ext;

    PUSB_DEV            root_hub;						//root hub
	HCD					hcd_interf;

} UHCI_DEV, *PUHCI_DEV;

#define lock_pending_endp_list( list_lock ) \
{\
	KeAcquireSpinLock( list_lock, &_pending_endp_list_lock_old_irql );\
}

#define unlock_pending_endp_list( list_lock ) \
{\
	KeReleaseSpinLock( list_lock, _pending_endp_list_lock_old_irql );\
}
typedef struct _UHCI_INTERRUPT
{
	ULONG 				level;
	ULONG 				vector;
	ULONG 				affinity;

} UHCI_INTERRUPT, *PUHCI_INTERRUPT;

typedef struct _UHCI_PORT
{
	PHYSICAL_ADDRESS 	Start;
	ULONG 				Length;

} UHCI_PORT, *PUHCI_PORT;

typedef NTSTATUS ( *PDISPATCH_ROUTINE )( PDEVICE_OBJECT dev_obj, PIRP irp );

#define NTDEV_TYPE_HCD			1
#define NTDEV_TYPE_CLIENT_DEV	2

typedef struct _DEVEXT_HEADER
{
	ULONG					type;
	PDISPATCH_ROUTINE 		dispatch;
	PDRIVER_STARTIO			start_io;

	struct _USB_DEV_MANAGER *dev_mgr; //mainly for use by cancel irp

} DEVEXT_HEADER, *PDEVEXT_HEADER;

typedef struct _DEVICE_EXTENSION
{
	//struct _USB_DEV_MANAGER 	*pdev_mgr;
	DEVEXT_HEADER		dev_ext_hdr;
	PDEVICE_OBJECT     	pdev_obj;
	PDRIVER_OBJECT  	pdrvr_obj;
	PUHCI_DEV 			uhci;

	//device resources
    PADAPTER_OBJECT     padapter;
	ULONG 				map_regs;
	PCM_RESOURCE_LIST 	res_list;
    ULONG               pci_addr;	// bus number | slot number | funciton number
	UHCI_INTERRUPT   	res_interrupt;
	UHCI_PORT 			res_port;

	PKINTERRUPT			uhci_int;
	KDPC   				uhci_dpc;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//helper macro
#define ListFirst( heAD, firST) \
{\
    if( IsListEmpty( ( heAD ) ) )\
        firST = NULL;\
    else\
	    firST = ( heAD )->Flink;\
}

#define ListNext( heAD, curreNT, neXT) \
{\
	if( IsListEmpty( ( heAD ) ) == FALSE )\
	{\
		neXT = (curreNT)->Flink;\
		if( neXT == heAD )\
			neXT = NULL;\
	}\
	else\
		neXT = NULL;\
}

#define ListPrev( heAD, curreNT, prEV) \
{\
	if( IsListEmpty( ( heAD ) ) == FALSE )\
	{\
		prEV = (curreNT)->Blink;\
		if( prEV == heAD )\
			prEV = NULL;\
	else\
		prEV = NULL;\
}

#define ListFirstPrev( heAD, firST) \
{\
    if( IsListEmpty( ( heAD ) ) )\
        firST = NULL;\
    else\
	    firST = ( heAD )->Blink;\
}

#define MergeList( liST1, liST2 )\
{\
	PLIST_ENTRY taIL1, taIL2;\
	if( IsListEmpty( liST2 ) == TRUE )\
	{\
		InsertTailList( liST1, liST2 );\
	}\
	else if( IsListEmpty( liST1 ) == TRUE )\
	{\
		InsertTailList( liST2, liST1 );\
	}\
	else\
	{\
		ListFirstPrev( liST1, taIL1 );\
		ListFirstPrev( liST2, taIL2 );\
\
		taIL1->Flink = ( liST2 );\
		( liST2 )->Blink = taIL1;\
\
		taIL2->Flink = ( liST1 );\
		( liST1 )->Blink = taIL2;\
	}\
}

PUHCI_TD
alloc_tds(
PUHCI_TD_POOL_LIST pool_list,
LONG count
);

VOID
free_tds(
PUHCI_TD_POOL_LIST pool_list,
PUHCI_TD  ptd
);

BOOLEAN
uhci_init(
PUHCI_DEV uhci,
PADAPTER_OBJECT padapter
);

BOOLEAN
uhci_destroy(
PUHCI_DEV uhci
);

// funcitons exported to dev-manager
BOOLEAN
uhci_add_device(
PUHCI_DEV uhci,
PUSB_DEV dev
);

BOOLEAN
uhci_remove_device(
PUHCI_DEV uhci,
PUSB_DEV dev
);

//helpers
VOID NTAPI
uhci_dpc_callback(
PKDPC dpc,
PVOID context,
PVOID sysarg1,
PVOID sysarg2
);

#if 0
static VOID
uhci_flush_adapter_buf()
{
#ifdef _X86
	__asm invd;
#endif
}
#endif

NTSTATUS
uhci_submit_urb(
PUHCI_DEV uhci,
PUSB_DEV pdev,
PUSB_ENDPOINT pendp,
struct _URB *urb
);

//must have dev_lock acquired
NTSTATUS
uhci_internal_submit_bulk(
PUHCI_DEV uhci,
struct _URB *urb
);

NTSTATUS
uhci_internal_submit_iso(
PUHCI_DEV uhci,
struct _URB *urb
);

NTSTATUS
uhci_internal_submit_ctrl(
PUHCI_DEV uhci,
struct _URB *urb
);

NTSTATUS
uhci_internal_submit_int(
PUHCI_DEV uhci,
struct _URB *urb
);

BOOLEAN
uhci_remove_bulk_from_schedule(
PUHCI_DEV uhci,
struct _URB *urb
);

#define uhci_remove_ctrl_from_schedule uhci_remove_bulk_from_schedule

BOOLEAN
uhci_remove_iso_from_schedule(
PUHCI_DEV uhci,
struct _URB *urb
);

BOOLEAN
uhci_remove_int_from_schedule(
PUHCI_DEV uhci,
struct _URB *urb
);

BOOLEAN
uhci_remove_urb_from_schedule(
PUHCI_DEV uhci,
struct _URB *urb
);

BOOLEAN
uhci_is_xfer_finished(  //will set urb error code here
struct _URB *urb
);

NTSTATUS
uhci_set_error_code(
struct _URB *urb,
ULONG raw_status
);

BOOLEAN
uhci_insert_tds_qh(
PUHCI_QH pqh,
PUHCI_TD td_chain
);

BOOLEAN
uhci_insert_qh_urb(
struct _URB *urb,
PUHCI_QH qh_chain
);

BOOLEAN
uhci_insert_urb_schedule(
PUHCI_DEV uhci,
struct _URB *urb
);

BOOLEAN
uhci_claim_bandwidth(
PUHCI_DEV uhci,
struct _URB *urb,
BOOLEAN claim_bw
);

BOOLEAN
uhci_process_pending_endp(
PUHCI_DEV uhci
);

NTSTATUS
uhci_cancel_urb(
PUHCI_DEV uhci,
PUSB_DEV pdev,
PUSB_ENDPOINT endp,
struct _URB *urb
);

VOID
uhci_generic_urb_completion(
struct _URB *urb,
PVOID context
);

// the following are NT driver definitions

// NT device name
#define UHCI_DEVICE_NAME "\\Device\\UHCI"

// File system device name.   When you execute a CreateFile call to open the
// device, use "\\.\GpdDev", or, given C's conversion of \\ to \, use
// "\\\\.\\GpdDev"

#define DOS_DEVICE_NAME "\\DosDevices\\UHCI"


#define CLR_RH_PORTSTAT( port_idx, x ) \
{\
	PUSHORT addr; \
	addr = ( PUSHORT )( uhci->port_base + port_idx ); \
	status = READ_PORT_USHORT( addr ); \
	status = ( status & 0xfff5 ) & ~( x ); \
	WRITE_PORT_USHORT( addr, ( USHORT )status ); \
}

#define SET_RH_PORTSTAT( port_idx, x ) \
{\
	PUSHORT addr; \
	addr = ( PUSHORT )( uhci->port_base + port_idx ); \
	status = READ_PORT_USHORT( addr ); \
	status = ( status & 0xfff5 ) | ( x ); \
	WRITE_PORT_USHORT( addr, ( USHORT )status ); \
}

//this is for dispatch routine
#define EXIT_DISPATCH( nTstatUs, iRp)\
{\
    if( nTstatUs != STATUS_PENDING)\
    {\
        iRp->IoStatus.Status = nTstatUs;\
		IoCompleteRequest( iRp, IO_NO_INCREMENT);\
		return nTstatUs;\
    }\
    IoMarkIrpPending( iRp);\
    return nTstatUs;\
}

#endif

