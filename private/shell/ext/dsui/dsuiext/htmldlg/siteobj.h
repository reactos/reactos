//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      siteobj.h
//
//  Contents:  CSiteObj class definition
//
//  History:   22-Jan-97 EricB      Created
//
//-----------------------------------------------------------------------------

#ifndef _SITE_OBJ_H
#define _SITE_OBJ_H

#include "view.h"
#include "evtsink.h"

class CSite;
class CInputEventSink;
 
typedef CSite * LPSITE;

//+----------------------------------------------------------------------------
//
//  Class:  CClientSite
//
//-----------------------------------------------------------------------------
class CClientSite : public IOleClientSite
{
protected:

#ifdef _DEBUG
    char szClass[16];
#endif

    ULONG   m_cRef;
    LPSITE  m_pSite;

public:
    CClientSite(LPSITE);
    ~CClientSite(void);

    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP SaveObject(void);
    STDMETHODIMP GetMoniker(DWORD, DWORD, LPMONIKER *);
    STDMETHODIMP GetContainer(LPOLECONTAINER *);
    STDMETHODIMP ShowObject(void);
    STDMETHODIMP OnShowWindow(BOOL);
    STDMETHODIMP RequestNewObjectLayout(void);
};

typedef CClientSite * LPCLIENTSITE;

//+----------------------------------------------------------------------------
//
//  Class:  CAdviseSink
//
//-----------------------------------------------------------------------------
class CAdviseSink : public IAdviseSink
{
protected:

#ifdef _DEBUG
    char szClass[16];
#endif

    ULONG       m_cRef;
    LPSITE      m_pSite;

public:
    CAdviseSink(LPSITE);
    ~CAdviseSink(void);

    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP_(void)  OnDataChange(LPFORMATETC, LPSTGMEDIUM);
    STDMETHODIMP_(void)  OnViewChange(DWORD, LONG);
    STDMETHODIMP_(void)  OnRename(LPMONIKER);
    STDMETHODIMP_(void)  OnSave(void);
    STDMETHODIMP_(void)  OnClose(void);
};

typedef CAdviseSink * LPPROPADVISESINK;

//+----------------------------------------------------------------------------
//
//  Class:  CInPlaceSite
//
//-----------------------------------------------------------------------------
class CInPlaceSite : public IOleInPlaceSite
{
protected:

#ifdef _DEBUG
    char szClass[16];
#endif

    ULONG           m_cRef;
    LPSITE          m_pSite;

public:
    CInPlaceSite(LPSITE);
    ~CInPlaceSite(void);

    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IOleWindow methods    
    STDMETHODIMP GetWindow(HWND *);
    STDMETHODIMP ContextSensitiveHelp(BOOL);
    // CInPlaceSite methods
    STDMETHODIMP CanInPlaceActivate(void);
    STDMETHODIMP OnInPlaceActivate(void);
    STDMETHODIMP OnUIActivate(void);
    STDMETHODIMP GetWindowContext(LPOLEINPLACEFRAME *,
                                  LPOLEINPLACEUIWINDOW *, LPRECT, LPRECT,
                                  LPOLEINPLACEFRAMEINFO);
    STDMETHODIMP Scroll(SIZE);
    STDMETHODIMP OnUIDeactivate(BOOL);
    STDMETHODIMP OnInPlaceDeactivate(void);
    STDMETHODIMP DiscardUndoState(void);
    STDMETHODIMP DeactivateAndUndo(void);
    STDMETHODIMP OnPosRectChange(LPCRECT);
};

typedef CInPlaceSite * LPINPLACESITE;

//+----------------------------------------------------------------------------
//
//  Class:  CDocSite
//
//-----------------------------------------------------------------------------
class CDocSite : public IOleDocumentSite
{
protected:

#ifdef _DEBUG
    char szClass[16];
#endif

    ULONG           m_cRef;
    LPSITE          m_pSite;

public:
    CDocSite(LPSITE);
    ~CDocSite(void);

    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP ActivateMe(IOleDocumentView *);
};

typedef CDocSite * LPDOCSITE;

//+----------------------------------------------------------------------------
// Trident Advanced host interfaces
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Class:  CDocHostUiHandler
//
//-----------------------------------------------------------------------------
class CDocHostUiHandler : public IDocHostUIHandler
{
protected:

#ifdef _DEBUG
    char szClass[20];
#endif

    ULONG       m_cRef;
    LPSITE      m_pSite;

public:
    CDocHostUiHandler(LPSITE);
    ~CDocHostUiHandler(void);

    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP GetHostInfo(DOCHOSTUIINFO * pInfo);
    STDMETHODIMP ShowUI(DWORD dwID, 
                        IOleInPlaceActiveObject * pActiveObject,
                        IOleCommandTarget * pCommandTarget,
                        IOleInPlaceFrame * pFrame,
                        IOleInPlaceUIWindow * pDoc);
    STDMETHODIMP HideUI(void);
    STDMETHODIMP UpdateUI(void);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP OnDocWindowActivate(BOOL fActivate);
    STDMETHODIMP OnFrameWindowActivate(BOOL fActivate);
    STDMETHODIMP ResizeBorder(LPCRECT prcBorder, 
                              IOleInPlaceUIWindow * pUIWindow, 
                              BOOL fRameWindow);
    STDMETHODIMP ShowContextMenu(DWORD dwID, POINT * pptPosition,
                                 IUnknown* pCommandTarget,
                                 IDispatch * pDispatchObjectHit);
    STDMETHODIMP TranslateAccelerator(LPMSG lpMsg,
                                      const GUID __RPC_FAR *pguidCmdGroup,
                                      DWORD nCmdID);
    STDMETHODIMP GetOptionKeyPath(BSTR* pbstrKey, ULONG cchstrKey);
    STDMETHODIMP GetDropTarget(IDropTarget __RPC_FAR *pDropTarget,
                               IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget);
    STDMETHODIMP GetExternal(IDispatch **pDispath);
    STDMETHODIMP TranslateUrl(DWORD dwTranslate, OLECHAR *pStr, OLECHAR **ppStr);
    STDMETHODIMP FilterDataObject(IDataObject *pObj, IDataObject **ppObj);
};

typedef CDocHostUiHandler * LPDOCUIHDLR;

//+----------------------------------------------------------------------------
//
//  Class:  CDosHostShowUi
//
//-----------------------------------------------------------------------------
class CDosHostShowUi : public IDocHostShowUI
{
protected:

#ifdef _DEBUG
    char szClass[16];
#endif

    ULONG       m_cRef;
    LPSITE      m_pSite;

public:
    CDosHostShowUi(LPSITE);
    ~CDosHostShowUi(void);

    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP ShowMessage(HWND hwnd, LPOLESTR lpstrText,
                             LPOLESTR lpstrCaption, DWORD dwType,
                             LPOLESTR lpstrHelpFile, DWORD dwHelpContext,
                             LRESULT * plResult);
    STDMETHODIMP ShowHelp(HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand,
                          DWORD dwData, POINT ptMouse,
                          IDispatch * pDispatchObjectHit);
};

typedef CDosHostShowUi * LPDHSHOWUI;

//+----------------------------------------------------------------------------
//
//  Class:  CSite
//
//-----------------------------------------------------------------------------
class CSite : public IServiceProvider
{
friend CInputEventSink;

public:

#ifdef _DEBUG
    char szClass[16];
#endif

    CSite(HWND hWnd, LPPROPVIEW pView);
    ~CSite();

    // IUnknown members
    STDMETHOD(QueryInterface)(REFIID riid,void FAR* FAR* ppv);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IServiceProvider methods
    STDMETHOD(QueryService)(REFGUID guid, REFIID iid, LPVOID * ppv);

    // Other functions
    BOOL        Create(LPCTSTR pchPath);
    void        Close(void);
    void        Activate(LONG);
    void        UpdateObjectRects(void);
    HRESULT     Load(LPCTSTR pchPath);

    // helper functions
    LPOLEINPLACEOBJECT  GetIPObject(void) {return m_pIOleIPObject;}
    LPINPLACESITE       GetIPSite(void) {return m_IPSite;}
    LPOLECOMMANDTARGET  GetCommandTarget(void) {return m_pIOleCommandTarget;}
    LPPROPVIEW  GetFrame(void) {return m_pView;}
    LPUNKNOWN   GetObjectUnknown(void){return m_pObj;} 
    void        SetCommandTarget(LPOLECOMMANDTARGET pTarget)
                                            {m_pIOleCommandTarget = pTarget;}
    void        SetDocView(LPOLEDOCUMENTVIEW pDocView)
                                                {m_pIOleDocView = pDocView;}
    void        SetIPObject(LPOLEINPLACEOBJECT pIPObject)
                                                {m_pIOleIPObject = pIPObject;}
    HWND        GetWindow(void) {return m_hWnd;}
    BOOL        ConnectSink();

private:
    BOOL        DisconnectSink();
    BOOL        GetTypeLib(void);

    //Object interfaces
    LPUNKNOWN           m_pObj;
    LPOLEOBJECT         m_pIOleObject;
    LPOLEINPLACEOBJECT  m_pIOleIPObject;
    LPOLEDOCUMENTVIEW   m_pIOleDocView;
    LPOLECOMMANDTARGET  m_pIOleCommandTarget;
    LPTYPEINFO          m_pTypeInfo;

    //Our interfaces
    LPCLIENTSITE        m_pClientSite;
    LPPROPADVISESINK    m_pAdviseSink;
    LPINPLACESITE       m_IPSite;
    LPDOCSITE           m_pDocSite;
    LPDOCUIHDLR         m_pDocHostUIHandler;
    LPDHSHOWUI          m_pDocHostShowUi;
    CInputEventSink   * m_pEventSink;

    // Attributes
    BOOL                m_bVisible;
    ULONG               m_cRef;

    HWND                m_hWnd;     // Client area window of parent
    DWORD               m_rgCookie[100];    // Advise cookie BUGBUG: make into a dynamic array
    DWORD               m_cCookieArrayElements;
    LPPROPVIEW          m_pView;
};

//DeleteInterfaceImp calls 'delete' and NULLs the pointer
#define DeleteInterfaceImp(p)\
{\
            if (NULL!=p)\
            {\
                delete p;\
                p=NULL;\
            }\
}


//ReleaseInterface calls 'Release' and NULLs the pointer
#define ReleaseInterface(p)\
{\
            IUnknown *pt=(IUnknown *)p;\
            p=NULL;\
            if (NULL!=pt)\
                pt->Release();\
}

#endif
