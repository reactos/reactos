/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/umtypes.h
 * PURPOSE:         Definitions needed for Native Headers if target is not Kernel-Mode.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */

#if !defined(_NTDEF_) && !defined(_NTDEF_H)
#define _NTDEF_
#define _NTDEF_H

/* DEPENDENCIES **************************************************************/
#include <winioctl.h>
#include <ntnls.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>

/* CONSTANTS *****************************************************************/

/* NTAPI/NTOSAPI Define */
#define DECLSPEC_ADDRSAFE
#define NTAPI __stdcall
#define FASTCALL __fastcall
#if !defined(_NTSYSTEM_)
#define NTSYSAPI     DECLSPEC_IMPORT
#define NTSYSCALLAPI DECLSPEC_IMPORT
#else
#define NTSYSAPI
#if defined(_NTDLLBUILD_)
#define NTSYSCALLAPI
#else
#define NTSYSCALLAPI DECLSPEC_ADDRSAFE
#endif
#endif
#if !defined(_M_CEE_PURE)
#define NTAPI_INLINE    NTAPI
#else
#define NTAPI_INLINE
#endif

/* Native API Return Value Macros */
#define NT_SUCCESS(x) ((x)>=0)
#define NT_WARNING(x) ((ULONG)(x)>>30==2)
#define NT_ERROR(x) ((ULONG)(x)>>30==3)

/* Limits */
#define MINCHAR             0x80
#define MAXCHAR             0x7f
#define MINSHORT            0x8000
#define MAXSHORT            0x7fff
#define MINLONG             0x80000000
#define MAXLONG             0x7fffffff
#define MAXUCHAR            0xff
#define MAXUSHORT           0xffff
#define MAXULONG            0xffffffff

/* TYPES *********************************************************************/

/* Basic Types that aren't defined in User-Mode Headers */
typedef CONST int CINT;
typedef CONST char *PCSZ;
typedef ULONG CLONG;
typedef short CSHORT;
typedef CSHORT *PCSHORT;
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef LONG KPRIORITY;
typedef LONG NTSTATUS, *PNTSTATUS;
#define STATIC static

/* Basic NT Types */
#if !defined(_NTSECAPI_H) && !defined(_SUBAUTH_H)
typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef CONST STRING* PCOEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;

#endif
