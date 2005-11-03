 1 stdcall AddIPAddress( long long long ptr ptr )
@ stub AllocateAndGetArpEntTableFromStack
@ stdcall AllocateAndGetIfTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpAddrTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpForwardTableFromStack( ptr long long long )
@ stdcall AllocateAndGetIpNetTableFromStack( ptr long long long )
@ stdcall AllocateAndGetTcpTableFromStack( ptr long long long )
@ stdcall AllocateAndGetUdpTableFromStack( ptr long long long )
@ stdcall CreateIpForwardEntry( ptr )
@ stdcall CreateIpNetEntry( ptr )
@ stdcall CreateProxyArpEntry( long long long )
@ stdcall DeleteIPAddress( long )
@ stdcall DeleteIpForwardEntry( ptr )
@ stdcall DeleteIpNetEntry( ptr )
@ stdcall DeleteProxyArpEntry( long long long )
@ stdcall EnableRouter( ptr ptr )
@ stdcall FlushIpNetTable( long )
@ stub FlushIpNetTableFromStack
@ stdcall GetAdapterIndex( wstr ptr )
@ stub GetAdapterOrderMap
@ stdcall GetAdaptersInfo( ptr ptr )
@ stdcall GetBestInterface( long ptr )
@ stub GetBestInterfaceFromStack
@ stdcall GetBestRoute( long long long )
@ stub GetBestRouteFromStack
@ stdcall GetFriendlyIfIndex( long )
@ stdcall GetIcmpStatistics( ptr )
@ stub GetIcmpStatsFromStack
@ stdcall GetIfEntry( ptr )
@ stub GetIfEntryFromStack
@ stdcall GetIfTable( ptr ptr long )
@ stub GetIfTableFromStack
@ stub GetIgmpList
@ stdcall GetInterfaceInfo( ptr ptr )
@ stdcall GetIpAddrTable( ptr ptr long )
@ stub GetIpAddrTableFromStack
@ stdcall GetIpForwardTable( ptr ptr long )
@ stub GetIpForwardTableFromStack
@ stdcall GetIpNetTable( ptr ptr long )
@ stub GetIpNetTableFromStack
@ stdcall GetIpStatistics( ptr )
@ stub GetIpStatsFromStack
@ stdcall GetNetworkParams( ptr ptr )
@ stdcall GetNumberOfInterfaces( ptr )
@ stdcall GetPerAdapterInfo( long ptr ptr )
@ stdcall GetRTTAndHopCount( long ptr long ptr )
@ stdcall GetTcpStatistics( ptr )
@ stub GetTcpStatsFromStack
@ stdcall GetTcpTable( ptr ptr long )
@ stub GetTcpTableFromStack
@ stdcall GetUdpStatistics( ptr )
@ stub GetUdpStatsFromStack
@ stdcall GetUdpTable( ptr ptr long )
@ stub GetUdpTableFromStack
@ stdcall GetUniDirectionalAdapterInfo( ptr ptr )
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
@ stub NhGetInterfaceNameFromGuid
@ stub NhpAllocateAndGetInterfaceInfoFromStack
@ stub NhpGetInterfaceIndexFromStack
@ stdcall NotifyAddrChange( ptr ptr )
@ stdcall NotifyRouteChange( ptr ptr )
@ stub NotifyRouteChangeEx
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
110 stdcall SetIpStatistics( ptr )
@ stub SetIpStatsToStack
112 stdcall SetIpTTL( long )
@ stub SetProxyArpEntryToStack
@ stub SetRouteWithRef
@ stdcall SetTcpEntry( ptr )
@ stub SetTcpEntryToStack
 99 stdcall SendARP( long long ptr ptr )
117 stdcall UnenableRouter( ptr ptr )
