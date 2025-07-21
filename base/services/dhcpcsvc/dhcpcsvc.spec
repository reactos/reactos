#
# PROJECT:     ReactOS Networking
# LICENSE:     GPL - See COPYING in the top level directory
# FILE:        lib/dhcpcsvc/dhcpcsvc.spec
# PURPOSE:     dhcpcsvc exports
# COPYRIGHT:   Copyright 2006 Ge van Geldorp <gvg@reactos.org>
#
@ stdcall DhcpAcquireParameters(wstr)
@ stub DhcpAcquireParametersByBroadcast
@ stdcall DhcpCApiCleanup()
@ stdcall DhcpCApiInitialize(ptr)
@ stub DhcpDelPersistentRequestParams
@ stub DhcpDeRegisterOptions
@ stub DhcpDeRegisterParamChange
@ stub DhcpEnumClasses
@ stub DhcpFallbackRefreshParams
@ stub DhcpHandlePnPEvent
@ stdcall DhcpLeaseIpAddress(long)
@ stub DhcpLeaseIpAddressEx
@ stdcall DhcpNotifyConfigChange(ptr ptr long long long long long)
@ stub DhcpNotifyConfigChangeEx
@ stub DhcpNotifyMediaReconnected
@ stub DhcpOpenGlobalEvent
@ stub DhcpPersistentRequestParams
@ stdcall DhcpQueryHWInfo(long ptr ptr ptr)
@ stub DhcpRegisterOptions
@ stub DhcpRegisterParamChange
@ stdcall DhcpReleaseIpAddressLease(long)
@ stub DhcpReleaseIpAddressLeaseEx
@ stdcall DhcpReleaseParameters(wstr)
@ stub DhcpRemoveDNSRegistrations
@ stdcall DhcpRenewIpAddressLease(long)
@ stub DhcpRenewIpAddressLeaseEx
@ stub DhcpRequestOptions
@ stdcall DhcpRequestParams(long ptr ptr ptr long ptr long ptr ptr ptr ptr)
@ stdcall DhcpStaticRefreshParams(long long long)
@ stub DhcpUndoRequestParams
@ stub McastApiCleanup
@ stub McastApiStartup
@ stub McastEnumerateScopes
@ stub McastGenUID
@ stub McastReleaseAddress
@ stub McastRenewAddress
@ stub McastRequestAddress
@ stdcall ServiceMain(long ptr)
