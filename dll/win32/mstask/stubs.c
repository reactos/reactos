
#include "precomp.h"
#include <atsvc_c.h>

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

typedef PWSTR SASEC_HANDLE;

HRESULT
WINAPI
ConvertAtJobsToTasks(void)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// See https://docs.microsoft.com/en-us/windows/win32/api/atacct/nf-atacct-getnetscheduleaccountinformation
HRESULT
WINAPI
GetNetScheduleAccountInformation(
    _In_z_ LPCWSTR pwszServerName,
    _In_ DWORD ccAccount,
    _Out_bytecap_(ccAccount) WCHAR wszAccount[])
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// For the following functions see https://winprotocoldoc.blob.core.windows.net/productionwindowsarchives/MS-TSCH/%5BMS-TSCH%5D.pdf

NET_API_STATUS
WINAPI
NetrJobAdd(
    _In_z_ ATSVC_HANDLE ServerName,
    _In_ LPAT_INFO pAtInfo,
    _Out_ LPDWORD pJobId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobDel(
    _In_z_ ATSVC_HANDLE ServerName,
    _In_ DWORD MinJobId,
    _In_ DWORD MaxJobId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobEnum(
    _In_z_ ATSVC_HANDLE ServerName,
    _Inout_ LPAT_ENUM_CONTAINER pEnumContainer,
    _In_ DWORD PreferedMaximumLength,
    _Out_ LPDWORD pTotalEntries,
    _Inout_ LPDWORD pResumeHandle)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

NET_API_STATUS
WINAPI
NetrJobGetInfo(
    _In_z_ ATSVC_HANDLE ServerName,
    _In_ DWORD JobId,
    _Outptr_ LPAT_INFO* ppAtInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

#define MAX_BUFFER_SIZE 273

HRESULT
WINAPI
SAGetAccountInformation(
    _In_z_ SASEC_HANDLE Handle,
    _In_z_ const wchar_t* pwszJobName,
    _In_range_(0, MAX_BUFFER_SIZE) DWORD ccBufferSize,
    _Inout_updates_z_(ccBufferSize) wchar_t wszBuffer[])
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SAGetNSAccountInformation(
    _In_z_ SASEC_HANDLE Handle,
    _In_range_(0, MAX_BUFFER_SIZE) DWORD ccBufferSize,
    _Inout_updates_z_(ccBufferSize) wchar_t wszBuffer[])
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SASetAccountInformation(
    _In_z_ SASEC_HANDLE Handle,
    _In_z_ const wchar_t* pwszJobName,
    _In_z_ const wchar_t* pwszAccount,
    _In_z_ const wchar_t* pwszPassword,
    _In_ DWORD dwJobFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SASetNSAccountInformation(
    _In_z_ SASEC_HANDLE Handle,
    _In_z_ const wchar_t* pwszAccount,
    _In_z_ const wchar_t* pwszPassword)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT
WINAPI
SetNetScheduleAccountInformation(
    _In_z_ LPCWSTR pwszServerName,
    _In_z_ LPCWSTR pwszAccount,
    _In_z_ LPCWSTR pwszPassword)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
