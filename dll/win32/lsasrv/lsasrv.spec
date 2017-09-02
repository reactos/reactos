@ stub DsRolerDcAsDc
@ stub DsRolerDcAsReplica
@ stub DsRolerDemoteDc
@ stub DsRolerGetDcOperationProgress
@ stub DsRolerGetDcOperationResults
@ stub LsaIAddNameToLogonSession
@ stdcall LsaIAllocateHeap(long) LsapAllocateHeap
@ stdcall LsaIAllocateHeapZero(long) LsapAllocateHeapZero
@ stub LsaIAuditAccountLogon
@ stub LsaIAuditAccountLogonEx
@ stub LsaIAuditKdcEvent
@ stub LsaIAuditKerberosLogon
@ stub LsaIAuditLogonUsingExplicitCreds
@ stub LsaIAuditNotifyPackageLoad
@ stub LsaIAuditPasswordAccessEvent
@ stub LsaIAuditSamEvent
@ stub LsaICallPackage
@ stub LsaICallPackageEx
@ stub LsaICallPackagePassthrough
@ stub LsaICancelNotification
@ stub LsaIChangeSecretCipherKey
@ stub LsaICryptProtectData
@ stub LsaICryptUnprotectData
@ stub LsaIDsNotifiedObjectChange
@ stub LsaIEnumerateSecrets
@ stub LsaIEventNotify
@ stub LsaIFilterSids
@ stub LsaIForestTrustFindMatch
@ stub LsaIFreeForestTrustInfo
@ stdcall LsaIFreeHeap(ptr) LsapFreeHeap
@ stub LsaIFreeReturnBuffer
@ stub LsaIFree_LSAI_PRIVATE_DATA #DATA
@ stub LsaIFree_LSAI_SECRET_ENUM_BUFFER
@ stdcall LsaIFree_LSAPR_ACCOUNT_ENUM_BUFFER(ptr)
@ stdcall LsaIFree_LSAPR_CR_CIPHER_VALUE(ptr)
@ stub LsaIFree_LSAPR_POLICY_DOMAIN_INFORMATION
@ stdcall LsaIFree_LSAPR_POLICY_INFORMATION(long ptr)
@ stdcall LsaIFree_LSAPR_PRIVILEGE_ENUM_BUFFER(ptr)
@ stdcall LsaIFree_LSAPR_PRIVILEGE_SET(ptr)
@ stdcall LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST(ptr)
@ stdcall LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR(ptr)
@ stdcall LsaIFree_LSAPR_TRANSLATED_NAMES(ptr)
@ stdcall LsaIFree_LSAPR_TRANSLATED_SIDS(ptr)
@ stub LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO
@ stub LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER
@ stub LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX
@ stub LsaIFree_LSAPR_TRUST_INFORMATION
@ stub LsaIFree_LSAPR_UNICODE_STRING
@ stub LsaIFree_LSAPR_UNICODE_STRING_BUFFER
@ stub LsaIFree_LSAP_SITENAME_INFO
@ stub LsaIFree_LSAP_SITE_INFO
@ stub LsaIFree_LSAP_SUBNET_INFO
@ stub LsaIFree_LSAP_UPN_SUFFIXES
@ stub LsaIFree_LSA_FOREST_TRUST_COLLISION_INFORMATION
@ stub LsaIFree_LSA_FOREST_TRUST_INFORMATION
@ stub LsaIGetBootOption
@ stub LsaIGetCallInfo
@ stub LsaIGetForestTrustInformation
@ stub LsaIGetLogonGuid
@ stub LsaIGetNbAndDnsDomainNames
@ stub LsaIGetPrivateData
@ stub LsaIGetSerialNumberPolicy
@ stub LsaIGetSerialNumberPolicy2
@ stub LsaIGetSiteName
@ stub LsaIHealthCheck
@ stub LsaIImpersonateClient
@ stub LsaIInitializeWellKnownSids
@ stub LsaIIsClassIdLsaClass
@ stub LsaIIsDsPaused
@ stub LsaIKerberosRegisterTrustNotification
@ stub LsaILookupWellKnownName
@ stub LsaINotifyChangeNotification
@ stub LsaINotifyNetlogonParametersChangeW
@ stub LsaINotifyPasswordChanged
@ stdcall LsaIOpenPolicyTrusted(ptr)
@ stub LsaIQueryForestTrustInfo
@ stub LsaIQueryInformationPolicyTrusted
@ stub LsaIQuerySiteInfo
@ stub LsaIQuerySubnetInfo
@ stub LsaIQueryUpnSuffixes
@ stub LsaIRegisterNotification
@ stub LsaIRegisterPolicyChangeNotificationCallback
@ stub LsaISafeMode
@ stub LsaISamIndicatedDsStarted
@ stub LsaISetBootOption
@ stub LsaISetClientDnsHostName
@ stub LsaISetLogonGuidInLogonSession
@ stub LsaISetPrivateData
@ stub LsaISetSerialNumberPolicy
@ stub LsaISetTimesSecret
@ stub LsaISetupWasRun
@ stub LsaITestCall
@ stub LsaIUnregisterAllPolicyChangeNotificationCallback
@ stub LsaIUnregisterPolicyChangeNotificationCallback
@ stub LsaIUpdateForestTrustInformation
@ stub LsaIWriteAuditEvent
@ stub LsapAuOpenSam
@ stub LsapCheckBootMode
@ stub LsapDsDebugInitialize
@ stub LsapDsInitializeDsStateInfo
@ stub LsapDsInitializePromoteInterface
@ stdcall LsapInitLsa()
@ stdcall LsarAddPrivilegesToAccount(ptr ptr)
@ stdcall LsarClose(ptr)
@ stdcall LsarCreateAccount(ptr ptr long ptr)
@ stdcall LsarCreateSecret(ptr ptr long ptr)
@ stdcall LsarCreateTrustedDomain(ptr ptr long ptr)
@ stdcall LsarCreateTrustedDomainEx(ptr ptr ptr long ptr)
@ stdcall LsarDelete(ptr)
@ stdcall LsarEnumerateAccounts(ptr ptr ptr long)
@ stdcall LsarEnumeratePrivileges(ptr ptr ptr long)
@ stdcall LsarEnumeratePrivilegesAccount(ptr ptr)
@ stdcall LsarEnumerateTrustedDomains(ptr ptr ptr long)
@ stdcall LsarEnumerateTrustedDomainsEx(ptr long ptr long)
@ stdcall LsarGetQuotasForAccount(ptr ptr)
@ stdcall LsarGetSystemAccessAccount(ptr ptr)
@ stdcall LsarLookupNames(ptr long ptr ptr ptr long ptr)
@ stdcall LsarLookupPrivilegeDisplayName(ptr ptr long long ptr ptr)
@ stdcall LsarLookupPrivilegeName(ptr ptr ptr)
@ stdcall LsarLookupPrivilegeValue(ptr ptr ptr)
@ stdcall LsarLookupSids(ptr ptr ptr ptr long ptr)
@ stdcall LsarLookupSids2(ptr ptr ptr ptr long long long long)
@ stdcall LsarOpenAccount(ptr ptr long ptr)
@ stdcall LsarOpenPolicy(ptr ptr long ptr)
@ stdcall LsarOpenPolicySce(ptr ptr long ptr)
@ stdcall LsarOpenSecret(ptr ptr long ptr)
@ stdcall LsarOpenTrustedDomain(ptr ptr long ptr)
@ stdcall LsarOpenTrustedDomainByName(ptr ptr long ptr)
@ stdcall LsarQueryDomainInformationPolicy(ptr long ptr)
@ stdcall LsarQueryForestTrustInformation(ptr ptr long ptr)
@ stdcall LsarQueryInfoTrustedDomain(ptr long ptr)
@ stdcall LsarQueryInformationPolicy(ptr long ptr)
@ stdcall LsarQuerySecret(ptr ptr ptr ptr ptr)
@ stdcall LsarQuerySecurityObject(ptr long ptr)
@ stdcall LsarQueryTrustedDomainInfo(ptr ptr long ptr)
@ stdcall LsarQueryTrustedDomainInfoByName(ptr ptr long ptr)
@ stdcall LsarRemovePrivilegesFromAccount(ptr long ptr)
@ stdcall LsarSetDomainInformationPolicy(ptr long long)
@ stdcall LsarSetForestTrustInformation(ptr ptr long ptr long ptr)
@ stdcall LsarSetInformationPolicy(ptr long ptr)
@ stdcall LsarSetInformationTrustedDomain(ptr long ptr)
@ stdcall LsarSetQuotasForAccount(ptr ptr)
@ stdcall LsarSetSecret(ptr ptr ptr)
@ stdcall LsarSetSecurityObject(ptr long ptr)
@ stdcall LsarSetSystemAccessAccount(ptr long)
@ stdcall LsarSetTrustedDomainInfoByName(ptr ptr long long)
@ stdcall ServiceInit()
