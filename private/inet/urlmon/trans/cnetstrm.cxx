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
#include <trans.h>


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

   ReportNotification(BINDSTATUS_SENDINGREQUEST);

   // need one of these
   //
   if (FAILED(hr = CreateBindCtx(0,&pbc)))
      goto End;

   //   form is mk:@progid:moniker
   //
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

   TransAssert((pParser));

   if (FAILED(hr = pParser->ParseDisplayName(pbc,wzDisplayName,&cchE,&pmk)))
   {
       goto End;
   }

   TransAssert((pmk));

#endif

   if (FAILED(hr = pmk->BindToStorage(pbc,0,IID_IStream,(void**)&pstm)))
   {
      hr = INET_E_RESOURCE_NOT_FOUND;
      goto End;
   }

   TransAssert((hr != MK_S_ASYNCHRONOUS));
   TransAssert((pstm));

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

   hr = _pCTrans->ReportData(BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION, stat.cbSize.LowPart,stat.cbSize.LowPart);

   _hrError = INET_E_DONE;

End:

   if (FAILED(hr))
   {
        SetCNetBindResult(GetLastError());
        _hrError = INET_E_RESOURCE_NOT_FOUND;
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
   TransAssert((hr != E_NOTIMPL));

   return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetStream::ReadDataHere
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
HRESULT CINetStream::ReadDataHere(BYTE *pBuffer, DWORD cbBytes, DWORD *pcbBytes)
{
    TransDebugOut((DEB_PROT, "%p _IN CINetStream::ReadDataHere\n", this));
    HRESULT hr = E_FAIL;

    TransAssert((cbBytes && pcbBytes));


    if (_pstm)
    {
        hr = _pstm->Read(pBuffer,cbBytes,pcbBytes);
    }

    TransDebugOut((DEB_PROT, "%p OUT CINetStream::ReadDataHere (_hrError:%lx, [hr:%lx,cbBytesAsked:%ld,cbBytesReturned:%ld]) \n",
                                    this, _hrError, hr, cbBytes, *pcbBytes));
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
    TransDebugOut((DEB_PROT, "%p _IN CINetStream::INetSeek\n", this));
    HRESULT hr = E_FAIL;

    if (_pstm)
    {
        hr = _pstm->Seek(dlibMove, dwOrigin, plibNewPosition);
    }

    TransDebugOut((DEB_PROT, "%p OUT CINetStream::INetSeek (hr:%lx) \n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p _IN CINetStream::LockFile\n", this));
    HRESULT hr = NOERROR;

    // nothing to do for now

    TransDebugOut((DEB_PROT,"%p OUT CINetStream::LockFile (hr:%lx)\n",this, hr));
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
    TransDebugOut((DEB_PROT, "%p IN CINetStream::UnlockFile\n", this));
    HRESULT hr = NOERROR;

    // nothing to do for now

    TransDebugOut((DEB_PROT,"%p OUT CINetStream::UnlockFile (hr:%lx)\n", this, hr));
    return hr;
}

