@ stub SamIAccountRestrictions
@ stub SamIConnect
@ stub SamICreateAccountByRid
@ stub SamIEnumerateAccountRids
@ stub SamIFree_SAMPR_ALIAS_INFO_BUFFER
@ stub SamIFree_SAMPR_DISPLAY_INFO_BUFFER
@ stub SamIFree_SAMPR_DOMAIN_INFO_BUFFER
@ stub SamIFree_SAMPR_ENUMERATION_BUFFER
@ stub SamIFree_SAMPR_GET_GROUPS_BUFFER
@ stub SamIFree_SAMPR_GET_MEMBERS_BUFFER
@ stub SamIFree_SAMPR_GROUP_INFO_BUFFER
@ stub SamIFree_SAMPR_PSID_ARRAY
@ stub SamIFree_SAMPR_RETURNED_USTRING_ARRAY
@ stub SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR
@ stub SamIFree_SAMPR_ULONG_ARRAY
@ stub SamIFree_SAMPR_USER_INFO_BUFFER
@ stub SamIGetPrivateData
@ stub SamIGetSerialNumberDomain
@ stdcall SamIInitialize()
@ stub SamINotifyDelta
@ stub SamISetAuditingInformation
@ stub SamISetPrivateData
@ stub SamISetSerialNumberDomain
@ stdcall SampInitializeRegistry()
@ stub SampRtlConvertUlongToUnicodeString
@ stdcall SamrAddMemberToAlias(ptr ptr)
@ stdcall SamrAddMemberToGroup(ptr long long)
@ stdcall SamrAddMultipleMembersToAlias(ptr ptr)
@ stdcall SamrChangePasswordUser(ptr long ptr ptr long ptr ptr long ptr long ptr)
@ stdcall SamrCloseHandle(ptr)
@ stdcall SamrConnect(ptr ptr long)
@ stdcall SamrCreateAliasInDomain(ptr ptr long ptr ptr)
@ stdcall SamrCreateGroupInDomain(ptr ptr long ptr ptr)
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
@ stdcall SamrSetInformationAlias(ptr long ptr)
@ stdcall SamrSetInformationDomain(ptr long ptr)
@ stdcall SamrSetInformationGroup(ptr long ptr)
@ stdcall SamrSetInformationUser(ptr long ptr)
@ stdcall SamrSetMemberAttributesOfGroup(ptr long long)
@ stdcall SamrSetSecurityObject(ptr long ptr)
@ stdcall SamrShutdownSamServer(ptr)
@ stdcall SamrTestPrivateFunctionsDomain(ptr)
@ stdcall SamrTestPrivateFunctionsUser(ptr)
; EOF