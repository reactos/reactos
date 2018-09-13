#ifndef _DHUIHAND_H
#define _DHUIHAND_H

class CDocHostUIHandler : 
               public IDocHostUIHandler
             , public IObjectWithSite
             , public IOleCommandTarget
             , public IDispatch
{
   public:

       // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);
    
    // IDocHostUIHandler
    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu( 
        DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
    virtual HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo);
    virtual HRESULT STDMETHODCALLTYPE ShowUI( 
        DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
        IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame,
        IOleInPlaceUIWindow *pDoc);
    virtual HRESULT STDMETHODCALLTYPE HideUI(void);
    virtual HRESULT STDMETHODCALLTYPE UpdateUI(void);
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorder( 
        LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator( 
        LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
    virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(BSTR *pbstrKey, DWORD dw);
    virtual HRESULT STDMETHODCALLTYPE GetDropTarget( 
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
    virtual HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDisp);
    virtual HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    virtual HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject *pDO, IDataObject **ppDORet);

    // IObjectWithSite
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);           

    //IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId) { return E_NOTIMPL; };
    virtual STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS  *pDispParams, VARIANT  *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr);

    CDocHostUIHandler::CDocHostUIHandler(void);
    CDocHostUIHandler::~CDocHostUIHandler(void);

protected:
    HRESULT GetAltExternal(IDispatch **ppDisp);    
    void ShowErrorDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags);   
    HRESULT ShowMessage(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags);
    void ShowFindDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags);
    HRESULT ShowPageSetupDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags);
    HRESULT ShowPrintDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags);
    HRESULT ShowPropertysheetDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags);
    HRESULT ShowPropertysheetDialogHelper(int cUnk, IUnknown ** apUnk, HWND hwndOwner, 
        IServiceProvider * pServiceProvider, IUnknown * punkBrowseDefault);
    HRESULT GetCommonPages(int cUnk, IUnknown **apUnk, CAUUID *pca);
    HRESULT ClsidToMoniker(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    IUnknown         * _punkSite;
    IOleInPlaceFrame * _pOleInPlaceFrame;
    HMENU              _hBrowseMenu;
    HMENU              _hEditMenu;    
    IDispatch        * _pExternal;

    IHTMLOptionsHolder * _pOptionsHolder;
    
    BOOL IsFindDialogUp(IWebBrowser2* pwb, IHTMLWindow2** ppWindow);

private:
    LONG     m_cRef;

};

BSTR GetFindText(IWebBrowser2* pwb);
void PutFindText(IWebBrowser2* pwb, LPCWSTR pwszFindText);

#define STR_FIND_DIALOG_NAME TEXT("__IE_FindDialog")
#define STR_FIND_DIALOG_TEXT TEXT("__IE_FindDialogText")

#endif //_DHUIHAND_H
