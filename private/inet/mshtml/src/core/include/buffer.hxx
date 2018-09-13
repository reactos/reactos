//    Buffer.hxx
//        Copyright 1996 Microsoft Corporation.  All rights reserved.
//
//        This file declares a class that provides a dynamically growing
//    buffer.
//
//    1/5/96 created - Chris Wilson (cwilso@microsoft.com)

#ifndef I_BUFFER_HXX_
#define I_BUFFER_HXX_
#pragma INCMSG("--- Beg 'buffer.hxx'")

class CStr;

//---------------------------------------------------------------------
//    Class Declaration:    CBuffer
//        The CBuffer class represents a dynamically growable buffer.  It
//    works just like the CString MFC class, except it does not implement
//    as many methods.
//---------------------------------------------------------------------
class CBuffer {
public:
    // Constructor - pass in the initial buffer size (granularity is
    // set to 1/4 of initial size).  If size==0, granularity is set to 32.
    CBuffer();
    ~CBuffer();

    // Trim any whitespace at the end of the string.
    void TrimTrailingWhitespace( void );
    // Swaps contents with another CBuffer
    void SwapContents( CBuffer & cBufOther );
    // Clear() clears the buffer & sets length to 0, but does not free memory.
    inline void Clear(void);
    // Append a character to the end of the buffer.
    inline void Append( const TCHAR chChar );
    // Append a NUL terminated string to the end of the buffer.
    inline void Append( LPCTSTR pcsz );
    // Append characters to the end of the buffer
    inline void Append( LPCTSTR pcsz, int iLen );
    // Set the buffer to new string (src doesn't need to be NUL terminated)
    inline void Set( const TCHAR * const pCharArray, const long lArrayLen);

    // Access the buffer as a LPTSTR.  Since we keep the
    // string terminated, this implementation is trivial.
    // It would be very nice to return a const LPTSTR (i.e. LPCTSTR)
    // but that would require more changes in other files.
    inline operator LPTSTR ( void ) const { return m_pszStringBuf; }
    inline Length( void ) const { return m_lStringLen; }

protected:
    // Grows the buffer by iSize + granularity.
    BOOL GrowBuffer( const int iSize = 1 );

protected:
    LPTSTR     m_pszStringBuf;    // The buffer
    LPTSTR     m_pszCurrChar;     // The current location in the buffer
    long       m_lBufSize;        // Current buffer length (in # of chars) including space for NUL
    long       m_lStringLen;      // Current string length (in # of chars, excluding NUL)

private:
    // Disallow copying operations
    NO_COPY( CBuffer );
};


//+------------------------------------------------------------------------
//
//  Class:      CBuffer2
// 
//  Synopsis:   Another string buffer implementation that never copies
//              memory to relocate it after it's been inserted in the
//              buffer.
//
//              Uses exponential allocation to reduce MemAlloc calls.
//
//-------------------------------------------------------------------------

#define INIT_BUFFER2_SIZE 1024    // Initial buffer size
#define MAX_BUFFER2_GROWTH 32     // Buffers up to 2^32 in size

class CBuffer2
{
public:
    CBuffer2();
    ~CBuffer2();

    HRESULT Append(TCHAR *pch, int cch);
    HRESULT Append(TCHAR *pch) { return Append(pch, _tcslen(pch)); }

    void Chop(int cch); // removes the last cch chars in the buffer
    
    void Clear();
    int Length();

    void TransferTo(CBuffer2 *pbuf2);

    HRESULT SetCStr(CStr *pcstr);

private:
    TCHAR *_pchCur;
    int _cchRemaining;
    int _cBuf;
    
    TCHAR *_apchBuf[MAX_BUFFER2_GROWTH];
};


//---------------------------------------------------------------------
//      CBuffer::Clear()
//              This method clears the buffer, but does not delete the buffer
//      memory space.  This is intensely useful for performance reasons.  This
//      function should not fail if called on an uninitialized buffer.
//---------------------------------------------------------------------
inline void CBuffer::Clear(void)
{
        // Reset the current position
        m_pszCurrChar = m_pszStringBuf;

        // Terminate the buffer (if there is one).
        if ( m_pszStringBuf )
                *m_pszStringBuf = _T('\0');

        // Length is now zero.
        m_lStringLen = 0;
}

inline void CBuffer::Append( const TCHAR chChar )
{
    // If our buffer is full and we fail to obtain more space..
    if ( ((m_lStringLen+1) >= m_lBufSize) && (!GrowBuffer(1)) )
        return;		// ..just return.

    m_lStringLen += 1;
    *m_pszCurrChar++ = chChar;      // Tack the new char on the end.
    *m_pszCurrChar = _T('\0');      // Set the next char to '\0', to terminate the string.
}

inline void CBuffer::Append( LPCTSTR pcsz )
{
    Append( pcsz, _tcsclen(pcsz) );
}

inline void CBuffer::Append( LPCTSTR pcsz, int iLen )
{
    // If our buffer is full and we fail to obtain more space..
    if ( ((m_lStringLen+iLen) >= m_lBufSize) && (!GrowBuffer(iLen)) )
        return;     // ..just return.

    // Copy source string, including NUL terminator
    CopyMemory(m_pszCurrChar, pcsz, iLen*sizeof(TCHAR));
    m_lStringLen += iLen;
    m_pszCurrChar += iLen;
    *m_pszCurrChar = _T('\0');
}

inline void CBuffer::Set( const TCHAR * const pCharArray, const long lCharCount )
{
    // If our buffer isn't big enough to hold iLength chars + NUL, and
    // we fail to obtain enough space..
    if ( (((long)(lCharCount * sizeof(TCHAR))) >= m_lBufSize) && (!GrowBuffer(lCharCount)) )
        return;    // ..just return.

    memcpy(m_pszStringBuf, pCharArray, lCharCount * sizeof(TCHAR));
    m_pszCurrChar = m_pszStringBuf + lCharCount;
    *m_pszCurrChar = _T('\0');        // Set the last char to '\0', to terminate the string.
    m_lStringLen = lCharCount;
}

//------------------------------------------------------------
//
//  Class : CDataListEnumerator
//
//      A simple parseing helper -- this returns a token separated by spaces
//          or by a separation character that is passed into the constructor.
//
//------------------------------------------------------------
class CDataListEnumerator
{
private:
        LPCTSTR pStr;
        LPCTSTR pStart;
        TCHAR   chSeparator;

public:
        CDataListEnumerator ( LPCTSTR pszString, TCHAR ch=_T(' '))
        {
            Assert ( pszString );
            pStr = pszString;
            chSeparator = ch;
        };

        BOOL    GetNext ( LPCTSTR *ppszNext, INT *pnLen );
};

#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)

inline BOOL 
CDataListEnumerator::GetNext ( LPCTSTR *ppszNext, INT *pnLen )
{
    // Find the start
    while ( ISSPACE(*pStr) ) pStr++;
    pStart = pStr;

    if ( !*pStr )
        return FALSE;

    // Find the end of the next token
    for (;!(*pStr == _T('\0') || *pStr == chSeparator || ISSPACE(*pStr)); pStr++);

    *pnLen = pStr - pStart;
    *ppszNext = pStart;

    return TRUE;
}



#pragma INCMSG("--- End 'buffer.hxx'")
#else
#pragma INCMSG("--- Dup 'buffer.hxx'")
#endif
