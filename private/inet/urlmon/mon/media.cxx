//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       media.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-20-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <mon.h>

PerfDbgTag(tagMedia,    "Urlmon", "Log Media methods", DEB_FORMAT);
PerfDbgTag(tagMediaApi, "Urlmon", "Log Media API",     DEB_ASYNCAPIS);

#if 1
CHAR vszTextPlain[] =                   "text/plain";
CHAR vszTextRichText[] =                "text/richtext";
CHAR vszImageXBitmap[] =                "image/x-xbitmap";
CHAR vszApplicationPostscript[] =       "application/postscript";
CHAR vszApplicationBase64[] =           "application/base64";
CHAR vszApplicationMacBinhex[] =        "application/macbinhex40";
CHAR vszApplicationPdf[] =              "application/pdf";
CHAR vszAudioAiff[] =                   "audio/x-aiff";
CHAR vszAudioBasic[] =                  "audio/basic";
CHAR vszAudioWav[] =                    "audio/wav";
CHAR vszImageGif[] =                    "image/gif";
CHAR vszImagePJpeg[] =                  "image/pjpeg";
CHAR vszImageJpeg[] =                   "image/jpeg";
CHAR vszImageTiff[] =                   "image/tiff";
CHAR vszImagePng[] =                    "image/x-png";
CHAR vszImagePng2[] =                   "image/png";
CHAR vszImageBmp[] =                    "image/bmp";
CHAR vszImageJG[] =                     "image/x-jg";
CHAR vszImageArt[] =                    "image/x-art";
CHAR vszImageEmf[] =                    "image/x-emf";
CHAR vszImageWmf[] =                    "image/x-wmf";
CHAR vszVideoAvi[] =                    "video/avi";
CHAR vszVideoMS[] =                     "video/x-msvideo";
CHAR vszVideoMpeg[] =                   "video/mpeg";
CHAR vszApplicationCompressed[] =       "application/x-compressed";
CHAR vszApplicationZipCompressed[] =    "application/x-zip-compressed";
CHAR vszApplicationGzipCompressed[] =   "application/x-gzip-compressed";
CHAR vszApplicationMSDownload[] =       "application/x-msdownload";
CHAR vszApplicationJava[] =             "application/java";
CHAR vszApplicationOctetStream[] =      "application/octet-stream";
CHAR vszTextHTML[] =                    "text/html";
CHAR vszApplicationCDF[] =              "application/x-cdf";
CHAR vszApplicationCommonDataFormat[] = "application/x-netcdf";
CHAR vszTextScriptlet[] =               "text/scriptlet";
#endif

const GUID IID_IMediaHolder =  {0x79eac9ce, 0xbaf9, 0x11ce, {0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b}};

#define XCLSID_MsHtml  {0x25336920, 0x03F9, 0x11cf, {0x8F, 0xD0, 0x00, 0xAA, 0x00, 0x68, 0x6F, 0x13}}
#define XCLSID_CDFVIEW {0xf39a0dc0, 0x9cc8, 0x11d0, {0xa5, 0x99, 0x0, 0xc0, 0x4f, 0xd6, 0x44, 0x33}}

BOOL g_fDefaultMediaRegistered = FALSE;
static MediaInfo rgMediaInfo[] =
{
       { vszTextHTML                    , 0, DATAFORMAT_TEXTORBINARY,   XCLSID_MsHtml, 0x00000070 ,0 ,CLSCTX_INPROC  }
      ,{ vszTextPlain                   , 0, DATAFORMAT_AMBIGUOUS,      {0} ,0 ,0 ,0  }
      ,{ vszTextRichText                , 0, DATAFORMAT_TEXT,           {0} ,0 ,0 ,0  }
      ,{ vszImageXBitmap                , 0, DATAFORMAT_TEXT,           {0} ,0 ,0 ,0  }
      ,{ vszApplicationPostscript       , 0, DATAFORMAT_TEXT,           {0} ,0 ,0 ,0  }
      ,{ vszApplicationBase64           , 0, DATAFORMAT_TEXT,           {0} ,0 ,0 ,0  }
      ,{ vszApplicationMacBinhex        , 0, DATAFORMAT_TEXT,           {0} ,0 ,0 ,0  }
      ,{ vszApplicationPdf              , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszAudioAiff                   , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszAudioBasic                  , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszAudioWav                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageGif                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImagePJpeg                  , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageJpeg                   , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageTiff                   , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImagePng                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImagePng2                   , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageBmp                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageJG                     , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageArt                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageEmf                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszImageWmf                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszVideoAvi                    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszVideoMS                     , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszVideoMpeg                   , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszApplicationCompressed       , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszApplicationZipCompressed    , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszApplicationGzipCompressed   , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszApplicationJava             , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszApplicationMSDownload       , 0, DATAFORMAT_BINARY,         {0} ,0 ,0 ,0  }
      ,{ vszApplicationOctetStream      , 0, DATAFORMAT_AMBIGUOUS,      {0} ,0 ,0 ,0  }
      ,{ vszApplicationCDF              , 0, DATAFORMAT_TEXT,           XCLSID_CDFVIEW ,(MI_GOTCLSID | MI_CLASSLOOKUP) ,0 ,CLSCTX_INPROC  }
      ,{ vszApplicationCommonDataFormat , 0, DATAFORMAT_AMBIGUOUS,      {0} ,0 ,0 ,0  }
      ,{ vszTextScriptlet               , 0, DATAFORMAT_TEXT,           {0} ,0 ,0 ,0  }
};

CMediaTypeHolder *g_pCMHolder = NULL;

//+---------------------------------------------------------------------------
//
//  Function:   GetMediaTypeHolder
//
//  Synopsis:   Retrieves the media type holder for this apartment
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CMediaTypeHolder *GetMediaTypeHolder()
{
    PerfDbgLog(tagMediaApi, NULL, "+GetMediaTypeHolder");
#ifdef PER_THREAD
    CUrlMkTls tls;

    CMediaTypeHolder *pCMHolder;

    if ((pCMHolder = tls->pCMediaHolder) == NULL)
    {
        tls->pCMediaHolder = pCMHolder = new CMediaTypeHolder();
    }
#else
    CLock lck(g_mxsMedia);

    if (g_pCMHolder == NULL)
    {
        g_pCMHolder = new CMediaTypeHolder();
    }

#endif //PER_THREAD

    PerfDbgLog1(tagMediaApi, NULL, "-GetMediaTypeHolder (pCMHolder:%lx)", g_pCMHolder);
    return g_pCMHolder;
}


void CMediaType::Initialize(LPSTR szType, CLIPFORMAT cfFormat)
{

    _pszType  = szType;
    _cfFormat = cfFormat;
}

void CMediaType::Initialize(CLIPFORMAT cfFormat, CLSID *pClsID)
{
    _pszType  = NULL;
    _cfFormat = cfFormat;
    _clsID = *pClsID;
    _dwInitFlags |= MI_GOTCLSID;
}


CMediaTypeHolder::CMediaTypeHolder() : _CRefs()
{
    _pCMTNode = NULL;
}

CMediaTypeHolder::~CMediaTypeHolder()
{
    CMediaTypeNode  *pCMTNode, *pNext;

    pCMTNode = _pCMTNode;

    // Delete everything that was allocated by Register.

    while (pCMTNode)
    {
        pNext = pCMTNode->GetNextNode();
        delete pCMTNode;
        pCMTNode = pNext;
    }

    _pCMTNode = NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::RegisterW
//
//  Synopsis:
//
//  Arguments:  [ctypes] --
//              [rgszTypes] --
//              [rgcfTypes] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CMediaTypeHolder::RegisterW(UINT ctypes, const LPCWSTR* rgszTypes, CLIPFORMAT* rgcfTypes)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::Register");
    HRESULT hr = NOERROR;

    UINT i;

    if (ctypes)
    {
        ULONG           ulSize;
        LPCWSTR         pwzStr;
        LPSTR           pszStr;
        LPSTR           pszHelp;

        LPSTR           pszTextBuffer;
        CMediaType      *pCMType;
        CMediaTypeNode  *pCMTNode;

        // Calculate size of single buffer needed to hold all strings.

        for (ulSize = i = 0; i < ctypes; i++)
        {
            pwzStr = *(rgszTypes + i);
            ulSize += wcslen(pwzStr) + 1;
            //PerfDbgLog2(tagMedia, this, "CMTHolder::Register(sz:%ws; len:%ld)", pszStr, ulSize));
        }

        pszTextBuffer = pszStr = new CHAR[ulSize];
        if (!pszTextBuffer)
        {
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pCMType = new CMediaType[ctypes];
        if (!pCMType)
        {
            delete pszTextBuffer;
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pCMTNode = new CMediaTypeNode(pCMType, pszTextBuffer, ctypes, _pCMTNode);
        if (!pCMTNode)
        {
            delete pCMType;
            delete pszTextBuffer;
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pszHelp = pszStr;

        for (i = 0; i < ctypes; i++)
        {
            pwzStr = *(rgszTypes + i);
            //wcscpy(pszHelp, pszStr);
            W2A(pwzStr, pszHelp, wcslen(pwzStr) + 1);
            (pCMType + i)->Initialize(pszHelp, *(rgcfTypes + i));
            pszHelp += strlen(pszHelp) + 1;
        }

        // New node is first on list.

        _pCMTNode = pCMTNode;
    }

RegisterExit:

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::Register (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::Register
//
//  Synopsis:
//
//  Arguments:  [ctypes] --
//              [rgszTypes] --
//              [rgcfTypes] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CMediaTypeHolder::Register(UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::Register");
    HRESULT hr = NOERROR;

    UINT i;

    if (ctypes)
    {
        ULONG           ulSize;
        LPCSTR          pszStr;
        LPSTR           pszNewStr;
        LPSTR           pszHelp;

        LPSTR           pszTextBuffer;
        CMediaType      *pCMType;
        CMediaTypeNode  *pCMTNode;

        // Calculate size of single buffer needed to hold all strings.

        for (ulSize = i = 0; i < ctypes; i++)
        {
            pszStr = *(rgszTypes + i);
            ulSize += strlen(pszStr) + 1;
            //PerfDbgLog2(tagMedia, this, "CMTHolder::Register(sz:%s; len:%ld)", pszStr, ulSize));
        }

        pszTextBuffer = pszNewStr = new CHAR[ulSize];
        if (!pszTextBuffer)
        {
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pCMType = new CMediaType[ctypes];
        if (!pCMType)
        {
            delete pszTextBuffer;
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pCMTNode = new CMediaTypeNode(pCMType, pszTextBuffer, ctypes, _pCMTNode);
        if (!pCMTNode)
        {
            delete pCMType;
            delete pszTextBuffer;
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pszHelp = pszNewStr;

        for (i = 0; i < ctypes; i++)
        {
            pszStr = *(rgszTypes + i);

            StrNCpy(pszHelp, pszStr, strlen(pszStr) + 1);
            *(rgcfTypes + i) = (CLIPFORMAT) RegisterClipboardFormat(pszStr);

            (pCMType + i)->Initialize(pszHelp, *(rgcfTypes + i));
            pszHelp += strlen(pszHelp) + 1;
        }

        // New node is first on list.

        if (!_pCMTNode)
        {
            _pCMTNode = pCMTNode;
        }
        else
        {
            CMediaTypeNode  *pCMTNext = _pCMTNode->GetNextNode();
            _pCMTNode->SetNextNode(pCMTNode);
            pCMTNode->SetNextNode(pCMTNext);
        }

    }

RegisterExit:

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::Register (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::Register
//
//  Synopsis:
//
//  Arguments:  [ctypes] --
//              [rgszTypes] --
//              [rgcfTypes] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CMediaTypeHolder::RegisterMediaInfo(UINT ctypes, MediaInfo *pMediaInfo, BOOL fFree)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::RegisterMediaInfo");
    HRESULT hr = NOERROR;

    UINT i;

    if (ctypes)
    {
        CMediaType      *pCMType;
        CMediaTypeNode  *pCMTNode;

        // Calculate size of single buffer needed to hold all strings.

        pCMType = (CMediaType *)pMediaInfo;
        if (!pCMType)
        {
            goto RegisterExit;
        }

        pCMTNode = new CMediaTypeNode(pCMType, NULL, ctypes, _pCMTNode, FALSE);
        if (!pCMTNode)
        {
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        for (i = 0; i < ctypes; i++)
        {
            CMediaType *pCMT = (pCMType + i);
            CLIPFORMAT cf = (CLIPFORMAT) RegisterClipboardFormat(pCMT->GetTypeString());
            pCMT->SetClipFormat(cf);
        }

        // New node is first on list.
        _pCMTNode = pCMTNode;
    }

RegisterExit:

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::RegisterMediaInfo (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::FindCMediaType
//
//  Synopsis:
//
//  Arguments:  [pszMimeStr] --
//              [ppCMType] --
//
//  Returns:
//
//  History:    3-26-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CMediaTypeHolder::FindCMediaType(CLIPFORMAT cfFormat, CMediaType **ppCMType)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::FindCMediaType");
    HRESULT         hr = E_INVALIDARG;
    CMediaTypeNode  *pCMTNode;
    CMediaType      *pCMType;
    UINT            i;

    UrlMkAssert((ppCMType));
    *ppCMType = NULL;

    pCMTNode = _pCMTNode;

    if (!pCMTNode)
    {
        hr = E_FAIL;
    }
    else while (pCMTNode)
    {
        pCMType = pCMTNode->GetMediaTypeArray();

        for (i = 0; i < pCMTNode->GetElementCount(); i++)
        {
            if (cfFormat == (pCMType + i)->GetClipFormat())
            {
                *ppCMType = pCMType + i;
                hr = NOERROR;
                break;
            }
        }

        if (*ppCMType)
        {
            break;
        }

        pCMTNode = pCMTNode->GetNextNode();
    }

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::FindCMediaType (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::FindCMediaType
//
//  Synopsis:
//
//  Arguments:  [pszMimeStr] --
//              [ppCMType] --
//
//  Returns:
//
//  History:    3-28-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CMediaTypeHolder::FindCMediaType(LPCSTR pszMimeStr, CMediaType **ppCMType)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::FindCMediaType");
    HRESULT         hr = E_INVALIDARG;
    CMediaTypeNode  *pCMTNode;
    CMediaType      *pCMType;
    UINT            i;

    *ppCMType = NULL;

    pCMTNode = _pCMTNode;

    if (!pCMTNode)
    {
        hr = E_FAIL;
    }
    else while (pCMTNode)
    {
        pCMType = pCMTNode->GetMediaTypeArray();

        for (i = 0; i < pCMTNode->GetElementCount(); i++)
        {
            if (!stricmp(pszMimeStr, (pCMType + i)->GetTypeString()))
            {
                *ppCMType = pCMType + i;
                hr = NOERROR;
                break;
            }
        }

        if (*ppCMType)
        {
            break;
        }

        pCMTNode = pCMTNode->GetNextNode();
    }

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::FindCMediaType (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMediaTypeHolder::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hr = NOERROR;

    PerfDbgLog2(tagMedia, this, "+CMediaTypeHolder::QueryInterface (%lx, %lx)", riid, ppv);

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IMediaHolder))
    {
        *ppv = (void FAR *)this;
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog2(tagMedia, this, "-CMediaTypeHolder::QueryInterface (%lx)[%lx]", hr, *ppv);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMediaTypeHolder::AddRef( void )
{
    LONG lRet = _CRefs++;
    PerfDbgLog1(tagMedia, this, "CMediaTypeHolder::AddRef (%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMediaTypeHolder::Release( void )
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::Release");

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }
    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::Release (%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::RegisterClassMapping
//
//  Synopsis:   registers a class mapping for given mimes strings
//
//  Arguments:  [ctypes] --
//              [rgszNames] --
//              [rgClsIDs] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    8-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMediaTypeHolder::RegisterClassMapping (DWORD ctypes, LPCSTR rgszNames[], CLSID rgClsIDs[], DWORD dwReserved)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::RegisterClassMapping");

    HRESULT hr = RegisterClass(ctypes, rgszNames, rgClsIDs );

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::RegisterClassMapping (hr:%lX)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::FindClassMapping
//
//  Synopsis:   returns the class for a given mime if registered
//
//  Arguments:  [szMime] --
//              [pClassID] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    8-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMediaTypeHolder::FindClassMapping(LPCSTR szMime, CLSID *pClassID, DWORD dwReserved)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::FindClassMapping");
    HRESULT hr = NOERROR;
    CLIPFORMAT cfTypes = CF_NULL;

    TransAssert((pClassID));
    *pClassID = CLSID_NULL;

    cfTypes = (CLIPFORMAT) RegisterClipboardFormat(szMime);
    if (cfTypes != CF_NULL)
    {
        CMediaType  *pCMType = NULL;
        hr = FindCMediaType(cfTypes, &pCMType);
        if (hr == NOERROR)
        {
            TransAssert((pCMType));
            hr = pCMType->GetClsID(pClassID);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::FindClassMapping (hr:%lx)", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CMediaTypeHolder::RegisterClass
//
//  Synopsis:
//
//  Arguments:  [ctypes] --
//              [rgszTypes] --
//              [rgcfTypes] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CMediaTypeHolder::RegisterClass(UINT ctypes, const LPCSTR* rgszTypes, CLSID *rgclsID)
{
    PerfDbgLog(tagMedia, this, "+CMediaTypeHolder::RegisterClass");
    HRESULT hr = NOERROR;

    UINT i;

    if (ctypes)
    {
        ULONG           ulSize;
        CMediaType      *pCMType;
        CMediaTypeNode  *pCMTNode;

        // Calculate size of single buffer needed to hold all strings.

        pCMType = new CMediaType[ctypes];
        if (!pCMType)
        {
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }

        pCMTNode = new CMediaTypeNode(pCMType, NULL, ctypes, _pCMTNode);
        if (!pCMTNode)
        {
            delete pCMType;
            hr = E_OUTOFMEMORY;
            goto RegisterExit;
        }
        for (i = 0; i < ctypes; i++)
        {
            CLIPFORMAT cfTypes;
            LPCSTR pszStr = *(rgszTypes + i);

            cfTypes = (CLIPFORMAT) RegisterClipboardFormat(pszStr);
            pCMType[i].Initialize(cfTypes,(rgclsID + i));
        }

        // New node is first on list.
        _pCMTNode = pCMTNode;
    }

RegisterExit:

    PerfDbgLog1(tagMedia, this, "-CMediaTypeHolder::RegisterClass (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterDefaultMediaType
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    3-28-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT RegisterDefaultMediaType()
{
    PerfDbgLog(tagMediaApi, NULL, "+RegisterDefaultMediaType");

    HRESULT hr = InternalRegisterDefaultMediaType();

    PerfDbgLog(tagMediaApi, NULL, "-RegisterDefaultMediaType");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   InternalRegisterDefaultMediaType
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    7-24-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT InternalRegisterDefaultMediaType()
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagMediaApi, NULL, "+InternalRegisterDefaultMediaType");
    static DWORD s_dwfHonorTextPlain = 42;
    CLock lck(g_mxsMedia);

    // Provide a registry hook for enabling urlmon to honor
    // text/plain, rather than defer to the extension.
    // Previously, this was considered ambiguous due to compat
    // reasons with older servers sending this for unknown content types.
    //
    // TODO:  Consider making this the default behavior because
    //        other browsers (e.g. Nav 4.61) are moving in this
    //        direction.
    if (s_dwfHonorTextPlain == 42)
    {
        HKEY hKey;
        DWORD dwType;
        DWORD dwSize = sizeof(s_dwfHonorTextPlain);
        s_dwfHonorTextPlain = 0;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 0, KEY_QUERY_VALUE, &hKey))
        {
            if (ERROR_SUCCESS != RegQueryValueEx(hKey, TEXT("IsTextPlainHonored"), NULL, &dwType, (LPBYTE)&s_dwfHonorTextPlain, &dwSize))
                s_dwfHonorTextPlain = 0;

            RegCloseKey(hKey);
        }
        // rgMediaInfo is a static global, so we only need to do this once.
        if (s_dwfHonorTextPlain)
        {
            DWORD d = 0;
            dwSize = sizeof(rgMediaInfo)/sizeof(MediaInfo);
            for (d = 0; d < dwSize; d++)
            {
                if (!lstrcmp(vszTextPlain, rgMediaInfo[d]._pszType))
                {
                    rgMediaInfo[d]._dwDataFormat = DATAFORMAT_TEXTORBINARY;  // remove the ambiguity
                    break;
                }
            }
        }
    }

    if (g_fDefaultMediaRegistered == FALSE)
    {
        CMediaTypeHolder *pCMHolder;
        g_fDefaultMediaRegistered = TRUE;

        pCMHolder = GetMediaTypeHolder();

        if (pCMHolder)
        {
            DWORD dwSize =sizeof(rgMediaInfo)/sizeof(MediaInfo);
            hr = pCMHolder->RegisterMediaInfo(dwSize,rgMediaInfo,FALSE);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    PerfDbgLog(tagMediaApi, NULL, "-InternalRegisterDefaultMediaType");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   FindMediaType
//
//  Synopsis:
//
//  Arguments:  [pwzType] --
//              [cfType] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT FindMediaType(LPCSTR pszType, CLIPFORMAT *cfType)
{
    HRESULT hr = E_OUTOFMEMORY;
    PerfDbgLog(tagMediaApi, NULL, "+FindMediaType");
    CMediaTypeHolder *pCMHolder;
    CLock lck(g_mxsMedia);

    UrlMkAssert((cfType));

    InternalRegisterDefaultMediaType();

    if ((pszType == NULL) || (cfType == CF_NULL))
    {
        hr = E_INVALIDARG;
    }
    else if ((pCMHolder = GetMediaTypeHolder()) != NULL)
    {

        CMediaType  *pCMType;
        *cfType = CF_NULL;

        hr = pCMHolder->FindCMediaType(pszType, &pCMType);
        if (hr == NOERROR)
        {
            *cfType = pCMType->GetClipFormat();
        }
    }
    else
    {
        hr = E_FAIL;
    }

    PerfDbgLog(tagMediaApi, NULL, "-FindMediaType");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMediaTypeW
//
//  Synopsis:
//
//  Arguments:  [pwzType] --
//              [cfType] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT FindMediaTypeW(LPCWSTR pwzType, CLIPFORMAT *pcfType)
{
    HRESULT hr = E_NOTIMPL;
    PerfDbgLog(tagMediaApi, NULL, "+FindMediaTypeW");
    CMediaTypeHolder *pCMHolder;
    CLock lck(g_mxsMedia);

    char szMime[SZMIMESIZE_MAX];
    W2A(pwzType, szMime, SZMIMESIZE_MAX);

    if (FindMediaType((LPCSTR)szMime,pcfType) != NOERROR)
    {
        *pcfType = (CLIPFORMAT) RegisterClipboardFormat(szMime);
        hr = NOERROR;
    }

    PerfDbgLog(tagMediaApi, NULL, "-FindMediaTypeW");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMediaString
//
//  Synopsis:
//
//  Arguments:  [cfFormat] --
//              [ppStr] --
//
//  Returns:
//
//  History:    3-29-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT FindMediaString(CLIPFORMAT cfFormat, LPSTR *ppStr)
{
    HRESULT hr = E_OUTOFMEMORY;
    PerfDbgLog1(tagMediaApi, NULL, "+FindMediaString (cfFormat:%d)", cfFormat);
    CMediaTypeHolder *pCMHolder;
    CLock lck(g_mxsMedia);

    UrlMkAssert((cfFormat));

    InternalRegisterDefaultMediaType();

    if ((pCMHolder = GetMediaTypeHolder()) != NULL)
    {
        CMediaType  *pCMType = NULL;
        hr = pCMHolder->FindCMediaType(cfFormat, &pCMType);
        if ((hr == NOERROR) && pCMType)
        {
            *ppStr = pCMType->GetTypeString();
            UrlMkAssert((*ppStr));
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    PerfDbgLog2(tagMediaApi, NULL, "-FindMediaString (clFormat:%d -> szMime:%s)",cfFormat,hr?"":*ppStr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMediaTypeFormat
//
//  Synopsis:
//
//  Arguments:  [pszType] --
//              [cfType] --
//              [pdwFormat] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT FindMediaTypeFormat(LPCWSTR pwzType, CLIPFORMAT *cfType, DWORD *pdwFormat)
{
    HRESULT hr = E_OUTOFMEMORY;
    PerfDbgLog(tagMediaApi, NULL, "+FindMediaTypeFormat");
    CMediaTypeHolder *pCMHolder;
    CLock lck(g_mxsMedia);

    UrlMkAssert((cfType));

    LPSTR pszType = DupW2A(pwzType);

    InternalRegisterDefaultMediaType();

    if ((pszType == NULL) || (cfType == CF_NULL) || (!pdwFormat))
    {
        hr = E_INVALIDARG;
    }
    else if ((pCMHolder = GetMediaTypeHolder()) != NULL)
    {

        CMediaType  *pCMType;
        *cfType = CF_NULL;

        hr = pCMHolder->FindCMediaType(pszType, &pCMType);
        if (hr == NOERROR)
        {
            *cfType = pCMType->GetClipFormat();
            *pdwFormat = pCMType->GetDataFormat();
        }
    }
    else
    {
        hr = E_FAIL;
    }

    if (pszType)
    {
        delete pszType;
    }

    PerfDbgLog4(tagMediaApi, NULL, "-FindMediaTypeFormat (hr:%lx Mime:%ws, cf:%ld, DataFormat:%ld)",
        hr, pwzType, cfType, *pdwFormat);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMediaTypeClassInfo
//
//  Synopsis:
//
//  Arguments:  [pszType] --
//              [pclsid] --
//              [pdwClsCtx] --
//
//  Returns:
//
//  History:    7-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT FindMediaTypeClassInfo(LPCSTR pszType, LPCSTR pszFileName, LPCLSID pclsid, DWORD *pdwClsCtx, DWORD dwFlags)
{
    HRESULT hr = E_OUTOFMEMORY;
    PerfDbgLog(tagMediaApi, NULL,"+FindMediaTypeClassInfo\n");
    CMediaTypeHolder *pCMHolder;
    BOOL fChecked = FALSE;
    CLock lck(g_mxsMedia);

    UrlMkAssert((pclsid));

    InternalRegisterDefaultMediaType();

    if ((pszType == NULL) || (!pclsid))
    {
        hr = E_INVALIDARG;
    }
    else if ((pCMHolder = GetMediaTypeHolder()) != NULL)
    {

        CMediaType  *pCMType;
        *pclsid = CLSID_NULL;

        hr = pCMHolder->FindCMediaType(pszType, &pCMType);
        if ( hr == NOERROR && !(dwFlags & MIMEFLAGS_IGNOREMIME_CLASSID) ) 
        {
            fChecked = TRUE;

            hr = pCMType->GetClsID(pclsid);

            if (hr == NOERROR)
            {
                hr = pCMType->GetClsCtx(pdwClsCtx);
                if (hr != NOERROR)
                {
                    hr = GetClsIDInfo(pclsid, 0, pdwClsCtx);
                    if (hr == NOERROR)
                    {
                        pCMType->SetClsCtx(*pdwClsCtx);
                    }
                }
            }
            else if (!pCMType->IsLookupDone())
            {
                pCMType->SetLookupDone();
                
                //
                // find the clsid and clsctx
                DWORD dwMimeFlags = 0;
                hr = GetMimeInfo((LPSTR)pszType, pclsid, FALSE, &dwMimeFlags);
                
                if (hr == NOERROR)
                {
                    pCMType->SetMimeInfo(*pclsid, dwMimeFlags);
                    hr = GetClsIDInfo(pclsid, 0, pdwClsCtx);
                    if (hr == NOERROR)
                    {
                        pCMType->SetClsCtx(*pdwClsCtx);
                    }
                } 
            
                // no class found yet - try the filename
                
                if (hr != NOERROR && pszFileName)
                {
                    WCHAR wzFileName[MAX_PATH];
                    
                    A2W((LPSTR)pszFileName,wzFileName, MAX_PATH);
                    
                    hr = GetClassFile(wzFileName, pclsid);

                    if (hr == NOERROR)
                    {
                        // do not trust filename-app/oc combination
                        // example: a.doc and a.xls are both marked as app/oc
                        if( strcmp(pszType, "application/octet-stream"))
                        {
                            
                            // found a class for this mime
                            pCMType->SetMimeInfo(*pclsid, dwMimeFlags);
                            hr = GetClsIDInfo(pclsid, 0, pdwClsCtx);
                            if (hr == NOERROR)
                            {
                                pCMType->SetClsCtx(*pdwClsCtx);
                            }
                        }
                        else
                        {
                            pCMType->SetLookupDone(FALSE);
                        }
    
                    }
                }
            }
        }
        
        if (   (hr != NOERROR || (dwFlags & MIMEFLAGS_IGNOREMIME_CLASSID) ) 
            && !fChecked )
        {
            hr = GetClassMime((LPSTR)pszType, pclsid, (dwFlags & MIMEFLAGS_IGNOREMIME_CLASSID));
        }
        
    }
    else
    {
        hr = E_FAIL;
    }

    PerfDbgLog1(tagMediaApi, NULL, "-FindMediaTypeClassInfo (hr:%lx)\n",hr);
    return hr;
}



