//+----------------------------------------------------------------------------
//  File:       persist.cxx
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------


// Includes -------------------------------------------------------------------
#include <mgr.hxx>
#include <factory.hxx>
#include "wininet.h"

// Constants ------------------------------------------------------------------
const LARGE_INTEGER LIB_ZERO   = { 0, 0 };
const ULONG BUFFER_SIZE        = 256;
const ULONG CHARS_PER_LINE     = 65;
const WCHAR LPKPATH[]          = L"LPKPath";
const TCHAR SZ_URLMON[]        = _T("URLMON.DLL");
const TCHAR SZ_ISVALIDURL[]    = _T("IsValidURL");
const TCHAR SZ_URLDOWNLOADTOCACHEFILE[] = _T("URLDownloadToCacheFileW");

typedef HRESULT (STDMETHODCALLTYPE *ISVALIDURL)(LPBC, LPCWSTR, DWORD);
typedef HRESULT (STDMETHODCALLTYPE *URLDOWNLOADTOCACHEFILE)(LPUNKNOWN,LPCWSTR,LPWSTR,
                                                            DWORD,DWORD,LPBINDSTATUSCALLBACK);


//+----------------------------------------------------------------------------
//
//  Member:     FindInStream
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
HRESULT
CLicenseManager::FindInStream(
    IStream *           pstm,
    BYTE *              pbData,
    ULONG               cbData)
{
    BYTE    bByte;
    ULONG   cbRead;
    ULONG   ibData = 0;
    HRESULT hr;

    Assert(pstm);
    Assert(pbData);
    Assert(cbData);

    // Read through the stream looking for the data
    for (;;)
    {
        // Read a byte of data
        hr = pstm->Read(&bByte, sizeof(BYTE), &cbRead);
        if (hr)
            goto Cleanup;
        if (!cbRead)
            break;

        if (bByte == pbData[ibData])
        {
            ibData++;
            if (ibData >= cbData)
                break;
        }
    }

    // If the data was found, return success
    hr = (ibData == cbData
                ? S_OK
                : E_FAIL);

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetClassID
//
//  Synopsis:   Return the object's CLSID
//
//  Arguments:  pclsid - Location at which to return the object's CLSID
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetClassID(
    CLSID * pclsid)
{
    if (!pclsid)
        return E_INVALIDARG;

    *pclsid = CLSID_LicenseManager;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     IsDirty
//
//  Synopsis:   Return whether the object is dirty or not
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::IsDirty()
{
    return (_fDirty
                ? S_OK
                : S_FALSE);
}


//+----------------------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::Load(
    IStream *   pstm)
{
    CBufferedStream bstm(pstm);
    LARGE_INTEGER   libCur;
    ULARGE_INTEGER  uibSize;
    HRESULT         hr;

    Assert(_fPersistStream || _fPersistPBag);

    if (!pstm)
        return E_INVALIDARG;

    if (_fLoaded)
        return E_UNEXPECTED;

    // Prepare the buffered stream for use
    hr = bstm.SetBufferSize(BUFFER_SIZE);
    if (hr)
        goto Cleanup;

    // Determine the size of the stream
    hr = pstm->Seek(LIB_ZERO, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&libCur);
    if (hr)
        goto Cleanup;
    hr = pstm->Seek(LIB_ZERO, STREAM_SEEK_END, &uibSize);
    if (hr)
        goto Cleanup;
    hr = pstm->Seek(libCur, STREAM_SEEK_SET, NULL);
    if (hr)
        goto Cleanup;

    // Load from the buffered stream
    Assert(!uibSize.HighPart);
    hr = Load(&bstm, uibSize.LowPart);

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
HRESULT
CLicenseManager::Load(
    IStream *   pstm,
    ULONG       cbSize)
{
    CMemoryStream   mstm;
    ULARGE_INTEGER  uibSize = { cbSize, 0 };
    char *          psz = NULL;
    OLECHAR *       polechar = NULL;
    DWORD           cch;
    DWORD           cchMax = 0;
    DWORD           cLic;
    DWORD           iLic;
    BOOL            fSkipClass;
    HRESULT         hr;

    // Scan for the LPK version identifier and skip over it
    cch = ::lstrlenA(g_pszLPKVersion1);
    hr = FindInStream(pstm, (BYTE *)g_pszLPKVersion1, cch);
    if (hr)
        goto Cleanup;

    // Allocate a memory-based stream to hold the binary data
    hr = mstm.SetSize(uibSize);
    if (hr)
        goto Cleanup;

    // Convert and load the LPK identifier
    hr = DecodeMIME64(pstm, &mstm, NULL);
    if (hr)
        goto Cleanup;
    Verify(SUCCEEDED(mstm.Seek(LIB_ZERO, STREAM_SEEK_SET, NULL)));
    hr = mstm.Read((void *)&_guidLPK, sizeof(_guidLPK), NULL);
    if (hr)
        goto Cleanup;

    // Convert and load the number of CLSID-License pairs
    Verify(SUCCEEDED(mstm.Seek(LIB_ZERO, STREAM_SEEK_SET, NULL)));
    hr = DecodeMIME64(pstm, &mstm, NULL);
    if (hr)
        goto Cleanup;
    Verify(SUCCEEDED(mstm.Seek(LIB_ZERO, STREAM_SEEK_SET, NULL)));
    hr = mstm.Read((void *)&cLic, sizeof(DWORD), NULL);
    if (hr)
        goto Cleanup;
    hr = _aryLic.SetSize((int)cLic);
    if (hr)
        goto Cleanup;
    ::memset((LICENSE *)_aryLic, 0, sizeof(LICENSE)*cLic);

    // Convert the remainder of the stream and from it load each CLSID-License pair
    // (If, somehow, invalid CLSIDs end up in the stream, skip over them during load)
    for (iLic = 0; iLic < cLic; )
    {
        Verify(SUCCEEDED(mstm.Seek(LIB_ZERO, STREAM_SEEK_SET, NULL)));
        hr = DecodeMIME64(pstm, &mstm, NULL);
        if (hr)
            goto Cleanup;
        Verify(SUCCEEDED(mstm.Seek(LIB_ZERO, STREAM_SEEK_SET, NULL)));

        hr = mstm.Read((void *)&(_aryLic[iLic].clsid), sizeof(_aryLic[0].clsid), NULL);
        if (hr)
            goto Cleanup;

        fSkipClass = (_aryLic[iLic].clsid == CLSID_NULL);

        hr = mstm.Read((void *)&cch, sizeof(DWORD), NULL);
        if (hr)
            goto Cleanup;

        if (cch > cchMax)
        {
            delete [] psz;
            delete [] polechar;

            psz      = new char[cch*sizeof(OLECHAR)]; // Review:JulianJ
            polechar = new OLECHAR[cch];
            if (!psz || !polechar)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            cchMax = cch;
        }
        Assert(psz);
        Assert(polechar);

		//
		// REVIEW JulianJ read cch*2 bytes as we persisted entire string
		//
        hr = mstm.Read((void *)psz, cch*sizeof(OLECHAR), NULL);
        if (hr)
            goto Cleanup;

        if (!fSkipClass)
        {
#if 1
			::memcpy(polechar, psz, cch*sizeof(OLECHAR));
#else
#ifndef _PPCMAC
            ::MultiByteToWideChar(CP_ACP, 0, psz, cch, polechar, cch);
#else
            ::memcpy(polechar, psz, cch);
#endif
#endif
            _aryLic[iLic].bstrLic = ::SysAllocStringLen(polechar, cch);
            if (!_aryLic[iLic].bstrLic)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            iLic++;
        }
        else
        {
            cLic--;
        }
    }

    // Ensure the array size is correct (in case any classes were skipped during load)
    if (cLic < (DWORD)_aryLic.Size())
    {
        Verify(SUCCEEDED(_aryLic.SetSize(cLic)));
    }

Cleanup:
    delete [] psz;
    delete [] polechar;
    _fLoaded = TRUE;
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::Save(
    IStream *   pstm,
    BOOL        fClearDirty)
{
    CBufferedStream bstm(pstm, CHARS_PER_LINE, FALSE);
    ULARGE_INTEGER  uibCur;
    TCHAR           szString[MAX_PATH];
    DWORD           cbBuf = 0;
    DWORD           cbLic;
    DWORD           cb;
    BYTE *          pb = NULL;
    BYTE *          pbNext;
    int             iLic;
    HRESULT         hr = S_OK;

    Assert(_fPersistStream);

    if (!pstm)
        return E_INVALIDARG;

    // If this is a new LPK, generate an identifying GUID
    if (_guidLPK == GUID_NULL)
    {
        Assert(!_fLoaded);
        Verify(SUCCEEDED(::CoCreateGuid(&_guidLPK)));
    }

    // Write the text header to the LPK
    for (iLic=IDS_COPYTEXT; iLic <= IDS_COPYTEXT_MAX; iLic++)
    {
        cb = ::LoadString((HINSTANCE)g_hinst, iLic, szString, ARRAY_SIZE(szString));

        hr = pstm->Write(szString, cb, NULL);
        if (hr)
            goto Cleanup;
        hr = pstm->Write(SZ_NEWLINE, CB_NEWLINE, NULL);
        if (hr)
            goto Cleanup;
    }

    // Write the version GUID to the LPK
    hr = pstm->Write(g_pszLPKVersion1, ::lstrlenA(g_pszLPKVersion1), NULL);
    if (hr)
        goto Cleanup;
    hr = pstm->Write(SZ_NEWLINE, CB_NEWLINE, NULL);
    if (hr)
        goto Cleanup;

    // Prepare the buffered stream as the target for encoding
    hr = bstm.SetBufferSize(BUFFER_SIZE);
    if (hr)
        goto Cleanup;
    
    // Write the identifying GUID to the LPK
    hr = EncodeMIME64((BYTE *)&_guidLPK, sizeof(_guidLPK), &bstm, NULL);
    if (hr)
        goto Cleanup;
    hr = bstm.Write(SZ_NEWLINE, CB_NEWLINE, NULL);
    if (hr)
        goto Cleanup;

    // Write the number of CLSID-License pairs to the LPK
    cb = (DWORD)_aryLic.Size();
    hr = EncodeMIME64((BYTE *)&cb, sizeof(cb), &bstm, NULL);
    if (hr)
        goto Cleanup;
    hr = bstm.Write(SZ_NEWLINE, CB_NEWLINE, NULL);
    if (hr)
        goto Cleanup;

    // Write each CLSID-License pair to the LPK
    // (If the array contains empty entries, they are still persisted; this is necessary
    //  because the number of entries persisted must match the count already written)
    for (iLic = 0; iLic < _aryLic.Size(); iLic++)
    {
        // Determine the amount of class data and ensure the buffer is sufficiently large
        cbLic = ::SysStringLen(_aryLic[iLic].bstrLic);
        cb    = sizeof(CLSID) + sizeof(DWORD) + (sizeof(OLECHAR) * cbLic);

        if (cb > cbBuf)
        {
            cbBuf = cb;
            delete [] pb;
            pb = new BYTE[cbBuf];
            if (!pb)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        pbNext = pb;

        // Fill the buffer with the persistent state of the class
        *((CLSID *)pbNext) = _aryLic[iLic].clsid;
        pbNext += sizeof(CLSID);

        *((DWORD *)pbNext) = cbLic;
        pbNext += sizeof(DWORD);

		//
		// REVIEW JulianJ, Weird - we seem to get back a length prefixed ansi string!
		//
#if 1
		memcpy(pbNext, _aryLic[iLic].bstrLic, cbLic * (sizeof(OLECHAR)));
#else
#ifndef _PPCMAC
        ::WideCharToMultiByte(CP_ACP, 0, _aryLic[iLic].bstrLic, cbLic, (LPSTR)pbNext, cbLic, NULL, NULL);
#else
        ::memcpy(pbNext, _aryLic[iLic].bstrLic, cbLic);
#endif
#endif
        // Encode the class to the stream
        hr = EncodeMIME64(pb, cb, &bstm, NULL);
        if (hr)
            goto Cleanup;
        hr = bstm.Write(SZ_NEWLINE, CB_NEWLINE, NULL);
        if (hr)
            goto Cleanup;
    }

    // Flush the buffered stream and mark the end of data
    // (Since not all streams support Seek and SetSize, errors from those methods
    //  are ignored; since the stream contains a count, it can be safely loaded
    //  without truncating unnecessary bytes)
    hr = bstm.Flush();
    if (hr)
        goto Cleanup;
    Verify(SUCCEEDED(pstm->Seek(LIB_ZERO, STREAM_SEEK_CUR, &uibCur)));
    Verify(SUCCEEDED(pstm->SetSize(uibCur)));

Cleanup:
    delete [] pb;
    _fLoaded = TRUE;
    _fDirty  = !fClearDirty && SUCCEEDED(hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetSizeMax
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::GetSizeMax(
    ULARGE_INTEGER *    pcbSize)
{
    if (!pcbSize)
        return E_INVALIDARG;

    pcbSize->LowPart  =
    pcbSize->HighPart = 0;
    return E_NOTIMPL;
}


//+----------------------------------------------------------------------------
//
//  Member:     InitNew
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::InitNew()
{
    if (_fLoaded)
        return E_UNEXPECTED;
    _fLoaded = TRUE;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::Load(
    IPropertyBag *  pPropBag,
    IErrorLog *     pErrorLog)
{
    HMODULE hURLMon = NULL;
    VARIANT var;
    HRESULT hr;

    Assert(_fPersistPBag);

    if (!pPropBag)
        return E_INVALIDARG;

    if (_fLoaded)
        return E_UNEXPECTED;

    ::VariantInit(&var);
    V_VT(&var)   = VT_BSTR;
    V_BSTR(&var) = NULL;

    // Read the path from which to load the LPK file
    hr = pPropBag->Read(LPKPATH, &var, pErrorLog);
    if (!hr)
    {
        CFileStream     fstm;
        FARPROC         fpIsValidURL;
        FARPROC         fpURLDownloadToCacheFileW;
        WCHAR           szCachedFilename[MAX_PATH];

        // Load the URL moniker library
        hURLMon = (HMODULE)::LoadLibrary(SZ_URLMON);
        if (!hURLMon)
        {
            hr = GetWin32Hresult();
            goto Cleanup;
        }

        // Check the path, if it is for an absolute URL, reject it
        // (Only relative URLs are accepted)
        fpIsValidURL = GetProcAddress(hURLMon, SZ_ISVALIDURL);
        if (!fpIsValidURL)
        {
            hr = GetWin32Hresult();
            goto Cleanup;
        }
        hr = (*((ISVALIDURL)fpIsValidURL))(NULL, V_BSTR(&var), 0);
        if (hr == S_OK)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }


        // Download the .LPK to a locally cached file
        fpURLDownloadToCacheFileW = GetProcAddress(hURLMon, SZ_URLDOWNLOADTOCACHEFILE);
        if (!fpURLDownloadToCacheFileW)
        {
            hr = GetWin32Hresult();
            goto Cleanup;
        }

		//
		// Get the service provider from our site
		//
		IServiceProvider * pServiceProvider;
		hr = GetSite(IID_IServiceProvider, (void**)&pServiceProvider);
		if (!SUCCEEDED(hr))
			goto Cleanup;

		//
		// Get an IBindHost from the service provider
		//
		IBindHost *pBindHost;
		hr = pServiceProvider->QueryService( 
			SID_IBindHost, IID_IBindHost, (void**)&pBindHost);
		pServiceProvider->Release();

		if (!SUCCEEDED(hr))
			goto Cleanup;

		//
		// Now create a full moniker
		//
		IMoniker *pMoniker;
		hr = pBindHost->CreateMoniker(V_BSTR(&var), NULL, &pMoniker,0);
		pBindHost->Release();
		
		if (!SUCCEEDED(hr))
			goto Cleanup;

		//
		// Create a bind context
		//
		IBindCtx * pBindCtx;
		hr = CreateBindCtx(0, &pBindCtx);
		if (!SUCCEEDED(hr))
		{
			pMoniker->Release();
			goto Cleanup;
		}

		//
		// Extract display name
		//
		LPOLESTR wszFullLPKPath;
		hr = pMoniker->GetDisplayName(pBindCtx, NULL, &wszFullLPKPath);
		pMoniker->Release();
		pBindCtx->Release();

		if (!SUCCEEDED(hr))
			goto Cleanup;

        hr = (*((URLDOWNLOADTOCACHEFILE)fpURLDownloadToCacheFileW))(
				_pUnkOuter,
				wszFullLPKPath,
				szCachedFilename,
				URLOSTRM_GETNEWESTVERSION,
				0, NULL);

		CoTaskMemFree(wszFullLPKPath);

        if (hr)
            goto Cleanup;

        // Open a stream on the file and load from the stream
        hr = fstm.Init(szCachedFilename, GENERIC_READ);
        if (!hr)
        {
            CBufferedStream mstm(&fstm);
            ULONG           cbSize;

            hr = mstm.SetBufferSize(BUFFER_SIZE);
            if (hr)
                goto Cleanup;

            Verify(SUCCEEDED(fstm.GetFileSize(&cbSize)));
            hr = Load(&mstm, cbSize);
        }
    }

Cleanup:
    _fLoaded = TRUE;
    ::VariantClear(&var);
    if (hURLMon)
    {
        ::FreeLibrary(hURLMon);
    }
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CLicenseManager::Save(
    IPropertyBag *  pPropBag,
    BOOL            fClearDirty,
    BOOL            fSaveAllProperties)
{
    UNREF(pPropBag);
    UNREF(fClearDirty);
    UNREF(fSaveAllProperties);
    return E_NOTIMPL;
}
