//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dlgbase.cxx
//
//  Contents:   CDialog base class
//
//  History:    19-Oct-94 BruceFo Created.
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "dlgbase.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CDialog::_WinDlgProc, static private
//
//  Synopsis:   Windows Dialog Procedure
//
//--------------------------------------------------------------------------

BOOL CALLBACK
CDialog::_WinDlgProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CDialog *pPropPage = NULL;

    if (msg==WM_INITDIALOG)
    {
       SetWindowLong(hwnd,GWL_USERDATA,lParam);
    }

    pPropPage = (CDialog*) GetWindowLong(hwnd,GWL_USERDATA);

    if (pPropPage != NULL)
    {
        return pPropPage->DlgProc(hwnd,msg,wParam,lParam);
    }
    else
    {
        return FALSE;
    }
}
