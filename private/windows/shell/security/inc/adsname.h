#ifndef _ADSNAME_H
#define _ADSNAME_H

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
BuildLDAPPathFromADsPath(LPCTSTR szADsPathName,
                         LPTSTR *pszLDAPPathName);  // free with LocalFree

BOOL WINAPI
_LookupAccountNameEx(LPCTSTR pszSystemName,
                     LPCTSTR pszAccountName,
                     PSID    pSid,
                     LPDWORD pdwSid,
                     LPTSTR  pszNT4Name,
                     DWORD   dwNT4Length,
                     LPTSTR  pszDomain,
                     LPDWORD pdwDomain,
                     PSID_NAME_USE peUse);

#ifdef __cplusplus
}
#endif

#endif  // _ADSNAME_H
