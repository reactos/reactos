/******************************************************************************

  Source File:  Profile Property Sheet.CPP

  This implements the code for the profile property sheet.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:

  11-01-96  a-robkj@microsoft.com- original version

******************************************************************************/

#include    "ICMUI.H"

#include    "Resource.H"

//  Private ConstructAssociations function- this constructs the list of
//  tentative associations- this starts out as the true list from the profile
//  object, but reflects all adds and deletes made on the Advanced page.

void    CProfilePropertySheet::ConstructAssociations() {

    m_cuaAssociate.Empty(); //  Clean it up!

    for (unsigned u = 0; u < m_cpTarget.AssociationCount(); u++) {
        for (unsigned uDelete = 0; 
             uDelete < m_cuaDelete.Count(); 
             uDelete++)

            if  (m_cuaDelete[uDelete] == m_cpTarget.Association(u))
                break;

        if  (uDelete == m_cuaDelete.Count())    //  Not yet deleted
            m_cuaAssociate.Add(m_cpTarget.Association(u));
    }

    //  Now, add any added associations

    for (u = 0; u < m_cuaAdd.Count(); u++)
        m_cuaAssociate.Add(m_cuaAdd[u]);
}


//  Class constructor- we use one dialog for the installed case, and another
//  for the uninstalled one.  This traded code for resource size, since the
//  two forms are similar enough I can use the same code to handle both.

//  This is a tricky constructor- it actually presents the dialog when the
//  instance is constructed.

CProfilePropertySheet::CProfilePropertySheet(HINSTANCE hiWhere,
                                            CProfile& cpTarget) :
    CDialog(hiWhere, 
        cpTarget.IsInstalled() ? UninstallInterface : InstallInterface), 
        m_cpTarget(cpTarget) {

    if  (!cpTarget.IsValid()) {
        for (int i = 0; i < sizeof m_pcdPage / sizeof m_pcdPage[0]; i++)
            m_pcdPage[i] = NULL;

        CGlobals::Report(InvalidProfileString, m_hwndParent);
        return;
    }

    m_bDelete = FALSE;
    ConstructAssociations();

    DoModal();
}

//  Class destructor- we have to get rid of the individual pages

CProfilePropertySheet::~CProfilePropertySheet() {
    for (int i = 0; i < sizeof m_pcdPage / sizeof (m_pcdPage[0]); i++)
        if  (m_pcdPage[i])
            delete  m_pcdPage[i];
}

//  Public method for noting tentative associations to be added

void    CProfilePropertySheet::Associate(unsigned uAdd) {
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

    ConstructAssociations();
}

//  Public Method for removing tentative associations

void    CProfilePropertySheet::Dissociate(unsigned uRemove) {
    //  First, see if it's on the add list.  If it is, remove it from there
    //  Otherwise, add us to the delete list.

    for (unsigned u = 0; u < m_cuaAdd.Count(); u++)
        if  (uRemove == m_cuaAdd[u])
            break;

    if  (u < m_cuaAdd.Count())
        m_cuaAdd.Remove(u);
    else
        m_cuaDelete.Add(uRemove);

    ConstructAssociations();
}

//  Dialog Initialization override

BOOL    CProfilePropertySheet::OnInit() {

    CString csWork;
    TC_ITEM tciThis = {TCIF_TEXT, 0, 0, NULL, 0, -1, 0};

    //  We'll begin by determining the bounding rectangle of the client
    //  area of the tab control

    RECT rcWork;

    GetWindowRect(GetDlgItem(m_hwnd, TabControl), &m_rcTab);
    GetWindowRect(m_hwnd, &rcWork);
    OffsetRect(&m_rcTab, -rcWork.left, -rcWork.top);
    SendDlgItemMessage(m_hwnd, TabControl, TCM_ADJUSTRECT, FALSE, 
        (LPARAM) &m_rcTab);

    //  Then, we create the classes for the two descendants, and
    //  initialize the first dialog.
    
    m_pcdPage[0] = new CInstallPage(this);
    m_pcdPage[1] = new CAdvancedPage(this);
    m_pcdPage[0] -> Create();
    m_pcdPage[0] -> Adjust(m_rcTab);
    
    //  Then, initialize the tab control

    csWork.Load(m_cpTarget.IsInstalled() ? 
        ShortUninstallString : ShortInstallString);

    tciThis.pszText = const_cast<LPTSTR>((LPCTSTR) csWork);

    SendDlgItemMessage(m_hwnd, TabControl, TCM_INSERTITEM, 0, 
        (LPARAM) &tciThis);

    csWork.Load(AdvancedString);

    tciThis.pszText = const_cast<LPTSTR>((LPCTSTR) csWork);

    SendDlgItemMessage(m_hwnd, TabControl, TCM_INSERTITEM, 1, 
        (LPARAM) &tciThis);

    //  Finally, let's set the icons for this little monster...

    HICON   hi = LoadIcon(m_hiWhere, 
        MAKEINTRESOURCE(m_cpTarget.IsInstalled() ? 
            DefaultIcon : UninstalledIcon));

    SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM) hi);

    return  TRUE;   //  We've not set the focus anywhere
}

//  Common control notification override

BOOL    CProfilePropertySheet::OnNotify(int idCtrl, LPNMHDR pnmh) {

    int iPage = !!SendMessage(pnmh -> hwndFrom, TCM_GETCURSEL, 0, 0);

    switch  (pnmh -> code) {
        case    TCN_SELCHANGING:

            m_pcdPage[iPage] -> Destroy();
            return  FALSE;      //  Allow the selection to change.

        case    TCN_SELCHANGE:

            //  Create the appropriate dialog

            m_pcdPage[iPage] -> Create();
            m_pcdPage[iPage] -> Adjust(m_rcTab);

            return  TRUE;
    }

    return  TRUE;   //  Claim to have handled it (perhaps a bit bogus).
}

//  Control Notification override

BOOL    CProfilePropertySheet::OnCommand(WORD wNotifyCode, WORD wid, 
                                         HWND hwndControl) {
    switch  (wid) {

        case    IDOK:

            if  (wNotifyCode == BN_CLICKED && !m_cpTarget.IsInstalled())
                m_cpTarget.Install();
            //  Remove any associations we're removing
            while   (m_cuaDelete.Count()) {
                m_cpTarget.Dissociate(m_cpTarget.Device(m_cuaDelete[0]));
                m_cuaDelete.Remove(0);
            }
            //  Add any associations we're adding
            while   (m_cuaAdd.Count()) {
                m_cpTarget.Associate(m_cpTarget.Device(m_cuaAdd[0]));
                m_cuaAdd.Remove(0);
            }
            break;

        case    UninstallButton:

            if  (wNotifyCode == BN_CLICKED)
                m_cpTarget.Uninstall(m_bDelete);
            break;
    }
    
    return  CDialog::OnCommand(wNotifyCode, wid, hwndControl);
}
