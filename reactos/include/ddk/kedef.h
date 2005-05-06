#ifndef __INCLUDE_DDK_KEDEF_H
#define __INCLUDE_DDK_KEDEF_H

typedef enum _KBUGCHECK_BUFFER_DUMP_STATE
{
	BufferEmpty,
	BufferInserted,
	BufferStarted,
	BufferFinished,
	BufferIncomplete
}KBUGCHECK_BUFFER_DUMP_STATE;

typedef enum _KINTERRUPT_MODE
{
   LevelSensitive,
   Latched,
} KINTERRUPT_MODE;

/*
 * PURPOSE: DPC importance
 */
typedef enum _KDPC_IMPORTANCE
{
	LowImportance,
	MediumImportance,
	HighImportance
} KDPC_IMPORTANCE;

typedef enum _EVENT_TYPE
{
   NotificationEvent,
   SynchronizationEvent,
} EVENT_TYPE;

typedef enum _KWAIT_REASON
{
   Executive,
   FreePage,
   PageIn,
   PoolAllocation,
   DelayExecution,
   Suspended,
   UserRequest,
   WrExecutive,
   WrFreePage,
   WrPageIn,
   WrPoolAllocation,
   WrDelayExecution,
   WrSuspended,
   WrUserRequest,
   WrEventPair,
   WrQueue,
   WrLpcReceive,
   WrLpcReply,
   WrVirtualMemory,
   WrPageOut,
   WrRendezvous,
   Spare2,
   WrGuardedMutex,
   Spare4,
   Spare5,
   Spare6,
   WrKernel,
   WrResource,
   WrPushLock,
   WrMutex,
   WrQuantumEnd,
   WrDispatchInt,
   WrPreempted,
   WrYieldExecution,
   MaximumWaitReason
} KWAIT_REASON;

#endif
