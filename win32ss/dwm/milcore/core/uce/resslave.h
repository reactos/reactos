// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+------------------------------------------------------------------------
//

//
// Description: Composition Base Resource Class
//
//-------------------------------------------------------------------------

#pragma once

MtExtern(CMilSlaveResource);

class CMilSlaveHandleTable;

//+------------------------------------------------------------------------
//
// Enum: NotificationEventArgs [Flags]
//
// Description:
//     Enumeration of possible notification event arguments.
//
//-------------------------------------------------------------------------

class NotificationEventArgs
{
private:
    // Make the default ctor inaccessible to prevent inheritance.
    NotificationEventArgs(){}

public:
    enum Flags
    {
        None                                     = 0,

        // HasSubDirtyRegion - Indicates that there is a sub dirty region
        // with-in the resource.
        HasSubDirtyRegion                        = 1,
    };
};

//+------------------------------------------------------------------------
//
// CMilSlaveResource
//
//-------------------------------------------------------------------------

class CMilSlaveResource : public CMILCOMBase
{
protected:
    CMilSlaveResource()
    {
        RtlZeroMemory(&m_flags, sizeof(m_flags));
    }

    virtual ~CMilSlaveResource();

    // Provided for CSlaveEtwEventResource which has an initialization step
    // that is not guarenteed to succeed and does not have a unique CREATE
    // packet to recognize its first ProcessPacket call.
    virtual HRESULT Initialize() { return S_OK; }

public:
    friend class CMilSlaveHandleTable;

    DECLARE_COM_BASE

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppvObject)
    {
        return E_NOTIMPL;
    }

    virtual bool IsOfType(MIL_RESOURCE_TYPE type) const = 0;


    //
    // Tell the specified resource to notify me when it is changing (OnChanging)
    // and when it has changed (OnChanged).  (Master resources also use
    // this method for ref-counting.)
    //

    HRESULT RegisterNotifier(__in_ecount_opt(1) CMilSlaveResource* pNotifier);


    //
    // Unregisters a notifier. The resource will be released and the pointer
    // to the resource object will be set to NULL.
    //

    template <class TResourceType>
    void UnRegisterNotifier(__deref_inout_ecount_opt(1) TResourceType*& pNotifier)
    {
        //
        // Ensure that the specified type inherits from CMilSlaveResource. The
        // following line doesn't generate any code in debug or retail but it
        // emits a compile-time error if TResourceType doesn't inherit from
        // CMilSlaveResource.
        //
        static_cast<CMilSlaveResource *>(static_cast<TResourceType *>(NULL));

        UnRegisterNotifierInternal(pNotifier);

        pNotifier = NULL;
    }

    //
    // Derived classes override this and call UnRegisterNotifier for each
    // of their member fields.
    //

    virtual void UnRegisterNotifiers()
    {
    }


    //
    // Atomic registration of multiple resources
    //

    template <class TResourceType>
    inline HRESULT RegisterNNotifiers(__in_ecount(n) TResourceType **prgNotifiers, UINT n)
    {
        //
        // Ensure that the specified type inherits from CMilSlaveResource. The
        // following line doesn't generate any code in debug or retail but it
        // emits a compile-time error if TResourceType doesn't inherit from
        // CMilSlaveResource.
        //
        static_cast<CMilSlaveResource *>(static_cast<TResourceType *>(NULL));

        return RegisterNNotifiersInternal(reinterpret_cast<CMilSlaveResource **>(prgNotifiers), n);
    }

    template <class TResourceType>
    inline void UnRegisterNNotifiers(__inout_ecount(n) TResourceType **prgNotifiers, UINT n)
    {
        //
        // Ensure that the specified type inherits from CMilSlaveResource. The
        // following line doesn't generate any code in debug or retail but it
        // emits a compile-time error if TResourceType doesn't inherit from
        // CMilSlaveResource.
        //
        static_cast<CMilSlaveResource *>(static_cast<TResourceType *>(NULL));

        UnRegisterNNotifiersInternal(reinterpret_cast<CMilSlaveResource **>(prgNotifiers), n);
    }

    //
    // Notification of changes for registered resources
    //

    VOID NotifyOnChanged(CMilSlaveResource *pSender, NotificationEventArgs::Flags e);
    VOID NotifyOnChanged(CMilSlaveResource *pSender)
    {
        NotifyOnChanged(pSender, NotificationEventArgs::None);
    }

    //
    // The dirty flag
    //

    void SetDirty(BOOL fDirty)
    {
        m_flags.Dirty = (fDirty != FALSE);
    }

    BOOL IsDirty() const
    {
        return m_flags.Dirty;
    }

    //+------------------------------------------------------------------------
    //  Member:
    //      CMilSlaveResource::EnterResource
    //
    //  Synopsis:
    //      This is used for cycle detection. Currently we ignore cycles.
    //      A count is maintained. The count can only go upto 2 as when the
    //      resource tries to enter the second time (loop!!!) it should not
    //      be able to enter and LeaveResource() should be called.
    //      Each call to this function should match a call to LeaveResource()
    //
    //  Example Usage:
    //      To implement this check for cycles, these functions are used as follows:-
    //      if (EnterResource())
    //      {
    //          ...
    //
    //      }
    //
    //      LeaveResource();
    //-------------------------------------------------------------------------
    bool EnterResource()
    {
        m_flags.cVisited++;
        Assert(m_flags.cVisited <= 2);
        return (m_flags.cVisited == 1);
    }

    void LeaveResource()
    {
        Assert(m_flags.cVisited >= 1);
        m_flags.cVisited--;
    }

    bool CanEnterResource() const
    {
        return (m_flags.cVisited == 0);
    }

private:

    //
    // The untyped notifier registration routines.
    //

    void UnRegisterNotifierInternal(__in_ecount_opt(1) CMilSlaveResource* pNotifier);

    HRESULT RegisterNNotifiersInternal(__in_ecount(n) CMilSlaveResource **prgNotifiers, UINT n);
    void UnRegisterNNotifiersInternal(__inout_ecount(n) CMilSlaveResource **prgNotifiers, UINT n);


protected:

    virtual BOOL OnChanged(CMilSlaveResource *pSender, NotificationEventArgs::Flags e);

    //
    // The following method is a helper used to convert handles in command
    // packets to pointers, by looking them up in the handle table. Because
    // command packets are packed to 1-byte boundaries, the handle pointer
    // argument may not be properly aligned to the machine word boundary.
    // The UNALIGNED keywork here is required for this to run correctly in
    // some CPU architectures. Note that there is a small run-time performance
    // cost to loading and storing unaligned data. In this case we are paying
    // this cost so that we can maximally compress the command protocol.
    //

    template <class TResourceType>
    HRESULT AddHandleToArrayAndReplace(
        __inout_ecount(1) HMIL_RESOURCE UNALIGNED *phObject,
        MIL_RESOURCE_TYPE resType,
        __in_ecount(1) DynArray<TResourceType *, TRUE> *prgpResource,
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable
        )
    {
        HRESULT hr = S_OK;

        if (*phObject)
        {
            // Grab the resource pointer from the handle table.
            TResourceType *pResource =
                static_cast<TResourceType*>(pHandleTable->GetResource(*phObject, resType));
            IFCNULL(pResource);

            // RegisterNotifier adds a reference to the resource. Since all resources in this array
            // are also registered we do no need to take another reference.
            IFC(RegisterNotifier(pResource));

            // We want to ensure that a given resource isn't in the dependency list twice.
            // However, in order to do this, we'd have to add an addition ref count on the object
            // to know when to remove the registered notifier.  For now, we'll allow duplicates in the
            // resource array because this will correctly address multi-use of dependents.
            MIL_THR(prgpResource->Add(pResource));

            if (SUCCEEDED(hr))
            {
                // Return the index into the resource array...
                *phObject = prgpResource->GetCount() - 1;
            }
            else
            {
                // Prevent leaks -- always unregister the resource on failure.
                UnRegisterNotifier(pResource);
            }
        }

    Cleanup:
        RRETURN(hr);
    }

protected:

    //
    // Store all the resources listening for changes to 'this' resource
    //

    CPtrMultiset<CMilSlaveResource> m_rgpListener;

    struct
    {
        //
        // The object state has been changed and now we need to re-render
        // it on the next render-pass.
        //

        UINT Dirty :1;

        //
        // To make sure if a loop exists, then notifications are not fired forever.
        // Eg:- loop can exist by Visual -> renderdata -> VisualBrush -> Visual.
        //

        UINT cVisited :2;
        
        //
        // Unused flags
        //

        UINT Reserved :27;

    } m_flags;
};


//+----------------------------------------------------------------------------
//
// CMilCyclicResourceListEntry
//
//  Synopsis:
//      A wrapper around LIST_ENTRY class which provides a virtual for getting
//      the CMilSlaveResource object.  Also handles registering and
//      unregistering with the handle table.
//
//-----------------------------------------------------------------------------

class CMilCyclicResourceListEntry : protected LIST_ENTRY
{
    friend class CDoubleLinkedList<CMilCyclicResourceListEntry>;

protected:
    CMilCyclicResourceListEntry(
        __inout_ecount(1) CMilSlaveHandleTable *pHTable
        );

    ~CMilCyclicResourceListEntry();


    void MarkAsUnlisted()
    {
        // Set Flink and Blink to 'this' to enable RemoveEntryList to be called
        // all the time without checking any special cases.
        Blink = Flink = this;
    }

    void RemoveFromList()
    {
        RemoveEntryList(this);
        MarkAsUnlisted();
    }

public:
    virtual CMilSlaveResource* GetResource() PURE;
};



