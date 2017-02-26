# ordinal exports
 1 stub MPR_1
 2 stub MPR_2
 3 stub MPR_3
 4 stub MPR_4
 5 stub MPR_5
 6 stub MPR_6
 7 stub MPR_7
 8 stub MPR_8
 9 stub MPR_9
12 stub MPR_12
13 stub MPR_13
14 stub MPR_14
15 stub MPR_15
16 stub MPR_16
17 stub MPR_17
18 stub MPR_18
19 stub MPR_19
20 stub MPR_20
21 stub MPR_21
22 stdcall @(long) MPR_Alloc
23 stdcall @(ptr long) MPR_ReAlloc
24 stdcall @(ptr) MPR_Free
25 stdcall @(ptr long) _MPR_25

@ stdcall MultinetGetConnectionPerformanceA(ptr ptr)
@ stdcall MultinetGetConnectionPerformanceW(ptr ptr)
@ stdcall MultinetGetErrorTextA(long long long)
@ stdcall MultinetGetErrorTextW(long long long)
@ stdcall NPSAuthenticationDialogA(ptr)
@ stdcall NPSCopyStringA(str ptr ptr)
@ stdcall NPSDeviceGetNumberA(str ptr ptr)
@ stdcall NPSDeviceGetStringA(long long ptr ptr)
@ stdcall NPSGetProviderHandleA(ptr)
@ stdcall NPSGetProviderNameA(long ptr)
@ stdcall NPSGetSectionNameA(long ptr)
@ stdcall NPSNotifyGetContextA(ptr)
@ stdcall NPSNotifyRegisterA(long ptr)
@ stdcall NPSSetCustomTextA(str)
@ stdcall NPSSetExtendedErrorA(long str)
@ stdcall PwdChangePasswordA(str ptr long ptr)
@ stdcall PwdChangePasswordW(wstr ptr long ptr)
@ stdcall PwdGetPasswordStatusA(str long ptr)
@ stdcall PwdGetPasswordStatusW(wstr long ptr)
@ stdcall PwdSetPasswordStatusA(str long long)
@ stdcall PwdSetPasswordStatusW(wstr long long)
@ stdcall WNetAddConnection2A(ptr str str long)
@ stdcall WNetAddConnection2W(ptr wstr wstr long)
@ stdcall WNetAddConnection3A(long ptr str str long)
@ stdcall WNetAddConnection3W(long ptr wstr wstr long)
@ stdcall WNetAddConnectionA(str str str)
@ stdcall WNetAddConnectionW(wstr wstr wstr)
@ stdcall WNetCachePassword(str long str long long long)
@ stdcall WNetCancelConnection2A(str long long)
@ stdcall WNetCancelConnection2W(wstr long long)
@ stdcall WNetCancelConnectionA(str long)
@ stdcall WNetCancelConnectionW(wstr long)
@ stdcall WNetCloseEnum(long)
@ stdcall WNetConnectionDialog1A(ptr)
@ stdcall WNetConnectionDialog1W(ptr)
@ stdcall WNetConnectionDialog(long long)
@ stdcall WNetDisconnectDialog1A(ptr)
@ stdcall WNetDisconnectDialog1W(ptr)
@ stdcall WNetDisconnectDialog(long long)
@ stdcall WNetEnumCachedPasswords(str long long ptr long)
@ stdcall WNetEnumResourceA(long ptr ptr ptr)
@ stdcall WNetEnumResourceW(long ptr ptr ptr)
@ stub WNetFMXEditPerm
@ stub WNetFMXGetPermCaps
@ stub WNetFMXGetPermHelp
@ stub WNetFormatNetworkNameA
@ stub WNetFormatNetworkNameW
@ stdcall WNetGetCachedPassword(ptr long ptr ptr long)
@ stdcall WNetGetConnectionA(str ptr ptr)
@ stdcall WNetGetConnectionW(wstr ptr ptr)
@ stub WNetGetDirectoryTypeA
@ stub WNetGetHomeDirectoryA
@ stub WNetGetHomeDirectoryW
@ stdcall WNetGetLastErrorA(ptr ptr long ptr long)
@ stdcall WNetGetLastErrorW(ptr ptr long ptr long)
@ stdcall WNetGetNetworkInformationA(str ptr)
@ stdcall WNetGetNetworkInformationW(wstr ptr)
@ stub WNetGetPropertyTextA
@ stdcall WNetGetProviderNameA(long ptr ptr)
@ stdcall WNetGetProviderNameW(long ptr ptr)
@ stdcall WNetGetResourceInformationA(ptr ptr ptr ptr)
@ stdcall WNetGetResourceInformationW(ptr ptr ptr ptr)
@ stdcall WNetGetResourceParentA(ptr ptr ptr)
@ stdcall WNetGetResourceParentW(ptr ptr ptr)
@ stdcall WNetGetUniversalNameA (str long ptr ptr)
@ stdcall WNetGetUniversalNameW (wstr long ptr ptr)
@ stdcall WNetGetUserA(str ptr ptr)
@ stdcall WNetGetUserW(wstr wstr ptr)
@ stdcall WNetLogoffA(str long)
@ stdcall WNetLogoffW(wstr long)
@ stdcall WNetLogonA(str long)
@ stub WNetLogonNotify
@ stdcall WNetLogonW(wstr long)
@ stdcall WNetOpenEnumA(long long long ptr ptr)
@ stdcall WNetOpenEnumW(long long long ptr ptr)
@ stub WNetPasswordChangeNotify
@ stub WNetPropertyDialogA
@ stdcall WNetRemoveCachedPassword(long long long)
@ stub WNetRestoreConnection
@ stdcall WNetRestoreConnectionA(long str)
@ stdcall WNetRestoreConnectionW(long wstr)
@ stdcall WNetSetConnectionA(str long ptr)
@ stdcall WNetSetConnectionW(wstr long ptr)
@ stdcall WNetUseConnectionA(long ptr str str long str ptr ptr)
@ stdcall WNetUseConnectionW(long ptr wstr wstr long wstr ptr ptr)
@ stdcall WNetVerifyPasswordA(str ptr)
@ stdcall WNetVerifyPasswordW(wstr ptr)
