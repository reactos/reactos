#pragma once

#define WM_USER_SHELLEVENT WM_USER+88
#define WM_USER_FOLDEREVENT WM_USER+88

class CExplorerBand :
    public CComCoClass<CExplorerBand, &CLSID_ExplorerBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IBandNavigate,
    public IWinEventHandler,
    public INamespaceProxy,
    public IDispatch,
    public IDropSource,
    public IDropTarget,
    public CWindowImpl<CExplorerBand, CWindow, CControlWinTraits>
{

private:
    class NodeInfo
    {
    public:
        LPITEMIDLIST absolutePidl;
        LPITEMIDLIST relativePidl;
        BOOL expanded;
    };

    // *** BaseBarSite information ***
    CComPtr<IUnknown> pSite;

    // *** tree explorer band stuff ***
    BOOL fVisible;
    BOOL bFocused;
    DWORD dwBandID;

    void InitializeExplorerBand();
    void DestroyExplorerBand();

public:
    CExplorerBand();
    virtual ~CExplorerBand();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);

    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IBandNavigate methods ***
    virtual HRESULT STDMETHODCALLTYPE Select(long paramC);

    // *** INamespaceProxy ***
    virtual HRESULT STDMETHODCALLTYPE GetNavigateTarget(long paramC, long param10, long param14);
    virtual HRESULT STDMETHODCALLTYPE Invoke(long paramC);
    virtual HRESULT STDMETHODCALLTYPE OnSelectionChanged(long paramC);
    virtual HRESULT STDMETHODCALLTYPE RefreshFlags(long paramC, long param10, long param14);
    virtual HRESULT STDMETHODCALLTYPE CacheItem(long paramC);

    // *** IDispatch methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
    
    // *** IDropTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IDropSource methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
    virtual HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

    DECLARE_REGISTRY_RESOURCEID(IDR_EXPLORERBAND)
    DECLARE_NOT_AGGREGATABLE(CExplorerBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CExplorerBand)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IBandNavigate, IBandNavigate)
        COM_INTERFACE_ENTRY_IID(IID_INamespaceProxy, INamespaceProxy)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    END_COM_MAP()

    BEGIN_MSG_MAP(CExplorerBand)
    END_MSG_MAP()
};

extern "C"
HRESULT WINAPI CExplorerBand_Constructor(REFIID riid, LPVOID *ppv);