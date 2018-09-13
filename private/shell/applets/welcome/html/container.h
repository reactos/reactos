//////////////////////////////////////////////////////////////////////////
//
//  container.h
//
//      This file contains the complete class specification of an ActiveX
//      control container. This purpose of this container is to test
//      a single control being hosted.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef _CONTAINER_H_
#define _CONTAINER_H_

#include <ocidl.h>
#include <mshtmhst.h>

class CContainer :  public IOleClientSite, 
                    public IOleInPlaceSite,
                    public IOleInPlaceFrame,
                    public IOleControlSite,
                    public IDispatch,
                    public IDocHostUIHandler
{
    private:
        ULONG       m_cRefs;        // ref count
        HWND        m_hwnd;         // window handle of the container
        HWND        m_hwndStatus;   // status window handle
        IUnknown    *m_punk;        // IUnknown of contained object
        RECT        m_rect;         // size of control

    public:
        CContainer();
        ~CContainer();

    public:
        // *** IUnknown Methods ***
        STDMETHOD(QueryInterface)(REFIID riid, PVOID *ppvObject);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        // *** IOleClientSite Methods ***
        STDMETHOD (SaveObject)();
        STDMETHOD (GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER *ppMk);
        STDMETHOD (GetContainer)(LPOLECONTAINER *ppContainer);
        STDMETHOD (ShowObject)();
        STDMETHOD (OnShowWindow)(BOOL fShow);
        STDMETHOD (RequestNewObjectLayout)();

        // *** IOleWindow Methods ***
        STDMETHOD (GetWindow) (HWND * phwnd);
        STDMETHOD (ContextSensitiveHelp) (BOOL fEnterMode);

        // *** IOleInPlaceSite Methods ***
        STDMETHOD (CanInPlaceActivate) (void);
        STDMETHOD (OnInPlaceActivate) (void);
        STDMETHOD (OnUIActivate) (void);
        STDMETHOD (GetWindowContext) (IOleInPlaceFrame ** ppFrame, IOleInPlaceUIWindow ** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo);
        STDMETHOD (Scroll) (SIZE scrollExtent);
        STDMETHOD (OnUIDeactivate) (BOOL fUndoable);
        STDMETHOD (OnInPlaceDeactivate) (void);
        STDMETHOD (DiscardUndoState) (void);
        STDMETHOD (DeactivateAndUndo) (void);
        STDMETHOD (OnPosRectChange) (LPCRECT lprcPosRect);

        // *** IOleInPlaceUIWindow Methods ***
        STDMETHOD (GetBorder)(LPRECT lprectBorder);
        STDMETHOD (RequestBorderSpace)(LPCBORDERWIDTHS lpborderwidths);
        STDMETHOD (SetBorderSpace)(LPCBORDERWIDTHS lpborderwidths);
        STDMETHOD (SetActiveObject)(IOleInPlaceActiveObject * pActiveObject,
                                    LPCOLESTR lpszObjName);

        // *** IOleInPlaceFrame Methods ***
        STDMETHOD (InsertMenus)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
        STDMETHOD (SetMenu)(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
        STDMETHOD (RemoveMenus)(HMENU hmenuShared);
        STDMETHOD (SetStatusText)(LPCOLESTR pszStatusText);
        STDMETHOD (EnableModeless)(BOOL fEnable);
        STDMETHOD (TranslateAccelerator)(LPMSG lpmsg, WORD wID);

        // *** IOleControlSite Methods ***
        STDMETHOD (OnControlInfoChanged)(void);
        STDMETHOD (LockInPlaceActive)(BOOL fLock);
        STDMETHOD (GetExtendedControl)(IDispatch **ppDisp);
        STDMETHOD (TransformCoords)(POINTL *pptlHimetric, POINTF *pptfContainer, DWORD dwFlags);
        STDMETHOD (TranslateAccelerator)(LPMSG pMsg, DWORD grfModifiers);
        STDMETHOD (OnFocus)(BOOL fGotFocus);
        STDMETHOD (ShowPropertyFrame)(void);

        // *** IDispatch Methods ***
        STDMETHOD (GetIDsOfNames)(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid);
        STDMETHOD (GetTypeInfo)(unsigned int itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo);
        STDMETHOD (GetTypeInfoCount)(unsigned int FAR * pctinfo);
        STDMETHOD (Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr);

        // *** IDocHostUIHandler Methods ***
        STDMETHOD (ShowContextMenu)(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
        STDMETHOD (GetHostInfo)(DOCHOSTUIINFO *pInfo);
        STDMETHOD (ShowUI)(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
        STDMETHOD (HideUI)(void);
        STDMETHOD (UpdateUI)(void);
        //STDMETHOD (EnableModeless)(BOOL fEnable);
        STDMETHOD (OnDocWindowActivate)(BOOL fEnable);
        STDMETHOD (OnFrameWindowActivate)(BOOL fEnable);
        STDMETHOD (ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
        STDMETHOD (TranslateAccelerator)(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
        STDMETHOD (GetOptionKeyPath)(LPOLESTR *pchKey, DWORD dw);
        STDMETHOD (GetDropTarget)(IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
        STDMETHOD (GetExternal)(IDispatch **ppDispatch);
        STDMETHOD (TranslateUrl)(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
        STDMETHOD (FilterDataObject)(IDataObject *pDO, IDataObject **ppDORet);

    public:
        void add(BSTR clsid);
        void remove();
        void setParent(HWND hwndParent);
        void setLocation(int x, int y, int width, int height);
        void setVisible(BOOL fVisible);
        void setFocus(BOOL fFocus);
        void setStatusWindow(HWND hwndStatus);
        void translateKey(MSG msg);
        IDispatch *getDispatch();
        IUnknown * getUnknown();
};

#endif