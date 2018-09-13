//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dlgmodel.cxx
//
//  Contents:   Implementation of the object model for html based dialogs
//
//  History:    08-22-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::close
//
//  Synopsis:   close the dialog
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::close()
{
    PostMessage(_hwnd, WM_CLOSE, 0, 0);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::get_dialogArguments
//
//  Synopsis:   Get the argument
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::get_dialogArguments(VARIANT * pvar)
{
    RRETURN(SetErrorInfo(VariantCopy(pvar, &_varArgIn)));
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::get_menuArguments
//
//  Synopsis:   Get the argument (same as dialogArguments, but
//              renamed for neatness)
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::get_menuArguments(VARIANT * pvar)
{
    RRETURN(SetErrorInfo(VariantCopy(pvar, &_varArgIn)));
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::get_returnValue
//
//  Synopsis:   Get the return value
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::get_returnValue(VARIANT * pvar)
{
    RRETURN(SetErrorInfo(VariantCopy(pvar, &_varRetVal)));
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLDlg::put_returnValue
//
//  Synopsis:   Set the return value
//
//---------------------------------------------------------------------------

HRESULT
CHTMLDlg::put_returnValue(VARIANT var)
{
    RRETURN(SetErrorInfo(VariantCopy(&_varRetVal, &var)));
}


