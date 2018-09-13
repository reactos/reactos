#include "shwizard.h"
#include <string.h>
#include <tchar.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "winreg.h"

TCHAR g_szBmpFileName[MAX_PATH];
TCHAR g_DesktopBmpFile[MAX_PATH];
TCHAR g_szPicList[MAX_PATH];

INT_PTR APIENTRY PageT1Proc (HWND, UINT, WPARAM, LPARAM);
void PageT1_OnInitDialog (HWND);
void PageT1_OnSetActive  (HWND);

void AddPictureFilesToListBox(HWND hDlg, LPCTSTR pszDir);
void BrowseBmpToLst (HWND);
void OnSelChangePiclist (HWND);
void OnBrowse (HWND);

void OnTextChangeColor (HWND);
void OnBkgndChangeColor (HWND);
void OnDrawItem (int, LPDRAWITEMSTRUCT);
COLORREF GetButtonColor (int);
void RenderTheButton (LPDRAWITEMSTRUCT, COLORREF);


INT_PTR APIENTRY PageT1Proc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = TRUE;
    switch (msg)
    {
    case WM_COMMAND:
    {
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                OnBrowse(hDlg);
                break;
            case IDC_BTN_TEXT_COLOR:
                OnTextChangeColor(hDlg);
                break;
            case IDC_BTN_BKGND_COLOR:
                OnBkgndChangeColor(hDlg);
                break;
            default:
                bRet = FALSE;
                break;
            }
            break;
        }
        case LBN_SELCHANGE:
        {
            if (LOWORD(wParam) == IDC_PICLIST)
            {
                OnSelChangePiclist(hDlg);
            }
            break;
        }
        default:
            bRet = FALSE;
            break;
        }
        break;
    }
    case WM_DRAWITEM:
        OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
        break;
    case WM_INITDIALOG:
        PageT1_OnInitDialog(hDlg);
        return FALSE;
    case WM_DESTROY:
    {
        if (g_pCommonInfo->WasItCustomized() && g_pCommonInfo->WasThisOptionalPathUsed(IDD_PAGET1))
        {
            UpdateChangeBitmap(g_szBmpFileName);
        }
        break;
    }
    case WM_NOTIFY:
    {
        switch (((NMHDR FAR *)lParam)->code)
        {
            case PSN_QUERYCANCEL:
                bRet = FALSE;
            case PSN_KILLACTIVE:
            case PSN_RESET:
                Unsubclass(GetDlgItem(hDlg, IDC_EXPLORER));
                g_pCommonInfo->OnCancel(hDlg);
                break;
            case PSN_SETACTIVE:
                Subclass(GetDlgItem(hDlg, IDC_EXPLORER));
                PageT1_OnSetActive(hDlg);
                break;
            case PSN_WIZNEXT:
                Unsubclass(GetDlgItem(hDlg, IDC_EXPLORER));
                //ASSERT(g_pCommonInfo);
                g_pCommonInfo->OnNext(hDlg);
                break;
            case PSN_WIZBACK:
                Unsubclass(GetDlgItem(hDlg, IDC_EXPLORER));
                //ASSERT(g_pCommonInfo);
                g_pCommonInfo->OnBack(hDlg);
                break;
            default:
                bRet = FALSE;
                break;
        }
        break;
    }
    default:
        bRet = FALSE;
        break;
    }
    return bRet;
}


void GetWallpaperDirName(LPTSTR lpszWallPaperDir, int iBuffSize)
{
    //Compute the default wallpaper name.
    GetWindowsDirectory(lpszWallPaperDir, iBuffSize);
    PathAppend(lpszWallPaperDir, TEXT("Web"));

    //Read it from the registry key, if it is set!
    DWORD dwType;
    DWORD cbData = (DWORD)iBuffSize;
    SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), TEXT("WallPaperDir"), &dwType, (LPVOID)lpszWallPaperDir, &cbData);
    // BUGBUG: Currently the type is wrongly being set to REG_SZ. When this is changed by setup, uncomment the if() below.
    // if (dwType == REG_EXPAND_SZ)
    {
        TCHAR szExp[MAX_PATH];

        ExpandEnvironmentStrings(lpszWallPaperDir, szExp, ARRAYSIZE(szExp));
        lstrcpyn(lpszWallPaperDir, szExp, iBuffSize);
    }
}

void PageT1_OnInitDialog (HWND hDlg)
{
    HWND        hWndListBox;
    RECT        rect;
    COLORREF    crTemp;
    TCHAR       szDir[MAX_PATH], szTemp[MAX_PATH];

    // Files from two different directories are listed.
    // To find from which directory a particular file 
    // has come,the path of the file has been appended
    // at the end of the filename in the list box. Inorder
    // to make only the filename visible to the user,
    // it is appended after the tabstop
    // Eg:  Bubbles.bmp \t c:\Win95\Pictures

    // Set tabstop to the listbox
    hWndListBox = GetDlgItem(hDlg, IDC_PICLIST);
    GetWindowRect(hWndListBox, &rect);
    int x = rect.right - rect.left;
    SendMessage(hWndListBox, LB_SETTABSTOPS, 1, (LPARAM)&x);
    
    // Add (None) to the ListBox
    LoadString(g_hAppInst, IDS_ENTRY_NONE, szTemp, ARRAYSIZE(szTemp));
    SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)szTemp);

    // List the required files from Windows directory
    AddPictureFilesToListBox(hDlg, g_szWinDir);

    GetWallpaperDirName(szDir, ARRAYSIZE(szDir));
    AddPictureFilesToListBox(hDlg, szDir);
    
    //Get the path to the "My Pictures" folder
    if (S_OK == SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, 0, szDir))
    {
        // Add all pictures in "My Pictures" directory to the list!
        AddPictureFilesToListBox(hDlg, szDir);
    }
    
    if (lstrcmpi(g_szWinDir, g_szCurFolder))    // if the folder being customized is not %windir%
    {
        // List the required files from Current directory
        AddPictureFilesToListBox(hDlg, g_szCurFolder);
    }

    // If a folder is already customized & a bitmap 
    // background is already chosen, then bitmap listbox
    // comes-up with that bitmap item already selected.
    // Tile/Center buttons also reflect the previous
    // selection
                                                    
    if (!TemplateExists(g_szIniFile))    // Desktop.ini doesn't exist
    {
        SendMessage(hWndListBox, LB_SETCURSEL, 0, 0);   // Selects the first file in the ListBox
    }
    else
    {
        TCHAR szGUID[GUIDSTR_MAX];
        TCharStringFromGUID(VID_FolderState, szGUID);

        TCHAR szString[MAX_PATH];
        GetPrivateProfileString(szGUID, TEXT("IconArea_Text"),
                                        TEXT("0x00000000"),
                                        szString,
                                        ARRAYSIZE(szString),
                                        g_szIniFile);
        LPSTR lpszString;
        CHAR szStr[MAX_PATH];
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, szString, -1, szStr, sizeof(szStr), 0, 0);
        lpszString = szStr;
#else
        lpszString = szString;
#endif

        crTemp = (COLORREF)strtoul(lpszString, NULL, 16);

        if (crTemp != ShortcutColorText.crColor)
        {
            ShortcutColorText.crColor = crTemp;
            ShortcutColorText.iChanged = TRUE;      //FROM_INI;
        }

        GetPrivateProfileString(szGUID, TEXT("IconArea_TextBackground"),
                                        TEXT("0xFFFFFFFF"),
                                        szString,
                                        ARRAYSIZE(szString),
                                        g_szIniFile);

#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, szString, -1, szStr, sizeof(szStr), 0, 0);
        lpszString = szStr;
#else
        lpszString = szString;
#endif

        crTemp = (COLORREF)strtoul(lpszString, NULL, 16);

        if (crTemp != 0xFFFFFFFF)
        {
            ShortcutColorBkgnd.crColor = crTemp;
            ShortcutColorBkgnd.iChanged = TRUE;     //FROM_INI;
        }
        else
        {
            ShortcutColorBkgnd.crColor = GetSysColor(COLOR_3DFACE);
            ShortcutColorBkgnd.iChanged = FALSE;
        }

        int ret = GetPrivateProfileString(szGUID,
                                          TEXT("IconArea_Image"),
                                          TEXT("default"),
                                          szString,
                                          ARRAYSIZE(szString),
                                          g_szIniFile);

        if (lstrcmp(szString,TEXT("default")) != 0) {
            TCHAR szTemp2[MAX_PATH + 7];
            lstrcpyn(szTemp2, szString, ARRAYSIZE(szTemp2));
            LPTSTR pszTemp2 = szTemp2;
            if (StrCmpNI(TEXT("file://"), pszTemp2, 7) == 0) // ARRAYSIZE(TEXT("file://"))
            {
                pszTemp2 += 7;   // ARRAYSIZE(TEXT("file://"))
            }
            // handle relative references...
            PathCombine(pszTemp2, g_szCurFolder, pszTemp2);

            lstrcpy(g_DesktopBmpFile, pszTemp2);
            ret = GetPrivateProfileInt(szGUID,
                                       TEXT("IconArea_Pos"),
                                       -1,
                                       g_szIniFile);
//            if (ret != -1) {
//                m_fPicPos = ret;
//                CWnd::UpdateData(FALSE);
//            }

            /***********    
            Get the background bitmap previously
            selected. Add this to the Listbox if it is not present*/
            BrowseBmpToLst(hDlg);
        } else {
            SendMessage(hWndListBox, LB_SETCURSEL, 0, 0);
            LoadString(g_hAppInst, IDS_ENTRY_NONE, g_DesktopBmpFile, ARRAYSIZE(g_DesktopBmpFile));
        }
    }

    // set focus to the piclist...
    SetFocus(GetDlgItem(hDlg, IDC_PICLIST));
}

void PageT1_OnSetActive (HWND hDlg)
{
    // Make the preview etc. match the selection
    OnSelChangePiclist(hDlg);
    //ASSERT(g_pCommonInfo);
    g_pCommonInfo->OnSetActive(hDlg);
}   /*  end PageT1_OnSetActive() */


void AddFileToListBox(HWND hDlg, LPCTSTR pszFileName, LPCTSTR pszDir)
{
    TCHAR szString[MAX_PATH];

    lstrcpyn(szString, pszFileName, ARRAYSIZE(szString));
    StrCatBuff(szString, TEXT("\t"), ARRAYSIZE(szString));
    StrCatBuff(szString, pszDir, ARRAYSIZE(szString));
    SendMessage(GetDlgItem(hDlg, IDC_PICLIST), LB_ADDSTRING, 0, (LPARAM)szString);
}

/************************************************************
 void AddFilesToListBox(HWND hDlg, LPCTSTR Path) Fills the listbox 
 with filenames from the directory specified in the parameters

 Parameters: hDlg      - The handle to the dialog
             pszDir    - The directory to search in
             pszFilter - The filter to FindFirstFile()
 Return Value: void
 ************************************************************/

void AddFilesToListBox(HWND hDlg, LPCTSTR pszDir, LPCTSTR pszFilter)
{
    TCHAR szFilePath[MAX_PATH];
    lstrcpyn(szFilePath, pszDir, ARRAYSIZE(szFilePath));
    PathAppend(szFilePath, pszFilter);
    
    WIN32_FIND_DATA fdata;
    HANDLE handle = FindFirstFile(szFilePath, &fdata);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            AddFileToListBox(hDlg, fdata.cFileName, pszDir);
        } while (FindNextFile(handle, &fdata));
        FindClose(handle);
   }
}

static const LPCTSTR c_rgpszBackgroundFileExt[] =
{
    TEXT("*.BMP"), TEXT("*.JPG"),
    TEXT("*.GIF"), TEXT("*.TIF"),
    TEXT("*.PNG"), TEXT("*.DIB")
};

void AddPictureFilesToListBox(HWND hDlg, LPCTSTR pszDir)
{
    for (int i = 0; i < ARRAYSIZE(c_rgpszBackgroundFileExt); i++)
    {
        AddFilesToListBox(hDlg, pszDir, c_rgpszBackgroundFileExt[i]);
    }
}

/********************************************************
void BrowseBmpToLst(): Add the Back ground bitmap file name 
                       into the listbox if it does not exist
/********************************************************/

void BrowseBmpToLst (HWND hDlg)
{
    INT_PTR iCount, x;
    TCHAR *left, *right, szTemp[MAX_PATH] = {0};
    HWND hLbx = GetDlgItem(hDlg, IDC_PICLIST);
    
    left = g_DesktopBmpFile;
    right = PathFindFileName(g_DesktopBmpFile);

    lstrcpy(szTemp, right);
    lstrcat(szTemp, TEXT("\t"));
    _tcsncat(szTemp, left, (int)(right - left));
    lstrcpy(g_DesktopBmpFile, szTemp);

    iCount = SendMessage(hLbx, LB_GETCOUNT, 0, 0);

    for (x = 0; x < iCount; x++) {
        SendMessage(hLbx, LB_GETTEXT, x, (LPARAM)szTemp);
        if (lstrcmpi(g_DesktopBmpFile, szTemp) == 0) {
            SendMessage(hLbx, LB_SETCURSEL, x, 0);
            break;
        }
    }

    if (x >= iCount) {
        x = SendMessage(hLbx, LB_ADDSTRING, 0, (LPARAM)g_DesktopBmpFile);
        SendMessage(hLbx, LB_SETCURSEL, x, 0);
    }

}   /*  end BrowseBmpToLst() */


/************************************************************
 void OnSelchangePiclist(): As the selection in the 
 List box changes. Display picture() is called to display the
 selected file in the control.

 Parameters: None
 Return Value: void
 ************************************************************/

void OnSelChangePiclist (HWND hDlg)
{
    // Gets the selected string in the listbox.
    HWND hLbx = GetDlgItem(hDlg, IDC_PICLIST);
    INT_PTR index = SendMessage(hLbx, LB_GETCURSEL, 0, 0);
    SendMessage(hLbx, LB_GETTEXT, (WPARAM)index, (LPARAM)g_szPicList);

    // Rearranges the string to get the correct File name of the file to be displayed
    TCHAR szTemp[MAX_PATH];
    LoadString(g_hAppInst, IDS_ENTRY_NONE, szTemp, ARRAYSIZE(szTemp));
    if (lstrcmp(g_szPicList, szTemp) != 0)
    {
        TCHAR szTemp2[MAX_PATH];
        lstrcpyn(szTemp2, g_szPicList, ARRAYSIZE(szTemp2));

        // szTemp2 is now of the form - filename\tdirpath
        LPTSTR pszDir = StrChr(szTemp2, TEXT('\t'));
        // pszDir now points to - \tdirpath
        lstrcpyn(szTemp, pszDir + 1, ARRAYSIZE(szTemp));
        *pszDir = TEXT('\0');
        // szTemp2 is now filename\0
        PathAppend(szTemp, szTemp2);
        lstrcpy(g_szBmpFileName, szTemp);
        DisplayBackground(g_szBmpFileName, GetDlgItem(hDlg, IDC_EXPLORER));
    }
    else
    {
        lstrcpy(g_szBmpFileName, g_szPicList);
        DisplayNone(GetDlgItem(hDlg, IDC_EXPLORER));
    }
}

const TCHAR c_szFilter[] = TEXT("*.bmp;*.jpg;*.gif;*.tif;*.dib");
/************************************************************
 void OnBrowse(): This brings up the Standard File 
 Open dialog. Inorder to persist its previous location, the
 location is stored in the registry.

 Parameters: None
 Return Value: void
 ************************************************************/

void OnBrowse (HWND hDlg)
{
    // The filter is of the form DisplayString\0FilterString\0\0
    TCHAR szFilter[MAX_PATH];
    int cch = LoadString(g_hAppInst, IDS_BACKGROUND_FILES, szFilter, ARRAYSIZE(szFilter));
    lstrcpyn(szFilter + cch + 1, c_szFilter, ARRAYSIZE(szFilter) - cch - 1);
    szFilter[cch + 1 + ARRAYSIZE(c_szFilter)] = TEXT('\0');
    
    HKEY hkey;
    TCHAR szTemp[MAX_PATH];
    OPENFILENAME ofn = {0};
    DWORD dwType = REG_SZ, dwSize = SIZEOF(szTemp);
    BOOL bGotHKey = FALSE;

    szTemp[0] = TEXT('\0');
    //Queries the registry to get the previous location
    if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_FC_WIZARD, 0, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
    {
        bGotHKey = TRUE;
        if (RegQueryValueEx(hkey, REG_VAL_PERSISTEDBACKGROUNDFILENAME, 0, &dwType, (LPBYTE)szTemp, &dwSize) != ERROR_SUCCESS)
        {
            szTemp[0] = TEXT('\0');
        }
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szTemp;
    ofn.nMaxFile = ARRAYSIZE(szTemp);
    ofn.Flags = OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = TEXT("bmp");

    // Brings up the File open dialog with location from the registry
    // Displays the file selected & update the registry with the current file name

    if (GetOpenFileName(&ofn))
    {
        lstrcpy(g_DesktopBmpFile, ofn.lpstrFile);
        BrowseBmpToLst(hDlg);
        OnSelChangePiclist(hDlg);
        if (bGotHKey)
        {
            RegSetValueEx(hkey, REG_VAL_PERSISTEDBACKGROUNDFILENAME, 0, REG_SZ, (LPBYTE)ofn.lpstrFile, (lstrlen(ofn.lpstrFile) + 1) * SIZEOF(*ofn.lpstrFile));
        }
    }   

    if (bGotHKey)
    {
        RegCloseKey(hkey);
    }
}


// Refresh the preview and stuff
void OnTextChangeColor (HWND hDlg)
{
    ChooseShortcutColor(hDlg, &ShortcutColorText);

    OnSelChangePiclist (hDlg);

}   /*  end OnTextChangeColor() */


// Refresh the preview and stuff
void OnBkgndChangeColor (HWND hDlg)
{
    ChooseShortcutColor(hDlg, &ShortcutColorBkgnd);

    OnSelChangePiclist (hDlg);

}   /*  end OnBkgndChangeColor() */


void OnDrawItem (int iCtlID, LPDRAWITEMSTRUCT lpDIS)
{
    RenderTheButton(lpDIS, GetButtonColor(lpDIS->CtlID));

}   /*  end CShPageT1::OnDrawItem() */


COLORREF GetButtonColor (int iCtlID)
{
    if (iCtlID == IDC_BTN_TEXT_COLOR)
        return(ShortcutColorText.crColor);
    else
        return(ShortcutColorBkgnd.crColor);

}   /*  end GetButtonColor() */


void RenderTheButton (LPDRAWITEMSTRUCT lpDIS, COLORREF crColor)
{
    HBRUSH  hMyBrush;
    SIZE    thin;
    
    thin.cx = GetSystemMetrics(SM_CXBORDER);
    thin.cy = GetSystemMetrics(SM_CYBORDER);

    thin.cx = (!thin.cx) ? 1: thin.cx;
    thin.cy = (!thin.cy) ? 1: thin.cy;

    if (lpDIS->itemState & ODS_SELECTED) {
        DrawEdge(lpDIS->hDC, &lpDIS->rcItem, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        OffsetRect(&lpDIS->rcItem, 1, 1);
    } else
        DrawEdge(lpDIS->hDC, &lpDIS->rcItem, EDGE_RAISED, BF_RECT | BF_ADJUST);

    FillRect(lpDIS->hDC, &lpDIS->rcItem, GetSysColorBrush(COLOR_3DFACE));
        
    if (((lpDIS->itemState & ODS_FOCUS) && !(lpDIS->itemState & ODS_DISABLED))) {
        InflateRect(&lpDIS->rcItem, -thin.cx, -thin.cy);
        DrawFocusRect(lpDIS->hDC, &lpDIS->rcItem);
        InflateRect(&lpDIS->rcItem, thin.cx, thin.cy);
    }

    if (!(lpDIS->itemState & ODS_DISABLED)) {
        InflateRect(&lpDIS->rcItem, (-2 * thin.cx), (-2 * thin.cy));
        FrameRect(lpDIS->hDC, &lpDIS->rcItem, GetSysColorBrush(COLOR_3DFACE));
        InflateRect(&lpDIS->rcItem, -thin.cx, -thin.cy);

        FillRect(lpDIS->hDC, &lpDIS->rcItem, (hMyBrush = CreateSolidBrush(crColor)));
        DeleteObject(hMyBrush);
    }

}   /*  end RenderTheButton() */
