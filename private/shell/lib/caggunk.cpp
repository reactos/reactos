#include "proj.h"
#include "caggunk.h"



ULONG CAggregatedUnknown::AddRef()
{
    return _punkAgg->AddRef();
}

ULONG CAggregatedUnknown::Release()
{
    return _punkAgg->Release();
}


HRESULT CAggregatedUnknown::QueryInterface(REFIID riid, void **ppvObj)
{
    return _punkAgg->QueryInterface(riid, ppvObj);
}


HRESULT CAggregatedUnknown::CUnkInner::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
        InterlockedIncrement(&_cRef);
        return S_OK;
    }

    CAggregatedUnknown* pparent = IToClass(CAggregatedUnknown, _unkInner, this);
    return pparent->v_InternalQueryInterface(riid, ppvObj);
}

ULONG CAggregatedUnknown::CUnkInner::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}


ULONG CAggregatedUnknown::CUnkInner::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    CAggregatedUnknown* pparent = IToClass(CAggregatedUnknown, _unkInner, this);

    if (!pparent->v_HandleDelete(&_cRef))
    {
        _cRef = 1000; // protect against cached pointers bumping us up then down

        delete pparent;
    }

    return 0;
}

CAggregatedUnknown::CAggregatedUnknown(IUnknown* punkAgg)
{
    _punkAgg = punkAgg ? punkAgg : &_unkInner;
}

CAggregatedUnknown::~CAggregatedUnknown()
{
}

//
//  Convert our controlling unknown to its canonical IUnknown *without*
//  altering the reference count on the outer object.
//
//  This is critical in order for QueryOuterInterface to work properly.
//
//  Returns NULL if something horrible went wrong.
//
//  OLE Magic:  Since objects are required also to return the canonical
//  IUnknown in response to any QI(IUnknown), it follows that the canonical
//  IUnknown remains valid so long as there are any outstanding references
//  to the object.  In other words, you can Release() the canonical IUnknown
//  and the pointer remains valid so long as you keep the object alive by
//  other means.
//
//  Believe it or not, this is a feature.  It's in the book!
//
IUnknown *CAggregatedUnknown::_GetCanonicalOuter(void)
{
    IUnknown *punkAggCanon;
    HRESULT hres = _punkAgg->QueryInterface(IID_IUnknown, (void **)&punkAggCanon);
    if (SUCCEEDED(hres)) 
    {
        punkAggCanon->Release(); // see "OLE Magic" comment above
        return punkAggCanon;
    } 
    else 
    {
        // The outer object is most likely some other shell component,
        // so let's ASSERT so whoever owns the outer component will fix it.
        ASSERT(!"The outer object's implementation of QI(IUnknown) is broken.");
        return NULL;
    }
}

void CAggregatedUnknown::_ReleaseOuterInterface(IUnknown** ppunk)
{
    ASSERT(IS_VALID_CODE_PTR(_punkAgg, IUnknown));

    IUnknown *punkAggCanon = _GetCanonicalOuter(); // non-refcounted pointer

    //
    //  SHReleaseOuterInterface can handle punkAggCanon == NULL
    //
    SHReleaseOuterInterface(punkAggCanon, ppunk);
}

HRESULT CAggregatedUnknown::_QueryOuterInterface(REFIID riid, void ** ppvOut)
{
    IUnknown *punkAggCanon = _GetCanonicalOuter(); // non-refcounted pointer
    //
    //  SHQueryOuterInterface can handle punkAggCanon == NULL.
    //
    return SHQueryOuterInterface(punkAggCanon, riid, ppvOut);
}
