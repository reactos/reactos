#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

CCHAR KeNumberProcessors;
ULONG KeDcacheFlushCount;
ULONG KeActiveProcessors;
ULONG KeProcessorArchitecture;
ULONG KeProcessorLevel;
ULONG KeProcessorRevision;
ULONG KeFeatureBits;


extern LONG KiTickOffset;
extern ULONG KeTimeAdjustment;

ULONG
KiComputeTimerTableIndex(IN LONGLONG DueTime)
{
    ULONG Hand;
    DPRINT1("DueTime: %I64x Max: %lx\n", DueTime, KeMaximumIncrement);
    
    //
    // Compute the timer table index
    //
    Hand = (DueTime / KeMaximumIncrement);
    DPRINT1("HAND: %lx\n", Hand);
    Hand %= TIMER_TABLE_SIZE;
    DPRINT1("HAND: %lx\n", Hand);
    return Hand;
}

VOID
NTAPI
KeUpdateSystemTime(IN PKTRAP_FRAME TrapFrame,
                   IN KIRQL Irql,
                   IN ULONG Increment)
{
    PKPRCB Prcb = KeGetPcr()->Prcb;
    LARGE_INTEGER SystemTime, InterruptTime;
    ULONG Hand;
    
    //
    // Do nothing if this tick is being skipped
    //
    if (Prcb->SkipTick)
    {
        //
        // Handle it next time
        //
        Prcb->SkipTick = FALSE;
        return;
    }
    
    //
    // Add the increment time to the shared data
    //
    InterruptTime.HighPart = SharedUserData->InterruptTime.High1Time;
    InterruptTime.LowPart = SharedUserData->InterruptTime.LowPart;
    InterruptTime.QuadPart += Increment;
    SharedUserData->InterruptTime.High1Time = InterruptTime.HighPart;
    SharedUserData->InterruptTime.LowPart = InterruptTime.LowPart;
    SharedUserData->InterruptTime.High2Time = InterruptTime.HighPart;
    
    //
    // Update tick count
    //
    KiTickOffset -= Increment;
    
    //
    // Check for incomplete tick
    //
    if (KiTickOffset > 0)
    {
        
    }
    else
    {
        //
        // Update the system time
        //
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
        SystemTime.QuadPart += KeTimeAdjustment;
        SharedUserData->SystemTime.High1Time = SystemTime.HighPart;
        SharedUserData->SystemTime.LowPart = SystemTime.LowPart;
        SharedUserData->SystemTime.High2Time = SystemTime.HighPart;
        
        //
        // Update the tick count
        //
        SystemTime.HighPart = KeTickCount.High1Time;
        SystemTime.LowPart = KeTickCount.LowPart;
        SystemTime.QuadPart += 1;
        KeTickCount.High1Time = SystemTime.HighPart;
        KeTickCount.LowPart = SystemTime.LowPart;
        KeTickCount.High2Time = SystemTime.HighPart;
        
        //
        // Update it in the shared user data
        //
        SharedUserData->TickCount.High1Time = SystemTime.HighPart;
        SharedUserData->TickCount.LowPart = SystemTime.LowPart;
        SharedUserData->TickCount.High2Time = SystemTime.HighPart;
    }
    
    //
    // Check for timer expiration
    //
    Hand = KeTickCount.LowPart & (TIMER_TABLE_SIZE - 1);
    Hand <<= 4;
    if (KiTimerTableListHead[Hand].Time.QuadPart > InterruptTime.QuadPart)
    {
        //
        // Timer has expired!
        //
        DPRINT1("TIMER EXPIRATION!!!\n");
        while (TRUE);
    }
    
    //
    // Check if we should request a debugger break
    //
    if (KdDebuggerEnabled)
    {
        //
        // Check if we should break
        //
        if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }
    
    //
    // Check if this was a full tick
    //
    if (KiTickOffset <= 0)
    {
        //
        // Updare the tick offset
        //
        KiTickOffset += KeMaximumIncrement;
        
        //
        // Update system runtime
        //
        KeUpdateRunTime(NULL, CLOCK2_LEVEL);
    }
    else
    {
        //
        // Increase interrupt count and exit
        //
        Prcb->InterruptCount++;
    }
}

VOID
NTAPI
KeUpdateRunTime(IN PKTRAP_FRAME TrapFrame,
                IN KIRQL Irql)
{
    UNIMPLEMENTED;
    while (TRUE);  
}
