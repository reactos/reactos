/* KERNEL TYPES **************************************************************/

#ifndef __INCLUDE_DDK_KETYPES_H
#define __INCLUDE_DDK_KETYPES_H

struct _KMUTANT;

typedef LONG KPRIORITY;
   
typedef VOID (*PKBUGCHECK_CALLBACK_ROUTINE)(PVOID Buffer, ULONG Length);
typedef BOOLEAN (*PKSYNCHRONIZE_ROUTINE)(PVOID SynchronizeContext);

struct _KAPC;

typedef VOID (*PKNORMAL_ROUTINE)(PVOID NormalContext,
				 PVOID SystemArgument1,
				 PVOID SystemArgument2);
typedef VOID (*PKKERNEL_ROUTINE)(struct _KAPC* Apc,
				PKNORMAL_ROUTINE* NormalRoutine,
				PVOID* NormalContext,
				PVOID* SystemArgument1,
				PVOID* SystemArgument2);
	      
typedef VOID (*PKRUNDOWN_ROUTINE)(struct _KAPC* Apc);	      

typedef struct
/*
 * PURPOSE: Object describing the wait a thread is currently performing
 */
{
   LIST_ENTRY WaitListEntry;
   struct _KTHREAD* Thread;
   PVOID Object;
   struct _KWAIT_BLOCK* NextWaitBlock;
   USHORT WaitKey;
   USHORT WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK;

typedef struct _DISPATCHER_HEADER
{
   UCHAR Type;
   UCHAR Absolute;
   UCHAR Size;
   UCHAR Inserted;
   LONG SignalState;
   LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

struct _KDPC;

typedef struct _KTIMER
{
   /*
    * Pointers to maintain the linked list of activated timers
    */
   LIST_ENTRY entry;
   
   /*
    * Absolute expiration time in system time units
    */
   signed long long expire_time;

   /*
    * Optional dpc associated with the timer 
    */
   struct _KDPC* dpc;
   
   /*
    * True if the timer is signaled
    */
   BOOLEAN signaled;
   
   /*
    * True if the timer is in the system timer queue
    */
   BOOLEAN running;
   
   /*
    * Type of the timer either Notification or Synchronization
    */
   TIMER_TYPE type;
   
   /*
    * Period of the timer in milliseconds (zero if once-only)
    */
   ULONG period;
   
} KTIMER, *PKTIMER;

struct _KSPIN_LOCK;

typedef struct _KSPIN_LOCK
{
   KIRQL irql;
} KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KDEVICE_QUEUE
{
   LIST_ENTRY ListHead;
   BOOLEAN Busy;
   KSPIN_LOCK Lock;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

#if RIGHT_DEFINITION_PROVIDED_ABOVE
#define _KTHREAD _ETHREAD

typedef struct _KTHREAD
/*
 * PURPOSE: Describes a thread of execution
 */
{
   CSHORT Type;
   CSHORT Size;
 
   /*
    * PURPOSE: Head of the queue of apcs
    */
   LIST_ENTRY ApcQueueHead;
   
   /*
    * PURPOSE: Entry in the linked list of threads
    */
   LIST_ENTRY Entry;
   
   /*
    * PURPOSE: Current state of the thread
    */
   ULONG State;
   
   /*
    * PURPOSE: Priority modifier of the thread
    */
   ULONG Priority;
   
   /*
    * PURPOSE: Pointer to our process
    */
   struct _EPROCESS* Process;
   
   /*
    * PURPOSE: Handle of our process
    */
   HANDLE ProcessHandle;
   
   /*
    * PURPOSE: Thread affinity mask
    */
   ULONG AffinityMask;
   
   /*
    * PURPOSE: Saved thread context
    */
   hal_thread_state context;
   
   /*
    * PURPOSE: Timeout for the thread to be woken up
    */
   signed long long int wake_time;
   
} KTHREAD, *PKTHREAD, *PETHREAD;
#endif

	      
typedef struct _KAPC
{
   CSHORT Type;
   CSHORT Size;
   ULONG Spare0;
   struct _KTHREAD* Thread;
   LIST_ENTRY ApcListEntry;
   PKKERNEL_ROUTINE KernelRoutine;
   PKRUNDOWN_ROUTINE RundownRoutine;
   PKNORMAL_ROUTINE NormalRoutine;
   PVOID NormalContext;
   PVOID SystemArgument1;
   PVOID SystemArgument2;
   CCHAR ApcStateIndex;
   KPROCESSOR_MODE ApcMode;
   BOOLEAN Inserted;
} KAPC, *PKAPC;

typedef struct
{
   LIST_ENTRY Entry;
   PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
   PVOID Buffer;
   ULONG Length;
   PUCHAR Component;
   ULONG Checksum;
   UCHAR State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;
   
typedef struct
{
   DISPATCHER_HEADER Header;
   LIST_ENTRY MutantListEntry;
   struct _KTHREAD* OwnerThread;
   BOOLEAN Abandoned;
   UCHAR ApcDisable;
} KMUTEX, *PKMUTEX;
   
typedef struct
{
   DISPATCHER_HEADER Header;
   LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE; 

typedef struct _KEVENT
/*
 * PURPOSE: Describes an event
 */
{
   /*
    * PURPOSE: So we can use the general wait routine
    */
   DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT;



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



typedef struct _KDEVICE_QUEUE_ENTRY
{
   LIST_ENTRY Entry;
   ULONG Key;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

typedef struct _WAIT_CONTEXT_BLOCK
{
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

struct _KINTERRUPT;

typedef BOOLEAN (*PKSERVICE_ROUTINE)(struct _KINTERRUPT* Interrupt, 
			     PVOID ServiceContext);

typedef struct _KINTERRUPT
{
   ULONG Vector;
   KAFFINITY ProcessorEnableMask;
   PKSPIN_LOCK IrqLock;
   BOOLEAN Shareable;
   BOOLEAN FloatingSave;
   PKSERVICE_ROUTINE ServiceRoutine;
   PVOID ServiceContext;
   LIST_ENTRY Entry;
   KIRQL SynchLevel;
} KINTERRUPT, *PKINTERRUPT;

#endif /* __INCLUDE_DDK_KETYPES_H */
