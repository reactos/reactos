/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Network Address Translation implementation
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/* Borrow this from some headers... */
typedef struct
{
  union
  {
    struct { UCHAR s_b1,s_b2,s_b3,s_b4; } S_un_b;
    struct { USHORT s_w1,s_w2; } S_un_w;
    ULONG S_addr;
  } S_un;
} in_addr;

typedef struct
{
  union
  {
    UCHAR _S6_u8[16];
    USHORT _S6_u16[8];
    ULONG _S6_u32[4];
  } S6_un;
} in6_addr;


/*
* @implemented
*/
LPSTR
NTAPI
RtlIpv4AddressToStringA(
	PULONG IP,
	LPSTR Buffer
	)
{
  in_addr addr;
  addr.S_un.S_addr = *IP;
  return Buffer + sprintf(Buffer, "%u.%u.%u.%u", addr.S_un.S_un_b.s_b1,
                                                 addr.S_un.S_un_b.s_b2,
                                                 addr.S_un.S_un_b.s_b3,
                                                 addr.S_un.S_un_b.s_b4);
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4AddressToStringExA(
	PULONG IP,
	PULONG Port,
	LPSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @implemented
*/
LPWSTR
NTAPI
RtlIpv4AddressToStringW(
	PULONG IP,
	LPWSTR Buffer
	)
{
  in_addr addr;
  addr.S_un.S_addr = *IP;
  return Buffer + swprintf(Buffer, L"%u.%u.%u.%u", addr.S_un.S_un_b.s_b1,
                                                   addr.S_un.S_un_b.s_b2,
                                                   addr.S_un.S_un_b.s_b3,
                                                   addr.S_un.S_un_b.s_b4);
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(
	PULONG IP,
	PULONG Port,
	LPWSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressExA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(
	IN LPWSTR IpString,
	IN ULONG Base,
    OUT PULONG PtrToIpAddr,
    OUT PULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv4StringToAddressExW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringA(
	PULONG IP,
	LPSTR Buffer
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringExA(
	PULONG IP,
	PULONG Port,
	LPSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringW(
	PULONG IP,
	LPWSTR Buffer
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6AddressToStringExW(
	PULONG IP,
	PULONG Port,
	LPWSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressExA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
