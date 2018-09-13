//+----------------------------------------------------------------------------
//  File:       mime64.hxx
//
//  Synopsis:   This file contains MIME64 encode/decode definitions
//
//-----------------------------------------------------------------------------


#ifndef _MIME64_HXX
#define _MIME64_HXX


// Prototypes ----------------------------------------------------------------
HRESULT ProcessAttachMIME64();
HRESULT EncodeMIME64(BYTE * pbSrc, UINT cbSrc, IStream * pstmDest, ULONG * pcbWritten);
HRESULT DecodeMIME64(IStream * pstmSrc, IStream * pstmDest, ULONG * pcbWritten);


#endif  // _MIME64_HXX

