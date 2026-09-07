// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//     Implementation of the client-side resource handle table.
// 
//  Notes:
//     This implementation is not thread safe.
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"
#include "WPFEventTrace.h"

MtDefine(CMilSlaveHandleTable, Mem, "CMilSlaveHandleTable");


//+----------------------------------------------------------------------------
//
//  Member:    CMilSlaveHandleTable::CreateEmptyResource
//
//  Synopsis:  Create a resource and places it in the handle table
//             at the specified handle.
//
//-----------------------------------------------------------------------------

HRESULT CMilSlaveHandleTable::CreateEmptyResource(
    __in CComposition* pDevice,
    __in CMilServerChannel* pChannel,
    __in const MILCMD_CHANNEL_CREATERESOURCE* pCmd,
    __deref_out_ecount(1) CMilSlaveResource **ppResource
    )
{
    HRESULT hr = S_OK;

    CMilSlaveResource *pResource = NULL;
    HANDLE_ENTRY *pHandleEntry = NULL;

    //
    // Allocate a handle and create the requested resource...
    //

    IFC(AllocateEntryAtHandle(
        pCmd->Handle, 
        pCmd->resType, 
        &pHandleEntry
        ));

    IFC(CResourceFactory::Create(
        pDevice,
        this,
        pCmd->resType,
        &pResource
        ));

    //
    // Initialize the resource and store it in the handle table
    //

    IFC(pResource->Initialize());
    IFC(InitializeHandle(pCmd->Handle, pResource));


    //
    // Done, return the pointer to the resource.
    //

    *ppResource = pResource;

    EventWriteCreateWpfGfxResource(pResource, pChannel->GetChannel(), pCmd->Handle, pCmd->resType);

    pResource = NULL; // Transfering the ref-count to the out argument.
    pHandleEntry = NULL;

Cleanup:
    if (FAILED(hr)) 
    {
        //
        // We did try to allocate a handle and/or create a resource and failed.
        // Deallocate the handle and release the resource (InitializeHandle
        // addref'ed the resource, DeleteHandle will take care of that).
        //

        if (pHandleEntry != NULL) 
        {
            //
            // It is safe to ignore the DeleteHandle result as it can only fail
            // if the handle hasn't been allocated before...
            //

            IGNORE_HR(DeleteHandle(pCmd->Handle));
        }

        ReleaseInterface(pResource);
    }

    RRETURN(hr);
}

/**************************************************************************
*
*   CMilSlaveHandleTable::CMilSlaveHandleTable
*
**************************************************************************/

CMilSlaveHandleTable::CMilSlaveHandleTable() :
    m_handletable(sizeof(HANDLE_ENTRY)),
    m_pComposition(NULL),
    m_cyclicResourceList()
{
}

CMilSlaveHandleTable::~CMilSlaveHandleTable()
{
    //
    // Walk the handle table and release any resources that we have
    // failed to delete using MILCMD_CHANNEL_DELETERESOURCE path.
    //

    if (ReleaseHandleTableEntries(NULL))
    {
        TraceTag((tagMILWarning,
                  "CMilSlaveHandleTable::~CMilSlaveHandleTable: some resources have been leaked"
                  ));
    }    

    //
    // Now we free up any resource that was created and added to the
    // handle table and now still exists in memory because someone besides
    // the handle table has a reference to it.
    // eg. an island existing in resource graph.
    //
    BreakLinksForCyclicResources();
}


/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::GetObjectType()
*
*  returns the type of the object
***************************************************************************/

MIL_RESOURCE_TYPE CMilSlaveHandleTable::GetObjectType(HMIL_OBJECT object)
{
    return (MIL_RESOURCE_TYPE)m_handletable.GetObjectType(object);
}

/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::GetResource()
*
* Description:
*    Gets the resource at the specified handle ensuring it is of the
*    requested type
*
**************************************************************************/

__out_ecount_opt(1) CMilSlaveResource *
CMilSlaveHandleTable::GetResource(
    HMIL_RESOURCE hres,
    MIL_RESOURCE_TYPE type
    ) const
{
    if (m_handletable.ValidEntry(hres))
    {
        HANDLE_ENTRY *pEntry = (HANDLE_ENTRY*)ENTRY_RECORD(m_handletable, hres);

        if (pEntry->pResource != NULL
            && pEntry->pResource->IsOfType(type)) 
        {
            return pEntry->pResource;
        }
    }

    return NULL;
}

/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::AllocateEntryAtHandle
*
* Description:
*   allocates a specific handle entry
*
**************************************************************************/

HRESULT CMilSlaveHandleTable::AllocateEntryAtHandle(
    HMIL_RESOURCE hres,
    MIL_RESOURCE_TYPE type,
    HANDLE_ENTRY **ppEntry
    )
{
    //
    // Assign the table index to this new resource. Grow the table if necessary.
    //

    HRESULT hr = THR(m_handletable.AssignEntry(hres, type));

    if (SUCCEEDED(hr))
    {
        //
        // Retrieve the address of the entry.
        //

        HANDLE_ENTRY *pEntry = (HANDLE_ENTRY*)ENTRY_RECORD(m_handletable, hres);

        //
        // Initialize the fields.
        //

        pEntry->pResource = NULL;

        *ppEntry = pEntry;
    }

    RRETURN(hr);
}

/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::InitializeHandle
*
* Description:
*   Initializes the handle's entry with the resources data
*
**************************************************************************/

HRESULT CMilSlaveHandleTable::InitializeHandle(
    HMIL_RESOURCE hResource,
    CMilSlaveResource *pResource
    )
{
    HRESULT hr = E_HANDLE;

    Assert(pResource);
    Assert(m_handletable.ValidEntry(hResource));

    HANDLE_ENTRY *phe = GetEntry(hResource);
    if (phe)
    {
        hr = S_OK;
        Assert(phe->type != TYPE_NULL);
        Assert(phe->pResource == NULL);

        phe->pResource = pResource;
        pResource->AddRef();
    }
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member: CMilSlaveHandleTable::BreakLinksForCyclicResources
//
//  Synopsis:
//
//      Resources that can be used to create cycles are registered in the 
//      m_cyclicResourceList. Because we are using a ref-counting scheme 
//      for life-time management, cyclic resource graphs might never get released. 
//      To break cyclic resource graphs we are going through the list of
//      all resources that can introduce cycles and release the resources
//      they are referencing thus breaking up any cycles.
//
//------------------------------------------------------------------------

void CMilSlaveHandleTable::BreakLinksForCyclicResources()
{
    while (!m_cyclicResourceList.IsEmpty())
    {
        CMilCyclicResourceListEntry *pCurrent = m_cyclicResourceList.RemoveHeadEntry();

        CMilSlaveResource *pResource = pCurrent->GetResource();
        Assert(pResource);

        //
        //  We AddRef it, so pResource does not get deleted while we are in its
        //  unregister notifier function.  The function expects the caller to
        //  have a reference and does not protect itself against being freed in
        //  the middle of doing its work.
        //
        pResource->AddRef();
        pResource->UnRegisterNotifiers();
        pResource->NotifyOnChanged(pResource);
        pResource->Release();
    }
}


//+-----------------------------------------------------------------------
//
//  Member: CMilSlaveHandleTable::DuplicateHandle
//
//  Synopsis:
//
//    Duplicates a handle between channels of a partition.
//
//  Returns: HRESULT
//
//------------------------------------------------------------------------

HRESULT CMilSlaveHandleTable::DuplicateHandle(
    __in CMilServerChannel* pSourceChannel,
    HMIL_RESOURCE hOriginal,
    __in CMilServerChannel* pTargetChannel,
    HMIL_RESOURCE hDuplicate
    )
{
    HRESULT hr = S_OK;

    HANDLE_ENTRY* pOriginalEntry = NULL;
    HANDLE_ENTRY* pDuplicateEntry = NULL;

    //
    // Allocate the duplicated entry (the handle table storage could get
    // reallocated, so do not fetch the original entry before allocation). 
    //
    CMilSlaveHandleTable *pTargetHandleTable = pTargetChannel->GetChannelTable();
    IFC(pTargetHandleTable->AllocateEntryAtHandle(
        hDuplicate,
        GetObjectType(hOriginal),
        &pDuplicateEntry
        ));


    //
    // Fetch and validate the original entry.
    //

    pOriginalEntry = GetEntry(hOriginal);

    CHECKPTR(pOriginalEntry);
    CHECKPTR(pOriginalEntry->pResource);


    //
    // Duplicate by copying the resource pointer and addrefing it -- as
    // easy as this thanks to the fact that we only duplicate within
    // a partition.
    //

    pDuplicateEntry->pResource = pOriginalEntry->pResource;
    pDuplicateEntry->pResource->AddRef();

Cleanup:
    RRETURN(hr);
}


/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::DeleteHandle
*
* Description:
*   Releases the handle in the table and deletes the resource
*
**************************************************************************/

HRESULT CMilSlaveHandleTable::DeleteHandle(HMIL_RESOURCE hResource)
{
    HRESULT hr = E_HANDLE;

    HANDLE_ENTRY *phe = GetEntry(hResource);
    if (phe && phe->type != TYPE_NULL)
    {
        if (phe->pResource) 
        {
            phe->pResource->Release();
            phe->pResource = NULL;
        }

        //
        // clear the entry
        //

        m_handletable.DestroyHandle(hResource);

        hr = S_OK;
    }

    RRETURN(hr);
}


/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::GetEntry
*
* Description:
*   Retrieves the entry from the table for the given resource
*
**************************************************************************/

CMilSlaveHandleTable::HANDLE_ENTRY *
CMilSlaveHandleTable::GetEntry(HMIL_RESOURCE hres)
{
    if (m_handletable.ValidEntry(hres))
    {
        HANDLE_ENTRY *pEntry = (HANDLE_ENTRY*)ENTRY_RECORD(m_handletable, hres);
        return pEntry;
    }

    return NULL;
}

/**************************************************************************
*
* Name:
*   CMilSlaveHandleTable::ReleaseAllResources
*
* Description:
*   Walks the table and releases all resources for a given channel.
*
**************************************************************************/

bool CMilSlaveHandleTable::ReleaseHandleTableEntries(
    __in_ecount_opt(1) const CComposition *pComposition
    )
{
    HANDLE_ENTRY *pEntry = NULL;
    bool fReleased = false;

    for (UINT i = 0; i < m_handletable.m_cHandleCount; i++)
    {
        pEntry = GetEntry(i);
        if (pEntry)
        {
            if (pEntry->pResource)
            {
                if (pComposition)
                {
                    //
                    //Future Consideration:  this code needs to be refactored so that
                    //  it is clear which resources belong to which composition. At the
                    //  same time, we should consider adding a cleanup method to the
                    //  resource interface.
                    //

                    IGNORE_HR(pComposition->ReleaseResource(
                                  this,
                                  static_cast<HMIL_RESOURCE>(i),
                                  pEntry->pResource,
                                  true // fCleanupShutdown
                                  ));
                }
                else
                {
                    ReleaseInterfaceNoNULL(pEntry->pResource);

                    m_handletable.DestroyHandle(i);
                }

                fReleased = true;
            }
        }
    }

    return fReleased;
}


