//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:        SDLL.hxx
//
//  Contents:    
//
//  Classes:     
//
//  Functions:   
//
//  History:    04-23-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#ifndef SDLL_HXX_
#define SDLL_HXX_

//+---------------------------------------------------------------------------
//
//  Class:       CShellDll 
//
//  Purpose:     class wrapper for calling API from delay 
//               loaded shell32.dll  
//
//  Interface:    
//                        
//
//  History:     04-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes: 
//
//----------------------------------------------------------------------------

class CShellDll 
{
public:
#define DELAYSHELLAPI(_fn, _args, _nargs) \
    HINSTANCE _fn _args { \
        HRESULT hres = Init(); \
        HINSTANCE hI = NULL; \
        if (SUCCEEDED(hres)) { \
            hI = _pfn##_fn _nargs; \
        } \
        return hI;    } \
    HINSTANCE (STDAPICALLTYPE* _pfn##_fn) _args;

    HRESULT Init();
    CShellDll();
    ~CShellDll();


    DELAYSHELLAPI( ShellExecuteA, 
        (   HWND   hwnd, 
            LPCSTR lpOperation, 
            LPCSTR lpFile, 
            LPCSTR lpParameters,
            LPCSTR lpDirectory, 
            INT    nShowCmd 
        ), 
        ( hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd )
    )

private:
    BOOL    _fInited;
    HMODULE _hMod;
};


//+---------------------------------------------------------------------------
//
//  Method:     CShellDll::CShellDll
//
//  Synopsis:
//
//  History:    04-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline 
CShellDll::CShellDll() 
{
    _fInited = FALSE;
    _hMod = NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CShellDll::~CShellDll
//
//  Synopsis:
//
//  History:    04-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
CShellDll::~CShellDll()
{
    if( _fInited)
        FreeLibrary(_hMod);
}


//+---------------------------------------------------------------------------
//
//  Method:     CShellDll::Init
//
//  Synopsis:
//
//  History:    04-24-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
HRESULT CShellDll::Init()
{
    HRESULT hr = S_OK;
    if( !_fInited )
    {
        _hMod = LoadLibrary("shell32.dll");
        if( !_hMod )
            hr = HRESULT_FROM_WIN32(GetLastError());
        else 
        {
#define CHECKAPI(_fn) \
            *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hMod, #_fn); \
            if( !(_pfn##_fn)) hr = E_UNEXPECTED;

            CHECKAPI(ShellExecuteA);
        }
    }

    if( hr == S_OK )
        _fInited = TRUE;

    return hr;
}

#endif
 

