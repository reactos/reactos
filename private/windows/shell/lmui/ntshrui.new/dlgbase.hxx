//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dlgbase.hxx
//
//  Contents:   CDialog base class
//
//  History:    19-Oct-94 BruceFo Created.
//
//--------------------------------------------------------------------------

#ifndef __DLGBASE_HXX__
#define __DLGBASE_HXX__

class CDialog
{
public:

    //
    // constructor, destructor
    //

    CDialog(
        IN HWND hwndParent,
        IN LPWSTR lpszTemplate
        )
        :
        _hwndParent(hwndParent),
        _lpszTemplate(lpszTemplate)
    {
    }

    virtual ~CDialog() { }

    int
    DoModal(
        VOID
        )
    {
        return DialogBoxParam(
                    g_hInstance,
                    _lpszTemplate,
                    _hwndParent,
                    _WinDlgProc,
                    (LPARAM) this);
    }

    virtual
    BOOL
    DlgProc(
        IN HWND hwnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        ) = 0;

private:

    //
    // Dialog procedures
    //

    static
    BOOL CALLBACK
    _WinDlgProc(
        IN HWND hwnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Class variables
    //

    HWND    _hwndParent;
    LPTSTR  _lpszTemplate;
};

#endif  // __DLGBASE_HXX__
