@ stub BreakRecordsIntoBlob
@ stub CombineRecordsInBlob
@ stdcall DnsAcquireContextHandle_A(long ptr ptr)
@ stdcall DnsAcquireContextHandle_UTF8(long ptr ptr)
@ stdcall DnsAcquireContextHandle_W(long ptr ptr)
@ stdcall DnsAddRecordSet_A()
@ stdcall DnsAddRecordSet_UTF8()
@ stdcall DnsAddRecordSet_W()
@ stdcall DnsAllocateRecord()
@ stdcall DnsApiAlloc(long)
@ stdcall DnsApiHeapReset()
@ stdcall DnsApiFree(ptr)
@ stub DnsApiRealloc
@ stub DnsApiSetDebugGlobals
@ stdcall DnsAsyncRegisterHostAddrs_A()
@ stdcall DnsAsyncRegisterHostAddrs_UTF8()
@ stdcall DnsAsyncRegisterHostAddrs_W()
@ stdcall DnsAsyncRegisterInit()
@ stdcall DnsAsyncRegisterTerm()
@ stdcall DnsCheckNameCollision_A()
@ stdcall DnsCheckNameCollision_UTF8()
@ stdcall DnsCheckNameCollision_W()
@ stdcall DnsCopyStringEx()
@ stdcall DnsCreateReverseNameStringForIpAddress()
@ stdcall DnsCreateStandardDnsNameCopy()
@ stdcall DnsCreateStringCopy()
@ stdcall DnsDhcpSrvRegisterHostName_W()
@ stdcall DnsDhcpSrvRegisterInit()
@ stdcall DnsDhcpSrvRegisterTerm()
@ stdcall DnsDisableAdapterDomainNameRegistration()
@ stdcall DnsDisableBNodeResolverThread()
@ stdcall DnsDisableDynamicRegistration()
@ stdcall DnsDowncaseDnsNameLabel()
@ stdcall DnsEnableAdapterDomainNameRegistration()
@ stdcall DnsEnableBNodeResolverThread()
@ stdcall DnsEnableDynamicRegistration()
@ stdcall DnsExtractRecordsFromMessage_UTF8(ptr long ptr)
@ stdcall DnsExtractRecordsFromMessage_W(ptr long ptr)
@ stdcall DnsFindAuthoritativeZone()
@ stdcall DnsFlushResolverCache()
@ stdcall DnsFlushResolverCacheEntry_A(str)
@ stdcall DnsFlushResolverCacheEntry_UTF8()
@ stdcall DnsFlushResolverCacheEntry_W()
@ stdcall DnsFree(ptr long)
@ stdcall DnsFreeAdapterInformation()
@ stub DnsFreeConfigStructure
@ stdcall DnsFreeNetworkInformation()
@ stdcall DnsFreeSearchInformation()
@ stdcall DnsGetBufferLengthForStringCopy()
@ stdcall DnsGetCacheDataTable(ptr)
@ stdcall DnsGetDnsServerList()
@ stdcall DnsGetDomainName()
@ stdcall DnsGetHostName_A()
@ stdcall DnsGetHostName_UTF8()
@ stdcall DnsGetHostName_W()
@ stdcall DnsGetIpAddressInfoList()
@ stdcall DnsGetIpAddressList()
@ stdcall DnsGetLastServerUpdateIP()
@ stdcall DnsGetMaxNumberOfAddressesToRegister()
@ stdcall DnsGetNetworkInformation()
@ stdcall DnsGetPrimaryDomainName_A()
@ stdcall DnsGetPrimaryDomainName_UTF8()
@ stdcall DnsGetPrimaryDomainName_W()
@ stdcall DnsGetSearchInformation()
@ stub DnsGlobals
@ stdcall DnsIpv6AddressToString()
@ stdcall DnsIpv6StringToAddress()
@ stdcall DnsIsAdapterDomainNameRegistrationEnabled()
@ stdcall DnsIsAMailboxType()
@ stdcall DnsIsDynamicRegistrationEnabled()
@ stdcall DnsIsStatusRcode()
@ stdcall DnsIsStringCountValidForTextType()
@ stdcall DnsMapRcodeToStatus()
@ stdcall DnsModifyRecordSet_A()
@ stdcall DnsModifyRecordSet_UTF8()
@ stdcall DnsModifyRecordSet_W()
@ stdcall DnsModifyRecordsInSet_A(ptr ptr long ptr ptr ptr)
@ stdcall DnsModifyRecordsInSet_UTF8(ptr ptr long ptr ptr ptr)
@ stdcall DnsModifyRecordsInSet_W(ptr ptr long ptr ptr ptr)
@ stdcall DnsNameCompareEx_A()
@ stdcall DnsNameCompareEx_UTF8()
@ stdcall DnsNameCompareEx_W()
@ stdcall DnsNameCompare_A(str str)
@ stdcall DnsNameCompare_W(wstr wstr)
@ stdcall DnsNameCopy()
@ stdcall DnsNameCopyAllocate()
@ stdcall DnsNotifyResolver()
@ stub DnsNotifyResolverClusterIp
@ stub DnsNotifyResolverEx
@ stdcall DnsQueryConfig(long long wstr ptr ptr ptr)
@ stdcall DnsQueryEx()
@ stdcall DnsQuery_A(str long long ptr ptr ptr)
@ stdcall DnsQuery_UTF8(str long long ptr ptr ptr)
@ stdcall DnsQuery_W(wstr long long ptr ptr ptr)
@ stdcall DnsRecordBuild_UTF8()
@ stdcall DnsRecordBuild_W()
@ stdcall DnsRecordCompare(ptr ptr)
@ stdcall DnsRecordCopyEx(ptr long long)
@ stdcall DnsRecordListFree(ptr long)
@ stdcall DnsRecordSetCompare(ptr ptr ptr ptr)
@ stdcall DnsRecordSetCopyEx(ptr long long)
@ stdcall DnsRecordSetDetach(ptr)
@ stdcall DnsRecordStringForType()
@ stdcall DnsRecordStringForWritableType()
@ stdcall DnsRecordTypeForName()
@ stub DnsRegisterClusterAddress
@ stdcall DnsRelationalCompare_UTF8()
@ stdcall DnsRelationalCompare_W()
@ stdcall DnsReleaseContextHandle(ptr)
@ stdcall DnsRemoveRegistrations()
@ stdcall DnsReplaceRecordSetA(ptr long ptr ptr ptr)
@ stdcall DnsReplaceRecordSet_A()
@ stdcall DnsReplaceRecordSetUTF8(ptr long ptr ptr ptr)
@ stdcall DnsReplaceRecordSet_UTF8()
@ stdcall DnsReplaceRecordSetW(ptr long ptr ptr ptr)
@ stdcall DnsReplaceRecordSet_W()
@ stdcall DnsServiceNotificationDeregister_A()
@ stdcall DnsServiceNotificationDeregister_UTF8()
@ stdcall DnsServiceNotificationDeregister_W()
@ stdcall DnsServiceNotificationRegister_A()
@ stdcall DnsServiceNotificationRegister_UTF8()
@ stdcall DnsServiceNotificationRegister_W()
@ stdcall DnsSetMaxNumberOfAddressesToRegister()
@ stdcall DnsStatusString()
@ stdcall DnsStringCopyAllocateEx()
@ stdcall DnsUnicodeToUtf8()
@ stdcall DnsUpdate()
@ stdcall DnsUpdateTest_A()
@ stdcall DnsUpdateTest_UTF8()
@ stdcall DnsUpdateTest_W()
@ stdcall DnsUtf8ToUnicode()
@ stdcall DnsValidateName_A(str long)
@ stdcall DnsValidateName_UTF8(str long)
@ stdcall DnsValidateName_W(wstr long)
@ stdcall DnsValidateUtf8Byte()
@ stdcall DnsWinsRecordFlagForString()
@ stdcall DnsWinsRecordFlagString()
@ stdcall DnsWriteQuestionToBuffer_UTF8(ptr ptr str long long long)
@ stdcall DnsWriteQuestionToBuffer_W(ptr ptr wstr long long long)
@ stdcall DnsWriteReverseNameStringForIpAddress()
@ stub Dns_AddRecordsToMessage
@ stub Dns_AllocateMsgBuf
@ stub Dns_BuildPacket
@ stub Dns_CacheSocketCleanup
@ stub Dns_CacheSocketInit
@ stub Dns_CleanupWinsock
@ stub Dns_CloseConnection
@ stub Dns_CloseHostFile
@ stub Dns_CloseSocket
@ stub Dns_CreateMulticastSocket
@ stub Dns_CreateSocket
@ stub Dns_CreateSocketEx
@ stub Dns_FindAuthoritativeZoneLib
@ stub Dns_GetIpAddresses
@ stub Dns_GetLocalIpAddressArray
@ stub Dns_GetRandomXid
@ stub Dns_InitializeMsgRemoteSockaddr
@ stub Dns_InitializeWinsock
@ stub Dns_InitQueryTimeouts
@ stub Dns_OpenHostFile
@ stub Dns_OpenTcpConnectionAndSend
@ stub Dns_ParseMessage
@ stub Dns_ParsePacketRecord
@ stub Dns_PingAdapterServers
@ stub Dns_ReadHostFileLine
@ stub Dns_ReadPacketName
@ stub Dns_ReadPacketNameAllocate
@ stub Dns_ReadRecordStructureFromPacket
@ stub Dns_RecvTcp
@ stub Dns_ResetNetworkInfo
@ stub Dns_SendAndRecvUdp
@ stub Dns_SendEx
@ stub Dns_SetRecordDatalength
@ stub Dns_SkipPacketName
@ stub Dns_SkipToRecord
@ stub Dns_UpdateLib
@ stub Dns_UpdateLibEx
@ stub Dns_WriteDottedNameToPacket
@ stub Dns_WriteQuestionToMessage
@ stub Dns_WriteRecordStructureToPacketEx
@ stdcall GetCurrentTimeInSeconds()
@ stub GetRecordsForLocalName
@ stub NetInfo_Build
@ stub NetInfo_Clean
@ stub NetInfo_Copy
@ stub NetInfo_Free
@ stub NetInfo_IsForUpdate
@ stub NetInfo_ResetServerPriorities
@ stub QueryDirectEx
@ stdcall Query_Main(wstr long long ptr)
@ stub Reg_ReadGlobalsEx
