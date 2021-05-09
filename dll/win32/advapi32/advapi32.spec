1 stdcall I_ScGetCurrentGroupStateW(ptr wstr ptr)
@ stdcall A_SHAFinal(ptr ptr)
@ stdcall A_SHAInit(ptr)
@ stdcall A_SHAUpdate(ptr ptr long)
@ stdcall AbortSystemShutdownA(ptr)
@ stdcall AbortSystemShutdownW(ptr)
@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmA(str ptr str str ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr)
@ stdcall AccessCheckByTypeAndAuditAlarmA(str ptr str str ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByTypeAndAuditAlarmW(wstr ptr wstr wstr ptr ptr long long long ptr long ptr long ptr ptr ptr)
@ stub AccessCheckByTypeResultList
@ stdcall AccessCheckByTypeResultListAndAuditAlarmA(str ptr str str ptr long long long long ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByTypeResultListAndAuditAlarmByHandleA(str ptr ptr str str ptr long long long long ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByTypeResultListAndAuditAlarmByHandleW(wstr ptr ptr wstr wstr ptr long long long long ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByTypeResultListAndAuditAlarmW(wstr ptr wstr wstr ptr long long long long ptr long ptr long ptr ptr ptr)
@ stdcall AddAccessAllowedAce(ptr long long ptr)
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall AddAccessDeniedAce(ptr long long ptr)
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall AddAce(ptr long long ptr long)
@ stdcall AddAuditAccessAce(ptr long long ptr long long)
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
@ stdcall AddUsersToEncryptedFile(wstr ptr)
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr)
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr)
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall AllocateLocallyUniqueId(ptr)
@ stdcall AreAllAccessesGranted(long long)
@ stdcall AreAnyAccessesGranted(long long)
@ stdcall BackupEventLogA(long str)
@ stdcall BackupEventLogW(long wstr)
@ stdcall BuildExplicitAccessWithNameA(ptr str long long long)
@ stdcall BuildExplicitAccessWithNameW(ptr wstr long long long)
@ stdcall BuildImpersonateExplicitAccessWithNameA(ptr str ptr long long long)
@ stdcall BuildImpersonateExplicitAccessWithNameW(ptr wstr ptr long long long)
@ stdcall BuildImpersonateTrusteeA(ptr ptr)
@ stdcall BuildImpersonateTrusteeW(ptr ptr)
@ stdcall BuildSecurityDescriptorA(ptr ptr long ptr long ptr ptr ptr ptr)
@ stdcall BuildSecurityDescriptorW(ptr ptr long ptr long ptr ptr ptr ptr)
@ stdcall BuildTrusteeWithNameA(ptr str)
@ stdcall BuildTrusteeWithNameW(ptr wstr)
@ stdcall BuildTrusteeWithObjectsAndNameA(ptr ptr long str str str)
@ stdcall BuildTrusteeWithObjectsAndNameW(ptr ptr long wstr wstr wstr)
@ stdcall BuildTrusteeWithObjectsAndSidA(ptr ptr ptr ptr ptr)
@ stdcall BuildTrusteeWithObjectsAndSidW(ptr ptr ptr ptr ptr)
@ stdcall BuildTrusteeWithSidA(ptr ptr)
@ stdcall BuildTrusteeWithSidW(ptr ptr)
@ stub CancelOverlappedAccess
@ stdcall ChangeServiceConfig2A(long long ptr)
@ stdcall ChangeServiceConfig2W(long long ptr)
@ stdcall ChangeServiceConfigA(long long long long wstr str ptr str str str str)
@ stdcall ChangeServiceConfigW(long long long long wstr wstr ptr wstr wstr wstr wstr)
@ stdcall CheckTokenMembership(long ptr ptr)
@ stdcall ClearEventLogA(long str)
@ stdcall ClearEventLogW(long wstr)
@ stub CloseCodeAuthzLevel
@ stdcall CloseEncryptedFileRaw(ptr)
@ stdcall CloseEventLog(long)
@ stdcall CloseServiceHandle(long)
@ stub CloseTrace
@ stdcall CommandLineFromMsiDescriptor(wstr ptr ptr)
@ stub ComputeAccessTokenFromCodeAuthzLevel
@ stdcall ControlService(long long ptr)
@ stdcall ControlTraceA(double str ptr long) ntdll.EtwControlTraceA
@ stdcall ControlTraceW(double wstr ptr long) ntdll.EtwControlTraceW
@ stub ConvertAccessToSecurityDescriptorA
@ stub ConvertAccessToSecurityDescriptorW
@ stub ConvertSDToStringSDRootDomainA
@ stub ConvertSDToStringSDRootDomainW
@ stub ConvertSecurityDescriptorToAccessA
@ stub ConvertSecurityDescriptorToAccessNamedA #ConvertSecurityDescriptorToAccessA
@ stub ConvertSecurityDescriptorToAccessNamedW #ConvertSecurityDescriptorToAccessW
@ stub ConvertSecurityDescriptorToAccessW
@ stdcall ConvertSecurityDescriptorToStringSecurityDescriptorA(ptr long long ptr ptr)
@ stdcall ConvertSecurityDescriptorToStringSecurityDescriptorW(ptr long long ptr ptr)
@ stdcall ConvertSidToStringSidA(ptr ptr)
@ stdcall ConvertSidToStringSidW(ptr ptr)
@ stub ConvertStringSDToSDDomainA
@ stub ConvertStringSDToSDDomainW
@ stub ConvertStringSDToSDRootDomainA
@ stub ConvertStringSDToSDRootDomainW
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorA(str long ptr ptr)
@ stdcall ConvertStringSecurityDescriptorToSecurityDescriptorW(wstr long ptr ptr)
@ stdcall ConvertStringSidToSidA(ptr ptr)
@ stdcall ConvertStringSidToSidW(ptr ptr)
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr)
@ stdcall CopySid(long ptr ptr)
@ stub CreateCodeAuthzLevel
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr)
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long ptr ptr)
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long ptr ptr)
@ stdcall CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr)
@ stdcall CreateProcessAsUserW(long str str ptr ptr long long ptr str ptr ptr)
@ stdcall CreateProcessWithLogonW(wstr wstr wstr long wstr wstr long ptr wstr ptr ptr)
@ stdcall CreateProcessWithTokenW(ptr long wstr wstr long ptr wstr ptr ptr)
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr)
@ stdcall CreateServiceA(long str str long long long long str str ptr str str str)
@ stdcall CreateServiceW(long wstr wstr long long long long wstr wstr ptr wstr wstr wstr)
@ stdcall CreateTraceInstanceId(ptr ptr) ntdll.EtwCreateTraceInstanceId
@ stdcall CreateWellKnownSid(long ptr ptr ptr)
@ stdcall CredDeleteA(str long long)
@ stdcall CredDeleteW(wstr long long)
@ stdcall CredEnumerateA(str long ptr ptr)
@ stdcall CredEnumerateW(wstr long ptr ptr)
@ stdcall CredFree(ptr)
@ stdcall CredGetSessionTypes(long ptr)
@ stub CredGetTargetInfoA
@ stub CredGetTargetInfoW
@ stdcall CredIsMarshaledCredentialA(str)
@ stdcall CredIsMarshaledCredentialW(wstr)
@ stdcall CredMarshalCredentialA(long ptr str)
@ stdcall CredMarshalCredentialW(long ptr wstr)
@ stub CredProfileLoaded
@ stdcall CredReadA(str long long ptr)
@ stdcall CredReadDomainCredentialsA(ptr long ptr ptr)
@ stdcall CredReadDomainCredentialsW(ptr long ptr ptr)
@ stdcall CredReadW(wstr long long ptr)
@ stub CredRenameA
@ stub CredRenameW
@ stdcall CredUnmarshalCredentialA(str ptr ptr)
@ stdcall CredUnmarshalCredentialW(wstr ptr ptr)
@ stdcall CredWriteA(ptr long)
# CredWriteDomainCredentialsA
@ stub CredWriteDomainCredentialsW
@ stdcall CredWriteW(ptr long)
@ stub CredpConvertCredential
@ stub CredpConvertTargetInfo
@ stub CredpDecodeCredential
@ stub CredpEncodeCredential
@ stdcall CryptAcquireContextA(ptr str str long long)
@ stdcall CryptAcquireContextW(ptr wstr wstr long long)
@ stdcall CryptContextAddRef(long ptr long)
@ stdcall CryptCreateHash(long long long long ptr)
@ stdcall CryptDecrypt(long long long long ptr ptr)
@ stdcall CryptDeriveKey(long long long long ptr)
@ stdcall CryptDestroyHash(long)
@ stdcall CryptDestroyKey(long)
@ stdcall CryptDuplicateHash(long ptr long ptr)
@ stdcall CryptDuplicateKey(long ptr long ptr)
@ stdcall CryptEncrypt(long long long long ptr ptr long)
@ stdcall CryptEnumProviderTypesA(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProviderTypesW(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProvidersA(long ptr long ptr ptr ptr)
@ stdcall CryptEnumProvidersW(long ptr long ptr ptr ptr)
@ stdcall CryptExportKey(long long long long ptr ptr)
@ stdcall CryptGenKey(long long long ptr)
@ stdcall CryptGenRandom(long long ptr)
@ stdcall CryptGetDefaultProviderA(long ptr long ptr ptr)
@ stdcall CryptGetDefaultProviderW(long ptr long ptr ptr)
@ stdcall CryptGetHashParam(long long ptr ptr long)
@ stdcall CryptGetKeyParam(long long ptr ptr long)
@ stdcall CryptGetProvParam(long long ptr ptr long)
@ stdcall CryptGetUserKey(long long ptr)
@ stdcall CryptHashData(long ptr long long)
@ stdcall CryptHashSessionKey(long long long)
@ stdcall CryptImportKey(long ptr long long long ptr)
@ stdcall CryptReleaseContext(long long)
@ stdcall CryptSetHashParam(long long ptr long)
@ stdcall CryptSetKeyParam(long long ptr long)
@ stdcall CryptSetProvParam(long long ptr long)
@ stdcall CryptSetProviderA(str long)
@ stdcall CryptSetProviderExA(str long ptr long)
@ stdcall CryptSetProviderExW(wstr long ptr long)
@ stdcall CryptSetProviderW(wstr long)
@ stdcall CryptSignHashA(long long ptr long ptr ptr)
@ stdcall CryptSignHashW(long long ptr long ptr ptr)
@ stdcall CryptVerifySignatureA(long ptr long long ptr long)
@ stdcall CryptVerifySignatureW(long ptr long long ptr long)
@ stdcall DecryptFileA(str long)
@ stdcall DecryptFileW(wstr long)
@ stdcall DeleteAce(ptr long)
@ stdcall DeleteService(long)
@ stdcall DeregisterEventSource(long)
@ stdcall DestroyPrivateObjectSecurity(ptr)
@ stub DuplicateEncryptionInfoFile
@ stdcall DuplicateToken(long long ptr)
@ stdcall DuplicateTokenEx(long long ptr long long ptr)
@ stdcall ElfBackupEventLogFileA(long ptr)
@ stdcall ElfBackupEventLogFileW(long ptr)
@ stdcall ElfChangeNotify(long long)
@ stdcall ElfClearEventLogFileA(long ptr)
@ stdcall ElfClearEventLogFileW(long ptr)
@ stdcall ElfCloseEventLog(long)
@ stdcall ElfDeregisterEventSource(long)
@ stdcall ElfFlushEventLog(long)
@ stdcall ElfNumberOfRecords(long ptr)
@ stdcall ElfOldestRecord(long ptr)
@ stdcall ElfOpenBackupEventLogA(ptr ptr ptr)
@ stdcall ElfOpenBackupEventLogW(ptr ptr ptr)
@ stdcall ElfOpenEventLogA(ptr ptr ptr)
@ stdcall ElfOpenEventLogW(ptr ptr ptr)
@ stdcall ElfReadEventLogA(long long long ptr long ptr ptr)
@ stdcall ElfReadEventLogW(long long long ptr long ptr ptr)
@ stdcall ElfRegisterEventSourceA(ptr ptr ptr)
@ stdcall ElfRegisterEventSourceW(ptr ptr ptr)
@ stdcall ElfReportEventA(long long long long ptr long long ptr ptr long ptr ptr)
@ stdcall ElfReportEventAndSourceW(long long ptr long long long ptr ptr long long ptr ptr long ptr ptr)
@ stdcall ElfReportEventW(long long long long ptr long long ptr ptr long ptr ptr)
@ stdcall EnableTrace(long long long ptr double) ntdll.EtwEnableTrace
@ stdcall EncryptFileA(str)
@ stdcall EncryptFileW(wstr)
@ stub EncryptedFileKeyInfo
@ stdcall EncryptionDisable(wstr long)
@ stdcall EnumDependentServicesA(long long ptr long ptr ptr)
@ stdcall EnumDependentServicesW(long long ptr long ptr ptr)
@ stdcall EnumServiceGroupW(ptr long long ptr long ptr ptr ptr wstr)
@ stdcall EnumServicesStatusA(long long long ptr long ptr ptr ptr)
@ stdcall EnumServicesStatusExA(long long long long ptr long ptr ptr ptr str)
@ stdcall EnumServicesStatusExW(long long long long ptr long ptr ptr ptr wstr)
@ stdcall EnumServicesStatusW(long long long ptr long ptr ptr ptr)
@ stdcall EnumerateTraceGuids(ptr long ptr) ntdll.EtwEnumerateTraceGuids
@ stdcall EqualDomainSid(ptr ptr ptr)
@ stdcall EqualPrefixSid(ptr ptr)
@ stdcall EqualSid(ptr ptr)
@ stdcall FileEncryptionStatusA(str ptr)
@ stdcall FileEncryptionStatusW(wstr ptr)
@ stdcall FindFirstFreeAce(ptr ptr)
@ stdcall FlushTraceA(double str ptr) ntdll.EtwFlushTraceA
@ stdcall FlushTraceW(double wstr ptr) ntdll.EtwFlushTraceW
@ stub FreeEncryptedFileKeyInfo
@ stdcall FreeEncryptionCertificateHashList(ptr)
@ stdcall FreeInheritedFromArray(ptr long ptr)
@ stdcall FreeSid(ptr)
@ stub GetAccessPermissionsForObjectA
@ stub GetAccessPermissionsForObjectW
@ stdcall GetAce(ptr long ptr)
@ stdcall GetAclInformation(ptr ptr long long)
@ stdcall GetAuditedPermissionsFromAclA(ptr ptr ptr ptr)
@ stdcall GetAuditedPermissionsFromAclW(ptr ptr ptr ptr)
@ stdcall GetCurrentHwProfileA(ptr)
@ stdcall GetCurrentHwProfileW(ptr)
@ stdcall GetEffectiveRightsFromAclA(ptr ptr ptr)
@ stdcall GetEffectiveRightsFromAclW(ptr ptr ptr)
@ stdcall GetEventLogInformation(long long ptr long ptr)
@ stdcall GetExplicitEntriesFromAclA(ptr ptr ptr) advapi32.GetExplicitEntriesFromAclW
@ stdcall GetExplicitEntriesFromAclW(ptr ptr ptr)
@ stdcall GetFileSecurityA(str long ptr long ptr)
@ stdcall GetFileSecurityW(wstr long ptr long ptr)
@ stub GetInformationCodeAuthzLevelW
@ stub GetInformationCodeAuthzPolicyW
@ stdcall GetInheritanceSourceA(str long long long ptr long ptr ptr ptr ptr)
@ stdcall GetInheritanceSourceW(wstr long long long ptr long ptr ptr ptr ptr)
@ stdcall GetKernelObjectSecurity(long long ptr long ptr)
@ stdcall GetLengthSid(ptr)
@ stub GetLocalManagedApplicationData
@ stub GetLocalManagedApplications
@ stub GetManagedApplicationCategories
@ stub GetManagedApplications
@ stdcall GetMultipleTrusteeA(ptr)
@ stdcall GetMultipleTrusteeOperationA(ptr)
@ stdcall GetMultipleTrusteeOperationW(ptr)
@ stdcall GetMultipleTrusteeW(ptr)
@ stdcall GetNamedSecurityInfoA(str long long ptr ptr ptr ptr ptr)
@ stub GetNamedSecurityInfoExA
@ stub GetNamedSecurityInfoExW
@ stdcall GetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr ptr)
@ stdcall GetNumberOfEventLogRecords(long ptr)
@ stdcall GetOldestEventLogRecord(long ptr)
@ stub GetOverlappedAccessResults
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr)
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr)
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr)
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr)
@ stdcall GetSecurityDescriptorLength(ptr) ntdll.RtlLengthSecurityDescriptor
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr)
@ stdcall GetSecurityDescriptorRMControl(ptr ptr)
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr)
@ stdcall GetSecurityInfo(long long long ptr ptr ptr ptr ptr)
@ stdcall GetSecurityInfoExA(long long long str str ptr ptr ptr ptr)
@ stdcall GetSecurityInfoExW(long long long wstr wstr ptr ptr ptr ptr)
@ stdcall GetServiceDisplayNameA(ptr str ptr ptr)
@ stdcall GetServiceDisplayNameW(ptr wstr ptr ptr)
@ stdcall GetServiceKeyNameA(long str ptr ptr)
@ stdcall GetServiceKeyNameW(long wstr ptr ptr)
@ stdcall GetSidIdentifierAuthority(ptr)
@ stdcall GetSidLengthRequired(long)
@ stdcall GetSidSubAuthority(ptr long)
@ stdcall GetSidSubAuthorityCount(ptr)
@ stdcall GetTokenInformation(long long ptr long ptr)
@ stdcall GetTraceEnableFlags(double) ntdll.EtwGetTraceEnableFlags
@ stdcall GetTraceEnableLevel(double) ntdll.EtwGetTraceEnableLevel
@ stdcall GetTraceLoggerHandle(ptr) ntdll.EtwGetTraceLoggerHandle
@ stdcall GetTrusteeFormA(ptr)
@ stdcall GetTrusteeFormW(ptr)
@ stdcall GetTrusteeNameA(ptr)
@ stdcall GetTrusteeNameW(ptr)
@ stdcall GetTrusteeTypeA(ptr)
@ stdcall GetTrusteeTypeW(ptr)
@ stdcall GetUserNameA(ptr ptr)
@ stdcall GetUserNameW(ptr ptr)
@ stdcall GetWindowsAccountDomainSid(ptr ptr ptr)
@ stdcall I_QueryTagInformation(ptr long ptr)
@ stdcall I_ScIsSecurityProcess()
@ stdcall I_ScPnPGetServiceName(ptr wstr long)
@ stub I_ScSendTSMessage
@ stdcall I_ScSetServiceBitsA(ptr long long long str)
@ stdcall I_ScSetServiceBitsW(ptr long long long wstr)
@ stub IdentifyCodeAuthzLevelW
@ stdcall ImpersonateAnonymousToken(ptr)
@ stdcall ImpersonateLoggedOnUser(long)
@ stdcall ImpersonateNamedPipeClient(long)
@ stdcall ImpersonateSelf(long)
@ stdcall InitializeAcl(ptr long long)
@ stdcall InitializeSecurityDescriptor(ptr long)
@ stdcall InitializeSid(ptr ptr long)
@ stdcall InitiateSystemShutdownA(str str long long long)
@ stdcall InitiateSystemShutdownExA(str str long long long long)
@ stdcall InitiateSystemShutdownExW(wstr wstr long long long long)
@ stdcall InitiateSystemShutdownW(str str long long long)
@ stub InstallApplication
@ stdcall IsTextUnicode(ptr long ptr)
@ stdcall IsTokenRestricted(long)
@ stub IsTokenUntrusted
@ stdcall IsValidAcl(ptr)
@ stdcall IsValidSecurityDescriptor(ptr)
@ stdcall IsValidSid(ptr)
@ stdcall IsWellKnownSid(ptr long)
@ stdcall LockServiceDatabase(ptr)
@ stdcall LogonUserA(str str str long long ptr)
@ stdcall LogonUserExA(str str str long long ptr ptr ptr ptr ptr)
@ stdcall LogonUserExW(wstr wstr wstr long long ptr ptr ptr ptr ptr)
@ stdcall LogonUserW(wstr wstr wstr long long ptr)
@ stdcall LookupAccountNameA(str str ptr ptr ptr ptr ptr)
@ stdcall LookupAccountNameW(wstr wstr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidA(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupAccountSidW(ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LookupPrivilegeDisplayNameA(str str str ptr ptr)
@ stdcall LookupPrivilegeDisplayNameW(wstr wstr wstr ptr ptr)
@ stdcall LookupPrivilegeNameA(str ptr ptr long)
@ stdcall LookupPrivilegeNameW(wstr ptr ptr long)
@ stdcall LookupPrivilegeValueA(ptr ptr ptr)
@ stdcall LookupPrivilegeValueW(ptr ptr ptr)
@ stub LookupSecurityDescriptorPartsA
@ stub LookupSecurityDescriptorPartsW
@ stdcall LsaAddAccountRights(ptr ptr ptr long)
@ stdcall LsaAddPrivilegesToAccount(ptr ptr)
@ stdcall LsaClearAuditLog(ptr)
@ stdcall LsaClose(ptr)
@ stdcall LsaCreateAccount(ptr ptr long ptr)
@ stdcall LsaCreateSecret(ptr ptr long ptr)
@ stdcall LsaCreateTrustedDomain(ptr ptr long ptr)
@ stdcall LsaCreateTrustedDomainEx(ptr ptr ptr long ptr)
@ stdcall LsaDelete(ptr)
@ stdcall LsaDeleteTrustedDomain(ptr ptr)
@ stdcall LsaEnumerateAccountRights(ptr ptr ptr ptr)
@ stdcall LsaEnumerateAccounts(ptr ptr ptr long ptr)
@ stdcall LsaEnumerateAccountsWithUserRight(ptr ptr ptr ptr)
@ stdcall LsaEnumeratePrivileges(ptr ptr ptr long ptr)
@ stdcall LsaEnumeratePrivilegesOfAccount(ptr ptr)
@ stdcall LsaEnumerateTrustedDomains(ptr ptr ptr long ptr)
@ stdcall LsaEnumerateTrustedDomainsEx(ptr ptr ptr long ptr)
@ stdcall LsaFreeMemory(ptr)
@ stdcall LsaGetQuotasForAccount(ptr ptr)
@ stdcall LsaGetRemoteUserName(ptr ptr ptr)
@ stdcall LsaGetSystemAccessAccount(ptr ptr)
@ stdcall LsaGetUserName(ptr ptr)
@ stub LsaICLookupNames
@ stub LsaICLookupNamesWithCreds
@ stub LsaICLookupSids
@ stub LsaICLookupSidsWithCreds
@ stdcall LsaLookupNames2(ptr long long ptr ptr ptr)
@ stdcall LsaLookupNames(ptr long ptr ptr ptr)
@ stdcall LsaLookupPrivilegeDisplayName(ptr ptr ptr ptr)
@ stdcall LsaLookupPrivilegeName(ptr ptr ptr)
@ stdcall LsaLookupPrivilegeValue(ptr ptr ptr)
@ stdcall LsaLookupSids(ptr long ptr ptr ptr)
@ stdcall LsaNtStatusToWinError(long)
@ stdcall LsaOpenAccount(ptr ptr long ptr)
@ stdcall LsaOpenPolicy(ptr ptr long ptr)
@ stdcall LsaOpenPolicySce(ptr ptr long ptr)
@ stdcall LsaOpenSecret(ptr ptr long ptr)
@ stdcall LsaOpenTrustedDomain(ptr ptr long ptr)
@ stdcall LsaOpenTrustedDomainByName(ptr ptr long ptr)
@ stdcall LsaQueryDomainInformationPolicy(ptr long ptr)
@ stdcall LsaQueryForestTrustInformation(ptr ptr ptr)
@ stdcall LsaQueryInfoTrustedDomain(ptr long ptr)
@ stdcall LsaQueryInformationPolicy(ptr long ptr)
@ stdcall LsaQuerySecret(ptr ptr ptr ptr ptr)
@ stdcall LsaQuerySecurityObject(ptr long ptr)
@ stdcall LsaQueryTrustedDomainInfo(ptr ptr long ptr)
@ stdcall LsaQueryTrustedDomainInfoByName(ptr ptr long ptr)
@ stdcall LsaRemoveAccountRights(ptr ptr long ptr long)
@ stdcall LsaRemovePrivilegesFromAccount(ptr long ptr)
@ stdcall LsaRetrievePrivateData(ptr ptr ptr)
@ stdcall LsaSetDomainInformationPolicy(ptr long ptr)
@ stdcall LsaSetForestTrustInformation(ptr ptr ptr long ptr)
@ stdcall LsaSetInformationPolicy(ptr long ptr)
@ stdcall LsaSetInformationTrustedDomain(ptr long ptr)
@ stdcall LsaSetQuotasForAccount(ptr ptr)
@ stdcall LsaSetSecret(ptr ptr ptr)
@ stdcall LsaSetSecurityObject(ptr long ptr)
@ stdcall LsaSetSystemAccessAccount(ptr long)
@ stdcall LsaSetTrustedDomainInfoByName(ptr ptr long ptr)
@ stdcall LsaSetTrustedDomainInformation(ptr ptr long ptr)
@ stdcall LsaStorePrivateData(ptr ptr ptr)
@ stdcall MD4Final(ptr)
@ stdcall MD4Init(ptr)
@ stdcall MD4Update(ptr ptr long)
@ stdcall MD5Final(ptr)
@ stdcall MD5Init(ptr)
@ stdcall MD5Update(ptr ptr long)
@ stub MSChapSrvChangePassword2
@ stub MSChapSrvChangePassword
@ stdcall MakeAbsoluteSD2(ptr ptr)
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall MakeSelfRelativeSD(ptr ptr ptr)
@ stdcall MapGenericMask(ptr ptr) ntdll.RtlMapGenericMask
@ stdcall NotifyBootConfigStatus(long)
@ stdcall NotifyChangeEventLog(long long)
@ stdcall ObjectCloseAuditAlarmA(str ptr long)
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long)
@ stdcall ObjectDeleteAuditAlarmA(str ptr long)
@ stdcall ObjectDeleteAuditAlarmW(wstr ptr long)
@ stdcall ObjectOpenAuditAlarmA(str ptr str str ptr long long long ptr long long ptr)
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr)
@ stdcall ObjectPrivilegeAuditAlarmA(str ptr long long ptr long)
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long)
@ stdcall OpenBackupEventLogA(str str)
@ stdcall OpenBackupEventLogW(wstr wstr)
@ stdcall OpenEncryptedFileRawA(str long ptr)
@ stdcall OpenEncryptedFileRawW(wstr long ptr)
@ stdcall OpenEventLogA(str str)
@ stdcall OpenEventLogW(wstr wstr)
@ stdcall OpenProcessToken(long long ptr)
@ stdcall OpenSCManagerA(str str long)
@ stdcall OpenSCManagerW(wstr wstr long)
@ stdcall OpenServiceA(long str long)
@ stdcall OpenServiceW(long wstr long)
@ stdcall OpenThreadToken(long long long ptr)
@ stdcall -ret64 OpenTraceA(ptr)
@ stdcall -ret64 OpenTraceW(ptr)
@ stdcall PrivilegeCheck(ptr ptr ptr)
@ stdcall PrivilegedServiceAuditAlarmA(str str long ptr long)
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long)
@ stub ProcessIdleTasks
@ stdcall ProcessTrace(ptr long ptr ptr)
@ stdcall QueryAllTracesA(ptr long ptr) ntdll.EtwQueryAllTracesA
@ stdcall QueryAllTracesW(ptr long ptr) ntdll.EtwQueryAllTracesW
@ stdcall QueryRecoveryAgentsOnEncryptedFile(wstr ptr)
@ stdcall QueryServiceConfig2A(long long ptr long ptr)
@ stdcall QueryServiceConfig2W(long long ptr long ptr)
@ stdcall QueryServiceConfigA(long ptr long ptr)
@ stdcall QueryServiceConfigW(long ptr long ptr)
@ stdcall QueryServiceLockStatusA(long ptr long ptr)
@ stdcall QueryServiceLockStatusW(long ptr long ptr)
@ stdcall QueryServiceObjectSecurity(long long ptr long ptr)
@ stdcall QueryServiceStatus(long ptr)
@ stdcall QueryServiceStatusEx(long long ptr long ptr)
@ stdcall QueryTraceA(double str ptr) ntdll.EtwQueryTraceA
@ stdcall QueryTraceW(double str ptr) ntdll.EtwQueryTraceA
@ stdcall QueryUsersOnEncryptedFile(wstr ptr)
@ stdcall ReadEncryptedFileRaw(ptr ptr ptr)
@ stdcall ReadEventLogA(long long long ptr long ptr ptr)
@ stdcall ReadEventLogW(long long long ptr long ptr ptr)
@ stdcall RegCloseKey(long)
@ stdcall RegConnectRegistryA(str long ptr)
@ stub RegConnectRegistryExA
@ stub RegConnectRegistryExW
@ stdcall RegConnectRegistryW(wstr long ptr)
@ stdcall RegCreateKeyA(long str ptr)
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr)
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr)
@ stdcall RegCreateKeyW(long wstr ptr)
@ stdcall RegDeleteKeyA(long str)
@ stdcall RegDeleteKeyExA(long str long long)
@ stdcall RegDeleteKeyExW(long wstr long long)
@ stdcall RegDeleteKeyW(long wstr)
@ stdcall RegDeleteValueA(long str)
@ stdcall RegDeleteValueW(long wstr)
@ stdcall RegDisablePredefinedCache()
@ stdcall RegDisableReflectionKey(ptr)
@ stdcall RegEnableReflectionKey(ptr)
@ stdcall RegEnumKeyA(long long ptr long)
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumKeyW(long long ptr long)
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegFlushKey(long)
@ stdcall RegGetKeySecurity(long long ptr ptr)
@ stdcall RegGetValueA(long str str long ptr ptr ptr)
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr)
@ stdcall RegLoadKeyA(long str str)
@ stdcall RegLoadKeyW(long wstr wstr)
@ stdcall RegNotifyChangeKeyValue(long long long long long)
@ stdcall RegOpenCurrentUser(long ptr)
@ stdcall RegOpenKeyA(long str ptr)
@ stdcall RegOpenKeyExA(long str long long ptr)
@ stdcall RegOpenKeyExW(long wstr long long ptr)
@ stdcall RegOpenKeyW(long wstr ptr)
@ stdcall RegOpenUserClassesRoot(ptr long long ptr)
@ stdcall RegOverridePredefKey(long long)
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryMultipleValuesA(long ptr long ptr ptr)
@ stdcall RegQueryMultipleValuesW(long ptr long ptr ptr)
@ stdcall RegQueryReflectionKey(ptr ptr)
@ stdcall RegQueryValueA(long str ptr ptr)
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr)
@ stdcall RegQueryValueW(long wstr ptr ptr)
@ stdcall RegReplaceKeyA(long str str str)
@ stdcall RegReplaceKeyW(long wstr wstr wstr)
@ stdcall RegRestoreKeyA(long str long)
@ stdcall RegRestoreKeyW(long wstr long)
@ stdcall RegSaveKeyA(long ptr ptr)
@ stdcall RegSaveKeyExA(long str ptr long)
@ stdcall RegSaveKeyExW(long str ptr long)
@ stdcall RegSaveKeyW(long ptr ptr)
@ stdcall RegSetKeySecurity(long long ptr)
@ stdcall RegSetValueA(long str long ptr long)
@ stdcall RegSetValueExA(long str long long ptr long)
@ stdcall RegSetValueExW(long wstr long long ptr long)
@ stdcall RegSetValueW(long wstr long ptr long)
@ stdcall RegUnLoadKeyA(long str)
@ stdcall RegUnLoadKeyW(long wstr)
@ stdcall RegisterEventSourceA(ptr ptr)
@ stdcall RegisterEventSourceW(ptr ptr)
@ stub RegisterIdleTask
@ stdcall RegisterServiceCtrlHandlerA(str ptr)
@ stdcall RegisterServiceCtrlHandlerExA(str ptr ptr)
@ stdcall RegisterServiceCtrlHandlerExW(wstr ptr ptr)
@ stdcall RegisterServiceCtrlHandlerW(wstr ptr)
@ stdcall RegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr) ntdll.EtwRegisterTraceGuidsA
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) ntdll.EtwRegisterTraceGuidsW
@ stub RemoveTraceCallback
@ stdcall RemoveUsersFromEncryptedFile(wstr ptr)
@ stdcall ReportEventA(long long long long ptr long long str ptr)
@ stdcall ReportEventW(long long long long ptr long long wstr ptr)
@ stdcall RevertToSelf()
@ stdcall SaferCloseLevel(ptr)
@ stdcall SaferComputeTokenFromLevel(ptr ptr ptr long ptr)
@ stdcall SaferCreateLevel(long long long ptr ptr)
@ stub SaferGetLevelInformation
@ stdcall SaferGetPolicyInformation(long long long ptr ptr ptr)
@ stdcall SaferIdentifyLevel(long ptr ptr ptr)
@ stdcall SaferRecordEventLogEntry(ptr wstr ptr)
@ stub SaferSetLevelInformation
@ stub SaferSetPolicyInformation
@ stub SaferiChangeRegistryScope
@ stub SaferiCompareTokenLevels
@ stub SaferiIsExecutableFileType
@ stub SaferiPopulateDefaultsInRegistry
@ stub SaferiRecordEventLogEntry
@ stub SaferiReplaceProcessThreadTokens
@ stub SaferiSearchMatchingHashRules
@ stdcall SetAclInformation(ptr ptr long long)
@ stub SetEntriesInAccessListA
@ stub SetEntriesInAccessListW
@ stdcall SetEntriesInAclA(long ptr ptr ptr)
@ stdcall SetEntriesInAclW(long ptr ptr ptr)
@ stub SetEntriesInAuditListA
@ stub SetEntriesInAuditListW
@ stdcall SetFileSecurityA(str long ptr)
@ stdcall SetFileSecurityW(wstr long ptr)
@ stub SetInformationCodeAuthzLevelW
@ stub SetInformationCodeAuthzPolicyW
@ stdcall SetKernelObjectSecurity(long long ptr)
@ stdcall SetNamedSecurityInfoA(str long ptr ptr ptr ptr ptr)
@ stub SetNamedSecurityInfoExA
@ stub SetNamedSecurityInfoExW
@ stdcall SetNamedSecurityInfoW(wstr long ptr ptr ptr ptr ptr)
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long)
@ stub SetPrivateObjectSecurityEx
@ stdcall SetSecurityDescriptorControl(ptr long long)
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long)
@ stdcall SetSecurityDescriptorGroup(ptr ptr long)
@ stdcall SetSecurityDescriptorOwner(ptr ptr long)
@ stdcall SetSecurityDescriptorRMControl(ptr ptr)
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long)
@ stdcall SetSecurityInfo(long long long ptr ptr ptr ptr)
@ stub SetSecurityInfoExA
@ stub SetSecurityInfoExW
@ stdcall SetServiceBits(long long long long)
@ stdcall SetServiceObjectSecurity(long long ptr)
@ stdcall SetServiceStatus(long long)
@ stdcall SetThreadToken(ptr ptr)
@ stdcall SetTokenInformation(long long ptr long)
@ stub SetTraceCallback
@ stub SetUserFileEncryptionKey
@ stdcall StartServiceA(long long ptr)
@ stdcall StartServiceCtrlDispatcherA(ptr)
@ stdcall StartServiceCtrlDispatcherW(ptr)
@ stdcall StartServiceW(long long ptr)
@ stdcall StartTraceA(ptr str ptr) ntdll.EtwStartTraceA
@ stdcall StartTraceW(ptr wstr ptr) ntdll.EtwStartTraceW
@ stdcall StopTraceA(double str ptr) ntdll.EtwStopTraceA
@ stdcall StopTraceW(double wstr ptr) ntdll.EtwStopTraceA
@ stdcall SystemFunction001(ptr ptr ptr)
@ stdcall SystemFunction002(ptr ptr ptr)
@ stdcall SystemFunction003(ptr ptr)
@ stdcall SystemFunction004(ptr ptr ptr)
@ stdcall SystemFunction005(ptr ptr ptr)
@ stdcall SystemFunction006(ptr ptr)
@ stdcall SystemFunction007(ptr ptr)
@ stdcall SystemFunction008(ptr ptr ptr)
@ stdcall SystemFunction009(ptr ptr ptr)
@ stdcall SystemFunction010(ptr ptr ptr)
@ stdcall SystemFunction011(ptr ptr ptr) SystemFunction010
@ stdcall SystemFunction012(ptr ptr ptr)
@ stdcall SystemFunction013(ptr ptr ptr)
@ stdcall SystemFunction014(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction015(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction016(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction017(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction018(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction019(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction020(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction021(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction022(ptr ptr ptr) SystemFunction012
@ stdcall SystemFunction023(ptr ptr ptr) SystemFunction013
@ stdcall SystemFunction024(ptr ptr ptr)
@ stdcall SystemFunction025(ptr ptr ptr)
@ stdcall SystemFunction026(ptr ptr ptr) SystemFunction024
@ stdcall SystemFunction027(ptr ptr ptr) SystemFunction025
@ stdcall SystemFunction028(long long)
@ stdcall SystemFunction029(long long)
@ stdcall SystemFunction030(ptr ptr)
@ stdcall SystemFunction031(ptr ptr) SystemFunction030
@ stdcall SystemFunction032(ptr ptr)
@ stdcall SystemFunction033(long long)
@ stdcall SystemFunction034(long long)
@ stdcall SystemFunction035(str)
@ stdcall SystemFunction036(ptr long) # RtlGenRandom
@ stdcall SystemFunction040(ptr long long) # RtlEncryptMemory
@ stdcall SystemFunction041(ptr long long) # RtlDecryptMemory
@ stdcall TraceEvent(double ptr) ntdll.EtwTraceEvent
@ stdcall TraceEventInstance(double ptr ptr ptr) ntdll.EtwTraceEventInstance
@ varargs TraceMessage() ntdll.EtwTraceMessage
@ stdcall TraceMessageVa() ntdll.EtwTraceMessageVa
@ stdcall TreeResetNamedSecurityInfoA(str ptr ptr ptr ptr ptr ptr long ptr ptr ptr)
@ stdcall TreeResetNamedSecurityInfoW(wstr long long ptr ptr ptr ptr long ptr long ptr)
@ stub TrusteeAccessToObjectA
@ stub TrusteeAccessToObjectW
@ stub UninstallApplication
@ stdcall UnlockServiceDatabase(ptr)
@ stub UnregisterIdleTask
@ stdcall UnregisterTraceGuids(double) ntdll.EtwUnregisterTraceGuids
@ stdcall UpdateTraceA(double str ptr) ntdll.EtwUpdateTraceA
@ stdcall UpdateTraceW(double wstr ptr) ntdll.EtwUpdateTraceW
@ stub WdmWmiServiceMain
@ stub WmiCloseBlock
@ stub WmiCloseTraceWithCursor
@ stub WmiConvertTimestamp
@ stub WmiDevInstToInstanceNameA
@ stub WmiDevInstToInstanceNameW
@ stub WmiEnumerateGuids
@ stub WmiExecuteMethodA
@ stub WmiExecuteMethodW
@ stub WmiFileHandleToInstanceNameA
@ stub WmiFileHandleToInstanceNameW
@ stub WmiFreeBuffer
@ stub WmiGetFirstTraceOffset
@ stub WmiGetNextEvent
@ stub WmiGetTraceHeader
@ stub WmiMofEnumerateResourcesA
@ stub WmiMofEnumerateResourcesW
@ stdcall WmiNotificationRegistrationA(ptr long ptr long long) ntdll.EtwNotificationRegistrationA
@ stdcall WmiNotificationRegistrationW(ptr long ptr long long) ntdll.EtwNotificationRegistrationW
@ stub WmiOpenBlock
@ stub WmiOpenTraceWithCursor
@ stub WmiParseTraceEvent
@ stub WmiQueryAllDataA
@ stub WmiQueryAllDataMultipleA
@ stub WmiQueryAllDataMultipleW
@ stub WmiQueryAllDataW
@ stub WmiQueryGuidInformation
@ stub WmiQuerySingleInstanceA
@ stub WmiQuerySingleInstanceMultipleA
@ stub WmiQuerySingleInstanceMultipleW
@ stub WmiQuerySingleInstanceW
@ stdcall WmiReceiveNotificationsA(long long long long) ntdll.EtwReceiveNotificationsA
@ stdcall WmiReceiveNotificationsW(long long long long) ntdll.EtwReceiveNotificationsW
@ stub WmiSetSingleInstanceA
@ stub WmiSetSingleInstanceW
@ stub WmiSetSingleItemA
@ stub WmiSetSingleItemW
@ stub Wow64Win32ApiEntry
@ stdcall WriteEncryptedFileRaw(ptr ptr ptr)
@ stdcall -version=0x600+ RegLoadMUIStringW(ptr wstr wstr long ptr long wstr) advapi32_vista.RegLoadMUIStringW
@ stdcall -version=0x600+ RegLoadMUIStringA(ptr str str long ptr long str) advapi32_vista.RegLoadMUIStringA
