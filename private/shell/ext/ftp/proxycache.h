/*****************************************************************************\
    FILE: proxycache.h
    
    DESCRIPTION:
        FTP Folder uses WININET which doesn't work thru CERN proxies.  In that
    case, we need to hand control of the FTP URL back to the browser to do the
    old URLMON handling of it.  The problem is that testing for a CERN proxy
    blocking access is expensive.
\*****************************************************************************/

#ifndef _PROXYCACHE_H
#define _PROXYCACHE_H

// Public APIs (DLL wide)
BOOL ProxyCache_IsProxyBlocking(LPCITEMIDLIST pidl, BOOL * pfIsBlocking);
void ProxyCache_SetProxyBlocking(LPCITEMIDLIST pidl, BOOL fIsBlocking);

#endif // _PROXYCACHE_H



