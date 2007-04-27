#ifndef _DSGETDC_H
#define _DSGETDC_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

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

#ifdef UNICODE
typedef DOMAIN_CONTROLLER_INFOW DOMAIN_CONTROLLER_INFO, *PDOMAIN_CONTROLLER_INFO;
typedef DS_DOMAIN_TRUSTSW DS_DOMAIN_TRUSTS, *PDS_DOMAIN_TRUSTS;
#define DsEnumerateDomainTrusts DsEnumerateDomainTrustsW
#define DsGetDcName DsGetDcNameW
#else
typedef DOMAIN_CONTROLLER_INFOA DOMAIN_CONTROLLER_INFO, *PDOMAIN_CONTROLLER_INFO;
typedef DS_DOMAIN_TRUSTSA DS_DOMAIN_TRUSTS, *PDS_DOMAIN_TRUSTS;
#define DsEnumerateDomainTrusts DsEnumerateDomainTrustsA
#define DsGetDcName DsGetDcNameA
#endif

#ifdef __cplusplus
}
#endif
#endif