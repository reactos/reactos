// devdrvpg.h : header file
//

#ifndef __DEVDRVPG_H__
#define __DEVDRVPG_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devdrvpg.h

Abstract:

    header file for devdrvpg.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "proppage.h"

//
// help topic ids
//
#define IDH_DISABLEHELP (DWORD(-1))
#define idh_devmgr_driver_copyright     106130  // Driver: "" (Static)
#define idh_devmgr_driver_change_driver 106140  // Driver: "&Change Driver..." (Button)
#define idh_devmgr_devdrv_details       400400  // Driver: "Driver Details..." (Button)
#define idh_devmgr_devdrv_uninstall     400500  // Driver: "&Uninstall" (Button)
#define idh_devmgr_driver_driver_files  106100  // Driver: "" (ListBox)
#define idh_devmgr_driver_provider      106110  // Driver: "" (Static)
#define idh_devmgr_driver_file_version  106120  // Driver: "" (Static)
#define idh_devmgr_driver_provider_main 106122  // Driver tab static
#define idh_devmgr_driver_date_main     106124  // Driver tab static
#define idh_devmgr_driver_version_main  106126  // Driver tab static
#define idh_devmgr_digital_signer       106127  //driver tab static- digital signer




class CDeviceDriverPage : public CPropSheetPage
{
public:
    CDeviceDriverPage() :
        m_pDriver(NULL), m_pDevice(NULL), m_hwndDigitalSignerTip(NULL),
        CPropSheetPage(g_hInstance, IDD_DEVDRV_PAGE)
        {}

    ~CDeviceDriverPage();
    HPROPSHEETPAGE Create(CDevice* pDevice)
    {
        ASSERT(pDevice);
        m_pDevice = pDevice;
        
        // override PROPSHEETPAGE structure here...
        m_psp.lParam = (LPARAM)this;
        return CreatePage();
    }
    static BOOL UpdateDriver(CDevice* pDevice, 
                             HWND hDlg,
                             BOOL fReInstall,
                             BOOL *pfChanged = NULL,
                             DWORD *pdwReboot = NULL
                             );
    static BOOL UninstallDrivers(CDevice* pDevice, 
                                 HWND hDlg,
                                 BOOL *pfUninstalled = NULL
                                 );

protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //virtual BOOL OnNotify(LPNMHDR pnmhdr);
    virtual void UpdateControls(LPARAM lParam = 0);
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
private:
    void InitializeDriver();
    CDriver*    m_pDriver;
    CDevice*    m_pDevice;
    HWND        m_hwndDigitalSignerTip;
};

/////////////////////////////////////////////////////////////////////////////
// CDriverFilesDlg dialog

class CDriverFilesDlg : public CDialog
{
public:
    CDriverFilesDlg(CDevice* pDevice, CDriver* pDriver)
        : CDialog(IDD_DRIVERFILES),
          m_pDriver(pDriver), m_pDevice(pDevice)
        {}
    virtual BOOL OnInitDialog();
    virtual void OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnDestroy();
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
private:
    void ShowCurDriverFileDetail();
    CDriver*    m_pDriver;
    CDevice*    m_pDevice;
};


#endif // _DEVDRVPG_H__
