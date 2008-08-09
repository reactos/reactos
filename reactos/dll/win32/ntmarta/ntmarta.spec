@ stdcall AccFreeIndexArray(ptr long ptr)
@ stdcall AccGetInheritanceSource(wstr long long long ptr long ptr ptr ptr ptr)
;AccProvHandleGrantAccessRights
@ stdcall AccRewriteGetExplicitEntriesFromAcl(ptr ptr ptr)
@ stdcall AccRewriteGetHandleRights(ptr long long ptr ptr ptr ptr ptr)
@ stdcall AccRewriteGetNamedRights(wstr long long ptr ptr ptr ptr ptr)
@ stdcall AccRewriteSetEntriesInAcl(long ptr ptr ptr)
@ stdcall AccRewriteSetHandleRights(ptr long long ptr)
@ stdcall AccRewriteSetNamedRights(wstr long long ptr)
@ stdcall AccTreeResetNamedSecurityInfo(wstr long long ptr ptr ptr ptr long ptr ptr ptr)
;AccConvertAccessMaskToActrlAccess
;AccConvertAccessToSD
;AccConvertAccessToSecurityDescriptor
;AccConvertAclToAccess
;AccConvertSDToAccess
;AccGetAccessForTrustee
;AccGetExplicitEntries
;AccLookupAccountName
;AccLookupAccountSid;
;AccLookupAccountTrustee
;AccProvCancelOperation;
;AccProvGetAccessInfoPerObjectType
;AccProvGetAllRights
;AccProvGetCapabilities
;AccProvGetOperationResults
;AccProvGetTrusteesAccess
;AccProvGrantAccessRights
;AccProvHandleGetAccessInfoPerObjectType
;AccProvHandleGetAllRights
;AccProvHandleGetTrusteesAccess
;AccProvHandleIsAccessAudited
;AccProvHandleIsObjectAccessible
;AccProvHandleRevokeAccessRights
;AccProvHandleRevokeAuditRights
;AccProvHandleSetAccessRights
;AccProvIsAccessAudited
;AccProvIsObjectAccessible
;AccProvRevokeAccessRights
;AccProvSetAccessRights
;AccSetEntriesInAList
;EventGuidToName
;EventNameFree
