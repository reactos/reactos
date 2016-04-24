/******************************************************************************
 *                            Security Manager Functions                      *
 ******************************************************************************/

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
SeAccessCheck(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
  _In_ BOOLEAN SubjectContextLocked,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ACCESS_MASK PreviouslyGrantedAccess,
  _Outptr_opt_ PPRIVILEGE_SET *Privileges,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PACCESS_MASK GrantedAccess,
  _Out_ PNTSTATUS AccessStatus);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurity(
  _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
  _In_opt_ PSECURITY_DESCRIPTOR ExplicitDescriptor,
  _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
  _In_ BOOLEAN IsDirectoryObject,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ POOL_TYPE PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurityEx(
  _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
  _In_opt_ PSECURITY_DESCRIPTOR ExplicitDescriptor,
  _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
  _In_opt_ GUID *ObjectType,
  _In_ BOOLEAN IsDirectoryObject,
  _In_ ULONG AutoInheritFlags,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
SeDeassignSecurity(
  _Inout_ PSECURITY_DESCRIPTOR *SecurityDescriptor);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
SeValidSecurityDescriptor(
  _In_ ULONG Length,
  _In_reads_bytes_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
ULONG
NTAPI
SeObjectCreateSaclAccessBits(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext(
  _Inout_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeUnlockSubjectContext(
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeCaptureSubjectContext(
  _Out_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeLockSubjectContext(
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext);
$endif (_WDMDDK_)

$if (_NTDDK_)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  _In_ LUID PrivilegeValue,
  _In_ KPROCESSOR_MODE PreviousMode);
$endif (_NTDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext(
  _Inout_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
BOOLEAN
NTAPI
SePrivilegeCheck(
  _Inout_ PPRIVILEGE_SET RequiredPrivileges,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_ KPROCESSOR_MODE AccessMode);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarm(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarm(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarm(
  _In_ PVOID Object,
  _In_ HANDLE Handle);

NTKERNELAPI
TOKEN_TYPE
NTAPI
SeTokenType(
  _In_ PACCESS_TOKEN Token);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsAdmin(
  _In_ PACCESS_TOKEN Token);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsRestricted(
  _In_ PACCESS_TOKEN Token);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryAuthenticationIdToken(
  _In_ PACCESS_TOKEN Token,
  _Out_ PLUID AuthenticationId);

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySessionIdToken(
  _In_ PACCESS_TOKEN Token,
  _Out_ PULONG SessionId);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurity(
  _In_ PETHREAD ClientThread,
  _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
  _In_ BOOLEAN RemoteSession,
  _Out_ PSECURITY_CLIENT_CONTEXT ClientContext);

NTKERNELAPI
VOID
NTAPI
SeImpersonateClient(
  _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
  _In_opt_ PETHREAD ServerThread);

NTKERNELAPI
NTSTATUS
NTAPI
SeImpersonateClientEx(
  _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
  _In_opt_ PETHREAD ServerThread);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
  _In_ BOOLEAN ServerIsRemote,
  _Out_ PSECURITY_CLIENT_CONTEXT ClientContext);

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySecurityDescriptorInfo(
  _In_ PSECURITY_INFORMATION SecurityInformation,
  _Out_writes_bytes_(*Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Inout_ PULONG Length,
  _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfo(
  _In_opt_ PVOID Object,
  _In_ PSECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
  _In_ POOL_TYPE PoolType,
  _In_ PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfoEx(
  _In_opt_ PVOID Object,
  _In_ PSECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
  _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
  _In_ ULONG AutoInheritFlags,
  _In_ POOL_TYPE PoolType,
  _In_ PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeAppendPrivileges(
  _Inout_ PACCESS_STATE AccessState,
  _In_ PPRIVILEGE_SET Privileges);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEvents(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileOrGlobalEvents(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

VOID
NTAPI
SeSetAccessStateGenericMapping(
  _Inout_ PACCESS_STATE AccessState,
  _In_ PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(
  _In_ PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(
  _In_ PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(
  _In_ PLUID LogonId);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryInformationToken(
  _In_ PACCESS_TOKEN Token,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Outptr_result_buffer_(_Inexpressible_(token-dependent)) PVOID *TokenInformation);
$endif (_NTIFS_)

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
$if (_NTIFS_)
#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingHardLinkEvents(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
SeFilterToken(
  _In_ PACCESS_TOKEN ExistingToken,
  _In_ ULONG Flags,
  _In_opt_ PTOKEN_GROUPS SidsToDisable,
  _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
  _In_opt_ PTOKEN_GROUPS RestrictedSids,
  _Outptr_ PACCESS_TOKEN *FilteredToken);

NTKERNELAPI
VOID
NTAPI
SeAuditHardLinkCreation(
  _In_ PUNICODE_STRING FileName,
  _In_ PUNICODE_STRING LinkName,
  _In_ BOOLEAN bSuccess);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEventsWithContext(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingHardLinkEventsWithContext(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

#endif
$endif (_NTIFS_)

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03SP1)

_At_(AuditParameters->ParameterCount, _Const_)
NTSTATUS
NTAPI
SeSetAuditParameter(
  _Inout_ PSE_ADT_PARAMETER_ARRAY AuditParameters,
  _In_ SE_ADT_PARAMETER_TYPE Type,
  _In_range_(<,SE_MAX_AUDIT_PARAMETERS) ULONG Index,
  _In_reads_(_Inexpressible_("depends on SE_ADT_PARAMETER_TYPE"))
    PVOID Data);

NTSTATUS
NTAPI
SeReportSecurityEvent(
  _In_ ULONG Flags,
  _In_ PUNICODE_STRING SourceName,
  _In_opt_ PSID UserSid,
  _In_ PSE_ADT_PARAMETER_ARRAY AuditParameters);

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
  _In_ PVOID ObjectType,
  _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_DESCRIPTOR ParentSecurityDescriptor);

#ifdef SE_NTFS_WORLD_CACHE
VOID
NTAPI
SeGetWorldRights(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PGENERIC_MAPPING GenericMapping,
  _Out_ PACCESS_MASK GrantedAccess);
#endif /* SE_NTFS_WORLD_CACHE */
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarmWithTransaction(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_opt_ GUID *TransactionId,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarmWithTransaction(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_opt_ GUID *TransactionId,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeExamineSacl(
  _In_ PACL Sacl,
  _In_ PACCESS_TOKEN Token,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN AccessGranted,
  _Out_ PBOOLEAN GenerateAudit,
  _Out_ PBOOLEAN GenerateAlarm);

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarmWithTransaction(
  _In_ PVOID Object,
  _In_ HANDLE Handle,
  _In_opt_ GUID *TransactionId);

NTKERNELAPI
VOID
NTAPI
SeQueryTokenIntegrity(
  _In_ PACCESS_TOKEN Token,
  _Inout_ PSID_AND_ATTRIBUTES IntegritySA);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSessionIdToken(
  _In_ PACCESS_TOKEN Token,
  _In_ ULONG SessionId);

NTKERNELAPI
VOID
NTAPI
SeAuditHardLinkCreationWithTransaction(
  _In_ PUNICODE_STRING FileName,
  _In_ PUNICODE_STRING LinkName,
  _In_ BOOLEAN bSuccess,
  _In_opt_ GUID *TransactionId);

NTKERNELAPI
VOID
NTAPI
SeAuditTransactionStateChange(
  _In_ GUID *TransactionId,
  _In_ GUID *ResourceManagerId,
  _In_ ULONG NewTransactionState);
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
  _In_ PACCESS_TOKEN Token);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingAnyFileEventsWithContext(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
  _Out_opt_ PBOOLEAN StagingEnabled);

NTKERNELAPI
VOID
NTAPI
SeExamineGlobalSacl(
  _In_ PUNICODE_STRING ObjectType,
  _In_ PACL ResourceSacl,
  _In_ PACCESS_TOKEN Token,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN AccessGranted,
  _Inout_ PBOOLEAN GenerateAudit,
  _Inout_opt_ PBOOLEAN GenerateAlarm);

NTKERNELAPI
VOID
NTAPI
SeMaximumAuditMaskFromGlobalSacl(
  _In_opt_ PUNICODE_STRING ObjectTypeName,
  _In_ ACCESS_MASK GrantedAccess,
  _In_ PACCESS_TOKEN Token,
  _Inout_ PACCESS_MASK AuditMask);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

NTSTATUS
NTAPI
SeReportSecurityEventWithSubCategory(
  _In_ ULONG Flags,
  _In_ PUNICODE_STRING SourceName,
  _In_opt_ PSID UserSid,
  _In_ PSE_ADT_PARAMETER_ARRAY AuditParameters,
  _In_ ULONG AuditSubcategoryId);

BOOLEAN
NTAPI
SeAccessCheckFromState(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PTOKEN_ACCESS_INFORMATION PrimaryTokenInformation,
  _In_opt_ PTOKEN_ACCESS_INFORMATION ClientTokenInformation,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ACCESS_MASK PreviouslyGrantedAccess,
  _Outptr_opt_result_maybenull_ PPRIVILEGE_SET *Privileges,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PACCESS_MASK GrantedAccess,
  _Out_ PNTSTATUS AccessStatus);

NTKERNELAPI
VOID
NTAPI
SeFreePrivileges(
  _In_ PPRIVILEGE_SET Privileges);

NTSTATUS
NTAPI
SeLocateProcessImageName(
  _Inout_ PEPROCESS Process,
  _Outptr_ PUNICODE_STRING *pImageFileName);

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
