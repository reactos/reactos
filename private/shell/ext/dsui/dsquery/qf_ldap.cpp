#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Local functions / data
/----------------------------------------------------------------------------*/

static TCHAR szQueryString[] = TEXT("QueryString");

static COLUMNINFO columnsRawLDAP[] = 
{
    0, 20, IDS_CN,          0, c_szName,          
    0, 20, IDS_OBJECTCLASS, DSCOLUMNPROP_OBJECTCLASS, NULL,
    0, 60, IDS_DESCRIPTION, 0, c_szDescription,
};

//
// Help ID mappings
//

static DWORD const aFormHelpIDs[] =
{
    IDC_LDAP, IDH_LDAP_QUERY,
    0, 0
};


/*-----------------------------------------------------------------------------
/ PageProc_RawLDAP
/ ----------------
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
HRESULT CALLBACK PageProc_RawLDAP(LPCQPAGE pPage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    TCHAR szBuffer[MAX_PATH];
    USES_CONVERSION;

    TraceEnter(TRACE_FORMS, "PageProc_RawLDAP");

    switch ( uMsg )
    {
        case CQPM_INITIALIZE:
        case CQPM_RELEASE:
            break;

        case CQPM_ENABLE:
            EnableWindow(GetDlgItem(hwnd, IDC_LDAP), (BOOL)wParam);
            break;

        case CQPM_GETPARAMETERS:
        {
            LPDSQUERYPARAMS* ppDsQueryParams = (LPDSQUERYPARAMS*)lParam;

            // If we already have some query params then lets add to the query string,
            // if no then we must construct a new query.

            if ( *ppDsQueryParams )
            {
                if ( GetDlgItemText(hwnd, IDC_LDAP, szBuffer, ARRAYSIZE(szBuffer)) )
                {
                        hr = QueryParamsAddQueryString(ppDsQueryParams, T2W(szBuffer));
                        FailGracefully(hr, "Failed to append query to existing query string");
                }
            }
            else
            {
                if ( GetDlgItemText(hwnd, IDC_LDAP, szBuffer, ARRAYSIZE(szBuffer)) )
                {
                    hr = QueryParamsAlloc(ppDsQueryParams, T2W(szBuffer), GLOBAL_HINSTANCE, ARRAYSIZE(columnsRawLDAP), columnsRawLDAP);
                    FailGracefully(hr, "Failed to build DS argument block");
                }
            }

            break;
        }

        case CQPM_CLEARFORM:
            SetDlgItemText(hwnd, IDC_LDAP, TEXT(""));
            break;
    
        case CQPM_PERSIST:
        {
            BOOL fRead = (BOOL)wParam;
            IPersistQuery* pPersistQuery = (IPersistQuery*)lParam;

            // Read/Write the current query string from the file, if reading and we cannot
            // get the string then no real problem, just ignore it.

            if ( fRead )
            {
                if ( SUCCEEDED(pPersistQuery->ReadString(c_szMsPropertyWell, szQueryString, szBuffer, ARRAYSIZE(szBuffer))) )
                {
                    Trace(TEXT("Query string from file is: %s"), szBuffer);
                    SetDlgItemText(hwnd, IDC_LDAP, szBuffer);
                }
            }
            else
            {
                if ( GetDlgItemText(hwnd, IDC_LDAP, szBuffer, ARRAYSIZE(szBuffer)) )
                {
                    Trace(TEXT("Writing query string to file: %s"), szBuffer);
                    hr = pPersistQuery->WriteString(c_szMsPropertyWell, szQueryString, szBuffer);
                    FailGracefully(hr, "Failed when writing out raw query string");
                }
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
            // we don't generate any class list
            break;

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
/ DlgProc_RawLDAP
/ ---------------
/   Handle operations specific to the RAW LDAP query form.
/
/ In:
/   hwnd, uMsg, wParam, lParam = standard parameters
/
/ Out:
/   INT_PTR
/----------------------------------------------------------------------------*/
INT_PTR CALLBACK DlgProc_RawLDAP(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR fResult = 0;
    LPCQPAGE pQueryPage;

    if ( uMsg == WM_INITDIALOG )
    {
        pQueryPage = (LPCQPAGE)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pQueryPage);

        Edit_LimitText(GetDlgItem(hwnd, IDC_LDAP), MAX_PATH);
    }
    else
    {
        pQueryPage = (LPCQPAGE)GetWindowLongPtr(hwnd, DWLP_USER);

        switch ( uMsg )
        {
            case WM_SIZE:
            {
                HWND hwndLDAP = GetDlgItem(hwnd, IDC_LDAP);
                RECT rect;

                // size the edit control to cover the entire form, retain the original
                // height, but apply the left border to the edit control

                GetRealWindowInfo(hwndLDAP, &rect, NULL);
                SetWindowPos(hwndLDAP, NULL, 
                             0, 0, 
                             LOWORD(lParam)-(rect.left*2), 
                             HIWORD(lParam)-rect.top-rect.left,
                             SWP_NOMOVE|SWP_NOZORDER);
                break;
            }

            case WM_CONTEXTMENU:
            {
                WinHelp((HWND)wParam, DSQUERY_HELPFILE, HELP_CONTEXTMENU, (DWORD_PTR)aFormHelpIDs);
                fResult = TRUE;
                break;
            }
        }
    }

    return fResult;    
}
