// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Visual cache manager.
//
//------------------------------------------------------------------------------
MtExtern(CVisualCacheManager);
 
class CVisualCacheManager : public CMILRefCountBase
{

private:
    // Constructor
    CVisualCacheManager(
        __in CComposition *pComposition,
        __in CMILFactory *pFactory
        );

    // Destructor
    virtual ~CVisualCacheManager();

protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CVisualCacheManager));

public:
    // CVisualCacheManager factory
    static HRESULT Create(
        __in_ecount(1) CComposition *pComposition,
        __in_ecount(1) CMILFactory *pFactory,
        __deref_out_ecount(1) CVisualCacheManager **ppVisualCacheManager
        );

    HRESULT MarkCacheForUpdate(CMilVisualCacheSet *pVisualCaches);

    HRESULT UpdateCaches();

    HRESULT RegisterVisualCache(
        __in_ecount(1) CMilVisualCacheSet *pVisualCaches
        );
    
    bool UnregisterVisualCache(
        __in_ecount(1) CMilVisualCacheSet *pVisualCaches
        );

    void NotifyDeviceLost();

    HRESULT GetBaseRenderInterface(
        __deref_out_xcount(1) IRenderTargetInternal **ppIRT
        DBG_ANALYSIS_COMMA_PARAM(__out CoordinateSpaceId::Enum *pdbgTargetCoordSpaceId)
        );

private:
    // Composition object that owns this manager.
    CComposition *m_pCompositionNoRef;
    
    // The owner's factory object.
    CMILFactory *m_pFactoryNoRef;

    // Software render interface.
    IRenderTargetInternal *m_pSoftwareRenderInterface;
    
    // List of caches to be updated this frame.
    DynArray<CMilVisualCacheSet*> m_arrCachesToUpdateNoRef;
    
    // List of all caches currently registered with this composition, but does not hold references.
    DynArray<CMilVisualCacheSet*> m_arrAllDeviceCachesNoRef;
};

