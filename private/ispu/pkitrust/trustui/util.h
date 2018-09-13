//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       util.h
//
//  Contents:   Utility functions
//
//  History:    12-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__UTIL_H__)
#define __UTIL_H__

//
// The following help us retrieve the publisher and publisher cert issuer
// names out of the cert context.  They are stolen from SOFTPUB.  Note
// that the returned strings must be CoTaskMemFree'd
//

extern void TUIGoLink(HWND hParent, WCHAR *pszWhere);
extern WCHAR *GetGoLink(SPC_LINK *psLink);

#endif
