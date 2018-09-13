//-----------------------------------------------------------------------------
//
// Microsoft Forms
// Copyright: (c) 1994-1997, Microsoft Corporation
// All rights Reserved.
// Information contained herein is Proprietary and Confidential.
//
// File         CBUFSTR.HXX
//
// Contents     Class definition for a buffered, appendable string class
//
// Classes      CBufferedStr
//
//
//  History:
//              7-10-97     t-chrisr     created
//-----------------------------------------------------------------------------

#ifndef I_CBUFSTR_HXX_
#define I_CBUFSTR_HXX_
#pragma INCMSG("--- Beg 'cbufstr.hxx'")

MtExtern(CBufferedStr)

class CBufferedStr
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBufferedStr))
    CBufferedStr::CBufferedStr() 
    {
        _pchBuf = NULL;
        _cchBufSize = 0;
        _cchIndex =0;
    }
    CBufferedStr::~CBufferedStr() { Free(); }

    /*
        Creates a buffer and initializes it with the supplied TCHAR*
    */
    HRESULT Set( const TCHAR* pch = NULL );
    void Free () { delete [] _pchBuf; }

    /*
        Adds at the end of the buffer, growing it it necessary.
    */
    HRESULT QuickAppend ( const TCHAR* pch ) { return(QuickAppend(pch, _tcslen(pch))); }
    HRESULT QuickAppend ( const TCHAR* pch , ULONG cch );

    /*
        Returns current size of the buffer
    */
    UINT    Size() { return _pchBuf ? _cchBufSize : 0; }

    /*
        Returns current length of the buffer string
    */
    UINT    Length() { return _pchBuf ? _cchIndex : 0; }

    operator LPTSTR () const { return _pchBuf; }

    TCHAR * _pchBuf;                // Actual buffer
    UINT    _cchBufSize;            // Size of _pchBuf
    UINT    _cchIndex;              // Length of _pchBuf
};

#pragma INCMSG("--- End 'cbufstr.hxx'")
#else
#pragma INCMSG("*** Dup 'cbufstr.hxx'")
#endif
