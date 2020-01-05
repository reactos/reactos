#include "wdf.h"

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfMemoryCopyToBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY SourceMemory,
    __in
    size_t SourceOffset,
    __out_bcount( NumBytesToCopyTo)
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    )
/*++

Routine Description:
    Copies memory from a WDFMEMORY handle to a raw pointer

Arguments:
    SourceMemory - Memory handle whose contents we are copying from.

    SourceOffset - Offset into SourceMemory from which the copy starts.

    Buffer - Memory whose contents we are copying into

    NumBytesToCopyTo - Number of bytes to copy into buffer.

Return Value:

    STATUS_BUFFER_TOO_SMALL - SourceMemory is smaller than the requested number
        of bytes to be copied.

    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfMemoryCopyFromBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY DestinationMemory,
    __in
    size_t DestinationOffset,
    __in
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    )
/*++

Routine Description:
    Copies memory from a raw pointer into a WDFMEMORY handle

Arguments:
    DestinationMemory - Memory handle whose contents we are copying into

    DestinationOffset - Offset into DestinationMemory from which the copy
        starts.

    Buffer - Buffer whose context we are copying from

    NumBytesToCopyFrom - Number of bytes to copy from

Return Value:

    STATUS_INVALID_BUFFER_SIZE - DestinationMemory is not large enough to contain
        the number of bytes requested to be copied

    NTSATUS

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

} // extern "C"
