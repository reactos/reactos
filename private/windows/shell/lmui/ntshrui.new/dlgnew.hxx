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
        IN HWND hwndParent
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

    // IN
    CShareInfo*    m_pInfoList; // the main list of nodes!

    // IN/OUT
    CShareInfo*    m_pShareInfo;

    // IN/OUT, The list of shares to replace. These shares are deleted on
    // apply. They have a different path, but the same share name as a
    // new share.
    CShareInfo*    m_pReplaceList;

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

    BOOL    _bShareNameChanged;
    BOOL    _bCommentChanged;
    int     _wMaxUsers;

    BOOL                 _fSecDescModified;
    PSECURITY_DESCRIPTOR _pStoredSecDesc;

    WNDPROC _pfnAllowProc;
};

#endif  // __DLGNEW_HXX__
