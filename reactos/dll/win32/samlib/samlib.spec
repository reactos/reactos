@ stdcall SamAddMemberToAlias(ptr ptr)
@ stub SamAddMemberToGroup
@ stub SamAddMultipleMembersToAlias
@ stub SamChangePasswordUser2
@ stub SamChangePasswordUser3
@ stub SamChangePasswordUser
@ stdcall SamCloseHandle(ptr)
@ stdcall SamConnect(ptr ptr long ptr)
@ stub SamConnectWithCreds
@ stdcall SamCreateAliasInDomain(ptr ptr long ptr ptr)
@ stdcall SamCreateGroupInDomain(ptr ptr long ptr ptr)
@ stub SamCreateUser2InDomain
@ stdcall SamCreateUserInDomain(ptr ptr long ptr ptr)
@ stub SamDeleteAlias
@ stub SamDeleteGroup
@ stub SamDeleteUser
@ stdcall SamEnumerateAliasesInDomain(ptr ptr ptr long ptr)
@ stdcall SamEnumerateDomainsInSamServer(ptr ptr ptr long ptr)
@ stub SamEnumerateGroupsInDomain
@ stub SamEnumerateUsersInDomain
@ stdcall SamFreeMemory(ptr)
@ stdcall SamGetAliasMembership(ptr long ptr ptr ptr)
@ stub SamGetCompatibilityMode
@ stub SamGetDisplayEnumerationIndex
@ stub SamGetGroupsForUser
@ stdcall SamGetMembersInAlias(ptr ptr ptr)
@ stub SamGetMembersInGroup
@ stdcall SamLookupDomainInSamServer(ptr ptr ptr)
@ stub SamLookupIdsInDomain
@ stdcall SamLookupNamesInDomain(ptr long ptr ptr ptr)
@ stdcall SamOpenAlias(ptr long long ptr)
@ stdcall SamOpenDomain(ptr long ptr ptr)
@ stdcall SamOpenGroup(ptr long long ptr)
@ stdcall SamOpenUser(ptr long long ptr)
@ stub SamQueryDisplayInformation
@ stdcall SamQueryInformationAlias(ptr long ptr)
@ stdcall SamQueryInformationDomain(ptr long ptr)
@ stdcall SamQueryInformationGroup(ptr long ptr)
@ stdcall SamQueryInformationUser(ptr long ptr)
@ stub SamQuerySecurityObject
@ stub SamRemoveMemberFromAlias
@ stub SamRemoveMemberFromForeignDomain
@ stub SamRemoveMemberFromGroup
@ stub SamRemoveMultipleMembersFromAlias
@ stub SamRidToSid
@ stdcall SamSetInformationAlias(ptr long ptr)
@ stdcall SamSetInformationDomain(ptr long ptr)
@ stdcall SamSetInformationGroup(ptr long ptr)
@ stdcall SamSetInformationUser(ptr long ptr)
@ stub SamSetMemberAttributesOfGroup
@ stub SamSetSecurityObject
@ stdcall SamShutdownSamServer(ptr)
@ stub SamTestPrivateFunctionsDomain
@ stub SamTestPrivateFunctionsUser
@ stub SamiChangeKeys
@ stub SamiChangePasswordUser2
@ stub SamiChangePasswordUser3
@ stub SamiChangePasswordUser
@ stub SamiEncryptPasswords
@ stub SamiGetBootKeyInformation
@ stub SamiLmChangePasswordUser
@ stub SamiOemChangePasswordUser2
@ stub SamiSetBootKeyInformation
@ stub SamiSetDSRMPassword
@ stub SamiSetDSRMPasswordOWF
