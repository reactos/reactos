/*****************************************************************************\
    FILE: passwordapi.h
    
    DESCRIPTION:
        We want to store FTP passwords in a secure API.  We will use the
    PStore APIs on WinNT and the PWL APIs on Win9x.  This code was taken
    from wininet.

    Copyright (c) 1998  Microsoft Corporation
\*****************************************************************************/

#ifndef _PASSWORDAPI_H
#define _PASSWORDAPI_H

#include "priv.h"
#include "util.h"


// ----------------Public function prototypes----------------------

// Determines availability of credential cache.
HRESULT InitCredentialPersist(void);

// Persist credentials (username/password).
HRESULT SetCachedCredentials(LPCWSTR pszKey, LPCWSTR pszValue);

// Get persisted credentials (username/password).
HRESULT GetCachedCredentials(LPCWSTR pszKey, LPWSTR pszValue, DWORD cchSize);

// Remove persisted credentials (username/password).
HRESULT RemoveCachedCredentials(LPCWSTR pszKey);

HRESULT InitCredentialPersist(void);


#endif // _PASSWORDAPI_H
