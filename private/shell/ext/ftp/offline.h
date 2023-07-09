/****************************************************\
    FILE: offline.h

    DESCRIPTION:
        Handle 'offline' status and Dial-up UI
\****************************************************/

#ifndef _OFFLINE_H
#define _OFFLINE_H


#ifdef FEATURE_OFFLINE
BOOL IsGlobalOffline(VOID);
VOID SetOffline(IN BOOL fOffline);
#endif // FEATURE_OFFLINE

HRESULT AssureNetConnection(HINTERNET hint, HWND hwndParent, LPCWSTR pwzServerName, LPCITEMIDLIST pidl, BOOL fShowUI);


#endif // _OFFLINE_H

