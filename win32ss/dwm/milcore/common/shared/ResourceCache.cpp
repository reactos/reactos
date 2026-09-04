// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CMILResourceCache and CMILResourceIndex implementations and
//      complete definition for CMILResourceCacheIndexManager and
//      CMILCacheableResourceDummy global, helper classes
//

#include "precomp.hpp"


#define MAX_CACHE_INDICES  32

DeclareTag(tagNoCaching, "MIL", "No Caching");
MtDefine(CMILSimpleResourceCache, MILRender, "CMILSimpleResourceCache");

#ifdef __cplusplus
extern "C" {
#endif

// copied from wdm.h, which cannot be included due to hundreds of redefinitions
 
typedef struct _RTL_BITMAP {
    ULONG SizeOfBitMap;                     // Number of bits in bit map
    PULONG Buffer;                          // Pointer to the bit map itself
} RTL_BITMAP;
typedef RTL_BITMAP *PRTL_BITMAP;

__drv_maxIRQL(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap (
    __out PRTL_BITMAP BitMapHeader,
    __in __drv_aliasesMem PULONG BitMapBuffer,
    __in ULONG SizeOfBitMap
    );

NTSYSAPI
VOID
NTAPI
RtlSetBits (
    __in PRTL_BITMAP BitMapHeader,
    __in_range(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    __in_range(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
    );

__success(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet (
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG NumberToFind,
    __in ULONG HintIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits (
    __in PRTL_BITMAP BitMapHeader
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet (
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG StartingIndex,
    __in ULONG Length
    );

NTSYSAPI
VOID
NTAPI
RtlClearBits (
    __in PRTL_BITMAP BitMapHeader,
    __in_range(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    __in_range(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
    );

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------
//
//  Class: CMILResourceCacheIndexManager
//
//  Description:
//     Class that will maintain the indices that may be used with
//     a CMILResourceCache object
//
//------------------------------------------------------------------------------

class CMILResourceCacheIndexManager
{
public:
    CMILResourceCacheIndexManager();
    ~CMILResourceCacheIndexManager();

    HRESULT AllocateIndex(__out_ecount(1) IMILResourceCache::ValidIndex *puIndex);
    HRESULT ReleaseIndex(IMILResourceCache::ValidIndex uIndex);

private:
    CCriticalSection m_cs;
    RTL_BITMAP m_BitMap;
    ULONG m_BitMapBuffer[(MAX_CACHE_INDICES+31)/32];
};

CMILResourceCacheIndexManager g_ResourceCacheIndexManager;


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCacheIndexManager::CMILResourceCacheIndexManager
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CMILResourceCacheIndexManager::CMILResourceCacheIndexManager()
{
    Assert(MAX_CACHE_INDICES <= sizeof(m_BitMapBuffer)*8);

    HRESULT hr;

    hr = THR(m_cs.Init());

    if (SUCCEEDED(hr))
    {
        ZeroMemory(m_BitMapBuffer, sizeof(m_BitMapBuffer));

        RtlInitializeBitMap(
            &m_BitMap,
            m_BitMapBuffer,
            MAX_CACHE_INDICES
            );

        Assert(m_BitMap.SizeOfBitMap <= sizeof(m_BitMapBuffer)*8);

        // Reserve Sw cache location
        RtlSetBits(&m_BitMap, CMILResourceCache::SwRealizationCacheIndex, 1);
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCacheIndexManager::~CMILResourceCacheIndexManager
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CMILResourceCacheIndexManager::~CMILResourceCacheIndexManager()
{
    if (m_cs.IsValid())
    {
        Assert(RtlNumberOfSetBits(&m_BitMap) == 1);
        // For some reason ntdll.dll doesn't export RtlTestBit
        //Assert(RtlTestBit(&m_BitMap, CMILResourceCache::SwRealizationCacheIndex));
        Assert(RtlAreBitsSet(&m_BitMap, CMILResourceCache::SwRealizationCacheIndex, 1));
    }
    m_cs.DeInit();
}


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCacheIndexManager::AllocateIndex
//
//  Synopsis:  Find an unused index and return it
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCacheIndexManager::AllocateIndex(
    __out_ecount(1) IMILResourceCache::ValidIndex *puIndex
    )
{
    Assert(puIndex);

    if (!m_cs.IsValid()) RRETURN(E_FAIL);

    if (IsTagEnabled(tagNoCaching)) RRETURN(E_FAIL);

    HRESULT hr = S_OK;

    m_cs.Enter();

    *puIndex = RtlFindClearBitsAndSet(&m_BitMap, 1, 0);

    if (*puIndex == -1)
    {
        hr = THR(E_OUTOFMEMORY);
    }

    m_cs.Leave();

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCacheIndexManager::ReleaseIndex
//
//  Synopsis:  Mark the given index as free
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCacheIndexManager::ReleaseIndex(
    IMILResourceCache::ValidIndex uIndex
    )
{
    HRESULT hr = S_OK;

    m_cs.Enter();

    // For some reason ntdll.dll doesn't export RtlTestBit
    //Assert(RtlTestBit(&m_BitMap, uIndex));
    Assert(RtlAreBitsSet(&m_BitMap, uIndex, 1));

    RtlClearBits(&m_BitMap, uIndex, 1);

    m_cs.Leave();

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::AllocateResourceIndex
//
//  Synopsis:  Delegate allocation to global CMILLResourceCacheIndexManager
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCache::AllocateResourceIndex(
    __out_ecount(1) ValidIndex *puIndex
    )
{
    HRESULT hr;

    hr = THR(g_ResourceCacheIndexManager.AllocateIndex(puIndex));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::ReleaseResourceIndex
//
//  Synopsis:  Delegate release to global CMILLResourceCacheIndexManager
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCache::ReleaseResourceIndex(
    ValidIndex uIndex
    )
{
    HRESULT hr;

    hr = THR(g_ResourceCacheIndexManager.ReleaseIndex(uIndex));

    RRETURN(hr);
}



#if !RESOURCE_CACHE_SINGLE_THREADED
//------------------------------------------------------------------------------
//
//  Class: CMILCacheableResourceDummy
//
//  Description:
//     A dummy a CMILCacheableResource object that can be placed in a
//     resource cache and an invalid entry.
//
//------------------------------------------------------------------------------

class CMILCacheableResourceDummy :
    public IMILCacheableResource
{
    STDMETHOD_(ULONG, AddRef)(void) override { return 0; }
    STDMETHOD_(ULONG, Release)(void) override { return 0; }
    STDMETHOD(CacheAddRef)(void) override { return E_POINTER; }
    STDMETHOD_(void, CacheRelease)(void) override {}
    bool IsValid(void) const override { return false; }
} g_MILCacheableResourceDummy;

#define INVALID_CACHEABLE_RESOURCE  (&g_MILCacheableResourceDummy)

#define RESOURCE_CACHE_EXCLUSIVE_RELEASE    LONG_MIN
#define RESOURCE_CACHE_EXCLUSIVE_RELEASE_FLAG   0x40000000
#define RESOURCE_CACHE_EXCLUSIVE_GROW       (LONG_MIN & ~RESOURCE_CACHE_EXCLUSIVE_RELEASE_FLAG)

#endif !RESOURCE_CACHE_SINGLE_THREADED


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::CMILResourceCache
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CMILResourceCache::CMILResourceCache()
{
#if !RESOURCE_CACHE_SINGLE_THREADED || DBG
    m_cInCall = 0;
#endif !RESOURCE_CACHE_SINGLE_THREADED || DBG
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::~CMILResourceCache
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CMILResourceCache::~CMILResourceCache()
{
    Assert(m_cInCall == 0);

    while (Count > 0)
    {
        IMILCacheableResource *pIResource;

        pIResource = GetDataBuffer()[--Count];
        if (pIResource)
        {
            pIResource->CacheRelease();
        }
    }
}


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::GetResource
//
//  Synopsis:  Lookup up the resource at the given index and tests it
//             for basic validity before returning a pointer.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CMILResourceCache::GetResource(
    ValidIndex uIndex,
    __deref_out_ecount_opt(1) IMILCacheableResource **ppResource
    )
{
    if (IsTagEnabled(tagNoCaching)) RRETURN(E_FAIL);

    HRESULT hr = S_OK;

#if RESOURCE_CACHE_SINGLE_THREADED
    Enter();
#else !RESOURCE_CACHE_SINGLE_THREADED
    LONG cInCall;

    //
    // Check entrancy with simple interlocking
    // protection mechanism.
    //

    cInCall = InterlockedIncrement(&m_cInCall);
    Assert(cInCall != 0);

    if (cInCall < 0)
    {
        if (cInCall & RESOURCE_CACHE_EXCLUSIVE_RELEASE_FLAG)
        {
            //
            // Decrement is not allowed on failed entrance
            // since the exclusive lock could have been
            // released during this time.
            //

            RRETURN(E_ACCESSDENIED);
        }

        //
        // We hit upon an exclusive lock for growth.
        // In this case we wait for any reallocation to
        // take place.  When the growth is completed
        // the call count will preserve our original
        // increment.
        //

        while (m_cInCall < 0)
        {
            Assert(m_cInCall != 0);
            SleepEx(0, TRUE);
        }
        Assert(m_cInCall > 0);

    }
#endif RESOURCE_CACHE_SINGLE_THREADED

    // Set default value for returned resource
    *ppResource = NULL;

    //
    // We don't have to worry about Count shrinking
    // on us ever; if there is a pending set that
    // would make Count high enough, that is just too
    // bad.
    //

    if (uIndex < Count)
    {
        IMILCacheableResource *pResource;

#if RESOURCE_CACHE_SINGLE_THREADED
        pResource = GetDataBuffer()[uIndex];
#else !RESOURCE_CACHE_SINGLE_THREADED

        IMILCacheableResource * volatile *ppResourceEntry;

        ppResourceEntry = &GetDataBuffer()[uIndex];

        //
        // Grab the stored resource and temporarily set
        // the entry to INVALID.  This will not hinder the
        // ability to update this entry on with a Set call
        // and it makes coincident Gets spin.
        //

        do
        {
            pResource = *ppResourceEntry;
            if (pResource == INVALID_CACHEABLE_RESOURCE)
            {
                //
                // We just read the entry and it is invalid
                // indicating that we are handling another
                // coincident Get.  Just give up the rest of
                // this time slice so the other Get may
                // complete its AddRef
                //

                SleepEx(0, TRUE);

                //
                // Reset the compare to a valid value for
                // this loop to exit on.  There is only one
                // good value that we always know about: NULL.
                //

                pResource = NULL;
            }
        } while (pResource !=
                 InterlockedCompareExchangePointer(
                     reinterpret_cast<volatile PVOID *>(ppResourceEntry),
                     INVALID_CACHEABLE_RESOURCE,
                     pResource)
                 );
#endif RESOURCE_CACHE_SINGLE_THREADED

        //
        // Check if we've snagged a valid resource
        //

        if (pResource)
        {
#if !RESOURCE_CACHE_SINGLE_THREADED
            Assert(pResource != INVALID_CACHEABLE_RESOURCE);
#endif !RESOURCE_CACHE_SINGLE_THREADED

            //
            // Note that we have no protection on when a resource
            // can be made invalid.  For now we assume that we
            // will not be invalidating any resources while
            // we may be actively trying to use them.  Since, we
            // are in a Get we assume that this thread is trying
            // to use the resource and there are external
            // protections.
            //

            if (pResource->IsValid())
            {
                //
                // Return the resource
                //

                pResource->AddRef();
                *ppResource = pResource;
            }
            else
            {
                //
                // Release this invalid resource
                //

                pResource->CacheRelease();
                pResource = NULL;
#if RESOURCE_CACHE_SINGLE_THREADED
                GetDataBuffer()[uIndex] = NULL;
#endif RESOURCE_CACHE_SINGLE_THREADED
            }
        }

#if !RESOURCE_CACHE_SINGLE_THREADED
        //
        // Try to restore this resource to the cache entry.
        // If the resource is invalid we are actually placing
        // NULL into this entry.
        //

        if (INVALID_CACHEABLE_RESOURCE !=
            InterlockedCompareExchangePointer(
                reinterpret_cast<volatile PVOID *>(ppResourceEntry),
                pResource,
                INVALID_CACHEABLE_RESOURCE)
            )
        {
            //
            // If there was a non-NULL set while we were busy referencing
            // this resource then we need to CacheRelease this resource.
            // Note that this won't invalidate the resource since we've
            // already taken another reference.
            //

            if (pResource)
            {
                pResource->CacheRelease();
            }
        }
#endif !RESOURCE_CACHE_SINGLE_THREADED
    }

#if RESOURCE_CACHE_SINGLE_THREADED
    Leave();
#else !RESOURCE_CACHE_SINGLE_THREADED
    //
    // Decrement to say we're done.  The new value
    // should always be non-negative.
    //

    cInCall = InterlockedDecrement(&m_cInCall);
    Assert(cInCall >= 0);
#endif RESOURCE_CACHE_SINGLE_THREADED

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::SetResource
//
//  Synopsis:  Places the resource at the specified index and release
//             any resource that was previously there.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CMILResourceCache::SetResource(
    ValidIndex uIndex,
    __in_ecount_opt(1) IMILCacheableResource *pResource
    )
{
    if (IsTagEnabled(tagNoCaching)) RRETURN(E_FAIL);

    HRESULT hr = S_OK;
    IMILCacheableResource **ppResourceEntry;
    IMILCacheableResource *pResourceOld;

#if RESOURCE_CACHE_SINGLE_THREADED
    Enter();
#else !RESOURCE_CACHE_SINGLE_THREADED
    LONG cInCall;

    //
    // Check entrancy with simple interlocking
    // protection mechanism.
    //

    cInCall = InterlockedIncrement(&m_cInCall);
    Assert(cInCall != 0);

    if (cInCall < 0)
    {
        if (cInCall & RESOURCE_CACHE_EXCLUSIVE_RELEASE_FLAG)
        {
            //
            // Decrement is not allowed on failed entrance
            // since the exclusive lock could have been
            // released during this time.
            //

            RRETURN(E_ACCESSDENIED);
        }

        //
        // We hit upon an exclusive lock for growth.
        // In this case we wait for any reallocation to
        // take place.  When the growth is completed
        // the call count will preserve our original
        // increment.
        //

        while (m_cInCall < 0)
        {
            Assert(m_cInCall != 0);
            SleepEx(0, TRUE);
        }
        Assert(m_cInCall > 0);
    }
#endif RESOURCE_CACHE_SINGLE_THREADED

    //
    // Make sure the required space is reserved
    //

    if (uIndex >= Count)
    {
        IFC(EnsureCount(uIndex+1));
    }

    Assert(Count > uIndex);
    ppResourceEntry = &GetDataBuffer()[uIndex];

    //
    // Reference the new resource
    //

    if (pResource)
    {
        IFC(pResource->CacheAddRef());
    }

    //
    // Swap new and old resources
    //
    // If we were in the middle of a Get then it is possible
    // that the resource entry is only temporarily INVALID.  Get
    // is responsible for releasing the old resource in this
    // case.
    //
    // Since the INVALID value Get sets is a global dummy
    // resource that doesn't care about CacheRelease(),
    // we don't have to do any special pointer checks.
    //
    // If another Set is happening then the last one wins.
    //

    #if !defined(_M_IA64) && !defined(_M_AMD64)
    #pragma warning(push)
    // Disable these for not IA64/AMD64 because InterlockedExchangePointer is
    // #defined to InterlockedExchange, which uses LONGs.
    #pragma warning(disable : 4311 4312 )
    #endif

    pResourceOld =
        static_cast<IMILCacheableResource *>
            (InterlockedExchangePointer((void **)ppResourceEntry, pResource));

    #if !defined(_M_IA64) && !defined(_M_AMD64)
    #pragma warning(pop)
    #endif

    //
    // Release any prior resource
    //

    if (pResourceOld)
    {
        pResourceOld->CacheRelease();
    }

Cleanup:
#if RESOURCE_CACHE_SINGLE_THREADED
    Leave();
#else !RESOURCE_CACHE_SINGLE_THREADED
    //
    // Decrement to say we're done.  The new value
    // should always be non-negative.
    //

    cInCall = InterlockedDecrement(&m_cInCall);
    Assert(cInCall >= 0);
#endif RESOURCE_CACHE_SINGLE_THREADED

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::ReleaseResources
//
//  Synopsis:  Releases all cached resources on this object
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCache::ReleaseResources()
{
    HRESULT hr = S_OK;

#if RESOURCE_CACHE_SINGLE_THREADED
    Enter();
#else !RESOURCE_CACHE_SINGLE_THREADED
    //
    // Spin until we can take exclusive access to this cache
    //

    while (0 != InterlockedCompareExchange(
                    &m_cInCall,
                    RESOURCE_CACHE_EXCLUSIVE_RELEASE,
                    0)
           )
    {
        SleepEx(0, TRUE);
    }
#endif RESOURCE_CACHE_SINGLE_THREADED

    UINT i = Count;
    IMILCacheableResource **ppResource = GetDataBuffer();

    while (i > 0)
    {
        if (*ppResource)
        {
            (*ppResource)->CacheRelease();
            *ppResource = NULL;
        }

        ppResource++;
        i--;
    }

#if RESOURCE_CACHE_SINGLE_THREADED
    Leave();
#else !RESOURCE_CACHE_SINGLE_THREADED
    //
    // No need to lock this assignment since we have exclusive access
    //

    Assert(m_cInCall < 0);
    m_cInCall = 0;
#endif RESOURCE_CACHE_SINGLE_THREADED

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::ReleaseOtherResources
//
//  Synopsis:  Releases all cached resources that don't match the given cache
//             index.
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCache::ReleaseOtherResources(
    ValidIndex uIndex
    )
{
    HRESULT hr = S_OK;

#if RESOURCE_CACHE_SINGLE_THREADED
    Enter();
#else !RESOURCE_CACHE_SINGLE_THREADED
    //
    // Spin until we can take exclusive access to this cache
    //

    while (0 != InterlockedCompareExchange(
                    &m_cInCall,
                    RESOURCE_CACHE_EXCLUSIVE_RELEASE,
                    0)
           )
    {
        SleepEx(0, TRUE);
    }
#endif RESOURCE_CACHE_SINGLE_THREADED

    IMILCacheableResource **ppResource = GetDataBuffer();

    for(ValidIndex i = 0; i < Count; i++)
    {
        if (i != uIndex && ppResource[i] != NULL)
        {
            ppResource[i]->CacheRelease();
            ppResource[i] = NULL;
        }
    }

#if RESOURCE_CACHE_SINGLE_THREADED
    Leave();
#else !RESOURCE_CACHE_SINGLE_THREADED
    //
    // No need to lock this assignment since we have exclusive access
    //

    Assert(m_cInCall < 0);
    m_cInCall = 0;
#endif RESOURCE_CACHE_SINGLE_THREADED

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceCache::EnsureCount
//
//  Synopsis:  Increase the space available to cache indices in
//
//-------------------------------------------------------------------------
HRESULT
CMILResourceCache::EnsureCount(
    UINT cNeeded
    )
{
    HRESULT hr = S_OK;

#if RESOURCE_CACHE_SINGLE_THREADED
    Assert(m_cInCall > 0);
#else !RESOURCE_CACHE_SINGLE_THREADED
    LONG cInCall;

    //
    // This thread must have taken shared access
    // before calling this routine.
    //
    // Release shared access to acquire
    // exclusive access
    //

    Assert(m_cInCall > 0);
    cInCall = InterlockedDecrement(&m_cInCall);
    Assert(cInCall >= 0);

    //
    // Spin until we can take exclusive access to this cache
    //

    while (0 != InterlockedCompareExchange(
                    &m_cInCall,
                    RESOURCE_CACHE_EXCLUSIVE_GROW,
                    0)
           )
    {
        SleepEx(0, TRUE);
    }
#endif RESOURCE_CACHE_SINGLE_THREADED

    //
    // Check if count is already sufficient
    //

    if (cNeeded > Count)
    {
        UINT cAdditional = (cNeeded - Count);

        //
        // Ensure we have the required capacity
        //

        if (cNeeded > Capacity)
        {
            // ReserveSpace is base on Count (not Capacity)
            IFC(ReserveSpace(cAdditional, TRUE));
        }

        Assert(Capacity >= cNeeded);

        //
        // Zero out any entries we haven't used before.
        //

        ZeroMemory(
            &(GetDataBuffer()[Count]),
            sizeof(IMILCacheableResource *) * cAdditional);
        Count = cNeeded;
    }

Cleanup:
#if !RESOURCE_CACHE_SINGLE_THREADED
    //
    // Release exclusive access and acquire
    // shared access at the same time making
    // sure to keep all blocked calls' increments
    // intact
    //

    LONG cCurrent;

    do
    {
        Assert(m_cInCall < 0);
        cCurrent = m_cInCall;
    } while (cCurrent != InterlockedCompareExchange(
                            &m_cInCall,
                            cCurrent - RESOURCE_CACHE_EXCLUSIVE_GROW + 1,
                            cCurrent));
#endif !RESOURCE_CACHE_SINGLE_THREADED

    Assert(m_cInCall > 0);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CMILResourceIndex::~CMILResourceIndex
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CMILResourceIndex::~CMILResourceIndex(
    )
{
    if (m_uCacheIndex != CMILResourceCache::InvalidToken)
    {
        CMILResourceCache::ReleaseResourceIndex(m_uCacheIndex);
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CMILResourceIndex::AcquireIndex
//
//  Synopsis:  Acquires a resource cache index for use
//
//-------------------------------------------------------------------------

HRESULT 
CMILResourceIndex::AcquireIndex(
    )
{
    Assert(m_uCacheIndex == CMILResourceCache::InvalidToken);

    RRETURN(CMILResourceCache::AllocateResourceIndex(&m_uCacheIndex));
}



//+------------------------------------------------------------------------
//
//  Function:  CMILSimpleResourceCache::GetResource
//
//  Synopsis:  Retrieves the resources at the specified cache index. 
//
//-------------------------------------------------------------------------

HRESULT 
CMILSimpleResourceCache::GetResource(
    __in ValidIndex uIndex, 
    __out_opt IMILCacheableResource **ppResource)
{
    UINT count = m_resources.GetCount();

    // The method returns NULL if no valid resource can be found therefore
    // initializing the out argument to NULL.

    *ppResource = NULL;

    if (uIndex < count)
    {
        IMILCacheableResource *pResource = m_resources[uIndex];
        if (pResource != NULL)
        {
            // Found a potential resource - need to check if it is still valid. 
            // Resources can become invalid for various reasons, including if we
            // loose a DX device against which a resource has been created. 
            if (pResource->IsValid())
            {
                // If the resource is still valid return it.
                pResource->AddRef();
                *ppResource = pResource;
            }
            else
            {
                // Resource is not valid anymore. Remove it from the cache and return
                // NULL. 
                pResource->CacheRelease();
                m_resources[uIndex] = NULL;
            }                
        }
    }
    
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  CMILSimpleResourceCache::dtor
//
//-------------------------------------------------------------------------

CMILSimpleResourceCache::~CMILSimpleResourceCache()
{
    UINT count = m_resources.GetCount();
    IMILCacheableResource *pResource = NULL;

    // Release all resources from the cache. 

    for (UINT i = 0; i < count; i++)
    {
        pResource = m_resources[i];
        m_resources[i] = NULL;

        if (pResource != NULL)
        {
            pResource->CacheRelease();
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:  CMILSimpleResourceCache::GetResource
//
//  Synopsis:  Sets the resource at the specified cache index.
//
//-------------------------------------------------------------------------

HRESULT 
CMILSimpleResourceCache::SetResource(
    __in ValidIndex uIndex, 
    __in_opt IMILCacheableResource *pResource)
{
    HRESULT hr = S_OK;

    UINT count = m_resources.GetCount();      

    //
    // Make sure the dynarray has enough space for holding the resource at 
    // the specified index. If there isn't enough space grow the dynarray
    // backing storage.

    if (uIndex >= count)
    {
        UINT additional = uIndex - count + 1;  
        IFC(m_resources.AddAndSet(additional, NULL));
    }

    {
        // Release the old resource.         
        IMILCacheableResource* pOldResource = m_resources[uIndex];
        if (pOldResource != NULL)
        {
            pOldResource->CacheRelease();
        }

        // Reference the new resource.
        if (pResource)
        {
            IFC(pResource->CacheAddRef());
        }

        m_resources[uIndex] = pResource;
    }

Cleanup:
    RRETURN(hr);
}


