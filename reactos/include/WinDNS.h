/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        include/WinDNS.h
 * PURPOSE:     Structures and definitions needed for DNSAPI compatibility.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#ifndef WINDNS_H
#define WINDNS_H

/* The WinDNS Header */
#ifdef __USE_W32API
#include_next <windns.h>
#else

/* Constants provided by DNSAPI */
#define DNS_CONFIG_FLAG_ALLOC TRUE

#define BIT(n) (1<<(n))
#define DNS_QUERY_STANDARD 0
#define DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE BIT(0)
#define DNS_QUERY_USE_TCP_ONLY BIT(1)
#define DNS_QUERY_NO_RECURSION BIT(2)
#define DNS_QUERY_BYPASS_CACHE BIT(3)
#define DNS_QUERY_NO_WIRE_QUERY BIT(4)
#define DNS_QUERY_NO_LOCAL_NAME BIT(5)
#define DNS_QUERY_NO_HOSTS_FILE BIT(6)
#define DNS_QUERY_NO_NETBT BIT(7)
#define DNS_QUERY_WIRE_ONLY BIT(8)
#define DNS_QUERY_RETURN_MESSAGE BIT(9)
#define DNS_QUERY_TREAT_AS_FQDN BIT(12)
#define DNS_QUERY_DONT_RESET_TTL_VALUES BIT(23)
#define DNS_QUERY_RESERVED (0xff << 24)
#define DNS_QUERY_CACHE_ONLY BIT(4)

#define DNS_TYPE_ZERO 0
#define DNS_TYPE_A 1
#define DNS_TYPE_NS 2
#define DNS_TYPE_MD 3
#define DNS_TYPE_MF 4
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_SOA 6
#define DNS_TYPE_MB 7
#define DNS_TYPE_MG 8
#define DNS_TYPE_MR 9
#define DNS_TYPE_NULL 10
#define DNS_TYPE_WKS 11
#define DNS_TYPE_PTR 12
#define DNS_TYPE_HINFO 13
#define DNS_TYPE_MINFO 14
#define DNS_TYPE_MX 15
#define DNS_TYPE_TXT 16
#define DNS_TYPE_RP 17
#define DNS_TYPE_AFSDB 18
#define DNS_TYPE_X25 19
#define DNS_TYPE_ISDN 20
#define DNS_TYPE_RT 21
#define DNS_TYPE_NSAP 22
#define DNS_TYPE_NSAPPTR 23
#define DNS_TYPE_SIG 24
#define DNS_TYPE_KEY 25
#define DNS_TYPE_PX 26
#define DNS_TYPE_GPOS 27
#define DNS_TYPE_AAAA 28
#define DNS_TYPE_LOC 29
#define DNS_TYPE_NXT 30
#define DNS_TYPE_EID 31 /* I've never heard of nimrod before. */
#define DNS_TYPE_NIMLOC 32 /* But it does exist. */
#define DNS_TYPE_SRV 33
#define DNS_TYPE_ATMA 34
#define DNS_TYPE_NAPTR 35
#define DNS_TYPE_KX 36
#define DNS_TYPE_CERT 37
#define DNS_TYPE_A6 38
#define DNS_TYPE_DNAME 39
#define DNS_TYPE_SINK 40
#define DNS_TYPE_OPT 41

#define DNS_TYPE_UINFO 100
#define DNS_TYPE_UID 101
#define DNS_TYPE_GID 102
#define DNS_TYPE_UNSPEC 103

#define DNS_TYPE_ADDRS 248
#define DNS_TYPE_TKEY 249
#define DNS_TYPE_TSIG 250
#define DNS_TYPE_IXFR 251
#define DNS_TYPE_AXFR 252
#define DNS_TYPE_MAILB 253
#define DNS_TYPE_MAILA 254
#define DNS_TYPE_ANY 255

#define DNS_SWAPX(x) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

#define DNS_RTYPE_ZERO DNS_SWAPX(DNS_TYPE_ZERO)
#define DNS_RTYPE_A DNS_SWAPX(DNS_TYPE_A)
#define DNS_RTYPE_NS DNS_SWAPX(DNS_TYPE_NS)
#define DNS_RTYPE_MD DNS_SWAPX(DNS_TYPE_MD)
#define DNS_RTYPE_MF DNS_SWAPX(DNS_TYPE_MF)
#define DNS_RTYPE_CNAME DNS_SWAPX(DNS_TYPE_CNAME)
#define DNS_RTYPE_SOA DNS_SWAPX(DNS_TYPE_SOA)
#define DNS_RTYPE_MB DNS_SWAPX(DNS_TYPE_MB)
#define DNS_RTYPE_MG DNS_SWAPX(DNS_TYPE_MG)
#define DNS_RTYPE_MR DNS_SWAPX(DNS_TYPE_MR)
#define DNS_RTYPE_NULL DNS_SWAPX(DNS_TYPE_NULL)
#define DNS_RTYPE_WKS DNS_SWAPX(DNS_TYPE_WKS)
#define DNS_RTYPE_PTR DNS_SWAPX(DNS_TYPE_PTR)
#define DNS_RTYPE_HINFO DNS_SWAPX(DNS_TYPE_HINFO)
#define DNS_RTYPE_MINFO DNS_SWAPX(DNS_TYPE_MINFO)
#define DNS_RTYPE_MX DNS_SWAPX(DNS_TYPE_MX)
#define DNS_RTYPE_TEXT DNS_SWAPX(DNS_TYPE_TEXT)
#define DNS_RTYPE_RP DNS_SWAPX(DNS_TYPE_RP)
#define DNS_RTYPE_AFSDB DNS_SWAPX(DNS_TYPE_AFSDB)
#define DNS_RTYPE_X25 DNS_SWAPX(DNS_TYPE_X25)
#define DNS_RTYPE_ISDN DNS_SWAPX(DNS_TYPE_ISDN)
#define DNS_RTYPE_RT DNS_SWAPX(DNS_TYPE_RT)
#define DNS_RTYPE_NSAP DNS_SWAPX(DNS_TYPE_NSAP)
#define DNS_RTYPE_NSAPPTR DNS_SWAPX(DNS_TYPE_NSAPPTR)
#define DNS_RTYPE_SIG DNS_SWAPX(DNS_TYPE_SIG)
#define DNS_RTYPE_KEY DNS_SWAPX(DNS_TYPE_KEY)
#define DNS_RTYPE_PX DNS_SWAPX(DNS_TYPE_PX)
#define DNS_RTYPE_GPOS DNS_SWAPX(DNS_TYPE_GPOS)
#define DNS_RTYPE_AAAA DNS_SWAPX(DNS_TYPE_AAAA)
#define DNS_RTYPE_LOC DNS_SWAPX(DNS_TYPE_LOC)
#define DNS_RTYPE_NXT DNS_SWAPX(DNS_TYPE_NXT)
#define DNS_RTYPE_EID DNS_SWAPX(DNS_TYPE_EID)
#define DNS_RTYPE_NIMLOC DNS_SWAPX(DNS_TYPE_NIMLOC)
#define DNS_RTYPE_SRV DNS_SWAPX(DNS_TYPE_SRV)
#define DNS_RTYPE_ATMA DNS_SWAPX(DNS_TYPE_ATMA)
#define DNS_RTYPE_NAPTR DNS_SWAPX(DNS_TYPE_NAPTR)
#define DNS_RTYPE_KX DNS_SWAPX(DNS_TYPE_KX)
#define DNS_RTYPE_CERT DNS_SWAPX(DNS_TYPE_CERT)
#define DNS_RTYPE_A6 DNS_SWAPX(DNS_TYPE_A6)
#define DNS_RTYPE_DNAME DNS_SWAPX(DNS_TYPE_DNAME)
#define DNS_RTYPE_SINK DNS_SWAPX(DNS_TYPE_SINK)
#define DNS_RTYPE_OPT DNS_SWAPX(DNS_TYPE_OPT)
#define DNS_RTYPE_UINFO DNS_SWAPX(DNS_TYPE_UINFO)
#define DNS_RTYPE_UID DNS_SWAPX(DNS_TYPE_UID)
#define DNS_RTYPE_GID DNS_SWAPX(DNS_TYPE_GID)
#define DNS_RTYPE_UNSPEC DNS_SWAPX(DNS_TYPE_UNSPEC)
#define DNS_RTYPE_ADDRS DNS_SWAPX(DNS_TYPE_ADDRS)
#define DNS_RTYPE_TKEY DNS_SWAPX(DNS_TYPE_TKEY)
#define DNS_RTYPE_TSIG DNS_SWAPX(DNS_TYPE_TSIG)
#define DNS_RTYPE_IXFR DNS_SWAPX(DNS_TYPE_IXFR)
#define DNS_RTYPE_AXFR DNS_SWAPX(DNS_TYPE_AXFR)
#define DNS_RTYPE_MAILB DNS_SWAPX(DNS_TYPE_MAILB)
#define DNS_RTYPE_MAILA DNS_SWAPX(DNS_TYPE_MAILA)
#define DNS_RTYPE_ANY DNS_SWAPX(DNS_TYPE_ANY)

/* Simple types */
typedef LONG DNS_STATUS;
typedef DNS_STATUS *PDNS_STATUS;
typedef DWORD IP4_ADDRESS;
typedef IP4_ADDRESS *PIP4_ADDRESS;
typedef struct {
  DWORD IP6Dword[4];
} IP6_ADDRESS, *PIP6_ADDRESS;

typedef enum {
  DnsFreeFlat,
  DnsFreeRecordList,
  DnsFreeParsedMessageFields
} DNS_FREE_TYPE;

typedef enum {
  DnsNameDomain,
  DnsNameDomainLabel,
  DnsNameHostnameFull,
  DnsNameHostnameLabel,
  DnsNameWildcard,
  DnsNameSrvRecord
} DNS_NAME_FORMAT;

/* The below is for repetitive names that distinguish character coding */
#define DNS_PASTE(x,y) x ## y
#define DNSCT_TRIPLET(x) DNS_PASTE(x,_W), DNS_PASTE(x,_A), DNS_PASTE(x,_UTF8)

typedef enum {
  DNSCT_TRIPLET(DnsConfigPrimaryDomainName),
  DNSCT_TRIPLET(DnsConfigAdapterDomainName),
  DnsConfigDnsServerList,
  DnsConfigSearchList,
  DnsConfigAdapterInfo,
  DnsConfigPrimaryHostNameRegistrationEnabled,
  DnsConfigAdapterHostNameRegistrationEnabled,
  DnsConfigAddressRegistrationMaxCount,
  DNSCT_TRIPLET(DnsConfigHostName),
  DNSCT_TRIPLET(DnsConfigFullHostName)
} DNS_CONFIG_TYPE;

/* Aggregates provided by DNSAPI */

typedef struct {
  DWORD Section : 2;
  DWORD Delete : 1;
  DWORD CharSet : 2;
  DWORD Unused : 3;
  DWORD Reserved : 24;
} DNS_RECORD_FLAGS;

typedef struct _IP4_ARRAY {
  DWORD AddrCount;
  IP4_ADDRESS AddrArray[1]; /* Avoid zero-length array warning */
} IP4_ARRAY, *PIP4_ARRAY;

typedef struct _DNS_HEADER {
  WORD Xid;
  BYTE RecursionDesired;
  BYTE Truncation;
  BYTE Authoritative;
  BYTE Opcode;
  BYTE IsResponse;
  BYTE ResponseCode;
  BYTE Reserved;
  BYTE RecursionAvailable;
  WORD QuestionCount;
  WORD NameServerCount;
  WORD AdditionalCount;
} DNS_HEADER, *PDNS_HEADER;

typedef struct _DNS_MESSAGE_BUFFER {
  DNS_HEADER MessageHead;
  CHAR MessageBody[1];
} DNS_MESSAGE_BUFFER, *PDNS_MESSAGE_BUFFER;

/* Some DNSAPI structs are sensitive to character coding.  This will allow
 * us to define them once. 
 *
 * the XNAME(n) macro turns the name into either nA or nW
 * the PSTR type expands to either LPWSTR or LPSTR.
 */

/* A record, indicates an IPv4 address. */
typedef struct {
  IP4_ADDRESS IpAddress;
} DNS_A_DATA, *PDNS_A_DATA;

typedef struct {
  DWORD dwByteCount;
  BYTE Data[1];
} DNS_NULL_DATA, *PDNS_NULL_DATA;

typedef struct {
  IP4_ADDRESS IpAddress;
  UCHAR chProtocol;
  BYTE BitMask[1];
} DNS_WKS_DATA, *PDNS_WKS_DATA;

typedef struct {
  IP6_ADDRESS Ip6Address;
} DNS_AAAA_DATA, *PDNS_AAAA_DATA;

typedef struct {
  WORD wFlags;
  BYTE chProtocol;
  BYTE chAlgorithm;
  BYTE Key[1];
} DNS_KEY_DATA, *PDNS_KEY_DATA;

typedef struct {
  WORD wVersion;
  WORD wSize;
  WORD wHorPrec;
  WORD wVerPrec;
  DWORD dwLatitude;
  DWORD dwLongitude;
  DWORD dwAltitude;
} DNS_LOC_DATA, *PDNS_LOC_DATA;

/* Here's the ugly bit, we import twice; once for wchar and once for char. */
#define XNAME(a) a ## A
#define PSTR LPSTR
#define _WINDNS_STRUCT_DEFINITIONS
#include "WinDNS.h"
#undef XNAME
#undef PSTR
#define XNAME(a) a ## W
#define PSTR LPWSTR
#include "WinDNS.h"
#undef XNAME
#undef PSTR
#undef _WINDNS_STRUCT_DEFINITIONS

#ifndef _DISABLE_TIDENTS
#ifdef _UNICODE
#define DECLARE_STRUCT(x) typedef DNS_PASTE(x,W) x , *DNS_PASTE(P,x)
#define DECLARE_FUNC(x) DNS_PASTE(x,_W)
#else /* _UNICODE */
#define DECLARE_STRUCT(x) typedef DNS_PASTE(x,A) x , *DNS_PASTE(P,x)
#define DECLARE_FUNC(x) DNS_PASTE(x,_A)
#endif /* _UNICODE */

/* Now, all the declarations go off. */

DECLARE_STRUCT(DNS_PTR_DATA);
DECLARE_STRUCT(DNS_SOA_DATA);
DECLARE_STRUCT(DNS_MINFO_DATA);
DECLARE_STRUCT(DNS_MX_DATA);
DECLARE_STRUCT(DNS_TXT_DATA);
DECLARE_STRUCT(DNS_SIG_DATA);
DECLARE_STRUCT(DNS_NXT_DATA);
DECLARE_STRUCT(DNS_SRV_DATA);
DECLARE_STRUCT(DNS_RECORD);

#define DnsAcquireContextHandle DECLARE_FUNC(DnsAcquireContextHandle)
#define DnsValidateName DECLARE_FUNC(DnsValidateName)
#define DnsQuery DECLARE_FUNC(DnsQuery)

#endif /* _DISABLE_TIDENTS */

/* Prototypes */

void WINAPI DnsReleaseContextHandle( HANDLE ContextHandle );

DNS_STATUS WINAPI DnsAcquireContextHandle_W
( DWORD CredentialsFlags, 
  PVOID Credentials, 
  HANDLE *ContextHandle );
DNS_STATUS WINAPI DnsAcquireContextHandle_UTF8
( DWORD CredentialsFlags, 
  PVOID Credentials, 
  HANDLE *ContextHandle );
DNS_STATUS WINAPI DnsAcquireContextHandle_A
( DWORD CredentialsFlags, 
  PVOID Credentials, 
  HANDLE *ContextHandle );

DNS_STATUS WINAPI DnsValidateName_W
( LPCWSTR Name, DNS_NAME_FORMAT Format );
DNS_STATUS WINAPI DnsValidateName_UTF8
( LPCSTR Name, DNS_NAME_FORMAT Format );
DNS_STATUS WINAPI DnsValidateName_A
( LPCSTR Name, DNS_NAME_FORMAT Format );

DNS_STATUS WINAPI DnsQuery_W
( LPCWSTR Name, WORD Type, DWORD Options, PIP4_ARRAY Servers,
  PDNS_RECORDW *Result, PVOID *Reserved );
DNS_STATUS WINAPI DnsQuery_UTF8
( LPCSTR Name, WORD Type, DWORD Options, PIP4_ARRAY Servers,
  PDNS_RECORDA *Result, PVOID *Reserved );
DNS_STATUS WINAPI DnsQuery_A
( LPCSTR Name, WORD Type, DWORD Options, PIP4_ARRAY Servers,
  PDNS_RECORDA *Result, PVOID *Reserved );
VOID WINAPI DnsFree( PVOID Data, DNS_FREE_TYPE FreeType );
VOID WINAPI DnsRecordListFree( PVOID RecordList, 
			       DNS_FREE_TYPE FreeType );

#endif//WINDNS_H

#ifdef _WINDNS_STRUCT_DEFINITIONS

/* PTR record, indicates an additional lookup for the given name is needed. */
typedef struct {
  PSTR pNameHost;
} XNAME(DNS_PTR_DATA), *XNAME(PDNS_PTR_DATA);

typedef struct {
  PSTR pNamePrimaryServer;
  PSTR pNameAdministrator;
  DWORD dwSerialNo;
  DWORD dwRefresh;
  DWORD dwRetry;
  DWORD dwExpire;
  DWORD dwDefaultTtl;
} XNAME(DNS_SOA_DATA), *XNAME(PDNS_SOA_DATA);

/* MINFO record, who to mail about questions or problems. */
typedef struct {
  PSTR pNameMailbox;
  PSTR pNameErrorsMailbox;
} XNAME(DNS_MINFO_DATA), *XNAME(PNDS_MINFO_DATA);

/* MX record, who processes the mail for this domain? */
typedef struct {
  PSTR pNameExchange;
  WORD wPreference;
  WORD Pad;
} XNAME(DNS_MX_DATA), *XNAME(PDNS_MX_DATA);

/* TXT record, miscellaneous information. */
typedef struct {
  DWORD dwStringCount;
  PSTR pStringArray[1];
} XNAME(DNS_TXT_DATA), *XNAME(PDNS_TXT_DATA);

typedef struct {
  PSTR pNameSigner;
  WORD wTypeCovered;
  BYTE cbAlgorithm;
  BYTE cbLabelCount;
  DWORD dwOriginalTtl;
  DWORD dwExpiration;
  DWORD dwTimeSigned;
  WORD wKeyTag;
  WORD Pad;
  BYTE Signature[1];
} XNAME(DNS_SIG_DATA), *XNAME(PDNS_SIG_DATA);

typedef struct {
  PSTR pNameNext;
  WORD wNumTypes;
  WORD wTypes[1];
} XNAME(DNS_NXT_DATA), *XNAME(PDNS_NXT_DATA);

typedef struct {
  PSTR pNameTarget;
  WORD wPriority;
  WORD wWeight;
  WORD wPort;
  WORD Pad;
} XNAME(DNS_SRV_DATA), *XNAME(PDNS_SRV_DATA);

typedef struct XNAME(_DNS_RECORD_) {
  struct XNAME(_DNS_RECORD_) *pNext;
  PSTR pName;
  WORD wType;
  WORD wDataLength;
  
  union {
    DWORD DW;
    DNS_RECORD_FLAGS S;
  } Flags;
  
  DWORD dwTtl;
  DWORD dwReserved;
  
  union {
    DNS_A_DATA A;
    XNAME(DNS_SOA_DATA) SOA, Soa;
    XNAME(DNS_PTR_DATA) PTR, Ptr, NS, Ns, CNAME, Cname, MB, Mb, MD, Md, MF, Mf, MG, Mg, MR, Mr;
    XNAME(DNS_MINFO_DATA) MINFO, Minfo, RP, Rp;
    XNAME(DNS_MX_DATA) MX, Mx, AFSDB, Afsdb, RT, Rt;
    XNAME(DNS_TXT_DATA) HINFO, Hinfo, ISDN, Isdn, TXT, Txt;
    DNS_NULL_DATA Null;
    DNS_WKS_DATA WKS, Wks;
    DNS_AAAA_DATA AAAA;
    DNS_KEY_DATA KEY, Key;
    XNAME(DNS_SIG_DATA) SIG, Sig;
    XNAME(DNS_NXT_DATA) NXT, Nxt;
    XNAME(DNS_SRV_DATA) SRV, Srv;
  } Data;
} XNAME(DNS_RECORD), *XNAME(PDNS_RECORD);

#endif /* __USE_W32API */
#endif /* _WINDNS_STRUCT_DEFINITIONS */

