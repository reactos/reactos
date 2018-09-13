// 
// Pei-Hwa Lin (peiwhal), July 25, 1997
//

#include "urltrk.h"

#define TRACK_FLAGS (TRACK_OFFLINE_CACHE_ENTRY | TRACK_ONLINE_CACHE_ENTRY)

#ifdef unix
extern "C"
#endif /* unix */
BOOL WINAPI
IsLoggingEnabledA
(
    IN LPCSTR  pszUrl
)
{
    CHAR        szCanonicalUrl[INTERNET_MAX_URL_LENGTH];
    DWORD       dwSize = INTERNET_MAX_URL_LENGTH;
    BOOL        fTrack = FALSE;
    ULONG       dwTrack;

    // canonicalize URL
    InternetCanonicalizeUrlA(pszUrl, szCanonicalUrl, &dwSize, ICU_DECODE);
 
    dwTrack = _IsLoggingEnabled(szCanonicalUrl);
      
    fTrack = (dwTrack & TRACK_FLAGS);
    SetLastError(0);
    return fTrack;

}


BOOL WINAPI
WriteHitLogging
(
    IN LPHIT_LOGGING_INFO lpLogInfo
)
{
    CHAR        szCanonicalUrl[INTERNET_MAX_URL_LENGTH];
    DWORD       dwSize = INTERNET_MAX_URL_LENGTH;
    BOOL        bRet = FALSE;
    BOOL        foffline;
    ULONG       dwTrack; 
    MY_LOGGING_INFO  mLi;

    if (!lpLogInfo->lpszLoggedUrlName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return bRet;
    }

    // canonicalize URL
    InternetCanonicalizeUrlA(lpLogInfo->lpszLoggedUrlName, szCanonicalUrl, &dwSize, ICU_DECODE);
 
    // don't log if it's disalbed at first place
    dwTrack = _IsLoggingEnabled(szCanonicalUrl);

    foffline = IsGlobalOffline();
    if (dwTrack & TRACK_FLAGS)
    {
        if (((dwTrack & TRACK_OFFLINE_CACHE_ENTRY) && foffline) ||
            ((dwTrack & TRACK_ONLINE_CACHE_ENTRY) && !foffline))
        {
            mLi.pLogInfo = lpLogInfo;
            mLi.fOffLine = foffline;
            bRet = _WriteHitLogging(&mLi);
        }
    }
    else
    {
        SetLastError(0);
        bRet = TRUE;
    }

    return bRet;
}
