typedef enum _KINTERRUPT_MODE
{
   LevelSensitive,
   Latched,
} KINTERRUPT_MODE;

typedef enum _EVENT_TYPE
{
   NotificationEvent,
   SynchronizationEvent,
   SemaphoreType,
   ProcessType,
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
   WrDelayExecution,
   WrSuspended,
   WrUserRequest,
   WrQueue,
   WrLpcReceive,
   WrLpcReply,
   WrVirtualMemory,
   WrPageOut,
   WrRendezvous,
   Spare2,
   Spare3,
   Spare4,
   Spare5,
   Spare6,
   WrKernel,
   MaximumWaitReason,
} KWAIT_REASON;
