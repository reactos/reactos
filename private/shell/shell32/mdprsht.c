//-----------------------------------------------------------------------------------
//  File : mdprsht.c  (Mounted Drive Property sheet code)
//
//  Description :
//      Contains code for the Mounted Drive property sheet dialog
//
//  History :
//      arulk           03/09/98            created
//
//-----------------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop
#include "propsht.h"

#ifdef WINNT
#include <winbase.h>

//drivesx.c
BOOL _DrvGeneralDlgProc(
     HWND hDlg, 
     UINT uMessage, 
     WPARAM wParam, 
     LPARAM lParam);


int 
GetDriveIDForVolumeName(
    IN  PWSTR   Name
    )

{
    WCHAR   driveName[10];
    UCHAR   driveLetter;
    WCHAR   otherName[MAX_PATH];
    BOOL    b;

    driveName[1] = ':';
    driveName[2] = '\\';
    driveName[3] = 0;

    for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {

        driveName[0] = driveLetter;
        b = GetVolumeNameForVolumeMountPointW(driveName, otherName,
                                             MAX_PATH);
        if (!b) {
            continue;
        }

        if (!lstrcmpi(Name, otherName)) {
            break;
        }
    }

    if (driveLetter <='Z') {
        return (driveLetter - 'A');
    } else {
        return -1;
    }
}

BOOL ShowDriveProperties(HWND hwnd, int iDrive, LPCTSTR pcszHostFullPath)
{
    BOOL fSuccess = FALSE;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpage[1];
    DRIVEPROPSHEETPAGE dpsp = {0};
    HPROPSHEETPAGE hpage;
    TCHAR szTitle[MAX_PATH];
    int len;

    PathBuildRoot(szTitle, iDrive);
    len = lstrlen(szTitle);
    if(0 > wnsprintf(szTitle + len, ARRAYSIZE(szTitle) - len, TEXT(" (%s)"), pcszHostFullPath))
        szTitle[ARRAYSIZE(szTitle)-1]=TEXT('\0');
    
    psh.dwFlags    = PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.dwSize     = SIZEOF(psh);
    psh.hInstance  = HINST_THISDLL;
    psh.nPages     = 1;     // incremented in callback
    psh.nStartPage = 0;   // set below if specified
    psh.phpage     = ahpage;
    psh.pszCaption = szTitle;

    dpsp.psp.dwSize      = SIZEOF(dpsp);    // extra data
    dpsp.psp.dwFlags     = PSP_DEFAULT;
    dpsp.psp.hInstance   = HINST_THISDLL;
    dpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_GENERAL);
    dpsp.psp.pfnDlgProc  = _DrvGeneralDlgProc,
    dpsp.iDrive          = iDrive;

    hpage = CreatePropertySheetPage(&dpsp.psp);
    if (!hpage)
    {
        return FALSE;
    }
    
    psh.phpage[0] = hpage;

    _try
    {
        if (PropertySheet(&psh) >= 0)   // IDOK or IDCANCEL (< 0 is error)
            fSuccess = TRUE;
    }
    _except(UnhandledExceptionFilter(GetExceptionInformation()))
    {
            DebugMsg(DM_ERROR, TEXT("PRSHT: Fault in property sheet"));
    }
    return fSuccess;   
}


BOOL InitMountedDrvPrsht(FILEPROPSHEETPAGE * pfpsp)
{
    TCHAR szBuffer[MAX_PATH];
    TCHAR szVolName[MAX_PATH];
    SHFILEINFO sfi;
    HICON hiconT;
    WIN32_FILE_ATTRIBUTE_DATA fd;
    DWORD dwVolumeFlags = 0; 
    BOOL  bMounted = pfpsp->fIsMountedDrive;  // Are we showing this dialog for a mounted folder or
                                             // for a root folder of a drive ?

    //Common intialization

    dwVolumeFlags = GetVolumeFlags(pfpsp->szPath);
    // test for file-based compression.
    if (dwVolumeFlags & FS_FILE_COMPRESSION)
    {
        // filesystem supports compression
        pfpsp->fIsCompressionAvailable = TRUE;
    }

    // test for file-based encryption.
    if (dwVolumeFlags & FS_FILE_ENCRYPTION)
    {
        // filesystem supports encryption
        pfpsp->fIsEncryptionAvailable = TRUE;
    }

    //Get Information about the file
    SHGetFileInfo(pfpsp->szPath, 0, &sfi, SIZEOF(sfi),
                                   SHGFI_ICON|SHGFI_LARGEICON |
                                   SHGFI_DISPLAYNAME|
                                   SHGFI_TYPENAME);
    if (sfi.hIcon)
    {
        hiconT = Static_SetIcon(GetDlgItem(pfpsp->hDlg, IDC_DRV_ICON), sfi.hIcon);

        if (hiconT)
        {
            DestroyIcon(hiconT);
        }
    }



    //Mounted Drive specific initialization

    if (bMounted)
    {
        //Make sure the path ends with a backslash. otherwise the following api wont work
        PathAddBackslash(pfpsp->szPath);        
        if (GetVolumeNameForVolumeMountPointW(pfpsp->szPath, szVolName, ARRAYSIZE(szVolName)))
        {
           pfpsp->iDrive = GetDriveIDForVolumeName(szVolName);
        }
    }


    // Initialization based on the context

    // Set the Label of the folder
    if (!bMounted)
    {
        //Root Folder case. The Label is basically root folder name (for example c:\ ) 
        SetWindowText(GetDlgItem(pfpsp->hDlg, IDC_DRV_LABEL), pfpsp->szPath);

        //Also change the type name
        SetWindowText(GetDlgItem(pfpsp->hDlg, IDC_DRV_TYPE), sfi.szTypeName);
    }
    else    
    {
        //Mounted Drive case. The Label is actually the mount point directoty name
        // ie if the mount point is c:\foo1\foo2  then  the name is foo2
        SetWindowText(GetDlgItem(pfpsp->hDlg, IDC_DRV_LABEL), sfi.szDisplayName);
    }

    // Set the Target Volume. ie set the text box with volume label
    if (!bMounted)
    {
        //Root Folder case. Target Volume is same as the current volume lable
        if (!GetVolumeInformation(pfpsp->szPath, szBuffer, ARRAYSIZE(szBuffer), NULL, NULL,
                                      NULL, NULL,0))
        {
            szBuffer[0] = 0;
        }
    }
    else
    {
        //Use the Volname obtained before to get the mounted drive's label.
        if (!GetVolumeInformation(szVolName, szBuffer, ARRAYSIZE(szBuffer), NULL, NULL,
                                           NULL, NULL,0))
        {
                szBuffer[0] = 0;
        }
    }

    SetWindowText(GetDlgItem(pfpsp->hDlg, IDC_DRV_TARGET), szBuffer);        

    // if we are showing this dialog for normal drive then remove
    // the drive properties button as this property sheet will be
    // shown along with the drive properties property page.
    
    // OR

    // if the mounted  volume  doesn't have  an drive id associated with it
    // then we can't really show the properties for mounted volume so nuke the
    // drive properties button in that case also
    if (!bMounted ||  (pfpsp->iDrive < 0 ))
    {
        // Remove the properties button if we are not showing this dialog 
        // for a mounted drive
        ShowWindow(GetDlgItem(pfpsp->hDlg,   IDC_DRV_PROPERTIES),   FALSE);
        EnableWindow(GetDlgItem(pfpsp->hDlg, IDC_DRV_PROPERTIES), FALSE);               
    }

    // Set the location
    // the full path to the folder that contains this file.
    lstrcpy(szBuffer, pfpsp->szPath);
    PathRemoveFileSpec(szBuffer);
    PathSetDlgItemPath(pfpsp->hDlg, IDC_DRV_LOCATION, szBuffer);


    //Get the attributes
    if (!GetFileAttributesEx(pfpsp->szPath, GetFileExInfoStandard,  &fd))
    {
        // if this failed we should clear out some values as to not show
        // garbage on the screen.
        _fmemset(&fd, TEXT('\0'), SIZEOF(fd));
    }


    // set the initial attributes
    SetInitialFileAttribs(pfpsp, fd.dwFileAttributes, fd.dwFileAttributes);

    // set the current attributes to the same as the initial
    memcpy(&pfpsp->asCurrent, &pfpsp->asInitial, SIZEOF(pfpsp->asCurrent));
   
    // Set the created  time
    SetDateTimeText(pfpsp->hDlg, IDC_DRV_CREATED, &fd.ftCreationTime);

    //
    // now setup all the controls on the dialog based on the attribs
    // that we have
    //

    // ReadOnly and Hidden are always on the general tab
    CheckDlgButton(pfpsp->hDlg, IDD_READONLY, pfpsp->asInitial.fReadOnly);
    CheckDlgButton(pfpsp->hDlg, IDD_HIDDEN,   pfpsp->asInitial.fHidden);

    return TRUE;
}


//
// Descriptions:
//      This is the dialog procedure for the mounted drive general 
//      property sheet page 
//
// History:
//  03/06/98        arulk          Created
//

BOOL CALLBACK MountedDrvPrshtDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE *pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            pfpsp = (FILEPROPSHEETPAGE *)lParam;
            pfpsp->hDlg = hDlg;
            pfpsp->fMountedDrive = TRUE;  //This structure is not used for file property page
            InitMountedDrvPrsht(pfpsp);
            break;
        }

        case WM_DESTROY:
        {
            HICON hIcon;

            hIcon = Static_GetIcon(GetDlgItem(hDlg, IDC_DRV_ICON), NULL);
            if (hIcon)
                DestroyIcon(hIcon);
            break;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDD_HIDDEN:
                case IDD_READONLY:
                    break;

                case IDC_ADVANCED:
                    DialogBoxParam(HINST_THISDLL, 
                                   MAKEINTRESOURCE(DLG_FOLDERATTRIBS),
                                   hDlg,
                                   AdvancedFileAttribsDlgProc,
                                   (LPARAM)pfpsp);
                    break;

                case IDC_DRV_PROPERTIES:
                {
                    TCHAR szBuffer[MAX_PATH];

                    lstrcpy(szBuffer, pfpsp->szPath);
                    PathRemoveFileSpec(szBuffer);

                    ShowDriveProperties(pfpsp->hDlg, pfpsp->iDrive, szBuffer);
                    break;
                }
                default:
                    return(FALSE);
            }
            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
        }
        
        case WM_NOTIFY:
        {
            switch (((NMHDR *)lParam)->code) 
            {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                {
                    BOOL bRet = TRUE;

                    pfpsp->asCurrent.fReadOnly = (IsDlgButtonChecked(hDlg, IDD_READONLY) == BST_CHECKED);
                    //
                    // GetFileAttributes always returns FILE_ATTRIBUTE_HIDDEN (and trying to call SetFileAttributes
                    // to unset FILE_ATTRIBUTE_HIDDEN succeeds, even though the next time you call GetFileAttributes,
                    // FILE_ATTRIBUTE_HIDDEN will still be set, sigh...) so we punt on the hidden stuff for volumes.
                    //
                    //pfpsp->asCurrent.fHidden = (IsDlgButtonChecked(hDlg, IDD_HIDDEN) == BST_CHECKED);

                    // check to see if the user wants to apply the attribs recursively or not
                    bRet = DialogBoxParam(HINST_THISDLL, 
                                          MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                                          hDlg,
                                          RecursivePromptDlgProc,
                                          (LPARAM)pfpsp);

                    if (bRet)
                    {
                        bRet = ApplySingleFileAttributes(pfpsp);
                    }

                    if (bRet)
                    {
                        // we sucessfully applied the attribs, so we need to change the cancel button to close
                        PropSheet_CancelToClose(GetParent(pfpsp->hDlg));
                    }
                    else
                    {
                        // the user hit cancel, so we return true to prevent the property sheet form closeing
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }
                    break;
                }
                
                default:
                    return(FALSE);
            }
            break;
        }

        
        default:
            return FALSE;
    }
    
    return TRUE;
}



//
// Descriptions:
//  This functions adds the mounted drive property sheet dialog
//
// History:
//  03/06/98        arulk          Created
//

HPROPSHEETPAGE AddMountedDrvPage(LPCTSTR pszFile, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam, BOOL bMounted)
{
    FILEPROPSHEETPAGE fpsp;

    ZeroMemory(&fpsp, SIZEOF(fpsp));

    fpsp.psp.dwSize      = SIZEOF(fpsp);
    fpsp.psp.dwFlags     = PSP_DEFAULT;
    fpsp.psp.hInstance   = HINST_THISDLL;
    fpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_MOUNTEDDRV_GENERAL);
    fpsp.psp.pfnDlgProc  = MountedDrvPrshtDlgProc;

    //Are we showing this page for mounted volume or for an ordinary volume
    fpsp.fIsMountedDrive = BOOLIFY(bMounted);

    // we set fIsDirectory to always be true, since this is a volume
    fpsp.fIsDirectory = TRUE;

    lstrcpy(fpsp.szPath , pszFile);

    // if we are not showing this dialog for mounted volume change the title to from
    // "General" to "Contents
    if (!bMounted)
    {
        fpsp.psp.dwFlags |= PSP_USETITLE;
        fpsp.psp.pszTitle = MAKEINTRESOURCE(IDS_CONTENTS);
    }
    
    return CreatePropertySheetPage( &fpsp.psp );

}

#endif //WINNT
