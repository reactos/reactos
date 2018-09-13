/*
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop


__unknown::__unknown(const IID * riid) : 
        _riid(riid) 
{
#ifdef RENTAL_MODEL
    TLSDATA * ptlsdata = GetTlsData();
    _lRefs = (ULONG)(REF_OFFSET | (ptlsdata->_reModel == Rental ? REF_RENTAL : 0));
#endif

}

#ifdef RENTAL_MODEL
__unknown::__unknown(const IID * riid, RentalEnum re) : 
        _riid(riid), 
        _lRefs((ULONG)(REF_OFFSET | (re == Rental ? REF_RENTAL : 0))) 
{
}

RentalEnum 
__unknown::model()
{
    return (_lRefs & REF_RENTAL) ? Rental : MultiThread;
}
#endif


HRESULT 
__unknown::QueryInterface(IUnknown * punk, REFIID riid, void ** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (riid == IID_IUnknown || riid == *_riid)
    {
        punk->AddRef();
        *ppvObject = punk;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}


ULONG
__unknown::AddRef()
{
    ULONG ul;

    if (_lRefs & REF_RENTAL)
        ul = _lRefs = _lRefs + (ULONG)REF_OFFSET;
    else
        ul = (ULONG)(InterlockedExchangeAdd((LONG*)&_lRefs, REF_OFFSET) + REF_OFFSET);

    return ul >> (ULONG)REF_SHIFT;
}


ULONG 
__unknown::Decrement()
{
    ULONG ul;

    if (_lRefs & REF_RENTAL)
        ul = _lRefs = _lRefs - (ULONG)REF_OFFSET;
    else
        ul = (ULONG)(InterlockedExchangeAdd((LONG*)&_lRefs, -REF_OFFSET) - REF_OFFSET);

    return ul >> (ULONG)REF_SHIFT;
}


ULONG 
__unknown::Release()
{
    ULONG ul = Decrement();

    if (ul == 0)
    {
        STACK_ENTRY_IUNKNOWN(this);
        TRY
        {
            delete this;
        }
        CATCH
        {
        }
        ENDTRY
    }
    return ul;
}


#ifdef RENTAL_MODEL
__comexport::__comexport(const IID * riid, Object * p) : __unknown(riid, p->getBase()->model()), _pWrapped(p)
#else
__comexport::__comexport(const IID * riid, Object * p) : __unknown(riid), _pWrapped(p)
#endif
{
    ::IncrementComponents();
}


__comexport::~__comexport()
{
    ::DecrementComponents();
}


HRESULT
__comexport::QueryInterface(IUnknown * punk, REFIID riid, void ** ppvObject)
{
    HRESULT hr = __unknown::QueryInterface(punk, riid, ppvObject);
    if (hr)
    {
        hr = _pWrapped->QueryInterface(riid, ppvObject);
    }
    return hr;
}


ULONG 
__comexport::Release()
{
    ULONG ul = Decrement();

    if (ul == 0)
    {
        STACK_ENTRY_IUNKNOWN(this);
        TRY
        {
            delete this;
        }
        CATCH
        {
        }
        ENDTRY
    }
    return ul;
}


Object * 
__comexport::getExported(IUnknown * p, REFIID refIID)
{
    Object * o;
    checkhr(p->QueryInterface(IID_Object, (void **)&o));
    Object * w;
    checkhr(o->QueryInterface(refIID, (void **)&w));
    return w;
}
