#include "shellprv.h"
#include "ids.h"

#include "ftcmmn.h"
#include "ftdlg.h"
#include "ftascstr.h" //there only for the new CFTAssocStore


CFTDlg::CFTDlg(ULONG_PTR ulpAHelpIDsArray, BOOL fAutoDelete) :
    _fAutoDelete(fAutoDelete), _rgdwHelpIDsArray(ulpAHelpIDsArray)
{
}

CFTDlg::~CFTDlg()
{
    if (_pAssocStore)
        delete _pAssocStore;
}

HRESULT CFTDlg::_InitAssocStore()
{
    ASSERT(!_pAssocStore);

    _pAssocStore = new CFTAssocStore();

    return _pAssocStore ? S_OK : E_OUTOFMEMORY;
}

INT_PTR CFTDlg::DoModal(HINSTANCE hinst, LPTSTR pszResource, HWND hwndParent)
{
    PROPSHEETPAGE psp;
    psp.lParam = (LPARAM)this;
    return DialogBoxParam(hinst, pszResource, hwndParent, CFTDlg::FTDlgWndProc, (LPARAM)&psp);
}

ULONG_PTR CFTDlg::GetHelpIDsArray()
{
    return _rgdwHelpIDsArray;
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CFTDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            lRes = OnInitDialog(wParam, lParam);
            break;

        case WM_COMMAND:
            lRes = OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
            lRes = OnNotify(wParam, lParam);
            break;

        case WM_DESTROY:
            lRes = OnDestroy(wParam, lParam);
            break;

        case WM_CTRL_SETFOCUS:
            lRes = OnCtrlSetFocus(wParam, lParam);
            break;

        case WM_HELP:
        {
            HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;
            int iCtrlID = GetDlgCtrlID(hwndItem);

            WinHelp(hwndItem, NULL, HELP_WM_HELP, GetHelpIDsArray());

            lRes = TRUE;
            break;
        }
        case WM_CONTEXTMENU:
        {
            if (HTCLIENT == (int)SendMessage(_hwnd, WM_NCHITTEST, 0, lParam))
            {
                POINT pt;
                HWND hwndItem = NULL;
                int iCtrlID = 0;
        
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);
                ScreenToClient(_hwnd, &pt);
        
                hwndItem = ChildWindowFromPoint(_hwnd, pt);
                iCtrlID = GetDlgCtrlID(hwndItem);

                WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU,
                    GetHelpIDsArray());
        
                lRes = TRUE;
            }
            else
                lRes = FALSE;

            break;
        }
        default:
            lRes = DefWndProc(uMsg, wParam, lParam);
            break;
    }

    return lRes;
}

LRESULT CFTDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDOK:
            lRes = OnOK(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case IDCANCEL:
            lRes = OnCancel(GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        default:
            lRes = DefWndProc(WM_COMMAND, wParam, lParam);
            break;
    }

    return lRes;    
}

LRESULT CFTDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    return DefWndProc(WM_COMMAND, wParam, lParam);
}

LRESULT CFTDlg::OnDestroy(WPARAM wParam, LPARAM lParam)
{
    if (_pAssocStore)
    {
        delete _pAssocStore;
        _pAssocStore = NULL;
    }

    if (_fAutoDelete)
    {
        SetWindowLongPtr(_hwnd, GWLP_USERDATA, NULL);
        delete this;
    }
    else
    {
        ResetHWND();
    }

    return FALSE;
}

LRESULT CFTDlg::OnCtrlSetFocus(WPARAM wParam, LPARAM lParam)
{
    SetFocus((HWND)lParam);

    return TRUE;
}

//static
LRESULT CFTDlg::DefWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                           LPARAM lParam)
{
    return FALSE;
}

LRESULT CFTDlg::DefWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWndProc(_hwnd, uMsg, wParam, lParam);
}

//static
BOOL_PTR CALLBACK CFTDlg::FTDlgWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFTDlg* pThis = (CFTDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (WM_INITDIALOG == uMsg)
    {
        pThis = (CFTDlg*)(((PROPSHEETPAGE*)lParam)->lParam);

        if (pThis)
        {
            pThis->SetHWND(hwnd);

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
    }

    if ( pThis )
    {
        return pThis->WndProc(uMsg, wParam, lParam);
    }

    return CFTDlg::DefWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CFTDlg::OnOK(WORD wNotif)
{
    return FALSE;
}

LRESULT CFTDlg::OnCancel(WORD wNotif)
{
    return FALSE;
}

//static
void CFTDlg::MakeDefaultProgIDDescrFromExt(LPTSTR pszProgIDDescr, DWORD cchProgIDDescr,
        LPTSTR pszExt)
{
    TCHAR szTemplate[25];
    TCHAR szExt[MAX_EXT];

    lstrcpyn(szExt, pszExt, ARRAYSIZE(szExt));

    LoadString(g_hinst, IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));

    CharUpper(szExt);

    wnsprintf(pszProgIDDescr, cchProgIDDescr, szTemplate, szExt);
}
