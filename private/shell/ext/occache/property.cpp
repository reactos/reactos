#include "item.h"
#include "parseinf.h"
#include <mstask.h>
#include <iehelpid.h>
#include "parseinf.h"

#include <mluisupp.h>

#ifdef AUTO_UPDATE
#define NUM_PAGES 4
#else
#define NUM_PAGES 3
#endif

#define DEFAULT_LANG_CODEPAGE_PAIR                   0x040904B0
#define MAX_QUERYPREFIX_LEN                          512
#define MAX_QUERYSTRING_LEN                          1024

// defined in utils.cpp
extern LPCTSTR g_lpszUpdateInfo;
extern LPCTSTR g_lpszCookieValue;
extern LPCTSTR g_lpszSavedValue;

///////////////////////////////////////////////////////////////////////////////
// functions that deal with web check

// define a macro to make life easier
#define QUIT_IF_FAIL if (FAILED(hr)) goto Exit

void DestroyDialogIcon(HWND hDlg)
{
    HICON hIcon = (HICON)SendDlgItemMessage(
                                   hDlg, IDC_STATIC_ICON, 
                                   STM_GETICON, 0, 0);
    if (hIcon != NULL)
       DestroyIcon(hIcon);

}

  
///////////////////////////////////////////////////////////////////////////////
// functions that deal with property page 1

void InitPropPage1(HWND hDlg, LPARAM lParam)
{
    BOOL bHasActiveX;
    BOOL bHasJava;

    SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    LPCONTROLPIDL pcpidl = (LPCONTROLPIDL)((LPPROPSHEETPAGE)lParam)->lParam;


    // draw control icon
    {
        HICON hIcon = ExtractIcon(g_hInst, GetStringInfo(pcpidl, SI_LOCATION), 0);
        if (hIcon == NULL)
            hIcon = GetDefaultOCIcon( pcpidl );
        Assert(hIcon != NULL);
        SendDlgItemMessage(hDlg, IDC_STATIC_ICON, STM_SETICON, (WPARAM)hIcon, 0);
    }

    SetDlgItemText(hDlg, IDC_STATIC_CONTROL, GetStringInfo(pcpidl, SI_CONTROL));
    SetDlgItemText(hDlg, IDC_STATIC_CREATION, GetStringInfo(pcpidl, SI_CREATION));
    SetDlgItemText(hDlg, IDC_STATIC_LASTACCESS, GetStringInfo(pcpidl, SI_LASTACCESS));
    SetDlgItemText(hDlg, IDC_STATIC_CLSID, GetStringInfo(pcpidl, SI_CLSID));
    SetDlgItemText(hDlg, IDC_STATIC_CODEBASE, GetStringInfo(pcpidl, SI_CODEBASE));

    TCHAR szBuf[MESSAGE_MAXSIZE];


    GetContentBools( pcpidl, &bHasActiveX, &bHasJava );
    if ( bHasJava )
    {
        if ( bHasActiveX )
            MLLoadString(IDS_PROPERTY_TYPE_MIXED, szBuf, MESSAGE_MAXSIZE);
        else
            MLLoadString(IDS_PROPERTY_TYPE_JAVA, szBuf, MESSAGE_MAXSIZE);
    }
    else
        MLLoadString(IDS_PROPERTY_TYPE_ACTX, szBuf, MESSAGE_MAXSIZE);

    SetDlgItemText(hDlg, IDC_STATIC_TYPE, szBuf);

    GetStatus(pcpidl, szBuf, MESSAGE_MAXSIZE);
    SetDlgItemText(hDlg, IDC_STATIC_STATUS, szBuf);

    DWORD dwSizeSaved = GetSizeSaved(pcpidl);
    TCHAR szSize[20];
    wsprintf(szSize, "%u", dwSizeSaved);
    
    // insert commas to separate groups of digits
    int nLen = lstrlen(szSize);
    int i = 0, j = (nLen <= 3 ? nLen : (nLen % 3));
    TCHAR *pCh = szSize + j;

    for (; i < j; i++)
        szBuf[i] = szSize[i];

    for (; *pCh != '\0'; i++, pCh++)
    {
        if (((pCh - szSize) % 3 == j) && (i > 0))
            szBuf[i++] = ',';
        szBuf[i] = *pCh;
    }
    szBuf[i] = '\0';

    TCHAR szBytes[BYTES_MAXSIZE];

    MLLoadString(IDS_PROPERTY_BYTES, szBytes, BYTES_MAXSIZE);
    lstrcat(szBuf, TEXT(" "));
    lstrcat(szBuf, szBytes);
    lstrcat(szBuf, TEXT("  ("));

    GetSizeSaved(pcpidl, szBuf + lstrlen(szBuf));
    lstrcat(szBuf, TEXT(")"));
    SetDlgItemText(hDlg, IDC_STATIC_TOTALSIZE, szBuf);
}

// Dialog proc for page 1
INT_PTR CALLBACK ControlItem_PropPage1Proc(
                                  HWND hDlg, 
                                  UINT message, 
                                  WPARAM wParam, 
                                  LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    LPCONTROLPIDL pcpidl = lpPropSheet ? (LPCONTROLPIDL)lpPropSheet->lParam : NULL;

    static DWORD aIds[] = {
        IDC_STATIC_LABEL_TYPE, IDH_DLOAD_TYPE,
        IDC_STATIC_LABEL_CREATION, IDH_DLOAD_CREATED,
        IDC_STATIC_LABEL_LASTACCESS, IDH_DLOAD_LASTACC,
        IDC_STATIC_LABEL_TOTALSIZE, IDH_DLOAD_TOTALSIZE,
        IDC_STATIC_LABEL_CLSID, IDH_DLOAD_ID,
        IDC_STATIC_LABEL_STATUS, IDH_DLOAD_STATUS,
        IDC_STATIC_LABEL_CODEBASE, IDH_DLOAD_CODEBASE,
        IDC_STATIC_TYPE, IDH_DLOAD_TYPE,
        IDC_STATIC_CREATION, IDH_DLOAD_CREATED,
        IDC_STATIC_LASTACCESS, IDH_DLOAD_LASTACC,
        IDC_STATIC_TOTALSIZE, IDH_DLOAD_TOTALSIZE,
        IDC_STATIC_CLSID, IDH_DLOAD_ID,
        IDC_STATIC_STATUS, IDH_DLOAD_STATUS,
        IDC_STATIC_CODEBASE, IDH_DLOAD_CODEBASE,
        IDC_STATIC_CONTROL, IDH_DLOAD_OBJNAME,
        0, 0 
    };

    switch(message) {
        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)(((LPHELPINFO)lParam)->hItemHandle), "iexplore.hlp", HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, "iexplore.hlp", HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)aIds);
            break;
        
        case WM_INITDIALOG:
            InitPropPage1(hDlg, lParam);
            break;            
        
        case WM_DESTROY:
            DestroyDialogIcon(hDlg);
            break;

        case WM_COMMAND:
            // user can't change anything, so we don't care about any messages

            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// functions that deal with property page 2

int ListCtrl_InsertColumn(
                       HWND hwnd,
                       int nCol, 
                       LPCTSTR lpszColumnHeading, 
                       int nFormat,
                           int nWidth, 
                       int nSubItem)
{
        LV_COLUMN column;
        column.mask = LVCF_TEXT|LVCF_FMT;
        column.pszText = (LPTSTR)lpszColumnHeading;
        column.fmt = nFormat;
        if (nWidth != -1)
        {
                column.mask |= LVCF_WIDTH;
                column.cx = nWidth;
        }
        if (nSubItem != -1)
        {
                column.mask |= LVCF_SUBITEM;
                column.iSubItem = nSubItem;
        }

    return (int)::SendMessage(hwnd, LVM_INSERTCOLUMN, nCol, (LPARAM)&column);
}

BOOL ListCtrl_SetItemText(
                     HWND hwnd,
                     int nItem, 
                     int nSubItem, 
                     LPCTSTR lpszItem)
{
        LV_ITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = nItem;
        lvi.iSubItem = nSubItem;
        lvi.stateMask = 0;
        lvi.state = 0;
        lvi.pszText = (LPTSTR) lpszItem;
        lvi.iImage = 0;
        lvi.lParam = 0;
        return (BOOL)::SendMessage(hwnd, LVM_SETITEM, 0, (LPARAM)&lvi);
}

int ListCtrl_InsertItem(
                     HWND hwnd,
                     UINT nMask, 
                     int nItem, 
                     LPCTSTR lpszItem, 
                     UINT nState, 
                     UINT nStateMask,
                         int nImage, 
                     LPARAM lParam)
{
        LV_ITEM item;
        item.mask = nMask;
        item.iItem = nItem;
        item.iSubItem = 0;
        item.pszText = (LPTSTR)lpszItem;
        item.state = nState;
        item.stateMask = nStateMask;
        item.iImage = nImage;
        item.lParam = lParam;

    return (int)::SendMessage(hwnd, LVM_INSERTITEM, 0, (LPARAM)&item);
}

void InitPropPage2(HWND hDlg, LPARAM lParam)
{
    int                iFileNameWidth = 0;

    SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    LPCONTROLPIDL pcpidl = (LPCONTROLPIDL)((LPPROPSHEETPAGE)lParam)->lParam;
    UINT cTotalFiles = GetTotalNumOfFiles(pcpidl);

    {
        HICON hIcon = ExtractIcon(g_hInst, GetStringInfo(pcpidl, SI_LOCATION), 0);
        if (hIcon == NULL)
            hIcon = GetDefaultOCIcon( pcpidl );
        Assert(hIcon != NULL);
        SendDlgItemMessage(hDlg, IDC_STATIC_ICON, STM_SETICON, (WPARAM)hIcon, 0);
    }

    // insert columns into file list box
    RECT rect;
    int nWidth;
    TCHAR szBuf[MAX_PATH];
    HWND hwndCtrl = GetDlgItem(hDlg, IDC_DEPENDENCYLIST);

    Assert(::IsWindow(hwndCtrl));
    GetClientRect(hwndCtrl, &rect);
    nWidth = rect.right - rect.left;

    MLLoadString(IDS_LISTTITLE_FILENAME, szBuf, MAX_PATH);
    iFileNameWidth = nWidth * 7 / 10;
    ListCtrl_InsertColumn(
                      hwndCtrl, 
                      0, 
                      szBuf, 
                      LVCFMT_LEFT, 
                      iFileNameWidth, 0);

    MLLoadString(IDS_LISTTITLE_FILESIZE, szBuf, MAX_PATH);
    ListCtrl_InsertColumn(
                      hwndCtrl, 
                      1, 
                      szBuf, 
                      LVCFMT_LEFT, 
                      nWidth * 3 / 10, 
                      0);

    // insert dependent files into list box
    int iIndex = -1;
    LONG lResult = ERROR_SUCCESS;
    DWORD dwFileSize = 0;
    BOOL bOCXRemovable = IsModuleRemovable(GetStringInfo(pcpidl, SI_LOCATION));

        for (UINT iFile = 0; iFile < cTotalFiles; iFile++)
        {
            if (!GetDependentFile(pcpidl, iFile, szBuf, &dwFileSize))
            {
                Assert(FALSE);
                break;
            }

            // put a star after file name if file is not safe for removal
            if (!bOCXRemovable)
            {
                lstrcat(szBuf, TEXT("*"));
            }
            else if (!IsModuleRemovable(szBuf))
            {
                // check if it is inf file.
                TCHAR szExt[10];
                MLLoadString(IDS_EXTENSION_INF, szExt, 10);
                int nLen = lstrlen(szBuf);
                int nLenExt = lstrlen(szExt);
                if ((nLen > nLenExt) && 
                    (lstrcmpi(szBuf+(nLen-nLenExt), szExt) != 0))
                    lstrcat(szBuf, TEXT("*"));
            }            

            PathCompactPath(NULL, szBuf, iFileNameWidth);
            iIndex = ListCtrl_InsertItem(hwndCtrl, LVIF_TEXT, iFile, szBuf, 0, 0, 0, 0);

            if (dwFileSize > 0)
            {
                TCHAR szBuf2[100];
                wsprintf(szBuf2, "%u", dwFileSize);
            
                // insert commas to separate groups of digits
                int nLen = lstrlen(szBuf2);
                int i = 0, j = (nLen <= 3 ? nLen : (nLen % 3));
                TCHAR *pCh = szBuf2 + j;

                for (; i < j; i++)
                    szBuf[i] = szBuf2[i];

                for (; *pCh != '\0'; i++, pCh++)
                {
                    if (((pCh - szBuf2) % 3 == j) && (i > 0))
                        szBuf[i++] = ',';
                    szBuf[i] = *pCh;
                }
                szBuf[i] = '\0';
            }
            else
                MLLoadString(IDS_STATUS_DAMAGED, szBuf, MAX_PATH);

            ListCtrl_SetItemText(hwndCtrl, iIndex, 1, szBuf);
        }

    // insert columns into file list box
    hwndCtrl = GetDlgItem(hDlg, IDC_PACKAGELIST);

    Assert(::IsWindow(hwndCtrl));
    GetClientRect(hwndCtrl, &rect);
    nWidth = (rect.right - rect.left) / 2;

    MLLoadString(IDS_LISTTITLE_PACKAGENAME, szBuf, MAX_PATH);
    ListCtrl_InsertColumn(
                      hwndCtrl, 
                      0, 
                      szBuf, 
                      LVCFMT_LEFT, 
                      nWidth, 0);

    MLLoadString(IDS_LISTTITLE_NAMESPACE, szBuf, MAX_PATH);
    ListCtrl_InsertColumn(
                      hwndCtrl, 
                      1, 
                      szBuf, 
                      LVCFMT_LEFT, 
                      nWidth, 0);

    // insert dependent packages into list box
    UINT         cTotalPackages = 0;;

    if ( pcpidl->ci.dwIsDistUnit )
    {
        CParseInf    parseInf;

        if ( SUCCEEDED(parseInf.DoParseDU( GetStringInfo( pcpidl, SI_LOCATION), GetStringInfo( pcpidl,SI_CLSID ))) )
        {
            CPackageNode *ppn;

            for ( ppn = parseInf.GetFirstPackage();
                  ppn != NULL;
                  ppn = parseInf.GetNextPackage(), cTotalPackages++ )
            {
                iIndex = ListCtrl_InsertItem(hwndCtrl, LVIF_TEXT, cTotalPackages, ppn->GetName(), 0, 0, 0, 0);
                ListCtrl_SetItemText(hwndCtrl, iIndex, 1, ppn->GetNamespace());
            }
        }
    }

     // update description
    {
        TCHAR szMsg[MESSAGE_MAXSIZE];
        TCHAR szBuf[MESSAGE_MAXSIZE];
        // BUG: This is not the correct way to make va_list for Alpha
        DWORD_PTR adwArgs[3];
        adwArgs[0] =  cTotalFiles;
        adwArgs[1] =  cTotalPackages;
        adwArgs[2] =  (DWORD_PTR) GetStringInfo(pcpidl, SI_CONTROL);
        MLLoadString(IDS_MSG_DEPENDENCY, szBuf, MESSAGE_MAXSIZE);
        FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szBuf, 0, 0, szMsg, MESSAGE_MAXSIZE, (va_list*)adwArgs );
        SetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, szMsg);
    }
}

// Dialog proc for page 2
INT_PTR CALLBACK ControlItem_PropPage2Proc(
                                  HWND hDlg, 
                                  UINT message, 
                                  WPARAM wParam, 
                                  LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    LPCONTROLPIDL pcpidl = lpPropSheet ? (LPCONTROLPIDL)lpPropSheet->lParam : NULL;

    static DWORD aIds[] = {
        IDC_DEPENDENCYLIST, IDH_DLOAD_FILE_DEP,
        IDC_PACKAGELIST, IDH_DLOAD_JAVAPKG_DEP,
        0, 0 
    };

    switch(message) {
        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)(((LPHELPINFO)lParam)->hItemHandle), "iexplore.hlp", HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, "iexplore.hlp", HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)aIds);
            break;
        
        case WM_INITDIALOG:
            InitPropPage2(hDlg, lParam);
            break;            
        
        case WM_DESTROY:
            DestroyDialogIcon(hDlg);
            break;

        case WM_COMMAND:
            // user can't change anything, so we don't care about any messages

            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}

#if 0
// Do UI update
BOOL Page3_OnCommand(HWND hDlg, WORD wCmd)
{
    HWND hwnd = GetDlgItem(hDlg, IDC_CHECK_NEVERUPDATE);
    Assert(hwnd != NULL);

    // if top check box is checked, disable the edit box
    BOOL bEnable = ((int)::SendMessage(hwnd, BM_GETCHECK, 0, 0) != 1);

    hwnd = GetDlgItem(hDlg, IDC_EDIT_UPDATEINTERVAL);
    Assert(hwnd != NULL);

    EnableWindow(hwnd, bEnable);

    // if top check box is not checked and edit box does not
    // have the focus and it is empty, put in default interval
    if (bEnable && (GetFocus() != hwnd))
    {
        TCHAR szText[10];
        if (GetWindowText(hwnd, szText, 10) == 0)
        {
            // wsprintf(szText, "%i", g_dwDefaultInterval);
            SetWindowText(hwnd, szText);
        }
    }

    return TRUE;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// functions that deal with property page 4

void InitPropPage4(HWND hDlg, LPARAM lParam)
{
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    LPCONTROLPIDL pcpidl = (LPCONTROLPIDL)((LPPROPSHEETPAGE)lParam)->lParam;

    LPTSTR lpszFileName = (LPTSTR)GetStringInfo(pcpidl, SI_LOCATION);
    TCHAR szBuf[MESSAGE_MAXSIZE];
    DWORD dwHandle = 0, dwSizeVer = 0;
    LPVOID lpData = NULL;
    LPTSTR lpBuffer = NULL;
    UINT uSize = 0;
    LPVOID lpVerData = NULL;
    UINT uLen = 0;
    DWORD dwLangCodePage = 0;
    char szQueryPrefix[MAX_QUERYPREFIX_LEN];
    char szQueryString[MAX_QUERYSTRING_LEN];
    char szLangCodePad[8]; // Padded string rep of a dword
    char *pszTmp = NULL;
    char  szVBufPad[4];  // To fit a DWORD
    char  szVBuf[4];     // To fit a DWORD (padded)
            

    // draw control icon
    {
        HICON hIcon = ExtractIcon(g_hInst, GetStringInfo(pcpidl, SI_LOCATION), 0);
        if (hIcon == NULL)
            hIcon = GetDefaultOCIcon( pcpidl );
        Assert(hIcon != NULL);
        SendDlgItemMessage(hDlg, IDC_STATIC_ICON, STM_SETICON, (WPARAM)hIcon, 0);
    }

    // set page header
    if (MLLoadString(IDS_VERSION_PAGE_HEADER, szBuf, MESSAGE_MAXSIZE))
    {
        TCHAR szHeading[MESSAGE_MAXSIZE];
        wsprintf(szHeading, szBuf, GetStringInfo(pcpidl, SI_CONTROL));
        SetDlgItemText(hDlg, IDC_STATIC_VER_HEADING, szHeading);
    }

    // set Version field
    SetDlgItemText(
             hDlg, IDC_STATIC_VER_VERSION, 
             GetStringInfo(pcpidl, SI_VERSION));

    dwSizeVer = GetFileVersionInfoSize(lpszFileName, &dwHandle);
    if (dwSizeVer <= 0)
        return;

    lpData = (LPVOID)new BYTE[dwSizeVer];
    if (lpData == NULL)
        return;


    if (GetFileVersionInfo(lpszFileName, 0, dwSizeVer, lpData))
    {
        // Get correct codepage information

        if (!VerQueryValue(lpData, "\\VarFileInfo\\Translation", &lpVerData,
                           &uLen))
        {
            wsprintf(szQueryPrefix, "\\StringFileInfo\\%x\\CompanyName",
                     DEFAULT_LANG_CODEPAGE_PAIR);
        }
        else
        {
            ASSERT(lpVerData);
            wsprintf(szVBuf, "%x", LOWORD(*((DWORD *)lpVerData)));
            
            // Pad the low word to 4 digits

            lstrcpy(szVBufPad, "0000");
            pszTmp = szVBufPad + (4 - lstrlen(szVBuf));
            ASSERT(pszTmp > szVBufPad);
            lstrcpy(pszTmp, szVBuf);

            lstrcpy(szLangCodePad, szVBufPad);

            // Pad the high word to 4 digits

            wsprintf(szVBuf, "%x", HIWORD(*((DWORD *)lpVerData)));
            lstrcpy(szVBufPad, "0000");
            pszTmp = szVBufPad + (4 - lstrlen(szVBuf));
            ASSERT(pszTmp > szVBufPad);
            lstrcpy(pszTmp, szVBuf);

            // Concatenate to get a codepage/lang-id string
            lstrcat(szLangCodePad, szVBufPad);

            lstrcpy(szQueryPrefix, "\\StringFileInfo\\");
            lstrcat(szQueryPrefix, szLangCodePad);
        }

        // set Company field
        wsprintf(szQueryString, "%s\\CompanyName", szQueryPrefix);
        if (VerQueryValue(
                     lpData,
                     szQueryString,
                     (LPVOID*)&lpBuffer, &uSize))
            SetDlgItemText(hDlg, IDC_STATIC_VER_COMPANY, lpBuffer);
        
        // set Description field
        wsprintf(szQueryString, "%s\\FileDescription", szQueryPrefix);
        if (VerQueryValue(
                     lpData,
                     szQueryString,
                     (LPVOID*)&lpBuffer, &uSize))
            SetDlgItemText(hDlg, IDC_STATIC_VER_DESCRIPTION, lpBuffer);

        // set CopyRight field
        wsprintf(szQueryString, "%s\\LegalCopyright", szQueryPrefix);
        if (VerQueryValue(
                     lpData,
                     szQueryString,
                     (LPVOID*)&lpBuffer, &uSize))
            SetDlgItemText(hDlg, IDC_STATIC_VER_COPYRIGHT, lpBuffer);

        // set Language field
        if (VerQueryValue(
                     lpData,
                     TEXT("\\VarFileInfo\\Translation"), 
                     (LPVOID*)&lpBuffer, &uSize))
        {
            LPWORD lpLangId = (LPWORD)lpBuffer;
            VerLanguageName(*lpLangId, szBuf, MESSAGE_MAXSIZE);
            SetDlgItemText(hDlg, IDC_STATIC_VER_LANGUAGE, szBuf);
        }
    }

    delete lpData;
}

// Dialog proc for page 4
INT_PTR CALLBACK ControlItem_PropPage4Proc(
                                  HWND hDlg, 
                                  UINT message, 
                                  WPARAM wParam, 
                                  LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE) GetWindowLongPtr(hDlg, DWLP_USER);
    LPCONTROLPIDL pcpidl = lpPropSheet ? (LPCONTROLPIDL)lpPropSheet->lParam : NULL;

    static DWORD aIds[] = {
        IDC_STATIC_VER_LABEL_VERSION, IDH_DLOAD_VERSION,
        IDC_STATIC_VER_LABEL_DESCRIPTION, IDH_DLOAD_DESC,
        IDC_STATIC_VER_LABEL_COMPANY, IDH_DLOAD_COMPANY,
        IDC_STATIC_VER_LABEL_LANGUAGE, IDH_DLOAD_LANG,
        IDC_STATIC_VER_LABEL_COPYRIGHT, IDH_DLOAD_COPYRIGHT,
        IDC_STATIC_VER_VERSION, IDH_DLOAD_VERSION,
        IDC_STATIC_VER_DESCRIPTION, IDH_DLOAD_DESC,
        IDC_STATIC_VER_COMPANY, IDH_DLOAD_COMPANY,
        IDC_STATIC_VER_LANGUAGE, IDH_DLOAD_LANG,
        IDC_STATIC_VER_COPYRIGHT, IDH_DLOAD_COPYRIGHT,
        IDC_STATIC_CONTROL, IDH_DLOAD_OBJNAME,
        0, 0 
    };


    switch(message) {
        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)(((LPHELPINFO)lParam)->hItemHandle), "iexplore.hlp", HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, "iexplore.hlp", HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)aIds);
            break;
        
        case WM_INITDIALOG:
            InitPropPage4(hDlg, lParam);
            break;            
        
        case WM_DESTROY:
            DestroyDialogIcon(hDlg);
            break;

        case WM_COMMAND:
            // user can't change anything, so we don't care about any messages

            break;

        default:
            return FALSE;
            
    } // end of switch
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// functions that deal with property dialog

HRESULT CreatePropDialog(
                     HWND hwnd, 
                     LPCONTROLPIDL pcpidl) 
{
#ifdef AUTO_UPDATE
    PROPSHEETPAGE psp[NUM_PAGES] = {{0},{0},{0},{0}};
#else
    PROPSHEETPAGE psp[NUM_PAGES] = {{0},{0},{0}};
#endif
    PROPSHEETHEADER psh = {0};

    // initialize propsheet page 1.
    psp[0].dwSize          = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags         = 0;
    psp[0].hInstance       = MLGetHinst();
    psp[0].pszTemplate     = MAKEINTRESOURCE(IDD_PROP_GENERAL);
    psp[0].pszIcon         = NULL;
    psp[0].pfnDlgProc      = ControlItem_PropPage1Proc;
    psp[0].pszTitle        = NULL;
    psp[0].lParam          = (LPARAM)pcpidl; // send it the cache entry struct

    // initialize propsheet page 2.
    psp[1].dwSize          = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags         = 0;
    psp[1].hInstance       = MLGetHinst();
    psp[1].pszTemplate     = MAKEINTRESOURCE(IDD_PROP_DEPENDENCY);
    psp[1].pszIcon         = NULL;
    psp[1].pfnDlgProc      = ControlItem_PropPage2Proc;
    psp[1].pszTitle        = NULL;
    psp[1].lParam          = (LPARAM)pcpidl; // send it the cache entry struct


#ifdef AUTO_UPDATE
    // initialize propsheet page 3.
    psp[2].dwSize          = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags         = 0;
    psp[2].hInstance       = MLGetHinst();
    psp[2].pszTemplate     = MAKEINTRESOURCE(IDD_PROP_UPDATE);
    psp[2].pszIcon         = NULL;
    psp[2].pfnDlgProc      = ControlItem_PropPage3Proc;
    psp[2].pszTitle        = NULL;
    psp[2].lParam          = (LPARAM)pcpidl; // send it the cache entry struct
#endif

    // initialize propsheet page 4.
    psp[NUM_PAGES-1].dwSize          = sizeof(PROPSHEETPAGE);
    psp[NUM_PAGES-1].dwFlags         = 0;
    psp[NUM_PAGES-1].hInstance       = MLGetHinst();
    psp[NUM_PAGES-1].pszTemplate     = MAKEINTRESOURCE(IDD_PROP_VERSION);
    psp[NUM_PAGES-1].pszIcon         = NULL;
    psp[NUM_PAGES-1].pfnDlgProc      = ControlItem_PropPage4Proc;
    psp[NUM_PAGES-1].pszTitle        = NULL;
    psp[NUM_PAGES-1].lParam          = (LPARAM)pcpidl; // send it the cache entry struct

    // initialize propsheet header.
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW|PSH_PROPTITLE;
    psh.hwndParent  = hwnd;
    psh.pszCaption  = GetStringInfo(pcpidl, SI_CONTROL);
    psh.nPages      = NUM_PAGES;
    psh.nStartPage  = 0;
    psh.ppsp        = (LPCPROPSHEETPAGE)&psp;

    // invoke the property sheet
    PropertySheet(&psh);

    return NOERROR;
}
