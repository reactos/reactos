//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       pages.hxx
//
//  Contents:   property pages for provider, domain/workgroup, server, share
//
//  History:    26-Sep-95        BruceFo     Created
//
//--------------------------------------------------------------------------

#ifndef __PAGES_HXX__
#define __PAGES_HXX__

class CNetObj;

class CPage
{
    DECLARE_SIG;

public:

    //
    // Main page dialog procedure: static
    //

    static
    INT_PTR CALLBACK
    DlgProcPage(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
        );

    //
    // constructor, destructor, 2nd phase constructor
    //

    CPage(
        IN HWND hwndPage,
        IN LPARAM lParam
        );

    ~CPage();

    HRESULT
    InitInstance(
        VOID
        );

private:

    //
    // Main page dialog procedure: non-static
    //

    INT_PTR
    _PageProc(
        IN HWND hWnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Window messages and notifications
    //

    BOOL
    _OnInitDialog(
        IN HWND hwnd,
        IN HWND hwndFocus,
        IN LPARAM lInitParam
        );

    BOOL
    _OnNotify(
        IN HWND hwnd,
        IN int idCtrl,
        IN LPNMHDR phdr
        );

    //
    // Other helper methods
    //

    HWND
    _GetFrameWindow(
        VOID
        )
    {
        return GetParent(_hwndPage);
    }

    BOOL
    _OnInitNetwork(
        IN HWND hwnd,
        IN LPNETRESOURCE pnr
        );

    BOOL
    _OnInitDomain(
        IN HWND hwnd,
        IN LPNETRESOURCE pnr
        );

    BOOL
    _OnInitServerOrShare(
        IN HWND hwnd,
        IN LPNETRESOURCE pnr,
        IN BOOL fServer
        );

    VOID
    _SetServerType(
        HWND hwnd,
        int idControl,
        PSERVER_INFO_101 pinfo
        );

    //
    // Private class variables
    //

    HWND        _hwndPage;          // HWND to the property page
    CNetObj*    _pNetObj;

};

#endif  // __PAGES_HXX__
