/*
 * PROJECT:     ReactOS DNS Resolver Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        sdk/lib/dnsresolv/dnsresolv.h
 * PURPOSE:     Public header for the DNS resolver library based on nslookup
 * COPYRIGHT:   Copyright 2009 Lucas Suggs <lucas.suggs@gmail.com>
 *              Copyright 2024 ReactOS Contributors
 */

#ifndef _DNSRESOLV_H
#define _DNSRESOLV_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#define _INC_WINDOWS
#include <winbase.h>
#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DNS Record Type constants */
#define DNSRESOLV_TYPE_A      0x01
#define DNSRESOLV_TYPE_NS     0x02
#define DNSRESOLV_TYPE_CNAME  0x05
#define DNSRESOLV_TYPE_SOA    0x06
#define DNSRESOLV_TYPE_WKS    0x0B
#define DNSRESOLV_TYPE_PTR    0x0C
#define DNSRESOLV_TYPE_MX     0x0F
#define DNSRESOLV_TYPE_SRV    0x21
#define DNSRESOLV_TYPE_ANY    0xFF

/* DNS Class constants */
#define DNSRESOLV_CLASS_IN    0x01
#define DNSRESOLV_CLASS_ANY   0xFF

/* DNS Opcode constants */
#define DNSRESOLV_OPCODE_QUERY    0x00
#define DNSRESOLV_OPCODE_IQUERY   0x01
#define DNSRESOLV_OPCODE_STATUS   0x02

/* DNS Response Code constants */
#define DNSRESOLV_RCODE_NOERROR   0x00
#define DNSRESOLV_RCODE_FORMERR   0x01
#define DNSRESOLV_RCODE_FAILURE   0x02
#define DNSRESOLV_RCODE_NXDOMAIN  0x03
#define DNSRESOLV_RCODE_NOTIMP    0x04
#define DNSRESOLV_RCODE_REFUSED   0x05

/* Miscellaneous constants */
#define DNSRESOLV_DEFAULT_PORT    53
#define DNSRESOLV_DEFAULT_TIMEOUT 2
#define DNSRESOLV_DEFAULT_RETRY   1
#define DNSRESOLV_ARPA_SIG        ".in-addr.arpa"
#define DNSRESOLV_DEFAULT_ROOT    "A.ROOT-SERVERS.NET."

/*
 * Configuration structure for DNS resolver functions.
 * Callers fill this in (or use DnsResolv_InitConfig for defaults)
 * and pass it to the API functions instead of relying on any global state.
 */
typedef struct _DNS_RESOLVER_CONFIG
{
    CHAR   ServerAddress[16]; /* Dotted-decimal IPv4 address of the DNS server */
    USHORT Port;              /* DNS port (default: 53)                        */
    DWORD  Timeout;           /* Socket receive timeout in seconds (default: 2)*/
    DWORD  Retry;             /* Number of send retries (default: 1)           */
    BOOL   Recurse;           /* Set the Recursion Desired flag in queries      */
} DNS_RESOLVER_CONFIG, *PDNS_RESOLVER_CONFIG;

/*
 * DnsResolv_InitConfig
 *
 * Fill pConfig with safe default values.
 * ServerAddress is left empty; the caller must set it before querying.
 */
VOID
DnsResolv_InitConfig(
    _Out_ PDNS_RESOLVER_CONFIG pConfig);

/*
 * DnsResolv_IsValidIP
 *
 * Returns TRUE if pInput is a valid dotted-decimal IPv4 address string.
 */
BOOL
DnsResolv_IsValidIP(
    _In_ PCHAR pInput);

/*
 * DnsResolv_ReverseIP
 *
 * Converts a dotted-decimal IPv4 address (e.g. "1.2.3.4") into its
 * reversed form suitable for PTR queries (e.g. "4.3.2.1").
 * pReturn must be at least 16 bytes.
 */
VOID
DnsResolv_ReverseIP(
    _In_  PCHAR pIP,
    _Out_ PCHAR pReturn);

/*
 * DnsResolv_ExtractName
 *
 * Parses a DNS wire-format domain name (with pointer compression) from
 * pBuffer at the given Offset.  The decoded, dot-separated name is
 * written to pOutput.  Limit caps the number of wire bytes consumed
 * (0 means no explicit limit).
 *
 * Returns the number of wire bytes consumed from pBuffer starting at
 * Offset (not counting bytes followed through a pointer).
 */
int
DnsResolv_ExtractName(
    _In_  PCHAR  pBuffer,
    _Out_ PCHAR  pOutput,
    _In_  USHORT Offset,
    _In_  UCHAR  Limit);

/*
 * DnsResolv_ExtractIP
 *
 * Reads the four address bytes of an A record starting at Offset in
 * pBuffer and writes a dotted-decimal string into pOutput.
 *
 * Returns the number of bytes consumed (always 4).
 */
int
DnsResolv_ExtractIP(
    _In_  PCHAR  pBuffer,
    _Out_ PCHAR  pOutput,
    _In_  USHORT Offset);

/*
 * DnsResolv_SendRequest
 *
 * Sends the raw DNS query in pInBuffer to the server described by
 * pConfig and waits for a response using select().  The response is
 * written to pOutBuffer; *pOutBufferLength must be set to the buffer
 * size on entry and is updated to the actual response length on exit.
 *
 * Returns TRUE on success, FALSE on any socket error or timeout.
 */
BOOL
DnsResolv_SendRequest(
    _In_    PCHAR                pInBuffer,
    _In_    ULONG                InBufferLength,
    _Out_   PCHAR                pOutBuffer,
    _Inout_ PULONG               pOutBufferLength,
    _In_    PDNS_RESOLVER_CONFIG pConfig);

/*
 * DnsResolv_Lookup
 *
 * High-level DNS lookup.  Resolves pAddr (a hostname or dotted-decimal
 * IPv4 address) using the server in pConfig.
 *
 * QueryType selects the record type (DNSRESOLV_TYPE_*).  Pass 0 to
 * auto-detect: TYPE_PTR when pAddr is an IPv4 address, TYPE_A otherwise.
 *
 * On success the result string (IP address or hostname) is written to
 * pResult (caller must supply at least 256 bytes) and TRUE is returned.
 * Returns FALSE on any error or if no answer is present.
 */
BOOL
DnsResolv_Lookup(
    _In_  PCHAR                pAddr,
    _In_  USHORT               QueryType,
    _Out_ PCHAR                pResult,
    _In_  PDNS_RESOLVER_CONFIG pConfig);

/* ---------- Conversion utilities ---------- */

PCHAR  DnsResolv_OpcodeIDtoOpcodeName(_In_ UCHAR  Opcode);
PCHAR  DnsResolv_RCodeIDtoRCodeName  (_In_ UCHAR  RCode);
PCHAR  DnsResolv_TypeIDtoTypeName    (_In_ USHORT TypeID);
USHORT DnsResolv_TypeNametoTypeID    (_In_ PCHAR  TypeName);
PCHAR  DnsResolv_ClassIDtoClassName  (_In_ USHORT ClassID);
USHORT DnsResolv_ClassNametoClassID  (_In_ PCHAR  ClassName);

#ifdef __cplusplus
}
#endif

#endif /* _DNSRESOLV_H */
