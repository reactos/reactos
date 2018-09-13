/******************************************************************************

  Source File: Property Dialogs.CPP

  Implements the dialogs used in the Profile Management UI.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version

******************************************************************************/

#include    "ICMUI.H"

#include    <stdlib.h>
#include    "Resource.H"

//  CInstallPage member functions

//  Class constructor

CInstallPage::CInstallPage(CProfilePropertySheet *pcpps) :
    CDialog(pcpps -> Instance(),
        pcpps -> Profile().IsInstalled() ? UninstallPage : InstallPage, 
        pcpps -> Window()), m_cppsBoss(*pcpps){ }

CInstallPage::~CInstallPage() {}

//  OnInit function- this initializes the property page

BOOL    CInstallPage::OnInit() {

    SetDlgItemText(m_hwnd, ProfileNameControl, m_cppsBoss.Profile().GetName());
    if  (m_cppsBoss.Profile().IsInstalled())
        CheckDlgButton(m_hwnd, DeleteFileControl, 
            m_cppsBoss.DeleteIsOK() ? BST_CHECKED : BST_UNCHECKED);

    return  TRUE;
}

BOOL    CInstallPage::OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl) {
    switch  (wid) {
        case    DeleteFileControl:

            if  (wNotifyCode == BN_CLICKED) {
                m_cppsBoss.DeleteOnUninstall(
                    IsDlgButtonChecked(m_hwnd, DeleteFileControl) == 
                        BST_CHECKED);
                return  TRUE;
            }

            break;

        case    ProfileNameControl:
            if  (wNotifyCode == EN_SETFOCUS) {
                //  We don't want the entire string selected and scrolled
                SendDlgItemMessage(m_hwnd, ProfileNameControl, EM_SETSEL,
                    0, 0);
                return  TRUE;
            }
            break;
    }

    return  FALSE;
}

//  CAdvancedPage member functions

CAdvancedPage::CAdvancedPage(CProfilePropertySheet *pcpps) :
    CDialog(pcpps -> Instance(), AdvancedPage, pcpps -> Window()), 
        m_cppsBoss(*pcpps){ }

//  Class destructor

CAdvancedPage::~CAdvancedPage() {}

//  Update private function- fill the list box, and set all the buttons
//  properly.

void    CAdvancedPage::Update() {

    //  Add the associations to the list

    SendDlgItemMessage(m_hwnd, DeviceListControl, LB_RESETCONTENT, 0, 0);

    for (unsigned u = m_cppsBoss.AssociationCount(); u--; ) {
        int iItem = SendDlgItemMessage(m_hwnd, DeviceListControl, LB_ADDSTRING,
            0, (LPARAM) m_cppsBoss.DisplayName(u));
        SendDlgItemMessage(m_hwnd, DeviceListControl, LB_SETITEMDATA, iItem,
            (LPARAM) m_cppsBoss.Association(u));
    }

    //  If there are no associations, disable the Remove Devices button

    EnableWindow(GetDlgItem(m_hwnd, RemoveDeviceButton), 
        m_cppsBoss.Profile().AssociationCount());

    //  If there are no devices, or all are associated, disable the Add 
    //  Devices button.

    EnableWindow(GetDlgItem(m_hwnd, AddDeviceButton), 
        m_cppsBoss.Profile().DeviceCount() && 
            m_cppsBoss.Profile().DeviceCount() > m_cppsBoss.AssociationCount());
}

//  OnInit function- this initializes the property page

BOOL    CAdvancedPage::OnInit() {

    SetDlgItemText(m_hwnd, ProfileNameControl, m_cppsBoss.Profile().GetName());

    //  Add the associations to the list, etc.

    Update();
    return  TRUE;
}

//  OnCommand override- handles control notifications

BOOL    CAdvancedPage::OnCommand(WORD wNotifyCode, WORD wid, HWND hwndCtl) {

    switch  (wid) {

        case    AddDeviceButton:

            if  (wNotifyCode == BN_CLICKED) {
                CAddDeviceDialog    cadd(m_cppsBoss, m_hwnd);
                Update();
                return  TRUE;
            }
            break;

        case    RemoveDeviceButton:

            if  (wNotifyCode == BN_CLICKED) {

                int i = SendDlgItemMessage(m_hwnd, DeviceListControl,
                    LB_GETCURSEL, 0, 0);

                if  (i == -1)
                    return  TRUE;

                unsigned uItem = SendDlgItemMessage(m_hwnd, DeviceListControl,
                    LB_GETITEMDATA, i, 0);
                m_cppsBoss.Dissociate(uItem);
                Update();
                return  TRUE;
            }
            break;

        case    DeviceListControl:
            switch  (wNotifyCode) {

                case    LBN_SELCHANGE:
                    EnableWindow(GetDlgItem(m_hwnd, RemoveDeviceButton),
                        -1 != SendMessage(hwndCtl, LB_GETCURSEL, 0, 0));
                    return  TRUE;
            }
            break;
    }

    return  FALSE;
}


//  CAddDeviceDialog class constructor

CAddDeviceDialog::CAddDeviceDialog(CProfilePropertySheet& cpps, 
                                   HWND hwndParent) : 
    CDialog(cpps.Instance(), AddDeviceDialog, hwndParent), m_cppsBoss(cpps) {
    DoModal();
}

//  Dialog Initialization routine

BOOL    CAddDeviceDialog::OnInit() {

    CProfile&   cpThis = m_cppsBoss.Profile();
    m_hwndList = GetDlgItem(m_hwnd, DeviceListControl);
    m_hwndButton = GetDlgItem(m_hwnd, AddDeviceButton);

    //  This must not list associated (tentatively) devices, per the spec

    for (unsigned uDevice = 0; uDevice < cpThis.DeviceCount(); uDevice++) {
        for (unsigned u = 0; u < m_cppsBoss.AssociationCount(); u++)
            if  (uDevice == m_cppsBoss.Association(u))
                break;
        if  (u < m_cppsBoss.AssociationCount())
            continue;   //  Don't insert this one...
        int idItem = SendMessage(m_hwndList, LB_ADDSTRING, 0, 
            (LPARAM) cpThis.DisplayName(uDevice));
        SendMessage(m_hwndList, LB_SETITEMDATA, idItem, (LPARAM) uDevice);
    }

    if  (SendMessage(m_hwndList, LB_GETCOUNT, 0, 0))
        SendMessage(m_hwndList, LB_SETCURSEL, 0, 0);
    
    EnableWindow(m_hwndButton, -1 !=  
        SendMessage(m_hwndList, LB_GETCURSEL, 0, 0));
    return  TRUE;
}

//  Dialog notification handler

BOOL    CAddDeviceDialog::OnCommand(WORD wNotification, WORD wid, 
                                    HWND hwndControl){

    switch  (wNotification) {

        case    LBN_SELCHANGE:
            EnableWindow(m_hwndButton, -1 != 
                SendMessage(m_hwndList, LB_GETCURSEL, 0, 0));
            return  TRUE;

        case    BN_CLICKED:
            if  (wid == AddDeviceButton) {

                int i = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

                if  (i == -1)
                    return  TRUE;

                unsigned uItem = (unsigned) SendMessage(m_hwndList, 
                    LB_GETITEMDATA, i, 0);
                m_cppsBoss.Associate(uItem);
            }
            break;

        case    LBN_DBLCLK: 
            return  OnCommand(BN_CLICKED, AddDeviceButton, m_hwndButton);
    }

    return  CDialog::OnCommand(wNotification, wid, hwndControl);
}
