/*----------------------------------------------------------------------------
/ Title;
/   factory.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Implements IClassFactory for My Documents classes.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ CMyDocsClassFactory
/----------------------------------------------------------------------------*/

CMyDocsClassFactory::CMyDocsClassFactory()
{
}

#undef CLASS_NAME
#define CLASS_NAME CMyDocsClassFactory
#include "unknown.inc"

STDMETHODIMP
CMyDocsClassFactory::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IClassFactory, (LPCLASSFACTORY)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));

}


/*-----------------------------------------------------------------------------
/ IClassFactory methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsClassFactory::CreateInstance( IUnknown* pOuter,
                                     REFIID riid,
                                     LPVOID* ppvObject
                                    )
{
    HRESULT hr;

    MDTraceEnter(TRACE_FACTORY, "CMyDocsClassFactory::CreateInstance");
    MDTraceGUID("Interface requested", riid);

    MDTraceAssert(ppvObject);

    // No support for aggregation, if we have an outer class then bail

    if ( pOuter )
        ExitGracefully(hr, CLASS_E_NOAGGREGATION, "Aggregation is not supported");

    if (IsEqualIID( riid, IID_IShellCopyHook ))
    {
        CMyDocsCopyHook * pMDCH = new CMyDocsCopyHook();

        if ( !pMDCH )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate CMyDocsCopyHook");

        hr = pMDCH->QueryInterface(riid, ppvObject);

        if ( FAILED(hr) )
            delete pMDCH;

    }
    else
    {
        // Our IShellFolder implementation is in CMyDocsFolder along with several
        // other interfaces, therefore lets just create that object and allow
        // the QI process to continue there.

        CMyDocsFolder* pMDF = new CMyDocsFolder( );

        if ( !pMDF )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate CMyDocsFolder");

        hr = pMDF->QueryInterface(riid, ppvObject);

        if ( FAILED(hr) )
            delete pMDF;
    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

STDMETHODIMP
CMyDocsClassFactory::LockServer(BOOL fLock)
{
    return S_OK;                // not supported
}




