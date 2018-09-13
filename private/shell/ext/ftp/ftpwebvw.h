/*****************************************************************************\
    FILE: ftpwebvw.h

    DESCRIPTION:
        This file exists so WebView can automate the Ftp Shell Extension and get
    information like the MessageOfTheDay.
\*****************************************************************************/

#ifndef _FTPWEBVIEW_H_
#define _FTPWEBVIEW_H_

#include "dspsprt.h"
#include "cowsite.h"
#include "cobjsafe.h"
#include "msieftp.h"

HRESULT CFtpWebView_Create(REFIID riid, void **ppvObj);

/*****************************************************************************\
    CLASS: CFtpWebView

    DESCRIPTION:
        This file exists so WebView can automate the Ftp Shell Extension and get
    information like the MessageOfTheDay.
\*****************************************************************************/

class CFtpWebView 
                :  public CImpIDispatch
                ,  public CObjectWithSite
                ,  public CObjectSafety
                ,  public IFtpWebView
{    

public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // *** IFtpWebView ***
    virtual STDMETHODIMP get_Server(BSTR * pbstr);
    virtual STDMETHODIMP get_Directory(BSTR * pbstr);
    virtual STDMETHODIMP get_UserName(BSTR * pbstr);
    virtual STDMETHODIMP get_PasswordLength(long * plLength);
    virtual STDMETHODIMP get_EmailAddress(BSTR * pbstr);
    virtual STDMETHODIMP put_EmailAddress(BSTR bstr);
    virtual STDMETHODIMP get_CurrentLoginAnonymous(VARIANT_BOOL * pfAnonymousLogin);
    virtual STDMETHODIMP get_MessageOfTheDay(BSTR * pbstr);
    virtual STDMETHODIMP LoginAnonymously(void);
    virtual STDMETHODIMP LoginWithPassword(BSTR bUserName, BSTR bPassword);
    virtual STDMETHODIMP LoginWithoutPassword(BSTR bUserName);
    virtual STDMETHODIMP InvokeHelp(void);

public:
    // Friend Functions
    friend HRESULT CFtpWebView_Create(REFIID riid, void **ppvObj);

private:
    // Private Member Variables
    int                     m_cRef;

    // Private Member Functions
    CFtpWebView();
    ~CFtpWebView();

    HRESULT _GetIFtpWebView(IFtpWebView ** ppfwb);
};


#endif // _FTPWEBVIEW_H_