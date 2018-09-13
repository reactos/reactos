#include "shellprv.h"
#pragma  hdrstop

#include "lnktrack.h"
#include "resolve.h"

#ifdef WINNT
#include <tracker.h>
#endif

#include "treewkcb.h"

DWORD CALLBACK LinkFindThreadProc(LPVOID pv)
{
    RESOLVE_SEARCH_DATA *prs = pv;
    TCHAR szPath[MAX_PATH];

#ifdef WINNT

    HRESULT hr = S_OK;

    // Attempt to find the link using the CTracker
    // object (which uses NTFS object IDs and persisted information
    // about link-source moves).

    if (prs->ptracker)
    {
        hr = Tracker_Search( prs->ptracker,     // Explicit this pointer
                             prs->dwTimeLimit,  // GetTickCount()-relative timeout
                             prs->pfd,          // Original WIN32_FIND_DATA
                             &prs->fdFound,     // WIN32_FIND_DATA of new location
                             prs->uFlags,       // SLR_ flags
                                                // TrkMendRestriction flags
                             prs->TrackerRestrictions
                           );
 
        if( SUCCEEDED(hr) )
        {
           // We've found the link source, and we're certain it's correct.
           // So set the score to the highest possible value, and
           // return.
 
           prs->iScore = MIN_NO_UI_SCORE;
           prs->bContinue = FALSE;
        }
        else if( HRESULT_FROM_WIN32(ERROR_POTENTIAL_FILE_FOUND) == hr )
        {
            // We've found "a" link source, but we're not certain it's correct.
            // Allow the search algorithm below to run and see if it finds
            // a better match.

            prs->iScore = MIN_NO_UI_SCORE - 1;
        }
        else if( HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT) == hr )
        {
            // The CTracker search stopped because we've timed out.
            prs->bContinue = FALSE;
        }
    }   // if( prs->ptracker )

    // Attempt to find the link source using an enumerative search
    // (unless the downlevel search has been suppressed by the caller)

    if (prs->bContinue && !(prs->fifFlags & FIF_NODRIVE))
        DoDownLevelSearch(prs, szPath, MIN_NO_UI_SCORE);

#else // #ifdef WINNT

        DoDownLevelSearch(prs, szPath, MIN_NO_UI_SCORE);

#endif // #ifdef WINNT ... #else


    if (prs->hDlg)
        PostMessage(prs->hDlg, WM_COMMAND, IDOK, 0);

    return prs->iScore;
}

#define IDT_SHOWME      1

void LinkFindInit(HWND hDlg, RESOLVE_SEARCH_DATA *prs)
{
    DWORD idThread;
    TCHAR szFmt[128];
    TCHAR szTemp[MAX_PATH + ARRAYSIZE(szFmt)];

    GetDlgItemText(hDlg, IDD_NAME, szFmt, ARRAYSIZE(szFmt));
    wsprintf(szTemp, szFmt, prs->pfd->cFileName);
    SetDlgItemText(hDlg, IDD_NAME, szTemp);

    prs->hDlg = hDlg;

    prs->hThread = CreateThread(NULL, 0, LinkFindThreadProc, prs, 0, &idThread);
    if (!prs->hThread)
    {
        DebugMsg(DM_TRACE, TEXT("Failed to create search thread"));
        EndDialog(hDlg, IDCANCEL);
    }
    else
    {
        HWND hwndAni = GetDlgItem(hDlg, IDD_STATUS);

        Animate_Open(hwndAni, MAKEINTRESOURCE(IDA_SEARCH)); // open the resource
        Animate_Play(hwndAni, 0, -1, -1);     // play from start to finish and repeat
    }

#ifdef WINNT
    //
    //  Annoying flicker:  If the tracker does nothing (common case) and the
    //  shortcut refers to a drive that no longer exists, there would be
    //  some flicker as the dialog box appeared and then immediately
    //  destroyed itself.
    //
    //  Avoid this flicker by deferring the ShowWindow until 1/10 second
    //  has elapsed.  If the SetTimer fails, we'll show the dialog after all.
    //

    prs->idtDelayedShow = 0;
    if (prs->fifFlags & FIF_NODRIVE) {
        prs->idtDelayedShow = SetTimer(hDlg, IDT_SHOWME, 100, 0);
    }
#endif

}

BOOL_PTR CALLBACK LinkFindDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    RESOLVE_SEARCH_DATA *prs = (RESOLVE_SEARCH_DATA *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        LinkFindInit(hDlg, (RESOLVE_SEARCH_DATA *)lParam);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDD_BROWSE:
            prs->hDlg = NULL;           // don't let the thread close us
            prs->bContinue = FALSE;     // cancel thread

            Animate_Stop(GetDlgItem(hDlg, IDD_STATUS));

            if (GetFileNameFromBrowse(hDlg, prs->fd.cFileName, ARRAYSIZE(prs->fd.cFileName), prs->pszSearchOriginFirst, prs->pfd->cFileName, NULL, NULL))
            {
                HANDLE hfind = FindFirstFile(prs->fd.cFileName, &prs->fdFound);
                ASSERT(hfind != INVALID_HANDLE_VALUE);
                FindClose(hfind);
                lstrcpy(prs->fdFound.cFileName, prs->fd.cFileName);

                prs->iScore = MIN_NO_UI_SCORE;
                wParam = IDOK;
            }
            else
            {
                wParam = IDCANCEL;
            }
            // Fall through...

        case IDCANCEL:
            // tell searching thread to stop
            prs->bContinue = FALSE;

#ifdef WINNT
            // if the searching thread is currently in the tracker
            // waiting for results, wake it up and tell it to abort
            if( prs->ptracker )
                Tracker_CancelSearch( prs->ptracker );
#endif

            // Fall through...

        case IDOK:
            // thread posts this to us

            ASSERT(prs->hThread);

            // We will attempt to wait up to 5 seconds for the thread to terminate
            if (WaitForSingleObject(prs->hThread, 5000) == WAIT_TIMEOUT)
            {
                // BUGBUG: if this timed out we potentially leaked the list
                // of paths that we are searching (PATH_NODE list)

                TerminateThread(prs->hThread, (DWORD)-1);       // Blow it away!
            }

            CloseHandle(prs->hThread);

            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;
        }
        break;

#ifdef WINNT
    case WM_TIMER:
        if (wParam == prs->idtDelayedShow)
        {
            prs->idtDelayedShow = 0;
            KillTimer(hDlg, wParam);
            ShowWindow(hDlg, SW_SHOW);
        }
        break;

    case WM_WINDOWPOSCHANGING:
        if (prs->idtDelayedShow) {
            LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;
            pwp->flags &= ~SWP_SHOWWINDOW;
        }
        break;
#endif

    default:
        return FALSE;
    }
    return TRUE;
}

DWORD
GetTimeOut(DWORD uFlags)
{
    DWORD dwTimeOut = HIWORD(uFlags);
    if (dwTimeOut == 0)
        dwTimeOut = NOUI_SEARCH_TIMEOUT;
    else
    if (dwTimeOut == 0xFFFF)
    {
       TCHAR tszTimeOut[10];
       LONG cbTimeOut = SIZEOF(tszTimeOut);
   
       tszTimeOut[0] = 0;

       if (ERROR_SUCCESS == SHRegQueryValue(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Tracking\\TimeOut"),
                     tszTimeOut,
                     &cbTimeOut))
          dwTimeOut = StrToInt(tszTimeOut);
       else
          dwTimeOut = NOUI_SEARCH_TIMEOUT;
    }
    return(dwTimeOut);
}


typedef struct {
    LPCTSTR pszLinkName;
    LPCTSTR pszNewTarget;
} DEADLINKDATA;

BOOL_PTR DeadLinkProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DEADLINKDATA* pdld = GetWindowPtr(hwnd, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndT;
     
            pdld = (DEADLINKDATA*)lParam;
            SetWindowPtr(hwnd, DWLP_USER, pdld);

            HWNDWSPrintf(GetDlgItem(hwnd, IDC_DEADTEXT1), PathFindFileName(pdld->pszLinkName), TRUE);
        
            hwndT = GetDlgItem(hwnd, IDC_DEADTEXT2);
            if (hwndT) 
                HWNDWSPrintf(hwndT, pdld->pszNewTarget, TRUE);
            
            return TRUE;
        }

        case WM_COMMAND:
        {
            int id = GET_WM_COMMAND_ID(wParam, lParam);
            HWND hwndCtrl = GetDlgItem(hwnd, id);
            if ((id != IDHELP) && 
                SendMessage(hwndCtrl, WM_GETDLGCODE, 0, 0) & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))
            {
                EndDialog(hwnd, id);
                return TRUE;
            }
            break;
        }
    }
    
    return FALSE;
}

// in:
//      hwnd            NULL implies NOUI
//      uFlags          IShellLink::Resolve flags parameter (SLR_*)
//      pszFolder       place to look
//      ptracker        The CTracker object
//      fifFlags        FIF_* flags
//
// in/out:
//      pfd             in: thing we are looking for on input (cFileName unqualified path)
//                      out: if return is TRUE filled in with the new find info
// returns:
//      IDOK            found something
//      IDNO            didn't find it
//      IDCANCEL        user canceled the operation
//

int FindInFolder(HWND hwnd, UINT uFlags, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, LPCTSTR pszCurFile
#ifdef WINNT
                 , struct CTracker *ptracker, DWORD TrackerRestrictions,
                   UINT fifFlags
#endif
)
{
    RESOLVE_SEARCH_DATA rs;
    TCHAR szSearchStart[MAX_PATH];

    lstrcpy(szSearchStart, pszPath);
    PathRemoveFileSpec(szSearchStart);

    rs.iScore = 0;              // nothing found yet
    rs.bContinue = TRUE;
    rs.pszSearchOriginFirst = szSearchStart;
    rs.pszSearchOrigin = szSearchStart;

    rs.dwTimeLimit = GetTickCount() + UI_SEARCH_TIMEOUT;

    rs.dwMatch = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;      // must match bits
    rs.pfd = pfd;
    rs.hDlg = NULL;
    rs.uFlags = uFlags;

#ifdef WINNT
    rs.ptracker = ptracker;
    rs.TrackerRestrictions = TrackerRestrictions;
    rs.fifFlags = fifFlags;
#endif

    if (uFlags & SLR_NO_UI)
    {
        rs.dwTimeLimit = GetTickCount() + GetTimeOut(uFlags);

        LinkFindThreadProc(&rs);
    }
    else
    {
        switch (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_LINK_SEARCH), hwnd, LinkFindDlgProc, (LPARAM)&rs)) {
        case IDOK:
            break;
            
        default:
            return IDCANCEL;            // cancel stuff below
        }
    }

    if (rs.iScore < MIN_NO_UI_SCORE)
    {
        int idDlg;
        int idReturn;
        DEADLINKDATA dld;
        dld.pszLinkName = pszPath;
        dld.pszNewTarget = rs.fdFound.cFileName;
        

        if (uFlags & SLR_NO_UI)
            // the NO UI case
            return IDNO;

#ifdef WINNT
        if (rs.fifFlags & FIF_NODRIVE) {
            LPCTSTR pszName = pszCurFile ? (LPCTSTR)PathFindFileName(pszCurFile) : c_szNULL;

            ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_LINKUNAVAILABLE),
                            MAKEINTRESOURCE(IDS_LINKERROR),
                            MB_OK | MB_ICONEXCLAMATION, pszName);
            return IDNO;
        }
#endif

        if (rs.iScore <= MIN_SHOW_USER_SCORE) {
            // couldn't find anything worth reporting... offer to delete
            // DebugMsg(DM_TRACE, TEXT("ShellLink::Resolve() failed"));
            idDlg = DLG_DEADSHORTCUT;
        } else 
            idDlg = DLG_DEADSHORTCUT_MATCH;

        if (pszCurFile)
        {
            idReturn = (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(idDlg), hwnd,
                DeadLinkProc, (LPARAM)&dld);
        }
        else
        {
            if (idDlg == DLG_DEADSHORTCUT) {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_LINKNOTFOUND), MAKEINTRESOURCE(IDS_LINKERROR),
                                MB_OK | MB_ICONEXCLAMATION, PathFindFileName(pszPath));
                idReturn = IDCANCEL;
            } else {
                idReturn = ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_LINKCHANGED), MAKEINTRESOURCE(IDS_LINKERROR),
                                    MB_YESNO | MB_ICONEXCLAMATION, PathFindFileName(pszPath), rs.fdFound.cFileName);
                if (idReturn == IDNO)
                    idReturn = IDCANCEL;
            }
        }
        
        switch (idReturn) {
        case IDYES:
            // fix it!
            ASSERT(idDlg == DLG_DEADSHORTCUT_MATCH);
            // only time we can fix it is if we found some sort of match
            break;
            
        case IDC_DELETE:
            // I don't think this can be null...
            if (EVAL(pszCurFile)) {
                TCHAR szName[MAX_PATH + 1] = {0};
                SHFILEOPSTRUCT fo = {
                    hwnd,
                    FO_DELETE,
                    szName,
                    NULL, 
                    FOF_NOCONFIRMATION
                };

                lstrcpy(szName, pszCurFile);
                SHFileOperation(&fo);
            }
            // fall through

        case IDCANCEL:
            return idReturn;
        }
    }

    *pfd = rs.fdFound;
    return IDOK;
}
