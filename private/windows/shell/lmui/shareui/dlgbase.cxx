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

INT_PTR CALLBACK
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
       SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
    }

    pPropPage = (CDialog*) GetWindowLongPtr(hwnd,GWLP_USERDATA);

    if (pPropPage != NULL)
    {
        return pPropPage->DlgProc(hwnd,msg,wParam,lParam);
    }
    else
    {
        return FALSE;
    }
}
