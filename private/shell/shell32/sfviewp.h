#define DV_ISANYICONMODE(fvm) ((fvm) == FVM_ICON || (fvm) == FVM_SMALLICON)

#ifndef _SFVIEWP_H_
#define _SFVIEWP_H_

#include "defview.h"
#include <mshtmhst.h>   // move it to priv.h eventually
#include "urlmon.h"
#include "dvtasks.h"
#include <perhist.h>
#include "cowsite.h"
#include "inetsmgr.h"

class CGenList
{
public:
    CGenList(UINT cbItem) : m_hList(NULL), m_cbItem(cbItem) {}
    ~CGenList() {Empty();}

    LPVOID GetPtr(UINT i)
        {return(i<GetItemCount() ? DSA_GetItemPtr(m_hList, i) : NULL);}

    UINT GetItemCount() {return(m_hList ? DSA_GetItemCount(m_hList) : 0);}

    int Add(LPVOID pv, int nInsert);

    void Empty() {if (m_hList) DSA_Destroy(m_hList); m_hList=NULL;}

protected:
    void Steal(CGenList* pList)
    {
        Empty();
        m_cbItem = pList->m_cbItem;
        m_hList = pList->m_hList;
        pList->m_hList = NULL;
    }

private:
    UINT m_cbItem;
    HDSA m_hList;
} ;

class CViewsList : public CGenList
{
public:
    CViewsList() : CGenList(SIZEOF(SFVVIEWSDATA*)), m_bGotDef(FALSE) {}
    ~CViewsList() {Empty();}

    SFVVIEWSDATA* GetPtr(UINT i)
    {
        SFVVIEWSDATA** ppViewsData = (SFVVIEWSDATA**)CGenList::GetPtr(i);
        return(ppViewsData ? *ppViewsData : NULL);
    }


    int Add(const SFVVIEWSDATA*pView, int nInsert, BOOL bCopy);
    int Add(const SFVVIEWSDATA*pView, BOOL bCopy=TRUE) {return Add(pView, DA_LAST, bCopy);}
    int Prepend(const SFVVIEWSDATA*pView, BOOL bCopy=TRUE) {return Add(pView, 0, bCopy);}
    void AddReg(HKEY hkParent, LPCTSTR pszSubKey);
    void AddCLSID(CLSID const* pclsid);
    void AddIni(LPCTSTR szIniFile, LPCTSTR szPath);

    void SetDef(SHELLVIEWID const *pvid) { m_bGotDef=TRUE; m_vidDef=*pvid; }
    BOOL GetDef(SHELLVIEWID *pvid) { if (m_bGotDef) *pvid=m_vidDef; return(m_bGotDef); }

    void Empty();

    static SFVVIEWSDATA* CopyData(const SFVVIEWSDATA* pData);

    int NextUnique(int nLast);
    int NthUnique(int nUnique);

private:
    BOOL m_bGotDef;
    SHELLVIEWID m_vidDef;
} ;

typedef struct
{
    LPARAM  lParamSort;
    int iDirection;
    int iLastColumnClick;
} DVSAVESTATE, *PDVSAVESTATE;

class DVSAVEHEADER
{
public:
    WORD          cbSize;
    WORD          wUnused; // junk on stack at this location has been saved in the registry since Win95... bummer
    DWORD         ViewMode;
    POINTS        ptScroll;
    WORD          cbColOffset;
    WORD          cbPosOffset;
    DVSAVESTATE   dvState;

    UINT GetColumnsInfo(LPBYTE* ppColInfo)
    {
        if (cbColOffset >= SIZEOF(DVSAVEHEADER))
        {
            *ppColInfo = ((LPBYTE)this) + cbColOffset;
            return cbPosOffset - cbColOffset;
        }
        else
        {
            *ppColInfo = NULL;
            return 0;
        }
    }
} ;
typedef DVSAVEHEADER* PDVSAVEHEADER;

// Even though we don't currently store anything we care
// about in this structure relating to the view state,
// the cbStreamSize value fixes a bug in Win95 where we
// read to the end of the stream instead of just reading
// in the same number of bytes we wrote out.
//
typedef struct
{
    DWORD       dwSignature;    // DVSAVEHEADEREX_SIGNATURE
    WORD        cbSize;         // size of this structure, in bytes
    WORD        wVersion;       // DVSAVEHEADEREX_VERSION
    DWORD       cbStreamSize;   // size of all info saved, in bytes
    DWORD       dwUnused;       // used to be SIZE szExtended (ie4 beta1)
    WORD        cbColOffset;    // overrides DVSAVEHEADER.cbColOffset
    WORD        wAlign;
} DVSAVEHEADEREX, *PDVSAVEHEADEREX;

#define DVSAVEHEADEREX_SIGNATURE 0xf0f0f0f0 // don't conflict with CCOLSHEADER_SIGNATURE
#define DVSAVEHEADEREX_VERSION 3 // for easy versioning

//=============================================================================
// CDVDropTarget : class definition
//=============================================================================
class CDVDropTarget // dvdt
{        
public:
    HRESULT DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    HRESULT DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    HRESULT DragLeave();
    HRESULT Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    void LeaveAndReleaseData(class CDefView *that);
    void ReleaseDataObject();
    void ReleaseCurrentDropTarget();

    IDataObject *       pdtobj;         // from DragEnter()/Drop()
    RECT                rcLockWindow;   // WindowRect of hwnd for DAD_ENTER
    int                 itemOver;       // item we are visually dragging over
    IDropTarget *       pdtgtCur;       // current drop target, derived from hit testing
    DWORD               dwEffectOut;    // last *pdwEffect out
    DWORD               grfKeyState;    // cached key state
    POINT               ptLast;         // last dragged position
    AUTO_SCROLL_DATA    asd;            // for auto scrolling
    DWORD               dwLastTime;     // for auto-opening folders
} ;

//
//  This is a proxy IDropTarget object, which wraps Trident's droptarget.
//
class CHostDropTarget : public IDropTarget
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IDropTarget ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    IDropTarget* _pdtDoc;   // Drop target of the trident
    IDropTarget* _pdtFrame; // Drop target of the frame
};


class CSFVSite : public IOleInPlaceSite,
                 public IOleClientSite,
                 public IOleDocumentSite,
                 public IServiceProvider,
                 public IOleCommandTarget,
                 public IDocHostUIHandler,
                 public IOleControlSite,
                 public IInternetSecurityManager,
                 public IDispatch       //For ambient properties.
{
    friend CHostDropTarget;
public:
    CSFVSite()  { ASSERT(m_peds == NULL); }
    ~CSFVSite() {
                    if (m_peds) {
                        m_peds->Release();
                        m_peds = NULL;
                    }
                }

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // IOleWindow
    virtual STDMETHODIMP GetWindow(
        /* [out] */ HWND *phwnd);

    virtual STDMETHODIMP ContextSensitiveHelp(
        /* [in] */ BOOL fEnterMode);

    // IOleInPlaceSite
    virtual STDMETHODIMP CanInPlaceActivate( void);

    virtual STDMETHODIMP OnInPlaceActivate( void);

    virtual STDMETHODIMP OnUIActivate( void);

    virtual STDMETHODIMP GetWindowContext(
        /* [out] */ IOleInPlaceFrame **ppFrame,
        /* [out] */ IOleInPlaceUIWindow **ppDoc,
        /* [out] */ LPRECT lprcPosRect,
        /* [out] */ LPRECT lprcClipRect,
        /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo);

    virtual STDMETHODIMP Scroll(
        /* [in] */ SIZE scrollExtant);

    virtual STDMETHODIMP OnUIDeactivate(
        /* [in] */ BOOL fUndoable);

    virtual STDMETHODIMP OnInPlaceDeactivate( void);

    virtual STDMETHODIMP DiscardUndoState( void);

    virtual STDMETHODIMP DeactivateAndUndo( void);

    virtual STDMETHODIMP OnPosRectChange(
        /* [in] */ LPCRECT lprcPosRect);

    // IOleClientSite
    virtual STDMETHODIMP SaveObject( void);

    virtual STDMETHODIMP GetMoniker(
        /* [in] */ DWORD dwAssign,
        /* [in] */ DWORD dwWhichMoniker,
        /* [out] */ IMoniker **ppmk);

    virtual STDMETHODIMP GetContainer(
        /* [out] */ IOleContainer **ppContainer);

    virtual STDMETHODIMP ShowObject( void);

    virtual STDMETHODIMP OnShowWindow(
        /* [in] */ BOOL fShow);

    virtual STDMETHODIMP RequestNewObjectLayout( void);

    // IOleDocumentSite
    virtual STDMETHODIMP ActivateMe(
        /* [in] */ IOleDocumentView *pviewToActivate);

    // IServiceProvider
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IOleControlSite
    virtual HRESULT STDMETHODCALLTYPE OnControlInfoChanged()
        { return E_NOTIMPL; };
    virtual HRESULT STDMETHODCALLTYPE LockInPlaceActive(BOOL fLock)
        { return E_NOTIMPL; };
    virtual HRESULT STDMETHODCALLTYPE GetExtendedControl(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
        { *ppDisp = NULL; return E_NOTIMPL; };
    virtual HRESULT STDMETHODCALLTYPE TransformCoords(POINTL __RPC_FAR *pPtlHimetric, POINTF __RPC_FAR *pPtfContainer,DWORD dwFlags)
        { return E_NOTIMPL; };
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG __RPC_FAR *pMsg,DWORD grfModifiers);

    virtual HRESULT STDMETHODCALLTYPE OnFocus(BOOL fGotFocus)
        { return E_NOTIMPL; };
    virtual HRESULT STDMETHODCALLTYPE ShowPropertyFrame(void)
        { return E_NOTIMPL; };

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

    // IInternetSecurityManager
    virtual STDMETHODIMP SetSecuritySite(IInternetSecurityMgrSite *pSite) { return INET_E_DEFAULT_ACTION; };
    virtual STDMETHODIMP GetSecuritySite(IInternetSecurityMgrSite **ppSite) { return INET_E_DEFAULT_ACTION; };
    virtual STDMETHODIMP MapUrlToZone(LPCWSTR pwszUrl, DWORD * pdwZone, DWORD dwFlags) { return INET_E_DEFAULT_ACTION; };
    virtual STDMETHODIMP GetSecurityId(LPCWSTR pwszUrl, BYTE * pbSecurityId, DWORD * pcbSecurityId, DWORD_PTR dwReserved) { return INET_E_DEFAULT_ACTION; };
    virtual STDMETHODIMP ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE * pPolicy, DWORD cbPolicy, BYTE * pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
    virtual STDMETHODIMP QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE ** ppPolicy, DWORD * pcbPolicy, BYTE * pContext, DWORD cbContext, DWORD dwReserved) { return INET_E_DEFAULT_ACTION; };
    virtual STDMETHODIMP SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags) { return INET_E_DEFAULT_ACTION; };
    virtual STDMETHODIMP GetZoneMappings(DWORD dwZone, IEnumString ** ppenumString, DWORD dwFlags) { return INET_E_DEFAULT_ACTION; };

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount) (unsigned int *pctinfo)
        { return E_NOTIMPL; };
    STDMETHOD(GetTypeInfo) (unsigned int itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return E_NOTIMPL; };
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)
        { return E_NOTIMPL; };
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams,
                        VARIANT FAR* pvarResult,EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr);

    CHostDropTarget _dt;

    IExpDispSupport * m_peds;

} ;

class CSFVFrame : public IOleInPlaceFrame
                , public IAdviseSink
                , public IPropertyNotifySink  //for READYSTATE
{
public:
    enum
    {
        UNDEFINEDVIEW = -3,
        NOEXTVIEW = -2,
        HIDEEXTVIEW = -1,
    } ;

    CSFVFrame() : m_uView(NOEXTVIEW), m_bEnableStandardViews(TRUE),
        m_pOleObj(NULL), m_pActiveSVExt(NULL), m_pActiveSFV(NULL), m_pDefViewExtInit2(NULL),
        m_bgColor(CLR_INVALID), m_uActiveExtendedView(UNDEFINEDVIEW), m_uViewNew(NOEXTVIEW)
    {
    }
    ~CSFVFrame();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // IOleInPlaceUIWindow
    STDMETHODIMP GetBorder(LPRECT lprectBorder);
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);

    // IOleInPlaceFrame
    STDMETHODIMP InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHODIMP SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHODIMP RemoveMenus(HMENU hmenuShared);
    STDMETHODIMP SetStatusText(LPCOLESTR pszStatusText);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP TranslateAccelerator(LPMSG lpmsg, WORD wID);

    // IAdviseSink
    STDMETHODIMP_(void) OnDataChange(FORMATETC *, STGMEDIUM *);
    STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex);
    STDMETHODIMP_(void) OnRename(IMoniker *);
    STDMETHODIMP_(void) OnSave();
    STDMETHODIMP_(void) OnClose();

    // *** IPropertyNotifySink methods ***
    virtual STDMETHODIMP OnChanged(DISPID dispid);
    virtual STDMETHODIMP OnRequestEdit(DISPID dispid);

    COLORREF   m_bgColor;  //Icon text background color for active desktop
private:
    friend class CSFVSite;
    CSFVSite m_cSite;

    friend class CDefView;

    class CBindStatusCallback : public IBindStatusCallback
                              , public IServiceProvider
    {
        friend CSFVFrame;
    protected:
        // *** IUnknown methods ***
        virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
        virtual STDMETHODIMP_(ULONG) AddRef(void) ;
        virtual STDMETHODIMP_(ULONG) Release(void);
    
        // *** IServiceProvider ***
        virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);
    
        // *** IBindStatusCallback ***
        virtual STDMETHODIMP OnStartBinding(
            /* [in] */ DWORD grfBSCOption,
            /* [in] */ IBinding *pib);
        virtual STDMETHODIMP GetPriority(
            /* [out] */ LONG *pnPriority);
        virtual STDMETHODIMP OnLowResource(
            /* [in] */ DWORD reserved);
        virtual STDMETHODIMP OnProgress(
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        virtual STDMETHODIMP OnStopBinding(
            /* [in] */ HRESULT hresult,
            /* [in] */ LPCWSTR szError);
        virtual STDMETHODIMP GetBindInfo(
            /* [out] */ DWORD *grfBINDINFOF,
            /* [unique][out][in] */ BINDINFO *pbindinfo);
        virtual STDMETHODIMP OnDataAvailable(
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC *pformatetc,
            /* [in] */ STGMEDIUM *pstgmed);
        virtual STDMETHODIMP OnObjectAvailable(
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk);
    };
    
    friend class CBindStatusCallback;
    CBindStatusCallback _bsc;


//
// External views stuff
//
// We have DocObject extensions and IShellView extensions
// A (DocObject) extension can
public:
    HRESULT InitObj(IUnknown* pObj, LPCITEMIDLIST pidlHere, int iView);
    HRESULT _DefaultGetExtViews(SHELLVIEWID * pvid, IEnumSFVViews ** ppev);
    void GetExtViews(BOOL bForce=FALSE);
    void MergeExtViewsMenu(HMENU hmenuView, CDefView * pdsv);
    BOOL _StringFromView(int uView, LPTSTR pszString, UINT cb, UINT idString);
    BOOL _StringFromViewID(SHELLVIEWID const *pvid, LPTSTR pszString, UINT cb, UINT idString);
    BOOL _ColorFromView(int uView, COLORREF * pcr, int crid);
    BOOL _ColorFromViewID(SHELLVIEWID const *pvid, COLORREF * pcr, int crid);
    HRESULT ShowExtView(int uView, BOOL bForce);
    HRESULT ShowExtView(SHELLVIEWID const *pvid, BOOL bForce);
    int CmdIdFromUid(int uid);
    int CurExtViewId()   // only works for docobj extended views.
    {
        if(m_uActiveExtendedView >= 0)
            return CmdIdFromUid(m_uActiveExtendedView);
        else
        {
            if(m_pOleObjNew)
            return CmdIdFromUid(m_uViewNew);
            else
                return(-1);
        }
    }
    UINT UidFromCmdId(UINT cmdid) 
    { 
        ASSERT((cmdid >= SFVIDM_VIEW_EXTFIRST) &&
               ((cmdid - SFVIDM_VIEW_EXTFIRST) < (UINT) ARRAYSIZE(m_mpCmdIdToUid)));
        ASSERT(m_mpCmdIdToUid[cmdid - SFVIDM_VIEW_EXTFIRST] != -1)
        return (m_mpCmdIdToUid[cmdid - SFVIDM_VIEW_EXTFIRST]);
    };
    // IsWebView - if the current view is undefined, then we've never
    // switched into an extended view (ie, the window was just created), so
    // check the pending view.  (If the default is non web view, then there's no
    // pending view (it will be NOEXTVIEW) so we correctly return FALSE in that case.)
    //
    // NOTE: this used to be IsExtendedView, since we generically support all doc-object
    // type extended views, but this is only used for Web View, and in NT5 we specifically
    // limited the doc-object support to VID_WebView, so I renamed this for clarity...
    //
    BOOL IsWebView(void) { return (UNDEFINEDVIEW == m_uActiveExtendedView ? m_uViewNew != NOEXTVIEW : m_uActiveExtendedView != NOEXTVIEW); }
    BOOL IsSFVExtension(void) { return (NULL != m_pActiveSVExt); }
    HRESULT _HasFocusIO();
    HRESULT _UIActivateIO(BOOL fActivate, MSG *pMsg);

    int  GetViewIdFromGUID(SHELLVIEWID const *pvid, SFVVIEWSDATA** ppItem = NULL);
    HWND GetExtendedViewWindow();

    // query if the view requires delegation
    IShellFolderView * GetExtendedISFV( void ) { return m_pActiveSFV; }
    IShellView2 * GetExtendedISV( void ) { return m_pActiveSVExt; }
    BOOL IsExtendedSFVModal();
    HRESULT SFVAutoAutoArrange( DWORD dwReserved);
    void KillActiveSFV(void);
    BOOL _IsHoster(int uView);
    HRESULT SetViewWindowStyle(DWORD dwBits, DWORD dwVal);

    HRESULT SetRect(LPRECT prc);

    HRESULT GetView(SHELLVIEWID* pvid, ULONG uView);
    HRESULT GetCurrentExtView(SHELLVIEWID* pvid);

    HRESULT GetCommandTarget(IOleCommandTarget** ppct);

    // allow the frame to handle the choice on delegation on translate accelerator...
    HRESULT OnTranslateAccelerator(LPMSG pmsg, BOOL* pbTabOffLastTridentStop);

    HRESULT _SetDesktopListViewIconTextColors(BOOL fNotify);    // used in defview.cpp

private:
    CViewsList m_lViews;
    BOOL m_bGotViews;
    BOOL m_bEnableStandardViews;
    TCHAR m_szWebViewKeyName[MAX_PATH];

    int m_uView;                      // these are the sfvextension views.
    int m_uActiveExtendedView;       // toggleable view on???
    UINT m_cSFVExt;
    UINT m_uState:2;                // SVUIA_* for m_pOleObj (extended view)
    IOleObject* m_pOleObj;
    IOleDocumentView* m_pDocView;
    IOleInPlaceActiveObject* m_pActive;
    IViewObject *m_pvoActive;

    // for now, assume only one view-ext, later on, need to keep alive them all...
    IShellView2 * m_pActiveSVExt;

    // the IShellfolderView that must be delegated to....
    IShellFolderView * m_pActiveSFV;

    // Implemented by thumbnail
    IDefViewExtInit2* m_pDefViewExtInit2;

    HWND m_hActiveSVExtHwnd;

    friend LRESULT CALLBACK DefView_WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
    friend void DefView_SetFocus(CDefView * pdsv);

    void _CleanUpOleObj(IOleObject* pOleObj);
    void _CleanUpOleObjAndDt(IOleObject* pOleObj);
    void _CleanupNewOleObj();
    void _CleanupOldDocObject( void );

    HRESULT _ShowExtView_Helper(IOleObject* pOleObj, int uView);
    HRESULT _CreateNewOleObj(IOleObject **ppOleObj, int uView);
    HRESULT _SwitchToNewOleObj();
    HRESULT _UpdateZonesStatusPane(IOleObject *pOleObj);

    //Fields that store details about the new OLE object while we wait for
    //it to reach a READYSTATE_INTERACTIVE.
    IOleObject* m_pOleObjNew;
    int      m_uViewNew;
    BOOL m_fSwitchedToNewOleObj:1;
    int m_mpCmdIdToUid[MAX_EXT_VIEWS];    // map command id's to view list id's

    BOOL _SetupReadyStateNotifyCapability();
    BOOL _RemoveReadyStateNotifyCapability();

    DWORD    m_dwConnectionCookie;
    BOOL     m_fReadyStateInteractiveProcessed;
    BOOL     m_fReadyStateComplete;
    IOleObject* m_pOleObjReadyState;
} ;


class CCallback
{
public:
    CCallback(IShellFolderViewCB* psfvcb) : m_psfvcb(psfvcb)
    {
        if (m_psfvcb)
            m_psfvcb->AddRef();
    }
    ~CCallback()
    {
        if (m_psfvcb)
            m_psfvcb->Release();
    }

    IShellFolderViewCB *GetSFVCB() { return m_psfvcb; }

    HRESULT SetCallback(IShellFolderViewCB* pNewCB, IShellFolderViewCB** ppOldCB)
    {
        // We Release the callback for us, and then AddRef it for the caller who now
        // owns the object, which does nothing
        *ppOldCB = m_psfvcb;
        m_psfvcb = pNewCB;
        if (pNewCB)
            pNewCB->AddRef();
        return NOERROR;
    }

    HRESULT CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return m_psfvcb ? m_psfvcb->MessageSFVCB(uMsg, wParam, lParam) : E_NOTIMPL;
    }

    BOOL HasCB() { return m_psfvcb != NULL; }

private:
    IShellFolderViewCB* m_psfvcb;
} ;

// class used to provide the background context menu in defview extensions
HRESULT CBackgrndMenu_CreateInstance( CDefView * pDefView, REFIID riid, void ** ppvObj );

class CBackgrndMenu : public IContextMenu3,
                      public IObjectWithSite

{
    public:
        friend HRESULT CBackgrndMenu_CreateInstance( CDefView * pDefView, REFIID riid, void ** ppvObj );

        STDMETHOD ( QueryInterface ) ( REFIID riid, void ** ppvObj );
        STDMETHOD_( ULONG, AddRef ) ( void );
        STDMETHOD_( ULONG, Release ) ( void );

        STDMETHOD(QueryContextMenu)( HMENU hmenu, UINT indexMenu,
                                     UINT idCmdFirst, UINT idCmdLast,
                                     UINT uFlags);

        STDMETHOD(InvokeCommand)( LPCMINVOKECOMMANDINFO lpici );

        STDMETHOD(GetCommandString)( UINT_PTR idCmd, UINT uType,
                                     UINT * pwReserved, LPSTR pszName,
                                     UINT cchMax);

        STDMETHOD(HandleMenuMsg)( UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam);
        STDMETHOD(HandleMenuMsg2)( UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  LRESULT* plResult);
        STDMETHOD(SetSite)(IUnknown*);
        STDMETHOD(GetSite)(REFIID,void**);

    protected:
        CBackgrndMenu( CDefView * pDefView, HRESULT * pHr );
        ~CBackgrndMenu();

        CDefView * m_pDefView;
        LONG m_cRef;
        IContextMenu * m_pFolderMenu;
        IContextMenu * m_pcmSel;
        IUnknown* m_powsSite;
        BOOL m_fFlush : 1;
        BOOL m_fpcmSelAlreadyThere : 1;
};

// Variable Column stuff

typedef struct
{
    TCHAR szName[MAX_COLUMN_NAME_LEN];
    DWORD cChars;   // number of characters wide for default
    DWORD fmt;
    DWORD csFlags;  // SHCOLSTATE flags
} COL_INFO;

//Possible values for m_iCustomizable
#define YES_CUSTOMIZABLE                1
#define DONTKNOW_IF_CUSTOMIZABLE        0
#define MAYBE_CUSTOMIZABLE             -1 //It is a filesystem folder
#define NOT_CUSTOMIZABLE               -2

// For communicating with the background property extractor
class CBackgroundColInfo
{
    private:
        CBackgroundColInfo (void);
    public:
        CBackgroundColInfo (LPCITEMIDLIST pidl, UINT uiCol, STRRET& strRet);
        ~CBackgroundColInfo (void);

        LPCITEMIDLIST   GetPIDL (void)      const   {   return(_pidl);          }
        UINT            GetColumn (void)    const   {   return(_uiCol);         }
        LPCTSTR         GetText (void)      const   {   return(&_szText[0]);    }
    private:
        const LPCITEMIDLIST     _pidl;
        const UINT              _uiCol;
              TCHAR             _szText[MAX_COLUMN_NAME_LEN];
};


#define CGID_DefViewFrame2   IID_IDefViewFrame2


//
// Class definition of CDefView
//
class CDefView : // dsv
    public IShellView2,
    public IShellFolderView,
    public IOleCommandTarget, // so psb can talk to extended views
    public IDropTarget,
    public IViewObject,
    public IDefViewFrame2,
    public IServiceProvider,
    public IDocViewSite,
    public IInternetSecurityMgrSite
    {
public:
    CDefView(LPSHELLFOLDER pshf, IShellFolderViewCB* psfvcb, IShellView* psvOuter);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // IShellView
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP Refresh();
    STDMETHODIMP CreateViewWindow(IShellView *lpPrevView,
        LPCFOLDERSETTINGS lpfs, IShellBrowser *psb, RECT *prc, HWND *phWnd);
    STDMETHODIMP DestroyViewWindow();
    STDMETHODIMP UIActivate(UINT uState);
    STDMETHODIMP GetCurrentInfo(LPFOLDERSETTINGS lpfs);
    STDMETHODIMP TranslateAccelerator(LPMSG pmsg);
    STDMETHODIMP AddPropertySheetPages(DWORD dwReserved, LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam);
    STDMETHODIMP SaveViewState();
    STDMETHODIMP SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags);
    STDMETHODIMP GetItemObject(UINT uItem, REFIID riid, void **ppv);
    // IShellView2
    STDMETHOD(GetView)(SHELLVIEWID* pvid, ULONG uView);
    STDMETHOD(CreateViewWindow2)(LPSV2CVW2_PARAMS pParams);
    STDMETHOD(HandleRename)(LPCITEMIDLIST pidl);
    STDMETHOD(SelectAndPositionItem)(LPCITEMIDLIST pidlItem, UINT uFlags, POINT *ppt);

    // *** IShellFolderView methods ***
    STDMETHOD(Rearrange) (LPARAM lParamSort);
    STDMETHOD(GetArrangeParam) (LPARAM *plParamSort);
    STDMETHOD(ArrangeGrid) (THIS);
    STDMETHOD(AutoArrange) (THIS);
    STDMETHOD(GetAutoArrange) (THIS);
    STDMETHOD(AddObject) (LPITEMIDLIST pidl, UINT *puItem);
    STDMETHOD(GetObject) (LPITEMIDLIST *ppidl, UINT uItem);
    STDMETHOD(RemoveObject) (LPITEMIDLIST pidl, UINT *puItem);
    STDMETHOD(GetObjectCount) (UINT *puCount);
    STDMETHOD(SetObjectCount) (UINT uCount, UINT dwFlags);
    STDMETHOD(UpdateObject) (LPITEMIDLIST pidlOld, LPITEMIDLIST pidlNew,
        UINT *puItem);
    STDMETHOD(RefreshObject) (LPITEMIDLIST pidl, UINT *puItem);
    STDMETHOD(SetRedraw) (BOOL bRedraw);
    STDMETHOD(GetSelectedCount) (UINT *puSelected);
    STDMETHOD(GetSelectedObjects) (LPCITEMIDLIST **pppidl, UINT *puItems);
    STDMETHOD(IsDropOnSource) (IDropTarget *pDropTarget);
    STDMETHOD(GetDragPoint) (POINT *ppt);
    STDMETHOD(GetDropPoint) (POINT *ppt);
    STDMETHOD(MoveIcons) (IDataObject *pDataObject);
    STDMETHOD(SetItemPos) (LPCITEMIDLIST pidl, POINT *ppt);
    STDMETHOD(IsBkDropTarget) (IDropTarget *pDropTarget);
    STDMETHOD(SetClipboard) (BOOL bMove);
    STDMETHOD(SetPoints) (IDataObject *pDataObject);
    STDMETHOD(GetItemSpacing) (ITEMSPACING *pSpacing);
    STDMETHOD(SetCallback) (IShellFolderViewCB* pNewCB,
        IShellFolderViewCB** ppOldCB);
    STDMETHOD(Select) (UINT dwFlags );
    STDMETHOD(QuerySupport) (UINT * pdwSupport );
    STDMETHOD(SetAutomationObject) (IDispatch *pdisp);

    // IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
        { return _dvdt.DragEnter(pdtobj, grfKeyState, ptl, pdwEffect); }
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
        { return _dvdt.DragOver(grfKeyState, ptl, pdwEffect); }
    STDMETHODIMP DragLeave()
        { return _dvdt.DragLeave(); }
    STDMETHODIMP Drop(IDataObject *pdtobj,
                    DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { return _dvdt.Drop(pdtobj, grfKeyState, pt, pdwEffect); }

    // IViewObject
    STDMETHODIMP Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
        const RECTL *, const RECTL *, int (*)(ULONG_PTR), ULONG_PTR);
    STDMETHODIMP GetColorSet(DWORD, LONG, void *, DVTARGETDEVICE *, HDC,
        LOGPALETTE **);
    STDMETHODIMP Freeze(DWORD, LONG, void *, DWORD *);
    STDMETHODIMP Unfreeze(DWORD);
    STDMETHODIMP SetAdvise(DWORD, DWORD, IAdviseSink *);
    STDMETHODIMP GetAdvise(DWORD *, DWORD *, IAdviseSink **);

    // IDefViewFrame
    STDMETHODIMP GetWindowLV(HWND * phwnd);
    STDMETHODIMP ReleaseWindowLV(void);
    STDMETHODIMP GetShellFolder(IShellFolder **ppsf);

    // IDefViewFrame2
    STDMETHODIMP GetWindowLV2(HWND * phwnd, IUnknown * punk);
    STDMETHODIMP AutoAutoArrange(DWORD dwReserved);

    // IServiceProvider
    STDMETHODIMP  QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IDocViewSite methods ***
    STDMETHOD(OnSetTitle) (VARIANTARG *pvTitle);

    HRESULT InitializeVariableColumns(DWORD *pdwColList);
    BOOL IsColumnHidden(UINT uCol);
    BOOL IsColumnOn(UINT uCol);
    HRESULT AddColumnsToMenu(HMENU hm, DWORD dwBase);
    HRESULT SetColumnState(UINT uCol, DWORD dwMask, DWORD dwState);
    HRESULT MapRealToVisibleColumn(UINT uRealCol, UINT *puVisCol);
    HRESULT MapVisibleToRealColumn(UINT uVisCol, UINT *puReal);
    UINT GetMaxColumns();

    // handle messages
    LRESULT _OnCreate(HWND hWnd);
    LRESULT _OnNotify(NMHDR *pnm);
    LRESULT _TBNotify(NMHDR *pnm);
    LRESULT _OnLVNotify(NM_LISTVIEW *plvn);
    LRESULT _OnBeginDrag(NM_LISTVIEW * lpnm);

    int _FindItem(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlFound, BOOL fSamePtr);
    int _UpdateObject(LPITEMIDLIST *ppidl, BOOL bCopy = FALSE);
    void _FilterDPAs(HDPA hdpa, HDPA hdpaOld);
    int _RefreshObject(LPITEMIDLIST *ppidl);
    int _RemoveObject(LPCITEMIDLIST pidl, BOOL fSamePtr);
    BOOL _GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt);

    void _OnGetInfoTip(NMLVGETINFOTIP *plvn);

    void _OnRename(LPCITEMIDLIST* ppidl);
    LPITEMIDLIST _ObjectExists(LPCITEMIDLIST pidl);
    UINT _GetExplorerFlag();

    // private stuff
    void PropagateOnViewChange(DWORD dwAspect, LONG lindex);
    void PropagateOnClose();
    BOOL OnActivate(UINT uState);
    BOOL OnDeactivate();
    void SwapWindow(void);
    BOOL HasCurrentViewWindowFocus();
    HWND ViewWindowSetFocus();

    BOOL _IsReportView();
    
    inline BOOL _IsOwnerData() { return _fs.fFlags & FWF_OWNERDATA; }
    inline BOOL _IsDesktop()   { return _fs.fFlags & FWF_DESKTOP; }

    int CheckCurrentViewMenuItem(HMENU hmenu);
    void InitViewMenu(HMENU hmInit);
    void CheckToolbar();
    void OnListViewDelete(int iItem, LPITEMIDLIST pidl);
    void HandleKeyDown(LV_KEYDOWN *lpnmhdr);
    BOOL SaveCols(LPSTREAM pstm);
    HRESULT SavePos(LPSTREAM pstm);
    void AddColumns();
    void _ShowControl(UINT uControl, int idCmd);
    LRESULT OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu);
    void _SetUpMenus(UINT uState);
    void SelectSelectedItems();
    inline BOOL _fDeferSelect() {return 
        (this->_hwndStatic || !_HasNormalView() || (_uState == SVUIA_DEACTIVATE) || 
                (m_cFrame.m_dwConnectionCookie /*&& !m_cFrame.m_fReadyStateInteractiveProcessed*/ ));}
    inline BOOL _fItemsDeferred() { return _hdsaSelect != NULL; }
    void _ClearSelectList();
    void DropFiles(HDROP hdrop);
    LRESULT OldDragMsgs(UINT uMsg, WPARAM wParam, const DROPSTRUCT * lpds);
    void AddCopyHook();
    int FindCopyHook(BOOL fRemoveInvalid);
    void RemoveCopyHook();
    void ContextMenu(DWORD dwPos);
    LPITEMIDLIST _GetViewPidl(); // return copy of pidl of folder we're viewing
    BOOL _IsViewDesktop();
    BOOL _GetPath(LPTSTR pszPath);
    HRESULT _GetNameAndFlags(UINT gdnFlags, LPTSTR psz, UINT cch, DWORD *pdwFlags);
    int _CheckIfCustomizable();

    LRESULT SwitchToHyperText(UINT uID, BOOL fForce);
    LRESULT Command(IContextMenu *pcmSel, WPARAM wParam, LPARAM lParam);
    LRESULT WndSize(HWND hWnd);
    BOOL EnumerationTimeout(BOOL bRefresh);
    void _ShowListviewIcons();
    void _OnMenuTermination();
    void FillDone(HDPA hdpaNew,
        PDVSAVEHEADER pSaveHeader, UINT uLen, BOOL bRefresh, BOOL fInteractive);
    void OnLVSelectionChange(NM_LISTVIEW *plvn);
    void RegisterSFVEvents(IUnknown * pTarget, BOOL fConnect);
    HRESULT FillObjects(BOOL bRefresh,
        PDVSAVEHEADER pSaveHeader, UINT uLen, BOOL fInteractive);
    HRESULT FillObjectsShowHide(BOOL bRefresh,
        PDVSAVEHEADER pSaveHeader, UINT uLen, BOOL fInteractive);

    HRESULT _GetDetailsHelper(int i, DETAILSINFO *pdi);
    HRESULT CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL HasCB() {return(m_cCallback.HasCB());}
    HRESULT NotifyAutomation(DISPID dispid);
    void CheckIfSelectedAndNotifyAutomation(LPCITEMIDLIST pidl, int iItem);
    HRESULT _ReloadListviewContent();
    HRESULT ReloadContent(BOOL fForce = FALSE);
    BOOL _HasNormalView()
        { return(!m_cFrame.IsWebView() || _fGetWindowLV || _fCombinedView); }

    HRESULT _SwitchToViewIDPVID(UINT uID, SHELLVIEWID const *pvid, BOOL bForce);
    HRESULT _SwitchToViewID(UINT uID, BOOL bForce)
        { return(_SwitchToViewIDPVID(uID, NULL, bForce)); }
    HRESULT _SwitchToViewPVID(SHELLVIEWID const *pvid, BOOL bForce)
        { return(_SwitchToViewIDPVID(0, pvid, bForce)); }
    HRESULT _SwitchToViewFVM(UINT fvmNew, DWORD dwStyle, BOOL fForce);
    void _UpdateListviewColors(BOOL fClassic);
    LRESULT _SwitchDesktopHTML(BOOL fOn, BOOL fForce);
    void InitSelectionMode();
    void UpdateSelectionMode();

    void _OnMoveWindowToTop(HWND hwnd);

    HWND GetChildViewWindow();
    BOOL _InvokeCustomWizard();

    HRESULT _OnViewWindowActive();
    void _UpdateRegFlags();

    BOOL _CheckSingleClickDialog(void);

    void _DoColumnsMenu(int x, int y);
    BOOL _HandleColumnToggle(UINT uCol, BOOL bRefresh);
    void _SameViewMoveIcons();
    void _PostSelChangedMessage();
    HRESULT _GetIPersistHistoryObject(IPersistHistory **ppph);
    BOOL _ShouldEnableButton(UINT uiCmd, DWORD dwAttr, int iIndex);
    void _EnableDisableTBButtons();

    HRESULT _SaveViewState(IStream *pstm);
    HRESULT _GetStorageStream(DWORD grfMode, IStream* *ppIStream);
    HRESULT _SaveGlobalViewState(void);
    HRESULT _LoadGlobalViewState(IStream* *ppIStream);
    HRESULT _ResetGlobalViewState(void);
    LPITEMIDLIST _GetPIDL(int i);

    BOOL _IsGhosted(LPCITEMIDLIST pidl);
    void _RestoreAllGhostedFileView();
    BOOL _CanShowWebView();

    LONG                    _cRef;
    SHELLVIEWID const       *_pvidPending;

    CDVDropTarget           _dvdt;

    IShellView *            _psvOuter;       // May be NULL
    IShellFolder            *_pshf;
    IShellFolder2           *_pshf2;
    IShellBrowser           *_psb;
    ICommDlgBrowser         *_pcdb;          // extended ICommDlgBrowser
    FOLDERSETTINGS          _fs;
    IContextMenu            *_pcmSel;       // pcm for selected objects.
    IContextMenu            *_pcmBackground;
    DWORD                   _dwAttrSel;      // dwAttrs for selected objects
    IShellIcon *            _psi;            // for getting icon fast
    IShellIconOverlay *     _psio;           // For getting iconOverlay fast
    CLSID                   _clsid;         // the clsid of this pshf;
    
    HWND                    _hwndMain;
    HWND                    _hwndView;
    HWND                    _hwndListview;
    HWND                    _hwndStatic;
    HACCEL                  _hAccel;
    int                     _fmt;

#ifdef DEBUG
    HRESULT                 _hres;           // Enum result, used for tracemsgs
#endif

    UINT                    _uState;         // SVUIA_*
    HMENU                   _hmenuCur;

    ULONG                   _uRegister;

    POINT                   _ptDrop;

    POINT                   _ptDragAnchor;   // start of the drag
    int                     _itemCur;        // The current item in the drop target

    IDataObject             *_pdtobjHdrop;   // for 3.1 HDROP drag/drop
    IDropTarget             *_pdtgtBack;     // of the background (shell folder)

    IShellDetails          *_psd;
    // Officially, _pdr should be an IDelayedRelease interface, but it
    // only has IUnknown member functions, so why bother?
    IUnknown                *_pdr;
    UINT                    _cxChar;

    LPCITEMIDLIST           _pidlMonitor;
    LONG                    _lFSEvents;

    DVSAVESTATE             _dvState;
    PDVSAVEHEADER           _pSaveHeader;
    UINT                    _uSaveHeaderLen;
    TBBUTTON*               _pbtn;
    int                     _cButtons;          // count of buttons that are showing by default
    int                     _cTotalButtons;     // count of buttons including those hidden by default

    IShellTaskScheduler   * _pScheduler;
    LONG                    _cRefForIdle;    // did idle thread forget
                                            // to release this

    BITBOOL     _bDragSource:1;
    BITBOOL     _bDropAnchor:1;

    BITBOOL     _bItemsMoved:1;
    BITBOOL     _bClearItemPos:1;

    BITBOOL     _bHaveCutStuff:1;
    BITBOOL     _bClipViewer:1;

    BITBOOL     _fShowAllObjects:1;
    BITBOOL     _fInLabelEdit:1;
    BITBOOL     _fDisabled:1;

    BITBOOL     _bUpdatePending:1;
    BITBOOL     _bUpdatePendingPending:1;
    BITBOOL     _bBkFilling:1;

    BITBOOL     _bContextMenuMode:1;
    BITBOOL     _bMouseMenu:1;
    BITBOOL     _fHasDeskWallPaper:1;

    BITBOOL     _fShowCompColor:1;

    BITBOOL     _bRegisteredDragDrop:1;

    BITBOOL     _fEnumFailed:1;    // TRUE if enum failed.

    BITBOOL     _fGetWindowLV:1;

    BITBOOL     _fClassic:1; // SSF_WIN95CLASSIC setting/restriction

    BITBOOL     _fCombinedView:1;   // Implies a regional listview layered on top of an extended view
    BITBOOL     _fCycleFocus:1;     // 1=got callback to do CycleFocus

    BITBOOL     _fSelectionChangePending:1;
    BITBOOL     _fCanActivateNow:1; // FALSE from creation until we can be activated, TRUE implies we can SHDVID_CANACTIVATENOW
    BITBOOL     _fShowListviewIconsOnActivate:1; // TRUE iff a _ShowListviewIcons() came in while 'invisible'
    BITBOOL     _fWin95ViewState:1;    // TRUE iff Advanced option set to Win95 behavior
    BITBOOL     _fDesktopModal:1;      // TRUE iff desktop is in modal state.
    BITBOOL     _fDesktopRefreshPending:1; // TRUE iff a refresh of desktop was prevented because of modal state.
    BITBOOL     _fRefreshBuffered:1;   // TRUE iff a buffered refresh is pending!
    BITBOOL     _fHasListViewFocus:1;
    BITBOOL     _bExtHasFocus:1;    //TRUE if SFV extension's state is SVUIA_ACTIVATE_FOCUS
    BITBOOL     _fIsXV:1;           // extended shell view (thumbview) hosted in OC
    BITBOOL     _bInSortCallBack:1;     // TRUE if defview should not start background task to get extended col data
    BITBOOL     _bLoadedColumns:1;     // TRUE after we've loaded cols from the savestream. (after we're switched to details)
    // Combined view colors that can be specified via registry or desktop.ini

    BITBOOL     _fIsAsyncDefView:1; //TRUE if Defview is Asynchronous
    BITBOOL     _bEmptyingScheduler : 1;  // used to stop re-entracny in the task scheduler..

    BITBOOL     _bAutoSelChangeTimerSet:1;  // indicates if the timer to send the sel change notification to the automation obj is set
    
    COLORREF                _crCustomColors[CRID_COLORCOUNT];

    // for single click activation
    DWORD                   _dwSelectionMode;

    HWND                    _hwndNextViewer;

    LRESULT                 _iStdBMOffset;
    LRESULT                 _iViewBMOffset;

    CCallback               m_cCallback;    // Optional client callback

    HDSA                    _hdsaSelect;    // List of items that are selected.

    int                     _iLastFind;

    HANDLE                  _AsyncIconEvent;
    long                    _AsyncIconCount;
    ULONG                   _AsyncIconTime;

    UINT                    _uDefToolbar;
    CSFVFrame               m_cFrame;

    ULONG                   _uCachedSelAttrs;
    UINT                    _uCachedSelCount;
    
    class CColumnPointer *_pcp;
    COL_INFO    *m_pColItems;
    UINT        m_cColItems;
    DWORD       m_dwConnectionCookie;
    DWORD       _LastSortColType;   // used in sorting extended columns

#ifdef DEBUG
    TIMEVAR(_Update);
    TIMEVAR(_Fill);
    TIMEVAR(_GetIcon);
    TIMEVAR(_GetName);
    TIMEVAR(_FSNotify);
    TIMEVAR(_AddObject);
    TIMEVAR(_EnumNext);
    TIMEVAR(_RestoreState);
    TIMEVAR(_WMNotify);
    TIMEVAR(_LVChanging);
    TIMEVAR(_LVChanged);
    TIMEVAR(_LVDelete);
    TIMEVAR(_LVGetDispInfo);
#endif

public:     // TODO: Make this protected after we have finished converting the entire file.
    BOOL IsSafeToDefaultVerb(void);
    void _ProcessDblClick(LPNMITEMACTIVATE pnmia);
    HRESULT _InvokeCommand(IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici);
    void _FocusOnSomething(void);
    void _UpdateIcon(CDVGetIconTask *paid);
    void _UpdateColData(CBackgroundColInfo *pbgci);
    void _UpdateOverlay(int iList, int iOverlay);
    HRESULT _GetIconAsync(LPCITEMIDLIST pidl, int *piIcon, BOOL fCanWait);
    HRESULT _GetOverlayIndexAsync(LPCITEMIDLIST pidl, int iList);
    HRESULT EmptyBkgrndThread( BOOL fTerminate );
    IContextMenu *_GetContextMenuFromSelection();
    DWORD _GetNeededSecurityAction(void);
    HRESULT _ZoneCheck(DWORD dwFlags, DWORD dwAllowAction);
    void _ShowAndActivate();

private:
    ~CDefView();

    void MergeViewMenu(HMENU hmenu, HMENU hmenuMerge);

    void MergeToolBar(BOOL bCanRestore);
    BOOL _MergeIExplorerToolbar(UINT cExtButtons);
    void _CopyDefViewButton(PTBBUTTON ptbbDest, PTBBUTTON ptbbSrc);
    int _GetButtons(PTBBUTTON* ppbtn, LPINT pcButtons, LPINT pcTotalButtons);

    void _PostEnumDoneMessage();
    void _SetCachedToolbarSelectionAttrs(ULONG dwAttrs);
    BOOL _GetCachedToolbarSelectionAttrs(ULONG *pdwAttr);

    LRESULT _OnFSNotify(LONG lNotification, LPCITEMIDLIST* ppidl);

    BOOL _InternalRearrange(void);
    BOOL _IsExtendedColumn(INT_PTR iReal, DWORD *pdwState);
    void _RestoreState(PDVSAVEHEADER pInSaveHeader, UINT uLen);
    BOOL _RestorePos(PDVSAVEHEADER pSaveHeader, UINT uLen);
    UINT _GetBackgroundTaskCount(REFTASKOWNERID rtid);
    void _SetSortArrows(void);
    UINT _GetSaveHeader(PDVSAVEHEADER *ppSaveHeader);
    PFNDPACOMPARE _GetCompareFunction(void);
    void _GetSortDefaults(DVSAVESTATE *pSaveState);

    static  int CALLBACK _CompareExact(void *p1, void *p2, LPARAM lParam);
    static  int CALLBACK _Compare(void *p1, void *p2, LPARAM lParam);
    static  int CALLBACK _CompareExtended(LPARAM dw1, LPARAM dw2, LPARAM lParam);
    static  int CALLBACK _DVITEM_Compare(void *p1, void *p2, LPARAM lParam);

    friend class CBackgrndMenu;
    friend class CSFVSite;
    friend class CSFVFrame;
    friend class CDVBkgrndEnumTask;
    
    IWebViewOCWinMan    *_pocWinMan;

    UINT                m_uCols;

    IDispatch           *m_pauto;      // pointer to idispatch

    BOOL     _fFileListEnumDone : 1;  // wait until the automation object is loaded, to notify it of DISPID_FILELISTENUMDONE
    BOOL     _fEnumDoneNotified : 1;  // make sure that we don't notify DISPID_FILELISTENUMDONE more than once

    // advisory connection
    IAdviseSink *_padvise;
    DWORD _advise_aspect;
    DWORD _advise_advf;

    // Is this folder customizable using a desktop.ini?
    // In other words, is this folder in a write-able media AND either it 
    // not have a desktop.ini OR if it is there, it is writeable!
    int   m_iCustomizable;

    friend LRESULT CALLBACK DefView_WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
    friend DWORD DefView_GetAttributesFromSelection(CDefView * pdsv, DWORD dwAttrMask);
    friend void ViewWindow_BestFit(CDefView * pdsv, BOOL bTimeout);
    friend void DefView_InitSelectionMode(CDefView * pdsv);
    friend void DefView_SetViewMode(CDefView * pdsv, UINT fvmNew, DWORD dwStyle);
    friend void DefView_SetFocus(CDefView * pdsv);


    friend void DV_GetMenuHelpText(CDefView * pdsv, UINT_PTR id, LPTSTR pszText, UINT cchText);
    friend void DV_GetToolTipText(CDefView * pdsv, UINT_PTR id, LPTSTR pszText, UINT cchText);
    friend void SetDefaultViewSettings(CDefView * pdsv);
    friend void DV_PaintErrMsg(HWND hWnd, CDefView * pdsv);
    friend void DV_PaintCombinedView(HWND hWnd, CDefView * pdsv);
} ;

void DV_UpdateStatusBar(CDefView *pdsv, BOOL fInitialize);

typedef struct {
    LPITEMIDLIST    pidl;
    UINT            uFlagsSelect;
} DVDelaySelItem;

HRESULT CreateEnumCViewList(CViewsList *pViews, IEnumSFVViews **ppObj);


// Compatibility with SFV_Message

class CBaseShellFolderViewCB : public IShellFolderViewCB, public CObjectWithSite
{
public:
    CBaseShellFolderViewCB(IUnknown *punkFolder, LPCITEMIDLIST pidl, LONG lEvents);
    STDMETHOD(RealMessage)(UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IShellFolderViewCB
    STDMETHODIMP MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    virtual ~CBaseShellFolderViewCB();

    HWND m_hwndMain;
    IShellFolder* m_pshf;

protected:
    LONG m_cRef;
    LPCITEMIDLIST m_pidl;
    LONG m_lEvents;
} ;

// Called CSHRegKey because ATL already has a class called CRegKey.

class CSHRegKey
{
public:
    CSHRegKey(HKEY hkParent, LPCTSTR pszSubKey, BOOL bCreate=FALSE)
    {
        DebugMsg(TF_LIFE, TEXT("ctor CSHRegKey(%s) %x"), pszSubKey, this);
        if ((bCreate ? RegCreateKey(hkParent, pszSubKey, &m_hk)
            : RegOpenKeyEx(hkParent, pszSubKey, 0, KEY_READ, &m_hk))!=ERROR_SUCCESS)
        {
            m_hk = NULL;
        }
    }
    CSHRegKey(HKEY hk) { DebugMsg(TF_LIFE, TEXT("ctor CSHRegKey %x"), this); m_hk=hk; }
    ~CSHRegKey()
    {
        DebugMsg(TF_LIFE, TEXT("dtor CSHRegKey %x"), this);
        if (m_hk) RegCloseKey(m_hk);
    }

    operator HKEY() const { return(m_hk); }
    operator !() const { return(m_hk==NULL); }

    HRESULT QueryValue(LPCTSTR szSub, LPTSTR pszVal, LONG cb)
        { return(SHRegQueryValue(m_hk, szSub, pszVal, &cb)); }

    HRESULT QueryValueEx(LPCTSTR szSub, LPBYTE pszVal, LONG cb)
        { return(SHQueryValueEx(m_hk, szSub, 0, NULL, pszVal, (LPDWORD)&cb)); }

private:
    HKEY m_hk;
};

#define HANDLE_SFVM_SUPPORTSIDENTITY(pv, wp, lP, fn) \
        ((fn)(pv))    /* Only 1 parameter */


// status bar helpers
STDAPI_(void) ResizeStatus(IUnknown *psite, UINT cx);
STDAPI_(void) InitializeStatus(IUnknown *psite);
STDAPI_(void) SetStatusText(IUnknown *psite, LPCTSTR *ppszText, int iStart, int iEnd);

#endif // _SFVIEWP_H_
