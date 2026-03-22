#ifndef _NSLOOKUP_H
#define _NSLOOKUP_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <tchar.h>
#include <stdio.h>

/* DNS resolver library (provides TYPE_*, CLASS_*, OPCODE_*, RCODE_* numeric
 * constants, DnsResolv_* functions, and DNS_RESOLVER_CONFIG). */
#include <dnsresolv.h>

/* Query-type string constants (nslookup-specific, not in dnsresolv.h) */
#define TypeA       "A"
#define TypeAAAA    "AAAA"
#define TypeBoth    "A+AAAA"
#define TypeAny     "ANY"
#define TypeCNAME   "CNAME"
#define TypeMX      "MX"
#define TypeNS      "NS"
#define TypePTR     "PTR"
#define TypeSOA     "SOA"
#define TypeSRV     "SRV"

/* Aliases for dnsresolv type constants (keeps existing code unchanged) */
#define TYPE_A      DNSRESOLV_TYPE_A
#define TYPE_NS     DNSRESOLV_TYPE_NS
#define TYPE_CNAME  DNSRESOLV_TYPE_CNAME
#define TYPE_SOA    DNSRESOLV_TYPE_SOA
#define TYPE_WKS    DNSRESOLV_TYPE_WKS
#define TYPE_PTR    DNSRESOLV_TYPE_PTR
#define TYPE_MX     DNSRESOLV_TYPE_MX
#define TYPE_ANY    DNSRESOLV_TYPE_ANY

/* Class string constants */
#define ClassIN     "IN"
#define ClassAny    "ANY"

/* Aliases for dnsresolv class constants */
#define CLASS_IN    DNSRESOLV_CLASS_IN
#define CLASS_ANY   DNSRESOLV_CLASS_ANY

/* Aliases for dnsresolv opcode constants */
#define OPCODE_QUERY    DNSRESOLV_OPCODE_QUERY
#define OPCODE_IQUERY   DNSRESOLV_OPCODE_IQUERY
#define OPCODE_STATUS   DNSRESOLV_OPCODE_STATUS

/* Opcode string constants */
#define OpcodeQuery     "QUERY"
#define OpcodeIQuery    "IQUERY"
#define OpcodeStatus    "STATUS"
#define OpcodeReserved  "RESERVED"

/* Aliases for dnsresolv rcode constants */
#define RCODE_NOERROR   DNSRESOLV_RCODE_NOERROR
#define RCODE_FORMERR   DNSRESOLV_RCODE_FORMERR
#define RCODE_FAILURE   DNSRESOLV_RCODE_FAILURE
#define RCODE_NXDOMAIN  DNSRESOLV_RCODE_NXDOMAIN
#define RCODE_NOTIMP    DNSRESOLV_RCODE_NOTIMP
#define RCODE_REFUSED   DNSRESOLV_RCODE_REFUSED

/* Rcode string constants */
#define RCodeNOERROR    "NOERROR"
#define RCodeFORMERR    "FORMERR"
#define RCodeFAILURE    "FAILURE"
#define RCodeNXDOMAIN   "NXDOMAIN"
#define RCodeNOTIMP     "NOTIMP"
#define RCodeREFUSED    "REFUSED"
#define RCodeReserved   "RESERVED"

/* Aliases for dnsresolv miscellaneous constants */
#define DEFAULT_ROOT    DNSRESOLV_DEFAULT_ROOT
#define ARPA_SIG        DNSRESOLV_ARPA_SIG

typedef struct _STATE
{
    BOOL debug;
    BOOL defname;
    BOOL d2;
    BOOL recurse;
    BOOL search;
    BOOL vc;
    BOOL ignoretc;
    BOOL MSxfr;
    CHAR domain[256];
    CHAR srchlist[6][256];
    CHAR root[256];
    DWORD retry;
    DWORD timeout;
    DWORD ixfrver;
    PCHAR type;
    PCHAR Class;
    USHORT port;
    CHAR DefaultServer[256];
    CHAR DefaultServerAddress[16];
} STATE, *PSTATE;

/* nslookup.c */

extern STATE    State;

/* utility.c */

BOOL SendRequest( PCHAR pInBuffer,
                  ULONG InBufferLength,
                  PCHAR pOutBuffer,
                  PULONG pOutBufferLength );

void    PrintD2( PCHAR pBuffer, DWORD BufferLength );
void    PrintDebug( PCHAR pBuffer, DWORD BufferLength );

#endif /* _NSLOOKUP_H */
