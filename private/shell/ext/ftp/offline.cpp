/****************************************************\
    FILE: offline.cpp

    DESCRIPTION:
        Handle 'offline' status and Dial-up UI
\****************************************************/

#include <wininet.h>
#include "priv.h"
#include "util.h"


#ifdef FEATURE_OFFLINE
/****************************************************\
    FUNCTION: IsGlobalOffline

    DESCRIPTION:
        Determines whether wininet is in global offline mode

    PARAMETERS:
        None

    RETURN VALUE:
        BOOL
            TRUE    - offline
            FALSE   - online
\****************************************************/
BOOL IsGlobalOffline(VOID)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if(InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}


/****************************************************\
    FUNCTION: SetOffline

    DESCRIPTION:
        Sets wininet's offline mode

    PARAMETERS:
        fOffline - online or offline

    RETURN VALUE:
        None.
\****************************************************/
VOID SetOffline(IN BOOL fOffline)
{
    INTERNET_CONNECTED_INFO ci = {0};

    if(fOffline)
    {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    }
    else
    {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
}
#endif // FEATURE_OFFLINE


/****************************************************\
    FUNCTION: AssureNetConnection

    DESCRIPTION:
\****************************************************/
HRESULT AssureNetConnection(HINTERNET hint, HWND hwndParent, LPCWSTR pwzServerName, LPCITEMIDLIST pidl, BOOL fShowUI)
{
    HRESULT hr = S_OK;

#ifdef FEATURE_OFFLINE
    if (IsGlobalOffline())
    {
        // Assume we need to cancel the FTP operation because we are offline.
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

        // Can we prompt to go online?
        if (fShowUI)
        {
            TCHAR szTitle[MAX_PATH];
            TCHAR szPromptMsg[MAX_PATH];

            EVAL(LoadString(HINST_THISDLL, IDS_FTPERR_TITLE, szTitle, ARRAYSIZE(szTitle)));
            EVAL(LoadString(HINST_THISDLL, IDS_OFFLINE_PROMPTTOGOONLINE, szPromptMsg, ARRAYSIZE(szPromptMsg)));

            if (IDYES == MessageBox(hwndParent, szPromptMsg, szTitle, (MB_ICONQUESTION | MB_YESNO)))
            {
                SetOffline(FALSE);
                hr = S_OK;
            }
        }
    }
#endif // FEATURE_OFFLINE

#ifdef FEATURE_DIALER
    if (S_OK == hr)
    {
        TCHAR szUrl[MAX_URL_STRING];

        StrCpyN(szUrl, TEXT("ftp://"), ARRAYSIZE(szUrl));
        StrCatBuff(szUrl, pwzServerName, ARRAYSIZE(szUrl));

        // PERF: Does this value get cached?
        if (FALSE == InternetCheckConnection(szUrl, FLAG_ICC_FORCE_CONNECTION, 0)
            ||
#ifdef FEATURE_TEST_DIALER
        (IDNO == MessageBox(hwndParent, TEXT("TEST: Do you want to dial?"), TEXT("Test Dialer"), MB_YESNO))
#endif // FEATURE_TEST_DIALER
            )
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }
#endif // FEATURE_DIALER

    return hr;
}
