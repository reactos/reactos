/* $Id: lsa.c,v 1.1 2001/09/03 20:42:44 ea Exp $
 */
#include <ddk/ntddk.h>

/* LsaCallAuthenticationPackage@28 */
NTSTATUS STDCALL LsaCallAuthenticationPackage (
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaDeregisterLogonProcess@8 */
NTSTATUS STDCALL LsaDeregisterLogonProcess (
    DWORD Unknown0,
    DWORD Unknown1
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaFreeReturnBuffer@4 */
NTSTATUS STDCALL LsaFreeReturnBuffer (PVOID Buffer)
{
    ULONG Size = 0; /* required by MEM_RELEASE */
    
    return ZwFreeVirtualMemory (
               NtCurrentProcess(),
	       & Buffer,
	       & Size,
	       MEM_RELEASE
               );
}

/* LsaLogonUser@56 */
NTSTATUS STDCALL LsaLogonUser (
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6,
    DWORD Unknown7,
    DWORD Unknown8,
    DWORD Unknown9,
    DWORD Unknown10,
    DWORD Unknown11,
    DWORD Unknown12,
    DWORD Unknown13
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaLookupAuthenticationPackage@12 */
NTSTATUS STDCALL LsaLookupAuthenticationPackage (
    DWORD	Unknown0,
    DWORD	Unknown1,
    DWORD	Unknown2
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/* LsaRegisterLogonProcess@12 */
NTSTATUS STDCALL LsaRegisterLogonProcess (
    DWORD	Unknown0,
    DWORD	Unknown1,
    DWORD	Unknown2
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
