/**************************************************************\
    FILE: bandprxy.cpp

    DESCRIPTION:
        The CBandProxy class will allow bands to navigate a
    generic browser window.  This will work correctly if the
    band is tied to the Browser Window because it's a ToolBar.
    Or if it's a toolband, each time a navigation happens,
    the top most browser window needs to be found or a new window 
    created.
\**************************************************************/

#include "priv.h"
#include "sccls.h"
#include "dbgmem.h"
#include "itbar.h"
#include "itbdrop.h"
#include "util.h"
//#include "winlist.h"
#include "bandprxy.h"

#define DM_PERSIST      DM_TRACE        // trace IPS::Load, ::Save, etc.


//================================================================= 
// Implementation of CBandProxy
//=================================================================

/****************************************************\
    FUNCTION: CBandProxy_CreateInstance
  
    DESCRIPTION:
        This function will create an instance of the
    CBandProxy COM object.
\****************************************************/
HRESULT CBandProxy_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CBandProxy * p = new CBandProxy();
    if (p) 
    {
        *ppunk = SAFECAST(p, IBandProxy *);
        return NOERROR;
    }

    return E_OUTOFMEMORY;
}


/****************************************************\
    FUNCTION: Address Band Constructor
\****************************************************/
CBandProxy::CBandProxy() : _cRef(1)
{
    DllAddRef();
    TraceMsg(TF_SHDLIFE, "ctor CBandProxy %x", this);

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!_pwb);
    ASSERT(!_punkSite);
}


/****************************************************\
    FUNCTION: Address Band destructor
\****************************************************/
CBandProxy::~CBandProxy()
{
    ATOMICRELEASE(_pwb);
    ATOMICRELEASE(_punkSite);

    TraceMsg(TF_SHDLIFE, "dtor CBandProxy %x", this);
    DllRelease();
}



//===========================
// *** IUnknown Interface ***
/****************************************************\
    FUNCTION: AddRef
\****************************************************/
ULONG CBandProxy::AddRef()
{
    _cRef++;
    return _cRef;
}

/****************************************************\
    FUNCTION: Release
\****************************************************/
ULONG CBandProxy::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/****************************************************\
    FUNCTION: QueryInterface
\****************************************************/
HRESULT CBandProxy::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IBandProxy))
    {
        *ppvObj = SAFECAST(this, IBandProxy*);
    } 
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


//================================
//  ** IBandProxy Interface ***

/****************************************************\
    FUNCTION: SetSite
  
    DESCRIPTION:
        This function will be called to have this
    Toolband try to obtain enough information about it's
    parent Toolbar to create the Band window and maybe
    connect to a Browser Window.  
\****************************************************/
HRESULT CBandProxy::SetSite(IUnknown * punk)
{
    HRESULT hr = S_OK;

    // On UNIX, we always have a browser.
    // Note, that there's no memory leak happened, 
    // because we get the browser only once 
    // and release it once too (in destructor).
#ifndef DISABLE_ACTIVEDESKTOP_FOR_UNIX
    _fHaveBrowser = FALSE;
    ATOMICRELEASE(_pwb);
#endif

    IUnknown_Set(&_punkSite, punk);
    return hr;
}


/****************************************************\
    FUNCTION: CreateNewWindow
  
    DESCRIPTION:
        If this function succeeds, the caller must
    use and release the returned interface quickly.  The
    caller cannot hold on to the Interface because
    the user may close the window and make releasing
    it impossible.
\****************************************************/
HRESULT CBandProxy::CreateNewWindow(IUnknown** ppunk)
{
#ifndef UNIX
    return CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                                 IID_IUnknown, (void **)ppunk);
#else
    return CoCreateInternetExplorer( IID_IUnknown, CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                                     (void **) ppunk );
#endif
    // ZekeL: Add code to prep new Browser here.
}


IWebBrowser2* CBandProxy::_GetBrowser()
{
    if (!_fHaveBrowser)
    {
        IUnknown * punkHack;

        _fHaveBrowser = TRUE;

        // HACK: Bands docked on the side of the screen besides the Taskbar will be
        //       able to get a IWebBrowser2 interface pointer.  But we expect this
        //       to be pointing to a valid browser that we are attached to.  Navigating
        //       this interface appears to create new windows, which is not what we
        //       want, because we will try to recycle windows and do special behavior
        //       if the shift key is down.  This QS will detect this case and prevent
        //       it from confusing us.
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SShellDesktop, IID_IUnknown, (void**)&punkHack)))
            punkHack->Release();
        else
            IUnknown_QueryService(_punkSite, SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID*)&_pwb);
    }

    return _pwb;
}


// this does the default UI work of opening a new window if the shift
// key is down
// or creating a browser if one isn't available already
IWebBrowser2* CBandProxy::_GetBrowserWindow()
{
    IUnknown* punk = NULL;
    IWebBrowser2* pwb = NULL;

    GetBrowserWindow(&punk);

    if (punk) {
        punk->QueryInterface(IID_IWebBrowser2, (LPVOID*)&pwb);

        // Always make browser visible.
        MakeBrowserVisible(punk);
        punk->Release();
    }
    
    return pwb;
}

/****************************************************\
    FUNCTION: GetBrowserWindow
  
    DESCRIPTION:
        this is to just *GET* the browser.  It does not
    do any auto-creating work.
        If this function succseeds, the caller must
    use and release the returned interface quickly.  The
    caller cannot hold on to the Interface because
    the user may close the window and make releasing
    it impossible.

\****************************************************/
HRESULT CBandProxy::GetBrowserWindow(IUnknown** ppunk)
{
    HRESULT hr;

    *ppunk = _GetBrowser();
    if (*ppunk)
    {
        (*ppunk)->AddRef();
        hr =  S_OK;
    } 
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


/****************************************************\
    FUNCTION: IsConnected
  
    DESCRIPTION:
        Indicate if we have a direct connection to the
    browser window.
    S_FALSE == no
    S_OK == yes.
\****************************************************/
HRESULT CBandProxy::IsConnected()
{
    return _GetBrowser() ? S_OK : S_FALSE;
}


/****************************************************\
    FUNCTION: MakeBrowserVisible
  
    DESCRIPTION:
        Make browser visible.
\****************************************************/
HRESULT CBandProxy::MakeBrowserVisible(IUnknown* punk)
{
    IWebBrowserApp * pdie;

    if (SUCCEEDED(punk->QueryInterface(IID_IWebBrowserApp, (void**)&pdie)))
    {
        pdie->put_Visible(TRUE);
        
        HWND hwnd;
        if (SUCCEEDED(SHGetTopBrowserWindow(punk, &hwnd)))
        {
            if (IsIconic(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
        }
        
        pdie->Release();
    }

    return S_OK;
}


/****************************************************\
    FUNCTION: NavigateToPIDL
  
    DESCRIPTION:
        The caller needs to free the PIDL parameter and
    it can be done at any time.  (No need to worry
    about async navigation)  
\****************************************************/
HRESULT CBandProxy::NavigateToPIDL(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;

    IWebBrowser2* pwb = _GetBrowserWindow();
    if (pwb)
    {
        VARIANT varThePidl;

        if (InitVariantFromIDList(&varThePidl, pidl))
        {
            hr = pwb->Navigate2(&varThePidl, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);

            VariantClear(&varThePidl);
        }
        else
            hr = E_OUTOFMEMORY;

        pwb->Release();
    }
    else
    {
        LPCITEMIDLIST pidlTemp;
        IShellFolder* psf;
        
        if (SUCCEEDED(IEBindToParentFolder(pidl, &psf, &pidlTemp)))
        {
            IContextMenu* pcm;

            hr = psf->GetUIObjectOf(NULL, 1, &pidlTemp, IID_IContextMenu, 0, (LPVOID*)&pcm);
            if (SUCCEEDED(hr))
            {
                hr = IContextMenu_Invoke(pcm, NULL, NULL, 0);
                pcm->Release();
            }
            psf->Release();
        }
    }

    return hr;
}




/****************************************************\
    FUNCTION: NavigateToUrlOLE
  
    DESCRIPTION:
        Navigate to the Specified URL.  
\****************************************************/
HRESULT CBandProxy::_NavigateToUrlOLE(BSTR bstrURL, VARIANT * pvFlags)
{
    HRESULT hr = S_OK;

    ASSERT(bstrURL); // must have valid URL to browse to

    IWebBrowser2* pwb = _GetBrowserWindow();
    // This will assert if someone was hanging around in the debugger
    // too long.  While will cause the call to timing out.
    if (pwb) 
    {
        VARIANT varURL;
        varURL.vt = VT_BSTR;
        varURL.bstrVal = bstrURL;

        hr = pwb->Navigate2(&varURL, pvFlags, PVAREMPTY, PVAREMPTY, PVAREMPTY);
        // VariantClear() not called because caller will free the allocated string.
        pwb->Release();
    } else {
        SHELLEXECUTEINFO sei;
        USES_CONVERSION;

        FillExecInfo(sei, NULL, //hwnd, 
                     NULL, W2T(bstrURL), NULL, NULL, SW_SHOWNORMAL);
        if (ShellExecuteEx(&sei))
            hr = S_OK;
        else
            hr = E_FAIL;

    }


    return hr;
}


/****************************************************\
    FUNCTION: NavigateToURLW
  
    DESCRIPTION:
        Navigate to the Specified URL.
\****************************************************/
HRESULT CBandProxy::NavigateToURL(LPCWSTR lpwzURL, VARIANT * Flags)
{
    HRESULT hr;

    SA_BSTR sstrPath;
    StrCpyNW(sstrPath.wsz, lpwzURL, ARRAYSIZE(sstrPath.wsz));
    sstrPath.cb = lstrlenW(sstrPath.wsz) * SIZEOF(WCHAR);
    hr = _NavigateToUrlOLE(sstrPath.wsz, Flags);

    return hr;
}
