/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxValidateFunctions.h

Abstract:

    Split from FxValidateFunctions.hpp
    (FxValidateFunctions.hpp has moved to shared\inc)

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/
#ifndef _FXREQUESTVALIDATEFUNCTIONS_HPP_
#define _FXREQUESTVALIDATEFUNCTIONS_HPP_

#if defined(EVENT_TRACING)
extern "C" {
#include "FxRequestValidateFunctions.hpp.tmh"
}
#endif

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#define WDF_REQUEST_SEND_OPTIONS_VALID_FLAGS            \
       (WDF_REQUEST_SEND_OPTION_TIMEOUT             |   \
        WDF_REQUEST_SEND_OPTION_SYNCHRONOUS         |   \
        WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE |   \
        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET)

#else // (FX_CORE_MODE == FX_CORE_USER_MODE)
#define WDF_REQUEST_SEND_OPTIONS_VALID_FLAGS            \
       (WDF_REQUEST_SEND_OPTION_TIMEOUT             |   \
        WDF_REQUEST_SEND_OPTION_SYNCHRONOUS         |   \
        WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE |   \
        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET     |   \
        WDF_REQUEST_SEND_OPTION_IMPERSONATE_CLIENT  |   \
        WDF_REQUEST_SEND_OPTION_IMPERSONATION_IGNORE_FAILURE)
#endif

NTSTATUS
__inline
FxValidateRequestOptions(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ PWDF_REQUEST_SEND_OPTIONS Options,
    _In_opt_ FxRequestBase* Request = NULL
    )
{
    if (Options == NULL) {
        return STATUS_SUCCESS;
    }

    if (Options->Size != sizeof(WDF_REQUEST_SEND_OPTIONS)) {
        //
        // Size is wrong, bale out
        //
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "Options %p Size incorrect, expected %d, got %d",
                            Options, sizeof(WDF_REQUEST_SEND_OPTIONS),
                            Options->Size);

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if ((Options->Flags & ~WDF_REQUEST_SEND_OPTIONS_VALID_FLAGS) != 0) {
        //
        // Invalid flags
        //
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "Options %p Flags 0x%x invalid, valid mask is 0x%x",
                            Options, Options->Flags,
                            WDF_REQUEST_SEND_OPTIONS_VALID_FLAGS);

        return STATUS_INVALID_PARAMETER;
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    UNREFERENCED_PARAMETER(Request);

    if ((Options->Flags & WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET) &&
        (Options->Flags & ~WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET)) {
        //
        // If WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET is set, no other bits
        // can be set.
        //
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "Options %p, if WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET (0x%x) is "
            "set, no other Flags 0x%x can be set",
            Options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET, Options->Flags);

        return STATUS_INVALID_PARAMETER;
    }

#else // FX_CORE_USER_MODE

    if ((Options->Flags & WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET) &&
        (Options->Flags & ~(WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET |
                            WDF_REQUEST_SEND_OPTION_IMPERSONATION_FLAGS))) {
        //
        // If WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET is set, no other bits
        // can be set except impersonation flags.
        //
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "Options %p, if WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET (0x%x) is "
            "set, no other Flags 0x%x can be set except impersonation flags "
            "%!status!",
            Options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET, Options->Flags,
            STATUS_INVALID_PARAMETER);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Verify the send option flags.
    //
    if ((Options->Flags & WDF_REQUEST_SEND_OPTION_IMPERSONATION_FLAGS) != 0) {
        //
        // The request must be a create request (which also means it can never
        // be a driver-created request).
        //
        if (Request == NULL ||
            Request->IsAllocatedFromIo() == FALSE ||
            Request->GetSubmitFxIrp()->GetMajorFunction() != IRP_MJ_CREATE) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "WDF_REQUEST_SEND_OPTION impersonation flags may only "
                "be set on Create requests. %!status!",
                STATUS_INVALID_PARAMETER);

            return STATUS_INVALID_PARAMETER;
        }

        if ((Options->Flags & WDF_REQUEST_SEND_OPTION_IMPERSONATION_IGNORE_FAILURE) != 0 &&
            (Options->Flags & WDF_REQUEST_SEND_OPTION_IMPERSONATE_CLIENT) == 0) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Driver must set WDF_REQUEST_SEND_OPTION_IMPERSONATION_"
                "IGNORE_FAILURE with WDF_REQUEST_SEND_OPTION_IMPERSONATE_CLIENT."
                " %!status!", STATUS_INVALID_PARAMETER);

            return STATUS_INVALID_PARAMETER;
        }
    }

#endif // FX_CORE_MODE

    return STATUS_SUCCESS;
}

#endif // _FXREQUESTVALIDATEFUNCTIONS_HPP_
