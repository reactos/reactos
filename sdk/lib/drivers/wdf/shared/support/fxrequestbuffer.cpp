/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxRequestBuffer.cpp

Abstract:

    This module implements a memory union object

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "FxSupportPch.hpp"

extern "C" {
#include "FxRequestBuffer.tmh"
}

FxRequestBuffer::FxRequestBuffer(
    VOID
    )
{
    DataType = FxRequestBufferUnspecified;
    RtlZeroMemory(&u, sizeof(u));
}

NTSTATUS
FxRequestBuffer::ValidateMemoryDescriptor(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_MEMORY_DESCRIPTOR Descriptor,
    __in ULONG Flags
    )
{
    IFxMemory* pMemory;
    NTSTATUS status;

    if (Descriptor == NULL) {
        if (Flags & MemoryDescriptorNullAllowed) {
            return STATUS_SUCCESS;
        }
        else {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                "A NULL Descriptor is not allowed");

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // For each type, check to see if the buffer is non NULL and err out if the
    // calller considers this an error.  If the buffer is NULL, but a length
    // was specified, this is considered an error.
    //
    switch (Descriptor->Type) {
    case WdfMemoryDescriptorTypeBuffer:
        if (Descriptor->u.BufferType.Buffer == NULL) {
            if ((Flags & MemoryDescriptorNoBufferAllowed) == 0) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "A NULL Buffer is not allowed");

                return STATUS_INVALID_PARAMETER;
            }
            else if (Descriptor->u.BufferType.Length != 0) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "Buffer is NULL, but a length (0x%x) is specified",
                    Descriptor->u.BufferType.Length);

                return STATUS_INVALID_PARAMETER;
            }
        }

        SetBuffer(Descriptor->u.BufferType.Buffer,
                  Descriptor->u.BufferType.Length);

        status = STATUS_SUCCESS;
        break;

    case WdfMemoryDescriptorTypeMdl:
        if (Descriptor->u.MdlType.Mdl == NULL) {
            if ((Flags & MemoryDescriptorNoBufferAllowed) == 0) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "A NULL MDL is not allowed");

                return STATUS_INVALID_PARAMETER;
            }
            else if (Descriptor->u.MdlType.BufferLength != 0) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "MDL is NULL, but a length (0x%x) is specified",
                    Descriptor->u.MdlType.BufferLength);

                return STATUS_INVALID_PARAMETER;
            }
        }

        SetMdl(Descriptor->u.MdlType.Mdl, Descriptor->u.MdlType.BufferLength);
        status = STATUS_SUCCESS;
        break;

    case WdfMemoryDescriptorTypeHandle:
        pMemory = NULL;
        if (Descriptor->u.HandleType.Memory == NULL) {
            if (Flags & MemoryDescriptorNoBufferAllowed) {
                status = STATUS_SUCCESS;
            }
            else {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "A NULL WDFMEMORY handle is not allowed");

                status = STATUS_INVALID_PARAMETER;
            }
        }
        else {
            FxObjectHandleGetPtr(FxDriverGlobals,
                                 Descriptor->u.HandleType.Memory,
                                 IFX_TYPE_MEMORY,
                                 (PVOID*) &pMemory);

            status = pMemory->ValidateMemoryOffsets(
                Descriptor->u.HandleType.Offsets);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                    "Memory offset values are not valid %!STATUS!", status);
            }
        }

        if (NT_SUCCESS(status) && pMemory != NULL) {
            SetMemory(pMemory, Descriptor->u.HandleType.Offsets);
        }
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

ULONG
FxRequestBuffer::GetBufferLength(
    VOID
    )
{
    switch (DataType) {
    case FxRequestBufferMemory:
        //
        // If the BufferLength and BufferOffset is zero, then the transfer length is same
        // as the length of the request.
        //
        if (u.Memory.Offsets == NULL ||
            (u.Memory.Offsets->BufferOffset == 0 && u.Memory.Offsets->BufferLength == 0)) {
            return (ULONG) u.Memory.Memory->GetBufferSize();
        }
        else {
            //
            // If the BufferLength value is zero then the transfer length is request length
            // minus the offset value.
            //
            if (u.Memory.Offsets->BufferLength == 0) {
                return ((ULONG) u.RefMdl.Memory->GetBufferSize() - (ULONG) u.RefMdl.Offsets->BufferOffset);
            }
            else {
                return (ULONG) u.Memory.Offsets->BufferLength;
            }
        }
        break;

    case FxRequestBufferMdl:
        return u.Mdl.Length;

    case FxRequestBufferReferencedMdl:
        //
        // If the BufferLength and BufferOffset is zero, then the transfer length is same
        // as the length of the request.
        //
        if (u.RefMdl.Offsets == NULL ||
            (u.RefMdl.Offsets->BufferOffset == 0 && u.RefMdl.Offsets->BufferLength == 0)) {
            return (ULONG) u.RefMdl.Memory->GetBufferSize();
        }
        else {
            //
            // If the BufferLength value is zero then the transfer length is request length
            // minus the offset value.
            //
            if (u.RefMdl.Offsets->BufferLength == 0) {
                return ((ULONG) u.RefMdl.Memory->GetBufferSize() - (ULONG) u.RefMdl.Offsets->BufferOffset);
            }
            else {
                return (ULONG) u.RefMdl.Offsets->BufferLength;
            }
        }

    case FxRequestBufferBuffer:
        return u.Buffer.Length;

    default:
        return 0;
    }
}

_Must_inspect_result_
NTSTATUS
FxRequestBuffer::GetBuffer(
    __deref_out PVOID* Buffer
    )
{
    switch (DataType) {
    case FxRequestBufferUnspecified:
        *Buffer = NULL;
        return STATUS_SUCCESS;

    case FxRequestBufferMemory:
        if (u.Memory.Offsets != NULL) {
            *Buffer = WDF_PTR_ADD_OFFSET(u.Memory.Memory->GetBuffer(),
                                         u.Memory.Offsets->BufferOffset);
        }
        else {
            *Buffer = u.Memory.Memory->GetBuffer();
        }
        return STATUS_SUCCESS;

    case FxRequestBufferBuffer:
        *Buffer = u.Buffer.Buffer;
        return STATUS_SUCCESS;

    case FxRequestBufferMdl:
        *Buffer = Mx::MxGetSystemAddressForMdlSafe(u.Mdl.Mdl, NormalPagePriority);
        if (*Buffer != NULL) {
            return STATUS_SUCCESS;
        }
        else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    case FxRequestBufferReferencedMdl:
        *Buffer = Mx::MxGetSystemAddressForMdlSafe(u.RefMdl.Mdl, NormalPagePriority);
        if (*Buffer != NULL) {
            if (u.RefMdl.Offsets != NULL) {
                *Buffer = WDF_PTR_ADD_OFFSET(*Buffer,
                                             u.RefMdl.Offsets->BufferOffset);
            }
            return STATUS_SUCCESS;
        }
        else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    default:
        return STATUS_INVALID_PARAMETER;
    }
}


VOID
FxRequestBuffer::AssignValues(
    __deref_out_opt PVOID* PPBuffer,
    __deref_out_opt PMDL* PPMdl,
    __out PULONG BufferLength
    )
{
    PVOID pBuffer;
    PMDL pMdl;
    size_t bufferSize;

    //
    // Make sure we have valid double pointers, make life simpler below
    //
    if (PPBuffer == NULL) {
        PPBuffer = &pBuffer;
    }
    if (PPMdl == NULL) {
        PPMdl = &pMdl;
    }

    switch (DataType) {
    case FxRequestBufferMemory:
        pBuffer = u.Memory.Memory->GetBuffer();
        bufferSize = u.Memory.Memory->GetBufferSize();

        if (u.Memory.Offsets != NULL) {
            if (u.Memory.Offsets->BufferLength > 0) {
                bufferSize = u.Memory.Offsets->BufferLength;
            }
            if (u.Memory.Offsets->BufferOffset > 0) {
                pBuffer = WDF_PTR_ADD_OFFSET(pBuffer, u.Memory.Offsets->BufferOffset);
            }
        }

        *PPBuffer = pBuffer;
        *BufferLength = (ULONG) bufferSize;
        break;

    case FxRequestBufferMdl:
        *PPMdl = u.Mdl.Mdl;
        *PPBuffer = NULL;
        *BufferLength = u.Mdl.Length;
        break;

    case FxRequestBufferBuffer:
        *PPMdl = NULL;
        *PPBuffer = u.Buffer.Buffer;
        *BufferLength = u.Buffer.Length;
        break;

    case FxRequestBufferReferencedMdl:
        *PPMdl = u.RefMdl.Mdl;
        *PPBuffer = NULL;
        if (u.RefMdl.Offsets != NULL && u.RefMdl.Offsets->BufferLength > 0) {
            *BufferLength = (ULONG) u.RefMdl.Offsets->BufferLength;
        }
        else {
            *BufferLength = (ULONG) u.RefMdl.Memory->GetBufferSize();
        }
        break;

    default:
        *PPMdl = NULL;
        *PPBuffer = NULL;
        *BufferLength = 0;
        break;
    }
}
