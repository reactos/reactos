/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Runtime Library
 * PURPOSE:           Network Address Translation implementation
 * PROGRAMMER:        Alex Ionescu (alexi@tinykrnl.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* maximum length of an ipv4 address expressed as a string */
#define IPV4_ADDR_STRING_MAX_LEN 16

/* maximum length of an ipv4 port expressed as a string */
#define IPV4_PORT_STRING_MAX_LEN 7 /* with the leading ':' */

/* network to host order conversion for little endian machines */
#define WN2H(w) (((w & 0xFF00) >> 8) | ((w & 0x00FF) << 8))

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
LPSTR
NTAPI
RtlIpv4AddressToStringA(IN struct in_addr *Addr,
                        OUT PCHAR S)
{
    INT Length;

    if (!S) return (LPSTR)~0;

    Length = sprintf(S, "%u.%u.%u.%u", Addr->S_un.S_un_b.s_b1,
                                            Addr->S_un.S_un_b.s_b2,
                                            Addr->S_un.S_un_b.s_b3,
                                            Addr->S_un.S_un_b.s_b4);

    return S + Length;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4AddressToStringExA(IN struct in_addr *Address,
                          IN USHORT Port,
                          OUT PCHAR AddressString,
                          IN OUT PULONG AddressStringLength)
{
    CHAR Buffer[IPV4_ADDR_STRING_MAX_LEN+IPV4_PORT_STRING_MAX_LEN];
    ULONG Length;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    Length = sprintf(Buffer, "%u.%u.%u.%u", Address->S_un.S_un_b.s_b1,
                                            Address->S_un.S_un_b.s_b2,
                                            Address->S_un.S_un_b.s_b3,
                                            Address->S_un.S_un_b.s_b4);

    if (Port) Length += sprintf(Buffer + Length, ":%u", WN2H(Port));

    if (*AddressStringLength > Length)
    {
        *AddressStringLength = Length + 1;
        strcpy(AddressString, Buffer);
        return STATUS_SUCCESS;
    }

    *AddressStringLength = Length + 1;
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
LPWSTR
NTAPI
RtlIpv4AddressToStringW(IN struct in_addr *Addr,
                        OUT PWCHAR S)
{
    INT Length;

    if (!S) return (LPWSTR)~0;

    Length = swprintf(S, L"%u.%u.%u.%u", Addr->S_un.S_un_b.s_b1,
                                         Addr->S_un.S_un_b.s_b2,
                                         Addr->S_un.S_un_b.s_b3,
                                         Addr->S_un.S_un_b.s_b4);
    return S + Length;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(IN struct in_addr *Address,
                          IN USHORT Port,
                          OUT PWCHAR AddressString,
                          IN OUT PULONG AddressStringLength)
{
    WCHAR Buffer[IPV4_ADDR_STRING_MAX_LEN+IPV4_PORT_STRING_MAX_LEN];
    ULONG Length;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    Length = swprintf(Buffer, L"%u.%u.%u.%u", Address->S_un.S_un_b.s_b1,
                                              Address->S_un.S_un_b.s_b2,
                                              Address->S_un.S_un_b.s_b3,
                                              Address->S_un.S_un_b.s_b4);

    if (Port) Length += swprintf(Buffer + Length, L":%u", WN2H(Port));

    if (*AddressStringLength > Length)
    {
        *AddressStringLength = Length + 1;
        wcscpy(AddressString, Buffer);
        return STATUS_SUCCESS;
    }

    *AddressStringLength = Length + 1;
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4StringToAddressA(
    _In_ PCSTR String,
    _In_ BOOLEAN Strict,
    _Out_ PCSTR *Terminator,
    _Out_ struct in_addr *Addr)
{
    NTSTATUS Status;
    ANSI_STRING AddressA;
    UNICODE_STRING AddressW;
    PCWSTR TerminatorW = NULL;

    Status = RtlInitAnsiStringEx(&AddressA, String);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAnsiStringToUnicodeString(&AddressW, &AddressA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlIpv4StringToAddressW(AddressW.Buffer,
                                     Strict,
                                     &TerminatorW,
                                     Addr);

    ASSERT(TerminatorW >= AddressW.Buffer);
    *Terminator = String + (TerminatorW - AddressW.Buffer);

    RtlFreeUnicodeString(&AddressW);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIpv4StringToAddressExA(
    _In_ PCSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ struct in_addr *Address,
    _Out_ PUSHORT Port)
{
    NTSTATUS Status;
    ANSI_STRING AddressA;
    UNICODE_STRING AddressW;

    Status = RtlInitAnsiStringEx(&AddressA, AddressString);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlAnsiStringToUnicodeString(&AddressW, &AddressA, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlIpv4StringToAddressExW(AddressW.Buffer, Strict, Address, Port);

    RtlFreeUnicodeString(&AddressW);

    return Status;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(IN PCWSTR String,
                        IN BOOLEAN Strict,
                        OUT PCWSTR *Terminator,
                        OUT struct in_addr *Addr)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressExW(IN PCWSTR AddressString,
                          IN BOOLEAN Strict,
                          OUT struct in_addr *Address,
                          OUT PUSHORT Port)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringA(IN struct in6_addr *Addr,
                        OUT PCHAR S)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringExA(IN struct in6_addr *Address,
                          IN ULONG ScopeId,
                          IN ULONG Port,
                          OUT PCHAR AddressString,
                          IN OUT PULONG AddressStringLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringW(IN struct in6_addr *Addr,
                        OUT PWCHAR S)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringExW(IN struct in6_addr *Address,
                          IN ULONG ScopeId,
                          IN USHORT Port,
                          IN OUT PWCHAR AddressString,
                          IN OUT PULONG AddressStringLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressA(IN PCHAR Name,
                        OUT PCHAR *Terminator,
                        OUT struct in6_addr *Addr)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressExA(IN PCHAR AddressString,
                          OUT struct in6_addr *Address,
                          OUT PULONG ScopeId,
                          OUT PUSHORT Port)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(IN PWCHAR Name,
                        OUT PCHAR *Terminator,
                        OUT struct in6_addr *Addr)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(IN PWCHAR AddressName,
                          OUT struct in6_addr *Address,
                          OUT PULONG ScopeId,
                          OUT PUSHORT Port)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
