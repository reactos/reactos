/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    proppage.cpp

Abstract:

    This module implements CPropSheetPage class, the base class of
    CDeviceGeneralPage, CClassGeneralPage and CDeviceDriverPage.

Author:

    William Hsieh (williamh) created

Revision History:


--*/
#include "devmgr.h"
#include "proppage.h"

//
// CPropSheetPage implementation
//


CPropSheetPage::CPropSheetPage(
    HINSTANCE hInst,
    UINT idTemplate
    )
{
    memset(&m_psp, 0, sizeof(m_psp));
    m_psp.dwSize = sizeof(m_psp);
    m_psp.dwFlags = PSP_USECALLBACK;
    m_psp.pszTemplate = MAKEINTRESOURCE(idTemplate);
    m_psp.lParam = 0;
    m_psp.pfnCallback = CPropSheetPage::PageCallback;
    m_psp.pfnDlgProc = PageDlgProc;
    m_psp.hInstance = hInst;
    m_Active = FALSE;
    //
    // By default, we want every derived page to update its contents
    // on each PSN_SETACTIVE so that it has up-to-date information
    // presented since any page in the same property sheet may
    // change the target object and there are not reliable ways
    // to synchronize changes among pages.
    //
    m_AlwaysUpdateOnActive = TRUE;
    // Right after WM_INITDIALOG, we will receive a
    // PSN_SETACTIVE and by setting m_UpdateControlsPending
    // to TRUE, the PSN_SETACTIVE handler will call UpdateControls
    // to refresh the page.
    // derived classes can turn this off if they wish to
    // update the dialog box only once in OnInitDialog.
    // Also, since m_AlwaysUpdateOnActive is TRUE by-default,
    // m_UpdateControlPending is FALSE by-default.
    m_UpdateControlsPending = FALSE;

    m_IDCicon = 0;
}

INT_PTR
CPropSheetPage::PageDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CPropSheetPage* pThis = (CPropSheetPage *) GetWindowLongPtr(hDlg, DWLP_USER);
    LPNMHDR pnmhdr;
    BOOL Result;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
        PROPSHEETPAGE* ppsp = (PROPSHEETPAGE *)lParam;
        pThis = (CPropSheetPage *) ppsp->lParam;
        ASSERT(pThis);
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
        pThis->m_hDlg = hDlg;
        Result = pThis->OnInitDialog(ppsp);
        break;
        }
    case WM_COMMAND:
        {
        if (pThis)
            Result = pThis->OnCommand(wParam, lParam);
        else
            Result = FALSE;
        break;
        }
    case WM_NOTIFY:
        {
        pnmhdr = (LPNMHDR)lParam;
        switch (pnmhdr->code)
        {
            case PSN_SETACTIVE:
            ASSERT(pThis);
            Result = pThis->OnSetActive();
            break;
            case PSN_KILLACTIVE:
            ASSERT(pThis);
            Result = pThis->OnKillActive();
            break;
            case PSN_APPLY:
            ASSERT(pThis);
            Result = pThis->OnApply();
            break;
            case PSN_RESET:
            ASSERT(pThis);
            Result = pThis->OnReset();
            break;
            case PSN_WIZFINISH:
            ASSERT(pThis);
            Result = pThis->OnWizFinish();
            break;
            case PSN_WIZNEXT:
            ASSERT(pThis);
            Result = pThis->OnWizNext();
            break;
            case PSN_WIZBACK:
            ASSERT(pThis);
            Result = pThis->OnWizBack();
            break;
            default:
            ASSERT(pThis);
            pThis->OnNotify(pnmhdr);
            Result = FALSE;
            break;
        }
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0L);
        break;
        }
    case WM_DESTROY:
        {
        if (pThis)
            Result = pThis->OnDestroy();
        else
            Result = FALSE;
        break;
        }
    case PSM_QUERYSIBLINGS:
        {
        ASSERT(pThis);
        Result = pThis->OnQuerySiblings(wParam, lParam);
        break;
        }
    case WM_HELP:
        {
        ASSERT(pThis);
        pThis->OnHelp((LPHELPINFO)lParam);
        break;
        }
    case WM_CONTEXTMENU:
        {
        ASSERT(pThis);
        pThis->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
        return TRUE;
        }
    default:
        {
        Result = FALSE;
        break;
        }
    }
    return Result;
}

UINT
CPropSheetPage::DestroyCallback()
{
    delete this;
    return TRUE;
}

//
// This function is the property page create/desrtroy callback.
// It monitors the PSPSCB_RELEASE to delete this object.
// This is the most reliable way to free objects associated with
// a property page. A property page which is never activated
// will not receive a WM_DESTROY message because its window is not
// created.
//
UINT
CPropSheetPage::PageCallback(
    HWND hDlg,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
    )
{
    ASSERT(ppsp);
    CPropSheetPage* pThis = (CPropSheetPage*)ppsp->lParam;
    if (PSPCB_CREATE == uMsg && pThis)
    {
    pThis->CreateCallback();
    }
    else if (PSPCB_RELEASE == uMsg && pThis)
    {
    return pThis->DestroyCallback();
    }
    return TRUE;
}


BOOL
CPropSheetPage::OnQuerySiblings(
    WPARAM wParam,
    LPARAM lParam
    )
{
    ASSERT(m_hDlg);
    DMQUERYSIBLINGCODE Code = (DMQUERYSIBLINGCODE)wParam;
    // properties of the device attached to this page have
    // changed. Try to update the controls if we are currently
    // active. If we are active at this time, signal a flag
    // so that on PSN_SETACTIVE will do the update.
    if (QSC_PROPERTY_CHANGED == Code)
    {
    if (m_Active)
    {
        UpdateControls(lParam);
        m_UpdateControlsPending = FALSE;
    }
    else
    {
        // wait for SetActive to update the controls
        m_UpdateControlsPending = TRUE;
    }
    }
    SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, 0L);
    return FALSE;
}


BOOL
CPropSheetPage::OnDestroy()
{
    HICON hIcon;

    if (m_IDCicon)
    {
        if (hIcon = (HICON)SendDlgItemMessage(m_hDlg, m_IDCicon, STM_GETICON, 0, 0))
        {
            DestroyIcon(hIcon);
        }
        m_IDCicon = 0;
    }
    return FALSE;
}
