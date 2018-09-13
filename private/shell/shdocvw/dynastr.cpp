#include "priv.h"
#include "dynastr.h"

#define DM_DYNASTR  DM_TRACE

LPWSTR DynaStrW::_Realloc(UINT cch)
{
    ASSERT(cch >= CCH_DYNASTR_MIN);
    UINT cb = cch*SIZEOF(_ach[0]);

    if (_psz != _ach) {
        if ( LocalSize(_psz) < cb ) {
            LPWSTR pszNew = (LPWSTR)LocalReAlloc(_psz, cb, LMEM_MOVEABLE);
            TraceMsg(DM_DYNASTR, "DynaStrW::_Realloc reallocated memory %x->%x", _psz, pszNew);
            if (!pszNew) {
                return NULL;
            }
            _psz = pszNew;
        }
    } else {
        LPWSTR pszNew = (LPWSTR)LocalAlloc(LPTR, cb);
        TraceMsg(DM_DYNASTR, "DynaStrW::_Realloc allocated memory %x", pszNew);
        if (!pszNew) {
            return NULL;
        }
        _psz = pszNew;
    }

    return _psz;
}

BOOL DynaStrW::lstrcpy(LPCWSTR pwszSrc)
{
    UINT cch = lstrlenW(pwszSrc);
    if (cch < CCH_DYNASTR_MIN || _Realloc(cch+1)) {
        ASSERT(_psz==_ach || LocalSize(_psz) >= cch+1*SIZEOF(_psz[0]));
        memcpy(_psz, pwszSrc, (cch+1)*SIZEOF(_psz[0]));
        return TRUE;
    }
    return FALSE;
}

BOOL DynaStrW::lstrcpy(LPCSTR pszSrc)
{
    UINT cch = MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, NULL, 0);
    if (cch < CCH_DYNASTR_MIN || _Realloc(cch+1)) {
        ASSERT(_psz==_ach || LocalSize(_psz) >= cch+1*SIZEOF(_psz[0]));
        MultiByteToWideChar(CP_ACP, 0, pszSrc, -1, _psz, cch+1);
        return TRUE;
    }
    return FALSE;
}

DynaStrW::~DynaStrW()
{
    if (_psz != _ach) {
        TraceMsg(DM_DYNASTR, "dtr ~DynaStrW freeing allocated memory %x", _psz);
        LocalFree(_psz);
    }
}


LPSTR DynaStrA::_Realloc(UINT cch)
{
    ASSERT(cch >= CCH_DYNASTR_MIN);
    UINT cb = cch*SIZEOF(_ach[0]);

    if (_psz != _ach) {
        if ( LocalSize(_psz) < cb ) {
            LPSTR pszNew = (LPSTR)LocalReAlloc(_psz, cb, LMEM_MOVEABLE);
            TraceMsg(DM_DYNASTR, "DynaStrA::_Realloc reallocated memory %x->%x", _psz, pszNew);
            if (!pszNew) {
                return NULL;
            }
            _psz = pszNew;
        }
    } else {
        LPSTR pszNew = (LPSTR)LocalAlloc(LPTR, cb);
        TraceMsg(DM_DYNASTR, "DynaStrA::_Realloc allocated memory %x", pszNew);
        if (!pszNew) {
            return NULL;
        }
        _psz = pszNew;
    }

    return _psz;
}

BOOL DynaStrA::lstrcpy(LPCSTR pszSrc)
{
    UINT cch = lstrlen(pszSrc);
    if (cch < CCH_DYNASTR_MIN || _Realloc(cch+1)) {
        ASSERT(_psz==_ach || LocalSize(_psz) >= cch+1*SIZEOF(_psz[0]));
        memcpy(_psz, pszSrc, (cch+1)*SIZEOF(_psz[0]));
        return TRUE;
    }
    return FALSE;
}

BOOL DynaStrA::lstrcpy(LPCWSTR pwszSrc)
{
    UINT cch = WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, NULL, 0, NULL, NULL);
    if (cch < CCH_DYNASTR_MIN || _Realloc(cch+1)) {
        ASSERT(_psz==_ach || LocalSize(_psz) >= cch+1*SIZEOF(_psz[0]));
        WideCharToMultiByte(CP_ACP, 0, pwszSrc, -1, _psz, cch+1, NULL, NULL);
        return TRUE;
    }
    return FALSE;
}

DynaStrA::~DynaStrA()
{
    if (_psz != _ach) {
        TraceMsg(DM_DYNASTR, "dtr ~DynaStrA freeing allocated memory %x", _psz);
        LocalFree(_psz);
    }
}

// Some test cases
#if 0
extern HRESULT test0(LPCWSTR pwsz);

//
// Code size:   58 bytes
// Stack frame: 4096+
//
HRESULT test1(LPSTR pszIn)
{
    WCHAR wsz[MAX_URL_STRING*2];
    MultiByteToWideChar(CP_ACP, 0, pszIn, -1, wsz, ARRAYSIZE(wsz));
    return test0(wsz);
}

//
// Code size:   70 bytes
// Stack frame: 0+
//
HRESULT test2(LPSTR pszIn)
{
    LPWSTR pwsz = (LPWSTR)LocalAlloc(LPTR, 2*MAX_URL_STRING*SIZEOF(WCHAR));
    HRESULT hres;
    if (pwsz) {
        MultiByteToWideChar(CP_ACP, 0, pszIn, -1, pwsz, MAX_URL_STRING);
        hres = test0(pwsz);
        LocalFree(pwsz);
    } else {
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

//
// Code size:   68 bytes
// Stack frame: 0+
//
HRESULT test3(LPSTR pszIn)
{
    DynaStrW wsz;
    HRESULT hres;
    if (wsz.lstrcpy(pszIn)) {
        hres = test0(wsz);
    } else {
        hres = E_OUTOFMEMORY;
    }
    return hres;
}
#endif
