#include "priv.h"
#include "caggunk.h"



ULONG CAggregatedUnknown::AddRef()
{
    return _punkAgg->AddRef();
}

ULONG CAggregatedUnknown::Release()
{
    return _punkAgg->Release();
}


HRESULT CAggregatedUnknown::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    return _punkAgg->QueryInterface(riid, ppvObj);
}


HRESULT CAggregatedUnknown::CUnkInner::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
#ifdef DEBUG
    // use QISearch so we'll be 'registered' as a QI-like func
    static const QITAB qit[] = {
        QITABENT(CAggregatedUnknown::CUnkInner, IUnknown),  // IID_IUnknown
        { 0 },
    };
    if (SUCCEEDED(QISearch(this, qit, riid, ppvObj)))
        return S_OK;
#else
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
        _cRef++;
        return S_OK;
    }
#endif

    CAggregatedUnknown* pparent = IToClass(CAggregatedUnknown, _unkInner, this);
    return pparent->v_InternalQueryInterface(riid, ppvObj);
}

ULONG CAggregatedUnknown::CUnkInner::AddRef(void)
{
    _cRef++;
    return _cRef;
}


ULONG CAggregatedUnknown::CUnkInner::Release(void)
{
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    CAggregatedUnknown* pparent = IToClass(CAggregatedUnknown, _unkInner, this);

    _cRef = 1000; // protect against cached pointers bumping us up then down

    delete pparent;

    return 0;
}

CAggregatedUnknown::CAggregatedUnknown(IUnknown* punkAgg)
{
    _punkAgg = punkAgg ? punkAgg : &_unkInner;
}

CAggregatedUnknown::~CAggregatedUnknown()
{
}

void CAggregatedUnknown::_ReleaseOuterInterface(IUnknown** ppunk)
{
    ASSERT(IS_VALID_CODE_PTR(_punkAgg, IUnknown));

    _punkAgg->AddRef();

    // If the _QueryInterface failed below, then punk will be NULL when
    // the inner object releases.
    ATOMICRELEASE((*ppunk));
}

HRESULT CAggregatedUnknown::_QueryOuterInterface(REFIID riid, LPVOID* ppvOut)
{
    HRESULT hres = _punkAgg->QueryInterface(riid, ppvOut);
    _punkAgg->Release();

    ASSERT(FAILED(hres) || *ppvOut);  // this should always set here.  either by ourselves or our aggregator
    
    return hres;
}

