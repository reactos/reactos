//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CCodeFt.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>

PerfDbgTag(tagDft,     "Pluggable MF", "Log CDataFt", DEB_PROT)
    DbgTag(tagDftErr,  "Pluggable MF", "Log CDataFtErrors", DEB_PROT|DEB_ERROR)
#define SZDFILTERROOT  "PROTOCOLS\\Filter\\Data Filter\\"


//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::QueryInterface
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    06-26-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CStdZFilter::QueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagDft, this, "+CStdFilter::QueryInterface");

    VDATEPTROUT(ppvObj, void*);
    VDATETHIS(this);

    HRESULT hr = NOERROR;
    if( riid == IID_IUnknown || 
        riid == IID_IDataFilter )
    {
        *ppvObj = this;
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    PerfDbgLog1(
        tagDft, this, "-CStdZFilter::QueryInterface(hr:%1x)", hr);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::AddRef
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    06-26-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) 
CStdZFilter::AddRef()
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::AddRef");
    LONG lRet = ++_CRefs;
    DllAddRef();
    PerfDbgLog1(tagDft, this, "-CStdZFilter::AddRef(cRef:%1d)", lRet);

    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::Release
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    06-26-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) 
CStdZFilter::Release()
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::Release");
    LONG lRet = --_CRefs;
    if(!lRet)
        delete this;
    DllRelease();
    PerfDbgLog1(tagDft, this, "-CStdZFilter::Release(cRef:%1d)", lRet);

    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::DoEncode
//
//  Synopsis:
//
//  Arguments:  [dwFlags]           - flags
//              [lInBufferSize]     - Input Buffer Size
//              [pbInBuffer]        - Input Buffer
//              [lOutBufferSize]    - Output Buffer Size  
//              [pbOutBuffer]       - Output Buffer
//              [lInBytesAvailable] - Data available in Input Buffer
//              [plInBytesRead]     - Total data read from input buffer
//              [plOutBytesWritten] - Total data written to output buffer
//              [dwReserved]        - currently not used 
//
//  Returns:    NOERROR or E_FAIL
//
//  History:    06-27-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CStdZFilter::DoEncode(
        DWORD dwFlags,
        LONG  lInBufferSize,
        BYTE* pbInBuffer,
        LONG  lOutBufferSize,  
        BYTE* pbOutBuffer,
        LONG  lInBytesAvailable,
        LONG* plInBytesRead,
        LONG* plOutBytesWritten,
        DWORD dwReserved
)
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::DoEncode");
    HRESULT hr = NOERROR;

    if( !_pEncodeCtx )
        hr = CreateCompression(&_pEncodeCtx, _ulSchema);

    if( _pEncodeCtx && !FAILED(hr) )
    {
        PProtAssert( pbInBuffer && pbOutBuffer && 
                     plInBytesRead && plOutBytesWritten);

        hr = Compress(
            _pEncodeCtx,
            pbInBuffer,
            lInBytesAvailable,
            pbOutBuffer, 
            lOutBufferSize,
            plInBytesRead,
            plOutBytesWritten, 
            _cEncLevel
        );
    }

    PerfDbgLog1(tagDft, this, "-CStdZFilter::DoEncode(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::DoDecode
//
//  Synopsis:
//
//  Arguments:  [dwFlags]           - flags
//              [lInBufferSize]     - Input Buffer Size
//              [pbInBuffer]        - Input Buffer
//              [lOutBufferSize]    - Output Buffer Size  
//              [pbOutBuffer]       - Output Buffer
//              [lInBytesAvailable] - Data available in Input Buffer
//              [plInBytesRead]     - Total data read from input buffer
//              [plOutBytesWritten] - Total data written to output buffer
//              [dwReserved]        - reserved 
//
//  Returns:    NOERROR or E_FAIL
//
//  History:    06-27-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CStdZFilter::DoDecode(
        DWORD dwFlags,
        LONG  lInBufferSize,
        BYTE* pbInBuffer,
        LONG  lOutBufferSize,  
        BYTE* pbOutBuffer,
        LONG  lInBytesAvailable,
        LONG* plInBytesRead,
        LONG* plOutBytesWritten,
        DWORD dwReserved
)
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::DoDecode");
    HRESULT hr = NOERROR;

    if( !_pDecodeCtx )
        hr = CreateDecompression(&_pDecodeCtx, _ulSchema);

    if( _pDecodeCtx && !FAILED(hr) )
    {
        PProtAssert( pbInBuffer && pbOutBuffer && 
                     plInBytesRead && plOutBytesWritten);

        hr = Decompress(
            _pDecodeCtx,
            pbInBuffer,
            lInBytesAvailable,
            pbOutBuffer, 
            lOutBufferSize,
            plInBytesRead,
            plOutBytesWritten
        );
    }

    PerfDbgLog1(tagDft, this, "-CStdZFilter::DoDecode(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::SetEncodingLevel
//
//  Synopsis:
//
//  Arguments:  [cEncLevel]     - encoding level (between 1-100) 
//
//  Returns:    NOERROR or E_FAIL
//
//  History:    06-27-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP 
CStdZFilter::SetEncodingLevel(DWORD dwEncLevel)
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::SetEncodingLevel");
    HRESULT hr = NOERROR;
    
    if( dwEncLevel >= 1 && dwEncLevel <= 100 )
    {
        // input value should be between 1 - 100] 
        // our formula is _cEncLevel = dwEncLevel * 10 / 100
        _cEncLevel = (dwEncLevel * 10) / 100;
        if( !_cEncLevel )
            ++_cEncLevel;
    }
    else
        hr = E_INVALIDARG;

    PerfDbgLog1(
        tagDft, this, "-CStdZFilter::SetEncodingLevel(hr:%1x)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::InitFilter
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:    NOERROR or E_FAIL (dll init failed)
//
//  History:    06-27-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CStdZFilter::InitFilter()
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::InitFilter");
    HRESULT hr = NOERROR;


	// init the compressor
	if (FAILED(InitCompression()))
        hr = E_FAIL;

    // init the decompressor
    if (FAILED(InitDecompression()))
        hr = E_FAIL;

    PerfDbgLog(tagDft, this, "-CStdZFilter::InitFilter");
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::CStdZFilter
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    06-26-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CStdZFilter::CStdZFilter(ULONG ulSchema)
    : _CRefs()
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::CStdZFilter");
    _pEncodeCtx = NULL;
    _pDecodeCtx = NULL;

    // accepts the number between 1[fastest] - 10[most compres]
    // the formula is _cEncLevel/10 = 50/100 
    _cEncLevel= 5; 

    _ulSchema = ulSchema;

    PerfDbgLog(tagDft, this, "-CStdZFilter::CStdZFilter");
}

//+---------------------------------------------------------------------------
//
//  Method:     CStdZFilter::~CStdZFilter
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    06-26-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CStdZFilter::~CStdZFilter()
{
    PerfDbgLog(tagDft, this, "+CStdZFilter::~CStdZFilter");
    if( _pEncodeCtx )
        DestroyCompression(_pEncodeCtx);
    if( _pDecodeCtx )
        DestroyDecompression(_pDecodeCtx);
    PerfDbgLog(tagDft, this, "-CStdZFilter::~CStdZFilter");
}


//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::QueryInterface
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CEncodingFilterFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagDft, this, "+CEncodingFilterFactory::QueryInterface");
    VDATEPTROUT(ppvObj, void*);
    VDATETHIS(this);

    HRESULT hr = NOERROR;
    if( riid == IID_IUnknown ||
        riid == IID_IEncodingFilterFactory )
    {
        *ppvObj = this;
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    PerfDbgLog1(
        tagDft, this, "-CEncodingFilterFactory::QueryInterface(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::AddRef
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) 
CEncodingFilterFactory::AddRef()
{
    PerfDbgLog(tagDft, this, "+CEncodingFilterFactory::AddRef");
    LONG lRet = ++_CRefs;
    PerfDbgLog1( 
        tagDft, this, "-CEncodingFilterFactory::AddRef(cRef:%1d)", lRet);
    return lRet;
} 

//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::Release
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) 
CEncodingFilterFactory::Release()
{
    PerfDbgLog(tagDft, this, "+CEncodingFilterFactory::Release");
    LONG lRet = --_CRefs;
    if(!lRet)
        delete this;

    PerfDbgLog1( 
        tagDft, this, "-CEncodingFilterFactory::Release(cRef:%1d)", lRet);
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::FindBestFilter
//
//  Synopsis:
//
//  Arguments:  [grfETYPEIn]  - input encoding type
//              [grfETYPEOut] - output encoding type
//              [info]        - DATAINFO (what the data looks like)
//              [ppDF]        - output DataFilter pointer  
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      currently it calls FindDefaultFilter()
//
//----------------------------------------------------------------------------
STDMETHODIMP
CEncodingFilterFactory::FindBestFilter(
        LPCWSTR         pwzCodeIn,
        LPCWSTR         pwzCodeOut,
        DATAINFO        info,
        IDataFilter**   ppDF 
) 
{
    PerfDbgLog(tagDft, this, "+CEncodingFilterFactory::FindBestFilter");
    HRESULT hr = NOERROR;
    hr = GetDefaultFilter(pwzCodeIn, pwzCodeOut, ppDF);
    PerfDbgLog1(
        tagDft, this, "-CEncodingFilterFactory::FindBestFilter(hr:%1x)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::GetDefaultFilter
//
//  Synopsis:
//
//  Arguments:  [tIn]       - input encoding type
//              [tOut]      - output encoding type
//              [ppDF]      - output DataFilter pointer  
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
STDMETHODIMP
CEncodingFilterFactory::GetDefaultFilter(
        LPCWSTR         pwzCodeIn,
        LPCWSTR         pwzCodeOut,
        IDataFilter**   ppDF
)
{
    PerfDbgLog(tagDft, this, "+CEncodingFilterFactory::GetDefaultFilter");
    VDATEPTROUT(ppDF, void*);
    *ppDF = NULL;

    HRESULT hr = E_FAIL;
    BOOL fBuiltIn = FALSE;

    if( (!_wcsicmp(pwzCodeIn, L"gzip") && !_wcsicmp(pwzCodeOut, L"text")) ||  
        (!_wcsicmp(pwzCodeIn, L"text") && !_wcsicmp(pwzCodeOut, L"gzip") ) ) 
    {
        // GZIP 
        *ppDF = new CStdZFilter(COMPRESSION_FLAG_GZIP); 
        fBuiltIn = TRUE;
        if( !(*ppDF) )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else 
    if((!_wcsicmp(pwzCodeIn, L"deflate") && !_wcsicmp(pwzCodeOut, L"text"))||        (!_wcsicmp(pwzCodeIn, L"text") && !_wcsicmp(pwzCodeOut, L"deflate")) ) 
    {
        // deflate
        *ppDF = new CStdZFilter(COMPRESSION_FLAG_DEFLATE); 
        fBuiltIn = TRUE;
        if( !(*ppDF) )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // lookup reg to find out the class
        CLSID   clsid = CLSID_NULL;
        hr = LookupClsIDFromReg(pwzCodeIn, pwzCodeOut, &clsid, 0);
        if( hr == NOERROR )
        {   // Get Clsid
            IClassFactory*  pCF = NULL;
            hr = CoGetClassObject(
                clsid, CLSCTX_INPROC_SERVER, NULL, 
                IID_IClassFactory, (void**)&pCF );

            if( hr == NOERROR )
            {   // Get Class Fac
                IUnknown*   pUnk = NULL;
                hr = pCF->CreateInstance(NULL, IID_IUnknown, (void**)&pUnk);

                if( hr == NOERROR )
                {   // Created Instance
                    hr = pUnk->QueryInterface(IID_IDataFilter, (void**)ppDF); 

                    pUnk->Release();
                }   // Created Instance
            
                pCF->Release();

            }   // Get Class Fac
        }   // Get Clsid
    }   // lookup reg to find out the class

    // only do this for built-in filter (this is not universal)
    if( (*ppDF) && fBuiltIn )
    {
        hr = ((CStdZFilter*)(*ppDF))->InitFilter();
    }

    PerfDbgLog1(
        tagDft, this, "-CEncodingFilterFactory::GetDefaultFilter(hr:%1x)",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::LookupClsIDFromReg
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    08-20-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
STDMETHODIMP
CEncodingFilterFactory::LookupClsIDFromReg(
        LPCWSTR         pwzCodeIn, 
        LPCWSTR         pwzCodeOut, 
        CLSID*          pclsid,
        DWORD           dwFlags
)
{
    HRESULT hr = E_FAIL;

    CHAR szCodeIn[ULPROTOCOLLEN];
    CHAR szCodeOut[ULPROTOCOLLEN];
    CHAR szCodeKey[2*ULPROTOCOLLEN+1];
    DWORD dwLen = 256;
    CHAR szDFilterKey[MAX_PATH];
    BOOL fFoundKey = FALSE;
    HKEY hDFKey = NULL;
    DWORD dwType;

    W2A(pwzCodeIn, szCodeIn, ULPROTOCOLLEN);
    W2A(pwzCodeOut, szCodeOut, ULPROTOCOLLEN);

    // check buffer overrun
    if( (strlen(szCodeIn) + strlen(szCodeOut) + strlen(szDFilterKey) + 2)
        <= MAX_PATH )
    {
    
        // try key#1
        strcpy(szDFilterKey, SZDFILTERROOT);
        strcat(szDFilterKey, szCodeIn);
        strcat(szDFilterKey, "#");
        strcat(szDFilterKey, szCodeOut);

        if( RegOpenKeyEx(
            HKEY_CLASSES_ROOT, szDFilterKey, 0, 
            KEY_QUERY_VALUE, &hDFKey ) == ERROR_SUCCESS)
        {
            if( RegQueryValueEx(
                hDFKey, SZCLASS, NULL, &dwType, (LPBYTE)szDFilterKey, &dwLen
                ) == ERROR_SUCCESS
            )
            {
                hr = CLSIDFromStringA(szDFilterKey, pclsid);
                fFoundKey = TRUE;
            }

            RegCloseKey(hDFKey);
        }


        // try key#2
        if( !fFoundKey )
        {
            strcpy(szDFilterKey, SZDFILTERROOT);
            strcat(szDFilterKey, szCodeOut);
            strcat(szDFilterKey, "#");
            strcat(szDFilterKey, szCodeIn);

            if( RegOpenKeyEx(
                HKEY_CLASSES_ROOT, szDFilterKey, 0, 
                KEY_QUERY_VALUE, &hDFKey ) == ERROR_SUCCESS )
            {
                if( RegQueryValueEx(
                        hDFKey, SZCLASS, NULL, &dwType, 
                        (LPBYTE)szDFilterKey, &dwLen
                    ) == ERROR_SUCCESS
                )
                {
                    hr = CLSIDFromStringA(szDFilterKey, pclsid);
                }

                RegCloseKey(hDFKey);
            }
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::CEncodingFilterFactory
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
CEncodingFilterFactory::CEncodingFilterFactory()
    : _CRefs()
{
    PerfDbgLog(
        tagDft, this, "+CEncodingFilterFactory::CEncodingFilterFactory");

    PerfDbgLog(
        tagDft, this, "-CEncodingFilterFactory::CEncodingFilterFactory");
}

//+---------------------------------------------------------------------------
//
//  Method:     CEncodingFilterFactory::~CEncodingFilterFactory
//
//  Synopsis:
//
//  Arguments:  
//
//  Returns:
//
//  History:    04-29-97   DanpoZ (Danpo Zhang)   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------
CEncodingFilterFactory::~CEncodingFilterFactory()
{
    PerfDbgLog(
        tagDft, this, "+CEncodingFilterFactory::~CEncodingFilterFactory");

    PerfDbgLog(
        tagDft, this, "-CEncodingFilterFactory::~CEncodingFilterFactory");
}

