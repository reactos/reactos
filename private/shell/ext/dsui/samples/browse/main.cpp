/*----------------------------------------------------------------------------
/ Title;
/   main.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Browse the DS
/----------------------------------------------------------------------------*/
#include "windows.h"
#include "windowsx.h"
#include "commctrl.h"
#include "resource.h"
#include "shlobj.h"
#include "shlwapi.h"

//
// Private headers from the shell/dsui projects
//

#include "comctrlp.h"
#include "shellapi.h"
#include "shsemip.h"
#include "shlwapip.h"
#include "common.h"

#pragma hdrstop

#define INITGUID
#include "dsshell.h"


/*-----------------------------------------------------------------------------
/ Globals and stuff
/----------------------------------------------------------------------------*/

#define INI_FILENAME      TEXT("browse.ini")

HINSTANCE g_hInstance = NULL;

BOOL CALLBACK Browse_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void Browse_OnInitDialog(HWND hDlg, LPTSTR pDefaultScope);
void Browse_OnBrowse(HWND hDlg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#define WRITE_STRING(tag, value)    \
            WritePrivateProfileString(TEXT("Browse"), tag, value, INI_FILENAME);
            
#define WRITE_INT(tag, value)   \
            WritePrivateProfileStruct(TEXT("Browse"), tag, &value, SIZEOF(value), INI_FILENAME)

#define READ_STRING(tag, value)    \
            GetPrivateProfileString(TEXT("Browse"), tag, TEXT(""), value, ARRAYSIZE(value), INI_FILENAME);

#define READ_INT(tag, value)    \
            GetPrivateProfileStruct(TEXT("Browse"), tag, &value, SIZEOF(value), INI_FILENAME)

#define CHECK_BUTTON(hDlg, idc, state)  \
            CheckDlgButton(hDlg, idc,  state ? BST_CHECKED:BST_UNCHECKED)
                

/*-----------------------------------------------------------------------------
/ Browse_DlgProc
/ -------------
/   Handle dialog messages for this window.
/
/ In:
/   hDlg -> dialog to be initialized
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
BOOL CALLBACK Browse_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = FALSE;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            Browse_OnInitDialog(hDlg, (LPTSTR)lParam);
            break;

        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;

                case IDC_BROWSE:
                    Browse_OnBrowse(hDlg);
                    break;

                case IDC_DEFAULTSCOPE:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_SCOPE), 
                                    IsDlgButtonChecked(hDlg, IDC_DEFAULTSCOPE)== BST_CHECKED);
                    break;
                }

                case IDC_EXPANDONOPEN:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_EXPANDTO), 
                                    IsDlgButtonChecked(hDlg, IDC_EXPANDONOPEN)== BST_CHECKED);
                    break;
                }
                
                case IDC_RETURNFORMAT:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_RETURNAS), 
                                    IsDlgButtonChecked(hDlg, IDC_RETURNFORMAT)== BST_CHECKED);
                    break;
                }

                case IDC_CREDENTIALS:
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), IsDlgButtonChecked(hDlg, IDC_CREDENTIALS)== BST_CHECKED);
                    EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), IsDlgButtonChecked(hDlg, IDC_CREDENTIALS)== BST_CHECKED);
                    break;
                }

                default:
                    break;

            }
        }
    }

    return fResult;
}


/*-----------------------------------------------------------------------------
/ Browse_OnInitDialog
/ ------------------
/   Initialize the dialog (the result viewer).
/
/ In:
/   hDlg -> dialog to be initialized
/   pDefaultScope -> default scope to use when browsing
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void Browse_OnInitDialog(HWND hDlg, LPTSTR pDefaultScope)
{
    TCHAR szBuffer[MAX_PATH];
    BOOL fNoButtons = FALSE;
    BOOL fNoLines = FALSE;
    BOOL fNoLinesAtRoot = FALSE;
    BOOL fNoRoot = FALSE;
    BOOL fIncludeHidden = FALSE;
    BOOL fExpandOnOpen = FALSE;
    BOOL fEntireDS = FALSE;
    BOOL fReturnFormat = FALSE;
    BOOL fCredentials = FALSE;

    READ_STRING(TEXT("DefaultScope"), szBuffer);
    SetDlgItemText(hDlg, IDC_SCOPE, szBuffer);
    CHECK_BUTTON(hDlg, IDC_DEFAULTSCOPE, lstrlen(szBuffer) != 0);        
    
    READ_STRING(TEXT("ExpandTo"), szBuffer);
    SetDlgItemText(hDlg, IDC_EXPANDTO, szBuffer);
    CHECK_BUTTON(hDlg, IDC_EXPANDTO, lstrlen(szBuffer) != 0);        

    READ_INT(TEXT("ExpandOnOpen"), fExpandOnOpen);
    READ_INT(TEXT("NoButtons"), fNoButtons);
    READ_INT(TEXT("NoLines"), fNoLines);
    READ_INT(TEXT("NoLinesAtRoot"), fNoLinesAtRoot);
    READ_INT(TEXT("IncludeHidden"), fIncludeHidden);
    READ_INT(TEXT("EntireDS"), fEntireDS);
    READ_INT(TEXT("ReturnFormat"), fReturnFormat);
    READ_INT(TEXT("Credentials"), fCredentials);

    CHECK_BUTTON(hDlg, IDC_EXPANDONOPEN, fExpandOnOpen);
    CHECK_BUTTON(hDlg, IDC_NOBUTTONS, fNoButtons);
    CHECK_BUTTON(hDlg, IDC_NOLINES, fNoLines);
    CHECK_BUTTON(hDlg, IDC_NOLINESATROOT, fNoLinesAtRoot);
    CHECK_BUTTON(hDlg, IDC_INCLUDEHIDDEN, fIncludeHidden);
    CHECK_BUTTON(hDlg, IDC_ENTIREDS, fEntireDS);
    CHECK_BUTTON(hDlg, IDC_RETURNFORMAT, fReturnFormat);
    CHECK_BUTTON(hDlg, IDC_CREDENTIALS, fCredentials);

    EnableWindow(GetDlgItem(hDlg, IDC_EXPANDTO), IsDlgButtonChecked(hDlg, IDC_EXPANDONOPEN)== BST_CHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_SCOPE), IsDlgButtonChecked(hDlg, IDC_DEFAULTSCOPE)== BST_CHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_RETURNAS), IsDlgButtonChecked(hDlg, IDC_RETURNFORMAT)== BST_CHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), IsDlgButtonChecked(hDlg, IDC_CREDENTIALS)== BST_CHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), IsDlgButtonChecked(hDlg, IDC_CREDENTIALS)== BST_CHECKED);
}


/*-----------------------------------------------------------------------------
/ Browse_OnBrowse
/ ----------------
/   Browse the DS using the DsBrowseForContainer API
/
/ In:
/   hDlg -> parent of find dialog
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void Browse_OnBrowse(HWND hDlg)
{
    HRESULT hr;
    TCHAR szRoot[MAX_PATH] = TEXT("");
    TCHAR szBuffer[MAX_PATH] = TEXT("");
    WCHAR szUserName[MAX_PATH] = TEXT("");
    WCHAR szPassword[MAX_PATH] = TEXT("");
    DSBROWSEINFO dsbi = { 0 };
    UINT idResult;
    BOOL fNoButtons = FALSE;
    BOOL fNoLines = FALSE;
    BOOL fNoLinesAtRoot = FALSE;
    BOOL fNoRoot = FALSE;
    BOOL fIncludeHidden = FALSE;
    BOOL fExpandOnOpen = FALSE;
    BOOL fEntireDS = FALSE;
    BOOL fReturnFormat = FALSE;
    BOOL fCredentials = FALSE;

    // initialize the structure (its already NULL)

    dsbi.cbStruct = SIZEOF(dsbi);
    dsbi.hwndOwner = hDlg;
    dsbi.pszTitle = TEXT("Select a location to start the query from");
    dsbi.pszRoot = NULL;
    dsbi.pszPath = szBuffer;
    dsbi.cchPath = ARRAYSIZE(szBuffer);
    dsbi.dwFlags = 0;
    dsbi.pfnCallback = NULL;
    dsbi.lParam = (LPARAM)0;

    // read the parameters from the dialog

    GetDlgItemText(hDlg, IDC_SCOPE, szRoot, ARRAYSIZE(szRoot));
    WRITE_STRING(TEXT("DefaultScope"), szRoot);

    GetDlgItemText(hDlg, IDC_EXPANDTO, szBuffer, ARRAYSIZE(szBuffer));
    WRITE_STRING(TEXT("ExpandTo"), szBuffer);

    fNoButtons = IsDlgButtonChecked(hDlg, IDC_NOBUTTONS) == BST_CHECKED;
    fNoLines = IsDlgButtonChecked(hDlg, IDC_NOLINES) == BST_CHECKED;
    fNoRoot = IsDlgButtonChecked(hDlg, IDC_NOROOT) == BST_CHECKED;
    fNoLinesAtRoot = IsDlgButtonChecked(hDlg, IDC_NOLINESATROOT) == BST_CHECKED;
    fIncludeHidden = IsDlgButtonChecked(hDlg, IDC_INCLUDEHIDDEN) == BST_CHECKED;
    fEntireDS = IsDlgButtonChecked(hDlg, IDC_ENTIREDS) == BST_CHECKED;
    fExpandOnOpen = IsDlgButtonChecked(hDlg, IDC_EXPANDONOPEN) == BST_CHECKED;
    fReturnFormat = IsDlgButtonChecked(hDlg, IDC_RETURNFORMAT) == BST_CHECKED;
    fCredentials = IsDlgButtonChecked(hDlg, IDC_CREDENTIALS) == BST_CHECKED;
    
    WRITE_INT(TEXT("ExpandOnOpen"), fExpandOnOpen);
    WRITE_INT(TEXT("NoButtons"), fNoButtons);
    WRITE_INT(TEXT("NoLines"), fNoLines);
    WRITE_INT(TEXT("NoLinesAtRoot"), fNoLinesAtRoot);
    WRITE_INT(TEXT("IncludeHidden"), fIncludeHidden);
    WRITE_INT(TEXT("EntireDS"), fEntireDS);
    WRITE_INT(TEXT("ReturnFormat"), fReturnFormat);
    WRITE_INT(TEXT("Credentials"), fCredentials);
              
    // browse using the flags etc set above

    if ( IsDlgButtonChecked(hDlg, IDC_DEFAULTSCOPE) == BST_CHECKED )
        dsbi.pszRoot = szRoot;

    if ( fNoButtons )
        dsbi.dwFlags |= DSBI_NOBUTTONS;

    if ( fNoLines )
        dsbi.dwFlags |= DSBI_NOLINES;

    if ( fNoLinesAtRoot )
        dsbi.dwFlags |= DSBI_NOLINESATROOT;

    if ( fIncludeHidden )
        dsbi.dwFlags |= DSBI_INCLUDEHIDDEN;

    if ( fEntireDS )
        dsbi.dwFlags |= DSBI_ENTIREDIRECTORY;

    if ( fExpandOnOpen && lstrlen(szBuffer) )
        dsbi.dwFlags |= DSBI_EXPANDONOPEN;

    if ( fReturnFormat )
        dsbi.dwFlags |= DSBI_RETURN_FORMAT;

    if ( fCredentials )
    {
        GetDlgItemText(hDlg, IDC_USERNAME, szUserName, ARRAYSIZE(szUserName));
        GetDlgItemText(hDlg, IDC_PASSWORD, szPassword, ARRAYSIZE(szPassword));

        dsbi.dwFlags |= DSBI_HASCREDENTIALS;
        dsbi.pUserName = szUserName;
        dsbi.pPassword = szPassword;
    }
        
    idResult = DsBrowseForContainer(&dsbi);

    if ( idResult != IDOK )
        wsprintf(szBuffer, TEXT("Result code was %08x"), idResult);

    SetDlgItemText(hDlg, IDC_RESULT, szBuffer);
}


/*-----------------------------------------------------------------------------
/ Main function for the app - handle initalisation etc.
/----------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pDefaultScope = NULL;

    g_hInstance = hInstance;

    InitCommonControls();
    CoInitialize(NULL);    

    if ( lpCmdLine && *lpCmdLine )
    {
        MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, szBuffer, ARRAYSIZE(szBuffer));
        pDefaultScope = szBuffer;
    }

    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_BROWSE), NULL, Browse_DlgProc, (LPARAM)pDefaultScope);

    CoUninitialize();

    return 0;
}
