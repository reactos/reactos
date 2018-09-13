//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:        OAUTDLL.hxx
//
//  Contents:    
//
//  Classes:     
//
//  Functions:   
//
//  History:    05-15-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#ifndef OAUTDLL_HXX_
#define OAUTDLL_HXX_

//+---------------------------------------------------------------------------
//
//  Class:       COleAutDll
//
//  Purpose:     class wrapper for calling API from delay 
//               loaded oleaut32.dll  
//
//  Interface:    
//                        
//
//  History:     05-15-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes: 
//
//----------------------------------------------------------------------------

class COleAutDll 
{
public:
    // ctor, dtor
    COleAutDll();
    ~COleAutDll();

    // member funcs
    HRESULT Init();

#define DELAYOLEAUTAPI_HR(_fn, _args, _nargs) \
    HRESULT _fn _args { \
        HRESULT hr = Init(); \
        if (SUCCEEDED(hr)) { \
            hr = _pfn##_fn _nargs; \
        } \
        return hr;    } \
    HRESULT (STDAPICALLTYPE* _pfn##_fn) _args;

#define DELAYOLEAUTAPI_BSTR(_fn, _args, _nargs) \
    BSTR _fn _args { \
        HRESULT hres = Init(); \
        BSTR bstr = NULL; \
        if (SUCCEEDED(hres)) { \
            bstr = _pfn##_fn _nargs; \
        } \
        return bstr;    } \
    BSTR (STDAPICALLTYPE* _pfn##_fn) _args;

#define DELAYOLEAUTAPI_INT(_fn, _args, _nargs) \
    INT _fn _args { \
        HRESULT hres = Init(); \
        INT iRet = FALSE;\
        if (SUCCEEDED(hres)) { \
            iRet = _pfn##_fn _nargs; \
        } \
        return iRet;    } \
    INT (STDAPICALLTYPE* _pfn##_fn) _args;

#define DELAYOLEAUTAPI_UINT(_fn, _args, _nargs) \
    INT _fn _args { \
        HRESULT hres = Init(); \
        UINT iRet ;\
        if (SUCCEEDED(hres)) { \
            iRet = _pfn##_fn _nargs; \
        } \
        return iRet;    } \
    UINT (STDAPICALLTYPE* _pfn##_fn) _args;

#define DELAYOLEAUTAPI_VOID(_fn, _args, _nargs) \
    void _fn _args { \
        HRESULT hres = Init(); \
        if (SUCCEEDED(hres)) { \
            _pfn##_fn _nargs; \
        } \
    } \
    void (STDAPICALLTYPE* _pfn##_fn) _args;


    // APIs
    DELAYOLEAUTAPI_HR( VariantClear,  
        (VARIANTARG*    pvarg),
        (pvarg)
    )


    DELAYOLEAUTAPI_HR( VariantInit,  
        (VARIANTARG*    pvarg),
        (pvarg)
    )

    DELAYOLEAUTAPI_HR( VariantCopy,  
        (VARIANTARG*    pvargDest,
         VARIANTARG*    pvargSrc
        ),
        (pvargDest, pvargSrc)
    )

    DELAYOLEAUTAPI_HR( VariantChangeType,  
        (VARIANTARG*    pvargDest,
         VARIANTARG*    pvargSrc,
         USHORT         wFlags,
         VARTYPE        vt 
        ),
        (pvargDest, pvargSrc, wFlags, vt)
    )

    DELAYOLEAUTAPI_HR( LoadTypeLib,  
        (const OLECHAR* szFile,
         ITypeLib**     pptlib
        ),
        (szFile, pptlib)
    )


    DELAYOLEAUTAPI_BSTR( SysAllocStringByteLen,  
        (LPCSTR         psz,
         UINT           len
        ),
        (psz, len )
    )

    DELAYOLEAUTAPI_BSTR( SysAllocString,  
        (const OLECHAR* psz),
        (psz)
    )

    DELAYOLEAUTAPI_UINT( SysStringByteLen,  
        (BSTR           bstr),
        (bstr)
    )

    DELAYOLEAUTAPI_VOID( SysFreeString,  
        (BSTR           bstr),
        (bstr)
    )

private:
    BOOL    _fInited;
    HMODULE _hMod;
};


//+---------------------------------------------------------------------------
//
//  Method:     COleAutDll::COleAutDll
//
//  Synopsis:
//
//  History:    05-15-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline 
COleAutDll::COleAutDll() 
{
    _fInited = FALSE;
    _hMod = NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     COleAutDll::~COleAutDll
//
//  Synopsis:
//
//  History:    05-15-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
COleAutDll::~COleAutDll()
{
    if( _fInited)
        FreeLibrary(_hMod);
}


//+---------------------------------------------------------------------------
//
//  Method:     COleAutDll::Init
//
//  Synopsis:
//
//  History:    05-15-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
HRESULT COleAutDll::Init()
{
    HRESULT hr = NOERROR;
    if( !_fInited )
    {
        _hMod = LoadLibrary("oleaut32.dll");
        if( !_hMod )
            hr = HRESULT_FROM_WIN32(GetLastError());
        else 
        {
#define CHECKOAUTAPI(_fn) \
            *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hMod, #_fn); \
            if( !(_pfn##_fn)) hr = E_UNEXPECTED;

            CHECKOAUTAPI(VariantClear);
            CHECKOAUTAPI(VariantInit);
            CHECKOAUTAPI(VariantCopy);
            CHECKOAUTAPI(VariantChangeType);
            CHECKOAUTAPI(SysAllocStringByteLen);
            CHECKOAUTAPI(SysAllocString);
            CHECKOAUTAPI(SysStringByteLen);
            CHECKOAUTAPI(SysFreeString);
            CHECKOAUTAPI(LoadTypeLib);
        }


        if( hr == NOERROR )
            _fInited = TRUE;
    }


    return hr;
}

#endif
 

