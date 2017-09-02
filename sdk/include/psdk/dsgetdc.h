#ifndef _DSGETDC_H
#define _DSGETDC_H

#ifdef __cplusplus
extern "C" {
#endif

#define DS_DOMAIN_IN_FOREST       0x01
#define DS_DOMAIN_DIRECT_OUTBOUND 0x02
#define DS_DOMAIN_TREE_ROOT       0x04
#define DS_DOMAIN_PRIMARY         0x08
#define DS_DOMAIN_NATIVE_MODE     0x10
#define DS_DOMAIN_DIRECT_INBOUND  0x20

typedef struct _DOMAIN_CONTROLLER_INFOA
{
	LPSTR DomainControllerName;
	LPSTR DomainControllerAddress;
	ULONG DomainControllerAddressType;
	GUID DomainGuid;
	LPSTR DomainName;
	LPSTR DnsForestName;
	ULONG Flags;
	LPSTR DcSiteName;
	LPSTR ClientSiteName;
} DOMAIN_CONTROLLER_INFOA, *PDOMAIN_CONTROLLER_INFOA;

typedef struct _DOMAIN_CONTROLLER_INFOW
{
	LPWSTR DomainControllerName;
	LPWSTR DomainControllerAddress;
	ULONG DomainControllerAddressType;
	GUID DomainGuid;
	LPWSTR DomainName;
	LPWSTR DnsForestName;
	ULONG Flags;
	LPWSTR DcSiteName;
	LPWSTR ClientSiteName;
} DOMAIN_CONTROLLER_INFOW, *PDOMAIN_CONTROLLER_INFOW;

typedef struct _DS_DOMAIN_TRUSTSA
{
	LPSTR NetbiosDomainName;
	LPSTR DnsDomainName;
	ULONG Flags;
	ULONG ParentIndex;
	ULONG TrustType;
	ULONG TrustAttributes;
	PSID DomainSid;
	GUID DomainGuid;
} DS_DOMAIN_TRUSTSA, *PDS_DOMAIN_TRUSTSA;

typedef struct _DS_DOMAIN_TRUSTSW
{
	LPWSTR NetbiosDomainName;
	LPWSTR DnsDomainName;
	ULONG Flags;
	ULONG ParentIndex;
	ULONG TrustType;
	ULONG TrustAttributes;
	PSID DomainSid;
	GUID DomainGuid;
} DS_DOMAIN_TRUSTSW, *PDS_DOMAIN_TRUSTSW;

DWORD WINAPI
DsAddressToSiteNamesA(
	LPCSTR ComputerName,
	DWORD EntryCount,
	PSOCKET_ADDRESS SocketAddresses,
	LPSTR **SiteNames);

DWORD WINAPI
DsAddressToSiteNamesW(
	LPCWSTR ComputerName,
	DWORD EntryCount,
	PSOCKET_ADDRESS SocketAddresses,
	LPWSTR **SiteNames);

DWORD WINAPI
DsAddressToSiteNamesExA(
	LPCSTR ComputerName,
	DWORD EntryCount,
	PSOCKET_ADDRESS SocketAddresses,
	LPSTR **SiteNames,
	LPSTR **SubnetNames);

DWORD WINAPI
DsAddressToSiteNamesExW(
	LPCWSTR ComputerName,
	DWORD EntryCount,
	PSOCKET_ADDRESS SocketAddresses,
	LPWSTR **SiteNames,
	LPWSTR **SubnetNames);

DWORD WINAPI
DsDeregisterDnsHostRecordsA(
	LPSTR ServerName,
	LPSTR DnsDomainName,
	GUID *DomainGuid,
	GUID *DsaGuid,
	LPSTR DnsHostName);

DWORD WINAPI
DsDeregisterDnsHostRecordsW(
	LPWSTR ServerName,
	LPWSTR DnsDomainName,
	GUID *DomainGuid,
	GUID *DsaGuid,
	LPWSTR DnsHostName);

DWORD WINAPI
DsEnumerateDomainTrustsA(
	LPSTR ServerName,
	ULONG Flags,
	PDS_DOMAIN_TRUSTSA* Domains,
	PULONG DomainCount);

DWORD WINAPI
DsEnumerateDomainTrustsW(
	LPWSTR ServerName,
	ULONG Flags,
	PDS_DOMAIN_TRUSTSW* Domains,
	PULONG DomainCount);

DWORD WINAPI
DsGetDcNameA(
	LPCSTR ComputerName,
	LPCSTR DomainName,
	GUID* DomainGuid,
	LPCSTR SiteName,
	ULONG Flags,
	PDOMAIN_CONTROLLER_INFOA* DomainControllerInfo);

DWORD WINAPI
DsGetDcNameW(
	LPCWSTR ComputerName,
	LPCWSTR DomainName,
	GUID* DomainGuid,
	LPCWSTR SiteName,
	ULONG Flags,
	PDOMAIN_CONTROLLER_INFOW* DomainControllerInfo);

DWORD WINAPI
DsGetDcSiteCoverageA(
	LPCSTR ServerName,
	PULONG EntryCount,
	LPSTR **SiteNames);

DWORD WINAPI
DsGetDcSiteCoverageW(
	LPCWSTR ServerName,
	PULONG EntryCount,
	LPWSTR **SiteNames);

DWORD WINAPI
DsGetForestTrustInformationW(
	LPCWSTR ServerName,
	LPCWSTR TrustedDomainName,
	DWORD Flags,
	PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo);

DWORD WINAPI
DsGetSiteNameA(
	LPCSTR ComputerName,
	LPSTR *SiteName);

DWORD WINAPI
DsGetSiteNameW(
	LPCWSTR ComputerName,
	LPWSTR *SiteName);

DWORD WINAPI
DsMergeForestTrustInformationW(
	LPCWSTR DomainName,
	PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo,
	PLSA_FOREST_TRUST_INFORMATION OldForestTrustInfo,
	PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo);

DWORD WINAPI
DsValidateSubnetNameA(
	LPCSTR SubnetName);

DWORD WINAPI
DsValidateSubnetNameW(
	LPCWSTR SubnetName);

#ifdef UNICODE
typedef DOMAIN_CONTROLLER_INFOW DOMAIN_CONTROLLER_INFO, *PDOMAIN_CONTROLLER_INFO;
typedef DS_DOMAIN_TRUSTSW DS_DOMAIN_TRUSTS, *PDS_DOMAIN_TRUSTS;
#define DsAddressToSiteNames DsAddressToSiteNamesW
#define DsAddressToSiteNamesEx DsAddressToSiteNamesExW
#define DsEnumerateDomainTrusts DsEnumerateDomainTrustsW
#define DsGetDcName DsGetDcNameW
#define DsGetDcSiteCoverage DsGetDcSiteCoverageW
#define DsGetSiteName DsGetSiteNameW
#define DsValidateSubnetName DsValidateSubnetNameW
#else
typedef DOMAIN_CONTROLLER_INFOA DOMAIN_CONTROLLER_INFO, *PDOMAIN_CONTROLLER_INFO;
typedef DS_DOMAIN_TRUSTSA DS_DOMAIN_TRUSTS, *PDS_DOMAIN_TRUSTS;
#define DsAddressToSiteNames DsAddressToSiteNamesA
#define DsAddressToSiteNamesEx DsAddressToSiteNamesExA
#define DsEnumerateDomainTrusts DsEnumerateDomainTrustsA
#define DsGetDcName DsGetDcNameA
#define DsGetDcSiteCoverage DsGetDcSiteCoverageA
#define DsGetSiteName DsGetSiteNameA
#define DsValidateSubnetName DsValidateSubnetNameA
#endif

#ifdef __cplusplus
}
#endif
#endif
