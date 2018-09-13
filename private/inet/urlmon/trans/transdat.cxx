//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       transdata.cxx
//
//  Contents:   Contains the module which provide data passed on in OnDataAvailable
//
//  Classes:
//
//  Functions:
//
//  History:    12-07-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#ifndef unix
#include "..\stg\rostmdir.hxx"
#include "..\stg\rostmfil.hxx"
#else
#include "../stg/rostmdir.hxx"
#include "../stg/rostmfil.hxx"
#endif /* unix */

PerfDbgTag(tagCTransData,    "Urlmon", "Log CTransData",        DEB_DATA);
    DbgTag(tagCTransDataErr, "Urlmon", "Log CTransData Errors", DEB_DATA|DEB_ERROR);

HRESULT FindMediaType(LPCSTR pszType, CLIPFORMAT *cfType);
HRESULT FindMediaTypeW(LPCWSTR pwzType, CLIPFORMAT *cfType);

static LPSTR g_szCF_NULL = "*/*";

char  szContent[]           = "Content Type";
char  szClassID[]           = "CLSID";
char  szFlags[]             = "Flags";
char  szExtension[]         = "Extension";
char  szMimeKey[]           = "MIME\\Database\\Content Type\\";
const ULONG ulMimeKeyLen    = ((sizeof(szMimeKey)/sizeof(char))-1);

// The byte combination that identifies that a file is a storage of
// some kind
const BYTE SIGSTG[] = {0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1};
const BYTE CBSIGSTG = sizeof(SIGSTG);


#define CBSNIFFDATA_MAX 256

#ifdef DBG
char *szDataSinkName[] =
{
     "Unknown"
    ,"StreamNoCopyData"
    ,"File"
    ,"Storage"
    ,"StreamOnFile"
    ,"StreamBindToObject"
    ,"GenericStream"
};

#define GetDataSinkName(ds) szDataSinkName[ds]

#else

#define GetDataSinkName(ds) ""

#endif


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::CTransData
//
//  Synopsis:
//
//  Arguments:  [pTrans] --
//              [DWORD] --
//              [dwSizeBuffer] --
//              [fBindToObject] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransData::CTransData(CTransaction *pTrans, LPBYTE pByte,DWORD dwSizeBuffer, BOOL fBindToObject) : _CRefs()
{
    _TransDataState = TransData_Initialized;
    _wzMime[0] = '\0';
    _wzFileName[0]= 0;
    _pwzUrl = 0;
    _pStgMed = 0;
    _lpBuffer = pByte;
    _cbBufferSize = dwSizeBuffer;
    _cbDataSize = 0;
    _cbTotalBytesRead = 0;
    _cbReadReturn = 0;
    _cbBufferFilled = 0;
    _cbDataSniffMin = DATASNIFSIZE_MIN;
    _pEnumFE = 0;
    _pStgMed = NULL;
    _pProt = NULL;

    _fBindToObject = fBindToObject;
    _fMimeTypeVerified = TRUE;
    _fDocFile = FALSE;
    _fInitialized = FALSE;
    _fRemoteReady = FALSE;
    _fCache = FALSE;
    _fLocked = FALSE;
    _fFileAsStmOnFile = FALSE;
    _fEOFOnSwitchSink = FALSE; 

    _hFile = NULL;
    _cbBytesReported = 0;
    _dwAttached = 0;
    _grfBindF  = 0;
    _grfBSC = 0;

}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::~CTransData
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CTransData::~CTransData()
{
    PerfDbgLog(tagCTransData, this, "+CTransData::~CTransData");

    if (_pwzUrl)
    {
        delete [] _pwzUrl;
    }
    
    if (_pProt)
    {
        if (_fLocked)
        {
            _pProt->UnlockRequest();
        }
        _pProt->Release();
        _pProt = NULL;
    }

    if (_pBndCtx)
    {
        _pBndCtx->Release();
    }

    if (_pStgMed)
    {
        TransAssert((   (_pStgMed->pUnkForRelease == NULL)
                     || (_pStgMed->pUnkForRelease == this)));

        if (_pStgMed->tymed == TYMED_ISTREAM)
        {
            _pStgMed->pstm->Release();
        }
        else if (_pStgMed->tymed == TYMED_FILE)
        {
            if (_pStgMed->lpszFileName)
            {
                delete _pStgMed->lpszFileName;
            }
        }

        DbgLog1(tagCTransData, this, "=== CTransData::~CTransData: (pStgMed:%lx)", _pStgMed);
        delete _pStgMed;
        _pStgMed = NULL;
    }

    if (_hFile)
    {
        DbgLog1(tagCTransData, this, "=== CTransData::~CTransData (CloseHandle(%lx)", _hFile);
        CloseHandle(_hFile);
        _hFile = NULL;
    }

    if (_lpBuffer)
    {
        delete _lpBuffer;
    }

    PerfDbgLog(tagCTransData, this, "-CTransData::~CTransData");
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCTransData, this, "+CTransData::QueryInterface");

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_ITransactionData))
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTransData::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTransData::AddRef(void)
{
    LONG lRet = ++_CRefs;
    PerfDbgLog1(tagCTransData, this, "CTransData::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTransData::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CTransData::Release(void)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::Release");

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetTransactionData
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [ppwzFilename] --
//              [ppwzMime] --
//              [pdwSizeTotal] --
//              [pdwSizeAvailable] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    9-09-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::GetTransactionData(LPCWSTR pwzUrl, LPOLESTR *ppwzFilename, LPOLESTR *ppwzMime,
                                            DWORD *pdwSizeTotal, DWORD *pdwSizeAvailable, DWORD  dwReserved)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::GetTransactionData");
    HRESULT hr = NOERROR;
    TransAssert((pwzUrl && ppwzFilename && ppwzMime && pdwSizeTotal && pdwSizeAvailable ));
    TransAssert((_wzFileName[0] != 0));


    if  (ppwzFilename && ppwzMime && pdwSizeTotal && pdwSizeAvailable)
    {
        LPWSTR pwzUrlLocal = GetUrl();
        TransAssert((pwzUrlLocal));

        DbgLog2(tagCTransData, this, "=== CTransData::GetTransactionData (pwzUrlLocal:%ws, pwzUrl:%ws)", pwzUrlLocal, pwzUrl);

        if (!wcscmp(pwzUrl, _pwzUrl))
        {
            *ppwzFilename = OLESTRDuplicate(_wzFileName);

            *ppwzMime = OLESTRDuplicate(_wzMime);

            if (_cbDataSize)
            {
                *pdwSizeTotal  = _cbDataSize;
            }
            else
            {
                *pdwSizeTotal  = _cbTotalBytesRead;
            }
            *pdwSizeAvailable = _cbTotalBytesRead;

        }
        else
        {
            *ppwzFilename = 0;
            *ppwzMime = NULL;
            *pdwSizeTotal = 0;
            *pdwSizeAvailable = 0;
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog5(tagCTransData, this, "-CTransData::GetTransactionData (hr:%lx, Filename:%ws, Mime:%ws, _cbDataSize:%ld, _cbTotalBytesRead:%ld)",
        hr, XDBG(*ppwzFilename,L""), XDBG(*ppwzMime,L""), _cbDataSize, _cbTotalBytesRead);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::Create
//
//  Synopsis:   Set up the transaction data object.
//
//  Arguments:  [pTrans] -- pointer to transaction
//              [riid] --   riid the users passed in
//              [ppCTD] --  the transdata object passed back
//
//  Returns:
//
//  History:    1-18-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::Create(LPCWSTR pwUrl, DWORD grfBindF, REFIID riid, IBindCtx *pBndCtx,
                                BOOL fBindToObject, CTransData **ppCTD)
{
    HRESULT hr = NOERROR;
    PerfDbgLog1(tagCTransData, NULL, "+CTransData::Create(fBindToObject:%d)", fBindToObject);
    CTransData *pCTData;

    LPBYTE   lpBuffer;
    ULONG    cbBufferSize;
    cbBufferSize = DNLD_BUFFER_SIZE;

    TransAssert((DATASNIFSIZEDOCFILE_MIN <= cbBufferSize));
    lpBuffer = (LPBYTE) new BYTE[cbBufferSize];
    pCTData = new CTransData(NULL,lpBuffer,cbBufferSize, fBindToObject);

    if (lpBuffer && pCTData)
    {
        *ppCTD = pCTData;
        pCTData->Initialize(pwUrl, grfBindF, riid, pBndCtx);

        // Try to get an IEnumFORMATETC pointer from the bind context
        //hr = GetObjectParam(pbc, REG_ENUMFORMATETC, IID_IEnumFORMATETC, (IUnknown**)&_pEnumFE);
    }
    else
    {
        if (pCTData)
        {
            delete pCTData;
        }
        else if (lpBuffer)
        {
            delete lpBuffer;
        }

        hr = E_OUTOFMEMORY;
        *ppCTD = 0;
    }

    PerfDbgLog2(tagCTransData, NULL, "-CTransData::Create (out:%lx,hr:%lx)", *ppCTD, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::Initialize
//
//  Synopsis:
//
//  Arguments:  [riid] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::Initialize(LPCWSTR pwzUrl, DWORD grfBindF, REFIID riid, IBindCtx *pBndCtx)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::Initialize");
    HRESULT hr = NOERROR;

    if (_fInitialized == FALSE)
    {
        _ds = DataSink_Unknown;
        _formatetc.tymed = TYMED_NULL;
        _grfBindF = grfBindF;
        _pBndCtx = pBndCtx;
        _pBndCtx->AddRef();
        
        _pwzUrl = OLESTRDuplicate((LPWSTR)pwzUrl);
        if (!_pwzUrl) 
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            if (_fBindToObject)
            {
                _formatetc.tymed = TYMED_ISTREAM;
                _ds = DataSink_Unknown;
            }
            else if (riid == IID_IUnknown)
            {
                // this is the BindToStorage
                _formatetc.tymed = TYMED_FILE;
                _ds = DataSink_File;
            }
            else if (riid == IID_IStream)
            {
                // We do not know yet which kind of stream
                // SetDataSink will determine this.

                _formatetc.tymed = TYMED_ISTREAM;
                _ds = DataSink_StreamOnFile;
            }
            else if (riid == IID_IStorage)
            {
                BIND_OPTS bindopts;
                bindopts.cbStruct = sizeof(BIND_OPTS);
                hr = pBndCtx->GetBindOptions(&bindopts);
                _grfMode = bindopts.grfMode;

                _formatetc.tymed = TYMED_ISTORAGE;
                _ds = DataSink_Storage;

            }
            else
            {
                // this call should fail
                hr = E_INVALIDARG;
                TransAssert((FALSE && "Unknown data sink for this request!"));
            }
        }
        
        if (SUCCEEDED(hr))
        {
            HRESULT hr1;
            ITransactionData *pCTransData = NULL;
            LPWSTR pwzFilename = NULL;
            LPWSTR pwzMime = NULL;

            hr1 = GetObjectParam(pBndCtx, SZ_TRANSACTIONDATA, IID_ITransactionData, (IUnknown **)&pCTransData);
            DbgLog2(tagCTransData, this, "=== CTransData::Initialize GetObjectParam: pbndctx:%lx, hr:%lx)", pBndCtx, hr1);

            if (SUCCEEDED(hr1))
            {
                TransAssert((pCTransData));
                hr1 = pCTransData->GetTransactionData(_pwzUrl,&pwzFilename, &pwzMime, &_cbDataSize, &_cbTotalBytesRead, 0);
                DbgLog5(tagCTransData, this, "=== CTransData::Initialize GetTransactionData (hr:%lx, Filename:%ws, Mime:%ws, _cbDataSize:%ld, _cbTotalBytesRead:%ld)", hr1, pwzFilename, pwzMime, _cbDataSize, _cbTotalBytesRead);
                pCTransData->Release();
            }


            if (SUCCEEDED(hr1) )
            {
                // set the url filename
                SetFileName(pwzFilename);
                if (pwzMime)
                {
                    SetMimeType(pwzMime);
                    _fMimeTypeVerified = TRUE;
                }
                _fRemoteReady = TRUE;

                if (pwzMime)
                {
                    delete pwzMime;
                }
                if (pwzFilename)
                {
                    delete pwzFilename;
                }
            }
        }
        _fInitialized = TRUE;
    }
    else
    {
        _dwAttached++;
        
        if (_pBndCtx)
        {
            _pBndCtx->Release();
        }

        _pBndCtx = pBndCtx;

        if (_pBndCtx)
        {
            _pBndCtx->AddRef();
        }

        // no set up the righte datasink
        if (riid == IID_IUnknown)
        {
            SwitchDataSink(DataSink_File);
        }
        else if (   (riid == IID_IStream)
                 && (grfBindF & BINDF_NEEDFILE))
        {
            SwitchDataSink(DataSink_StreamOnFile);
        }

    }

    PerfDbgLog3(tagCTransData, this, "-CTransData::Initialize (_formatetc.tymed:%ld, _ds:%lx, hr:%lx)", _formatetc.tymed,_ds, hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::PrepareThreadTransfer
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::PrepareThreadTransfer()
{
    PerfDbgLog(tagCTransData, this, "+CTransData::PrepareThreadTransfer");
    HRESULT hr = NOERROR;

    if (_pBndCtx)
    {
        _pBndCtx->Release();
        _pBndCtx = NULL;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::PrepareThreadTransfer (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetDataSink
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DataSink CTransData::GetDataSink()
{
    PerfDbgLog(tagCTransData, this, "+CTransData::GetDataSink");
    DataSink dsRet = DataSink_Unknown;

    if (_ds == DataSink_Unknown)
    {
        DWORD dwBindF = GetBindFlags();

        if (   (dwBindF & BINDF_ASYNCSTORAGE)
            && (dwBindF & BINDF_PULLDATA)
            && (_formatetc.tymed == TYMED_ISTREAM))
        {
            dsRet = _ds = DataSink_StreamNoCopyData;
        }
    }
    else
    {
        dsRet = _ds;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::GetDataSink (dsRet:%lx)", dsRet);
    return dsRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::SetDataSink
//
//  Synopsis:
//
//  Arguments:  [dwBindF] --
//
//  Returns:
//
//  History:    2-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DataSink CTransData::SetDataSink(DWORD dwBindF)
{
    PerfDbgLog1(tagCTransData, this, "+CTransData::SetDataSink (_ds:%lx)", _ds);

    TransAssert((_formatetc.tymed != TYMED_NULL));
    _grfBindF = dwBindF;

    switch (_ds)
    {
    case DataSink_Unknown:
    {
        if (_fBindToObject)
        {
            TransAssert((_formatetc.tymed == TYMED_ISTREAM));
            _ds = DataSink_StreamBindToObject;
        }
    }
    break;
    case DataSink_File:
    {
        TransAssert((_formatetc.tymed == TYMED_FILE));

    }
    break;
    case DataSink_StreamOnFile:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        if ((dwBindF & BINDF_ASYNCSTORAGE) && (dwBindF & BINDF_PULLDATA))  
        {
            _ds = DataSink_StreamNoCopyData;
        }
    }
    break;

    case DataSink_StreamBindToObject:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        TransAssert((_fBindToObject == TRUE));

        if (IsObjectReady() == NOERROR)
        {
            // 
            // change it to file - stream on file will be opened
            //
            _ds = DataSink_StreamOnFile;
        }
        else if ((dwBindF & BINDF_ASYNCSTORAGE) && (dwBindF & BINDF_PULLDATA))
        {
            _ds = DataSink_StreamNoCopyData;
        }

    }
    break;

    case DataSink_Storage:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTORAGE));
    }
    break;

    case DataSink_GenericStream:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
    }
    break;

    default:
        TransAssert((FALSE && "CTransData::SetDataSink -- Invalid data location"));
    break;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::SetDataSink (_ds:%lx)", _ds);
    return _ds;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::SwitchDataSink
//
//  Synopsis:
//
//  Arguments:  [dwBindF] --
//
//  Returns:
//
//  History:    2-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DataSink CTransData::SwitchDataSink(DataSink dsNew)
{
    PerfDbgLog2(tagCTransData, this, "+CTransData::SwitchDataSink (_ds:%lx, dsNew:%lx)", _ds, dsNew);

    TransAssert((_ds != DataSink_Unknown));
    TransAssert((_formatetc.tymed != TYMED_NULL));
    HRESULT hr = NOERROR;

    switch (_ds)
    {
    case DataSink_File:
    {
        TransAssert((_formatetc.tymed == TYMED_FILE));
    }
    break;
    case DataSink_StreamOnFile:
    {
         TransAssert((_wzFileName[0] != 0));
         _formatetc.tymed = TYMED_ISTREAM;
    }
    break;

    case DataSink_GenericStream:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
    }
    break;

    case DataSink_StreamNoCopyData:
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        if (   dsNew == DataSink_StreamOnFile
            || dsNew == DataSink_GenericStream)
        {
            _ds = dsNew;
            DWORD dwNew = 0;
            hr = OnDataReceived(_grfBSC, 0, 0, &dwNew);
            if( hr == S_FALSE)
            {
                _fEOFOnSwitchSink = TRUE;
            }
        }

    break;

    case DataSink_StreamBindToObject:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        TransAssert((_fBindToObject == TRUE));
        _ds = dsNew;
        if (_ds == DataSink_File)
        {
            _formatetc.tymed = TYMED_FILE;
            DWORD dwNew = 0;
            hr = OnDataReceived(_grfBSC, 0, 0, &dwNew);
            if( hr == S_FALSE)
            {
                _fEOFOnSwitchSink = TRUE;
            }
        }

    }
    break;

    case DataSink_Storage:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTORAGE));
    }
    break;

    default:
        TransAssert((FALSE && "CTransData::SwitchDataSink -- Invalid data location"));
    break;
    }

    PerfDbgLog2(tagCTransData, this, "-CTransData::SwitchDataSink (_ds:%lx, dsNew:%lx)", _ds, dsNew);
    return _ds;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::IsFileRequired()
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CTransData::IsFileRequired()
{
    PerfDbgLog(tagCTransData, this, "+CTransData::IsFileRequired()");
    BOOL fRet = FALSE;
    TransAssert((_ds != DataSink_Unknown));

    switch (_ds)
    {
    case DataSink_File:
    case DataSink_StreamOnFile:
        fRet = TRUE;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::IsFileRequired() (fRet:%d)", fRet);
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetData
//
//  Synopsis:
//
//  Arguments:  [ppformatetc] --
//              [ppStgMed] --
//              [grfBSCF] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::GetData(FORMATETC **ppformatetc, STGMEDIUM **ppStgMed, DWORD grfBSCF)
{
    PerfDbgLog1(tagCTransData, this, "+CTransData::GetData (_ds:%ld)", _ds);
    HRESULT hr = INET_E_DATA_NOT_AVAILABLE;
    BOOL fNewStgMed = FALSE;
    DataSink ds;

    *ppStgMed = 0;
    *ppformatetc = 0;


    if (_pStgMed == NULL)
    {
        // first find the formatETC based on
        // clipformat and the EnumFormatETC
        FindFormatETC();

        _pStgMed = new STGMEDIUM;
        if (_pStgMed == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto End;
        }

        *ppformatetc = &_formatetc;

        _pStgMed->tymed = TYMED_NULL;
        _pStgMed->hGlobal = NULL;
        _pStgMed->pUnkForRelease = 0;
        _pStgMed->pstm = NULL;
        fNewStgMed = TRUE;

    }

    ds = _ds;
    if( _fFileAsStmOnFile && _ds == DataSink_File )
    {
        ds = DataSink_StreamOnFile;
    } 
    
    switch (ds)
    {
    case DataSink_File:
    {
        //TransAssert((   (grfBSCF & ~BSCF_LASTDATANOTIFICATION)
        //             || ((grfBSCF & BSCF_LASTDATANOTIFICATION) && (_formatetc.tymed == TYMED_FILE)) ));

        if (_wzFileName[0] != 0)
        {
            TransAssert((_wzFileName[0] != 0));
            _pStgMed->tymed = TYMED_FILE;

            if (_pStgMed->lpszFileName == NULL)
            {
                _pStgMed->lpszFileName = (LPWSTR) new WCHAR [wcslen(_wzFileName)+2];
                if (_pStgMed->lpszFileName)
                {
                    wcscpy(_pStgMed->lpszFileName, _wzFileName);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    goto End;
                }
            }

            if (_pStgMed->pUnkForRelease == NULL)
            {
                _pStgMed->pUnkForRelease = this;
            }

            hr = NOERROR;
            DbgLog1(tagCTransData, this, "+CTransData::GetData -> TYMED_FILE: %ws", _wzFileName);
        }
        else
        {
            // filename is not available yet
            hr = INET_E_DATA_NOT_AVAILABLE;
        }
    }
    break;

    case DataSink_GenericStream:
    case DataSink_StreamBindToObject:
    case DataSink_StreamNoCopyData:
    {
        DbgLog1(tagCTransData, this, "=== CTransData::GetData (_ds:%lx)", _ds);
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));

        if (_pStgMed->tymed == TYMED_NULL)
        {
            CReadOnlyStreamDirect *pCRoStm = new CReadOnlyStreamDirect( this, _grfBindF);

            if (pCRoStm)
            {
                _pStgMed->tymed = TYMED_ISTREAM;
                _pStgMed->pstm = pCRoStm;
                _pStgMed->pUnkForRelease = this;
                hr = NOERROR;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            DbgLog1(tagCTransData, this, "+CTransData::GetData -> TYMED_ISTREAM: %lx", pCRoStm);
        }
        else
        {
            hr = NOERROR;
        }

        DbgLog2(tagCTransData, this, "=== CTransData::GetData (_ds:%lx,pstm:%lx)", _ds,_pStgMed->pstm);
        DbgLog2(tagCTransData, this, "=== (_cbBufferFilled:%ld, _cbBufferSize:%ld)", _cbBufferFilled,_cbBufferSize);

    }
    break;

    case DataSink_StreamOnFile:
    {
        DbgLog1(tagCTransData, this, "=== CTransData::GetData (_ds:%lx)", _ds);
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));

        if (_wzFileName[0] != 0)
        {
            if (_pStgMed->tymed == TYMED_NULL)
            {
                if( _pStgMed->pstm )
                {
                    _pStgMed->tymed = TYMED_ISTREAM;
                    _pStgMed->pUnkForRelease = this;

                    // When pStm gets created, the ctor set the ref count
                    // to 1, so no need any additional AddRef.
                    //
                    // Here we are not create a new pStm, we need to do an
                    // AddRef on the pStm because 
                    // CBinding::OnDataNotification (the only caller of this)
                    // will call ReleaseStgMedia() which will call 
                    // _pStgMeg->pstm->Release()

                    _pStgMed->pstm->AddRef();

                    hr = NOERROR;
                }
                else
                {
                    char szTempFile[MAX_PATH];
                    CReadOnlyStreamFile *pCRoStm = NULL;
                    W2A(_wzFileName, szTempFile, MAX_PATH);

                    hr = CReadOnlyStreamFile::Create(szTempFile, &pCRoStm);

                    if (pCRoStm)
                    {
                        _pStgMed->tymed = TYMED_ISTREAM;
                        _pStgMed->pstm = pCRoStm;
                        _pStgMed->pUnkForRelease = this;
                        hr = NOERROR;
                    }
                    else
                    {
                        // filename is not available yet
                        hr = INET_E_DATA_NOT_AVAILABLE;
                    }

                    DbgLog2(tagCTransData, this, "+CTransData::GetData -> TYMED_ISTREAM: %lx (hr:%lx)", pCRoStm,hr);
                }

            }
            else
            {
                TransAssert((_pStgMed->tymed == TYMED_ISTREAM));
                hr = NOERROR;
            }
        }
        else
        {
            DbgLog(tagCTransDataErr, this, "+CTransData::GetData ->StreamOnFile: no filename!");

            // filename is not available yet
            hr = INET_E_DATA_NOT_AVAILABLE;
        }

    }
    break;

    case DataSink_Storage:
    {
        DbgLog2(tagCTransData, this, "=== CTransData::GetData (_ds:%lx, _wzFileName:%ws)", _ds, _wzFileName);
        TransAssert((_formatetc.tymed == TYMED_ISTORAGE));

        TransAssert((_wzFileName != NULL));

        if (_wzFileName[0] != 0)
        {
            if (_pStgMed->tymed == TYMED_NULL)
            {
                IStorage *pStg = NULL;

                hr = StgOpenStorage(_wzFileName, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &pStg );
                DbgLog1(tagCTransData, this, "+CTransData::GetData -> TYMED_ISTORAGE: %ws", _wzFileName);

                if (pStg)
                {
                    _pStgMed->tymed = TYMED_ISTORAGE;
                    _pStgMed->pstg = pStg;
                    _pStgMed->pUnkForRelease = this;
                    hr = NOERROR;
                }

                DbgLog2(tagCTransData, this, "+CTransData::GetData -> TYMED_ISTORAGE: %lx (hr:%lx)", pStg,hr);
            }
            else
            {
                TransAssert((_pStgMed->tymed == TYMED_ISTORAGE));
                hr = NOERROR;
            }
        }
        // else return default error
    }
    break;

    default:
        // this needs to be implemented
        TransAssert((FALSE && "CTransData::GetData -- Invalid data location"));
    break;
    }

    if (SUCCEEDED(hr) && _pStgMed)
    {
        // this object was addref on punkforrelease
        *ppStgMed = _pStgMed;
        *ppformatetc = &_formatetc;

        // Use ourselves as the unknown for release, so that the temp file
        // doesn't get removed when ReleaseStgMedium() is called.  We need
        // to AddRef ourselves to balance the Release that will occur.

        if (_pStgMed->pUnkForRelease)
        {
            AddRef();
            if (   _pProt 
                && fNewStgMed
                && SUCCEEDED(_pProt->LockRequest(0)) )
            {
                    _fLocked = TRUE;
            }
        }

        hr = NOERROR;

        TransAssert((_pStgMed->pUnkForRelease != NULL));
    }

End:
    PerfDbgLog1(tagCTransData, this, "-CTransData::GetData (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::ReadHere
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBuffer] --
//              [pdwRead] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::ReadHere(LPBYTE pBuffer, DWORD cbBuffer, DWORD *pdwRead)
{
    PerfDbgLog2(tagCTransData, this, "+CTransData::ReadHere (_ds:%lx,cbBuffer:%ld)", _ds,cbBuffer);
    HRESULT hr = NOERROR;

    switch (_ds)
    {
    case DataSink_StreamNoCopyData:
    case DataSink_StreamBindToObject:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        BOOL fRead = TRUE;
        DWORD dwCopy = 0;
        DWORD dwCopyNew = 0;

        if (_cbBufferFilled)
        {
            fRead = FALSE;

            // copy data form the local buffer to the provide buffer
            if (cbBuffer < _cbBufferFilled)
            {
                dwCopy = cbBuffer;
                memcpy(pBuffer, _lpBuffer, cbBuffer);
                // move the memory to the front
                memcpy(_lpBuffer, _lpBuffer + cbBuffer, _cbBufferFilled - cbBuffer);
                _cbBufferFilled -= cbBuffer;
                hr = S_OK;
            }
            else if (cbBuffer == _cbBufferFilled)
            {
                dwCopy = _cbBufferFilled;
                memcpy(pBuffer, _lpBuffer, _cbBufferFilled);
                _cbBufferFilled = 0;
                hr = S_OK;
            }
            else
            {
                //
                // user buffer is greater than what is available in
                //
                dwCopy = _cbBufferFilled;
                memcpy(pBuffer, _lpBuffer, _cbBufferFilled);
                _cbBufferFilled = 0;
                fRead = TRUE;
                hr = E_PENDING;
            }
        }

        // now read from the wire
        if ((_cbBufferFilled == 0) && (fRead == TRUE))
        {
            // read data from our buffer
            if (_TransDataState == TransData_ProtoTerminated)
            {
                // download completed
                hr = (dwCopy) ? S_OK : S_FALSE;
            }
            else if (pBuffer && cbBuffer)
            {
                hr = _pProt->Read(pBuffer + dwCopy, cbBuffer - dwCopy, &dwCopyNew);
                _cbTotalBytesRead += dwCopyNew;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }

        if (pdwRead)
        {
            *pdwRead = dwCopy + dwCopyNew;

            if (*pdwRead && (hr != E_PENDING))
            // some data in buffer
            {
                hr = S_OK;
            }
        }
    }
    break;

    case DataSink_StreamOnFile:
    {
        DbgLog(tagCTransData, this, "=== CTransData::ReadHere (_ds:DataSink_StreamNoCopyData)");
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        TransAssert((pdwRead != NULL));

        // read data from our buffer
        if (_TransDataState == TransData_ProtoTerminated)
        {
            // download completed
            hr = S_FALSE;
        }
        else if (pBuffer && cbBuffer)
        {
            hr = _pProt->Read(pBuffer, cbBuffer, pdwRead);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    break;

    case DataSink_File:
    {
        TransAssert((_formatetc.tymed == TYMED_FILE));

    }
    break;

    default:
        // this needs to be implemented
        hr = E_FAIL;
        TransAssert((FALSE && "CTransData::ReadHere -- Invalid data location"));
    break;
    }

    TransAssert((   (   (hr == S_FALSE && *pdwRead == 0)
                     || (hr == S_OK && *pdwRead != 0)
                     || (hr == E_PENDING)
                     || (hr == E_INVALIDARG)
                     || (hr == INET_E_DATA_NOT_AVAILABLE)
                     || (hr == INET_E_DOWNLOAD_FAILURE)
                    )
                 && "CTransData::ReadHere -- Invalid return code"));


    PerfDbgLog3(tagCTransData, this, "-CTransData::ReadHere (hr:%lx,cbBuffer:%ld,pdwRead:%ld)", hr,cbBuffer,*pdwRead);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-30-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::Seek");
    HRESULT hr;

    hr = _pProt->Seek(dlibMove, dwOrigin,plibNewPosition);

    PerfDbgLog1(tagCTransData, this, "-CTransData::Seek (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetReadBuffer
//
//  Synopsis:
//
//  Arguments:  [ppBuffer] --
//              [pcbBytes] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::GetReadBuffer(BYTE **ppBuffer, DWORD *pcbBytes)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::GetReadBuffer");
    HRESULT hr;

    TransAssert(((_cbBufferSize - _cbBufferFilled) >= 0));

    *ppBuffer = _lpBuffer + _cbBufferFilled;
    *pcbBytes = _cbBufferSize - _cbBufferFilled;

    hr = ((_cbBufferSize - _cbBufferFilled) > 0) ? NOERROR : E_FAIL;

    PerfDbgLog3(tagCTransData, this, "-CTransData::GetReadBuffer (pBuffer:%lx,size:%ld,hr:%lx)",
        *ppBuffer, *pcbBytes, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::OnDataInBuffer
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBytesAvailable] --
//              [dwBytesTotal] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::OnDataInBuffer(BYTE *pBuffer, DWORD cbBytesRead, DWORD dwBytesTotal)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::OnDataInBuffer");
    HRESULT hr = NOERROR;

    _cbTotalBytesRead += cbBytesRead;
    _cbBufferFilled += cbBytesRead;

    PerfDbgLog1(tagCTransData, this, "-CTransData::OnDataInBuffer (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::OnDataReceived
//
//  Synopsis:
//
//  Arguments:  [cbBytesAvailable] --
//              [dwBytesTotalRead] --
//              [dwTotalSize] --
//
//  Returns:
//
//  History:    2-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::OnDataReceived(DWORD grfBSC, DWORD cbBytesAvailable, DWORD dwTotalSize, DWORD *pcbNewAvailable)
{
    PerfDbgLog4(tagCTransData, this, "+CTransData::OnDataReceived (_ds:%s, grfBSC:%lx,  cbBytesAvailable:%ld, _cbTotalBytesRead:%ld)",
        GetDataSinkName(_ds), grfBSC, cbBytesAvailable, _cbTotalBytesRead);
    HRESULT hr = NOERROR;

    *pcbNewAvailable = cbBytesAvailable;
    _grfBSC |= grfBSC;

    switch (_ds)
    {
    case DataSink_StreamNoCopyData:
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));

    case DataSink_StreamBindToObject:
    {
        DWORD dwNewData = 0;
        TransAssert((_pProt && _cbDataSniffMin));

        // _cbTotalBytesRead = # of bytes read so far
        if (_cbTotalBytesRead < _cbDataSniffMin)
        {
            // no bytes read so far
            TransAssert((_cbTotalBytesRead < _cbDataSniffMin));
            // read data into buffer and report progess
            hr = _pProt->Read(_lpBuffer + _cbBufferFilled, _cbBufferSize - _cbBufferFilled, &dwNewData);
            _cbTotalBytesRead += dwNewData;
            _cbBufferFilled += dwNewData;

            // now check if this is docfile
            // if so download at least 2k
            if (!_fDocFile && _cbBufferFilled && (IsDocFile(_lpBuffer, _cbBufferFilled) == S_OK))
            {
                _fDocFile = TRUE;
                _cbDataSniffMin =  (dwTotalSize && dwTotalSize < DATASNIFSIZEDOCFILE_MIN) ? dwTotalSize : DATASNIFSIZEDOCFILE_MIN;
            }

            if ((hr == E_PENDING) && (_cbTotalBytesRead < _cbDataSniffMin))
            {
                // do not report anything - wait until we get more data
                // a request is pending at this time
                // need more data to sniff properly
                hr  = S_NEEDMOREDATA;
            }
            else if (hr == NOERROR || hr == E_PENDING)
            {
                TransAssert((_cbTotalBytesRead != 0));

                // report the data we have in the buffer or
                // the available #
                DWORD cbBytesReport =  (cbBytesAvailable > _cbTotalBytesRead) ? cbBytesAvailable : _cbTotalBytesRead + 1;

                if (dwTotalSize && ((cbBytesReport > dwTotalSize)))
                {
                    cbBytesReport =  dwTotalSize;
                }

               *pcbNewAvailable = cbBytesReport;
            }
        }
    }

    break;


    case DataSink_File:
    case DataSink_Storage:
    case DataSink_StreamOnFile:
    {
        DWORD dwNewData = 0;

        TransAssert((_pProt));

        if (_cbTotalBytesRead < _cbDataSniffMin)
        {
            _cbDataSniffMin =  (dwTotalSize && dwTotalSize < DATASNIFSIZEDOCFILE_MIN) ? dwTotalSize : DATASNIFSIZEDOCFILE_MIN;

            // read data into buffer and report progess
            hr = _pProt->Read(_lpBuffer + _cbBufferFilled, _cbBufferSize - _cbBufferFilled, &dwNewData);

            _cbTotalBytesRead += dwNewData;
            _cbBufferFilled += dwNewData;

            if ((hr == E_PENDING) && (_cbTotalBytesRead < _cbDataSniffMin))
            {
                // do not report anything - wait until we get more data
                // a request is pending at this time
                // need more data to sniff properly
                hr  = S_NEEDMOREDATA;
            }
            else if (hr == NOERROR || hr == E_PENDING)
            {
                TransAssert((_cbTotalBytesRead != 0));
            }
            *pcbNewAvailable = _cbTotalBytesRead;
        }

        // Note: read until pending or eof and report progress
        //       this is important to keep the download going
        if (hr == NOERROR)
        {
            // reset the buffer - don't overwrite sniffing data
            _cbBufferFilled = (_fMimeTypeVerified) ? 0 : _cbDataSniffMin;
            
            // bugbug: need special flag which indicates not to read if fully available
            //if (!(grfBSC & BSCF_DATAFULLYAVAILABLE))
            
            if (1)
            {
                //read as much data until S_OK or E_PENDING or error
                do
                {
                    hr = _pProt->Read(_lpBuffer + _cbBufferFilled, _cbBufferSize - _cbBufferFilled, &dwNewData);
                    _cbTotalBytesRead += dwNewData;

                } while (hr == NOERROR);

                // report available data
                if (hr == NOERROR || hr == E_PENDING)
                {
                    TransAssert((_cbTotalBytesRead != 0));
                }
                *pcbNewAvailable = (cbBytesAvailable > _cbTotalBytesRead) ? _cbTotalBytesRead : _cbTotalBytesRead;
            }
            else
            {
                TransAssert((dwTotalSize == cbBytesAvailable));
                *pcbNewAvailable = dwTotalSize;
            }

        }
    }
    break;

    default:
        TransAssert((FALSE && "CTransData::OnDataReceived -- Invalid data location"));
    break;
    }

    // cbBytesAvailable might be off be 1
    //TransAssert((cbBytesAvailable <= *pcbNewAvailable));

    PerfDbgLog1(tagCTransData, this, "-CTransData::OnDataReceived (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::OnStart
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-23-96   JohannP (Johann Posch)   Created
//
//  Notes:      TransData does not keep CINet alive
//              and will NOT call delete on it!
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::OnStart(IOInetProtocol *pCINet)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCTransData, this, "+CTransData::OnStart");
    TransAssert((pCINet != NULL));

    if (_pProt)
    {
        _pProt->Release();
        _pProt = NULL;
    }

    switch (_ds)
    {
    case DataSink_StreamBindToObject:
    case DataSink_StreamNoCopyData:
    case DataSink_StreamOnFile:
    case DataSink_GenericStream:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        // this point is not addref by CTransData
        TransAssert((_pProt == NULL));
        _pProt = pCINet;
    }
    break;

    case DataSink_Storage:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTORAGE));
        // this point is not addref by CTransData
        TransAssert((_pProt == NULL));
        _pProt = pCINet;
    }
    break;


    case DataSink_File:
    {
        TransAssert((_formatetc.tymed == TYMED_FILE));
        // this point is not addref by CTransData
        TransAssert((_pProt == NULL));
        _pProt = pCINet;
    }
    break;

    default:
        TransAssert((FALSE && "Invalid data location"));
    break;
    }

    if (_pProt)
    {
        _pProt->AddRef();
    }
    else
    {
        TransAssert((FALSE));
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::OnStart (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::OnTerminate
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::OnTerminate()
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCTransData, this, "+CTransData::OnTerminate");

    TransAssert((_TransDataState < TransData_ProtoTerminated));

    switch (_ds)
    {

    case DataSink_Storage:
    case DataSink_File:
    {
        TransAssert((_formatetc.tymed == TYMED_FILE) || (_formatetc.tymed == TYMED_ISTORAGE));
        DbgLog2(tagCTransData, this, ">>> CTransData::OnTerminate (hr:%lx, _wzTempFile:%ws)", hr, _wzFileName);
    }
    break;

    case DataSink_StreamBindToObject:
    case DataSink_StreamNoCopyData:
    case DataSink_StreamOnFile:
    case DataSink_GenericStream:
    {
        TransAssert((_formatetc.tymed == TYMED_ISTREAM));
        TransAssert((_pProt != NULL));
    }
    break;
    default:
        TransAssert((FALSE && "Invalid data location"));
    break;
    }

    if (_pBndCtx)
    {
        _pBndCtx->Release();
        _pBndCtx = NULL;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::OnTerminate (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::FindFormatETC
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::FindFormatETC()
{
    HRESULT hr = NOERROR;
    PerfDbgLog1(tagCTransData, this, "+CTransData::FindFormatETC (_cbBufferFilled:%ld)", _cbBufferFilled);

    _formatetc.ptd = NULL;
    _formatetc.dwAspect = DVASPECT_CONTENT;
    _formatetc.lindex = -1;

    _formatetc.cfFormat = 0;
    TransAssert((   _formatetc.tymed == TYMED_ISTREAM
                 || _formatetc.tymed == TYMED_FILE
                 || _formatetc.tymed == TYMED_ISTORAGE ));

    LPCWSTR pwzStrOrg = GetMimeType();
    LPCWSTR pwzStr = pwzStrOrg;

    // If not already done so, attempt to
    // verify mime type by examining data.
    if (!_fMimeTypeVerified)
    {
        DWORD dwFlags = 0;
        DWORD dwSniffFlags = 0;
        DWORD cbLen = sizeof(dwFlags);
        LPWSTR pwzFileName = GetFileName();
        LPWSTR pwzStrOut = 0;



        // the buffer should contain data if the no mime
        TransAssert((    (_cbBufferFilled == 0 && (pwzStr || pwzFileName))
                     ||  ( _cbBufferFilled != 0) ));

        FindMimeFromData(NULL, pwzFileName,_lpBuffer, _cbBufferFilled, pwzStrOrg, dwSniffFlags,  &pwzStrOut, 0);

        if (pwzStrOut)
        {
            SetMimeType(pwzStrOut);
            pwzStr = GetMimeType();
        }
        
        delete [] pwzStrOut;
        _fMimeTypeVerified = TRUE;
    }

    // the new mime should never be NULL if we had a proposed mime
    TransAssert((   (pwzStrOrg && pwzStr)
                 || (pwzStrOrg == NULL)     ));

    if (pwzStr)
    {
        char szMime[SZMIMESIZE_MAX];
        W2A(pwzStr, szMime, SZMIMESIZE_MAX);

        CLIPFORMAT cfType;
        if (FindMediaTypeW(pwzStr,&cfType) != NOERROR)
        {
            _formatetc.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(szMime);
        }
        else
        {
            _formatetc.cfFormat = cfType;
        }
    }
    else
    {
        _formatetc.cfFormat = CF_NULL;
    }

    // Check if the format that we got is one of the format that
    // is requested, and if so, use it
#ifdef UNUSED
    if (_pEnumFE)
    {
        FORMATETC FmtetcT;
        BOOL fDone = FALSE;

        _pEnumFE->Reset();

        while (!fDone && ((hr = _pEnumFE->Next(1, &FmtetcT,NULL)) == NOERROR))
        {
            TransAssert((SUCCEEDED(hr)));

            if (FmtetcT.cfFormat == _cfFormat)
            {
                _formatetc.cfFormat = _cfFormat;
            }
        }
    }
#endif //UNUSED

    PerfDbgLog3(tagCTransData, this, "-CTransData::FindFormatETC (hr:%lx, szStr:%ws, _formatetc.cfFormat:%lx)", hr, pwzStr?pwzStr:L"", _formatetc.cfFormat);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetAcceptStr
//
//  Synopsis:
//
//  Arguments:  [ppwzStr] --
//              [pcElements] --
//
//  Returns:
//
//  History:    3-29-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::GetAcceptStr(LPWSTR *ppwzStr, ULONG *pcElements)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCTransData, this, "+CTransData::GetAcceptStr");
    IEnumFORMATETC *pIEnumFE = NULL;

    TransAssert((ppwzStr));
    TransAssert((*pcElements > 0));

    CHAR pszUnknownName[MAX_PATH];
    ULONG cMimes = 0;
    
    *ppwzStr = 0;
    hr = GetObjectParam(_pBndCtx, REG_ENUMFORMATETC, IID_IEnumFORMATETC, (IUnknown**)&pIEnumFE);

    if (hr == NOERROR)
    {
        BOOL fCF_NULL = FALSE;
        #define ELEMENTS 10
        ULONG cElementsIn = ELEMENTS;
        ULONG cElements = 0;
        FORMATETC rgFormatEtc[ELEMENTS];


        pIEnumFE->Reset();
        // find # of elements
        do
        {
            ULONG cEl = 0;
            hr = pIEnumFE->Next(cElementsIn, rgFormatEtc ,&cEl);
            cElements += cEl;
        } while (hr == S_OK);

        UrlMkAssert((cElements > 0));

        if  (   (cElements > 0)
             && (hr == S_OK || hr == S_FALSE))
        {

            {
                ULONG cElementsOut = 0;
                pIEnumFE->Reset();
                do
                {
                    ULONG cEl = 0;
                    FORMATETC *pFmtEtc = rgFormatEtc;

                    hr = pIEnumFE->Next(cElements, rgFormatEtc ,&cEl);

                    for (ULONG i = 0; i < cEl; i++)
                    {
                        if( cMimes >=  (*pcElements - 1)  )
                        {
                            // exceeding the income array size, stop
                            break;
                        }


                        LPSTR szFormat = NULL;
                        CLIPFORMAT  cfFormat = (pFmtEtc + i)->cfFormat;
                        if (cfFormat == CF_NULL)
                        {
                            fCF_NULL = TRUE;
                        }
                        else
                        {
                            hr = FindMediaString(cfFormat, &szFormat);
                            if (hr != NOERROR || !szFormat)
                            {
                                // unknown cfFormat
                                if( GetClipboardFormatName(cfFormat, pszUnknownName, MAX_PATH))
                                {
                                    hr = NOERROR;
                                    szFormat = pszUnknownName;
                                } 
                                else
                                {
                                    // word97 will send out cfFormat=1
                                    // which associated to "" string
                                    hr = NOERROR;
                                }
                            }
                            if( szFormat )
                            {

                                *(ppwzStr + cMimes)= NULL; 
                                *(ppwzStr + cMimes) = DupA2W(szFormat);
                                if( *(ppwzStr + cMimes) )
                                {
                                    cMimes++;
                                } 
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                    break;
                                }
                            }
                        }
                    }
                    cElementsOut += cEl;

                } while ( (cElementsOut < cElements) && (hr == NOERROR) );

                // append the cf_null (*/*)
                if( hr == NOERROR && (fCF_NULL || (cMimes == 0)) )
                {
                    *(ppwzStr + cMimes) = DupA2W(g_szCF_NULL);
                    if( !*(ppwzStr + cMimes) )
                    {
                        hr = E_OUTOFMEMORY;
                    } 
                    else
                    { 
                        cMimes++;
                        hr = NOERROR;
                    }
                }

                if( hr == NOERROR )
                {
                    *(ppwzStr + cMimes) = NULL;
                }
            }
        }

        pIEnumFE->Release();
    }
    else
    {
        {
            *ppwzStr = DupA2W(g_szCF_NULL);
            if( *ppwzStr )
            {
                cMimes = 1;
                *(ppwzStr + 1) = NULL;
                hr = NOERROR;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if( hr == NOERROR )
    {
        *pcElements = cMimes;
    }
    else
    {
        *pcElements = 0;
        if( cMimes >= 1 )
        {
            for( ULONG i = 0; i < cMimes; i ++)
            {
                delete [] *(ppwzStr + i);
            }
        }
        *ppwzStr = NULL;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::GetAcceptStr (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetAcceptMimes
//
//  Synopsis:
//
//  Arguments:  [ppStr] --
//
//  Returns:
//
//  History:    3-29-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::GetAcceptMimes(LPWSTR *ppwzStr, ULONG cel, ULONG *pcElements)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCTransData, this, "+CTransData::GetAcceptMimes");

    TransAssert((ppwzStr && pcElements && *pcElements));

    if (ppwzStr && pcElements)
    {
        if( *pcElements > 1 )
        {
            hr = GetAcceptStr( ppwzStr, pcElements );
        }
        else if( *pcElements == 1)
        {
            // zero terminated 
            *ppwzStr = NULL;
        }
        else
        {
            hr =  E_INVALIDARG;
        }
    }
    else
    {
        hr =  E_INVALIDARG;
    }
    PerfDbgLog1(tagCTransData, this, "-CTransData::GetAcceptMimes (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::SetClipFormat
//
//  Synopsis:
//
//  Arguments:  [cfFormat] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::SetClipFormat(CLIPFORMAT  cfFormat)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCTransData, this, "+CTransData::SetClipFormat");

    _cfFormat = cfFormat;

    PerfDbgLog1(tagCTransData, this, "-CTransData::SetClipFormat (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::IsObjectReady
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::IsObjectReady(  )
{
    HRESULT hr = E_FAIL;
    PerfDbgLog(tagCTransData, this, "+CTransData::IsObjectReady");

    // check size and
    if ((_cbDataSize != 0) && (_cbDataSize == _cbTotalBytesRead))
    {
        hr = NOERROR;
    }

    PerfDbgLog3(tagCTransData, this, "-CTransData::IsObjectReady (hr:%lx, _cbDataSize:%ld, _cbTotalBytesRead:%ld)", hr, _cbDataSize, _cbTotalBytesRead);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::InProgress
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::InProgress()
{
    HRESULT hr = S_FALSE;
    PerfDbgLog(tagCTransData, this, "+CTransData::InProgress");

    if (_grfBSC & (BSCF_LASTDATANOTIFICATION | BSCF_FIRSTDATANOTIFICATION))
    {
        hr = S_FALSE;
    }
    else if (_cbTotalBytesRead == 0)
    {
        // check if some bits already in the buffer

        hr = S_OK;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::InProgress (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetFileName
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR CTransData::GetFileName()
{
    PerfDbgLog(tagCTransData, this, "+CTransData::GetFileName");
    LPWSTR pwzFileName = NULL;

    //TransAssert((_wzFileName[0] != 0));

    if (_wzFileName[0] != 0)
    {
        pwzFileName = _wzFileName;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::GetFileName (szFile:%ws)", pwzFileName?pwzFileName:L"");
    return pwzFileName;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetClassID
//
//  Synopsis:
//
//  Arguments:  [pclsid] --
//              [fReOpen] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::GetClassID(CLSID clisdIn, CLSID *pclsid)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::GetClassID");
    WCHAR wzMime[SZMIMESIZE_MAX];
    HRESULT hr;
    LPWSTR pwzTempFile = NULL;

    DWORD fIgnoreMimeClsid = GetBindFlags() & BINDF_IGNOREMIMECLSID;
    LPCWSTR pwzMime = GetMimeType();

    pwzTempFile = GetFileName();

    if (   (_cbBufferFilled != 0)
             && (_cbTotalBytesRead <= _cbBufferSize)
             && (IsDocFile(_lpBuffer, _cbBufferFilled) == S_OK))
    {
        _fDocFile = TRUE;
        _fMimeTypeVerified = TRUE;

        // Storage file (docfile) case
        // GetClassFileOrMime takes care of class mapping
        hr = GetClassFileOrMime2(_pBndCtx, pwzTempFile, _lpBuffer, _cbBufferFilled, pwzMime, 0, pclsid, fIgnoreMimeClsid);
        if (hr != NOERROR)
        {
            // S_FALSE means keep downloading
            hr = S_FALSE;
        }
    }
    else
    {
        if (!_fMimeTypeVerified)
        {
            DWORD dwFlags = 0;
            DWORD dwSniffFlags = 0;
            DWORD cbLen = sizeof(dwFlags);

            LPWSTR  pwzStr = 0;
            FindMimeFromData(NULL, pwzTempFile, _lpBuffer, _cbBufferFilled, pwzMime, dwSniffFlags,  &pwzStr, 0);
            if (pwzStr)
            {
                SetMimeType(pwzStr);
            }
            _fMimeTypeVerified = TRUE;
            delete [] pwzStr;
        }

        hr = GetClassFileOrMime2(_pBndCtx, pwzTempFile, NULL, 0, pwzMime, 0, pclsid, fIgnoreMimeClsid);
    }

    #if DBG==1
    if (hr == NOERROR)
    {
        LPOLESTR pszStr;
        StringFromCLSID(*pclsid, &pszStr);
        DbgLog2(tagCTransData, this, "CTransData::GetClassID (file:%ws)(class:%ws)",
            pwzTempFile, pszStr);
        delete pszStr;
    }
    #endif

    PerfDbgLog1(tagCTransData, this, "-CTransData::GetClassID (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::SetMimeType
//
//  Synopsis:
//
//  Arguments:  [pszMime] --
//
//  Returns:
//
//  History:    2-07-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::SetMimeType(LPCWSTR pwzMime)
{
    PerfDbgLog2(tagCTransData, this, "+CTransData::SetMimeType (OldMime:%ws; MimeStr:%ws)", _wzMime?_wzMime:L"", pwzMime?pwzMime:L"");
    HRESULT hr = NOERROR;

    if (pwzMime)
    {

        #if DBG_XXX
        if (pwzMime)
        {
            if (   !wcscmp(_wzMime, CFWSTR_MIME_HTML)
                || !wcscmp(_wzMime, CFWSTR_MIME_TEXT))
            {
                if (   !wcscmp(pwzMime, CFWSTR_MIME_RAWDATA)
                    || !wcscmp(pwzMime,L"application/octet-stream"))
                {
                    DbgLog2(tagCTransDataErr, "=== SetMimeType: OldMime:%ws, NewMime:%ws",
                        _wzMime, pwzMime);
                    //TransAssert((FALSE));
                }
            }
        }

        if (_wzMime[0] != 0 && strcmp(_wzMime,pwzMime))
        {
            DbgLog2(tagTransDataErr, "=== SetMimeType: OldMime:%s, NewMime:%s",
                _wzMime, pwzMime);
        }

        #endif //DBG


        DWORD cLen = wcslen((LPWSTR)pwzMime);

        if (cLen >= SZMIMESIZE_MAX)
        {
            cLen = SZMIMESIZE_MAX - 1;
            wcsncpy(_wzMime, (LPWSTR)pwzMime, cLen);
            _wzMime[cLen] = 0;
        }
        else
        {
            wcscpy(_wzMime, (LPWSTR)pwzMime);
        }
    }
    else
    {
        DbgLog(tagCTransDataErr, this, "CTransData::SetMimeType ->invalid mime");
    }

    PerfDbgLog2(tagCTransData, this, "-CTransData::SetMimeType (hr:%lx, Mime:%ws)", hr,_wzMime?_wzMime:L"");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::SetFileName
//
//  Synopsis:
//
//  Arguments:  [szFile] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CTransData::SetFileName(LPWSTR pwzFile)
{
    PerfDbgLog(tagCTransData, this, "+CTransData::SetFileName");

    TransAssert((pwzFile));
    wcscpy(_wzFileName, pwzFile);

    PerfDbgLog2(tagCTransData, this, "-CTransData::SetFileName (_wzFileName:%ws, hr:%lx)", _wzFileName?_wzFileName:L"", NOERROR);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CTransData::GetMimeType
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-24-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPCWSTR CTransData::GetMimeType()
{
    PerfDbgLog(tagCTransData, this, "+CTransData::GetMimeType");
    LPWSTR pwzStr = NULL;

    if (_wzMime[0] != 0)
    {
        pwzStr = _wzMime;
    }

    PerfDbgLog1(tagCTransData, this, "-CTransData::GetMimeType (pStr:%ws)", pwzStr?pwzStr:L"");
    return pwzStr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CTransData::OnEndofData()
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    7-30-97   DanpoZ(Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CTransData::OnEndofData()
{
    PerfDbgLog1(tagCTransData, this, "+CTransData::OnEndofData(_ds:%ld)", _ds);
    BOOL fNewStgMed = FALSE;

    if (_pStgMed  && _ds == DataSink_StreamOnFile && _pStgMed->pstm )
    {
        ((CReadOnlyStreamFile*)(_pStgMed->pstm))->SetDataFullyAvailable();
    }

    PerfDbgLog(tagCTransData, this, "-CTransData::OnEndofData");
}
