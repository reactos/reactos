/* KERNEL TYPES **************************************************************/

#ifndef __INCLUDE_DDK_KETYPES_H
#define __INCLUDE_DDK_KETYPES_H

/* include ntos/ketypes.h here? */

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

struct _DISPATCHER_HEADER;



#include <pshpack1.h>

typedef struct _DISPATCHER_HEADER
{
   UCHAR      Type;
   UCHAR      Absolute;
   UCHAR      Size;
   UCHAR      Inserted;
   LONG       SignalState;
   LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

#include <poppack.h>

typedef struct _KQUEUE
{
   DISPATCHER_HEADER Header;
   LIST_ENTRY        EntryListHead;
   ULONG             CurrentCount;
   ULONG             MaximumCount;
   LIST_ENTRY        ThreadListHead;
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

typedef ULONG_PTR KSPIN_LOCK, *PKSPIN_LOCK;

typedef struct _KDEVICE_QUEUE
{
  CSHORT Type;
  CSHORT Size;
  LIST_ENTRY DeviceListHead;
  KSPIN_LOCK Lock;
  BOOLEAN Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE;


#include <pshpack1.h>

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
} KAPC, *PKAPC;

#include <poppack.h>

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

#include <pshpack1.h>

typedef struct _KSEMAPHORE
{
   DISPATCHER_HEADER Header;
   LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE;

#include <poppack.h>

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

typedef struct _KSPIN_LOCK_QUEUE {
    struct _KSPIN_LOCK_QUEUE * volatile Next;
    PKSPIN_LOCK volatile Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
    KSPIN_LOCK_QUEUE LockQueue;
    KIRQL OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

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

/*
 * PURPOSE: Defines a delayed procedure call object
 */
#include <pshpack1.h>

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
} KDPC, *PKDPC;

#include <poppack.h>


typedef struct _KDEVICE_QUEUE_ENTRY
{
   LIST_ENTRY DeviceListEntry;
   ULONG SortKey;
   BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY;

typedef struct _WAIT_CONTEXT_BLOCK
{
  KDEVICE_QUEUE_ENTRY WaitQueueEntry;
  /*
   * XXX THIS IS WRONG XXX
   *
   * Our headers have enough circular dependancies that
   * I can't figure out, given 5 minutes of testing, what
   * order to include them in to get PDRIVER_CONTROL to be
   * defined here.  The proper definition of the next item
   * is:
   *
   * PDRIVER_CONTROL DeviceRoutine;
   *
   * but instead we use PVOID until headers are fixed.
   */
  PVOID DeviceRoutine;
  PVOID DeviceContext;
  ULONG NumberOfMapRegisters;
  PVOID DeviceObject;
  PVOID CurrentIrp;
  PKDPC BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

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

typedef struct _M128 {
    ULONGLONG Low;
    LONGLONG High;
} M128, *PM128;

typedef struct _KEXCEPTION_FRAME {
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;
    ULONG64 InitialStack;
    M128 Xmm6;
    M128 Xmm7;
    M128 Xmm8;
    M128 Xmm9;
    M128 Xmm10;
    M128 Xmm11;
    M128 Xmm12;
    M128 Xmm13;
    M128 Xmm14;
    M128 Xmm15;
    ULONG64 TrapFrame;
    ULONG64 CallbackStack;
    ULONG64 OutputBuffer;
    ULONG64 OutputLength;
    UCHAR ExceptionRecord[64];
    ULONG64 Fill1;
    ULONG64 Rbp;
    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Return;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

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
