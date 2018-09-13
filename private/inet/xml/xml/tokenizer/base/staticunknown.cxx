/*
 * @(#)StaticUnknown.cxx 1.0 3/12/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "staticunknown.hxx"

StaticUnknown* g_pUnkList = NULL;
long g_cComponents = 0;

HRESULT RegisterStaticUnknown(IUnknown** ppUnk)
{
    StaticUnknown* pEnt = new_ne StaticUnknown;
    if (pEnt == NULL)
        return E_OUTOFMEMORY;

    (*ppUnk)->AddRef();
    pEnt->ppUnk = ppUnk;
    pEnt->pNext = g_pUnkList;
    g_pUnkList = pEnt;
    return S_OK;
}

void ReleaseAllUnknowns()
{
    while (g_pUnkList != NULL)
    {
        IUnknown** ppUnk = g_pUnkList->ppUnk;
        (*ppUnk)->Release();
        *ppUnk = NULL;
        StaticUnknown* temp = g_pUnkList;
        g_pUnkList = g_pUnkList->pNext;
        delete temp;
    }
}

long IncrementComponents()
{
    return ::InterlockedIncrement(&g_cComponents);
}

long DecrementComponents()
{
    long result = ::InterlockedDecrement(&g_cComponents);
    if (g_cComponents == 0)
    {
        ::ReleaseAllUnknowns();
    }
    return result;
}


