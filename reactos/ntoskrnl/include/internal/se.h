#ifndef __NTOSKRNL_INCLUDE_INTERNAL_SE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_SE_H

extern POBJECT_TYPE SepTokenObjectType;

/* SID Authorities */
extern SID_IDENTIFIER_AUTHORITY SeNullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeWorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeLocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeCreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeNtSidAuthority;

/* SIDs */
extern PSID SeNullSid;
extern PSID SeWorldSid;
extern PSID SeLocalSid;
extern PSID SeCreatorOwnerSid;
extern PSID SeCreatorGroupSid;
extern PSID SeCreatorOwnerServerSid;
extern PSID SeCreatorGroupServerSid;
extern PSID SeNtAuthoritySid;
extern PSID SeDialupSid;
extern PSID SeNetworkSid;
extern PSID SeBatchSid;
extern PSID SeInteractiveSid;
extern PSID SeServiceSid;
extern PSID SeAnonymousLogonSid;
extern PSID SePrincipalSelfSid;
extern PSID SeLocalSystemSid;
extern PSID SeAuthenticatedUserSid;
extern PSID SeRestrictedCodeSid;
extern PSID SeAliasAdminsSid;
extern PSID SeAliasUsersSid;
extern PSID SeAliasGuestsSid;
extern PSID SeAliasPowerUsersSid;
extern PSID SeAliasAccountOpsSid;
extern PSID SeAliasSystemOpsSid;
extern PSID SeAliasPrintOpsSid;
extern PSID SeAliasBackupOpsSid;

/* Privileges */
extern LUID SeCreateTokenPrivilege;
extern LUID SeAssignPrimaryTokenPrivilege;
extern LUID SeLockMemoryPrivilege;
extern LUID SeIncreaseQuotaPrivilege;
extern LUID SeUnsolicitedInputPrivilege;
extern LUID SeTcbPrivilege;
extern LUID SeSecurityPrivilege;
extern LUID SeTakeOwnershipPrivilege;
extern LUID SeLoadDriverPrivilege;
extern LUID SeCreatePagefilePrivilege;
extern LUID SeIncreaseBasePriorityPrivilege;
extern LUID SeSystemProfilePrivilege;
extern LUID SeSystemtimePrivilege;
extern LUID SeProfileSingleProcessPrivilege;
extern LUID SeCreatePermanentPrivilege;
extern LUID SeBackupPrivilege;
extern LUID SeRestorePrivilege;
extern LUID SeShutdownPrivilege;
extern LUID SeDebugPrivilege;
extern LUID SeAuditPrivilege;
extern LUID SeSystemEnvironmentPrivilege;
extern LUID SeChangeNotifyPrivilege;
extern LUID SeRemoteShutdownPrivilege;

/* DACLs */
extern PACL SePublicDefaultUnrestrictedDacl;
extern PACL SePublicOpenDacl;
extern PACL SePublicOpenUnrestrictedDacl;
extern PACL SeUnrestrictedDacl;

/* SDs */
extern PSECURITY_DESCRIPTOR SePublicDefaultSd;
extern PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SePublicOpenSd;
extern PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SeSystemDefaultSd;
extern PSECURITY_DESCRIPTOR SeUnrestrictedSd;


/* Functions */

BOOLEAN SeInit1(VOID);
BOOLEAN SeInit2(VOID);
BOOLEAN SeInitSRM(VOID);

VOID SepInitLuid(VOID);
VOID SepInitPrivileges(VOID);
BOOLEAN SepInitSecurityIDs(VOID);
BOOLEAN SepInitDACLs(VOID);
BOOLEAN SepInitSDs(VOID);

VOID SeDeassignPrimaryToken(struct _EPROCESS *Process);

NTSTATUS STDCALL
SepCreateImpersonationTokenDacl(PTOKEN Token,
                                PTOKEN PrimaryToken,
                                PACL *Dacl);

VOID SepInitializeTokenImplementation(VOID);

PTOKEN STDCALL SepCreateSystemProcessToken(VOID);

NTSTATUS SeExchangePrimaryToken(struct _EPROCESS* Process,
				PACCESS_TOKEN NewToken,
				PACCESS_TOKEN* OldTokenP);

NTSTATUS
SeCaptureLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Src,
				ULONG PrivilegeCount,
				KPROCESSOR_MODE PreviousMode,
				PLUID_AND_ATTRIBUTES AllocatedMem,
				ULONG AllocatedLength,
				POOL_TYPE PoolType,
				ULONG d,
				PLUID_AND_ATTRIBUTES* Dest,
				PULONG Length);

VOID
SeReleaseLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Privilege,
				KPROCESSOR_MODE PreviousMode,
				ULONG a);

BOOLEAN
SepPrivilegeCheck(PTOKEN Token,
		  PLUID_AND_ATTRIBUTES Privileges,
		  ULONG PrivilegeCount,
		  ULONG PrivilegeControl,
		  KPROCESSOR_MODE PreviousMode);

NTSTATUS
STDCALL
SepDuplicateToken(PTOKEN Token,
		  POBJECT_ATTRIBUTES ObjectAttributes,
		  BOOLEAN EffectiveOnly,
		  TOKEN_TYPE TokenType,
		  SECURITY_IMPERSONATION_LEVEL Level,
		  KPROCESSOR_MODE PreviousMode,
		  PTOKEN* NewAccessToken);

NTSTATUS
SepCaptureSecurityQualityOfService(IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                                   IN KPROCESSOR_MODE AccessMode,
                                   IN POOL_TYPE PoolType,
                                   IN BOOLEAN CaptureIfKernel,
                                   OUT PSECURITY_QUALITY_OF_SERVICE *CapturedSecurityQualityOfService,
                                   OUT PBOOLEAN Present);

VOID
SepReleaseSecurityQualityOfService(IN PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService  OPTIONAL,
                                   IN KPROCESSOR_MODE AccessMode,
                                   IN BOOLEAN CaptureIfKernel);

NTSTATUS
SepCaptureSid(IN PSID InputSid,
              IN KPROCESSOR_MODE AccessMode,
              IN POOL_TYPE PoolType,
              IN BOOLEAN CaptureIfKernel,
              OUT PSID *CapturedSid);

VOID
SepReleaseSid(IN PSID CapturedSid,
              IN KPROCESSOR_MODE AccessMode,
              IN BOOLEAN CaptureIfKernel);

NTSTATUS
SepCaptureAcl(IN PACL InputAcl,
              IN KPROCESSOR_MODE AccessMode,
              IN POOL_TYPE PoolType,
              IN BOOLEAN CaptureIfKernel,
              OUT PACL *CapturedAcl);

VOID
SepReleaseAcl(IN PACL CapturedAcl,
              IN KPROCESSOR_MODE AccessMode,
              IN BOOLEAN CaptureIfKernel);

#define SepAcquireTokenLockExclusive(Token)                                    \
  do {                                                                         \
    KeEnterCriticalRegion();                                                   \
    ExAcquireResourceExclusive(((PTOKEN)Token)->TokenLock, TRUE);              \
  while(0)

#define SepAcquireTokenLockShared(Token)                                       \
  do {                                                                         \
    KeEnterCriticalRegion();                                                   \
    ExAcquireResourceShared(((PTOKEN)Token)->TokenLock, TRUE);                 \
  while(0)

#define SepReleaseTokenLock(Token)                                             \
  do {                                                                         \
    ExReleaseResource(((PTOKEN)Token)->TokenLock);                             \
    KeLeaveCriticalRegion();                                                   \
  while(0)

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_SE_H */

/* EOF */
