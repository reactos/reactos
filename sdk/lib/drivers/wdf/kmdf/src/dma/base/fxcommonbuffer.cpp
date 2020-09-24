/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCommonBuffer.cpp

Abstract:

    WDF CommonBuffer Object

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "FxDmaPCH.hpp"

extern "C" {
#include "FxCommonBuffer.tmh"
}

FxCommonBuffer::FxCommonBuffer(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxDmaEnabler * pDmaEnabler
    ) :
    FxNonPagedObject(FX_TYPE_COMMON_BUFFER, sizeof(FxCommonBuffer), FxDriverGlobals)
{
    m_DmaEnabler           = pDmaEnabler;
    m_BufferRawVA          = NULL;   // allocated buffer base (unaligned)
    m_BufferAlignedVA      = NULL;   // aligned buffer base
    m_BufferAlignedLA.QuadPart = NULL;   // aligned physical buffer base
    m_BufferRawLA.QuadPart = NULL;   // allocated buffer phy base (unaligned)
    m_Length               = 0;
    m_RawLength            = 0;

    MarkDisposeOverride(ObjectDoNotLock);

    //
    // By default use the alignment of the dma enabler.
    //
    m_Alignment = m_DmaEnabler->GetAlignment();
}

BOOLEAN
FxCommonBuffer::Dispose()
{
    FreeCommonBuffer();
    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxCommonBuffer::AllocateCommonBuffer(
    __in size_t  Length
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    ULONGLONG offset;
    ULONG result;

    //
    // Must be running at PASIVE_LEVEL
    //
    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    m_Length = Length;

    //
    // If required, add alignment to the length.
    // If alignment is <= page-1, we actually don't need to do it b/c
    // AllocateCommonBuffer allocates at least a page of memory, regardless
    // of the requested Length. If driver version is < v1.11, we still add the
    // alignment to the length for compatibility.
    //
    if (m_Alignment > PAGE_SIZE-1 ||
        pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11) == FALSE) {

        status = RtlSizeTAdd(Length, m_Alignment, &m_RawLength);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                "WDFDMAENABLER %p AllocateCommonBuffer:  overflow when adding Length "
                "%I64d + Alignment %I64d", GetObjectHandle(), Length, m_Alignment);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }
    }
    else {
        m_RawLength = Length;
    }

    m_DmaEnabler->AllocateCommonBuffer(m_RawLength,
                                       &m_BufferRawVA,
                                       &m_BufferRawLA);
    if (m_BufferRawVA) {

        m_BufferAlignedVA = FX_ALIGN_VIRTUAL_ADDRESS(m_BufferRawVA, m_Alignment);
        m_BufferAlignedLA.QuadPart = FX_ALIGN_LOGICAL_ADDRESS(m_BufferRawLA, m_Alignment);

        if (m_Alignment > PAGE_SIZE-1) {
            //
            // If the alignment mask is over a page-size then the aligned virtual
            // and aligned logical could be pointing to different locations
            // in memory. So ajdust the VA to match the LA address by adding
            // the offset of alignedLA and RawLA to VA. By doing this we
            // only guarantee alignment of LA when the page alignment exceeds PAGE_SIZE.
            //
            status = RtlULongLongSub(m_BufferAlignedLA.QuadPart,
                                     m_BufferRawLA.QuadPart,
                                     &offset);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMAENABLER %p AllocateCommonBuffer:  overflow when subtracting "
                    "RawLA %I64x from AlignedLA %I64x",
                    GetObjectHandle(), m_BufferRawLA.QuadPart, m_BufferAlignedLA.QuadPart);
                FxVerifierDbgBreakPoint(pFxDriverGlobals);
                return status;
            }

            status = RtlULongLongToULong(offset, &result);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMAENABLER %p AllocateCommonBuffer:  overflow when  "
                    "converting from ULongLong %I64d to ULong",
                    GetObjectHandle(), offset);
                FxVerifierDbgBreakPoint(pFxDriverGlobals);
                return status;
            }

            m_BufferAlignedVA = WDF_PTR_ADD_OFFSET(m_BufferRawVA, result);
        }
        else {
            ASSERT(m_BufferAlignedVA == m_BufferRawVA);
            ASSERT(m_BufferAlignedLA.QuadPart == m_BufferRawLA.QuadPart);
        }
    } else {
        m_Length                   = 0;
        m_RawLength                = 0;
        m_BufferAlignedVA          = NULL;
        m_BufferAlignedLA.QuadPart = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

VOID
FxCommonBuffer::FreeCommonBuffer(
    VOID
    )
{
    //
    // Free this CommonBuffer per DmaEnabler
    //
    if (m_BufferRawVA != NULL) {
        m_DmaEnabler->FreeCommonBuffer((ULONG) m_RawLength,
                                        m_BufferRawVA,
                                        m_BufferRawLA);
    }
}


