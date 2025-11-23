#ifndef _NETSH_H_
#define _NETSH_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NETSH_ERROR_BASE                  15000
#define ERROR_NO_ENTRIES                  (NETSH_ERROR_BASE + 0)
#define ERROR_INVALID_SYNTAX              (NETSH_ERROR_BASE + 1)
#define ERROR_PROTOCOL_NOT_IN_TRANSPORT   (NETSH_ERROR_BASE + 2)
#define ERROR_NO_CHANGE                   (NETSH_ERROR_BASE + 3)
#define ERROR_CMD_NOT_FOUND               (NETSH_ERROR_BASE + 4)
#define ERROR_ENTRY_PT_NOT_FOUND          (NETSH_ERROR_BASE + 5)
#define ERROR_DLL_LOAD_FAILED             (NETSH_ERROR_BASE + 6)
#define ERROR_INIT_DISPLAY                (NETSH_ERROR_BASE + 7)
#define ERROR_TAG_ALREADY_PRESENT         (NETSH_ERROR_BASE + 8)
#define ERROR_INVALID_OPTION_TAG          (NETSH_ERROR_BASE + 9)
#define ERROR_NO_TAG                      (NETSH_ERROR_BASE + 10)
#define ERROR_MISSING_OPTION              (NETSH_ERROR_BASE + 11)
#define ERROR_TRANSPORT_NOT_PRESENT       (NETSH_ERROR_BASE + 12)
#define ERROR_SHOW_USAGE                  (NETSH_ERROR_BASE + 13)
#define ERROR_INVALID_OPTION_VALUE        (NETSH_ERROR_BASE + 14)
#define ERROR_OKAY                        (NETSH_ERROR_BASE + 15)
#define ERROR_CONTINUE_IN_PARENT_CONTEXT  (NETSH_ERROR_BASE + 16)
#define ERROR_SUPPRESS_OUTPUT             (NETSH_ERROR_BASE + 17)
#define ERROR_HELPER_ALREADY_REGISTERED   (NETSH_ERROR_BASE + 18)
#define ERROR_CONTEXT_ALREADY_REGISTERED  (NETSH_ERROR_BASE + 19)
#define ERROR_PARSING_FAILURE             (NETSH_ERROR_BASE + 20)
#define NETSH_ERROR_END          ERROR_CONTEXT_ALREADY_REGISTERED

typedef enum _NS_REQS
{
    NS_REQ_ZERO = 0,
    NS_REQ_PRESENT = 1,
    NS_REQ_ALLOW_MULTIPLE = 2,
    NS_REQ_ONE_OR_MORE = 3
} NS_REQS;

enum NS_CMD_FLAGS
{
    CMD_FLAG_PRIVATE     = 0x01,
    CMD_FLAG_INTERACTIVE = 0x02,
    CMD_FLAG_LOCAL       = 0x08,
    CMD_FLAG_ONLINE      = 0x10,
    CMD_FLAG_HIDDEN      = 0x20,
    CMD_FLAG_LIMIT_MASK  = 0xffff,
    CMD_FLAG_PRIORITY    = 0x80000000
};

enum NS_MODE_CHANGE
{
    NETSH_COMMIT       = 0,
    NETSH_UNCOMMIT     = 1,
    NETSH_FLUSH        = 2,
    NETSH_COMMIT_STATE = 3,
    NETSH_SAVE         = 4
};

#define DEFAULT_CONTEXT_PRIORITY 100

#define NETSH_ROOT_GUID          {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}}

typedef
DWORD
(WINAPI GET_RESOURCE_STRING_FN)(
    _In_ DWORD dwMsgID,
    _Out_ LPWSTR lpBuffer,
    _In_ DWORD nBufferMax);

typedef GET_RESOURCE_STRING_FN *PGET_RESOURCE_STRING_FN;

typedef
DWORD
(WINAPI NS_DLL_INIT_FN)(
    _In_ DWORD dwNetshVersion,
    _Out_ PVOID pReserved);

typedef NS_DLL_INIT_FN *PNS_DLL_INIT_FN;

typedef
DWORD
(WINAPI NS_HELPER_START_FN)(
    _In_ const GUID *pguidParent,
    _In_ DWORD dwVersion);

typedef NS_HELPER_START_FN *PNS_HELPER_START_FN;

typedef
DWORD
(WINAPI NS_HELPER_STOP_FN)(
    _In_ DWORD dwReserved);

typedef NS_HELPER_STOP_FN *PNS_HELPER_STOP_FN;

typedef
DWORD
(WINAPI NS_CONTEXT_COMMIT_FN)(
    _In_ DWORD dwAction);

typedef NS_CONTEXT_COMMIT_FN *PNS_CONTEXT_COMMIT_FN;

typedef
DWORD
(WINAPI NS_CONTEXT_CONNECT_FN)(
    _In_ LPCWSTR pwszMachine);

typedef NS_CONTEXT_CONNECT_FN *PNS_CONTEXT_CONNECT_FN;

typedef
DWORD
(WINAPI NS_CONTEXT_DUMP_FN)(
    _In_ LPCWSTR pwszRouter,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwArgCount,
    _In_ LPCVOID pvData);

typedef NS_CONTEXT_DUMP_FN *PNS_CONTEXT_DUMP_FN;

typedef
BOOL
(WINAPI NS_OSVERSIONCHECK)(
    _In_ UINT CIMOSType,
    _In_ UINT CIMOSProductSuite,
    _In_ LPCWSTR CIMOSVersion,
    _In_ LPCWSTR CIMOSBuildNumber,
    _In_ LPCWSTR CIMServicePackMajorVersion,
    _In_ LPCWSTR CIMServicePackMinorVersion,
    _In_ UINT uiReserved,
    _In_ DWORD dwReserved);

typedef NS_OSVERSIONCHECK *PNS_OSVERSIONCHECK;

typedef
DWORD
(WINAPI FN_HANDLE_CMD)(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone);

typedef FN_HANDLE_CMD *PFN_HANDLE_CMD;

typedef struct _CMD_ENTRY
{
    LPCWSTR pwszCmdToken;
    PFN_HANDLE_CMD pfnCmdHandler;
    DWORD dwShortCmdHelpToken;
    DWORD dwCmdHlpToken;
    DWORD dwFlags;
    PNS_OSVERSIONCHECK pOsVersionCheck;
} CMD_ENTRY, *PCMD_ENTRY;

typedef struct _CMD_GROUP_ENTRY
{
    LPCWSTR pwszCmdGroupToken;
    DWORD dwShortCmdHelpToken;
    ULONG ulCmdGroupSize;
    DWORD dwFlags;
    PCMD_ENTRY pCmdGroup;
    PNS_OSVERSIONCHECK pOsVersionCheck;
} CMD_GROUP_ENTRY, *PCMD_GROUP_ENTRY;

typedef struct _NS_HELPER_ATTRIBUTES
{
    union
    {
        struct
        {
            DWORD dwVersion;
            DWORD dwReserved;
        };
        ULONGLONG _ullAlign;
    };
    GUID guidHelper;
    PNS_HELPER_START_FN pfnStart;
    PNS_HELPER_STOP_FN pfnStop;
} NS_HELPER_ATTRIBUTES, *PNS_HELPER_ATTRIBUTES;

typedef struct _NS_CONTEXT_ATTRIBUTES
{
    union
    {
        struct
        {
            DWORD dwVersion;
            DWORD dwReserved;
        };
        ULONGLONG _ullAlign;
    };
    LPWSTR pwszContext;
    GUID guidHelper;
    DWORD dwFlags;
    ULONG ulPriority;
    ULONG ulNumTopCmds;
    CMD_ENTRY *pTopCmds;
    ULONG ulNumGroups;
    CMD_GROUP_ENTRY *pCmdGroups;
    PNS_CONTEXT_COMMIT_FN pfnCommitFn;
    PNS_CONTEXT_DUMP_FN pfnDumpFn;
    PNS_CONTEXT_CONNECT_FN pfnConnectFn;
    PVOID pReserved;
    PNS_OSVERSIONCHECK pfnOsVersionCheck;
} NS_CONTEXT_ATTRIBUTES, *PNS_CONTEXT_ATTRIBUTES;

typedef struct _TAG_TYPE
{
    LPCWSTR pwszTag;
    DWORD dwRequired;
    BOOL bPresent;
} TAG_TYPE, *PTAG_TYPE;

typedef struct _TOKEN_VALUE
{
    LPCWSTR pwszToken;
    DWORD dwValue;
} TOKEN_VALUE, *PTOKEN_VALUE;

DWORD
WINAPI
MatchEnumTag(
    _In_ HANDLE hModule,
    _In_ LPCWSTR pwcArg,
    _In_ DWORD dwNumArg,
    _In_ const TOKEN_VALUE *pEnumTable,
    _Out_ PDWORD pdwValue);

BOOL
WINAPI
MatchToken(
    _In_ LPCWSTR pwszUserToken,
    _In_ LPCWSTR pwszCmdToken);

DWORD
WINAPI
PreprocessCommand(
    _In_opt_ HANDLE hModule,
    _Inout_ LPWSTR *ppwcArguments,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _Inout_ PTAG_TYPE pttTags,
    _In_ DWORD dwTagCount,
    _In_ DWORD dwMinArgs,
    _In_ DWORD dwMaxArgs,
    _Out_ DWORD *pdwTagType);

DWORD
CDECL
PrintError(
    _In_opt_ HANDLE hModule,
    _In_ DWORD dwErrId,
    ...);

DWORD
CDECL
PrintMessageFromModule(
    _In_ HANDLE hModule,
    _In_ DWORD dwMsgId,
    ...);

DWORD
CDECL
PrintMessage(
    _In_ LPCWSTR pwszMessage,
    ...);

DWORD
WINAPI
RegisterContext(
    _In_ const NS_CONTEXT_ATTRIBUTES *pChildContext);

DWORD
WINAPI
RegisterHelper(
    _In_ const GUID *pguidParentContext,
    _In_ const NS_HELPER_ATTRIBUTES *pfnRegisterSubContext);

#ifdef __cplusplus
}
#endif

#endif /* _NETSH_H_ */
