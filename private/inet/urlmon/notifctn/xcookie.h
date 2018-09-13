//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       xcookie.h
//
//  Contents:   the CPackage cookie class
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//  NOTE:       clean up work left
//----------------------------------------------------------------------------

//typedef NOTIFICATIONCOOKIE XCOOKIE;
//typedef SYSTEMTIME XCOOKIE;
#include <delaydll.h>

typedef struct _tagXCOOKIE
{
    FILETIME    _date;
    DWORD       _dwThreadID;
    DWORD       _dwCounter;
} XCOOKIE;

#ifdef __cplusplus
#define REFXCOOKIE const XCOOKIE &
//#define REFNOTIFICATIONCOOKIE const SYSTEMTIME &
#else   // ! __cplusplus
#define REFXCOOKIE const XCOOKIE *const
//#define REFNOTIFICATIONCOOKIE const SYSTEMTIME *const
#endif  // __cplusplus

#ifdef _is_a_guid_


#ifdef __cplusplus
#define REFXCOOKIE const XCOOKIE &
#define REFNOTIFICATIONCOOKIE const NOTIFICATIONCOOKIE &
#else   // ! __cplusplus
#define REFXCOOKIE const XCOOKIE *const
#define REFNOTIFICATIONCOOKIE const NOTIFICATIONCOOKIE *const
#endif  // __cplusplus


#ifdef __cplusplus
inline BOOL IsEqualXCOOKIE(REFXCOOKIE rxcookie1, REFXCOOKIE rxcookie2)
{
    return !memcmp(&rxcookie1, &rxcookie2, sizeof(XCOOKIE));
}
#else   //  ! __cplusplus
#define IsEqualXCOOKIE(rxcookie1, rxcookie2) (!memcmp(rxcookie1, rxcookie2, sizeof(XCOOKIE)))
#endif  //  __cplusplus

#ifdef __cplusplus

// because XCOOKIE is defined elsewhere in WIN32 land, the operator == and !=
// are moved outside the class to global scope.

inline BOOL operator==(const XCOOKIE& xcookieOne, const XCOOKIE& xcookieOther)
{
#ifdef _WIN32
    return !memcmp(&xcookieOne,&xcookieOther,sizeof(XCOOKIE));
#else
    return !_fmemcmp(&xcookieOne,&xcookieOther,sizeof(XCOOKIE)); }
#endif
}

inline BOOL operator!=(const XCOOKIE& xcookieOne, const XCOOKIE& xcookieOther)
{
    return !(xcookieOne == xcookieOther);
}

#endif // __cpluscplus

#endif //_is_a_guid_

#ifdef __cplusplus

ULONG GetGlobalCounter();

class CPkgCookie : public XCOOKIE
{
public:
    //
    // this ctor creates a new unique cookie
    //
    CPkgCookie()
    {
        SYSTEMTIME      st;
        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &_date);
        _dwThreadID = GetCurrentThreadId();
        _dwCounter = (DWORD)GetGlobalCounter();
    }
    CPkgCookie(CLSID &clsid)
    {
        memcpy(this,&clsid,sizeof(CPkgCookie));
    }
    CPkgCookie(REFCLSID rclsid)
    {
        memcpy(this,&rclsid,sizeof(CPkgCookie));
    }

    inline operator NOTIFICATIONCOOKIE ()
    {
        return *((CLSID *)this);
    }

    inline operator NOTIFICATIONCOOKIE* ()
    {
        return (CLSID *)this;
    }
};

#endif // __cpluscplus

