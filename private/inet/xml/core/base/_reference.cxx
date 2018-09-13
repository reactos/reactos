/*
 * @(#)_reference.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */


#include "core.hxx"
#pragma hdrstop

DeclareTag(tagRentalAssign, "Base", "warn on assignment: rental = non-rental");

extern HINSTANCE g_hInstance;

#if DBG == 1
#ifdef RENTAL_MODEL
BOOL checkRentalAssign(void * pWhere, void * pWhat)
{
    Base * pBaseWhere = LockingIsObject(pWhere);
    Base * pBaseWhat = LockingIsObject(pWhat);
    if (pBaseWhere && pBaseWhat)
    {
        if (!pBaseWhere->isRental() && pBaseWhat->isRental())
            return FALSE;
#if DBG == 1
        if (pBaseWhere->isRental() && !pBaseWhat->isRental())
            TraceTag((tagRentalAssign, "Warning ! Assigning non-rental object (%p) to rental object (%p)", pBaseWhat, pBaseWhere));
#endif
    }
    return TRUE;
}
#endif
#endif

void assign(IUnknown ** ppref, void * pref)
{
    IUnknown *punkRef = *ppref;
    if (pref)
    {
#ifdef RENTAL_MODEL
        Assert(checkRentalAssign(ppref, pref));
#endif
        ((Object *)pref)->AddRef();
    }
    (*ppref) = (Object *)pref; 
    if (punkRef) punkRef->Release();
}    

void assignMT(IUnknown ** ppref, void * pref)
{
    if (pref) ((Object *)pref)->AddRef();
    IUnknown * pprev = (IUnknown *)INTERLOCKEDEXCHANGE_PTR(ppref, pref);
    if (pprev) pprev->Release();
}    

void release(IUnknown ** ppref)
{
    IUnknown * pUnk = *ppref;
    if (pUnk) 
    {
        *ppref = NULL;
        pUnk->Release();
    }
}

void releaseMT(IUnknown ** ppref)
{
    IUnknown * pprev = (IUnknown *)INTERLOCKEDEXCHANGE_PTR(ppref, 0);
    if (pprev) pprev->Release();
}


void assignRO(IUnknown ** ppref, IUnknown * pref, const bool addRef)
{

    if (pref)
    {
#ifdef RENTAL_MODEL
        Assert(checkRentalAssign(ppref, pref));
#endif
        if (addRef)
        {
            pref->AddRef();
        }
    }

    IUnknown * pprev = *ppref;

    if (IsAddRef(pprev))
    {
        pprev = RawPointer(pprev);            
        if (pprev)
        {
            pprev->Release();
        }
    }

    *ppref =  MakePointer(pref, addRef);
}


void weakAssign(Object ** ppref, void * pref)
{
    Object *pObj = *ppref;
#if DBG == 1
    Assert(!pref || LockingIsObject(pref));
#endif
    if (pref) 
    {
#ifdef RENTAL_MODEL
        Assert(checkRentalAssign(ppref, pref));
#endif
        ((Object *)pref)->weakAddRef();
    }
    (*ppref) = (Object *)pref; 
    if (pObj) pObj->weakRelease();
}    

void weakRelease(Object ** ppref)
{
    if (*ppref) 
    {
        (*ppref)->weakRelease();
        *ppref = NULL;
    }
}

_globalreference * _globalreference::Object;
extern void TlsClear();

void ClearReferences()
{
    _globalreference * ref = _globalreference::Object;

    while(ref)
    {
        IUnknown * p = ref->_p;
        if (p)
        {
            p->Release();
            ref->_p = null;
        }
        ref = ref->_next;
    }

    TlsClear();
}

extern ShareMutex * g_pMutexSR;

void _globalreference::assign(void * pref)
{
    if (!_next)
    {
        MutexLock lock(g_pMutexSR);
        if (!_next)
        {
            _next = Object;
            Object = this;
        }
    }
#if DBG == 1 && defined(RENTAL_MODEL)
    if (pref)
    {
        Base * pBase = LockingIsObject(pref);
        Assert(!pBase || !pBase->isRental());
    }
#endif
    ::assignMT(&_p, pref);
}

//
// helper function for pulling ITypeInfo out of our typelib
//
HRESULT GetTypeInfo(REFGUID libid, int ord, LCID lcid, REFGUID uuid, ITypeInfo **ppITypeInfo)
{
    HRESULT    hr;
    ITypeLib  *pITypeLib;

    // Just in case we can't find the type library anywhere
    *ppITypeInfo = NULL;

    /*
     * The type libraries are registered under 0 (neutral),
     * 7 (German), and 9 (English) with no specific sub-
     * language, which would make them 407 or 409 and such.
     * If you are sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here.
     */
    // BUGBUG don't do this yet...
    hr = E_FAIL; //LoadRegTypeLib(libid, 1, 0, PRIMARYLANGID(lcid), &pITypeLib);

    /*
     * If LoadRegTypeLib fails, try loading directly with
     * LoadTypeLib, which will register the library for us.
     * Note that there's no default case here because the
     * prior switch will have filtered lcid already.
     *
     * NOTE:  You should prepend your DIR registry key to the
     * .TLB name so you don't depend on it being it the PATH.
     * This sample will be updated later to reflect this.
     */
    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];
        //
        // BUGBUG - Should use unicode on WinNT
        //
        char szPath[MAX_PATH];
        GetModuleFileNameA(g_hInstance, szPath, sizeof(szPath)/sizeof(szPath[0]));
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, sizeof(szPath)/sizeof(szPath[0]));
        int l = _tcslen(wszPath);
        // BUGBUG can handle up to 9 type libs this way...
        Assert(ord < 10);
        wszPath[l++] = L'\\';
        wszPath[l++] = L'0' + ord;
        wszPath[l] = L'\0';

        switch (PRIMARYLANGID(lcid))
        {
        case LANG_NEUTRAL:
        case LANG_ENGLISH:
            hr = LoadTypeLib(wszPath, &pITypeLib);
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        //Got the type lib, get type info for the interface we want
        hr = pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }

    return(hr);
}

