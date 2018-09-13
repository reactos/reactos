#ifdef unix

#include <tchar.h>
#include "inetcplp.h"
#include <shsemip.h>
#include <mluisupp.h>

#include <mainwin.h>  // _MAX_FNAME
#include <unistd.h>

#define UNIX_EDITOR_ENV TEXT("EDITOR")
#define UNIX_EDITOR_REG TEXT("command")

//
// Private Functions and Structures
//
BOOL ProgramsDlgInit( HWND hDlg);

typedef struct {
    HWND hDlg;          // dialog windows handle
    HWND hwndMail;      // Mail dropdown
    HWND hwndNews;      // News dropdown
    HWND hwndCalendar;  // Calendar dropdown
    HWND hwndContact;   // Contact dropdown
    HWND hwndCall;      // Internet call dropdown

    BOOL bAssociationCheck;     // Is IE the default browser?

    int  iMail;
    int  iNews;
    int  iCalendar;
    int  iContact;
    int  iCall; 
    BOOL fChanged;
} PROGRAMSPAGE, *LPPROGRAMSPAGE;

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef WALLET    
typedef int (*PFN_DISPLAYWALLETPAYDIALOG_PROC)(HWND, HINSTANCE, LPTSTR, int);
typedef int (*PFN_DISPLAYWALLETADDRDIALOG_PROC)(HWND, HINSTANCE, LPTSTR, int);
#endif

void FindEditClient(LPTSTR szProtocol, HWND hwndDlg, int nIDDlgItem, LPTSTR szPath)
{
    TCHAR    szCurrent[MAX_PATH];
    TCHAR    szMsg[MAX_PATH];
    HKEY    hkey;
    DWORD   dw;
    HWND    hwnd;

    // get the name of the new client
    if (hwnd = GetDlgItem(hwndDlg, nIDDlgItem))
    {
        Edit_GetText(hwnd, szCurrent, MAX_PATH);
        if (RegCreateKeyEx(HKEY_CURRENT_USER, szPath,
            0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &hkey, &dw) == ERROR_SUCCESS)
        {        
            DWORD   cb;
    
            cb = (lstrlen(szCurrent)+1)*sizeof(TCHAR);
            RegSetValueEx(hkey, REGSTR_PATH_CURRENT, NULL, REG_SZ, (LPBYTE)szCurrent, cb);
            // close the keys
            RegCloseKey(hkey);        
        
        }   // if RegCreateKeyEx()
    }
}   // FindEditClient()

BOOL  FoundProgram(HWND hwndDlg, int nIDDlgItem)
{
    TCHAR   szCurrent[MAX_PATH];
    CHAR    szCurrentA[MAX_PATH]; 
    TCHAR   szMsg[MAX_PATH];
    HWND    hwnd;
    DWORD   dwCurChar;
	BOOL    bPath = FALSE;

    // get the name of the new client
    if (hwnd = GetDlgItem(hwndDlg, nIDDlgItem))
    {
        if (!IsWindowEnabled(hwnd))
            return TRUE;
        Edit_GetText(hwnd, szCurrent, MAX_PATH);
        for (dwCurChar = 0; dwCurChar < lstrlen(szCurrent); dwCurChar++)
        {
            if (szCurrent[dwCurChar] == TEXT('/'))
            {
                bPath = TRUE;
                break;
            }
        }
        if (!bPath)  // if it's file name with no path we assume it's in the user's PATH
            return TRUE;
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, szCurrent, -1, szCurrentA, MAX_PATH, NULL, NULL);
        if (access(szCurrentA, X_OK) == 0)
#else
        if (access(szCurrent, X_OK) == 0)
#endif
            return TRUE;
    }

    return FALSE;
}   // FoundProgram()


HRESULT ViewScript(TCHAR *lpszPath)
{
    HRESULT         hr = S_OK;

    TCHAR           tszPath[MAX_PATH];
    TCHAR           tszCommand[INTERNET_MAX_URL_LENGTH];
    TCHAR           tszExpandedCommand[INTERNET_MAX_URL_LENGTH];
    UINT            nCommandSize;
    int             i;
    HKEY    hkey;
    DWORD   dw;
    TCHAR *pchPos;
    BOOL bMailed;
    STARTUPINFO stInfo;

    _tcscpy(tszPath, lpszPath);

    hr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_VSOURCECLIENTS, 0, NULL, 0, KEY_READ, NULL, &hkey, &dw);
    if (hr != ERROR_SUCCESS)
        goto Cleanup;
    dw = INTERNET_MAX_URL_LENGTH;
    hr = RegQueryValueEx(hkey, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)tszCommand, &dw);
    if (hr != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        goto Cleanup;
    }

    dw = ExpandEnvironmentStrings(tszCommand, tszExpandedCommand, INTERNET_MAX_URL_LENGTH);
    if (!dw)
     {
        _tcscpy(tszExpandedCommand, tszCommand);
     }
    _tcscat(tszCommand, tszExpandedCommand);
    for (i = _tcslen(tszCommand); i > 0; i--)
	if (tszCommand[i] == TEXT('/'))
	{
	    tszCommand[i] = TEXT('\0');
	    break;
	}
    _tcscat(tszCommand, TEXT(" "));
    _tcscat(tszCommand, tszPath);

    memset(&stInfo, 0, sizeof(stInfo));
    stInfo.cb = sizeof(stInfo);
    stInfo.wShowWindow= SW_SHOWNORMAL;
    bMailed = CreateProcess(tszExpandedCommand, tszCommand, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &stInfo, NULL);
 
Cleanup:

    return hr;
}

BOOL EditScript(HKEY hkeyProtocol)
{
    HKEY hKey;
    TCHAR tszCurrent[MAX_PATH];
    TCHAR tszScript[MAX_PATH];
    DWORD dw;

    dw = MAX_PATH;
    if (RegQueryValueEx(hkeyProtocol, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)tszCurrent, &dw)
        != ERROR_SUCCESS)
    {
        return FALSE;
    }    
    ExpandEnvironmentStrings(tszCurrent, tszScript, INTERNET_MAX_URL_LENGTH);

    return ViewScript(tszScript);
}

BOOL FindScript(HWND hwndLabel, HKEY hkeyProtocol)
{
    TCHAR tszCurrent[2*MAX_PATH + 1 ];
    TCHAR tszScript[2*MAX_PATH + 1];
    TCHAR tszFilter[5];
    DWORD dw;
    OPENFILENAME ofn;

    dw = MAX_PATH;
    if (RegQueryValueEx(hkeyProtocol, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)tszCurrent, &dw)
        != ERROR_SUCCESS)
    {
        return FALSE;
    }    

    tszCurrent[MAX_PATH] = TEXT('\0');

    ExpandEnvironmentStrings(tszCurrent, tszScript, INTERNET_MAX_URL_LENGTH);
    _tcscpy(tszCurrent, tszScript);

    BOOL bDirFound = FALSE;
    int  i;
    for (i = _tcslen(tszCurrent) - 1; i>=0; i--)
        if (tszCurrent[i] == TEXT('/') )
	{
	    tszCurrent[i] = TEXT('\0');
        bDirFound = TRUE;
	    break;
	}

    if( !bDirFound )
        tszCurrent[0] = TEXT('\0');
    else
        _tcscpy( tszScript, tszCurrent+i+1 );

    tszScript[ _MAX_FNAME - 1 ] = TEXT('\0');

    memset((void*)&tszFilter, 0, 5 * sizeof(TCHAR));
    tszFilter[0] = TEXT('*');
    tszFilter[2] = TEXT('*');

    memset((void*)&ofn, 0, sizeof(ofn));
    ofn.lpstrFilter = tszFilter;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndLabel;
    ofn.lpstrFile = tszScript;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = tszCurrent;
    ofn.Flags = OFN_HIDEREADONLY;

    if (GetOpenFileName(&ofn))
    {
        SendMessage(hwndLabel, EM_SETSEL, 0, -1);
        SendMessage(hwndLabel, EM_REPLACESEL, 0, (LPARAM)tszScript);
    }    

    return TRUE;
}
#endif


// Function used to determine if the given file exists in the directory
// that the current process is running from
BOOL LocalFileCheck(LPCTSTR aszFileName)
{
    // Determinate the base path of the current process binary
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, sizeof(szPath)/sizeof(szPath[0]));

    // Find the final element separator if there is one
#ifdef UNICODE
    LPTSTR szPathName = _tcsrchr(szPath, FILENAME_SEPARATOR_W);
#else
    LPTSTR szPathName = _tcsrchr(szPath, FILENAME_SEPARATOR);
#endif
    DWORD dwPathLength;
    if (szPathName)
    {
        szPathName[1] = TEXT('\0');

        dwPathLength = szPathName-szPath+1;
    }
    else
    {
	dwPathLength = _tcslen(szPath)+1;

#ifdef UNICODE
        _tcsncat(szPath, FILENAME_SEPARATOR_STR_W, sizeof(szPath)/
#else
        _tcsncat(szPath, FILENAME_SEPARATOR_STR, sizeof(szPath)/
#endif
            sizeof(szPath[0])-dwPathLength);
    }

    // Append the target file name to the base path (make sure
    // the new string can't overflow the buffer)
    _tcsncat(szPath, aszFileName, sizeof(szPath)/sizeof(szPath[0])-
        dwPathLength-1);

    // Look for the given file without trying to open it
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(szPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return TRUE;
    }
    else
    {
        FindClose(hFind);

        return(FALSE);
    }
}

UINT RegPopulateEditText(HWND hwndCB, HKEY hkeyProtocol)
{
    TCHAR           szCurrent           [MAX_PATH];
    TCHAR           szExpanded          [MAX_PATH];
    FILETIME        ftLastWriteTime;

    DWORD   cb;

    cb = sizeof(szCurrent);
    if (RegQueryValueEx(hkeyProtocol, REGSTR_PATH_CURRENT, NULL, NULL, (LPBYTE)szCurrent, &cb)
        != ERROR_SUCCESS)
    {
        szCurrent[0]=TEXT('\0');
    }

    ExpandEnvironmentStrings(szCurrent, szExpanded, MAX_PATH);
    SendMessage(hwndCB, EM_REPLACESEL, (WPARAM) 0, (LPARAM) szExpanded);

    return 0;
}   // RegPopulateEditText()

static const CHAR szCacheLockStatus[] = "unixGetWininetCacheLockStatus";
static const TCHAR szCacheLockStatusDll[] = TEXT("WININET.DLL");
typedef void (WINAPI *LPCACHELOCKSTATUS)(BOOL *pBoolReadOnly, TCHAR **ppszLockingHost);

BOOL IsCacheReadOnly()
{
     HINSTANCE hCacheLockStatusDll = NULL;
     LPCACHELOCKSTATUS fnCacheLockStatus;
     BOOL bReadOnlyCacheLockStatus;

     hCacheLockStatusDll = LoadLibrary(szCacheLockStatusDll); 
     if (hCacheLockStatusDll)
     {
        fnCacheLockStatus = (LPCACHELOCKSTATUS)GetProcAddress(hCacheLockStatusDll, szCacheLockStatus); 
        FreeLibrary(hCacheLockStatusDll);
     }

     if (fnCacheLockStatus)
        fnCacheLockStatus(&bReadOnlyCacheLockStatus, NULL);

     return bReadOnlyCacheLockStatus;
}

BOOL CALLBACK FontUpdDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hProg = GetDlgItem(hDlg, IDC_FONTUPD_PROG);
    UINT nTimer = 1;

    switch (uMsg)
    {
       case WM_INITDIALOG:
	    HCURSOR hOldCursor = NULL;
	    HCURSOR hNewCursor = NULL;

	    hNewCursor = LoadCursor(NULL, IDC_WAIT);
	    if (hNewCursor) 
	        hOldCursor = SetCursor(hNewCursor);
	    SendMessage(hProg, PBM_SETRANGE, 0, MAKELPARAM(0, FONT_UPDATE_TICK));
	    SendMessage(hProg, PBM_SETSTEP, 1, 0);
	    SetTimer(hDlg, nTimer, 600, NULL);
	    break;
       case WM_TIMER:
	    KillTimer(hDlg, nTimer);
	    MwFontCacheUpdate(TRUE, FontUpdateFeedBack, FONT_UPDATE_TICK, (void*)hDlg);
	    break;
       case WM_CLOSE:
            EndDialog(hDlg, 0);
       case PBM_SETRANGE:
	    SendMessage(hProg, PBM_SETRANGE, wParam, lParam);
            break;
        case PBM_SETSTEP:
	    SendMessage(hProg, PBM_SETSTEP, wParam, lParam);
            break;
        case PBM_STEPIT:
	    SendMessage(hProg, PBM_STEPIT, wParam, lParam);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}


void  FontUpdateFeedBack(int nTick, void *pvParam) 
{
    HWND hDlg = (HWND)pvParam;
    MSG msg;
    int iMsg = 0;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && iMsg++ < 20)
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    } 
    if (hDlg)
        SendMessage(hDlg, PBM_STEPIT, 0, 0);    
    if (nTick == FONT_UPDATE_TICK - 1)
        SendMessage(hDlg, WM_CLOSE, 0, 0);        
}


VOID DrawXFontButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    SIZE thin   = { GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER) };
    RECT rc     = lpdis->rcItem;
    HDC hdc     = lpdis->hDC;
    BOOL bFocus = ((lpdis->itemState & ODS_FOCUS) && !(lpdis->itemState & ODS_DISABLED));

    if (!thin.cx) thin.cx = 1;
    if (!thin.cy) thin.cy = 1;

    FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

    //Draw Icon
    HICON hXFIcon;
    if (hXFIcon = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_FONT)))
    {
        DrawIcon(hdc, (rc.right + rc.left) / 2 - 8, (rc.top + rc.bottom / 2) / 2 - 8, hXFIcon);
    }

    // Draw any caption
    TCHAR szCaption[80];
    int cyText = (rc.bottom + rc.top)/2;

    if (GetWindowText(lpdis->hwndItem, szCaption, ARRAYSIZE(szCaption)))
    {
        COLORREF crText;

        RECT rcText = rc;
        rcText.top = cyText;

        int nOldMode = SetBkMode(hdc, TRANSPARENT);

        if (lpdis->itemState & ODS_DISABLED)
        {
            // Draw disabled text using the embossed look
            crText = SetTextColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
            RECT rcOffset = rcText;
            OffsetRect(&rcOffset, 1, 1);
            DrawText(hdc, szCaption, -1, &rcOffset, DT_VCENTER|DT_SINGLELINE);
            SetTextColor(hdc, GetSysColor(COLOR_BTNSHADOW));
        }
        else
        {
            crText = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        }
        DrawText(hdc, szCaption, -1, &rcText, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
        SetTextColor(hdc, crText);
        SetBkMode(hdc, nOldMode);
    }

    // Draw the button portion
    if (lpdis->itemState & ODS_SELECTED)
    {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        OffsetRect(&rc, 1, 1);
    }
    else
    {
        DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_ADJUST);
    }

    if (bFocus)
    {
        InflateRect(&rc, -thin.cx, -thin.cy);
        DrawFocusRect(hdc, &rc);
        InflateRect(&rc, thin.cx, thin.cy);
    }
}
