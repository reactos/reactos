#ifndef ROSDHCP_PUBLIC_H
#define ROSDHCP_PUBLIC_H

enum {
    DhcpReqLeaseIpAddress,
    DhcpReqQueryHWInfo,
    DhcpReqReleaseIpAddress,
    DhcpReqRenewIpAddress,
    DhcpReqStaticRefreshParams,
    DhcpReqGetAdapterInfo,
};

typedef struct _COMM_DHCP_REQ {
    UINT Type;
    DWORD AdapterIndex;
    union {
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
    struct {
        BOOL DhcpEnabled;
        DWORD DhcpServer;
        time_t LeaseObtained;
        time_t LeaseExpires;
    } GetAdapterInfo;
} COMM_DHCP_REPLY;

#define DHCP_PIPE_NAME "\\\\.\\pipe\\dhcpclient"

#endif/*ROSDHCP_PUBLIC_H*/
