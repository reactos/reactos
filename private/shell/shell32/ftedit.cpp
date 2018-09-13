#include "shellprv.h"
#include "ids.h"
#include "help.h"

#include "ascstr.h"
#include "ftdlg.h"
#include "ftedit.h"
#include "ftcmmn.h"

#define ID_TIMER 2222

const static DWORD cs_rgdwHelpIDsArray[] =
{  // Context Help IDs
    IDC_FT_EDIT_EXT_EDIT_TEXT,  IDH_FCAB_FT_NE_FILEEXT,
    IDC_FT_EDIT_EXT_EDIT,       IDH_FCAB_FT_NE_FILEEXT,
    IDC_FT_EDIT_PID_COMBO_TEXT, IDH_FCAB_FT_NE_FILETYPE,
    IDC_FT_EDIT_PID_COMBO,      IDH_FCAB_FT_NE_FILETYPE,
    IDC_FT_EDIT_ADVANCED,       IDH_FCAB_FT_NE_ADV_BUT,
    IDC_NO_HELP_1,              NO_HELP,
    0, 0
};

CFTEditDlg::CFTEditDlg(FTEDITPARAM* pftEditParam, BOOL fAutoDelete) :
    CFTDlg((ULONG_PTR)cs_rgdwHelpIDsArray, fAutoDelete), _pftEditParam(pftEditParam),
    _iLVSel(-1)
{
}

CFTEditDlg::~CFTEditDlg()
{
}
///////////////////////////////////////////////////////////////////////////////
// Logic specific to our problem
LRESULT CFTEditDlg::OnInitDialog(WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = E_FAIL;  

    if (_pftEditParam)
    {
        hres = _InitAssocStore();

        if (SUCCEEDED(hres))
        {
            _hHeapProgID = HeapCreate(0, 8 * 1024, 0);

            if (!_hHeapProgID)
                hres = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hres))
            SetDlgItemText(_hwnd, IDC_FT_EDIT_EXT_EDIT, TEXT(""));
    }

    if (FAILED(hres))
        EndDialog(_hwnd, -1);
    else
        Edit_LimitText(GetDlgItem(_hwnd, IDC_FT_EDIT_EXT_EDIT), MAX_EXT - 1);

    // Return TRUE so that system set focus
    return TRUE;
}

LRESULT CFTEditDlg::OnEdit(WORD wNotif)
{
    if (_fAdvanced)
    {
        if (EN_CHANGE == wNotif)
        {
            if (_nTimer)
            {
                KillTimer(_hwnd, _nTimer);
                _nTimer = 0;
            }

            _nTimer = SetTimer(_hwnd, ID_TIMER, 400, NULL);
        }
    }

    return FALSE;
}

LRESULT CFTEditDlg::OnTimer(UINT nTimer)
{
    // Kill the timer
    KillTimer(_hwnd, _nTimer);
    _nTimer = 0;

    _ProgIDComboHelper();

    return TRUE;
}

HRESULT CFTEditDlg::_ProgIDComboHelper()
{
    TCHAR szExt[MAX_EXT];
    TCHAR szProgIDDescr[MAX_PROGIDDESCR];
    DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);
    HRESULT hres = E_FAIL;

    GetDlgItemText(_hwnd, IDC_FT_EDIT_EXT_EDIT, szExt, ARRAYSIZE(szExt));

    hres = _GetProgIDDescrFromExt(szExt, szProgIDDescr, &cchProgIDDescr);
    
    if (SUCCEEDED(hres))
        _SelectProgIDDescr(szProgIDDescr);

    return hres;
}

HRESULT CFTEditDlg::_GetProgIDDescrFromExt(LPTSTR pszExt, LPTSTR pszProgIDDescr,
        DWORD* pcchProgIDDescr)
{
    IAssocInfo* pAI = NULL;
    HRESULT hres = _pAssocStore->GetAssocInfo(pszExt, AIINIT_EXT, &pAI);

    if (SUCCEEDED(hres))
    {
        hres = pAI->GetString(AISTR_PROGIDDESCR, pszProgIDDescr, pcchProgIDDescr);
        pAI->Release();
    }
    return hres;
}

LRESULT CFTEditDlg::OnAdvancedButton(WORD wNotif)
{
    DECLAREWAITCURSOR;
    TCHAR szAdvBtnText[50];

    SetWaitCursor();

    _fAdvanced = !_fAdvanced;

    LoadString(g_hinst, _fAdvanced ? IDS_FT_ADVBTNTEXTEXPAND : IDS_FT_ADVBTNTEXTCOLLAPS,
        szAdvBtnText, ARRAYSIZE(szAdvBtnText));

    SetWindowText(GetDlgItem(_hwnd, IDC_FT_EDIT_ADVANCED), szAdvBtnText);

    _ConfigureDlg();

    UpdateWindow(_hwnd);

    if (_fAdvanced)
    {
        HWND hwndCombo = GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO);

        // Is the combobox filled yet?
        if (!ComboBox_GetCount(hwndCombo))
        {
            _FillProgIDDescrCombo();

            // Select the <New> item

            if (FAILED(_ProgIDComboHelper()))
            {
                TCHAR szNew[20];

                if (LoadString(g_hinst, IDS_FT_NEW, szNew, ARRAYSIZE(szNew)))
                {
                    int iIndex = ComboBox_FindStringExact(hwndCombo, -1, szNew);

                    if (CB_ERR != iIndex)
                        ComboBox_SetCurSel(hwndCombo, iIndex);
                }
            }
        }
    }

    ResetWaitCursor();

    return FALSE;
}

void CFTEditDlg::_ConfigureDlg()
{
    // Need to:
    //  - position OK and Cancel
    //  - resize dlg
    //  - Show/hide Combo and its text

    RECT rcControl;
    RECT rcDialog;
    RECT rcCancel;
    RECT rcOK;

    int iStdMargins = 0;
    int iStdSpaceBetweenControls = 0;

    GetWindowRect(_hwnd, &rcDialog);

    GetWindowRect(GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO_TEXT), 
        &rcControl);

    // Calculate the folowing (cannot be fixed, varies with dialog font)

    // [msadek]; screen coordinates. need to consider the mirrored case
    if(IS_WINDOW_RTL_MIRRORED(_hwnd))
    {
        iStdMargins = rcDialog.right - rcControl.right;
    }
    else
    {
        iStdMargins = rcControl.left - rcDialog.left;    
    }
    iStdSpaceBetweenControls = MulDiv(4, iStdMargins, 7);

    // Move Cancel and OK button
    GetWindowRect(GetDlgItem(_hwnd, 
        _fAdvanced ? IDC_FT_EDIT_PID_COMBO : IDC_FT_EDIT_EXT_EDIT), 
        &rcControl);

    MapWindowRect(HWND_DESKTOP, _hwnd, &rcControl);

    GetWindowRect(GetDlgItem(_hwnd, IDCANCEL), &rcCancel);
    MapWindowRect(HWND_DESKTOP, _hwnd, &rcCancel);

    OffsetRect(&rcCancel, 0, -rcCancel.top);

    GetWindowRect(GetDlgItem(_hwnd, IDOK), &rcOK);
    MapWindowRect(HWND_DESKTOP, _hwnd, &rcOK); 
    OffsetRect(&rcOK, 0, -rcOK.top);

    OffsetRect(&rcCancel, 0, rcControl.bottom + iStdSpaceBetweenControls);
    OffsetRect(&rcOK, 0, rcControl.bottom + iStdSpaceBetweenControls);

    SetWindowPos(GetDlgItem(_hwnd, IDOK), NULL, 
            rcOK.left, rcOK.top, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
    SetWindowPos(GetDlgItem(_hwnd, IDCANCEL), NULL, 
            rcCancel.left, rcCancel.top, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);

    // Resize Dlg
    ClientToScreen(_hwnd, ((POINT*)&rcCancel) + 1);

    rcDialog.bottom = rcCancel.bottom + iStdMargins;
    SetWindowPos(_hwnd, NULL, 
            0, 0, rcDialog.right - rcDialog.left, rcDialog.bottom - rcDialog.top,
            SWP_NOMOVE|SWP_SHOWWINDOW|SWP_NOZORDER);

    // Show/Hide Combo and its text
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO_TEXT), _fAdvanced);
    ShowWindow(GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO), _fAdvanced);

    // Set focus to combo
    SetFocus(GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO));
}

LRESULT CFTEditDlg::OnOK(WORD wNotif)
{
    HRESULT hres = S_FALSE;

    // Pick up the extension
    GetDlgItemText(_hwnd, IDC_FT_EDIT_EXT_EDIT, _pftEditParam->szExt,
        _pftEditParam->dwExt);

    // Is it empty?
    if (0 != (*_pftEditParam->szExt))
    {
        // No, that's good

        // BUGBUG: do some check for valid extension name

        IAssocInfo* pAI = NULL;

        hres = _pAssocStore->GetAssocInfo(_pftEditParam->szExt, 
                            AIINIT_EXT, &pAI);

        if (SUCCEEDED(hres))
        {
            BOOL fExist = FALSE;

            hres = pAI->GetBOOL(AIBOOL_EXTEXIST, &fExist);

            // Is this extension already existing?
            if (SUCCEEDED(hres) && !fExist)
            {
                // No, create it
                // Check for spaces in the ext name
                LPTSTR pszExt = _pftEditParam->szExt;

                while (*pszExt && (S_FALSE != hres))
                {
                    if (TEXT(' ') == *pszExt)
                    {
                        hres = S_FALSE;

                        ShellMessageBox(g_hinst, _hwnd,
                            MAKEINTRESOURCE(IDS_FT_MB_NOSPACEINEXT),
                            MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP);

                        // Set focus to Ext combo
                        PostMessage(_hwnd, WM_CTRL_SETFOCUS, (WPARAM)0,
                            (LPARAM)GetDlgItem(_hwnd, IDC_FT_EDIT_EXT_EDIT));
                    }

                    ++pszExt;
                }

                if (SUCCEEDED(hres) && (S_FALSE != hres))
                    hres = pAI->Create();
            }

            if (SUCCEEDED(hres) && (S_FALSE != hres))
                hres = _HandleProgIDAssoc(pAI, _pftEditParam->szExt, fExist);

            pAI->Release();
        }
    }
    else
    {
        ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_MB_NOEXT),
            MAKEINTRESOURCE(IDS_FT), MB_OK | MB_ICONSTOP);

        // Set focus to Ext combo
        PostMessage(_hwnd, WM_CTRL_SETFOCUS, (WPARAM)0,
            (LPARAM)GetDlgItem(_hwnd, IDC_FT_EDIT_EXT_EDIT));
    }
    
    // If we fail, we are in serious trouble, so we just close the dialog
    ASSERT(SUCCEEDED(hres));

    if (S_FALSE != hres)
        EndDialog(_hwnd, IDOK);

    return FALSE;
}

HRESULT CFTEditDlg::_GetProgIDInfo(IAssocInfo* pAI, LPTSTR pszProgID,
    DWORD* pcchProgID, BOOL* pfNewProgID, BOOL* pfExplicitNew)
{
    HWND hwndCombo = GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO);

    *pfNewProgID = FALSE;
    *pfExplicitNew = FALSE;

    if (ComboBox_GetCount(hwndCombo))
    {
        int iSel = ComboBox_GetCurSel(hwndCombo);

        if (CB_ERR != iSel)
        {
            LPTSTR pszTmpProgID = (LPTSTR)ComboBox_GetItemData(hwndCombo, iSel);

            // Is this the "<New>" item (the only one with a ProgID == NULL)?
            if (!pszTmpProgID)
            {
                // Yes
                *pfNewProgID = TRUE;
                *pfExplicitNew = TRUE;
            }
            else
            {
                // No
                StrCpyN(pszProgID, pszTmpProgID, *pcchProgID);
            }
        }        
    }
    else
    {
        *pfNewProgID = TRUE;
    }

    return S_OK;
}

HRESULT CFTEditDlg::_HandleProgIDAssoc(IAssocInfo* pAI, LPTSTR pszExt, BOOL fExtExist)
{
    TCHAR szProgID[MAX_PROGID];
    DWORD cchProgID = ARRAYSIZE(szProgID);
    BOOL fNewProgID = FALSE;
    BOOL fExplicitNew = FALSE;

    *szProgID = 0;
    HRESULT hres = _GetProgIDInfo(pAI, szProgID, &cchProgID, &fNewProgID, &fExplicitNew);

    if (SUCCEEDED(hres))
    {
        // Is this Extension already existing?
        if (fExtExist)
        {
            //
            // First make sure it's not the exact same ext - progID assoc
            //
            TCHAR szTmpProgID[MAX_PROGID];
            DWORD cchTmpProgID = ARRAYSIZE(szTmpProgID);

            hres = pAI->GetString(AISTR_PROGID, szTmpProgID, &cchTmpProgID);

            // Did we got a progID?
            if (SUCCEEDED(hres))
            { 
                // Yes
                // Are they the same?
                if (0 == lstrcmpi(szTmpProgID, szProgID))
                {
                    // Yes, fail, nothing more to do
                    hres = E_FAIL;
                }
                else
                {
                    // No, go on
                    hres = S_OK;
                }
            }
            else
            {
                // No, there probably is no ProgID, go on
                hres = S_OK;
            }
            //
            // Unless the user chose <New> explicitly ask if he wants to break the assoc
            //
            // Did the user explicitly chose <New> (and we did not failed already)?
            if (!fExplicitNew && SUCCEEDED(hres))
            {
                // We need to warn user that he will break existing assoc
                TCHAR szProgIDDescr[MAX_PROGIDDESCR];
                DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);

                hres = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr, &cchProgIDDescr);

                if (SUCCEEDED(hres))
                {
                    if (IDNO == ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_FT_EDIT_ALREADYASSOC),
                        MAKEINTRESOURCE(IDS_FT_EDIT_ALRASSOCTITLE), MB_YESNO | MB_ICONEXCLAMATION,
                        pszExt, szProgIDDescr, pszExt, szProgIDDescr))
                    {
                        // S_FALSE means user does not want to go on
                        hres = S_FALSE;
                    }
                }
                else
                {
                    // no progIDDescr...  Check if we have a progID
                    TCHAR szProgID[MAX_PROGID];
                    DWORD cchProgID = ARRAYSIZE(szProgID);

                    hres = pAI->GetString(AISTR_PROGID, szProgID, &cchProgID);

                    if (FAILED(hres))
                    {
                        // no progID, set hres to S_OK so that we go on and create one
                        hres = S_OK;
                    }
                }
            }
        }

        // Should we go on and create new progID?
        if (SUCCEEDED(hres) && (S_FALSE != hres) && fNewProgID)
        {
            // Yes, create it
            IAssocInfo* pAIProgID = NULL;

            hres = _pAssocStore->GetAssocInfo(NULL, AIINIT_PROGID, &pAIProgID);

            if (SUCCEEDED(hres))
            {
                hres = pAIProgID->Create();

                if (SUCCEEDED(hres))
                {
                    TCHAR szExt[MAX_EXT];
                    DWORD cchExt = ARRAYSIZE(szExt);
                    TCHAR szProgIDDescr[MAX_PROGIDDESCR];

                    HRESULT hresTmp = pAI->GetString(AISTR_EXT, szExt, &cchExt);

                    if (SUCCEEDED(hresTmp) && *szExt)
                    {
                        MakeDefaultProgIDDescrFromExt(szProgIDDescr, ARRAYSIZE(szProgIDDescr), szExt);

                        hresTmp = pAIProgID->SetString(AISTR_PROGIDDESCR, szProgIDDescr);
                    }

                    // Get the ProgID for later use
                    pAIProgID->GetString(AISTR_PROGID, szProgID, &cchProgID);
                }

                pAIProgID->Release();
            }
        }

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            // Set the new extension progID
            hres = pAI->SetString(AISTR_PROGID, szProgID);

            if (SUCCEEDED(hres))
            {
                // Get the description
                pAI->GetString(AISTR_PROGIDDESCR, _pftEditParam->szProgIDDescr,
                    &(_pftEditParam->dwProgIDDescr));
            }
        }
    }

    return hres;
}

LRESULT CFTEditDlg::OnCancel(WORD wNotif)
{
    EndDialog(_hwnd, IDCANCEL);

    return FALSE;
}

HRESULT CFTEditDlg::_FillProgIDDescrCombo()
{
    HWND hwndCombo = GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO);

    // Data stuff
    IEnumAssocInfo* pEnum = NULL;
    HRESULT hres = _pAssocStore->EnumAssocInfo(
        ASENUM_PROGID | ASENUM_ASSOC_ALL, NULL, AIINIT_NONE, &pEnum);

    if (SUCCEEDED(hres))
    {
        IAssocInfo* pAI = NULL;

        while ((E_OUTOFMEMORY != hres) && (S_OK == pEnum->Next(&pAI)))
        {
            TCHAR szProgIDDescr[MAX_PROGIDDESCR];
            DWORD cchProgIDDescr = ARRAYSIZE(szProgIDDescr);

            hres = pAI->GetString(AISTR_PROGIDDESCR, szProgIDDescr, &cchProgIDDescr);

            if (SUCCEEDED(hres))
            {
                int iIndex = CB_ERR;
                
                if (*szProgIDDescr)
                {
                    if (CB_ERR == ComboBox_FindStringExact(hwndCombo, -1, szProgIDDescr))
                        iIndex = ComboBox_AddString(hwndCombo, szProgIDDescr);
                }

                if ((CB_ERR != iIndex) && (CB_ERRSPACE != iIndex))
                {
                    TCHAR szProgID[MAX_PROGID];
                    DWORD cchProgID = ARRAYSIZE(szProgID);

                    hres = pAI->GetString(AISTR_PROGID, szProgID, &cchProgID);

                    if (SUCCEEDED(hres))
                    {
                        LPTSTR pszProgID = _AddProgID(szProgID);

                        if (pszProgID)
                        {
                            lstrcpy(pszProgID, szProgID);
                            ComboBox_SetItemData(hwndCombo, iIndex, pszProgID);
                        }
                        else
                        {
                            // Out of memory
                            hres = E_OUTOFMEMORY;

                            ShellMessageBox(g_hinst, _hwnd, MAKEINTRESOURCE(IDS_ERROR + 
                                ERROR_NOT_ENOUGH_MEMORY), MAKEINTRESOURCE(IDS_FT), 
                                MB_OK | MB_ICONSTOP);

                            // Already allocated mem will be cleaned-up in OnDestroy
                        }
                    }
                }
            }

            pAI->Release();
        }
        pEnum->Release();

        if (SUCCEEDED(hres))
        {
            TCHAR szNew[20];

            if (LoadString(g_hinst, IDS_FT_NEW, szNew, ARRAYSIZE(szNew)))
            {
                int iIndex = ComboBox_InsertString(hwndCombo, 0, szNew);

                if (CB_ERR != iIndex)
                    ComboBox_SetItemData(hwndCombo, iIndex, NULL);
            }
        }
    }

    return hres;
}

BOOL CFTEditDlg::_SelectProgIDDescr(LPTSTR pszProgIDDescr)
{
    int iIndex = ComboBox_SelectString(GetDlgItem(_hwnd, IDC_FT_EDIT_PID_COMBO),
        -1, pszProgIDDescr);

    return ((CB_ERR != iIndex) ? TRUE : FALSE);
}

LRESULT CFTEditDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    _CleanupProgIDs();

    CFTDlg::OnDestroy(wParam, lParam);

    return FALSE;
}

LPTSTR CFTEditDlg::_AddProgID(LPTSTR pszProgID)
{
    ASSERT(_hHeapProgID);

    return (LPTSTR)HeapAlloc(_hHeapProgID, 0, (lstrlen(pszProgID) + 1) * sizeof(TCHAR));
}

void CFTEditDlg::_CleanupProgIDs()
{
    if (_hHeapProgID)
        HeapDestroy(_hHeapProgID);
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CFTEditDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_FT_EDIT_ADVANCED:
            lRes = OnAdvancedButton(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDC_FT_EDIT_EXT_EDIT:
            lRes = OnEdit(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        default:
            lRes = CFTDlg::OnCommand(wParam, lParam);
            break;
    }

    return lRes;    
}

LRESULT CFTEditDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(uMsg)
    {
        case WM_TIMER:
            if (ID_TIMER == wParam)
                lRes = OnTimer((UINT)wParam);
            else
                lRes = CFTDlg::WndProc(uMsg, wParam, lParam);
            break;

        default:
            lRes = CFTDlg::WndProc(uMsg, wParam, lParam);
            break;
    }

    return lRes;
}
