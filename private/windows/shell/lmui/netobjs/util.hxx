//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       util.hxx
//
//  Contents:   Misc helper functions
//
//  History:    5-Apr-95    BruceFo Created
//
//----------------------------------------------------------------------------

#ifndef __UTIL_HXX__
#define __UTIL_HXX__

//----------------------------------------------------------------------------

VOID
MyFormatMessageText(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    IN va_list * parglist
    );

DWORD
MyCommonDialog(
    IN HWND    hwnd,
    IN HRESULT dwMsgCode,
    IN PWSTR   pszCaption,
    IN DWORD   dwFlags,
    IN va_list arglist
    );

VOID
MyErrorDialog(
    IN HWND hwnd,
    IN HRESULT dwErrorCode,
    ...
    );

HRESULT
MessageFromError(
    NET_API_STATUS err
    );

//----------------------------------------------------------------------------

class CWaitCursor
{
public:
    CWaitCursor(UINT idResCursor = 0);
    ~CWaitCursor();

private:
    HCURSOR _hcurWait;
    HCURSOR _hcurOld;
};

//----------------------------------------------------------------------------

NET_API_STATUS
MyNetpGetDomainNameEx (
    IN  LPWSTR MachineName,
    OUT LPWSTR* DomainNamePtr, // alloc and set ptr (free with NetApiBufferFree)
    OUT PBOOL IsWorkgroupName
    );

#endif // __UTIL_HXX__
