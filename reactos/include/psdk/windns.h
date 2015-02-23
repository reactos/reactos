#ifndef _WINDNS_H
#define _WINDNS_H

#ifdef __cplusplus
extern "C" {
#endif

#define DNS_QUERY_STANDARD 0x0
#define DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE 0x1
#define DNS_QUERY_USE_TCP_ONLY 0x2
#define DNS_QUERY_NO_RECURSION 0x4
#define DNS_QUERY_BYPASS_CACHE 0x8
#define DNS_QUERY_NO_WIRE_QUERY	0x10
#define DNS_QUERY_NO_LOCAL_NAME 0x20
#define DNS_QUERY_NO_HOSTS_FILE 0x40
#define DNS_QUERY_NO_NETBT 0x80
#define DNS_QUERY_TREAT_AS_FQDN 0x1000
#define DNS_QUERY_WIRE_ONLY 0x100
#define DNS_QUERY_RETURN_MESSAGE 0x200
#define DNS_QUERY_DONT_RESET_TTL_VALUES 0x100000
#define DNS_QUERY_RESERVED 0xff000000

#define DNS_UPDATE_SECURITY_USE_DEFAULT	0x0
#define DNS_UPDATE_SECURITY_OFF 0x10
#define DNS_UPDATE_SECURITY_ON 0x20
#define DNS_UPDATE_SECURITY_ONLY 0x100
#define DNS_UPDATE_CACHE_SECURITY_CONTEXT 0x200
#define DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT 0x400
#define DNS_UPDATE_FORCE_SECURITY_NEGO 0x800
#define DNS_UPDATE_RESERVED 0xffff0000

#define DNS_CONFIG_FLAG_ALLOC TRUE

#ifndef RC_INVOKE
typedef DWORD IP4_ADDRESS;
typedef _Return_type_success_(return == 0) DWORD DNS_STATUS;
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

typedef enum _DNS_CHARSET
{
	DnsCharSetUnknown,
	DnsCharSetUnicode,
	DnsCharSetUtf8,
	DnsCharSetAnsi
} DNS_CHARSET;
typedef enum
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
	DnsConfigFullHostName_UTF8
} DNS_CONFIG_TYPE;
typedef enum
{
	DnsFreeFlat = 0,
	DnsFreeRecordList,
	DnsFreeParsedMessageFields
} DNS_FREE_TYPE;
typedef enum _DNS_NAME_FORMAT
{
	DnsNameDomain,
	DnsNameDomainLabel,
	DnsNameHostnameFull,
	DnsNameHostnameLabel,
	DnsNameWildcard,
	DnsNameSrvRecord
} DNS_NAME_FORMAT;
typedef enum
{
	DnsSectionQuestion,
	DnsSectionAnswer,
	DnsSectionAuthority,
	DnsSectionAdditional
} DNS_SECTION;
typedef struct _IP4_ARRAY {
	DWORD AddrCount;
	IP4_ADDRESS AddrArray[1];
} IP4_ARRAY, *PIP4_ARRAY;
typedef struct {
	DWORD IP6Dword[4];
} IP6_ADDRESS, *PIP6_ADDRESS, DNS_IP6_ADDRESS, *PDNS_IP6_ADDRESS;

typedef struct _DNS_HEADER {
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

typedef struct _DNS_MESSAGE_BUFFER {
	DNS_HEADER MessageHead;
	CHAR MessageBody[1];
} DNS_MESSAGE_BUFFER, *PDNS_MESSAGE_BUFFER;
typedef struct _DnsRecordFlags {
	DWORD Section	:2;
	DWORD Delete	:1;
	DWORD CharSet	:2;
	DWORD Unused	:3;
	DWORD Reserved	:24;
} DNS_RECORD_FLAGS;
#define DNSREC_QUESTION 0
#define DNSREC_ANSWER 1
#define DNSREC_AUTHORITY 2
#define DNSREC_ADDITIONAL 3
typedef struct {
	IP4_ADDRESS IpAddress;
} DNS_A_DATA, *PDNS_A_DATA;
typedef struct {
	DNS_IP6_ADDRESS Ip6Address;
} DNS_AAAA_DATA, *PDNS_AAAA_DATA;
#define DNS_ATMA_MAX_ADDR_LENGTH 20
typedef struct {
	BYTE AddressType;
	BYTE Address[DNS_ATMA_MAX_ADDR_LENGTH];
} DNS_ATMA_DATA, *PDNS_ATMA_DATA;
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
typedef struct {
	LPSTR pNameMailbox;
	LPSTR pNameErrorsMailbox;
} DNS_MINFO_DATAA, *PDNS_MINFO_DATAA;
typedef struct {
	LPWSTR pNameMailbox;
	LPWSTR pNameErrorsMailbox;
} DNS_MINFO_DATAW, *PDNS_MINFO_DATAW;
typedef struct {
	LPSTR pNameExchange;
	WORD wPreference;
	WORD Pad;
} DNS_MX_DATAA, *PDNS_MX_DATAA;
typedef struct {
	LPWSTR pNameExchange;
	WORD wPreference;
	WORD Pad;
} DNS_MX_DATAW, *PDNS_MX_DATAW;
typedef struct {
	DWORD dwByteCount;
	BYTE Data[1];
} DNS_NULL_DATA, *PDNS_NULL_DATA;
typedef struct {
	LPSTR pNameNext;
	WORD wNumTypes;
	WORD wTypes[1];
} DNS_NXT_DATAA, *PDNS_NXT_DATAA;
typedef struct {
	LPWSTR pNameNext;
	WORD wNumTypes;
	WORD wTypes[1];
} DNS_NXT_DATAW, *PDNS_NXT_DATAW;
typedef struct {
	LPSTR pNameHost;
} DNS_PTR_DATAA, *PDNS_PTR_DATAA;
typedef struct {
	LPWSTR pNameHost;
} DNS_PTR_DATAW, *PDNS_PTR_DATAW;
typedef struct {
	LPSTR pNameSigner;
	WORD wTypeCovered;
	BYTE chAlgorithm;
	BYTE chLabelCount;
	DWORD dwOriginalTtl;
	DWORD dwExpiration;
	DWORD dwTimeSigned;
	WORD wKeyTag;
	WORD Pad;
	BYTE Signature[1];
} DNS_SIG_DATAA, *PDNS_SIG_DATAA;
typedef struct {
	LPWSTR pNameSigner;
	WORD wTypeCovered;
	BYTE chAlgorithm;
	BYTE chLabelCount;
	DWORD dwOriginalTtl;
	DWORD dwExpiration;
	DWORD dwTimeSigned;
	WORD wKeyTag;
	WORD Pad;
	BYTE Signature[1];
} DNS_SIG_DATAW, *PDNS_SIG_DATAW;
typedef struct {
	LPSTR pNamePrimaryServer;
	LPSTR pNameAdministrator;
	DWORD dwSerialNo;
	DWORD dwRefresh;
	DWORD dwRetry;
	DWORD dwExpire;
	DWORD dwDefaultTtl;
} DNS_SOA_DATAA, *PDNS_SOA_DATAA;
typedef struct {
	LPWSTR pNamePrimaryServer;
	LPWSTR pNameAdministrator;
	DWORD dwSerialNo;
	DWORD dwRefresh;
	DWORD dwRetry;
	DWORD dwExpire;
	DWORD dwDefaultTtl;
} DNS_SOA_DATAW, *PDNS_SOA_DATAW;
typedef struct {
	LPSTR pNameTarget;
	WORD wPriority;
	WORD wWeight;
	WORD wPort;
	WORD Pad;
} DNS_SRV_DATAA, *PDNS_SRV_DATAA;
typedef struct {
	LPWSTR pNameTarget;
	WORD wPriority;
	WORD wWeight;
	WORD wPort;
	WORD Pad;
} DNS_SRV_DATAW, *PDNS_SRV_DATAW;
typedef struct {
	DWORD dwStringCount;
	LPSTR pStringArray[1];
} DNS_TXT_DATAA, *PDNS_TXT_DATAA;
typedef struct {
	DWORD dwStringCount;
	LPWSTR pStringArray[1];
} DNS_TXT_DATAW, *PDNS_TXT_DATAW;
typedef struct {
	LPSTR pNameAlgorithm;
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
typedef struct {
	LPWSTR pNameAlgorithm;
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
typedef struct {
	LPSTR pNameAlgorithm;
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
typedef struct {
	LPWSTR pNameAlgorithm;
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
typedef struct {
	DWORD dwMappingFlag;
	DWORD dwLookupTimeout;
	DWORD dwCacheTimeout;
	DWORD cWinsServerCount;
	IP4_ADDRESS WinsServers[1];
} DNS_WINS_DATA, *PDNS_WINS_DATA;
typedef struct {
	DWORD dwMappingFlag;
	DWORD dwLookupTimeout;
	DWORD dwCacheTimeout;
	LPSTR pNameResultDomain;
} DNS_WINSR_DATAA, *PDNS_WINSR_DATAA;
typedef struct {
	DWORD dwMappingFlag;
	DWORD dwLookupTimeout;
	DWORD dwCacheTimeout;
	LPWSTR pNameResultDomain;
} DNS_WINSR_DATAW, *PDNS_WINSR_DATAW;
typedef struct _DNS_WIRE_QUESTION {
	WORD QuestionType;
	WORD QuestionClass;
} DNS_WIRE_QUESTION, *PDNS_WIRE_QUESTION;
typedef struct _DNS_WIRE_RECORD {
	WORD RecordType;
	WORD RecordClass;
	DWORD TimeToLive;
	WORD DataLength;
} DNS_WIRE_RECORD, *PDNS_WIRE_RECORD;
typedef struct {
	IP4_ADDRESS IpAddress;
	UCHAR chProtocol;
	BYTE BitMask[1];
} DNS_WKS_DATA, *PDNS_WKS_DATA;
typedef struct _DnsRecordA {
	struct _DnsRecordA* pNext;
	LPSTR pName;
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
		DNS_SOA_DATAA SOA, Soa;
		DNS_PTR_DATAA PTR, Ptr, NS, Ns, CNAME, Cname, MB, Mb,
			      MD, Md, MF, Mf, MG, Mg, MR, Mr;
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
	} Data;
} DNS_RECORDA, *PDNS_RECORDA;
typedef struct _DnsRecordW {
	struct _DnsRecordW* pNext;
	LPWSTR pName;
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
		DNS_SOA_DATAW SOA, Soa;
		DNS_PTR_DATAW PTR, Ptr, NS, Ns, CNAME, Cname, MB, Mb,
			      MD, Md, MF, Mf, MG, Mg, MR, Mr;
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
	} Data;
} DNS_RECORDW, *PDNS_RECORDW;

#ifdef UNICODE
#define DNS_RECORD DNS_RECORDW
#define PDNS_RECORD PDNS_RECORDW
#else
#define DNS_RECORD DNS_RECORDA
#define PDNS_RECORD PDNS_RECORDA
#endif

typedef struct _DnsRRSet {
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

DNS_STATUS
WINAPI
DnsAcquireContextHandle_A(
  _In_ DWORD CredentialFlags,
  _In_opt_ PVOID Credentials,
  _Outptr_ PHANDLE pContext);

DNS_STATUS
WINAPI
DnsAcquireContextHandle_W(
  _In_ DWORD CredentialFlags,
  _In_opt_ PVOID Credentials,
  _Outptr_ PHANDLE pContext);

DNS_STATUS
WINAPI
DnsExtractRecordsFromMessage_W(
  _In_ PDNS_MESSAGE_BUFFER pDnsBuffer,
  _In_ WORD wMessageLength,
  _Outptr_ PDNS_RECORD *ppRecord);

DNS_STATUS
WINAPI
DnsExtractRecordsFromMessage_UTF8(
  _In_ PDNS_MESSAGE_BUFFER pDnsBuffer,
  _In_ WORD wMessageLength,
  _Outptr_ PDNS_RECORD *ppRecord);

DNS_STATUS
WINAPI
DnsModifyRecordsInSet_A(
  _In_opt_ PDNS_RECORD pAddRecords,
  _In_opt_ PDNS_RECORD pDeleteRecords,
  _In_ DWORD Options,
  _In_opt_ HANDLE hCredentials,
  _Inout_opt_ PIP4_ARRAY pExtraList,
  _Inout_opt_ PVOID pReserved);

DNS_STATUS
WINAPI
DnsModifyRecordsInSet_W(
  _In_opt_ PDNS_RECORD pAddRecords,
  _In_opt_ PDNS_RECORD pDeleteRecords,
  _In_ DWORD Options,
  _In_opt_ HANDLE hCredentials,
  _Inout_opt_ PIP4_ARRAY pExtraList,
  _Inout_opt_ PVOID pReserved);

DNS_STATUS
WINAPI
DnsModifyRecordsInSet_UTF8(
  _In_opt_ PDNS_RECORD pAddRecords,
  _In_opt_ PDNS_RECORD pDeleteRecords,
  _In_ DWORD Options,
  _In_opt_ HANDLE hCredentials,
  _Inout_opt_ PIP4_ARRAY pExtraList,
  _Inout_opt_ PVOID pReserved);

BOOL WINAPI DnsNameCompare_A(_In_ PCSTR, _In_ PCSTR);
BOOL WINAPI DnsNameCompare_W(_In_ PCWSTR, _In_ PCWSTR);

DNS_STATUS
WINAPI
DnsQuery_A(
  _In_ PCSTR pszName,
  _In_ WORD wType,
  _In_ DWORD Options,
  _Inout_opt_ PIP4_ARRAY pExtra,
  _Outptr_result_maybenull_ PDNS_RECORD *ppQueryResults,
  _Outptr_opt_result_maybenull_ PVOID *pReserved);

DNS_STATUS
WINAPI
DnsQuery_W(
  _In_ PCWSTR pszName,
  _In_ WORD wType,
  _In_ DWORD Options,
  _Inout_opt_ PIP4_ARRAY pExtra,
  _Outptr_result_maybenull_ PDNS_RECORD *ppQueryResults,
  _Outptr_opt_result_maybenull_ PVOID *pReserved);

DNS_STATUS
WINAPI
DnsQuery_UTF8(
  _In_ PCSTR pszName,
  _In_ WORD wType,
  _In_ DWORD Options,
  _Inout_opt_ PIP4_ARRAY pExtra,
  _Outptr_result_maybenull_ PDNS_RECORD *ppQueryResults,
  _Outptr_opt_result_maybenull_ PVOID *pReserved);

DNS_STATUS
WINAPI
DnsQueryConfig(
  _In_ DNS_CONFIG_TYPE Config,
  _In_ DWORD Flag,
  _In_opt_ PWSTR pwsAdapterName,
  _In_opt_ PVOID pReserved,
  _Out_writes_bytes_to_opt_(*pBufLen, *pBufLen) PVOID pBuffer,
  _Inout_ PDWORD pBufLen);

BOOL WINAPI DnsRecordCompare(_In_ PDNS_RECORD, _In_ PDNS_RECORD);

PDNS_RECORD
WINAPI
DnsRecordCopyEx(
  _In_ PDNS_RECORD pRecord,
  _In_ DNS_CHARSET CharSetIn,
  _In_ DNS_CHARSET CharSetOut);

void WINAPI DnsRecordListFree(_Inout_opt_ PDNS_RECORD, _In_ DNS_FREE_TYPE);

BOOL
WINAPI
DnsRecordSetCompare(
  _Inout_ PDNS_RECORD pRR1,
  _Inout_ PDNS_RECORD pRR2,
  _Outptr_opt_result_maybenull_ PDNS_RECORD *ppDiff1,
  _Outptr_opt_result_maybenull_ PDNS_RECORD *ppDiff2);

PDNS_RECORD
WINAPI
DnsRecordSetCopyEx(
  _In_ PDNS_RECORD pRecordSet,
  _In_ DNS_CHARSET CharSetIn,
  _In_ DNS_CHARSET CharSetOut);

PDNS_RECORD WINAPI DnsRecordSetDetach(_Inout_ PDNS_RECORD);
void WINAPI DnsReleaseContextHandle(_In_ HANDLE);

DNS_STATUS
WINAPI
DnsReplaceRecordSetA(
  _In_ PDNS_RECORD pReplaceSet,
  _In_ DWORD Options,
  _In_opt_ HANDLE hContext,
  _Inout_opt_ PIP4_ARRAY pExtraInfo,
  _Inout_opt_ PVOID pReserved);

DNS_STATUS
WINAPI
DnsReplaceRecordSetW(
  _In_ PDNS_RECORD pReplaceSet,
  _In_ DWORD Options,
  _In_opt_ HANDLE hContext,
  _Inout_opt_ PIP4_ARRAY pExtraInfo,
  _Inout_opt_ PVOID pReserved);

DNS_STATUS
WINAPI
DnsReplaceRecordSetUTF8(
  _In_ PDNS_RECORD pReplaceSet,
  _In_ DWORD Options,
  _In_opt_ HANDLE hContext,
  _Inout_opt_ PIP4_ARRAY pExtraInfo,
  _Inout_opt_ PVOID pReserved);

DNS_STATUS WINAPI DnsValidateName_A(_In_ LPCSTR, _In_ DNS_NAME_FORMAT);
DNS_STATUS WINAPI DnsValidateName_W(_In_ LPCWSTR, _In_ DNS_NAME_FORMAT);
DNS_STATUS WINAPI DnsValidateName_UTF8(_In_ LPCSTR, _In_ DNS_NAME_FORMAT);

BOOL
WINAPI
DnsWriteQuestionToBuffer_W(
  _Inout_ PDNS_MESSAGE_BUFFER pDnsBuffer,
  _Inout_ PDWORD pdwBufferSize,
  _In_ LPWSTR pszName,
  _In_ WORD wType,
  _In_ WORD Xid,
  _In_ BOOL fRecursionDesired);

BOOL
WINAPI
DnsWriteQuestionToBuffer_UTF8(
  _Inout_ PDNS_MESSAGE_BUFFER pDnsBuffer,
  _Inout_ PDWORD pdwBufferSize,
  _In_ LPSTR pszName,
  _In_ WORD wType,
  _In_ WORD Xid,
  _In_ BOOL fRecursionDesired);

#ifdef UNICODE
#define DNS_MINFO_DATA DNS_MINFO_DATAW
#define PDNS_MINFO_DATA PDNS_MINFO_DATAW
#define DNS_MX_DATA DNS_MX_DATAW
#define PDNS_MX_DATA PDNS_MX_DATAW
#define DNS_NXT_DATA DNS_NXT_DATAW
#define PDNS_NXT_DATA PDNS_NXT_DATAW
#define DNS_PTR_DATA DNS_PTR_DATAW
#define PDNS_PTR_DATA PDNS_PTR_DATAW
#define DNS_SIG_DATA DNS_SIG_DATAW
#define PDNS_SIG_DATA PDNS_SIG_DATAW
#define DNS_SOA_DATA DNS_SOA_DATAW
#define PDNS_SOA_DATA PDNS_SOA_DATAW
#define DNS_TXT_DATA DNS_TXT_DATAW
#define PDNS_TXT_DATA PDNS_TXT_DATAW
#define DNS_TKEY_DATA DNS_TKEY_DATAW
#define PDNS_TKEY_DATA PDNS_TKEY_DATAW
#define DNS_WINSR_DATA DNS_WINSR_DATAW
#define PDNS_WINSR_DATA PDNS_WINSR_DATAW
#define DnsAcquireContextHandle DnsAcquireContextHandle_W
#define DnsModifyRecordsInSet DnsModifyRecordsInSet_W
#define DnsNameCompare DnsNameCompare_W
#define DnsQuery DnsQuery_W
#define DnsReplaceRecordSet DnsReplaceRecordSetW
#define DnsValidateName DnsValidateName_W
#else
#define DNS_MINFO_DATA DNS_MINFO_DATAA
#define PDNS_MINFO_DATA PDNS_MINFO_DATAA
#define DNS_MX_DATA DNS_MX_DATAA
#define PDNS_MX_DATA PDNS_MX_DATAA
#define DNS_NXT_DATA DNS_NXT_DATAA
#define PDNS_NXT_DATA PDNS_NXT_DATAA
#define DNS_PTR_DATA DNS_PTR_DATAA
#define PDNS_PTR_DATA PDNS_PTR_DATAA
#define DNS_SIG_DATA DNS_SIG_DATAA
#define PDNS_SIG_DATA PDNS_SIG_DATAA
#define DNS_SOA_DATA DNS_SOA_DATAA
#define PDNS_SOA_DATA PDNS_SOA_DATAA
#define DNS_TXT_DATA DNS_TXT_DATAA
#define PDNS_TXT_DATA PDNS_TXT_DATAA
#define DNS_TKEY_DATA DNS_TKEY_DATAA
#define PDNS_TKEY_DATA PDNS_TKEY_DATAA
#define DNS_WINSR_DATA DNS_WINSR_DATAA
#define PDNS_WINSR_DATA PDNS_WINSR_DATAA
#define DnsAcquireContextHandle DnsAcquireContextHandle_A
#define DnsModifyRecordsInSet DnsModifyRecordsInSet_A
#define DnsNameCompare DnsNameCompare_A
#define DnsQuery DnsQuery_A
#define DnsReplaceRecordSet DnsReplaceRecordSetA
#define DnsValidateName DnsValidateName_A
#endif

#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif

#endif /* _WINDNS_H */
