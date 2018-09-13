#include "stdafx.h"
#pragma hdrstop

#ifdef POSTSPLIT

const TCHAR c_szSetup[] = REGSTR_PATH_SETUP TEXT("\\Setup");
const TCHAR c_szSharedDir[] = TEXT("SharedDir");

static const LPCTSTR c_rgpszWallpaperExt[] = {
    TEXT("BMP"), TEXT("GIF"),
    TEXT("JPG"), TEXT("JPE"),
    TEXT("JPEG"),TEXT("DIB"),
    TEXT("PNG"), TEXT("HTM"),
    TEXT("HTML")
};

#define c_szHelpFile    TEXT("Display.hlp")
const static DWORD aBackHelpIDs[] = {  // Context Help IDs
    IDC_BACK_SELECT,    IDH_DISPLAY_BACKGROUND_WALLPAPERLIST,
    IDC_BACK_WPLIST,    IDH_DISPLAY_BACKGROUND_WALLPAPERLIST,
    IDC_BACK_BROWSE,    IDH_DISPLAY_BACKGROUND_BROWSE_BUTTON,
    IDC_BACK_PATTERN,   IDH_DISPLAY_BACKGROUND_PATTERN_BUTTON,
    IDC_BACK_DISPLAY,   IDH_DISPLAY_BACKGROUND_PICTUREDISPLAY,
    IDC_BACK_WPSTYLE,   IDH_DISPLAY_BACKGROUND_PICTUREDISPLAY,
    IDC_BACK_PREVIEW,   IDH_DISPLAY_BACKGROUND_MONITOR,
    0, 0
};

CBackPropSheetPage::CBackPropSheetPage(void)
{
    //
    // Initialize a bunch of propsheetpage variables.
    //
    dwSize = sizeof(CBackPropSheetPage);
    dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    hInstance = HINST_THISDLL;
    pszTemplate = MAKEINTRESOURCE(IDD_BACKGROUND);
    // hIcon = NULL; // unused (PSP_USEICON is not set)
    // pszTitle = NULL; // unused (PSP_USETITLE is not set)
    pfnDlgProc = _DlgProc;
    // lParam   = 0;     // unused
    pfnCallback = NULL;
    // pcRefParent = NULL;
}

int CBackPropSheetPage::_AddAFileToLV(LPCTSTR pszDir, LPTSTR pszFile, UINT nBitmap)
{
    int index;
    LPTSTR pszPath = (LPTSTR)LocalAlloc(LPTR,
        ((pszDir ? lstrlen(pszDir) : 0) + lstrlen(pszFile) + 2) * sizeof(TCHAR));

    if (pszPath)
    {
        if (pszDir)
        {
            lstrcpy(pszPath, pszDir );
            lstrcat(pszPath, TEXT("\\"));
            lstrcat(pszPath, pszFile);
        }
        else if (pszFile && *pszFile && (lstrcmpi(pszFile, g_szNone) != 0))
        {
            lstrcpy(pszPath, pszFile);
        }
        else
        {
            *pszPath = TEXT('\0');
        }

        pszFile = PathFindFileName(pszFile);
        PathRemoveExtension(pszFile);
        PathMakePretty(pszFile);

        LV_ITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM | (nBitmap != -1 ? LVIF_IMAGE : 0);
        lvi.iItem = 0x7FFFFFFF;
        lvi.pszText = pszFile;
        lvi.iImage = nBitmap;
        lvi.lParam = (LPARAM)pszPath;

        index = ListView_InsertItem(_hwndLV, &lvi);
        ListView_SetColumnWidth(_hwndLV, 0, LVSCW_AUTOSIZE);

        if (index == -1)
        {
            LocalFree((HANDLE)pszPath);
        }
    }

    return index;
}

void CBackPropSheetPage::_AddFilesToLV(LPCTSTR pszDir, LPCTSTR pszSpec, UINT nBitmap)
{
    WIN32_FIND_DATA fd;
    HANDLE h;
    TCHAR szBuf[MAX_PATH];

    lstrcpy(szBuf, pszDir);
    StrCatBuff(szBuf, TEXT("\\*."), SIZECHARS(szBuf));
    StrCatBuff(szBuf, pszSpec, SIZECHARS(szBuf));

    h = FindFirstFile(szBuf, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Skip files that are "Super-hidden" like "Winnt.bmp" and "Winnt256.bmp"
            if((fd.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) != (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) 
                _AddAFileToLV(pszDir, fd.cFileName, nBitmap);
        }
        while (FindNextFile(h, &fd));

        FindClose(h);
    }
}

int CBackPropSheetPage::_FindWallpaper(LPCTSTR pszFile)
{
    int nItems = ListView_GetItemCount(_hwndLV);
    int i;

    for (i=0; i<nItems; i++)
    {
        LV_ITEM lvi = {0};

        lvi.iItem = i;
        lvi.mask = LVIF_PARAM;
        ListView_GetItem(_hwndLV, &lvi);
        if (lstrcmpi(pszFile, (LPCTSTR)lvi.lParam) == 0)
        {
            return i;
        }
    }

    return -1;
}

void CBackPropSheetPage::_UpdatePreview(WPARAM flags)
{
    WALLPAPEROPT wpo;

    wpo.dwSize = sizeof(WALLPAPEROPT);

    g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
    if (wpo.dwStyle & WPSTYLE_TILE)
    {
        flags |= BP_TILE;
    }
    else if(wpo.dwStyle & WPSTYLE_STRETCH)
            flags |= BP_STRETCH;
    

    SendDlgItemMessage(_hwnd, IDC_BACK_PREVIEW, WM_SETBACKINFO, flags, 0);
}

void CBackPropSheetPage::_EnableControls(void)
{
    if (_fAllowChanges)
    {
        BOOL fEnable;

        WALLPAPEROPT wpo = { SIZEOF(wpo) };
        g_pActiveDesk->GetWallpaperOptions(&wpo, 0);

        WCHAR wszWallpaper[INTERNET_MAX_URL_LENGTH];
        LPTSTR pszWallpaper;

        g_pActiveDesk->GetWallpaper(wszWallpaper, ARRAYSIZE(wszWallpaper), 0);
#ifndef UNICODE
        CHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        SHUnicodeToAnsi(wszWallpaper, szWallpaper, ARRAYSIZE(szWallpaper));
        pszWallpaper = szWallpaper;
#else
        pszWallpaper = (LPTSTR)wszWallpaper;
#endif
        BOOL fIsPicture = IsWallpaperPicture(pszWallpaper);

        //
        // Pattern button only enabled when we are viewing
        // a picture in centered mode, or when we have no
        // wallpaper at all.
        //
        fEnable = ((wpo.dwStyle == WPSTYLE_CENTER) && fIsPicture) || !*pszWallpaper;
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_PATTERN), fEnable);

        //
        // Style combo only enabled if a non-null picture
        // is being viewed.
        //
        fEnable = fIsPicture && (*pszWallpaper) && (!_fPolicyForStyle);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPSTYLE), fEnable);

//  98/09/10 vtan #209753: Also remember to disable the corresponding
//  text item with the keyboard shortcut. Not disabling this will
//  allow the shortcut to be invoked but will cause the incorrect
//  dialog item to be "clicked".

        (BOOL)EnableWindow(GetDlgItem(_hwnd, IDC_BACK_DISPLAY), fEnable);
    }
}

int CBackPropSheetPage::_GetImageIndex(LPCTSTR pszFile)
{
    int iRet = 0;

    if (pszFile && *pszFile)
    {
        LPCTSTR pszExt = PathFindExtension(pszFile);
        if (*pszExt == TEXT('.'))
        {
            pszExt++;
            for (iRet=0; iRet<ARRAYSIZE(c_rgpszWallpaperExt); iRet++)
            {
                if (lstrcmpi(pszExt, c_rgpszWallpaperExt[iRet]) == 0)
                {
                    //
                    // Add one because 'none' took the 0th slot.
                    //
                    iRet++;
                    return(iRet);
                }
            }
            //
            // If we fell off the end of the for loop here,
            // this is a file with unknown extension. So, we assume that
            // it is a normal wallpaper and it gets the Bitmap's icon
            //
            iRet = 1;
        }
        else
        {
            //
            // Unknown files get Bitmap's icon.
            //
            iRet = 1;
        }
    }

    return iRet;
}

void CBackPropSheetPage::_SetNewWallpaper(LPCTSTR pszFile)
{
    TCHAR szFile[INTERNET_MAX_URL_LENGTH];
    TCHAR szTemp[INTERNET_MAX_URL_LENGTH];

    //
    // Make a copy of the file name.
    //
    lstrcpy(szFile, pszFile);

    //
    // Replace all "(none)" with empty strings.
    //
    if (!szFile || (lstrcmpi(szFile, g_szNone) == 0))
    {
        szFile[0] = TEXT('\0');
    }

    //
    // Replace net drives with UNC names.
    //
    if(
#ifndef UNICODE
        !IsDBCSLeadByte(szFile[0]) &&
#endif
        (szFile[1] == TEXT(':')) )
    {
        TCHAR szDrive[3];
        ULONG cchTemp = ARRAYSIZE(szTemp);

        lstrcpyn(szDrive, szFile, ARRAYSIZE(szDrive));
        if ((SHWNetGetConnection(szDrive, szTemp, &cchTemp) ==
            NO_ERROR) && (szTemp[0] == TEXT('\\')) && (szTemp[1] == TEXT('\\')))
        {
            lstrcat(szTemp, szFile+2);
            lstrcpy(szFile, szTemp);
        }
    }

    WCHAR   wszTemp[INTERNET_MAX_URL_LENGTH];
    LPTSTR  pszTemp;
    //
    // If necessary, update the desk state object.
    //
    g_pActiveDesk->GetWallpaper(wszTemp, ARRAYSIZE(wszTemp), 0);
#ifndef UNICODE
    SHUnicodeToAnsi(wszTemp, szTemp, ARRAYSIZE(szTemp));
    pszTemp = szTemp;
#else
    pszTemp = (LPTSTR)wszTemp;
#endif
    if (lstrcmpi(pszTemp, szFile) != 0)
    {
        LPWSTR  pwszFile;
#ifndef UNICODE
        SHAnsiToUnicode(szFile, wszTemp, ARRAYSIZE(wszTemp));
        pwszFile = wszTemp;
#else
        pwszFile = (LPWSTR)szFile;
#endif
        g_pActiveDesk->SetWallpaper(pwszFile, 0);
    }

    //
    // Update the preview picture of the new wallpaper.
    //
    _UpdatePreview(0);

    //
    // If necessary, add the new item to the listview.
    //
    TCHAR   szTemp2[INTERNET_MAX_URL_LENGTH];
    // If the wallpaper does not have a directory specified, (this may happen if other apps. change this value),
    // we have to figure it out.
    GetWallpaperWithPath(szFile, szTemp2, ARRAYSIZE(szTemp2));

    int iSelectionNew = *szTemp2 ? _FindWallpaper(szTemp2) : 0;
    if (iSelectionNew == -1)
    {
        iSelectionNew = _AddAFileToLV(NULL, szTemp2, _GetImageIndex(szTemp2));
    }

    //
    // If necessary, select the item in the listview.
    //
    int iSelected = ListView_GetNextItem(_hwndLV, -1, LVNI_SELECTED);
    if (iSelected != iSelectionNew)
    {
        ListView_SetItemState(_hwndLV, iSelectionNew, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    //
    // Put all controls in correct enabled state.
    //
    _EnableControls();

    //
    // Make sure the selected item is visible.
    //
    ListView_EnsureVisible(_hwndLV, iSelectionNew, FALSE);
}

int CALLBACK CBackPropSheetPage::_SortBackgrounds(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    TCHAR szFile1[MAX_PATH], szFile2[MAX_PATH];
    LPTSTR lpszFile1, lpszFile2;

    lstrcpy(szFile1, (LPTSTR)lParam1);
    lpszFile1 = PathFindFileName(szFile1);
    PathRemoveExtension(lpszFile1);
    PathMakePretty(lpszFile1);

    lstrcpy(szFile2, (LPTSTR)lParam2);
    lpszFile2 = PathFindFileName(szFile2);
    PathRemoveExtension(lpszFile2);
    PathMakePretty(lpszFile2);

    return lstrcmpi(lpszFile1, lpszFile2);
}

void CBackPropSheetPage::_AddPicturesFromDir(LPCTSTR pszDirName)
{
    int i;
    //
    // Add only the *.BMP files first (They can be shown even when Active Desktop is OFF)
    //
    _AddFilesToLV(pszDirName, c_rgpszWallpaperExt[0], 1);

    if(_fAllowHtml)
    {
        // BMPs have already been added; So, start from the next.
        for (i=1; i<ARRAYSIZE(c_rgpszWallpaperExt); i++)
        {
            // The .htm extension includes the .html files. Hence the .html extension is skipped.
            // The .jpe extension includes the .jpeg files too. So, it .jpeg is skipped too!
            if((lstrcmpi(c_rgpszWallpaperExt[i],TEXT("HTML")) == 0) ||
               (lstrcmpi(c_rgpszWallpaperExt[i],TEXT("JPEG")) == 0))
                continue;
            _AddFilesToLV(pszDirName, c_rgpszWallpaperExt[i], i+1);
        }
    }
}

void CBackPropSheetPage::_OnInitDialog(HWND hwnd)
{
    int i;
    TCHAR szBuf[MAX_PATH];
    

    //
    // Set some member variables.
    //
    _hwnd = hwnd;
    _hwndLV = GetDlgItem(hwnd, IDC_BACK_WPLIST);
    _hwndWPStyle = GetDlgItem(hwnd, IDC_BACK_WPSTYLE);
    HWND hWndPrev = GetDlgItem(hwnd, IDC_BACK_PREVIEW);
    if (hWndPrev) {
        // Turn off mirroring for this control.
        SetWindowBits(hWndPrev, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
    }

    InitDeskHtmlGlobals();

    if (!g_pActiveDesk)
    {
        HRESULT  hres;
        IActiveDesktopP * piadp;

        if (SUCCEEDED(hres = CActiveDesktop_InternalCreateInstance((LPUNKNOWN *)&piadp, IID_IActiveDesktopP)))
        {
            WCHAR wszScheme[MAX_PATH];
            DWORD dwcch = ARRAYSIZE(wszScheme);

            // Get the global "edit" scheme and set ourselves us to read from and edit that scheme
            if (SUCCEEDED(piadp->GetScheme(wszScheme, &dwcch, SCHEME_GLOBAL | SCHEME_EDIT)))
            {
                piadp->SetScheme(wszScheme, SCHEME_LOCAL);
                
            }
            hres = piadp->QueryInterface(IID_IActiveDesktop, (LPVOID *)&g_pActiveDesk);
            piadp->Release();
        }

        if (FAILED(hres))
        {
            return;
        }
    }
    else
    {
        g_pActiveDesk->AddRef();
    }

    //
    // Read in the restrictions.
    //
    _fForceAD = SHRestricted(REST_FORCEACTIVEDESKTOPON);
    _fAllowAD = _fForceAD || (!SHRestricted(REST_NOACTIVEDESKTOP));
    _fAllowChanges = !SHRestricted(REST_NOCHANGINGWALLPAPER);
    
    if (_fAllowAD == FALSE)
    {
        _fAllowHtml = FALSE;
    }
    else
    {
        _fAllowHtml = !SHRestricted(REST_NOHTMLWALLPAPER);
    }

    //
    // Check to see if there is a policy for Wallpaper name and wallpaper style.
    //
    _fPolicyForWallpaper = ReadPolicyForWallpaper(NULL, 0);
    _fPolicyForStyle = ReadPolicyForWPStyle(NULL);

    //
    // Get the images into the listview.
    //
    HIMAGELIST himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), ILC_MASK, ARRAYSIZE(c_rgpszWallpaperExt),
        ARRAYSIZE(c_rgpszWallpaperExt));
    if (himl)
    {
        SHFILEINFO sfi;

        //
        // Add the 'None' icon.
        //
        HICON hIconNone = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_BACK_NONE),
            IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), 0);
        ImageList_AddIcon(himl, hIconNone);

        int iPrefixLen = lstrlen(TEXT("foo."));
        lstrcpy(szBuf, TEXT("foo.")); //Pass "foo.bmp" etc., to SHGetFileInfo instead of ".bmp"
        for (i=0; i<ARRAYSIZE(c_rgpszWallpaperExt); i++)
        {
            lstrcpy(szBuf+iPrefixLen, c_rgpszWallpaperExt[i]);

            if (SHGetFileInfo(szBuf, 0, &sfi, SIZEOF(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES))
            {
                ImageList_AddIcon(himl, sfi.hIcon);
                DestroyIcon(sfi.hIcon);
            }
        }
        ListView_SetImageList(_hwndLV, himl, LVSIL_SMALL);
    }

    //
    // Get the directory with the wallpaper files.
    //
    GetWindowsDirectory(szBuf, ARRAYSIZE(szBuf));
    GetStringFromReg(HKEY_LOCAL_MACHINE, c_szSetup, c_szSharedDir, NULL, szBuf, ARRAYSIZE(szBuf));

    //
    // Add the single column that we want.
    //
    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    ListView_InsertColumn(_hwndLV, 0, &lvc);

    //
    // Add 'none' option.
    //
    _AddAFileToLV(NULL, g_szNone, 0);

    //
    // Add only the *.BMP files in the windows directory.
    //
    _AddFilesToLV(szBuf, c_rgpszWallpaperExt[0], 1);

    //
    // Get the wallpaper Directory name
    //
    GetWallpaperDirName(szBuf, ARRAYSIZE(szBuf));

    // Add all pictures from Wallpaper directory to the list!
    _AddPicturesFromDir(szBuf);
    
    //Get the path to the "My Pictures" folder
    if (S_OK == SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, 0, szBuf))
    {
        // Add all pictures in "My Pictures" directory to the list!
        _AddPicturesFromDir(szBuf);
    }
    
    //
    // Sort the standard items.
    //
    ListView_SortItems(_hwndLV, _SortBackgrounds, 0);

    WCHAR   wszBuf[MAX_PATH];
    LPTSTR  pszBuf;

    //
    // Add & select the current setting.
    //
    g_pActiveDesk->GetWallpaper(wszBuf, ARRAYSIZE(wszBuf), 0);

    //Convert wszBuf to szBuf.
#ifndef UNICODE
    SHUnicodeToAnsi(wszBuf, szBuf, ARRAYSIZE(szBuf));
    pszBuf = szBuf;
#else
    pszBuf = (LPTSTR)wszBuf;
#endif

    if (!_fAllowHtml && !IsNormalWallpaper(pszBuf))
    {
        *pszBuf = TEXT('\0');
    }
    _SetNewWallpaper(pszBuf);

    //
    // Fill and select the style combo.
    // Note: We do NOT add "Stretch" item for Older Operating systems because they
    // had a plus tab that had a "Stretch" check box that interfered with us. In the 
    // new Plus tab, that check box had been removed. So, we support "Stretch" through
    // our background page drop down combo box.
    int	iEndStyle;

    iEndStyle = (g_bRunOnNT5 || g_bRunOnMemphis)? WPSTYLE_STRETCH : (WPSTYLE_STRETCH - 1);
    
    for (i=0; i<= iEndStyle; i++)
    {
        LoadString(HINST_THISDLL, IDS_WPSTYLE+i, szBuf, ARRAYSIZE(szBuf));
        ComboBox_AddString(_hwndWPStyle, szBuf);
    }
    WALLPAPEROPT wpo;
    wpo.dwSize = sizeof(WALLPAPEROPT);
    g_pActiveDesk->GetWallpaperOptions(&wpo, 0);

    ComboBox_SetCurSel(_hwndWPStyle, ((g_bRunOnNT5 || g_bRunOnMemphis) ? wpo.dwStyle : (wpo.dwStyle & WPSTYLE_TILE)));

    //
    // Adjust various UI components.
    //
    if (!_fAllowChanges)
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_DISPLAY), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPSTYLE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_BROWSE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_PATTERN), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPLIST), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_SELECT), FALSE);
    }

    //
    // Disable controls based on the policies
    //
    if(_fPolicyForWallpaper)
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_BROWSE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPLIST), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_SELECT), FALSE);
    }

    if(_fPolicyForStyle)
    {
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_WPSTYLE), FALSE);
        EnableWindow(GetDlgItem(_hwnd, IDC_BACK_DISPLAY), FALSE);
    }
    
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);

    //if activedesktop is forced to be on, this overrides the NOACTIVEDESKTOP restriction
    if(_fForceAD)
    {
        if(!co.fActiveDesktop)
        {
            co.fActiveDesktop = TRUE;
            g_pActiveDesk->SetDesktopItemOptions(&co, 0);
        }
    }
    else
    {
        //See if Active Desktop is to be turned off by restriction.
        if (!_fAllowAD)
        {
            if (co.fActiveDesktop)
            {
                co.fActiveDesktop = FALSE;
                g_pActiveDesk->SetDesktopItemOptions(&co, 0);
            }
        }
    }

    _EnableControls();
}



// This function checks to see if the currently selected wallpaper is a HTML wallpaper
// and if so, it makes sure that the active desktop is enabled. If it is disabled
// then it prompts the user asking a question to see if the user wants to enable it.

BOOL EnableADifHtmlWallpaper(HWND hwnd)
{
    BOOL    fRet = FALSE;
    IADesktopP2 * piadp2;
    DWORD   dwFlags = 0;
    
    if(FAILED(g_pActiveDesk->QueryInterface(IID_IADesktopP2, (LPVOID *)&piadp2)))
        return(FALSE);

    //See if the ActiveDesktop component is dirty.
    piadp2->GetADObjectFlags(&dwFlags, GADOF_DIRTY);

    //If the object is NOT dirty, that probably means another property sheet has
    // already did the required work. We don't need to re-do it again!
    if(!(dwFlags & GADOF_DIRTY))
    {
        piadp2->Release();
        return(FALSE);      //Nothing to do!
    }
    
    COMPONENTSOPT co;
    co.dwSize = sizeof(COMPONENTSOPT);
    g_pActiveDesk->GetDesktopItemOptions(&co, 0);

    //If the active desktop is currently ON, then we can support any wallpaper!
    if(co.fActiveDesktop)
    {
        //Re-read the wallpaper if needed for active desktop.
        piadp2->ReReadWallpaper();
        //Nothing more to check!
    }
    else
    {
        // Currently the active desktop is turned OFF. See, if we have a suitable
        // wallpaper.
        WCHAR wszWallpaper[INTERNET_MAX_URL_LENGTH];
        LPTSTR pszWallpaper;

        g_pActiveDesk->GetWallpaper(wszWallpaper, ARRAYSIZE(wszWallpaper), 0);
#ifndef UNICODE
        CHAR szWallpaper[INTERNET_MAX_URL_LENGTH];
        SHUnicodeToAnsi(wszWallpaper, szWallpaper, ARRAYSIZE(szWallpaper));
        pszWallpaper = szWallpaper;
#else
        pszWallpaper = (LPTSTR)wszWallpaper;
#endif
        //If this is a normal BMP file. Nothing to check.
        if(!IsNormalWallpaper(pszWallpaper))
        {
            // This wallpaper is either a HTML file or an JPG etc., image.
            // Since the active desktop is currently OFF, ask the user if he wants 
            // to switch ON the active desktop!
            TCHAR szMsg[MAX_PATH];
            TCHAR szTitle[MAX_PATH];

            LoadString(HINST_THISDLL, IDS_INTERNET_EXPLORER, szTitle, ARRAYSIZE(szTitle));
            LoadString(HINST_THISDLL, IDS_CONFIRM_TURNINGON_AD, szMsg, ARRAYSIZE(szMsg));
            if(MessageBox(hwnd, szMsg, szTitle, MB_YESNO) != IDNO)
            {
                int     iComponentCount;

                // 99/05/10 #300414: Turning Active Desktop on to display
                // HTML/JPG wallpaper. Uncheck ALL components so the wallpaper
                // display unobstructed. Only uncheck a checked component and
                // apply the changes if a checked state was changed.

                if (SUCCEEDED(g_pActiveDesk->GetDesktopItemCount(&iComponentCount, 0)))
                {
                    int     i;

                    for (i = 0; i < iComponentCount; ++i)
                    {
                        COMPONENT   component;

                        component.dwSize = sizeof(component);
                        component.dwID = 0;

                        if (SUCCEEDED(g_pActiveDesk->GetDesktopItem(i, &component, 0)) &&
                            (component.fChecked != 0))
                        {
                            component.fChecked = FALSE;

                            // WARNING: DO NOT CHANGE THE ORDER OF THE FOLLOWING
                            // STATEMENT. IActiveDesktop::ModifyDesktopItem() NEEDS
                            // TO BE CALLED.

                            THR(g_pActiveDesk->ModifyDesktopItem(&component, COMP_ELEM_CHECKED));
                        }
                    }
                }

                //The end-user agreed to turn ON the active desktop.
                co.fActiveDesktop = TRUE;
                g_pActiveDesk->SetDesktopItemOptions(&co, 0);

                fRet = TRUE;
            }
        }
    }

    piadp2->Release();
    return(fRet);
}

void CBackPropSheetPage::_OnNotify(LPNMHDR lpnm)
{
    WCHAR   wszBuf[INTERNET_MAX_URL_LENGTH];
#ifndef UNICODE
    CHAR    szSelected[INTERNET_MAX_URL_LENGTH];
#endif

    switch (lpnm->code)
    {
    case PSN_SETACTIVE:
        //
        // Make sure the correct wallpaper is selected.
        //
        LPTSTR  pszBuf;

        g_pActiveDesk->GetWallpaper(wszBuf, ARRAYSIZE(wszBuf), 0);

#ifndef UNICODE
        SHUnicodeToAnsi(wszBuf, szSelected, ARRAYSIZE(szSelected));
        pszBuf = szSelected;
#else
        pszBuf = (LPTSTR)wszBuf;
#endif

        _SetNewWallpaper(pszBuf);
        break;

    case PSN_APPLY:
        {
            EnableADifHtmlWallpaper(_hwnd);
            g_pActiveDesk->ApplyChanges(AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH);
            SetWindowLongPtr(_hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        }
        break;

    case LVN_ITEMCHANGED:
        NM_LISTVIEW *pnmlv = (NM_LISTVIEW *)lpnm;
        if ((pnmlv->uChanged & LVIF_STATE) &&
            (pnmlv->uNewState & LVIS_SELECTED))
        {
            LV_ITEM lvi = {0};

            lvi.iItem = pnmlv->iItem;
            lvi.mask = LVIF_PARAM;
            ListView_GetItem(_hwndLV, &lvi);
            LPCTSTR pszSelectedNew = (LPCTSTR)lvi.lParam;
            LPCTSTR pszCurrent;

            g_pActiveDesk->GetWallpaper(wszBuf, ARRAYSIZE(wszBuf), 0);
#ifndef UNICODE
            SHUnicodeToAnsi(wszBuf, szSelected, ARRAYSIZE(szSelected));
            pszCurrent = szSelected;
#else
            pszCurrent = (LPTSTR)wszBuf;
#endif
            if (lstrcmp(pszSelectedNew, pszCurrent) != 0)
            {
                _SetNewWallpaper(pszSelectedNew);
                EnableApplyButton(_hwnd);
            }
        }
        break;
    }
}

void CBackPropSheetPage::_OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    switch (wID)
    {
    case IDC_BACK_WPSTYLE:
        switch (wNotifyCode)
        {
        case CBN_SELCHANGE:
            WALLPAPEROPT wpo;

            wpo.dwSize = sizeof(WALLPAPEROPT);
            g_pActiveDesk->GetWallpaperOptions(&wpo, 0);
            wpo.dwStyle = ComboBox_GetCurSel(_hwndWPStyle);
            g_pActiveDesk->SetWallpaperOptions(&wpo, 0);

            _EnableControls();
            _UpdatePreview(0);
            EnableApplyButton(_hwnd);
            break;
        }
        break;

    case IDC_BACK_BROWSE:
        {
            WCHAR wszFileName[INTERNET_MAX_URL_LENGTH];
            g_pActiveDesk->GetWallpaper(wszFileName, ARRAYSIZE(wszFileName), 0);

            DWORD adwFlags[] =  { GFN_PICTURE,        GFN_PICTURE,        0, 0};
            int   aiTypes[]  =  { IDS_BACK_FILETYPES, IDS_ALL_PICTURES,   0, 0};
            LPTSTR pszFileName;
#ifndef UNICODE
            CHAR  szFileName[INTERNET_MAX_URL_LENGTH];

            SHUnicodeToAnsi(wszFileName, szFileName, ARRAYSIZE(szFileName));
            pszFileName = szFileName;
#else
            pszFileName = (LPTSTR)wszFileName;
#endif
            if (_fAllowHtml)
            {
                SetFlag(adwFlags[0], GFN_LOCALHTM);
                SetFlag(adwFlags[2], GFN_LOCALHTM);
                aiTypes[2] = IDS_HTMLDOC;
            }

            if (*pszFileName == TEXT('\0'))
            {
                GetWindowsDirectory(pszFileName, ARRAYSIZE(wszFileName));
                //
                // GetFileName breaks up the string into a directory and file
                // component, so we append a slash to make sure everything
                // is considered part of the directory.
                //
                lstrcat(pszFileName, TEXT("\\"));
            }
            if (GetFileName(_hwnd, pszFileName, ARRAYSIZE(wszFileName), aiTypes, adwFlags) &&
                ValidateFileName(_hwnd, pszFileName, IDS_BACK_TYPE1))
            {
                if (_fAllowHtml || IsNormalWallpaper(pszFileName))
                {
                    _SetNewWallpaper(pszFileName);
                    EnableApplyButton(_hwnd);
                }
            }
        }
        break;

    case IDC_BACK_PATTERN:
        if (DialogBox(HINST_THISDLL, MAKEINTRESOURCE(IDD_PATTERN), _hwnd, PatternDlgProc) >= 0)
        {
            _UpdatePreview(0);
            EnableApplyButton(_hwnd);
        }
        break;
    }
}

void CBackPropSheetPage::_OnDestroy()
{
    if (g_pActiveDesk)
    {
        if(g_pActiveDesk->Release() == 0)
            g_pActiveDesk = NULL;
    }
}

BOOL_PTR CALLBACK CBackPropSheetPage::_DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBackPropSheetPage *pbpsp = (CBackPropSheetPage *)GetWindowPtr(hdlg, DWLP_USER);

    switch(uMsg)
    {
    case WM_INITDIALOG:
        pbpsp = (CBackPropSheetPage *)lParam;
        SetWindowPtr(hdlg, DWLP_USER, pbpsp);

        pbpsp->_OnInitDialog(hdlg);
        break;

    case WM_NOTIFY:
        pbpsp->_OnNotify((LPNMHDR)lParam);
        break;

    case WM_COMMAND:
        pbpsp->_OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        break;

    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DISPLAYCHANGE:
    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
        SHPropagateMessage(hdlg, uMsg, wParam, lParam, TRUE);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                HELP_WM_HELP, (ULONG_PTR)aBackHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
                (ULONG_PTR)(LPVOID) aBackHelpIDs);
        break;

    case WM_DESTROY:
        {
            TCHAR szFileName[MAX_PATH];

            //Delete the tempoaray HTX file created for non-HTML wallpaper preview.
            GetTempPath(ARRAYSIZE(szFileName), szFileName);
            lstrcatn(szFileName, PREVIEW_PICTURE_FILENAME, ARRAYSIZE(szFileName));

            DeleteFile(szFileName);

            pbpsp->_OnDestroy();
        }
        break;
    }

    return FALSE;
}
#endif
