// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_resourcemgmt
//      $Keywords:
//
//  $Description:
//      Contains CHwBrushPool and CHwBrushPoolManager declarations
//
//      Manages Hardware Brush Pool.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHwBrushPoolManager);

class CHwBrushPoolManager;
class CHwBrushPool;
class CHwBrush;
class CHwSolidBrush;

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwCacheablePoolBrush
//
//  Synopsis:
//      Interface to a cacheable brush that allows it to be cached by a
//      IMILResourceCache and supports extracting a realization for multiple
//      contexts.
//
//------------------------------------------------------------------------------
class CHwCacheablePoolBrush :
    public IMILCacheableResource,
    public CMILPoolResource,
    public CHwBrush
{
public:
    DEFINE_POOLRESOURCE_REF_COUNT_BASE

    override STDMETHOD(CacheAddRef)(void)
    {
        AddRef();
        return S_OK;
    }

    override STDMETHOD_(void, CacheRelease)(void)
    {
        Release();
    }

    virtual HRESULT SetBrushAndContext(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext
        ) PURE;

protected:

    CHwCacheablePoolBrush(
        __in_ecount(1) IMILPoolManager *pManager,
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) :
        CMILPoolResource(pManager),
        CHwBrush(pDevice)
    {
        m_oListEntry.Next = NULL;

        // Use head initialization so RemoveListHead can always be use, even if
        // this is not added to a managers list.
        InitializeListHead(&m_leAll);

        m_fValid = TRUE;
    }

    virtual ~CHwCacheablePoolBrush()
    {
        // Remove from managers list (or its own list is never added to a
        // manager's list.)
        RemoveEntryList(&m_leAll);
    }

protected:
    //
    // Give manager class friend access so that it may store
    // management data per resource.
    //

    friend CHwBrushPoolManager;

    //
    // Pool management data
    //

    union {
        SLIST_ENTRY m_oListEntry;
        CHwCacheablePoolBrush *m_pcpbNextInPool;
    };
    LIST_ENTRY m_leAll;
    BOOL m_fValid;

};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwBrushPoolManager
//
//  Synopsis:
//      Provides the basic management of a HW brush realization pool that allows
//      reuse of brushes no longer cached to the device independent brush
//      object.
//
//      The lifetime is controlled by the owner and the existence of any
//      currently cached brush realizations, since those realizations are not
//      tracked by the manager, but will always call back to the manager to
//      control its lifetime.
//
//      This class has one pure virtual method to create a brush of a specific
//      type.
//
//------------------------------------------------------------------------------
class CHwBrushPoolManager :
    public IMILPoolManager
{
public:

    //
    // Manager methods called by child resources
    //

    // Used to notify the manager that there are no outstanding uses and
    //  the manager has full control.
    override void UnusedNotification(
        __inout_ecount(1) CMILPoolResource *pUnused
        );

    // Used to notify the manager that the resource is no longer usable
    //  and should be removed from the pool.
    override void UnusableNotification(
        __inout_ecount(1) CMILPoolResource *pUnusable
        );

    //
    // Methods called by the brush pool
    //

    HRESULT AllocateHwBrush(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext,
        __deref_out_ecount(1) CHwBrush ** const ppHwBrush
        );

    void ReleaseUnusedBrushes();

    void Release();

protected:

    CHwBrushPoolManager(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );
    virtual ~CHwBrushPoolManager();

    virtual HRESULT CreateHwBrush( 
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext,
        __deref_out_ecount(1) CHwBrush ** const ppHwBrush
        ) PURE;

    void AddToList(
        __inout_ecount(1) CHwCacheablePoolBrush *pNewHwBrush
        );

private:

    // This method reduces the count of brushes that will call this
    // manager at some time.  When there are no outstanding brushes
    // and the pool, which created this pool manager, Release's it,
    // the count will reach -1 and the object will be deleted.
    MIL_FORCEINLINE void DecOutstanding()
    {
        LONG cOutstanding = InterlockedDecrement(&m_cOutstandingBrushes);

        if (cOutstanding == -1)
        {
            #if DBG
            Assert(m_fDbgReleased);
            #endif
            delete this;
        }
    }

    void ConsolidateUnusedLists();
    void MarkAllBrushesInvalid();
    void Remove(
        __inout_ecount(1) CHwCacheablePoolBrush *pcpbOld
        );

private:
    // Thread safe list of brushes that have recently become unused
    SLIST_HEADER m_oUnusedListHead;

    // List of all brushes
    LIST_ENTRY m_leAllHead;

    // Count of brushes in readily available list
    ULONG m_cReadyToUse;

    // List of brushes that are readily available
    CHwCacheablePoolBrush *m_pcpbHead;
    CHwCacheablePoolBrush *m_pcpbTail;

    // Count of all brushes currently in use.  When the manager
    // has been released by the referencing pool object this
    // value is decremented by 1 thus enabling it to reach -1.
    // When the count is -1 this manager should be deleted.
    LONG m_cOutstandingBrushes;

protected:
    // Non-ref'ed pointer to D3D device abstraction
    CD3DDeviceLevel1 *m_pDeviceNoRef;

#if DBG
private:
    // If true this manager is being released from the pool and
    // once all outstanding brushes are dereferenced this
    // object should be deleted.
    BOOL m_fDbgReleased;
#endif DBG
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwLinearGradientBrushPoolManager
//
//  Synopsis:
//      This is the linear gradient brush specific version of a
//      CHwBrushPoolManager.  Its specialization is the ability to realize a D3D
//      version of a CMILBrushLinearGradient.
//
//------------------------------------------------------------------------------
class CHwLinearGradientBrushPoolManager
    : public CHwBrushPoolManager
{
public:

protected:

    //
    // Give container class sole ability to create a pool manager
    //

    friend CHwBrushPool;

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBrushPoolManager));

    CHwLinearGradientBrushPoolManager(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) :
        CHwBrushPoolManager(pDevice)
    {
    }

protected:

    override HRESULT CreateHwBrush(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext,
        __deref_out_ecount(1) CHwBrush ** const ppHwBrush
        );
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwRadialGradientBrushPoolManager
//
//  Synopsis:
//      This is the radial gradient brush specific version of a
//      CHwBrushPoolManager.  Its specialization is the ability to realize a D3D
//      version of a CMILBrushRadialGradient.
//
//------------------------------------------------------------------------------
class CHwRadialGradientBrushPoolManager
    : public CHwBrushPoolManager
{
public:

protected:

    //
    // Give container class sole ability to create a pool manager
    //

    friend CHwBrushPool;

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBrushPoolManager));

    CHwRadialGradientBrushPoolManager(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        ) :
        CHwBrushPoolManager(pDevice)
    {
    }

protected:

    override HRESULT CreateHwBrush(
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext,
        __deref_out_ecount(1) CHwBrush ** const ppHwBrush
        );
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwBrushPool
//
//  Synopsis:
//      Controls realized instances of each brush type.  This class will take a
//      device independent brush and return a HW Brush. The returned brush may
//      already have realized color sources if it was cached.
//
//      It will delegate allocation of different brush types to type specific
//      brush managers or in the case of solid brushes a single scratch brush
//      with no resources of its own will be used.
//
//      This pool is intended to live in a CD3DDeviceLevel1 as a member.
//
//------------------------------------------------------------------------------
class CHwBrushPool
{
public:
    CHwBrushPool();
    ~CHwBrushPool();
    
    HRESULT Init(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    HRESULT GetHwBrush( 
        __inout_ecount(1) CMILBrush *pBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext,
        __deref_out_ecount(1) CHwBrush ** const ppHwBrush
        );

private:

    CHwSolidBrush *m_psbScratch;

    CHwLinearGradientBrushPoolManager *m_pbpmGradientLinear;
    CHwRadialGradientBrushPoolManager *m_pbpmGradientRadial;

    CHwBitmapBrush *m_pbbScratch;

};




