#ifndef _ACCCTRL_H
#define _ACCCTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#define AccFree LocalFree

#define ACTRL_RESERVED            0x00000000
#define ACTRL_ACCESS_PROTECTED    0x00000001
#define ACTRL_ACCESS_ALLOWED      0x00000001
#define ACTRL_ACCESS_DENIED       0x00000002
#define ACTRL_AUDIT_SUCCESS       0x00000004
#define ACTRL_AUDIT_FAILURE       0x00000008
#define ACTRL_SYSTEM_ACCESS       0x04000000
#define ACTRL_DELETE              0x08000000
#define ACTRL_READ_CONTROL        0x10000000
#define ACTRL_CHANGE_ACCESS       0x20000000
#define ACTRL_CHANGE_OWNER        0x40000000
#define ACTRL_SYNCHRONIZE         0x80000000
#define ACTRL_STD_RIGHTS_ALL      0xf8000000

#define ACTRL_FILE_READ           0x00000001
#define ACTRL_FILE_WRITE          0x00000002
#define ACTRL_FILE_APPEND         0x00000004
#define ACTRL_FILE_READ_PROP      0x00000008
#define ACTRL_FILE_WRITE_PROP     0x00000010
#define ACTRL_FILE_EXECUTE        0x00000020
#define ACTRL_FILE_READ_ATTRIB    0x00000080
#define ACTRL_FILE_WRITE_ATTRIB   0x00000100
#define ACTRL_FILE_CREATE_PIPE    0x00000200

#define ACTRL_DIR_LIST            0x00000001
#define ACTRL_DIR_CREATE_OBJECT   0x00000002
#define ACTRL_DIR_CREATE_CHILD    0x00000004
#define ACTRL_DIR_DELETE_CHILD    0x00000040
#define ACTRL_DIR_TRAVERSE        0x00000020

#define ACTRL_KERNEL_TERMINATE    0x00000001
#define ACTRL_KERNEL_THREAD       0x00000002
#define ACTRL_KERNEL_VM           0x00000004
#define ACTRL_KERNEL_VM_READ      0x00000008
#define ACTRL_KERNEL_VM_WRITE     0x00000010
#define ACTRL_KERNEL_DUP_HANDLE   0x00000020
#define ACTRL_KERNEL_PROCESS      0x00000040
#define ACTRL_KERNEL_SET_INFO     0x00000080
#define ACTRL_KERNEL_GET_INFO     0x00000100
#define ACTRL_KERNEL_CONTROL      0x00000200
#define ACTRL_KERNEL_ALERT        0x00000400
#define ACTRL_KERNEL_GET_CONTEXT  0x00000800
#define ACTRL_KERNEL_SET_CONTEXT  0x00001000
#define ACTRL_KERNEL_TOKEN        0x00002000
#define ACTRL_KERNEL_IMPERSONATE  0x00004000
#define ACTRL_KERNEL_DIMPERSONATE 0x00008000

#define ACTRL_PRINT_SADMIN        0x00000001
#define ACTRL_PRINT_SLIST         0x00000002
#define ACTRL_PRINT_PADMIN        0x00000004
#define ACTRL_PRINT_PUSE          0x00000008
#define ACTRL_PRINT_JADMIN        0x00000010

#define ACTRL_SVC_GET_INFO        0x00000001
#define ACTRL_SVC_SET_INFO        0x00000002
#define ACTRL_SVC_STATUS          0x00000004
#define ACTRL_SVC_LIST            0x00000008
#define ACTRL_SVC_START           0x00000010
#define ACTRL_SVC_STOP            0x00000020
#define ACTRL_SVC_PAUSE           0x00000040
#define ACTRL_SVC_INTERROGATE     0x00000080
#define ACTRL_SVC_UCONTROL        0x00000100

#define ACTRL_REG_QUERY           0x00000001
#define ACTRL_REG_SET             0x00000002
#define ACTRL_REG_CREATE_CHILD    0x00000004
#define ACTRL_REG_LIST            0x00000008
#define ACTRL_REG_NOTIFY          0x00000010
#define ACTRL_REG_LINK            0x00000020

#define ACTRL_WIN_CLIPBRD         0x00000001
#define ACTRL_WIN_GLOBAL_ATOMS    0x00000002
#define ACTRL_WIN_CREATE          0x00000004
#define ACTRL_WIN_LIST_DESK       0x00000008
#define ACTRL_WIN_LIST            0x00000010
#define ACTRL_WIN_READ_ATTRIBS    0x00000020
#define ACTRL_WIN_WRITE_ATTRIBS   0x00000040
#define ACTRL_WIN_SCREEN          0x00000080
#define ACTRL_WIN_EXIT            0x00000100

#define ACTRL_ACCESS_NO_OPTIONS                 0x00000000
#define ACTRL_ACCESS_SUPPORTS_OBJECT_ENTRIES    0x00000001

#define ACCCTRL_DEFAULT_PROVIDERA   "Windows NT Access Provider"
#define ACCCTRL_DEFAULT_PROVIDERW   L"Windows NT Access Provider"

#define TRUSTEE_ACCESS_ALLOWED    0x00000001L
#define TRUSTEE_ACCESS_READ       0x00000002L
#define TRUSTEE_ACCESS_WRITE      0x00000004L
#define TRUSTEE_ACCESS_EXPLICIT   0x00000001L
#define TRUSTEE_ACCESS_READ_WRITE (TRUSTEE_ACCESS_READ | TRUSTEE_ACCESS_WRITE)
#define TRUSTEE_ACCESS_ALL        0xFFFFFFFFL

#define NO_INHERITANCE                      0x0
#define SUB_OBJECTS_ONLY_INHERIT            0x1
#define SUB_CONTAINERS_ONLY_INHERIT         0x2
#define SUB_CONTAINERS_AND_OBJECTS_INHERIT  0x3
#define INHERIT_NO_PROPAGATE                0x4
#define INHERIT_ONLY                        0x8
#define INHERITED_ACCESS_ENTRY              0x10
#define INHERITED_PARENT                    0x10000000
#define INHERITED_GRANDPARENT               0x20000000

#define SI_EDIT_PERMS               0x00000000
#define SI_EDIT_OWNER               0x00000001
#define SI_EDIT_AUDITS              0x00000002
#define SI_CONTAINER                0x00000004
#define SI_READONLY                 0x00000008
#define SI_ADVANCED                 0x00000010
#define SI_RESET                    0x00000020
#define SI_OWNER_READONLY           0x00000040
#define SI_EDIT_PROPERTIES          0x00000080
#define SI_OWNER_RECURSE            0x00000100
#define SI_NO_ACL_PROTECT           0x00000200
#define SI_NO_TREE_APPLY            0x00000400
#define SI_PAGE_TITLE               0x00000800
#define SI_SERVER_IS_DC             0x00001000
#define SI_RESET_DACL_TREE          0x00004000
#define SI_RESET_SACL_TREE          0x00008000
#define SI_OBJECT_GUID              0x00010000
#define SI_EDIT_EFFECTIVE           0x00020000
#define SI_RESET_DACL               0x00040000
#define SI_RESET_SACL               0x00080000
#define SI_RESET_OWNER              0x00100000
#define SI_NO_ADDITIONAL_PERMISSION 0x00200000
#define SI_MAY_WRITE                0x10000000
#define SI_EDIT_ALL                 (SI_EDIT_OWNER |SI_EDIT_PERMS | SI_EDIT_AUDITS)

#define SI_ACCESS_SPECIFIC          0x00010000
#define SI_ACCESS_GENERAL           0x00020000
#define SI_ACCESS_CONTAINER         0x00040000
#define SI_ACCESS_PROPERTY          0x00080000

typedef ULONG INHERIT_FLAGS, *PINHERIT_FLAGS;
typedef ULONG ACCESS_RIGHTS, *PACCESS_RIGHTS;

typedef enum _ACCESS_MODE
{
    NOT_USED_ACCESS = 0,
    GRANT_ACCESS,
    SET_ACCESS,
    DENY_ACCESS,
    REVOKE_ACCESS,
    SET_AUDIT_SUCCESS,
    SET_AUDIT_FAILURE
} ACCESS_MODE;

typedef enum _SE_OBJECT_TYPE
{
    SE_UNKNOWN_OBJECT_TYPE = 0,
    SE_FILE_OBJECT,
    SE_SERVICE,
    SE_PRINTER,
    SE_REGISTRY_KEY,
    SE_LMSHARE,
    SE_KERNEL_OBJECT,
    SE_WINDOW_OBJECT,
    SE_DS_OBJECT,
    SE_DS_OBJECT_ALL,
    SE_PROVIDER_DEFINED_OBJECT,
    SE_WMIGUID_OBJECT,
    SE_REGISTRY_WOW64_32KEY,
    SE_REGISTRY_WOW64_64KEY
} SE_OBJECT_TYPE;

typedef enum _TRUSTEE_TYPE
{
    TRUSTEE_IS_UNKNOWN,
    TRUSTEE_IS_USER,
    TRUSTEE_IS_GROUP,
    TRUSTEE_IS_DOMAIN,
    TRUSTEE_IS_ALIAS,
    TRUSTEE_IS_WELL_KNOWN_GROUP,
    TRUSTEE_IS_DELETED,
    TRUSTEE_IS_INVALID,
    TRUSTEE_IS_COMPUTER
} TRUSTEE_TYPE;

typedef enum _TRUSTEE_FORM
{
    TRUSTEE_IS_SID,
    TRUSTEE_IS_NAME,
    TRUSTEE_BAD_FORM,
    TRUSTEE_IS_OBJECTS_AND_SID,
    TRUSTEE_IS_OBJECTS_AND_NAME
} TRUSTEE_FORM;

typedef enum _MULTIPLE_TRUSTEE_OPERATION
{
    NO_MULTIPLE_TRUSTEE,
    TRUSTEE_IS_IMPERSONATE
} MULTIPLE_TRUSTEE_OPERATION;

typedef struct _TRUSTEE_A
{
    struct _TRUSTEE_A           *pMultipleTrustee;
    MULTIPLE_TRUSTEE_OPERATION  MultipleTrusteeOperation;
    TRUSTEE_FORM                TrusteeForm;
    TRUSTEE_TYPE                TrusteeType;
    LPSTR                       ptstrName;
} TRUSTEE_A, *PTRUSTEE_A, TRUSTEEA, *PTRUSTEEA;

typedef struct _TRUSTEE_W
{
    struct _TRUSTEE_W           *pMultipleTrustee;
    MULTIPLE_TRUSTEE_OPERATION  MultipleTrusteeOperation;
    TRUSTEE_FORM                TrusteeForm;
    TRUSTEE_TYPE                TrusteeType;
    LPWSTR                      ptstrName;
} TRUSTEE_W, *PTRUSTEE_W, TRUSTEEW, *PTRUSTEEW;

typedef struct _ACTRL_ACCESS_ENTRYA
{
    TRUSTEE_A       Trustee;
    ULONG           fAccessFlags;
    ACCESS_RIGHTS   Access;
    ACCESS_RIGHTS   ProvSpecificAccess;
    INHERIT_FLAGS   Inheritance;
    LPCSTR          lpInheritProperty;
} ACTRL_ACCESS_ENTRYA, *PACTRL_ACCESS_ENTRYA;

typedef struct _ACTRL_ACCESS_ENTRYW
{
    TRUSTEE_W       Trustee;
    ULONG           fAccessFlags;
    ACCESS_RIGHTS   Access;
    ACCESS_RIGHTS   ProvSpecificAccess;
    INHERIT_FLAGS   Inheritance;
    LPCWSTR         lpInheritProperty;
} ACTRL_ACCESS_ENTRYW, *PACTRL_ACCESS_ENTRYW;

typedef struct _ACTRL_ACCESS_ENTRY_LISTA
{
    ULONG                  cEntries;
    ACTRL_ACCESS_ENTRYA    *pAccessList;
} ACTRL_ACCESS_ENTRY_LISTA, *PACTRL_ACCESS_ENTRY_LISTA;

typedef struct _ACTRL_ACCESS_ENTRY_LISTW
{
    ULONG                  cEntries;
    ACTRL_ACCESS_ENTRYW    *pAccessList;
} ACTRL_ACCESS_ENTRY_LISTW, *PACTRL_ACCESS_ENTRY_LISTW;

typedef struct _ACTRL_PROPERTY_ENTRYA
{
    LPCSTR                      lpProperty;
    PACTRL_ACCESS_ENTRY_LISTA   pAccessEntryList;
    ULONG                       fListFlags;
} ACTRL_PROPERTY_ENTRYA, *PACTRL_PROPERTY_ENTRYA;

typedef struct _ACTRL_PROPERTY_ENTRYW
{
    LPCWSTR                     lpProperty;
    PACTRL_ACCESS_ENTRY_LISTW   pAccessEntryList;
    ULONG                       fListFlags;
} ACTRL_PROPERTY_ENTRYW, *PACTRL_PROPERTY_ENTRYW;

typedef struct _ACTRL_ALISTA
{
    ULONG                       cEntries;
    PACTRL_PROPERTY_ENTRYA      pPropertyAccessList;
} ACTRL_ACCESSA, *PACTRL_ACCESSA, ACTRL_AUDITA, *PACTRL_AUDITA;

typedef struct _ACTRL_ALISTW
{
    ULONG                       cEntries;
    PACTRL_PROPERTY_ENTRYW      pPropertyAccessList;
} ACTRL_ACCESSW, *PACTRL_ACCESSW, ACTRL_AUDITW, *PACTRL_AUDITW;

typedef struct _TRUSTEE_ACCESSA
{
    LPSTR           lpProperty;
    ACCESS_RIGHTS   Access;
    ULONG           fAccessFlags;
    ULONG           fReturnedAccess;
} TRUSTEE_ACCESSA, *PTRUSTEE_ACCESSA;

typedef struct _TRUSTEE_ACCESSW
{
    LPWSTR          lpProperty;
    ACCESS_RIGHTS   Access;
    ULONG           fAccessFlags;
    ULONG           fReturnedAccess;
} TRUSTEE_ACCESSW, *PTRUSTEE_ACCESSW;

typedef struct _ACTRL_OVERLAPPED
{
    _ANONYMOUS_UNION
    union
    {
        PVOID Provider;
        ULONG Reserved1;
    } DUMMYUNIONNAME;
    ULONG       Reserved2;
    HANDLE      hEvent;
} ACTRL_OVERLAPPED, *PACTRL_OVERLAPPED;

typedef struct _ACTRL_ACCESS_INFOA
{
    ULONG       fAccessPermission;
    LPSTR       lpAccessPermissionName;
} ACTRL_ACCESS_INFOA, *PACTRL_ACCESS_INFOA;

typedef struct _ACTRL_ACCESS_INFOW
{
    ULONG       fAccessPermission;
    LPWSTR      lpAccessPermissionName;
} ACTRL_ACCESS_INFOW, *PACTRL_ACCESS_INFOW;

typedef struct _ACTRL_CONTROL_INFOA
{
    LPSTR       lpControlId;
    LPSTR       lpControlName;
} ACTRL_CONTROL_INFOA, *PACTRL_CONTROL_INFOA;

typedef struct _ACTRL_CONTROL_INFOW
{
    LPWSTR      lpControlId;
    LPWSTR      lpControlName;
} ACTRL_CONTROL_INFOW, *PACTRL_CONTROL_INFOW;

typedef struct _EXPLICIT_ACCESS_A
{
    DWORD        grfAccessPermissions;
    ACCESS_MODE  grfAccessMode;
    DWORD        grfInheritance;
    TRUSTEE_A    Trustee;
} EXPLICIT_ACCESS_A, *PEXPLICIT_ACCESS_A, EXPLICIT_ACCESSA, *PEXPLICIT_ACCESSA;

typedef struct _EXPLICIT_ACCESS_W
{
    DWORD        grfAccessPermissions;
    ACCESS_MODE  grfAccessMode;
    DWORD        grfInheritance;
    TRUSTEE_W    Trustee;
} EXPLICIT_ACCESS_W, *PEXPLICIT_ACCESS_W, EXPLICIT_ACCESSW, *PEXPLICIT_ACCESSW;

typedef struct _OBJECTS_AND_SID
{
    DWORD   ObjectsPresent;
    GUID    ObjectTypeGuid;
    GUID    InheritedObjectTypeGuid;
    SID     *pSid;
} OBJECTS_AND_SID, *POBJECTS_AND_SID;

typedef struct _OBJECTS_AND_NAME_A
{
    DWORD    ObjectsPresent;
    SE_OBJECT_TYPE ObjectType;
    LPSTR    ObjectTypeName;
    LPSTR    InheritedObjectTypeName;
    LPSTR    ptstrName;
} OBJECTS_AND_NAME_A, *POBJECTS_AND_NAME_A;

typedef struct _OBJECTS_AND_NAME_W
{
    DWORD          ObjectsPresent;
    SE_OBJECT_TYPE ObjectType;
    LPWSTR   ObjectTypeName;
    LPWSTR   InheritedObjectTypeName;
    LPWSTR   ptstrName;
} OBJECTS_AND_NAME_W, *POBJECTS_AND_NAME_W;

#if (_WIN32_WINNT >= 0x0501)
typedef struct
{
    LONG     GenerationGap;
    LPSTR    AncestorName;
} INHERITED_FROMA, *PINHERITED_FROMA;

typedef struct
{
    LONG     GenerationGap;
    LPWSTR   AncestorName;
} INHERITED_FROMW, *PINHERITED_FROMW;
#endif /* (_WIN32_WINNT >= 0x0501) */

typedef struct _SI_OBJECT_INFO
{
    DWORD     dwFlags;
    HINSTANCE hInstance;
    LPWSTR    pszServerName;
    LPWSTR    pszObjectName;
    LPWSTR    pszPageTitle;
    GUID      guidObjectType;
} SI_OBJECT_INFO, *PSI_OBJECT_INFO;

typedef struct _SI_ACCESS
{
    const GUID  *pguid;
    ACCESS_MASK mask;
    LPCWSTR     pszName;
    DWORD       dwFlags;
} SI_ACCESS, *PSI_ACCESS;

typedef struct _SI_INHERIT_TYPE
{
    const GUID *pguid;
    ULONG      dwFlags;
    LPCWSTR    pszName;
} SI_INHERIT_TYPE, *PSI_INHERIT_TYPE;

typedef enum _SI_PAGE_TYPE
{
    SI_PAGE_PERM = 0,
    SI_PAGE_ADVPERM,
    SI_PAGE_AUDIT,
    SI_PAGE_OWNER
} SI_PAGE_TYPE;

typedef struct _FN_OBJECT_MGR_FUNCTIONS
{
    ULONG Placeholder;
} FN_OBJECT_MGR_FUNCTS, *PFN_OBJECT_MGR_FUNCTS;

typedef enum _PROG_INVOKE_SETTING
{
    ProgressInvokeNever = 1,
    ProgressInvokeEveryObject,
    ProgressInvokeOnError,
    ProgressCancelOperation,
    ProgressRetryOperation
} PROG_INVOKE_SETTING, *PPROG_INVOKE_SETTING;

typedef VOID (WINAPI *FN_PROGRESSW)(LPWSTR pObjectName,
                                    DWORD Status,
                                    PPROG_INVOKE_SETTING pInvokeSetting,
                                    PVOID Args,
                                    BOOL SecuritySet);
typedef VOID (WINAPI *FN_PROGRESSA)(LPSTR pObjectName,
                                    DWORD Status,
                                    PPROG_INVOKE_SETTING pInvokeSetting,
                                    PVOID Args,
                                    BOOL SecuritySet);

#ifdef UNICODE
#define ACCCTRL_DEFAULT_PROVIDER ACCCTRL_DEFAULT_PROVIDERW
typedef TRUSTEE_W TRUSTEE_, *PTRUSTEE_;
typedef TRUSTEEW TRUSTEE, *PTRUSTEE;
typedef ACTRL_ACCESSW ACTRL_ACCESS, *PACTRL_ACCESS;
typedef ACTRL_ACCESS_ENTRY_LISTW ACTRL_ACCESS_ENTRY_LIST, *PACTRL_ACCESS_ENTRY_LIST;
typedef ACTRL_ACCESS_INFOW ACTRL_ACCESS_INFO, *PACTRL_ACCESS_INFO;
typedef ACTRL_ACCESS_ENTRYW ACTRL_ACCESS_ENTRY, *PACTRL_ACCESS_ENTRY;
typedef ACTRL_AUDITW ACTRL_AUDIT, *PACTRL_AUDIT;
typedef ACTRL_CONTROL_INFOW ACTRL_CONTROL_INFO, *PACTRL_CONTROL_INFO;
typedef EXPLICIT_ACCESS_W EXPLICIT_ACCESS_, *PEXPLICIT_ACCESS_;
typedef EXPLICIT_ACCESSW EXPLICIT_ACCESS, *PEXPLICIT_ACCESS;
typedef TRUSTEE_ACCESSW TRUSTEE_ACCESS, *PTRUSTEE_ACCESS;
typedef OBJECTS_AND_NAME_W OBJECTS_AND_NAME_, *POBJECTS_AND_NAME_;

#if (_WIN32_WINNT >= 0x0501)
typedef INHERITED_FROMW INHERITED_FROM, *PINHERITED_FROM;
typedef FN_PROGRESSW FN_PROGRESS;
#define HAS_FN_PROGRESSW
#endif
#else
#define ACCCTRL_DEFAULT_PROVIDER ACCCTRL_DEFAULT_PROVIDERA
typedef TRUSTEE_A TRUSTEE_, *PTRUSTEE_;
typedef TRUSTEEA TRUSTEE, *PTRUSTEE;
typedef ACTRL_ACCESSA ACTRL_ACCESS, *PACTRL_ACCESS;
typedef ACTRL_ACCESS_ENTRY_LISTA ACTRL_ACCESS_ENTRY_LIST, *PACTRL_ACCESS_ENTRY_LIST;
typedef ACTRL_ACCESS_INFOA ACTRL_ACCESS_INFO, *PACTRL_ACCESS_INFO;
typedef ACTRL_ACCESS_ENTRYA ACTRL_ACCESS_ENTRY, *PACTRL_ACCESS_ENTRY;
typedef ACTRL_AUDITA ACTRL_AUDIT, *PACTRL_AUDIT;
typedef ACTRL_CONTROL_INFOA ACTRL_CONTROL_INFO, *PACTRL_CONTROL_INFO;
typedef EXPLICIT_ACCESS_A EXPLICIT_ACCESS_, *PEXPLICIT_ACCESS_;
typedef EXPLICIT_ACCESSA EXPLICIT_ACCESS, *PEXPLICIT_ACCESS;
typedef TRUSTEE_ACCESSA TRUSTEE_ACCESS, *PTRUSTEE_ACCESS;
typedef OBJECTS_AND_NAME_A OBJECTS_AND_NAME_, *POBJECTS_AND_NAME_;

#if (_WIN32_WINNT >= 0x0501)
typedef INHERITED_FROMA INHERITED_FROM, *PINHERITED_FROM;
typedef FN_PROGRESSA FN_PROGRESS;
#define HAS_FN_PROGRESSA
#endif /* (_WIN32_WINNT >= 0x0501) */

#endif /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif /* _ACCCTRL_H */
