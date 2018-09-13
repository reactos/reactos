/**************************************************************\
    FILE: bandprxy.h

    DESCRIPTION:
        The CBandProxy class will allow bands to navigate a
    generic browser window.  This will work correctly if the
    band is tied to the Browser Window because it's a ToolBar.
    Or if it's a toolband, each time a navigation happens,
    the top most browser window needs to be found or a new window 
    created.
\**************************************************************/

#ifndef _BANDPRXY_H
#define _BANDPRXY_H

#include "bands.h"


///////////////////////////////////////////////////////////////////
// #DEFINEs
#define    SEC_DEFAULT             0x0000
#define    SEC_WAIT                0x0002
#define    SEC_SHELLSERVICEOBJECTS 0x0004
#define    SEC_NOUI                0x0008


/**************************************************************\
    CLASS: CBandProxy

    DESCRIPTION:
        The CBandProxy class will allow bands to navigate a
    generic browser window.  This will work correctly if the
    band is tied to the Browser Window because it's a ToolBar.
    Or if it's a toolband, each time a navigation happens,
    the top most browser window needs to be found or a new window 
    created.
\**************************************************************/
class CBandProxy
                : public IBandProxy // (Includes IUnknown)
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IBandProxy methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);        
    virtual STDMETHODIMP CreateNewWindow(IUnknown** ppunk);        
    virtual STDMETHODIMP GetBrowserWindow(IUnknown** ppunk);        
    virtual STDMETHODIMP IsConnected();
    virtual STDMETHODIMP NavigateToPIDL(LPCITEMIDLIST pidl);        
    virtual STDMETHODIMP NavigateToURL(LPCWSTR wzUrl, VARIANT * Flags);        

    // Constructor / Destructor
    CBandProxy();
    ~CBandProxy(void);

    // Friend Functions
    friend HRESULT CBandProxy_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);   

protected:
    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////
    int             _cRef;

    BITBOOL         _fHaveBrowser : 1;  // We haven't tried to get _pwb
    IWebBrowser2 *  _pwb;
    IUnknown *      _punkSite;

    HRESULT _NavigateToUrlOLE(BSTR bstrURL, VARIANT * Flags);
    HRESULT MakeBrowserVisible(IUnknown* punk);
    IWebBrowser2* _GetBrowserWindow();
    IWebBrowser2* _GetBrowser();
};


#endif /* _BANDPRXY_H */
