// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//      Declaration of the master handle table class. This handle table
//      resides on the server side and is used for bookkeeping handle
//      allocations. 
//
//----------------------------------------------------------------------------

MtExtern(CMilMasterHandleTable);


//+---------------------------------------------------------------------------
//
//    Class: 
//        CMilMasterHandleTable
//
//    Synopsis:
//        Server side handle table. Exposes resource handle creation,
//        duplication and destruction for MilCore clients.
//
//----------------------------------------------------------------------------

class CMilMasterHandleTable
{
public:
    CMilMasterHandleTable();

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilMasterHandleTable));


    HRESULT CreateOrAddRefOnChannel(
        __in_ecount(1) CMilChannel* pChannel,
        MIL_RESOURCE_TYPE type,
        __inout_ecount(1) HMIL_RESOURCE *ph
        );

    HRESULT DuplicateHandle(
        __in_ecount(1) CMilChannel* pSourceChannel,
        HMIL_RESOURCE hOriginal,
        __in_ecount(1) CMilChannel* pTargetChannel,
        __out_ecount(1) HMIL_RESOURCE* phDuplicate
        );

    HRESULT ReleaseOnChannel(
        __in_ecount(1) CMilChannel* pChannel,
        HMIL_RESOURCE h,
        __out_ecount_opt(1) BOOL *pfDeleted
        );

    HRESULT GetRefCountOnChannel(
        __in_ecount(1) CMilChannel* pChannel,
        HMIL_RESOURCE h,
        __out_ecount_opt(1) UINT *pcRefs
        );

    // this method exposes implementation details 
    //    and should not be public (see CMilChannel::Commit for
    //    the current use pattern).
    VOID FlushChannelHandles(UINT idxFree);

private:

    struct HANDLE_ENTRY
    {
        MIL_RESOURCE_TYPE type; // type must be the first field in the entry.
        UINT refCount;

        //
        // We use two handle-tables; one master and one slave and the values
        // of the handle entries must be kept in sync. Also, we share the
        // same handle-table across multiple channels. Because of this, an
        // entry may be deleted on one thread/channel, queued for submission
        // to the slave table, but not actually submitted before the handle
        // entry is reused on the master table via a different thread/channel.
        // The reuse may be submitted before the appropriate flush on the first
        // channel and hence cause a table collision in the slave.
        // In order to work around this problem, we keep the master handle
        // entry alive in this table and mark which channel 'owns' the dead
        // entry. When the channel is flushed, these entries are cleaned up
        // and reclaimed as empty handle entries.
        //

        UINT idxFree;
    };

    HANDLE_ENTRY *GetEntry(HMIL_RESOURCE hResource) const;

    //
    // The handle table data.
    //

    HANDLE_TABLE m_handletable;
};



