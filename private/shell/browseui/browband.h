#ifndef _BROWBAND_H_
#define _BROWBAND_H_


class CBrowserBand :
    public CToolBand
    ,public IContextMenu
    ,public IWinEventHandler
    ,public IDispatch
    ,public IBrowserBand
    ,public IPersistPropertyBag
{

public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CToolBand::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) { return CToolBand::Release(); };

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    
    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi) ;

    // *** IPersistPropertyBag methods ***
    virtual STDMETHODIMP InitNew()
        { return E_NOTIMPL; };
    virtual STDMETHODIMP Load(IPropertyBag* pPBag, IErrorLog *pErrLog);
    virtual STDMETHODIMP Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
        { return E_NOTIMPL; };

    // *** IPersistStream methods ***
    // (others use base class implementation) 
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // *** IContextMenu methods ***
    STDMETHOD(QueryContextMenu)(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags);

    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR    idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) { return E_NOTIMPL; };

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo){return(E_NOTIMPL);}
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo){return(E_NOTIMPL);}
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid){return(E_NOTIMPL);}
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                  DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr);

    // *** IBrowserBand methods ***
    STDMETHOD(GetObjectBB)(THIS_ REFIID riid, LPVOID *ppv);
    STDMETHOD(SetBrowserBandInfo)(THIS_ DWORD dwMask, PBROWSERBANDINFO pbbi);
    STDMETHOD(GetBrowserBandInfo)(THIS_ DWORD dwMask, PBROWSERBANDINFO pbbi);

protected:
    CBrowserBand();
    virtual ~CBrowserBand();

    friend HRESULT CBrowserBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);       // for ctor
    friend IDeskBand* CBrowserBand_Create(LPCITEMIDLIST pidl);

    virtual void _Connect(BOOL fConnect);
    HRESULT _CreateOCHost();
    virtual void _InitBrowser(void);
    virtual HRESULT _NavigateOC();
    LRESULT _OnNotify(LPNMHDR pnm);
    SIZE _GetCurrentSize();

#ifdef DEBUG
    void _DebugTestCode();
#endif
    void _MakeSizesConsistent(LPSIZE psizeCur);

    IWebBrowser2 * _pauto;
    IOleInPlaceActiveObject * _poipao;
    LPITEMIDLIST _pidl;
    DWORD _dwcpCookie;
    WCHAR _wszTitle[40];
    BOOL    _fBlockSIDProxy:1; // SID_SProxyBrowser
    BOOL    _fBlockDrop:1;     // should we block drop on contents

    BOOL    _fCustomTitle:1;
    DWORD   _dwModeFlags;
    SIZE    _sizeMin;
    SIZE    _sizeMax;
};


#endif // _BROWBAND_H_
