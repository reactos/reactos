#ifndef _WINSVC_UNDOC_H_
#define _WINSVC_UNDOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__midl) && !defined(__WIDL__)
typedef enum _TAG_INFO_LEVEL
{
    TagInfoLevelNameFromTag = 1,
} TAG_INFO_LEVEL;

typedef enum _TAG_TYPE
{
    TagTypeService = 1,
} TAG_TYPE;

typedef struct _TAG_INFO_NAME_FROM_TAG_IN_PARAMS
{
    DWORD dwPid;
    DWORD dwTag;
} TAG_INFO_NAME_FROM_TAG_IN_PARAMS, *PTAG_INFO_NAME_FROM_TAG_IN_PARAMS;

typedef struct _TAG_INFO_NAME_FROM_TAG_OUT_PARAMS
{
    TAG_TYPE TagType;
    LPWSTR pszName;
} TAG_INFO_NAME_FROM_TAG_OUT_PARAMS, *PTAG_INFO_NAME_FROM_TAG_OUT_PARAMS;

typedef struct _TAG_INFO_NAME_FROM_TAG
{
    TAG_INFO_NAME_FROM_TAG_IN_PARAMS InParams;
    TAG_INFO_NAME_FROM_TAG_OUT_PARAMS OutParams;
} TAG_INFO_NAME_FROM_TAG, *PTAG_INFO_NAME_FROM_TAG;
#endif

DWORD
WINAPI
I_QueryTagInformation(
    PVOID Unused,
    TAG_INFO_LEVEL dwInfoLevel,
    PTAG_INFO_NAME_FROM_TAG InOutParams);

DWORD
WINAPI
I_ScGetCurrentGroupStateW(
    _In_ SC_HANDLE hSCManager,
    _In_ LPWSTR pszGroupName,
    _Out_ LPDWORD pdwGroupState);

VOID
WINAPI
I_ScIsSecurityProcess(VOID);

DWORD
WINAPI
I_ScPnPGetServiceName(
    _In_ SERVICE_STATUS_HANDLE hServiceStatus,
    _Out_ LPWSTR lpServiceName,
    _In_ DWORD cchServiceName);

/* I_ScSendPnPMessage */

/* I_ScSendTSMessage */

BOOL
WINAPI
I_ScSetServiceBitsA(
    _In_ SERVICE_STATUS_HANDLE hServiceStatus,
    _In_ DWORD dwServiceBits,
    _In_ BOOL bSetBitsOn,
    _In_ BOOL bUpdateImmediately,
    _In_ LPSTR lpString);

BOOL
WINAPI
I_ScSetServiceBitsW(
    _In_ SERVICE_STATUS_HANDLE hServiceStatus,
    _In_ DWORD dwServiceBits,
    _In_ BOOL bSetBitsOn,
    _In_ BOOL bUpdateImmediately,
    _In_ LPWSTR lpString);

#ifdef UNICODE
#define I_ScSetServiceBits I_ScSetServiceBitsW
#else /* UNICODE */
#define I_ScSetServiceBits I_ScSetServiceBitsA
#endif

DWORD
WINAPI
I_ScValidatePnpService(
    _In_ LPCWSTR pszMachineName,
    _In_ LPCWSTR pszServiceName,
    _Out_ SERVICE_STATUS_HANDLE *phServiceStatus);

#ifdef __cplusplus
}
#endif

#endif /* _WINSVC_UNDOC_H_ */
