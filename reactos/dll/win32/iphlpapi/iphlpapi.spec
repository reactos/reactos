@ stdcall AddIPAddress( long long long ptr ptr )
@ stub AllocateAndGetArpEntTableFromStack
@ stdcall AllocateAndGetIfTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpAddrTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpForwardTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpNetTableFromStack( ptr long long long )
@ stub AllocateAndGetTcpExTable2FromStack
@ stub AllocateAndGetTcpExTableFromStack
@ stdcall AllocateAndGetTcpTableFromStack( ptr long long long )
@ stub AllocateAndGetUdpExTable2FromStack
@ stub AllocateAndGetUdpExTableFromStack
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
@ stdcall -stub GetExtendedUdpTable( ptr ptr long long long long )
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
@ stub GetOwnerModuleFromUdpEntry
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
@ stub Icmp6CreateFile
@ stub Icmp6ParseReplies
@ stub Icmp6SendEcho2
@ stdcall IcmpCloseHandle(ptr)
@ stdcall IcmpCreateFile()
@ stdcall -stub IcmpParseReplies(ptr long)
@ stdcall -stub IcmpSendEcho2(ptr ptr ptr ptr long ptr long ptr ptr long long)
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
@ stdcall NhGetInterfaceNameFromDeviceGuid(long long long long long)
@ stub NhGetInterfaceNameFromGuid
@ stdcall NhpAllocateAndGetInterfaceInfoFromStack(ptr ptr long ptr long)
@ stub NhpGetInterfaceIndexFromStack
@ stdcall NotifyAddrChange( ptr ptr )
@ stdcall NotifyRouteChange( ptr ptr )
@ stub NotifyRouteChangeEx
@ stub NotifySecurityHealthChange
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
@ stub _PfAddFiltersToInterface@24
@ stub _PfAddGlobalFilterToInterface@8
@ stub _PfBindInterfaceToIPAddress@12
@ stub _PfBindInterfaceToIndex@16
@ stub _PfCreateInterface@24
@ stub _PfDeleteInterface@4
@ stub _PfDeleteLog@0
@ stub _PfGetInterfaceStatistics@16
@ stub _PfMakeLog@4
@ stub _PfRebindFilters@8
@ stub _PfRemoveFilterHandles@12
@ stub _PfRemoveFiltersFromInterface@20
@ stub _PfRemoveGlobalFilterFromInterface@8
@ stub _PfSetLogBuffer@28
@ stub _PfTestPacket@20
@ stub _PfUnBindInterface@4
@ stub do_echo_rep
@ stub do_echo_req
@ stub register_icmp
