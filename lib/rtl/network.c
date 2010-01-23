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

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
LPSTR
NTAPI
RtlIpv4AddressToStringA(IN struct in_addr *Addr,
                        OUT PCHAR S)
{
    return S + sprintf(S, "%u.%u.%u.%u", Addr->S_un.S_un_b.s_b1,
                                         Addr->S_un.S_un_b.s_b2,
                                         Addr->S_un.S_un_b.s_b3,
                                         Addr->S_un.S_un_b.s_b4);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlIpv4AddressToStringExA(IN struct in_addr *Address,
                          IN USHORT Port,
                          OUT PCHAR AddressString,
                          IN OUT PULONG AddressStringLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
LPWSTR
NTAPI
RtlIpv4AddressToStringW(IN struct in_addr *Addr,
                        OUT PWCHAR S)
{
    return S + swprintf(S, L"%u.%u.%u.%u", Addr->S_un.S_un_b.s_b1,
                                           Addr->S_un.S_un_b.s_b2,
                                           Addr->S_un.S_un_b.s_b3,
                                           Addr->S_un.S_un_b.s_b4);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(IN struct in_addr *Address,
                          IN USHORT Port,
                          OUT PWCHAR AddressString,
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
RtlIpv4StringToAddressA(IN PCHAR String,
                        IN BOOLEAN Strict,
                        OUT PCHAR *Terminator,
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
RtlIpv4StringToAddressExA(IN PCHAR AddressString,
                          IN BOOLEAN Strict,
                          OUT struct in_addr *Address,
                          IN PUSHORT Port)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(IN PWCHAR String,
                        IN UCHAR Strict,
                        OUT PWCHAR Terminator,
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
RtlIpv4StringToAddressExW(IN PWCHAR AddressString,
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
