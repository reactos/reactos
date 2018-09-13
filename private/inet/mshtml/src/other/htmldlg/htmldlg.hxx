//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       htmldlg.hxx
//
//  Contents:   Definitions for the Trident html dialogs
//
//  History:    06-14-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#ifndef I_HTMLDLG_HXX_
#define I_HTMLDLG_HXX_
#pragma INCMSG("--- Beg 'htmldlg.hxx'")

#ifndef X_OTHRDISP_H_
#define X_OTHRDISP_H_
#include "othrdisp.h"
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for ITargetFrame, ITargetEmbedding
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"   // for IDocHostUIHandler
#endif

#ifndef X_SAFEOCX_H_
#define X_SAFEOCX_H_
#include <safeocx.h> // for IActiveXSafetyProvider
#endif

#define HTMLDLGINFO_RESIZEOFF       0
#define HTMLDLGINFO_RESIZEON        WS_THICKFRAME


#define _hxx_
#include "htmldlg.hdl"

interface IHTMLDocument2;
EXTERN_C const GUID DIID_HTMLDocumentEvents;

// Needs this for the IDocument interface
#ifndef X_DOCUMENT_H_
#define X_DOCUMENT_H_
#include <document.h>
#endif

// Needs this for the IElement interface
#ifndef X_ELEMENT_H_
#define X_ELEMENT_H_
#include <element.h>
#endif

//
// Forward decls
//

class CDynamicCF;
class CHTMLDlg;
class CHTMLDlgSite;
class CHTMLDlgFrame;
class CHTMLPropPageCF;
class CHTMLDlgExtender;
class CHTMLDlgModel;
class CHTMLDlgSink;
class CCommitEngine;
class CElement;


//+---------------------------------------------------------------------------
//
//  Class:      CHTMLPropPageCF
//
//+---------------------------------------------------------------------------

MtExtern(CHTMLPropPageCF)

class CHTMLPropPageCF : public CDynamicCF
{
    typedef CDynamicCF super;
    
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHTMLPropPageCF))
    CHTMLPropPageCF(IMoniker * pmk);
    ~CHTMLPropPageCF();
       
    // IClassFactory methods
    STDMETHOD(CreateInstance)(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj);    

private:   
    IMoniker  * _pmk;
};


//+---------------------------------------------------------------------------
//
//  Class:      CHTMLDlgExtender
//
//  Purpose:    Events sink for IPropertyNotifySink + extender
//
//----------------------------------------------------------------------------

MtExtern(CHTMLDlgExtender)

class CHTMLDlgExtender : public IPropertyNotifySink
{
public:
    // constructor / destructor
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHTMLDlgExtender))
    CHTMLDlgExtender(CHTMLDlg * pDlg, IHTMLElement * pHTMLElement, DISPID dispid);
    ~CHTMLDlgExtender();
    
    // IUnknown
    DECLARE_FORMS_STANDARD_IUNKNOWN(CHTMLDlgExtender);

    // IPropertyNotifySink methods
    STDMETHOD(OnChanged)  (DISPID dispid);
    STDMETHOD(OnRequestEdit) (DISPID dispid);

    // misc helpers
    HRESULT Value_PropPageToObject ();
    HRESULT Value_ObjectToPropPage ();

    enum EXCHANGEVALUEBY
    {
        EXCHANGEVALUEBY_VALUE,
        EXCHANGEVALUEBY_INNERTEXT
    };

    // Data members
    CHTMLDlg *      _pDlg;
    DISPID          _dispid;
    IHTMLElement *  _pHTMLElement;
    DWORD           _dwCookie;
    EXCHANGEVALUEBY _ExchangeValueBy;
};


//+------------------------------------------------------------------------
//
//  Class:      CHTMLDlgSite
//
//  Purpose:    OLE/Document Site for embedded object
//
//-------------------------------------------------------------------------

//
// marka - CHTMLDlgSite now implements an IInternetSecurityManager
// this is to BYPASS the normal Security settings for a trusted HTML Dialog in a "clean" way
//
// see more detailed comment in DlgSite.cxx
//
//

class CHTMLDlgSite :
        public IOleClientSite,
        public IOleInPlaceSite,
        public IOleControlSite,
        public IDispatch,
        public IServiceProvider,
        public ITargetFrame,
        public ITargetFrame2,
        public IDocHostUIHandler,
        public IOleCommandTarget,
        public IInternetSecurityManager
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLDlg, HTMLDlg)

    // IOleClientSite methods
    STDMETHOD(SaveObject)               (void);
    STDMETHOD(GetMoniker)               (
                DWORD dwAssign, 
                DWORD dwWhichMoniker,
                LPMONIKER FAR* ppmk);
    STDMETHOD(GetContainer)             (LPOLECONTAINER FAR* ppContainer);
    STDMETHOD(ShowObject)               (void);
    STDMETHOD(OnShowWindow)             (BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)   (void);

    // IOleWindow methods
    STDMETHOD(GetWindow)                (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp)     (BOOL fEnterMode);

    // IOleInPlaceSite methods
    STDMETHOD(CanInPlaceActivate)       (void);
    STDMETHOD(OnInPlaceActivate)        (void);
    STDMETHOD(OnUIActivate)             (void);
    STDMETHOD(GetWindowContext)         (
                LPOLEINPLACEFRAME FAR *     lplpFrame,
                LPOLEINPLACEUIWINDOW FAR *  lplpDoc,
                LPOLERECT                   lprcPosRect,
                LPOLERECT                   lprcClipRect,
                LPOLEINPLACEFRAMEINFO       lpFrameInfo);
    STDMETHOD(Scroll)                   (OLESIZE scrollExtent);
    STDMETHOD(OnUIDeactivate)           (BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)      (void);
    STDMETHOD(DiscardUndoState)         (void);
    STDMETHOD(DeactivateAndUndo)        (void);
    STDMETHOD(OnPosRectChange)          (LPCOLERECT lprcPosRect);

    // IOleControlSite methods
    STDMETHOD(OnControlInfoChanged)     (void);
    STDMETHOD(LockInPlaceActive)        (BOOL fLock);
    STDMETHOD(GetExtendedControl)       (IDispatch **);
    STDMETHOD(TransformCoords)          (
                POINTL* pptlHimetric,
                POINTF* pptfContainer,
                DWORD dwFlags);
    STDMETHOD(TranslateAccelerator)     (LPMSG lpmsg, DWORD grfModifiers);
    STDMETHOD(OnFocus)                  (BOOL fGotFocus);
    STDMETHOD(ShowPropertyFrame)        (void);

    // IDispatch methods
    STDMETHOD(GetTypeInfoCount)         (UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)              (
                UINT itinfo, 
                LCID lcid, 
                ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)            (
                REFIID                riid,
                LPOLESTR *            rgszNames,
                UINT                  cNames,
                LCID                  lcid,
                DISPID FAR*           rgdispid);
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    // IServiceProvider methods
    STDMETHOD(QueryService)             (
                REFGUID sid, 
                REFIID iid, 
                LPVOID * ppv);

    // IInternetSecurityManager methods
    STDMETHOD ( SetSecuritySite ) ( IInternetSecurityMgrSite *pSite );
    STDMETHOD ( GetSecuritySite ) ( IInternetSecurityMgrSite **ppSite );
    STDMETHOD( MapUrlToZone ) (
                        LPCWSTR     pwszUrl,
                        DWORD*      pdwZone,
                        DWORD       dwFlags
                    );  
    STDMETHOD( GetSecurityId ) ( 
            /* [in] */ LPCWSTR pwszUrl,
            /* [size_is][out] */ BYTE __RPC_FAR *pbSecurityId,
            /* [out][in] */ DWORD __RPC_FAR *pcbSecurityId,
            /* [in] */ DWORD_PTR dwReserved);
    STDMETHOD( ProcessUrlAction) (
                        LPCWSTR     pwszUrl,
                        DWORD       dwAction,
                        BYTE*   pPolicy,    // output buffer pointer
                        DWORD   cbPolicy,   // output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwFlags,    // See enum PUAF for details.
                        DWORD   dwReserved);
    STDMETHOD ( QueryCustomPolicy )  (
                        LPCWSTR     pwszUrl,
                        REFGUID     guidKey,
                        BYTE**  ppPolicy,   // pointer to output buffer pointer
                        DWORD*  pcbPolicy,  // pointer to output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwReserved );
    STDMETHOD( SetZoneMapping )  (
                        DWORD   dwZone,        // absolute zone index
                        LPCWSTR lpszPattern,   // URL pattern with limited wildcarding
                        DWORD   dwFlags       // add, change, delete
    );
    STDMETHOD( GetZoneMappings ) (
                        DWORD   dwZone,        // absolute zone index
                        IEnumString  **ppenumString,   // output buffer size
                        DWORD   dwFlags        // reserved, pass 0
    );
    
    //  ITargetFrame methods
    STDMETHOD(SetFrameName)(LPCWSTR pszFrameName)
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameName)(LPWSTR *ppszFrameName)                 
        {return E_NOTIMPL;}
    STDMETHOD(GetParentFrame)(IUnknown **ppunkParent)
        { *ppunkParent = NULL; return S_OK;}
    STDMETHOD(FindFrame)(
        LPCWSTR pszTargetName, 
        IUnknown *ppunkContextFrame, 
        DWORD dwFlags, 
        IUnknown **ppunkTargetFrame) 
        {return E_NOTIMPL;}
    STDMETHOD(SetFrameSrc)(LPCWSTR pszFrameSrc)                    
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameSrc)(LPWSTR *ppszFrameSrc)                   
        {return E_NOTIMPL;}
    STDMETHOD(GetFramesContainer)(IOleContainer **ppContainer)
        {return E_NOTIMPL;}
    STDMETHOD(SetFrameOptions)(DWORD dwFlags)                      
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameOptions)(DWORD *pdwFlags);
    STDMETHOD(SetFrameMargins)(DWORD dwWidth, DWORD dwHeight)      
        {return E_NOTIMPL;}
    STDMETHOD(GetFrameMargins)(DWORD *pdwWidth, DWORD *pdwHeight)  
        {return E_NOTIMPL;}
    STDMETHOD(RemoteNavigate)(ULONG cLength, ULONG *pulData)       
        {return E_NOTIMPL;}
    STDMETHOD(OnChildFrameActivate)(IUnknown *pUnkChildFrame)      
        {return E_NOTIMPL;}
    STDMETHOD(OnChildFrameDeactivate)(IUnknown *pUnkChildFrame)    
        {return E_NOTIMPL;}

    //  ITargetFrame2 methods (those not in ITargetFrame)
    STDMETHOD(FindFrame)(LPCWSTR pszTargetName, DWORD dwFlags, IUnknown **ppunkTargetFrame)  {return E_NOTIMPL;}
    STDMETHOD(GetTargetAlias)(LPCWSTR pszTargetName, LPWSTR * ppszTargetAlias)               {return E_NOTIMPL;}

    // IDocHostUIHandler methods
    STDMETHOD(GetHostInfo)(DOCHOSTUIINFO * pInfo);
    STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject * pActiveObject, IOleCommandTarget * pCommandTarget, IOleInPlaceFrame * pFrame, IOleInPlaceUIWindow * pDoc)
        {return S_OK;}
    STDMETHOD(HideUI) (void)
        {return S_OK;}
    STDMETHOD(UpdateUI) (void)
        {return S_OK;}
    STDMETHOD(EnableModeless)(BOOL fEnable)
        {return S_OK;}
    STDMETHOD(OnDocWindowActivate)(BOOL fActivate)
        {return S_OK;}
    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate)
        {return S_OK;}
    STDMETHOD(ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fRameWindow)
        {return S_OK;}
    STDMETHOD(ShowContextMenu)(DWORD dwID, POINT * pptPosition, IUnknown * pcmdtReserved, IDispatch * pDispatchObjectHit)
        {return S_FALSE;}
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
        {return S_FALSE;}
    STDMETHOD(GetOptionKeyPath)(LPOLESTR * ppchKey, DWORD dw)
        {return S_FALSE;}
    STDMETHOD(GetDropTarget)(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget)
        {return S_FALSE;}
    STDMETHOD(GetExternal)(IDispatch **ppDisp);
    STDMETHOD(TranslateUrl) (DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    STDMETHOD(FilterDataObject) (IDataObject *pDO, IDataObject **ppDORet);

    // IOleCommandTarget methods
    STDMETHOD(QueryStatus)(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec)(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);
    
};


//+------------------------------------------------------------------------
//
//  Class:      CHTMLDlgFrame
//
//  Purpose:    OLE Frame for active object
//
//-------------------------------------------------------------------------

MtExtern(CHTMLDlgFrame)

class CHTMLDlgFrame : public IOleInPlaceFrame
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHTMLDlgFrame))
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLDlg, HTMLDlg)

    // IOleWindow methods
    STDMETHOD(GetWindow)                (HWND FAR* lphwnd);
    STDMETHOD(ContextSensitiveHelp)     (BOOL fEnterMode);

    // IOleInPlaceUIWindow methods
    STDMETHOD(GetBorder)                (LPOLERECT lprectBorder);
    STDMETHOD(RequestBorderSpace)       (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetBorderSpace)           (LPCBORDERWIDTHS lpborderwidths);
    STDMETHOD(SetActiveObject)          (
                LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
                LPCTSTR                   lpszObjName);

    // IOleInPlaceFrame methods
    STDMETHOD(InsertMenus)              (
                HMENU hmenuShared, 
                LPOLEMENUGROUPWIDTHS 
                lpMenuWidths);
    STDMETHOD(SetMenu)                  (
                HMENU hmenuShared, 
                HOLEMENU holemenu, 
                HWND hwndActiveObject);
    STDMETHOD(RemoveMenus)              (HMENU hmenuShared);
    STDMETHOD(SetStatusText)            (LPCTSTR lpszStatusText);
    STDMETHOD(EnableModeless)           (BOOL fEnable);
    STDMETHOD(TranslateAccelerator)     (LPMSG lpmsg, WORD wID);
};


//+------------------------------------------------------------------------
//
//  Class:      CHTMLDlgSink
//
//  Purpose:    Dispatch event sink for aggregated document
//
//-------------------------------------------------------------------------

class CHTMLDlgSink : public CBaseEventSink
{
typedef CBaseEventSink super;

private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLDlg, HTMLDlg)

    // IDispatch methods
    STDMETHOD(Invoke)                   (
                DISPID          dispidMember,
                REFIID          riid,
                LCID            lcid,
                WORD            wFlags,
                DISPPARAMS *    pdispparams,
                VARIANT *       pvarResult,
                EXCEPINFO *     pexcepinfo,
                UINT *          puArgErr);

    
};
//+------------------------------------------------------------------------
//
//  Class:      CDlgDocPNS
//
//  Purpose:    Events sink for IPropertyNotifySink. for OM property sharing
//    with the document. similer to DlgExtender above
//
//-------------------------------------------------------------------------

class CDlgDocPNS : public IPropertyNotifySink
{
private:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))
public:
    DECLARE_FORMS_SUBOBJECT_IUNKNOWN(CHTMLDlg)

    // IPropertyNotifySink methods

    STDMETHOD(OnChanged)  (DISPID dispid);
    STDMETHOD(OnRequestEdit) (DISPID dispid) { return(S_OK); }

    // Data members

    DWORD        _dwCookie;
    CHTMLDlg *   _pDlg;

};

//+------------------------------------------------------------------------
//
//  Struct:     HTMLDLGINFO
//
//-------------------------------------------------------------------------

struct HTMLDLGINFO
{
    HTMLDLGINFO::HTMLDLGINFO()
        { memset(this, 0, sizeof(*this)); }

    IMoniker *  pmk;                    // Moniker to load from
    HWND        hwndParent;
    BOOL        fPropPage;              // true = coming from proppage
};

//+------------------------------------------------------------------------
//
//  Class:      CHTMLDlg
//
//  Purpose:    The xobject of the aggregated object
//
//-------------------------------------------------------------------------

MtExtern(CHTMLDlg)

class CHTMLDlg :
        public CBase,
        public IPropertyPage2
{
    DECLARE_CLASS_TYPES(CHTMLDlg, CBase)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHTMLDlg))

    CHTMLDlg(IUnknown *pUnkOuter, BOOL fTrusted, IUnknown *pUnkHost);
    ~CHTMLDlg();

    static HRESULT  CreateHTMLDlgIndirect(
        IUnknown *pUnkOuter,
        HTMLDLGINFO *pdlginfo,
        REFIID riid,
        void **ppv);
                
    // CBase methods
    DECLARE_PLAIN_IUNKNOWN(CHTMLDlg)

    virtual const CBase::CLASSDESC *GetClassDesc() const
        { return &s_classdesc;}
    virtual void                Passivate();

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IDispatch methods
    NV_DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
            REFIID riid,
            LPOLESTR * rgszNames,
            UINT cNames,
            LCID lcid,
            DISPID * rgdispid));

    NV_DECLARE_TEAROFF_METHOD(Invoke, invoke,    (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr));

    // IDispatchEx methods
    HRESULT STDMETHODCALLTYPE GetDispID(
            BSTR bstrName,
            DWORD grfdex,
            DISPID *pid);
  
    HRESULT STDMETHODCALLTYPE InvokeEx(
        DISPID dispidMember,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS * pdispparams,
        VARIANT * pvarResult,
        EXCEPINFO * pexcepinfo,
        IServiceProvider *pSrvProvider);

    HRESULT STDMETHODCALLTYPE GetNextDispID(
            DWORD grfdex,
            DISPID id,
            DISPID *prgid);

    // IPropertyPage methods
    STDMETHOD(SetPageSite)              (IPropertyPageSite *pPageSite);
    STDMETHOD(Activate)                 (
                HWND hwndParent,
                LPCRECT prc,
                BOOL fModal);
    STDMETHOD(Deactivate)               ();
    STDMETHOD(GetPageInfo)              (PROPPAGEINFO *ppageinfo);
    STDMETHOD(SetObjects)               (ULONG cUnk, IUnknown **ppUnk);
    STDMETHOD(Show)                     (UINT nCmdShow);
    STDMETHOD(Move)                     (LPCRECT prc);
    STDMETHOD(IsPageDirty)              ();
    STDMETHOD(Apply)                    ();
    STDMETHOD(Help)                     (LPCOLESTR bstrHelpDir);
    STDMETHOD(TranslateAccelerator)     (LPMSG lpmsg);

    // IPropertyPage2 methods
    STDMETHOD(EditProperty)             (DISPID dispid);

    // Other
    HRESULT         Create(HTMLDLGINFO *pdlginfo, TCHAR * pchOptions = NULL);
    void            GetViewRect(RECT *prc);
    static  LRESULT CALLBACK HTMLDlgWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);
    LRESULT         OnDestroy();
    LRESULT         OnActivate(WORD wFlags);
    LRESULT         OnWindowPosChanged(RECT *prc);
    LRESULT         OnClose();
    HRESULT         GetDoc (IHTMLDocument2 ** ppHTMLDoc);
    void            SetDirty(DWORD dw);
    HRESULT         OnPropertyChange(CHTMLDlgExtender *pXtend);
    HRESULT         InitValues();
    HRESULT         UpdateValues();
    HRESULT         ConnectElement(IHTMLElement * pHTMLElement, BSTR bstrID);
    HRESULT         GetDocumentTitle(BSTR *pbstr);
    HRESULT         OnPropertyChange(DISPID dispid, DWORD dwFlags);
    void            MoveStatusWindow();
    void            DividePartsInStatusWindow();
    void            OnReadyStateChange();
    HRESULT         LoadDocSynchronous(IStream *pStm, TCHAR *pchUrl);
    HRESULT         SetTrustedOnDoc(HTMLDLGINFO * pdlginfo);
    HRESULT         EnsureLayout();
    HRESULT         Terminate ();


    // property Helper Functions
    long            GetTop();
    long            GetLeft();
    long            GetWidth();
    long            GetHeight();
    CElement *      GetHTML();
    long            GetFontSize(CElement *pElem = NULL);
    void            VerifyDialogRect( RECT * pRect, HWND hwndRef);

    static HRESULT  ParseOptions (HTMLDLGINFO * pDlgInfo, TCHAR * pchOptions);
    
    // Zone info helper function
    void CacheZonesIcons();

    #define _CHTMLDlg_
    #include "htmldlg.hdl"

    // Subobjects
    CHTMLDlgSite                    _Site;
    CHTMLDlgFrame                   _Frame;
    CDlgDocPNS                      _PNS;
    
    // Member variables
    IUnknown *                      _pUnkObj;
    IOleObject *                    _pOleObj;
    IOleInPlaceObject *             _pInPlaceObj;
    IOleInPlaceActiveObject *       _pInPlaceActiveObj;
    IPropertyPageSite *             _pPageSite;
    IServiceProvider *              _pHostServiceProvider;

    // Data members
    HWND                            _hwnd;
    HWND                            _hwndStatus;
    HWND                            _hwndTopParent;
    RECT                            _rcView;
    HINSTANCE                       _hInst;
    CCommitEngine *                 _pEngine;
    CCommitHolder *                 _pHolder;
    LCID                            _lcid;
    long                            _lDefaultFontSize;
    HTMLDlgEdge                     _enumEdge;
    HTMLDlgFlag                     _enumfScroll;
    HTMLDlgFlag                     _enumfMin;
    HTMLDlgFlag                     _enumfMax;
    HTMLDlgFlag                     _enumfHelp;
    HTMLDlgFlag                     _enumfStatus;
    HTMLDlgFlag                     _enumfResizeable;
    
    // obj model data members
    CVariant                        _varArgIn;
    CVariant                        _varRetVal;

    CPtrAry<IDispatch *>            _aryDispObjs;
    CPtrAry<CHTMLDlgExtender *>     _aryXObjs;
  
    DWORD                           _dwFrameOptions;
    DWORD                           _dwWS;
    DWORD                           _dwWSEx;

    // Flags
    unsigned                        _fActive:1;  // Activate has been called
    unsigned                        _fPropPageMode:1;
    unsigned                        _fDirty:1;
    unsigned                        _fInitializing:1;
    unsigned                        _fTrusted:1; // Trusted if called from applications hosting Trident
                                                 // UnTrusted if called from scripts 
    unsigned                        _fKeepHidden:1; // don't do ::ShowWindow()
    unsigned                        _fAutoExit:1;   // auto exit after script is run
    unsigned                        _fInteractive:1;// Received READYSTATE_INTERACTIVE
                                                    //   (only in dialog mode)
    unsigned                        _fIsModeless:1; // is this a modeless dialog
    
    // Static members
    static ATOM                         s_atomWndClass;
    static const CLASSDESC              s_classdesc;
};

EXTERN_C const GUID CLSID_HTMLDocument;
class CDoc;

//=============================================================
//
//  Class : CThreadDialogProcParam - is responsible for getting data 
//      transfered from the primary thread (where the modeless
//      was invoked from) to the helper thread that the modeless
//      is actually running on.
//
//=============================================================
class CThreadDialogProcParam : public IHTMLModelessInit
{
public:
    CThreadDialogProcParam(IMoniker * pmk,
                           VARIANT  * pvarArgIn=NULL);
    ~CThreadDialogProcParam ();

    // IUnknown
    //---------------------------------
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)
    { 
        return PrivateQueryInterface(iid, ppv); 
    }
    STDMETHOD_(ULONG, AddRef) (void)
    {    
        _ulRefs++;    
        return _ulRefs; 
    }
    STDMETHOD_(ULONG, Release) (void)
    {
        ULONG ulRefs = --_ulRefs;

        if (ulRefs == 0)
            delete this;

        return ulRefs;
    }

    //IDispatch
    //---------------------------------------------------
    STDMETHODIMP         GetTypeInfoCount(UINT *pctinfo)
        {    return S_OK; };
    STDMETHODIMP         GetTypeInfo(UINT iTInfo, LCID lcid,
                                     ITypeInfo **ppTInfo)
        {    return S_OK; };
    
    STDMETHODIMP         GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                                       UINT cNames, LCID lcid,
                                       DISPID *rgDispId)
        {    return S_OK; };
    
    STDMETHODIMP         Invoke(DISPID dispIdMember,
                                REFIID riid,
                                LCID lcid,
                                WORD wFlags,
                                DISPPARAMS *pDispParams,
                                VARIANT *pVarResult,
                                EXCEPINFO *pExcepInfo,
                                UINT *puArgErr)
        {    return S_OK; };


    // IHTMLModelessInit
    //----------------------------------------------------
    #define _CThreadDialogProcParam_
    #include "htmldlg.hdl"


    //Helper Functions
    //----------------------------------------------------
    STDMETHOD(PrivateQueryInterface) (REFIID iid, LPVOID * ppv);

    // Member variables
    //-------------------------------
    ULONG           _ulRefs;
    CVariant        _varParam;   
    CVariant        _varOptions; 
    IMoniker      * _pmk;
    CDoc           *_pParentDoc;
};


#pragma INCMSG("--- End 'htmldlg.hxx'")
#else
#pragma INCMSG("*** Dup 'htmldlg.hxx'")
#endif
