// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    htslave.h

Abstract:

    Description: Implementation of the slave handle table class.
        CMilSlaveHandleTable


--*/

MtExtern(CMilSlaveHandleTable);

class CComposition;

class CMilSlaveHandleTable :
    public CMILHandleTable
{
public:

    CMilSlaveHandleTable();
    ~CMilSlaveHandleTable();

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilSlaveHandleTable));


    //
    // Creates the resource and places it in the handle table
    // at the specified handle
    //

    HRESULT CreateEmptyResource(
        __in CComposition* pDevice,
        __in CMilServerChannel* pChannel,
        __in const MILCMD_CHANNEL_CREATERESOURCE* pCmd,
        __deref_out_ecount(1) CMilSlaveResource **ppResource
        );

    MIL_RESOURCE_TYPE GetObjectType(HMIL_OBJECT object);

    __out_ecount_opt(1) CMilSlaveResource *GetResource(
        HMIL_RESOURCE hResource,
        MIL_RESOURCE_TYPE type
        ) const;


    BOOL IsValidResource(
        HMIL_RESOURCE hResource,
        MIL_RESOURCE_TYPE type,
        BOOL fHandleMustExist = TRUE
        ) const
    {
        // Return true if handle is NULL but isn't required or
        return ((HMIL_RESOURCE_NULL == hResource) && !fHandleMustExist) ||
                // GetResource returns a valid resource
                (GetResource(hResource, type) != NULL);
    }

    HRESULT DuplicateHandle(
        __in CMilServerChannel* pSourceChannel,
        HMIL_RESOURCE hOriginal,
        __in CMilServerChannel* pTargetChannel,
        HMIL_RESOURCE hDuplicate
        );

    HRESULT DeleteHandle(HMIL_RESOURCE hResource);

    void SetComposition(CComposition * pComposition) { m_pComposition = pComposition; }
    CComposition *GetComposition() { return m_pComposition; }

    bool ReleaseHandleTableEntries(
        __in_ecount_opt(1) const CComposition *pComposition
        );

    void RegisterCyclicResource(
        __inout_ecount(1) CMilCyclicResourceListEntry *cyclicResource
        )
    {
        m_cyclicResourceList.InsertAtHead(cyclicResource);
    }

protected:

    //
    // Helper method for render target creation.
    //

    HRESULT InitializeHandle(
        HMIL_RESOURCE hResource,
        CMilSlaveResource *pResource
        );

protected:

    struct HANDLE_ENTRY
    {
        MIL_RESOURCE_TYPE type; // type must be the first field in the entry.
        CMilSlaveResource *pResource;
    };

    HRESULT AllocateEntryAtHandle(
        HMIL_RESOURCE hres,
        MIL_RESOURCE_TYPE type,
        HANDLE_ENTRY **ppHandle
        );

    HANDLE_ENTRY *GetEntry(HMIL_RESOURCE hResource);

    //
    // The handle table data.
    //

    HANDLE_TABLE m_handletable;
    CComposition *m_pComposition;

private:

    //
    // List that is used to track resources which can create cycles. We need to
    // keep this list since these resources might still be alive during
    // shutdown (because of cyclic reference).
    //
    CDoubleLinkedList<CMilCyclicResourceListEntry> m_cyclicResourceList;

    void BreakLinksForCyclicResources();
};



