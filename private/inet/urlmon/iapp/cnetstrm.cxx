//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cnetstrm.cxx
//
//  Contents:   Implements the stream: protocol
//
//  Classes:    CINetStream
//
//  Functions:
//
//  History:    5/3/96 Created Craig Critchley [craigc]
//
//----------------------------------------------------------------------------
#include <iapp.h>
#include <shlwapip.h>

PerfDbgTag(tagCINetStream, "Urlmon", "Log CINetStream", DEB_PROT);

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::CINetStream
//
//  Synopsis:   Constructs a stream protcol object
//
//  Arguments:  rclsid
//
//  Returns:
//
//  History:    5/3/96 Created Craig Critchley [craigc]
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetStream::CINetStream(REFCLSID rclsid, IUnknown *pUnkOuter) : CINet(rclsid,pUnkOuter)
{
    PerfDbgLog(tagCINetStream, this, "CINetStream::CINetStream");
    _dwIsA = DLD_PROTOCOL_STREAM;
    _pstm = NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::~CINetStream
//
//  Synopsis:   destroys a stream protocol object
//
//  Arguments:
//
//  Returns:
//
//  History:    5/3/96 Created Craig Critchley [craigc]
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetStream::~CINetStream()
{
    PerfDbgLog(tagCINetStream, this, "CINetStream::~CInetStream");
    if (_pstm)
    {
        _pstm->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::INetAsyncOpen
//
//  Synopsis:   opens and synchronously downloads data from a stream
//
//  Arguments:  rclsid
//
//  Returns:
//
//  History:    5/3/96 Created Craig Critchley [craigc]
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetStream::INetAsyncOpen()
{
    PerfDbgLog(tagCINetStream, this, "+CINetStream::INetAsyncOpen");
    IBindCtx * pbc = 0;
    IMoniker * pmk = 0;
    IStream * pstm = 0;
    IParseDisplayName * pParser = 0;
    STATSTG stat;
    CLSID clsid;
    CHAR szDisplayName[MAX_PATH];
    WCHAR wzDisplayName[MAX_PATH];
    WCHAR wzProgId[MAX_PATH];
    LPSTR pa, psz;
    LPWSTR pwz, pwzI;
    int cch;
    HRESULT hr = E_NOTIMPL;
    ULONG cchE;
    BOOL fGotMIMEType = FALSE;
    ULONG cchServerName, cchObjectName;

    ReportNotification(BINDSTATUS_SENDINGREQUEST);

    // need one of these
    //
    if (FAILED(hr = CreateBindCtx(0,&pbc)))
        goto End;

    //   form is mk:@progid:moniker
    //
    cchServerName = strlen(GetServerName());
    cchObjectName = strlen(GetObjectName());

    if ((cchServerName + cchObjectName) >= MAX_PATH)
    {
        hr = E_FAIL;
        goto End;
    }

    strcpy(szDisplayName,GetServerName());
    strcat(szDisplayName,psz = GetObjectName());

    // if the moniker has a file extension, try to
    // determine the MIME type that way...
    //
    psz = FindFileExtension(psz);
    if (psz)
    {
        char szMime[MAX_PATH];
        DWORD cb = MAX_PATH;

        if (SUCCEEDED(GetMimeFromExt(psz,szMime,&cb)))
        {
            ReportNotification(BINDSTATUS_MIMETYPEAVAILABLE, szMime);
            fGotMIMEType = TRUE;
        }
    }

    A2W(szDisplayName,wzDisplayName,MAX_PATH);

    //   find progid
    //
    for (pwz = wzDisplayName, pwzI = wzProgId; *pwz; pwz++)
    {
        if (*pwz == '@')
        {
            pwzI = wzProgId;
        }
        else if (*pwz == ':')
        {
            *pwzI = 0;

            // the remainder may have a filename with a useful
            // extension... just in case, set the filename...
            //
            LPSTR pszStr = DupW2A(pwz+1);
            if (pszStr)
            {
                ReportNotification(BINDSTATUS_CACHEFILENAMEAVAILABLE, pszStr);
                delete pszStr;
            }

            break;
        }
        else
        {
            *pwzI++ = *pwz;
        }
    }

#if 0
    if (FAILED(hr = MkParseDisplayName(pbc,wzDisplayName,&cchE,&pmk)) && pmk)
        goto End;
#else
    // BUGBUG
    //
    //   MkParseDisplayName was opening another instance of app
    //   force inproc server.  ick.
    //

    if (FAILED(hr = CLSIDFromProgID(wzProgId,&clsid)))
    {
        goto End;
    }

    if (FAILED(hr = CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER,
                                     IID_IParseDisplayName,(void**)&pParser)))
    {
        goto End;
    }

    PProtAssert((pParser));

    if (FAILED(hr = pParser->ParseDisplayName(pbc,wzDisplayName,&cchE,&pmk)))
    {
        goto End;
    }

    PProtAssert((pmk));

#endif

    if (FAILED(hr = pmk->BindToStorage(pbc,0,IID_IStream,(void**)&pstm)))
    {
        hr = INET_E_RESOURCE_NOT_FOUND;
        goto End;
    }

    PProtAssert((hr != MK_S_ASYNCHRONOUS));
    PProtAssert((pstm));

    _pstm = pstm;
    _pstm->AddRef();

    // now we have a stream - stuff it into the trans data
    //
    if (FAILED(hr = pstm->Stat(&stat,STATFLAG_NONAME)))
    {
        goto End;
    }

    _cbTotalBytesRead = stat.cbSize.LowPart;
    _cbDataSize = stat.cbSize.LowPart;

    ReportResultAndStop(NOERROR, _cbTotalBytesRead, _cbDataSize );

    _hrError = INET_E_DONE;

    End:

    if (FAILED(hr))
    {
        SetCNetBindResult(GetLastError());
        hr = _hrError = INET_E_RESOURCE_NOT_FOUND;
        ReportResultAndStop(_hrError);
    }

    // play nice, no leaks
    //
    if (pParser)
    {
        pParser->Release();
    }

    if (pmk)
    {
        pmk->Release();
    }
    if (pbc)
    {
        pbc->Release();
    }
    if (pstm)
    {
        pstm->Release();
    }

    // make sure I set this any way I get out
    //
    PProtAssert((hr != E_NOTIMPL));

    PerfDbgLog1(tagCINetStream, this, "-CINetStream::INetAsyncOpen (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::Read
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBytes] --
//              [pcbBytes] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINetStream::Read(void *pBuffer, DWORD cbBytes, DWORD *pcbBytes)
{
    PerfDbgLog(tagCINetStream, this, "+CINetStream::Read");
    HRESULT hr = E_FAIL;

    PProtAssert((cbBytes && pcbBytes));


    if (_pstm)
    {
        hr = _pstm->Read(pBuffer,cbBytes,pcbBytes);
    }

    PerfDbgLog4(tagCINetStream, this, "-CINetStream::Read (_hrError:%lx, [hr:%lx,cbBytesAsked:%ld,cbBytesReturned:%ld])",
        _hrError, hr, cbBytes, *pcbBytes);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::INetSeek
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
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetStream::INetSeek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagCINetStream, this, "+CINetStream::INetSeek");
    HRESULT hr = E_FAIL;

    if (_pstm)
    {
        hr = _pstm->Seek(dlibMove, dwOrigin, plibNewPosition);
    }

    PerfDbgLog1(tagCINetStream, this, "-CINetStream::INetSeek (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::LockFile
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    8-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetStream::LockFile(BOOL fRetrieve)
{
    PerfDbgLog(tagCINetStream, this, "+CINetStream::LockFile");
    HRESULT hr = NOERROR;

    // nothing to do for now

    PerfDbgLog1(tagCINetStream, this, "-CINetStream::LockFile (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::UnlockFile
//
//  Synopsis:   unlocks the file
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetStream::UnlockFile()
{
    PerfDbgLog(tagCINetStream, this, "+CINetStream::UnlockFile");
    HRESULT hr = NOERROR;

    // nothing to do for now

    PerfDbgLog1(tagCINetStream, this, "-CINetStream::UnlockFile (hr:%lx)", hr);
    return hr;
}
