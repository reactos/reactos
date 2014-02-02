#pragma once

class CMenuDeskBar:
    public CComCoClass<CMenuDeskBar, &CLSID_MenuDeskBar>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IOleCommandTarget,
    public IServiceProvider,
    public IInputObjectSite,
    public IInputObject,
    public IMenuPopup,
    public IObjectWithSite,
    public IBanneredBar,
    public IInitializeObject
{
public:

    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp,BOOL fSet);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid,PVOID *ppvSite);

    // *** IBanneredBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetIconSize(THIS_ DWORD iIcon);
    virtual HRESULT STDMETHODCALLTYPE GetIconSize(THIS_ DWORD* piIcon);
    virtual HRESULT STDMETHODCALLTYPE SetBitmap(THIS_ HBITMAP hBitmap);
    virtual HRESULT STDMETHODCALLTYPE GetBitmap(THIS_ HBITMAP* phBitmap);

    // *** IInitializeObject methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(THIS);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);
    
    // *** IInputObjectSite methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(THIS_ LPUNKNOWN lpUnknown, BOOL bFocus);
    
    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(THIS_ BOOL bActivating, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO(THIS);
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(THIS_ LPMSG lpMsg);

DECLARE_REGISTRY_RESOURCEID(IDR_MENUDESKBAR)
DECLARE_NOT_AGGREGATABLE(CMenuDeskBar)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMenuDeskBar)
    COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
    COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
    COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
    COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
    COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IMenuPopup)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    COM_INTERFACE_ENTRY_IID(IID_IBanneredBar, IBanneredBar)
    COM_INTERFACE_ENTRY_IID(IID_IInitializeObject, IInitializeObject)
END_COM_MAP()

};
