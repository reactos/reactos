//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       hyplnk.hxx
//
//  Contents:   Definition of the Hyperlink class (common base for the <AREA>
//              and the <A> elements).
//
//  Classes:    CHyperlink
//
//----------------------------------------------------------------------------

#ifndef I_HYPLNK_HXX_
#define I_HYPLNK_HXX_
#pragma INCMSG("--- Beg 'hyplnk.hxx'")

MtExtern(CHyperlink)

class CHyperlink : public CElement
{
    DECLARE_CLASS_TYPES(CHyperlink, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHyperlink))

    CHyperlink(ELEMENT_TAG etag, CDoc *pDoc) :
        super(etag, pDoc)
    {}


    // URL accessors
    virtual HRESULT SetUrl(BSTR bstrUrl) = 0;
    virtual LPCTSTR GetUrl() const = 0;
    virtual LPCTSTR GetTarget() const = 0;
    virtual HRESULT GetUrlTitle(CStr *pstr) = 0;

    STDMETHODIMP get_href(BSTR * p);
    STDMETHODIMP put_href(BSTR v);
    STDMETHODIMP get_host(BSTR *pstr);
    STDMETHODIMP put_host(BSTR str);
    STDMETHODIMP get_hostname(BSTR *pstr);
    STDMETHODIMP put_hostname(BSTR str);
    STDMETHODIMP get_pathname(BSTR *pstr);
    STDMETHODIMP put_pathname(BSTR str);
    STDMETHODIMP get_port(BSTR *pstr);
    STDMETHODIMP put_port(BSTR str);
    STDMETHODIMP get_protocol(BSTR *pstr);
    STDMETHODIMP put_protocol(BSTR str);
    STDMETHODIMP get_search(BSTR *pstr);
    STDMETHODIMP put_search(BSTR str);
    STDMETHODIMP get_hash(BSTR *pstr);
    STDMETHODIMP put_hash(BSTR str);

    HRESULT ClickAction(CMessage * pMessage);

    // helper methods
    HRESULT         SetUrlComponent(const BSTR strComp, URLCOMP_ID ucid);
    HRESULT         GetUrlComponent(BSTR     * pstrComp, 
                                    URLCOMP_ID ucid, 
                                    TCHAR   ** ppchUrl = NULL,
                                    DWORD      dwFlags = 0);
    LPTSTR  GetHyperlinkCursor();
    HRESULT CopyLinkToClipboard(const TCHAR * pchDesc=NULL);
    HRESULT SetStatusText();

    HRESULT QueryStatusHelper (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    HRESULT ExecHelper (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    virtual HRESULT SetVisited() { return S_OK; }
    
private:
    NO_COPY(CHyperlink);


public:
    RECT        _rcWobbleZone;

    unsigned    _fCanClick:1;
    unsigned    _fOMSetHasOccurred:1;   // NS OM behaves differently 
    unsigned    _fAvailableOffline:1;
    unsigned    _fAvailableOfflineValid:1;
    unsigned    _fHasMouseOverCancelled:1;      // mouseOver can cancel status text
    unsigned    _fBaseUrlChanged:1;             // OnPropertyChange was caused by a base tag change

};

#pragma INCMSG("--- End 'hyplnk.hxx'")
#else
#pragma INCMSG("*** Dup 'hyplnk.hxx'")
#endif
