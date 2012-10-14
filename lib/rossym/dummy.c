
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

BOOLEAN
RosSymGetAddressInformation(PROSSYM_INFO RosSymInfo,
                            ULONG_PTR RelativeAddress,
                            ULONG *LineNumber,
                            char *FileName,
                            char *FunctionName)
{
    return FALSE;
}

VOID
RosSymInitKernelMode(VOID)
{
}
