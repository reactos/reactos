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

DWORD
MyConfirmationDialog(
    IN HWND hwnd,
    IN HRESULT dwMsgCode,
    IN DWORD dwFlags,
    ...
    );

VOID
MyErrorDialog(
    IN HWND hwnd,
    IN HRESULT dwErrorCode,
    ...
    );

PWSTR
NewDup(
    IN const WCHAR* psz
    );

PWSTR
GetResourceString(
    IN DWORD dwId
    );

PWSTR
FindLastComponent(
    IN WCHAR* pszStr
    );

PSECURITY_DESCRIPTOR
CopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc
    );

UINT
WarnDelShare(
    IN HWND hwnd,
    IN UINT idMsg,
    IN PWSTR pszShare,
    IN PWSTR pszPath
    );

DWORD
ConfirmStopShare(
    IN HWND hDlg,
    IN UINT uType,
    IN LPWSTR pszName
    );

NET_API_STATUS
ShareConnectionInfo(
    IN LPWSTR pszShare,
    OUT LPDWORD pcConns,
    OUT LPDWORD pcOpens
    );

VOID
DisplayError(
    IN HWND           hwnd,
    IN HRESULT        dwErrorCode,
    IN NET_API_STATUS err,
    IN PWSTR          pszShare
    );

VOID
DisplayLanmanError(
    IN HWND           hwnd,
    IN HRESULT        dwErrorCode,
    IN NET_API_STATUS err,
    IN PWSTR          pszShare
    );

BOOL
IsValidShareName(
    IN PCWSTR pszShareName,
    OUT HRESULT* uId
    );

VOID
SetErrorFocus(
    IN HWND hwnd,
    IN UINT idCtrl
    );

DWORD
ConfirmReplaceShare(
    IN HWND hwnd,
    IN PCWSTR pszShareName,
    IN PCWSTR pszOldPath,
    IN PCWSTR pszNewPath
    );

BOOL
IsWorkstationProduct(
    VOID
    );

#endif // __UTIL_HXX__
