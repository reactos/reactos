typedef LONG KPRIORITY;
   
typedef VOID (*PKBUGCHECK_CALLBACK_ROUTINE)(PVOID Buffer, ULONG Length);
typedef BOOLEAN (*PKSYNCHRONIZE_ROUTINE)(PVOID SynchronizeContext);
   
typedef struct
{
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;
   
typedef struct
{
} KMUTEX, *PKMUTEX;
   
typedef struct
{
} KSEMAPHORE, *PKSEMAPHORE; 

typedef struct
{
} KTHREAD, *PKTHREAD;


/*
 * PURPOSE: Included in every object that a thread can wait on
 */
typedef struct 
{
   /*
    * PURPOSE: True if the object is signaling a change of state
    */
   BOOLEAN signaled;
   
   /*
    * PURPOSE: Head of the queue of threads waiting on this object
    */
   LIST_ENTRY wait_queue_head;
   
   /*
    * PURPOSE: True if all the threads waiting should be woken when the
    * object changes state
    */
   BOOLEAN wake_all;
   
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;


/*
 * PURPOSE: Describes an event
 */
typedef struct _KEVENT
{
   /*
    * PURPOSE: So we can use the general wait routine
    */
   DISPATCHER_HEADER hdr;
   
   /*
    * PURPOSE: Type of event, notification or synchronization
    */
   EVENT_TYPE type;
} KEVENT, *PKEVENT;


typedef struct _KSPIN_LOCK
{
   KIRQL irql;
} KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct
{
} KAPC;

typedef struct
{
} UNICODE_STRING;

typedef VOID (*PDRIVER_ADD_DEVICE)(VOID);

struct _KDPC;

/*
 * PURPOSE: Defines a delayed procedure call routine
 * NOTE:
 *      Dpc = The associated DPC object
 *      DeferredContext = Driver defined context for the DPC
 *      SystemArgument[1-2] = Undocumented. 
 * 
 */
typedef VOID (*PKDEFERRED_ROUTINE)(struct _KDPC* Dpc, PVOID DeferredContext, 
			   PVOID SystemArgument1, PVOID SystemArgument2);

typedef struct _KDPC
/*
 * PURPOSE: Defines a delayed procedure call object
 */
{
   /*
    * PURPOSE: Magic value to check this is the current object type
    */
   SHORT Type;
   
   /*
    * PURPOSE: Target processor or zero if untargetted
    */
   UCHAR Number;
   
   /*
    * PURPOSE: Indication of desired latency before exection
    */
   UCHAR Importance;
   
   LIST_ENTRY DpcListEntry; 
   PKDEFERRED_ROUTINE DeferredRoutine;
   PVOID DeferredContext;
   PVOID SystemArgument1;
   PVOID SystemArgument2;
   
   /*
    * PURPOSE: If non-zero then already in queue
    */
   PULONG Lock;
   
} KDPC, *PKDPC;


typedef struct _KDEVICE_QUEUE
{
   LIST_ENTRY ListHead;
   BOOLEAN Busy;
   KSPIN_LOCK Lock;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

typedef struct _KDEVICE_QUEUE_ENTRY
{
   LIST_ENTRY Entry;
   ULONG Key;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

typedef struct _WAIT_CONTEXT_BLOCK
{
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;
