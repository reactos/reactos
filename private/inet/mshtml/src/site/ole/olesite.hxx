//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       olesite.hxx
//
//  Contents:   COleSite and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_OLESITE_HXX_
#define I_OLESITE_HXX_
#pragma INCMSG("--- Beg 'olesite.hxx'")

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#pragma INCMSG("--- Beg <prgsnk.h>")
#include <prgsnk.h>
#pragma INCMSG("--- End <prgsnk.h>")
#endif

#define _hxx_
#include "olesite.hdl"

#define VIEWSTATUS_SURFACE   0x10    // BUGBUG: define in ocidl.idl
#define VIEWSTATUS_3DSURFACE 0x20    // BUGBUG: define in ocidl.idl

// misc helper functions
HRESULT CLSIDFromHtmlString(TCHAR *pchClsid, CLSID *pclsid);


// forward references
interface ICursor;
class COleSite;
class CCodeLoad;
class CProgressBindStatusCallback;

class COleLayout;

MtExtern(OleCreateInfo)

//+---------------------------------------------------------------------------
//
//  Class:      COleSiteCPC (OSCPC)
//
//  Purpose:    Connection container for an OLE site, which forwards
//              the connections of its control.
//
//  Notes:      It is assumed that the last entry in the acpi array
//              is the one used for the aggregated control.
//
//----------------------------------------------------------------------------

class COleSiteCPC : public CConnectionPointContainer
{
typedef CConnectionPointContainer super;

public:
    COleSiteCPC(COleSite * pOleSite, IUnknown * pUnkPrivate);

    virtual CONNECTION_POINT_INFO *GetCPI() { return _acpi;}

    CONNECTION_POINT_INFO   _acpi[6];
};

//+---------------------------------------------------------------------------
//
//  Class:      COleSiteEventSink (OSES)
//
//  Purpose:    Evet sink for IDispatch events.
//
//----------------------------------------------------------------------------

class COleSiteEventSink : public CBaseEventSink
{
public:
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(COleSite)

    // IDispatch methods
    STDMETHOD(Invoke)(
            DISPID              dispidMember,
            REFIID              riid,
            LCID                lcid,
            WORD                wFlags,
            DISPPARAMS FAR*     pdispparams,
            VARIANT FAR*        pvarResult,
            EXCEPINFO FAR*      pexcepinfo,
            UINT FAR*           puArgErr);
};

//
//  Maximum number of calls to LockInPlaceActive that can be nested
//
#define MAX_LOCK_INPLACEACTIVE  15

//
//  ID to be used for the refresh timer (this is ued if we're hosting
//  trident directly, as opposed to through shdocvw
//
#define REFRESH_TIMER_ID (1)

class CPropertyBag;

MtExtern(COleSiteCLock)

//+---------------------------------------------------------------------------
//
//  Class:      COleSite (olesite)
//
//  Purpose:    Site object capable of containing OLE objects.
//
//----------------------------------------------------------------------------

class NOVTABLE COleSite : public CSite
{
    DECLARE_CLASS_TYPES(COleSite, CSite)

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:

    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)
    DECLARE_TEAROFF_TABLE(IServiceProvider)

    DECLARE_LAYOUT_FNS(COleLayout)

    //  Refresh Implementation
    NV_DECLARE_ONTICK_METHOD(RefreshCallback, refreshcallback, (UINT uTimerID));

    // IServiceProvider methods
    NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, LPVOID * ppv));

private:

    LPOLESTR _pstrRefreshURL;
    UINT     _iRefreshTime;
public:
    //
    // Methods used to create an OLE Site
    //

    COleSite(ELEMENT_TAG etag, CDoc *pDoc);   // normal constructor

    //
    // IPrivateUnknown members
    //

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    //
    // CBase methods
    //
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
        {
            if (pfExpando)
            {
                *pfExpando = Doc()->_fExpando;
            }
            if (!_pAtomTable)
            {
                _pAtomTable = new CAtomTable();
            }
            return _pAtomTable;
        }

    //
    // ISpecifyPropertyPages methods
    //
    STDMETHOD(GetPages)(CAUUID * pPages);

    //
    // IDispatchEx methods
    //

    NV_DECLARE_TEAROFF_METHOD(ContextThunk_InvokeEx, contextthunk_invokeex, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider) );

    STDMETHOD(GetDispID)(BSTR bstrName, DWORD grfdex, DISPID *pid);

    STDMETHOD(GetNextDispID)(
                DWORD grfdex,
                DISPID id,
                DISPID *prgid);

    STDMETHOD(GetMemberName)(DISPID id, BSTR *pbstrName);

    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk);

    STDMETHOD(ContextInvokeEx)(
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider,
            IUnknown *pUnkContext);

    HRESULT RemapActivexExpandoDispid(DISPID * pid);

    //
    // IProvideMultiClassInfo methods
    //

    HRESULT STDMETHODCALLTYPE GetClassInfo(ITypeInfo ** ppTI);
    HRESULT STDMETHODCALLTYPE GetGUID(DWORD dwGuidKind, GUID * pGUID);
    HRESULT STDMETHODCALLTYPE GetMultiTypeInfoCount(ULONG *pcti);
    HRESULT STDMETHODCALLTYPE GetInfoOfIndex(
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource);

    //
    // ISupportErrorInfo methods
    //

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID iid);

    enum
    {
        FSI_NONE            = 0,
        FSI_CLIENTSITESET   = 2,
        FSI_QUICKACTIVATE   = 4,
        FSI_PNSCONNECTED    = 8
    };

    //
    // Control creation
    //

    struct OLECREATEINFO
    {
        OLECREATEINFO()
            { memset(this, 0, sizeof(*this)); }
        ~OLECREATEINFO();

        CLSID               clsid;
        TCHAR *             pchSourceUrl;
        TCHAR *             pchDataUrl;
        DWORD               dwMajorVer;
        DWORD               dwMinorVer;
        TCHAR *             pchMimeType;
        TCHAR *             pchClassid;
        IStream *           pStream;
        IStorage *          pStorage;
        IPropertyBag *      pPropBag;
        IDataObject *       pDataObject;
        TCHAR *             pchFileName;
        IStream *           pStreamHistory;
        IBindCtx *          pBindCtxHistory;
        IUnknown *          pShortCutInfo;
    };

    HRESULT CreateObject(
        OLECREATEINFO * pinfo);
    virtual HRESULT CreateObjectNow (
        REFIID          piidObject,
        IUnknown *      punkObject,
        OLECREATEINFO * pinfo);
    HRESULT InstantiateObjectFromCF(
        IClassFactory * pCF,
        OLECREATEINFO * pinfo);
    HRESULT InstantiateObjectFromDataObject(
        IDataObject *   pDataObject);

    NV_DECLARE_ONCALL_METHOD(DeferredCreateObject, deferredcreateobject, (DWORD_PTR));

    virtual void AfterObjectInstantiation();

    //
    // Misc. helpers for creating/loading controls.
    //

    HRESULT InitNewObject();
    HRESULT LoadObject(IStream * pStm);
    HRESULT LoadObject(IStorage * pStg);
    HRESULT LoadObject(IPropertyBag * pPropBag);
    HRESULT LoadObject(TCHAR *pchDataUrl, TCHAR *pchFileName);
    HRESULT LoadHistoryStream(IStream *pStream, IBindCtx *pbc);
    HRESULT SaveHistoryStream(IStream *pStream);
    BOOL    SaveFormatSupported();
    HRESULT ConnectControl1(DWORD *pdwInitFlags);
    HRESULT ConnectControl2(DWORD *pdwInitFlags);
    BOOL    AllowCreate(REFCLSID clsid);

    HRESULT AssignWclsidFromControl();
    void    SetMiscStatusFlags(DWORD dwStatus);
    void    SetViewStatusFlags(DWORD dwStatus);

    // ICommandTarget methods

    HRESULT STDMETHODCALLTYPE QueryStatus(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    HRESULT STDMETHODCALLTYPE Exec(
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    // Init2 override

    virtual HRESULT Init2(CInit2Context * pContext);

    // Handle notification.
    //
    virtual void    Notify(CNotification *pNF);

    // Handle messages by bubbling out or in when necessary
    //
    virtual HRESULT BUGCALL HandleMessage(CMessage * pMessage);
    virtual void Passivate();
    void ReleaseObject();
    void EnterTree();
    
    void ClearRefresh();

    virtual HWND GetHwnd();

    virtual HRESULT GetNaturalExtent(DWORD dwMode, SIZEL *psizel);

    virtual HRESULT OnControlRequestEdit(DISPID dispid) { return S_OK; }
    virtual HRESULT OnControlChanged(DISPID dispid);
    void    OnControlReadyStateChanged(BOOL fForceComplete=FALSE);

    virtual HRESULT OnFailToCreate();
    virtual HRESULT PostLoad() { return S_OK; }
    
    BOOL GetBoolPropertyOnObject(DISPID dispid);
    virtual IsEnabled() { return GetBoolPropertyOnObject(DISPID_ENABLED); }
    virtual IsValid() { return GetBoolPropertyOnObject(DISPID_VALID); }

    HRESULT GetColors(CColorInfo *pCI);

    HRESULT GetReadyState(long *plReadyState);

#if DBG==1
    virtual void VerifyReadyState(long lReadyState) {}
#endif


    virtual HRESULT OnMnemonic(LPMSG lpmsg);
    virtual BOOL OnMenuEvent(int id, UINT code);

    virtual HRESULT GetControlInfo(CONTROLINFO *pCI);

    virtual HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    virtual HRESULT YieldCurrency(CElement *pElemNew);
    virtual void    YieldUI(CElement *pElemNew);
    virtual HRESULT BecomeUIActive();

#ifndef NO_MENU
    HRESULT OnInitMenuPopup(HMENU hmenu, int item, BOOL fSystemMenu);
#endif


    HRESULT OnInactiveMouseButtonMessage(CMessage *, LRESULT *plResult);
    HRESULT OnInactiveMousePtrMessage(CMessage *, CTreeNode * pNodeContext, LRESULT *plResult);

    // ProgSink function

    virtual DWORD   GetProgSinkClass() {return PROGSINK_CLASS_CONTROL;}
    virtual DWORD   GetProgSinkClassOther() {return PROGSINK_CLASS_OTHER;}

    //
    // Public Helper Functions
    //

    enum
    {
        // Byte 0 is COleSite state

        VALIDATE_ATTACHED = 1,
        VALIDATE_LOADED   = 2,
        VALIDATE_INPLACE  = 3,
        VALIDATE_WINDOWLESSINPLACE = 4,
        VALIDATE_NOTRENDERING = 5,

        // Byte 1 is CDoc state

        VALIDATE_DOC_ALIVE = (1 << 8),
        VALIDATE_DOC_SITE  = (2 << 8),
        VALIDATE_DOC_INPLACE = (3 << 8)
    };

    BOOL IllegalSiteCall(DWORD dwFlags);

    INSTANTCLASSINFO *GetInstantClassInfo();
    QUICKCLASSINFO * GetQuickClassInfo();
    CLASSINFO * GetClassInfo();
    IID * GetpIIDDispEvent();
    CLSID * GetpCLSID();

    IUnknown *  PunkCtrl() const { return _pUnkCtrl; }

    HRESULT SetClientSite(IOleClientSite *pClientSite);

    BOOL    IsSafeToInitialize(REFIID riid);
    BOOL    IsSafeToScript();
    BOOL    AccessAllowed(IDispatch *pDisp);
    BOOL    IsOleProxy();

    // What kind of critter is really running in this site
    enum OLESITE_TAG {
        OSTAG_ACTIVEX,  // ActiveX control
        OSTAG_APPLET,   // Java applet
        OSTAG_FRAME,    // Frame
        OSTAG_IFRAME,   // IFrame
        OSTAG_PLUGIN,   // Plug-in
        OSTAG_UNKNOWN   // who knows?
    };

    OLESITE_TAG OlesiteTag();

    HRESULT GetAmbientProp(DISPID dispid, VARIANT *pVarResult);

    HRESULT QueryControlInterface(REFIID iid, void **ppvObj);
    HRESULT QuerySafeLoadInterface(REFIID iid, void **ppvObj);

    void    CacheDispatch(void);   // Cache a pointer to the control's
                                   //   IDispatch implementation
    BOOL    IsVTableValid();        // get (and verify) default dual interface

    HRESULT GetProperty(UINT uVTableOffsetGet, DISPID dispidGet, VARTYPE vtType,
                        VARIANT* pVar);
    HRESULT SetProperty(UINT uVTableOffsetGet, DISPID dispidGet, VARTYPE vtType,
                        VARIANT* pVar);

    HRESULT SetHostNames(void);    // Pass our host names along to the control

    void    DoEmbedVerbs(USHORT usVerbIndex);
    BOOL    ActivationChangeRequiresRedraw();
    void    ReleaseCodeLoad();

    HRESULT IsClean();
    HRESULT SizeToFit();

    void RegisterForRelease();
    void UnregisterForRelease();
    
    HRESULT EnsureParamBag();
    HRESULT ReleaseParamBag();

    enum ExchangeParamBagDir
        { TOCONTROL, FROMCONTROL };

    HRESULT ExchangeParamBag(ExchangeParamBagDir dir);

    void    EnsurePrivateSink();

    HRESULT OnCommand(const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt=0,
                      VARIANTARG* pvarargIn=0, VARIANTARG* pvarargOut=0);

    HRESULT IE3XObjInvoke(
        DISPID       dispidMember,
        REFIID       riid,
        LCID         lcid,
        WORD         wFlags,
        DISPPARAMS * pdispparams,
        VARIANT *    pvarResult,
        EXCEPINFO *  pexcepinfo,
        UINT *       puArgErr);

#if DBG == 1
    ULONG GetulSSN() { return _ulSSN; }
#endif

    //
    // locks
    //

    enum OLESITELOCK_FLAG
    {
        OLESITELOCK_NONE            = 0,
        OLESITELOCK_TRANSITION      = 1 << 0,   // Block TransitionTo
        OLESITELOCK_SETEXTENT       = 1 << 1,   // Called IOleObject::SetExtent on ocx
        OLESITELOCK_INPLACEACTIVATE = 1 << 2,   // In the middle of DoVerb(INPLACE_ACTIVATE)
        OLESITELOCK_MAX             = LONG_MAX  // Force enum to 32 bits
    };

    class CLock
    {
    public:

        CLock(COleSite * pOleSite, OLESITELOCK_FLAG enumLockFlagsOleSite = OLESITELOCK_NONE)
        {
            _pOleSite = pOleSite;
            if (_pOleSite)
            {
                _wLockFlagsOleSite = _pOleSite->_wLockFlagsOleSite;
                _pOleSite->_wLockFlagsOleSite |= ((WORD)enumLockFlagsOleSite);
#if defined(UNIX)  && defined(ux10)
// IEUNIX: Workaround for a Reference problem in ColeSite.
//         See IEUNIX bug. 10448.
                _pOleSite->SubAddRef();
#else
                _pOleSite->PrivateAddRef();
#endif
            }
        }

        ~CLock()
        {
            if (_pOleSite)
            {
                _pOleSite->_wLockFlagsOleSite = _wLockFlagsOleSite;
#if defined(UNIX)  && defined(ux10)
// IEUNIX: Workaround for a Reference problem in ColeSite.
//         See IEUNIX bug. 10448.
                _pOleSite->SubRelease();
#else
                _pOleSite->PrivateRelease();
#endif
            }
        }

    private:
        COleSite *  _pOleSite;
        WORD        _wLockFlagsOleSite;
    };

    BOOL TestLock(OLESITELOCK_FLAG enumLockFlagsOleSite)
    {
        return _wLockFlagsOleSite & ((WORD)enumLockFlagsOleSite);
    }

    //
    // State transition and focus
    //

    //
    // State transition a`la OLE CD
    //   default implementation handles going in and out OS_UIACTIVE state
    //   should call OnUIActivate/OnUIDeactivate on form
    //-------------------------------------------------------------------------
    //  +override : for embedded objects to conform OLE2 spec
    //  -call super : no
    //  -call parent : no
    //  -call children : no
    //-------------------------------------------------------------------------
    HRESULT TransitionTo(OLE_SERVER_STATE state, LPMSG pMsg = NULL);

    //
    // Returns the OLE state this object should be in
    //-------------------------------------------------------------------------
    //  +override : when state behavior is more complex
    //  -call super : no
    //  -call parent : no
    //  +call children : yes
    //-------------------------------------------------------------------------
    OLE_SERVER_STATE BaselineState(OLE_SERVER_STATE osMax);

    OLE_SERVER_STATE State(void)
                { return (OLE_SERVER_STATE)_state; }

    //
    // Transitions this object to its proper baseline state,
    //   calls BaseLineState and TransitionTo
    //-------------------------------------------------------------------------
    //  -override : when state behavior is more complex
    //  -call super : no
    //  -call parent : no
    //  +call children : yes
    //-------------------------------------------------------------------------
    HRESULT TransitionToBaselineState(OLE_SERVER_STATE osMax);

    //
    // Transitions to baseline state unless UI_ACTIVE
    //-------------------------------------------------------------------------
    HRESULT TransitionToCorrectState();

    //
    // Helper function to inplace activate an object.
    //-------------------------------------------------------------------------
    HRESULT InPlaceActivate(IOleObject *pOleObject, HWND hwnd, LPMSG pMsg);

    static BOOL CheckDisplayAsDefault(CElement * pElem);

    //
    // Invoke IDispatch or vtable as appropriate
    //-------------------------------------------------------------------------
    enum VTBL_PROP {VTBL_PROPSET, VTBL_PROPGET};
    static HRESULT VTableDispatch (IDispatch *pDisp,
                                   VARTYPE propType,
                                   VTBL_PROP propDirection,
                                   void *pData,
                                   unsigned int uVTblOffset);

    #define _COleSite_
    #include "olesite.hdl"

    NV_STDMETHOD(attachEvent)(BSTR bstrEvent, IDispatch* pDisp, VARIANT_BOOL *pResult);
    NV_STDMETHOD(detachEvent)(BSTR bstrEvent, IDispatch *pDisp);

    //+---------------------------------------------------------------------------
    //
    //  Class:      COleSite::CClient
    //
    //  Synopsis:   The true client site.
    //
    //  Notes:      Do not add data members to this class.
    //
    //              Because we don't derive from this class, we do not declare
    //              methods as virtual by using the STDMETHOD macro.  This makes
    //              the tearoff interface methods more efficient.
    //
    //----------------------------------------------------------------------------

    class CClient :
        public CVoid,
        public IOleClientSite,
        public IAdviseSinkEx,
        public IPropertyNotifySink
    {
        DECLARE_CLASS_TYPES(CClient, CVoid)

    public:
        // Tear off interfaces
        DECLARE_TEAROFF_TABLE(IOleInPlaceSiteWindowless)
        DECLARE_TEAROFF_TABLE(IOleControlSite)
        DECLARE_TEAROFF_TABLE(IServiceProvider)
        DECLARE_TEAROFF_TABLE(IOleCommandTarget)
        DECLARE_TEAROFF_TABLE(IDispatch)
        DECLARE_TEAROFF_TABLE(IBindHost)
#ifndef NO_DATABINDING
        DECLARE_TEAROFF_TABLE(IBoundObjectSite)
#endif // ndef NO_DATABINDING
        DECLARE_TEAROFF_TABLE(ISecureUrlHost)

#ifdef WIN16
        static ULONG STDMETHODCALLTYPE privateaddref(CClient *pObj)
        { return pObj->AddRef(); }
        static ULONG STDMETHODCALLTYPE privaterelease(CClient *pObj)
        { return pObj->Release(); }
        static HRESULT STDMETHODCALLTYPE privatequeryinterface(CClient *pObj, REFIID iid, void **ppvObj)
        { return pObj->QueryInterface(iid, ppvObj); }
#endif
        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IOleClientSite methods
        HRESULT STDMETHODCALLTYPE SaveObject();
        HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign,
                DWORD dwWhichMoniker,
                LPMONIKER FAR* ppmk);
        HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER FAR* ppContainer);
        HRESULT STDMETHODCALLTYPE ShowObject();
        HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL fShow);
        HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();

        // IOleWindow methods
        NV_DECLARE_TEAROFF_METHOD(GetWindow, getwindow, (HWND FAR* lphwnd));
        NV_DECLARE_TEAROFF_METHOD(ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode));

        // IOleInPlaceSite methods
        NV_DECLARE_TEAROFF_METHOD(CanInPlaceActivate, caninplaceactivate, ());
        NV_DECLARE_TEAROFF_METHOD(OnInPlaceActivate, oninplaceactivate, ());
        NV_DECLARE_TEAROFF_METHOD(OnUIActivate, onuiactivate, ());
        NV_DECLARE_TEAROFF_METHOD(GetWindowContext, getwindowcontext, (
                    LPOLEINPLACEFRAME FAR* lplpFrame,
                    LPOLEINPLACEUIWINDOW FAR* lplpDoc,
                    LPOLERECT lprcPosRect,
                    LPOLERECT lprcClipRect,
                    LPOLEINPLACEFRAMEINFO lpFrameInfo));
        NV_DECLARE_TEAROFF_METHOD(Scroll, scroll, (OLESIZE scrollExtent));
        NV_DECLARE_TEAROFF_METHOD(OnUIDeactivate, onuideactivate, (BOOL fUndoable));
        NV_DECLARE_TEAROFF_METHOD(OnInPlaceDeactivate, oninplacedeactivate, ());
        NV_DECLARE_TEAROFF_METHOD(DiscardUndoState, discardundostate, ());
        NV_DECLARE_TEAROFF_METHOD(DeactivateAndUndo, deactivateandundo, ());
        NV_DECLARE_TEAROFF_METHOD(OnPosRectChange, onposrectchange, (LPCOLERECT lprcPosRect));

        // IOleInPlaceSiteEx methods
        NV_DECLARE_TEAROFF_METHOD(OnInPlaceActivateEx, oninplaceactivateex, (BOOL *, DWORD));
        NV_DECLARE_TEAROFF_METHOD(OnInPlaceDeactivateEx, oninplacedeactivateex, (BOOL));
        NV_DECLARE_TEAROFF_METHOD(RequestUIActivate, requestuiactivate, ());

        // IOleInPlaceSiteWindowless methods
        NV_DECLARE_TEAROFF_METHOD(CanWindowlessActivate, canwindowlessactivate, ());
        NV_DECLARE_TEAROFF_METHOD(GetCapture, getcapture, ());
        NV_DECLARE_TEAROFF_METHOD(SetCapture, setcapture, (BOOL fCapture));
        NV_DECLARE_TEAROFF_METHOD(SetFocus, setfocus, (BOOL fFocus));
        NV_DECLARE_TEAROFF_METHOD(GetFocus, getfocus, ());
        NV_DECLARE_TEAROFF_METHOD(OnDefWindowMessage, ondefwindowmessage, (UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult));
        NV_DECLARE_TEAROFF_METHOD(GetDC, getdc, (LPCRECT prc, DWORD dwFlags, HDC * phDC));
        NV_DECLARE_TEAROFF_METHOD(ReleaseDC, releasedc, (HDC hDC));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRect, invalidaterect, (LPCRECT prc, BOOL fErase));
        NV_DECLARE_TEAROFF_METHOD(InvalidateRgn, invalidatergn, (HRGN hrgn, BOOL fErase));
        NV_DECLARE_TEAROFF_METHOD(ScrollRect, scrollrect, (int dx, int dy, LPCRECT lprcScroll, LPCRECT lprcClip));
        NV_DECLARE_TEAROFF_METHOD(AdjustRect, adjustrect, (LPRECT prc));

        // IAdviseSink methods
        void STDMETHODCALLTYPE OnDataChange(
                FORMATETC FAR* pFormatetc,
                STGMEDIUM FAR* pStgmed);
        void STDMETHODCALLTYPE OnViewChange(
                DWORD dwAspect, LONG lindex);
        void STDMETHODCALLTYPE OnRename(LPMONIKER pmk);
        void STDMETHODCALLTYPE OnSave();
        void STDMETHODCALLTYPE OnClose();

        // IAdviseSink2 methods
        void STDMETHODCALLTYPE OnLinkSrcChange(IMoniker * pmk);

        // IAdviseSinkEx methods
        void STDMETHODCALLTYPE OnViewStatusChange(DWORD dwViewStatus);

        //  IDispatch methods
        NV_DECLARE_TEAROFF_METHOD(GetTypeInfoCount , gettypeinfocount , ( UINT * pctinfo ));
        NV_DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
                REFIID                riid,
                LPTSTR *              rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID *              rgdispid));
        NV_DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo, (UINT,ULONG, ITypeInfo**));
        NV_DECLARE_TEAROFF_METHOD(Invoke, invoke, (
                DISPID dispidMember,
                REFIID riid,
                LCID lcid,
                WORD wFlags,
                DISPPARAMS FAR* pdispparams,
                VARIANT FAR* pvarResult,
                EXCEPINFO FAR* pexcepinfo,
                UINT FAR* puArgErr));

        //  IOleControlSite methods
        NV_DECLARE_TEAROFF_METHOD(OnControlInfoChanged, oncontrolinfochanged, ());
        NV_DECLARE_TEAROFF_METHOD(LockInPlaceActive, lockinplaceactive, (BOOL fLock));
        NV_DECLARE_TEAROFF_METHOD(GetExtendedControl, getextendedcontrol, (IDispatch **ppDisp));
        NV_DECLARE_TEAROFF_METHOD(TransformCoords, transformcoords, (POINTL *pPtlHiMetric,
                POINTF *pPtfContainer,
                DWORD dwFlags));
        NV_DECLARE_TEAROFF_METHOD(TranslateAccelerator, translateaccelerator, (LPMSG pmsg, DWORD grfModifiers));
        NV_DECLARE_TEAROFF_METHOD(OnFocus, onfocus, (BOOL fGotFocus));
        NV_DECLARE_TEAROFF_METHOD(ShowPropertyFrame, showpropertyframe, ());

        //  IServiceProvider methods
        NV_DECLARE_TEAROFF_METHOD(QueryService, queryservice, (REFGUID guidService, REFIID iid, void ** ppv));

        //  IOleCommandTarget methods
        NV_DECLARE_TEAROFF_METHOD(QueryStatus, querystatus, (
                GUID * pguidCmdGroup,
                ULONG cCmds,
                MSOCMD rgCmds[],
                MSOCMDTEXT * pcmdtext));
        NV_DECLARE_TEAROFF_METHOD(Exec, exec, (
                GUID * pguidCmdGroup,
                DWORD nCmdID,
                DWORD nCmdexecopt,
                VARIANTARG * pvarargIn,
                VARIANTARG * pvarargOut));

#ifndef NO_DATABINDING
        // IBoundObjectSite methods
        NV_DECLARE_TEAROFF_METHOD(GetCursor, getcursor, (
                    DISPID dispid,
                    ICursor **ppCursor,
                    LPVOID FAR* ppcidOut));
#endif // ndef NO_DATABINDING

        // IPropertyNotifySink methods

        HRESULT STDMETHODCALLTYPE OnChanged(DISPID dispid);
        HRESULT STDMETHODCALLTYPE OnRequestEdit(DISPID dispid);

        //
        // IBindHost Methods
        //

        NV_DECLARE_TEAROFF_METHOD(CreateMoniker, createmoniker,
            (LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved));
        NV_DECLARE_TEAROFF_METHOD(MonikerBindToStorage, monikerbindtostorage,
            (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));
        NV_DECLARE_TEAROFF_METHOD(MonikerBindToObject, monikerbindtoobject,
            (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));

        //
        // ISecureUrlHost Methods
        //

        NV_DECLARE_TEAROFF_METHOD(ValidateSecureUrl, validatesecureurl,
            (BOOL* fAllow, OLECHAR* pchUrlInQuestion, DWORD dwFlags));

        NV_DECLARE_ONCALL_METHOD(DeferredOnPosRectChange, deferredonposrectchange, (DWORD_PTR));

        RECT *  _prcPending;

        // Helper functions
        CDoc *      Doc();
        COleSite *  MyOleSite();

        #if DBG==1
        ULONG       _ulRefsLocal;
        #endif

    };

    // This helper provides the functionality for both CObjectElement's and
    // CPluginSite's get_BaseHref() method:
    HRESULT GetBaseHref(BSTR *pbstr);


    HRESULT GetInterfaceProperty(UINT uGetOffset, DISPID dispid, REFIID riid,
                                IUnknown** ppunk);
public:
    friend class CClient;
    friend class CObject;
    friend class COleSitePNS;
    friend class COleSiteCPC;
    friend class COleSiteEventSink;
    friend class CHtmObjectContext;

    //+-----------------------------------------------------------------------
    //
    //  Member data
    //
    //------------------------------------------------------------------------

    CClient _Client;

    WORD                _wLockFlagsOleSite;     // lock flags

    //
    // In order to minimize alignment padding, we need to order the members
    // by size, which precludes ordering by section (persistent or transient).
    // So, the following commenting syntax is used to indicate persistent or
    // transient members: [P] - Persistent member, [T] - Transient member
    //
    DWORD               _dwPropertyNotifyCookie;
    IUnknown            * _pUnkCtrl;   // [T] - The control at this site
    IDispatch           * _pDisp;      // [T] - pointer to ctrl's IDispatch
    IViewObject         * _pVO;        // [T] - pointer to ctrl's IViewObject

    IOleInPlaceObject   * _pInPlaceObject;  // [T] - cached ptr while _pObj is
                                            //       InPlace

    CPropertyBag        * _pParamBag;  // [T] - lives only during load/save
                                       //       procedures

    DWORD_PTR           _dwEventSinkIn;  // [T] - Event sink on the
                                       //       aggregated control.
    DWORD               _dwProgCookie; // [T] - Cookie for progress sink
    LONG                _lReadyState;  // [T] - Tracked control ready state
    IUnknown *          _pUnkCtrlES;   // [T] - Exposed event sink for
                                       //       primary dispinterface on ctrl
#ifndef WIN16
    CCodeLoad *      _pCodeLoad; // [T] - Code download context
#endif
    
    CAtomTable          * _pAtomTable; // [T] - atom table for <Object> tag this
                                       //       allow the max number of expando
                                       //       objects for ActiveX controls.

    WORD                _wclsid;            // word class identifier.
    IStream *           _pStreamHistory;
    IUnknown *          _pUnkPrintDelegate; // IUnkown for printing


    // -----------
    //
    // All bitfields are transient.
    //
    // The compiler always allocates a DWORD-aligned DWORD for any bitfields
    // contained in a struct, so any smaller datatypes (short, byte, etc.)
    // should follow the bitfields.
    //
    unsigned    _fDispatchCached : 1;       // 0  have we attempted to QI for the control's IDispatch?
    unsigned    _fVTableCached : 1;         // 1  have we attempted to get vtable interface?
    unsigned    _fSetHostNames : 1;         // 2  have we set the control's host names?
    unsigned    _fUseViewObjectEx : 1;      // 3  _pVO == &IViewObjectEx
    unsigned    _fWindowlessInplace : 1;    // 4  true if in windowless inplace state.
    unsigned    _fInsideOut : 1;            // 5  Set for OLEMISC_INSIDEOUT
    unsigned    _fAlwaysRun : 1;            // 6  Set for OLEMISC_ALWAYSRUN
    unsigned    _fActivateWhenVisible : 1;  // 7  Set for OLEMISC_ACTIVATEWHENVISIBLE
    unsigned    _fHidden : 1;               // 8  Deactivate after drag drop.
    unsigned    _fDVAspectOpaque : 1;       // 9  does ctrl support DVASPECT_OPAQUE?
    unsigned    _fDVAspectTransparent : 1;  // 10 does ctrl support DVASPECT_TRANSPARENT?
    unsigned    _fFailedToCreate: 1;        // 11 Code download failed.
    unsigned    _fUseInPlaceObjectWindowless : 1;// 12 supports IOleInPlaceObjectWindowless
    unsigned    _fKnowSafeToScript : 1;     // 13 Do we know if ocx is safetoscript?
    unsigned    _fDirty : 1;                // 14 Is this site dirty?
    unsigned    _fDeactivateOnMouseExit : 1;// 15
    unsigned    _fActsLikeLabel : 1;        // 16 does site act like a label?
    unsigned    _fEatsReturn : 1;           // 17 does object eat returns?
    unsigned    _state : 3;                 // 18-20 OLE_SERVER_STATE
    unsigned    _cLockInPlaceActive : 4;    // 21-24 protect window from destruction
    unsigned    _fObjAvailable : 1;         // 25 TRUE if control is available
    unsigned    _fSafeToScript : 1;         // 26 TRUE if ocx is safe to script
    unsigned    _fXAggregate : 1;           // 27 TRUE if we're aggregating the ocx
    unsigned    _fCanDoShColorsChange : 1;   // 28 TRUE if the site knows about SHDVID_ONCOLORSCHANGE
    unsigned    _fDataSameDomain : 1;       // 29 TRUE if the DATA attrib points to the same domain as the doc.
    unsigned    _fRegisteredForRelease : 1; // 30 TRUE if registered to get SN_RELEASEOBJECT notifications
    unsigned    _fObjectReleased : 1;       // 31 TRUE if OLE object has been released.
    unsigned    _fInBecomeCurrent : 1;      // 33 TRUE if this is in the scope of a BecomeCurrent call on myself
    //+-----------------------------------------------------------------------
    //
    //  Class descriptor
    //
    //------------------------------------------------------------------------

    static const CLSID *            s_apclsidPages[];
#define CPI_OFFSET_OLESITECONTROL   5
};


//+---------------------------------------------------------------------------
//
//  Class:      CProgressBindStatusCallback
//
//  Synopsis:   Privides an IBindStatusCallback interface which will
//              forward progress notifications on to the CDoc so it can
//              update the progress bars.
//
//              This class is created in the IBindHost methods and registered
//              with the IBindCtx that gets used during the bind operation.
//
//              Instances of this class live only while the bind is underway.
//
//              Beware: some of the RegisterBindStatusCallback() docs say
//              that it will multiplex notifications.  This is false.  We
//              must forward notifications to the previously registered callback
//              ourself.
//
//----------------------------------------------------------------------------

MtExtern(CProgressBindStatusCallback)

class CProgressBindStatusCallback : public IBindStatusCallback, public IServiceProvider
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CProgressBindStatusCallback))

    // ctor/dtor

    CProgressBindStatusCallback();
    ~CProgressBindStatusCallback();

    // IUnknown

    DECLARE_FORMS_STANDARD_IUNKNOWN(CProgressBindStatusCallback);

    // IServiceProvider methods
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID iid, void ** ppv);


    // IBindStatusCallback methods

    STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pib);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(OnLowResource)(DWORD dwReserved);
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)(DWORD *grfBINDF, BINDINFO *pbindinfo);
    STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM  *pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);

    // Misc.

    HRESULT Init(
        CDoc *pDoc,
        DWORD dwCompatFlags,
        IBindStatusCallback *pBSCChain,
        IBindCtx *pbctx);
    void    Terminate();

    class CLock
    {
    public:
        CLock(CProgressBindStatusCallback *pBSC);
        ~CLock();

        CProgressBindStatusCallback *   _pBSC;
    };

    // Data members

    DWORD                _dwCompatFlags;    // Compatibility flags
    DWORD                _dwProgCookie;     // Cookie from progress loader
    ULONG                _dwBindf;          // from pdoc->_pHtmLoadCtx
    IProgSink           *_pProgSink;        // The progsink
    IBindStatusCallback *_pBSCChain;        // Chained bindstatuscallback to call
    IBindCtx            *_pBCtx;            // Bind ctx for operation.
    IBinding            *_pBinding;         // Binding for operation.
    CDoc                *_pDoc;             // Ptr to owner doc.
    BOOL                 _fAbort;           // TRUE if Abort was called
};

//+---------------------------------------------------------------------------
//
//  COleSite inline methods
//
//----------------------------------------------------------------------------

inline COleSite *
COleSite::CClient::MyOleSite()
{
    return CONTAINING_RECORD(this, COleSite, _Client);
};

inline CDoc *
COleSite::CClient::Doc()
{
    return MyOleSite()->Doc();
}


inline INSTANTCLASSINFO *
COleSite::GetInstantClassInfo()
{
    return Doc()->_clsTab.GetInstantClassInfo(_wclsid);
}

inline QUICKCLASSINFO *
COleSite::GetQuickClassInfo()
{
    return Doc()->_clsTab.GetQuickClassInfo(_wclsid, _pUnkCtrl);
}

inline CLASSINFO *
COleSite::GetClassInfo()
{
    return Doc()->_clsTab.GetClassInfo(_wclsid, _pUnkCtrl);
}

inline IID *
COleSite::GetpIIDDispEvent()
{
    QUICKCLASSINFO* pQCI = Doc()->_clsTab.GetQuickClassInfo(_wclsid, _pUnkCtrl);
    if (pQCI)
    {
        return &pQCI->iidDispEvent;
    }
    else
    {
        return NULL;
    }
}

inline CLSID *
COleSite::GetpCLSID()
{
    return Doc()->_clsTab.GetpCLSID(_wclsid);
}


//+---------------------------------------------------------------------------
//
// This macro provides trace dumps of all OLE interface calls.  We use the
// THR() macro within it to get the failure simulation & other features of
// THR().
//
//----------------------------------------------------------------------------
#if DBG == 1

#define THR_OLE(x)              (TraceOLE(THR(x), FALSE, #x, __FILE__, __LINE__,this))
#define THR_OLEO(x,olesite)     (TraceOLE(THR(x), FALSE, #x, __FILE__, __LINE__,(olesite)))

#else // #if DBG == 1

#define THR_OLE(x)              (x)
#define THR_OLEO(x,olesite)     (x)

#endif // #if DBG == 1

#pragma INCMSG("--- End 'olesite.hxx'")
#else
#pragma INCMSG("*** Dup 'olesite.hxx'")
#endif
