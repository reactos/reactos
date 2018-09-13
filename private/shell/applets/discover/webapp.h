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

class CEventSink;

class CWebApp
{
    private:
        HINSTANCE       m_hInstance;        // application instance
        HWND            m_hwnd;             // window handle
        CContainer      *m_pContainer;      // container object
        IWebBrowser2    *m_pweb;            // IE4 IWebBrowser interface pointer

    public:
        CWebApp();

        BOOL Register(HINSTANCE hInstance);
        void InitializeData();
        void Create();
        void MessageLoop();

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // Window Messages
        void OnCreate(HWND hwnd);
        void OnDestroy();
        void OnSize(int width, int height);
        void OnKeyDown(UINT msg, WPARAM wParam, LPARAM lParam);
        void OnSetFocus(HWND hwndLose);
        void OnExit();
        void OnRelease();

        // Calls to IE4 WebOC
        void Navigate(BSTR url);
};

#endif