/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Memory allocate api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryCreatePreallocated)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    PVOID Buffer,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out   //deref cud be null if unable to allocate memory
    WDFMEMORY* PMemory
    )/*++

Routine Description:
    External API provided to the client driver to create a WDFMEMORY object
    whose associated buffers are supplied by the caller.  This API is provided
    so that the caller does not need to allocate a new buffer every time she
    wants to pass a WDFMEMORY object to an API which requires it.  It is up to
    the client driver to free the Buffer and Context at the appropriate time.

Arguments:
    Attributes - Context to associate with the returned WDFMEMORY handle
    
    Buffer - Buffer to associate with the returned WDFMEMORY handle

    BufferSize - Size of Buffer in bytes

    PMemory  - Handle to be returned to the caller

Return Value:
    STATUS_INVALID_PARAMETER - if required parameters are incorrect

    STATUS_INSUFFICIENT_RESOURCES - if no resources are available

    STATUS_SUCCESS  - success

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfMemoryAssignBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY Memory,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize)
    PVOID Buffer,
    _In_
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
