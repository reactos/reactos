//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       client.hxx
//
//  Contents:   CClient
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#ifndef __CLIENT_HXX__
#define __CLIENT_HXX__

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

#ifndef X_CDBASE_HXX_
#define X_CDBASE_HXX_
#include "cdbase.hxx"
#endif

#ifndef X_INTERNED_H_
#define X_INTERNED_H_
#include "internal.h"
#endif

// Forward declarations
class CHTMLAppFrame;
class CHTMLApp;

class CClient :
    public IOleClientSite,
    public IOleCommandTarget,
    public IOleDocumentSite,
    public IOleInPlaceSite,
    public IAdviseSink,
    public IServiceProvider,
    public IInternetSecurityManager,
    public IDocHostUIHandler,
    public IDocHostShowUI,
    public IHTMLOMWindowServices
{

public:
    CClient();
    ~CClient();

    HRESULT Create(REFCLSID clsid);
    HRESULT Load(IMoniker *pMk);
    HRESULT Show();
    void Resize();
    LRESULT DelegateMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    HRESULT SendCommand(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut);

    CHTMLAppFrame * Frame();
    
    // IUnknown methods
    //--------------------------------------------------------------
    DECLARE_SUBOBJECT_IUNKNOWN(CHTMLApp, HTMLApp)

    void Passivate();

    // IOleClientSite methods
    //--------------------------------------------------------------
    STDMETHOD(SaveObject)();
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR * ppmk);
    STDMETHOD(GetContainer)(LPOLECONTAINER FAR * ppContainer);
    STDMETHOD(ShowObject)();
    STDMETHOD(OnShowWindow)(BOOL fShow);
    STDMETHOD(RequestNewObjectLayout)();

    // IOleCommandTarget methods
    //--------------------------------------------------------------
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

    // IOleWindow methods
    //--------------------------------------------------------------
    STDMETHOD(GetWindow)(HWND FAR *lpHwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

    // IOleInPlaceSite methods
    //--------------------------------------------------------------
    STDMETHOD(CanInPlaceActivate)();
    STDMETHOD(OnInPlaceActivate)();
    STDMETHOD(OnUIActivate)();
    STDMETHOD(GetWindowContext)(
                LPOLEINPLACEFRAME FAR* lplpFrame,
                LPOLEINPLACEUIWINDOW FAR* lplpDoc,
                LPOLERECT lprcPosRect,
                LPOLERECT lprcClipRect,
                LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHOD(Scroll)(OLESIZE scrollExtent);
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
    STDMETHOD(OnInPlaceDeactivate)();
    STDMETHOD(DiscardUndoState)();
    STDMETHOD(DeactivateAndUndo)();
    STDMETHOD(OnPosRectChange)(LPCOLERECT lprcPosRect);
                
    // IOleDocumentSite methods
    //--------------------------------------------------------------
    STDMETHOD(ActivateMe)(IOleDocumentView * pViewToActivate);

    // IAdviseSink methods
    //--------------------------------------------------------------
    STDMETHOD_(void, OnDataChange)(FORMATETC * pFormatetc, STGMEDIUM * pmedium);
    STDMETHOD_(void, OnViewChange)(DWORD dwAspect, long lindex);
    STDMETHOD_(void, OnRename)(LPMONIKER pmk);
    STDMETHOD_(void, OnSave)();
    STDMETHOD_(void, OnClose)();

    // IServiceProvider methods
    //--------------------------------------------------------------
    STDMETHOD(QueryService)(REFGUID rguidService, REFIID riid, void ** ppvObj);

    // IInternetSecurityManager methods
    //--------------------------------------------------------------
    STDMETHOD(SetSecuritySite)(IInternetSecurityMgrSite *pSite );
    STDMETHOD(GetSecuritySite)(IInternetSecurityMgrSite **ppSite );
    STDMETHOD(MapUrlToZone)(LPCWSTR pwszUrl, DWORD * pdwZone, DWORD dwFlags);  
    STDMETHOD(GetSecurityId)(LPCWSTR pwszUrl, BYTE __RPC_FAR * pbSecurityId, 
                                DWORD __RPC_FAR * pcbSecurityId, DWORD_PTR dwReserved);
    STDMETHOD(ProcessUrlAction)(LPCWSTR pwszUrl, DWORD dwAction, BYTE * pPolicy,
                                DWORD cbPolicy, BYTE * pContext, DWORD cbContext,
                                DWORD dwFlags, DWORD dwReserved);
    STDMETHOD (QueryCustomPolicy)(LPCWSTR pwszUrl, REFGUID guidKey, BYTE ** ppPolicy,
                                    DWORD * pcbPolicy, BYTE * pContext, DWORD cbContext,
                                    DWORD dwReserved);
    STDMETHOD(SetZoneMapping)(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags);
    STDMETHOD(GetZoneMappings)(DWORD dwZone, IEnumString ** ppenumString, DWORD dwFlags);

    // IDocHostUIHandler methods
    //--------------------------------------------------------------
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
    STDMETHOD(ShowContextMenu)(DWORD dwID, POINT * pptPosition, IUnknown * pcmdtReserved, IDispatch * pDispatchObjectHit);
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
        {return S_FALSE;}
    STDMETHOD(GetOptionKeyPath)(LPOLESTR * ppchKey, DWORD dw)
        {return S_FALSE;}
    STDMETHOD(GetDropTarget)(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget)
        {return S_FALSE;}
    STDMETHOD(GetExternal)(IDispatch **ppDisp);
    STDMETHOD(TranslateUrl) (DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
        {return S_FALSE;}
    STDMETHOD(FilterDataObject) (IDataObject *pDO, IDataObject **ppDORet)
        {return S_FALSE;}
    
    // IDocHostShowUI methods
    //--------------------------------------------------------------
    STDMETHOD(ShowMessage)(HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption,
            DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT *plResult);
    STDMETHOD(ShowHelp)(HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand,
            DWORD dwData, POINT ptMouse, IDispatch *pDispatchObjectHit)
        { return S_FALSE; }  // BUGBUG: Eventually, this will fire a help event.

    // IHTMLOMWindowServices methods
    //--------------------------------------------------------------
    STDMETHOD(moveTo)(long x, long y);
    STDMETHOD(moveBy)(long x, long y);
    STDMETHOD(resizeTo)(long x, long y);
    STDMETHOD(resizeBy)(long x, long y);
    
private:
    HRESULT QueryObjectInterface(REFIID riid, void ** ppv);
    CHTMLApp * App();
    
public:
    IUnknown *          _pUnk;
    IOleObject *        _pIoo;
    IOleDocumentView *  _pView;
};

#endif // #define __CLIENT_HXX__


