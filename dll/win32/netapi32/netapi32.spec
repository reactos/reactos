@ stub CredpValidateTargetName
@ stdcall DsAddressToSiteNamesA(str long ptr str)
@ stdcall DsAddressToSiteNamesExA(str long ptr str str)
@ stdcall DsAddressToSiteNamesExW(wstr long ptr wstr wstr)
@ stdcall DsAddressToSiteNamesW(wstr long ptr wstr)
@ stdcall DsDeregisterDnsHostRecordsA(str str ptr ptr str)
@ stdcall DsDeregisterDnsHostRecordsW(wstr wstr ptr ptr wstr)
8 stdcall DsEnumerateDomainTrustsA(wstr long ptr ptr)
9 stdcall DsEnumerateDomainTrustsW(wstr long ptr ptr)
@ stub DsGetDcCloseW
@ stdcall DsGetDcNameA(str str ptr str long ptr)
@ stdcall DsGetDcNameW(wstr wstr ptr wstr long ptr)
@ stdcall DsGetDcNameWithAccountA(str str long str ptr str long ptr)
@ stdcall DsGetDcNameWithAccountW(wstr wstr long wstr ptr wstr long ptr)
@ stub DsGetDcNextA
@ stub DsGetDcNextW
@ stub DsGetDcOpenA
@ stub DsGetDcOpenW
@ stdcall DsGetDcSiteCoverageA(str ptr str)
@ stdcall DsGetDcSiteCoverageW(wstr ptr wstr)
@ stdcall DsGetForestTrustInformationW(wstr wstr long ptr)
@ stdcall DsGetSiteNameA(str str)
@ stdcall DsGetSiteNameW(wstr wstr)
@ stdcall DsMergeForestTrustInformationW(wstr ptr ptr ptr)
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
@ stdcall DsValidateSubnetNameA(str)
@ stdcall DsValidateSubnetNameW(wstr)
@ stub I_BrowserDebugCall
@ stdcall I_BrowserDebugTrace(wstr str)
@ stdcall I_BrowserQueryEmulatedDomains(wstr ptr ptr)
@ stdcall I_BrowserQueryOtherDomains(wstr ptr ptr ptr)
@ stdcall I_BrowserQueryStatistics(wstr ptr)
@ stdcall I_BrowserResetNetlogonState(wstr)
@ stdcall I_BrowserResetStatistics(wstr)
@ stdcall I_BrowserServerEnum(wstr wstr wstr long ptr long ptr ptr long wstr ptr)
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
@ stdcall I_NetServerSetServiceBits(wstr wstr long long)
@ stub I_NetServerSetServiceBitsEx
@ stub I_NetServerTrustPasswordsGet
@ stub I_NetlogonComputeClientDigest
@ stub I_NetlogonComputeServerDigest
@ stub I_NetlogonGetTrustRid
@ stdcall NetAddAlternateComputerName(wstr wstr wstr wstr long)
@ stdcall NetAlertRaise(wstr ptr long)
@ stdcall NetAlertRaiseEx(wstr ptr long wstr)
@ stdcall NetApiBufferAllocate(long ptr)
@ stdcall NetApiBufferFree(ptr)
@ stdcall NetApiBufferReallocate(ptr long ptr)
@ stdcall NetApiBufferSize(ptr ptr)
@ stdcall NetAuditClear(wstr wstr wstr)
@ stdcall NetAuditRead(wstr wstr ptr long ptr long long ptr long ptr ptr)
@ stdcall NetAuditWrite(long ptr long wstr ptr)
@ stdcall NetBrowserStatisticsGet(wstr long ptr)
@ stdcall NetConfigGet(wstr wstr wstr ptr)
@ stdcall NetConfigGetAll(wstr wstr ptr)
@ stdcall NetConfigSet(wstr wstr wstr long long ptr long)
@ stdcall NetConnectionEnum(wstr wstr long ptr long ptr ptr ptr)
@ stdcall NetDfsAdd(wstr wstr wstr wstr long)
@ stdcall NetDfsAddFtRoot(wstr wstr wstr wstr long)
@ stdcall NetDfsAddStdRoot(wstr wstr wstr long)
@ stdcall NetDfsAddStdRootForced(wstr wstr wstr wstr)
@ stdcall NetDfsEnum(wstr long long ptr ptr ptr)
@ stdcall NetDfsGetClientInfo(wstr wstr wstr long ptr)
@ stdcall NetDfsGetDcAddress(wstr ptr ptr ptr)
@ stdcall NetDfsGetFtContainerSecurity(wstr long ptr ptr);
@ stdcall NetDfsGetInfo(wstr wstr wstr long ptr)
@ stdcall NetDfsGetSecurity(wstr ptr ptr ptr)
@ stdcall NetDfsGetStdContainerSecurity(wstr ptr ptr ptr)
@ stub NetDfsManagerGetConfigInfo
@ stdcall NetDfsManagerInitialize(wstr long)
@ stub NetDfsManagerSendSiteInfo
@ stdcall NetDfsMove(wstr wstr long)
@ stdcall NetDfsRemove(wstr wstr wstr)
@ stdcall NetDfsRemoveFtRoot(wstr wstr wstr long)
@ stdcall NetDfsRemoveFtRootForced(wstr wstr wstr wstr long)
@ stdcall NetDfsRemoveStdRoot(wstr wstr long)
@ stdcall NetDfsRename(wstr wstr)
@ stdcall NetDfsSetClientInfo(wstr wstr wstr long ptr)
@ stdcall NetDfsSetFtContainerSecurity(wstr ptr ptr)
@ stdcall NetDfsSetInfo(wstr wstr wstr long ptr)
@ stdcall NetDfsSetSecurity(wstr ptr ptr)
@ stdcall NetDfsSetStdContainerSecurity(wstr ptr ptr)
@ stdcall NetEnumerateComputerNames(wstr long long ptr ptr)
@ stdcall NetEnumerateTrustedDomains(wstr ptr)
@ stdcall NetErrorLogClear(wstr wstr ptr)
@ stdcall NetErrorLogRead(wstr wstr ptr long ptr long long ptr long ptr ptr)
@ stdcall NetErrorLogWrite(ptr long wstr ptr long ptr long ptr)
@ stdcall NetFileClose(wstr long)
@ stdcall NetFileEnum(wstr wstr wstr long ptr long ptr ptr ptr)
@ stdcall NetFileGetInfo(wstr long long ptr)
@ stdcall NetGetAnyDCName(wstr wstr ptr)
@ stdcall NetGetDCName(wstr wstr ptr)
@ stdcall NetGetDisplayInformationIndex(wstr long wstr ptr)
@ stdcall NetGetJoinInformation(wstr ptr ptr)
@ stdcall NetGetJoinableOUs(wstr wstr wstr wstr ptr ptr)
@ stdcall NetGroupAdd(wstr long ptr ptr)
@ stdcall NetGroupAddUser(wstr wstr wstr)
@ stdcall NetGroupDel(wstr wstr)
@ stdcall NetGroupDelUser(wstr wstr wstr)
@ stdcall NetGroupEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetGroupGetInfo(wstr wstr long ptr)
@ stdcall NetGroupGetUsers(wstr wstr long ptr long ptr ptr ptr)
@ stdcall NetGroupSetInfo(wstr wstr long ptr ptr)
@ stdcall NetGroupSetUsers(wstr wstr long ptr long)
@ stdcall NetJoinDomain(wstr wstr wstr wstr wstr long)
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
@ stdcall NetLogonGetTimeServiceParentDomain(wstr ptr ptr)
@ stdcall NetLogonSetServiceBits(wstr long long)
@ stdcall NetMessageBufferSend(wstr wstr wstr ptr long)
@ stdcall NetMessageNameAdd(wstr wstr)
@ stdcall NetMessageNameDel(wstr wstr)
@ stdcall NetMessageNameEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetMessageNameGetInfo(wstr wstr long ptr)
@ stdcall NetQueryDisplayInformation(wstr long long long long ptr ptr)
@ stdcall NetRegisterDomainNameChangeNotification(ptr)
@ stub NetRemoteComputerSupports
@ stdcall NetRemoteTOD(wstr ptr)
@ stdcall NetRemoveAlternateComputerName(wstr wstr wstr wstr long)
@ stdcall NetRenameMachineInDomain(wstr wstr wstr wstr long)
@ stdcall NetReplExportDirAdd(wstr long ptr ptr)
@ stdcall NetReplExportDirDel(wstr wstr)
@ stdcall NetReplExportDirEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetReplExportDirGetInfo(wstr wstr long ptr)
@ stdcall NetReplExportDirLock(wstr wstr)
@ stdcall NetReplExportDirSetInfo(wstr wstr long ptr ptr)
@ stdcall NetReplExportDirUnlock(wstr wstr long)
@ stdcall NetReplGetInfo(wstr long ptr)
@ stdcall NetReplImportDirAdd(wstr long ptr ptr)
@ stdcall NetReplImportDirDel(wstr wstr)
@ stdcall NetReplImportDirEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetReplImportDirGetInfo(wstr wstr long ptr)
@ stdcall NetReplImportDirLock(wstr wstr)
@ stdcall NetReplImportDirUnlock(wstr wstr long)
@ stdcall NetReplSetInfo(wstr long ptr ptr)
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
@ stdcall NetServerSetInfo(wstr long ptr ptr)
@ stdcall NetServerTransportAdd(wstr long ptr)
@ stdcall NetServerTransportAddEx(wstr long ptr)
@ stdcall NetServerTransportDel(wstr long ptr)
@ stdcall NetServerTransportEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetServiceControl(wstr wstr long long ptr)
@ stdcall NetServiceEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetServiceGetInfo(wstr wstr long ptr)
@ stdcall NetServiceInstall(wstr wstr long ptr ptr)
@ stdcall NetSessionDel(wstr wstr wstr)
@ stdcall NetSessionEnum(wstr wstr wstr long ptr long ptr ptr ptr)
@ stdcall NetSessionGetInfo(wstr wstr wstr long ptr)
@ stdcall NetSetPrimaryComputerName(wstr wstr wstr wstr long)
@ stdcall NetShareAdd(wstr long ptr ptr)
@ stdcall NetShareCheck(wstr wstr ptr)
@ stdcall NetShareDel(wstr wstr long)
@ stdcall NetShareDelSticky(wstr wstr long)
@ stdcall NetShareEnum(wstr long ptr long ptr ptr ptr)
@ stdcall NetShareEnumSticky(wstr long ptr long ptr ptr ptr)
@ stdcall NetShareGetInfo(wstr wstr long ptr)
@ stdcall NetShareSetInfo(wstr wstr long ptr ptr)
@ stdcall NetStatisticsGet(wstr wstr long long ptr)
@ stdcall NetUnjoinDomain(wstr wstr wstr long)
@ stdcall NetUnregisterDomainNameChangeNotification(ptr)
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
@ stdcall NetValidateName(wstr wstr wstr wstr long)
@ stub NetValidatePasswordPolicy
@ stub NetValidatePasswordPolicyFree
@ stdcall NetWkstaGetInfo(wstr long ptr)
@ stdcall NetWkstaSetInfo(wstr long ptr ptr)
@ stdcall NetWkstaTransportAdd(wstr long ptr ptr)
@ stdcall NetWkstaTransportDel(wstr wstr long)
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
@ stdcall NetpAllocWStrFromStr(str)
@ stdcall NetpAllocWStrFromWStr(wstr)
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
