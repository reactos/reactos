/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    ntfuncs.h

Abstract:

    Contains definitions for dynamically loaded NTDLL functions.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_NTFUNCS
#define _INC_NTFUNCS

typedef NTSYSAPI NTSTATUS 
(NTAPI * PFNNTQUERYSYSTEMINFORMATION)(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef NTSYSAPI LARGE_INTEGER 
(NTAPI * PFNRTLEXTENDEDLARGEINTEGERDIVIDE)(
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
    );

typedef NTSYSAPI BOOLEAN
(NTAPI * PFNRTLGETNTPRODUCTTYPE)(
    PNT_PRODUCT_TYPE    NtProductType
    );

#endif // _INC_NTFUNCS



