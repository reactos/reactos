/******************************************************************************
 *                            Security Manager Functions                      *
 ******************************************************************************/

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
SeAccessCheck(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
  IN BOOLEAN SubjectContextLocked,
  IN ACCESS_MASK DesiredAccess,
  IN ACCESS_MASK PreviouslyGrantedAccess,
  OUT PPRIVILEGE_SET *Privileges OPTIONAL,
  IN PGENERIC_MAPPING GenericMapping,
  IN KPROCESSOR_MODE AccessMode,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurity(
  IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
  IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
  OUT PSECURITY_DESCRIPTOR *NewDescriptor,
  IN BOOLEAN IsDirectoryObject,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
  IN PGENERIC_MAPPING GenericMapping,
  IN POOL_TYPE PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurityEx(
  IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
  IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
  OUT PSECURITY_DESCRIPTOR *NewDescriptor,
  IN GUID *ObjectType OPTIONAL,
  IN BOOLEAN IsDirectoryObject,
  IN ULONG AutoInheritFlags,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
  IN PGENERIC_MAPPING GenericMapping,
  IN POOL_TYPE PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeDeassignSecurity(
  IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor);

NTKERNELAPI
BOOLEAN
NTAPI
SeValidSecurityDescriptor(
  IN ULONG Length,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
ULONG
NTAPI
SeObjectCreateSaclAccessBits(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext(
  IN OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeUnlockSubjectContext(
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeCaptureSubjectContext(
  OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeLockSubjectContext(
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext);
$endif (_WDMDDK_)

$if (_NTDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  IN LUID PrivilegeValue,
  IN KPROCESSOR_MODE PreviousMode);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext(
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
BOOLEAN
NTAPI
SePrivilegeCheck(
  IN OUT PPRIVILEGE_SET RequiredPrivileges,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
  IN KPROCESSOR_MODE AccessMode);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarm(
  IN PUNICODE_STRING ObjectTypeName,
  IN PVOID Object OPTIONAL,
  IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PACCESS_STATE AccessState,
  IN BOOLEAN ObjectCreated,
  IN BOOLEAN AccessGranted,
  IN KPROCESSOR_MODE AccessMode,
  OUT PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarm(
  IN PUNICODE_STRING ObjectTypeName,
  IN PVOID Object OPTIONAL,
  IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PACCESS_STATE AccessState,
  IN BOOLEAN ObjectCreated,
  IN BOOLEAN AccessGranted,
  IN KPROCESSOR_MODE AccessMode,
  OUT PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarm(
  IN PVOID Object,
  IN HANDLE Handle);

NTKERNELAPI
TOKEN_TYPE
NTAPI
SeTokenType(
  IN PACCESS_TOKEN Token);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsAdmin(
  IN PACCESS_TOKEN Token);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsRestricted(
  IN PACCESS_TOKEN Token);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryAuthenticationIdToken(
  IN PACCESS_TOKEN Token,
  OUT PLUID AuthenticationId);

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySessionIdToken(
  IN PACCESS_TOKEN Token,
  OUT PULONG SessionId);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurity(
  IN PETHREAD ClientThread,
  IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
  IN BOOLEAN RemoteSession,
  OUT PSECURITY_CLIENT_CONTEXT ClientContext);

NTKERNELAPI
VOID
NTAPI
SeImpersonateClient(
  IN PSECURITY_CLIENT_CONTEXT ClientContext,
  IN PETHREAD ServerThread OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
SeImpersonateClientEx(
  IN PSECURITY_CLIENT_CONTEXT ClientContext,
  IN PETHREAD ServerThread OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
  IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
  IN BOOLEAN ServerIsRemote,
  OUT PSECURITY_CLIENT_CONTEXT ClientContext);

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySecurityDescriptorInfo(
  IN PSECURITY_INFORMATION SecurityInformation,
  OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN OUT PULONG Length,
  IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfo(
  IN PVOID Object OPTIONAL,
  IN PSECURITY_INFORMATION SecurityInformation,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
  IN POOL_TYPE PoolType,
  IN PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfoEx(
  IN PVOID Object OPTIONAL,
  IN PSECURITY_INFORMATION SecurityInformation,
  IN PSECURITY_DESCRIPTOR ModificationDescriptor,
  IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
  IN ULONG AutoInheritFlags,
  IN POOL_TYPE PoolType,
  IN PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeAppendPrivileges(
  IN OUT PACCESS_STATE AccessState,
  IN PPRIVILEGE_SET Privileges);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEvents(
  IN BOOLEAN AccessGranted,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileOrGlobalEvents(
  IN BOOLEAN AccessGranted,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

VOID
NTAPI
SeSetAccessStateGenericMapping(
  IN OUT PACCESS_STATE AccessState,
  IN PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(
  IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(
  IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(
  IN PLUID LogonId);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryInformationToken(
  IN PACCESS_TOKEN Token,
  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
  OUT PVOID *TokenInformation);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
$if (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingHardLinkEvents(
  IN BOOLEAN AccessGranted,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
SeFilterToken(
  IN PACCESS_TOKEN ExistingToken,
  IN ULONG Flags,
  IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
  IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
  IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
  OUT PACCESS_TOKEN *FilteredToken);

NTKERNELAPI
VOID
NTAPI
SeAuditHardLinkCreation(
  IN PUNICODE_STRING FileName,
  IN PUNICODE_STRING LinkName,
  IN BOOLEAN bSuccess);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEventsWithContext(
  IN BOOLEAN AccessGranted,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingHardLinkEventsWithContext(
  IN BOOLEAN AccessGranted,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL);

#endif
$endif (_NTIFS_)

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03SP1)

NTSTATUS
NTAPI
SeSetAuditParameter(
  IN OUT PSE_ADT_PARAMETER_ARRAY AuditParameters,
  IN SE_ADT_PARAMETER_TYPE Type,
  IN ULONG Index,
  IN PVOID Data);

NTSTATUS
NTAPI
SeReportSecurityEvent(
  IN ULONG Flags,
  IN PUNICODE_STRING SourceName,
  IN PSID UserSid OPTIONAL,
  IN PSE_ADT_PARAMETER_ARRAY AuditParameters);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */
$endif (_WDMDDK_)

$if (_WDMDDK_ || _NTIFS_)
#if (NTDDI_VERSION >= NTDDI_VISTA)
$endif (_WDMDDK_ || _NTIFS_)
$if (_WDMDDK_)
NTKERNELAPI
ULONG
NTAPI
SeComputeAutoInheritByObjectType(
  IN PVOID ObjectType,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
  IN PSECURITY_DESCRIPTOR ParentSecurityDescriptor OPTIONAL);

#ifdef SE_NTFS_WORLD_CACHE
VOID
NTAPI
SeGetWorldRights(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PGENERIC_MAPPING GenericMapping,
  OUT PACCESS_MASK GrantedAccess);
#endif /* SE_NTFS_WORLD_CACHE */
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarmWithTransaction(
  IN PUNICODE_STRING ObjectTypeName,
  IN PVOID Object OPTIONAL,
  IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PACCESS_STATE AccessState,
  IN BOOLEAN ObjectCreated,
  IN BOOLEAN AccessGranted,
  IN KPROCESSOR_MODE AccessMode,
  IN GUID *TransactionId OPTIONAL,
  OUT PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarmWithTransaction(
  IN PUNICODE_STRING ObjectTypeName,
  IN PVOID Object OPTIONAL,
  IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PACCESS_STATE AccessState,
  IN BOOLEAN ObjectCreated,
  IN BOOLEAN AccessGranted,
  IN KPROCESSOR_MODE AccessMode,
  IN GUID *TransactionId OPTIONAL,
  OUT PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeExamineSacl(
  IN PACL Sacl,
  IN PACCESS_TOKEN Token,
  IN ACCESS_MASK DesiredAccess,
  IN BOOLEAN AccessGranted,
  OUT PBOOLEAN GenerateAudit,
  OUT PBOOLEAN GenerateAlarm);

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarmWithTransaction(
  IN PVOID Object,
  IN HANDLE Handle,
  IN GUID *TransactionId OPTIONAL);

NTKERNELAPI
VOID
NTAPI
SeQueryTokenIntegrity(
  IN PACCESS_TOKEN Token,
  IN OUT PSID_AND_ATTRIBUTES IntegritySA);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSessionIdToken(
  IN PACCESS_TOKEN Token,
  IN ULONG SessionId);

NTKERNELAPI
VOID
NTAPI
SeAuditHardLinkCreationWithTransaction(
  IN PUNICODE_STRING FileName,
  IN PUNICODE_STRING LinkName,
  IN BOOLEAN bSuccess,
  IN GUID *TransactionId OPTIONAL);

NTKERNELAPI
VOID
NTAPI
SeAuditTransactionStateChange(
  IN GUID *TransactionId,
  IN GUID *ResourceManagerId,
  IN ULONG NewTransactionState);
$endif (_NTIFS_)
$if (_WDMDDK_ || _NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */
$endif (_WDMDDK_ || _NTIFS_)
$if (_NTIFS_)

#if (NTDDI_VERSION >= NTDDI_VISTA || (NTDDI_VERSION >= NTDDI_WINXPSP2 && NTDDI_VERSION < NTDDI_WS03))
NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsWriteRestricted(
  IN PACCESS_TOKEN Token);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingAnyFileEventsWithContext(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL);

NTKERNELAPI
VOID
NTAPI
SeExamineGlobalSacl(
  IN PUNICODE_STRING ObjectType,
  IN PACCESS_TOKEN Token,
  IN ACCESS_MASK DesiredAccess,
  IN BOOLEAN AccessGranted,
  IN OUT PBOOLEAN GenerateAudit,
  IN OUT PBOOLEAN GenerateAlarm OPTIONAL);

NTKERNELAPI
VOID
NTAPI
SeMaximumAuditMaskFromGlobalSacl(
  IN PUNICODE_STRING ObjectTypeName OPTIONAL,
  IN ACCESS_MASK GrantedAccess,
  IN PACCESS_TOKEN Token,
  IN OUT PACCESS_MASK AuditMask);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

NTSTATUS
NTAPI
SeReportSecurityEventWithSubCategory(
  IN ULONG Flags,
  IN PUNICODE_STRING SourceName,
  IN PSID UserSid OPTIONAL,
  IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
  IN ULONG AuditSubcategoryId);

BOOLEAN
NTAPI
SeAccessCheckFromState(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PTOKEN_ACCESS_INFORMATION PrimaryTokenInformation,
  IN PTOKEN_ACCESS_INFORMATION ClientTokenInformation OPTIONAL,
  IN ACCESS_MASK DesiredAccess,
  IN ACCESS_MASK PreviouslyGrantedAccess,
  OUT PPRIVILEGE_SET *Privileges OPTIONAL,
  IN PGENERIC_MAPPING GenericMapping,
  IN KPROCESSOR_MODE AccessMode,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus);

NTKERNELAPI
VOID
NTAPI
SeFreePrivileges(
  IN PPRIVILEGE_SET Privileges);

NTSTATUS
NTAPI
SeLocateProcessImageName(
  IN OUT PEPROCESS Process,
  OUT PUNICODE_STRING *pImageFileName);

#define SeLengthSid( Sid ) \
    (8 + (4 * ((SID *)Sid)->SubAuthorityCount))

#define SeDeleteClientSecurity(C)  {                                           \
            if (SeTokenType((C)->ClientToken) == TokenPrimary) {               \
                PsDereferencePrimaryToken( (C)->ClientToken );                 \
            } else {                                                           \
                PsDereferenceImpersonationToken( (C)->ClientToken );           \
            }                                                                  \
}

#define SeStopImpersonatingClient() PsRevertToSelf()

#define SeQuerySubjectContextToken( SubjectContext )                \
    ( ARGUMENT_PRESENT(                                             \
        ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken   \
        ) ?                                                         \
    ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken :     \
    ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->PrimaryToken )

extern NTKERNELAPI PSE_EXPORTS SeExports;
$endif (_NTIFS_)
