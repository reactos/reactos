/*
 * PROJECT:     ReactOS NetLogon Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NetLogon service RPC server
 * COPYRIGHT:   Eric Kohl 2019 <eric.kohl@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

//#include "lmerr.h"

WINE_DEFAULT_DEBUG_CHANNEL(netlogon);

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
RpcThreadRoutine(
    LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncacn_np", 20, L"\\pipe\\netlogon", NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(logon_v1_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    if (Status != RPC_S_OK)
    {
        ERR("RpcServerListen() failed (Status %lx)\n", Status);
    }

    return 0;
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


/* Function 0 */
NET_API_STATUS
__stdcall
NetrLogonUasLogon(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ wchar_t *UserName,
    _In_ wchar_t *Workstation,
    _Out_ PNETLOGON_VALIDATION_UAS_INFO *ValidationInformation)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 1 */
NET_API_STATUS
__stdcall
NetrLogonUasLogoff(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ wchar_t *UserName,
    _In_ wchar_t *Workstation,
    _Out_ PNETLOGON_LOGOFF_UAS_INFO LogoffInformation)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 2 */
NTSTATUS
__stdcall
NetrLogonSamLogon(
    _In_opt_ LOGONSRV_HANDLE LogonServer,
    _In_opt_ wchar_t *ComputerName,
    _In_opt_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_opt_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ NETLOGON_LOGON_INFO_CLASS LogonLevel,
    _In_ PNETLOGON_LEVEL LogonInformation,
    _In_ NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    _Out_ PNETLOGON_VALIDATION ValidationInformation,
    _Out_ UCHAR *Authoritative)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 3 */
NTSTATUS
__stdcall
NetrLogonSamLogoff(
    _In_opt_ LOGONSRV_HANDLE LogonServer,
    _In_opt_ wchar_t *ComputerName,
    _In_opt_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_opt_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ NETLOGON_LOGON_INFO_CLASS LogonLevel,
    _In_ PNETLOGON_LEVEL LogonInformation)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 4 */
NTSTATUS
__stdcall
NetrServerReqChallenge(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_CREDENTIAL ClientChallenge,
    _Out_ PNETLOGON_CREDENTIAL ServerChallenge)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 5 */
NTSTATUS
__stdcall
NetrServerAuthenticate(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_CREDENTIAL ClientCredential,
    _Out_ PNETLOGON_CREDENTIAL ServerCredential)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 6 */
NTSTATUS
__stdcall
NetrServerPasswordSet(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ PENCRYPTED_NT_OWF_PASSWORD UasNewPassword)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 7 */
NTSTATUS
__stdcall
NetrDatabaseDeltas(
    _In_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD DatabaseID,
    _Inout_ PNLPR_MODIFIED_COUNT DomainModifiedCount,
    _Out_ PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    _In_ DWORD PreferredMaximumLength)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 8 */
NTSTATUS
__stdcall
NetrDatabaseSync(
    _In_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD DatabaseID,
    _Inout_ ULONG *SyncContext,
    _Out_ PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    _In_ DWORD PreferredMaximumLength)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 9 */
NTSTATUS
__stdcall
NetrAccountDeltas(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t * ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ PUAS_INFO_0 RecordId,
    _In_ DWORD Count,
    _In_ DWORD Level,
    _Out_ UCHAR *Buffer,
    _In_ DWORD BufferSize,
    _Out_ ULONG *CountReturned,
    _Out_ ULONG *TotalEntries,
    _Out_ PUAS_INFO_0 NextRecordId)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 10 */
NTSTATUS
__stdcall
NetrAccountSync(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD Reference,
    _In_ DWORD Level,
    _Out_ UCHAR *Buffer,
    _In_ DWORD BufferSize,
    _Out_ ULONG *CountReturned,
    _Out_ ULONG *TotalEntries,
    _Out_ ULONG *NextReference,
    _Out_ PUAS_INFO_0 LastRecordId)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 11 */
NET_API_STATUS
__stdcall
NetrGetDCName(
    _In_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *DomainName,
    _Out_ wchar_t **Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 12 */
NET_API_STATUS
__stdcall
NetrLogonControl(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ DWORD FunctionCode,
    _In_ DWORD QueryLevel,
    _Out_ PNETLOGON_CONTROL_QUERY_INFORMATION Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 13 */
NET_API_STATUS
__stdcall
NetrGetAnyDCName(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *DomainName,
    _Out_ wchar_t **Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 14 */
NET_API_STATUS
__stdcall
NetrLogonControl2(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ DWORD FunctionCode,
    _In_ DWORD QueryLevel,
    _In_ PNETLOGON_CONTROL_DATA_INFORMATION Data,
    _Out_ PNETLOGON_CONTROL_QUERY_INFORMATION Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 15 */
NTSTATUS
__stdcall
NetrServerAuthenticate2(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_CREDENTIAL ClientCredential,
    _Out_ PNETLOGON_CREDENTIAL ServerCredential,
    _Inout_ ULONG *NegotiateFlags)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 16 */
NTSTATUS
__stdcall
NetrDatabaseSync2(
    _In_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD DatabaseID,
    _In_ SYNC_STATE RestartState,
    _Inout_ ULONG *SyncContext,
    _Out_ PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    _In_ DWORD PreferredMaximumLength)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 17 */
NTSTATUS
__stdcall
NetrDatabaseRedo(
    _In_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ UCHAR *ChangeLogEntry,
    _In_ DWORD ChangeLogEntrySize,
    _Out_ PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 18 */
NET_API_STATUS
__stdcall
NetrLogonControl2Ex(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ DWORD FunctionCode,
    _In_ DWORD QueryLevel,
    _In_ PNETLOGON_CONTROL_DATA_INFORMATION Data,
    _Out_ PNETLOGON_CONTROL_QUERY_INFORMATION Buffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 19 */
NTSTATUS
__stdcall
NetrEnumerateTrustedDomains(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _Out_ PDOMAIN_NAME_BUFFER DomainNameBuffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 20 */
NET_API_STATUS
__stdcall
DsrGetDcName(
    _In_opt_ LOGONSRV_HANDLE ComputerName,
    _In_opt_ wchar_t *DomainName,
    _In_opt_ GUID *DomainGuid,
    _In_opt_ GUID *SiteGuid,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 21 */
NTSTATUS
__stdcall
NetrLogonGetCapabilities(
    _In_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD QueryLevel,
    _Out_ PNETLOGON_CAPABILITIES ServerCapabilities)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 22 */
NTSTATUS
__stdcall
NetrLogonSetServiceBits(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ DWORD ServiceBitsOfInterest,
    _In_ DWORD ServiceBits)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 23 */
NET_API_STATUS
__stdcall
NetrLogonGetTrustRid(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *DomainName,
    _Out_ ULONG *Rid)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 24 */
NET_API_STATUS
__stdcall
NetrLogonComputeServerDigest(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ ULONG Rid,
    _In_ UCHAR *Message,
    _In_ ULONG MessageSize,
    _Out_ CHAR NewMessageDigest[16],
    _Out_ CHAR OldMessageDigest[16])
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 25 */
NET_API_STATUS
__stdcall
NetrLogonComputeClientDigest(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *DomainName,
    _In_ UCHAR *Message,
    _In_ ULONG MessageSize,
    _Out_ CHAR NewMessageDigest[16],
    _Out_ CHAR OldMessageDigest[16])
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 26 */
NTSTATUS
__stdcall
NetrServerAuthenticate3(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_CREDENTIAL ClientCredential,
    _Out_ PNETLOGON_CREDENTIAL ServerCredential,
    _Inout_ ULONG *NegotiateFlags,
    _Out_ ULONG *AccountRid)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 27 */
NET_API_STATUS
__stdcall
DsrGetDcNameEx(
    _In_opt_ LOGONSRV_HANDLE ComputerName,
    _In_opt_ wchar_t *DomainName,
    _In_opt_ GUID *DomainGuid,
    _In_opt_ wchar_t *SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 28 */
NET_API_STATUS
__stdcall
DsrGetSiteName(
    _In_opt_ LOGONSRV_HANDLE ComputerName,
    _Out_ wchar_t **SiteName)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 29 */
NTSTATUS
__stdcall
NetrLogonGetDomainInfo(
    _In_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD Level,
    _In_ PNETLOGON_WORKSTATION_INFORMATION WkstaBuffer,
    _Out_ PNETLOGON_DOMAIN_INFORMATION DomBuffer)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 30 */
NTSTATUS
__stdcall
NetrServerPasswordSet2(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ PNL_TRUST_PASSWORD ClearNewPassword)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 31 */
NTSTATUS
__stdcall
NetrServerPasswordGet(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _Out_ PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 32 */
NTSTATUS
__stdcall
NetrLogonSendToSam(
    _In_opt_ LOGONSRV_HANDLE PrimaryName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ UCHAR *OpaqueBuffer,
    _In_ ULONG OpaqueBufferSize)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 33 */
NET_API_STATUS
__stdcall
DsrAddressToSiteNamesW(
    _In_opt_ LOGONSRV_HANDLE ComputerName,
    _In_ DWORD EntryCount,
    _In_ PNL_SOCKET_ADDRESS SocketAddresses,
    _Out_ PNL_SITE_NAME_ARRAY *SiteNames)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 34 */
NET_API_STATUS
__stdcall
DsrGetDcNameEx2(
    _In_opt_ LOGONSRV_HANDLE ComputerName,
    _In_opt_ wchar_t *AccountName,
    _In_ ULONG AllowableAccountControlBits,
    _In_opt_ wchar_t *DomainName,
    _In_opt_ GUID *DomainGuid,
    _In_opt_ wchar_t *SiteName,
    _In_ ULONG Flags,
    _Out_ PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    UNIMPLEMENTED;
    return NERR_DCNotFound;
}


/* Function 35 */
NET_API_STATUS
__stdcall
NetrLogonGetTimeServiceParentDomain(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _Out_ wchar_t **DomainName,
    _Out_ int *PdcSameSite)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 36 */
NET_API_STATUS
__stdcall
NetrEnumerateTrustedDomainsEx(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _Out_ PNETLOGON_TRUSTED_DOMAIN_ARRAY Domains)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 37 */
NET_API_STATUS
__stdcall
DsrAddressToSiteNamesExW(
    _In_opt_ LOGONSRV_HANDLE ComputerName,
    _In_ DWORD EntryCount,
    _In_ PNL_SOCKET_ADDRESS SocketAddresses,
    _Out_ PNL_SITE_NAME_EX_ARRAY *SiteNames)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 38 */
NET_API_STATUS
__stdcall
DsrGetDcSiteCoverageW(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _Out_ PNL_SITE_NAME_ARRAY *SiteNames)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 39 */
NTSTATUS
__stdcall
NetrLogonSamLogonEx(
    _In_ handle_t ContextHandle,
    _In_opt_ wchar_t *LogonServer,
    _In_opt_ wchar_t *ComputerName,
    _In_ NETLOGON_LOGON_INFO_CLASS LogonLevel,
    _In_ PNETLOGON_LEVEL LogonInformation,
    _In_ NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    _Out_ PNETLOGON_VALIDATION ValidationInformation,
    _Out_ UCHAR *Authoritative,
    _Inout_ ULONG *ExtraFlags)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 40 */
NET_API_STATUS
__stdcall
DsrEnumerateDomainTrusts(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ ULONG Flags,
    _Out_ PNETLOGON_TRUSTED_DOMAIN_ARRAY Domains)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 41 */
NET_API_STATUS
__stdcall
DsrDeregisterDnsHostRecords(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *DnsDomainName,
    _In_opt_ GUID *DomainGuid,
    _In_opt_ GUID *DsaGuid,
    _In_ wchar_t *DnsHostName)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 42 */
NTSTATUS
__stdcall
NetrServerTrustPasswordsGet(
    _In_opt_ LOGONSRV_HANDLE TrustedDcName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _Out_ PENCRYPTED_NT_OWF_PASSWORD EncryptedNewOwfPassword,
    _Out_ PENCRYPTED_NT_OWF_PASSWORD EncryptedOldOwfPassword)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 43 */
NET_API_STATUS
__stdcall
DsrGetForestTrustInformation(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_opt_ wchar_t *TrustedDomainName,
    _In_ DWORD Flags,
    _Out_ PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 44 */
NTSTATUS
__stdcall
NetrGetForestTrustInformation(
    _In_opt_ LOGONSRV_HANDLE ServerName,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ DWORD Flags,
    _Out_ PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 45 */
NTSTATUS
__stdcall
NetrLogonSamLogonWithFlags(
    _In_opt_ LOGONSRV_HANDLE LogonServer,
    _In_opt_ wchar_t *ComputerName,
    _In_opt_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Inout_opt_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _In_ NETLOGON_LOGON_INFO_CLASS LogonLevel,
    _In_ PNETLOGON_LEVEL LogonInformation,
    _In_ NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    _Out_ PNETLOGON_VALIDATION ValidationInformation,
    _Out_ UCHAR *Authoritative,
    _Inout_ ULONG *ExtraFlags)
{
    UNIMPLEMENTED;
    return 0;
}


/* Function 46 */
NTSTATUS
__stdcall
NetrServerGetTrustInfo(
    _In_opt_ LOGONSRV_HANDLE TrustedDcName,
    _In_ wchar_t *AccountName,
    _In_ NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType,
    _In_ wchar_t *ComputerName,
    _In_ PNETLOGON_AUTHENTICATOR Authenticator,
    _Out_ PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    _Out_ PENCRYPTED_NT_OWF_PASSWORD EncryptedNewOwfPassword,
    _Out_ PENCRYPTED_NT_OWF_PASSWORD EncryptedOldOwfPassword,
    _Out_ PNL_GENERIC_RPC_DATA *TrustInfo)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
