// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DResource and CD3DResourceManager implementation
//

//  Notes:
//      To achieve thread safety we follow the following rules:
//          1. The resource manager's data is only modified under the caller
//             provided multi-thread protection with the exception of the free
//             list which is itself thread safe.
//          2. Threading protection is verified via the owner of the resource
//             manager which is the CD3DDeviceLevel1.
//              - This means the owner must be available whenever the resource
//                manager may be accessed.
//              - As the resource manager is a member of the CD3DDeviceLevel1
//                we simply walk the list of any outstanding resources and
//                remove their references to the resource manager in a thread
//                safe manner via DestroyAllResources when the CD3DDeviceLevel1
//                is being destroyed.
//          3. Resources may only be registered under threading protection.
//          4. Resources may only be destroyed under threading protection.
//          5. The ability to destroy a resource implies the caller holds a
//             reference to the resource.  The destroy method is called
//             DestroyAndRelease to enforce this requirement.
//          5. Resources should only be destroyed once.  Checking IsValid is
//             the default way to check this, but this is not asserted because
//             we allow various resource implementations to mark themselves as
//             invalid before DestroyAndRelease (or UnusableNofication) is
//             called.
//              - Any code unsure of this state should check IsValid (or
//                appropriate) under the threading protection before destroying
//                the resource.
//          6. Resources may be released from any thread.  If not under the
//             threading protection, then the resource will not be fully
//             released (nor will its actual D3D resources) until
//             DestroyFreedResources is called.
//              - Release calls UnusedNotification on the resource manager,
//                which is able to check thread protection via the device.  See
//                rule #2.
//          7. DestroyFreedResources may only be called under thread
//             protection.
//
//-----------------------------------------------------------------------------

class CD3DSwapChain;
class CD3DResourceManager;

//------------------------------------------------------------------------------
//
//  Class: CD3DResource
//
//  Description:
//     Base object that represents a trackable d3d resource.  Any d3d resource
//     allocated should be tracked with this object.
//
//------------------------------------------------------------------------------

class CD3DResource : public CMILPoolResource
{
private:
    // Let resource manager manipulate linked list
    friend CD3DResourceManager;

    //
    // Pool management data
    //

    mutable LIST_ENTRY m_leResourceList;
    SLIST_ENTRY m_sleFreeList;

public:
    UINT GetResourceSize() const;
    virtual bool IsValid() const;

    void SetAsEvictable();

    bool IsEvictable() const
    {
        return m_fIsEvictable;
    }

    virtual bool RequiresDelayedRelease() const
    {
        // Most resources don't require any delay before deletion.
        return false;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    DestroyAndRelease
    //
    //  Synopsis:  Remove this resource from the manager and call back to
    //             ReleaseD3DResources to make sure all of the D3D resources
    //             are actually cleaned up
    //
    //  Notes:     Destroy may only be called under proper protection,
    //             which is currently the active device thread.
    //
    //-------------------------------------------------------------------------

    void DestroyAndRelease();

#if DBG
    bool DbgIsAssociatedWithDevice(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice
        ) const;
#endif

protected:
    CD3DResource();
    virtual ~CD3DResource();

    void Init(
        __inout_ecount(1) IMILPoolManager *pManager, 
        UINT uiResourceSize
        );

    // Helper methods to look up actual manager and device objects
    __out_ecount(1) CD3DResourceManager &Manager() const;
    CD3DDeviceLevel1 &Device() const;

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const = 0;
#endif

private:

    virtual void ReleaseD3DResources() = 0;

#if DBG_ANALYSIS
    mutable UINT64 m_uDbgFrameLastUsed;
#endif
    mutable UINT   m_uActiveDepthLastUsed;

    static const UINT c_uDepthNotUsed = 0;

    bool m_fIsEvictable;   // if this is true, it's up to us to delete the resource not D3D
                           // defaults to false
protected:

    UINT m_uResourceSize;
    
    bool m_fResourceValid;
};

/*=========================================================================*\

    DEFINE_RESOURCE_REF_COUNT_BASE:

    Include this macro in the public methods list for all classes multiply
    inheriting reference counting interfaces such as IMILRefCount and IUnknown.

\*=========================================================================*/

#define DEFINE_RESOURCE_REF_COUNT_BASE                                               \
    override ULONG STDMETHODCALLTYPE AddRef(void) {return CD3DResource::AddRef();} \
    override ULONG STDMETHODCALLTYPE Release(void) {return CD3DResource::Release();}

//------------------------------------------------------------------------------
//
//  Class: CD3DResourceManager
//
//  Description:
//     Tracks all resources we allocated with a particular d3d device.  
//
//------------------------------------------------------------------------------

class CD3DResourceManager :
    public IMILPoolManager
{
public:

    CD3DResourceManager();
    ~CD3DResourceManager();

    void Init(__in_ecount(1) CD3DDeviceLevel1 *pDevice);

    //
    // Manager methods called by child resources
    //

    void RegisterResource(__inout_ecount(1) CD3DResource *pResource);

    // Used to notify the manager that there are no outstanding uses and
    //  the manager has full control.
    void UnusedNotification(__inout_ecount(1) CMILPoolResource *pUnused);

    // Used to notify the manager that the resource is no longer usable
    //  and should be removed from the pool.
    void UnusableNotification(__inout_ecount(1) CMILPoolResource *pUnusable);

    // Provide lookup from resource manager to device class
    __out_ecount(1) CD3DDeviceLevel1 &Device() const { return *m_pDevice; }

    //
    // Additional methods for the manager's owner (CD3DDeviceLevel1)
    //

    // Walk all outstanding resources and invalidate/destroy them
    void DestroyAllResources();

    // Frees up video memory when we're out. 
    bool FreeSomeVideoMemory(const HRESULT hD3DResult);

    // Call this on whenever a CD3DResource is used by a device. In an OOVM situation,
    // the resource manager will not be able to destroy the resource until the current
    // UseContext completes.
    void Use(__inout_ecount(1) const CD3DResource &d3dResource);

    // Call this at the end of a frame
    void EndFrame();

    enum DestroyResourcesStyle {
        WithDelay,
        WithoutDelay,
    };

    // Call this when resources should be destroyed.  AT LEAST call it
    // at the advance of a frame.
    UINT DestroyResources(DestroyResourcesStyle eStyle);

    UINT DestroyReleasedResourcesFromLastFrame();

    // Call this at the beginning of a method that uses CD3DResources
    UINT EnterUseContext()
    {
        return ++m_uCurrentUseContextDepth;
    }

    // Call this at the end of a method that uses CD3DResources
    void ExitUseContext(UINT uDepth);

    bool IsInAUseContext() const
    {
        return (m_uCurrentUseContextDepth > 0);
    }

    void InvalidateAndDestroyResource(__inout_ecount(1) CD3DResource *pResource);

private:
    void AddToVideoMemoryUsage(UINT cbBytes);
    void SubtractFromVideoMemoryUsage(UINT cbBytes);

    void DestroyResource(__inout_ecount(1) CD3DResource *pResource);
    
    UINT DestroyListOfResources(__in_ecount(1) LIST_ENTRY *pListHead);
    UINT DestroyListOfReleasedResources(__in_ecount(1) SLIST_ENTRY *pListHead);
    
    UINT DestroySomeActiveResources();

    __out_ecount_opt(1) CD3DResource *FindLRUResourceInAPreviousFrame() const;
    __out_ecount_opt(1) CD3DResource *FindMRUResourceInCurrentFrame() const;

    __out_ecount_opt(1) CD3DResource *
    GetUnusedResourceFromList(
        __in_ecount(1) const LIST_ENTRY *pListHead,
        __in_ecount(1) const LIST_ENTRY *pEntryToGet
        ) const;

    bool AreActiveResources();

    void TryReleaseNonResidentVideoMemoryResourcesInList(
        __in_ecount(1) LIST_ENTRY *pListHead
        );

    // Depth count of the current UseContext. It could overflow if we were 
    // somehow able to make 4.3 billion nested draw calls...
    UINT m_uCurrentUseContextDepth;

    // Thread safe list of released resources queued for destruction.
    SLIST_HEADER m_slhReleased;

    // Non-thread safe list of resources that need to be released,
    // preferably for performance reasons after waiting a frame.
    // (But if OOVM demands they can be released at any time.)
    PSLIST_ENTRY m_psleDelayReleased;

    LIST_ENTRY m_leNonEvictHead;
    LIST_ENTRY m_leEvictPrevFramesHead;
    LIST_ENTRY m_leEvictCurFrameNotInUseHead;
    LIST_ENTRY m_leEvictCurFrameInUseHead;

    UINT m_totalVMConsumption;
    UINT m_peakVMConsumption;

    // The CD3DDeviceLevel1 is used to check threading protection
    // Note that it is not reference counted.
    CD3DDeviceLevel1 *m_pDevice;

#if DBG_ANALYSIS
private:
    bool DbgResourceIsActive(__in_ecount(1) const CD3DResource *pResource);

    void DbgAssertThreadProtection() const;

    void DbgAssertPrevFrameListSorted();

private:
    UINT m_cDbgResources;

    // This is incremented at least once every frame and will not overflow.
    // For example, if we're doing 60 FPS on two monitors this will be incremented
    // 120 times/second and won't overflow until 4.8 billion years later.
    UINT64 m_uDbgFrameCount;

    bool m_fDbgAllowResourceListChanges;    
#endif
};

class CD3DUseContextGuard
{
public:
    CD3DUseContextGuard(__in_ecount(1) CD3DDeviceLevel1 &d3dDevice);
    ~CD3DUseContextGuard();

private:
    CD3DDeviceLevel1 &m_d3dDeviceNoRef;
    UINT m_uDepth;
};

//------------------------------------------------------------------------------
// Inlines
//------------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Function:  CD3DResource::GetResourceSize
//
//  Synopsis:  Returns the expected video memory usage for the resource.
//             Note that this information is a best guess and isn't
//             guaranteed to be correct.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE UINT
CD3DResource::GetResourceSize() const
{
    return m_uResourceSize;
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DResource::IsValid
//
//  Synopsis:  Before accessing a resource, the caller must check IsValid 
//             to see if the resource has been destroyed.  Resources
//             are only destroyed under the threading protection.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE bool
CD3DResource::IsValid() const
{
    return m_fResourceValid;
}




