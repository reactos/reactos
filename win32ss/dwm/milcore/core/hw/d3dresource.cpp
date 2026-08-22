// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DResourceManager and CD3DResource implementation
//

#include "precomp.hpp"

// This meter tracks the total size of all d3d resource allocations.  Note that we
// put these in the metrics section so that we easily see the total usage for our 
// component in the working set section.
MtDefine(D3DResourceUsage, MILHwMetrics, "Approximate D3D resources");

#if DBG
#define AssertThreadProtection()  DbgAssertThreadProtection()
#else
#define AssertThreadProtection()
#endif

//+------------------------------------------------------------------------
//
//  Function:  CD3DResource::CD3DResource
//
//  Synopsis:  ctor
//
//             Note that we assume that RegisterResource was called
//             by the resource manager during creation, so the
//             ctor doesn't need to do it.
//
//-------------------------------------------------------------------------
CD3DResource::CD3DResource()
    : CMILPoolResource(NULL)
{
    m_uResourceSize = 0;
#if DBG_ANALYSIS
    m_uDbgFrameLastUsed = 0;
#endif
    m_uActiveDepthLastUsed = c_uDepthNotUsed;
    m_fResourceValid = false;
    m_fIsEvictable = false;
    m_sleFreeList.Next = NULL;
    m_leResourceList.Flink = NULL;
    m_leResourceList.Blink = NULL;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResource::~CD3DResource
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DResource::~CD3DResource()
{
    // The resource manager should have already been NULLed by a call to
    // CD3DResourceManager::DestroyResource.
    Assert(m_pManager == NULL);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResource::InitResource
//
//  Synopsis:  Set size and register with the resource manager
//
//-------------------------------------------------------------------------
void
CD3DResource::Init(
    __inout_ecount(1) IMILPoolManager *pManager,
    UINT uResourceSize
    )
{
    // Resource sizes can be 0 or greater.  We allow size 0 for classes that
    // don't directly hold D3D resource, but that hold onto other CD3DResources
    // and/or have a need to be destroyed when the device is destroyed.

    m_uResourceSize = uResourceSize;
    m_fResourceValid = true;

    CD3DResourceManager *pResourceManager =
        DYNCAST(CD3DResourceManager, pManager);
    Assert(pResourceManager);

    pResourceManager->RegisterResource(this);
    m_pManager = pManager;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResource::Manager and CD3DResource::Device
//
//  Synopsis:  Helper method to look up actual manager and device objects
//
//-----------------------------------------------------------------------------

__out_ecount(1)
CD3DResourceManager &CD3DResource::Manager() const
{
    Assert(m_pManager);
    return *DYNCAST(CD3DResourceManager, m_pManager);
}

CD3DDeviceLevel1 &CD3DResource::Device() const
{
    return Manager().Device();
}


//+------------------------------------------------------------------------
//
//  Member:    CD3DResource::DestroyAndRelease
//
//  Synopsis:  Remove this resource from the manager and call back to
//             ReleaseD3DResources to make sure all of the D3D resources
//             are actually cleaned up
//
//  Notes:     DestroyAndRelease may only be called under proper protection,
//             which is currently the active device thread.
//

void 
CD3DResource::DestroyAndRelease()
{
    Assert(GetRefCount() > 0);
    Assert(m_pManager);

    // Destroy the resource only if it's valid.  If it isn't valid it has
    // already been destroyed.
    if (m_fResourceValid)
    {
        //
        // Mark resource as invalid
        //

        m_fResourceValid = false;

        //
        // Notify the manager that this resource is now unusable
        //  (The manager will call back via ReleaseD3DResources.)
        //  The manager will assert threading protection.
        //

        m_pManager->UnusableNotification(this);
    }

    //
    // Now release the reference held by the caller, but since this resource
    // has been removed from the manager's control just delete this object.
    //
    // Note that we could still call Release() here, but that could be
    // confusing since Release() has a code path to call UnusedNotification,
    // which places resources on the free list.
    //

    if (InterlockedDecrementULONG(&m_cRef) == 0)
    {
        delete this;
    }

    return;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::SetAsEvictable
//
//  Synopsis:  Enables eviction for this resource
//
//-------------------------------------------------------------------------

void
CD3DResource::SetAsEvictable()
{
    Assert(IsValid());
   
    m_fIsEvictable = true;

    CD3DResourceManager &manager = Manager();

#pragma warning(push)
    //   Unexplained 64bit alignment error
    //  64-bit compilers appear to have a problem with passing *this to
    //  a method that takes a ref. See similar suppressions in d3ddevice.cpp
    // error C4328: 'void CD3DResourceManager::Use(const CD3DResource &)' : indirection alignment of formal parameter 1 (16) is greater than the actual argument alignment (8)
#pragma warning(disable : 4328)
    if (manager.IsInAUseContext())
    {
        manager.Use(*this);
    }
    else
    {
        // Whatever was holding onto us no longer cares about us. This is
        // sort of like a use context completing. We'll do a quick
        // Enter/Use/Exit to position ourselves as the next MRU resource
        // to be evicted.
        ENTER_USE_CONTEXT_FOR_SCOPE(Device());

        manager.Use(*this);
    }
#pragma warning(pop)
}

#if DBG
//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DResource::DbgIsAssociatedWithDevice
//
//  Synopsis:
//      returns whether this resource is associated with the specified device.
//

bool
CD3DResource::DbgIsAssociatedWithDevice(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice
    ) const
{
    return &Device() == pDevice;
}
#endif

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::AreActiveResources
//
//  Synopsis:  Returns true if there is at least one active resource
//
//-------------------------------------------------------------------------

bool
CD3DResourceManager::AreActiveResources()
{
    return !IsListEmpty(&m_leNonEvictHead)              || 
           !IsListEmpty(&m_leEvictPrevFramesHead)       ||
           !IsListEmpty(&m_leEvictCurFrameNotInUseHead) ||
           !IsListEmpty(&m_leEvictCurFrameInUseHead);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::RegisterResource
//
//  Synopsis:  Adds the resource to the current list
//
//-------------------------------------------------------------------------
void 
CD3DResourceManager::RegisterResource(
    __inout_ecount(1) CD3DResource *pResource
    )
{
    AssertThreadProtection();

    Assert(pResource);
    Assert(pResource->IsValid());

    #if DBG
    AssertConstMsgA(!DbgResourceIsActive(pResource), "Resource already registered");
    #endif

    //
    // Add to list
    //
    // Don't ref count since we don't want the resource manager to
    // keep a resource alive.  Note that the resource is responsible
    // for unregistering before it is destroyed.
    //

    Assert(pResource->m_leResourceList.Flink == NULL);
    Assert(pResource->m_leResourceList.Blink == NULL);
    if (!AreActiveResources())
    {
        Assert(m_totalVMConsumption == 0);
        #if DBG
        Assert(m_cDbgResources == 0);
        #endif
    }

    if (pResource->m_fIsEvictable)
    {
        InsertTailList(&m_leEvictCurFrameInUseHead, &pResource->m_leResourceList);
        // RegisterResource is called by CreateFoo and is considered a Use
        Use(*pResource);
    }
    else
    {
        InsertTailList(&m_leNonEvictHead, &pResource->m_leResourceList);
    }

    //
    // Add consumption counters
    //

    MtAdd(pResource->GetPerfMeterTag(), +1, pResource->GetResourceSize());

    AddToVideoMemoryUsage(pResource->GetResourceSize());

    #if DBG
        m_cDbgResources++;
    #endif

    return;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::AddToVideoMemoryUsage
//
//  Synopsis:  Function which adds cbBytes to the total number of bytes counted
//                  as used in video memory.
//
//-------------------------------------------------------------------------
void 
CD3DResourceManager::AddToVideoMemoryUsage(UINT cbBytes)
{
    AssertThreadProtection();

    if (   g_pMediaControl
        && !m_pDevice->IsSWDevice()
       )
    {
        CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();

        LONG lAddend = cbBytes;
        LONG lOldValue = InterlockedExchangeAdd(reinterpret_cast<LONG *>(&pFile->VideoMemoryUsage),
                                               lAddend);
        LONG lNewValue = lOldValue + lAddend;
        if (static_cast<DWORD>(lNewValue) > pFile->VideoMemoryUsageMax)
        {
            InterlockedExchange(reinterpret_cast<LONG *>(&pFile->VideoMemoryUsageMax),
                                lNewValue);
        }
    }

    m_totalVMConsumption += cbBytes;
    if (m_peakVMConsumption < m_totalVMConsumption)
    {
        m_peakVMConsumption = m_totalVMConsumption;
    }

}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::SubtractFromVideoMemoryUsage
//
//  Synopsis:  Function which subtracts cbBytes from the total number of bytes counted
//                  as used in video memory.
//
//-------------------------------------------------------------------------
void 
CD3DResourceManager::SubtractFromVideoMemoryUsage(UINT cbBytes)
{
    AssertThreadProtection();

    if (   g_pMediaControl
        && !m_pDevice->IsSWDevice()
       )
    {
        CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();

        LONG lResourceSize = static_cast<LONG>(cbBytes);
        LONG lOldValue = InterlockedExchangeAdd(reinterpret_cast<LONG *>(&pFile->VideoMemoryUsage),
                               -1*lResourceSize);

        LONG lNewValue = lOldValue - lResourceSize;
        if (static_cast<DWORD>(lNewValue) < pFile->VideoMemoryUsageMin)
        {
            InterlockedExchange(reinterpret_cast<LONG *>(&pFile->VideoMemoryUsageMin),
                                lNewValue);
        }
    }

    m_totalVMConsumption -= cbBytes;

}


//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::CD3DResourceManager
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DResourceManager::CD3DResourceManager()
{
    InitializeListHead(&m_leNonEvictHead);
    InitializeListHead(&m_leEvictPrevFramesHead);
    InitializeListHead(&m_leEvictCurFrameNotInUseHead);
    InitializeListHead(&m_leEvictCurFrameInUseHead);
    
    InitializeSListHead(&m_slhReleased);
    m_psleDelayReleased = NULL;

    m_totalVMConsumption = 0;
    m_peakVMConsumption  = 0;

#if DBG_ANALYSIS
    // Starting at 1 so that a resource with a m_uDbgFrameLastUsed of 0
    // is "unused"
    m_uDbgFrameCount = 1;

    m_fDbgAllowResourceListChanges = true;
#endif

    m_uCurrentUseContextDepth = 0;

    #if DBG
        m_cDbgResources = 0;
    #endif
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::~CD3DResourceManager
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DResourceManager::~CD3DResourceManager()
{
    // Assert that we don't leak resources
    AssertMsg(QueryDepthSList(&m_slhReleased) == 0,
              "CD3DResourceManager released before released resources");
    AssertMsg(m_psleDelayReleased == NULL,
              "CD3DResourceManager released before released resources");
    AssertMsg(IsListEmpty(&m_leNonEvictHead),
              "CD3DResourceManager released before non evictable resources");
    AssertMsg(IsListEmpty(&m_leEvictPrevFramesHead),
              "CD3DResourceManager released before previous frame evictable resources");
    AssertMsg(IsListEmpty(&m_leEvictCurFrameNotInUseHead),
              "CD3DResourceManager released before current frame not used evictable resources");
    AssertMsg(IsListEmpty(&m_leEvictCurFrameInUseHead),
              "CD3DResourceManager released before current frame used evictable resources");

    Assert(m_uCurrentUseContextDepth == 0);
    
    #if DBG
    Assert(m_cDbgResources == 0);
    #endif
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResourceManager::Init
//
//  Synopsis:  Initialization function
//
//-------------------------------------------------------------------------
void
CD3DResourceManager::Init(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    // The device pointer is used to check threading protection.
    // ...resource, but not otherwise.  This is safe because the must be at least
    // one reference to the device in order for a RegisterResource call to be
    // made as this ResourceManager is a member of the device.
    m_pDevice = pDevice;

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::UnusedNotification
//
//  Synopsis:  Marks resource as unused and to be destroyed when possible.
//
//             If this thread has the appropriate protections then destruction
//             will be handled now.  Otherwise the resource will be placed on
//             the deferred destruction list.
//

void 
CD3DResourceManager::UnusedNotification(
    __inout_ecount(1) CMILPoolResource *pUnused
    )
{
    Assert(pUnused);

    CD3DResource *pResource = DYNCAST(CD3DResource, pUnused);
    Assert(pResource);

    // The resource should always be valid at this point.  When it becomes
    // invalid UnusableNotification should be used, but that can only happen if
    // there is a reference to the resource.  UnusableNotification will modify
    // the resource such that UnusedNotification won't be called.  Also see
    // CMILPoolResource::Release.
    Assert(pResource->IsValid());
    
    InterlockedPushEntrySList(&m_slhReleased, &(pResource->m_sleFreeList));

    //
    // Deleting video memory resources immediately is very expensive in XP as
    // well as the current LDDM runtime, so we delay destroying the resources
    // until later.
    //
    // After the above bug is fixed, destroying immediately in LDDM shouldn't
    // suffer from the same perf hit so we should re-enable it.
    //

#if NEVER
    if (m_pDevice->IsProtected(false /* Do not force entry check */))
    {
        //
        // Since this context is protected destroy the resource now.
        //

        DestroyReleasedResourcesFromLastFrame();
    }
#endif

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::UnusableNotification
//
//  Synopsis:  Remove resource from tracking list after making sure the
//             resources have been released 
//

void
CD3DResourceManager::UnusableNotification(
    __inout_ecount(1) CMILPoolResource *pUnusable
    )
{
    AssertThreadProtection();

    CD3DResource *pResource = DYNCAST(CD3DResource, pUnusable);
    Assert(pResource);

    // The resource should always be marked invalid at this point.
    Assert(!pResource->IsValid());

    //
    // Make sure device has release all its resources
    //

    DestroyResource(pResource);

    //
    // Since this context is protected go ahead and destroy any free resources.
    //

    DestroyReleasedResourcesFromLastFrame();

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroyAllResources
//
//  Synopsis:  For each resource call DestroyResource which will
//             1. Release the CD3DResource's D3D resources
//             2. Unregister it
//
//             Note that this call is typically called after a mode change,
//             whenever we need to recreate the underlying device, and at
//             device destruction.  After this call there should not be any
//             resources with a reference to this object.
//

void 
CD3DResourceManager::DestroyAllResources()
{
    AssertThreadProtection();
        
    // Quick out if there is nothing to do.
    if (AreActiveResources())
    {
        DestroyReleasedResourcesFromLastFrame();
        
        UINT uExpectedInFreeList = DestroySomeActiveResources();

        //
        // Check if all resources expected to be on the free list are on it.
        //
        // If not then there is another thread (or several) that was interrupted
        // during a CD3DResource::Release call between the reference count
        // decrement and the call to UnusedNotification where it is added to the
        // free list.  So, give up this time slice to let it/them finish.  Wait
        // until they have all completed.
        //

        USHORT usFreeListDepth = QueryDepthSList(&m_slhReleased);
        Assert(usFreeListDepth <= uExpectedInFreeList);

        while (usFreeListDepth != uExpectedInFreeList)
        {
            TraceTag((tagWarning,
                      "%u resources remain to be added to free list - waiting...",
                      uExpectedInFreeList - usFreeListDepth
                      ));

            // Give up time slice
            Sleep(1);

            // Check again
            usFreeListDepth = QueryDepthSList(&m_slhReleased);
        }

        //
        // Clean up any resources on the free list.
        //

        DestroyResources(WithoutDelay);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroySomeActiveResources
//
//  Synopsis:  Walk lists of active resources and destroy them if someone
//             else hasn't already (or is about to) put them on the released
//             list.
//

UINT
CD3DResourceManager::DestroySomeActiveResources()
{
    UINT count = 0;
    count += DestroyListOfResources(&m_leNonEvictHead);
    count += DestroyListOfResources(&m_leEvictPrevFramesHead);
    count += DestroyListOfResources(&m_leEvictCurFrameNotInUseHead);
    count += DestroyListOfResources(&m_leEvictCurFrameInUseHead);

#if DBG
    // Make sure lists and count are in agreement
    Assert(AreActiveResources() == (m_cDbgResources != 0));
#endif
        
    return count;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroyListOfResources
//
//  Synopsis:  Destroys all the resources in pListHead. See 
//             DestroyAllResources() for more comments.
//

UINT
CD3DResourceManager::DestroyListOfResources(__in_ecount(1) LIST_ENTRY *pListHead)
{
    UINT uExpectedInFreeList = 0;

    LIST_ENTRY *pListEntry = pListHead;

    while (pListEntry->Flink != pListHead)
    {
        LIST_ENTRY *pCurFlink = pListEntry->Flink;

        CD3DResource *pResource =
            CONTAINING_RECORD(pCurFlink,
                              CD3DResource,
                              m_leResourceList);

        //
        // This manager doesn't hold a reference count to the resource, but now
        // wants to change some of the protected state (its resources); so it
        // acquires a reference for the duration of that operation.
        //

        LONG cResourceRefs = InterlockedIncrementULONG(&pResource->m_cRef);

        //
        // Check for a reference count of 1, which indicates that the resource
        // is now or very soon will be on the free list.  Otherwise, destroy
        // the resource.
        //

        if (cResourceRefs == 1)
        {
            // Restore value of zero for consistency
            pResource->m_cRef = 0;
              
            ++uExpectedInFreeList;

            // The current resource wasn't removed; so we need to advance the
            // list entry pointer.
            Assert(pListEntry->Flink == pCurFlink);
            pListEntry = pCurFlink;
        }
        else
        {
            InvalidateAndDestroyResource(pResource);

            //
            // Now release the reference, but since this resource has been
            // removed from this manager's control just delete this object.
            //
            // Note that we could still call Release() here, but that could be
            // confusing since Release() has a code path to call
            // UnusedNotification, which places resources on the free list and
            // has other side-effects that we don't handle here like
            // potentially changing the resource list.
            //

            if (InterlockedDecrementULONG(&pResource->m_cRef) == 0)
            {
                delete pResource;
            }

            // The current resource should have been removed.
            Assert(pListEntry->Flink != pCurFlink);
        }
    }

    return uExpectedInFreeList;
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroyFreedResources
//
//  Synopsis:  Walk list of freed resources and destroy them.
//
//  Notes:     This member may be reentered when it destroys a collection
//             resource, such as a CD3DSwapChain.
//
//  Returns:   Number of resources destroyed.
//
//-----------------------------------------------------------------------------

UINT
CD3DResourceManager::DestroyListOfReleasedResources(
    __in_ecount(1) SLIST_ENTRY *pList
    )
{
    AssertThreadProtection();
    
    UINT cnt = 0;

    while (pList)
    {
        // Dereference resource from list entry
        CD3DResource *pUnused =
            CONTAINING_RECORD(
                pList,
                CD3DResource,
                m_sleFreeList
                );

        // Get next resource on list
        pList = pList->Next;

        //
        // Destroy the resource
        //
        // Note that destruction of this resource may cause other resources to
        // become unused (free).  In that case UnusedNotification will be
        // called then a nested call to DestroyReleasedResources will be made.
        //

        DestroyResource(pUnused);

        // Delete the resource
        delete pUnused;

        ++cnt;
    }

    return cnt;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroyResource
//
//  Synopsis:  Request a CD3DResource release its D3D resources and then remove
//             it from tracking
//

void 
CD3DResourceManager::DestroyResource(
    __inout_ecount(1) CD3DResource *pResource
    )
{
    AssertThreadProtection();

    Assert(pResource);
    Assert(pResource->m_pManager);

#if DBG_ANALYSIS
    AssertConstMsgA(DbgResourceIsActive(pResource), "Resource not in resource list");
    // Nested calls are not allowed because it could affect the integrity of the resource
    // lists. For example, if DestroyResource(A) ends up doing DestroyResource(B) and
    // B is the entry before A in one of the resource lists.
    AssertMsg(m_fDbgAllowResourceListChanges, "Nested DestroyResource calls are not allowed!");

    // This looks odd considering we just Asserted this, but if we always set 
    // m_fDbgAllowResourceListChanges to true at the end then we'd only hit
    // the Assert once on the first violation in the debugger.
    bool fDbgSetAllowResourceListChangesInThisMethod = false;
    if (m_fDbgAllowResourceListChanges)
    {
        m_fDbgAllowResourceListChanges = false;
        fDbgSetAllowResourceListChangesInThisMethod = true;
    }
#endif

    //
    // Callback to resource to release D3D resources
    //

    pResource->ReleaseD3DResources();

    //
    // Remove from management and list
    //
    // Note that the resource's m_pManager field is modified here and it is
    // assumed that either a reference is held on the resource or it is being
    // destroyed from the free list.
    //

    pResource->m_pManager = NULL;
    RemoveEntryList(&pResource->m_leResourceList);

    //
    // Subtract consumption counters
    //

    UINT uResourceSize = pResource->GetResourceSize();
    MtAdd(pResource->GetPerfMeterTag(), -1, -1*uResourceSize);

    SubtractFromVideoMemoryUsage(uResourceSize);

#if DBG_ANALYSIS
    m_cDbgResources--;
    if (fDbgSetAllowResourceListChangesInThisMethod)
    {
        m_fDbgAllowResourceListChanges = true;
    }
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::EndFrame
//
//  Synopsis:  This should be called when a frame is done. It updates the
//             frame count and moves all of the current frame resources 
//             to the previous frame list.
//

void
CD3DResourceManager::EndFrame()
{
#if DBG_STEP_RENDERING    
    // Step rendering will call Present() multiple times while in a 
    // Use context and we don't want to do anything then.
    if (m_pDevice->DbgInStepRenderingPresent())
    {
        return;
    }
#endif
    
    // We shouldn't be in a UseContext once everything is done being
    // drawn.
    Assert(m_uCurrentUseContextDepth == 0);
    
    //
    // 1. Concatenate the current frame list onto the end of the previous
    //    frame list because the current frame is over.
    //
    //    Here's an example of how the lists work:
    //
    //                             In Use        Not In Use
    //          EnterUC
    //              Use(a)           a               -
    //              Use(b)           ab              -
    //              EnterUC
    //                  Use(c)       abc             -
    //              ExitUC           ab              c
    //              EnterUC
    //                  Use(d)       abd             c
    //              ExitUC           ab              cd
    //              EnterUC
    //                  Use(a)       ab              cd  (note: a is not moved)
    //              ExitUC           ab              cd  (note: a is still in use)
    //              Use(e)           abe             cd
    //          ExitUC               -               cdabe
    //

    if (!IsListEmpty(&m_leEvictCurFrameNotInUseHead))
    {
        AppendTailList(&m_leEvictPrevFramesHead, &m_leEvictCurFrameNotInUseHead);
        RemoveEntryList(&m_leEvictCurFrameNotInUseHead);
        InitializeListHead(&m_leEvictCurFrameNotInUseHead);
    }

    // Since there shouldn't be any active use context, nothing should be
    // in use!
    Assert(IsListEmpty(&m_leEvictCurFrameInUseHead));

#if DBG_ANALYSIS
    DbgAssertPrevFrameListSorted();

    //
    // 2. Advance to next frame
    //
    ++m_uDbgFrameCount;
#endif
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroyReleasedResourcesFromLastFrame
//
//  Synopsis:  Destroys resources from the previous frame and returns how
//             many it destroyed.
//
//  Returns:   Number of resources destroyed.
//
//-----------------------------------------------------------------------------

UINT
CD3DResourceManager::DestroyReleasedResourcesFromLastFrame()
{
    PSLIST_ENTRY psle = m_psleDelayReleased;
    m_psleDelayReleased = NULL;
    return DestroyListOfReleasedResources(psle);
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DestroyResources
//
//  Synopsis:  This should be called when resources should be deleted, but
//             AT LEAST every time the frame is advanced. Resources from this
//             frame are destroyed if they don't ask for a dely or if
//             eStyle is WithoutDelay.
//
//             Released resources that aren't deleted are moved to the
//             list to be deleted next frame.
//
//  Returns:   Number of resources destroyed.
//
//-----------------------------------------------------------------------------

UINT
CD3DResourceManager::DestroyResources(DestroyResourcesStyle eStyle)
{
    AssertThreadProtection();

    UINT count = 0;

    // Process resources from this frame, deleting some
    // postponing others.
    PSLIST_ENTRY ple = InterlockedFlushSList(&m_slhReleased);
    while (ple)
    {
        // Dereference resource from list entry
        CD3DResource *pUnused =
            CONTAINING_RECORD(
                ple,
                CD3DResource,
                m_sleFreeList
                );
        
        PSLIST_ENTRY pleNext = ple->Next;

        if (eStyle == WithDelay && pUnused->RequiresDelayedRelease())
        {
            ple->Next = m_psleDelayReleased;
            m_psleDelayReleased = ple;
        }
        else
        {
            ++count;
            DestroyResource(pUnused);
            // Delete the resource
            delete pUnused;
        }

        ple = pleNext;
    }

    return count;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::ExitUseContext
//
//  Synopsis:  You must call this when leaving a method which starts with
//             EnterUseContext()
//

void 
CD3DResourceManager::ExitUseContext(UINT uDepth)
{
    AssertMsg(m_uCurrentUseContextDepth > 0, "Called ExitUseContext one too many times");
    Assert(uDepth == m_uCurrentUseContextDepth);

    //
    // The current frame in use list is actually a stack. Resources will be
    // grouped by depth and the most recently used resources are at the tail.
    // We will walk from the back to find the all of the resources from the
    // current context and move them to the current frame not in use list.
    //
    // See EndFrame for an example
    //

    LIST_ENTRY *pCurEntry = m_leEvictCurFrameInUseHead.Blink;
    while (pCurEntry != &m_leEvictCurFrameInUseHead)
    {
        CD3DResource *pResource =
            CONTAINING_RECORD(pCurEntry,
                              CD3DResource,
                              m_leResourceList);    

        Assert(pResource->m_uActiveDepthLastUsed != CD3DResource::c_uDepthNotUsed);

        if (pResource->m_uActiveDepthLastUsed == m_uCurrentUseContextDepth)
        {
            pResource->m_uActiveDepthLastUsed = CD3DResource::c_uDepthNotUsed;
        }
        else
        {
            // We've reached resources from earlier contexts so it's time to stop.
#if DBG_ANALYSIS
            // Verify the stack's ordering
            {
                UINT uDbgActiveDepthLastUsed = pResource->m_uActiveDepthLastUsed;
                LIST_ENTRY *pDbgCurEntry = pCurEntry;
                while (pDbgCurEntry != &m_leEvictCurFrameInUseHead)
                {
                    const CD3DResource *pDbgResource =
                        CONTAINING_RECORD(pDbgCurEntry,
                                          CD3DResource,
                                          m_leResourceList);  

                    Assert(pDbgResource->m_uActiveDepthLastUsed <= uDbgActiveDepthLastUsed);
                    Assert(pDbgResource->m_uActiveDepthLastUsed != CD3DResource::c_uDepthNotUsed);
                    Assert(pDbgResource->m_uDbgFrameLastUsed == m_uDbgFrameCount);

                    uDbgActiveDepthLastUsed = pDbgResource->m_uActiveDepthLastUsed;

                    pDbgCurEntry = pDbgCurEntry->Blink;
                }
            }
#endif      
            break;
        }

        pCurEntry = pCurEntry->Blink;
    }
   
    if (pCurEntry->Flink != &m_leEvictCurFrameInUseHead)
    {
        LIST_ENTRY *pFirstItem = pCurEntry->Flink;
        LIST_ENTRY *pLastItem = m_leEvictCurFrameInUseHead.Blink; 

        // make pCurEntry the new tail entry for the cur frame in use list
        pCurEntry->Flink = &m_leEvictCurFrameInUseHead;
        m_leEvictCurFrameInUseHead.Blink = pCurEntry;

        // put pFirstItem <-> ... <-> pLastItem into the cur frame not in use
        // list
        LIST_ENTRY *pNotInUseTail = m_leEvictCurFrameNotInUseHead.Blink;

        pNotInUseTail->Flink = pFirstItem;
        pFirstItem->Blink = pNotInUseTail;

        m_leEvictCurFrameNotInUseHead.Blink = pLastItem;
        pLastItem->Flink = &m_leEvictCurFrameNotInUseHead;
    }

    //
    // 2. Exit the current depth
    //
    --m_uCurrentUseContextDepth;
}


//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::Use
//
//  Synopsis:  Call whenever a resource is used
//

void
CD3DResourceManager::Use(__inout_ecount(1) const CD3DResource &d3dResource)
{
    if (d3dResource.m_fIsEvictable)
    {
        Assert(d3dResource.IsValid());
        Assert(IsInAUseContext());
        AssertDeviceEntry(*m_pDevice);

#if DBG_ANALYSIS
        d3dResource.m_uDbgFrameLastUsed = m_uDbgFrameCount;
#endif
        // Only update the depth and move the resource if it isn't being used 
        // in the current frame already.
        if (d3dResource.m_uActiveDepthLastUsed == CD3DResource::c_uDepthNotUsed)
        {
            d3dResource.m_uActiveDepthLastUsed = m_uCurrentUseContextDepth;

            LIST_ENTRY *pResourceListNode = &d3dResource.m_leResourceList;
            RemoveEntryList(pResourceListNode);
            InsertTailList(&m_leEvictCurFrameInUseHead, pResourceListNode);     
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::InvalidateAndDestroyResource
//
//  Synopsis:  Calls DestroyResource and marks the resource as invalid.
//
//  Warning:   In general, destruction should only be done internally by the
//             manager itself so only call this method if absolutely necessary. 
//             It's dangerous to call this on a non-evictable resource since 
//             this will destroy any D3D resources.
//

void
CD3DResourceManager::InvalidateAndDestroyResource(
    __inout_ecount(1) CD3DResource *pResource
    )
{
    //
    // Mark resource as invalid
    //  This is not strictly required for proper operation as there
    //  shouldn't be anyone who would query this from different thread.
    //  However we do this anyway to make the state consistent for
    //  debugging.  There are asserts in ReleaseD3DResource that check
    //  for this state.
    //
    
    pResource->m_fResourceValid = false;
    
    //
    // Destroy the resource
    //
    
    DestroyResource(pResource);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResource::GetUnusedResourceFromList
//
//  Synopsis:  Makes sure that pEntryToGet's isn't the head and
//             does some DBG sanity checks.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE
__out_ecount_opt(1) CD3DResource *
CD3DResourceManager::GetUnusedResourceFromList(
    __in_ecount(1) const LIST_ENTRY *pListHead,
    __in_ecount(1) const LIST_ENTRY *pEntryToGet
    ) const
{
    CD3DResource *pResource = NULL;
    
    if (pEntryToGet != pListHead)
    {
        pResource = CONTAINING_RECORD(pEntryToGet, CD3DResource, m_leResourceList);

        Assert(pResource->m_uActiveDepthLastUsed == CD3DResource::c_uDepthNotUsed);
        Assert(pResource->IsValid() && pResource->IsEvictable());
    }

    return pResource;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::FindLRUResourceInAPreviousFrame
//
//  Synopsis:  Finds the LRU evictable resource if possible. It will return 
//             NULL if there isn't one.
//

__out_ecount_opt(1) CD3DResource *
CD3DResourceManager::FindLRUResourceInAPreviousFrame() const
{
    //
    // Since the lists are sorted by Use, oldest -> newest, the first 
    // item is the LRU resource. See EndFrame() for more on sorting.
    //

    return GetUnusedResourceFromList(
        &m_leEvictPrevFramesHead, 
        m_leEvictPrevFramesHead.Flink
        );
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::FindMRUResourceInCurrentFrame
//
//  Synopsis:  Finds the MRU evictable resource if possible. It will return 
//             NULL if there isn't one.
//

__out_ecount_opt(1) CD3DResource *
CD3DResourceManager::FindMRUResourceInCurrentFrame() const
{
    //
    // Since the lists are sorted by Use, oldest -> newest, the last 
    // item is the MRU resource. See EndFrame() for more on sorting.
    //

    return GetUnusedResourceFromList(
        &m_leEvictCurFrameNotInUseHead, 
        m_leEvictCurFrameNotInUseHead.Blink
        );
}


//+------------------------------------------------------------------------
//
//  Member:    CD3DResource::FreeSomeVideoMemory
//
//  Synopsis:  After every hardware device allocation, this method should
//             be called. If the device allocation failed because we were
//             OOVM, this method will try to free up memory. If this method
//             returns true, the allocation should be retried.
//
//             For simplicity, put BEGIN_DEVICE_ALLOCATION and 
//             END_DEVICE_ALLOCATION around the device call and everything
//             is taken care of for you.
//
//  Returns:
//         true  - we were able to free some video memory... try the
//                 allocation again
//
//         false - there was nothing we could do:
//                 1) The HRESULT is not OOVM (or OOM in RGBRast's case)
//                 2) There are no more evictable items that we can free
//
//-------------------------------------------------------------------------
bool 
CD3DResourceManager::FreeSomeVideoMemory(const HRESULT hD3DResult)
{
    AssertThreadProtection();

    bool fResourceEvicted = false;

    // RGBRast never returns OOVM so we need to check OOM for it
    if (hD3DResult == D3DERR_OUTOFVIDEOMEMORY || 
        (hD3DResult == E_OUTOFMEMORY && m_pDevice->IsSWDevice()))
    {
        fResourceEvicted = DestroyReleasedResourcesFromLastFrame() > 0;

        if (fResourceEvicted)
        {
            goto Cleanup;
        }
        
        fResourceEvicted = DestroyResources(WithDelay) > 0;

        if (fResourceEvicted)
        {
            goto Cleanup;
        }

        fResourceEvicted = DestroyReleasedResourcesFromLastFrame() > 0;

        if (fResourceEvicted)
        {
            goto Cleanup;
        }
        
#if DBG
        DbgAssertPrevFrameListSorted();
#endif

        //
        // First, try to evict the LRU item from older frames.
        // If that fails, try to evict the MRU item from the
        // current frame that is not in use any more.
        //

        CD3DResource *pResourceToDestroy = FindLRUResourceInAPreviousFrame();
        if (!pResourceToDestroy)
        {
            pResourceToDestroy = FindMRUResourceInCurrentFrame();
        }

        //
        // If we found something, toss it!
        //
            
        if (pResourceToDestroy)
        {   
            // Make sure the resource stays alive since we're going to mess with it
            // in InvalidateAndDestroyResource
            LONG cResourceRefs = InterlockedIncrementULONG(&pResourceToDestroy->m_cRef);
            if (cResourceRefs == 1)
            {
                // The resource is or soon will be on the free list. The beginning of
                // this method cleans the free list so this was added to the free list
                // between then and now by another thread. Sleep and pretend like we
                // freed some memory so when this method gets called a second time
                // the free list cleaning code will destroy it.
                pResourceToDestroy->m_cRef = 0;
                Sleep(1);
                fResourceEvicted = true;
                goto Cleanup;
            }
            else
            { 
                Assert(   pResourceToDestroy->IsValid() 
                       && pResourceToDestroy->IsEvictable());

                InvalidateAndDestroyResource(pResourceToDestroy);
                fResourceEvicted = true;

                if (InterlockedDecrementULONG(&pResourceToDestroy->m_cRef) == 0)
                {
                    // Really we should be calling Release(), but since we 
                    // Destroyed the resource it no longer has a manager and we 
                    // know that Release() is just going to delete it. Also, since
                    // we aren't depending on the list to stay intact,
                    // we have no reason to set m_fDbgAllowResourceListChanges
                    delete pResourceToDestroy;
                }    
            }
        }
    }

Cleanup:
    
    return fResourceEvicted;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DUseContextGuard::CD3DUseContextGuard
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DUseContextGuard::CD3DUseContextGuard(__in_ecount(1) CD3DDeviceLevel1 &d3dDevice)
    : m_d3dDeviceNoRef(d3dDevice)
{
    m_uDepth = m_d3dDeviceNoRef.EnterUseContext();
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DUseContextGuard::~CD3DUseContextGuard
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DUseContextGuard::~CD3DUseContextGuard()
{
    m_d3dDeviceNoRef.ExitUseContext(m_uDepth);
}

#if DBG_ANALYSIS

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DbgFindEntryInList
//
//  Synopsis:  Determines if resource is currently in the list
//

static bool
DbgFindEntryInList(
    __in_ecount(1) const LIST_ENTRY *pListHead,
    __in_ecount(1) const LIST_ENTRY *pResourceEntry
    )
{
    for (const LIST_ENTRY *pSrchEntry = pListHead->Flink;
         pSrchEntry != pListHead;
         pSrchEntry = pSrchEntry->Flink)
    {
        if (pSrchEntry == pResourceEntry)
        {
            return true;
        }
    }

    return false;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DbgFindResource
//
//  Synopsis:  Determines if resource is currently in any of the active lists
//
//-----------------------------------------------------------------------------
bool
CD3DResourceManager::DbgResourceIsActive(
    __in_ecount(1) const CD3DResource *pResource
    )
{    
    //
    // Get the convenient search address
    //
    const LIST_ENTRY *pResourceEntry = &pResource->m_leResourceList;

    if (DbgFindEntryInList(&m_leEvictCurFrameInUseHead, pResourceEntry)     || 
        DbgFindEntryInList(&m_leEvictCurFrameNotInUseHead, pResourceEntry)  ||
        DbgFindEntryInList(&m_leEvictPrevFramesHead, pResourceEntry)        ||
        DbgFindEntryInList(&m_leNonEvictHead, pResourceEntry))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DResourceManager::DbgAssertThreadProtection
//
//  Synopsis:  Asserts that current thread has proper device
//              threading protection
//
//-----------------------------------------------------------------------------
void
CD3DResourceManager::DbgAssertThreadProtection() const
{
    Assert(m_pDevice);
    AssertDeviceEntry(*m_pDevice);
}

//+----------------------------------------------------------------------------
//
//  Member:    DbgAssertPrevFrameListSorted
//
//  Synopsis:  Asserts that the previous frame list is sorted small frame ->
//             big frame and that nothing in it is in use.
//
//             Note: The frame counter could potentially wrap causing this to
//                   fire but it's an unsigned, 64-bit integere so we may not
//                   live to see that day.
//

void
CD3DResourceManager::DbgAssertPrevFrameListSorted()
{
    UINT64 uLastResourceFrameUsed = 0;
    
    LIST_ENTRY *pCurEntry = m_leEvictPrevFramesHead.Flink;
    while (pCurEntry != &m_leEvictPrevFramesHead)
    {              
        const CD3DResource *pCurResource =
            CONTAINING_RECORD(pCurEntry,
                              CD3DResource,
                              m_leResourceList);

        Assert(pCurResource->m_uDbgFrameLastUsed != 0 && 
               pCurResource->m_uDbgFrameLastUsed >= uLastResourceFrameUsed);
        Assert(pCurResource->m_uActiveDepthLastUsed == CD3DResource::c_uDepthNotUsed);

        uLastResourceFrameUsed = pCurResource->m_uDbgFrameLastUsed;

        pCurEntry = pCurEntry->Flink;
    }
}

#endif DBG_ANALYSIS





