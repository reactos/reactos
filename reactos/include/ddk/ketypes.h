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

typedef struct _KWAIT_BLOCK
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
   UCHAR      Type;
   UCHAR      Absolute;
   UCHAR      Size;
   UCHAR      Inserted;
   LONG       SignalState;
   LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;


typedef struct _KQUEUE
{
   DISPATCHER_HEADER Header;
   LIST_ENTRY        EntryListHead;
   ULONG             CurrentCount;
   ULONG             MaximumCount;
   LIST_ENTRY        ThreadListEntry;
} KQUEUE, *PKQUEUE;

struct _KDPC;

typedef struct _KTIMER
 {
    DISPATCHER_HEADER Header;
    ULARGE_INTEGER DueTime;
    LIST_ENTRY TimerListEntry;
    struct _KDPC* Dpc;
    LONG Period;
 } KTIMER, *PKTIMER;

/*
typedef struct _KTIMER
{
   LIST_ENTRY entry;
   signed long long expire_time;
   struct _KDPC* dpc;
   BOOLEAN signaled;
   BOOLEAN running;
   TIMER_TYPE type;
   ULONG period;   
} KTIMER, *PKTIMER;
*/
 
struct _KSPIN_LOCK;

typedef struct _KSPIN_LOCK
{
   ULONG Lock;
} KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KDEVICE_QUEUE
{
   LIST_ENTRY ListHead;
   BOOLEAN Busy;
   KSPIN_LOCK Lock;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;

	      
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
} KMUTEX, *PKMUTEX, KMUTANT, *PKMUTANT;
   
typedef struct
{
   DISPATCHER_HEADER Header;
   LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE; 

typedef struct _KEVENT
{
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
   SHORT Type;
   UCHAR Number;
   UCHAR Importance;   
   LIST_ENTRY DpcListEntry; 
   PKDEFERRED_ROUTINE DeferredRoutine;
   PVOID DeferredContext;
   PVOID SystemArgument1;
   PVOID SystemArgument2;
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
