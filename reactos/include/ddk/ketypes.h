/* KERNEL TYPES **************************************************************/

#ifndef __INCLUDE_DDK_KETYPES_H
#define __INCLUDE_DDK_KETYPES_H

#define MB_FLAGS_MEM_INFO         (0x1)
#define MB_FLAGS_BOOT_DEVICE      (0x2)
#define MB_FLAGS_COMMAND_LINE     (0x4)
#define MB_FLAGS_MODULE_INFO      (0x8)
#define MB_FLAGS_AOUT_SYMS        (0x10)
#define MB_FLAGS_ELF_SYMS         (0x20)
#define MB_FLAGS_MMAP_INFO        (0x40)
#define MB_FLAGS_DRIVES_INFO      (0x80)
#define MB_FLAGS_CONFIG_TABLE     (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME (0x200)
#define MB_FLAGS_APM_TABLE        (0x400)
#define MB_FLAGS_GRAPHICS_TABLE   (0x800)

typedef struct _LOADER_MODULE
{
   ULONG ModStart;
   ULONG ModEnd;
   ULONG String;
   ULONG Reserved;
} LOADER_MODULE, *PLOADER_MODULE;

typedef struct _LOADER_PARAMETER_BLOCK
{
   ULONG Flags;
   ULONG MemLower;
   ULONG MemHigher;
   ULONG BootDevice;
   ULONG CommandLine;
   ULONG ModsCount;
   ULONG ModsAddr;
   UCHAR Syms[12];
   ULONG MmapLength;
   ULONG MmapAddr;
   ULONG DrivesCount;
   ULONG DrivesAddr;
   ULONG ConfigTable;
   ULONG BootLoaderName;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

#ifdef __NTOSKRNL__
extern CHAR EXPORTED KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
#else
extern CHAR IMPORTED KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK IMPORTED KeLoaderBlock;
#endif


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

struct _DISPATCHER_HEADER;

typedef struct _KWAIT_BLOCK
/*
 * PURPOSE: Object describing the wait a thread is currently performing
 */
{
   LIST_ENTRY WaitListEntry;
   struct _KTHREAD* Thread;
   struct _DISPATCHER_HEADER *Object;
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
} __attribute__((packed)) DISPATCHER_HEADER;


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
   USHORT Inserted;
} __attribute__((packed)) KAPC, *PKAPC;

typedef struct _KBUGCHECK_CALLBACK_RECORD
{
   LIST_ENTRY Entry;
   PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
   PVOID Buffer;
   ULONG Length;
   PUCHAR Component;
   ULONG Checksum;
   UCHAR State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;

typedef struct _KMUTEX
{
   DISPATCHER_HEADER Header;
   LIST_ENTRY MutantListEntry;
   struct _KTHREAD* OwnerThread;
   BOOLEAN Abandoned;
   UCHAR ApcDisable;
} KMUTEX, *PKMUTEX, KMUTANT, *PKMUTANT;

typedef struct _KSEMAPHORE
{
   DISPATCHER_HEADER Header;
   LONG Limit;
} __attribute__((packed)) KSEMAPHORE, *PKSEMAPHORE;

typedef struct _KEVENT
{
   DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT;

typedef struct _KEVENT_PAIR
{
   CSHORT Type;
   CSHORT Size;
   KEVENT LowEvent;
   KEVENT HighEvent;
} KEVENT_PAIR, *PKEVENT_PAIR;


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

/*
 * PURPOSE: Defines a delayed procedure call object
 */
typedef struct _KDPC
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
} __attribute__((packed)) KDPC, *PKDPC;



typedef struct _KDEVICE_QUEUE_ENTRY
{
   LIST_ENTRY Entry;
   ULONG Key;
   BOOLEAN Inserted;
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

typedef struct _KSYSTEM_TIME
{
   ULONG LowPart;
   LONG High1Part;
   LONG High2Part;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct _EPROCESS EPROCESS, *PEPROCESS;

#endif /* __INCLUDE_DDK_KETYPES_H */
