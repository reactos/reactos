//+------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       allocspy.cxx
//
//  Contents:   IMallocSpy implementation.
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifndef X_VMEM_HXX_
#define X_VMEM_HXX_
#include "vmem.hxx"
#endif

#include "vmem.cxx"

static int g_mtSpy = 0;

class CMallocSpy : public IMallocSpy
{
public:

    CMallocSpy();

    // IUnknown methods

    STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IMallocSpy methods

    STDMETHOD_(SIZE_T,  PreAlloc)(SIZE_T cbRequest);
    STDMETHOD_(void *, PostAlloc)(void *pvActual);
    STDMETHOD_(void *, PreFree)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(void,   PostFree)(BOOL fSpyed);
    STDMETHOD_(SIZE_T,  PreRealloc)(void *pvRequest, SIZE_T cbRequest, void **ppvActual, BOOL fSpyed);
    STDMETHOD_(void *, PostRealloc)(void *pvActual, BOOL fSpyed);
    STDMETHOD_(void *, PreGetSize)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(SIZE_T,  PostGetSize)(SIZE_T cbActual, BOOL fSpyed);
    STDMETHOD_(void *, PreDidAlloc)(void *pvRequest, BOOL fSpyed);
    STDMETHOD_(BOOL,   PostDidAlloc)(void *pvRequest, BOOL fSpyed, BOOL fActual);
    STDMETHOD_(void,   PreHeapMinimize)();
    STDMETHOD_(void,   PostHeapMinimize)();

    ULONG               _ulRef;
    BOOL                _fRegistered;
    BOOL                _fStrict;
    DWORD               _dwFlags;
    BOOL                _fMeter;
};

void EnterSpyAlloc()
{
    EnsureThreadState();
    DBGTHREADSTATE * pts = DbgGetThreadState();
    pts->fSpyAlloc += 1;
    pts->mtSpy = pts->mtSpyUser ? pts->mtSpyUser : g_mtSpy;
}

void LeaveSpyAlloc()
{
    TLS(fSpyAlloc) -= 1;
}

struct SPYBLK
{
    int     mt;
};

size_t
SpyPreAlloc(size_t cbRequest)
{
    EnsureThreadState();
    TLS(cbRequest) = cbRequest;
    return sizeof(SPYBLK);
}

void *
SpyPostAlloc(void * pvActual, DWORD dwFlags)
{
    EnsureThreadState();
    SPYBLK * psb = (SPYBLK *)pvActual;
    size_t cbRequest = TLS(cbRequest);
    void * pvRequest;

    if (!psb)
        return NULL;

    pvRequest = VMemAlloc(cbRequest, dwFlags, psb);

    if (pvRequest)
    {
        psb->mt = TLS(mtSpy);
        MtAdd(psb->mt, 1, (LONG)cbRequest);
    }

    return(pvRequest);
}

void *
SpyPreFree(void * pvRequest)
{
    if (!pvRequest)
        return NULL;

    VMEMINFO * pvmi = VMemIsValid(pvRequest);
    SPYBLK * psb = NULL;

    AssertSz((BOOL)(ULONG_PTR)pvmi, "SpyPreFree - can't find supposedly allocated block");
    
    if (pvmi)
    {
        psb = (SPYBLK *)pvmi->pvUser;
        MtAdd(psb->mt, -1, -(LONG)pvmi->cb);
        VMemFree(pvRequest);
    }

    return(psb);
}

void
SpyPostFree(void)
{
}

size_t
SpyPreRealloc(void *pvRequest, size_t cbRequest, void **ppv)
{
    EnsureThreadState();
    size_t          cb;
    DBGTHREADSTATE *pts  = DbgGetThreadState();

    pts->cbRequest = cbRequest;
    pts->pvRequest = pvRequest;

    if (pvRequest == NULL)
    {
        *ppv = NULL;
        cb = sizeof(SPYBLK);
    }
    else if (cbRequest == 0)
    {
        *ppv = SpyPreFree(pvRequest);
        cb = 0;
    }
    else
    {
        VMEMINFO * pvmi = VMemIsValid(pvRequest);

        AssertSz((BOOL)(ULONG_PTR)pvmi, "SpyPreRealloc - can't find supposedly allocated block");

        *ppv = pvmi->pvUser;
        cb = sizeof(SPYBLK);
    }

    return cb;
}

void *
SpyPostRealloc(void * pvActual, DWORD dwFlags)
{
    EnsureThreadState();
    DBGTHREADSTATE *pts         = DbgGetThreadState();
    void *          pvRequest   = pts->pvRequest;
    size_t          cbRequest   = pts->cbRequest;
    void *          pvReturn    = NULL;

    if (pvRequest == NULL)
    {
        pvReturn = SpyPostAlloc(pvActual, dwFlags);
    }
    else if (pts->cbRequest == 0)
    {
        Assert(pvActual == NULL);
        pvReturn = NULL;
    }
    else if (pvActual)
    {
        VMEMINFO *  pvmi = VMemIsValid(pvRequest);
        LONG        lVal = (LONG)(cbRequest - pvmi->cb);
        SPYBLK *    psb  = (SPYBLK *)pvActual;

        if (VMemRealloc(&pvRequest, cbRequest, dwFlags, psb) == S_OK)
        {
            MtAdd(psb->mt, 0, lVal);
            pvReturn = pvRequest;
        }
    }

    return pvReturn;
}

void *
SpyPreGetSize(void *pvRequest)
{
    EnsureThreadState();
    TLS(pvRequest) = pvRequest;
    VMEMINFO * pvmi = VMemIsValid(pvRequest);
    return pvmi ? pvmi->pvUser : NULL;
}

size_t
SpyPostGetSize(size_t cb)
{
    EnsureThreadState();
    DBGTHREADSTATE * pts = DbgGetThreadState();
    return pts->pvRequest ? VMemIsValid(pts->pvRequest)->cb : 0;
}

void *
SpyPreDidAlloc(void *pvRequest)
{
    return(VMemIsValid(pvRequest));
}

BOOL
SpyPostDidAlloc(void *pvRequest, BOOL fActual)
{
    return fActual;
}

struct MTRBLK
{
    LONG    cb;
    int     mt;
};

size_t
MtrPreAlloc(size_t cbRequest)
{
    EnsureThreadState();
    TLS(cbRequest) = cbRequest;
    return(sizeof(MTRBLK) + cbRequest);
}

void *
MtrPostAlloc(void * pvActual)
{
    EnsureThreadState();
    MTRBLK * pmb = (MTRBLK *)pvActual;

    if (!pmb)
        return NULL;

    pmb->cb = TLS(cbRequest);
    pmb->mt = TLS(mtSpy);

    MtAdd(pmb->mt, 1, pmb->cb);

    return(pmb + 1);
}

void *
MtrPreFree(void * pvRequest)
{
    if (!pvRequest)
        return NULL;

    MTRBLK * pmb = (MTRBLK *)pvRequest - 1;

    MtAdd(pmb->mt, -1, -pmb->cb);

    return(pmb);
}

void
MtrPostFree(void)
{
}

size_t
MtrPreRealloc(void *pvRequest, size_t cbRequest, void **ppv)
{
    EnsureThreadState();
    size_t          cb;
    DBGTHREADSTATE *pts = DbgGetThreadState();

    pts->cbRequest = cbRequest;
    pts->pvRequest = pvRequest;

    if (pvRequest == NULL)
    {
        *ppv = NULL;
        cb   = sizeof(MTRBLK) + cbRequest;
    }
    else if (cbRequest == 0)
    {
        *ppv = MtrPreFree(pvRequest);
        cb   = 0;
    }
    else
    {
        *ppv = (MTRBLK *)pvRequest - 1;
        cb   = sizeof(MTRBLK) + cbRequest;
    }

    return cb;
}

void *
MtrPostRealloc(void * pvActual)
{
    EnsureThreadState();
    void *          pvReturn;
    DBGTHREADSTATE *pts = DbgGetThreadState();
    MTRBLK *        pmb = pvActual ? (MTRBLK *)pvActual : NULL;

    if (pts->pvRequest == NULL)
    {
        pvReturn = MtrPostAlloc(pvActual);
    }
    else if (pts->cbRequest == 0)
    {
        Assert(pvActual == NULL);
        pvReturn = NULL;
    }
    else
    {
        if (pvActual == NULL)
        {
            pvReturn = NULL;
        }
        else
        {
            LONG lVal = (LONG)pts->cbRequest - (LONG)pmb->cb;

            MtAdd(pmb->mt, 0, (LONG)pts->cbRequest - (LONG)pmb->cb);

            pmb->cb = pts->cbRequest;

            pvReturn = pmb + 1;
        }
    }

    return pvReturn;
}

void *
MtrPreGetSize(void *pvRequest)
{
    return pvRequest ? (MTRBLK *)pvRequest - 1 : NULL;
}

size_t
MtrPostGetSize(size_t cb)
{
    return(cb ? cb - sizeof(MTRBLK) : 0);
}

void *
MtrPreDidAlloc(void *pvRequest)
{
    return(pvRequest ? (MTRBLK *)pvRequest - 1 : NULL);
}

BOOL
MtrPostDidAlloc(void *pvRequest, BOOL fActual)
{
    return fActual;
}

inline
CMallocSpy::CMallocSpy()
{
    _ulRef = 0;
}


STDMETHODIMP
CMallocSpy::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IMallocSpy)
    {
        *ppv = (IMallocSpy *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CMallocSpy::AddRef()
{
    return InterlockedIncrement((LONG *)&_ulRef);
}

STDMETHODIMP_(ULONG)
CMallocSpy::Release()
{
    return InterlockedDecrement((LONG *)&_ulRef);
}

STDMETHODIMP_(SIZE_T)
CMallocSpy::PreAlloc(SIZE_T cbRequest)
{
    ULONG ul;

    EnterSpyAlloc();

    if (_fStrict)
        ul = SpyPreAlloc(cbRequest);
    else if (_fMeter)
        ul = MtrPreAlloc(cbRequest);
    else
        ul = DbgPreAlloc(cbRequest);

    LeaveSpyAlloc();

    return ul;
}

STDMETHODIMP_(void *)
CMallocSpy::PostAlloc(void *pvActual)
{
    void * pv;

    EnterSpyAlloc();

    if (_fStrict)
        pv = SpyPostAlloc(pvActual, _dwFlags);
    else if (_fMeter)
        pv = MtrPostAlloc(pvActual);
    else
        pv = DbgPostAlloc(pvActual);

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(void *)
CMallocSpy::PreFree(void *pvRequest, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (!fSpyed)
        pv = pvRequest;
    else if (_fStrict)
        pv = SpyPreFree(pvRequest);
    else if (_fMeter)
        pv = MtrPreFree(pvRequest);
    else
        pv = DbgPreFree(pvRequest);

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(void)
CMallocSpy::PostFree(BOOL fSpyed)
{
    EnterSpyAlloc();

    if (fSpyed)
    {
        if (_fStrict)
            SpyPostFree();
        else if (_fMeter)
            MtrPostFree();
        else
            DbgPostFree();
    }

    LeaveSpyAlloc();
}

STDMETHODIMP_(SIZE_T)
CMallocSpy::PreRealloc(
    void *pvRequest,
    SIZE_T cbRequest,
    void **ppvActual,
    BOOL fSpyed)
{
    SIZE_T cb;

    EnterSpyAlloc();

    if (!fSpyed)
    {
        *ppvActual = pvRequest;
        cb = cbRequest;
    }
    else if (_fStrict)
        cb = SpyPreRealloc(pvRequest, cbRequest, ppvActual);
    else if (_fMeter)
        cb = MtrPreRealloc(pvRequest, cbRequest, ppvActual);
    else
        cb = DbgPreRealloc(pvRequest, cbRequest, ppvActual);

    LeaveSpyAlloc();

    return cb;
}

STDMETHODIMP_(void *)
CMallocSpy::PostRealloc(void *pvActual, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (!fSpyed)
        pv = pvActual;
    else if (_fStrict)
        pv = SpyPostRealloc(pvActual, _dwFlags);
    else if (_fMeter)
        pv = MtrPostRealloc(pvActual);
    else
        pv = DbgPostRealloc(pvActual);

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(void *)
CMallocSpy::PreGetSize(void *pvRequest, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (!fSpyed)
        pv = pvRequest;
    else if (_fStrict)
        pv = SpyPreGetSize(pvRequest);
    else if (_fMeter)
        pv = MtrPreGetSize(pvRequest);
    else
        pv = DbgPreGetSize(pvRequest);

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(SIZE_T)
CMallocSpy::PostGetSize(SIZE_T cbActual, BOOL fSpyed)
{
    SIZE_T cb;

    EnterSpyAlloc();

    if (!fSpyed)
        cb = cbActual;
    else if (_fStrict)
        cb = SpyPostGetSize(cbActual);
    else if (_fMeter)
        cb = MtrPostGetSize(cbActual);
    else
        cb = DbgPostGetSize(cbActual);

    LeaveSpyAlloc();

    return cb;
}

STDMETHODIMP_(void *)
CMallocSpy::PreDidAlloc(void *pvRequest, BOOL fSpyed)
{
    void * pv;

    EnterSpyAlloc();

    if (!fSpyed)
        pv = pvRequest;
    else if (_fStrict)
        pv = SpyPreDidAlloc(pvRequest);
    else if (_fMeter)
        pv = MtrPreDidAlloc(pvRequest);
    else
        pv = DbgPreDidAlloc(pvRequest);

    LeaveSpyAlloc();

    return pv;
}

STDMETHODIMP_(BOOL)
CMallocSpy::PostDidAlloc(void *pvRequest, BOOL fSpyed, BOOL fActual)
{
    BOOL f;

    EnterSpyAlloc();

    if (!fSpyed)
        f = fActual;
    else if (_fStrict)
        f = SpyPostDidAlloc(pvRequest, fActual);
    else if (_fMeter)
        f = MtrPostDidAlloc(pvRequest, fActual);
    else
        f = DbgPostDidAlloc(pvRequest, fActual);

    LeaveSpyAlloc();

    return f;
}

STDMETHODIMP_(void)
CMallocSpy::PreHeapMinimize()
{
}

STDMETHODIMP_(void)
CMallocSpy::PostHeapMinimize()
{
}

static CMallocSpy sSpy;


void
DbgRegisterMallocSpy()
{
#if !defined(_MAC) && !defined(WIN16)
    EnsureThreadState();
    if (SUCCEEDED(CoRegisterMallocSpy(&sSpy)))
    {
#ifdef PERFMETER
        if (g_mtSpy == 0)
            g_mtSpy = MtRegister("mtCoTaskMem", "mtWorkingSet", "CoTaskMemAlloc");
#endif
        sSpy._fRegistered = TRUE;
#if defined(RETAILBUILD) && defined(PERFTAGS) && !defined(PERFMETER)
        static int g_tgCoMemoryStrict = PerfRegister("tgCoMemoryStrict", "!Memory", "Use VMem for CoTaskMemAlloc");
        sSpy._fStrict = IsPerfEnabled(g_tgCoMemoryStrict);
        sSpy._dwFlags = (GetPrivateProfileIntA("perftags", "tgMemoryStrictHead", FALSE, "\\msxmldbg.ini") ? 0 : VMEM_BACKSIDESTRICT) |
                        (GetPrivateProfileIntA("perftags", "tgMemoryStrictAlign", FALSE, "\\msxmldbg.ini") ? VMEM_BACKSIDEALIGN8 : 0);
#else
        sSpy._fStrict = IsTagEnabled(tagCoMemoryStrict);
        sSpy._dwFlags = (IsTagEnabled(tagMemoryStrictTail) ? VMEM_BACKSIDESTRICT : 0) |
                        (IsTagEnabled(tagMemoryStrictAlign) ? VMEM_BACKSIDEALIGN8 : 0);
#endif
#if defined(RETAILBUILD) && defined(PERFMETER)
        sSpy._fMeter = !sSpy._fStrict;
#else
        sSpy._fMeter = FALSE;
#endif
    }
#endif
}

void
DbgRevokeMallocSpy(void)
{
#if !defined(_MAC) && !defined(WIN16)
    if (sSpy._fRegistered)
    {
        sSpy._fRegistered = FALSE;
        CoRevokeMallocSpy();
    }
#endif
}
