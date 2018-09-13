/*****************************************************************************\
    FILE: proxycache.cpp
    
    DESCRIPTION:
        FTP Folder uses WININET which doesn't work thru CERN proxies.  In that
    case, we need to hand control of the FTP URL back to the browser to do the
    old URLMON handling of it.  The problem is that testing for a CERN proxy
    blocking access is expensive.
\*****************************************************************************/

#include "priv.h"
#include "util.h"

#define PROXY_CACHE_SIZE    15

typedef struct
{
    TCHAR szServerName[INTERNET_MAX_HOST_NAME_LENGTH];
    BOOL fIsBlocking;
} PROXYCACHEENTRY;

static int g_nLastIndex = 0;
static BOOL g_fInited = FALSE;
static TCHAR g_szProxyServer[MAX_URL_STRING] = {0};
static PROXYCACHEENTRY g_ProxyCache[PROXY_CACHE_SIZE];






/////////////////////////////////////////////////////////////////////////
///////  Private helpers    /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void ProxyCache_Init(void)
{
    g_nLastIndex = 0;

    for (int nIndex = 0; nIndex < ARRAYSIZE(g_ProxyCache); nIndex++)
    {
        g_ProxyCache[nIndex].fIsBlocking = FALSE;
        g_ProxyCache[nIndex].szServerName[0] = 0;
    }

    g_fInited = TRUE;
}


/****************************************************\
    FUNCTION: ProxyCache_WasProxyChanged

    DESCRIPTION:
        See if someone changed the proxy settings via
    the inetcpl.  This is important because it is
    frustration to find FTP fails because of the proxy
    settings, fix the proxy settings, and then it still
    doesn't work because we cached the results.
\****************************************************/
BOOL ProxyCache_WasProxyChanged(void)
{
    BOOL fWasChanged = FALSE;
    TCHAR szCurrProxyServer[MAX_URL_STRING];
    DWORD cbSize = SIZEOF(szCurrProxyServer);

    // PERF: If I wanted to be really fast, I would cache the hkey
    //       so this would be faster.  But since my DLL can be loaded/unloaded
    //       serveral times in a process, I would leak each instance unless I
    //       released the hkey in DLL_PROCESS_DETACH
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_INTERNET_SETTINGS_LAN, SZ_REGVALUE_PROXY_SERVER, NULL, szCurrProxyServer, &cbSize))
    {
        // Is this the first time? (Is g_szProxyServer empty?)
        if (!g_szProxyServer[0])
            StrCpyN(g_szProxyServer, szCurrProxyServer, ARRAYSIZE(g_szProxyServer));

        // Did it change?
        if (StrCmp(szCurrProxyServer, g_szProxyServer))
            fWasChanged = TRUE;
    }

    return fWasChanged;
}



/////////////////////////////////////////////////////////////////////////
///////  APIs helpers    /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


/****************************************************\
    FUNCTION: ProxyCache_IsProxyBlocking

    DESCRIPTION:
        Look in the cache with the FTP server in pidl
    and see if we have a cached value that indicates
    if it's blocked by the proxy.

    PARAMETERS:
        *pfIsBlocking - Is the proxy blocking.
        return - Is the value cached
\****************************************************/
BOOL ProxyCache_IsProxyBlocking(LPCITEMIDLIST pidl, BOOL * pfIsBlocking)
{
    BOOL fIsInCache = FALSE;

    if (ProxyCache_WasProxyChanged())
        ProxyCache_Init();  // Purge the results

    *pfIsBlocking = FALSE;  // Assume we don't know.
    if (!g_fInited)
    {
        ProxyCache_Init();
    }
    else
    {
        int nCount = ARRAYSIZE(g_ProxyCache);
        TCHAR szNewServer[INTERNET_MAX_HOST_NAME_LENGTH];

        // BUGBUG: Check if the proxy has changed.

        // Is this the same server we tried last time?  If so,
        // let's just cache the return value.
        FtpPidl_GetServer(pidl, szNewServer, ARRAYSIZE(szNewServer));
        for (int nIndex = g_nLastIndex; nCount && g_ProxyCache[nIndex].szServerName[0]; nCount--, nIndex--)
        {
            if (nIndex < 0)
                nIndex = (PROXY_CACHE_SIZE - 1);

            if (!StrCmp(szNewServer, g_ProxyCache[nIndex].szServerName))
            {
                // Yes, so bail.
                *pfIsBlocking = g_ProxyCache[nIndex].fIsBlocking;
                fIsInCache = TRUE;
                break;
            }
        }
    }

    return fIsInCache;
}


/****************************************************\
    FUNCTION: ProxyCache_SetProxyBlocking

    DESCRIPTION:

    PARAMETERS:
        *pfIsBlocking - Is the proxy blocking.
        return - Is the value cached
\****************************************************/
void ProxyCache_SetProxyBlocking(LPCITEMIDLIST pidl, BOOL fIsBlocking)
{
    TCHAR szNewServer[INTERNET_MAX_HOST_NAME_LENGTH];

    // Add it to the cache because our caller will hit the server to
    // verify and we can be ready for next time.
    g_nLastIndex++;
    if (g_nLastIndex >= PROXY_CACHE_SIZE)
        g_nLastIndex = 0;

    FtpPidl_GetServer(pidl, szNewServer, ARRAYSIZE(szNewServer));
    StrCpyN(g_ProxyCache[g_nLastIndex].szServerName, szNewServer, ARRAYSIZE(g_ProxyCache[g_nLastIndex].szServerName));
    g_ProxyCache[g_nLastIndex].fIsBlocking = fIsBlocking;
}
