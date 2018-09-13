//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cdlinfo.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    02-20-97   t-alans (Alan Shi)   Created
//
//----------------------------------------------------------------------------

#include <trans.h>
#include <objbase.h>
#include <wchar.h>

// AS: ICodeDownloadInfo added to urlmon.idl (local change)
//     modified urlint.h to add SZ_CODEDOWNLOADINFO

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::CCodeDownloadInfo
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCodeDownloadInfo::CCodeDownloadInfo()
: _szCodeBase( NULL )
, _ulMajorVersion( 0 )
, _ulMinorVersion( 0 )
, _cRefs( 1 )
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::~CCodeDownloadInfo
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CCodeDownloadInfo::~CCodeDownloadInfo()
{
    if (_szCodeBase != NULL)
    {
        CoTaskMemFree((void *)_szCodeBase);
        _szCodeBase = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::QueryInterface
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT          hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ICodeDownloadInfo))
    {
        *ppvObj = (void *)this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::AddRef
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCodeDownloadInfo::AddRef(void)
{
    return ++_cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::Release
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCodeDownloadInfo::Release(void)
{
    if (!--_cRefs)
    {
        delete this;
    }

    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetCodeBase
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetCodeBase(LPWSTR *szCodeBase)
{
    wcscpy(*szCodeBase, _szCodeBase);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetCodeBase
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetCodeBase(LPCWSTR szCodeBase)
{
    HRESULT               hr = E_FAIL;
    long                  lStrlen = 0;
    
    if (_szCodeBase != NULL)
    {
        CoTaskMemFree((void *)_szCodeBase);
        _szCodeBase = NULL;
    }
#ifndef unix
    lStrlen = 2 * (wcslen(szCodeBase) + 1);
#else
    lStrlen =  sizeof(WCHAR) * (wcslen(szCodeBase) + 1);
#endif /* unix */
    _szCodeBase = (LPWSTR)CoTaskMemAlloc(lStrlen);
    hr = (_szCodeBase == NULL) ? (E_OUTOFMEMORY) : (S_OK);
    if (_szCodeBase != NULL)
    {
        wcscpy(_szCodeBase, szCodeBase);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetMinorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetMinorVersion(ULONG ulVersion)
{
    _ulMinorVersion = ulVersion;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetMinorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetMinorVersion(ULONG *pulVersion)
{
    *pulVersion = _ulMinorVersion;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetMajorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetMajorVersion(ULONG ulVersion)
{
    _ulMajorVersion = ulVersion;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetMajorVersion
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetMajorVersion(ULONG *pulVersion)
{
    *pulVersion = _ulMajorVersion;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::GetClassID
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::GetClassID(CLSID *clsid)
{
    *clsid = _clsid;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCodeDownloadInfo::SetClassID
//
//  Synopsis:
//
//  Arguments:  
//              
//              
//
//  Returns:
//
//  History:    01-27-1997   t-alans (Alan Shi)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CCodeDownloadInfo::SetClassID(CLSID clsid)
{
    _clsid = clsid;

    return S_OK;
}

