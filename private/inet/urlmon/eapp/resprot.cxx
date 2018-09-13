//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       resprot.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <eapp.h>
#include <tchar.h>

#define MAX_ID 10000

#define WITH_TAGS

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pTrans] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pTrans, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD dwReserved)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::Start\n", this));
    HRESULT hr = NOERROR;
    WCHAR    wzURL[MAX_URL_SIZE];

    EProtAssert((!_pProtSink && pOIBindInfo && pTrans));
    EProtAssert((_pwzUrl == NULL));

    hr = CBaseProtocol::Start(pwzUrl,pTrans, pOIBindInfo, grfSTI, dwReserved);

    if ( (grfSTI & PI_PARSE_URL) )
    {
        hr =  ParseAndBind(FALSE);
    }
    else if (hr == NOERROR)
    {
        // asked to go async as soon as possible
        // use the switch mechanism which will \
        // call back later on ::Continue
        if (grfSTI & PI_FORCE_ASYNC)
        {
            hr = E_PENDING;
            PROTOCOLDATA protdata;
            protdata.grfFlags = PI_FORCE_ASYNC;
            protdata.dwState = RES_STATE_BIND;
            protdata.pData = 0;
            protdata.cbData = 0;

            _pProtSink->Switch(&protdata);
        }
        else
        {
            hr =  ParseAndBind(TRUE);
        }
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::Start (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::Continue(PROTOCOLDATA *pStateInfoIn)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::Continue\n", this));
    HRESULT hr = E_FAIL;

    EProtAssert((!pStateInfoIn->pData && pStateInfoIn->cbData && (pStateInfoIn->dwState == RES_STATE_BIND)));

    if (pStateInfoIn->dwState == RES_STATE_BIND)
    {
        hr =  ParseAndBind();
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::Continue (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::Read
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ULONG] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::Read (cb:%ld)\n", this,cb));
    HRESULT hr = NOERROR;

    if (_cbBuffer > _cbPos)
    {
        ULONG cbCopy = (cb < (_cbBuffer - _cbPos)) ? cb :  _cbBuffer - _cbPos;

        memcpy((LPVOID)pv, ((LPBYTE)_pBuffer) + _cbPos, cbCopy);

        _cbPos += cbCopy;

        *pcbRead = cbCopy;

        hr =  (_cbPos < _cbBuffer) ? S_OK : S_FALSE;
    }
    else
    {
        hr = S_FALSE;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::Read (pcbRead:%ld, hr:%lx)\n",this,*pcbRead, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:      WORK: not done
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::Seek\n", this));
    HRESULT hr = NOERROR;

    if (dwOrigin == STREAM_SEEK_SET)
    {
        if (dlibMove.LowPart >= 0)
        {
            _cbPos = dlibMove.LowPart;

            if (plibNewPosition)
            {
                plibNewPosition->HighPart = 0;
                plibNewPosition->LowPart = _cbPos;
            }
        }
        else
        {
            hr = STG_E_INVALIDPOINTER;
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::Seek (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::LockRequest(DWORD dwOptions)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::LockRequest\n", this));

    HRESULT hr = NOERROR;

    if (OpenTempFile())
    {
        DWORD dwWrite;
        if (!WriteFile(_hFile, _pBuffer, _cbBuffer, &dwWrite,NULL))
        {
            hr = E_FAIL;
        }
        CloseTempFile();
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::LockRequest (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::UnlockRequest()
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::UnlockRequest\n", this));
    HRESULT hr = NOERROR;

    //CloseTempFile();

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::UnlockRequest (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::CResProtocol
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
CResProtocol::CResProtocol(REFCLSID rclsid, IUnknown *pUnkOuter, IUnknown **ppUnkInner) : CBaseProtocol(rclsid, pUnkOuter, ppUnkInner)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::CResProtocol \n", this));

    _hInst = NULL;
    _cbBuffer = 0;
    _pBuffer = 0;
    _hgbl = 0;
    _cbPos = 0;

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::CResProtocol \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::~CResProtocol
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CResProtocol::~CResProtocol()
{

    if (_hgbl)
    {
        UnlockResource(_hgbl);
    }
    if (_hInst)
    {
        FreeLibrary(_hInst);
    }

    EProtDebugOut((DEB_PLUGPROT, "%p _IN/OUT CResProtocol::~CResProtocol \n", this));
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::GetResource
//
//  Synopsis:
//
//  Arguments:  [pwzFileName] --
//              [pwzResName] --
//              [pwzResType] --
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::GetResource(LPCWSTR pwzFileName, LPCWSTR pwzResName, LPCWSTR pwzResType, LPCWSTR pwzMime)
{
    EProtDebugOut((DEB_PLUGPROT, "CResProtocol::GetResource\n)"));
    HRESULT     hr = NOERROR;
    DWORD       dwError = 0;
    HRSRC       hrsrc;

    LPSTR pszFileName =  DupW2A(pwzFileName );
    LPSTR pszResName  =  DupW2A(pwzResName  );
    LPSTR pszResType  =  DupW2A(pwzResType  );


    if (!pszFileName || !pszResName || !pszResType)
    {
        hr = E_OUTOFMEMORY;
    }
    else do
    {
        _pProtSink->ReportProgress(BINDSTATUS_SENDINGREQUEST, pwzResName);

        _hInst = LoadLibraryEx(pszFileName, NULL, DONT_RESOLVE_DLL_REFERENCES);
        if (!_hInst)
        {
            hr = INET_E_RESOURCE_NOT_FOUND;
            dwError = GetLastError();
            break;
        }

        if (!wcscmp(pwzResName, L"?"))
        {
            for (int i = 0 ; i < MAX_ID; i++)
            {
                hrsrc = FindResource(_hInst, (LPSTR)MAKEINTRESOURCE(i), pszResType);

                if (hrsrc)
                {
                    EProtDebugOut((DEB_PLUGPROT, "CResProtocol::GetResource (szResName:#%ld, wzRestype:%s\n)",i,pszResType));
                    i = MAX_ID;
                }
            }
        }
        else
        {
            hrsrc = FindResource(_hInst, pszResName, pszResType);
        }

        if (!hrsrc)
        {
            hr = INET_E_OBJECT_NOT_FOUND;
            dwError = GetLastError();
            break;
        }

        _hgbl = LoadResource(_hInst, hrsrc);
        if (!_hgbl)
        {
            hr = INET_E_DATA_NOT_AVAILABLE;
            dwError = GetLastError();
            break;
        }

        _pBuffer = LockResource(_hgbl);
        if (!_pBuffer)
        {
            hr = INET_E_DATA_NOT_AVAILABLE;
            dwError = GetLastError();
            break;
        }

        _cbBuffer = SizeofResource(_hInst, hrsrc);


        if (_grfBindF & INTERNET_FLAG_NEED_FILE)
        {
            LockRequest(0);
        }

#ifdef UNUSED
        if (OpenTempFile())
        {
            DWORD dwWrite;
            if (!WriteFile(_hFile, _pBuffer, _cbBuffer, &dwWrite,NULL))
            {
                dwError = GetLastError();
                break;
            }
            CloseTempFile();
        }
#endif //UNUSED

        hr = NOERROR;
        break;

    }  while (1);

    if (dwError || (hr != NOERROR))
    {
        _pProtSink->ReportResult(hr, dwError, 0);
    }
    else
    {
        if (pwzMime)
        {
            _pProtSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pwzMime);
        }

        _bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;
        _pProtSink->ReportData(_bscf, _cbBuffer, _cbBuffer);
    }

    if (pszFileName)
    {
        delete pszFileName;
    }

    if (pszResName)
    {
        delete pszResName;
    }

    if (pszResType)
    {
        delete pszResType;
    }


    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::GetResource (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CResProtocol::ParseAndBind
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CResProtocol::ParseAndBind(BOOL fBind)
{
    EProtDebugOut((DEB_PLUGPROT, "%p _IN CResProtocol::ParseAndBind\n", this));

    HRESULT hr = MK_E_SYNTAX;

    WCHAR wzlURL[MAX_URL_SIZE];

    wcscpy(wzlURL, _wzFullURL);


    do
    {
        // check if protocol part
        LPWSTR pwz = wcschr(wzlURL, ':');

        pwz++;

        if (wcsnicmp(pwz, L"//", 2) && wcsnicmp(pwz, L"\\\\", 2))
        {
            break;
        }

        // find the file name and path
        LPWSTR pwzFileName = pwz + 2;

        EProtAssert((pwzFileName));

        // the file is
        LPWSTR pwz1 = wcsrchr(wzlURL, '/');

        if (!pwz1)
        {
            break;
        }

        *pwz1 = '\0';
        pwz1++;

        if (!*pwz1)
        {
            break;
        }
#ifdef WITH_TAGS
        LPWSTR pwzResDelimiter = wcschr(pwz1, '?');

        if (!pwzResDelimiter)
        {
            break;
        }

        LPWSTR pwzResTag = wcsstr(pwz1, L"name:");

        if (!pwzResTag)
        {
            break;
        }

        LPWSTR pwzTypeTag = wcsstr(pwz1, L"type:");

        if (!pwzTypeTag)
        {
            break;
        }

        LPWSTR pwzMimeTag = wcsstr(pwz1, L"mime:");

        // get the resource name
        LPWSTR pwzResName = wcschr(pwzResTag, ':');
        pwzResName++;

        //find the end of the resource name
        LPWSTR pwzResType = wcschr(pwzTypeTag, ':');
        pwzResType++;

        LPWSTR pwzMime = 0;
        if (pwzMimeTag)
        {
            pwzMime = wcschr(pwzMimeTag, ':');
            pwzMime++;
            *pwzMimeTag = 0;
        }

        *pwzResTag = 0;
        *pwzTypeTag = 0;


#else
        // find the delimiter for the private part
        LPWSTR pwzResName = wcschr(pwz1, '?');

        if (!pwzResName)
        {
            break;
        }

        // get the resource name
        pwzResName++;

        //find the end of the resource name
        LPWSTR pwzResType = wcschr(pwzResName, ' ');

        if (!pwzResType)
        {
            break;
        }

        *pwzResType = '\0';
        pwzResType++;
        EProtDebugOut((DEB_PLUGPROT, "CResProtocol::GetResource (wzResName:%ws,pwzResType:%ws\n)",pwzResName,pwzResType));


        /*
        {
            pwzResType =  (LPWSTR)MAKEINTRESOURCE(RT_ICON);
            EProtDebugOut((DEB_PLUGPROT, "CResProtocol::GetResource (wzResName:%ws,pwzResType:RT_ICON\n)",pwzResName));
        }
        */
#endif //WITH_TAGS


        EProtAssert(((WCHAR *)pwzFileName && (WCHAR *)pwzResName));
        if (fBind && pwzFileName && pwzResName && pwzResType)
        {
            hr = GetResource(pwzFileName, pwzResName, pwzResType, pwzMime);
        }

        break;

    }  while (1);


    if (hr == MK_E_SYNTAX)
    {
        _pProtSink->ReportResult(hr, 0, 0);
    }

    EProtDebugOut((DEB_PLUGPROT, "%p OUT CResProtocol::ParseAndBind (hr:%lx)\n", this,hr));
    return hr;
}


