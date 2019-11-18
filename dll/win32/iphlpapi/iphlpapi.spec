@ stdcall AddIPAddress( long long long ptr ptr )
@ stub AllocateAndGetArpEntTableFromStack
@ stdcall AllocateAndGetIfTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpAddrTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpForwardTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpNetTableFromStack( ptr long long long )
@ stdcall AllocateAndGetTcpExTable2FromStack( ptr long long long long long )
@ stdcall AllocateAndGetTcpExTableFromStack( ptr long long long long )
@ stdcall AllocateAndGetTcpTableFromStack( ptr long long long )
@ stdcall AllocateAndGetUdpExTable2FromStack( ptr long long long long long )
@ stdcall AllocateAndGetUdpExTableFromStack( ptr long long long long )
@ stdcall AllocateAndGetUdpTableFromStack( ptr long long long )
@ stdcall CancelIPChangeNotify(ptr)
@ stub CancelSecurityHealthChangeNotify
@ stdcall CreateIpForwardEntry( ptr )
@ stdcall CreateIpNetEntry( ptr )
@ stdcall CreateProxyArpEntry( long long long )
@ stdcall DeleteIPAddress( long )
@ stdcall DeleteIpForwardEntry( ptr )
@ stdcall DeleteIpNetEntry( ptr )
@ stdcall DeleteProxyArpEntry( long long long )
@ stdcall DisableMediaSense(ptr ptr)
@ stdcall EnableRouter( ptr ptr )
@ stdcall FlushIpNetTable( long )
@ stub FlushIpNetTableFromStack
@ stdcall GetAdapterIndex( wstr ptr )
@ stdcall GetAdapterOrderMap()
@ stdcall GetAdaptersAddresses( long long ptr ptr ptr )
@ stdcall GetAdaptersInfo( ptr ptr )
@ stdcall GetBestInterface( long ptr )
@ stdcall GetBestInterfaceEx(ptr ptr)
@ stub GetBestInterfaceFromStack
@ stdcall GetBestRoute( long long long )
@ stub GetBestRouteFromStack
@ stdcall GetExtendedTcpTable( ptr ptr long long long long )
@ stdcall GetExtendedUdpTable( ptr ptr long long long long )
@ stdcall GetFriendlyIfIndex( long )
@ stdcall GetIcmpStatistics( ptr )
@ stdcall GetIcmpStatisticsEx(ptr long)
@ stub GetIcmpStatsFromStack
@ stub GetIcmpStatsFromStackEx
@ stdcall GetIfEntry( ptr )
@ stub GetIfEntryFromStack
@ stdcall GetIfTable( ptr ptr long )
@ stub GetIfTableFromStack
@ stub GetIgmpList
@ stdcall GetInterfaceInfo( ptr ptr )
@ stdcall GetIpAddrTable( ptr ptr long )
@ stub GetIpAddrTableFromStack
@ stdcall GetIpErrorString(long ptr ptr)
@ stdcall GetIpForwardTable( ptr ptr long )
@ stub GetIpForwardTableFromStack
@ stdcall GetIpNetTable( ptr ptr long )
@ stub GetIpNetTableFromStack
@ stdcall GetIpStatistics( ptr )
@ stdcall GetIpStatisticsEx(ptr long)
@ stub GetIpStatsFromStack
@ stub GetIpStatsFromStackEx
@ stdcall GetNetworkParams( ptr ptr )
@ stdcall GetNumberOfInterfaces( ptr )
@ stub GetOwnerModuleFromTcp6Entry
@ stdcall GetOwnerModuleFromTcpEntry ( ptr long ptr ptr )
@ stub GetOwnerModuleFromUdp6Entry
@ stdcall GetOwnerModuleFromUdpEntry ( ptr long ptr ptr )
@ stdcall GetPerAdapterInfo( long ptr ptr )
@ stdcall GetRTTAndHopCount( long ptr long ptr )
@ stub GetTcpExTable2FromStack
@ stdcall GetTcpStatistics( ptr )
@ stdcall GetTcpStatisticsEx(ptr long)
@ stub GetTcpStatsFromStack
@ stub GetTcpStatsFromStackEx
@ stdcall GetTcpTable( ptr ptr long )
@ stub GetTcpTableFromStack
@ stub GetUdpExTable2FromStack
@ stdcall GetUdpStatistics( ptr )
@ stdcall GetUdpStatisticsEx(ptr long)
@ stub GetUdpStatsFromStack
@ stub  GetUdpStatsFromStackEx
@ stdcall GetUdpTable( ptr ptr long )
@ stub GetUdpTableFromStack
@ stdcall GetUniDirectionalAdapterInfo( ptr ptr )
@ stdcall Icmp6CreateFile()
@ stdcall Icmp6ParseReplies(ptr long)
@ stdcall Icmp6SendEcho2(ptr ptr ptr ptr ptr ptr ptr long ptr ptr long long)
@ stdcall IcmpCloseHandle(ptr)
@ stdcall IcmpCreateFile()
@ stdcall IcmpParseReplies(ptr long)
@ stdcall IcmpSendEcho2(ptr ptr ptr ptr long ptr long ptr ptr long long)
@ stdcall IcmpSendEcho(ptr long ptr long ptr ptr long long)
@ stub InternalCreateIpForwardEntry
@ stub InternalCreateIpNetEntry
@ stub InternalDeleteIpForwardEntry
@ stub InternalDeleteIpNetEntry
@ stub InternalGetIfTable
@ stub InternalGetIpAddrTable
@ stub InternalGetIpForwardTable
@ stub InternalGetIpNetTable
@ stub InternalGetTcpTable
@ stub InternalGetUdpTable
@ stub InternalSetIfEntry
@ stub InternalSetIpForwardEntry
@ stub InternalSetIpNetEntry
@ stub InternalSetIpStats
@ stub InternalSetTcpEntry
@ stdcall IpReleaseAddress( ptr )
@ stdcall IpRenewAddress( ptr )
@ stub IsLocalAddress
@ stub NTPTimeToNTFileTime
@ stub NTTimeToNTPTime
@ stub NhGetGuidFromInterfaceName
@ stdcall NhGetInterfaceNameFromDeviceGuid(ptr ptr ptr long long)
@ stdcall NhGetInterfaceNameFromGuid(ptr ptr ptr long long)
@ stdcall NhpAllocateAndGetInterfaceInfoFromStack(ptr ptr long ptr long)
@ stub NhpGetInterfaceIndexFromStack
@ stdcall NotifyAddrChange( ptr ptr )
@ stdcall NotifyRouteChange( ptr ptr )
@ stub NotifyRouteChangeEx
@ stub NotifySecurityHealthChange
@ stdcall _PfAddFiltersToInterface@24()
@ stdcall _PfAddGlobalFilterToInterface@8()
@ stdcall _PfBindInterfaceToIPAddress@12()
@ stdcall _PfBindInterfaceToIndex@16()
@ stdcall _PfCreateInterface@24()
@ stdcall _PfDeleteInterface@4()
@ stdcall _PfDeleteLog@0()
@ stdcall _PfGetInterfaceStatistics@16()
@ stdcall _PfMakeLog@4()
@ stdcall _PfRebindFilters@8()
@ stdcall _PfRemoveFilterHandles@12()
@ stdcall _PfRemoveFiltersFromInterface@20()
@ stdcall _PfRemoveGlobalFilterFromInterface@8()
@ stdcall _PfSetLogBuffer@28()
@ stdcall _PfTestPacket@20()
@ stdcall _PfUnBindInterface@4()
@ stdcall RestoreMediaSense(ptr ptr)
@ stdcall SendARP(long long ptr ptr)
@ stub SetAdapterIpAddress
@ stub SetBlockRoutes
@ stdcall SetIfEntry( ptr )
@ stub SetIfEntryToStack
@ stdcall SetIpForwardEntry( ptr )
@ stdcall SetIpForwardEntryToStack( ptr )
@ stub SetIpMultihopRouteEntryToStack
@ stdcall SetIpNetEntry( ptr )
@ stub SetIpNetEntryToStack
@ stub SetIpRouteEntryToStack
@ stdcall SetIpStatistics( ptr )
@ stub SetIpStatsToStack
@ stdcall SetIpTTL( long )
@ stub SetProxyArpEntryToStack
@ stub SetRouteWithRef
@ stdcall SetTcpEntry( ptr )
@ stub SetTcpEntryToStack
@ stdcall UnenableRouter( ptr ptr )
@ stub do_echo_rep
@ stub do_echo_req
@ stub register_icmp
