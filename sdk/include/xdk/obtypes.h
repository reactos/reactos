/******************************************************************************
 *                            Object Manager Types                            *
 ******************************************************************************/

$if (_WDMDDK_)
#define MAXIMUM_FILENAME_LENGTH           256
#define OBJ_NAME_PATH_SEPARATOR           ((WCHAR)L'\\')

#define OBJECT_TYPE_CREATE                0x0001
#define OBJECT_TYPE_ALL_ACCESS            (STANDARD_RIGHTS_REQUIRED | 0x1)

#define DIRECTORY_QUERY                   0x0001
#define DIRECTORY_TRAVERSE                0x0002
#define DIRECTORY_CREATE_OBJECT           0x0004
#define DIRECTORY_CREATE_SUBDIRECTORY     0x0008
#define DIRECTORY_ALL_ACCESS              (STANDARD_RIGHTS_REQUIRED | 0xF)

#define SYMBOLIC_LINK_QUERY               0x0001
#define SYMBOLIC_LINK_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED | 0x1)

#define DUPLICATE_CLOSE_SOURCE            0x00000001
#define DUPLICATE_SAME_ACCESS             0x00000002
#define DUPLICATE_SAME_ATTRIBUTES         0x00000004

#define OB_FLT_REGISTRATION_VERSION_0100  0x0100
#define OB_FLT_REGISTRATION_VERSION       OB_FLT_REGISTRATION_VERSION_0100

typedef ULONG OB_OPERATION;

#define OB_OPERATION_HANDLE_CREATE        0x00000001
#define OB_OPERATION_HANDLE_DUPLICATE     0x00000002

typedef struct _OB_PRE_CREATE_HANDLE_INFORMATION {
  _Inout_ ACCESS_MASK DesiredAccess;
  _In_ ACCESS_MASK OriginalDesiredAccess;
} OB_PRE_CREATE_HANDLE_INFORMATION, *POB_PRE_CREATE_HANDLE_INFORMATION;

typedef struct _OB_PRE_DUPLICATE_HANDLE_INFORMATION {
  _Inout_ ACCESS_MASK DesiredAccess;
  _In_ ACCESS_MASK OriginalDesiredAccess;
  _In_ PVOID SourceProcess;
  _In_ PVOID TargetProcess;
} OB_PRE_DUPLICATE_HANDLE_INFORMATION, *POB_PRE_DUPLICATE_HANDLE_INFORMATION;

typedef union _OB_PRE_OPERATION_PARAMETERS {
  _Inout_ OB_PRE_CREATE_HANDLE_INFORMATION CreateHandleInformation;
  _Inout_ OB_PRE_DUPLICATE_HANDLE_INFORMATION DuplicateHandleInformation;
} OB_PRE_OPERATION_PARAMETERS, *POB_PRE_OPERATION_PARAMETERS;

typedef struct _OB_PRE_OPERATION_INFORMATION {
  _In_ OB_OPERATION Operation;
  _ANONYMOUS_UNION union {
    _In_ ULONG Flags;
    _ANONYMOUS_STRUCT struct {
      _In_ ULONG KernelHandle:1;
      _In_ ULONG Reserved:31;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  _In_ PVOID Object;
  _In_ POBJECT_TYPE ObjectType;
  _Out_ PVOID CallContext;
  _In_ POB_PRE_OPERATION_PARAMETERS Parameters;
} OB_PRE_OPERATION_INFORMATION, *POB_PRE_OPERATION_INFORMATION;

typedef struct _OB_POST_CREATE_HANDLE_INFORMATION {
  _In_ ACCESS_MASK GrantedAccess;
} OB_POST_CREATE_HANDLE_INFORMATION, *POB_POST_CREATE_HANDLE_INFORMATION;

typedef struct _OB_POST_DUPLICATE_HANDLE_INFORMATION {
  _In_ ACCESS_MASK GrantedAccess;
} OB_POST_DUPLICATE_HANDLE_INFORMATION, *POB_POST_DUPLICATE_HANDLE_INFORMATION;

typedef union _OB_POST_OPERATION_PARAMETERS {
  _In_ OB_POST_CREATE_HANDLE_INFORMATION CreateHandleInformation;
  _In_ OB_POST_DUPLICATE_HANDLE_INFORMATION DuplicateHandleInformation;
} OB_POST_OPERATION_PARAMETERS, *POB_POST_OPERATION_PARAMETERS;

typedef struct _OB_POST_OPERATION_INFORMATION {
  _In_ OB_OPERATION Operation;
  _ANONYMOUS_UNION union {
    _In_ ULONG Flags;
    _ANONYMOUS_STRUCT struct {
      _In_ ULONG KernelHandle:1;
      _In_ ULONG Reserved:31;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  _In_ PVOID Object;
  _In_ POBJECT_TYPE ObjectType;
  _In_ PVOID CallContext;
  _In_ NTSTATUS ReturnStatus;
  _In_ POB_POST_OPERATION_PARAMETERS Parameters;
} OB_POST_OPERATION_INFORMATION,*POB_POST_OPERATION_INFORMATION;

typedef enum _OB_PREOP_CALLBACK_STATUS {
  OB_PREOP_SUCCESS
} OB_PREOP_CALLBACK_STATUS, *POB_PREOP_CALLBACK_STATUS;

typedef OB_PREOP_CALLBACK_STATUS
(NTAPI *POB_PRE_OPERATION_CALLBACK)(
  _In_ PVOID RegistrationContext,
  _Inout_ POB_PRE_OPERATION_INFORMATION OperationInformation);

typedef VOID
(NTAPI *POB_POST_OPERATION_CALLBACK)(
  _In_ PVOID RegistrationContext,
  _In_ POB_POST_OPERATION_INFORMATION OperationInformation);

typedef struct _OB_OPERATION_REGISTRATION {
  _In_ POBJECT_TYPE *ObjectType;
  _In_ OB_OPERATION Operations;
  _In_ POB_PRE_OPERATION_CALLBACK PreOperation;
  _In_ POB_POST_OPERATION_CALLBACK PostOperation;
} OB_OPERATION_REGISTRATION, *POB_OPERATION_REGISTRATION;

typedef struct _OB_CALLBACK_REGISTRATION {
  _In_ USHORT Version;
  _In_ USHORT OperationRegistrationCount;
  _In_ UNICODE_STRING Altitude;
  _In_ PVOID RegistrationContext;
  _In_ OB_OPERATION_REGISTRATION *OperationRegistration;
} OB_CALLBACK_REGISTRATION, *POB_CALLBACK_REGISTRATION;

typedef struct _OBJECT_NAME_INFORMATION {
  UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

/* Exported object types */
#ifdef _NTSYSTEM_
extern POBJECT_TYPE NTSYSAPI CmKeyObjectType;
extern POBJECT_TYPE NTSYSAPI ExEventObjectType;
extern POBJECT_TYPE NTSYSAPI ExSemaphoreObjectType;
extern POBJECT_TYPE NTSYSAPI IoFileObjectType;
extern POBJECT_TYPE NTSYSAPI PsThreadType;
extern POBJECT_TYPE NTSYSAPI SeTokenObjectType;
extern POBJECT_TYPE NTSYSAPI PsProcessType;
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(CmKeyObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(IoFileObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(ExEventObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(ExSemaphoreObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(TmTransactionManagerObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(TmResourceManagerObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(TmEnlistmentObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(TmTransactionObjectType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(PsProcessType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(PsThreadType)
__CREATE_NTOS_DATA_IMPORT_ALIAS(SeTokenObjectType)
extern POBJECT_TYPE *CmKeyObjectType;
extern POBJECT_TYPE *IoFileObjectType;
extern POBJECT_TYPE *ExEventObjectType;
extern POBJECT_TYPE *ExSemaphoreObjectType;
extern POBJECT_TYPE *TmTransactionManagerObjectType;
extern POBJECT_TYPE *TmResourceManagerObjectType;
extern POBJECT_TYPE *TmEnlistmentObjectType;
extern POBJECT_TYPE *TmTransactionObjectType;
extern POBJECT_TYPE *PsProcessType;
extern POBJECT_TYPE *PsThreadType;
extern POBJECT_TYPE *SeTokenObjectType;
#endif

$endif (_WDMDDK_)
$if (_NTIFS_)
typedef enum _OBJECT_INFORMATION_CLASS {
  ObjectBasicInformation = 0,
  ObjectTypeInformation = 2,
$endif (_NTIFS_)
$if (_NTIFS_) // we should remove these, but the kernel needs them :-/
  /* Not for public use */
  ObjectNameInformation = 1,
  ObjectTypesInformation = 3,
  ObjectHandleFlagInformation = 4,
  ObjectSessionInformation = 5,
  MaxObjectInfoClass
$endif (_NTIFS_)
$if (_NTIFS_)
} OBJECT_INFORMATION_CLASS;

$endif (_NTIFS_)

