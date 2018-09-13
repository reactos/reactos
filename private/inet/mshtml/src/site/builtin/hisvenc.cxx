//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       hisvenc.cxx
//
//  Contents:   History value encoder
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_HISVENC_HXX_
#define X_HISVENC_HXX_
#include "hisvenc.hxx"
#endif

MtDefine(CHistValEncReader, Utilities, "CHistValEncReader")
MtDefine(CHistValEncWriter, Utilities, "CHistValEncWriter")
MtDefine(CHisValConverter,  Utilities, "CHisValConverter")
MtDefine(CHistValEncReaderPchBuf, CHistValEncReader, "CHistValEncReader::_pHistValBuffer")

HRESULT
CHistValEncWriter::UnicodeToWB(TCHAR * pchIn)
{
    HRESULT hr=S_OK;
    int     cch;

    Assert(pchIn);

    // So MultiByteFromWideChar knows what to convert
    _pchBuffer = (TCHAR *)pchIn;
    _cchBuffer = _cchBufferMax = _tcslen(pchIn) + 1;

    hr = THR(MultiByteFromWideChar(FALSE, &cch));

    // Do this to prevent the destructor from trying to
    // free _pchBuffer
     _pchBuffer = NULL;
     _cchBuffer = _cchBufferMax = 0;

    return hr; 
}

HRESULT
CHistValEncReader::WBToUnicode(unsigned char * pchIn)
{
    HRESULT hr=S_OK;
    int     cch;

    Assert(pchIn);

    // So MultiByteFromWideChar knows what to convert
    _pbBuffer = _pbBufferPtr = pchIn;
    _cbBuffer = _cbBufferMax = lstrlenA((const char *)pchIn) + 1;

    hr = THR(WideCharFromMultiByte(FALSE, &cch));

    _pbBuffer = NULL;   // no need to free the writer's memory

    return hr; 
}

HRESULT
CHistValEncReader::MakeRoomForChars(int cch)
{
    if (!_pchBuffer)
    {
        _cchBuffer = _cbBufferMax;      // make the room big enough

        _pchBuffer = (TCHAR *)MemAlloc( Mt(CHistValEncReaderPchBuf),
                                        _cchBuffer * sizeof(TCHAR));
        if (!_pchBuffer)
            RRETURN( E_OUTOFMEMORY );

        _pchEnd = _pchBuffer;

        *_pchEnd = _T('\0');
    }
    else
    {
        AssertSz(FALSE, "CHistValEncReader::MakeRoomForChars : room should be enough");
    }

    CEncodeReader::MakeRoomForChars(cch);
    RRETURN(S_OK);
}

HRESULT
CHisValConverter::Convert(CStr &cstr, CODEPAGE cpFrom, CODEPAGE cpTo)
{
    HRESULT hr = S_OK;
    CHistValEncReader   hvReader(cpTo);
    CHistValEncWriter   hvWriter(cpFrom);

    hr = hvWriter.UnicodeToWB(cstr);

    if (FAILED(hr))
        goto Cleanup;

    hvReader.WBToUnicode((unsigned char *)hvWriter._pbBuffer);

    if (FAILED(hr))
        goto Cleanup;

    cstr.Set(hvReader._pchBuffer);

Cleanup:
    return hr;
}
