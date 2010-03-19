$if (_WDMDDK_)
#ifndef _NTTMAPI_
#define _NTTMAPI_

#include <ktmtypes.h>

#define TRANSACTIONMANAGER_QUERY_INFORMATION     (0x0001)
#define TRANSACTIONMANAGER_SET_INFORMATION       (0x0002)
#define TRANSACTIONMANAGER_RECOVER               (0x0004)
#define TRANSACTIONMANAGER_RENAME                (0x0008)
#define TRANSACTIONMANAGER_CREATE_RM             (0x0010)
#define TRANSACTIONMANAGER_BIND_TRANSACTION      (0x0020)

#define TRANSACTIONMANAGER_GENERIC_READ            (STANDARD_RIGHTS_READ            |\
                                                    TRANSACTIONMANAGER_QUERY_INFORMATION)

#define TRANSACTIONMANAGER_GENERIC_WRITE           (STANDARD_RIGHTS_WRITE           |\
                                                    TRANSACTIONMANAGER_SET_INFORMATION     |\
                                                    TRANSACTIONMANAGER_RECOVER             |\
                                                    TRANSACTIONMANAGER_RENAME              |\
                                                    TRANSACTIONMANAGER_CREATE_RM)

#define TRANSACTIONMANAGER_GENERIC_EXECUTE         (STANDARD_RIGHTS_EXECUTE)

#define TRANSACTIONMANAGER_ALL_ACCESS              (STANDARD_RIGHTS_REQUIRED        |\
                                                    TRANSACTIONMANAGER_GENERIC_READ        |\
                                                    TRANSACTIONMANAGER_GENERIC_WRITE       |\
                                                    TRANSACTIONMANAGER_GENERIC_EXECUTE     |\
                                                    TRANSACTIONMANAGER_BIND_TRANSACTION)

#define TRANSACTION_QUERY_INFORMATION     (0x0001)
#define TRANSACTION_SET_INFORMATION       (0x0002)
#define TRANSACTION_ENLIST                (0x0004)
#define TRANSACTION_COMMIT                (0x0008)
#define TRANSACTION_ROLLBACK              (0x0010)
#define TRANSACTION_PROPAGATE             (0x0020)
#define TRANSACTION_RIGHT_RESERVED1       (0x0040)

#define TRANSACTION_GENERIC_READ            (STANDARD_RIGHTS_READ            |\
                                             TRANSACTION_QUERY_INFORMATION   |\
                                             SYNCHRONIZE)

#define TRANSACTION_GENERIC_WRITE           (STANDARD_RIGHTS_WRITE           |\
                                             TRANSACTION_SET_INFORMATION     |\
                                             TRANSACTION_COMMIT              |\
                                             TRANSACTION_ENLIST              |\
                                             TRANSACTION_ROLLBACK            |\
                                             TRANSACTION_PROPAGATE           |\
                                             SYNCHRONIZE)

#define TRANSACTION_GENERIC_EXECUTE         (STANDARD_RIGHTS_EXECUTE         |\
                                             TRANSACTION_COMMIT              |\
                                             TRANSACTION_ROLLBACK            |\
                                             SYNCHRONIZE)

#define TRANSACTION_ALL_ACCESS              (STANDARD_RIGHTS_REQUIRED        |\
                                             TRANSACTION_GENERIC_READ        |\
                                             TRANSACTION_GENERIC_WRITE       |\
                                             TRANSACTION_GENERIC_EXECUTE)

#define TRANSACTION_RESOURCE_MANAGER_RIGHTS (TRANSACTION_GENERIC_READ        |\
                                             STANDARD_RIGHTS_WRITE           |\
                                             TRANSACTION_SET_INFORMATION     |\
                                             TRANSACTION_ENLIST              |\
                                             TRANSACTION_ROLLBACK            |\
                                             TRANSACTION_PROPAGATE           |\
                                             SYNCHRONIZE)

#define RESOURCEMANAGER_QUERY_INFORMATION        (0x0001)
#define RESOURCEMANAGER_SET_INFORMATION          (0x0002)
#define RESOURCEMANAGER_RECOVER                  (0x0004)
#define RESOURCEMANAGER_ENLIST                   (0x0008)
#define RESOURCEMANAGER_GET_NOTIFICATION         (0x0010)
#define RESOURCEMANAGER_REGISTER_PROTOCOL        (0x0020)
#define RESOURCEMANAGER_COMPLETE_PROPAGATION     (0x0040)

#define RESOURCEMANAGER_GENERIC_READ        (STANDARD_RIGHTS_READ                 |\
                                             RESOURCEMANAGER_QUERY_INFORMATION    |\
                                             SYNCHRONIZE)

#define RESOURCEMANAGER_GENERIC_WRITE       (STANDARD_RIGHTS_WRITE                |\
                                             RESOURCEMANAGER_SET_INFORMATION      |\
                                             RESOURCEMANAGER_RECOVER              |\
                                             RESOURCEMANAGER_ENLIST               |\
                                             RESOURCEMANAGER_GET_NOTIFICATION     |\
                                             RESOURCEMANAGER_REGISTER_PROTOCOL    |\
                                             RESOURCEMANAGER_COMPLETE_PROPAGATION |\
                                             SYNCHRONIZE)

#define RESOURCEMANAGER_GENERIC_EXECUTE     (STANDARD_RIGHTS_EXECUTE              |\
                                             RESOURCEMANAGER_RECOVER              |\
                                             RESOURCEMANAGER_ENLIST               |\
                                             RESOURCEMANAGER_GET_NOTIFICATION     |\
                                             RESOURCEMANAGER_COMPLETE_PROPAGATION |\
                                             SYNCHRONIZE)

#define RESOURCEMANAGER_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED             |\
                                             RESOURCEMANAGER_GENERIC_READ         |\
                                             RESOURCEMANAGER_GENERIC_WRITE        |\
                                             RESOURCEMANAGER_GENERIC_EXECUTE)

#define ENLISTMENT_QUERY_INFORMATION             (0x0001)
#define ENLISTMENT_SET_INFORMATION               (0x0002)
#define ENLISTMENT_RECOVER                       (0x0004)
#define ENLISTMENT_SUBORDINATE_RIGHTS            (0x0008)
#define ENLISTMENT_SUPERIOR_RIGHTS               (0x0010)

#define ENLISTMENT_GENERIC_READ        (STANDARD_RIGHTS_READ           |\
                                        ENLISTMENT_QUERY_INFORMATION)

#define ENLISTMENT_GENERIC_WRITE       (STANDARD_RIGHTS_WRITE          |\
                                        ENLISTMENT_SET_INFORMATION     |\
                                        ENLISTMENT_RECOVER             |\
                                        ENLISTMENT_SUBORDINATE_RIGHTS  |\
                                        ENLISTMENT_SUPERIOR_RIGHTS)

#define ENLISTMENT_GENERIC_EXECUTE     (STANDARD_RIGHTS_EXECUTE        |\
                                        ENLISTMENT_RECOVER             |\
                                        ENLISTMENT_SUBORDINATE_RIGHTS  |\
                                        ENLISTMENT_SUPERIOR_RIGHTS)

#define ENLISTMENT_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED       |\
                                        ENLISTMENT_GENERIC_READ        |\
                                        ENLISTMENT_GENERIC_WRITE       |\
                                        ENLISTMENT_GENERIC_EXECUTE)

typedef enum _TRANSACTION_OUTCOME {
  TransactionOutcomeUndetermined = 1,
  TransactionOutcomeCommitted,
  TransactionOutcomeAborted,
} TRANSACTION_OUTCOME;


typedef enum _TRANSACTION_STATE {
  TransactionStateNormal = 1,
  TransactionStateIndoubt,
  TransactionStateCommittedNotify,
} TRANSACTION_STATE;


typedef struct _TRANSACTION_BASIC_INFORMATION {
  GUID TransactionId;
  ULONG State;
  ULONG Outcome;
} TRANSACTION_BASIC_INFORMATION, *PTRANSACTION_BASIC_INFORMATION;

typedef struct _TRANSACTIONMANAGER_BASIC_INFORMATION {
  GUID TmIdentity;
  LARGE_INTEGER VirtualClock;
} TRANSACTIONMANAGER_BASIC_INFORMATION, *PTRANSACTIONMANAGER_BASIC_INFORMATION;

typedef struct _TRANSACTIONMANAGER_LOG_INFORMATION {
  GUID LogIdentity;
} TRANSACTIONMANAGER_LOG_INFORMATION, *PTRANSACTIONMANAGER_LOG_INFORMATION;

typedef struct _TRANSACTIONMANAGER_LOGPATH_INFORMATION {
  ULONG LogPathLength;
  WCHAR LogPath[1];
} TRANSACTIONMANAGER_LOGPATH_INFORMATION, *PTRANSACTIONMANAGER_LOGPATH_INFORMATION;

typedef struct _TRANSACTIONMANAGER_RECOVERY_INFORMATION {
  ULONGLONG LastRecoveredLsn;
} TRANSACTIONMANAGER_RECOVERY_INFORMATION, *PTRANSACTIONMANAGER_RECOVERY_INFORMATION;

typedef struct _TRANSACTION_PROPERTIES_INFORMATION {
  ULONG IsolationLevel;
  ULONG IsolationFlags;
  LARGE_INTEGER Timeout;
  ULONG Outcome;
  ULONG DescriptionLength;
  WCHAR Description[1];
} TRANSACTION_PROPERTIES_INFORMATION, *PTRANSACTION_PROPERTIES_INFORMATION;

typedef struct _TRANSACTION_BIND_INFORMATION {
  HANDLE TmHandle;
} TRANSACTION_BIND_INFORMATION, *PTRANSACTION_BIND_INFORMATION;

typedef struct _TRANSACTION_ENLISTMENT_PAIR {
  GUID EnlistmentId;
  GUID ResourceManagerId;
} TRANSACTION_ENLISTMENT_PAIR, *PTRANSACTION_ENLISTMENT_PAIR;

typedef struct _TRANSACTION_ENLISTMENTS_INFORMATION {
  ULONG NumberOfEnlistments;
  TRANSACTION_ENLISTMENT_PAIR EnlistmentPair[1];
} TRANSACTION_ENLISTMENTS_INFORMATION, *PTRANSACTION_ENLISTMENTS_INFORMATION;

typedef struct _TRANSACTION_SUPERIOR_ENLISTMENT_INFORMATION {
  TRANSACTION_ENLISTMENT_PAIR SuperiorEnlistmentPair;
} TRANSACTION_SUPERIOR_ENLISTMENT_INFORMATION, *PTRANSACTION_SUPERIOR_ENLISTMENT_INFORMATION;

typedef struct _RESOURCEMANAGER_BASIC_INFORMATION {
  GUID ResourceManagerId;
  ULONG DescriptionLength;
  WCHAR Description[1];
} RESOURCEMANAGER_BASIC_INFORMATION, *PRESOURCEMANAGER_BASIC_INFORMATION;

typedef struct _RESOURCEMANAGER_COMPLETION_INFORMATION {
  HANDLE IoCompletionPortHandle;
  ULONG_PTR CompletionKey;
} RESOURCEMANAGER_COMPLETION_INFORMATION, *PRESOURCEMANAGER_COMPLETION_INFORMATION;

typedef enum _KTMOBJECT_TYPE {
  KTMOBJECT_TRANSACTION,
  KTMOBJECT_TRANSACTION_MANAGER,
  KTMOBJECT_RESOURCE_MANAGER,
  KTMOBJECT_ENLISTMENT,
  KTMOBJECT_INVALID
} KTMOBJECT_TYPE, *PKTMOBJECT_TYPE;

typedef struct _KTMOBJECT_CURSOR {
  GUID LastQuery;
  ULONG ObjectIdCount;
  GUID ObjectIds[1];
} KTMOBJECT_CURSOR, *PKTMOBJECT_CURSOR;

typedef enum _TRANSACTION_INFORMATION_CLASS {
  TransactionBasicInformation,
  TransactionPropertiesInformation,
  TransactionEnlistmentInformation,
  TransactionSuperiorEnlistmentInformation
} TRANSACTION_INFORMATION_CLASS;

typedef enum _TRANSACTIONMANAGER_INFORMATION_CLASS {
  TransactionManagerBasicInformation,
  TransactionManagerLogInformation,
  TransactionManagerLogPathInformation,
  TransactionManagerRecoveryInformation = 4
} TRANSACTIONMANAGER_INFORMATION_CLASS;

typedef enum _RESOURCEMANAGER_INFORMATION_CLASS {
  ResourceManagerBasicInformation,
  ResourceManagerCompletionInformation,
} RESOURCEMANAGER_INFORMATION_CLASS;

typedef struct _ENLISTMENT_BASIC_INFORMATION {
  GUID EnlistmentId;
  GUID TransactionId;
  GUID ResourceManagerId;
} ENLISTMENT_BASIC_INFORMATION, *PENLISTMENT_BASIC_INFORMATION;

typedef struct _ENLISTMENT_CRM_INFORMATION {
  GUID CrmTransactionManagerId;
  GUID CrmResourceManagerId;
  GUID CrmEnlistmentId;
} ENLISTMENT_CRM_INFORMATION, *PENLISTMENT_CRM_INFORMATION;

typedef enum _ENLISTMENT_INFORMATION_CLASS {
  EnlistmentBasicInformation,
  EnlistmentRecoveryInformation,
  EnlistmentCrmInformation
} ENLISTMENT_INFORMATION_CLASS;

typedef struct _TRANSACTION_LIST_ENTRY {
#if defined(__cplusplus)
  ::UOW UOW;
#else
  UOW UOW;
#endif
} TRANSACTION_LIST_ENTRY, *PTRANSACTION_LIST_ENTRY;

typedef struct _TRANSACTION_LIST_INFORMATION {
  ULONG NumberOfTransactions;
  TRANSACTION_LIST_ENTRY TransactionInformation[1];
} TRANSACTION_LIST_INFORMATION, *PTRANSACTION_LIST_INFORMATION;

typedef NTSTATUS
(NTAPI *PFN_NT_CREATE_TRANSACTION)(
  OUT PHANDLE TransactionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN LPGUID Uow OPTIONAL,
  IN HANDLE TmHandle OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN ULONG IsolationLevel OPTIONAL,
  IN ULONG IsolationFlags OPTIONAL,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  IN PUNICODE_STRING Description OPTIONAL);

typedef NTSTATUS
(NTAPI *PFN_NT_OPEN_TRANSACTION)(
  OUT PHANDLE TransactionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN LPGUID Uow OPTIONAL,
  IN HANDLE TmHandle OPTIONAL);

typedef NTSTATUS
(NTAPI *PFN_NT_QUERY_INFORMATION_TRANSACTION)(
  IN HANDLE TransactionHandle,
  IN TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  OUT PVOID TransactionInformation,
  IN ULONG TransactionInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

typedef NTSTATUS
(NTAPI *PFN_NT_SET_INFORMATION_TRANSACTION)(
  IN HANDLE TransactionHandle,
  IN TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  IN PVOID TransactionInformation,
  IN ULONG TransactionInformationLength);

typedef NTSTATUS
(NTAPI *PFN_NT_COMMIT_TRANSACTION)(
  IN HANDLE TransactionHandle,
  IN BOOLEAN Wait);

typedef NTSTATUS
(NTAPI *PFN_NT_ROLLBACK_TRANSACTION)(
  IN HANDLE TransactionHandle,
  IN BOOLEAN Wait);

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransactionManager(
  OUT PHANDLE TmHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PUNICODE_STRING LogFileName OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN ULONG CommitStrength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransactionManager(
  OUT PHANDLE TmHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PUNICODE_STRING LogFileName OPTIONAL,
  IN LPGUID TmIdentity OPTIONAL,
  IN ULONG OpenOptions OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameTransactionManager(
  IN PUNICODE_STRING LogFileName,
  IN LPGUID ExistingTransactionManagerGuid);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollforwardTransactionManager(
  IN HANDLE TransactionManagerHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverTransactionManager(
  IN HANDLE TransactionManagerHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransactionManager(
  IN HANDLE TransactionManagerHandle,
  IN TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  OUT PVOID TransactionManagerInformation,
  IN ULONG TransactionManagerInformationLength,
  OUT PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransactionManager(
  IN HANDLE TmHandle OPTIONAL,
  IN TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  IN PVOID TransactionManagerInformation,
  IN ULONG TransactionManagerInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateTransactionObject(
  IN HANDLE RootObjectHandle OPTIONAL,
  IN KTMOBJECT_TYPE QueryType,
  IN OUT PKTMOBJECT_CURSOR ObjectCursor,
  IN ULONG ObjectCursorLength,
  OUT PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransaction(
  OUT PHANDLE TransactionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN LPGUID Uow OPTIONAL,
  IN HANDLE TmHandle OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN ULONG IsolationLevel OPTIONAL,
  IN ULONG IsolationFlags OPTIONAL,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  IN PUNICODE_STRING Description OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransaction(
  OUT PHANDLE TransactionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN LPGUID Uow,
  IN HANDLE TmHandle OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransaction(
  IN HANDLE TransactionHandle,
  IN TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  OUT PVOID TransactionInformation,
  IN ULONG TransactionInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransaction(
  IN HANDLE TransactionHandle,
  IN TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  IN PVOID TransactionInformation,
  IN ULONG TransactionInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitTransaction(
  IN HANDLE TransactionHandle,
  IN BOOLEAN Wait);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackTransaction(
  IN HANDLE TransactionHandle,
  IN BOOLEAN Wait);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEnlistment(
  OUT PHANDLE EnlistmentHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE ResourceManagerHandle,
  IN HANDLE TransactionHandle,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN NOTIFICATION_MASK NotificationMask,
  IN PVOID EnlistmentKey OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEnlistment(
  OUT PHANDLE EnlistmentHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE ResourceManagerHandle,
  IN LPGUID EnlistmentGuid,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationEnlistment(
  IN HANDLE EnlistmentHandle,
  IN ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  OUT PVOID EnlistmentInformation,
  IN ULONG EnlistmentInformationLength,
  OUT PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationEnlistment(
  IN HANDLE EnlistmentHandle OPTIONAL,
  IN ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  IN PVOID EnlistmentInformation,
  IN ULONG EnlistmentInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PVOID EnlistmentKey OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadOnlyEnlistment(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackComplete(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSinglePhaseReject(
  IN HANDLE EnlistmentHandle,
  IN PLARGE_INTEGER TmVirtualClock OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateResourceManager(
  OUT PHANDLE ResourceManagerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE TmHandle,
  IN LPGUID RmGuid,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN ULONG CreateOptions OPTIONAL,
  IN PUNICODE_STRING Description OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenResourceManager(
  OUT PHANDLE ResourceManagerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN HANDLE TmHandle,
  IN LPGUID ResourceManagerGuid OPTIONAL,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverResourceManager(
  IN HANDLE ResourceManagerHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNotificationResourceManager(
  IN HANDLE ResourceManagerHandle,
  OUT PTRANSACTION_NOTIFICATION TransactionNotification,
  IN ULONG NotificationLength,
  IN PLARGE_INTEGER Timeout OPTIONAL,
  OUT PULONG ReturnLength OPTIONAL,
  IN ULONG Asynchronous,
  IN ULONG_PTR AsynchronousContext OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationResourceManager(
  IN HANDLE ResourceManagerHandle,
  IN RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  OUT PVOID ResourceManagerInformation,
  IN ULONG ResourceManagerInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationResourceManager(
  IN HANDLE ResourceManagerHandle,
  IN RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  IN PVOID ResourceManagerInformation,
  IN ULONG ResourceManagerInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterProtocolAddressInformation(
  IN HANDLE ResourceManager,
  IN PCRM_PROTOCOL_ID ProtocolId,
  IN ULONG ProtocolInformationSize,
  IN PVOID ProtocolInformation,
  IN ULONG CreateOptions OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationComplete(
  IN HANDLE ResourceManagerHandle,
  IN ULONG RequestCookie,
  IN ULONG BufferLength,
  IN PVOID Buffer);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationFailed(
  IN HANDLE ResourceManagerHandle,
  IN ULONG RequestCookie,
  IN NTSTATUS PropStatus);

#endif /* NTDDI_VERSION >= NTDDI_VISTA */

#endif /* !_NTTMAPI_ */
$endif
