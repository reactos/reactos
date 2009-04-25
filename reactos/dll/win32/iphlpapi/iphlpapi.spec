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
@ stub CancelIPChangeNotify
@ stub CancelSecurityHealthChangeNotify
@ stdcall CreateIpForwardEntry( ptr )
@ stdcall CreateIpNetEntry( ptr )
@ stdcall CreateProxyArpEntry( long long long )
@ stdcall DeleteIPAddress( long )
@ stdcall DeleteIpForwardEntry( ptr )
@ stdcall DeleteIpNetEntry( ptr )
@ stdcall DeleteProxyArpEntry( long long long )
@ stub DisableMediaSense
@ stdcall EnableRouter( ptr ptr )
@ stdcall FlushIpNetTable( long )
@ stub FlushIpNetTableFromStack
@ stdcall GetAdapterIndex( wstr ptr )
@ stub GetAdapterOrderMap
@ stdcall GetAdaptersAddresses( long long ptr ptr ptr )
@ stdcall GetAdaptersInfo( ptr ptr )
@ stdcall GetBestInterface( long ptr )
@ stub GetBestInterfaceEx
@ stub GetBestInterfaceFromStack
@ stdcall GetBestRoute( long long long )
@ stub GetBestRouteFromStack
@ stub GetExtendedTcpTable
@ stub GetExtendedUdpTable
@ stdcall GetFriendlyIfIndex( long )
@ stdcall GetIcmpStatistics( ptr )
@ stub GetIcmpStatisticsEx
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
@ stub GetIpErrorString
@ stdcall GetIpForwardTable( ptr ptr long )
@ stub GetIpForwardTableFromStack
@ stdcall GetIpNetTable( ptr ptr long )
@ stub GetIpNetTableFromStack
@ stdcall GetIpStatistics( ptr )
@ stub GetIpStatisticsEx
@ stub GetIpStatsFromStack
@ stub GetIpStatsFromStackEx
@ stdcall GetNetworkParams( ptr ptr )
@ stdcall GetNumberOfInterfaces( ptr )
@ stub GetOwnerModuleFromTcp6Entry
@ stub GetOwnerModuleFromTcpEntry
@ stub GetOwnerModuleFromUdp6Entry
@ stub GetOwnerModuleFromUdpEntry
@ stdcall GetPerAdapterInfo( long ptr ptr )
@ stdcall GetRTTAndHopCount( long ptr long ptr )
@ stub GetTcpExTable2FromStack
@ stdcall GetTcpStatistics( ptr )
@ stub GetTcpStatisticsEx
@ stub GetTcpStatsFromStack
@ stub GetTcpStatsFromStackEx
@ stdcall GetTcpTable( ptr ptr long )
@ stub GetTcpTableFromStack
@ stub GetUdpExTable2FromStack
@ stdcall GetUdpStatistics( ptr )
@ stub GetUdpStatisticsEx
@ stub GetUdpStatsFromStack
@ stub  GetUdpStatsFromStackEx
@ stdcall GetUdpTable( ptr ptr long )
@ stub GetUdpTableFromStack
@ stdcall GetUniDirectionalAdapterInfo( ptr ptr )
@ stub Icmp6CreateFile
@ stub Icmp6ParseReplies
@ stub Icmp6SendEcho2
@ stub IcmpCloseHandle
@ stub IcmpCreateFile
@ stub IcmpParseReplies
@ stub IcmpSendEcho
@ stub IcmpSendEcho2
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
@ stub NhGetInterfaceNameFromDeviceGuid
@ stub NhGetInterfaceNameFromGuid
@ stub NhpAllocateAndGetInterfaceInfoFromStack
@ stub NhpGetInterfaceIndexFromStack
@ stdcall NotifyAddrChange( ptr ptr )
@ stdcall NotifyRouteChange( ptr ptr )
@ stub NotifyRouteChangeEx
@ stub NotifySecurityHealthChange
@ stub RestoreMediaSense
@ stub SendARP
@ stub SetAdapterIpAddress
@ stub SetBlockRoutes
@ stdcall SetIfEntry( ptr )
@ stub SetIfEntryToStack
@ stdcall SetIpForwardEntry( ptr )
@ stub SetIpForwardEntryToStack
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
