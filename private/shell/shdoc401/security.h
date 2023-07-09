// Be careful; there is also a security.h file in sdk\inc.
/**************************************************************\
    FILE: security.h

    DESCRIPTION:
        This file will contain helper functions and objects that
    help deal with security.  This mainly means Zones Security, but
    can include other types.
\**************************************************************/

#ifndef _SECURITY_H
#define _SECURITY_H

#include <urlmon.h>

SHDOCAPI ZoneCheckPidl(LPCITEMIDLIST pidl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
SHDOCAPI ZoneCheckHDrop(IDataObject * pido, DWORD dwEffect, DWORD dwAction, DWORD dwFlags, IInternetSecurityMgrSite * pisms);

#endif // _SECURITY_H
