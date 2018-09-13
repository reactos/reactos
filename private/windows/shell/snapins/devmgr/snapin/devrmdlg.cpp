/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devrmdlg.cpp

Abstract:

    This module implements CRemoveDevDlg -- device removing dialog box

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "hwprof.h"
#include "devrmdlg.h"

//
// help topic ids
//
const DWORD g_a210HelpIDs[]=
{
        IDC_REMOVEDEV_ICON,     IDH_DISABLEHELP,        // Confirm Device Removal: "" (Static)
        IDC_REMOVEDEV_DEVDESC,  IDH_DISABLEHELP,        // Confirm Device Removal: "" (Static)
        IDC_REMOVEDEV_WARNING,  IDH_DISABLEHELP,        // Confirm Device Removal: "" (Static)
        0, 0
};

//
// CRemoveDevDlg implementation
//
BOOL CRemoveDevDlg::OnInitDialog() 
{
    SetDlgItemText(m_hDlg, IDC_REMOVEDEV_DEVDESC, m_pDevice->GetDisplayName());
    HICON hIconOld;
    hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_REMOVEDEV_ICON,
                                         STM_SETICON,
                                         (WPARAM)(m_pDevice->LoadClassIcon()),
                                         0
                                         );
    if (hIconOld)
        DestroyIcon(hIconOld);

    try
    {
        String str;
        str.LoadString(g_hInstance, IDS_REMOVEDEV_WARN);
        SetDlgItemText(m_hDlg, IDC_REMOVEDEV_WARNING, str);
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        return FALSE;
    }

    return TRUE;
}

void
CRemoveDevDlg::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam))
    {
        if (IDOK == LOWORD(wParam))
        {
            OnOk();
        }
        
        else if (IDCANCEL == LOWORD(wParam))
        {
            EndDialog(m_hDlg, IDCANCEL);
        }
    }
}

void CRemoveDevDlg::OnOk()
{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    int hwpfIndex;
    BOOL Continue = TRUE;

    DEBUGBREAK_ON(DEBUG_OPTIONS_BREAKON_REMOVEDEVICE);
    CHwProfile* phwpf;
    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;

    //
    // Uninstall does not apply to specific profiles -- it is global.
    //
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;
    
    //
    // walk down the tree and remove all of this device's children
    //
    if (m_pDevice->GetChild() &&
        !IsRemoveSubtreeOk(m_pDevice->GetChild(), &rmdParams))
    {
        //
        // Children refuse the removal. Cancel the removal.
        //
        MsgBoxParam(m_hDlg, IDS_DESCENDANTS_VETO, 0, MB_OK | MB_ICONINFORMATION);
        EndDialog(m_hDlg, IDCANCEL);
        return;
    }

    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);
    m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                       (PSP_CLASSINSTALL_HEADER)&rmdParams,
                       sizeof(rmdParams));
    
    BOOL RemovalOK;
    
    //
    // Either this device has no children or the children has no
    // objection on removal. Remove it.
    //
    HCURSOR hCursorOld;
    hCursorOld = SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
    
    RemovalOK = m_pDevice->m_pMachine->DiCallClassInstaller(DIF_REMOVE, *m_pDevice);
    
    if (hCursorOld)
    {
        SetCursor(hCursorOld);
    }
    
    m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice, NULL, 0);
    
    if (RemovalOK)
    {
        EndDialog(m_hDlg, IDOK);
    }
    
    else
    {
        //
        // Can not removed the device, return Cancel so that
        // the caller know what is going on.
        //
        MsgBoxParam(m_hDlg, IDS_UNINSTALL_FAILED, 0, MB_OK | MB_ICONINFORMATION);
        EndDialog(m_hDlg, IDCANCEL);
    }
}

//
// This function walks the substree started with the given CDevice to
// see if it is ok to removed the CDevice.
// INPUT:
//      pDevice  -- the device
//      prmdParams  -- parameter used to call the setupapi
// OUTPUT:
//      TRUE -- it is ok to remove
//      FALSE -- it is NOT ok to remove
BOOL
CRemoveDevDlg::IsRemoveSubtreeOk(
    CDevice* pDevice,
    PSP_REMOVEDEVICE_PARAMS prmdParams
    )
{
    BOOL Result = TRUE;


    HDEVINFO hDevInfo;
    while (Result && pDevice)
    {
        //
        // if the device has children, remove all of them.
        //
        if (Result && pDevice->GetChild())
        {
            Result = IsRemoveSubtreeOk(pDevice->GetChild(), prmdParams);
        }
        
        //
        // create a new HDEVINFO just for this device -- we do not want
        // to change anything in the main device tree maintained by CMachine
        //
        hDevInfo = pDevice->m_pMachine->DiCreateDeviceInfoList(NULL, m_hDlg);
        
        if (INVALID_HANDLE_VALUE == hDevInfo)
        {
            return FALSE;
        }
        
        SP_DEVINFO_DATA DevData;
        DevData.cbSize = sizeof(DevData);
        CDevInfoList DevInfoList(hDevInfo, m_hDlg);
        
        //
        // include the device in the newly created hdevinfo
        //
        DevInfoList.DiOpenDeviceInfo(pDevice->GetDeviceID(), m_hDlg, 0,
                                     &DevData);

        DevInfoList.DiSetClassInstallParams(&DevData,
                                            (PSP_CLASSINSTALL_HEADER)prmdParams,
                                            sizeof(SP_REMOVEDEVICE_PARAMS)
                                            );
        
        //
        // remove this devnode.
        //
        Result = DevInfoList.DiCallClassInstaller(DIF_REMOVE, &DevData);
        DevInfoList.DiSetClassInstallParams(&DevData, NULL, 0);
        
        //
        // continue the query on all the siblings
        //
        pDevice = pDevice->GetSibling();
    }

    return Result;
}

BOOL
CRemoveDevDlg::OnDestroy()
{
    HICON hIcon;

    if(hIcon = (HICON)SendDlgItemMessage(m_hDlg, IDC_REMOVEDEV_ICON, STM_GETICON, 0, 0)) {
        DestroyIcon(hIcon);
    }
    
    return FALSE;
}

BOOL
CRemoveDevDlg::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
            (ULONG_PTR)g_a210HelpIDs);
    return FALSE;
}

BOOL
CRemoveDevDlg::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a210HelpIDs);
    return FALSE;
}
