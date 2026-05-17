#
# PROJECT:     ReactOS Networking
# LICENSE:     GPL - See COPYING in the top level directory
# FILE:        lib/dhcpcsvc/dhcpcsvc.spec
# PURPOSE:     dhcpcsvc exports
# COPYRIGHT:   Copyright 2006 Ge van Geldorp <gvg@reactos.org>
#
@ stdcall DhcpAcquireParameters(wstr)
@ stdcall DhcpAcquireParametersByBroadcast(wstr)
@ stdcall DhcpCApiCleanup()
@ stdcall DhcpCApiInitialize(ptr)
@ stub DhcpDelPersistentRequestParams
@ stub DhcpDeRegisterOptions
@ stub DhcpDeRegisterParamChange
@ stdcall DhcpEnumClasses(long wstr long long)
@ stdcall DhcpFallbackRefreshParams(wstr)
@ stdcall DhcpHandlePnPEvent(long long wstr long long)
@ stub DhcpLeaseIpAddress
@ stub DhcpLeaseIpAddressEx
@ stdcall DhcpNotifyConfigChange(ptr ptr long long long long long)
@ stdcall DhcpNotifyConfigChangeEx(ptr ptr long long long long long long)
@ stub DhcpNotifyMediaReconnected
@ stub DhcpOpenGlobalEvent
@ stub DhcpPersistentRequestParams
@ stub DhcpQueryHWInfo
@ stub DhcpRegisterOptions
@ stub DhcpRegisterParamChange
@ stub DhcpReleaseIpAddressLease
@ stub DhcpReleaseIpAddressLeaseEx
@ stdcall DhcpReleaseParameters(wstr)
@ stdcall DhcpRemoveDNSRegistrations()
@ stub DhcpRenewIpAddressLease
@ stub DhcpRenewIpAddressLeaseEx
@ stub DhcpRequestOptions
@ stdcall DhcpRequestParams(long ptr ptr ptr long ptr long ptr ptr ptr ptr)
@ stdcall DhcpStaticRefreshParams(long long long)
@ stdcall DhcpUndoRequestParams(long ptr wstr wstr)
@ stub McastApiCleanup
@ stub McastApiStartup
@ stub McastEnumerateScopes
@ stub McastGenUID
@ stub McastReleaseAddress
@ stub McastRenewAddress
@ stub McastRequestAddress
@ stdcall ServiceMain(long ptr)
