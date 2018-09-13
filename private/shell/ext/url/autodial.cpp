/*****************************************************************/
/**               Microsoft Windows                             **/
/**           Copyright (C) Microsoft Corp., 1995               **/
/*****************************************************************/

//
//  AUTODIAL.CPP - winsock autodial hook code
//

//  HISTORY:
//
//  3/22/95 jeremys     Created.
//  4/11/97 darrenmi    Moved functionality to wininet. Only stubs remain.
//

#include "project.hpp"
#include <wininet.h>

/*******************************************************************

    The following stubs are retained for compatibility.  This 
    functionality has been moved to wininet.

********************************************************************/

INTSHCUTAPI BOOL WINAPI InetIsOffline(DWORD dwFlags)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if(InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

INTSHCUTAPI STDAPI_(BOOL) WINAPI SetInetOffline(BOOL fOffline)
{
    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));
    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));

    return fOffline;
}

// forward this call to wininet.  Toast once appropriate registry entries
// are set.
extern "C" void AutodialHookCallback(DWORD dwOpCode, LPCVOID lpParam);
extern "C" void InternetAutodialCallback(DWORD dwOpCode, LPCVOID lpParam);

void AutodialHookCallback(DWORD dwOpCode,LPCVOID lpParam)
{
    InternetAutodialCallback(dwOpCode, lpParam);
}
