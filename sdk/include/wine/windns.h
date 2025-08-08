/*
 * DNS support
 *
 * Copyright (C) 2006 Matthew Kehrer
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINDNS_H
#define __WINE_WINDNS_H

#ifdef __cplusplus
extern "C" {
#endif

#define DNS_TYPE_ZERO       0x0000
#define DNS_TYPE_A          0x0001
#define DNS_TYPE_NS         0x0002
#define DNS_TYPE_MD         0x0003
#define DNS_TYPE_MF         0x0004
#define DNS_TYPE_CNAME      0x0005
#define DNS_TYPE_SOA        0x0006
#define DNS_TYPE_MB         0x0007
#define DNS_TYPE_MG         0x0008
#define DNS_TYPE_MR         0x0009
#define DNS_TYPE_NULL       0x000a
#define DNS_TYPE_WKS        0x000b
#define DNS_TYPE_PTR        0x000c
#define DNS_TYPE_HINFO      0x000d
#define DNS_TYPE_MINFO      0x000e
#define DNS_TYPE_MX         0x000f
#define DNS_TYPE_TEXT       0x0010
#define DNS_TYPE_RP         0x0011
#define DNS_TYPE_AFSDB      0x0012
#define DNS_TYPE_X25        0x0013
#define DNS_TYPE_ISDN       0x0014
#define DNS_TYPE_RT         0x0015
#define DNS_TYPE_NSAP       0x0016
#define DNS_TYPE_NSAPPTR    0x0017
#define DNS_TYPE_SIG        0x0018
#define DNS_TYPE_KEY        0x0019
#define DNS_TYPE_PX         0x001a
#define DNS_TYPE_GPOS       0x001b
#define DNS_TYPE_AAAA       0x001c
#define DNS_TYPE_LOC        0x001d
#define DNS_TYPE_NXT        0x001e
#define DNS_TYPE_EID        0x001f
#define DNS_TYPE_NIMLOC     0x0020
#define DNS_TYPE_SRV        0x0021
#define DNS_TYPE_ATMA       0x0022
#define DNS_TYPE_NAPTR      0x0023
#define DNS_TYPE_KX         0x0024
#define DNS_TYPE_CERT       0x0025
#define DNS_TYPE_A6         0x0026
#define DNS_TYPE_DNAME      0x0027
#define DNS_TYPE_SINK       0x0028
#define DNS_TYPE_OPT        0x0029
#define DNS_TYPE_UINFO      0x0064
#define DNS_TYPE_UID        0x0065
#define DNS_TYPE_GID        0x0066
#define DNS_TYPE_UNSPEC     0x0067
#define DNS_TYPE_ADDRS      0x00f8
#define DNS_TYPE_TKEY       0x00f9
#define DNS_TYPE_TSIG       0x00fa
#define DNS_TYPE_IXFR       0x00fb
#define DNS_TYPE_AXFR       0x00fc
#define DNS_TYPE_MAILB      0x00fd
#define DNS_TYPE_MAILA      0x00fe
#define DNS_TYPE_ALL        0x00ff
#define DNS_TYPE_ANY        0x00ff

#define DNS_TYPE_WINS       0xff01
#define DNS_TYPE_WINSR      0xff02
#define DNS_TYPE_NBSTAT     (DNS_TYPE_WINSR)

#define DNS_QUERY_STANDARD                  0x00000000
#define DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE 0x00000001
#define DNS_QUERY_USE_TCP_ONLY              0x00000002
#define DNS_QUERY_NO_RECURSION              0x00000004
#define DNS_QUERY_BYPASS_CACHE              0x00000008
#define DNS_QUERY_NO_WIRE_QUERY             0x00000010
#define DNS_QUERY_NO_LOCAL_NAME             0x00000020
#define DNS_QUERY_NO_HOSTS_FILE             0x00000040
#define DNS_QUERY_NO_NETBT                  0x00000080
#define DNS_QUERY_WIRE_ONLY                 0x00000100
#define DNS_QUERY_RETURN_MESSAGE            0x00000200
#define DNS_QUERY_MULTICAST_ONLY            0x00000400
#define DNS_QUERY_NO_MULTICAST              0x00000800
#define DNS_QUERY_TREAT_AS_FQDN             0x00001000
#define DNS_QUERY_ADDRCONFIG                0x00002000
#define DNS_QUERY_DUAL_ADDR                 0x00004000
#define DNS_QUERY_DONT_RESET_TTL_VALUES     0x00100000
#define DNS_QUERY_DISABLE_IDN_ENCODING      0x00200000
#define DNS_QUERY_APPEND_MULTILABEL         0x00800000
#define DNS_QUERY_DNSSEC_OK                 0x01000000
#define DNS_QUERY_DNSSEC_CHECKING_DISABLED  0x02000000
#define DNS_QUERY_RESERVED                  0xff000000

#define INLINE_WORD_FLIP(out, in) { WORD _in = (in); (out) = (_in << 8) | (_in >> 8); }
#define INLINE_HTONS(out, in) INLINE_WORD_FLIP(out, in)
#define INLINE_NTOHS(out, in) INLINE_WORD_FLIP(out, in)

#define DNS_BYTE_FLIP_HEADER_COUNTS(header) { \
        DNS_HEADER *_head = (header); \
        INLINE_HTONS( _head->Xid, _head->Xid ); \
        INLINE_HTONS( _head->QuestionCount, _head->QuestionCount ); \
        INLINE_HTONS( _head->AnswerCount, _head->AnswerCount ); \
        INLINE_HTONS( _head->NameServerCount, _head->NameServerCount ); \
        INLINE_HTONS( _head->AdditionalCount, _head->AdditionalCount ); }

typedef enum _DNS_NAME_FORMAT
{
    DnsNameDomain,
    DnsNameDomainLabel,
    DnsNameHostnameFull,
    DnsNameHostnameLabel,
    DnsNameWildcard,
    DnsNameSrvRecord
} DNS_NAME_FORMAT;

typedef enum _DNS_FREE_TYPE
{
    DnsFreeFlat,
    DnsFreeRecordList,
    DnsFreeParsedMessageFields
} DNS_FREE_TYPE;

typedef enum _DNS_CHARSET
{
    DnsCharSetUnknown,
    DnsCharSetUnicode,
    DnsCharSetUtf8,
    DnsCharSetAnsi
} DNS_CHARSET;

typedef enum _DNS_CONFIG_TYPE
{
    DnsConfigPrimaryDomainName_W,
    DnsConfigPrimaryDomainName_A,
    DnsConfigPrimaryDomainName_UTF8,
    DnsConfigAdapterDomainName_W,
    DnsConfigAdapterDomainName_A,
    DnsConfigAdapterDomainName_UTF8,
    DnsConfigDnsServerList,
    DnsConfigSearchList,
    DnsConfigAdapterInfo,
    DnsConfigPrimaryHostNameRegistrationEnabled,
    DnsConfigAdapterHostNameRegistrationEnabled,
    DnsConfigAddressRegistrationMaxCount,
    DnsConfigHostName_W,
    DnsConfigHostName_A,
    DnsConfigHostName_UTF8,
    DnsConfigFullHostName_W,
    DnsConfigFullHostName_A,
    DnsConfigFullHostName_UTF8,

    /* These are undocumented and return a DNS_ADDR_ARRAY */
    DnsConfigDnsServersUnspec = 4144,
    DnsConfigDnsServersIpv4,
    DnsConfigDnsServersIpv6
} DNS_CONFIG_TYPE;

typedef enum _DnsSection
{
    DnsSectionQuestion,
    DnsSectionAnswer,
    DnsSectionAuthority,
    DnsSectionAddtional /* Not a typo, as per Microsoft's headers */
} DNS_SECTION;

typedef LONG DNS_STATUS, *PDNS_STATUS;
typedef DWORD IP4_ADDRESS, *PIP4_ADDRESS;

typedef struct
{
    DWORD IP6Dword[4];
} IP6_ADDRESS, *PIP6_ADDRESS, DNS_IP6_ADDRESS, *PDNS_IP6_ADDRESS;

#define SIZEOF_IP4_ADDRESS                   4
#define IP4_ADDRESS_STRING_LENGTH           16
#define IP6_ADDRESS_STRING_LENGTH           65
#define DNS_ADDRESS_STRING_LENGTH           IP6_ADDRESS_STRING_LENGTH
#define IP4_ADDRESS_STRING_BUFFER_LENGTH    IP4_ADDRESS_STRING_LENGTH
#define IP6_ADDRESS_STRING_BUFFER_LENGTH    IP6_ADDRESS_STRING_LENGTH

#define DNS_MAX_NAME_LENGTH                 255
#define DNS_MAX_LABEL_LENGTH                63
#define DNS_MAX_NAME_BUFFER_LENGTH          (DNS_MAX_NAME_LENGTH + 1)
#define DNS_MAX_LABEL_BUFFER_LENGTH         (DNS_MAX_LABEL_LENGTH + 1)

typedef struct _IP4_ARRAY
{
    DWORD AddrCount;
    IP4_ADDRESS AddrArray[1];
} IP4_ARRAY, *PIP4_ARRAY;

#define DNS_OPCODE_QUERY          0
#define DNS_OPCODE_IQUERY         1
#define DNS_OPCODE_SERVER_STATUS  2
#define DNS_OPCODE_UNKNOWN        3
#define DNS_OPCODE_NOTIFY         4
#define DNS_OPCODE_UPDATE         5

#define DNS_RCODE_NOERROR    0
#define DNS_RCODE_FORMERR    1
#define DNS_RCODE_SERVFAIL   2
#define DNS_RCODE_NXDOMAIN   3
#define DNS_RCODE_NOTIMPL    4
#define DNS_RCODE_REFUSED    5
#define DNS_RCODE_YXDOMAIN   6
#define DNS_RCODE_YXRRSET    7
#define DNS_RCODE_NXRRSET    8
#define DNS_RCODE_NOTAUTH    9
#define DNS_RCODE_NOTZONE    10
#define DNS_RCODE_MAX        15
#define DNS_RCODE_BADVERS    16
#define DNS_RCODE_BADSIG     16
#define DNS_RCODE_BADKEY     17
#define DNS_RCODE_BADTIME    18

#define DNS_RCODE_NO_ERROR         DNS_RCODE_NOERROR
#define DNS_RCODE_FORMAT_ERROR     DNS_RCODE_FORMERR
#define DNS_RCODE_SERVER_FAILURE   DNS_RCODE_SERVFAIL
#define DNS_RCODE_NAME_ERROR       DNS_RCODE_NXDOMAIN
#define DNS_RCODE_NOT_IMPLEMENTED  DNS_RCODE_NOTIMPL

#include <pshpack1.h>
typedef struct _DNS_HEADER
{
    WORD Xid;
    BYTE RecursionDesired:1;
    BYTE Truncation:1;
    BYTE Authoritative:1;
    BYTE Opcode:4;
    BYTE IsResponse:1;
    BYTE ResponseCode:4;
    BYTE CheckingDisabled:1;
    BYTE AuthenticatedData:1;
    BYTE Reserved:1;
    BYTE RecursionAvailable:1;
    WORD QuestionCount;
    WORD AnswerCount;
    WORD NameServerCount;
    WORD AdditionalCount;
} DNS_HEADER, *PDNS_HEADER;
#include <poppack.h>

typedef struct _DNS_MESSAGE_BUFFER
{
    DNS_HEADER MessageHead;
    CHAR MessageBody[1];
} DNS_MESSAGE_BUFFER, *PDNS_MESSAGE_BUFFER;

typedef struct
{
    IP4_ADDRESS IpAddress;
} DNS_A_DATA, *PDNS_A_DATA;

typedef struct _DnsRecordFlags
{
    DWORD Section :2;
    DWORD Delete :1;
    DWORD CharSet :2;
    DWORD Unused :3;
    DWORD Reserved :24;
} DNS_RECORD_FLAGS;

typedef struct
{
    PSTR  pNamePrimaryServer;
    PSTR  pNameAdministrator;
    DWORD dwSerialNo;
    DWORD dwRefresh;
    DWORD dwRetry;
    DWORD dwExpire;
    DWORD dwDefaultTtl;
} DNS_SOA_DATAA, *PDNS_SOA_DATAA;

typedef struct
{
    PWSTR pNamePrimaryServer;
    PWSTR pNameAdministrator;
    DWORD dwSerialNo;
    DWORD dwRefresh;
    DWORD dwRetry;
    DWORD dwExpire;
    DWORD dwDefaultTtl;
} DNS_SOA_DATAW, *PDNS_SOA_DATAW;

DECL_WINELIB_TYPE_AW(DNS_SOA_DATA)
DECL_WINELIB_TYPE_AW(PDNS_SOA_DATA)

typedef struct
{
    PSTR pNameHost;
} DNS_PTR_DATAA, *PDNS_PTR_DATAA;

typedef struct
{
    PWSTR pNameHost;
} DNS_PTR_DATAW, *PDNS_PTR_DATAW;

DECL_WINELIB_TYPE_AW(DNS_PTR_DATA)
DECL_WINELIB_TYPE_AW(PDNS_PTR_DATA)

typedef struct
{
    PSTR pNameMailbox;
    PSTR pNameErrorsMailbox;
} DNS_MINFO_DATAA, *PDNS_MINFO_DATAA;

typedef struct
{
    PWSTR pNameMailbox;
    PWSTR pNameErrorsMailbox;
} DNS_MINFO_DATAW, *PDNS_MINFO_DATAW;

DECL_WINELIB_TYPE_AW(DNS_MINFO_DATA)
DECL_WINELIB_TYPE_AW(PDNS_MINFO_DATA)

typedef struct
{
    PSTR pNameExchange;
    WORD wPreference;
    WORD Pad;
} DNS_MX_DATAA, *PDNS_MX_DATAA;

typedef struct
{
    PWSTR pNameExchange;
    WORD wPreference;
    WORD Pad;
} DNS_MX_DATAW, *PDNS_MX_DATAW;

DECL_WINELIB_TYPE_AW(DNS_MX_DATA)
DECL_WINELIB_TYPE_AW(PDNS_MX_DATA)

typedef struct
{
    DWORD dwStringCount;
    PSTR pStringArray[1];
} DNS_TXT_DATAA, *PDNS_TXT_DATAA;

typedef struct
{
    DWORD dwStringCount;
    PWSTR pStringArray[1];
} DNS_TXT_DATAW, *PDNS_TXT_DATAW;

DECL_WINELIB_TYPE_AW(DNS_TXT_DATA)
DECL_WINELIB_TYPE_AW(PDNS_TXT_DATA)

typedef struct
{
    DWORD dwByteCount;
    BYTE Data[1];
} DNS_NULL_DATA, *PDNS_NULL_DATA;

typedef struct
{
    IP4_ADDRESS IpAddress;
    UCHAR chProtocol;
    BYTE BitMask[1];
} DNS_WKS_DATA, *PDNS_WKS_DATA;

typedef struct
{
    DNS_IP6_ADDRESS Ip6Address;
} DNS_AAAA_DATA, *PDNS_AAAA_DATA;

typedef struct
{
    WORD wFlags;
    BYTE chProtocol;
    BYTE chAlgorithm;
    WORD wKeyLength;
    WORD wPad;
    BYTE Key[1];
} DNS_KEY_DATA, *PDNS_KEY_DATA;

typedef struct
{
    WORD wVersion;
    WORD wSize;
    WORD wHorPrec;
    WORD wVerPrec;
    DWORD dwLatitude;
    DWORD dwLongitude;
    DWORD dwAltitude;
} DNS_LOC_DATA, *PDNS_LOC_DATA;

typedef struct
{
    WORD wTypeCovered;
    BYTE chAlgorithm;
    BYTE chLabelCount;
    DWORD dwOriginalTtl;
    DWORD dwExpiration;
    DWORD dwTimeSigned;
    WORD wKeyTag;
    WORD wSignatureLength;
    PSTR pNameSigner;
    BYTE Signature[1];
} DNS_SIG_DATAA, *PDNS_SIG_DATAA;

typedef struct
{
    WORD wTypeCovered;
    BYTE chAlgorithm;
    BYTE chLabelCount;
    DWORD dwOriginalTtl;
    DWORD dwExpiration;
    DWORD dwTimeSigned;
    WORD wKeyTag;
    WORD wSignatureLength;
    PWSTR pNameSigner;
    BYTE Signature[1];
} DNS_SIG_DATAW, *PDNS_SIG_DATAW;

DECL_WINELIB_TYPE_AW(DNS_SIG_DATA)
DECL_WINELIB_TYPE_AW(PDNS_SIG_DATA)

#define DNS_ATMA_MAX_ADDR_LENGTH 20

typedef struct
{
    BYTE AddressType;
    BYTE Address[DNS_ATMA_MAX_ADDR_LENGTH];
} DNS_ATMA_DATA, *PDNS_ATMA_DATA;

typedef struct
{
    PSTR pNameNext;
    WORD wNumTypes;
    WORD wTypes[1];
} DNS_NXT_DATAA, *PDNS_NXT_DATAA;

typedef struct
{
    PWSTR pNameNext;
    WORD wNumTypes;
    WORD wTypes[1];
} DNS_NXT_DATAW, *PDNS_NXT_DATAW;

DECL_WINELIB_TYPE_AW(DNS_NXT_DATA)
DECL_WINELIB_TYPE_AW(PDNS_NXT_DATA)

typedef struct
{
    PSTR pNameTarget;
    WORD wPriority;
    WORD wWeight;
    WORD wPort;
    WORD Pad;
} DNS_SRV_DATAA, *PDNS_SRV_DATAA;

typedef struct
{
    PWSTR pNameTarget;
    WORD wPriority;
    WORD wWeight;
    WORD wPort;
    WORD Pad;
} DNS_SRV_DATAW, *PDNS_SRV_DATAW;

DECL_WINELIB_TYPE_AW(DNS_SRV_DATA)
DECL_WINELIB_TYPE_AW(PDNS_SRV_DATA)

typedef struct
{
    PSTR pNameAlgorithm;
    PBYTE pAlgorithmPacket;
    PBYTE pKey;
    PBYTE pOtherData;
    DWORD dwCreateTime;
    DWORD dwExpireTime;
    WORD wMode;
    WORD wError;
    WORD wKeyLength;
    WORD wOtherLength;
    UCHAR cAlgNameLength;
    BOOL bPacketPointers;
} DNS_TKEY_DATAA, *PDNS_TKEY_DATAA;

typedef struct
{
    PWSTR pNameAlgorithm;
    PBYTE pAlgorithmPacket;
    PBYTE pKey;
    PBYTE pOtherData;
    DWORD dwCreateTime;
    DWORD dwExpireTime;
    WORD wMode;
    WORD wError;
    WORD wKeyLength;
    WORD wOtherLength;
    UCHAR cAlgNameLength;
    BOOL bPacketPointers;
} DNS_TKEY_DATAW, *PDNS_TKEY_DATAW;

DECL_WINELIB_TYPE_AW(DNS_TKEY_DATA)
DECL_WINELIB_TYPE_AW(PDNS_TKEY_DATA)

typedef struct
{
    PSTR pNameAlgorithm;
    PBYTE pAlgorithmPacket;
    PBYTE pSignature;
    PBYTE pOtherData;
    LONGLONG i64CreateTime;
    WORD wFudgeTime;
    WORD wOriginalXid;
    WORD wError;
    WORD wSigLength;
    WORD wOtherLength;
    UCHAR cAlgNameLength;
    BOOL bPacketPointers;
} DNS_TSIG_DATAA, *PDNS_TSIG_DATAA;

typedef struct
{
    PWSTR pNameAlgorithm;
    PBYTE pAlgorithmPacket;
    PBYTE pSignature;
    PBYTE pOtherData;
    LONGLONG i64CreateTime;
    WORD wFudgeTime;
    WORD wOriginalXid;
    WORD wError;
    WORD wSigLength;
    WORD wOtherLength;
    UCHAR cAlgNameLength;
    BOOL bPacketPointers;
} DNS_TSIG_DATAW, *PDNS_TSIG_DATAW;

typedef struct
{
    DWORD dwMappingFlag;
    DWORD dwLookupTimeout;
    DWORD dwCacheTimeout;
    DWORD cWinsServerCount;
    IP4_ADDRESS WinsServers[1];
} DNS_WINS_DATA, *PDNS_WINS_DATA;

typedef struct
{
    DWORD dwMappingFlag;
    DWORD dwLookupTimeout;
    DWORD dwCacheTimeout;
    PSTR pNameResultDomain;
} DNS_WINSR_DATAA, *PDNS_WINSR_DATAA;

typedef struct
{
    DWORD dwMappingFlag;
    DWORD dwLookupTimeout;
    DWORD dwCacheTimeout;
    PWSTR pNameResultDomain;
} DNS_WINSR_DATAW, *PDNS_WINSR_DATAW;

DECL_WINELIB_TYPE_AW(DNS_WINSR_DATA)
DECL_WINELIB_TYPE_AW(PDNS_WINSR_DATA)

typedef struct
{
    WORD wDataLength;
    WORD wPad;
    BYTE Data[1];
}
DNS_OPT_DATA, *PDNS_OPT_DATA;

typedef struct _DnsRecordA
{
    struct _DnsRecordA *pNext;
    PSTR pName;
    WORD wType;
    WORD wDataLength;
    union
    {
        DWORD DW;
        DNS_RECORD_FLAGS S;
    } Flags;
    DWORD dwTtl;
    DWORD dwReserved;
    union
    {
        DNS_A_DATA A;
        DNS_SOA_DATAA SOA, Soa;
        DNS_PTR_DATAA PTR, Ptr, NS, Ns, CNAME, Cname, MB, Mb, MD, Md, MF, Mf, MG, Mg, MR, Mr;
        DNS_MINFO_DATAA MINFO, Minfo, RP, Rp;
        DNS_MX_DATAA MX, Mx, AFSDB, Afsdb, RT, Rt;
        DNS_TXT_DATAA HINFO, Hinfo, ISDN, Isdn, TXT, Txt, X25;
        DNS_NULL_DATA Null;
        DNS_WKS_DATA WKS, Wks;
        DNS_AAAA_DATA AAAA;
        DNS_KEY_DATA KEY, Key;
        DNS_SIG_DATAA SIG, Sig;
        DNS_ATMA_DATA ATMA, Atma;
        DNS_NXT_DATAA NXT, Nxt;
        DNS_SRV_DATAA SRV, Srv;
        DNS_TKEY_DATAA TKEY, Tkey;
        DNS_TSIG_DATAA TSIG, Tsig;
        DNS_WINS_DATA WINS, Wins;
        DNS_WINSR_DATAA WINSR, WinsR, NBSTAT, Nbstat;
        DNS_OPT_DATA OPT, Opt;
    } Data;
} DNS_RECORDA, *PDNS_RECORDA;

typedef struct _DnsRecordW
{
    struct _DnsRecordW *pNext;
    PWSTR pName;
    WORD wType;
    WORD wDataLength;
    union
    {
        DWORD DW;
        DNS_RECORD_FLAGS S;
    } Flags;
    DWORD dwTtl;
    DWORD dwReserved;
    union
    {
        DNS_A_DATA A;
        DNS_SOA_DATAW SOA, Soa;
        DNS_PTR_DATAW PTR, Ptr, NS, Ns, CNAME, Cname, MB, Mb, MD, Md, MF, Mf, MG, Mg, MR, Mr;
        DNS_MINFO_DATAW MINFO, Minfo, RP, Rp;
        DNS_MX_DATAW MX, Mx, AFSDB, Afsdb, RT, Rt;
        DNS_TXT_DATAW HINFO, Hinfo, ISDN, Isdn, TXT, Txt, X25;
        DNS_NULL_DATA Null;
        DNS_WKS_DATA WKS, Wks;
        DNS_AAAA_DATA AAAA;
        DNS_KEY_DATA KEY, Key;
        DNS_SIG_DATAW SIG, Sig;
        DNS_ATMA_DATA ATMA, Atma;
        DNS_NXT_DATAW NXT, Nxt;
        DNS_SRV_DATAW SRV, Srv;
        DNS_TKEY_DATAW TKEY, Tkey;
        DNS_TSIG_DATAW TSIG, Tsig;
        DNS_WINS_DATA WINS, Wins;
        DNS_WINSR_DATAW WINSR, WinsR, NBSTAT, Nbstat;
        DNS_OPT_DATA OPT, Opt;
    } Data;
} DNS_RECORDW, *PDNS_RECORDW;

#if defined(__WINESRC__) || defined(UNICODE)
typedef DNS_RECORDW DNS_RECORD;
typedef PDNS_RECORDW PDNS_RECORD;
#else
typedef DNS_RECORDA DNS_RECORD;
typedef PDNS_RECORDA PDNS_RECORD;
#endif

typedef struct _DnsRRSet
{
    PDNS_RECORD pFirstRR;
    PDNS_RECORD pLastRR;
} DNS_RRSET, *PDNS_RRSET;

#define DNS_RRSET_INIT( rrset )                          \
{                                                        \
    PDNS_RRSET  _prrset = &(rrset);                      \
    _prrset->pFirstRR = NULL;                            \
    _prrset->pLastRR = (PDNS_RECORD) &_prrset->pFirstRR; \
}

#define DNS_RRSET_ADD( rrset, pnewRR ) \
{                                      \
    PDNS_RRSET  _prrset = &(rrset);    \
    PDNS_RECORD _prrnew = (pnewRR);    \
    _prrset->pLastRR->pNext = _prrnew; \
    _prrset->pLastRR = _prrnew;        \
}

#define DNS_RRSET_TERMINATE( rrset ) \
{                                    \
    PDNS_RRSET  _prrset = &(rrset);  \
    _prrset->pLastRR->pNext = NULL;  \
}

#define DNS_ADDR_MAX_SOCKADDR_LENGTH 32

#include <pshpack1.h>

typedef struct _DnsAddr
{
    char MaxSa[DNS_ADDR_MAX_SOCKADDR_LENGTH];
    union {
        DWORD DnsAddrUserDword[8];
    } Data;
} DNS_ADDR, *PDNS_ADDR;

typedef struct _DnsAddrArray
{
    DWORD MaxCount;
    DWORD AddrCount;
    DWORD Tag;
    WORD Family;
    WORD WordReserved;
    DWORD Flags;
    DWORD MatchFlag;
    DWORD Reserved1;
    DWORD Reserved2;
    DNS_ADDR AddrArray[1];
} DNS_ADDR_ARRAY, *PDNS_ADDR_ARRAY;

#include <poppack.h>

#define DNS_QUERY_RESULTS_VERSION1  0x1

typedef struct _DNS_QUERY_RESULT
{
    ULONG Version;
    DNS_STATUS QueryStatus;
    ULONG64 QueryOptions;
    DNS_RECORD *pQueryRecords;
    void *Reserved;
} DNS_QUERY_RESULT, *PDNS_QUERY_RESULT;

typedef void WINAPI DNS_QUERY_COMPLETION_ROUTINE(void*,DNS_QUERY_RESULT*);
typedef DNS_QUERY_COMPLETION_ROUTINE *PDNS_QUERY_COMPLETION_ROUTINE;

#define DNS_QUERY_REQUEST_VERSION1  0x1

typedef struct _DNS_QUERY_REQUEST
{
    ULONG Version;
    const WCHAR *QueryName;
    WORD QueryType;
    ULONG64 QueryOptions;
    PDNS_ADDR_ARRAY pDnsServerList;
    ULONG InterfaceIndex;
    PDNS_QUERY_COMPLETION_ROUTINE pQueryCompletionCallback;
    void *pQueryContext;
} DNS_QUERY_REQUEST, *PDNS_QUERY_REQUEST;

typedef struct _DNS_QUERY_CANCEL
{
    char Reserved[32];
} DNS_QUERY_CANCEL, *PDNS_QUERY_CANCEL;

typedef struct _DNS_CACHE_ENTRY
{
    struct _DNS_CACHE_ENTRY* Next;
    const WCHAR *Name;
    WORD Type;
    WORD DataLength;
    ULONG Flags;
} DNS_CACHE_ENTRY, *PDNS_CACHE_ENTRY;

typedef void WINAPI DNS_SERVICE_BROWSE_CALLBACK(DWORD, void *, PDNS_RECORD);
typedef DNS_SERVICE_BROWSE_CALLBACK *PDNS_SERVICE_BROWSE_CALLBACK;

typedef struct _DNS_SERVICE_BROWSE_REQUEST
{
    ULONG Version;
    ULONG InterfaceIndex;
    const WCHAR *QueryName;
    union
    {
        PDNS_SERVICE_BROWSE_CALLBACK pBrowseCallback;
        DNS_QUERY_COMPLETION_ROUTINE *pBrowseCallbackV2;
    };
    void *pQueryContext;
} DNS_SERVICE_BROWSE_REQUEST, *PDNS_SERVICE_BROWSE_REQUEST;

typedef struct _DNS_SERVICE_CANCEL
{
    void *reserved;
} DNS_SERVICE_CANCEL, *PDNS_SERVICE_CANCEL;

DNS_STATUS WINAPI DnsAcquireContextHandle_A(DWORD,PVOID,PHANDLE);
DNS_STATUS WINAPI DnsAcquireContextHandle_W(DWORD,PVOID,PHANDLE);
#define DnsAcquireContextHandle WINELIB_NAME_AW(DnsAcquireContextHandle_)
DNS_STATUS WINAPI DnsExtractRecordsFromMessage_W(PDNS_MESSAGE_BUFFER,WORD,PDNS_RECORDW*);
DNS_STATUS WINAPI DnsExtractRecordsFromMessage_UTF8(PDNS_MESSAGE_BUFFER,WORD,PDNS_RECORDA*);
VOID WINAPI DnsFree(PVOID,DNS_FREE_TYPE);
DNS_STATUS WINAPI DnsModifyRecordsInSet_A(PDNS_RECORDA,PDNS_RECORDA,DWORD,HANDLE,PVOID,PVOID);
DNS_STATUS WINAPI DnsModifyRecordsInSet_W(PDNS_RECORDW,PDNS_RECORDW,DWORD,HANDLE,PVOID,PVOID);
DNS_STATUS WINAPI DnsModifyRecordsInSet_UTF8(PDNS_RECORDA,PDNS_RECORDA,DWORD,HANDLE,PVOID,PVOID);
#define DnsModifyRecordsInSet WINELIB_NAME_AW(DnsModifyRecordsInSet_)
BOOL WINAPI DnsNameCompare_A(PCSTR,PCSTR);
BOOL WINAPI DnsNameCompare_W(PCWSTR,PCWSTR);
#define DnsNameCompare WINELIB_NAME_AW(DnsNameCompare_)
DNS_STATUS WINAPI DnsQuery_A(PCSTR,WORD,DWORD,PVOID,PDNS_RECORDA*,PVOID*);
DNS_STATUS WINAPI DnsQuery_W(PCWSTR,WORD,DWORD,PVOID,PDNS_RECORDW*,PVOID*);
DNS_STATUS WINAPI DnsQuery_UTF8(PCSTR,WORD,DWORD,PVOID,PDNS_RECORDA*,PVOID*);
#define DnsQuery WINELIB_NAME_AW(DnsQuery_)
DNS_STATUS WINAPI DnsQueryEx(DNS_QUERY_REQUEST*,DNS_QUERY_RESULT*,DNS_QUERY_CANCEL*);
DNS_STATUS WINAPI DnsCancelQuery(DNS_QUERY_CANCEL*);
DNS_STATUS WINAPI DnsQueryConfig(DNS_CONFIG_TYPE,DWORD,PCWSTR,PVOID,PVOID,PDWORD);
BOOL WINAPI DnsRecordCompare(PDNS_RECORD,PDNS_RECORD);
PDNS_RECORD WINAPI DnsRecordCopyEx(PDNS_RECORD,DNS_CHARSET,DNS_CHARSET);
VOID WINAPI DnsRecordListFree(PDNS_RECORD,DNS_FREE_TYPE);
BOOL WINAPI DnsRecordSetCompare(PDNS_RECORD,PDNS_RECORD,PDNS_RECORD*,PDNS_RECORD*);
PDNS_RECORD WINAPI DnsRecordSetCopyEx(PDNS_RECORD,DNS_CHARSET,DNS_CHARSET);
PDNS_RECORD WINAPI DnsRecordSetDetach(PDNS_RECORD);
void WINAPI DnsReleaseContextHandle(HANDLE);
DNS_STATUS WINAPI DnsReplaceRecordSetA(PDNS_RECORDA,DWORD,HANDLE,PVOID,PVOID);
DNS_STATUS WINAPI DnsReplaceRecordSetW(PDNS_RECORDW,DWORD,HANDLE,PVOID,PVOID);
DNS_STATUS WINAPI DnsReplaceRecordSetUTF8(PDNS_RECORDA,DWORD,HANDLE,PVOID,PVOID);
#define DnsReplaceRecordSet WINELIB_NAME_AW(DnsReplaceRecordSet)
DNS_STATUS WINAPI DnsServiceBrowse(PDNS_SERVICE_BROWSE_REQUEST, PDNS_SERVICE_CANCEL);
DNS_STATUS WINAPI DnsValidateName_A(PCSTR,DNS_NAME_FORMAT);
DNS_STATUS WINAPI DnsValidateName_W(PCWSTR, DNS_NAME_FORMAT);
DNS_STATUS WINAPI DnsValidateName_UTF8(PCSTR,DNS_NAME_FORMAT);
#define DnsValidateName WINELIB_NAME_AW(DnsValidateName_)
BOOL WINAPI DnsWriteQuestionToBuffer_W(PDNS_MESSAGE_BUFFER,PDWORD,PCWSTR,WORD,WORD,BOOL);
BOOL WINAPI DnsWriteQuestionToBuffer_UTF8(PDNS_MESSAGE_BUFFER,PDWORD,PCSTR,WORD,WORD,BOOL);
BOOL WINAPI DnsGetCacheDataTable(PDNS_CACHE_ENTRY*);

#ifdef __cplusplus
}
#endif

#endif
