//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       datastm.cxx
//
//  Contents:   CDataStream IStream wrapper
//
//  History:    04-22-1997   DBau (David Bau)    Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DATASTM_HXX_
#define X_DATASTM_HXX_
#include "datastm.hxx"
#endif

#ifndef X_SUBSTM_HXX_
#define X_SUBSTM_HXX_
#include "substm.hxx"
#endif

DeclareTag(tagDataStream, "DataStream", "DataStream methods")
MtDefine(CDataStream, Utilities, "CDataStream")
MtDefine(CDataStream_aryLocations_pv, CDataStream, "CDataStream::_aryLocations::_pv")

// String lengths are written twice, once xor'ed with a cookie, in order to
// lower the probability that we read a huge length and run out of
// memory when the read stream doesn't match the written stream.

#define DWORD_STRING_COOKIE (0xABACADAB)
#define DWORD_CSTR_COOKIE   (0xC0FFEE25)


//+------------------------------------------------------------------------
//
//  Class:      CDataStream
//
//              For robust and convenient saving and loading on a stream.
//
//              SaveData/LoadData - persists a block of bytes of known size;
//                          unlike IStream::Read, CDataStream::LoadData never
//                          succeeds on a partial read.
//
//              SaveDataNow/SaveDataLater - like SaveData, but used to save
//                          out of order. SaveDataLater reserves a position in
//                          the stream, and SaveDataNow writes the data into
//                          a reserved position.
//
//              SaveString/LoadString - for saving and loading a null-
//                          terminated, MemAlloc'ed unicode string.
//
//              SaveCStr/LoadCStr - for saving and loading a CStr.
//
//              BeginSaveSubstream/EndSaveSubstream - creates a
//                          substream of the main stream for writing;
//                          (the substream correctly handles things like
//                          setsize, stat, and seek)
//                          After writing is finished, EndSaveSubstream
//                          detaches the substream from the main stream
//                          and saves the stream length.
//                          The substream must be released after use.
//
//              LoadSubstream - creates a read-only substream (a clone)
//                          which has the data previously saved by
//                          *SaveSubstream.
//                          The substream must be released after use.
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::SaveData
//
//  Synopsis:   Wrapper for IStream::Write
//
//-------------------------------------------------------------------------
HRESULT CDataStream::SaveData(void *pv, ULONG cb)
{
    HRESULT hr;
    ULONG cbWritten;
    
    Assert(pv);
    Assert(!_pSubstream); // Cannot save data in the middle of a substream operation

    //
    // BUGBUG (lmollico) - for some reason on NT5 we are getting
    // here and _pStream is equal to 1, which is bad. This is IE5 b1 bug
    // #23145
    //
    if (!_pStream)
        return E_FAIL;

    hr = THR(_pStream->Write(pv, cb, &cbWritten));
    if (hr)
        RRETURN(hr);

    if (cb != cbWritten)
    {
        Assert(0); // malformed stream
        return E_FAIL;
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::SaveDataLater
//
//  Synopsis:   Encapsulates the work of seeking back and
//              forth to save data.
//
//              BUGBUG: convenient, but not particularly efficient
//
//-------------------------------------------------------------------------
HRESULT CDataStream::SaveDataLater(DWORD *pdwCookie, ULONG cb)
{
    HRESULT hr;
    LARGEINT ib;
    CLocation loc;
    
    Assert(pdwCookie);
    Assert(!_pSubstream);

    hr = THR(_aryLocations.EnsureSize(_aryLocations.Size()+1));
    if (hr)
        goto Cleanup;

    hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &loc._ib.uli));
    if (hr)
        goto Cleanup;

    loc._cb = cb;
    loc._dwCookie = _aryLocations.Size() ? _aryLocations[_aryLocations.Size()-1]._dwCookie+1 : 0;

    *pdwCookie = loc._dwCookie;
    
    Verify(!_aryLocations.AppendIndirect(&loc));

    ib.i64 = cb;
    hr = THR(_pStream->Seek(ib.li, STREAM_SEEK_CUR, NULL));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::SaveDataNow
//
//  Synopsis:   Wrapper for IStream::Write
//
//-------------------------------------------------------------------------
HRESULT CDataStream::SaveDataNow(DWORD dwCookie, void *pv, ULONG cb)
{
    HRESULT hr = E_FAIL;
    LARGEINT ibCur;
    int c;

    Assert(pv);
    
    for (c = _aryLocations.Size(); c;)
    {
        c--;
        if (_aryLocations[c]._dwCookie == dwCookie)
        {
            hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &ibCur.uli));
            if (hr)
                goto Cleanup;
                
            hr = THR(_pStream->Seek(_aryLocations[c]._ib.li, STREAM_SEEK_SET, NULL));
            if (hr)
                goto Cleanup;
                
            Assert(_aryLocations[c]._cb == cb);
            _aryLocations.Delete(c);

            hr = THR(SaveData(pv, cb));
            if (hr)
                goto Cleanup;
                
            hr = THR(_pStream->Seek(ibCur.li, STREAM_SEEK_SET, NULL));
            if (hr)
                goto Cleanup;
                
            return S_OK;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::LoadData
//
//  Synopsis:   Wrapper for IStream::Read. Fails rather than
//              reading an incomplete chunk of data.
//
//-------------------------------------------------------------------------
HRESULT CDataStream::LoadData(void *pv, ULONG cb)
{
    HRESULT hr;
    ULONG cbRead;

    Assert(pv);

    hr = THR(_pStream->Read(pv, cb, &cbRead));
    if (hr)
        RRETURN(hr);

    if (cb != cbRead)
    {
        return E_FAIL;
    }

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CDataStream::SaveString
//
//  Synopsis:   Saves a null-terminated string by prefixing
//              the length in the stream
//
//-------------------------------------------------------------------------
HRESULT CDataStream::SaveString(TCHAR *pch)
{
    HRESULT hr;
    ULONG len;

    len = pch ? _tcslen(pch) : (DWORD)(-1);

    hr = THR(SaveDword(DWORD_STRING_COOKIE ^ len));
    if (hr)
        goto Cleanup;

    hr = THR(SaveDword(len));
    if (hr)
        goto Cleanup;

    if (pch)
    {
        hr = THR(SaveData(pch, sizeof(TCHAR) * len));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::LoadString
//
//  Synopsis:   Loads and allocates a null-terminated string
//              that was previously saved by SaveString
//
//-------------------------------------------------------------------------
HRESULT CDataStream::LoadString(TCHAR **ppch)
{
    HRESULT hr;
    ULONG len;
    DWORD dwCode;

    Assert(ppch);

    hr = THR(LoadDword(&dwCode));
    if (hr)
        goto Cleanup;

    hr = THR(LoadDword(&len));
    if (hr)
        goto Cleanup;

    if ((len ^ dwCode) != DWORD_STRING_COOKIE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(MemRealloc(Mt(CDataStream), (void**)ppch, sizeof(TCHAR)*(len + 1)));
    if (hr)
        goto Cleanup;

    if (*ppch)
    {
        hr = THR(LoadData(*ppch, sizeof(TCHAR) * len));
        if (hr)
            goto Cleanup;

        (*ppch)[len] = _T('\0');
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::SaveCStr
//
//  Synopsis:   Saves a CStr into the stream
//
//-------------------------------------------------------------------------
HRESULT CDataStream::SaveCStr(const CStr *pcstr)
{
    HRESULT hr;
    ULONG len;

    len = *pcstr ? pcstr->Length() : (DWORD)(-1);

    hr = THR(SaveDword(DWORD_CSTR_COOKIE ^ len));
    if (hr)
        goto Cleanup;

    hr = THR(SaveDword(len));
    if (hr)
        goto Cleanup;

    if (*pcstr)
    {
        hr = THR(SaveData(*pcstr, sizeof(TCHAR) * len));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::LoadCStr
//
//  Synopsis:   Loads a CStr previously saved by SaveCStr
//
//-------------------------------------------------------------------------
HRESULT CDataStream::LoadCStr(CStr *pcstr)
{
    HRESULT hr;
    ULONG len;
    ULONG dwCode;

    Assert(pcstr);

    hr = THR(LoadDword(&dwCode));
    if (hr)
        goto Cleanup;

    hr = THR(LoadDword(&len));
    if (hr)
        goto Cleanup;

    if ((len ^ dwCode) != DWORD_CSTR_COOKIE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (len == (DWORD)-1)
    {
        pcstr->Free();
    }
    else
    {
        pcstr->Set(NULL, len);
        
        hr = THR(LoadData(*pcstr, sizeof(TCHAR) * len));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::BeginSaveSubstream
//
//  Synopsis:   Returns a new sub-IStream into which arbitrary
//              data can be saved immediately. Once writes
//              into the substream are completed,
//              EndSaveSubstream must be called.
//
//-------------------------------------------------------------------------
HRESULT CDataStream::BeginSaveSubstream(IStream **ppSubstream)
{
    HRESULT hr;
    
    Assert(!_pSubstream);

    hr = THR(SaveDataLater(&_dwLengthCookie, sizeof(LARGEINT)));
    if (hr)
        goto Cleanup;

    hr = THR(CreateWritableSubstream(&_pSubstream, _pStream));
    if (hr) 
        goto Cleanup;

    _pSubstream->AddRef();
    *ppSubstream = _pSubstream;
        
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::EndSaveSubstream
//
//  Synopsis:   Completes the write of a substream into the
//              main stream (the length of the substream is
//              prefixed).
//
//-------------------------------------------------------------------------
HRESULT CDataStream::EndSaveSubstream()
{
    HRESULT hr;
    LARGEINT cb;
    
    Assert(_pSubstream);

    // Ask substream to seek to its end
    hr = THR(_pSubstream->Seek(LI_ZERO.li, STREAM_SEEK_END, &cb.uli));
    if (hr)
        goto Cleanup;

    _pSubstream->Detach(); // Ensure no further writes
    ClearInterface(&_pSubstream);
    
    // Save length
    hr = THR(SaveDataNow(_dwLengthCookie, &cb, sizeof(LARGEINT)));
    if (hr) 
        goto Cleanup;
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDataStream::LoadSubstream
//
//  Synopsis:   Returns a new substream out of which arbitrary
//              data can be read at any time.
//
//-------------------------------------------------------------------------
HRESULT CDataStream::LoadSubstream(IStream **pStreamOut)
{
    LARGEINT cb;
    CSubstream *psubstream = NULL;
    HRESULT hr;
    
    hr = THR(LoadData(&cb, sizeof(LARGEINT)));
    if (hr)
        goto Cleanup;

    hr = THR(CreateReadOnlySubstream(&psubstream, _pStream, cb.uli));
    if (hr)
        goto Cleanup;

    hr = THR(_pStream->Seek(cb.li, STREAM_SEEK_CUR, NULL));
    if (hr)
        goto Cleanup;

    *pStreamOut = psubstream;
    psubstream = NULL;

Cleanup:
    delete psubstream;
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDataStream::DumpStreamInfo
//
//  Synopsis:   Dump stream information for debugging
//
//-------------------------------------------------------------------------

#if (DBG == 1)

void CDataStream::DumpStreamInfo()
{
    HRESULT     hr;
    LARGEINT    ib;
    STATSTG     stat = { 0 };
    BYTE      * pBytes = NULL;
    BOOL        fHaveCurrent = FALSE;

    IGNORE_HR(_pStream->Stat(&stat, 0));

    TraceTag((tagDataStream, "Dumping Stream at 0x%x", _pStream));
    TraceTag((tagDataStream, "    pwcsName=%ws", stat.pwcsName ? stat.pwcsName : L"<no name>"));
    TraceTag((tagDataStream, "    cbSize(LowPart)=0x%x, cbSize(HighPart)=0x%x", 
                    stat.cbSize.LowPart, stat.cbSize.HighPart));

    if (stat.pwcsName)
        CoTaskMemFree(stat.pwcsName);
    
    // Get the current position of the stream into ib
    hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_CUR, &ib.uli));
    if (hr)
        goto Cleanup;
    fHaveCurrent = TRUE;

    // Reset the stream to the beginning
    hr = THR(_pStream->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    // BUGBUG: use the metered memory functions.
    pBytes = (BYTE *) LocalAlloc(LPTR, stat.cbSize.LowPart);
    if (!pBytes)
        goto Cleanup;

    hr = THR(_pStream->Read(pBytes, stat.cbSize.LowPart, NULL));
    if (hr)
        goto Cleanup;

    TraceTag((tagDataStream, "    Data at start of stream: >>>%.80s<<<", pBytes));

    TraceTag((tagDataStream, "    Data at end of stream: >>>%.80s<<<", 
        max(pBytes + stat.cbSize.LowPart - 80, pBytes)));

Cleanup:
    // Restore the position of the stream
    if (fHaveCurrent)
        _pStream->Seek(ib.li, STREAM_SEEK_SET, NULL);

    if (pBytes)
        LocalFree(pBytes);
    return;
}

#endif
