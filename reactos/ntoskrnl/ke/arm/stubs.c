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
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
KeUpdateRunTime(IN PKTRAP_FRAME TrapFrame,
                IN KIRQL Irql)
{
    UNIMPLEMENTED;
    while (TRUE);  
}
