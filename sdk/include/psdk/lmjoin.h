#ifndef _LMJOIN_H
#define _LMJOIN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NETSETUP_JOIN_STATUS
{
    NetSetupUnknownStatus = 0,
    NetSetupUnjoined,
    NetSetupWorkgroupName,
    NetSetupDomainName
} NETSETUP_JOIN_STATUS, *PNETSETUP_JOIN_STATUS;

#define NETSETUP_JOIN_DOMAIN              0x00000001
#define NETSETUP_ACCT_CREATE              0x00000002
#define NETSETUP_ACCT_DELETE              0x00000004
#define NETSETUP_WIN9X_UPGRADE            0x00000010
#define NETSETUP_DOMAIN_JOIN_IF_JOINED    0x00000020
#define NETSETUP_JOIN_UNSECURE            0x00000040
#define NETSETUP_MACHINE_PWD_PASSED       0x00000080
#define NETSETUP_DEFER_SPN_SET            0x00000100
#define NETSETUP_JOIN_DC_ACCOUNT          0x00000200
#define NETSETUP_JOIN_WITH_NEW_NAME       0x00000400
#define NETSETUP_INSTALL_INVOCATION       0x00040000
#define NETSETUP_IGNORE_UNSUPPORTED_FLAGS 0x10000000

#define NETSETUP_VALID_UNJOIN_FLAGS      (NETSETUP_ACCT_DELETE | \
                                          NETSETUP_JOIN_DC_ACCOUNT | \
                                          NETSETUP_IGNORE_UNSUPPORTED_FLAGS)

NET_API_STATUS
NET_API_FUNCTION
NetJoinDomain(
    _In_opt_ LPCWSTR lpServer,
    _In_ LPCWSTR lpDomain,
    _In_opt_ LPCWSTR lpAccountOU,
    _In_opt_ LPCWSTR lpAccount,
    _In_opt_ LPCWSTR lpPassword,
    _In_ DWORD fJoinOptions);

NET_API_STATUS
NET_API_FUNCTION
NetUnjoinDomain(
    _In_opt_ LPCWSTR lpServer,
    _In_opt_ LPCWSTR lpAccount,
    _In_opt_ LPCWSTR lpPassword,
    _In_ DWORD fUnjoinOptions);

NET_API_STATUS
NET_API_FUNCTION
NetGetJoinInformation(
    _In_opt_ LPCWSTR lpServer,
    _Out_opt_ LPWSTR *lpNameBuffer,
    _Out_ PNETSETUP_JOIN_STATUS BufferType);

#ifdef __cplusplus
}
#endif

#endif
