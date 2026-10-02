// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



//+--------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of the master handle table class. This handle table
//      resides on the server side and is used for bookkeeping handle
//      allocations. 
//
//----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilMasterHandleTable, Mem, "CMilMasterHandleTable");

CMilMasterHandleTable::CMilMasterHandleTable() :
    m_handletable(sizeof(HANDLE_ENTRY))
{
}


CMilMasterHandleTable::HANDLE_ENTRY *
CMilMasterHandleTable::GetEntry(
    HMIL_RESOURCE hResource
    ) const
{
    AssertConstMsg(
        (HandleToULong(g_csCompositionEngine.OwningThread()))== GetCurrentThreadId(),
        "Unsynchronized access to the handle-table"
        );

    if (m_handletable.ValidEntry(hResource))
    {
        return reinterpret_cast<HANDLE_ENTRY*>(ENTRY_RECORD(
            m_handletable,
            hResource
            ));
    }

    return NULL;
}

//+-----------------------------------------------------------------------
//
//  Member: CMilMasterHandleTable::CreateOrAddRefOnChannel
//
//  Synopsis:
//
//    This is actually two distinct functions which key off of the value
//    of the ph variable.
//    if *ph == NULL, then this function constructs a handle entry for a
//    new resource of the specified type and returns the handle to it.
//    if *ph != NULL, then this function addrefs the resource specified
//    and the type argument is unused.
//
//  Returns: HRESULT
//
//------------------------------------------------------------------------

HRESULT
CMilMasterHandleTable::CreateOrAddRefOnChannel(
    __in_ecount(1) CMilChannel* pChannel,
    MIL_RESOURCE_TYPE type,
    __inout_ecount(1) HMIL_RESOURCE* ph
    )
{
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    if (*ph == HMIL_RESOURCE_NULL)
    {
        //
        // Create case - input handle is NULL.
        // First reserve space in the handle table and mark it with the type.
        // This can fail for out of memory, so that must be handled.
        //

        Assert(type != TYPE_NULL);

        HMIL_RESOURCE handle = 0;

        MIL_THR(m_handletable.GetNewEntry(type, &handle));

        if (SUCCEEDED(hr))
        {
            //
            // Retrieve a pointer to the handle entry. This must be valid
            // because we just succeeded creating it, so assert that.
            //

            HANDLE_ENTRY *phe = GetEntry(handle);

            Assert(phe != NULL);
            IFCNULL(phe);

            //
            // Queue a packet to the composition device to create a
            // corresponding entry in its table. If this fails, undo the
            // handle entry creation and return an error.
            //

            MILCMD_CHANNEL_CREATERESOURCE create = {
                MilCmdChannelCreateResource,
                handle,
                phe->type
            };

            MIL_THR(pChannel->SendCommand(&create, sizeof(create)));

            if (SUCCEEDED(hr))
            {
                //
                // Everything worked, set up the entry properly and return the
                // new handle.
                //

                phe->refCount = 1;
                *ph = handle;
            }
            else
            {
                m_handletable.DestroyHandle(handle);
            }
        }
    }
    else
    {
        //
        // Reference case, ph contains an existing resource. Reference count it.
        //

        HANDLE_ENTRY *phe = GetEntry(*ph);

        AssertMsg(phe != NULL, "The handle passed in must be a valid handle.");
        IFCNULL(phe);

        phe->refCount++;
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//    Member: 
//        CMilMasterHandleTable::DuplicateHandle
//
//    Synopsis:
//        Duplicates a handle between channels of a partition.
//
//        Duplication is only allowed within the same partition
//        (see the channel level DuplicateHandle).
//
//------------------------------------------------------------------------

HRESULT CMilMasterHandleTable::DuplicateHandle(
    __in_ecount(1) CMilChannel* pSourceChannel,
    HMIL_RESOURCE hOriginal,
    __in_ecount(1) CMilChannel* pTargetChannel,
    __out_ecount(1) HMIL_RESOURCE* phDuplicate
    )
{
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    const HANDLE_ENTRY* pOriginalEntry = NULL;
    HANDLE_ENTRY* pDuplicateEntry = NULL;
    HMIL_RESOURCE hDuplicate = NULL;


    //
    // Obtain the original entry from the handle table.
    //

    pOriginalEntry = GetEntry(hOriginal);
    IFCNULL(pOriginalEntry);


    //
    // Get the target channel's Master Handle Table and
    // reserve space in the handle table and mark it with the type.
    //

    CMilMasterHandleTable *pTargetHandleTable = &pTargetChannel->m_handleTable;
    IFC(pTargetHandleTable->m_handletable.GetNewEntry(pOriginalEntry->type, &hDuplicate));


    //
    // Retrieve a pointer to the handle entry. This must be valid
    // because we just succeeded creating it, so assert that.
    //

    pDuplicateEntry = pTargetHandleTable->GetEntry(hDuplicate);
    IFCNULL(pDuplicateEntry);


    //
    // Queue a packet to the composition device to perform duplication
    // on its table. If this fails, undo the handle entry creation and 
    // return an error.
    //

    {
        MILCMD_CHANNEL_DUPLICATEHANDLE cmd = 
            {
                MilCmdChannelDuplicateHandle,
                hOriginal,                      // original handle
                pTargetChannel->GetChannel(),   // target channel's handle
                hDuplicate                      // duplicated handle
            };

        MIL_THR(pSourceChannel->SendCommand(
                    &cmd, 
                    sizeof(cmd)
                    ));

        if (SUCCEEDED(hr))
        {
            //
            // Everything worked, set up the entry properly 
            // and return the new handle.
            //

            pDuplicateEntry->refCount = 1;
            *phDuplicate = hDuplicate;
        }
        else
        {
            pTargetHandleTable->m_handletable.DestroyHandle(hDuplicate);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member: CMilMasterHandleTable::ReleaseOnChannel
//
//  Synopsis:
//
//    Decrement the reference count on the specified handle entry specified
//    by the h argument. If the ref count goes to zero the resource is deleted
//    and pfDeleted is set to TRUE.
//
//    pfDeleted is an optional argument and can be set to NULL.
//
//  Returns: HRESULT
//
//------------------------------------------------------------------------

HRESULT
CMilMasterHandleTable::ReleaseOnChannel(
    __in_ecount(1) CMilChannel* pChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) BOOL* pfDeleted
    )
{
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    Assert(h != HMIL_RESOURCE_NULL);

    if (pfDeleted)
    {
        *pfDeleted = FALSE;
    }

    HANDLE_ENTRY *phe = GetEntry(h);

    if (phe == NULL || phe->refCount <= 0)
    {
        //
        // If this ever happens we would end up in a corrupted state that is difficult to debug.
        //
        
        MilUnexpectedError(E_UNEXPECTED, TEXT("ReleaseOnChannel was called on a resource that is not anymore on this channel"));

        IFC(E_UNEXPECTED);
    }

    if (phe && phe->type != EMPTY_ENTRY)
    {
        if (phe->refCount == 1)
        {
            //
            // Queue a packet to destroy the slave handle entry.
            //

            MILCMD_CHANNEL_DELETERESOURCE del = 
                {
                    MilCmdChannelDeleteResource, 
                    h, 
                    phe->type
                };

            IFC(pChannel->SendCommand(&del, sizeof(del)));

            //
            // Clear the master handle table entry. This is only done if we
            // are successful at queueing a release to the composition device.
            // If this fails we return an error and the handle remains
            // allocated. If we were to destroy this handle entry under this
            // condition, then later we'd have an object collision in the
            // composition device handle-table due to an allocation at the same
            // location.
            //
            // We should be clearing out the master table entry here but
            // because of multi-channel issues, we can't. See the comment
            // on the definition of the pChannel field in the header file.
            //
            // m_handletable.DestroyHandle(h);
            //
            // Instead we mark the entry as unusable for a short duration
            // till the channel is flushed by linking it into a per-channel
            // free list.
            //

            phe->idxFree = pChannel->GetFreeIndex();
            pChannel->SetFreeIndex(h);

            if (pfDeleted)
            {
                *pfDeleted = TRUE;
            }
        }

        //
        // Only decrease the reference count if we've haven't failed to send
        // the MILCMD_RESOURCE_DELETE command. 
        // 
        // The most probable cause of a failure is an out of memory situation.
        // It's better to leak a handle in such case and hope that we will
        // clean the partition up later than risk desynchronizing handle tables.
        // 

        --phe->refCount;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member: CMilMasterHandleTable::GetRefCountOnChannel
//
//  Synopsis:
//
//    Returns the reference count on the specified handle entry specified
//    by the h argument. 
//
//  Returns: HRESULT
//
//------------------------------------------------------------------------

HRESULT
CMilMasterHandleTable::GetRefCountOnChannel(
    __in_ecount(1) CMilChannel* pChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) UINT *pcRefs
    )
{
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    Assert(h != HMIL_RESOURCE_NULL);

    const HANDLE_ENTRY *phe = GetEntry(h);

    if (phe == NULL || phe->refCount < 0)
    {
        //
        // If this ever happens we would end up in a corrupted state that is difficult to debug.
        //
        
        MilUnexpectedError(E_UNEXPECTED, TEXT("GetRefCountOnChannel was called on a resource that is not anymore on this channel"));

        IFC(E_UNEXPECTED);
    }

    if (phe->type != EMPTY_ENTRY)
    {
        *pcRefs = phe->refCount;
    }
    else
    {
        *pcRefs = 0;
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member: CMilMasterHandleTable::FlushChannelHandles
//
//  Synopsis:
//
//    Flush any handles blocked from deletion on this channel. They are
//    blocked till the channel is able to flush any pending delete commands
//    to the slave.
//
//  Returns: none
//
//------------------------------------------------------------------------

VOID
CMilMasterHandleTable::FlushChannelHandles(
    UINT idxFree
    )
{
    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    if (idxFree > 0)
    {
        //
        // Array of HANDLE_ENTRY structs starting from the beginning of the table.
        //

        const HANDLE_ENTRY *phe = 
            reinterpret_cast<HANDLE_ENTRY*>(m_handletable.m_pvTable);

        //
        // Loop over all the handles, freeing up any entries blocked for this
        // channel's flush command.
        //

        while (idxFree != 0)
        {
            Assert(phe[idxFree].refCount==0);
            UINT idxNext = phe[idxFree].idxFree;
            m_handletable.DestroyHandle(idxFree);
            idxFree = idxNext;
        }
    }
}


