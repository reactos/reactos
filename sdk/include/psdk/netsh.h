#ifndef _NETSH_H_
#define _NETSH_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef
DWORD
(WINAPI *PGET_RESOURCE_STRING_FN)(
    _In_ DWORD dwMsgID,
    _Out_ LPWSTR lpBuffer,
    _In_ DWORD nBufferMax);

typedef
DWORD
(WINAPI *PNS_DLL_INIT_FN)(
    _In_ DWORD dwNetshVersion,
    _Out_ PVOID pReserved);

typedef
DWORD
(WINAPI *PNS_HELPER_START_FN)(
    _In_ const GUID *pguidParent,
    _In_ DWORD dwVersion);

typedef
DWORD
(WINAPI *PNS_HELPER_STOP_FN)(
    _In_ DWORD dwReserved);

typedef
DWORD
(WINAPI *PNS_CONTEXT_COMMIT_FN)(
    _In_ DWORD dwAction);

typedef
DWORD
(WINAPI *PNS_CONTEXT_CONNECT_FN)(
    _In_ LPCWSTR pwszMachine);

typedef
DWORD
(WINAPI *PNS_CONTEXT_DUMP_FN)(
    _In_ LPCWSTR pwszRouter,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwArgCount,
    _In_ LPCVOID pvData);

typedef
BOOL
(WINAPI *PNS_OSVERSIONCHECK)(
    _In_ UINT CIMOSType,
    _In_ UINT CIMOSProductSuite,
    _In_ LPCWSTR CIMOSVersion,
    _In_ LPCWSTR CIMOSBuildNumber,
    _In_ LPCWSTR CIMServicePackMajorVersion,
    _In_ LPCWSTR CIMServicePackMinorVersion,
    _In_ UINT uiReserved,
    _In_ DWORD dwReserved);

typedef
DWORD
(WINAPI *PFN_HANDLE_CMD)(
    _In_ LPCWSTR pwszMachine,
    _In_ LPWSTR *ppwcArguments,
    _In_ DWORD dwCurrentIndex,
    _In_ DWORD dwArgCount,
    _In_ DWORD dwFlags,
    _In_ LPCVOID pvData,
    _Out_ BOOL *pbDone);


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
    _In_ HANDLE hModule,
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
