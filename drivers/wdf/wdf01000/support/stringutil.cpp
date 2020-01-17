#include "common/stringutil.h"
#include "common/dbgtrace.h"
#include <ntintsafe.h>


_Must_inspect_result_
NTSTATUS
FxDuplicateUnicodeString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in const UNICODE_STRING* Source,
    __out PUNICODE_STRING Destination
    )
/*++

Routine Description:
    Makes a deep copy from Source to Destination.

    Destination is assumed to have been initialized by the caller, be owned by
    an internal Fx object.  Destination could already contain a previously
    allocated buffer.  If one exists and is large enough, it will be reused
    for the copy.

    This function guarantees that the Buffer will be NULL terminated.  While
    this is not necessary because Length describes the length of the buffer, the
    resulting buffer can be copied back to the client driver and we cannot trust
    the client driver to treat the string as an unterminated buffer, typically
    the client driver will just extract the buffer and treat it as NULL
    terminated.  To be defensive with this type of (mis)use, we always NULL
    terminate the resuling Buffer.

Arguments:
    Source - source struct to copy from.  This string can originate from the
             client driver.

    Destination - destination struct to copy to.  This struct is assumed to be
                  internal and is not given to the outside caller

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;
    USHORT srcCbLength, srcCbLengthAndNull, dstMaxCbLength;

    //
    // NOTE: We assume the sources string will be smaller than 64k.
    //
    srcCbLength = Source->Length;
    dstMaxCbLength = Destination->MaximumLength;

    status = RtlUShortAdd(srcCbLength,
                          sizeof(UNICODE_NULL),
                          &srcCbLengthAndNull);
    
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
            "Interger overflow occured when duplicating string %!STATUS!", 
            status);
        return status;
    }

    //
    // First see if we already have enough memory to hold the string + a NULL
    //
    if (dstMaxCbLength < srcCbLengthAndNull)
    {
        //
        // Allocate enough memory for the source string and a NULL character
        //
        dstMaxCbLength = srcCbLengthAndNull;

        //
        // We need to allocate memory.  Free any old memory first.
        //
        if (Destination->Buffer != NULL)
        {
            FxPoolFree(Destination->Buffer);

            RtlZeroMemory(Destination, sizeof(UNICODE_STRING));
        }

        Destination->Buffer = (PWSTR) FxPoolAllocate(
            FxDriverGlobals, PagedPool, dstMaxCbLength);

        if (Destination->Buffer == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGERROR,
                "Failed to allocate memory when duplicating string %!STATUS!", 
                status);
            return status;
        }

        Destination->MaximumLength =  dstMaxCbLength;
    }

    //
    // If we get here and we have a buffer, then we can just copy the
    // string into the buffer.
    //
    RtlCopyMemory(Destination->Buffer, Source->Buffer, srcCbLength);
    Destination->Length = srcCbLength;

    //
    // Make sure the string is NULL terminated and there is room for the NULL
    //
    ASSERT(Destination->Length + sizeof(UNICODE_NULL) <=
                                                    Destination->MaximumLength);
    Destination->Buffer[Destination->Length/sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;
}