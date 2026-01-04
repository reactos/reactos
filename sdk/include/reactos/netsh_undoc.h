#ifndef __NETSH_UNDOC_H__
#define __NETSH_UNDOC_H__

VOID
WINAPI
FreeQuotedString(
    _In_ LPWSTR pszQuotedString);

VOID
WINAPI
FreeString(
    _In_ LPWSTR pszString);

LPWSTR
WINAPI
MakeQuotedString(
    _In_ LPWSTR pszString);

LPWSTR
CDECL
MakeString(
    _In_ HANDLE hModule,
    _In_ DWORD dwMsgId,
    ...);

DWORD
WINAPI 
NsGetFriendlyNameFromIfName(
    _In_ DWORD dwParam1,
    _In_ PWSTR pszIfName, 
    _Inout_ PWSTR pszFriendlyName,
    _Inout_ PDWORD pdwFriendlyName);

#endif /* __NETSH_UNDOC_H__ */
