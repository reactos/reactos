@ stub DsAddressToSiteNames
@ stub DsAddressToSiteNamesEx
@ stub DsDeregisterDnsHostRecords
@ stub DsEnumerateDomainTrusts
@ stub DsGetDcClose
@ stdcall DsGetDcNameA(str str ptr str long ptr)
@ stdcall DsGetDcNameW(wstr wstr ptr wstr long ptr)
@ stub DsGetDcNext
@ stub DsGetDcOpen
@ stub DsGetDcSiteCoverage
@ stub DsGetForestTrustInformationW
@ stub DsGetSiteNameA # (str str)
@ stdcall DsGetSiteNameW(wstr wstr)
@ stub DsMergeForestTrustInformationW
@ stdcall DsRoleFreeMemory(ptr)
@ stdcall DsRoleGetPrimaryDomainInformation(wstr long ptr)
@ stub DsValidateSubnetName
@ stub I_BrowserDebugCall
@ stub I_BrowserDebugTrace
@ stdcall I_BrowserQueryEmulatedDomains(wstr ptr ptr)
@ stub I_BrowserQueryOtherDomains
@ stub I_BrowserQueryStatistics
@ stub I_BrowserResetNetlogonState
@ stub I_BrowserResetStatistics
@ stub I_BrowserServerEnum
@ stdcall I_BrowserSetNetlogonState(wstr wstr wstr long)
@ stub I_NetAccountDeltas
@ stub I_NetAccountSync
@ stub I_NetDatabaseDeltas
@ stub I_NetDatabaseRedo
@ stub I_NetDatabaseSync2
@ stub I_NetDatabaseSync
@ stub I_NetDfsCreateExitPoint
@ stub I_NetDfsCreateLocalPartition
@ stub I_NetDfsDeleteExitPoint
@ stub I_NetDfsDeleteLocalPartition
@ stub I_NetDfsFixLocalVolume
@ stub I_NetDfsGetVersion
@ stub I_NetDfsIsThisADomainName
@ stub I_NetDfsModifyPrefix
@ stub I_NetDfsSetLocalVolumeState
@ stub I_NetDfsSetServerInfo
@ stub I_NetGetDCList
@ stub I_NetListCanonicalize
@ stub I_NetListTraverse
@ stub I_NetLogonControl2
@ stub I_NetLogonControl
@ stub I_NetLogonSamLogoff
@ stub I_NetLogonSamLogon
@ stub I_NetLogonUasLogoff
@ stub I_NetLogonUasLogon
@ stub I_NetNameCanonicalize
@ stdcall I_NetNameCompare(ptr wstr wstr ptr ptr)
@ stdcall I_NetNameValidate(ptr wstr ptr ptr)
@ stub I_NetPathCanonicalize
@ stub I_NetPathCompare
@ stub I_NetPathType
@ stub I_NetServerAuthenticate2
@ stub I_NetServerAuthenticate
@ stub I_NetServerPasswordSet
@ stub I_NetServerReqChallenge
@ stub I_NetServerSetServiceBits
@ stub I_NetServerSetServiceBitsEx
@ stub NetAlertRaise
@ stub NetAlertRaiseEx
@ stdcall NetApiBufferAllocate(long ptr)
@ stdcall NetApiBufferFree(ptr)
@ stdcall NetApiBufferReallocate(ptr long ptr)
@ stdcall NetApiBufferSize(ptr ptr)
@ stub NetAuditClear
@ stub NetAuditRead
@ stub NetAuditWrite
@ stub NetBrowserStatisticsGet
@ stub NetConfigGet
@ stub NetConfigGetAll
@ stub NetConfigSet
@ stub NetConnectionEnum
@ stub NetDfsAdd
@ stub NetDfsEnum
@ stub NetDfsGetInfo
@ stub NetDfsManagerGetConfigInfo
@ stub NetDfsMove
@ stub NetDfsRemove
@ stub NetDfsRename
@ stub NetDfsSetInfo
@ stub NetEnumerateTrustedDomains
@ stub NetErrorLogClear
@ stub NetErrorLogRead
@ stub NetErrorLogWrite
@ stub NetFileClose
@ stub NetFileEnum
@ stub NetFileGetInfo
@ stub NetGetAnyDCName
@ stdcall NetGetDCName(wstr wstr ptr)
@ stub NetGetDisplayInformationIndex
@ stdcall NetGetJoinInformation(wstr ptr ptr)
@ stub NetGroupAdd
@ stub NetGroupAddUser
@ stub NetGroupDel
@ stub NetGroupDelUser
@ stub NetGroupEnum
@ stub NetGroupGetInfo
@ stub NetGroupGetUsers
@ stub NetGroupSetInfo
@ stub NetGroupSetUsers
@ stdcall NetLocalGroupAdd(wstr long ptr ptr)
@ stdcall NetLocalGroupAddMember(wstr wstr ptr)
@ stdcall NetLocalGroupAddMembers(wstr wstr long ptr long)
@ stdcall NetLocalGroupDel(wstr wstr)
@ stdcall NetLocalGroupDelMember(wstr wstr ptr)
@ stdcall NetLocalGroupDelMembers(wstr wstr long ptr long)
@ stdcall NetLocalGroupEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetLocalGroupGetInfo(wstr wstr long ptr)
@ stdcall NetLocalGroupGetMembers(wstr wstr long ptr long ptr ptr ptr)
@ stdcall NetLocalGroupSetInfo(wstr wstr long ptr ptr)
@ stdcall NetLocalGroupSetMembers(wstr wstr long ptr long)
@ stub NetMessageBufferSend
@ stub NetMessageNameAdd
@ stub NetMessageNameDel
@ stub NetMessageNameEnum
@ stub NetMessageNameGetInfo
@ stdcall NetQueryDisplayInformation(wstr long long long long ptr ptr)
@ stub NetRemoteComputerSupports
@ stub NetRemoteTOD
@ stub NetReplExportDirAdd
@ stub NetReplExportDirDel
@ stub NetReplExportDirEnum
@ stub NetReplExportDirGetInfo
@ stub NetReplExportDirLock
@ stub NetReplExportDirSetInfo
@ stub NetReplExportDirUnlock
@ stub NetReplGetInfo
@ stub NetReplImportDirAdd
@ stub NetReplImportDirDel
@ stub NetReplImportDirEnum
@ stub NetReplImportDirGetInfo
@ stub NetReplImportDirLock
@ stub NetReplImportDirUnlock
@ stub NetReplSetInfo
@ stub NetRplAdapterAdd
@ stub NetRplAdapterDel
@ stub NetRplAdapterEnum
@ stub NetRplBootAdd
@ stub NetRplBootDel
@ stub NetRplBootEnum
@ stub NetRplClose
@ stub NetRplConfigAdd
@ stub NetRplConfigDel
@ stub NetRplConfigEnum
@ stub NetRplGetInfo
@ stub NetRplOpen
@ stub NetRplProfileAdd
@ stub NetRplProfileClone
@ stub NetRplProfileDel
@ stub NetRplProfileEnum
@ stub NetRplProfileGetInfo
@ stub NetRplProfileSetInfo
@ stub NetRplSetInfo
@ stub NetRplSetSecurity
@ stub NetRplVendorAdd
@ stub NetRplVendorDel
@ stub NetRplVendorEnum
@ stub NetRplWkstaAdd
@ stub NetRplWkstaClone
@ stub NetRplWkstaDel
@ stub NetRplWkstaEnum
@ stub NetRplWkstaGetInfo
@ stub NetRplWkstaSetInfo
@ stub NetScheduleJobAdd
@ stub NetScheduleJobDel
@ stub NetScheduleJobEnum
@ stub NetScheduleJobGetInfo
@ stub NetServerComputerNameAdd
@ stub NetServerComputerNameDel
@ stub NetServerDiskEnum
@ stdcall NetServerEnum(wstr long ptr long ptr ptr long wstr ptr)
@ stdcall NetServerEnumEx(wstr long ptr long ptr ptr long wstr wstr)
@ stdcall NetServerGetInfo(wstr long ptr)
@ stub NetServerSetInfo
@ stub NetServerTransportAdd
@ stub NetServerTransportAddEx
@ stub NetServerTransportDel
@ stub NetServerTransportEnum
@ stub NetServiceControl
@ stub NetServiceEnum
@ stub NetServiceGetInfo
@ stub NetServiceInstall
@ stub NetSessionDel
@ stdcall NetSessionEnum(wstr wstr wstr long ptr long ptr ptr ptr)
@ stub NetSessionGetInfo
@ stub NetShareAdd
@ stub NetShareCheck
@ stdcall NetShareDel(wstr wstr long)
@ stub NetShareDelSticky
@ stdcall NetShareEnum(wstr long ptr long ptr ptr ptr)
@ stub NetShareEnumSticky
@ stub NetShareGetInfo
@ stub NetShareSetInfo
@ stdcall NetStatisticsGet(wstr wstr long long ptr)
@ stdcall NetUseAdd(wstr long ptr ptr)
@ stub NetUseDel
@ stdcall NetUseEnum(wstr long ptr long ptr ptr ptr)
@ stub NetUseGetInfo
@ stdcall NetUserAdd(wstr long ptr ptr)
@ stdcall NetUserChangePassword(wstr wstr wstr wstr)
@ stdcall NetUserDel(wstr wstr)
@ stdcall NetUserEnum(wstr long long ptr long ptr ptr ptr)
@ stub NetUserGetGroups
@ stdcall NetUserGetInfo(wstr wstr long ptr)
@ stdcall NetUserGetLocalGroups(wstr wstr long long ptr long ptr ptr)
@ stdcall NetUserModalsGet(wstr long ptr)
@ stub NetUserModalsSet
@ stub NetUserSetGroups
@ stub NetUserSetInfo
@ stdcall NetWkstaGetInfo(wstr long ptr)
@ stub NetWkstaSetInfo
@ stub NetWkstaTransportAdd
@ stub NetWkstaTransportDel
@ stdcall NetWkstaTransportEnum (wstr long ptr long ptr ptr ptr)
@ stub NetWkstaUserEnum
@ stdcall NetWkstaUserGetInfo(wstr long ptr)
@ stub NetWkstaUserSetInfo
@ stdcall NetapipBufferAllocate(long ptr) NetApiBufferAllocate
@ stdcall Netbios(ptr)
@ stub NetpAccessCheck
@ stub NetpAccessCheckAndAudit
@ stub NetpAllocConfigName
@ stub NetpAllocStrFromStr
@ stub NetpAllocStrFromWStr
@ stub NetpAllocTStrFromString
@ stub NetpAllocWStrFromStr
@ stub NetpAllocWStrFromWStr
@ stub NetpApiStatusToNtStatus
@ stub NetpAssertFailed
@ stub NetpCloseConfigData
@ stub NetpCopyStringToBuffer
@ stub NetpCreateSecurityObject
@ stub NetpDbgDisplayServerInfo
@ stub NetpDbgPrint
@ stdcall NetpDeleteSecurityObject(long) ntdll.RtlDeleteSecurityObject
@ stdcall NetpGetComputerName(ptr)
@ stub NetpGetConfigBool
@ stub NetpGetConfigDword
@ stub NetpGetConfigTStrArray
@ stub NetpGetConfigValue
@ stub NetpGetDomainName
@ stub NetpGetFileSecurity
@ stub NetpGetPrivilege
@ stub NetpHexDump
@ stdcall NetpInitOemString(ptr str) ntdll.RtlInitAnsiString
@ stub NetpIsRemote
@ stub NetpIsUncComputerNameValid
@ stub NetpLocalTimeZoneOffset
@ stub NetpLogonPutUnicodeString
@ stub NetpNetBiosAddName
@ stub NetpNetBiosCall
@ stub NetpNetBiosDelName
@ stub NetpNetBiosGetAdapterNumbers
@ stub NetpNetBiosHangup
@ stub NetpNetBiosReceive
@ stub NetpNetBiosReset
@ stub NetpNetBiosSend
@ stdcall NetpNetBiosStatusToApiStatus(long)
@ stub NetpNtStatusToApiStatus
@ stub NetpOpenConfigData
@ stub NetpPackString
@ stub NetpReleasePrivilege
@ stub NetpSetConfigBool
@ stub NetpSetConfigDword
@ stub NetpSetConfigTStrArray
@ stub NetpSetFileSecurity
@ stub NetpSmbCheck
@ stub NetpStringToNetBiosName
@ stub NetpTStrArrayEntryCount
@ stub NetpwNameCanonicalize
@ stub NetpwNameCompare
@ stub NetpwNameValidate
@ stub NetpwPathCanonicalize
@ stub NetpwPathCompare
@ stub NetpwPathType
@ stub NlBindingAddServerToCache
@ stub NlBindingRemoveServerFromCache
@ stub NlBindingSetAuthInfo
@ stub RxNetAccessAdd
@ stub RxNetAccessDel
@ stub RxNetAccessEnum
@ stub RxNetAccessGetInfo
@ stub RxNetAccessGetUserPerms
@ stub RxNetAccessSetInfo
@ stub RxNetServerEnum
@ stub RxNetUserPasswordSet
@ stub RxRemoteApi
