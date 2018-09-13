/*****************************************************************************\
    FILE: ftpwebvw.h

    DESCRIPTION:
        This file exists so WebView can automate the Ftp Shell Extension and get
    information like the MessageOfTheDay.
\*****************************************************************************/

#include "priv.h"
#include "ftpwebvw.h"
#include "msieftp.h"
#include <shlguid.h>

//===========================
// *** IDispatch Interface ***
//===========================

// BUGBUG: Cane we nuke this?

STDMETHODIMP CFtpWebView::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CFtpWebView::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CFtpWebView::GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); 
}

STDMETHODIMP CFtpWebView::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}



//===========================
// *** IFtpWebView Interface ***
//===========================

/*****************************************************************************\
    FUNCTION: _GetIFtpWebView

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::_GetIFtpWebView(IFtpWebView ** ppfwb)
{
    IShellFolderViewCB * psfvcb = NULL;
    HRESULT hr = S_FALSE;

    ASSERT(_punkSite);
    if (EVAL(ppfwb))
        *ppfwb = NULL;

    IUnknown_QueryService(_punkSite, SID_ShellFolderViewCB, IID_IShellFolderViewCB, (LPVOID *) &psfvcb);
    // IE4's shell32 doesn't support QS(SID_ShellFolderViewCB, IID_IShellFolderViewCB), so we need to
    // QS(SID_ShellFolderViewCB, IShellFolderView) and then use IShellFolderView::SetCallback()
    if (!psfvcb)
    {
        IDefViewFrame * pdvf = NULL;

        IUnknown_QueryService(_punkSite, SID_DefView, IID_IDefViewFrame, (LPVOID *) &pdvf);
        if (EVAL(pdvf))
        {
            IShellFolderView * psfv = NULL;

            pdvf->QueryInterface(IID_IShellFolderView, (LPVOID *) &psfv);
            if (EVAL(psfv))
            {
                if (EVAL(SUCCEEDED(psfv->SetCallback(NULL, &psfvcb))))
                {
                    IShellFolderViewCB * psfvcbTemp = NULL;

                    if (SUCCEEDED(psfv->SetCallback(psfvcb, &psfvcbTemp)) && psfvcbTemp)
                    {
                        // We should get NULL back but if not, release the ref instead of leaking.
                        psfvcbTemp->Release();
                    }
                }

                psfv->Release();
            }

            pdvf->Release();
        }
    }
    
    if (EVAL(psfvcb))
    {
        if (EVAL(SUCCEEDED(psfvcb->QueryInterface(IID_IFtpWebView, (LPVOID *) ppfwb))))
            hr = S_OK;

        psfvcb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_MessageOfTheDay

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_MessageOfTheDay(BSTR * pbstr)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_MessageOfTheDay(pbstr);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_UserName

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_UserName(BSTR * pbstr)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_UserName(pbstr);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_Server

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_Server(BSTR * pbstr)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_Server(pbstr);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_Directory

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_Directory(BSTR * pbstr)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_Directory(pbstr);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_PasswordLength

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_PasswordLength(long * plLength)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_PasswordLength(plLength);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_EmailAddress

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_EmailAddress(BSTR * pbstr)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_EmailAddress(pbstr);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::put_EmailAddress

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::put_EmailAddress(BSTR bstr)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->put_EmailAddress(bstr);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::get_CurrentLoginAnonymous

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::get_CurrentLoginAnonymous(VARIANT_BOOL * pfAnonymousLogin)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->get_CurrentLoginAnonymous(pfAnonymousLogin);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::LoginAnonymously

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::LoginAnonymously(void)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->LoginAnonymously();
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::LoginWithPassword

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::LoginWithPassword(BSTR bUserName, BSTR bPassword)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->LoginWithPassword(bUserName, bPassword);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::LoginWithoutPassword

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::LoginWithoutPassword(BSTR bUserName)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->LoginWithoutPassword(bUserName);
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IFtpWebView::InvokeHelp

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView::InvokeHelp(void)
{
    IFtpWebView * pfwb;
    HRESULT hr = _GetIFtpWebView(&pfwb);

    if (EVAL(S_OK == hr))
    {
        ASSERT(pfwb);
        hr = pfwb->InvokeHelp();
        pfwb->Release();
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: CFtpWebView_Create

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpWebView_Create(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFtpWebView * pfwv = new CFtpWebView();

    if (EVAL(pfwv))
    {
        hr = pfwv->QueryInterface(riid, ppv);
        pfwv->Release();
    }

    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CFtpWebView::CFtpWebView() : m_cRef(1), CImpIDispatch(&IID_IFtpWebView)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.

    LEAK_ADDREF(LEAK_CFtpWebView);
}


/****************************************************\
    Destructor
\****************************************************/
/*****************************************************************************
 *
 *      FtpView_OnRelease (from shell32.IShellView)
 *
 *      When the view is released, clean up various stuff.
 *
 *      BUGBUG -- (Note that there is a race here, because this->hwndOwner
 *      doesn't get zero'd out on the OnWindowDestroy because the shell
 *      doesn't give us a pdvsci...)
 *
 *      We release the psf before triggering the timeout, which is a
 *      signal to the trigger not to do anything.
 *
 *      _UNDOCUMENTED_: This callback and its parameters are not documented.
 *
 *****************************************************************************/
CFtpWebView::~CFtpWebView()
{
    DllRelease();
    LEAK_DELREF(LEAK_CFtpWebView);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpWebView::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpWebView::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpWebView::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFtpWebView, IObjectWithSite),
        QITABENT(CFtpWebView, IDispatch),
        QITABENT(CFtpWebView, IObjectWithSite),
        QITABENT(CFtpWebView, IObjectSafety),
        QITABENT(CFtpWebView, IFtpWebView),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}
