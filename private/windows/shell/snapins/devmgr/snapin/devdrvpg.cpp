/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devdrvpg.cpp

Abstract:

    This module implements CDeviceDriverPage -- device driver property page

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// devdrvpg.cpp : implementation file
//

#include "devmgr.h"
#include "devdrvpg.h"
#include "cdriver.h"
#include "devrmdlg.h"

//
// help topic ids
//

const DWORD g_a106HelpIDs[]=
{
    IDC_DEVDRV_ICON,                    IDH_DISABLEHELP,                    // Driver: "" (Static)
    IDC_DEVDRV_DESC,                    IDH_DISABLEHELP,                    // Driver: "" (Static)
    IDC_DEVDRV_DRIVERFILE_DESC,         IDH_DISABLEHELP,                    // Driver: "" (Static)
    IDC_DEVDRV_TITLE_DRIVERPROVIDER,    idh_devmgr_driver_provider_main,
    IDC_DEVDRV_DRIVERPROVIDER,          idh_devmgr_driver_provider_main,
    IDC_DEVDRV_TITLE_DRIVERDATE,        idh_devmgr_driver_date_main,
    IDC_DEVDRV_DRIVERDATE,              idh_devmgr_driver_date_main,
    IDC_DEVDRV_TITLE_DRIVERVERSION,     idh_devmgr_driver_version_main,
    IDC_DEVDRV_DRIVERVERSION,           idh_devmgr_driver_version_main,
    IDC_DEVDRV_TITLE_DRIVERSIGNER,      idh_devmgr_digital_signer,
    IDC_DEVDRV_DRIVERSIGNER,            idh_devmgr_digital_signer,
    IDC_DEVDRV_DETAILS,                 idh_devmgr_devdrv_details,          // Driver: "Driver Details" (Button)
    IDC_DEVDRV_UNINSTALL,               idh_devmgr_devdrv_uninstall,        // Driver: "Uninstall" (Button)
    IDC_DEVDRV_CHANGEDRIVER,            idh_devmgr_driver_change_driver,    // Driver: "&Change Driver..." (Button)
    0, 0
};

CDeviceDriverPage::~CDeviceDriverPage()
{
    if (m_pDriver) {

        delete m_pDriver;
    }
}

BOOL
CDeviceDriverPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam)) {

        switch (LOWORD(wParam)) {

            case IDC_DEVDRV_DETAILS:
            {
                //
                //Show driver file details
                //
                CDriverFilesDlg DriverFilesDlg(m_pDevice, m_pDriver);
                DriverFilesDlg.DoModal(m_hDlg, (LPARAM) &DriverFilesDlg);

                break;
            }


            case IDC_DEVDRV_UNINSTALL:
            {
                BOOL fChanged;

                if (UninstallDrivers(m_pDevice, m_hDlg, &fChanged) && fChanged) {

                    //
                    // Since this device has been removed, we will just schedule a 
                    // refresh to update the device tree and then destroy the property
                    // page.
                    //
                    m_pDevice->m_pMachine->ScheduleRefresh();
                    ::DestroyWindow(GetParent(m_hDlg));
                }

                break;
            }

            case IDC_DEVDRV_CHANGEDRIVER:
            {
                BOOL fChanged;
                DWORD Reboot = 0;

                if (UpdateDriver(m_pDevice, m_hDlg, FALSE, &fChanged, &Reboot) && fChanged) {

                    //
                    // bugbug
                    //
                    // A refresh on m_pDevice->m_pMachine is necessary here.
                    // Since we are running on a different thread and each
                    // property page may have cached the HDEVINFO and the
                    // SP_DEVINFO_DATA, refresh on the CMachine object can not
                    // be done here. The problem is worsen by the fact that
                    // user can go back to the device tree and work on the tree
                    // while this property sheet is still up.
                    // A warninig message box???????
                    m_pDevice->PropertyChanged();
                    m_pDevice->GetClass()->PropertyChanged();
                    m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_PROPERTIES_CHANGE);
                    ::PostMessage(GetParent(m_hDlg), PSM_SETTITLE, PSH_PROPTITLE, (LPARAM)m_pDevice->GetDisplayName());
                    ::PostMessage(GetParent(m_hDlg), PSM_CANCELTOCLOSE, 0, 0);
                    UpdateControls();

                    if (Reboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
                    
                        m_pDevice->m_pMachine->DiTurnOnDiFlags(*m_pDevice, DI_NEEDREBOOT);
                    }
                }

                break;
            }

            default:
                break;
        }
    }

    return FALSE;
}

//
// This function uninstalls the drivers for the given device
// INPUT:
//  pDevice -- the object represent the device
//  hDlg    -- the property page window handle
//  pfChanged   -- optional buffer to receive if drivers
//                 were uninstalled.
// OUTPUT:
//  TRUE    -- function succeeded.
//  FALSE   -- function failed.
//
BOOL
CDeviceDriverPage::UninstallDrivers(
    CDevice* pDevice,
    HWND hDlg,
    BOOL *pfUninstalled
    )
{
    BOOL Return = FALSE;
    int MsgBoxResult;
    TCHAR szText[MAX_PATH];

    if(!pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege)
    {
        // Must be an admin and on the local machine to remove a device.
        return FALSE;
    }

    CRemoveDevDlg   TheDlg(pDevice);

    if (IDOK == TheDlg.DoModal(hDlg, (LPARAM) &TheDlg)) {

        DWORD DiFlags;
        DiFlags = pDevice->m_pMachine->DiGetFlags(*pDevice);
        PromptForRestart(hDlg, DiFlags, IDS_REMOVEDEV_RESTART);
        Return = TRUE;
    }

    return Return;
}

//
// This function updates drivers for the given device
// INPUT:
//  pDevice -- the object represent the device
//  hDlg    -- the property page window handle
//  fReinstalll -- TRUE if this is for reinstallation.
//  pfChanged   -- optional buffer to receive if driver changes
//                 have occured.
// OUTPUT:
//  TRUE    -- function succeeded.
//  FALSE   -- function failed.
//
BOOL
CDeviceDriverPage::UpdateDriver(
    CDevice* pDevice,
    HWND hDlg,
    BOOL fReInstall,
    BOOL *pfChanged,
    DWORD *pdwReboot
    )
{
    HCURSOR hCursorOld;
    BOOL Installed = FALSE;
    DWORD InstallError = 0;

    if (!pDevice || !pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {
        // Must be an admin and on the local machine to update a device.

        ASSERT(FALSE);
        return FALSE;
    }

    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    Installed = pDevice->m_pMachine->InstallDevInst(hDlg, pDevice->GetDeviceID(), TRUE, pdwReboot);

    if (!Installed) {

        InstallError = GetLastError();
    }

    if (hCursorOld) {

        SetCursor(hCursorOld);
    }

    //
    // We will assume that something changed when we called InstallDevInst()
    // unless it returned FALSE and GetLastError() == ERROR_CANCELLED
    //
    if (pfChanged) {

        *pfChanged = TRUE;

        if (!Installed && (ERROR_CANCELLED == InstallError)) {
            *pfChanged = FALSE;
        }
    }

    return TRUE;
}

void
CDeviceDriverPage::InitializeDriver()
{

    DEBUGBREAK_ON(DEBUG_OPTIONS_BREAKON_INITDRIVER);

    if (m_pDriver) {

        delete m_pDriver;
        m_pDriver = NULL;
    }

    m_pDriver = m_pDevice->CreateDriver();
    CDriverFile* pDrvFile;
    PVOID Context;

    if (m_pDriver && m_pDriver->GetFirstDriverFile(&pDrvFile, Context)) {

        ::EnableWindow(GetControl(IDC_DEVDRV_DETAILS), TRUE);
        String strAltText_1;
        strAltText_1.LoadString(g_hInstance, IDS_DEVDRV_DRIVERFILE);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERFILE_DESC, strAltText_1);
    }

    else {

        ::EnableWindow(GetControl(IDC_DEVDRV_DETAILS), FALSE);
        String strAltText_1;
        strAltText_1.LoadString(g_hInstance, IDS_DEVDRV_NODRIVERFILE);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERFILE_DESC, strAltText_1);
    }
}

void
CDeviceDriverPage::UpdateControls(
    LPARAM lParam
    )
{
    if (lParam) {

        m_pDevice = (CDevice*) lParam;
    }

    try {

        // can not change driver on remote machine or the user
        // has no Administrator privilege.
        if (!m_pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {

            ::ShowWindow(GetControl(IDC_DEVDRV_CHANGEDRIVER), SW_HIDE);
            ::ShowWindow(GetControl(IDC_DEVDRV_UNINSTALL), SW_HIDE);
        }

        //
        // Hide the uninstall button if the device cannot be uninstalled
        //
        else if (!m_pDevice->IsUninstallable()) {

            ::ShowWindow(GetControl(IDC_DEVDRV_UNINSTALL), SW_HIDE);
        }

        HICON hIconOld;
        m_IDCicon = IDC_DEVDRV_ICON;    // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DEVDRV_ICON, STM_SETICON,
                        (WPARAM)(m_pDevice->LoadClassIcon()),
                        0
                        );

        if (hIconOld) {

            DestroyIcon(hIconOld);
        }

        InitializeDriver();

        SetDlgItemText(m_hDlg, IDC_DEVDRV_DESC, m_pDevice->GetDisplayName());

        String strDriverProvider, strDriverDate, strDriverVersion, strDriverSigner;
        m_pDevice->GetProviderString(strDriverProvider);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERPROVIDER, strDriverProvider);
        m_pDevice->GetDriverDateString(strDriverDate);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERDATE, strDriverDate);
        m_pDevice->GetDriverVersionString(strDriverVersion);
        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERVERSION, strDriverVersion);
        
        if (m_pDriver) 
        {
            m_pDriver->GetDriverSignerString(strDriverSigner);
        }
        
        //
        // If we could not get a digital signature string or we are connect to a remote
        // machine then just display Unknown for the Digital Signer.
        //
        if (strDriverSigner.IsEmpty() || !m_pDevice->m_pMachine->IsLocal()) {

            strDriverSigner.LoadString(g_hInstance, IDS_UNKNOWN);
        }

        SetDlgItemText(m_hDlg, IDC_DEVDRV_DRIVERSIGNER, strDriverSigner);
        AddToolTips(m_hDlg, IDC_DEVDRV_DRIVERSIGNER, (LPTSTR)strDriverSigner, &m_hwndDigitalSignerTip);
    }

    catch (CMemoryException* e) {

        e->Delete();
        // report memory error
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
}

BOOL
CDeviceDriverPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
        (ULONG_PTR)g_a106HelpIDs);

    return FALSE;
}


BOOL
CDeviceDriverPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
        (ULONG_PTR)g_a106HelpIDs);

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Driver Files
//
const DWORD g_a110HelpIDs[]=
{
    IDC_DRIVERFILES_ICON,           IDH_DISABLEHELP,                // Driver: "" (Icon)
    IDC_DRIVERFILES_DESC,           IDH_DISABLEHELP,                // Driver: "" (Static)
    IDC_DRIVERFILES_FILES,          IDH_DISABLEHELP,                // Driver: "Provider:" (Static)
    IDC_DRIVERFILES_FILELIST,       idh_devmgr_driver_driver_files, // Driver: "" (ListBox)
    IDC_DRIVERFILES_TITLE_PROVIDER, idh_devmgr_driver_provider,     // Driver: "Provider:" (Static)
    IDC_DRIVERFILES_PROVIDER,       idh_devmgr_driver_provider,     // Driver: "" (Static)
    IDC_DRIVERFILES_TITLE_COPYRIGHT,idh_devmgr_driver_copyright,    // Driver: "Copyright:" (Static)
    IDC_DRIVERFILES_COPYRIGHT,      idh_devmgr_driver_copyright,    // Driver: "" (Static)
    IDC_DRIVERFILES_TITLE_VERSION,  idh_devmgr_driver_file_version, // Driver: "Version:" (Static)
    IDC_DRIVERFILES_VERSION,        idh_devmgr_driver_file_version, // Driver: "File Version:" (Static)
    0, 0
};

BOOL CDriverFilesDlg::OnInitDialog()
{
    try {

        HICON hIconOld;
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_ICON, STM_SETICON,
                                             (WPARAM)(m_pDevice->LoadClassIcon()), 0);

        if (hIconOld)
            DestroyIcon(hIconOld);

        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_DESC, m_pDevice->GetDisplayName());

        //
        //Initialize the list of drivers
        //
        CDriverFile* pDrvFile;
        PVOID Context;
        if (m_pDriver && m_pDriver->GetFirstDriverFile(&pDrvFile, Context)) {

            do {

                ASSERT(pDrvFile);
                LPCTSTR pFullPathName;
                pFullPathName = pDrvFile->GetFullPathName();
                if (pFullPathName) {

                    int iItem;
                    iItem = (int)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_FILELIST,
                                               LB_ADDSTRING, 0,
                                               (LPARAM)(LPTSTR)pFullPathName);

                    if (LB_ERR != iItem) {

                        SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_FILELIST, LB_SETITEMDATA,
                                           iItem, (LPARAM)pDrvFile);

                    }
                }
            } while (m_pDriver->GetNextDriverFile(&pDrvFile, Context));

            int FileIndex;
            FileIndex = (int)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_FILELIST, LB_GETCOUNT, 0, 0);

            if (LB_ERR != FileIndex && FileIndex >= 1) {

                SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_FILELIST, LB_SETCURSEL, 0, 0);
                ShowCurDriverFileDetail();

            } else {

                // nothing on the driver file list, disable it
                ::EnableWindow(GetControl(IDC_DRIVERFILES_FILELIST), FALSE);
            }
        }
    }

    catch (CMemoryException* e) {

        e->Delete();
        return FALSE;
    }

    return TRUE;
}

void
CDriverFilesDlg::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (BN_CLICKED == HIWORD(wParam)) {

        if (IDOK == LOWORD(wParam)) {

            EndDialog(m_hDlg, IDOK);

        } else if (IDCANCEL == LOWORD(wParam)) {

            EndDialog(m_hDlg, IDCANCEL);
        }

    } else if (LBN_SELCHANGE == HIWORD(wParam) && IDC_DRIVERFILES_FILELIST == LOWORD(wParam)) {

        ShowCurDriverFileDetail();
    }
}

BOOL
CDriverFilesDlg::OnDestroy()
{
    HICON hIcon;

    if(hIcon = (HICON)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_ICON, STM_GETICON, 0, 0)) {
        DestroyIcon(hIcon);
    }
    return FALSE;
}

BOOL
CDriverFilesDlg::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
        (ULONG_PTR)g_a110HelpIDs);
    return FALSE;
}


BOOL
CDriverFilesDlg::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a110HelpIDs);

    return FALSE;
}

void
CDriverFilesDlg::ShowCurDriverFileDetail()
{

    int iItem = (int)SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_FILELIST, LB_GETCURSEL, 0, 0);

    if (LB_ERR != iItem) {

        CDriverFile* pDrvFile = (CDriverFile*)(SendDlgItemMessage(m_hDlg, IDC_DRIVERFILES_FILELIST, LB_GETITEMDATA, iItem, 0));

        ASSERT(pDrvFile);

        TCHAR TempString[LINE_LEN];
        LPCTSTR pFullPathName;
        pFullPathName = pDrvFile->GetFullPathName();

        if (!pFullPathName || !pDrvFile->HasVersionInfo()) {

            LoadResourceString(IDS_NOT_PRESENT, TempString, ARRAYLEN(TempString));
            SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, TempString);
            SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, TempString);
            SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, TempString);

        } else {

            TempString[0] = _T('\0');
            LPCTSTR  pString;

            pString = pDrvFile->GetVersion();
            if (!pString && _T('\0') == TempString[0]) {

                LoadResourceString(IDS_NOT_AVAILABLE, TempString, ARRAYLEN(TempString));
                pString = TempString;
            }

            SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, (LPTSTR)pString);

            pString = pDrvFile->GetProvider();
            if (!pString && _T('\0') == TempString[0]) {

                LoadResourceString(IDS_NOT_AVAILABLE, TempString, ARRAYLEN(TempString));
                pString = TempString;
            }

            SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, (LPTSTR)pString);

            pString = pDrvFile->GetCopyright();
            if (!pString && _T('\0') == TempString[0]) {

                LoadResourceString(IDS_NOT_AVAILABLE, TempString, ARRAYLEN(TempString));
                pString = TempString;
            }

            SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, (LPTSTR)pString);
        }

    } else {

        // no selection
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_VERSION, TEXT(""));
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_PROVIDER, TEXT(""));
        SetDlgItemText(m_hDlg, IDC_DRIVERFILES_COPYRIGHT, TEXT(""));
    }
}
