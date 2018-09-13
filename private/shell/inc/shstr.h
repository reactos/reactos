/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: Shell String Class

    shstr.h

Author:

    Zeke Lucas (zekel)  27-Oct-96

Environment:

    User Mode - Win32

Revision History:


Abstract:

    this allows automatic resizing and stuff

    NOTE: this class is specifically designed to be used as a stack variable


--*/

#ifndef _SHSTR_H_

//  default shstr to MAX_PATH so that we can use them anywhere 
//  we currently assume a MAX_PATH buffer.
#define DEFAULT_SHSTR_LENGTH    MAX_PATH


#ifdef UNICODE
#define ShStr ShStrW
#define UrlStr UrlStrW
#else
#define ShStr ShStrA
#define UrlStr UrlStrA
#endif //UNICODE

class ShStrA
{
public:

    //
    //  Constructors
    //
    ShStrA();

    //
    //  Destructor
    //
    ~ShStrA()
        {Reset();}

    //
    // the first are the only ones that count
    //
    HRESULT SetStr(LPCSTR pszStr, DWORD cchStr);
    HRESULT SetStr(LPCSTR pszStr);
    HRESULT SetStr(LPCWSTR pwszStr, DWORD cchStr);

    // the rest just call into the first three
    HRESULT SetStr(LPCWSTR pwszStr)
        {return SetStr(pwszStr, (DWORD) -1);}
    HRESULT SetStr(ShStrA &shstr)
        {return SetStr(shstr._pszStr);}


    ShStrA& operator=(LPCSTR pszStr)
        {SetStr(pszStr); return *this;}
    ShStrA& operator=(LPCWSTR pwszStr)
        {SetStr(pwszStr); return *this;}
    ShStrA& operator=(ShStrA &shstr)
        {SetStr(shstr._pszStr); return *this;}


    LPSTR GetStr()
        {return _pszStr;}
    operator LPSTR()
        {return _pszStr;}

    HRESULT Append(LPCSTR pszStr, DWORD cchStr);
    HRESULT Append(LPCSTR pszStr)
        {return Append(pszStr, (DWORD) -1);}
    HRESULT Append(CHAR ch)
        {return Append(&ch, 1);}

    //
    //  the Clone methods return memory that must be freed
    //
    ShStrA *Clone();
    LPSTR CloneStrA();
    LPWSTR CloneStrW();
    LPSTR CloneStr()
        {return CloneStrA();}

    
    VOID Reset();
    VOID Trim();

#ifdef DEBUG
    BOOL IsValid();
#else
    inline BOOL IsValid()
    {return _pszStr != NULL;}
#endif //DEBUG

    DWORD GetSize()
        {ASSERT(!(_cchSize % DEFAULT_SHSTR_LENGTH)); return (_pszStr ? _cchSize : 0);}

    HRESULT SetSize(DWORD cchSize);
    DWORD GetLen()
        {return lstrlenA(_pszStr);}



protected:
//    friend UrlStr;
/*
    TCHAR GetAt(DWORD cch)
        {return cch < _cchSize ? _pszStr[cch] : TEXT('\0');}
    TCHAR SetAt(TCHAR ch, DWORD cch)
        {return cch < _cchSize ? _pszStr[cch] = ch : TEXT('\0');}
*/
private:

    HRESULT _SetStr(LPCSTR psz);
    HRESULT _SetStr(LPCSTR psz, DWORD cb);
    HRESULT _SetStr(LPCWSTR pwszStr, DWORD cchStr);

    CHAR _szDefaultBuffer[DEFAULT_SHSTR_LENGTH];
    LPSTR _pszStr;
    DWORD _cchSize;


}; //ShStrA


class ShStrW
{
public:

    //
    //  Constructors
    //
    ShStrW();

    //
    //  Destructor
    //
    ~ShStrW()
        {Reset();}

    //
    // the first are the only ones that count
    //
    HRESULT SetStr(LPCSTR pszStr, DWORD cchStr);
    HRESULT SetStr(LPCSTR pszStr);
    HRESULT SetStr(LPCWSTR pwszStr, DWORD cchStr);

    // the rest just call into the first three
    HRESULT SetStr(LPCWSTR pwszStr)
        {return SetStr(pwszStr, (DWORD) -1);}
    HRESULT SetStr(ShStrW &shstr)
        {return SetStr(shstr._pszStr);}


    ShStrW& operator=(LPCSTR pszStr)
        {SetStr(pszStr); return *this;}
    ShStrW& operator=(LPCWSTR pwszStr)
        {SetStr(pwszStr); return *this;}
    ShStrW& operator=(ShStrW &shstr)
        {SetStr(shstr._pszStr); return *this;}


    LPWSTR GetStr()
        {return _pszStr;}
    operator LPWSTR()
        {return _pszStr;}

    HRESULT Append(LPCWSTR pszStr, DWORD cchStr);
    HRESULT Append(LPCWSTR pszStr)
        {return Append(pszStr, (DWORD) -1);}
    HRESULT Append(WCHAR ch)
        {return Append(&ch, 1);}

    //
    //  the Clone methods return memory that must be freed
    //
    ShStrW *Clone();
    LPSTR CloneStrA();
    LPWSTR CloneStrW();
    LPWSTR CloneStr()
        {return CloneStrW();}

    
    VOID Reset();
    VOID Trim();

#ifdef DEBUG
    BOOL IsValid();
#else
    BOOL IsValid() 
    {return (BOOL) (_pszStr ? TRUE : FALSE);}
#endif //DEBUG

    DWORD GetSize()
        {ASSERT(!(_cchSize % DEFAULT_SHSTR_LENGTH)); return (_pszStr ? _cchSize : 0);}

    HRESULT SetSize(DWORD cchSize);
    DWORD GetLen()
        {return lstrlenW(_pszStr);}



protected:
//    friend UrlStr;
/*
    TCHAR GetAt(DWORD cch)
        {return cch < _cchSize ? _pszStr[cch] : TEXT('\0');}
    TCHAR SetAt(TCHAR ch, DWORD cch)
        {return cch < _cchSize ? _pszStr[cch] = ch : TEXT('\0');}
*/
private:

    HRESULT _SetStr(LPCSTR psz);
    HRESULT _SetStr(LPCSTR psz, DWORD cb);
    HRESULT _SetStr(LPCWSTR pwszStr, DWORD cchStr);

    WCHAR _szDefaultBuffer[DEFAULT_SHSTR_LENGTH];
    LPWSTR _pszStr;
    DWORD _cchSize;


}; //ShStrW

#ifdef UNICODE
typedef ShStrW  SHSTR;
typedef ShStrW  *PSHSTR;
#else
typedef ShStrA  SHSTR;
typedef ShStrA  *PSHSTR;
#endif //UNICODE

typedef ShStrW  SHSTRW;
typedef ShStrW  *PSHSTRW;

typedef ShStrA  SHSTRA;
typedef ShStrA  *PSHSTRA;



#if 0  //DISABLED until i have written the SHUrl* functions - zekel 7-Nov-96

class UrlStr 
{
public:
    UrlStr()
        {return;}

    operator LPCTSTR();
    operator SHSTR();

    UrlStr &SetUrl(LPCSTR pszUrl);
    UrlStr &SetUrl(LPCWSTR pwszUrl);
    UrlStr &SetUrl(LPCSTR pszUrl, DWORD cchUrl);
    UrlStr &SetUrl(LPCWSTR pwszUrl, DWORD cchUrl);

    DWORD GetScheme();
    VOID GetSchemeStr(PSHSTR pstrScheme);

    HRESULT Combine(LPCTSTR pszUrl, DWORD dwFlags);

/*
    ShStr &GetLocation();
    ShStr &GetAnchor();
    ShStr &GetQuery();

    HRESULT Canonicalize(DWORD dwFlags);
*/
protected:
    SHSTR  _strUrl;
};
#endif //DISABLED


#endif // _SHSTR_H_
