//--------------------------------------------------------------------------------
//
//  File:   propsext.cpp
//
//  General handling of OLE Entry points, CClassFactory and CPropSheetExt
//
//  Common Code for all display property sheet extension
//
//  Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
//
//--------------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

//
// Count number of objects and number of locks.
//
HINSTANCE    g_hInst = NULL;
BOOL         g_RunningOnNT = FALSE;
LPDATAOBJECT g_lpdoTarget = NULL;

ULONG        g_cObj = 0;
ULONG        g_cLock = 0;



//---------------------------------------------------------------------------
// DllMain()
//---------------------------------------------------------------------------
int APIENTRY DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID )
{
    if( dwReason == DLL_PROCESS_ATTACH )        // Initializing
    {
        if ((int)GetVersion() >= 0)
        {
            g_RunningOnNT = TRUE;
        }

        g_hInst = hInstance;

        DisableThreadLibraryCalls(hInstance);
    }

    return 1;
}
//---------------------------------------------------------------------------
//      DllGetClassObject()
//
//      If someone calls with our CLSID, create an IClassFactory and pass it to
//      them, so they can create and use one of our CPropSheetExt objects.
//
//---------------------------------------------------------------------------
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID* ppvOut )
{
    *ppvOut = NULL; // Assume Failure
    if( IsEqualCLSID( rclsid, g_CLSID_CplExt ) )
    {
        //
        //Check that we can provide the interface
        //
        if( IsEqualIID( riid, IID_IUnknown) ||
            IsEqualIID( riid, IID_IClassFactory )
           )
        {
            //Return our IClassFactory for CPropSheetExt objects
            *ppvOut = (LPVOID* )new CClassFactory();
            if( NULL != *ppvOut )
            {
                //AddRef the object through any interface we return
                ((CClassFactory*)*ppvOut)->AddRef();
                return NOERROR;
            }
            return E_OUTOFMEMORY;
        }
        return E_NOINTERFACE;
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}

//---------------------------------------------------------------------------
//      DllCanUnloadNow()
//
//      If we are not locked, and no objects are active, then we can exit.
//
//---------------------------------------------------------------------------
STDAPI DllCanUnloadNow()
{
    SCODE   sc;

    //
    //Our answer is whether there are any object or locks
    //
    sc = (0L == g_cObj && 0 == g_cLock) ? S_OK : S_FALSE;

    return ResultFromScode(sc);
}

//---------------------------------------------------------------------------
//      ObjectDestroyed()
//
//      Function for the CPropSheetExt object to call when it is destroyed.
//      Because we're in a DLL, we only track the number of objects here,
//      letting DllCanUnloadNow take care of the rest.
//---------------------------------------------------------------------------
void FAR PASCAL ObjectDestroyed( void )
{
    g_cObj--;
    return;
}

UINT CALLBACK
PropertySheeCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        if (g_lpdoTarget) {
            g_lpdoTarget->Release();
            g_lpdoTarget = NULL;
        }
        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}



//***************************************************************************
//
//  CClassFactory Class
//
//***************************************************************************



//---------------------------------------------------------------------------
//      Constructor
//---------------------------------------------------------------------------
CClassFactory::CClassFactory()
{
    m_cRef = 0L;
    return;
}

//---------------------------------------------------------------------------
//      Destructor
//---------------------------------------------------------------------------
CClassFactory::~CClassFactory( void )
{
    return;
}

//---------------------------------------------------------------------------
//      QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    //Any interface on this object is the object pointer.
    if( IsEqualIID( riid, IID_IUnknown ) ||
        IsEqualIID( riid, IID_IClassFactory )
       )
    {
        *ppv = (LPVOID)this;
        ++m_cRef;
        return NOERROR;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//      AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return ++m_cRef;
}

//---------------------------------------------------------------------------
//      Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    ULONG cRefT;

    cRefT = --m_cRef;

    if( 0L == m_cRef ) 
        delete this;

    return cRefT;
}

//---------------------------------------------------------------------------
//      CreateInstance()
//---------------------------------------------------------------------------
STDMETHODIMP
CClassFactory::CreateInstance( LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR *ppvObj
                              )
{
    CPropSheetExt*  pObj;
    HRESULT         hr = E_OUTOFMEMORY;

    *ppvObj = NULL;

    // We don't support aggregation at all.
    if( pUnkOuter )
    {
        return CLASS_E_NOAGGREGATION;
    }

    //Verify that a controlling unknown asks for IShellPropSheetExt
    if( IsEqualIID( riid, IID_IShellPropSheetExt ) )
    {
        //Create the object, passing function to notify on destruction
        pObj = new CPropSheetExt( pUnkOuter, ObjectDestroyed );

        if( NULL == pObj )
        {
            return hr;
        }

        hr = pObj->QueryInterface( riid, ppvObj );

        //Kill the object if initial creation or FInit failed.
        if( FAILED(hr) )
        {
            delete pObj;
        }
        else
        {
            g_cObj++;
        }
        return hr;
    }

    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//      LockServer()
//---------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer( BOOL fLock )
{
    if( fLock )
    {
        g_cLock++;
    }
    else
    {
        g_cLock--;
    }
    return NOERROR;
}



//***************************************************************************
//
//  CPropSheetExt Class
//
//***************************************************************************



//---------------------------------------------------------------------------
//  Constructor
//---------------------------------------------------------------------------
CPropSheetExt::CPropSheetExt( LPUNKNOWN pUnkOuter, LPFNDESTROYED pfnDestroy )
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pfnDestroy = pfnDestroy;
    return;
}

//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------
CPropSheetExt::~CPropSheetExt( void )
{
    return;
}

//---------------------------------------------------------------------------
//  QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppv = (IShellExtInit *) this;
    }

    if (IsEqualIID(riid, IID_IShellPropSheetExt))
    {
        *ppv = (LPVOID)this;
    }

    if (*ppv)
    {
        ++m_cRef;
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//---------------------------------------------------------------------------
//  AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::AddRef( void )
{
    return ++m_cRef;
}

//---------------------------------------------------------------------------
//  Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::Release( void )
{
ULONG cRefT;

    cRefT = --m_cRef;

    if( m_cRef == 0 )
    {
        // Tell the housing that an object is going away so that it
        // can shut down if appropriate.
        if( NULL != m_pfnDestroy )
        {
            (*m_pfnDestroy)();
        }
        delete this;
    }
    return cRefT;
}

//---------------------------------------------------------------------------
//  AddPages()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;
    TCHAR szTitle[ 30 ];

    LoadString( g_hInst, IDS_PAGE_TITLE, szTitle, ARRAYSIZE(szTitle) );
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_USETITLE | PSP_USECALLBACK;
    psp.hIcon = NULL;
    psp.hInstance = g_hInst;
    psp.pszTemplate =MAKEINTRESOURCE( PROP_SHEET_DLG );
    psp.pfnDlgProc = (DLGPROC)PropertySheeDlgProc;
    psp.pfnCallback = PropertySheeCallback;
    psp.pszTitle = szTitle;
    psp.lParam = 0;

#ifdef USESLINKCONTROL
    LinkWindow_RegisterClass();
#endif
    
    if( ( hpage = CreatePropertySheetPage( &psp ) ) == NULL )
    {
        return ( E_OUTOFMEMORY );
    }

    if( !lpfnAddPage(hpage, lParam ) )
    {
        DestroyPropertySheetPage(hpage );
        return ( E_FAIL );
    }
    return NOERROR;
}

//---------------------------------------------------------------------------
//  ReplacePage()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    return NOERROR;
}


//---------------------------------------------------------------------------
//  IShellExtInit member function- this interface needs only one
//---------------------------------------------------------------------------

STDMETHODIMP CPropSheetExt::Initialize(LPCITEMIDLIST pcidlFolder,
                                       LPDATAOBJECT pdoTarget,
                                       HKEY hKeyID)
{
    //
    //  The target data object is an HDROP, or list of files from the shell.
    //

    if (g_lpdoTarget)
    {
        g_lpdoTarget->Release();
        g_lpdoTarget = NULL;
    }

    if (pdoTarget)
    {
        g_lpdoTarget = pdoTarget;
        g_lpdoTarget->AddRef();
    }

    return  NOERROR;
}
