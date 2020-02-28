
#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

#define NET_API_STATUS DWORD
typedef HANDLE ATSVC_HANDLE, SASEC_HANDLE;
typedef struct _AT_INFO *LPAT_INFO;
typedef struct _AT_ENUM_CONTAINER *LPAT_ENUM_CONTAINER;

HRESULT
WINAPI
ConvertAtJobsToTasks(void)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
GetNetScheduleAccountInformation(
  LPCWSTR pwszServerName,
  DWORD ccAccount,
  WCHAR wszAccount[])
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobAdd(
   ATSVC_HANDLE ServerName,
   LPAT_INFO *pAtInfo,
   LPDWORD pJobId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobDel(
    ATSVC_HANDLE ServerName,
    DWORD MinJobId,
    DWORD MaxJobId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobEnum(
    ATSVC_HANDLE ServerName,
    LPAT_ENUM_CONTAINER pEnumContainer,
    DWORD PreferedMaximumLength,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobGetInfo(
    ATSVC_HANDLE ServerName,
    DWORD JobId,
    LPAT_INFO* ppAtInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SAGetAccountInformation( 
    SASEC_HANDLE Handle, 
    const wchar_t* pwszJobName, 
    DWORD ccBufferSize, 
    wchar_t wszBuffer[])
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SAGetNSAccountInformation( 
    SASEC_HANDLE Handle, 
    DWORD ccBufferSize, 
    wchar_t wszBuffer[])
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SASetAccountInformation( 
    SASEC_HANDLE Handle, 
    const wchar_t* pwszJobName, 
    const wchar_t* pwszAccount, 
    const wchar_t* pwszPassword, 
    DWORD dwJobFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SASetNSAccountInformation( 
    SASEC_HANDLE Handle, 
    const wchar_t* pwszAccount, 
    const wchar_t* pwszPassword)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SetNetScheduleAccountInformation(
    IN LPCWSTR pwszServerName,
    IN LPCWSTR pwszAccount,
    IN LPCWSTR pwszPassword)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

