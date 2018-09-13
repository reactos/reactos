/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    persist.h

Abstract:

Author:

    Adriaan Canter (adriaanc) 13-Jan-1998

Revision History:

    13-Jan-1998 adriaanc
        Created

--*/

#ifndef PERSIST_H
#define PERSIST_H

#include <pstore.h>

#define INTERNET_SETTINGS_KEY         "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define DISABLE_PASSWORD_CACHE_VALUE  "DisablePasswordCaching"
#define DISABLE_PASSWORD_CACHE        1

#define CRED_PERSIST_NOT_AVAIL        0
#define CRED_PERSIST_AVAIL            1
#define CRED_PERSIST_UNKNOWN          2

// PWL related defines.

// Password-cache-entry, this should be in PCACHE.
#define PCE_WWW_BASIC 0x13  
#define MAX_AUTH_FIELD_LENGTH           MAX_FIELD_LENGTH * 2

#define WNETDLL_MODULE                  "mpr.dll"
#define PSTORE_MODULE                   "pstorec.dll"
#define WNETGETCACHEDPASS               "WNetGetCachedPassword"
#define WNETCACHEPASS                   "WNetCachePassword"
#define WNETREMOVECACHEDPASS            "WNetRemoveCachedPassword"

// MPR.DLL exports used by top level API.
typedef DWORD (APIENTRY *PFWNETGETCACHEDPASSWORD)    (LPSTR, WORD, LPSTR, LPWORD, BYTE);
typedef DWORD (APIENTRY *PFWNETCACHEPASSWORD)        (LPSTR, WORD, LPSTR, WORD, BYTE, UINT);
typedef DWORD (APIENTRY *PFWNETREMOVECACHEDPASSWORD) (LPSTR, WORD, BYTE);


// ----------------Public function prototypes----------------------

// Determines availability of credential cache.
DWORD  InetInitCredentialPersist();

// Persist credentials (username/password).
DWORD InetSetCachedCredentials  (LPSTR szHost, 
                                 LPSTR szRealmOrDomain, 
                                 LPSTR szUser, 
                                 LPSTR szPass);


// Get persisted credentials (username/password).
DWORD InetGetCachedCredentials  (LPSTR szHost, 
                                 LPSTR szRealmOrDomain, 
                                 LPSTR szUser, 
                                 LPSTR szPass);


// Remove persisted credentials (username/password).
DWORD InetRemoveCachedCredentials (LPSTR szHost, LPSTR szRealmOrDomain);



#endif //PERSIST_H


