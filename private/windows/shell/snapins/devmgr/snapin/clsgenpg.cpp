/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    clsgenpg.cpp

Abstract:

    This module implements CClassGeneralPage -- class general property page

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// clsgenpg.cpp : implementation file
//

#include "devmgr.h"
#include "clsgenpg.h"

// help topic ids
const DWORD g_a108HelpIDs[]=
{
    IDC_CLSGEN_DESC, IDH_DISABLEHELP,
    IDC_CLSGEN_ICON, IDH_DISABLEHELP,
    IDC_CLSGEN_NAME, IDH_DISABLEHELP,
    0, 0,
};

BOOL
CClassGeneralPage::OnInitDialog(
    LPPROPSHEETPAGE ppsp
    )
{
    // notify CPropSheetData about the page creation
    // the controls will be initialize in UpdateControls virtual function.
    m_pClass->m_psd.PageCreateNotify(m_hDlg);
    return CPropSheetPage::OnInitDialog(ppsp);
}

UINT
CClassGeneralPage::DestroyCallback()
{
    // the property sheet is going away, consolidate the changes on the
    // device.
    // We do this because this is the page we are sure will be created --
    // this page is ALWAYS the first page.
    //

    // The DevInfoList returned from GetDevInfoList() function
    // is maintained by the class object during its lifetime.
    // we must NOT release the object.
    CDevInfoList* pClassDevInfo = m_pClass->GetDevInfoList();

    if (pClassDevInfo)
    {
        if (pClassDevInfo->DiGetExFlags(NULL) & DI_FLAGSEX_PROPCHANGE_PENDING)
        {
            //
            // property change pending, issue a DICS_PROPERTYCHANGE to the
            // class installer. A DICS_PROPCHANGE would basically remove the
            // device subtree and reenumerate it. If each property page issues
            // its own DICS_PROPCHANGE command, the device subtree would
            // be removed/reenumerate several times even though one is enough.
            // A property page sets DI_FLAGEX_PROPCHANGE_PENDING when it needs
            // a DICS_PROPCHANGE command to be issued.
            //
            SP_PROPCHANGE_PARAMS pcp;
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

            pcp.Scope = DICS_FLAG_GLOBAL;
            pcp.StateChange = DICS_PROPCHANGE;
            pClassDevInfo->DiSetClassInstallParams(NULL,
                                                   (PSP_CLASSINSTALL_HEADER)&pcp,
                                                   sizeof(pcp)
                                                   );
                                                   
            pClassDevInfo->DiCallClassInstaller(DIF_PROPERTYCHANGE, NULL);
            pClassDevInfo->DiTurnOnDiFlags(NULL, DI_PROPERTIES_CHANGE);
            pClassDevInfo->DiTurnOffDiExFlags(NULL, DI_FLAGSEX_PROPCHANGE_PENDING);
        }
        
        DWORD RestartFlags = pClassDevInfo->DiGetFlags();

        if (RestartFlags)
        // Do not use our window handle(or its parent) as the parent
        // to the newly create dialog because they are in "detroyed state".
        // WM_CLOSE does not help either.
        // NULL window handle(Desktop) should be okay here.
        {
            PromptForRestart(NULL, RestartFlags);
        }

        // notify CPropSheetData that the property sheet is going away
        m_pClass->m_psd.PageDestroyNotify(m_hDlg);
        if (RestartFlags & DI_PROPERTIES_CHANGE)
        {
            // Class properties changed. We need to refresh the machine.
            // Since we are running in a separate thread, we can not
            // call the refresh function, instead, we schedule it.
            // This must be done before enabling refresh.
            //
            m_pClass->m_pMachine->ScheduleRefresh();
        }
    }

    //
    // re-enable refresh because we disabled it when we are created --
    // see the Create member function. This may trigger a refresh
    //
    m_pClass->m_pMachine->EnableRefresh(TRUE);
    return CPropSheetPage::DestroyCallback();
}

void
CClassGeneralPage::UpdateControls(
    LPARAM lParam
    )
{
    if (lParam)
        m_pClass = (CClass*)lParam;

    HICON hClassIcon = m_pClass->LoadIcon();
    if (hClassIcon)
    {
        HICON hIconOld;
        m_IDCicon = IDC_CLSGEN_ICON;    // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_CLSGEN_ICON, STM_SETICON,
                                               (WPARAM)hClassIcon,
                                               0
                                               );

        if (NULL != hIconOld)
            DestroyIcon(hIconOld);
    }

    else
    {
        TRACE0(TEXT("LoadIcon failed\n"));
    }
    
    SetDlgItemText(m_hDlg, IDC_CLSGEN_NAME, m_pClass->GetDisplayName());
}

HPROPSHEETPAGE
CClassGeneralPage::Create(
    CClass* pClass
    )
{
    m_pClass = pClass;
    m_psp.lParam = (LPARAM) this;

    // no machine refresh while property sheet is up.
    // Refresh should be enabled again when this property sheet
    // is going away. See OnDestroy for detail.
    m_pClass->m_pMachine->EnableRefresh(FALSE);
    return CreatePage();
}



BOOL
CClassGeneralPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
            (ULONG_PTR)g_a108HelpIDs);
            
    return FALSE;
}


BOOL
CClassGeneralPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a108HelpIDs);
            
    return FALSE;
}
