//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft, 1997
//
//  File:       OleWindow.cpp
//
//  Contents:   CPreviewBand::IOleWindow methods
//
//  History:    7-24-97  Davepl  Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "PreviewBand.h"

// 
// IOleWindow::GetWindow for CPreviewBand
//

STDMETHODIMP CPreviewBand::GetWindow(HWND * phWnd)
{
    *phWnd = m_hWnd;
    return S_OK;
}

// 
// IOleWindow::ContextSensitiveHelp for CPreviewBand
//

STDMETHODIMP CPreviewBand::ContextSensitiveHelp(BOOL)
{
    return E_NOTIMPL;
}

