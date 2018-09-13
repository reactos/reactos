//////////////////////////////////////////////////////////////////////////
//
//  webapp.h
//
//      The purpose of this application is to demonstrate how a C++
//      application can host IE4, and manipulate the OC vtable.
//
//      This file contains the specification of the WebApp class.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#ifndef _WEBAPP_H_
#define _WEBAPP_H_

#include <exdisp.h>
#include <mshtml.h>
#include "container.h"
#include "inidata.h"

class CEventSink;

class CWebApp
{
    private:
        HINSTANCE       m_hInstance;        // application instance
        HWND            m_hwnd;             // window handle
        CContainer      *m_pContainer;      // container object
        IWebBrowser2    *m_pweb;            // IE4 IWebBrowser interface pointer
        DWORD           m_eventCookie;      // unique id for event wiring
        CEventSink      *m_pEvent;          // event object
        CIniData        m_IniData;          // info from ini and registry about display items
        bool            m_bVirgin;          // true until after we inject the menu items.  Prevents multiple injection.
        bool            m_bUseDA;           // true if we should use DirectAnimation.  This will be set to false if you
                                            // have a 16 color (or less) display (or if the no-animations poicy is set?).

    public:
        CWebApp();

        void Register(HINSTANCE hInstance);
        void InitializeData();
        void Create();
        void MessageLoop();

    public:
        // Event callbacks
        void eventBeforeNavigate2(VARIANT* URL, VARIANT_BOOL* Cancel);
        void eventDocumentComplete(VARIANT* URL);

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // Window Messages
        void OnCreate(HWND hwnd);
        void OnDestroy();
        void OnSize(int width, int height);
        void OnKeyDown(UINT msg, WPARAM wParam, LPARAM lParam);
        void OnSetFocus(HWND hwndLose);

        // Calls to IE4 WebOC
        void Navigate(BSTR url);

        // Private helper APIs
        IConnectionPoint * GetConnectionPoint(REFIID riid);
        void ConnectEvents();
        void DisconnectEvents();
        HRESULT FindIDInDoc(LPCTSTR szItemName, IHTMLElement ** ppElement );
        HRESULT SetBaseDirHack();
        void SetRunState();
};

class CEventSink : public IDispatch
{
    private:
        ULONG       m_cRefs;        // ref count
        CWebApp     *m_pApp;       // reference to web app object

    public:
        CEventSink(CWebApp *webapp);

    public:
        // *** IUnknown Methods ***
        STDMETHOD(QueryInterface)(REFIID riid, PVOID *ppvObject);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        // *** IDispatch Methods ***
        STDMETHOD (GetIDsOfNames)(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid);
        STDMETHOD (GetTypeInfo)(unsigned int itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo);
        STDMETHOD (GetTypeInfoCount)(unsigned int FAR * pctinfo);
        STDMETHOD (Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr);
};

#endif