//****************************************************************************
//
//  Copyright (c) 1994, Microsoft Corporation
//
//  File:  BUSY.CPP
//
//  Implementation file for the CBusy class.
//
//  History:
//
//      Scott V. Walker, SEA     6/30/94    Created.
//
//****************************************************************************

#include "stdafx.h"

#include "portable.h"

#include "busy.h"

//****************************************************************************
//
//  CBusy::CBusy
//
//****************************************************************************
CBusy::CBusy(CWnd *pParentWnd, LPCTSTR pszText)
{
    SetBusy(pParentWnd, pszText);
}

//****************************************************************************
//
//  CBusy::CBusy
//
//****************************************************************************
CBusy::CBusy(CWnd *pParentWnd, UINT nID)
{
    CString sText;

    sText.LoadString(nID);
    SetBusy(pParentWnd, sText);
}

//****************************************************************************
//
//  CBusy::CBusy
//
//****************************************************************************
CBusy::CBusy(CWnd *pParentWnd)
{
    SetBusy(pParentWnd, _T(""));
}

//****************************************************************************
//
//  CBusy::CBusy
//
//****************************************************************************
CBusy::CBusy()
{
    SetBusy(NULL, _T(""));
}

//****************************************************************************
//
//  CBusy::SetBusy
//
//****************************************************************************
void CBusy::SetBusy(CWnd *pParentWnd, LPCTSTR pszText)
{
    m_pParentWnd = pParentWnd;

    m_hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    if (m_pParentWnd != NULL)
    {
        TCHAR szOldText[255];

        // Retrieve the current text and save it 'til later.
        szOldText[0] = '\0';
        m_pParentWnd->SendMessage(WM_BUSY_GETTEXT, 255, (LPARAM)szOldText);
        m_sOldText = szOldText;

        if (pszText == NULL)
            pszText = _T("");

        m_pParentWnd->SendMessage(WM_BUSY_SETTEXT, 0, (LPARAM)pszText);
    }
}

//****************************************************************************
//
//  CBusy::~CBusy
//
//****************************************************************************
CBusy::~CBusy()
{
    ::SetCursor(m_hOldCursor);

    if (m_pParentWnd != NULL)
    {
        m_pParentWnd->SendMessage(WM_BUSY_SETTEXT, 0,
            (LPARAM)(LPCTSTR)m_sOldText);
    }
}
