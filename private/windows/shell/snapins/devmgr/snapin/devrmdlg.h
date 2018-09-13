// devrmdlg.h : header file
//

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devrmdlg.h

Abstract:

    header file for devrmdlg.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

//
// help topic ids
//

#define IDH_DISABLEHELP (DWORD(-1))
#define idh_devmgr_confirmrremoval_listbox  210100  // Confirm Device Removal: "" (Static)
#define idh_devmgr_confirmremoval_all   210110  // Confirm Device Removal: "Remove from &all configurations." (Button)
#define idh_devmgr_confirmremoval_specific  210120  // Confirm Device Removal: "Remove from &specific configuration." (Button)
#define idh_devmgr_confirmremoval_configuration 210130  // Confirm Device Removal: "" (ComboBox)


/////////////////////////////////////////////////////////////////////////////
// CRemoveDevDlg dialog

class CRemoveDevDlg : public CDialog
{
public:
    CRemoveDevDlg(CDevice* pDevice)
    : CDialog(IDD_REMOVE_DEVICE),
      m_pDevice(pDevice)
    {}
    virtual BOOL OnInitDialog();
    virtual void OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnDestroy();
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
private:
    void OnOk();
    CDevice*        m_pDevice;
    BOOL IsRemoveSubtreeOk(CDevice* pDevice, PSP_REMOVEDEVICE_PARAMS prmdParams);
};
