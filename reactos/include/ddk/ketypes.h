/* KERNEL TYPES **************************************************************/

#ifndef __INCLUDE_DDK_KETYPES_H
#define __INCLUDE_DDK_KETYPES_H

/* include ntos/ketypes.h here? */

#include <arc/arc.h>

# define RESTRICTED_POINTER __restrict

struct _KMUTANT;

typedef LONG KPRIORITY;

typedef LONG FLONG;


typedef VOID STDCALL_FUNC
(*PKBUGCHECK_CALLBACK_ROUTINE)(PVOID Buffer, ULONG Length);

typedef BOOLEAN STDCALL_FUNC
(*PKSYNCHRONIZE_ROUTINE)(PVOID SynchronizeContext);

struct _KAPC;

typedef VOID STDCALL_FUNC
(*PKNORMAL_ROUTINE)(PVOID NormalContext,
		    PVOID SystemArgument1,
		    PVOID SystemArgument2);

typedef VOID STDCALL_FUNC
(*PKKERNEL_ROUTINE)(struct _KAPC* Apc,
		    PKNORMAL_ROUTINE* NormalRoutine,
		    PVOID* NormalContext,
		    PVOID* SystemArgument1,
		    PVOID* SystemArgument2);

typedef VOID STDCALL_FUNC
(*PKRUNDOWN_ROUTINE)(struct _KAPC* Apc);

typedef enum _MODE 
{
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

#include <pshpack1.h>

typedef struct _DISPATCHER_HEADER 
{
    union {
        struct {
            UCHAR Type;
            UCHAR Absolute;
            UCHAR Size;
            union {
                UCHAR Inserted;
                BOOLEAN DebugActive;
            };
        };
        volatile LONG Lock;
    };
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

#include <poppack.h>

typedef struct _KQUEUE 
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY EntryListHead;
    ULONG CurrentCount;
    ULONG MaximumCount;
    LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;

typedef struct _KGATE
{
    DISPATCHER_HEADER Header;
} KGATE, *PKGATE, *RESTRICTED_POINTER PRKGATE;

struct _KDPC;

typedef struct _KTIMER 
{
    DISPATCHER_HEADER Header;
    ULARGE_INTEGER DueTime;
    LIST_ENTRY TimerListEntry;
    struct _KDPC *Dpc;
    LONG Period;
} KTIMER, *PKTIMER, *RESTRICTED_POINTER PRKTIMER;

typedef struct _KMUTANT 
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListEntry;
    struct _KTHREAD *RESTRICTED_POINTER OwnerThread;
    BOOLEAN Abandoned;
    UCHAR ApcDisable;
} KMUTANT, *PKMUTANT, *RESTRICTED_POINTER PRKMUTANT, KMUTEX, *PKMUTEX, *RESTRICTED_POINTER PRKMUTEX;

typedef struct _KGUARDED_MUTEX
{
    LONG Count;
    struct _KTHREAD* Owner;
    ULONG Contention;
    KGATE Gate;
    union {
        struct {
            SHORT KernelApcDisable;
            SHORT SpecialApcDisable;
        };
        ULONG CombinedApcDisable;
    };
} KGUARDED_MUTEX, *PKGUARDED_MUTEX;

typedef struct _KSEMAPHORE
{
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

typedef struct _KEVENT
{
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KDEVICE_QUEUE 
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    KSPIN_LOCK Lock;
    BOOLEAN Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE, *RESTRICTED_POINTER PRKDEVICE_QUEUE;

typedef struct _KDEVICE_QUEUE_ENTRY 
{
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY, *RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

/*
 * Size of the profile hash table.
 */
#define PROFILE_HASH_TABLE_SIZE      (32)

#include <pshpack2.h>

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
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;
#include <poppack.h>

#ifndef __USE_W32API

typedef struct _KAPC_STATE 
{
    LIST_ENTRY ApcListHead[MaximumMode];
    struct _KPROCESS *Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *RESTRICTED_POINTER PRKAPC_STATE;

#endif /* __USE_W32API */

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

struct _KDPC;

typedef struct _KSPIN_LOCK_QUEUE {
    struct _KSPIN_LOCK_QUEUE * volatile Next;
    PKSPIN_LOCK volatile Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
    KSPIN_LOCK_QUEUE LockQueue;
    KIRQL OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

typedef struct _KWAIT_BLOCK 
{
    LIST_ENTRY WaitListEntry;
    struct _KTHREAD *RESTRICTED_POINTER Thread;
    PVOID Object;
    struct _KWAIT_BLOCK *RESTRICTED_POINTER NextWaitBlock;
    USHORT WaitKey;
    USHORT WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK, *RESTRICTED_POINTER PRKWAIT_BLOCK;

/*
 * PURPOSE: Defines a delayed procedure call routine
 * NOTE:
 *      Dpc = The associated DPC object
 *      DeferredContext = Driver defined context for the DPC
 *      SystemArgument[1-2] = Undocumented. 
 * 
 */
typedef VOID STDCALL_FUNC
(*PKDEFERRED_ROUTINE)(struct _KDPC* Dpc,
		      PVOID DeferredContext,
		      PVOID SystemArgument1,
		      PVOID SystemArgument2);

#define DPC_NORMAL 0
#define DPC_THREADED 1
/*
 * PURPOSE: Defines a delayed procedure call object
 */
typedef struct _KDPC
{
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    LIST_ENTRY DpcListEntry;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PVOID DpcData;
} KDPC, *PKDPC;

typedef struct _KDPC_DATA {
  LIST_ENTRY  DpcListHead;
  ULONG  DpcLock;
  ULONG  DpcQueueDepth;
  ULONG  DpcCount;
} KDPC_DATA, *PKDPC_DATA;

typedef enum _KBUGCHECK_CALLBACK_REASON {
    KbCallbackInvalid,
    KbCallbackReserved1,
    KbCallbackSecondaryDumpData,
    KbCallbackDumpIo,
} KBUGCHECK_CALLBACK_REASON;

typedef
VOID
(*PKBUGCHECK_REASON_CALLBACK_ROUTINE) (
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PVOID Record, // This should be struct _KBUGCHECK_REASON_CALLBACK_RECORD* but minggw doesn't want to allow that...
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    );

typedef struct _KBUGCHECK_REASON_CALLBACK_RECORD {
    LIST_ENTRY Entry;
    PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine;
    PUCHAR Component;
    ULONG_PTR Checksum;
    KBUGCHECK_CALLBACK_REASON Reason;
    UCHAR State;
} KBUGCHECK_REASON_CALLBACK_RECORD, *PKBUGCHECK_REASON_CALLBACK_RECORD;

struct _KINTERRUPT;

typedef BOOLEAN STDCALL_FUNC
(*PKSERVICE_ROUTINE)(struct _KINTERRUPT* Interrupt,
		     PVOID ServiceContext);
typedef struct _EPROCESS EPROCESS, *PEPROCESS;

typedef HANDLE HSEMAPHORE;

typedef HANDLE HDRVOBJ;

typedef LONG FLOAT_LONG, *PFLOAT_LONG;

typedef LONG FLOATL;

typedef LONG FIX; /* fixed-point number */

/* copied from W32API */
typedef struct _KFLOATING_SAVE
{
  ULONG  ControlWord;
  ULONG  StatusWord;
  ULONG  ErrorOffset;
  ULONG  ErrorSelector;
  ULONG  DataOffset;
  ULONG  DataSelector;
  ULONG  Cr0NpxState;
  ULONG  Spare1;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

#endif /* __INCLUDE_DDK_KETYPES_H */
