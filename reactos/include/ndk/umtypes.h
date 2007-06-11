/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    umtypes.h

Abstract:

    Type definitions for the basic native types.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#if !defined(_NTDEF_) && !defined(_NTDEF_H)
#define _NTDEF_
#define _NTDEF_H

//
// NDK Applications must use Unicode
//
#ifndef UNICODE
#define UNICODE
#endif

//
// Don't use the SDK status values
//
#ifndef WIN32_NO_STATUS
#define WIN32_NO_STATUS
#endif

//
// Let the NDK know we're in Application Mode
//
#define NTOS_MODE_USER

//
// Dependencies
//
#include <windef.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <winioctl.h>
#include <ntnls.h>

//
// Compiler Definitions
//
#ifndef _MANAGED
#if defined(_M_IX86)
#define FASTCALL                        _fastcall
#else
#define FASTCALL
#endif
#else
#define FASTCALL                        NTAPI
#endif

#if !defined(_M_CEE_PURE)
#define NTAPI_INLINE                    NTAPI
#else
#define NTAPI_INLINE
#endif

//
// Alignment Macros
//
#define ALIGN_DOWN(s, t) \
    ((ULONG)(s) & ~(sizeof(t) - 1))

#define ALIGN_UP(s, t) \
    (ALIGN_DOWN(((ULONG)(s) + sizeof(t) - 1), t))

#define ALIGN_DOWN_POINTER(p, t) \
    ((PVOID)((ULONG_PTR)(p) & ~((ULONG_PTR)sizeof(t) - 1)))

#define ALIGN_UP_POINTER(p, t) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(p) + sizeof(t) - 1), t))

//
// Native API Return Value Macros
//
#define NT_SUCCESS(Status)              (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status)          ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status)              ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status)                ((((ULONG)(Status)) >> 30) == 3)

//
// Limits
//
#define MINCHAR                         0x80
#define MAXCHAR                         0x7f
#define MINSHORT                        0x8000
#define MAXSHORT                        0x7fff
#define MINLONG                         0x80000000
#define MAXLONG                         0x7fffffff
#define MAXUCHAR                        0xff
#define MAXUSHORT                       0xffff
#define MAXULONG                        0xffffffff

//
// CSR Macros
//
#define CSR_MAKE_OPCODE(s,m)            ((s) << 16) | (m)
#define CSR_API_ID_FROM_OPCODE(n)       ((ULONG)((USHORT)(n)))
#define CSR_SERVER_ID_FROM_OPCODE(n)    (ULONG)((n) >> 16)

//
// Basic Types that aren't defined in User-Mode Headers
//
typedef CONST int CINT;
typedef CONST char *PCSZ;
typedef ULONG CLONG;
typedef short CSHORT;
typedef CSHORT *PCSHORT;
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef LONG KPRIORITY;
typedef LONG NTSTATUS, *PNTSTATUS;

//
// Basic NT Types
//
#if !defined(_NTSECAPI_H) && !defined(_SUBAUTH_H) && !defined(_NTSECAPI_)

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

typedef struct _CSTRING
{
    USHORT Length;
    USHORT MaximumLength;
    CONST CHAR *Buffer;
} CSTRING, *PCSTRING;

#endif

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

//
// ClientID Structure
//
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef const UNICODE_STRING* PCUNICODE_STRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef CONST STRING* PCOEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;

#endif
