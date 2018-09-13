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

#include "shares.h"

VOID
MyFormatMessageText(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    IN va_list * parglist
    );

VOID
MyFormatMessage(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    ...
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

PSECURITY_DESCRIPTOR
CopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSecDesc
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

BOOL
DriveLetterShare(
    PWSTR pszShareName
    );

#if DBG == 1

VOID
DumpNetEnum(
    IN LPVOID pBufShares,
    IN ULONG entriesRead
    );

#endif // DBG == 1

struct SHARE_PROPSHEETPAGE
{
    PROPSHEETPAGE psp;
    PWSTR pszMachine;
    PWSTR pszShareName;
};

HRESULT
ShareDoProperties(
    IN IUnknown* punk,
    IN PWSTR pszMachine,
    IN PWSTR pszShareName
    );

HRESULT
ShareDoDelete(
    IN HWND hwndOwner,
    IN PWSTR pszMachine,
    IN PWSTR pszShareName
    );

HRESULT
ShareDoNew(
    IN IUnknown* punk,
    IN PWSTR pszMachine
    );

VOID FSSetStatusText(HWND hwndOwner, LPTSTR* ppszText, int iStart, int iEnd);

BOOL
IsLevelOk(
    IN PWSTR pszMachine,
    IN DWORD level
    );

VOID
SetDialogIconBig(
    IN HWND hwnd,
    WORD idIcon
    );

VOID
SetDialogIconSmall(
    IN HWND hwnd,
    WORD idIcon
    );

HRESULT
STRRETLoadString(
    UINT ids,
    STRRET* pstr
    );

HRESULT
STRRETCopy(
    LPTSTR pszString,
    STRRET* pstr
    );

VOID
FillSpecialID(
    LPIDSHARE pids,
    BYTE bFlags,        // SHID_SHARE_*
    UINT idsName
    );

VOID
StrNCopy(
    OUT LPWSTR pszTarget,
    IN LPCWSTR pszSource,
    IN DWORD cchTarget
    );

BOOL
LoadSFMSupportLibrary(
    VOID
    );

VOID
UnloadSFMSupportLibrary(
    VOID
    );

BOOL
LoadFPNWSupportLibrary(
    VOID
    );

VOID
UnloadFPNWSupportLibrary(
    VOID
    );

#endif // __UTIL_HXX__
