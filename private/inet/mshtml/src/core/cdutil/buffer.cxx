//      Buffer.cpp
//              Copyright 1996 Microsoft Corporation.  All rights reserved.
//
//              This file implements a class that provides a dynamically growing buffer.
//
//      1/5/96 created - Chris Wilson (cwilso@microsoft.com)
//      1/8/97 revised for Trident & converted to Unicode - cwilso

#include "headers.hxx"

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

MtDefine(CBuffer, Utilities, "CBuffer")
MtDefine(CBuffer2, Utilities, "CBuffer2")

#define INIT_BUFFER_COUNT     256   // # of characters for which space is initially alloc'd

//---------------------------------------------------------------------
//      CBuffer::CBuffer()
//              This function initializes the internal buffer to a default length
//  of 1024 characters.
//---------------------------------------------------------------------
CBuffer::CBuffer() :
    m_pszStringBuf(NULL), m_pszCurrChar(NULL), m_lBufSize(INIT_BUFFER_COUNT),
    m_lStringLen(0)
{
        // Allocate buffer space.
        m_pszStringBuf = m_pszCurrChar = (LPTSTR) MemAlloc(Mt(CBuffer),
            sizeof(TCHAR) * m_lBufSize);
        *m_pszStringBuf = _T('\0');
}

//---------------------------------------------------------------------
//      CBuffer::~CBuffer()
//              The destructor for the CBuffer object frees the string buffer
//      (if initialized) and resets all internal member variables, just to
//      be safe.
//---------------------------------------------------------------------
CBuffer::~CBuffer()
{
        if ( m_pszStringBuf )
        {
                MemFree(m_pszStringBuf);
                m_pszStringBuf = m_pszCurrChar = NULL;
                m_lBufSize = m_lStringLen = 0;
        }
}


//---------------------------------------------------------------------
//      CBuffer::GrowBuffer()
//              This method grows the buffer by the specified amount +
//      INIT_BUFFER_COUNT.
//
//---------------------------------------------------------------------
BOOL CBuffer::GrowBuffer( const int iSize /*=1*/ )
{
        long lNewBufSize = m_lBufSize + iSize + INIT_BUFFER_COUNT;
        HRESULT hr = MemRealloc(Mt(CBuffer), (void **)&m_pszStringBuf, lNewBufSize * sizeof(TCHAR));
        if (hr)
            return FALSE;   // ERROR reallocating!  Maybe return hr?

        m_lBufSize = lNewBufSize;
        m_pszCurrChar = m_pszStringBuf + m_lStringLen;
        return TRUE;
}

//---------------------------------------------------------------------
//      CBuffer::TrimTrailingWhitespace()
//              This function trims any whitespace at the end of the string in 
//      the internal buffer, properly keeping track of the string length.
//---------------------------------------------------------------------
void CBuffer::TrimTrailingWhitespace( void )
{
        while ( --m_pszCurrChar > m_pszStringBuf )
        {
                m_lStringLen--;
                if ( !isspace( *m_pszCurrChar ) )
                {
                        m_pszCurrChar++;
                        m_lStringLen++;
                        return;
                }
                *m_pszCurrChar = _T('\0');
        }
}

//---------------------------------------------------------------------
//      CBuffer::SwapContents()
//              This method swaps the contents of two CBuffers.
//---------------------------------------------------------------------
void CBuffer::SwapContents( CBuffer & cBufOther )
{
    LPTSTR pszTemp;
    long lTemp;

    pszTemp = m_pszStringBuf;
    m_pszStringBuf = cBufOther.m_pszStringBuf;
    cBufOther.m_pszStringBuf = pszTemp;

    pszTemp = m_pszCurrChar;
    m_pszCurrChar = cBufOther.m_pszCurrChar;
    cBufOther.m_pszCurrChar = pszTemp;

    lTemp = m_lStringLen;
    m_lStringLen = cBufOther.m_lStringLen;
    cBufOther.m_lStringLen = lTemp;

    lTemp = m_lBufSize;
    m_lBufSize = cBufOther.m_lBufSize;
    cBufOther.m_lBufSize = lTemp;
}


//+------------------------------------------------------------------------
//
//  Class:      CBuffer2
//
//  Synopsis:   A sequence of exponentially growing buffers, used for
//              reducing MemAllocs (logarithmic) and eliminating
//              MemReallocs (complete) when accumulating long strings.
// 
//              A new CBuffer2 contains room for the first 1024 chars
//              without an extra allocation.
//
//-------------------------------------------------------------------------
CBuffer2::CBuffer2()
{
    _pchCur = NULL;
    _cchRemaining = 0;
    _cBuf = 0;
}

void
CBuffer2::Clear()
{
    int c;
    TCHAR **ppch;

    for (c = _cBuf, ppch = _apchBuf; c; c -= 1, ppch += 1)
        MemFree(*ppch);
        
    _pchCur = NULL;
    _cchRemaining = 0;
    _cBuf = 0;
}
       
CBuffer2::~CBuffer2()
{
    int c;
    TCHAR **ppch;

    for (c = _cBuf, ppch = _apchBuf; c; c -= 1, ppch += 1)
        MemFree(*ppch);
}


//+------------------------------------------------------------------------
//
//  Method:     Length
//
//  Synopsis:   return init * (2^buffers - 1) - slop
// 
//-------------------------------------------------------------------------
int
CBuffer2::Length()
{
    return (INIT_BUFFER2_SIZE * ((1 << _cBuf) - 1)) - _cchRemaining;
}

//+------------------------------------------------------------------------
//
//  Method:     SetCStr
//
//  Synopsis:   Do a single alloc on the cstr, then copy contents in
//              one buffer at a time.
// 
//-------------------------------------------------------------------------
HRESULT
CBuffer2::SetCStr(CStr *pcstr)
{
    HRESULT hr;
    int cch;
    int cBufFull;
    int cchCur;
    TCHAR **ppchCur;
    TCHAR *pchTo;

    cch = Length();
    
    hr = THR(pcstr->Set(NULL, Length()));
    if (hr)
        RRETURN(hr);

    if (!cch)
        return S_OK;

    Assert(_cBuf);
    
    pchTo = *pcstr;
    cBufFull = _cBuf - 1;
    cchCur = INIT_BUFFER2_SIZE;
    ppchCur = _apchBuf;

    while (cBufFull)
    {
        memcpy(pchTo, *ppchCur, sizeof(TCHAR) * cchCur);
        
        pchTo += cchCur;
        cchCur *= 2;
        ppchCur += 1;
        cBufFull -= 1;
    }

    cchCur -= _cchRemaining;
    
    memcpy(pchTo, *ppchCur, sizeof(TCHAR) * cchCur);
    
#if DBG == 1
    pchTo += cchCur;

    Assert(pchTo - *pcstr == Length());
#endif    

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Method:     Append
//
//  Synopsis:   Append new stuff to the sequence of buffers, allocating
//              exponentially larger new buffers as needed.
// 
//-------------------------------------------------------------------------
HRESULT
CBuffer2::Append(TCHAR *pch, int cch)
{
    int cchToCopy;
    
    while (cch)
    {
        if (!_cchRemaining)
        {
            int cchAlloc;
            
            // Note: because of exponential growth, we'll really run out of
            // memory long before we hit this condition.
            
            if (_cBuf >= MAX_BUFFER2_GROWTH)
                return E_OUTOFMEMORY;

            cchAlloc = INIT_BUFFER2_SIZE * (1 << _cBuf);
            
            _pchCur = (TCHAR *)MemAlloc(Mt(CBuffer2), sizeof(TCHAR) * cchAlloc);
            if (!_pchCur)
                return E_OUTOFMEMORY;

            _cBuf += 1;
            _apchBuf[_cBuf - 1] = _pchCur;
            _cchRemaining = cchAlloc;
        }

        Assert(_cchRemaining && _pchCur);

        cchToCopy = min(_cchRemaining, cch);

        memcpy(_pchCur, pch, sizeof(TCHAR) * cchToCopy);

        cch -= cchToCopy;
        pch += cchToCopy;
        _cchRemaining -= cchToCopy;
        _pchCur += cchToCopy;
    }

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Method:     Chop
//
//  Synopsis:   Removes the last cch chars from the buffer
// 
//-------------------------------------------------------------------------
void
CBuffer2::Chop(int cch)
{
    int cchBuffer;

    Assert(cch <= Length());

    if (!cch)
        return;

    Assert(_cBuf);
    
    cchBuffer = INIT_BUFFER2_SIZE * (1 << (_cBuf - 1));

    while (cch >= cchBuffer - _cchRemaining)
    {
        Assert(_cBuf > 0);
        
        cch -= cchBuffer - _cchRemaining;
        
        MemFree(_apchBuf[_cBuf - 1]);
        _cBuf -= 1;
        _cchRemaining = 0;
        cchBuffer /= 2;
    }

    _cchRemaining += cch;
    _pchCur = _cBuf ? _apchBuf[_cBuf - 1] + cchBuffer - _cchRemaining : NULL;

    Assert(_cBuf || _cchRemaining == 0);
}

//+------------------------------------------------------------------------
//
//  Method:     Chop
//
//  Synopsis:   Removes the last cch chars from the buffer
// 
//-------------------------------------------------------------------------
void
CBuffer2::TransferTo(CBuffer2 *pbuf)
{
    // copy over pointers
    if (pbuf != this)
    {
        memcpy(pbuf, this, sizeof(CBuffer2));
        memset(this, 0, sizeof(CBuffer2));
    }
}

