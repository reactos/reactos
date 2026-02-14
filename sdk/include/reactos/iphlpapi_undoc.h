#ifndef __IPHLPAPI_UNDOC_H__
#define __IPHLPAPI_UNDOC_H__

DWORD
WINAPI
NhGetInterfaceNameFromDeviceGuid(
    _In_ const GUID * pInterfaceGUID,
    _Out_writes_bytes_to_(*pOutBufLen, *pOutBufLen) PWCHAR pInterfaceName,
    _Inout_ PULONG pOutBufLen,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

DWORD
WINAPI
NhGetInterfaceNameFromGuid(
    _In_ const GUID * pInterfaceGUID,
    _Out_writes_bytes_to_(*pOutBufLen, *pOutBufLen) PWCHAR pInterfaceName,
    _Inout_ PULONG pOutBufLen,
    DWORD dwUnknown4,
    DWORD dwUnknown5);

#endif /* __IPHLPAPI_UNDOC_H__ */
