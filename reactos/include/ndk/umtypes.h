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

/* Undef the following to avoid conflects including ntstatus.h with winnt.h */
#undef STATUS_WAIT_0
#undef STATUS_ABANDONED_WAIT_0
#undef STATUS_USER_APC
#undef STATUS_TIMEOUT
#undef STATUS_PENDING
#undef DBG_EXCEPTION_HANDLED
#undef DBG_CONTINUE
#undef STATUS_SEGMENT_NOTIFICATION
#undef DBG_TERMINATE_THREAD
#undef DBG_TERMINATE_PROCESS
#undef DBG_CONTROL_C
#undef DBG_CONTROL_BREAK
#undef DBG_COMMAND_EXCEPTION
#undef STATUS_GUARD_PAGE_VIOLATION
#undef STATUS_DATATYPE_MISALIGNMENT
#undef STATUS_BREAKPOINT
#undef STATUS_SINGLE_STEP
#undef DBG_EXCEPTION_NOT_HANDLED
#undef STATUS_ACCESS_VIOLATION
#undef STATUS_IN_PAGE_ERROR
#undef STATUS_INVALID_HANDLE
#undef STATUS_NO_MEMORY
#undef STATUS_ILLEGAL_INSTRUCTION
#undef STATUS_NONCONTINUABLE_EXCEPTION
#undef STATUS_INVALID_DISPOSITION
#undef STATUS_ARRAY_BOUNDS_EXCEEDED
#undef STATUS_FLOAT_DENORMAL_OPERAND
#undef STATUS_FLOAT_DIVIDE_BY_ZERO
#undef STATUS_FLOAT_INEXACT_RESULT
#undef STATUS_FLOAT_INVALID_OPERATION
#undef STATUS_FLOAT_OVERFLOW
#undef STATUS_FLOAT_STACK_CHECK
#undef STATUS_FLOAT_UNDERFLOW
#undef STATUS_INTEGER_DIVIDE_BY_ZERO
#undef STATUS_INTEGER_OVERFLOW
#undef STATUS_PRIVILEGED_INSTRUCTION
#undef STATUS_STACK_OVERFLOW
#undef STATUS_CONTROL_C_EXIT
#undef STATUS_FLOAT_MULTIPLE_FAULTS
#undef STATUS_FLOAT_MULTIPLE_TRAPS
#undef STATUS_REG_NAT_CONSUMPTION
#undef STATUS_SXS_EARLY_DEACTIVATION
#undef STATUS_SXS_INVALID_DEACTIVATION

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
