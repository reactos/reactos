/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxValidateFunctions.cpp

Abstract:

    Functions which validate external WDF data structures

Author:



Environment:

    Both kernel and user mode

Revision History:










--*/

#include "fxobjectpch.hpp"

// We use DoTraceMessage
extern "C" {
#if defined(EVENT_TRACING)
#include "FxValidateFunctions.tmh"
#endif
}

_Must_inspect_result_
NTSTATUS
FxValidateObjectAttributes(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in ULONG Flags
    )
{
    if (Attributes == NULL) {
        if (Flags & FX_VALIDATE_OPTION_ATTRIBUTES_REQUIRED) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDF_OBJECT_ATTRIBUTES required, %!STATUS!",
                (ULONG) STATUS_WDF_PARENT_NOT_SPECIFIED);

            return STATUS_WDF_PARENT_NOT_SPECIFIED;
        }
        else {
            return STATUS_SUCCESS;
        }
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

    if (Attributes->ContextTypeInfo != NULL) {
#pragma prefast(suppress:__WARNING_REDUNDANTTEST, "different structs of the same size")
        if (Attributes->ContextTypeInfo->Size !=
                                        sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO) &&
            Attributes->ContextTypeInfo->Size !=
                                        sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO_V1_0)) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p ContextTypeInfo %p Size %d incorrect, expected %d, %!STATUS!",
                Attributes, Attributes->ContextTypeInfo,
                Attributes->ContextTypeInfo->Size,
                sizeof(WDF_OBJECT_CONTEXT_TYPE_INFO), STATUS_INFO_LENGTH_MISMATCH);

            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // A ContextName != NULL and a ContextSize of 0 is allowed
        //
        if (Attributes->ContextTypeInfo->ContextSize > 0 &&
            Attributes->ContextTypeInfo->ContextName == NULL) {

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p ContextTypeInfo %p ContextSize %I64d is not zero, "
                "but ContextName is NULL, %!STATUS!",
                Attributes, Attributes->ContextTypeInfo,
                Attributes->ContextTypeInfo->ContextSize,
                STATUS_WDF_OBJECT_ATTRIBUTES_INVALID);

            return STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;
        }
    }

    if (Attributes->ContextSizeOverride > 0) {
        if (Attributes->ContextTypeInfo == NULL) {
            //
            // Can't specify additional size without a type
            //
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p ContextSizeOverride of %I64d specified, but no type "
                "information, %!STATUS!",
                Attributes, Attributes->ContextSizeOverride,
                STATUS_WDF_OBJECT_ATTRIBUTES_INVALID);

            return STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;
        }
        else if (Attributes->ContextSizeOverride <
                                    Attributes->ContextTypeInfo->ContextSize) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p ContextSizeOverride %I64d < "
                "ContextTypeInfo->ContextSize %I64d, %!STATUS!",
                Attributes, Attributes->ContextSizeOverride,
                Attributes->ContextTypeInfo->ContextSize,
                STATUS_WDF_OBJECT_ATTRIBUTES_INVALID);

            return STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;
        }
    }

    if (Flags & FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED) {
        if (Attributes->ParentObject != NULL) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p does not allow a parent object to be set, set to "
                "%p, %!STATUS!", Attributes, Attributes->ParentObject,
                STATUS_WDF_PARENT_ASSIGNMENT_NOT_ALLOWED);

            return STATUS_WDF_PARENT_ASSIGNMENT_NOT_ALLOWED;
        }
    }
    else if ((Flags & FX_VALIDATE_OPTION_PARENT_REQUIRED_FLAG) &&
             Attributes->ParentObject == NULL) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "ParentObject required in WDF_OBJECT_ATTRIBUTES %p, %!STATUS!",
            Attributes, STATUS_WDF_PARENT_NOT_SPECIFIED);

        return STATUS_WDF_PARENT_NOT_SPECIFIED;
    }

    // Enum range checks
    if ((Attributes->ExecutionLevel == WdfExecutionLevelInvalid) ||
        (Attributes->ExecutionLevel > WdfExecutionLevelDispatch)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "Attributes %p execution level set to %d, out of range, %!STATUS!",
            Attributes, Attributes->ExecutionLevel,
            STATUS_WDF_OBJECT_ATTRIBUTES_INVALID);
        return STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;
    }

    if ((Attributes->SynchronizationScope == WdfSynchronizationScopeInvalid) ||
        (Attributes->SynchronizationScope > WdfSynchronizationScopeNone)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
            "Attributes %p synchronization scope set to %d, out of range, %!STATUS!",
            Attributes, Attributes->SynchronizationScope,
            STATUS_WDF_OBJECT_ATTRIBUTES_INVALID);
        return STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;
    }

    if ((Flags & FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED) == 0) {

        //
        // If synchronization is not allowed for this object,
        // check the requested level to ensure none was specified.
        //
        if ((Attributes->SynchronizationScope != WdfSynchronizationScopeInheritFromParent) &&
            (Attributes->SynchronizationScope != WdfSynchronizationScopeNone)) {

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p does not allow synchronization scope too be set, "
                "but was set to %!WDF_SYNCHRONIZATION_SCOPE!, %!STATUS!",
                Attributes, Attributes->SynchronizationScope,
                STATUS_WDF_SYNCHRONIZATION_SCOPE_INVALID);

            return STATUS_WDF_SYNCHRONIZATION_SCOPE_INVALID;
        }
    }

    if ((Flags & FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED) == 0) {

        //
        // If execution level restrictions are not allowed for this object,
        // check the requested level to ensure none was specified.
        //
        if (Attributes->ExecutionLevel != WdfExecutionLevelInheritFromParent) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "Attributes %p does not allow execution level to be set, but was"
                " set to %!WDF_EXECUTION_LEVEL!, %!STATUS!",
                Attributes, Attributes->ExecutionLevel,
                STATUS_WDF_EXECUTION_LEVEL_INVALID);

            return STATUS_WDF_EXECUTION_LEVEL_INVALID;
        }
    }

    return STATUS_SUCCESS;
}
