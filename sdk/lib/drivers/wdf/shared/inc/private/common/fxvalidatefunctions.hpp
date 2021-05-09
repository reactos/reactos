/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxValidateFunctions.h

Abstract:

    Inline functions which validate external WDF data structures

Author:



Environment:

    Both kernel and user mode

Revision History:

        Made it mode agnostic

        Moved request options validation to FxRequestValidateFunctions.hpp
        in kmdf\inc\private

        When request is merged FxRequestValidateFunctions.hpp can be moved to
        shared directory

--*/

#ifndef _FXVALIDATEFUNCTIONS_H_
#define _FXVALIDATEFUNCTIONS_H_

extern "C" {

#if defined(EVENT_TRACING)
#include "FxValidateFunctions.hpp.tmh"
#endif

}

enum FX_VALIDATE_FUNCTIONS_FLAGS {
    FX_VALIDATE_OPTION_NONE_SPECIFIED = 0x00000000,
    FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED = 0x00000001,
    FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED = 0x00000002,
    FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED = 0x00000004,
    FX_VALIDATE_OPTION_ATTRIBUTES_REQUIRED = 0x00000008,

    // not used directly, use FX_VALIDATE_OPTION_PARENT_REQUIRED instead
    FX_VALIDATE_OPTION_PARENT_REQUIRED_FLAG = 0x00000010,

    // if a parent is required, the attributes themselves are requried
    FX_VALIDATE_OPTION_PARENT_REQUIRED = FX_VALIDATE_OPTION_PARENT_REQUIRED_FLAG |
                                         FX_VALIDATE_OPTION_ATTRIBUTES_REQUIRED,
};

_Must_inspect_result_
NTSTATUS
FxValidateObjectAttributes(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in ULONG Flags = FX_VALIDATE_OPTION_NONE_SPECIFIED
    );

_Must_inspect_result_
NTSTATUS
__inline
FxValidateObjectAttributesForParentHandle(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in ULONG Flags = FX_VALIDATE_OPTION_NONE_SPECIFIED
    )
{
    if (Attributes == NULL) {
        if (Flags & FX_VALIDATE_OPTION_PARENT_REQUIRED) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDF_OBJECT_ATTRIBUTES required, %!STATUS!",
                (ULONG) STATUS_WDF_PARENT_NOT_SPECIFIED);
        }
        return STATUS_WDF_PARENT_NOT_SPECIFIED;
    }

    if (Attributes->Size != sizeof(WDF_OBJECT_ATTRIBUTES)) {
        //
        // Size is wrong, bail out
        //
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "Attributes %p Size incorrect, expected %d, got %d, %!STATUS!",
                            Attributes, sizeof(WDF_OBJECT_ATTRIBUTES),
                            Attributes->Size, STATUS_INFO_LENGTH_MISMATCH);

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (Attributes->ParentObject == NULL) {
        if (Flags & FX_VALIDATE_OPTION_PARENT_REQUIRED) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "ParentObject required in WDF_OBJECT_ATTRIBUTES %p, %!STATUS!",
                Attributes, STATUS_WDF_PARENT_NOT_SPECIFIED);
        }
        return STATUS_WDF_PARENT_NOT_SPECIFIED;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
__inline
FxValidateUnicodeString(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING String
    )
{
    NTSTATUS status;

    status = STATUS_INVALID_PARAMETER;

    if (String->Length & 1) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "UNICODE_STRING %p, Length %d is odd, %!STATUS!",
            String, String->Length, status);

        return status;
    }

    if (String->MaximumLength & 1) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "UNICODE_STRING %p, MaximumLength %d is odd, %!STATUS!",
            String, String->MaximumLength, status);

        return status;
    }

    if (String->MaximumLength > 0 && String->Buffer == NULL) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "UNICODE_STRING %p, MaximumLength %d > 0, Buffer is NULL, %!STATUS!",
            String, String->MaximumLength, status);

        return status;
    }

    if (String->Length > String->MaximumLength) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "UNICODE_STRING %p, Length %d > MaximumLength %d, %!STATUS!",
            String, String->Length, String->MaximumLength, status);

        return status;
    }

    return STATUS_SUCCESS;
}

#endif // _FXVALIDATEFUNCTIONS_H_
