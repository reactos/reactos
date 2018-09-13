//+---------------------------------------------------------------------
//
//  File:       stdfact.cxx
//
//  Contents:   Standard IClassFactory implementation
//
//  Classes:    StdClassFactory
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::StdClassFactory, public
//
//  Synopsis:   Constructor for StdUnknown class
//
//----------------------------------------------------------------

StdClassFactory::StdClassFactory(void)
{
    _ulRefs = 0;
    _ulLocks = 0;
}

//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::AddRef, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
StdClassFactory::AddRef(void)
{
    return ++_ulRefs;
}


//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::Release, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
StdClassFactory::Release(void)
{
    Assert(_ulRefs > 0);
    --_ulRefs;

#if DBG
    if(_ulRefs == 0)
        DOUT(L"StdClassFactory::Release _ulRefs == 0\r\n");
#endif

    return _ulRefs;
}

//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdClassFactory::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
#ifdef VERBOSE_DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"StdClassFactory::QueryInterface (%lx)\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif //VERBOSE_DBG

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        AddRef();
        *ppv = (LPCLASSFACTORY)this;
        return NOERROR;
    }
    *ppv = NULL;
#ifdef VERBOSE_DBG
    DOUT(L"StdClassFactory::QueryInterface returning E_NOINTERFACE\r\n");
#endif //VERBOSE_DBG
    return E_NOINTERFACE;
}


//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::LockServer, public
//
//  Synopsis:   Method of IClassFactory interface
//
//----------------------------------------------------------------

STDMETHODIMP
StdClassFactory::LockServer (BOOL fLock)
{
    if (fLock)
    {
        _ulLocks++;
    }
    else
    {
        Assert(_ulLocks > 0);
        _ulLocks--;
    }
    return NOERROR;
}

#ifdef DOCGEN
//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::CreateInstance, public
//
//  Synopsis:   Manufactures an instance of the class
//
//  Notes:      This pure virtual function must be overridden by the inheriting
//              class because the base class does not know what class to
//              instantiate.
//
//----------------------------------------------------------------

STDMETHODIMP
StdClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
        REFIID iid,
        LPVOID FAR* ppv) {};

//REVIEW: how to enforce ref counting of Class factory in object
// constructor/destructor?  Can we do this in a conjunction of StdUnknown
// with StdClassFactory.
#endif  // DOCGEN

//+---------------------------------------------------------------
//
//  Member:     StdClassFactory::CanUnload, public
//
//  Synopsis:   Returns TRUE iff there are no extant instances of
//              this class, outstanding references on the class factory,
//              or locks on the class factory.
//
//  Notes:      This function is for use in the standard DllCanUnloadNow
//              function.
//
//----------------------------------------------------------------

BOOL
StdClassFactory::CanUnload(void)
{
    return _ulRefs == 0 && _ulLocks == 0;
}


