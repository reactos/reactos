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
  _Field_size_(LogPathLength) WCHAR LogPath[1];
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
  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG IsolationLevel,
  _In_opt_ ULONG IsolationFlags,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PUNICODE_STRING Description);

typedef NTSTATUS
(NTAPI *PFN_NT_OPEN_TRANSACTION)(
  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle);

typedef NTSTATUS
(NTAPI *PFN_NT_QUERY_INFORMATION_TRANSACTION)(
  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _Out_writes_bytes_(TransactionInformationLength) PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength,
  _Out_opt_ PULONG ReturnLength);

typedef NTSTATUS
(NTAPI *PFN_NT_SET_INFORMATION_TRANSACTION)(
  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _In_ PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength);

typedef NTSTATUS
(NTAPI *PFN_NT_COMMIT_TRANSACTION)(
  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait);

typedef NTSTATUS
(NTAPI *PFN_NT_ROLLBACK_TRANSACTION)(
  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait);

#if (NTDDI_VERSION >= NTDDI_VISTA)

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransactionManager(
  _Out_ PHANDLE TmHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PUNICODE_STRING LogFileName,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG CommitStrength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransactionManager(
  _Out_ PHANDLE TmHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PUNICODE_STRING LogFileName,
  _In_opt_ LPGUID TmIdentity,
  _In_opt_ ULONG OpenOptions);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameTransactionManager(
  _In_ PUNICODE_STRING LogFileName,
  _In_ LPGUID ExistingTransactionManagerGuid);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollforwardTransactionManager(
  _In_ HANDLE TransactionManagerHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverTransactionManager(
  _In_ HANDLE TransactionManagerHandle);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransactionManager(
  _In_ HANDLE TransactionManagerHandle,
  _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  _Out_writes_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
  _In_ ULONG TransactionManagerInformationLength,
  _Out_ PULONG ReturnLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransactionManager(
  _In_opt_ HANDLE TmHandle,
  _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
  _In_reads_bytes_(TransactionManagerInformationLength) PVOID TransactionManagerInformation,
  _In_ ULONG TransactionManagerInformationLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnumerateTransactionObject(
  _In_opt_ HANDLE RootObjectHandle,
  _In_ KTMOBJECT_TYPE QueryType,
  _Inout_updates_bytes_(ObjectCursorLength) PKTMOBJECT_CURSOR ObjectCursor,
  _In_ ULONG ObjectCursorLength,
  _Out_ PULONG ReturnLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTransaction(
  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ ULONG IsolationLevel,
  _In_opt_ ULONG IsolationFlags,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PUNICODE_STRING Description);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTransaction(
  _Out_ PHANDLE TransactionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ LPGUID Uow,
  _In_opt_ HANDLE TmHandle);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _Out_writes_bytes_(TransactionInformationLength) PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength,
  _Out_opt_ PULONG ReturnLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
  _In_reads_bytes_(TransactionInformationLength) PVOID TransactionInformation,
  _In_ ULONG TransactionInformationLength);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackTransaction(
  _In_ HANDLE TransactionHandle,
  _In_ BOOLEAN Wait);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEnlistment(
  _Out_ PHANDLE EnlistmentHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE ResourceManagerHandle,
  _In_ HANDLE TransactionHandle,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ ULONG CreateOptions,
  _In_ NOTIFICATION_MASK NotificationMask,
  _In_opt_ PVOID EnlistmentKey);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEnlistment(
  _Out_ PHANDLE EnlistmentHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE ResourceManagerHandle,
  _In_ LPGUID EnlistmentGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  _Out_writes_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
  _In_ ULONG EnlistmentInformationLength,
  _Out_ PULONG ReturnLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationEnlistment(
  _In_opt_ HANDLE EnlistmentHandle,
  _In_ ENLISTMENT_INFORMATION_CLASS EnlistmentInformationClass,
  _In_reads_bytes_(EnlistmentInformationLength) PVOID EnlistmentInformation,
  _In_ ULONG EnlistmentInformationLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PVOID EnlistmentKey);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrePrepareComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrepareComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCommitComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadOnlyEnlistment(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRollbackComplete(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSinglePhaseReject(
  _In_ HANDLE EnlistmentHandle,
  _In_opt_ PLARGE_INTEGER TmVirtualClock);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateResourceManager(
  _Out_ PHANDLE ResourceManagerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE TmHandle,
  _In_ LPGUID RmGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ ULONG CreateOptions,
  _In_opt_ PUNICODE_STRING Description);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenResourceManager(
  _Out_ PHANDLE ResourceManagerHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ HANDLE TmHandle,
  _In_opt_ LPGUID ResourceManagerGuid,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRecoverResourceManager(
  _In_ HANDLE ResourceManagerHandle);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNotificationResourceManager(
  _In_ HANDLE ResourceManagerHandle,
  _Out_ PTRANSACTION_NOTIFICATION TransactionNotification,
  _In_ ULONG NotificationLength,
  _In_opt_ PLARGE_INTEGER Timeout,
  _Out_opt_ PULONG ReturnLength,
  _In_ ULONG Asynchronous,
  _In_opt_ ULONG_PTR AsynchronousContext);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationResourceManager(
  _In_ HANDLE ResourceManagerHandle,
  _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  _Out_writes_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
  _In_ ULONG ResourceManagerInformationLength,
  _Out_opt_ PULONG ReturnLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationResourceManager(
  _In_ HANDLE ResourceManagerHandle,
  _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
  _In_reads_bytes_(ResourceManagerInformationLength) PVOID ResourceManagerInformation,
  _In_ ULONG ResourceManagerInformationLength);

_Must_inspect_result_
_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterProtocolAddressInformation(
  _In_ HANDLE ResourceManager,
  _In_ PCRM_PROTOCOL_ID ProtocolId,
  _In_ ULONG ProtocolInformationSize,
  _In_ PVOID ProtocolInformation,
  _In_opt_ ULONG CreateOptions);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationComplete(
  _In_ HANDLE ResourceManagerHandle,
  _In_ ULONG RequestCookie,
  _In_ ULONG BufferLength,
  _In_ PVOID Buffer);

_IRQL_requires_max_ (APC_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPropagationFailed(
  _In_ HANDLE ResourceManagerHandle,
  _In_ ULONG RequestCookie,
  _In_ NTSTATUS PropStatus);

#endif /* NTDDI_VERSION >= NTDDI_VISTA */

#endif /* !_NTTMAPI_ */
