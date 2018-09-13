//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dlgnew.hxx
//
//  Contents:   "New Share" dialog
//
//  History:    21-Feb-95 BruceFo Created
//
//--------------------------------------------------------------------------

#ifndef __DLGNEW_HXX__
#define __DLGNEW_HXX__

#include "dlgbase.hxx"
#include "resource.h"

class CShareInfo;

class CDlgNewShare : public CDialog
{
    DECLARE_SIG;

public:

    CDlgNewShare(
        IN HWND hwndParent,
        IN PWSTR pszMachine
        );

    ~CDlgNewShare();

    static
    LRESULT CALLBACK
    SizeWndProc(
        IN HWND hwnd,
        IN UINT wMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    BOOL
    DlgProc(
        IN HWND hwnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Public data
    //

private:

    BOOL
    _OnInitDialog(
        IN HWND hwnd
        );

    BOOL
    _OnCommand(
        IN HWND hwnd,
        IN WORD wNotifyCode,
        IN WORD wID,
        IN HWND hwndCtl
        );

    BOOL
    _OnOK(
        IN HWND hwnd
        );

    BOOL
    _OnPermissions(
        IN HWND hwnd
        );

    VOID
    _CacheMaxUses(
        IN HWND hwnd
        );

    //
    // Class variables
    //

    PWSTR    _pszMachine;
    CShareInfo* _pShareInfo;

    BOOL    _bShareNameChanged;
    BOOL    _bPathChanged;
    BOOL    _bCommentChanged;
    int     _wMaxUsers;

    BOOL                 _fSecDescModified;
    PSECURITY_DESCRIPTOR _pStoredSecDesc;

    WNDPROC _pfnAllowProc;
};

#endif  // __DLGNEW_HXX__
