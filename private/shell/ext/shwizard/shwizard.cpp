#include <windows.h>
#include <string.h>
#include <prsht.h>
#include <shlobj.h>
#include <wininet.h>
#include <shlwapi.h>
#include "resource.h"
#include "shwizard.h"
#include <tchar.h>

#ifndef CPP_FUNCTIONS
#define CPP_FUNCTIONS
#include <crtfree.h>
#endif

#define WM_CREATEWIZARD (WM_USER + 200) // Used to differ creating and processing the wizard
                                        // until after the WM_CREATE of the main window is completed
#define WM_HTML_BITMAP  (WM_USER + 201) // This is sent by the html thumbnail control
                                        // when it has completed image extraction

// globals
int g_fNextPage;        // page selection flag, used in StartPage.c
int g_iFlagA;           // Specifies whether the selected template is read-only or editable.

// The original ThumbnailWndProc, for subprocessing
WNDPROC g_lpThumbnailWndProc = NULL;

HWND      g_hWndMain;
HINSTANCE g_hAppInst;

SHORTCUTCOLOR   ShortcutColorText = {FALSE, 0x0};   // default to black, no change
SHORTCUTCOLOR   ShortcutColorBkgnd = {FALSE, 0xFFFFFFFF};   // default to transparent, no change

BOOL g_bTemplateCopied;
HWND g_hwndParent = NULL;

TCHAR g_szCurFolder[MAX_PATH];       // stores current directory
TCHAR g_szWinDir[MAX_PATH];
TCHAR g_szFullHTMLFile[MAX_PATH];    // full path to the local folder.htt file...
TCHAR g_szIniFile[MAX_PATH];         // the path to the ini file..

COLORREF g_crCustomColors[16];  // We use this to persist any user specified custom colors, across sessions

PROCESS_INFORMATION tpi;

void CreatePropertyPage (PROPSHEETPAGE *, int, DLGPROC);
INT_PTR CreateWizard ();
int CALLBACK PropSheetProc (HWND, UINT, LPARAM);
BOOL CreateThumbNailControl(HWND hWnd, IThumbnail** ppthumb);
void Init();
void DisplayBitmap(HDC hdc, HBITMAP hbm, HWND hwnd);

/*-------------------------------------------------------------------*
Function:       ModuleEntry()

Prototype:      int _stdcall ModuleEntry (void);

Description:    Module entry point, this calls WinMain()

Entry:          void

Returns:        int     -   Application return value
*--------------------------------------------------------------------*/

STDAPI_(int) ModuleEntry (void)
{
    int i;
    STARTUPINFOA si;

    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail

    SetErrorMode(SEM_FAILCRITICALERRORS);

    si.dwFlags = 0;
    GetStartupInfoA(&si);
    i = WinMain(GetModuleHandle(NULL), NULL, NULL, si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // We must use ExitThread, not ExitProcess, because some DLLs like
    // RPC create worker threads that never die (yuck)
    ExitProcess(i);  // We only come here when we are not the shell...
    return(i);

}   /*  ModuleEntry() */


/*-------------------------------------------------------------------*
Function:       WinMain()

Prototype:      int APIENTRY WinMain (HINSTANCE, HINSTANCE, LPSTR, int);

Description:    Application entry point.

Entry:          HINSTANCE   -   Module instance handle
                HINSTANCE   -   Previous module instance handle
                LPSTR       -   Module command line
                int         -   Value dictating how window should be shown

Returns:        int         -   Aplication return value
*--------------------------------------------------------------------*/

int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hAppInst = hInstance;

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        Init();
        CreateWizard();
        CoUninitialize();
    }
    return 0;

}   /*  end WinMain() */


LPTSTR StrTokEx_Local(LPTSTR pszString, LPTSTR pszDelim, LPTSTR pszToken, int cchMaxToken)
{
    if (pszToken && (cchMaxToken > 0))
    {
        pszToken[0] = TEXT('\0');
    }
    LPTSTR pszTemp = NULL;
    if (pszString && pszDelim)
    {
        pszTemp = _tcsspnp(pszString, pszDelim); // Skip leading pszDelim characters
        if (pszTemp)
        {
            int cch = _tcscspn(pszTemp, pszDelim);
            for (int i = 0; i < cch && pszTemp[i] && i < (cchMaxToken - 1); i++)
            {
                pszToken[i] = pszTemp[i];
            }
            pszToken[i] = TEXT('\0');
            pszTemp = &pszTemp[i];
        }
    }
    return pszTemp;
}


void Init()
{
    g_bTemplateCopied = FALSE;  // Always default to FALSE!  Otherwise, OnCancel()
                                // in FinishX will delete Folder.Htt.
                                // See OnCancel()

    LPTSTR pszCmdLine = GetCommandLine();
    // Skip past any white space preceeding the program name.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
    {
        pszCmdLine++;
    }
    if (*pszCmdLine == TEXT('\"'))
    {                                  
        while (*++pszCmdLine && (*pszCmdLine != TEXT('\"')))
        {
            // Do nothing
        }
        if (*pszCmdLine == TEXT('\"'))
        {
            pszCmdLine++;
        }
    }
    else
    {
        while (*pszCmdLine > TEXT(' '))
        {
            pszCmdLine++;
        }
    }
    // Skip past any white space preceeding the second token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
    {
        pszCmdLine++;
    }

    LPTSTR pszTemp = pszCmdLine;
    TCHAR szToken[MAX_PATH];
    pszTemp = StrTokEx_Local(pszTemp, TEXT(";"), szToken, ARRAYSIZE(szToken));  // Get the directory name
    if (pszTemp && szToken[0])
    {
        lstrcpy(g_szCurFolder, szToken);
        pszTemp = StrTokEx_Local(pszTemp, TEXT(";"), szToken, ARRAYSIZE(szToken));  // Get the handle to the folder window
        int iwnd = 0;
        if (szToken[0])
        {
            StrToIntEx(szToken, STIF_DEFAULT, &iwnd);
        }
        if (IsWindow((HWND)iwnd))
        {
            g_hwndParent = (HWND)iwnd;
        }
    }
    else
    {
        GetCurrentDirectory(ARRAYSIZE(g_szCurFolder), g_szCurFolder);
    }

    // Set g_szWinDir
    GetWindowsDirectory(g_szWinDir, ARRAYSIZE(g_szWinDir));

    PathCombine(g_szIniFile, g_szCurFolder, TEXT("desktop.ini"));

    // Copy out unknown.htm from the resource to %tempdir%
    InstallUnknownHTML(NULL, 0, TRUE);

    DWORD dwType, cbData = SIZEOF(g_crCustomColors);
    // Load the custom colors that the user may have chosen in the previous session.
    if (SHGetValue(HKEY_CURRENT_USER, REG_FC_WIZARD, REG_VAL_CUSTOMCOLORS, &dwType, (LPBYTE)g_crCustomColors, &cbData) != ERROR_SUCCESS)
    {
        memset(g_crCustomColors, 0, SIZEOF(g_crCustomColors));
    }
}

CCTF_CommonInfo* g_pCommonInfo = NULL;

INT_PTR CreateWizard()
{
    PROPSHEETPAGE   psp =         {0};  // defines the property sheet pages
    HPROPSHEETPAGE  ahpsp[8] =    {0};  // an array to hold the page's HPROPSHEETPAGE handles
    PROPSHEETHEADER psh =         {0};  // defines the property sheet
    CCTF_CommonInfo commonInfo;

    g_pCommonInfo = &commonInfo;
    // Create the Wizard pages
    // Common members for all pages.
    psp.dwSize =        sizeof(psp);
    psp.hInstance =     g_hAppInst;
    psp.lParam =        (LPARAM)g_pCommonInfo;
    
    // Welcome page
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_WELCOME);
    psp.pfnDlgProc =    CCTFWiz_Welcome::WndProc;
    psp.dwFlags =       PSP_DEFAULT | PSP_HIDEHEADER;
    ahpsp[0] =          CreatePropertySheetPage(&psp);
    
    // ChoosePath page
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_CHOOSE_PATH);
    psp.pfnDlgProc =        CCTF_ChoosePath::WndProc;
    psp.dwFlags =           PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_CHOOSEPATH_TITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_CHOOSEPATH_SUBTITLE);
    ahpsp[1] =              CreatePropertySheetPage(&psp);
    
    // Listview stuff page
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_PAGET1);
    psp.pfnDlgProc =        PageT1Proc;
    psp.dwFlags =           PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_BACKGROUND_TITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_BACKGROUND_SUBTITLE);
    ahpsp[2] =              CreatePropertySheetPage(&psp);
    
    // Comments page
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_COMMENT);
    psp.pfnDlgProc =        Comment_WndProc;
    psp.dwFlags =           PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_FOLDERCOMMENT_TITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_FOLDERCOMMENT_SUBTITLE);
    ahpsp[3] =              CreatePropertySheetPage(&psp);
    
    // Webview template selection page
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_PAGEA3);
    psp.pfnDlgProc =        PageA3Proc;
    psp.dwFlags =           PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_HTML_TITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HTML_SUBTITLE);
    ahpsp[4] =              CreatePropertySheetPage(&psp);

    // Finish customization page
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_FINISHT);
    psp.pfnDlgProc =    CCTFWiz_FinishCustomization::WndProc;
    psp.dwFlags =       PSP_DEFAULT | PSP_HIDEHEADER;
    ahpsp[5] =          CreatePropertySheetPage(&psp);

    // Remove customizations page
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_REMOVE);
    psp.pfnDlgProc =        RemoveProc;
    psp.dwFlags =           PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_UNCUSTOMIZE_TITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_UNCUSTOMIZE_SUBTITLE);
    ahpsp[6] =              CreatePropertySheetPage(&psp);

    // Finish remove customization page
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_FINISHR);
    psp.pfnDlgProc =    CCTFWiz_FinishUnCustomization::WndProc;
    psp.dwFlags =       PSP_DEFAULT | PSP_HIDEHEADER;
    ahpsp[7] =          CreatePropertySheetPage(&psp);

    // Create the property sheet
    psh.dwSize =            sizeof(psh);
    psh.hInstance =         g_hAppInst;
    psh.hwndParent =        g_hwndParent;
    psh.phpage =            ahpsp;
    psh.dwFlags =           PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.pszbmWatermark =    MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader    =    MAKEINTRESOURCE(IDB_BANNER);
    psh.nStartPage =        0;
    psh.nPages =            ARRAYSIZE(ahpsp);
    
    //Display the wizard and get done
    INT_PTR retVal = PropertySheet(&psh);
    if (commonInfo.WasItCustomized() || commonInfo.WasItUnCustomized())
    {
        // Persist custom colors that the user may have chosen.
        SHSetValue(HKEY_CURRENT_USER, REG_FC_WIZARD, REG_VAL_CUSTOMCOLORS, REG_BINARY, (LPBYTE)g_crCustomColors, SIZEOF(g_crCustomColors));
        ForceShellToRefresh();          // Always call ForceShellToRefresh() last!
    }
    return retVal;
}


/*-------------------------------------------------------------------*
Function:       PropSheetProc()

Prototype:      int CALLBACK PropSheetProc (HWND, UINT, LPARAM);

Description:    Called by PropertySheet() when initializing

Entry:          HWND        -   Parent window
                UINT        -   Message ID [PSCB_INITIALIZED | PSCB_PRECREATE]
                LPARAM      -   Addintional info

Returns:        int     -   Return value MUST be 0
*--------------------------------------------------------------------*/

int CALLBACK PropSheetProc (HWND hDlg, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        ShortcutColorText.iChanged  = 0;    // default to black, no change
        ShortcutColorText.crColor   = 0;
    
        ShortcutColorBkgnd.iChanged = 0;    // default to transparent, no change
        ShortcutColorBkgnd.crColor  = -1;
    }

    return(0);

}   /*  end PropSheetProc() */


/*-------------------------------------------------------------------*
Function:       CreatePropertyPage()

Prototype:      void CreatePropertyPage (PROPSHEETPAGE *, int, DLGPROC);

Description:    Fills a PROPSHEETPAGE struct

Entry:          PROPSHEETPAGE *     -   Property sheet struct to fill in
                int                 -   Dialog template ID
                DLGPROC             -   Dialog message handler

Returns:        nothing
*--------------------------------------------------------------------*/

void CreatePropertyPage (PROPSHEETPAGE *psp, int idDlg, DLGPROC pfnDlgProc)
{
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = 0;
    psp->hInstance = g_hAppInst;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pszIcon = NULL;
    psp->pfnDlgProc = pfnDlgProc;
    psp->lParam = 0;

}   /*  end FillInPropertyPage() */


// This WndProc can be subclassed by a window that needs to have thumbnail images of html's
// The client should send WM_SETIMAGE to this window with it's lParam = (LPTSTR)HtmlFilename
LRESULT APIENTRY ThumbNailSubClassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static IThumbnail* pThumbnail = NULL;    // Html to Bitmap converter
    static DWORD dwBackgroundID = 0;
    static HBITMAP hbm = NULL;
    PAINTSTRUCT ps;
    switch (message)
    {
        case WM_INITSUBPROC:
            if (!pThumbnail)
            {
                CoCreateInstance(CLSID_Thumbnail, NULL, CLSCTX_INPROC_SERVER, IID_IThumbnail, (void **)&pThumbnail);
                // Continue even if the above call fails. The preview is not critical.
            }
            if (pThumbnail)
            {
                pThumbnail->Init(hwnd, WM_HTML_BITMAP);
            }
            break;

        case WM_SETIMAGE:
            if (pThumbnail && lParam)
            {
                LPTSTR szFileName = (LPTSTR)lParam;
                LPWSTR pwsz;
#ifndef UNICODE
                WCHAR wszFileName[_MAX_PATH + 50];
                MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wszFileName, sizeof(wszFileName));
                pwsz = wszFileName;
#else
                pwsz = szFileName;
#endif
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                pThumbnail->GetBitmap(pwsz, ++dwBackgroundID, rcClient.right, rcClient.bottom);
            }
            break;

        // This is sent by the html thumbnail control when it has completed image extraction
        case WM_HTML_BITMAP:
            // may come through with NULL if the image extraction failed....
            if (wParam == dwBackgroundID && lParam)
            {
                // Display the bitmap, the handle for which is in lParam
                HDC hdc = GetDC(hwnd);
                if (hbm)
                {
                    DeleteObject(hbm);
                }
                hbm = (HBITMAP)lParam;
                DisplayBitmap(hdc, hbm, hwnd);
                ReleaseDC(hwnd, hdc);
            }
            else if (lParam)
            {
                // Bitmap for something no longer selected
                DeleteObject((HBITMAP)lParam);
            }
            break;
        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            DisplayBitmap(ps.hdc, hbm, hwnd);
            EndPaint(hwnd, &ps);
            break;

        case WM_UNINITSUBPROC:
        case WM_DESTROY:
            if (hbm)
            {
                DeleteObject(hbm);
                hbm = NULL;
            }
            if (pThumbnail)
            {
                pThumbnail->Release();
                pThumbnail = NULL;
            }
            break;

        default:
            if (g_lpThumbnailWndProc)
            {
                return (CallWindowProc(g_lpThumbnailWndProc, hwnd, message, wParam, lParam));
            }
            else
            {
                return(DefWindowProc(hwnd, message, wParam, lParam));
            }
    }

    return(0);
}

void DisplayBitmap(HDC hdc, HBITMAP hbm, HWND hwnd)
{
    if (hdc && hbm)
    {
        HDC hdcMem = CreateCompatibleDC(hdc);
        SetMapMode(hdcMem, GetMapMode(hdc));

        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
    }
}
