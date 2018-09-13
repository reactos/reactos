#include "shellprv.h"
#include "ids.h"

#include "ftadv.h"
#include "ftcmmn.h"
#include "ftaction.h"
#include "ftassoc.h"

const static DWORD cs_rgdwHelpIDsArray[] =
{  // Context Help IDs
    IDC_FT_CMD_ACTION,         IDH_FCAB_FT_CMD_ACTION,
    IDC_FT_CMD_EXETEXT,        IDH_FCAB_FT_CMD_EXE,
    IDC_FT_CMD_EXE,            IDH_FCAB_FT_CMD_EXE,
    IDC_FT_CMD_BROWSE,         IDH_FCAB_FT_CMD_BROWSE,
    IDC_FT_CMD_DDEGROUP,       IDH_FCAB_FT_CMD_USEDDE,
    IDC_FT_CMD_USEDDE,         IDH_FCAB_FT_CMD_USEDDE,
    IDC_FT_CMD_DDEMSG,         IDH_FCAB_FT_CMD_DDEMSG,
    IDC_FT_CMD_DDEAPP,         IDH_FCAB_FT_CMD_DDEAPP,
    IDC_FT_CMD_DDEAPPNOT,      IDH_FCAB_FT_CMD_DDEAPPNOT,
    IDC_FT_CMD_DDETOPIC,       IDH_FCAB_FT_CMD_DDETOPIC,
    0, 0
};

CFTActionDlg::CFTActionDlg(PROGIDACTION* pProgIDAction, LPTSTR pszProgIDDescr,
                BOOL fEdit, BOOL fAutoDelete) :
                CFTDlg((ULONG_PTR)cs_rgdwHelpIDsArray, fAutoDelete), 
                _pProgIDAction(pProgIDAction), _pszProgIDDescr(pszProgIDDescr),
                _fEdit(fEdit)
{
}

CFTActionDlg::~CFTActionDlg()
{
}

///////////////////////////////////////////////////////////////////////////////
// Logic specific to our problem
LRESULT CFTActionDlg::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    DECLAREWAITCURSOR;

    SetWaitCursor();

    if (_fEdit || _fShowAgain)
    {
        TCHAR szTitle[50 + MAX_PROGIDDESCR + 5];
        TCHAR szTitleTemplate[50];

        _fShowAgain = FALSE;

        if (LoadString(g_hinst, IDS_FT_EDITTITLE, szTitleTemplate, ARRAYSIZE(szTitleTemplate)))
        {
            wnsprintf(szTitle, ARRAYSIZE(szTitle), szTitleTemplate, _pszProgIDDescr);
            SetWindowText(_hwnd, szTitle);
        }

        SetDlgItemText(_hwnd, IDC_FT_CMD_ACTION, _pProgIDAction->szAction);
        SetDlgItemText(_hwnd, IDC_FT_CMD_EXE, _pProgIDAction->szCmdLine);
        SetDlgItemText(_hwnd, IDC_FT_CMD_DDEMSG, _pProgIDAction->szDDEMsg);
        SetDlgItemText(_hwnd, IDC_FT_CMD_DDEAPP, _pProgIDAction->szDDEApplication);
        SetDlgItemText(_hwnd, IDC_FT_CMD_DDEAPPNOT, _pProgIDAction->szDDEAppNotRunning);
        SetDlgItemText(_hwnd, IDC_FT_CMD_DDETOPIC, _pProgIDAction->szDDETopic);

        CheckDlgButton(_hwnd, IDC_FT_CMD_USEDDE, _pProgIDAction->fUseDDE);
        _ResizeDlgForDDE(_pProgIDAction->fUseDDE);
    }
    else
    {
        CheckDlgButton(_hwnd, IDC_FT_CMD_USEDDE, FALSE);
        _ResizeDlgForDDE(FALSE);
    }

    Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_CMD_ACTION), MAX_ACTION - 1);
    Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_CMD_EXE), MAX_ACTIONCMDLINE - 1);
    Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_CMD_DDEMSG), MAX_ACTIONDDEMSG - 1);
    Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_CMD_DDEAPP), MAX_ACTIONAPPL - 1);
    Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_CMD_DDEAPPNOT), MAX_ACTIONDDEAPPNOTRUN - 1);
    Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_CMD_DDETOPIC), MAX_ACTIONTOPIC - 1);

    ResetWaitCursor();

    // Return TRUE so that system set focus
    return TRUE;
}

BOOL CFTActionDlg::_Validate()
{
    BOOL bRet = TRUE;

    // Check the Action
    TCHAR szAction[MAX_ACTION];

    if (!GetDlgItemText(_hwnd, IDC_FT_CMD_ACTION, szAction, ARRAYSIZE(szAction)) ||
        !*szAction)
    {
        ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_NOACTION), 
            MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP);

        PostMessage(_hwnd, WM_CTRL_SETFOCUS, (WPARAM)0, 
            (LPARAM)GetDlgItem(_hwnd, IDC_FT_CMD_ACTION));

        bRet = FALSE;
    }

    if (bRet)
    {
        TCHAR szPath[MAX_PATH];
        LPTSTR pszFileName = NULL;
    
        // Check for valid exe
        GetDlgItemText(_hwnd, IDC_FT_CMD_EXE, szPath, ARRAYSIZE(szPath));
        PathRemoveArgs(szPath);
        PathUnquoteSpaces(szPath);

        pszFileName = PathFindFileName(szPath);

        if(!(*szPath) ||
            !(PathIsExe(szPath)) ||
            ((!(PathFileExists(szPath))) && (!(PathFindOnPath(pszFileName, NULL)))))
        {
            // Tell user that this exe is invalid
            ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_EXETEXT),
                MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP);

            PostMessage(_hwnd, WM_CTRL_SETFOCUS, (WPARAM)0,
                (LPARAM)GetDlgItem(_hwnd, IDC_FT_CMD_EXE));

            bRet = FALSE;
        }
    }

    return bRet;
}

void CFTActionDlg::SetShowAgain()
{
    _fShowAgain = TRUE;
}

BOOL _IsThereAnyPercentArgument(LPTSTR pszCommand)
{
    BOOL fRet = FALSE;
    LPTSTR pszArgs = PathGetArgs(pszCommand);

    if (pszArgs && *pszArgs)
    {
        if (StrStr(pszArgs, TEXT("%")))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

LRESULT CFTActionDlg::OnOK(WORD wNotif)
{
    BOOL fChecksPassed = FALSE;

    if (_Validate())
    {
        GetDlgItemText(_hwnd, IDC_FT_CMD_ACTION, _pProgIDAction->szAction, MAX_ACTION);

        // Is this a new action?
        if (!_fEdit)
            // Yes, initialize the old action field
            StrCpyN(_pProgIDAction->szOldAction, _pProgIDAction->szAction,
                ARRAYSIZE(_pProgIDAction->szOldAction));

        GetDlgItemText(_hwnd, IDC_FT_CMD_EXE, _pProgIDAction->szCmdLine, MAX_ACTIONCMDLINE);
        GetDlgItemText(_hwnd, IDC_FT_CMD_DDEMSG, _pProgIDAction->szDDEMsg, MAX_ACTIONDDEMSG);
        GetDlgItemText(_hwnd, IDC_FT_CMD_DDEAPP, _pProgIDAction->szDDEApplication, MAX_ACTIONAPPL);
        GetDlgItemText(_hwnd, IDC_FT_CMD_DDEAPPNOT, _pProgIDAction->szDDEAppNotRunning, MAX_ACTIONDDEAPPNOTRUN);
        GetDlgItemText(_hwnd, IDC_FT_CMD_DDETOPIC, _pProgIDAction->szDDETopic, MAX_ACTIONTOPIC);

        _pProgIDAction->fUseDDE = IsDlgButtonChecked(_hwnd, IDC_FT_CMD_USEDDE);

        // Append %1 to action field, if required
        if (!_IsThereAnyPercentArgument(_pProgIDAction->szCmdLine))
        {
            TCHAR* pszPercentToAppend;

            if (StrChr(_pProgIDAction->szCmdLine,TEXT('\\')))
            {
                if (App_IsLFNAware(_pProgIDAction->szCmdLine))
                    pszPercentToAppend = TEXT(" \"%1\"");
                else
                    pszPercentToAppend = TEXT(" %1");
            }
            else
            {
                TCHAR szFullPathFileName[MAX_PATH];
                //
                StrCpyN(szFullPathFileName, _pProgIDAction->szCmdLine,
                    ARRAYSIZE(szFullPathFileName));
                //PathFindOnPath: first param is the filename, if it is on the path
                //  then it returns fully qualified, if not return false.
                //Second param is optional directory to look in first
                if (PathFindOnPath(szFullPathFileName, NULL))
                {
                    if (App_IsLFNAware(szFullPathFileName))
                        pszPercentToAppend = TEXT(" \"%1\"");
                    else
                        pszPercentToAppend = TEXT(" %1");
                }
                else
                {//just in case, default to good old behavior.  Should not come here because
                    // ActionExeIsValid was done earlier
                    pszPercentToAppend = TEXT(" %1");
                }
            }
            //append...
            StrCatN(_pProgIDAction->szCmdLine, pszPercentToAppend, 
                ARRAYSIZE(_pProgIDAction->szCmdLine));
        }

        EndDialog(_hwnd, IDOK);
    }
    
    return FALSE;
}

LRESULT CFTActionDlg::OnCancel(WORD wNotif)
{
    EndDialog(_hwnd, IDCANCEL);

    return FALSE;
}

LRESULT CFTActionDlg::OnUseDDE(WORD wNotif)
{
    _ResizeDlgForDDE(IsDlgButtonChecked(_hwnd, IDC_FT_CMD_USEDDE));

    return FALSE;
}

LRESULT CFTActionDlg::OnBrowse(WORD wNotif)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szTitle[40];
    TCHAR szEXE[MAX_PATH];
    TCHAR szFilters[MAX_PATH];
    LPTSTR psz;
    
    szPath[0] = 0;
    
    LoadString(g_hinst, IDS_CAP_OPENAS, szTitle, ARRAYSIZE(szTitle));
    LoadString(g_hinst, IDS_FT_EXE, szEXE, ARRAYSIZE(szEXE));
    
    // And we need to convert #'s to \0's...
    LoadString(g_hinst, IDS_PROGRAMSFILTER, szFilters, ARRAYSIZE(szFilters));

    psz = szFilters;
    while (*psz)
    {
        if (*psz == TEXT('#'))
        {
            LPTSTR pszT = psz;
            psz = CharNext(psz);
            *pszT = TEXT('\0');
        }
        else
            psz = CharNext(psz);
    }
    
    if (GetFileNameFromBrowse(_hwnd, szPath, ARRAYSIZE(szPath), NULL, szEXE, szFilters, szTitle))
    {
        PathQuoteSpaces(szPath);
        SetDlgItemText(_hwnd, IDC_FT_CMD_EXE, szPath);
    }
    
    return FALSE;
}

void CFTActionDlg::_ResizeDlgForDDE(BOOL fShow)
{
    RECT rcDialog;
    RECT rcControl;
    
    GetWindowRect(_hwnd, &rcDialog);
    
    if(fShow)
        GetWindowRect(GetDlgItem(_hwnd, IDC_FT_CMD_DDEGROUP), &rcControl);
    else
        GetWindowRect(GetDlgItem(_hwnd, IDC_FT_CMD_USEDDE), &rcControl);
    
    // Hide/Show the windows to take care of the Tabbing.  If we don't hide them then
    // we tab through the "visible" window outside of the dialog.

    ShowWindow(GetDlgItem(_hwnd, IDC_FT_CMD_DDEMSG), fShow);
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_CMD_DDEAPP), fShow);
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_CMD_DDEAPPNOT), fShow);
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_CMD_DDETOPIC), fShow);

    ShowWindow(GetDlgItem(_hwnd, IDC_FT_CMD_DDEGROUP), fShow);
    SetWindowPos(GetDlgItem(_hwnd, IDC_FT_CMD_USEDDE), HWND_TOPMOST, 
        0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    
    MoveWindow(_hwnd, rcDialog.left, rcDialog.top, rcDialog.right - rcDialog.left,
        (rcControl.bottom - rcDialog.top) + 10, TRUE);
    
    SetFocus(GetDlgItem(_hwnd, IDC_FT_CMD_USEDDE));
}

LRESULT CFTActionDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    CFTDlg::OnDestroy(wParam, lParam);

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CFTActionDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_FT_CMD_USEDDE:
            // Resize Dialog to see/hide DDE controls
            lRes = OnUseDDE(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_CMD_BROWSE:
            lRes = OnBrowse(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        default:
            lRes = CFTDlg::OnCommand(wParam, lParam);
            break;
    }

    return lRes;    
}