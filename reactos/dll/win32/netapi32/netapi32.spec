@ stub CredpValidateTargetName
@ stub DsAddressToSiteNamesA
@ stub DsAddressToSiteNamesExA
@ stub DsAddressToSiteNamesExW
@ stub DsAddressToSiteNamesW
@ stub DsDeregisterDnsHostRecordsA
@ stub DsDeregisterDnsHostRecordsW
@ stub DsEnumerateDomainTrustsA
@ stub DsEnumerateDomainTrustsW
@ stub DsGetDcCloseW
@ stdcall DsGetDcNameA(str str ptr str long ptr)
@ stdcall DsGetDcNameW(wstr wstr ptr wstr long ptr)
@ stub DsGetDcNameWithAccountA
@ stub DsGetDcNameWithAccountW
@ stub DsGetDcNextA
@ stub DsGetDcNextW
@ stub DsGetDcOpenA
@ stub DsGetDcOpenW
@ stub DsGetDcSiteCoverageA
@ stub DsGetDcSiteCoverageW
@ stub DsGetForestTrustInformationW
@ stub DsGetSiteNameA
@ stdcall DsGetSiteNameW(wstr wstr)
@ stub DsMergeForestTrustInformationW
@ stub DsRoleAbortDownlevelServerUpgrade
@ stub DsRoleCancel
@ stub DsRoleDcAsDc
@ stub DsRoleDcAsReplica
@ stub DsRoleDemoteDc
@ stub DsRoleDnsNameToFlatName
@ stdcall DsRoleFreeMemory(ptr)
@ stub DsRoleGetDatabaseFacts
@ stub DsRoleGetDcOperationProgress
@ stub DsRoleGetDcOperationResults
@ stdcall DsRoleGetPrimaryDomainInformation(wstr long ptr)
@ stub DsRoleIfmHandleFree
@ stub DsRoleServerSaveStateForUpgrade
@ stub DsRoleUpgradeDownlevelServer
@ stub DsValidateSubnetNameA
@ stub DsValidateSubnetNameW
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
@ stub I_NetDfsGetFtServers
@ stub I_NetDfsGetVersion
@ stub I_NetDfsIsThisADomainName
@ stub I_NetDfsManagerReportSiteInfo
@ stub I_NetDfsModifyPrefix
@ stub I_NetDfsSetLocalVolumeState
@ stub I_NetDfsSetServerInfo
@ stub I_NetGetDCList
@ stub I_NetGetForestTrustInformation
@ stub I_NetListCanonicalize
@ stub I_NetListTraverse
@ stub I_NetLogonControl2
@ stub I_NetLogonControl
@ stub I_NetLogonGetDomainInfo
@ stub I_NetLogonSamLogoff
@ stub I_NetLogonSamLogon
@ stub I_NetLogonSamLogonEx
@ stub I_NetLogonSamLogonWithFlags
@ stub I_NetLogonSendToSam
@ stub I_NetLogonUasLogoff
@ stub I_NetLogonUasLogon
@ stub I_NetNameCanonicalize
@ stdcall I_NetNameCompare(ptr wstr wstr ptr ptr)
@ stdcall I_NetNameValidate(ptr wstr ptr ptr)
@ stub I_NetPathCanonicalize
@ stub I_NetPathCompare
@ stub I_NetPathType
@ stub I_NetServerAuthenticate2
@ stub I_NetServerAuthenticate3
@ stub I_NetServerAuthenticate
@ stub I_NetServerGetTrustInfo
@ stub I_NetServerPasswordGet
@ stub I_NetServerPasswordSet2
@ stub I_NetServerPasswordSet
@ stub I_NetServerReqChallenge
@ stub I_NetServerSetServiceBits
@ stub I_NetServerSetServiceBitsEx
@ stub I_NetServerTrustPasswordsGet
@ stub I_NetlogonComputeClientDigest
@ stub I_NetlogonComputeServerDigest
@ stub I_NetlogonGetTrustRid
@ stub NetAddAlternateComputerName
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
@ stub NetDfsAddFtRoot
@ stub NetDfsAddStdRoot
@ stub NetDfsAddStdRootForced
@ stub NetDfsEnum
@ stub NetDfsGetClientInfo
@ stub NetDfsGetDcAddress
@ stub NetDfsGetFtContainerSecurity
@ stub NetDfsGetInfo
@ stub NetDfsGetSecurity
@ stub NetDfsGetStdContainerSecurity
@ stub NetDfsManagerGetConfigInfo
@ stub NetDfsManagerInitialize
@ stub NetDfsManagerSendSiteInfo
@ stub NetDfsMove
@ stub NetDfsRemove
@ stub NetDfsRemoveFtRoot
@ stub NetDfsRemoveFtRootForced
@ stub NetDfsRemoveStdRoot
@ stub NetDfsRename
@ stub NetDfsSetClientInfo
@ stub NetDfsSetFtContainerSecurity
@ stub NetDfsSetInfo
@ stub NetDfsSetSecurity
@ stub NetDfsSetStdContainerSecurity
@ stub NetEnumerateComputerNames
@ stub NetEnumerateTrustedDomains
@ stub NetErrorLogClear
@ stub NetErrorLogRead
@ stub NetErrorLogWrite
@ stub NetFileClose
@ stdcall NetFileEnum(wstr wstr wstr long ptr long ptr ptr ptr)
@ stub NetFileGetInfo
@ stub NetGetAnyDCName
@ stdcall NetGetDCName(wstr wstr ptr)
@ stub NetGetDisplayInformationIndex
@ stdcall NetGetJoinInformation(wstr ptr ptr)
@ stub NetGetJoinableOUs
@ stdcall NetGroupAdd(wstr long ptr ptr)
@ stdcall NetGroupAddUser(wstr wstr wstr)
@ stdcall NetGroupDel(wstr wstr)
@ stdcall NetGroupDelUser(wstr wstr wstr)
@ stdcall NetGroupEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetGroupGetInfo(wstr wstr long ptr)
@ stdcall NetGroupGetUsers(wstr wstr long ptr long ptr ptr ptr)
@ stdcall NetGroupSetInfo(wstr wstr long ptr ptr)
@ stdcall NetGroupSetUsers(wstr wstr long ptr long)
@ stub NetJoinDomain
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
@ stub NetLogonGetTimeServiceParentDomain
@ stub NetLogonSetServiceBits
@ stub NetMessageBufferSend
@ stub NetMessageNameAdd
@ stub NetMessageNameDel
@ stub NetMessageNameEnum
@ stub NetMessageNameGetInfo
@ stdcall NetQueryDisplayInformation(wstr long long long long ptr ptr)
@ stub NetRegisterDomainNameChangeNotification
@ stub NetRemoteComputerSupports
@ stub NetRemoteTOD
@ stub NetRemoveAlternateComputerName
@ stub NetRenameMachineInDomain
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
@ stdcall NetScheduleJobAdd(wstr ptr ptr)
@ stdcall NetScheduleJobDel(wstr long long)
@ stdcall NetScheduleJobEnum(wstr ptr long ptr ptr ptr)
@ stdcall NetScheduleJobGetInfo(wstr long ptr)
@ stub NetServerComputerNameAdd
@ stub NetServerComputerNameDel
@ stdcall NetServerDiskEnum(wstr long ptr long ptr ptr ptr)
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
@ stub NetSetPrimaryComputerName
@ stdcall NetShareAdd(wstr long ptr ptr)
@ stub NetShareCheck
@ stdcall NetShareDel(wstr wstr long)
@ stub NetShareDelSticky
@ stdcall NetShareEnum(wstr long ptr long ptr ptr ptr)
@ stub NetShareEnumSticky
@ stdcall NetShareGetInfo(wstr wstr long ptr)
@ stub NetShareSetInfo
@ stdcall NetStatisticsGet(wstr wstr long long ptr)
@ stub NetUnjoinDomain
@ stub NetUnregisterDomainNameChangeNotification
@ stdcall NetUseAdd(wstr long ptr ptr)
@ stdcall NetUseDel(wstr wstr long)
@ stdcall NetUseEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetUseGetInfo(ptr ptr long ptr)
@ stdcall NetUserAdd(wstr long ptr ptr)
@ stdcall NetUserChangePassword(wstr wstr wstr wstr)
@ stdcall NetUserDel(wstr wstr)
@ stdcall NetUserEnum(wstr long long ptr long ptr ptr ptr)
@ stdcall NetUserGetGroups(wstr wstr long ptr long ptr ptr)
@ stdcall NetUserGetInfo(wstr wstr long ptr)
@ stdcall NetUserGetLocalGroups(wstr wstr long long ptr long ptr ptr)
@ stdcall NetUserModalsGet(wstr long ptr)
@ stdcall NetUserModalsSet(wstr long ptr ptr)
@ stdcall NetUserSetGroups(wstr wstr long ptr long)
@ stdcall NetUserSetInfo(wstr wstr long ptr ptr)
@ stub NetValidateName
@ stub NetValidatePasswordPolicy
@ stub NetValidatePasswordPolicyFree
@ stdcall NetWkstaGetInfo(wstr long ptr)
@ stdcall NetWkstaSetInfo(wstr long ptr ptr)
@ stub NetWkstaTransportAdd
@ stub NetWkstaTransportDel
@ stdcall NetWkstaTransportEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetWkstaUserEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetWkstaUserGetInfo(wstr long ptr)
@ stdcall NetWkstaUserSetInfo(wstr long ptr ptr)
@ stdcall NetapipBufferAllocate(long ptr) NetApiBufferAllocate
@ stdcall Netbios(ptr)
@ stub NetpAccessCheck
@ stub NetpAccessCheckAndAudit
@ stub NetpAddTlnFtinfoEntry
@ stub NetpAllocConfigName
@ stub NetpAllocFtinfoEntry
@ stub NetpAllocStrFromWStr
@ stub NetpAllocWStrFromStr
@ stub NetpAllocWStrFromWStr
@ stub NetpApiStatusToNtStatus
@ stub NetpAssertFailed
@ stub NetpCleanFtinfoContext
@ stub NetpCloseConfigData
@ stub NetpCopyFtinfoContext
@ stub NetpCopyStringToBuffer
@ stub NetpCreateSecurityObject
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
@ stub NetpInitFtinfoContext
@ stdcall NetpInitOemString(ptr str) ntdll.RtlInitAnsiString
@ stub NetpIsRemote
@ stub NetpIsUncComputerNameValid
@ stub NetpLocalTimeZoneOffset
@ stub NetpLogonPutUnicodeString
@ stub NetpMergeFtinfo
@ stub NetpNetBiosAddName
@ stub NetpNetBiosCall
@ stub NetpNetBiosDelName
@ stub NetpNetBiosGetAdapterNumbers
@ stub NetpNetBiosHangup
@ stub NetpNetBiosReceive
@ stub NetpNetBiosReset
@ stub NetpNetBiosSend
@ stdcall NetpNetBiosStatusToApiStatus(long)
@ stdcall NetpNtStatusToApiStatus(long)
@ stub NetpOpenConfigData
@ stub NetpPackString
@ stub NetpParmsQueryUserProperty
@ stub NetpParmsQueryUserPropertyWithLength
@ stub NetpParmsSetUserProperty
@ stub NetpParmsSetUserPropertyWithLength
@ stub NetpParmsUserPropertyFree
@ stub NetpReleasePrivilege
@ stub NetpSetFileSecurity
@ stub NetpSmbCheck
@ stub NetpStoreIntialDcRecord
@ stub NetpStringToNetBiosName
@ stub NetpTStrArrayEntryCount
@ stub NetpUpgradePreNT5JoinInfo
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
