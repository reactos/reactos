
#include <ntddk.h>
#include <reactos/rossym.h>

#define NDEBUG
#include <debug.h>

BOOLEAN
RosSymCreateFromFile(PVOID FileContext, PROSSYM_INFO *RosSymInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
RosSymDelete(PROSSYM_INFO RosSymInfo)
{
    UNIMPLEMENTED;
}

VOID
RosSymFreeInfo(PROSSYM_LINEINFO LineInfo)
{
    UNIMPLEMENTED;
}

BOOLEAN
RosSymGetAddressInformation(PROSSYM_INFO RosSymInfo,
                            ULONG_PTR RelativeAddress,
                            PROSSYM_LINEINFO RosSymLineInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
RosSymInit(PROSSYM_CALLBACKS Callbacks)
{
    UNIMPLEMENTED;
}

VOID
RosSymInitKernelMode(VOID)
{
    UNIMPLEMENTED;
}

BOOLEAN
RosSymAggregate(PROSSYM_INFO RosSymInfo, PCHAR Type, PROSSYM_AGGREGATE Aggregate)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
RosSymFreeAggregate(PROSSYM_AGGREGATE Aggregate)
{
    UNIMPLEMENTED;
}

