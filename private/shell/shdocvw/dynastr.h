#ifndef _DYNASTR_H_
#define _DYNASTR_H_

#ifdef DEBUG
#define CCH_DYNASTR_MIN 32
#else
#define CCH_DYNASTR_MIN 80
#endif

class DynaStrA {
protected:
    CHAR    _ach[CCH_DYNASTR_MIN];
    LPSTR   _psz;

    LPSTR _Realloc(UINT cch);

public:
    operator LPCSTR() { return _psz; }
    BOOL lstrcpy(LPCSTR pszSrc);
    BOOL lstrcat(LPCSTR pszCat);
    BOOL lstrcpy(LPCWSTR pwszSrc);
    DynaStrA() : _psz(_ach) {}
    ~DynaStrA();
};

class DynaStrW {
protected:
    WCHAR    _ach[CCH_DYNASTR_MIN];
    LPWSTR   _psz;

    LPWSTR _Realloc(UINT cch);

public:
    operator LPCWSTR() { return _psz; }
    BOOL lstrcpy(LPCWSTR pszSrc);
    BOOL lstrcat(LPCWSTR pszCat);
    BOOL lstrcpy(LPCSTR pwszSrc);
    void empty() { _psz[0] = '\0'; }
    DynaStrW() : _psz(_ach) {}
    ~DynaStrW();
};

#ifdef UNICODE
#define DynaStr DynaStrW
#else
#define DynaStr DynaStrA
#endif

#endif // _DYNASTR_H_

