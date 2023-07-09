#ifndef __UNXFILE__
#define __UNXFILE__

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void UnixEnsureDir( char* pszFile );
void UnixifyFileName( char* lpszName);

HRESULT CheckIEFeatureOnUnix(LPCWSTR pwszIEFeature, DWORD* dwInstalledVerHi, DWORD* dwInstalledVerLo, DWORD dwFlags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __UNXFILE__ */

