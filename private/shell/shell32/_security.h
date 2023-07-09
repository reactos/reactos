/**************************************************************\
    FILE: _security.h

    DESCRIPTION:
        This file will contain helper functions and objects that
    help deal with security.  This mainly means Zones Security, but
    can include other types.

    We can't call this file "security.h" because there's already
    a file with that name in sdk\inc.
\**************************************************************/

#ifndef __SECURITY_H
#define __SECURITY_H

#include <urlmon.h>

SHDOCAPI ZoneCheckPidl(LPCITEMIDLIST pidl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms);
SHDOCAPI ZoneCheckHDrop(IDataObject * pido, DWORD dwEffect, DWORD dwAction, DWORD dwFlags, IInternetSecurityMgrSite * pisms);

#endif // __SECURITY_H
