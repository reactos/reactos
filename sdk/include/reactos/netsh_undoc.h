#ifndef __NETSH_UNDOC_H__
#define __NETSH_UNDOC_H__

DWORD
WINAPI 
NsGetFriendlyNameFromIfName(
    _In_ DWORD dwParam1,
    _In_ PWSTR pszIfName, 
    _Inout_ PWSTR pszFriendlyName,
    _Inout_ PDWORD pdwFriendlyName);

#endif /* __NETSH_UNDOC_H__ */
