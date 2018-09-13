//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmsrc.cxx
//
//  Contents:   CHtmSrc - saves source HTML verbatim
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagHtmSrc,   "Dwn", "Trace CHtmInfo Source Access")

MtDefine(CHtmInfoSrcBuf, CHtmInfo, "CHtmInfo::_pbSrc")
MtDefine(CHtmInfoSrcDecodeBuf, CHtmInfo, "CHtmInfo::_pchDecoded")

// Internal -------------------------------------------------------------------

BYTE * 
FirstCRorLF(BYTE * pb, long length)
{
    Assert(pb);

    for (long i = 0; i < length; i++)
    {
        if (*pb == '\r' || *pb == '\n')
            return pb;
        pb++;
    }
    return NULL;
}

// CHtmCtx --------------------------------------------------------------------

BOOL
CHtmCtx::IsSourceAvailable()
{
    return(GetHtmInfo()->_cbSrc > 0);
}

HRESULT
CHtmCtx::CopyOriginalSource(IStream * pstm, DWORD dwFlags)
{
    RRETURN(GetHtmInfo()->CopyOriginalSource(pstm, dwFlags));
}

HRESULT
CHtmCtx::ReadUnicodeSource(TCHAR * pch, ULONG ich, ULONG cch, ULONG * pcch)
{
    RRETURN(GetHtmInfo()->ReadUnicodeSource(pch, ich, cch, pcch));
}

HRESULT
CHtmCtx::GetPretransformedFile(LPTSTR *ppch)
{
    RRETURN(GetHtmInfo()->GetPretransformedFile(ppch));
}

#ifdef XMV_PARSE
void
CHtmCtx::SetGenericParse(BOOL bDoGeneric)
{
    CHtmInfo *pHtmInfo = GetHtmInfo();
    if (pHtmInfo)
        pHtmInfo->SetGenericParse(bDoGeneric);
}
#endif

// CHtmInfo (Internal) --------------------------------------------------------

HRESULT
CHtmInfo::OnSource(BYTE * pb, ULONG cb)
{
    PerfDbgLog1(tagHtmSrc, this, "+CHtmInfo::OnSource (cb=%ld)", cb);

    HRESULT hr = S_OK;

    if (cb == 0)
        goto Cleanup;

    if (!_cstrFile)
    {
        ULONG cbBuf = _cbSrc + cb;

        if (cbBuf < 4096)
        {
            if (cbBuf > 1024)
                cbBuf = (cbBuf + 1023) & ~1023;
            else if (cbBuf > 256)
                cbBuf = (cbBuf + 255) & ~255;

            if (cbBuf > _cbBuf)
            {
                if (IsOpened())
                {
                    _fUniSrc = TRUE;
                }
                
                g_csHtmSrc.Enter();

                hr = THR(MemRealloc(Mt(CHtmInfoSrcBuf), (void **)&_pbSrc, cbBuf));

                if (hr == S_OK)
                {
                    _cbBuf = cbBuf;
                }

                g_csHtmSrc.Leave();
            }

            if (hr == S_OK)
            {
                memcpy(_pbSrc + _cbSrc, pb, cb);
            }
        }
        else
        {
            if (_pstmSrc == NULL)
            {
                g_csHtmSrc.Enter();

                _pstmSrc = new CDwnStm;

                if (_pstmSrc == NULL)
                    hr = E_OUTOFMEMORY;
                else
                {
                    _pstmSrc->SetSeekable();

                    if (IsOpened())
                    {
                        _fUniSrc = TRUE;
                    }

                    if (_pbSrc)
                    {
                        hr = THR(_pstmSrc->Write(_pbSrc, _cbSrc));

                        MemFree(_pbSrc);
                        _pbSrc = NULL;
                        _cbBuf = 0;
                    }
                }

                g_csHtmSrc.Leave();

                if (hr)
                    goto Cleanup;
            }

            hr = THR(_pstmSrc->Write(pb, cb));
            if (hr)
                goto Cleanup;
        }
    }

    // Update _cbSrc only after the data has actually been written

    _cbSrc += cb;

Cleanup:
    PerfDbgLog2(tagHtmSrc, this, "-CHtmInfo::OnSource (cbSrc=%ld,hr=%lX)", _cbSrc, hr);
    RRETURN(hr);
}

HRESULT
CHtmInfo::OpenSource(DWORD dwFlags)
{
    PerfDbgLog(tagHtmSrc, this, "+CHtmInfo::OpenSource");
    CStr *pFile = &_cstrFile;

    HRESULT hr = S_OK;
    if (dwFlags & HTMSRC_PRETRANSFORM)
    {
        if (_cstrPretransformedFile)
            pFile = &_cstrPretransformedFile;
        else
        {
            // we can't get to the original mime src, so return failure
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    if (*pFile)
    {
        ClearInterface(&_pstmFile);

        hr = THR(CreateStreamOnFile(*pFile,
                    STGM_READ | STGM_SHARE_DENY_NONE, &_pstmFile));
    }

Cleanup:
    PerfDbgLog1(tagHtmSrc, this, "-CHtmInfo::OpenSource (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmInfo::ReadSource(BYTE * pb, ULONG ib, ULONG cb, ULONG * pcb, DWORD dwFlags)
{
    PerfDbgLog2(tagHtmSrc, this, "+CHtmInfo::ReadSource (ib=%ld,cb=%ld)", ib, cb);

    HRESULT hr = S_OK;
    ULONG   cbSrc = (dwFlags & HTMSRC_PRETRANSFORM) ? MAXLONG : _cbSrc;  // make sure we read the entire file if its pretransform

    *pcb = 0;

    if (ib > cbSrc)
        cb = 0;
    else if (cb > cbSrc - ib)
        cb = cbSrc - ib;

    if (cb > 0)
    {
        if (_pstmFile)
        {
            LARGE_INTEGER li;
            ULARGE_INTEGER uli;

            li.LowPart  = ib;
            li.HighPart = 0;

            hr = THR(_pstmFile->Seek(li, STREAM_SEEK_SET, &uli));

            if (hr == S_OK)
                hr = THR(_pstmFile->Read(pb, cb, pcb));
        }
        else if (_pbSrc || _pstmSrc)
        {
            g_csHtmSrc.Enter();

            if (_pbSrc)
            {
                memcpy(pb, _pbSrc + ib, cb);
                *pcb = cb;
            }
            else if (_pstmSrc)
            {
                hr = THR(_pstmSrc->Seek(ib));

                if (hr == S_OK)
                    hr = THR(_pstmSrc->Read(pb, cb, pcb));
            }

            g_csHtmSrc.Leave();
        }
    }

    PerfDbgLog2(tagHtmSrc, this, "-CHtmInfo::ReadSource (*pcb=%ld,hr=%lX)", *pcb, hr);
    RRETURN(hr);
}

void
CHtmInfo::CloseSource()
{
    ClearInterface(&_pstmFile);
}

HRESULT
CHtmInfo::DecodeSource()
{
    PerfDbgLog(tagHtmSrc, this, "+CHtmInfo::DecodeSource");

    int     cchEncoded;
    LONG    cbToDecode = (LONG)_cbSrc;
    LONG    cbDecoded;
    LONG    cchDecoded;
    TCHAR * pchDecoded = NULL;
    TCHAR * pchEnd;
    BOOL    fOpened    = FALSE;
    HRESULT hr;

    hr = THR(OpenSource());
    if (hr)
        goto Cleanup;

    fOpened = TRUE;

    // Step 1: convert src to unicode and copy into pchDecoded
    if (_fUniSrc)
    {
        // already unicode
        ULONG ib = 0;

        if (cbToDecode >= sizeof(WCHAR))
        {
            WCHAR chTemp;
            hr = THR(ReadSource((BYTE *)&chTemp, 0, sizeof(WCHAR), (ULONG *)&cbDecoded));
            if (hr)
                goto Cleanup;

            // BUGBUG (davidd) support sizeof(WCHAR) of 2 and 4?  (NON/NATIVE_UNICODE_CODEPAGE)
            // BUGBUG (johnv) Support NONNATIVE_UNICODE_SIGNATURE?
            if (NATIVE_UNICODE_SIGNATURE == chTemp)
            {
                ib += sizeof(WCHAR);
                cbToDecode -= ib;
            }
        }

        pchDecoded = (TCHAR *)MemAlloc(Mt(CHtmInfoSrcDecodeBuf), (cbToDecode + sizeof(TCHAR)));
        if (!pchDecoded)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        
        hr = THR(ReadSource((BYTE *)pchDecoded, ib, cbToDecode, (ULONG *)&cbDecoded));
        if (hr)
            goto Cleanup;

        cchEncoded = cbDecoded / sizeof(TCHAR);
    }
    else
    {
        // needs conversion
        
        CEncodeReader encoder(_cpDoc, cbToDecode);

        hr = THR(encoder.PrepareToEncode());
        if (hr)
            goto Cleanup;

        Assert((LONG)encoder._cbBufferMax >= cbToDecode);

        hr = THR(ReadSource(encoder._pbBufferPtr, 0, cbToDecode, (ULONG *)&cbDecoded));
        if (hr)
            goto Cleanup;

        Assert(cbDecoded == cbToDecode);

        encoder._cbBuffer += cbDecoded;

        // The file will contain primarily single byte characters, so
        // ensure we have enough space. Add 1 for NormalizerCR function.
        pchDecoded = (TCHAR *)MemAlloc(Mt(CHtmInfoSrcDecodeBuf), (cbDecoded + 1) * sizeof(TCHAR));
        if (!pchDecoded)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // Number of wide character
        encoder._cchBuffer = cbDecoded;
        encoder._pchEnd = encoder._pchBuffer = pchDecoded;

        // Convert to unicode.
        hr = THR(encoder.WideCharFromMultiByte(TRUE, &cchEncoded));

        // It just get uglier and uglier...
        encoder._cchBuffer = 0;
        encoder._pchEnd = encoder._pchBuffer = NULL;

        if (hr)
            goto Cleanup;

        Assert(cchEncoded <= cbDecoded);
    }

    // Step 2: call NormalizeCR (it also writes a NULL into pchEnd)
    pchEnd = pchDecoded + cchEncoded;
    cchDecoded = cchEncoded - NormalizerChar(pchDecoded, &pchEnd);

    // commit:
    _cbDecoded  = cbDecoded;
    _cchDecoded = cchDecoded;
    MemFree(_pchDecoded);
    _pchDecoded = pchDecoded;
    pchDecoded  = NULL;

Cleanup:

    MemFree(pchDecoded);

    if (fOpened)
    {
        CloseSource();
    }

    PerfDbgLog2(tagHtmSrc, this, "-CHtmInfo::DecodeSource (cch=%ld,hr=%lX)", _cchDecoded, hr);
    RRETURN(hr);
}

// CHtmInfo (External) --------------------------------------------------------

HRESULT
CHtmInfo::CopyOriginalSource(IStream * pstm, DWORD dwFlags)
{
#ifdef UNIX
//Align abBuf so that it is on 4 ByteBoundary so as to avoid alignment faults  later on access to  encoder._pchBuffer 
    typedef union
    {
        DWORD   dwDummy;
        BYTE    _abBuf[4096 + 2];
    } uABBUF;
    uABBUF uabBuf;
#define abBuf (uabBuf._abBuf)

#else
    BYTE    abBuf[4096 + 2];
#endif
    BYTE *  pbCopy;
    BYTE *  pbBuf;
    BYTE *  pbCRorLF;
    BYTE    bLast         = 0;
    LONG    cbCopied      = 0;
    LONG    cbToCopy;
    LONG    cbToWrite;
    LONG    cbSrc         = (dwFlags & HTMSRC_PRETRANSFORM) ? (LONG)MAXLONG : (LONG)_cbSrc;  // make sure we read the entire file if its pretransform
    LONG    ibSrc         = 0;
    int     cch;
    BOOL    fOpened       = FALSE;
    BOOL    fUniSrc       = _fUniSrc;
    HRESULT hr;

    CEncodeWriter encoder(g_cpDefault, 1024);

    encoder._fEntitizeUnknownChars = FALSE;

    hr = THR(OpenSource(dwFlags));
    if (hr)
        goto Cleanup;

    fOpened = TRUE;

    while (ibSrc < cbSrc)
    {
        cbToCopy = cbSrc - ibSrc;

        if (cbToCopy > ARRAY_SIZE(abBuf) - 2)
            cbToCopy = ARRAY_SIZE(abBuf) - 2;

        hr = THR(ReadSource(abBuf, ibSrc, cbToCopy, (ULONG *)&cbToCopy, dwFlags));
        if (hr)
            goto Cleanup;

        if (cbToCopy == 0)
        {
            // File does not appear to be as long as we thought.  We'll be
            // happy with what we got.

            break;
        }

        abBuf[cbToCopy] = 0;
        abBuf[cbToCopy+1] = 0;
        pbCopy = abBuf;

        ibSrc += cbToCopy;

        if (dwFlags & HTMSRC_MULTIBYTE)
        {
            if (    (ibSrc == cbToCopy && cbToCopy >= 2)
                &&  (abBuf[0] == 0xFF && abBuf[1] == 0xFE))
            {
                // Skip over Unicode byte-order marker. 2 bytes or 4 bytes ?

                cbToCopy    -= 2;
                pbCopy      += sizeof(WCHAR);
                fUniSrc      = TRUE;

                if (cbToCopy == 0)
                    continue;
            }

            if (fUniSrc)
            {
                // Convert the Unicode source to MultiByte for the caller

                Assert(encoder._pchBuffer == NULL);
                encoder._pchBuffer = (TCHAR *)pbCopy;
                encoder._cchBuffer = cbToCopy / sizeof(TCHAR);
                encoder._cbBuffer  = 0;

                hr = THR(encoder.MultiByteFromWideChar(ibSrc == cbSrc, &cch));

                encoder._pchBuffer = NULL;
                encoder._cchBuffer = 0;

                if (hr)
                    goto Cleanup;

                pbCopy   = encoder._pbBuffer;
                cbToCopy = encoder._cbBuffer;

                if (cbToCopy == 0)
                    continue;
            }
        }

        pbBuf    = pbCopy;
        cbCopied = 0;

        for (;;)
        {
            pbCRorLF  = NULL;
            cbToWrite = 0;
            
            if (dwFlags & HTMSRC_FIXCRLF)
                pbCRorLF = FirstCRorLF(pbBuf, cbToCopy - cbCopied);
            
            if (pbCRorLF)
                cbToWrite = PTR_DIFF(pbCRorLF, pbBuf);
            else
                cbToWrite = cbToCopy - cbCopied;

            hr = THR(pstm->Write(pbBuf, cbToWrite, NULL));
            if (hr)
                goto Cleanup;

            cbCopied += cbToWrite;

            if (cbCopied >= cbToCopy)
                break;

            if (cbToWrite)
            {
                bLast = *(pbBuf + cbToWrite - 1);
                pbBuf += cbToWrite;
            }

            if (pbCRorLF)
            {
                // If LF is found and previous char is CR, 
                // then we don't write, Otherwise, write CR & LF to temp file.

                if (!(*pbCRorLF == '\n' && bLast == '\r')) 
                {
                    hr = THR(pstm->Write("\r\n", 2, NULL));
                    if (hr)
                        goto Cleanup;

                }

                bLast = *pbCRorLF;
                pbBuf = pbCRorLF + 1;
                cbCopied++;
            }
        }

        bLast = pbCopy[cbToCopy - 1];
    }

Cleanup:

    if (fOpened)
    {
        CloseSource();
    }

    PerfDbgLog1(tagHtmSrc, this, "-CHtmInfo::CopyOriginalSource (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmInfo::ReadUnicodeSource(TCHAR * pch, ULONG ich, ULONG cch, ULONG * pcch)
{
    PerfDbgLog2(tagHtmSrc, this, "+CHtmInfo::ReadUnicodeSource (ich=%ld,cch=%ld)", ich, cch);

    HRESULT hr = S_OK;

    if (_cbDecoded != _cbSrc)
    {
        hr = THR(DecodeSource());
    }

    if (hr == S_OK)
    {
        if (ich > _cchDecoded)
            cch = 0;
        else if (cch > _cchDecoded - ich)
            cch = _cchDecoded - ich;

        if (cch > 0)
        {
            CopyMemory(pch, _pchDecoded + ich, cch * sizeof(TCHAR));
        }

        *pcch = cch;
    }
    else
    {
        *pcch = 0;
    }

    PerfDbgLog2(tagHtmSrc, this, "-CHtmInfo::ReadUnicodeSource (*pcch=%ld,hr=%lX)", *pcch, hr);
    RRETURN(hr);
}

#ifdef XMV_PARSE
void
CHtmInfo::SetGenericParse(BOOL bDoGeneric)
{
    CHtmLoad *pHtmLoad = GetHtmLoad();
    if (pHtmLoad)
        pHtmLoad->SetGenericParse(bDoGeneric);
}
#endif
