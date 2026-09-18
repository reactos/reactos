// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CMILCacheableResource, CMILResourceCache, and
//      CMILResourceIndex declarations
//
//      These caching classes provide a mechanism to associate (and hold a
//      reference to) an object derived from CMILCacheableResource with another
//      object derived from or containing a CMILResourceCache.
//
//      The most common use of this is to associate a device dependent resource
//      such as a HwColorSource to some device independent CMILObject such as a
//      brush.
//-----------------------------------------------------------------------------

#pragma once

MtExtern(CMILSimpleResourceCache);

//------------------------------------------------------------------------------
//
//  Interface: IMILCacheableResource
//
//  Description:
//     Interface to a cacheable resource.
//
//------------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IMILCacheableResource

DECLARE_INTERFACE_(IMILCacheableResource, IMILRefCount)
{
    STDMETHOD(CacheAddRef)(
        THIS
        ) PURE;

    STDMETHOD_(void, CacheRelease)(
        THIS
        ) PURE;

    virtual bool IsValid() const PURE;
};


//------------------------------------------------------------------------------
//
//  Interface: IMILResourceCache
//
//  Description:
//     Interface to an object that supports cacheable resource
//     being attached to it.
//
//------------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IMILResourceCache

DECLARE_INTERFACE_(IMILResourceCache, IUnknown)
{
protected:
    // PREfast doesn't complain that these are static constants, but then it
    // also doesn't seem to validate range - no defects when specifically
    // violated.  Feel free to make these #defines if needed by PREfast/fix.
    static const ULONG MinValidIndex = 0;
    static const ULONG MaxValidIndex = 0xFFFFFFFE;

public:
    typedef __range(MinValidIndex, MaxValidIndex) ULONG ValidIndex;

    STDMETHOD(GetResource)(ValidIndex uIndex, __deref_out_ecount_opt(1) IMILCacheableResource **ppResource) PURE;
    STDMETHOD(SetResource)(ValidIndex uIndex, __in_ecount_opt(1) IMILCacheableResource *pResource) PURE;
    STDMETHOD_(void, GetUniquenessToken)(__out_ecount(1) UINT *puToken) const PURE;
};


//------------------------------------------------------------------------------
//
//  Class: CMILCacheableResource
//
//  Description:
//     Base interface to a cacheable resource.
//
//------------------------------------------------------------------------------

class CMILCacheableResource :
    public IMILCacheableResource
{
public:

    STDMETHOD(CacheAddRef)(void) override
    {
        AddRef();
        return S_OK;
    }

    STDMETHOD_(void, CacheRelease)(void) override
    {
        Release();
    }
};


//------------------------------------------------------------------------------
//
//  Class: CMILResourceCache
//
//  Description:
//     Base class for an object that supports cacheable resource
//     being attached to it.
//
//------------------------------------------------------------------------------

#define RESOURCE_CACHE_SINGLE_THREADED  0

#define RESOURCE_CACHE_INITIAL_SIZE     2

class CMILResourceCache :
    public IMILResourceCache,
#if RESOURCE_CACHE_INITIAL_SIZE
    protected DynArrayIA<IMILCacheableResource *, RESOURCE_CACHE_INITIAL_SIZE>
#else
    protected DynArray<IMILCacheableResource *>
#endif
{
public:

    static const ULONG InvalidToken = 0xFFFFFFFF;
    static const ULONG SwRealizationCacheIndex = MinValidIndex;

    static HRESULT AllocateResourceIndex(__out_ecount(1) ValidIndex *puIndex);
    static HRESULT ReleaseResourceIndex(ValidIndex uIndex);

    STDMETHOD(GetResource)(ValidIndex uIndex, __deref_out_ecount_opt(1) IMILCacheableResource **ppResource) override;
    STDMETHOD(SetResource)(ValidIndex uIndex, __in_ecount_opt(1) IMILCacheableResource *pResource) override;

    HRESULT ReleaseResources();

    HRESULT ReleaseOtherResources(ValidIndex uIndex);

protected:
    CMILResourceCache();
    ~CMILResourceCache();

    HRESULT EnsureCount(UINT cNeeded);

private:
#if RESOURCE_CACHE_SINGLE_THREADED
    // Check the single threading assumptions
    void Enter() { Assert(InterlockedIncrement(&m_cInCall) == 1); }
    void Leave() { Assert(InterlockedDecrement(&m_cInCall) == 0); }
#endif RESOURCE_CACHE_SINGLE_THREADED

#if !RESOURCE_CACHE_SINGLE_THREADED || DBG
    volatile LONG m_cInCall;
#endif !RESOURCE_CACHE_SINGLE_THREADED || DBG
};


//------------------------------------------------------------------------------
//
//  Class: CMILResourceIndex
//
//  Description:
//     Class that tracks allocation of a resource index from CMILResourceCache.
//     This class should be used as a base class so that the index will
//     be released after any resources that are cached using that index.
//
//     NOTE: It is the derived classes' responsibility to invalidate
//     all resources cached using the allocated resource index.
//
//------------------------------------------------------------------------------

class CMILResourceIndex
{
protected:
    CMILResourceIndex()
    {
        m_uCacheIndex = CMILResourceCache::InvalidToken;
    }

    ~CMILResourceIndex();

    HRESULT AcquireIndex();

protected:
    ULONG m_uCacheIndex;

};


//------------------------------------------------------------------------------
//
//  Class: CMILSimpleResourceCache
//
//  Description:
//     Simple caching class that can be used as a member of another class. 
//     Implements ref-counting and IMILCacheableResource to enable nested cache
//     usage. 
//
//------------------------------------------------------------------------------

class CMILSimpleResourceCache
{
    // PREfast doesn't complain that these are static constants, but then it
    // also doesn't seem to validate range - no defects when specifically
    // violated.  Feel free to make these #defines if needed by PREfast/fix.
    static const ULONG MinValidIndex = 0;
    static const ULONG MaxValidIndex = 0xFFFFFFFE;

public:
    typedef __range(MinValidIndex, MaxValidIndex) ULONG ValidIndex;

public:
    CMILSimpleResourceCache() {}
    ~CMILSimpleResourceCache();
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMILSimpleResourceCache));

    HRESULT GetResource(__in ValidIndex index, __out_opt IMILCacheableResource **ppResource);
    HRESULT SetResource(__in ValidIndex index, __in_opt IMILCacheableResource *pResource);

private:
    DynArrayIA<IMILCacheableResource*, RESOURCE_CACHE_INITIAL_SIZE> m_resources;
};




