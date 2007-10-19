#ifndef __EVENTS_H
#define __EVENTS_H

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

BOOLEAN
init_event_pool(
PUSB_EVENT_POOL pool
);

BOOLEAN
free_event(
PUSB_EVENT_POOL pool,
PUSB_EVENT pevent
); //add qhs till pnext == NULL

PUSB_EVENT
alloc_event(
PUSB_EVENT_POOL pool,
LONG count
);  //null if failed

BOOLEAN
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

BOOLEAN
init_timer_svc_pool(
PTIMER_SVC_POOL pool
);
BOOLEAN
free_timer_svc(
PTIMER_SVC_POOL pool,
PTIMER_SVC ptimer
);

PTIMER_SVC
alloc_timer_svc(
PTIMER_SVC_POOL pool,
LONG count
);  //null if failed

BOOLEAN
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

#endif
