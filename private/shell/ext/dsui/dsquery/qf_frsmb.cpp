#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Local functions / data
/----------------------------------------------------------------------------*/

static WCHAR c_szQueryPrefix[] = L"(objectClass=nTFRSMember)";

static LPWSTR c_szClassList[] = 
{
    L"nTFRSMember",
};

static PAGECTRL ctrls[] =
{
    IDC_DOMNAME, c_szName, FILTER_CONTAINS,
    IDC_DOMDESC, c_szDescription, FILTER_CONTAINS,
};

static COLUMNINFO columns[] = 
{
    0, 0, IDS_CN, 0, c_szDistinguishedName,          
    0, DEFAULT_WIDTH_DESCRIPTION, IDS_DESCRIPTION, 0, c_szDescription,
};

//
// Help ID mappings
//

static DWORD const aFormHelpIDs[] =
{
    0, 0
};


/*-----------------------------------------------------------------------------
/ PageProc_FrsMember
/ ------------------
/   PageProc for handling the messages for this object.
/
/ In:
/   pPage -> instance data for this form
/   hwnd = window handle for the form dialog
/   uMsg, wParam, lParam = message parameters
/
/ Out:
/   HRESULT (E_NOTIMPL) if not handled
/----------------------------------------------------------------------------*/
HRESULT CALLBACK PageProc_FrsMember(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPWSTR pQuery = NULL;

    TraceEnter(TRACE_FORMS, "PageProc_FrsMember");

    switch ( uMsg )
    {
        case CQPM_INITIALIZE:
        case CQPM_RELEASE:
            break;
            
        case CQPM_ENABLE:
            EnablePageControls(hwnd, ctrls, ARRAYSIZE(ctrls), (BOOL)wParam);
            break;

        case CQPM_GETPARAMETERS:
        {
            hr = GetQueryString(&pQuery, c_szQueryPrefix, hwnd, ctrls, ARRAYSIZE(ctrls));

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

            hr = PersistQuery(pPersistQuery, fRead, c_szMsFrsMembers, hwnd, ctrls, ARRAYSIZE(ctrls));
            FailGracefully(hr, "Failed to persist page");

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
/ DlgProc_FrsMember
/ ------------
/   Handle dialog specific message for the FRS Members page.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_FrsMember(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = 0;
    LPCQPAGE pQueryPage;

    if ( uMsg == WM_INITDIALOG )
    {
        pQueryPage = (LPCQPAGE)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pQueryPage);

        Edit_LimitText(GetDlgItem(hwnd, IDC_DOMNAME), MAX_PATH);
        Edit_LimitText(GetDlgItem(hwnd, IDC_DOMDESC), MAX_PATH);
    }

    return fResult;    
}
