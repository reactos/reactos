#include "shellprv.h"
#pragma  hdrstop

// from mtptarun.cpp
STDAPI_(void) CMtPt_SetWantUI(int iDrive);

//
// Converts an offset to a string to a string pointer.
//

LPCTSTR _Offset2Ptr(LPTSTR pszBase, UINT_PTR offset, UINT * pcb)
{
    LPTSTR pszRet;
    if (offset == 0) 
    {
        pszRet = NULL;
        *pcb = 0;
    } 
    else 
    {
        pszRet = (LPTSTR)((LPBYTE)pszBase + offset);
        *pcb = (lstrlen(pszRet) + 1) * SIZEOF(TCHAR);
    }
    return pszRet;
}


//
// exported networking APIs from shell32 (from netviewx.c)
//

STDAPI_(UINT) SHGetNetResource(HNRES hnres, UINT iItem, LPNETRESOURCE pnresOut, UINT cbMax)
{
    UINT iRet = 0;        // assume error
    LPNRESARRAY panr = GlobalLock(hnres);
    if (panr)
    {
        if (iItem==(UINT)-1)
        {
            iRet = panr->cItems;
        }
        else if (iItem < panr->cItems)
        {
            UINT cbProvider, cbRemoteName;
            LPCTSTR pszProvider = _Offset2Ptr((LPTSTR)panr, (UINT_PTR)panr->nr[iItem].lpProvider, &cbProvider);
            LPCTSTR pszRemoteName = _Offset2Ptr((LPTSTR)panr, (UINT_PTR)panr->nr[iItem].lpRemoteName, &cbRemoteName);
            iRet = SIZEOF(NETRESOURCE) + cbProvider + cbRemoteName;
            if (iRet <= cbMax)
            {
                LPTSTR psz = (LPTSTR)(pnresOut + 1);
                *pnresOut = panr->nr[iItem];
                if (pnresOut->lpProvider)
                {
                    pnresOut->lpProvider = psz;
                    lstrcpy(psz, pszProvider);
                    psz += cbProvider / SIZEOF(TCHAR);
                }
                if (pnresOut->lpRemoteName)
                {
                    pnresOut->lpRemoteName = psz;
                    lstrcpy(psz, pszRemoteName);
                }
            }
        }
        GlobalUnlock(hnres);
    }
    return iRet;
}


STDAPI_(DWORD) SHNetConnectionDialog(HWND hwnd, LPTSTR pszRemoteName, DWORD dwType)
{
    CONNECTDLGSTRUCT cds;
    NETRESOURCE nr = {0};
    DWORD mnr;

    cds.cbStructure = SIZEOF(cds);  /* size of this structure in bytes */
    cds.hwndOwner = hwnd;           /* owner window for the dialog */
    cds.lpConnRes = &nr;            /* Requested Resource info    */
    cds.dwFlags = CONNDLG_USE_MRU;  /* flags (see below) */
    cds.dwDevNum = 0;               /* number of device connected to */

    nr.dwType = dwType;

    if (pszRemoteName)
    {
        nr.lpRemoteName = pszRemoteName;
        cds.dwFlags = CONNDLG_RO_PATH;
    }
    mnr = WNetConnectionDialog1(&cds);
    if (mnr == WN_SUCCESS && dwType != RESOURCETYPE_PRINT && cds.dwDevNum != 0)
    {
        CMtPt_SetWantUI(cds.dwDevNum - 1 /* 1-based! */);
    }
    return mnr;
}

typedef struct
{
    HWND    hwnd;
    TCHAR   szRemoteName[MAX_PATH];
    DWORD   dwType;
} SHNETCONNECT;

DWORD CALLBACK _NetConnectThreadProc(void *pv)
{
    SHNETCONNECT *pshnc = (SHNETCONNECT *)pv;
    HWND hwndDestroy = NULL;

    if (!pshnc->hwnd)
    {
        RECT rc;
        LPPOINT ppt;
        HWND hwnd;
        DWORD pid;

        // Wild multimon guess - Since we don't have a parent window,
        // we will arbitrarily position ourselves in the same location as
        // the foreground window, if the foreground window belongs to our
        // process.
        hwnd = GetForegroundWindow();

        if (hwnd                                    && 
            GetWindowThreadProcessId(hwnd, &pid)    &&
            (pid == GetCurrentProcessId())          && 
            GetWindowRect(hwnd, &rc))
        {
            // Don't use the upper left corner exactly; slide down by
            // some fudge factor.  We definitely want to get past the
            // caption.
            rc.top += GetSystemMetrics(SM_CYCAPTION) * 4;
            rc.left += GetSystemMetrics(SM_CXVSCROLL) * 4;
            ppt = (LPPOINT)&rc;
        }
        else
        {
            ppt = NULL;
        }

        // Create a stub window so the wizard can establish an Alt+Tab icon
        hwndDestroy = _CreateStubWindow(ppt, NULL);
        pshnc->hwnd = hwndDestroy;
    }

    SHNetConnectionDialog(pshnc->hwnd, pshnc->szRemoteName[0] ? pshnc->szRemoteName : NULL, pshnc->dwType);

    if (hwndDestroy)
        DestroyWindow(hwndDestroy);

    LocalFree(pshnc);

    SHChangeNotifyHandleEvents();
    return 0;
}


STDAPI SHStartNetConnectionDialog(HWND hwnd, LPCTSTR pszRemoteName OPTIONAL, DWORD dwType)
{
    SHNETCONNECT *pshnc = (SHNETCONNECT *)LocalAlloc(LPTR, SIZEOF(SHNETCONNECT));
    if (pshnc)
    {
        pshnc->hwnd = hwnd;
        pshnc->dwType = dwType;
        if (pszRemoteName)
            lstrcpyn(pshnc->szRemoteName, pszRemoteName, ARRAYSIZE(pshnc->szRemoteName));

        if (!SHCreateThread(_NetConnectThreadProc, pshnc, CTF_PROCESS_REF | CTF_COINIT, NULL))
        {
            LocalFree((HLOCAL)pshnc);
        } 
    }
    return S_OK;    // whole thing is async, value here is meaningless
}


#ifdef UNICODE

STDAPI SHStartNetConnectionDialogA(HWND hwnd, LPCSTR pszRemoteName, DWORD dwType)
{
    WCHAR wsz[MAX_PATH];

    if (pszRemoteName)
    {
        SHAnsiToUnicode(pszRemoteName, wsz, SIZECHARS(wsz));
        pszRemoteName = (LPCSTR)wsz;
    }
    return SHStartNetConnectionDialog(hwnd, (LPCTSTR)pszRemoteName, dwType);
}

#else

STDAPI SHStartNetConnectionDialogW(HWND hwnd, LPCWSTR pszRemoteName, DWORD dwType)
{
    char sz[MAX_PATH];

    if (pszRemoteName)
    {
        SHUnicodeToAnsi(pszRemoteName, sz, SIZECHARS(sz));
        pszRemoteName = (LPCWSTR)sz;
    }

    return SHStartNetConnectionDialog(hwnd, (LPCTSTR)pszRemoteName, dwType);
}

#endif


// BUGBUG: daviddv - we used to just override the implementation of these two functions
// BUGBUG: by giving them the same name, now we use dllload.c we cannot do that so 
// BUGBUG: instead we just have macros that redirect here, in time we should nuke these
// BUGBUG: stubs and s/r the sources.

DWORD APIENTRY SHWNetDisconnectDialog1 (LPDISCDLGSTRUCT lpConnDlgStruct)
{
    TCHAR szLocalName[3];

    if (lpConnDlgStruct && lpConnDlgStruct->lpLocalName && lstrlen(lpConnDlgStruct->lpLocalName) > 2)
    {
        // Kludge allert, don't pass c:\ to API, instead only pass C:
        szLocalName[0] = lpConnDlgStruct->lpLocalName[0];
        szLocalName[1] = TEXT(':');
        szLocalName[2] = 0;
        lpConnDlgStruct->lpLocalName = szLocalName;
    }

    return WNetDisconnectDialog1 (lpConnDlgStruct);
}


DWORD APIENTRY SHWNetGetConnection (LPCTSTR lpLocalName, LPTSTR lpRemoteName, LPDWORD lpnLength)
{
    TCHAR szLocalName[3];

    if (lpLocalName && lstrlen(lpLocalName) > 2)
    {
        // Kludge allert, don't pass c:\ to API, instead only pass C:
        szLocalName[0] = lpLocalName[0];
        szLocalName[1] = TEXT(':');
        szLocalName[2] = 0;
        lpLocalName = szLocalName;
    }

    return WNetGetConnection (lpLocalName, lpRemoteName, lpnLength);
}
