#include "devmgr.h"


/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    prndlg.cpp

Abstract:

    This module implements CPrintDialog, the class that processes
    printer dialog

Author:

    William Hsieh (williamh) created

Revision History:


--*/


//
// CPrintDialog implementation
//


//
// help topic ids
//

const DWORD g_a207HelpIDs[]=
{
    IDC_PRINT_SYSTEM_SUMMARY,   idh_devmgr_print_system,
    IDC_PRINT_SELECT_CLASSDEVICE,   idh_devmgr_print_device,
    IDC_PRINT_ALL,          idh_devmgr_print_both,
    IDC_PRINT_REPORT_TYPE_TEXT, idh_devmgr_print_report,
    0, 0
};

HRESULT
CDevMgrPrintDialogCallback::QueryInterface(
    REFIID  riid,
    void**  ppv
    )
{
    if (!ppv) {
    
        return E_INVALIDARG;
    }
    
    HRESULT hr = S_OK;


    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown*)this;
    }
    
    else if (IsEqualIID(riid, IID_IPrintDialogCallback))
    {
        *ppv = (ISnapinAbout*)this;
    }
    
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    
    if (SUCCEEDED(hr))
    {
        AddRef();
    }

    return hr;
}

ULONG
CDevMgrPrintDialogCallback::AddRef()
{
    ::InterlockedIncrement((LONG*)&m_Ref);
    
    return m_Ref;
}

ULONG
CDevMgrPrintDialogCallback::Release()
{
    ::InterlockedDecrement((LONG*)&m_Ref);
    
    if (!m_Ref)
    {
        delete this;
        return 0;
    }

    return m_Ref;
}

HRESULT
CDevMgrPrintDialogCallback::InitDone()
{
    return S_OK;
}

HRESULT
CDevMgrPrintDialogCallback::SelectionChange()
{
    return S_OK;
}

HRESULT
CDevMgrPrintDialogCallback::HandleMessage(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT *pResult
    )
{
    *pResult = FALSE;
    
    switch (uMsg) {
    
    case WM_INITDIALOG:
        *pResult = OnInitDialog(hDlg);
        break;

    case WM_COMMAND:
        *pResult = OnCommand(hDlg, wParam, lParam);
        break;

    case WM_HELP:
        *pResult = OnHelp((LPHELPINFO)lParam);
        break;

    case WM_CONTEXTMENU:
        *pResult = OnContextMenu(hDlg, LOWORD(lParam), HIWORD(lParam), wParam);
        break;

    default:
        break;
    }
    
    return S_OK;
}

BOOL
CDevMgrPrintDialogCallback::OnInitDialog(
    HWND hWnd
    )
{
    
    int DefaultId = IDC_PRINT_SELECT_CLASSDEVICE;
    
    m_pPrintDialog->SetReportType(REPORT_TYPE_CLASSDEVICE);

    if (!(m_pPrintDialog->GetTypeEnableMask() & REPORT_TYPE_MASK_CLASSDEVICE))
    {
        EnableWindow(GetDlgItem(hWnd, IDC_PRINT_SELECT_CLASSDEVICE), FALSE);
        DefaultId =  IDC_PRINT_SYSTEM_SUMMARY;
        m_pPrintDialog->SetReportType(REPORT_TYPE_SUMMARY);
    }
    
    if (!(m_pPrintDialog->GetTypeEnableMask() & REPORT_TYPE_MASK_SUMMARY))
    {
        EnableWindow(GetDlgItem(hWnd, IDC_PRINT_SYSTEM_SUMMARY), FALSE);
        
        if (IDC_PRINT_SYSTEM_SUMMARY == DefaultId)
        {
            DefaultId = IDC_PRINT_ALL;
            m_pPrintDialog->SetReportType(REPORT_TYPE_SUMMARY_CLASSDEVICE);
        }
    }

    if (!(m_pPrintDialog->GetTypeEnableMask() & REPORT_TYPE_MASK_SUMMARY_CLASSDEVICE))
    {
        EnableWindow(GetDlgItem(hWnd, IDC_PRINT_ALL), FALSE);
    }

    CheckDlgButton(hWnd, DefaultId, BST_CHECKED);
    
    return TRUE;
}

UINT_PTR
CDevMgrPrintDialogCallback::OnCommand(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_PRINT_SELECT_CLASSDEVICE))
    {
        m_pPrintDialog->SetReportType(REPORT_TYPE_CLASSDEVICE);
    }

    else if (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_PRINT_SYSTEM_SUMMARY))
    {
        m_pPrintDialog->SetReportType(REPORT_TYPE_SUMMARY);
    }
    
    else if (BST_CHECKED == IsDlgButtonChecked(hWnd, IDC_PRINT_ALL))
    {
        m_pPrintDialog->SetReportType(REPORT_TYPE_SUMMARY_CLASSDEVICE);
    }

    return FALSE;
}

BOOL
CDevMgrPrintDialogCallback::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    int id = GetDlgCtrlID((HWND)pHelpInfo->hItemHandle);

    //
    // We only want to intercept help messages for controls that we are
    // responsible for.
    //
    if ((IDC_PRINT_SYSTEM_SUMMARY == id) ||
        (IDC_PRINT_SELECT_CLASSDEVICE == id) ||
        (IDC_PRINT_ALL == id) ||
        (IDC_PRINT_REPORT_TYPE_TEXT == id)) {

        WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
                (ULONG_PTR)g_a207HelpIDs);

        return TRUE;
    }

    //
    // If it is not one of the above controls then just let the normal help handle
    // the message.  We do this by returning FALSE
    //
    return FALSE;
}

BOOL
CDevMgrPrintDialogCallback::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos,
    WPARAM wParam
    )
{
    POINT pt;

    if (hWnd == (HWND)wParam) {
    
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        wParam = (WPARAM)ChildWindowFromPoint(hWnd, pt);
    }

    int id = GetDlgCtrlID((HWND)wParam);

    //
    // We only want to intercept help messages for controls that we are
    // responsible for.
    //
    if ((IDC_PRINT_SYSTEM_SUMMARY == id) ||
        (IDC_PRINT_SELECT_CLASSDEVICE == id) ||
        (IDC_PRINT_ALL == id) ||
        (IDC_PRINT_REPORT_TYPE_TEXT == id)) {
    
        WinHelp((HWND)wParam, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
                (ULONG_PTR)g_a207HelpIDs);

        return TRUE;
    }

    return FALSE;
}

BOOL
CPrintDialog::PrintDlg(
    HWND hwndOwner,
    DWORD TypeEnableMask
    )
{
    HRESULT hr;

    ASSERT(REPORT_TYPE_MASK_NONE != TypeEnableMask);

    memset(&m_PrintDlg, 0, sizeof(m_PrintDlg));

    CDevMgrPrintDialogCallback* pPrintDialogCallback = new CDevMgrPrintDialogCallback;
    pPrintDialogCallback->m_pPrintDialog = this;
    
    m_TypeEnableMask = TypeEnableMask;

    m_PrintDlg.lStructSize = sizeof(m_PrintDlg);
    m_PrintDlg.hwndOwner = hwndOwner;
    m_PrintDlg.hDC = NULL;
    m_PrintDlg.Flags = PD_ENABLEPRINTTEMPLATE | PD_RETURNDC | PD_NOPAGENUMS;
    m_PrintDlg.Flags2 = 0;
    m_PrintDlg.ExclusionFlags = 0;
    m_PrintDlg.hInstance = g_hInstance;
    m_PrintDlg.nCopies = 1;
    m_PrintDlg.nStartPage = START_PAGE_GENERAL;
    m_PrintDlg.lpCallback = (IUnknown*)pPrintDialogCallback;
    m_PrintDlg.lpPrintTemplateName = MAKEINTRESOURCE(IDD_PRINT);
    
    if (FAILED(hr = PrintDlgEx(&m_PrintDlg))) {
        
        return FALSE;
    }

    //
    // If the user did not want to print then return FALSE.
    // This can happen if the user hits Cancel on the print dialog.
    //
    if (m_PrintDlg.dwResultAction != PD_RESULT_PRINT) {

        return FALSE;
    }

    return TRUE;
}

