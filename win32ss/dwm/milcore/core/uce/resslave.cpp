// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+------------------------------------------------------------------------
//

//
// Description: Composition Base Resource Class
//
//-------------------------------------------------------------------------

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
// CMilSlaveResource::~CMilSlaveResource
//
//   Registers this resource as a listener with the specified pResource.
//-------------------------------------------------------------------------

CMilSlaveResource::~CMilSlaveResource()
{
}

//+------------------------------------------------------------------------
//
// CMilSlaveResource::RegisterNotifier
//
//   Registers this resource as a listener with the specified pResource.
//-------------------------------------------------------------------------

/* virtual */
HRESULT
CMilSlaveResource::RegisterNotifier(
    __in_ecount_opt(1) CMilSlaveResource* pNotifier
    )
{
    HRESULT hr = S_OK;

    if (pNotifier != NULL)
    {
        IFC(pNotifier->m_rgpListener.Add(this));
        pNotifier->AddRef();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//   CMilSlaveResource::UnRegisterNotifierInternal
//
//   Unregisters this resource from the specified pResource. Note that
//   this resource must have been added before as a listener to the
//   specified resource.
//
//   Note that we do not overwrite the notifier with NULL -- this is has
//   to be done in UnRegisterNotifier and UnRegisterNNotifiersInternal.
//
//-------------------------------------------------------------------------

/* virtual */
VOID
CMilSlaveResource::UnRegisterNotifierInternal(
    __in_ecount_opt(1) CMilSlaveResource *pNotifier
    )
{
    if (pNotifier != NULL)
    {
        if (pNotifier->m_rgpListener.Remove(this))
        {
            pNotifier->Release();
        }
    }
}

//+------------------------------------------------------------------------
//
//  CMilSlaveResource::OnChanged
//
//  Return value:
//     If false, the notification is not bubbled up any further in the
//     resource graph.
//
//-------------------------------------------------------------------------

BOOL
CMilSlaveResource::OnChanged(
    CMilSlaveResource *pSender,
    NotificationEventArgs::Flags e)
{
    return TRUE;
}

//+------------------------------------------------------------------------
//
// CMilSlaveResource::NotifyOnChanged()
//
//-------------------------------------------------------------------------

VOID
CMilSlaveResource::NotifyOnChanged(CMilSlaveResource *pSender, NotificationEventArgs::Flags e)
{
    // Fire the OnChanged event. If the OnChanged handler returns false, the
    // notification propagation stops here.
    if (EnterResource() && OnChanged(pSender, e))
    {
        CMilSlaveResource *pListener = NULL;
        CPtrMultiset<CMilSlaveResource>::Enumerator elt = m_rgpListener.GetEnumerator();
        while (NULL != (pListener = elt.MoveNext()))
        {
            pListener->NotifyOnChanged(this, e);
        }
    }

    LeaveResource();
}

//+------------------------------------------------------------------------
//
//  CMilSlaveResource::RegisterNNotifiersInternal
//
//     Eegisters a specified number of dependents in an atomic way, i.e.
//     either all of them are registered or none are.
//
//-------------------------------------------------------------------------

HRESULT CMilSlaveResource::RegisterNNotifiersInternal(
    __in_ecount(n) CMilSlaveResource **prgResource,
    UINT n
    )
{
    HRESULT hr = S_OK;

    UINT i = 0;

    for (i = 0; i < n; i++)
    {
        MIL_THR(RegisterNotifier(prgResource[i]));

        if (FAILED(hr))
        {
            break;
        }
    }

    if (FAILED(hr))
    {
        Assert(i < n);

        //
        // unregister all the ones that have been registered
        // effectively rollback the transaction.
        //

        UnRegisterNNotifiersInternal(prgResource, i);
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  CMilSlaveResource::UnRegisterNNotifiersInternal
//
//  Note that we overwrite the array with NULL values as we go.
//
//+------------------------------------------------------------------------

void CMilSlaveResource::UnRegisterNNotifiersInternal(
    __inout_ecount(n) CMilSlaveResource **prgResource,
    UINT n
    )
{
    for (UINT i = 0; i < n; i++)
    {
        UnRegisterNotifierInternal(prgResource[i]);
        prgResource[i] = NULL;
    }
}



//+----------------------------------------------------------------------------
//
//  Class:
//      CMilCyclicResourceListEntry
//
//  Synopsis:
//      Specialization of CMilResourceListEntry class that always registers
//      and unregisters itself with a handle table as a cyclic resource.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:
//      CMilCyclicResourceListEntry::CMilCyclicResourceListEntry
//
//  Synopsis:
//      Cyclic resource constructor registers itself with handle table.
//
//-----------------------------------------------------------------------------

CMilCyclicResourceListEntry::CMilCyclicResourceListEntry(
    __inout_ecount(1) CMilSlaveHandleTable *pHTable
    )
{
    MarkAsUnlisted();
    pHTable->RegisterCyclicResource(this);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CMilCyclicResourceListEntry::~CMilCyclicResourceListEntry
//
//  Synopsis:
//      Cyclic resource destructor which simply takes care to remove itself
//      from any list it may have been added to.
//
//-----------------------------------------------------------------------------

CMilCyclicResourceListEntry::~CMilCyclicResourceListEntry(
    )
{
    RemoveEntryList(this);
}



