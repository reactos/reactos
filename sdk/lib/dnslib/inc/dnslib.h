/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        include/mswsock.h
 * PURPOSE:     Ancillary Function Driver DLL header
 */

#pragma once

/* INCLUDES ******************************************************************/
#include <ws2atm.h>

/* ENUMERATIONS **************************************************************/

typedef enum _DNS_STRING_TYPE
{
    UnicodeString = 1,
    Utf8String,
    AnsiString,
} DNS_STRING_TYPE;

#define IpV4Address 3

/* TYPES *********************************************************************/

typedef struct _DNS_IPV6_ADDRESS
{
    ULONG Unknown;
    ULONG Unknown2;
    IP6_ADDRESS Address;
    ULONG Unknown3;
    ULONG Unknown4;
    DWORD Reserved;
    ULONG Unknown5;
} DNS_IPV6_ADDRESS, *PDNS_IPV6_ADDRESS;

typedef struct _DNS_ADDRESS
{
    union
    {
        struct
        {
            WORD AddressFamily;
            WORD Port;
            ATM_ADDRESS AtmAddress;
        };
        SOCKADDR_IN Ip4Address;
        SOCKADDR_IN6 Ip6Address;
    };
    ULONG AddressLength;
    DWORD Sub;
    ULONG Flag;
} DNS_ADDRESS, *PDNS_ADDRESS;

typedef struct _DNS_ARRAY
{
    ULONG AllocatedAddresses;
    ULONG UsedAddresses;
    ULONG Unknown[0x6];
    DNS_ADDRESS Addresses[1];
} DNS_ARRAY, *PDNS_ARRAY;

typedef struct _DNS_BLOB
{
    LPWSTR Name;
    PDNS_ARRAY DnsAddrArray;
    PHOSTENT Hostent;
    ULONG AliasCount;
    ULONG Unknown;
    LPWSTR Aliases[8];
} DNS_BLOB, *PDNS_BLOB;

typedef struct _DNS_FAMILY_INFO
{
    WORD AddrType;
    WORD DnsType;
    DWORD AddressSize;
    DWORD SockaddrSize;
    DWORD AddressOffset;
} DNS_FAMILY_INFO, *PDNS_FAMILY_INFO;

typedef struct _FLATBUFF
{
    PVOID Buffer;
    PVOID BufferEnd;
    ULONG_PTR BufferPos;
    SIZE_T BufferSize;
    SIZE_T BufferFreeSize;
} FLATBUFF, *PFLATBUFF;

/*
 * memory.c
 */
VOID
WINAPI
Dns_Free(IN PVOID Address);

PVOID
WINAPI
Dns_AllocZero(IN SIZE_T Size);

/*
 * addr.c
 */
PDNS_FAMILY_INFO
WINAPI
FamilyInfo_GetForFamily(IN WORD AddressFamily);

/*
 * dnsaddr.c
 */
VOID
WINAPI
DnsAddr_BuildFromIp4(
    IN PDNS_ADDRESS DnsAddress,
    IN IN_ADDR Address,
    IN WORD Unknown
);

VOID
WINAPI
DnsAddr_BuildFromIp6(
    IN PDNS_ADDRESS DnsAddress,
    IN PIN6_ADDR Address,
    IN ULONG ScopeId,
    IN WORD Port
);

PDNS_ARRAY
WINAPI
DnsAddrArray_Create(ULONG Count);

BOOL
WINAPI
DnsAddrArray_AddAddr(
    IN PDNS_ARRAY DnsAddrArray,
    IN PDNS_ADDRESS DnsAddress,
    IN WORD AddressFamily OPTIONAL,
    IN DWORD AddressType OPTIONAL
);

VOID
WINAPI
DnsAddrArray_Free(IN PDNS_ARRAY DnsAddrArray);

BOOL
WINAPI
DnsAddrArray_AddIp4(
    IN PDNS_ARRAY DnsAddrArray,
    IN IN_ADDR Address,
    IN DWORD AddressType
);

BOOL
WINAPI
DnsAddrArray_ContainsAddr(
    IN PDNS_ARRAY DnsAddrArray,
    IN PDNS_ADDRESS DnsAddress,
    IN DWORD AddressType
);

BOOLEAN
WINAPI
DnsAddr_BuildFromDnsRecord(
    IN PDNS_RECORD DnsRecord,
    OUT PDNS_ADDRESS DnsAddr
);

/*
 * hostent.c
 */
PHOSTENT
WINAPI
Hostent_Init(
    IN PVOID *Buffer,
    IN WORD AddressFamily,
    IN ULONG AddressSize,
    IN ULONG AddressCount,
    IN ULONG AliasCount
);

VOID
WINAPI
Hostent_ConvertToOffsets(IN PHOSTENT Hostent);

/*
 * flatbuf.c
 */
VOID
WINAPI
FlatBuf_Init(
    IN PFLATBUFF FlatBuffer,
    IN PVOID Buffer,
    IN SIZE_T Size
);

PVOID
WINAPI
FlatBuf_Arg_CopyMemory(
    IN OUT PULONG_PTR Position,
    IN OUT PSIZE_T FreeSize,
    IN PVOID Buffer,
    IN SIZE_T Size,
    IN ULONG Align
);

PVOID
WINAPI
FlatBuf_Arg_Reserve(
    IN OUT PULONG_PTR Position,
    IN OUT PSIZE_T FreeSize,
    IN SIZE_T Size,
    IN ULONG Align
);

PVOID
WINAPI
FlatBuf_Arg_WriteString(
    IN OUT PULONG_PTR Position,
    IN OUT PSIZE_T FreeSize,
    IN PVOID String,
    IN BOOLEAN IsUnicode
);

/*
 * sablob.c
 */
PDNS_BLOB
WINAPI
SaBlob_Create(
    IN ULONG Count
);

PDNS_BLOB
WINAPI
SaBlob_CreateFromIp4(
    IN LPWSTR Name,
    IN ULONG Count,
    IN PIN_ADDR AddressArray
);

VOID
WINAPI
SaBlob_Free(IN PDNS_BLOB Blob);

PHOSTENT
WINAPI
SaBlob_CreateHostent(
    IN OUT PULONG_PTR BufferPosition,
    IN OUT PSIZE_T RemainingBufferSpace,
    IN OUT PSIZE_T HostEntrySize,
    IN PDNS_BLOB Blob,
    IN DWORD StringType,
    IN BOOLEAN Relative,
    IN BOOLEAN BufferAllocated
);

INT
WINAPI
SaBlob_WriteNameOrAlias(
    IN PDNS_BLOB Blob,
    IN LPWSTR String,
    IN BOOLEAN IsAlias
);

PDNS_BLOB
WINAPI
SaBlob_Query(
    IN LPWSTR Name,
    IN WORD DnsType,
    IN ULONG Flags,
    IN PVOID *Reserved,
    IN DWORD AddressFamily
);

/*
 * string.c
 */
ULONG
WINAPI
Dns_StringCopy(
    OUT PVOID Destination,
    IN OUT PULONG DestinationSize,
    IN PVOID String,
    IN ULONG StringSize OPTIONAL,
    IN DWORD InputType,
    IN DWORD OutputType
);

LPWSTR
WINAPI
Dns_CreateStringCopy_W(IN LPWSTR Name);

ULONG
WINAPI
Dns_GetBufferLengthForStringCopy(
    IN PVOID String,
    IN ULONG Size OPTIONAL,
    IN DWORD InputType,
    IN DWORD OutputType
);

/*
 * straddr.c
 */
BOOLEAN
WINAPI
Dns_StringToAddressW(
    OUT PVOID Address,
    IN OUT PULONG AddressSize,
    IN LPWSTR AddressName,
    IN OUT PDWORD AddressFamily
);

LPWSTR
WINAPI
Dns_Ip4AddressToReverseName_W(
    OUT LPWSTR Name,
    IN IN_ADDR Address
);

LPWSTR
WINAPI
Dns_Ip6AddressToReverseName_W(
    OUT LPWSTR Name,
    IN IN6_ADDR Address
);

BOOLEAN
WINAPI
Dns_ReverseNameToDnsAddr_W(
    OUT PDNS_ADDRESS DnsAddr,
    IN LPWSTR Name
);

BOOLEAN
WINAPI
Dns_Ip4ReverseNameToAddress_W(
    OUT PIN_ADDR Address,
    IN LPWSTR Name
);
