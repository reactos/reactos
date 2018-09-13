#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Local functions / data
/----------------------------------------------------------------------------*/

static WCHAR c_szQueryPrefix[] = L"(uncName=*)(objectCategory=volume)";

static LPWSTR c_szClassList[] =
{
    L"volume",
};

static PAGECTRL ctrls[] =
{
    IDC_VOLNAME,     c_szName,     FILTER_CONTAINS,
    IDC_VOLKEYWORDS, c_szKeywords, FILTER_CONTAINS,
};

static COLUMNINFO columns[] = 
{
    0, 0, IDS_CN,       0, c_szName,          
    0, 0, IDS_UNCNAME,  0, c_szUNCName,
    0, 0, IDS_KEYWORDS, 0, c_szKeywords,
};

//
// Control help meppings
// 

static DWORD const aFormHelpIDs[] =
{
    IDC_VOLNAME, IDH_SHARED_FOLDER_NAMED,
    IDC_VOLKEYWORDS, IDH_KEYWORDS,
    0, 0
};


/*-----------------------------------------------------------------------------
/ PageProc_Volume
/ ---------------
/   PageProc for handling the messages for this object.
/
/ In:
/   pForm -> instance data for this form
/   hwnd = window handle for the form dialog
/   uMsg, wParam, lParam = message parameters
/
/ Out:
/   HRESULT (E_NOTIMPL) if not handled
/----------------------------------------------------------------------------*/
HRESULT CALLBACK PageProc_Volume(LPCQPAGE pForm, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPWSTR pQuery = NULL;

    TraceEnter(TRACE_FORMS, "PageProc_Volume");

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

            hr = PersistQuery(pPersistQuery, fRead, c_szMsVolume, hwnd, ctrls, ARRAYSIZE(ctrls));
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
/ DlgProc_Volume
/ --------------
/   Handle dialog specific message for the volume page.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_Volume(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = 0;
    LPCQPAGE pQueryPage;

    if ( uMsg == WM_INITDIALOG )
    {
        pQueryPage = (LPCQPAGE)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pQueryPage);

        Edit_LimitText(GetDlgItem(hwnd, IDC_VOLNAME), MAX_PATH);
        Edit_LimitText(GetDlgItem(hwnd, IDC_VOLKEYWORDS), MAX_PATH);
    }
    else if ( uMsg == WM_CONTEXTMENU )
    {
        WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aFormHelpIDs);
        fResult = TRUE;
    }

    return fResult;    
}
