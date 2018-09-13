#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"

TCHAR const szPrograms[] = TEXT("programs");

// REVIEW, we probably don't need to dyna-load commdlg, it will probably
// already be loaded

#define GETOPENFILENAME MAKEINTRESOURCE(1)      // use ordinal to get GetOpenFileName()

typedef struct {
    HWND hDlg;
    // input output
    LPTSTR   lpszExe;   // base file name (to search for)
    LPTSTR   lpszPath;  // starting location for search
    LPCTSTR  lpszName;  // doc type name "Winword Document"
#ifndef WIN32
    // local data
    HMODULE hCommDlg;   // module for hCommDlg
#endif
} FINDEXE_PARAMS, *LPFINDEXE_PARAMS;


int CALLBACK LocateCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    TCHAR szPath[MAX_PATH + 80];
    int id;
    LPFINDEXE_PARAMS lpfind = (LPFINDEXE_PARAMS)lpData;

    switch(uMsg)
    {
        case BFFM_SELCHANGED:
            SHGetPathFromIDList((LPITEMIDLIST)lParam, szPath);
            if ((lstrlen(szPath) + lstrlen(lpfind->lpszExe)) <  MAX_PATH)
            {
                PathAppend(szPath, lpfind->lpszExe);
                if (PathFileExistsAndAttributes(szPath, NULL))
                {
                    id = IDS_FILEFOUND;
                }
                else
                {
                    id = IDS_FILENOTFOUND;
                }
            }
            else
            {
                id = IDS_FILENOTFOUND;
            }
            SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, id);
            break;

        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, IDS_FILENOTFOUND);
            break;
    }
    return 0;
}

void _GetBrowseTitle(LPFINDEXE_PARAMS lpfind, LPTSTR lpszBuffer, UINT cchBuffer)
{
    TCHAR szTemplate[100];
    LoadString(HINST_THISDLL, IDS_FINDASSEXEBROWSETITLE, szTemplate, ARRAYSIZE(szTemplate));
    wnsprintf(lpszBuffer, cchBuffer, szTemplate, lpfind->lpszExe);
}

void DoBrowseForDir(LPFINDEXE_PARAMS lpfind)
{
    BROWSEINFO bi;
    LPITEMIDLIST pidl;
    TCHAR szBuffer[MAX_PATH + 100];

    bi.hwndOwner = lpfind->hDlg;
    bi.pidlRoot = NULL;

    _GetBrowseTitle(lpfind, szBuffer, ARRAYSIZE(szBuffer));
    bi.lpszTitle = szBuffer; // input and

    bi.pszDisplayName = szBuffer; // output
    bi.ulFlags = (BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_USENEWUI);
    bi.lpfn = LocateCallback;
    bi.lParam = (LPARAM)lpfind;
    pidl = SHBrowseForFolder(&bi);
    if (pidl) {
        DebugMsg(DM_TRACE, TEXT("szBuffer = %s"), szBuffer);
        SHGetPathFromIDList(pidl, szBuffer);
        SetDlgItemText(lpfind->hDlg, IDD_PATH, szBuffer);
        ILFree(pidl);
    }
}


void InitFindDlg(HWND hDlg, LPFINDEXE_PARAMS lpfind)
{
    TCHAR szPath[MAX_PATH]; /* This must be the same size as lpfind->lpszPath */
    TCHAR szBuffer[MAX_PATH + 100];

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lpfind);
    lpfind->hDlg = hDlg;

#ifndef WIN32
    lpfind->hCommDlg = NULL;    // init our local data
#endif

    GetDlgItemText(hDlg, IDD_TEXT1, szPath, ARRAYSIZE(szPath));
    wsprintf(szBuffer, szPath, lpfind->lpszExe, lpfind->lpszName);
    SetDlgItemText(hDlg, IDD_TEXT1, szBuffer);

    GetDlgItemText(hDlg, IDD_TEXT2, szPath, ARRAYSIZE(szPath));
    wsprintf(szBuffer, szPath, lpfind->lpszExe);
    SetDlgItemText(hDlg, IDD_TEXT2, szBuffer);

    SetDlgItemText(hDlg, IDD_PATH, lpfind->lpszPath);

    SHAutoComplete(GetDlgItem(hDlg, IDD_PATH), 0);
}

BOOL FindOk(LPFINDEXE_PARAMS lpfind)
{
    GetDlgItemText(lpfind->hDlg, IDD_PATH, lpfind->lpszPath, MAX_PATH);

    PathAppend(lpfind->lpszPath, lpfind->lpszExe);

    if (!PathFileExistsAndAttributes(lpfind->lpszPath, NULL))
    {
        ShellMessageBox(HINST_THISDLL, lpfind->hDlg,
                          MAKEINTRESOURCE(IDS_STILLNOTFOUND), NULL, MB_ICONHAND | MB_OK, (LPTSTR)lpfind->lpszPath);
        return FALSE;
    }

    // It would be nice if PathAppend (PathCanonicalize) quoted the path+file
    // if it contained spaces... we need to do this so that ShellExecuteNormal
    // won't get confused by the spaces and mis-parse the parameters
    if (lstrlen(lpfind->lpszPath) < MAX_PATH - 3)
    {
        TCHAR szBuf[MAX_PATH];
        wsprintf(szBuf,TEXT("\"%s\""),lpfind->lpszPath);
        lstrcpy(lpfind->lpszPath, szBuf);
    }

    // BUGBUG, we should use the registry
    WriteProfileString(szPrograms, lpfind->lpszExe, lpfind->lpszPath);
    return TRUE;
}

//----------------------------------------------------------------------------
// FindExeDlgProc was mistakenly exported in the original NT SHELL32.DLL when
// it didn't need to be (dlgproc's, like wndproc's don't need to be exported
// in the 32-bit world).  In order to maintain loadability of some stupid app
// which might have linked to it, we stub it here.  If some app ended up really
// using it, then we'll look into a specific fix for that app.
//
// -BobDay
//
BOOL_PTR WINAPI FindExeDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LONG lParam )
{
    return FALSE;
}

BOOL_PTR CALLBACK FindExeDlgProcA(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  LPFINDEXE_PARAMS lpfind = (LPFINDEXE_PARAMS)GetWindowLongPtr(hDlg, DWLP_USER);

  switch (wMsg)
    {
      case WM_INITDIALOG:
        InitFindDlg(hDlg, (LPFINDEXE_PARAMS)lParam);
        break;

#ifndef WIN32
      case WM_DESTROY:
        if (lpfind && ISVALIDHINSTANCE(lpfind->hCommDlg))
          {
            FreeLibrary(lpfind->hCommDlg);
            lpfind->hCommDlg = NULL;
          }
        break;
#endif

      case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
          {
            case IDD_BROWSE:
              DoBrowseForDir(lpfind);
              break;

            case IDOK:
              if (!FindOk(lpfind))
                break;

              // fall through

            case IDCANCEL:
              EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
              break;

            case IDHELP:
              // BUGBUG
              break;
          }

        break;

      default:
        return FALSE;
    }

  return TRUE;
}

 //
 // put up cool ui to find the exe responsible for performing
 // a ShellExecute()
 // "excel.exe foo.xls" -> "c:\excel\excel.exe foo.xls"
 //
 // in:
 //     hwnd        to post UI on
 //     lpCommand   command line to try to repair
 //     hkeyProgID  program ID
 //
 // out:
 //     lpCommand   change cmd line if we returned -1
 //
 // returns:
 //     -1  we found a new location lpCommand, use it
 //     or other win exec error codes, notably...
 //     2   we really didn't find it
 //     15  user cancel, fail the exec quietly
 //

int FindAssociatedExe(HWND hwnd, LPTSTR lpCommand,
                                 LPCTSTR pszDocument, HKEY hkeyProgID)
{
    FINDEXE_PARAMS find;
    TCHAR szPath[MAX_PATH];
    TCHAR szExe[MAX_PATH];
    TCHAR szType[MAX_PATH];
    LPTSTR lpArgs;

    // find the param list
    lstrcpy(szPath, lpCommand);
    PathRemoveArgs(szPath);

#ifdef DEBUG
    if (GetKeyState(VK_CONTROL) >= 0) {
#endif
    // check to see if the file does exist. if it does then
    // the original exec must have failed because some
    // dependant DLL is missing.  so we return file not
    // found, even though we really found the file

    PathAddExtension(szPath, NULL);
    if (PathFileExists(szPath))
        return SE_ERR_FNF;          // file exists, but some dll must not

#ifdef DEBUG
    }
#endif

    // strip off quotes around ["path+file"], otherwise
    // when we remove the path, we have [file"]
    if (szPath[0] == TEXT('"'))
    {
        int len = lstrlen(szPath);
        if (szPath[len-1] == TEXT('"'))
        {
            szPath[0] = szPath[len-1] = TEXT(' ');
            PathRemoveBlanks(szPath);
        }
        else
        {
            // badly registered command...
            ASSERT(0);
        }
    }

    // store the file name component
    lstrcpy(szExe, PathFindFileName(szPath));

    // look in win.ini
    // BUGBUG, we should store this stuff in the registry

    GetProfileString(szPrograms, szExe, szNULL, szPath, ARRAYSIZE(szPath));
    if (szPath[0]) {
        if (PathFileExists(szPath)) {
            lpArgs = PathGetArgs(lpCommand);
            if (*lpArgs) {
                lstrcat(szPath, c_szSpace);
                lstrcat(szPath, lpArgs);    // add the parameters
            }
            lstrcpy(lpCommand, szPath);     // return the new path
            return -1;                      // this means to try again
        }

        PathRemoveFileSpec(szPath);
    } else {
        /* Prompt with the disk that Windows is on */
        GetWindowsDirectory(szPath, ARRAYSIZE(szPath)-1);               // BUGBUG (DavePl) Why -1?
        szPath[3] = TEXT('\0');
    }

    SHGetTypeName(pszDocument, hkeyProgID, FALSE, szType, ARRAYSIZE(szType));

    find.lpszExe = szExe;       // base file name (to search for)
    find.lpszPath = szPath;     // starting location for search
    find.lpszName = szType;     // file type we are looking for

    switch (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_FINDEXE), hwnd, FindExeDlgProcA, (LPARAM)(LPFINDEXE_PARAMS)&find)) {
    case IDOK:
        lpArgs = PathGetArgs(lpCommand);
        if (*lpArgs) {
            lstrcat(szPath, c_szSpace);
            lstrcat(szPath, lpArgs);    // add the parameters
        }
        lstrcpy(lpCommand, szPath); // return the new path
        return -1;                  // file found and lpCommand fixed up

    case IDCANCEL:
        return ERROR_INVALID_FUNCTION; // This is the user cancel return

    default:
        return SE_ERR_FNF;             // stick with the file not found
    }
}
