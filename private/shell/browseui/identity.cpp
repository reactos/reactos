// identity.cpp
//
//  A dummy class whose sole purpose is to say, "Yes, I am BrowseUI"

#include "priv.h"
#include "sccls.h"

class CBrowseuiIdentity : public IUnknown
{
    public:
        STDMETHOD ( QueryInterface ) ( REFIID riid, LPVOID * ppvObj );
        STDMETHOD_( ULONG, AddRef ) ();
        STDMETHOD_( ULONG, Release ) ();

    protected:
        friend HRESULT CBrowseuiIdentity_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

        CBrowseuiIdentity();
        ~CBrowseuiIdentity();

        LONG            m_cRef;
};

STDAPI CBrowseuiIdentity_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // class factory should've done these
    ASSERT(pUnkOuter == NULL);
    ASSERT(*ppunk == NULL);

    CBrowseuiIdentity* pid = new CBrowseuiIdentity();

    if (pid)
    {
        *ppunk = SAFECAST(pid, IUnknown*);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CBrowseuiIdentity::CBrowseuiIdentity() : m_cRef(1)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CBrowseuiIdentity::~CBrowseuiIdentity()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBrowseuiIdentity::QueryInterface ( REFIID riid, LPVOID * ppvObj )
{
    if ( riid == IID_IUnknown )
    {
        *ppvObj = SAFECAST( this, IUnknown *);
        AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBrowseuiIdentity:: AddRef ()
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBrowseuiIdentity:: Release ()
{
    if ( InterlockedDecrement( &m_cRef ) == 0 )
    {
        delete this;
        return 0;
    }

    return m_cRef;
}
