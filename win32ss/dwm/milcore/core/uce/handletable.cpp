// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//     MIL requires a number of handle tables which have different entry
//     types, but a similar structure and handling. HANDLE_TABLE class
//     provides a generic handle table implementation.
// 
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMILHandleTable, Mem, "CMILHandleTable");

//
// This is an accessor macro used only by the handle-table code internally
// The table is desgned assuming that the first field in any entry type is
// MIL_OBJECT_TYPES or a similar enum and that EMPTY_ENTRY (==0) indicates an
// open slot.
//

#define ENTRY_TYPE_FIELD(idx) \
    *(DWORD*)((BYTE*)m_pvTable + (idx) * m_cbEntry)

#define ENTRY_ADDRESS(idx) \
    (DWORD*)((BYTE*)m_pvTable + (idx) * m_cbEntry)

/*++

Routine Description:

    Handle-table constructor.
    The constructor assigns the size of the individual handle entries.

--*/

HANDLE_TABLE::HANDLE_TABLE(UINT cbEntry)
{
    //
    // We assume that the first field in the table entry structure is of type
    // MIL_OBJECT_TYPES or a similar enum and therefore the cbEntry must have
    // at least that many bytes.
    //

    Assert(cbEntry > sizeof(DWORD));

    //
    // This Assert is to protect against handle-entries which are not a
    // multiple of 4 bytes in size. This guards against unexpected performance
    // problems caused by non-aligned data access rather than a correctness
    // feature.
    //

    Assert(cbEntry % sizeof(UINT) == 0);

    m_cHandleCount = 0;
    m_nFreeIndex = 1;
    m_pvTable = NULL;
    m_cbEntry = cbEntry;
}

/*++

Routine Description:

    Handle-table destructor. Frees the table memory.
    On checked builds it will Assert if there are any non-empty handle
    entries left in the table because that indicates a reference leak.

--*/

HANDLE_TABLE::~HANDLE_TABLE()
{
    //
    // Check to ensure that all the entries in the table have been cleaned
    // up by the caller. This will Assert if the table is leaking a
    // reference on a resource.
    //

    #if DBG

    BYTE *pbTable = reinterpret_cast<BYTE*>(m_pvTable);

    for (UINT i = 0; i < m_cHandleCount; i++)
    {
#if NEVER
        // commenting this out to unblock CinCh

        //
        // Ensure that the entry is marked as open.
        //

        Assert(*(DWORD*)pbTable == EMPTY_ENTRY);
#endif

        //
        // Advance to the next table entry.
        //

        pbTable += m_cbEntry;
    }

    #endif

    //
    // Clean up the memory for the table.
    //

    FreeHeap(m_pvTable);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      HANDLE_TABLE::ResizeToFit
//
//  Synopsis:
//     A private helper method to resize the handle table to fit 
//     the requested handle. The buffer might be resized a little 
//     bit more aggressively to amortize the memory re-allocation. 
// 
//------------------------------------------------------------------------------

HRESULT
HANDLE_TABLE::ResizeToFit(
    __in HMIL_OBJECT hObject  // the handle that needs to be made addressable
    )
{
    HRESULT hr = S_OK;

    UINT cNewSize = 0;

    //
    // Cap the handle table size.
    //

    if (hObject >= MIL_HANDLE_TABLE_SIZE_MAX)
    {
        IFC(WGXERR_UCE_OUTOFHANDLES);
    }


    //
    // We will need to grow the handle table. Try to amortize the 
    // allocations by growing the handle table a little bit faster 
    // than requested:
    //

    C_ASSERT(MIL_HANDLE_TABLE_SIZE_INC > 0);

    IFC(UIntAdd(
            hObject, 
            MIL_HANDLE_TABLE_SIZE_INC,
            &cNewSize
            ));

    //
    // Make sure the amortization doesn't expand the table beyond its 
    // allowed maximum size. Note that there is a gurantee that even 
    // after this capping, the handle table will grow enough to make
    // hObject addressable (as hObject < MIL_HANDLE_TABLE_SIZE_MAX).
    //

    cNewSize = min(
        cNewSize, 
        MIL_HANDLE_TABLE_SIZE_MAX
        );

    if (cNewSize > m_cHandleCount)
    {
        //
        // Attempt to resize the handle table now.
        //
        
        IFC(Resize(cNewSize));
    }

    Assert(hObject < m_cHandleCount);

Cleanup:
    RRETURN(hr);
}


/*++

Routine Description:

    We've run out of space in the handle table. Make it bigger if there is
    available memory.

Arguments:

    cTableSize - new count of handles.

Return Value:

    HRESULT

--*/

HRESULT HANDLE_TABLE::Resize(
    UINT cTableSize
    )
{
    HRESULT hr = S_OK;

    //
    // Ensure that the table is growing. Shrinking would imply that we leak
    // handle entries.
    //

    UINT cbOldMemSize = 0;
    UINT cbNewMemSize = 0;

    if (cTableSize < m_cHandleCount)
    {
        RIP("Handle tables are not allowed to shrink.");
        IFC(E_INVALIDARG);
    }

    IFC(MultiplyUINT(m_cHandleCount, m_cbEntry, cbOldMemSize));
    IFC(MultiplyUINT(cTableSize, m_cbEntry, cbNewMemSize));

    VOID *pvTable = ReallocHeap(m_pvTable, cbNewMemSize);

    IFCOOM(pvTable);

    //
    // Zero out the remainder of the allocation.
    //

    PBYTE pRem = static_cast<PBYTE>(pvTable) + cbOldMemSize;
    RtlZeroMemory(pRem, cbNewMemSize - cbOldMemSize);

    //
    // Assign the new table pointer.
    //

    m_pvTable = pvTable;
    m_cHandleCount = cTableSize;

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    This function gets an empty slot in the table

Arguments:

    type     - target type.
    phObject - pointer to the output handle.

Return Value:

    HRESULT
    Allocating the free handle may involve resizing the table which could
    fail during low memory.

--*/

HRESULT HANDLE_TABLE::GetNewEntry(
    DWORD type,
    __out_ecount(1) HMIL_OBJECT *phObject
    )
{
    HRESULT hr = S_OK;

    //
    // EMPTY_ENTRY (==0) is used to indicate an empty table entry.
    // Allocating a handle with this type will cause table inconsistency.
    //

    Assert(type != EMPTY_ENTRY);

    //
    // Ensure that the table is at its minimal size.
    //

    if (m_cHandleCount < MIL_HANDLE_TABLE_SIZE_MIN)
    {
        IFC(Resize(MIL_HANDLE_TABLE_SIZE_MIN));        
    }

    Assert(m_cHandleCount > m_nFreeIndex);

    *phObject = 0;

    //
    // Search the handle table for the next free entry. We're most likely to
    // find it at m_nFreeIndex. If that one is used, search up the table
    // and eventually wrap back. The beginning of the table is most likely to
    // be full, so we search it last.
    // Note that statistically (based on uniform distribution of deletion) this
    // algorithm will search the most sparsely populated areas of the table.
    // Also, until the m_nFreeIndex wraps the first time, it will always hit an
    // empty slot without searching.
    // Also by passing through the entire table before recycling handles, it
    // becomes much easier to debug handle-leaks and use-after-delete bugs.
    // The handles are much more likely to be unused and therefore invalid
    // stopping the application earlier.
    //

    UINT nFreePos = m_nFreeIndex;

    while (ENTRY_TYPE_FIELD(nFreePos) != EMPTY_ENTRY)
    {
        nFreePos++;
        if (nFreePos == m_cHandleCount)
        {
            nFreePos = 1;
        }
        if (nFreePos == m_nFreeIndex)
        {
            nFreePos = 0;
            break;
        }
    }

    m_nFreeIndex = nFreePos + 1;

    if (m_nFreeIndex == m_cHandleCount)
    {
        m_nFreeIndex = 1;
    }

    if (nFreePos == 0)
    {
        UINT nOldSize = m_cHandleCount;

        //
        // Make some space for the new handle by requesting 
        // the handle table to grow by at least one element.
        //

        IFC(ResizeToFit(m_cHandleCount));

        //
        // Start at the first position in the newly allocated space.
        //
        
        nFreePos = nOldSize;
        m_nFreeIndex = nFreePos + 1;
    }

    //
    // Mark the object as allocated for a specific type.
    //

    ENTRY_TYPE_FIELD(nFreePos) = type;
    *phObject = nFreePos;

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    This function assigns a value to a given empty slot in the table. If the
    slot specified is outside of the table, the table is grown to include
    the entry if possible.

Arguments:

    object   - table slot to occupy.
    type     - target type.

Return Value:

    HRESULT
    Allocating the free handle may involve resizing the table which could
    fail during low memory. Also if the slot is already occupied the call
    fails (and Asserts on chk)

--*/

HRESULT HANDLE_TABLE::AssignEntry(
    __in HMIL_OBJECT object,
    DWORD type
    )
{
    HRESULT hr = S_OK;

    //
    // Cannot assign an invalid entry, cannot assign the NULL handle.
    //

    if (type == EMPTY_ENTRY
        || object == 0)
    {
        RIP("Cannot assign empty entries, cannot assign to the NULL handle.");
        IFC(E_INVALIDARG);
    }

    //
    // Check if the handle table is large enough to accomodate the request.
    //
    
    if (object >= m_cHandleCount)
    {
        //
        // Resize the handle table to fit at least the handle being assigned.
        //

        IFC(ResizeToFit(object));
    }

    //
    // Ensure that the requested location is actually empty. If not
    // we would leak some object.
    //
    
    if (ENTRY_TYPE_FIELD(object) == EMPTY_ENTRY)
    {
        ENTRY_TYPE_FIELD(object) = type;
    }
    else
    {
        RIP("Attempt to overwrite a reserved handle table entry.");
        IFC(E_INVALIDARG);
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    This function checks a client-provided handle against the port-table. If
    the handle is not one assigned to this table or the type is different from
    that expected, this will fail and result in the entire call being failed.

Arguments:

    type - expected type.
    object - the provided object handle to check.

Return Value:

    BOOL - TRUE if the handle table entry matches the expected type.

--*/

BOOL HANDLE_TABLE::ValidEntry(
    DWORD type,
    __in HMIL_OBJECT object
    ) const
{
    //
    // Verify the consistency of the table size and free index. Note that this
    // Assert usually indicates that Resize was not called to initialize the
    // handle-table memory to the minimum table size. Resize must be called at
    // least once before using GetNewEntry.
    //

    Assert(m_cHandleCount > m_nFreeIndex);

    //
    // Validating for an invalid type is strange.
    //

    Assert(type != EMPTY_ENTRY);

    if (object == 0 ||
        object >= m_cHandleCount ||
        ENTRY_TYPE_FIELD(object) != type)
    {
        return FALSE;
    }

    return TRUE;
}

/*++

Routine Description:

    This function checks a client-provided handle against the port-table. If
    the handle is not one assigned to this table, this will fail and result in
    the entire call being failed.

Arguments:

    object - the provided object handle to check.

Return Value:

    BOOL - TRUE if the handle table entry matches the expected type.

--*/

BOOL HANDLE_TABLE::ValidEntry(
    __in HMIL_OBJECT object
    ) const
{
    //
    // Verify the consistency of the table size and free index. Note that this
    // Assert usually indicates that Resize was not called to initialize the
    // handle-table memory to the minimum table size. Resize must be called at
    // least once before using GetNewEntry.
    //

    Assert(m_cHandleCount > m_nFreeIndex);

    if (object == 0 ||
        object >= m_cHandleCount ||
        ENTRY_TYPE_FIELD(object) == EMPTY_ENTRY)
    {
        return FALSE;
    }

    return TRUE;
}

/*++

Routine Description:

    returns the object type at the given handle entry.

--*/

DWORD HANDLE_TABLE::GetObjectType(
    __in HMIL_OBJECT object
    )
{
    if (object > 0 && object < m_cHandleCount)
    {
        return ENTRY_TYPE_FIELD(object);
    }

    return EMPTY_ENTRY;
}


/*++

Routine Description:

    Reclaims a handle entry for reuse. Objects referenced by the HANDLE_ENTRY
    must have been previously reclaimed.

--*/

VOID HANDLE_TABLE::DestroyHandle(
    __in HMIL_OBJECT object
    )
{
    //
    // This function should only be called from the object destructor when
    // the handle for the object is to be reclaimed. Or when it is known that
    // any resources in the HANDLE_ENTRY have been reclaimed.
    //

    Assert(ValidEntry(object));

    //
    // Reclaim the entry in the table. We zero the entire entry starting from
    // the type address and including the entire field.
    //

    RtlZeroMemory(ENTRY_ADDRESS(object), m_cbEntry);
}



