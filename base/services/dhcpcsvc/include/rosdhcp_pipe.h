#ifndef ROSDHCP_PIPE_H
#define ROSDHCP_PIPE_H

enum {
    DhcpReqAcquireParams,
    DhcpReqReleaseParams,
    DhcpReqLeaseIpAddress,
    DhcpReqQueryHWInfo,
    DhcpReqReleaseIpAddress,
    DhcpReqRenewIpAddress,
    DhcpReqStaticRefreshParams,
};

typedef struct _COMM_DHCP_REQ {
    UINT Type;
    DWORD AdapterIndex;
    union
    {
        struct
        {
            CHAR AdapterName[64];
        } AcquireParams;
        struct {
            BOOL Inserted;
        } PnpEvent;
        struct {
            LPWSTR AdapterName;
            DHCPCAPI_PARAMS_ARRAY Params;
        } RegisterParamChange;
        struct {
            LPWSTR AdapterName;
            LPWSTR RequestId;
        } RequestParams, UndoRequestParams;
        struct {
            DWORD IPAddress;
            DWORD Netmask;
        } StaticRefreshParams;
    } Body;
} COMM_DHCP_REQ;

typedef union _COMM_DHCP_REPLY {
    DWORD Reply;
    struct {
        DWORD AdapterIndex;
        DWORD MediaType;
        DWORD Mtu;
        DWORD Speed;
    } QueryHWInfo;
} COMM_DHCP_REPLY;

#define DHCP_PIPE_NAME L"\\\\.\\pipe\\dhcpclient"

#endif/*ROSDHCP_PIPE_H*/
