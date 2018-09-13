//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       unicwrap.hxx
//
//  Contents:   Wrappers for all Unicode functions used in the Forms^3 project.
//
//----------------------------------------------------------------------------

#ifndef I_UNICWRAP_HXX_
#define I_UNICWRAP_HXX_
#pragma INCMSG("--- Beg 'unicwrap.hxx'")

#ifndef HIWORD64
  #ifdef _WIN64
    #define HIWORD64(p)     ((ULONG_PTR)(p) >> 16)
  #else
    #define HIWORD64        HIWORD
  #endif
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CConvertStr (CStr)
//
//  Purpose:    Base class for conversion classes.
//
//----------------------------------------------------------------------------

class CConvertStr
{
public:
    operator char *();

protected:
    CConvertStr(UINT uCP);
    ~CConvertStr();
    void Free();

    UINT    _uCP;
    LPSTR   _pstr;
    char    _ach[MAX_PATH * sizeof(WCHAR)];
};



//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::CConvertStr
//
//  Synopsis:   ctor.
//
//----------------------------------------------------------------------------

inline
CConvertStr::CConvertStr(UINT uCP)
{
    _uCP = uCP;
    _pstr = NULL;
}



//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::~CConvertStr
//
//  Synopsis:   dtor.
//
//----------------------------------------------------------------------------

inline
CConvertStr::~CConvertStr()
{
    Free();
}





//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::operator char *
//
//  Synopsis:   Returns the string.
//
//----------------------------------------------------------------------------

inline
CConvertStr::operator char *()
{
    return _pstr;
}



//+---------------------------------------------------------------------------
//
//  Class:      CStrIn (CStrI)
//
//  Purpose:    Converts string function arguments which are passed into
//              a Windows API.
//
//----------------------------------------------------------------------------

class CStrIn : public CConvertStr
{
public:
    CStrIn(LPCWSTR pwstr);
    CStrIn(LPCWSTR pwstr, int cwch);
    CStrIn(UINT uCP, LPCWSTR pwstr);
    CStrIn(UINT uCP, LPCWSTR pwstr, int cwch);
    int strlen();

protected:
    CStrIn();
    void Init(LPCWSTR pwstr, int cwch);

    int _cchLen;
};




//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Inits the class with a given length
//
//----------------------------------------------------------------------------

inline
CStrIn::CStrIn(LPCWSTR pwstr, int cwch) : CConvertStr(CP_ACP)
{
    Init(pwstr, cwch);
}

inline
CStrIn::CStrIn(UINT uCP, LPCWSTR pwstr, int cwch) : CConvertStr(uCP==1200?CP_ACP:uCP)
{
    Init(pwstr, cwch);
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Initialization for derived classes which call Init.
//
//----------------------------------------------------------------------------

inline
CStrIn::CStrIn() : CConvertStr(CP_ACP)
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::strlen
//
//  Synopsis:   Returns the length of the string in characters, excluding
//              the terminating NULL.
//
//----------------------------------------------------------------------------

inline int
CStrIn::strlen()
{
    return _cchLen;
}



//+---------------------------------------------------------------------------
//
//  Class:      CStrInMulti (CStrIM)
//
//  Purpose:    Converts multiple strings which are terminated by two NULLs,
//              e.g. "Foo\0Bar\0\0"
//
//----------------------------------------------------------------------------

class CStrInMulti : public CStrIn
{
public:
    CStrInMulti(LPCWSTR pwstr);
};



//+---------------------------------------------------------------------------
//
//  Class:      CStrOut (CStrO)
//
//  Purpose:    Converts string function arguments which are passed out
//              from a Windows API.
//
//----------------------------------------------------------------------------

class CStrOut : public CConvertStr
{
public:
    CStrOut(LPWSTR pwstr, int cwchBuf);
    ~CStrOut();

    int     BufSize();
    int     ConvertIncludingNul();
    int     ConvertExcludingNul();

private:
    LPWSTR  _pwstr;
    int     _cwchBuf;
};



//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::BufSize
//
//  Synopsis:   Returns the size of the buffer to receive an out argument,
//              including the terminating NULL.
//
//----------------------------------------------------------------------------

inline int
CStrOut::BufSize()
{
    return _cwchBuf * sizeof(WCHAR);
}





//
//	Multi-Byte ---> Unicode conversion
//

//+---------------------------------------------------------------------------
//
//  Class:      CConvertStrW (CStr)
//
//  Purpose:    Base class for multibyte conversion classes.
//
//----------------------------------------------------------------------------

class CConvertStrW
{
public:
    operator WCHAR *();

protected:
    CConvertStrW();
    ~CConvertStrW();
    void Free();

    LPWSTR   _pwstr;
    WCHAR    _awch[MAX_PATH * sizeof(WCHAR)];
};



//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::CConvertStrW
//
//  Synopsis:   ctor.
//
//----------------------------------------------------------------------------

inline
CConvertStrW::CConvertStrW()
{
    _pwstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::~CConvertStrW
//
//  Synopsis:   dtor.
//
//----------------------------------------------------------------------------

inline
CConvertStrW::~CConvertStrW()
{
    Free();
}

//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::operator WCHAR *
//
//  Synopsis:   Returns the string.
//
//----------------------------------------------------------------------------

inline 
CConvertStrW::operator WCHAR *()
{
    return _pwstr;
}


//+---------------------------------------------------------------------------
//
//  Class:      CStrInW (CStrI)
//
//  Purpose:    Converts multibyte strings into UNICODE
//
//----------------------------------------------------------------------------

class CStrInW : public CConvertStrW
{
public:
    CStrInW(LPCSTR pstr) { Init(pstr, -1); }
    CStrInW(LPCSTR pstr, int cch) { Init(pstr, cch); }
    CStrInW();

    int strlen();
    void Init(LPCSTR pstr, int cch);

protected:
    int _cwchLen;
};

//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::CStrInW
//
//  Synopsis:   Initialization for derived classes which call Init.
//
//----------------------------------------------------------------------------

inline
CStrInW::CStrInW()
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::strlen
//
//  Synopsis:   Returns the length of the string in characters, excluding
//              the terminating NULL.
//
//----------------------------------------------------------------------------

inline int
CStrInW::strlen()
{
    return _cwchLen;
}

//+---------------------------------------------------------------------------
//
//  Class:      CStrOutW (CStrO)
//
//  Purpose:    Converts returned unicode strings into ANSI.  Used for [out]
//				params (so we initialize with a buffer that should later be
//				filled with the correct ansi data)
//			
//
//----------------------------------------------------------------------------

class CStrOutW : public CConvertStrW
{
public:
    CStrOutW(LPSTR pstr, int cchBuf);
    ~CStrOutW();

    int     BufSize();
    int     ConvertIncludingNul();
    int     ConvertExcludingNul();

private:
    LPSTR  	_pstr;
    int     _cchBuf;
};

//+---------------------------------------------------------------------------
//
//  Member:     CStrOutW::BufSize
//
//  Synopsis:   Returns the size of the buffer to receive an out argument,
//              including the terminating NULL.
//
//----------------------------------------------------------------------------

inline int
CStrOutW::BufSize()
{
    return _cchBuf;
}

#pragma INCMSG("--- End 'unicwrap.hxx'")
#else
#pragma INCMSG("*** Dup 'unicwrap.hxx'")
#endif
