/******************************************************************************

  Source File:  Device Profile Management .CPP

  Change History:


  Implements the class which provides the various device profile management UI

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises, Inc. Production
  11-27-96  a-RobKj@microsoft.com coded it

******************************************************************************/

#include    "ICMUI.H"

//
// This function is obtained from the April 1998 Knowledge Base
// Its purpose is to determine if the current user is an
// Administrator and therefore priveledged to change profile
// settings.
//
// BOOL IsAdmin(void)
//
//      returns TRUE if user is an admin
//              FALSE if user is not an admin
//

#if defined(_WIN95_)

//
// Always administrator on Windows 9x platform.
//

BOOL IsAdmin(void) {

    return (TRUE);
}

#else

BOOL IsAdmin(void)
{
    BOOL   fReturn = FALSE;
    PSID   psidAdmin;
    
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    
    if ( AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        if(!CheckTokenMembership( NULL, psidAdmin, &fReturn )) {

          //
          // explicitly disallow Admin Access if CheckTokenMembership fails.
          // 

          fReturn = FALSE;
        }
        FreeSid ( psidAdmin);
    }
    
    return ( fReturn );
}

#endif // _WIN95_

/******************************************************************************

  List Managment functions

  The method used here is simlar to that used in the profile Managment sheets
  for managing device associations.

  We (not royal, I mean the OS and I) manage two "to-do" lists and use these
  to show the user the anticipated result of these lists being applied.  The
  first list is a list of existing associations that are to be broken.  The
  second is one of new associations to be made.  This is coupled with a list
  of the current associations.

  The "Removals" list is the indices of existing associations which will be
  borken.  The "Adds" list is a list of new profiles to be associated.  The
  "Profiles" list is the list of existing associations.

  Adding and removing profiles could mean removing an item from one of the work
  lists (undoing a previous selection), or adding one.  Each time such a change
  is made, the profile list box is emptied and refilled.  This lets us avoid
  mapping removals and additions more directly.

  When changes are commited, either with Apply or OK, we make or break
  associations as specified, then empty all of the lists, and rebuild the list
  of current associations.

  We use the ITEMDATA of the UI list box to handle the associations.  This lets
  the list remain sorted.

  All of the list management functions can be overriden, if needed.

******************************************************************************/

void    CDeviceProfileManagement::InitList() {

    //  Make sure the lists are empty.

    m_cuaRemovals.Empty();
    m_cpaAdds.Empty();

    //  Determine the associations for the target device.

    ENUMTYPE    et = {sizeof et, ENUM_TYPE_VERSION, ET_DEVICENAME, m_csDevice};

    CProfile::Enumerate(et, m_cpaProfile);
}

//  Fill the UI list of profiles

void    CDeviceProfileManagement::FillList(DWORD dwFlags) {

    //  Before reset list box, get current selection to restore later.

    CString csSelect;

    csSelect.Empty();

    LRESULT idSelect = LB_ERR;

    if ( !(dwFlags & DEVLIST_NOSELECT)) {

        //  Get current selected position.

        idSelect = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

        //  Get text length where currently selected, than allocate buffer for that.

        DWORD   dwLen = (DWORD) SendMessage(m_hwndList, LB_GETTEXTLEN, idSelect, 0);
        TCHAR  *pszSelect = new TCHAR[dwLen + 1];

        //  Get text itself.
    
        if (pszSelect != NULL) {

            if (SendMessage(m_hwndList, LB_GETTEXT, idSelect, (LPARAM) pszSelect) != LB_ERR) {
                csSelect = pszSelect;
            }

            delete pszSelect;
        }
    }

    //  reset list box

    SendMessage(m_hwndList, LB_RESETCONTENT, 0, 0);

    //  Fill the profile list box from the list of profiles

    for (unsigned u = 0; u < m_cpaProfile.Count(); u++) {

        //  Don't list profiles tentatively disassociated...

        for (unsigned uOut = 0; uOut < m_cuaRemovals.Count(); uOut++)
            if  (m_cuaRemovals[uOut] == u)
                break;

        if  (uOut < m_cuaRemovals.Count())
            continue;   //  Don't add this to list, it's been zapped!

        LRESULT id = SendMessage(m_hwndList, LB_ADDSTRING, 0,
            (LPARAM) (LPCTSTR) m_cpaProfile[u] -> GetName());

        SendMessage(m_hwndList, LB_SETITEMDATA, id, u);
    }

    //  Add the profiles that have been tentatively added...

    for (u = 0; u < m_cpaAdds.Count(); u ++) {
        LRESULT id = SendMessage(m_hwndList, LB_ADDSTRING, 0,
            (LPARAM) (LPCTSTR) m_cpaAdds[u] -> GetName());
        SendMessage(m_hwndList, LB_SETITEMDATA, id, u + m_cpaProfile.Count());
    }

    //  If we have any profiles, select the first one
    //  Otherwise, disable the "Remove" button, as there's nothing to remove

    unsigned itemCount = (m_cpaProfile.Count() + m_cpaAdds.Count() - m_cuaRemovals.Count());

    if  (itemCount) {

        // The Remove button must remain disabled
        // unless the user is Administrator.
        // This code is specific to the Monitor Profile
        // Property sheet.

        if (!m_bReadOnly) {
            EnableWindow(GetDlgItem(m_hwnd, RemoveButton), TRUE);
        }

        if ( !(dwFlags & DEVLIST_NOSELECT)) {

            // Find out the string selected previously.

            idSelect = LB_ERR;

            if (!csSelect.IsEmpty()) {
                idSelect = SendMessage(m_hwndList, LB_FINDSTRINGEXACT,
                                       (WPARAM) -1, (LPARAM) (LPCTSTR) csSelect);
            }

            // if could not find, just select first item.

            if (idSelect == LB_ERR) {
                idSelect = 0;
            }

            // Select it.

            SendMessage(m_hwndList, LB_SETCURSEL, idSelect, 0);
        }

    } else {

        HWND hwndRemove = GetDlgItem(m_hwnd, RemoveButton);

        // If focus is on Remove, move it to Add button.

        if (GetFocus() == hwndRemove) {

            HWND hwndAdd = GetDlgItem(m_hwnd, AddButton);

            SetFocus(hwndAdd);
            SendMessage(hwndRemove, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
            SendMessage(hwndAdd, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
        }

        EnableWindow(hwndRemove, FALSE);
    }

    // Apply button needs to remain disabled unless the
    // user has permision to make changes - ie. user
    // is Administrator.

    if  ((dwFlags & DEVLIST_CHANGED) && !(m_bReadOnly)) {
        EnableApplyButton();
        SettingChanged(TRUE);
    }
}

void CDeviceProfileManagement::GetDeviceTypeString(DWORD dwType,CString& csDeviceName) {

    DWORD id;

    switch (dwType) {

    case CLASS_MONITOR :
        id = ClassMonitorString;
        break;
    case CLASS_PRINTER :
        id = ClassPrinterString;
        break;
    case CLASS_SCANNER :
        id = ClassScannerString;
        break;
    case CLASS_LINK :
        id = ClassLinkString;
        break;
    case CLASS_ABSTRACT :
        id = ClassAbstractString;
        break;
    case CLASS_NAMED :
        id = ClassNamedString;
        break;
    case CLASS_COLORSPACE :
    default :
        id = ClassColorSpaceString;
        break;
    }

    // Load string.

    csDeviceName.Load(id);
}

//  Constructor

CDeviceProfileManagement::CDeviceProfileManagement(LPCTSTR lpstrDevice,
                                                   HINSTANCE hiWhere,
                                                   int idPage, DWORD dwType) {
	m_csDevice = lpstrDevice;
    m_dwType = dwType;
    m_psp.hInstance = hiWhere;
    m_psp.pszTemplate = MAKEINTRESOURCE(idPage);

    //  Setting m_bReadOnly to false enables functionality

    m_bReadOnly = FALSE;        // default setting is false

#if defined(_WIN95_)

    //
    // There is no way to detect printer supports CMYK or not on Win 9x.

    m_bCMYK = TRUE;

#else

    //  we need to check the device capabilities
    //  and determine if we're trying to associate
    //  a cmyk printer profile to a printer that
    //  doesn't support it.

    m_bCMYK = FALSE;            // default setting - don't support cmyk

    //  if the device is a printer

    if (m_dwType == CLASS_PRINTER) {

        HDC hdcThis = CGlobals::GetPrinterHDC(m_csDevice);

        //  if the printer supports CMYK

        if (hdcThis) {
            if (GetDeviceCaps(hdcThis, COLORMGMTCAPS) & CM_CMYK_COLOR) {
                m_bCMYK = TRUE;
            }
            DeleteDC(hdcThis);
        }
    }

#endif // defined(_WIN95_)
}

//  UI initialization

BOOL    CDeviceProfileManagement::OnInit() {

    InitList();

    m_hwndList = GetDlgItem(m_hwnd, ProfileListControl);

    //  Fill the profile list box

    FillList(DEVLIST_ONINIT);

    //  Disable apply button as default.

    DisableApplyButton();

    //  Nothing changed, yet.

    SettingChanged(FALSE);

    return  TRUE;
}

//  Command processing

BOOL    CDeviceProfileManagement::OnCommand(WORD wNotifyCode, WORD wid,
                                             HWND hwndCtl) {

    switch  (wNotifyCode) {

        case    BN_CLICKED:

            switch  (wid) {

                case    AddButton: {

                    unsigned i = 0, u = 0;

                    //  Time to do the old OpenFile dialog stuff...

                    CAddProfileDialog capd(m_hwnd, m_psp.hInstance);

                    //  See if a profile was selected

                    while(i < capd.ProfileCount()) {

                        //  Check profile validity and device type

                        CProfile cpTemp(capd.ProfileName(i));

                        //  CLASS_COLORSPACE and CLASS_MONITOR can be associated to
                        //  any device. Other (CLASS_SCANNER, CLASS_PRINTER) only
                        //  can be associated to much device.

                        if (    !cpTemp.IsValid() // Wrong profile type or invalid?
                             || (   cpTemp.GetType() != m_dwType
                                 && cpTemp.GetType() != CLASS_COLORSPACE
                        #if 1 // ALLOW_MONITOR_PROFILE_TO_ANY_DEVICE
                                 && cpTemp.GetType() != CLASS_MONITOR
                        #endif
                                )
                           ) {

                            //  Throw up a message box to inform the user of this

                            if (cpTemp.IsValid())
                            {
                                CString csDeviceType;  GetDeviceTypeString(m_dwType,csDeviceType);
                                CString csProfileType; GetDeviceTypeString(cpTemp.GetType(),csProfileType);

                                CGlobals::ReportEx(MismatchDeviceType, m_hwnd, FALSE,
                                               MB_OK|MB_ICONEXCLAMATION, 3,
                                               (LPTSTR)capd.ProfileNameAndExtension(i),
                                               (LPTSTR)csProfileType,
                                               (LPTSTR)csDeviceType);
                            }
                            else
                            {
                                CGlobals::ReportEx(InstFailedWithName, m_hwnd, FALSE,
                                               MB_OK|MB_ICONEXCLAMATION, 1,
                                               (LPTSTR)capd.ProfileNameAndExtension(i));
                            }

                            goto SkipToNext;
                        }

                        //  See if the profile has already been listed for addition

                        for (u = 0; u < m_cpaAdds.Count(); u++) {
                            if  (!lstrcmpi(m_cpaAdds[u] -> GetName(), cpTemp.GetName())) {
                                goto SkipToNext; //  This profile is already added
                            }
                        }

                        //  If this profile is on the existing list, either ignore
                        //  or zap it from the removal list, as the case may be

                        for (u = 0; u < m_cpaProfile.Count(); u++) {
                            if  (!lstrcmpi(m_cpaProfile[u] -> GetName(),
                                    cpTemp.GetName())) {
                                //  Is this one on the removal list?
                                for (unsigned uOut = 0;
                                     uOut < m_cuaRemovals.Count();
                                     uOut++) {
                                    if  (m_cuaRemovals[uOut] == u) {
                                        //  Was to be removed- undo that...
                                        m_cuaRemovals.Remove(uOut);
                                        FillList(DEVLIST_CHANGED);
                                        break;
                                    }
                                }
                                goto SkipToNext;
                            }   //  End of name in existing list
                        }

                        //  We need to check the device capabilities
                        //  and determine if we're trying to associate
                        //  a cmyk printer profile to a printer that
                        //  doesn't support it.

                        if  ((!m_bCMYK) && (cpTemp.GetColorSpace() == SPACE_CMYK)) {
                            CGlobals::ReportEx(UnsupportedProfile, m_hwnd, FALSE,
                                                MB_OK|MB_ICONEXCLAMATION, 2,
                                                (LPTSTR)m_csDevice,
                                                (LPTSTR)capd.ProfileNameAndExtension(i));
                            goto SkipToNext;
                        }

                        //  Add this profile to the list, item (max orig + index)

                        m_cpaAdds.Add(capd.ProfileName(i));

                        //  Change has been made, update the list

                        FillList(DEVLIST_CHANGED);
SkipToNext:
                        i++;
                    }

                    return  TRUE;
                }

                case    RemoveButton: {

                    //  Remove the selected profile

                    LRESULT id = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);
                    unsigned u = (unsigned) SendMessage(m_hwndList,
                        LB_GETITEMDATA, id, 0);

                    //  If this is a tentative add, just drop it, otherwise
                    //  note that it's been removed...

                    if  (u >= m_cpaProfile.Count())
                        m_cpaAdds.Remove(u - m_cpaProfile.Count());
                    else
                        m_cuaRemovals.Add(u);

                    //  That's it- just update the display, now...

                    FillList(DEVLIST_CHANGED);

                    // explicitly set the position of the current selection
                    // after the list has been recomputed.

                    int listsize = m_cpaProfile.Count()+m_cpaAdds.Count()-m_cuaRemovals.Count();
                    if (id >= listsize) id = listsize-1;
                    if (id < 0)         id = 0;
                    SendMessage(m_hwndList, LB_SETCURSEL, id, 0);

                    return  TRUE;
                }
            }
            break;

        case    LBN_SELCHANGE: {

            LRESULT id = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);
			
            if (id == -1) {
                EnableWindow(GetDlgItem(m_hwnd, RemoveButton), FALSE);
            } else {

                // The Remove button must remain disabled on a monitor
                // profile property page if the user isn't the
                // Administrator, otherwise enable remove button.

                EnableWindow(GetDlgItem(m_hwnd, RemoveButton), !m_bReadOnly);
            }

            return  TRUE;
        }
    }

    return  FALSE;
}

//  Property Sheet notification processing

BOOL    CDeviceProfileManagement::OnNotify(int idCtrl, LPNMHDR pnmh) {

    switch  (pnmh -> code) {

        case    PSN_APPLY:

            DisableApplyButton();

            if (SettingChanged()) {

                //  Apply the changes the user has made...

                SettingChanged(FALSE);

                while   (m_cpaAdds.Count()) {
                    if  (!m_cpaAdds[0] -> IsInstalled()) {
                        m_cpaAdds[0] -> Install();
                    }
                    m_cpaAdds[0] -> Associate(m_csDevice);
                    m_cpaAdds.Remove(0);
                }

                //  Now do the removals (actually just dissociations)

                while   (m_cuaRemovals.Count()) {
                    m_cpaProfile[m_cuaRemovals[0]] -> Dissociate(m_csDevice);
                    m_cuaRemovals.Remove(0);
                }

                InitList();
                FillList();

                SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            }

            return  TRUE;
    }

    return  FALSE;
}

//  This hook procedure both forces the use of the old-style common dialog
//  and changes the OK button to an Add button.  The actual button text is
//  a string resource, and hence localizable.

UINT_PTR APIENTRY CAddProfileDialog::OpenFileHookProc(HWND hDlg, UINT uMessage,
                                                  WPARAM wp, LPARAM lp) {
    switch  (uMessage) {

        case    WM_INITDIALOG: {

            CString csAddButton;

            OPENFILENAME    *pofn = (OPENFILENAME *) lp;

            csAddButton.Load(AddButtonText);

            SetDlgItemText(GetParent(hDlg), IDOK, csAddButton);
            return  TRUE;
        }
    }

    return  FALSE;
}

//  Once again, a constructor that actually does most of the work!

TCHAR gacColorDir[MAX_PATH] = _TEXT("\0");
TCHAR gacFilter[MAX_PATH]   = _TEXT("\0");

CAddProfileDialog::CAddProfileDialog(HWND hwndOwner, HINSTANCE hi) {

    TCHAR tempBuffer[MAX_PATH*10];

    // Empty the profile list.

    csa_Files.Empty();

    // Prepare file filter (if not yet).

    if (gacFilter[0] == NULL) {

        ULONG offset; /* 32bits is enough even for sundown */
        CString csIccFilter; CString csAllFilter;

        // If the filter is not built yet, build it here.

        csIccFilter.Load(IccProfileFilterString);
        csAllFilter.Load(AllProfileFilterString);
        offset = 0;
        lstrcpy(gacFilter+offset, csIccFilter);
        offset += lstrlen(csIccFilter)+1;
        lstrcpy(gacFilter+offset, TEXT("*.icm;*.icc"));
        offset += lstrlen(TEXT("*.icm;*.icc"))+1;
        lstrcpy(gacFilter+offset, csAllFilter);
        offset += lstrlen(csAllFilter)+1;
        lstrcpy(gacFilter+offset, TEXT("*.*"));
        offset += lstrlen(TEXT("*.*"))+1;
        *(gacFilter+offset) = TEXT('\0');
    }

    if (gacColorDir[0] == _TEXT('\0'))  {
        DWORD dwcbDir = MAX_PATH;
        GetColorDirectory(NULL, gacColorDir, &dwcbDir);
    }

    //  Time to do the old OpenFile dialog stuff...
    CString csTitle; csTitle.Load(AddProfileAssociation);

    //  Set initial filename as null.
    memset(tempBuffer, 0, sizeof tempBuffer);

    OPENFILENAME ofn = {
        sizeof ofn, hwndOwner, hi,
        gacFilter,
        NULL, 0, 1,
        tempBuffer, sizeof tempBuffer / sizeof tempBuffer[0],
        NULL, 0,
        gacColorDir,
        csTitle,
        OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ENABLEHOOK,
        0, 0,
        _TEXT("icm"),
        (LPARAM) this, OpenFileHookProc, NULL};

    if  (!GetOpenFileName(&ofn)) {
        if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL) {
            CGlobals::Report(TooManyFileSelected);
        }
    } else {
        if (tempBuffer[0] != TEXT('\0')) {

            TCHAR *pPath = tempBuffer;
            TCHAR *pFile = tempBuffer + lstrlen(tempBuffer) + 1;

            // remember the last access-ed directory.

            memset(gacColorDir, 0, sizeof pPath);
            memcpy(gacColorDir, pPath, ofn.nFileOffset*sizeof(TCHAR));

            if (*pFile) {
                TCHAR workBuffer[MAX_PATH];

                // This is multiple-selection
                // Work through the buufer to build profile file list.

                while (*pFile) {

                    lstrcpy(workBuffer,pPath);
                    lstrcat(workBuffer,TEXT("\\"));
                    lstrcat(workBuffer,pFile);

                    // Insert built profile pathname
                    AddProfile(workBuffer);

                    // Move on to next.
                    pFile = pFile + lstrlen(pFile) + 1;
                }
            }
            else {
                // Single selection case.
                AddProfile(pPath);

                #if HIDEYUKN_DBG
                MessageBox(NULL,pPath,TEXT(""),MB_OK);
                #endif
            }
        }
    }

    return;
}

//  Printer Profile Management

CONST DWORD PrinterUIHelpIds[] = {
    AddButton,              IDH_PRINTERUI_ADD,
    RemoveButton,           IDH_PRINTERUI_REMOVE,
    ProfileListControl,     IDH_PRINTERUI_LIST,

#if !defined(_WIN95_)
    ProfileListControlText, IDH_PRINTERUI_LIST,
    PrinterUIIcon,          IDH_DISABLED,
    DescriptionText,        IDH_DISABLED,
    DefaultButton,          IDH_PRINTERUI_DEFAULTBTN,
    AutoSelButton,          IDH_PRINTERUI_AUTOMATIC,
    AutoSelText,            IDH_PRINTERUI_AUTOMATIC,
    ManualSelButton,        IDH_PRINTERUI_MANUAL,
    ManualSelText,          IDH_PRINTERUI_MANUAL,
    DefaultProfileText,     IDH_PRINTERUI_DEFAULTTEXT,
    DefaultProfile,         IDH_PRINTERUI_DEFAULTTEXT,
#endif
    0, 0
};

//  Initialize lists override- call the base class, then set the default

void    CPrinterProfileManagement::InitList() {

    CDeviceProfileManagement::InitList();

    m_uDefault = m_cpaProfile.Count() ? 0 : (unsigned) -1;
}

//  Fill list override- write the correct default and call the base function

void    CPrinterProfileManagement::FillList(DWORD dwFlags) {

    //  If we are initializing list box, we want to put focus on
    //  "default" profile. here we won't below FillList set focus
    //  to first one.

    if (dwFlags & DEVLIST_ONINIT) {
        dwFlags |= DEVLIST_NOSELECT;
    }

    CDeviceProfileManagement::FillList(dwFlags);

    //  There is either no default profile, an existing profile is the
    //  default, or a newly selected one is.  Some people just like the
    //  selection operator.

    //  if there is only 1 profile in list box, we treat it as default profile.
	
    if (SendMessage(m_hwndList,LB_GETCOUNT,0,0) == 1) {
        m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, 0, 0);
    }

    if (m_uDefault == -1) {

        // There is no profile associated for this device.

        CString csNoProfile;
        csNoProfile.Load(NoProfileString);
        SetDlgItemText(m_hwnd, DefaultProfile, csNoProfile);     	

    } else {

        // If the default has been deleted, set default as last in list.

        if (m_uDefault >= (m_cpaProfile.Count() + m_cpaAdds.Count())) {

            m_uDefault = (m_cpaProfile.Count() + m_cpaAdds.Count()) - 1;
        }

        // Put default profile name in UI.

        CProfile *pcpDefault = (m_uDefault < m_cpaProfile.Count()) ? \
                                           m_cpaProfile[m_uDefault] : \
                                           m_cpaAdds[m_uDefault - m_cpaProfile.Count()];

        SetDlgItemText(m_hwnd, DefaultProfile, pcpDefault -> GetName());

        LRESULT idSelect = SendMessage(m_hwndList, LB_FINDSTRINGEXACT,
                                   (WPARAM) -1, (LPARAM) (LPCTSTR) pcpDefault -> GetName());

        // if could not find, just select first item.

        if (idSelect == LB_ERR) {
            idSelect = 0;
        }

        // Select it.

        SendMessage(m_hwndList, LB_SETCURSEL, idSelect, 0);
    }

    //  03-08-1997  Bob_Kjelgaard@Prodigy.Net   Memphis RAID 18420
    //  Disable the Default button if there aren't any profiles

    if (m_bManualMode && m_bAdminAccess) {
        EnableWindow(GetDlgItem(m_hwnd, DefaultButton),
                     m_cpaAdds.Count() + m_cpaProfile.Count() - m_cuaRemovals.Count());
    }
}

//  Printer Profile Management class constructor- doesn't need any individual
//  code at the moment.

CPrinterProfileManagement::CPrinterProfileManagement(LPCTSTR lpstrName,
                                                     HINSTANCE hiWhere) :
    CDeviceProfileManagement(lpstrName, hiWhere, PrinterUI, CLASS_PRINTER) {
}

//  This class overrides OnInit so it can disable the UI if the user lacks
//  authority to make changes.

BOOL    CPrinterProfileManagement::OnInit() {

    //  Call the base class routine first, as it does most of the work...

    CDeviceProfileManagement::OnInit();

    DWORD dwSize = sizeof(DWORD);

    //  Query current mode.

    if (!InternalGetDeviceConfig((LPCTSTR)m_csDevice, CLASS_PRINTER,
                                 MSCMS_PROFILE_ENUM_MODE, &m_bManualMode, &dwSize)) {

        //  Auto selection mode as default.

        m_bManualMode = FALSE;
    }

    //  Now, see if we have sufficient authority to administer the printer

    HANDLE  hPrinter;
    PRINTER_DEFAULTS    pd = {NULL, NULL, PRINTER_ACCESS_ADMINISTER};

    m_bAdminAccess  = TRUE;
    m_bLocalPrinter = TRUE;

    if  (OpenPrinter(const_cast<LPTSTR> ((LPCTSTR) m_csDevice), &hPrinter, &pd)) {

        //  We can administer the printer- proceed in the normal way.

#if !defined(_WIN95_)

        //  If the printer is "Network Printer", we don't allow user to install
        //  or uninstall color profile.

        BYTE  StackPrinterData[sizeof(PRINTER_INFO_4)+MAX_PATH*2];
        PBYTE pPrinterData = StackPrinterData;
        BOOL  bSuccess = TRUE;
        DWORD dwReturned;

        if (!GetPrinter(hPrinter, 4, pPrinterData, sizeof(StackPrinterData), &dwReturned)) {

            if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
                (pPrinterData = (PBYTE) LocalAlloc(LPTR, dwReturned))) {

                if (GetPrinter(hPrinter, 4, pPrinterData, dwReturned, &dwReturned)) {

                    bSuccess = TRUE;

                }
            }

        } else {

            bSuccess = TRUE;
        }
       
        if (bSuccess)
        {
            m_bLocalPrinter = ((PRINTER_INFO_4 *)pPrinterData)->pServerName ? FALSE : TRUE;
        }
        else
        {
            m_bAdminAccess = FALSE;
        }

        if (pPrinterData && (pPrinterData != StackPrinterData))
        {
            LocalFree(pPrinterData);
        }

#endif // !defined(_WIN95_)

        ClosePrinter(hPrinter);

    } else {

        m_bAdminAccess = FALSE;
    }

    // How many profile in listbox ?

    LRESULT itemCount = SendMessage(m_hwndList, LB_GETCOUNT, 0, 0);
    if (itemCount == LB_ERR) itemCount = 0;

    //  make sure the ancestor list code behaves correctly.
    //  You need Admin Access and a Local Printer to be able to add/remove profiles

    m_bReadOnly = !(m_bAdminAccess && m_bLocalPrinter);

    //  Enable/Disable the controls (if needed)

    CheckDlgButton(m_hwnd, AutoSelButton, m_bManualMode ? BST_UNCHECKED : BST_CHECKED);
    CheckDlgButton(m_hwnd, ManualSelButton, m_bManualMode ? BST_CHECKED : BST_UNCHECKED);

    //  Only administrator can change 'auto','manual' configuration.

    EnableWindow(GetDlgItem(m_hwnd, AutoSelButton), m_bAdminAccess && m_bLocalPrinter);
    EnableWindow(GetDlgItem(m_hwnd, ManualSelButton), m_bAdminAccess && m_bLocalPrinter);

    //  Only administrator and printer is at local, can install/uninstall color profile.

    EnableWindow(GetDlgItem(m_hwnd, AddButton), m_bAdminAccess && m_bLocalPrinter);
    EnableWindow(GetDlgItem(m_hwnd, RemoveButton), m_bAdminAccess && m_bLocalPrinter && itemCount);

    EnableWindow(m_hwndList, m_bAdminAccess);
    EnableWindow(GetDlgItem(m_hwnd, DefaultProfileText), m_bAdminAccess);
    EnableWindow(GetDlgItem(m_hwnd, DefaultProfile), m_bAdminAccess);

    //  Only with manual mode, these controls are enabled.

    EnableWindow(GetDlgItem(m_hwnd, DefaultButton), m_bAdminAccess && m_bManualMode
                                                         && m_bLocalPrinter && itemCount);

    if (!m_bAdminAccess) {

        //  Set the focus to the OK button

        SetFocus(GetDlgItem(m_hwndSheet, IDOK));
        return  FALSE;  //  Because we moved the focus!
    }

    return TRUE;
}

//  Command processing- we never let them click into the edit control, to
//  prevent them from editing it.

BOOL    CPrinterProfileManagement::OnCommand(WORD wNotifyCode, WORD wid,
                                             HWND hwndCtl) {

    switch  (wNotifyCode) {

        case    LBN_DBLCLK: {

            //  Retrieve the ID of the new default profile			
            //  only accept dblclk changes if the dialog
            //  is not read only - i.e. user is admin

            if  (m_bManualMode) {

                int id = (int)SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);
                m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, id, 0);

                //  Change has been made, update the list

                FillList(DEVLIST_CHANGED);
            }

            return  TRUE;
        }

        case    BN_CLICKED:

            switch  (wid) {

                case    AutoSelButton:
                case    ManualSelButton: {

                    // How many profile in listbox ?

                    LRESULT itemCount = SendMessage(m_hwndList, LB_GETCOUNT, 0, 0);
                    if (itemCount == LB_ERR) itemCount = 0;

                    m_bManualMode = (wid == ManualSelButton) ? TRUE : FALSE;

                    //  Only with manual mode, these controls are enabled.

                    EnableWindow(GetDlgItem(m_hwnd, DefaultButton), m_bManualMode && itemCount);

                    //  Configuarion has been changed, enable apply button.

                    EnableApplyButton();
                    SettingChanged(TRUE);

                    return TRUE;
                }

                case    RemoveButton: {

                    //  Make sure we've tracked the default profile correctly
                    //  when a profile is removed.
                    //  All cases break, because we then want the base class to
                    //  process this message.

                    LRESULT id = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

                    unsigned uTarget = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA,
                        id, 0);

                    if  (uTarget > m_uDefault || m_uDefault == (unsigned) -1)
                        break;  //  Nothing here to worry about

                    if  (m_uDefault == uTarget) {

                        if (CGlobals::ReportEx(AskRemoveDefault, m_hwnd, FALSE,
                                               MB_YESNO|MB_ICONEXCLAMATION,0) == IDYES) {

                            //  The default has been deleted- the profile
                            //  at the top of monitor profile list will be
                            //  made the default profile, if we have

                            LRESULT itemCount = SendMessage(m_hwndList, LB_GETCOUNT, 0, 0);

                            if ((itemCount != LB_ERR) && (itemCount > 1)) {
                                m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, 0, 0);
                                if (m_uDefault == uTarget) {
                                    m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, 1, 0);
                                }
                            } else {
                                m_uDefault = -1;
                            }

                            break;
                        } else {
                            return TRUE; // opration cancelled.
                        }
                    }

                    if  (uTarget < m_cpaProfile.Count())
                        break;  // We're fine

                    //  Must be an added profile below us in the list was
                    //  zapped- we need to decrement ourselves.

                    m_uDefault--;
                    break;
                }

                case    DefaultButton: {

                    LRESULT id = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

                    m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, id, 0);

                    //  Change has been made, update the list

                    FillList(DEVLIST_CHANGED);

                    return  TRUE;
                }
            }

        //  Deliberate fall-through (use a break if you add a case here)
    }

    //  Use common command handling if not handled above

    return  CDeviceProfileManagement::OnCommand(wNotifyCode, wid, hwndCtl);
}

//  Property Sheet notification processing

BOOL    CPrinterProfileManagement::OnNotify(int idCtrl, LPNMHDR pnmh) {

    switch  (pnmh -> code) {

        case    PSN_APPLY: {

            DisableApplyButton();

            //  If nothing changed, nothing need to do.

            if (!SettingChanged())
                return TRUE;

            if (m_bManualMode) {

                //  If the user hasn't selected a default, and we have
                //  associated profiles, then we can't allow this.

                //  03-08-1997  A-RobKj Fix for Memphis RAID 18416- if there's
                //  only one profile left, then it must be the default.

                if  (m_uDefault == (unsigned) -1 && (m_cpaAdds.Count() +
                     m_cpaProfile.Count() - m_cuaRemovals.Count()) > 1) {

                    CGlobals::Report(NoDefaultProfile, m_hwndSheet);
                    SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    break;
                }

                //  !!! This behavior is hardly depend on EnumColorProfiles() API !!!

                //  OK, if the default profile has changed, we have to delete default
                //  profile associations, and do the association for default profile
                //  in "last".

                //  Let the base class handle the cases where the default hasn't
                //  changed, or we started with no profiles and still have none.
                //
                //  03-08-1997  Sleazy code note.  The case where no default is
                //  selected but only one is assigned will now fall here.  Since the
                //  default happens to be the "last", and there only is one, letting
                //  the base class handle it is not a problem.  The list filling
                //  code will take care of the rest for us.

                if  (m_uDefault == (unsigned) -1) break;

                //  Remove default first (if default is associated), then associate later.

                if  (m_uDefault < m_cpaProfile.Count())
                    m_cpaProfile[m_uDefault] -> Dissociate(m_csDevice);

                //  Now do the other removals (actually just dissociations)

                for (unsigned u = 0; u < m_cuaRemovals.Count(); u++) {
                    m_cpaProfile[m_cuaRemovals[u]] -> Dissociate(m_csDevice);
                }

                //  Add in the new ones

                for (u = 0; u < m_cpaAdds.Count(); u++) {
                    if  (m_uDefault >= m_cpaProfile.Count())
                        if  (u == (m_uDefault - m_cpaProfile.Count()))
                            continue;   // this is default, will be done later

                    //  OK, add it back in...
                    m_cpaAdds[u] -> Associate(m_csDevice);
                }

                //  Finally, associate back default profile.

                if  (m_uDefault < m_cpaProfile.Count())
                    m_cpaProfile[m_uDefault] -> Associate(m_csDevice);
                else
                    m_cpaAdds[m_uDefault - m_cpaProfile.Count()] -> Associate(m_csDevice);

                //  Update the various working structures...

                InitList();
                FillList();

                SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);

                //  Now, we have updated settings.

                SettingChanged(FALSE);
            }

            // Update "auto/manual" status.

            InternalSetDeviceConfig((LPCTSTR)m_csDevice, CLASS_PRINTER,
                                    MSCMS_PROFILE_ENUM_MODE, &m_bManualMode, sizeof(DWORD));
        }
    }

    //  Let the base class handle everything else

    return  CDeviceProfileManagement::OnNotify(idCtrl, pnmh);
}

//  Context-sensitive help handler

BOOL    CPrinterProfileManagement::OnHelp(LPHELPINFO pHelp) {

    if (pHelp->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) pHelp->hItemHandle, WINDOWS_HELP_FILE,
                HELP_WM_HELP, (ULONG_PTR) (LPSTR) PrinterUIHelpIds);
    }

    return (TRUE);
}

BOOL    CPrinterProfileManagement::OnContextMenu(HWND hwnd) {

    WinHelp(hwnd, WINDOWS_HELP_FILE,
            HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) PrinterUIHelpIds);

    return (TRUE);
}

//  Scanner Profile Management
//  Scanner Profile Management class constructor- doesn't need any individual
//  code at the moment.

//  Scanner Profile Management

CONST DWORD ScannerUIHelpIds[] = {
#if !defined(_WIN95_)
    AddButton,          IDH_SCANNERUI_ADD,
    RemoveButton,       IDH_SCANNERUI_REMOVE,
    ProfileListControl, IDH_SCANNERUI_LIST,
    ProfileListControlText, IDH_SCANNERUI_LIST,
#endif
    0, 0
};

CScannerProfileManagement::CScannerProfileManagement(LPCTSTR lpstrName,
                                                     HINSTANCE hiWhere) :

    CDeviceProfileManagement(lpstrName, hiWhere, ScannerUI, CLASS_SCANNER) {
    m_bReadOnly = !IsAdmin();
}

//  This class overrides OnInit so it can disable the UI if the user lacks
//  authority to make changes.

BOOL    CScannerProfileManagement::OnInit() {

    //  Call the base class routine first, as it does most of the work...

    CDeviceProfileManagement::OnInit();

    //  Now, see if we have sufficient authority to administer the scanner
    //
    if (m_bReadOnly) {    
        // User is not Admin, Disable all of the controls

        EnableWindow(GetDlgItem(m_hwnd, AddButton), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, RemoveButton), FALSE);

        //  Set the focus to the OK button
        SetFocus(GetDlgItem(m_hwndSheet, IDOK));
        return  FALSE;  //  Because we moved the focus!
    }

    return TRUE;
}

//  Context-sensitive help handler

BOOL    CScannerProfileManagement::OnHelp(LPHELPINFO pHelp) {

    if (pHelp->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) pHelp->hItemHandle, WINDOWS_HELP_FILE,
                HELP_WM_HELP, (ULONG_PTR) (LPSTR) ScannerUIHelpIds);
    }

    return (TRUE);
}

BOOL    CScannerProfileManagement::OnContextMenu(HWND hwnd) {

    WinHelp(hwnd, WINDOWS_HELP_FILE,
            HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) ScannerUIHelpIds);

    return (TRUE);
}



//  Monitor Profile Management class- since the mechanism for default
//  profile manipulation is a bit sleazy, so is some of this code.

CONST DWORD MonitorUIHelpIds[] = {
    AddButton,              IDH_MONITORUI_ADD,
    RemoveButton,           IDH_MONITORUI_REMOVE,
    DefaultButton,          IDH_MONITORUI_DEFAULT,
    ProfileListControl,     IDH_MONITORUI_LIST,
#if !defined(_WIN95_)
    ProfileListControlText, IDH_MONITORUI_LIST,
    MonitorName,            IDH_MONITORUI_DISPLAY,
    MonitorNameText,        IDH_MONITORUI_DISPLAY,
    DefaultProfile,         IDH_MONITORUI_PROFILE,
    DefaultProfileText,     IDH_MONITORUI_PROFILE,
#endif
    0, 0
};

//  Initialize lists override- call the base class, then set the default

void    CMonitorProfileManagement::InitList() {

    CDeviceProfileManagement::InitList();

    m_uDefault = m_cpaProfile.Count() ? 0 : (unsigned) -1;
}

//  Fill list override- write the correct default and call the base function

void    CMonitorProfileManagement::FillList(DWORD dwFlags) {

    //  If we are initializing list box, we want to put focus on
    //  "default" profile. here we won't below FillList set focus
    //  to first one.

    if (dwFlags & DEVLIST_ONINIT) {
        dwFlags |= DEVLIST_NOSELECT;
    }

    CDeviceProfileManagement::FillList(dwFlags);

    //  There is either no default profile, an existing profile is the
    //  default, or a newly selected one is.  Some people just like the
    //  selection operator.

    //  if there is only 1 profile in list box, we treat it as default profile.
	
    if (SendMessage(m_hwndList,LB_GETCOUNT,0,0) == 1) {
        m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, 0, 0);
    }

    if (m_uDefault == -1) {

        // There is no profile associated for this device.

        CString csNoProfile;
        csNoProfile.Load(NoProfileString);
        SetDlgItemText(m_hwnd, DefaultProfile, csNoProfile);     	

    } else {

        // If the default has been deleted, set default as last in list.

        if (m_uDefault >= (m_cpaProfile.Count() + m_cpaAdds.Count())) {

            m_uDefault = (m_cpaProfile.Count() + m_cpaAdds.Count()) - 1;
        }

        // Put default profile name in UI.

        CProfile *pcpDefault = (m_uDefault < m_cpaProfile.Count()) ? \
                                           m_cpaProfile[m_uDefault] : \
                                           m_cpaAdds[m_uDefault - m_cpaProfile.Count()];

        SetDlgItemText(m_hwnd, DefaultProfile, pcpDefault -> GetName());

        // If we are initialing list box, put focus on default profile.

        if (dwFlags & DEVLIST_ONINIT) {

            LRESULT idSelect = SendMessage(m_hwndList, LB_FINDSTRINGEXACT,
                                   (WPARAM) -1, (LPARAM) (LPCTSTR) pcpDefault -> GetName());

            // if could not find, just select first item.

            if (idSelect == LB_ERR) {
                idSelect = 0;
            }

            // Select it.

            SendMessage(m_hwndList, LB_SETCURSEL, idSelect, 0);
        }
    }

    //  03-08-1997  Bob_Kjelgaard@Prodigy.Net   Memphis RAID 18420
    //  Disable the Default button if there aren't any profiles

    // We do it here, because this gets called any time the list changes.	
    // only allow Default button to be enabled if
    // the dialog isn't read only.
    // the remove button should remain dissabled
    // under all conditions while the user is not
    // administrator.

    if (m_bReadOnly) {
        EnableWindow(GetDlgItem(m_hwnd, RemoveButton), FALSE);
    } else {
        EnableWindow(GetDlgItem(m_hwnd, DefaultButton),
                     m_cpaAdds.Count() + m_cpaProfile.Count() - m_cuaRemovals.Count());
    }
}

//  Constructor

CMonitorProfileManagement::CMonitorProfileManagement(LPCTSTR lpstrName,
                                                     LPCTSTR lpstrFriendlyName,
                                                     HINSTANCE hiWhere) :
  CDeviceProfileManagement(lpstrName, hiWhere, MonitorUI, CLASS_MONITOR) {


   // if the user is not the administrator,
   // make this property sheet read only.

   m_bReadOnly = !IsAdmin();

   // Keep friendly name in MonitorProfileManagement class.

   m_csDeviceFriendlyName = lpstrFriendlyName;
}

//  UI Initialization

BOOL    CMonitorProfileManagement::OnInit() {

    //  Do common initializations

    CDeviceProfileManagement::OnInit();

    //  Mark the device name in the space provided

    SetDlgItemText(m_hwnd, MonitorName, m_csDeviceFriendlyName);
	
    //  Now, see if we have sufficient authority to administer the monitor

    if(m_bReadOnly) {

        //  User is not Admin, Disable all of the controls

        EnableWindow(GetDlgItem(m_hwnd, AddButton), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, RemoveButton), FALSE);
        EnableWindow(GetDlgItem(m_hwnd, DefaultButton), FALSE);

        //  EnableWindow(m_hwndList, FALSE);

        //  Set the focus to the OK button

        SetFocus(GetDlgItem(m_hwndSheet, IDOK));
        return  FALSE;  //  Because we moved the focus!
    }

    return  TRUE;
}

//  Command processing- we never let them click into the edit control, to
//  prevent them from editing it.

BOOL    CMonitorProfileManagement::OnCommand(WORD wNotifyCode, WORD wid,
                                             HWND hwndCtl) {

    switch  (wNotifyCode) {

        case    LBN_DBLCLK: {

            //  Retrieve the ID of the new default profile			
            //  only accept dblclk changes if the dialog
            //  is not read only - i.e. user is admin

            if  (!m_bReadOnly) {

                int id = (int)SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);
                m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, id, 0);

                //  Change has been made, update the list

                FillList(DEVLIST_CHANGED);
            }

            return  TRUE;
        }

        case    BN_CLICKED:

            switch  (wid) {

                case    RemoveButton: {

                    //  Make sure we've tracked the default profile correctly
                    //  when a profile is removed.
                    //  All cases break, because we then want the base class to
                    //  process this message.

                    LRESULT id = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

                    unsigned uTarget = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA,
                        id, 0);

                    if  (uTarget > m_uDefault || m_uDefault == (unsigned) -1)
                        break;  //  Nothing here to worry about

                    if  (m_uDefault == uTarget) {

                        if (CGlobals::ReportEx(AskRemoveDefault, m_hwnd, FALSE,
                                               MB_YESNO|MB_ICONEXCLAMATION,0) == IDYES) {

                            //  The default has been deleted- the profile
                            //  at the top of monitor profile list will be
                            //  made the default profile, if we have

                            LRESULT itemCount = SendMessage(m_hwndList, LB_GETCOUNT, 0, 0);

                            if ((itemCount != LB_ERR) && (itemCount > 1)) {
                                m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, 0, 0);
                                if (m_uDefault == uTarget) {
                                    m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, 1, 0);
                                }
                            } else {
                                m_uDefault = -1;
                            }

                            break;
                        } else {
                            return TRUE; // operation cancelled.
                        }
                    }

                    if  (uTarget < m_cpaProfile.Count())
                        break;  // We're fine

                    //  Must be an added profile below us in the list was
                    //  zapped- we need to decrement ourselves.

                    m_uDefault--;
                    break;
                }

                case    DefaultButton: {

                    LRESULT id = SendMessage(m_hwndList, LB_GETCURSEL, 0, 0);

                    m_uDefault = (unsigned)SendMessage(m_hwndList, LB_GETITEMDATA, id, 0);

                    //  Change has been made, update the list

                    FillList(DEVLIST_CHANGED);

                    return  TRUE;
                }
            }

        //  Deliberate fall-through (use a break if you add a case here)
    }

    //  Use common command handling if not handled above
    return  CDeviceProfileManagement::OnCommand(wNotifyCode, wid, hwndCtl);
}

//  Property Sheet notification processing

BOOL    CMonitorProfileManagement::OnNotify(int idCtrl, LPNMHDR pnmh) {

    switch  (pnmh -> code) {

        case    PSN_APPLY: {

            DisableApplyButton();

            //  If nothing changed, nothing need to do.

            if (!SettingChanged())
                return TRUE;

            //  If the user hasn't selected a default, and we have
            //  associated profiles, then we can't allow this.

            //  03-08-1997  A-RobKj Fix for Memphis RAID 18416- if there's
            //  only one profile left, then it must be the default.

            if  (m_uDefault == (unsigned) -1 && (m_cpaAdds.Count() +
                 m_cpaProfile.Count() - m_cuaRemovals.Count()) > 1) {

                CGlobals::Report(NoDefaultProfile, m_hwndSheet);
                SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                break;
            }

            //  !!! This behavior is hardly depend on EnumColorProfiles() API !!!

            //  OK, if the default profile has changed, we have to delete default
            //  profile associations, and do the association for default profile
            //  in "last".

            //  Let the base class handle the cases where the default hasn't
            //  changed, or we started with no profiles and still have none.
            //
            //  03-08-1997  Sleazy code note.  The case where no default is
            //  selected but only one is assigned will now fall here.  Since the
            //  default happens to be the "last", and there only is one, letting
            //  the base class handle it is not a problem.  The list filling
            //  code will take care of the rest for us.

            if  (m_uDefault == (unsigned) -1) break;

            //  Remove default first (if default is associated), then associate later.

            if  (m_uDefault < m_cpaProfile.Count())
                m_cpaProfile[m_uDefault] -> Dissociate(m_csDevice);

            //  Now do the other removals (actually just dissociations)

            for (unsigned u = 0; u < m_cuaRemovals.Count(); u++) {
                m_cpaProfile[m_cuaRemovals[u]] -> Dissociate(m_csDevice);
            }

            //  Add in the new ones

            for (u = 0; u < m_cpaAdds.Count(); u++) {
                if  (m_uDefault >= m_cpaProfile.Count())
                    if  (u == (m_uDefault - m_cpaProfile.Count()))
                        continue;   // this is default, will be done later

                //  OK, add it back in...
                m_cpaAdds[u] -> Associate(m_csDevice);
            }

            //  Finally, associate default profile.

            if  (m_uDefault < m_cpaProfile.Count())
                m_cpaProfile[m_uDefault] -> Associate(m_csDevice);
            else
                m_cpaAdds[m_uDefault - m_cpaProfile.Count()] -> Associate(m_csDevice);

            //  Update the various working structures...

            InitList();
            FillList();

            SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);

            //  Now, we have updated settings.

            SettingChanged(FALSE);
        }
    }

    //  Let the base class handle everything else

    return  CDeviceProfileManagement::OnNotify(idCtrl, pnmh);
}

//  Context-sensitive help handler

BOOL    CMonitorProfileManagement::OnHelp(LPHELPINFO pHelp) {

    if (pHelp->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) pHelp->hItemHandle, WINDOWS_HELP_FILE,
            HELP_WM_HELP, (ULONG_PTR) (LPSTR) MonitorUIHelpIds);
    }

    return (TRUE);
}

BOOL    CMonitorProfileManagement::OnContextMenu(HWND hwnd) {

    WinHelp(hwnd, WINDOWS_HELP_FILE,
            HELP_CONTEXTMENU, (ULONG_PTR) (LPSTR) MonitorUIHelpIds);

    return (TRUE);
}

