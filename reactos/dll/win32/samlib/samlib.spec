@ stub SamAddMemberToAlias
@ stub SamAddMemberToGroup
@ stub SamAddMultipleMembersToAlias
@ stub SamChangePasswordUser2
@ stub SamChangePasswordUser3
@ stub SamChangePasswordUser
@ stdcall SamCloseHandle(ptr)
@ stdcall SamConnect(ptr ptr long ptr)
@ stub SamConnectWithCreds
@ stub SamCreateAliasInDomain
@ stub SamCreateGroupInDomain
@ stub SamCreateUser2InDomain
@ stdcall SamCreateUserInDomain(ptr ptr long ptr ptr)
@ stub SamDeleteAlias
@ stub SamDeleteGroup
@ stub SamDeleteUser
@ stub SamEnumerateAliasesInDomain
@ stub SamEnumerateDomainsInSamServer
@ stub SamEnumerateGroupsInDomain
@ stub SamEnumerateUsersInDomain
@ stub SamFreeMemory
@ stub SamGetAliasMembership
@ stub SamGetCompatibilityMode
@ stub SamGetDisplayEnumerationIndex
@ stub SamGetGroupsForUser
@ stub SamGetMembersInAlias
@ stub SamGetMembersInGroup
@ stub SamLookupDomainInSamServer
@ stub SamLookupIdsInDomain
@ stub SamLookupNamesInDomain
@ stub SamOpenAlias
@ stdcall SamOpenDomain(ptr long ptr ptr)
@ stub SamOpenGroup
@ stdcall SamOpenUser(ptr long long ptr)
@ stub SamQueryDisplayInformation
@ stub SamQueryInformationAlias
@ stub SamQueryInformationDomain
@ stub SamQueryInformationGroup
@ stdcall SamQueryInformationUser(ptr long ptr)
@ stub SamQuerySecurityObject
@ stub SamRemoveMemberFromAlias
@ stub SamRemoveMemberFromForeignDomain
@ stub SamRemoveMemberFromGroup
@ stub SamRemoveMultipleMembersFromAlias
@ stub SamRidToSid
@ stub SamSetInformationAlias
@ stub SamSetInformationDomain
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

@ stdcall SamCreateUser(wstr wstr ptr)
@ stdcall SamCheckUserPassword(wstr wstr)
@ stdcall SamGetUserSid(wstr ptr)
