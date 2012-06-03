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
@ stub SamCreateGroupInDomain
@ stub SamCreateUser2InDomain
@ stdcall SamCreateUserInDomain(ptr ptr long ptr ptr)
@ stub SamDeleteAlias
@ stub SamDeleteGroup
@ stub SamDeleteUser
@ stub SamEnumerateAliasesInDomain
@ stdcall SamEnumerateDomainsInSamServer(ptr ptr ptr long ptr)
@ stub SamEnumerateGroupsInDomain
@ stub SamEnumerateUsersInDomain
@ stdcall SamFreeMemory(ptr)
@ stub SamGetAliasMembership
@ stub SamGetCompatibilityMode
@ stub SamGetDisplayEnumerationIndex
@ stub SamGetGroupsForUser
@ stub SamGetMembersInAlias
@ stub SamGetMembersInGroup
@ stdcall SamLookupDomainInSamServer(ptr ptr ptr)
@ stub SamLookupIdsInDomain
@ stub SamLookupNamesInDomain
@ stdcall SamOpenAlias(ptr long long ptr)
@ stdcall SamOpenDomain(ptr long ptr ptr)
@ stub SamOpenGroup
@ stdcall SamOpenUser(ptr long long ptr)
@ stub SamQueryDisplayInformation
@ stub SamQueryInformationAlias
@ stdcall SamQueryInformationDomain(ptr long ptr)
@ stub SamQueryInformationGroup
@ stdcall SamQueryInformationUser(ptr long ptr)
@ stub SamQuerySecurityObject
@ stub SamRemoveMemberFromAlias
@ stub SamRemoveMemberFromForeignDomain
@ stub SamRemoveMemberFromGroup
@ stub SamRemoveMultipleMembersFromAlias
@ stub SamRidToSid
@ stub SamSetInformationAlias
@ stdcall SamSetInformationDomain(ptr long ptr)
@ stub SamSetInformationGroup
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
