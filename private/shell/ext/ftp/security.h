/*****************************************************************************\
    FILE: security.h
\*****************************************************************************/

#ifndef _SECURITY_H
#define _SECURITY_H


BOOL ZoneCheckUrlAction(IUnknown * punkSite, DWORD dwAction, LPCTSTR pszUrl, DWORD dwFlags);
BOOL ZoneCheckPidlAction(IUnknown * punkSite, DWORD dwAction, LPCITEMIDLIST pidl, DWORD dwFlags);


#endif // _SECURITY_H
