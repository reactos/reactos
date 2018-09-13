#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Local functions / data
/----------------------------------------------------------------------------*/

static TCHAR szMachineRole[] = TEXT("MachineRole");

static LPWSTR c_szClassList[] =
{
    L"computer",
};

static struct
{
    INT idString;
    LPWSTR c_szPropertyValue;
}
machineRoleValues[] =
{
    IDS_ANY,         L"(sAMAccountType=805306369)",
    IDS_WKSSERVER,   L"(&(samAccountType=805306369)(!(primaryGroupId=516)))",
    IDS_DC,          L"(primaryGroupID=516)",
};

static PAGECTRL ctrls[] =
{
    IDC_COMPNAME,  c_szName,     FILTER_CONTAINS,
    IDC_COMPOWNER, L"managedBy", FILTER_CONTAINS,
};

static COLUMNINFO columns[] = 
{
    0, 0, IDS_CN,	        0, c_szName,
    0, 0, IDS_MACHINEROLE,  0, L"userAccountControl,{C40FBD00-88B9-11d2-84AD-00C04FA31A86}",
    0, 0, IDS_OWNER,	    0, L"managedBy,{DDE5783A-88B9-11d2-84AD-00C04FA31A86}",
    0, DEFAULT_WIDTH_DESCRIPTION, IDS_DESCRIPTION, 0, c_szDescription,
};

//
// Control help meppings
// 

static DWORD const aFormHelpIDs[] =
{
    IDC_COMPNAME, IDH_COMPUTER_NAME,
    IDC_COMPOWNER, IDH_OWNER,
    IDC_COMPROLE, IDH_ROLE, 
    0, 0,
};


/*-----------------------------------------------------------------------------
/ PageProc_Computer
/ -----------------
/   PageProc for finding computers.
/
/ In:
/   pForm -> instance data for this form
/   hwnd = window handle for the form dialog
/   uMsg, wParam, lParam = message parameters
/
/ Out:
/   HRESULT (E_NOTIMPL) if not handled
/----------------------------------------------------------------------------*/
HRESULT CALLBACK PageProc_Computer(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPWSTR pQuery = NULL;

    TraceEnter(TRACE_FORMS, "PageProc_Computer");

    switch ( uMsg )
    {
        case CQPM_INITIALIZE:
        case CQPM_RELEASE:
            break;
            
        case CQPM_ENABLE:
            EnablePageControls(hwnd, ctrls, ARRAYSIZE(ctrls), (BOOL)wParam);
            EnableWindow(GetDlgItem(hwnd, IDC_COMPROLE), (BOOL)wParam);
            break;

        case CQPM_GETPARAMETERS:
        {
            INT iCurSel;
           
            iCurSel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMPROLE));

            if ( (iCurSel < 0) || (iCurSel >= ARRAYSIZE(machineRoleValues)) )
                ExitGracefully(hr, E_FAIL, "Bad selection of computer role");

            hr = GetQueryString(&pQuery, machineRoleValues[iCurSel].c_szPropertyValue, hwnd, ctrls, ARRAYSIZE(ctrls));

            if ( SUCCEEDED(hr) )
            {
                hr = QueryParamsAlloc((LPDSQUERYPARAMS*)lParam, pQuery, GLOBAL_HINSTANCE, ARRAYSIZE(columns), columns);
                LocalFreeStringW(&pQuery);
            }

            FailGracefully(hr, "Failed to build DS argument block");            

            break;
        }
    
        case CQPM_CLEARFORM:
            ResetPageControls(hwnd, ctrls, ARRAYSIZE(ctrls));
            break;

        case CQPM_PERSIST:
        {
            BOOL fRead = (BOOL)wParam;
            IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;
            INT i;

            hr = PersistQuery(pPersistQuery, fRead, c_szMsComputer, hwnd, ctrls, ARRAYSIZE(ctrls));
            FailGracefully(hr, "Failed to persist page");

            if ( fRead )
            {
                if ( SUCCEEDED(pPersistQuery->ReadInt(c_szMsComputer, szMachineRole, &i)) )
                    ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMPROLE), i);
            }
            else
            {
                hr = pPersistQuery->WriteInt(c_szMsComputer, szMachineRole, 
                                                ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMPROLE)));
                FailGracefully(hr, "Failed when writing out computer type");
            }

            break;
        }

        case CQPM_HELP:
        {
            LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
            WinHelp((HWND)pHelpInfo->hItemHandle,
                    DSQUERY_HELPFILE,
                    HELP_WM_HELP,
                    (DWORD_PTR)aFormHelpIDs);
            break;
        }

        case DSQPM_GETCLASSLIST:
        {
            hr = ClassListAlloc((LPDSQUERYCLASSLIST*)lParam, c_szClassList, ARRAYSIZE(c_szClassList));
            FailGracefully(hr, "Failed to allocate class list");
            break;
        }

        case DSQPM_HELPTOPICS:
        {
            HWND hwndFrame = (HWND)lParam;
            HtmlHelp(hwndFrame, TEXT("omc.chm"), HH_HELP_FINDER, 0);
            break;
        }

        default:
            hr = E_NOTIMPL;
            break;
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ DlgProc_Computer
/ ----------------
/   Standard dialog proc for the page, handle any special buttons and other
/   such nastyness we must here.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_Computer(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = 0;
    LPCQPAGE pQueryPage;
    HWND hwndCtrl;

    if ( uMsg == WM_INITDIALOG )
    {
        TCHAR szBuffer[MAX_PATH];
        INT i;

        pQueryPage = (LPCQPAGE)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pQueryPage);

        Edit_LimitText(GetDlgItem(hwnd, IDC_COMPNAME), MAX_PATH);
        Edit_LimitText(GetDlgItem(hwnd, IDC_COMPOWNER), MAX_PATH);

        for ( i = 0 ; i < ARRAYSIZE(machineRoleValues) ; i++ )
        {
            LoadString(GLOBAL_HINSTANCE, machineRoleValues[i].idString, szBuffer, ARRAYSIZE(szBuffer));
            ComboBox_AddString(GetDlgItem(hwnd, IDC_COMPROLE), szBuffer);
        }

        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMPROLE), 0);
    }
    else if ( uMsg == WM_CONTEXTMENU )
    {
        WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aFormHelpIDs);
        fResult = TRUE;
    }

    return fResult;    
}
