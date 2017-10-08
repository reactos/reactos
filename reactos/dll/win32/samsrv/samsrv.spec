@ stub SamIAccountRestrictions
@ stub SamIAddDSNameToAlias
@ stub SamIAddDSNameToGroup
@ stub SamIAmIGC
@ stub SamIChangePasswordForeignUser
@ stub SamIChangePasswordForeignUser2
@ stdcall SamIConnect(ptr ptr long long)
@ stub SamICreateAccountByRid
@ stub SamIDemote
@ stub SamIDemoteUndo
@ stub SamIDoFSMORoleChange
@ stub SamIDsCreateObjectInDomain
@ stub SamIDsSetObjectInformation
@ stub SamIEnumerateAccountRids
@ stub SamIEnumerateInterdomainTrustAccountsForUpgrade
@ stub SamIFloatingSingleMasterOpEx
@ stub SamIFreeSidAndAttributesList
@ stub SamIFreeSidArray
@ stdcall SamIFreeVoid(ptr)
@ stdcall SamIFree_SAMPR_ALIAS_INFO_BUFFER(ptr long)
@ stdcall SamIFree_SAMPR_DISPLAY_INFO_BUFFER(ptr long)
@ stdcall SamIFree_SAMPR_DOMAIN_INFO_BUFFER(ptr long)
@ stdcall SamIFree_SAMPR_ENUMERATION_BUFFER(ptr)
@ stdcall SamIFree_SAMPR_GET_GROUPS_BUFFER(ptr)
@ stdcall SamIFree_SAMPR_GET_MEMBERS_BUFFER(ptr)
@ stdcall SamIFree_SAMPR_GROUP_INFO_BUFFER(ptr long)
@ stdcall SamIFree_SAMPR_PSID_ARRAY(ptr)
@ stdcall SamIFree_SAMPR_RETURNED_USTRING_ARRAY(ptr)
@ stdcall SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR(ptr)
@ stdcall SamIFree_SAMPR_ULONG_ARRAY(ptr)
@ stdcall SamIFree_SAMPR_USER_INFO_BUFFER(ptr long)
@ stub SamIFree_UserInternal6Information
@ stub SamIGCLookupNames
@ stub SamIGCLookupSids
@ stub SamIGetAliasMembership
@ stub SamIGetBootKeyInformation
@ stub SamIGetDefaultAdministratorName
@ stub SamIGetFixedAttributes
@ stub SamIGetInterdomainTrustAccountPasswordsForUpgrade
@ stub SamIGetPrivateData
@ stub SamIGetResourceGroupMembershipsTransitive
@ stub SamIGetSerialNumberDomain
@ stub SamIGetUserLogonInformation
@ stub SamIGetUserLogonInformation2
@ stub SamIGetUserLogonInformationEx
@ stub SamIImpersonateNullSession
@ stub SamIIncrementPerformanceCounter
@ stdcall SamIInitialize()
@ stub SamIIsDownlevelDcUpgrade
@ stub SamIIsExtendedSidMode
@ stub SamIIsRebootAfterPromotion
@ stub SamIIsSetupInProgress
@ stub SamILoadDownlevelDatabase
@ stub SamILoopbackConnect
@ stub SamIMixedDomain
@ stub SamIMixedDomain2
@ stub SamINT4UpgradeInProgress
@ stub SamINetLogonPing
@ stub SamINotifyDelta
@ stub SamINotifyRoleChange
@ stub SamINotifyServerDelta
@ stub SamIOpenAccount
@ stub SamIOpenUserByAlternateId
@ stub SamIPromote
@ stub SamIPromoteUndo
@ stub SamIQueryServerRole
@ stub SamIQueryServerRole2
@ stub SamIRemoveDSNameFromAlias
@ stub SamIRemoveDSNameFromGroup
@ stub SamIRelaceDownlevelDatabase
@ stub SamIResetBadPwdCountOnPdc
@ stub SamIRetrievePrimaryCredentials
@ stub SamIRevertNullSession
@ stub SamISameSite
@ stub SamISetAuditingInformation
@ stub SamISetMixedDomainFlag
@ stub SamISetPasswordForeignUser
@ stub SamISetPasswordForeignUser2
@ stub SamISetPasswordInfoOnPdc
@ stub SamISetPrivateData
@ stub SamISetSerialNumberDomain
@ stub SamIStorePrimaryCredentials
@ stub SamIUPNFromUserHandle
@ stub SamIUnLoadDownlevelDatabase
@ stub SamIUpdateLogonStatistics
@ stub SampAbortSingleLoopbackTask
@ stub SampAccountControlToFlags
@ stub SampAcquireSamLockExclusive
@ stub SampAcquireWriteLock
@ stub SampCommitBufferedWrites
@ stub SampConvertNt4SdToNt5Sd
@ stub SampDsChangePasswordUser
@ stub SampFlagsToAccountControl
@ stub SampGetDefaultSecurityDescriptorForClass
@ stub SampGetSerialNumberDomain2
@ stdcall SampInitializeRegistry()
@ stub SampInitializeSdConversion
@ stub SampInvalidateDomainCache
@ stub SampInvalidateRidRange
@ stub SampNetLogonNotificationRequired
@ stub SampNotifyReplicatedInChange
@ stub SampProcessSingleLoopbackTask
@ stub SampReleaseSamLockExclusive
@ stub SampReleaseWriteLock
@ stub SampRtlConvertUlongToUnicodeString
@ stub SampSetSerialNumberDomain2
@ stub SampUsingDsData
@ stub SampWriteGroupType
@ stdcall SamrAddMemberToAlias(ptr ptr)
@ stdcall SamrAddMemberToGroup(ptr long long)
@ stdcall SamrAddMultipleMembersToAlias(ptr ptr)
@ stdcall SamrChangePasswordUser(ptr long ptr ptr long ptr ptr long ptr long ptr)
@ stdcall SamrCloseHandle(ptr)
@ stdcall SamrConnect(ptr ptr long)
@ stdcall SamrCreateAliasInDomain(ptr ptr long ptr ptr)
@ stdcall SamrCreateGroupInDomain(ptr ptr long ptr ptr)
@ stdcall SamrCreateUser2InDomain(ptr ptr long long ptr ptr ptr)
@ stdcall SamrCreateUserInDomain(ptr ptr long ptr ptr)
@ stdcall SamrDeleteAlias(ptr)
@ stdcall SamrDeleteGroup(ptr)
@ stdcall SamrDeleteUser(ptr)
@ stdcall SamrEnumerateAliasesInDomain(ptr ptr ptr long ptr)
@ stdcall SamrEnumerateDomainsInSamServer(ptr ptr ptr long ptr)
@ stdcall SamrEnumerateGroupsInDomain(ptr ptr ptr long ptr)
@ stdcall SamrEnumerateUsersInDomain(ptr ptr long ptr long ptr)
@ stdcall SamrGetAliasMembership(ptr ptr ptr)
@ stdcall SamrGetGroupsForUser(ptr ptr)
@ stdcall SamrGetMembersInAlias(ptr ptr)
@ stdcall SamrGetMembersInGroup(ptr ptr)
@ stdcall SamrGetUserDomainPasswordInformation(ptr ptr)
@ stdcall SamrLookupDomainInSamServer(ptr ptr ptr)
@ stdcall SamrLookupIdsInDomain(ptr long ptr ptr ptr)
@ stdcall SamrLookupNamesInDomain(ptr long ptr ptr ptr)
@ stdcall SamrOpenAlias(ptr long long ptr)
@ stdcall SamrOpenDomain(ptr long ptr ptr)
@ stdcall SamrOpenGroup(ptr long long ptr)
@ stdcall SamrOpenUser(ptr long long ptr)
@ stdcall SamrQueryDisplayInformation(ptr long long long long ptr ptr ptr)
@ stdcall SamrQueryInformationAlias(ptr long ptr)
@ stdcall SamrQueryInformationDomain(ptr long ptr)
@ stdcall SamrQueryInformationGroup(ptr long ptr)
@ stdcall SamrQueryInformationUser(ptr long ptr)
@ stdcall SamrQuerySecurityObject(ptr long ptr)
@ stdcall SamrRemoveMemberFromAlias(ptr ptr)
@ stdcall SamrRemoveMemberFromForeignDomain(ptr ptr)
@ stdcall SamrRemoveMemberFromGroup(ptr long)
@ stdcall SamrRemoveMultipleMembersFromAlias(ptr ptr)
@ stdcall SamrRidToSid(ptr long ptr)
@ stdcall SamrSetInformationAlias(ptr long ptr)
@ stdcall SamrSetInformationDomain(ptr long ptr)
@ stdcall SamrSetInformationGroup(ptr long ptr)
@ stdcall SamrSetInformationUser(ptr long ptr)
@ stdcall SamrSetMemberAttributesOfGroup(ptr long long)
@ stdcall SamrSetSecurityObject(ptr long ptr)
@ stdcall SamrShutdownSamServer(ptr)
@ stdcall SamrTestPrivateFunctionsDomain(ptr)
@ stdcall SamrTestPrivateFunctionsUser(ptr)
@ stdcall SamrUnicodeChangePasswordUser2(ptr ptr ptr ptr ptr long ptr ptr)
; EOF
