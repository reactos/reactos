#ifndef __SHVOCX_H__
#define __SHVOCX_H__

#include "shocx.h"
#include "basesb.h"
#include "sfview.h"
#include "util.h" // for BSTR functions
#include "cobjsafe.h"
#include "ipstg.h"
#include "fldset.h"

class CWebBrowserOC;

#define CS_BACK 0x0001
#define CS_FORE 0x0002

#define AUTOSIZE_OFF            0x00000000L
#define AUTOSIZE_ON             0x00000001L

#define AUTOSIZE_PERCENTWIDTH   0x00000002L
#define AUTOSIZE_FULLSIZE       0x00000004L

#define VB_CLASSNAME_LENGTH 20            

//
// NOTES:
//
//  A CWebBrowserSB object is ALWAYS paired with CWebBrowserOC, and bahaves
// as a "ShellExplorer" OC together. CWebBrowserOC exports OLE control
// interfaces and an OLE automation interface (IWebBrowser) to the
// container. CWebBrowserSB exports IShellBrowser interface and a few other
// interfaces to the containee (IShellView object and DocObject).
//
//  It's important to know that those objects have pointers (not interface
// pointers but explicit object pointers) to each other. In order to avoid
// a circular reference, we don't AddRef to the pointer to CWebBrowserOC
// (_psvo).
//
class CWebBrowserSB : public CBASEBROWSER
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef() { return CBASEBROWSER::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release() { return CBASEBROWSER::Release(); };

    // IOleInPlaceUIWindow (also IOleWindow)
    virtual STDMETHODIMP EnableModelessSB(BOOL fEnable);
    virtual STDMETHODIMP TranslateAcceleratorSB(LPMSG lpmsg, WORD wID);
    virtual STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    virtual STDMETHODIMP OnViewWindowActive(struct IShellView * ppshv);

    // IBrowserService
    virtual STDMETHODIMP GetParentSite(struct IOleInPlaceSite** ppipsite);
    virtual STDMETHODIMP GetOleObject(struct IOleObject** ppobjv);
    virtual STDMETHODIMP SetNavigateState(BNSTATE bnstate);
    virtual STDMETHODIMP_(LRESULT) WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP_(LRESULT) OnNotify(NMHDR * pnm);
    virtual STDMETHODIMP ReleaseShellView();
    virtual STDMETHODIMP ActivatePendingView();
    virtual STDMETHODIMP SetTopBrowser();
    virtual STDMETHODIMP GetFolderSetData(struct tagFolderSetData* pfsd) { /* we modify base directly */ return S_OK; };
    virtual STDMETHODIMP _SwitchActivationNow( );
    
    // IServiceProvider
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    HRESULT QueryServiceItsOwn(REFGUID guidService, REFIID riid, LPVOID* ppvObj)
        { return CBASEBROWSER::QueryService(guidService, riid, ppvObj); }

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    CWebBrowserSB(IUnknown* pauto, CWebBrowserOC* psvo);

    void ReleaseShellExplorer(void)
        { _psvo = NULL; } // NOTE: Note that we haven't AddRef'ed it.
    

    IShellView* GetShellView() 
            { return _bbd._psv;};

    // Load/Save to be called by CWebBrowserOC's IPS::Save
    HRESULT Load(IStream *pStm);
    HRESULT Save(IStream *pStm /*, BOOL fClearDirty */);


protected:

    ~CWebBrowserSB();

    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void    _ViewChange(DWORD dwAspect, LONG lindex);

    //ViewStateStream related
    virtual STDMETHODIMP_(IStream*) v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName);
    
    // se DIRECTION_ flags below
    HRESULT _EnableModeless(BOOL fEnable, BOOL fDirection);
    HRESULT _TranslateAccelerator(LPMSG lpmsg, WORD wID, BOOL fDirection);
    HRESULT _IncludeByPidl(struct IShellView *psv, LPCITEMIDLIST pidl);
    HRESULT _IncludeByName(struct IShellView *psv, LPCTSTR pszInclude, LPCTSTR pszExclude);
    HRESULT _QueryServiceParent(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    BOOL    _IsDesktopOC(void);

    virtual BOOL    _HeyMoe_IsWiseGuy(void);


    friend CWebBrowserOC;
    CWebBrowserOC* _psvo;

    long _cbScriptNesting;
};


#define DIRECTION_FORWARD_TO_CHILD  FALSE
#define DIRECTION_FORWARD_TO_PARENT TRUE

class CWebBrowserOC : public CShellOcx
                    , public IWebBrowser2      // wrapped _pauto
                    , public CImpIExpDispSupport   // wrapped _pauto
                    , public IExpDispSupportOC
                    , public IPersistString
                    , public IOleCommandTarget
                    , public CObjectSafety
                    , public ITargetEmbedding
                    , public CImpIPersistStorage
                    , public IPersistHistory
{
public:
    // IUnknown (we multiply inherit from IUnknown, disambiguate here)
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj) { return CShellOcx::QueryInterface(riid, ppvObj); }
    STDMETHOD_(ULONG, AddRef)() { return CShellOcx::AddRef(); }
    STDMETHOD_(ULONG, Release)() { return CShellOcx::Release(); }

    // IDispatch (we multiply inherit from IDispatch, disambiguate here)
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) { return CShellOcx::GetTypeInfoCount(pctinfo); }
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CShellOcx::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
        { return CShellOcx::GetIDsOfNames(riid,rgszNames,cNames,lcid,rgdispid); }
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

    // IPersistXXX disambiguate here
    virtual STDMETHODIMP IsDirty(void) {return CShellOcx::IsDirty();}

    // IOleCommandTarget
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHOD(Exec)(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IWebBrowser
    STDMETHOD(GoBack)();
    STDMETHOD(GoForward)();
    STDMETHOD(GoHome)();
    STDMETHOD(GoSearch)();
    STDMETHOD(Navigate)(BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers);
    STDMETHOD(Refresh)();
    STDMETHOD(Refresh2)(VARIANT *Level);
    STDMETHOD(Stop)();
    STDMETHOD(get_Application)(IDispatch **ppDisp);
    STDMETHOD(get_Parent)(IDispatch **ppDisp);
    STDMETHOD(get_Container)(IDispatch **ppDisp);
    STDMETHOD(get_Document)(IDispatch **ppDisp);
    STDMETHOD(get_TopLevelContainer)(VARIANT_BOOL *pBool);
    STDMETHOD(get_Type)(BSTR * pbstrType);
    STDMETHOD(get_Left)(long * pl);
    STDMETHOD(put_Left)(long Left);
    STDMETHOD(get_Top)(long * pl);
    STDMETHOD(put_Top)(long Top);
    STDMETHOD(get_Width)(long * pl);
    STDMETHOD(put_Width)(long Width);
    STDMETHOD(get_Height)(long * pl);
    STDMETHOD(put_Height)(long Height);
    STDMETHOD(get_LocationName)(BSTR * pbstrLocationName);
    STDMETHOD(get_LocationURL)(BSTR * pbstrLocationURL);
    STDMETHOD(get_Busy)(VARIANT_BOOL * pBool);

    /* IWebBrowserApp methods */
    STDMETHOD(Quit)(THIS);
    STDMETHOD(ClientToWindow)(THIS_ int FAR* pcx, int FAR* pcy);
    STDMETHOD(PutProperty)(THIS_ BSTR szProperty, VARIANT vtValue);
    STDMETHOD(GetProperty)(THIS_ BSTR szProperty, VARIANT FAR* pvtValue);
    STDMETHOD(get_Name)(THIS_ BSTR FAR* pbstrName);
    STDMETHOD(get_HWND)(THIS_ long FAR* pHWND);
    STDMETHOD(get_FullName)(THIS_ BSTR FAR* pbstrFullName);
    STDMETHOD(get_Path)(THIS_ BSTR FAR* pbstrPath);
    STDMETHOD(get_FullScreen)(THIS_ VARIANT_BOOL FAR* pBool);
    STDMETHOD(put_FullScreen)(THIS_ VARIANT_BOOL Value);
    STDMETHOD(get_Visible)(THIS_ VARIANT_BOOL FAR* pBool);
    STDMETHOD(put_Visible)(THIS_ VARIANT_BOOL Value);
    STDMETHOD(get_StatusBar)(THIS_ VARIANT_BOOL FAR* pBool);
    STDMETHOD(put_StatusBar)(THIS_ VARIANT_BOOL Value);
    STDMETHOD(get_StatusText)(THIS_ BSTR FAR* pbstr);
    STDMETHOD(put_StatusText)(THIS_ BSTR bstr);
    STDMETHOD(get_ToolBar)(THIS_ int FAR* pBool);
    STDMETHOD(put_ToolBar)(THIS_ int Value);
    STDMETHOD(get_MenuBar)(THIS_ VARIANT_BOOL FAR* pValue);
    STDMETHOD(put_MenuBar)(THIS_ VARIANT_BOOL Value);

    // IWebBRowser2 methods
    STDMETHOD(Navigate2)(THIS_ VARIANT FAR* URL, VARIANT FAR* Flags, VARIANT FAR* TargetFrameName, VARIANT FAR* PostData, VARIANT FAR* Headers);
    STDMETHOD(ShowBrowserBar)(THIS_ VARIANT FAR* pvaClsid, VARIANT FAR* pvaShow, VARIANT FAR* pvaSize);
    STDMETHOD(QueryStatusWB)(THIS_ OLECMDID cmdID, OLECMDF FAR* pcmdf);
    STDMETHOD(ExecWB)(THIS_ OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT FAR* pvaIn, VARIANT FAR* pvaOut);
    STDMETHOD(get_ReadyState)(THIS_ READYSTATE FAR* plReadyState);
    STDMETHOD(get_Offline)(THIS_ VARIANT_BOOL FAR* pbOffline);
    STDMETHOD(put_Offline)(THIS_ VARIANT_BOOL bOffline);
    STDMETHOD(get_Silent)(THIS_ VARIANT_BOOL FAR* pbSilent);
    STDMETHOD(put_Silent)(THIS_ VARIANT_BOOL bSilent);
    STDMETHOD(get_RegisterAsBrowser)(THIS_ VARIANT_BOOL FAR* pbRegister);
    STDMETHOD(put_RegisterAsBrowser)(THIS_ VARIANT_BOOL bRegister);
    STDMETHOD(get_RegisterAsDropTarget)(THIS_ VARIANT_BOOL FAR* pbRegister);
    STDMETHOD(put_RegisterAsDropTarget)(THIS_ VARIANT_BOOL bRegister);
    STDMETHOD(get_TheaterMode)(THIS_ VARIANT_BOOL FAR* pValue);
    STDMETHOD(put_TheaterMode)(THIS_ VARIANT_BOOL Value);
    STDMETHOD(get_AddressBar)(THIS_ VARIANT_BOOL FAR* Value);
    STDMETHOD(put_AddressBar)(THIS_ VARIANT_BOOL Value);
    STDMETHOD(get_Resizable)(THIS_ VARIANT_BOOL FAR* Value) { return E_NOTIMPL; }
    STDMETHOD(put_Resizable)(THIS_ VARIANT_BOOL Value) { return E_NOTIMPL; }

    // *** CImpIExpDispSupport override ***
    virtual STDMETHODIMP OnTranslateAccelerator(MSG *pMsg,DWORD grfModifiers);
    virtual STDMETHODIMP OnInvoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                        VARIANT FAR* pVarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr);

    // *** IExpDispSupportOC ***
    virtual STDMETHODIMP OnOnControlInfoChanged();
    virtual STDMETHODIMP GetDoVerbMSG(MSG *pMsg);


    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID) { return CShellOcx::GetClassID(pClassID); }

    // IPersistString
    STDMETHOD(Initialize)(LPCWSTR pwszInit);

    // ITargetEmbedding
    STDMETHOD(GetTargetFrame)(ITargetFrame **ppTargetFrame);

    // IPersistStreamInit
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(InitNew)(void);

    // IPersistPropertyBag
    STDMETHOD(Load)(IPropertyBag *pBag, IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag *pBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // IOleObject
    virtual STDMETHODIMP Close(DWORD dwSaveOption);
    virtual STDMETHODIMP DoVerb(
        LONG iVerb,
        LPMSG lpmsg,
        IOleClientSite *pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect);
    virtual STDMETHODIMP SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);

    // IViewObject2
    virtual STDMETHODIMP Draw(
        DWORD dwDrawAspect,
        LONG lindex,
        void *pvAspect,
        DVTARGETDEVICE *ptd,
        HDC hdcTargetDev,
        HDC hdcDraw,
        LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds,
        BOOL ( __stdcall *pfnContinue )(ULONG_PTR dwContinue),
        ULONG_PTR dwContinue);

    virtual STDMETHODIMP GetColorSet(DWORD, LONG, void *, DVTARGETDEVICE *,
        HDC, LOGPALETTE **);

    virtual HRESULT STDMETHODCALLTYPE SetExtent( DWORD dwDrawAspect,
            SIZEL *psizel);
            
    // IOleControl
    virtual STDMETHODIMP GetControlInfo(LPCONTROLINFO pCI);
    virtual STDMETHODIMP OnMnemonic(LPMSG pMsg);
    virtual STDMETHODIMP OnAmbientPropertyChange(DISPID dispid);
    virtual STDMETHODIMP FreezeEvents(BOOL bFreeze);

    // IOleInPlaceActiveObject
    virtual HRESULT __stdcall OnFrameWindowActivate(BOOL fActivate);
    virtual STDMETHODIMP TranslateAccelerator(LPMSG lpmsg);
    virtual STDMETHODIMP EnableModeless(BOOL fEnable);

    // *** CShellOcx's CImpIConnectionPointContainer override ***
    virtual STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum);

    // *** IPersistHistory
    virtual STDMETHODIMP LoadHistory(IStream *pStream, IBindCtx *pbc);
    virtual STDMETHODIMP SaveHistory(IStream *pStream);
    virtual STDMETHODIMP SetPositionCookie(DWORD dwPositionCookie);
    virtual STDMETHODIMP GetPositionCookie(DWORD *pdwPositioncookie);

    // random public functions
    friend HRESULT CWebBrowserOC_SavePersistData(IStream *pstm, SIZE* psizeObj,
        FOLDERSETTINGS* pfs, IShellLinkA* plink, SHELLVIEWID* pvid,
        BOOL fOffline = FALSE, BOOL fSilent = FALSE,
        BOOL fRegisterAsBrowser = FALSE, BOOL fRegisterAsDropTarget = TRUE,
        BOOL fEmulateOldStream = FALSE, DWORD * pdwExtra = NULL);


protected:
    CWebBrowserOC(IUnknown* punkOuter, LPCOBJECTINFO poi);
    ~CWebBrowserOC();
    BOOL _InitializeOC(IUnknown* punkOuter);
    IUnknown* _GetInner() { return CShellOcx::_GetInner(); }
    friend HRESULT CWebBrowserOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

    // Override private virtual function
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT v_InternalQueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual void _OnSetClientSite(void);
    virtual HRESULT _OnActivateChange(IOleClientSite* pActiveSite, UINT uState);
    virtual void _OnInPlaceActivate(void);
    virtual void _OnInPlaceDeactivate(void);
    virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID iid);

    // Private non-virtual
    LRESULT _OnPaintPrint(HDC hdcPrint);
    LRESULT _OnCreate(LPCREATESTRUCT lpcs);
    HRESULT _BrowseObject(LPCITEMIDLIST pidlBrowseTo);
    void    _InitDefault(void);
    void    _OnSetShellView(IShellView*);
    void    _OnReleaseShellView(void);
    BOOL    _GetViewInfo(SHELLVIEWID* pvid);
    void    _RegisterWindow();
    void    _UnregisterWindow();
    HRESULT _SetDownloadState(HRESULT hresRet, DWORD nCmdexecopt, VARIANTARG *pvarargIn);
    void    _OnLoaded(BOOL fUpdateBrowserReadyState);
	HMODULE _GetBrowseUI();
    BOOL    _HeyMoe_IsWiseGuy(void) {return _fHostedInImagineer;}
	
    static LRESULT CALLBACK s_DVWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    IUnknown*       _pauto; // we aggregate this and pass it to the shellbrowser
    IWebBrowser2*   _pautoWB2;
    IExpDispSupport*_pautoEDS;

    BOOL            _fInit:1;               // TRUE iff we are initialized
    BOOL            _fEmulateOldStream:1;   // TRUE iff we emulate ie30 stream format
    BOOL            _fNavigateOnSetClientSite:1; // set when Loaded before SetClientSite
    BOOL            _fShouldRegisterAsBrowser:1; // TRUE iff OC should be registered in windows list
    BOOL            _fDidRegisterAsBrowser:1;    // TRUE iff registered in windows list
    BOOL            _fTopLevel:1;           // TRUE iff we're the topmost CBaseBrowser around
    BOOL            _fVisible:1;            // BETA1: bindable props of the OC that we can't
    BOOL            _fNoMenuBar:1;          //        call IEDisp's version of the props.
    BOOL            _fNoToolBar:1;          //        for beta2 we should let us call
    BOOL            _fNoStatusBar:1;        //        iedisp's version to get the events
    BOOL            _fFullScreen:1;         //        to fire and persistence correct.
    BOOL            _fTheaterMode:1;
    BOOL            _fNoAddressBar:1;
    BOOL            _fHostedInVB5:1;        // Our immediate container is the VB5 forms engine.
    BOOL            _fHostedInImagineer:1;  // 

    MSG             *_pmsgDoVerb;        // valid only when _fDoVerbMSGValid
    
    long            _cbCookie;              // our cookie for registering in windows list
    SIZE            _szIdeal;       // ideal size of view, based on _size.cx
    SIZE            _szNotify;      // last size we notified conainer

    FOLDERSETTINGS  _fs;            // FolderViewMode and FolderFlags

    // cached draw aspect incase we are not READSTATE_INTERACTIVE when we get SetExtent
    DWORD           _dwDrawAspect;
    
    friend CWebBrowserSB;
    CWebBrowserSB*      _psb;
    ITargetFramePriv*       _pTargetFramePriv;  // QueryService(IID_ITARGETFRAME2)

    IShellLinkA*        _plinkA;     // used in save/load code only

    IOleCommandTarget*  _pctContainer;  // container

    HGLOBAL             _hmemSB;        // Initializing stream

    CConnectionPoint    m_cpWB1Events;  // CShellOcx holds the WB2 event source
    LPMESSAGEFILTER     _lpMF;          // Pointer to message filter for cross-thread containers (e.g., AOL)

    HMODULE				_hBrowseUI;		// Handle for use in design mode brand drawing

    DWORD               _cPendingFreezeEvents;
};

#define IS_INITIALIZED if(!_fInit){TraceMsg(TF_WARNING,"shvocx: BOGUS CONTAINER calling when we haven't been initialized"); _InitDefault();}

#endif // __SHVOCX_H__

