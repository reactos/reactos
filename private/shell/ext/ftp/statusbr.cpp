/*****************************************************************************
 *
 *      statusbr.cpp - Take care of the status bar.
 *
 *****************************************************************************/

#include "priv.h"
#include "statusbr.h"

// HACKHACK: 
//      \nt\public\sdk\inc\multimon.h overrides the normal GetSystemMetrics() with
//      xGetSystemMetrics().  The problem is that you can't link because you need to
//      have at least one file in your project that #defines COMPILE_MULTIMON_STUBS
//      so that these stub override functions will get implemented.
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"


#define PANE_WIDTH_USERNAME     125
#define STATUS_PANES            3


//////////////////////////////////////////////////////////////////
//  General Text Pane
//////////////////////////////////////////////////////////////////

void CStatusBar::SetStatusMessage(UINT nMessageID, LPCTSTR pszExtra)
{
    if (NULL == this)
        return;

    _InitStatusBar();       // This is a NO-OP if it's already inited.

    ASSERTNONCRITICAL;
    if (m_hwndStatus)
    {
        TCHAR szMsg[256] = TEXT("%s");
        TCHAR szBuf[1024];

        if (nMessageID)
            LoadString(g_hinst, nMessageID, szMsg, ARRAYSIZE(szMsg));
        wnsprintf(szBuf, ARRAYSIZE(szBuf), szMsg, pszExtra);

//        TraceMsg(TF_FTPSTATUSBAR, "CStatusBar::SetStatusMessage() Message=%s", szBuf);
        SendMessage(m_hwndStatus, SB_SETTEXT, STATUS_PANE_STATUS, (LPARAM)szBuf);
        SendMessage(m_hwndStatus, SB_SETTIPTEXT, STATUS_PANE_STATUS, (LPARAM)szBuf);
        UpdateWindow(m_hwndStatus);
    }
}


//////////////////////////////////////////////////////////////////
//  User Name Pane
//////////////////////////////////////////////////////////////////

void CStatusBar::SetUserName(LPCTSTR pszUserName, BOOL fAnnonymous)
{
    TCHAR szTipText[MAX_PATH];
    TCHAR szStrTemplate[MAX_PATH];

    _InitStatusBar();       // This is a NO-OP if it's already inited.
    //TraceMsg(TF_FTPSTATUSBAR, "CStatusBar::SetUserName(pszUserName=%s, fAnnonymous=%d)", pszUserName, fAnnonymous);

    ASSERT(pszUserName);
    LoadString(HINST_THISDLL, IDS_USER_TEMPLATE, szStrTemplate, ARRAYSIZE(szStrTemplate));

    if (fAnnonymous)
    {
        TCHAR szAnnonymousName[MAX_PATH];

        LoadString(HINST_THISDLL, IDS_USER_ANNONYMOUS, szAnnonymousName, ARRAYSIZE(szAnnonymousName));
        wnsprintf(szTipText, ARRAYSIZE(szTipText), szStrTemplate, szAnnonymousName);
    }
    else
        wnsprintf(szTipText, ARRAYSIZE(szTipText), szStrTemplate, pszUserName);
        

    Str_SetPtr(&m_pszUserName, szTipText);

    LoadString(HINST_THISDLL, (fAnnonymous ? IDS_USER_ANNONTOOLTIP : IDS_USER_USERTOOLTIP), szStrTemplate, ARRAYSIZE(szStrTemplate));
    wnsprintf(szTipText, ARRAYSIZE(szTipText), szStrTemplate, pszUserName);
    Str_SetPtr(&m_pszUserNameTT, szTipText);
    _SetUserParts();
}


void CStatusBar::_SetUserParts(void)
{
    SendMessage(m_hwndStatus, SB_SETTEXT, STATUS_PANE_USERNAME, (LPARAM)(m_pszUserName ? m_pszUserName : TEXT("")));
    SendMessage(m_hwndStatus, SB_SETTIPTEXT, STATUS_PANE_USERNAME, (LPARAM)(m_pszUserNameTT ? m_pszUserNameTT : TEXT("")));
}


//////////////////////////////////////////////////////////////////
//  Icons Panes (Read, Write, ...)
//////////////////////////////////////////////////////////////////

#define GET_RESID_FROM_PERMISSION(nType, nAllowed)  (IDS_BEGIN_SB_TOOLTIPS + nType + (nAllowed ? 0 : 1))

void CStatusBar::SetFolderAttribute(BOOL fWriteAllowed)
{
    TCHAR szToolTip[MAX_PATH];

    _InitStatusBar();       // This is a NO-OP if it's already inited.

    m_fWriteAllowed = fWriteAllowed;
    LoadString(HINST_THISDLL, GET_RESID_FROM_PERMISSION(ITD_WriteAllowed, fWriteAllowed), szToolTip, ARRAYSIZE(szToolTip));
    _SetIconAndTip(ISLOT_WritePermission, fWriteAllowed ? ITD_WriteAllowed : ITD_WriteNotAllowed, szToolTip);
}

HRESULT CStatusBar::_SetIconAndTip(ICON_SLOT nIconSlot, ICON_TODISPLAY nIconToDisplay, LPCTSTR pszTip)
{
    /*
    if (EVAL(m_hwndStatus))
    {
        if (!m_arhiconGeneral[nIconToDisplay])
        {
            m_arhiconGeneral[nIconToDisplay] = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(nIconToDisplay + IDI_WRITE_ALLOWED), 
                IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
        }

        SendMessage(m_hwndStatus, SB_SETICON, STATUS_PANE_WRITEICON, (LPARAM)m_arhiconGeneral[nIconToDisplay]);
        SendMessage(m_hwndStatus, SB_SETTIPTEXT, STATUS_PANE_WRITEICON, (LPARAM)pszTip);
    }
*/
    return S_OK;
}


//////////////////////////////////////////////////////////////////
//  Zones Pane
//////////////////////////////////////////////////////////////////

void CStatusBar::UpdateZonesPane(LPCTSTR pszUrl)
{
    _InitStatusBar();       // This is a NO-OP if it's already inited.
    if (EVAL(SUCCEEDED(_LoadZoneInfo(pszUrl))))
        EVAL(SUCCEEDED(_SetZone()));
}

HRESULT CStatusBar::_LoadZoneInfo(LPCTSTR pszUrl)
{
    m_lCurrentZone = ZONE_UNKNOWN;
    if (!m_pism)
        CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, IID_IInternetSecurityManager, (void **)&m_pism);

    if (m_pism)
    {
        WCHAR wzUrl[MAX_URL_STRING];

        SHTCharToUnicode(pszUrl, wzUrl, ARRAYSIZE(wzUrl));
        m_pism->MapUrlToZone(wzUrl, (DWORD*)&m_lCurrentZone, 0);
    }        

    return S_OK;
}

HRESULT CStatusBar::_SetZone(void)
{
    ZONEATTRIBUTES za = {SIZEOF(za)};
    HICON hIcon = NULL;
            
    if (!m_arhiconZones[0])
        _CacheZonesIcons();
            
    if (m_pizm && (m_lCurrentZone != ZONE_UNKNOWN))
    {
        m_pizm->GetZoneAttributes(m_lCurrentZone, &za);
        hIcon = m_arhiconZones[m_lCurrentZone];
    }
    else
        _LoadStringW(HINST_THISDLL, IDS_ZONES_UNKNOWN, za.szDisplayName, ARRAYSIZE(za.szDisplayName));
    
    SendMessage(m_hwndStatus, SB_SETTEXTW, STATUS_PANE_ZONE, (LPARAM)za.szDisplayName);
    SendMessage(m_hwndStatus, SB_SETTIPTEXTW, STATUS_PANE_ZONE, (LPARAM)za.szDisplayName);
    SendMessage(m_hwndStatus, SB_SETICON, STATUS_PANE_ZONE, (LPARAM)hIcon);
    return S_OK;
}

void CStatusBar::_CacheZonesIcons(void)
{
    DWORD dwZoneCount = 0;

    if (!m_pizm)
        CoCreateInstance(CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER, IID_IInternetZoneManager, (void **)&m_pizm);
    
    if (EVAL(m_pizm))
    {
        DWORD dwZoneEnum;

        if (EVAL(SUCCEEDED(m_pizm->CreateZoneEnumerator(&dwZoneEnum, &dwZoneCount, 0))))
        {
            for (int nIndex=0; (DWORD)nIndex < dwZoneCount; nIndex++)
            {
                DWORD           dwZone;
                ZONEATTRIBUTES  za = {sizeof(ZONEATTRIBUTES)};
                WORD            iIcon=0;
                HICON           hIcon = NULL;

                m_pizm->GetZoneAt(dwZoneEnum, nIndex, &dwZone);

                // get the zone attributes for this zone
                m_pizm->GetZoneAttributes(dwZone, &za);

                // Zone icons are in two formats.
                // wininet.dll#1200 where 1200 is the res id.
                // or foo.ico directly pointing to an icon file.
                // search for the '#'
                LPWSTR pwsz = StrChrW(za.szIconPath, TEXTW('#'));

                if (pwsz)
                {
                    TCHAR           szIconPath[MAX_PATH];        
                    // if we found it, then we have the foo.dll#00001200 format
                    pwsz[0] = TEXTW('\0');
                    SHUnicodeToTChar(za.szIconPath, szIconPath, ARRAYSIZE(szIconPath));
                    iIcon = (WORD)StrToIntW(pwsz+1);
                    ExtractIconEx(szIconPath,(INT)(-1*iIcon), NULL, &hIcon, 1 );
                }
                else
                    hIcon = (HICON)ExtractAssociatedIconExW(HINST_THISDLL, za.szIconPath, (LPWORD)&iIcon, &iIcon);
                    
                if (nIndex < MAX_NUM_ZONES_ICONS)
                     m_arhiconZones[nIndex] = hIcon;
            }
            m_pizm->DestroyZoneEnumerator(dwZoneEnum);
        }
    }    
}


HRESULT CStatusBar::Resize(LONG x, LONG y)
{
    return _SetParts();
}


//////////////////////////////////////////////////////////////////
//  General Functions
//////////////////////////////////////////////////////////////////

HRESULT CStatusBar::_InitStatusBar(void)
{
    HRESULT hr = S_OK;

    // Only reformat the StatusBar if we haven't yet, or
    // if someone formatted it away from us. (@!%*#)
    if ((!m_fInited) ||
        (STATUS_PANES != SendMessage(m_hwndStatus, SB_GETPARTS, 0, 0L)))
    {
        m_fInited = TRUE;
        hr = _SetParts();
    }

    return hr;
}


HRESULT CStatusBar::_SetParts(void)
{
    HRESULT hr = S_OK;
    RECT rc;

    ASSERTNONCRITICAL;

#ifdef OLD_STYLE_STATUSBAR
    SendMessage(hwnd, SB_SETTEXT, 1 | SBT_NOBORDERS, 0);
#else // OLD_STYLE_STATUSBAR

    GetClientRect(m_hwndStatus, &rc);
    const UINT cxZone = ZoneComputePaneSize(m_hwndStatus);
    const UINT cxUserName = PANE_WIDTH_USERNAME;

    INT nStatusBarWidth = rc.right - rc.left;                             
    INT arnRtEdge[STATUS_PANES] = {1};
    INT nIconPaneWidth = GetSystemMetrics(SM_CXSMICON) + (GetSystemMetrics(SM_CXEDGE) * 4);
    INT nWidthReqd = cxZone + cxUserName + (nIconPaneWidth * 1);

    arnRtEdge[STATUS_PANE_STATUS] = max(1, nStatusBarWidth - nWidthReqd);

    nWidthReqd -= cxUserName;
    arnRtEdge[STATUS_PANE_USERNAME] = max(1, nStatusBarWidth - nWidthReqd);

    /*
    nWidthReqd -= (nIconPaneWidth);
    arnRtEdge[STATUS_PANE_WRITEICON] = max(1, nStatusBarWidth - nWidthReqd);
    */

    arnRtEdge[STATUS_PANE_ZONE] = -1;

    LRESULT nParts = 0;
    nParts = SendMessage(m_hwndStatus, SB_GETPARTS, 0, 0L);
    if (nParts != STATUS_PANES)
    {
        for ( int n = 0; n < nParts; n++)
        {
            SendMessage(m_hwndStatus, SB_SETTEXT, n, NULL);
            SendMessage(m_hwndStatus, SB_SETICON, n, NULL);
        }
        SendMessage(m_hwndStatus, SB_SETPARTS, 0, 0L);
    }

    SendMessage(m_hwndStatus, SB_SETPARTS, STATUS_PANES, (LPARAM)arnRtEdge);

    SendMessage(m_hwndStatus, SB_GETRECT, 1, (LPARAM)&rc);
    InflateRect(&rc, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));

    SendMessage(m_hwndStatus, SB_SETTEXT, 1, (LPARAM)SZ_EMPTY);
    SendMessage(m_hwndStatus, SB_SETMINHEIGHT, GetSystemMetrics(SM_CYSMICON) + GetSystemMetrics(SM_CYBORDER) * 2, 0L);
    _SetZone();
    _SetUserParts();

#endif // OLD_STYLE_STATUSBAR
    return hr;
}


/****************************************************\
    Constructor
\****************************************************/
CStatusBar::CStatusBar(HWND hwndStatus)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pism);
    ASSERT(!m_pizm);
    ASSERT(!m_pszUserName);
    ASSERT(!m_pszUserNameTT);

    m_hwndStatus = hwndStatus;
    m_lCurrentZone = ZONE_UNKNOWN;

    LEAK_ADDREF(LEAK_CStatusBar);
}


/****************************************************\
    Destructor
\****************************************************/
CStatusBar::~CStatusBar(void)
{
    int nIndex;

    IUnknown_Set((IUnknown **) &m_pism, NULL);
    IUnknown_Set((IUnknown **) &m_pizm, NULL);

    Str_SetPtr(&m_pszUserName, NULL);
    Str_SetPtr(&m_pszUserNameTT, NULL);

    for (nIndex = 0; nIndex < MAX_NUM_ZONES_ICONS; nIndex++)
    {
        if (m_arhiconZones[nIndex])
            DestroyIcon(m_arhiconZones[nIndex]);
    }

    for (nIndex = 0; nIndex < ITD_MAX; nIndex++)
    {
        if (m_arhiconGeneral[nIndex])
            DestroyIcon(m_arhiconGeneral[nIndex]);
    }

    ASSERTNONCRITICAL;
    SendMessage(m_hwndStatus, SB_SETTEXT, 1 | SBT_NOBORDERS, 0);

    DllRelease();
    LEAK_DELREF(LEAK_CStatusBar);
}
