/*
 * @(#)EncodingStream.cxx 1.0 6/10/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "encodingstream.hxx"

// NOTE: We are using the error codes defined in xmlerr.h
// which are in the precompiled header.

const int EncodingStream::BUFFERSIZE = 4096*sizeof(WCHAR);

EncodingStream::EncodingStream(IStream * stream): stream(stream), encoding(NULL), buf(NULL)
{
    // These objects are sometimes handed out to external clients.
    ::IncrementComponents();

    pfnWideCharFromMultiByte = NULL;
    pfnWideCharToMultiByte = NULL;
    btotal = bnext = startAt = 0;
    lastBuffer = false;
    bufsize = 0;
    _fEOF = false;
    _fReadStream = true;
    _fUTF8BOM = false;
    _fTextXML = false;
    _fSetCharset = false;
    _dwMode = 0;
    codepage = CP_UNDEFINED;
}

/**
 * Builds the EncodingStream for input.
 * Reads the first two bytes of the InputStream * in order to make a guess
 * as to the character encoding of the file.
 */
IStream * EncodingStream::newEncodingStream(IStream * stream)
{
    EncodingStream * es = new_ne EncodingStream(stream);
    if (es == NULL)
        return NULL;
    es->isInput = true;
    es->buf = NULL;
    return es;
}


/**
 * Builds the EncodingStream for output. The default encoding is UTF-8
 */
IStream * EncodingStream::newEncodingStream(IStream * stream, Encoding * encoding)
{
    HRESULT hr;

    if (!stream)
        return NULL;
    EncodingStream * es = new_ne EncodingStream(stream);
    if (es == NULL)
        return NULL;
    // set the default encoding to UTF-8
    if (encoding == NULL)
    {
        es->encoding = Encoding::newEncoding();
        if (es->encoding == NULL)
            goto error;
    }
    else
        es->encoding = encoding;
    es->isInput = false;
    es->maxCharSize = 3; // set a default value
    checkerr(CharEncoder::getWideCharToMultiByteInfo(es->encoding, &es->codepage, &es->pfnWideCharToMultiByte, &es->maxCharSize));

    // allocates bytebuffer according to maxCharSize, allowing enough space 
    // to hold bytes converted from wide characters in buffer
    es->bufsize = BUFFERSIZE * ((es->maxCharSize + 1) / sizeof(WCHAR));
    es->buf = new_ne BYTE[es->bufsize];
    if (es->buf == NULL)
        goto error;

    //
    // Notice: byteOrderMark was set to true only when the encoding is UCS-2, UTF-16 or UCS-4 
    //
    if (es->encoding->byteOrderMark)
    {
        BYTE szTemp[2];
        ULONG ulError;

        if (es->encoding->littleendian)
        {
            szTemp[0]= 0xff;
            szTemp[1]= 0xfe;
        }
        else
        {
            szTemp[0]= 0xfe;
            szTemp[1]= 0xff;
        }
        checkerr(stream->Write(szTemp, 2, &ulError));
        if (es->codepage == CP_UCS_4)
        {
            checkerr(stream->Write(szTemp, 2, &ulError));
        }
    }

    return es;

error:
    delete es;
    return NULL;
}

EncodingStream::~EncodingStream()
{
    // These objects are sometimes handed out to external clients.
    ::DecrementComponents();

    if (buf != NULL)
        delete [] buf;
    if (encoding != NULL)
        delete encoding;
    stream = NULL;
}

/**
 * Reads characters from stream and encode it to Unicode
 */
HRESULT STDMETHODCALLTYPE EncodingStream::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    HRESULT hr;

    ULONG num = 0;

    if (pcbRead != NULL)
        *pcbRead = 0;

    if (btotal == 0 && _fEOF)          // we already hit EOF - so return right away.
        return S_OK;

    // Calculate how many UNICODE chars we are allowed to return.
    cb /= sizeof(WCHAR);
    checkhr2( prepareForInput(cb));

    if (stream && _fReadStream)
    {
        // btotal = number of bytes already in start of buffer.
        if (cb > btotal)
        {
            hr = stream->Read(buf + btotal, cb - btotal, &num);
            if (hr == E_PENDING && num > 0)
            {
                // in which case we ignore the error, and continue on !!.
                // BUGBUG - this may be a problem.since we are changing the
                // return code returned from the stream.  This may mean we
                // should not ever hand out this stream outside of MSXML.
                hr = 0;
            }
            if (FAILED(hr))
            {
                return hr;
            }
            if (btotal == 0 && num == 0)
            {
                _fEOF = true;
                return hr;
            }
        }
        else
        {
            hr = S_OK;
        }
    }
    else if (btotal == 0)
    {
        return (lastBuffer) ? S_FALSE : E_PENDING;
    }

    btotal += num;
    UINT b = btotal, utotal = cb;
    if (b > cb)
    {
        // If we have more bytes in our buffer than the caller has
        // room for, then only return the number of bytes the caller
        // asked for -- otherwise pfnWideCharFromMultiByte will write
        // off the end of the caller's buffer.
        b = cb;
    }
    if (pfnWideCharFromMultiByte == NULL) // first read() call
    {
        checkhr2(autoDetect());
        if (pfnWideCharFromMultiByte == NULL) // failed to fully determine encoding
            return (lastBuffer) ? S_FALSE : E_PENDING;
        b -= bnext;
        startAt -= bnext;
    }
    hr = (this->pfnWideCharFromMultiByte)(&_dwMode, codepage, buf + bnext, &b, (TCHAR *)pv, &utotal);
    if (hr != S_OK)
        return hr;
    if (b == 0 && num == 0 && (stream || lastBuffer))
    {
        // stream says we're at the end, but pfnWideCharFromMultiByte
        // disagrees !!
        return XML_E_INCOMPLETE_ENCODING;
    }
    bnext += b;
    if (pcbRead != NULL)
        *pcbRead = utotal*sizeof(WCHAR);
    return (utotal == 0) ? E_PENDING : S_OK;
} 


// BUGBUG: According to RFC 2376 (XML Media type),
// when media type is "text/xml" and charset is missing
// the charset should be always "US-ASCII" no matter what encoding information is in
// the xml document. 
// However, because urlmon always sets MimeType to text/xml when it sniffs xml document,
// we cannot strictly following RFC 2376 (we want the mime type be set to application/xml by
// urlmon so we can detect the encoding according to what's in the XML language spec) 
// See bug 74716 for more info.

// When charset is set, we use it as the encoding scheme;
// otherwise, do auto-detect and ignore mime type information

/**
 * Checks the first two/four bytes of the input Stream in order to 
 * detect UTF-16/UCS-4 or UTF-8 encoding;
 * otherwise assume it is UTF-8
 */
HRESULT EncodingStream::autoDetect()
{
    // wait until we have enough to be sure.
    if (btotal < 2)
        return S_OK;

    unsigned int guess = (((unsigned char)buf[0]) << 8) + ((unsigned char)buf[1]);
    HRESULT hr;

    if (guess == 0xFEFF || guess == 0xFFFE) // BOM found
    {
        // wait until we have enough to be sure.
        if (btotal < 4)
            return S_OK;

        unsigned int guess1 = (((unsigned char)buf[2]) << 8) + ((unsigned char)buf[3]);
        if (guess == guess1)
        {
            if (!encoding)
            {
                static const TCHAR* wchUCS4 = _T("UCS-4");
                encoding = Encoding::newEncoding(wchUCS4, 5, (0xFFFE == guess), true);
            }
            bnext = 4;
        }
        else
        {
            if (!encoding)
            {
                static const TCHAR* wchUCS2 = _T("UCS-2");
                encoding = Encoding::newEncoding(wchUCS2, 5, (0xFFFE == guess), true);
            }
            bnext = 2;
        }

        if (NULL == encoding)
            return E_OUTOFMEMORY;       
        encoding->littleendian =  (0xFFFE == guess);
    }
    else
    {
        if (!encoding)
        {
            encoding = Encoding::newEncoding();
            if (NULL == encoding)
                return E_OUTOFMEMORY;
        }

        // In some system, such as win2k, there is BOM 0xEF BB BF for UTF8
        if (guess == 0xEFBB)
        {
            if (btotal < 3)
                return S_OK;
            if (buf[2] == 0xBF)
                _fUTF8BOM = true; 
            bnext = 3; 
        }
        else
        {
            encoding->byteOrderMark = false;
        }
    }

    checkhr2(CharEncoder::getWideCharFromMultiByteInfo(encoding, &codepage, &pfnWideCharFromMultiByte, &maxCharSize));
    return S_OK;
}


/**
 * Encodes Unicode characters and writes to output stream
 */
HRESULT STDMETHODCALLTYPE EncodingStream::Write(void const* pv, ULONG cb, ULONG * pcbWritten)
{
    HRESULT hr;
    UINT bytetotal;
    UINT cb1 = BUFFERSIZE / maxCharSize;
    ULONG position = 0;
    ULONG utotal;
    cb /= sizeof(WCHAR);

    if (pfnWideCharToMultiByte == NULL)
        return E_FAIL;

    do {
        if (cb1 + position > cb)
            cb1 = cb - position;
	    bytetotal = cb1 * maxCharSize;
        checkhr2((this->pfnWideCharToMultiByte)(&_dwMode, codepage, (WCHAR *)pv + position, &cb1, buf, &bytetotal));
        if (S_FALSE == hr) // unable to convert a character
            return E_UNEXPECTED;
        checkhr2(stream->Write((void *)buf, bytetotal, &utotal));
        position += cb1;
    } while (position < cb);

    if (pcbWritten)
        *pcbWritten = cb; 
    return S_OK;
}


/**
 * Switchs the character encoding of the input stream
 * Returns:
 *         S_OK: succeeded, and do not need re-read
 *         S_FALSE: succeeded, needs to re-read from <code> newPosition </code>
 *         Otherwise: error code
 * Notice: 
 *         This method only works for input stream, newPosition starts with 1
 */
HRESULT EncodingStream::switchEncodingAt(Encoding * newEncoding, int newPosition)
{
    // Ignore encoding information in the document when charset information is set from outside
    if (_fSetCharset)
        return S_OK;

    int l = newPosition - startAt;
    if (l < 0 || l > (int)bnext) 
    {
        // out of range
        delete newEncoding;
        return E_INVALIDARG;
    }

    UINT newcodepage;
    UINT newCharSize;
    //
    // get and check charset information
    //
    WideCharFromMultiByteFunc * pfn;
    HRESULT hr = CharEncoder::getWideCharFromMultiByteInfo(newEncoding, &newcodepage, &pfn, &newCharSize);
    if (hr != S_OK)
    {
        delete newEncoding;
        return E_INVALIDARG;
    }
    if (codepage == newcodepage)
    {
        delete newEncoding;
        return S_OK;
    }

    // Now if we are in UCS-2/UCS-4 we cannot switch out of UCS-2/UCS-4 and if we are
    // not in UCS-2/UCS-4 we cannot switch into UCS-2/UCS-4.
    // Also if UTF-8 BOM is presented, we cannot switch away
    if ((codepage != CP_UCS_2 && newcodepage == CP_UCS_2) ||
        (codepage == CP_UCS_2 && newcodepage != CP_UCS_2) ||
        (codepage != CP_UCS_4 && newcodepage == CP_UCS_4) ||
        (codepage == CP_UCS_4 && newcodepage != CP_UCS_4) ||
        (codepage == CP_UTF_8 && newcodepage != CP_UTF_8 && _fUTF8BOM))
    {
        delete newEncoding;
        return E_FAIL;
    }

    // Ok, then, let's make the switch.
    delete encoding;
    encoding = newEncoding;
    maxCharSize = newCharSize;
    codepage = newcodepage;
    pfnWideCharFromMultiByte = pfn;

    // Because the XML declaration is encoded in UTF-8, 
    // Mapping input characters to wide characters is one-to-one mapping
    if ((int)bnext != l)
    {
        bnext = l;
        return S_FALSE;
    }
    return S_OK;
}

HRESULT EncodingStream::prepareForInput(ULONG minlen)
{
    Assert(btotal >= bnext);
    btotal -= bnext;

    if (bufsize < minlen)
    {
        BYTE* newbuf = new_ne BYTE[minlen];
        if (newbuf == NULL)
            return E_OUTOFMEMORY;
        if (buf != NULL)
        {
            ::memcpy(newbuf, buf+bnext, btotal);
            delete[] buf;
        }
        buf = newbuf;
        bufsize = minlen;
    }
    else if (bnext > 0 && btotal > 0)
    {
        // Shift remaining bytes down to beginning of buffer.
        ::memmove(buf, buf + bnext, btotal);          
    }
    startAt += bnext;
    bnext = 0;
    return S_OK;
}


HRESULT EncodingStream::AppendData( const BYTE* buffer, ULONG length, BOOL lastBuffer)
{
    Assert(btotal >= bnext);
    this->lastBuffer = (lastBuffer != FALSE);
    HRESULT hr;
    ULONG minlen = length + (btotal - bnext); // make sure we don't loose any data
    if (minlen < BUFFERSIZE)
        minlen = BUFFERSIZE;
    checkhr2( prepareForInput(minlen));
    
    if (length > 0 && buffer != NULL)
    {
        // Copy raw data into new buffer.
        ::memcpy(buf + btotal, buffer, length);
        btotal += length;
    }
    if (pfnWideCharFromMultiByte == NULL) // first AppendData call
    {
        checkhr2(autoDetect());
    }
    return hr;
}

HRESULT EncodingStream::BufferData()
{
    HRESULT hr = S_OK;
    checkhr2( prepareForInput(0)); // shift down (so bnext=0).

    if (_fEOF)          // already hit the end of the stream.
        return S_FALSE;

    const DWORD BUFSIZE = 4096;

    DWORD dwRead = 1;

    while (S_OK == hr && dwRead > 0)
    {
        // if we cannot fit another buffer full, then re-allocate.
        DWORD minsize = (btotal+BUFSIZE > bufsize) ? bufsize + BUFSIZE : bufsize;
        checkhr2( prepareForInput(minsize)); // make space available.

        dwRead = 0;
        hr = stream->Read(buf + btotal, BUFSIZE, &dwRead);
        btotal += dwRead;
    }

    if (SUCCEEDED(hr) && dwRead == 0)
    {
        _fEOF = true;
        hr = S_FALSE; // return S_FALSE when at eof.
    }
    return hr;
}

Encoding *  
EncodingStream::getEncoding()
{
    return encoding;
}


void 
EncodingStream::SetMimeType(const WCHAR * pwszMimeType, int length)
{
    static const TCHAR * pszTEXT = _T("text/xml");

    if (8 == length)
        _fTextXML = StrCmpNI(pwszMimeType, pszTEXT, 8) == 0; // text/xml
    else
        _fTextXML = false;   // application/xml
}
    

void 
EncodingStream::SetCharset(const WCHAR * pwszCharset, int length)
{
    if (encoding)
        delete encoding;
    // assume BOM and little endian
    encoding = Encoding::newEncoding(pwszCharset, length, true, true);
    _fSetCharset = true;
}
