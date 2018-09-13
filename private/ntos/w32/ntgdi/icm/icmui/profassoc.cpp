/******************************************************************************

  Source File:  Profile Association Page.CPP

  Copyright (c) 1997 by Microsoft Corporation

  Change History:

  05-09-1997 hideyukn - Created

******************************************************************************/

#include    "ICMUI.H"

#include    "Resource.H"

//  It looks like the way to make the icon draw is to subclass the Icon control
//  in the window.  So, here's a Window Procedure for the subclass

CONST DWORD ProfileAssociationUIHelpIds[] = {
    AddButton,             IDH_ASSOCDEVICE_ADD,
    RemoveButton,          IDH_ASSOCDEVICE_REMOVE,
#if !defined(_WIN95_) // context-sentitive help
    ProfileFilename,       IDH_ASSOCDEVICE_NAME,
    DeviceListControl,     IDH_ASSOCDEVICE_LIST,
    DeviceListControlText, IDH_ASSOCDEVICE_LIST,
    StatusIcon,            IDH_DISABLED,
#endif
    0, 0
};

//  CProfileAssociationPage member functions

//  Class Constructor

CProfileAssociationPage::CProfileAssociationPage(HINSTANCE hiWhere,
                                                 LPCTSTR lpstrTarget) {
    m_pcpTarget = NULL;
    m_csProfile = lpstrTarget;
    m_psp.dwSize = sizeof m_psp;
    m_psp.dwFlags |= PSP_USETITLE;
    m_psp.hInstance = hiWhere;
    m_psp.pszTemplate = MAKEINTRESOURCE(AssociateDevicePage);
    m_psp.pszTitle = MAKEINTRESOURCE(AssociatePropertyString);
}

//  Class destructor

CProfileAssociationPage::~CProfileAssociationPage() {
    if (m_pcpTarget) {
        delete m_pcpTarget;
    }
}

//  Dialog box (property sheet) initialization

BOOL    CProfileAssociationPage::OnInit() {

    m_pcpTarget = new CProfile(m_csProfile);

    if (m_pcpTarget) {

        // Set profile filename

        SetDlgItemText(m_hwnd, ProfileFilename, m_pcpTarget->GetName());

        // Update ICON to show installed/non-installed status.

        HICON hIcon = LoadIcon(CGlobals::Instance(),
                               MAKEINTRESOURCE(m_pcpTarget->IsInstalled() ? DefaultIcon : UninstalledIcon));

        if (hIcon) {
            SendDlgItemMessage(m_hwnd, StatusIcon, STM_SETICON, (WPARAM) hIcon, 0);
        }

        // Clean up add/delete list.

        m_cuaAdd.Empty();
        m_cuaDelete.Empty();

        // Build tentitive association list.

        ConstructAssociations();

        // And then, fill up device listbox

        UpdateDeviceListBox();

        // And set focus on AddButton.

        SetFocus(GetDlgItem(m_hwnd,AddButton));

        DisableApplyButton();
        SettingChanged(FALSE);

        return TRUE;

    } else {
        return FALSE;
    }
}

//  Private ConstructAssociations function- this constructs the list of
//  tentative associations- this starts out as the true list from the profile
//  object.

VOID    CProfileAssociationPage::ConstructAssociations() {

    m_cuaAssociate.Empty(); //  Clean it up!

    for (unsigned u = 0; u < m_pcpTarget->AssociationCount(); u++) {

        for (unsigned uDelete = 0;
             uDelete < m_cuaDelete.Count();
             uDelete++) {
            if  (m_cuaDelete[uDelete] == m_pcpTarget->Association(u))
                break;
        }

        if  (uDelete == m_cuaDelete.Count())    //  Not yet deleted
            m_cuaAssociate.Add(m_pcpTarget->Association(u));
    }

    //  Now, add any added associations

    for (u = 0; u < m_cuaAdd.Count(); u++)
        m_cuaAssociate.Add(m_cuaAdd[u]);
}

//  Public method for noting tentative associations to be added

void    CProfileAssociationPage::Associate(unsigned uAdd) {

    //  First, see if it's on the delete list.  If it is, remove it from there
    //  Otherwise, add us to the add list, if it's a new association.

    for (unsigned u = 0; u < m_cuaDelete.Count(); u++)
        if  (uAdd == m_cuaDelete[u])
            break;

    if  (u < m_cuaDelete.Count())
        m_cuaDelete.Remove(u);
    else {
        for (u = 0; u < m_cuaAssociate.Count(); u++)
            if  (m_cuaAssociate[u] == uAdd)
                break;
        if  (u == m_cuaAssociate.Count())
            m_cuaAdd.Add(uAdd);
    }

    DeviceListChanged();
}

//  Public Method for removing tentative associations

void    CProfileAssociationPage::Dissociate(unsigned uRemove) {

    //  First, see if it's on the add list.  If it is, remove it from there
    //  Otherwise, add us to the delete list.

    for (unsigned u = 0; u < m_cuaAdd.Count(); u++)
        if  (uRemove == m_cuaAdd[u])
            break;

    if  (u < m_cuaAdd.Count())
        m_cuaAdd.Remove(u);
    else
        m_cuaDelete.Add(uRemove);

    DeviceListChanged();
}

VOID    CProfileAssociationPage::UpdateDeviceListBox() {

    //  Add the associations to the list

    SendDlgItemMessage(m_hwnd, DeviceListControl, LB_RESETCONTENT, 0, 0);

    for (unsigned u = 0; u < AssociationCount(); u++ ) {
        LRESULT iItem = SendDlgItemMessage(m_hwnd, DeviceListControl, LB_ADDSTRING,
            0, (LPARAM) DisplayName(u));
        SendDlgItemMessage(m_hwnd, DeviceListControl, LB_SETITEMDATA, iItem,
            (LPARAM) Association(u));
    }

    //  If there are no associations, disable the Remove Devices button

    HWND hwndRemove = GetDlgItem(m_hwnd,RemoveButton);

    //  If there are no more devices, or all are associated, disable the Add
    //  Devices button.
    //
    //  !!! To get more performance !!!
    //
    //  EnableWindow(GetDlgItem(m_hwnd, AddButton),
    //     m_pcpTarget->DeviceCount() && m_pcpTarget->DeviceCount() > AssociationCount());
    //
    //  !!! AddButton NEVER DISABLED !!!
    //

    // If focus is on Remove, move it to Add button.

    if (GetFocus() == hwndRemove) {

        HWND hwndAdd = GetDlgItem(m_hwnd, AddButton);

        SetFocus(hwndAdd);
        SendMessage(hwndRemove, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
        SendMessage(hwndAdd, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
    }

    EnableWindow(hwndRemove, !!(AssociationCount()));

    //  If there is any device, set focus to 1st entry.

    if  (SendDlgItemMessage(m_hwnd, DeviceListControl, LB_GETCOUNT, 0, 0))
        SendDlgItemMessage(m_hwnd, DeviceListControl, LB_SETCURSEL, 0, 0);
}

BOOL    CProfileAssociationPage::OnCommand(WORD wNotifyCode, WORD wid, HWND hwndControl) {

    switch (wid) {

        case AddButton :
            if  (wNotifyCode == BN_CLICKED) {
                CAddDeviceDialog cadd(this, m_hwnd);
                if (!cadd.bCanceled()) {
                    UpdateDeviceListBox();
                    EnableApplyButton();
                    SettingChanged(TRUE);
                }
                return  TRUE;
            }
            break;

        case RemoveButton :
            if  (wNotifyCode == BN_CLICKED) {
                LRESULT i = SendDlgItemMessage(m_hwnd, DeviceListControl,
                    LB_GETCURSEL, 0, 0);

                if  (i == -1)
                    return  TRUE;

                unsigned uItem = (unsigned)SendDlgItemMessage(m_hwnd, DeviceListControl,
                    LB_GETITEMDATA, i, 0);
                Dissociate(uItem);
                UpdateDeviceListBox();
                EnableApplyButton();
                SettingChanged(TRUE);
                return  TRUE;
            }
            break;

        case DeviceListControl :
            if (wNotifyCode == LBN_SELCHANGE) {
                EnableWindow(GetDlgItem(m_hwnd, RemoveButton),
                    -1 != SendDlgItemMessage(m_hwnd, DeviceListControl, LB_GETCURSEL, 0, 0));
                return  TRUE;
            }
            break;
    }

    return TRUE;
}

BOOL    CProfileAssociationPage::OnDestroy() {

    if (m_pcpTarget) {
        delete m_pcpTarget;
        m_pcpTarget = (CProfile *) NULL;
    }

    return FALSE;  // still need to handle this message by def. proc.
}

//  Common control notification override

BOOL    CProfileAssociationPage::OnNotify(int idCtrl, LPNMHDR pnmh) {

    switch  (pnmh -> code) {

        case PSN_APPLY: {

            if (SettingChanged()) {

                //  Disable apply button, because current request are
                //  going to be "Applied".

                DisableApplyButton();

                //  We are going to update changed.

                SettingChanged(FALSE);

                //  Remove any associations we're removing

                while   (m_cuaDelete.Count()) {
                    m_pcpTarget->Dissociate(m_pcpTarget->DeviceName(m_cuaDelete[0]));
                    m_cuaDelete.Remove(0);
                }

                //  Add any associations we're adding

                while   (m_cuaAdd.Count()) {
                    m_pcpTarget->Associate(m_pcpTarget->DeviceName(m_cuaAdd[0]));
                    m_cuaAdd.Remove(0);
                }

                //  Re-create CProfile object.
                //
                //  !!! Need to be done some performance work here.

                delete m_pcpTarget;
                m_pcpTarget = new CProfile(m_csProfile);

                if (!m_pcpTarget)
                {
                    // BUGBUG proper error should happen.

                    return FALSE;
                }

                //  Re-Build tentitive association list.

                ConstructAssociations();

                UpdateDeviceListBox();

                //  check the install status to refect icon.

                HICON hIcon = LoadIcon(CGlobals::Instance(),
                                   MAKEINTRESOURCE(m_pcpTarget->IsInstalled() ? DefaultIcon : UninstalledIcon));

                if (hIcon) {
                    SendDlgItemMessage(m_hwnd, StatusIcon, STM_SETICON, (WPARAM) hIcon, 0);
                }
            }

            break;
        }
    }

    return TRUE;
}

//  Context-sensitive help handler

BOOL    CProfileAssociationPage::OnHelp(LPHELPINFO pHelp) {

    if (pHelp->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) pHelp->hItemHandle, WINDOWS_HELP_FILE,
                HELP_WM_HELP, (ULONG_PTR) (LPSTR) ProfileAssociationUIHelpIds);
    }

    return (TRUE);
}

BOOL    CProfileAssociationPage::OnContextMenu(HWND hwnd) {

    DWORD iCtrlID = GetDlgCtrlID(hwnd);

    WinHelp(hwnd, WINDOWS_HELP_FILE,
            HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) ProfileAssociationUIHelpIds);

    return (TRUE);
}

//  Context Help for AddDevice Dialog.

CONST DWORD AddDeviceUIHelpIds[] = {
    AddDeviceButton,       IDH_ADDDEVICEUI_ADD,
    DeviceListControl,     IDH_ADDDEVICEUI_LIST,
    DeviceListControlText, IDH_ADDDEVICEUI_LIST,
    0, 0
};

//  CAddDeviceDialog class constructor

CAddDeviceDialog::CAddDeviceDialog(CProfileAssociationPage *pcpas,
                                   HWND hwndParent) :
    CDialog(pcpas->Instance(), AddDeviceDialog, hwndParent) {
    m_pcpasBoss = pcpas;
    m_bCanceled = TRUE;
    DoModal();
}

//  Dialog Initialization routine

BOOL    CAddDeviceDialog::OnInit() {

    CProfile * pcpThis = m_pcpasBoss->Profile();

    m_hwndList   = GetDlgItem(m_hwnd, DeviceListControl);
    m_hwndButton = GetDlgItem(m_hwnd, AddDeviceButton);

    //  This must not list associated (tentatively) devices, per the spec

    for (unsigned uDevice = 0; uDevice < pcpThis->DeviceCount(); uDevice++) {
        for (unsigned u = 0; u < m_pcpasBoss->AssociationCount(); u++)
            if  (uDevice == m_pcpasBoss->Association(u))
                break;
        if  (u < m_pcpasBoss->AssociationCount())
            continue;   //  Don't insert this one...

        LRESULT idItem = SendMessage(m_hwndList, LB_ADDSTRING, (WPARAM)0,
            (LPARAM) pcpThis->DisplayName(uDevice));
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

                LRESULT i = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

                if  (i == -1)
                    return  TRUE;

                unsigned uItem = (unsigned) SendMessage(m_hwndList,
                    LB_GETITEMDATA, i, 0);

                m_pcpasBoss->Associate(uItem);

                // Selection has been made.

                m_bCanceled = FALSE;
            }
            break;

        case    LBN_DBLCLK:
            return  OnCommand(BN_CLICKED, AddDeviceButton, m_hwndButton);
    }

    return  CDialog::OnCommand(wNotification, wid, hwndControl);
}

//  Context-sensitive help handler

BOOL    CAddDeviceDialog::OnHelp(LPHELPINFO pHelp) {

    if (pHelp->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) pHelp->hItemHandle, WINDOWS_HELP_FILE,
                HELP_WM_HELP, (ULONG_PTR) (LPSTR) AddDeviceUIHelpIds);
    }

    return (TRUE);
}

BOOL    CAddDeviceDialog::OnContextMenu(HWND hwnd) {

    DWORD iCtrlID = GetDlgCtrlID(hwnd);

    WinHelp(hwnd, WINDOWS_HELP_FILE,
            HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) AddDeviceUIHelpIds);

    return (TRUE);
}


