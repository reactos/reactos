
#include <ntddk.h>
#include <reactos/rossym.h>

#define NDEBUG
#include <debug.h>

BOOLEAN
RosSymCreateFromFile(PVOID FileContext, PROSSYM_INFO *RosSymInfo)
{
    return FALSE;
}

VOID
RosSymDelete(PROSSYM_INFO RosSymInfo)
{
}

VOID
RosSymFreeInfo(PROSSYM_LINEINFO LineInfo)
{
}

BOOLEAN
RosSymGetAddressInformation(PROSSYM_INFO RosSymInfo,
                            ULONG_PTR RelativeAddress,
                            PROSSYM_LINEINFO RosSymLineInfo)
{
    return FALSE;
}

VOID
RosSymInit(PROSSYM_CALLBACKS Callbacks)
{
}

VOID
RosSymInitKernelMode(VOID)
{
}
