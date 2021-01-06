/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCommonBuffer.hpp

Abstract:

    WDF CommonBuffer Object support

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#ifndef _FXCOMMONBUFFER_H_
#define _FXCOMMONBUFFER_H_

//
// Calculate an "aligned" address (Logical or Virtual) per
// a specific alignment value.
//
FORCEINLINE
PVOID
FX_ALIGN_VIRTUAL_ADDRESS(
    __in PVOID VA,
    __in size_t AlignTo
    )
{
   return (PVOID)(((ULONG_PTR)VA + AlignTo) & ~AlignTo);
}

FORCEINLINE
ULONGLONG
FX_ALIGN_LOGICAL_ADDRESS(
    __in PHYSICAL_ADDRESS LA,
    __in size_t AlignTo
    )
{
   return (LA.QuadPart + AlignTo) & ~((ULONGLONG)AlignTo);
}

//
// Declare the FxCommonBuffer class
//
class FxCommonBuffer : public FxNonPagedObject {

public:

    FxCommonBuffer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxDmaEnabler * pDmaEnabler
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateCommonBuffer(
        __in size_t   Length
        );

    VOID
    FreeCommonBuffer(
        VOID
        );

    __forceinline
    PHYSICAL_ADDRESS
    GetAlignedLogicalAddress(
        VOID
        )
    {
        return m_BufferAlignedLA;
    }

    __forceinline
    PVOID
    GetAlignedVirtualAddress(
        VOID
        )
    {
        return m_BufferAlignedVA;
    }

    __forceinline
    size_t
    GetLength(
        VOID
        )
    {
        return m_Length;
    }

    __forceinline
    VOID
    SetAlignment(
        __in ULONG Alignment
        )
    {
        m_Alignment = Alignment;
    }

protected:

    //
    // Unaligned virtual address
    //
    PVOID                 m_BufferRawVA;

    //
    // Aligned virtual address
    //
    PVOID                 m_BufferAlignedVA;

    //
    // Aligned logical address
    //
    PHYSICAL_ADDRESS      m_BufferAlignedLA;

    //
    // Unaligned logical address
    //
    PHYSICAL_ADDRESS      m_BufferRawLA;

    //
    // Pointer to the DMA enabler
    //
    FxDmaEnabler        * m_DmaEnabler;

    //
    // Length specified by the caller
    //
    size_t                m_Length;

    //
    // Actual length used to allocate buffer after adding the alignement
    // value.
    //
    size_t                m_RawLength;

    //
    // Alignment of the allocated buffer.
    //
    size_t               m_Alignment;

};

#endif // _FXCOMMONBUFFER_H_
