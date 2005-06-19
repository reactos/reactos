/* 
 * FIXME: *** NDK ***
 */

#ifndef __INCLUDE_NTOS_KRNLTYPES_H
#define __INCLUDE_NTOS_KRNLTYPES_H

#define DOE_UNLOAD_PENDING    0x1
#define DOE_DELETE_PENDING    0x2
#define DOE_REMOVE_PENDING    0x4
#define DOE_REMOVE_PROCESSED  0x8
#define DOE_START_PENDING     0x10

extern POBJECT_TYPE EXPORTED ExMutantObjectType;
extern POBJECT_TYPE EXPORTED ExTimerType;

/*
 * PURPOSE: Special timer associated with each device
 */
typedef struct _IO_TIMER {
   USHORT Type;				/* Every IO Object has a Type */
   USHORT TimerEnabled;			/* Tells us if the Timer is enabled or not */
   LIST_ENTRY IoTimerList;		/* List of other Timers on the system */
   PIO_TIMER_ROUTINE TimerRoutine;	/* The associated timer routine */
   PVOID Context;			/* Context */
   PDEVICE_OBJECT DeviceObject;		/* Driver that owns this IO Timer */
} IO_TIMER, *PIO_TIMER;

typedef struct _EX_QUEUE_WORKER_INFO {
    UCHAR QueueDisabled:1;
    UCHAR MakeThreadsAsNecessary:1;
    UCHAR WaitMode:1;
    ULONG WorkerCount:29;
} EX_QUEUE_WORKER_INFO, *PEX_QUEUE_WORKER_INFO;

typedef struct _EX_WORK_QUEUE {
    KQUEUE WorkerQueue;
    ULONG DynamicThreadCount;
    ULONG WorkItemsProcessed;
    ULONG WorkItemsProcessedLastPass;
    ULONG QueueDepthLastPass;
    EX_QUEUE_WORKER_INFO Info;    
} EX_WORK_QUEUE, *PEX_WORK_QUEUE;

typedef struct _KDPC_DATA 
{
    LIST_ENTRY  DpcListHead;
    ULONG  DpcLock;
    ULONG  DpcQueueDepth;
    ULONG  DpcCount;
} KDPC_DATA, *PKDPC_DATA;

typedef struct _KTRAP_FRAME 
{
    PVOID DebugEbp;
    PVOID DebugEip;
    PVOID DebugArgMark;
    PVOID DebugPointer;
    PVOID TempCs;
    PVOID TempEip;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;
    USHORT Gs;
    USHORT Reserved1;
    USHORT Es;
    USHORT Reserved2;
    USHORT Ds;
    USHORT Reserved3;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;
    ULONG PreviousMode;
    PVOID ExceptionList;
    USHORT Fs;
    USHORT Reserved4;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Ebp;
    ULONG ErrorCode;
    ULONG Eip;
    ULONG Cs;
    ULONG Eflags;
    ULONG Esp;
    USHORT Ss;
    USHORT Reserved5;
    USHORT V86_Es;
    USHORT Reserved6;
    USHORT V86_Ds;
    USHORT Reserved7;
    USHORT V86_Fs;
    USHORT Reserved8;
    USHORT V86_Gs;
    USHORT Reserved9;
} KTRAP_FRAME, *PKTRAP_FRAME;

typedef struct _PP_LOOKASIDE_LIST 
{
   struct _GENERAL_LOOKASIDE *P;
   struct _GENERAL_LOOKASIDE *L;
} PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;

typedef enum _PP_NPAGED_LOOKASIDE_NUMBER
{
   LookasideSmallIrpList = 0,
   LookasideLargeIrpList = 1,
   LookasideMdlList = 2,
   LookasideCreateInfoList = 3,
   LookasideNameBufferList = 4,
   LookasideTwilightList = 5,
   LookasideCompletionList = 6,
   LookasideMaximumList = 7
} PP_NPAGED_LOOKASIDE_NUMBER;

typedef enum _KOBJECTS {
   EventNotificationObject = 0,
   EventSynchronizationObject = 1,
   MutantObject = 2,
   ProcessObject = 3,
   QueueObject = 4,
   SemaphoreObject = 5,
   ThreadObject = 6,
   GateObject = 7,
   TimerNotificationObject = 8,
   TimerSynchronizationObject = 9,
   Spare2Object = 10,
   Spare3Object = 11,
   Spare4Object = 12,
   Spare5Object = 13,
   Spare6Object = 14,
   Spare7Object = 15,
   Spare8Object = 16,
   Spare9Object = 17,
   ApcObject = 18,
   DpcObject = 19,
   DeviceQueueObject = 20,
   EventPairObject = 21,
   InterruptObject = 22,
   ProfileObject = 23,
   ThreadedDpcObject = 24,
   MaximumKernelObject = 25
} KOBJECTS;

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

typedef struct _KEVENT_PAIR
{
    CSHORT Type;
    CSHORT Size;
    KEVENT LowEvent;
    KEVENT HighEvent;
} KEVENT_PAIR, *PKEVENT_PAIR;

typedef struct _RUNDOWN_DESCRIPTOR {
    ULONG_PTR References;
    KEVENT RundownEvent;
} RUNDOWN_DESCRIPTOR, *PRUNDOWN_DESCRIPTOR;

#endif /* __INCLUDE_NTOS_KRNLTYPES_H */

