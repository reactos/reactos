/*
 * @(#)StaticUnknown.cxx 1.0 3/12/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include <comcat.h>
#include "staticunknown.hxx"

StaticUnknown* g_pUnkList = NULL;
volatile long g_cComponents = 0;

extern ShareMutex * g_pMutexSR;


HRESULT RegisterStaticUnknown(IUnknown** ppUnk)
{
    // this should only be called from inside this thread
    Assert(GetTlsData() == g_pMutexSR->_ptlsdata);
    Assert(g_cComponents > 0);

    StaticUnknown* pNewEntry = new_ne StaticUnknown;
    if (NULL == pNewEntry)
        return E_OUTOFMEMORY;

    pNewEntry->ppUnk = ppUnk;
    pNewEntry->pNext = g_pUnkList;
    g_pUnkList = pNewEntry;

    // this should only be called from inside this thread
    Assert(GetTlsData() == g_pMutexSR->_ptlsdata);
    Assert(g_cComponents > 0);

    return S_OK;
}

void ReleaseAllUnknowns()
{
    MutexLock lock(g_pMutexSR);

    StaticUnknown * pHead = g_pUnkList;
    g_pUnkList = null;

TryAgain:
    TRY
    {
        while (pHead)
        {
            IUnknown ** ppUnk = pHead->ppUnk;
            IUnknown * pUnk = *ppUnk;
            *ppUnk = null;
            // if comonent count > 0, then someone may try and use it
            if (g_cComponents > 0)
            {
                *ppUnk = pUnk;
                // now we need to push 
                // what is left of the chain
                // back in the public chain
                g_pUnkList = pHead;
                break;
            }
            // release the static unknown
            pUnk->Release();
            // delete this entry in the list
            StaticUnknown* temp = pHead;
            pHead = pHead->pNext;
            delete temp;
        }
    }
    CATCH
    {
        // BUGBUG, The only thing which can throw an exception, is the 'delete'
        // thus it is safe to just resume the loop
        goto TryAgain;
        // this is better than rethrowing the exception, since it is more important
        // that we be play well with others, even in the case where we are running
        // out of memory, than that we report the error.
    }
    ENDTRY
}

long IncrementComponents()
{
    long l = ::InterlockedIncrement((long *)&g_cComponents);
    TraceTag((tagRefCount, "Component count after inc = %d", l));
    return l;
}

long DecrementComponents()
{
    long result = ::InterlockedDecrement((long *)&g_cComponents);
    Assert(result >= 0);
    TraceTag((tagRefCount, "Component count after dec = %d", result));
    if (g_cComponents == 0)
        ::ReleaseAllUnknowns();
    if (g_cComponents == 0)
    {
        Base::testForGC(Base::GC_FORCE);
    }
    return result;
}

long GetComponentCount()
{
    return g_cComponents;
}

#include "mlang.h"

HRESULT CreateMultiLanguage(IMultiLanguage ** ppUnk)
{
    HRESULT hr = S_OK;

    MutexLock lock(g_pMutexSR);

    // check again
    if (*ppUnk == NULL)
    {
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_IMultiLanguage2, (void**)ppUnk);
        if (hr)
        {
            hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_IMultiLanguage, (void**)ppUnk);
        }

        if (!hr)
        {
            // remember that we have a static foriegn IUnknown so we can clean it up later.
            hr = ::RegisterStaticUnknown((IUnknown **)ppUnk); 
            if (hr)
            {
                ::release(ppUnk);
            }
        }
    }

    return hr;
}

HRESULT CreateSecurityManager(IInternetSecurityManager ** ppUnk)
{
    HRESULT hr = S_OK;

    MutexLock lock(g_pMutexSR);

    // check again
    if (*ppUnk == NULL)
    {
        hr = CoCreateInstance(CLSID_InternetSecurityManager,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IInternetSecurityManager,
                                   (void **)ppUnk);
        if (!hr)
        {
            // Must register all static foreign IUnknowns (See base\StaticUnknown.hxx).
            hr = ::RegisterStaticUnknown((IUnknown **)ppUnk);
            if (hr)
            {
                ::release(ppUnk);
            }
        }
    }

    return hr;
}

HRESULT CreateCatalogInformation(ICatInformation ** ppUnk)
{
    HRESULT hr = S_OK;

    MutexLock lock(g_pMutexSR);

    // check again
    if (*ppUnk == NULL)
    {
        hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                                   NULL,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ICatInformation,
                                   (void **)ppUnk);
        if (!hr)
        {
            // Must register all static foreign IUnknowns (See base\StaticUnknown.hxx).
            hr = ::RegisterStaticUnknown((IUnknown **)ppUnk);
            if (hr)
            {
                ::release(ppUnk);
            }
        }
    }

    return hr;
}
