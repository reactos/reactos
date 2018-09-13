/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include <memory.h> // memmove.

#include <shlwapip.h>   // IsCharSpace
#ifdef UNIX
// Not needed under UNIX
#else
#ifndef _WIN64
#include <w95wraps.h>
#endif // _WIN64
#endif /* UNIX */

#include "chartype.hxx" // isWhiteSpace

#include "bufferedstream.hxx"
#include "xmlstream.hxx"
#include "../encoder/encodingstream.hxx"
#include <xmlparser.h>

const long BLOCK_SIZE = 4096;

// no point remembering a line buffer longer than this because client
// probably can't deal with that anyway.
const long MAX_LINE_BUFFER = 512;

//#define checkhr2(a) hr = a; if (hr != S_OK) return hr;

BufferedStream::BufferedStream(XMLStream *pXMLStream)
{
    _pchBuffer = NULL;
    _lSize = 0;
    _pXMLStream = pXMLStream;
    init();
}

void BufferedStream::init()
{
    _lCurrent = _lUsed = _lMark = 0;
    _lLine = 1; // lines start at 1.
    _lMarkedline = 1;
    _lLinepos = 0;
    _lMarkedlinepos = 0;
    _chLast = 0;
    _lStartAt = 0;
    _fEof = false;
    _lLockedPos = -1;
    _lLastWhiteSpace = -1;
    _lLockCount = 0;
    _fNotified = false;
    _fFrozen = false;
    _pPendingEncoding = NULL;
}

BufferedStream::~BufferedStream()
{
    delete [] _pchBuffer;
    _pStmInput = NULL;
    delete _pPendingEncoding;
    _pPendingEncoding = NULL;
}

HRESULT BufferedStream::Reset()
{
    init();
    delete[] _pchBuffer;
    _pchBuffer = NULL;
    _lSize = 0;
    _pStmInput = NULL;
    _lLockedPos = -1;
    _lLockCount = 0;
    _fFrozen = false;
    delete _pPendingEncoding;
    _pPendingEncoding = NULL;
    return S_OK;
}

HRESULT  
BufferedStream::Load( 
        /* [unique][in] */ EncodingStream __RPC_FAR *pStm)
{
    if (pStm != NULL)
    {
        init();
        _pStmInput = pStm;
        return S_OK;
    }
    else
    {
        _pStmInput = NULL;
    }
    return S_OK;
}

WCHAR*  
BufferedStream::getEncoding()
{   
    if (!_pStmInput)
        return NULL;
    else
    {
        Encoding* e = _pStmInput->getEncoding();
        if (e == NULL)
        {
            fillBuffer();
        }
        e = _pStmInput->getEncoding();
        return (e == NULL) ? NULL : e->charset;
    }
}


HRESULT 
BufferedStream::AppendData( const BYTE* in, ULONG length, BOOL lastBuffer)
{
    HRESULT hr;

    if (_fEof)
    {
        init();
    }

    if (!_pStmInput)
    {
        EncodingStream* stream = (EncodingStream*)EncodingStream::newEncodingStream(NULL); 
        if (stream == NULL)
            return E_OUTOFMEMORY;
        _pStmInput = stream;
        stream->Release(); // Smart pointer is holding a ref
    }

    checkhr2(_pStmInput->AppendData(in, length, lastBuffer));

    return S_OK;

}

HRESULT  
BufferedStream::nextChar( 
        /* [out] */ WCHAR* ch,
        /* [out] */ bool* fEOF)
{
    HRESULT hr;

    if (_lCurrent >= _lUsed)
    {
        if (_fEof)
        {
            *fEOF = true;
            return S_OK;
        }
        if (! _fNotified && _lUsed > 0)
        {
            _fNotified = true;          // notify data available BEFORE blowing

            // NOTE: this code approximates what prepareForInput does
            // in order to accurately predict when the buffer is about to
            // be re-allocated.

            long shift = _fFrozen ? 0 : getNewStart(); // is data about to shift?
            long used = _lUsed - shift; // this is how much is really used after shift
            if (_lSize - used < BLOCK_SIZE + 1) // +1 for null termination.
            {
                // we will reallocate !!  So return a special
                // return code
                hr = E_DATA_REALLOCATE;
            }
            else
                hr = E_DATA_AVAILABLE;    // away the old data so parser can save it if need be.
            checkhr2( _pXMLStream->ErrorCallback(hr) );
        }                   

        checkhr2( fillBuffer() );
        if (_fEof)
        {
            *fEOF = true;
            return S_OK;
        }
        _fNotified = false;
    }

    WCHAR result = _pchBuffer[_lCurrent++];

    switch (result)
    {
    case 0xa:
    case 0xd:
        if (result == 0xd || _chLast != 0xd)
            _lLine++; 
        _lLinepos = _lCurrent;
        _chLast = result;
        _lLastWhiteSpace = _lCurrent;
        break;
    case 0x20:
    case 0x9:
        _lLastWhiteSpace = _lCurrent;
        break;
    case 0xfffe:
    case 0xffff:
        return XML_E_BADCHARDATA;
    }

    *ch = result;
    return S_OK;
}

HRESULT BufferedStream::scanPCData( 
    /* [out] */ WCHAR* ch,
    /* [out] */ bool* fWhitespace)
{
    WCHAR result;
    bool foundNonWhiteSpace = false;

    if (! isWhiteSpace(*ch))
        foundNonWhiteSpace = true;

    // Then skip the data until we find '<', '>' or '&'
    while (_lCurrent < _lUsed)
    {
        result = _pchBuffer[_lCurrent++];

        switch (result)
        {
        case ']':
        case '>':
        case '<':
        case '&':
        case '\'':  // so this can be used to scan attribute values also.
        case '"':   // so this can be used to scan attribute values also.
            *ch = result;
            if (foundNonWhiteSpace)
                *fWhitespace = false;
            return S_OK;
            break;

        case 0xa:
        case 0xd:
            if (result == 0xd || _chLast != 0xd)
                _lLine++; 
            _lLinepos = _lCurrent;
            _chLast = result;
            _lLastWhiteSpace = _lCurrent;
            break;
        case 0x20:
        case 0x9:
            _lLastWhiteSpace = _lCurrent;
            break;
        case 0xfffe:
        case 0xffff:
            return XML_E_BADCHARDATA;
        default:
            foundNonWhiteSpace = true;
            break;
        }
    }

    // And just return E_PENDING if we run out of buffer.
    if (foundNonWhiteSpace)
        *fWhitespace = false;
    return E_PENDING;
}

long BufferedStream::getLine() 
{ 
    return _lMarkedline; 
}

long BufferedStream::getLinePos() 
{
    // _lMarkedlinepos is the position of the beginning of the marked line
    // relative to the beginning of the buffer, and _lMark is the 
    // position of the marked token relative to the beginning of the
    // buffer, So the position of the marked token relative to the 
    // current line is the difference between the two.
    // We also return a 1-based position so that the start of the
    // line = column 1.  This is consistent with the line numbers
    // which are also 1-based.
    return (_lMarkedlinepos > _lMark+1) ? 0 : _lMark+1-_lMarkedlinepos; 
}

long BufferedStream::getInputPos()
{
    return _lStartAt+_lMark;
}

WCHAR* BufferedStream::getLineBuf(ULONG* len, ULONG* startpos)
{
    *len = 0;
    if (_pchBuffer == NULL)
        return NULL;

    WCHAR* result = &_pchBuffer[_lMarkedlinepos];

    ULONG i = 0;
    // internal _pchBuffer is guarenteed to be null terminated.
    WCHAR ch = result[i];
    while (ch != 0 && ch != L'\n' && ch != L'\r')
    {
        i++;
        ch = result[i];
    }
    *len = i;
    // also return the line position relative to start of
    // returned buffer.
    *startpos = (_lMarkedlinepos > _lMark+1) ? 0 : _lMark+1-_lMarkedlinepos; 
    return result;
}

HRESULT BufferedStream::switchEncoding(const TCHAR * charset, ULONG len)
{
    HRESULT hr = S_OK;

    if (!_pStmInput)
    {
        hr = E_FAIL;
        goto CleanUp;
    }
    else
    {
        _pPendingEncoding = Encoding::newEncoding(charset, len);
        if (_pPendingEncoding == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }

        if (! _fFrozen)
        {
             hr = doSwitchEncoding();
        }
    }
CleanUp:
    return hr;
}

HRESULT BufferedStream::doSwitchEncoding()
{
    Encoding* encoding = _pPendingEncoding;
    _pPendingEncoding = NULL;

    HRESULT hr = _pStmInput->switchEncodingAt(encoding, _lStartAt + _lCurrent);
    if (hr == S_FALSE)
    {
        // need to re-read to force re-decode into new encoding.
        // In other words we have to forget that we read past this
        // position already so that the next call to nextChar
        // will call FillBuffer again.
        // (+1 so that nextChar works correctly).
        _lUsed = _lStartAt + _lCurrent;
        hr = S_OK;
    }
    else if (FAILED(hr))
    {
        hr = (hr == E_INVALIDARG) ? XML_E_INVALIDENCODING : XML_E_INVALIDSWITCH;
    }
    return hr;
}

// Returns a pointer to a contiguous block of text accumulated 
// from the last time Mark() was called up to but not including
// the last character read. (This allows a parser to have a
// lookahead character that is not included in the token).
HRESULT  
BufferedStream::getToken(const WCHAR**p, long* len)
{
    if (_pchBuffer == NULL)
        return E_FAIL;

    if (_lCurrent != _lCurrent2)
    {
        // need to fix up buffer since it is no
        // out of sync since we've been compressing
        // whitespace.

    }
    *p = &_pchBuffer[_lMark];
    *len = getTokenLength();
    return S_OK;
}


void 
BufferedStream::Lock()
{
    // We allow nested locking - where the outer lock wins - unlock only 
    // really unlocks when the outer lock is unlocked.
    if (++_lLockCount == 1)
    {
        _lLockedPos = _lMark;
        _lLockedLine = _lMarkedline;
        _lLockedLinePos = _lMarkedlinepos;
    }
}

void 
BufferedStream::UnLock()
{
    if (--_lLockCount == 0)
    {
        _lMark = _lLockedPos;
        _lMarkedline = _lLockedLine;
        _lMarkedlinepos = _lLockedLinePos;
        _lLockedPos = -1;
    }
}

HRESULT 
BufferedStream::Freeze()
{
    HRESULT hr;
    if (_lCurrent > _lMidPoint)
    {
        // Since we freeze the buffer a lot now (any time we're inside
        // a tag) we need to shift the bytes down in the buffer more
        // frequently in order to guarentee we have space in the buffer
        // when we need it.  Otherwize the buffer would tend to just
        // keep growing and growing.  So we shift the buffer when we
        // go past the midpoint.
        checkhr2( prepareForInput() ); 
        
    }
    _fFrozen = true;
    return S_OK;
}

HRESULT 
BufferedStream::UnFreeze()
{
    _fFrozen = false;
    if (_pPendingEncoding)
    {
        return doSwitchEncoding();
    }
    return S_OK;
}


HRESULT 
BufferedStream::fillBuffer()
{
    HRESULT hr;
    
    checkhr2( prepareForInput() ); 

    if (_pStmInput)
    {
        long space = _lSize - _lUsed - 1; // reserve 1 for NULL termination

        // get more bytes.
        ULONG read = 0;
        HRESULT rc = _pStmInput->Read(&_pchBuffer[_lUsed], space*sizeof(WCHAR), &read);

        _lUsed += read/sizeof(WCHAR); // stream must return unicode characters.
       _pchBuffer[_lUsed] = 0; // NULL terminate the _pchBuffer.

        if (FAILED(rc))
            return rc;

        if (read == 0)
        {
            _fEof = true;
            // increment _lCurrent, so that getToken returns
            // last character in file.
            _lCurrent++; _lCurrent2++;
        }
    }
    else
    {
        // SetInput or AppendData hasn't been called yet.
        return E_PENDING;
    }

    return S_OK;
}

HRESULT 
BufferedStream::prepareForInput()
{
    // move the currently used section of the _pchBuffer 
    // (buf[mark] to buf[used]) down to the beginning of
    // the _pchBuffer.

    long newstart = 0;

    // BUGBUG - if this code is changed BufferedStream::nextChar has to
    // be updated also so that they stay in sync, otherwise we might
    // re-allocated the buffer without generating an E_DATA_REALLOCATE
    // notification - which would be very bad (causes GPF's in the parser).

    if (! _fFrozen)  // can't shift bits if the buffer is frozen.
    {
        newstart = getNewStart();

        if (newstart > 0)
        {
            WCHAR* src = &_pchBuffer[newstart];
            _lUsed -= newstart;
            _lStartAt += newstart;
            ::memmove(_pchBuffer,src,_lUsed*sizeof(WCHAR));
            _lCurrent -= newstart;
            _lCurrent2 -= newstart;
            _lLastWhiteSpace -= newstart;
            _lLinepos = (_lLinepos > newstart) ? _lLinepos-newstart : 0;
            _lMarkedlinepos = (_lLinepos > newstart) ? _lMarkedlinepos-newstart : 0;
            _lMark -= newstart;
            _lLockedLinePos = (_lLockedLinePos > newstart) ? _lLockedLinePos-newstart : 0;
            _lLockedPos -= newstart;
        }
    }

    // make sure we have a reasonable amount of space
    // left in the _pchBuffer.
    long space = _lSize - _lUsed; 
    if (space > 0) space--; // reserve 1 for NULL termination
    if (_pchBuffer == NULL || space < BLOCK_SIZE)
    {
        // double the size of the buffer.
		long newsize = (_lSize == 0) ? BLOCK_SIZE : (_lSize*2);

        WCHAR* newbuf = new_ne WCHAR[newsize];
        if (newbuf == NULL)
        {
            // try more conservative allocation.
            newsize = _lSize + BLOCK_SIZE;
            newbuf = new_ne WCHAR[newsize];
        }
        if (newbuf == NULL && space == 0)
            return E_OUTOFMEMORY;

        if (newbuf != NULL)
        {
            if (_pchBuffer != NULL)
            {
                // copy old bytes to new _pchBuffer.
                ::memcpy(newbuf,_pchBuffer,_lUsed*sizeof(WCHAR));
                delete [] _pchBuffer;
            }
            newbuf[_lUsed] = 0; // make sure it's null terminated.
            _pchBuffer = newbuf;
            _lSize = newsize;
            _lMidPoint = newsize / 2;

        }
    }

    return S_OK;
}

long
BufferedStream::getNewStart()
{
    long newstart = 0;

    // Unless the buffer is frozen, in which case we just reallocate and
    // do no shifting of data.
    if (_lLockedPos > 0)
    {
        // and try and preserve the beginning of the marked line if we can
        if (_lLockedLinePos < _lLockedPos && 
            _lLockedPos - _lLockedLinePos < MAX_LINE_BUFFER)
        {
            newstart = _lLockedLinePos;
        }
    }
    else if (_lMark > 0)
    {
        // and try and preserve the beginning of the marked line if we can
        newstart = _lMark;
        if (_lMarkedlinepos < _lMark && 
            _lMark - _lMarkedlinepos < MAX_LINE_BUFFER) // watch out for long lines
        {
            newstart = _lMarkedlinepos;
        }
    }
    return newstart;
}

