/*
 * ntdef.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _NTDEF_
#define _NTDEF_
#pragma once

/* Dependencies */
#include <ctype.h>
#include <basetsd.h>
#include <guiddef.h>
#include <excpt.h>
#include <sdkddkver.h>
#include <specstrings.h>
#include <kernelspecs.h>

// FIXME: Shouldn't be included!
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pseudo Modifiers for Input Parameters */

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef NOTHING
#define NOTHING
#endif

#ifndef CRITICAL
#define CRITICAL
#endif

// FIXME: deprecated
#ifndef FAR
#define FAR
#endif

/* Constant modifier */
#ifndef CONST
#define CONST const
#endif

/* TRUE/FALSE */
#define FALSE   0
#define TRUE    1

/* NULL/NULL64 */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#define NULL64  0
#else
#define NULL    ((void *)0)
#define NULL64  ((void * POINTER_64)0)
#endif
#endif /* NULL */

/* Defines the "size" of an any-size array */
#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 1
#endif

/* Helper macro to enable gcc's extension.  */
#ifndef __GNU_EXTENSION
#ifdef __GNUC__
#define __GNU_EXTENSION __extension__
#else
#define __GNU_EXTENSION
#endif
#endif

#ifndef DUMMYUNIONNAME
#if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
#define _ANONYMOUS_UNION
#define _UNION_NAME(x) x
#define DUMMYUNIONNAME  u
#define DUMMYUNIONNAME1 u1
#define DUMMYUNIONNAME2 u2
#define DUMMYUNIONNAME3 u3
#define DUMMYUNIONNAME4 u4
#define DUMMYUNIONNAME5 u5
#define DUMMYUNIONNAME6 u6
#define DUMMYUNIONNAME7 u7
#define DUMMYUNIONNAME8 u8
#define DUMMYUNIONNAME9  u9
#else
#define _ANONYMOUS_UNION __GNU_EXTENSION
#define _UNION_NAME(x)
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME1
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#define DUMMYUNIONNAME6
#define DUMMYUNIONNAME7
#define DUMMYUNIONNAME8
#define DUMMYUNIONNAME9
#endif /* NONAMELESSUNION */
#endif /* !DUMMYUNIONNAME */

#ifndef DUMMYSTRUCTNAME
#if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
#define _ANONYMOUS_STRUCT
#define _STRUCT_NAME(x) x
#define DUMMYSTRUCTNAME s
#define DUMMYSTRUCTNAME1 s1
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#define DUMMYSTRUCTNAME4 s4
#define DUMMYSTRUCTNAME5 s5
#else
#define _ANONYMOUS_STRUCT __GNU_EXTENSION
#define _STRUCT_NAME(x)
#define DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME1
#define DUMMYSTRUCTNAME2
#define DUMMYSTRUCTNAME3
#define DUMMYSTRUCTNAME4
#define DUMMYSTRUCTNAME5
#endif /* NONAMELESSUNION */
#endif /* DUMMYSTRUCTNAME */

#if defined(STRICT_GS_ENABLED)
#pragma strict_gs_check(push, on)
#endif

#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM)
#define ALIGNMENT_MACHINE
#define UNALIGNED __unaligned
#if defined(_WIN64)
#define UNALIGNED64 __unaligned
#else
#define UNALIGNED64
#endif
#else
#undef ALIGNMENT_MACHINE
#define UNALIGNED
#define UNALIGNED64
#endif

#if defined(_WIN64) || defined(_M_ALPHA)
#define MAX_NATURAL_ALIGNMENT sizeof(ULONGLONG)
#define MEMORY_ALLOCATION_ALIGNMENT 16
#else
#define MAX_NATURAL_ALIGNMENT sizeof(ULONG)
#define MEMORY_ALLOCATION_ALIGNMENT 8
#endif

#if defined(_M_MRX000) && !(defined(MIDL_PASS) || defined(RC_INVOKED)) && defined(ENABLE_RESTRICTED)
#define RESTRICTED_POINTER __restrict
#else
#define RESTRICTED_POINTER
#endif

#define ARGUMENT_PRESENT(ArgumentPointer) \
  ((CHAR*)((ULONG_PTR)(ArgumentPointer)) != (CHAR*)NULL)

/* Returns the base address of a structure from a structure member */
#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
  ((type *)(((ULONG_PTR)address) - (ULONG_PTR)(&(((type *)0)->field))))
#endif

/* Returns the byte offset of the specified structure's member */
#ifndef __GNUC__
#define FIELD_OFFSET(Type, Field) ((LONG)(LONG_PTR)&(((Type*) 0)->Field))
#else
#define FIELD_OFFSET(Type, Field) ((LONG)__builtin_offsetof(Type, Field))
#endif

/* Returns the type's alignment */
#if defined(_MSC_VER)
#define TYPE_ALIGNMENT(t) __alignof(t)
#else
#define TYPE_ALIGNMENT(t) FIELD_OFFSET(struct { char x; t test; }, test)
#endif

#if defined(_AMD64_) || defined(_X86_)
#define PROBE_ALIGNMENT(_s) TYPE_ALIGNMENT(ULONG)
#elif defined(_IA64_) || defined(_ARM_)
#define PROBE_ALIGNMENT(_s) max((TYPE_ALIGNMENT(_s), TYPE_ALIGNMENT(ULONG))
#else
#error "unknown architecture"
#endif

#if defined(_WIN64)
#define PROBE_ALIGNMENT32(_s) TYPE_ALIGNMENT(ULONG)
#endif

/* Calling Conventions */
#if defined(_MANAGED)
#define FASTCALL __stdcall
#elif defined(_M_IX86)
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

#define NTAPI __stdcall


/* Import and Export Specifiers */

/* Done the same way as in windef.h for now */
#define DECLSPEC_IMPORT __declspec(dllimport)
#define DECLSPEC_NORETURN __declspec(noreturn)

#ifndef DECLSPEC_ADDRSAFE
#if defined(_MSC_VER) && (defined(_M_ALPHA) || defined(_M_AXP64))
#define DECLSPEC_ADDRSAFE  __declspec(address_safe)
#else
#define DECLSPEC_ADDRSAFE
#endif
#endif /* DECLSPEC_ADDRSAFE */

#ifndef DECLSPEC_NOTHROW
#if !defined(MIDL_PASS)
#define DECLSPEC_NOTHROW __declspec(nothrow)
#else
#define DECLSPEC_NOTHROW
#endif
#endif

#ifndef NOP_FUNCTION
#if defined(_MSC_VER)
#define NOP_FUNCTION __noop
#else
#define NOP_FUNCTION (void)0
#endif
#endif

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

/* Inlines */
#ifndef FORCEINLINE
#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#elif ( __MINGW_GNUC_PREREQ(4, 3)  &&  __STDC_VERSION__ >= 199901L)
# define FORCEINLINE extern inline __attribute__((__always_inline__,__gnu_inline__))
#else
# define FORCEINLINE extern __inline__ __attribute__((__always_inline__))
#endif
#endif /* FORCEINLINE */

#ifndef DECLSPEC_NOINLINE
#if (_MSC_VER >= 1300)
#define DECLSPEC_NOINLINE  __declspec(noinline)
#elif defined(__GNUC__)
#define DECLSPEC_NOINLINE __attribute__((noinline))
#else
#define DECLSPEC_NOINLINE
#endif
#endif /* DECLSPEC_NOINLINE */

#if !defined(_M_CEE_PURE)
#define NTAPI_INLINE NTAPI
#else
#define NTAPI_INLINE
#endif

/* Use to specify structure alignment */
#ifndef DECLSPEC_ALIGN
#if defined(_MSC_VER) && !defined(MIDL_PASS)
#define DECLSPEC_ALIGN(x) __declspec(align(x))
#elif defined(__GNUC__)
#define DECLSPEC_ALIGN(x) __attribute__ ((__aligned__(x)))
#else
#define DECLSPEC_ALIGN(x)
#endif
#endif /* DECLSPEC_ALIGN */

#ifndef SYSTEM_CACHE_ALIGNMENT_SIZE
#if defined(_AMD64_) || defined(_X86_)
#define SYSTEM_CACHE_ALIGNMENT_SIZE 64
#else
#define SYSTEM_CACHE_ALIGNMENT_SIZE 128
#endif
#endif

#ifndef DECLSPEC_CACHEALIGN
#define DECLSPEC_CACHEALIGN DECLSPEC_ALIGN(SYSTEM_CACHE_ALIGNMENT_SIZE)
#endif

#ifndef DECLSPEC_UUID
#if defined(_MSC_VER) && defined(__cplusplus)
#define DECLSPEC_UUID(x) __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif

#ifndef DECLSPEC_NOVTABLE
#if defined(_MSC_VER) && defined(__cplusplus)
#define DECLSPEC_NOVTABLE __declspec(novtable)
#else
#define DECLSPEC_NOVTABLE
#endif
#endif

#ifndef DECLSPEC_SELECTANY
#if defined(_MSC_VER) || defined(__GNUC__)
#define DECLSPEC_SELECTANY __declspec(selectany)
#else
#define DECLSPEC_SELECTANY
#endif
#endif

#ifndef DECLSPEC_DEPRECATED
#if (defined(_MSC_VER) || defined(__GNUC__)) && !defined(MIDL_PASS)
#define DECLSPEC_DEPRECATED __declspec(deprecated)
#define DEPRECATE_SUPPORTED
#else
#define DECLSPEC_DEPRECATED
#undef  DEPRECATE_SUPPORTED
#endif
#endif

#ifdef DEPRECATE_DDK_FUNCTIONS
#ifdef _NTDDK_
#define DECLSPEC_DEPRECATED_DDK DECLSPEC_DEPRECATED
#ifdef DEPRECATE_SUPPORTED
#define PRAGMA_DEPRECATED_DDK 1
#endif
#else
#define DECLSPEC_DEPRECATED_DDK
#define PRAGMA_DEPRECATED_DDK 1
#endif
#else
#define DECLSPEC_DEPRECATED_DDK
#define PRAGMA_DEPRECATED_DDK 0
#endif

/* Use to silence unused variable warnings when it is intentional */
#define UNREFERENCED_PARAMETER(P) {(P)=(P);}
#define UNREFERENCED_LOCAL_VARIABLE(L) ((void)(L))
#define DBG_UNREFERENCED_PARAMETER(P) {(P)=(P);}
#define DBG_UNREFERENCED_LOCAL_VARIABLE(L) ((void)(L))

/* min/max helper macros */
#ifndef NOMINMAX

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif /* NOMINMAX */

/* Tell windef.h that we have defined some basic types */
#define BASETYPES

/* Void Pointers */
typedef void *PVOID;
typedef void * POINTER_64 PVOID64;

/* Handle Type */
typedef void *HANDLE, **PHANDLE;;
#ifdef STRICT
#define DECLARE_HANDLE(n) typedef struct n##__{int unused;} *n
#else
#define DECLARE_HANDLE(n) typedef HANDLE n
#endif

/* Upper-Case Versions of Some Standard C Types */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#if !defined(MIDL_PASS)
typedef int INT;
#endif
#endif

/* Avoid redefinition in windef.h */
#define BASETYPES

/* Unsigned Types */
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned long ULONG, *PULONG;

typedef CONST UCHAR *PCUCHAR;
typedef CONST USHORT *PCUSHORT;
typedef CONST ULONG *PCULONG;

typedef double DOUBLE;

/* Signed Types */
typedef SHORT *PSHORT;
typedef LONG *PLONG;

/* Flag types */
typedef unsigned char FCHAR;
typedef unsigned short FSHORT;
typedef unsigned long FLONG;

typedef unsigned char BOOLEAN, *PBOOLEAN;
typedef ULONG LOGICAL, *PLOGICAL;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS, *PNTSTATUS;;
typedef signed char SCHAR, *PSCHAR;

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef _Return_type_success_(return >= 0) LONG HRESULT;
#endif

/* 64-bit types */
#define _ULONGLONG_
__GNU_EXTENSION typedef __int64 LONGLONG, *PLONGLONG;
__GNU_EXTENSION typedef unsigned __int64 ULONGLONG, *PULONGLONG;
#define _DWORDLONG_
typedef ULONGLONG DWORDLONG, *PDWORDLONG;

/* Update Sequence Number */
typedef LONGLONG USN;

/* ANSI (Multi-byte Character) types */
typedef CHAR *PCHAR, *LPCH, *PCH, *PNZCH;
typedef CONST CHAR *LPCCH, *PCCH, *PCNZCH;
typedef _Null_terminated_ CHAR *NPSTR, *LPSTR, *PSTR;
typedef _Null_terminated_ PSTR *PZPSTR;
typedef _Null_terminated_ CONST PSTR *PCZPSTR;
typedef _Null_terminated_ CONST CHAR *LPCSTR, *PCSTR;
typedef _Null_terminated_ PCSTR *PZPCSTR;

typedef _NullNull_terminated_ CHAR *PZZSTR;
typedef _NullNull_terminated_ CONST CHAR *PCZZSTR;

/* Pointer to an Asciiz string */
typedef _Null_terminated_ CHAR *PSZ;
typedef _Null_terminated_ CONST char *PCSZ;

/* UNICODE (Wide Character) types */
typedef wchar_t WCHAR;
typedef WCHAR *PWCHAR, *LPWCH, *PWCH;
typedef CONST WCHAR *LPCWCH, *PCWCH;
typedef _Null_terminated_ WCHAR *NWPSTR, *LPWSTR, *PWSTR;
typedef _Null_terminated_ PWSTR *PZPWSTR;
typedef _Null_terminated_ CONST PWSTR *PCZPWSTR;
typedef _Null_terminated_ WCHAR UNALIGNED *LPUWSTR, *PUWSTR;
typedef _Null_terminated_ CONST WCHAR *LPCWSTR, *PCWSTR;
typedef _Null_terminated_ PCWSTR *PZPCWSTR;
typedef _Null_terminated_ CONST WCHAR UNALIGNED *LPCUWSTR, *PCUWSTR;

typedef _NullNull_terminated_ WCHAR *PZZWSTR;
typedef _NullNull_terminated_ CONST WCHAR *PCZZWSTR;
typedef _NullNull_terminated_ WCHAR UNALIGNED *PUZZWSTR;
typedef _NullNull_terminated_ CONST WCHAR UNALIGNED *PCUZZWSTR;

typedef  WCHAR *PNZWCH;
typedef  CONST WCHAR *PCNZWCH;
typedef  WCHAR UNALIGNED *PUNZWCH;
typedef  CONST WCHAR UNALIGNED *PCUNZWCH;

#if (_WIN32_WINNT >= 0x0600) || (defined(__cplusplus) && defined(WINDOWS_ENABLE_CPLUSPLUS))
typedef CONST WCHAR *LPCWCHAR, *PCWCHAR;
typedef CONST WCHAR UNALIGNED *LPCUWCHAR, *PCUWCHAR;
typedef unsigned long UCSCHAR, *PUCSCHAR, *PUCSSTR;
typedef const UCSCHAR *PCUCSCHAR, *PCUCSSTR;
typedef UCSCHAR UNALIGNED *PUUCSCHAR, *PUUCSSTR;
typedef const UCSCHAR UNALIGNED *PCUUCSCHAR, *PCUUCSSTR;
#define UCSCHAR_INVALID_CHARACTER (0xffffffff)
#define MIN_UCSCHAR (0)
#define MAX_UCSCHAR (0x0010FFFF)
#endif /* _WIN32_WINNT >= 0x0600 */

#ifdef  UNICODE

#ifndef _TCHAR_DEFINED
typedef WCHAR TCHAR, *PTCHAR;
typedef WCHAR TUCHAR, *PTUCHAR;
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */
typedef LPWCH LPTCH, PTCH;
typedef LPCWCH LPCTCH, PCTCH;
typedef LPWSTR PTSTR, LPTSTR;
typedef LPCWSTR PCTSTR, LPCTSTR;
typedef LPUWSTR PUTSTR, LPUTSTR;
typedef LPCUWSTR PCUTSTR, LPCUTSTR;
typedef LPWSTR LP;
typedef PZZWSTR PZZTSTR;
typedef PCZZWSTR PCZZTSTR;
typedef PUZZWSTR PUZZTSTR;
typedef PCUZZWSTR PCUZZTSTR;
typedef PZPWSTR PZPTSTR;
typedef PNZWCH PNZTCH;
typedef PCNZWCH PCNZTCH;
typedef PUNZWCH PUNZTCH;
typedef PCUNZWCH PCUNZTCH;
#define __TEXT(quote) L##quote

#else /* UNICODE */

#ifndef _TCHAR_DEFINED
typedef char TCHAR, *PTCHAR;
typedef unsigned char TUCHAR, *PTUCHAR;
#define _TCHAR_DEFINED
#endif /* !_TCHAR_DEFINED */
typedef LPCH LPTCH, PTCH;
typedef LPCCH LPCTCH, PCTCH;
typedef LPSTR PTSTR, LPTSTR, PUTSTR, LPUTSTR;
typedef LPCSTR PCTSTR, LPCTSTR, PCUTSTR, LPCUTSTR;
typedef PZZSTR PZZTSTR, PUZZTSTR;
typedef PCZZSTR PCZZTSTR, PCUZZTSTR;
typedef PZPSTR PZPTSTR;
typedef PNZCH PNZTCH, PUNZTCH;
typedef PCNZCH PCNZTCH, PCUNZTCH;
#define __TEXT(quote) quote         // r_winnt

#endif /* UNICODE */                // r_winnt
#define TEXT(quote) __TEXT(quote)   // r_winnt

/* Cardinal Data Types */
typedef char CCHAR;
typedef CCHAR *PCCHAR;
typedef short CSHORT, *PCSHORT;
typedef ULONG CLONG, *PCLONG;

/* NLS basics (Locale and Language Ids) */
typedef unsigned long LCID, *PLCID;
typedef unsigned short LANGID;

#ifndef __COMPARTMENT_ID_DEFINED__
#define __COMPARTMENT_ID_DEFINED__
typedef enum
{
    UNSPECIFIED_COMPARTMENT_ID = 0,
    DEFAULT_COMPARTMENT_ID
} COMPARTMENT_ID, *PCOMPARTMENT_ID;
#endif /* __COMPARTMENT_ID_DEFINED__ */

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#endif
/* Used to store a non-float 8 byte aligned structure */
typedef struct _QUAD
{
    _ANONYMOUS_UNION union
    {
        __GNU_EXTENSION __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    } DUMMYUNIONNAME;
} QUAD, *PQUAD, UQUAD, *PUQUAD;

/* Large Integer Unions */
#if defined(MIDL_PASS)
typedef struct _LARGE_INTEGER {
#else
typedef union _LARGE_INTEGER {
    _ANONYMOUS_STRUCT struct
    {
        ULONG LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;
    struct
    {
        ULONG LowPart;
        LONG HighPart;
    } u;
#endif /* MIDL_PASS */
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#if defined(MIDL_PASS)
typedef struct _ULARGE_INTEGER {
#else
typedef union _ULARGE_INTEGER {
    _ANONYMOUS_STRUCT struct
    {
        ULONG LowPart;
        ULONG HighPart;
    } DUMMYSTRUCTNAME;
    struct
    {
        ULONG LowPart;
        ULONG HighPart;
    } u;
#endif /* MIDL_PASS */
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#ifdef _MSC_VER
#pragma warning(pop) /* disable:4201 */
#endif

/* Physical Addresses are always treated as 64-bit wide */
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

/* Locally Unique Identifier */
typedef struct _LUID {
    ULONG LowPart;
    LONG HighPart;
} LUID, *PLUID;

/* Native API Return Value Macros */
#define NT_SUCCESS(Status)              (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status)          ((((ULONG)(Status)) >> 30) == 1)
#define NT_WARNING(Status)              ((((ULONG)(Status)) >> 30) == 2)
#define NT_ERROR(Status)                ((((ULONG)(Status)) >> 30) == 3)

/* String Types */
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
#ifdef MIDL_PASS
  [size_is(MaximumLength / 2), length_is((Length) / 2)] PUSHORT Buffer;
#else
  _Field_size_bytes_part_(MaximumLength, Length) PWCH Buffer;
#endif
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

#define UNICODE_NULL ((WCHAR)0)
#define UNICODE_STRING_MAX_BYTES ((USHORT) 65534)
#define UNICODE_STRING_MAX_CHARS (32767)

typedef struct _CSTRING {
  USHORT Length;
  USHORT MaximumLength;
  CONST CHAR *Buffer;
} CSTRING, *PCSTRING;
#define ANSI_NULL ((CHAR)0)

typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
#ifdef MIDL_PASS
  [size_is(MaximumLength), length_is(Length) ]
#endif
  _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef CONST STRING* PCOEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;

typedef struct _STRING32 {
    USHORT   Length;
    USHORT   MaximumLength;
    ULONG  Buffer;
} STRING32, *PSTRING32,
  UNICODE_STRING32, *PUNICODE_STRING32,
  ANSI_STRING32, *PANSI_STRING32;

typedef struct _STRING64 {
    USHORT   Length;
    USHORT   MaximumLength;
    ULONGLONG  Buffer;
} STRING64, *PSTRING64,
  UNICODE_STRING64, *PUNICODE_STRING64,
  ANSI_STRING64, *PANSI_STRING64;

/* LangID and NLS */
#define MAKELANGID(p, s)       ((((USHORT)(s)) << 10) | (USHORT)(p))
#define PRIMARYLANGID(lgid)    ((USHORT)(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((USHORT)(lgid) >> 10)

#define NLS_VALID_LOCALE_MASK  0x000fffff

#define MAKELCID(lgid, srtid)  ((ULONG)((((ULONG)((USHORT)(srtid))) << 16) |  \
                                         ((ULONG)((USHORT)(lgid)))))
#define MAKESORTLCID(lgid, srtid, ver)                                        \
                               ((ULONG)((MAKELCID(lgid, srtid)) |             \
                                    (((ULONG)((USHORT)(ver))) << 20)))
#define LANGIDFROMLCID(lcid)   ((USHORT)(lcid))
#define SORTIDFROMLCID(lcid)   ((USHORT)((((ULONG)(lcid)) >> 16) & 0xf))
#define SORTVERSIONFROMLCID(lcid)  ((USHORT)((((ULONG)(lcid)) >> 20) & 0xf))

#define LOCALE_NAME_MAX_LENGTH   85

/* Object Attributes */
typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;
  PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

typedef struct _OBJECT_ATTRIBUTES32 {
  ULONG Length;
  ULONG RootDirectory;
  ULONG ObjectName;
  ULONG Attributes;
  ULONG SecurityDescriptor;
  ULONG SecurityQualityOfService;
} OBJECT_ATTRIBUTES32, *POBJECT_ATTRIBUTES32;
typedef CONST OBJECT_ATTRIBUTES32 *PCOBJECT_ATTRIBUTES32;

typedef struct _OBJECT_ATTRIBUTES64 {
  ULONG Length;
  ULONG64 RootDirectory;
  ULONG64 ObjectName;
  ULONG Attributes;
  ULONG64 SecurityDescriptor;
  ULONG64 SecurityQualityOfService;
} OBJECT_ATTRIBUTES64, *POBJECT_ATTRIBUTES64;
typedef CONST OBJECT_ATTRIBUTES64 *PCOBJECT_ATTRIBUTES64;

/* Values for the Attributes member */
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

/* Helper Macro */
#define InitializeObjectAttributes(p,n,a,r,s) { \
  (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
  (p)->RootDirectory = (r); \
  (p)->ObjectName = (n); \
  (p)->Attributes = (a); \
  (p)->SecurityDescriptor = (s); \
  (p)->SecurityQualityOfService = NULL; \
}

#define RTL_CONSTANT_OBJECT_ATTRIBUTES(n,a) { \
  sizeof(OBJECT_ATTRIBUTES), \
  NULL, \
  RTL_CONST_CAST(PUNICODE_STRING)(n), \
  a, \
  NULL, \
  NULL  \
}

#define RTL_INIT_OBJECT_ATTRIBUTES(n, a) \
    RTL_CONSTANT_OBJECT_ATTRIBUTES(n, a)

/* Product Types */
typedef enum _NT_PRODUCT_TYPE {
  NtProductWinNt = 1,
  NtProductLanManNt,
  NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef enum _EVENT_TYPE {
  NotificationEvent,
  SynchronizationEvent
} EVENT_TYPE;

typedef enum _TIMER_TYPE {
  NotificationTimer,
  SynchronizationTimer
} TIMER_TYPE;

typedef enum _WAIT_TYPE {
  WaitAll,
  WaitAny
} WAIT_TYPE;

/* Doubly Linked Lists */
typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

typedef struct LIST_ENTRY32 {
  ULONG Flink;
  ULONG Blink;
} LIST_ENTRY32, *PLIST_ENTRY32;

typedef struct LIST_ENTRY64 {
  ULONGLONG Flink;
  ULONGLONG Blink;
} LIST_ENTRY64, *PLIST_ENTRY64;

/* Singly Linked Lists */
typedef struct _SINGLE_LIST_ENTRY {
  struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

typedef struct _SINGLE_LIST_ENTRY32 {
    ULONG Next;
} SINGLE_LIST_ENTRY32, *PSINGLE_LIST_ENTRY32;

typedef struct _PROCESSOR_NUMBER {
  USHORT Group;
  UCHAR Number;
  UCHAR Reserved;
} PROCESSOR_NUMBER, *PPROCESSOR_NUMBER;

_IRQL_requires_same_
_Function_class_(EXCEPTION_ROUTINE)
typedef
EXCEPTION_DISPOSITION
NTAPI
EXCEPTION_ROUTINE(
    _Inout_ struct _EXCEPTION_RECORD *ExceptionRecord,
    _In_ PVOID EstablisherFrame,
    _Inout_ struct _CONTEXT *ContextRecord,
    _In_ PVOID DispatcherContext);

typedef EXCEPTION_ROUTINE *PEXCEPTION_ROUTINE;

typedef struct _GROUP_AFFINITY {
  KAFFINITY Mask;
  USHORT Group;
  USHORT Reserved[3];
} GROUP_AFFINITY, *PGROUP_AFFINITY;

/* Helper Macros */

#define RTL_FIELD_TYPE(type, field)    (((type*)0)->field)
#define RTL_BITS_OF(sizeOfArg)         (sizeof(sizeOfArg) * 8)
#define RTL_BITS_OF_FIELD(type, field) (RTL_BITS_OF(RTL_FIELD_TYPE(type, field)))

#ifdef __cplusplus
extern "C++" template<typename _Type> struct _RTL_remove_const_template;
extern "C++" template<typename _Type> struct _RTL_remove_const_template<const _Type&> { typedef _Type type; };
#define _RTL_CONSTANT_STRING_remove_const_macro(s) \
    (const_cast<_RTL_remove_const_template<decltype((s)[0])>::type*>(s))
extern "C++" template<class _Ty> struct _RTL_CONSTANT_STRING_type_check_template;
extern "C++" template<class _Ty, int _Count>	struct _RTL_CONSTANT_STRING_type_check_template<const _Ty (&)[_Count]> { typedef char type; };
#define _RTL_CONSTANT_STRING_type_check(s) _RTL_CONSTANT_STRING_type_check_template<decltype(s)>::type
#else
#define _RTL_CONSTANT_STRING_remove_const_macro(s) (s)
char _RTL_CONSTANT_STRING_type_check(const void *s);
#endif
#define RTL_CONSTANT_STRING(s) { \
    sizeof(s)-sizeof((s)[0]), \
    sizeof(s) / sizeof(_RTL_CONSTANT_STRING_type_check(s)), \
    _RTL_CONSTANT_STRING_remove_const_macro(s) }

#define RTL_FIELD_SIZE(type, field) (sizeof(((type *)0)->field))

#define RTL_SIZEOF_THROUGH_FIELD(type, field) \
    (FIELD_OFFSET(type, field) + RTL_FIELD_SIZE(type, field))

#define RTL_CONTAINS_FIELD(Struct, Size, Field) \
    ( (((PCHAR)(&(Struct)->Field)) + sizeof((Struct)->Field)) <= (((PCHAR)(Struct))+(Size)) )

#define RTL_NUMBER_OF_V1(A) (sizeof(A)/sizeof((A)[0]))

#ifdef __GNUC__
#define RTL_NUMBER_OF_V2(A) \
    (({ int _check_array_type[__builtin_types_compatible_p(typeof(A), typeof(&A[0])) ? -1 : 1]; (void)_check_array_type; }), \
    RTL_NUMBER_OF_V1(A))
#else
/// \todo implement security checks for cplusplus / MSVC
#define RTL_NUMBER_OF_V2(A) RTL_NUMBER_OF_V1(A)
#endif

#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif
#define ARRAYSIZE(A)    RTL_NUMBER_OF_V2(A)
#define _ARRAYSIZE(A)   RTL_NUMBER_OF_V1(A)

/* Type Limits */
#define MINCHAR   0x80
#define MAXCHAR   0x7f
#define MINSHORT  0x8000
#define MAXSHORT  0x7fff
#define MINLONG   0x80000000
#define MAXLONG   0x7fffffff
#define MAXUCHAR  0xff
#define MAXUSHORT 0xffff
#define MAXULONG  0xffffffff
#define MAXLONGLONG (0x7fffffffffffffffLL)

/* Multiplication and Shift Operations. Note: we don't use inline
   asm functions, the compiler can optimize this better. */
#define Int32x32To64(a,b) (((__int64)(long)(a))*((__int64)(long)(b)))
#define UInt32x32To64(a,b) ((unsigned __int64)(unsigned int)(a)*(unsigned __int64)(unsigned int)(b))

#if defined(MIDL_PASS)|| defined(RC_INVOKED) || defined(_M_CEE_PURE)
/* Use native math */
#define Int64ShllMod32(a,b) ((unsigned __int64)(a)<<(b))
#define Int64ShraMod32(a,b) (((__int64)(a))>>(b))
#define Int64ShrlMod32(a,b) (((unsigned __int64)(a))>>(b))
#else
/* Use intrinsics */
#define Int64ShllMod32(a,b) __ll_lshift(a,b)
#define Int64ShraMod32(a,b) __ll_rshift(a,b)
#define Int64ShrlMod32(a,b) __ull_rshift(a,b)
#endif

/* C_ASSERT Definition */
#define C_ASSERT(expr) extern char (*c_assert(void)) [(expr) ? 1 : -1]

/* Eliminate Microsoft C/C++ compiler warning 4715 */
#if defined(_MSC_VER)
# define DEFAULT_UNREACHABLE default: __assume(0)
#elif defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5))))
# define DEFAULT_UNREACHABLE default: __builtin_unreachable()
#else
# define DEFAULT_UNREACHABLE default: break
#endif

#define VER_WORKSTATION_NT                  0x40000000
#define VER_SERVER_NT                       0x80000000
#define VER_SUITE_SMALLBUSINESS             0x00000001
#define VER_SUITE_ENTERPRISE                0x00000002
#define VER_SUITE_BACKOFFICE                0x00000004
#define VER_SUITE_COMMUNICATIONS            0x00000008
#define VER_SUITE_TERMINAL                  0x00000010
#define VER_SUITE_SMALLBUSINESS_RESTRICTED  0x00000020
#define VER_SUITE_EMBEDDEDNT                0x00000040
#define VER_SUITE_DATACENTER                0x00000080
#define VER_SUITE_SINGLEUSERTS              0x00000100
#define VER_SUITE_PERSONAL                  0x00000200
#define VER_SUITE_BLADE                     0x00000400
#define VER_SUITE_EMBEDDED_RESTRICTED       0x00000800
#define VER_SUITE_SECURITY_APPLIANCE        0x00001000
#define VER_SUITE_STORAGE_SERVER            0x00002000
#define VER_SUITE_COMPUTE_SERVER            0x00004000
#define VER_SUITE_WH_SERVER                 0x00008000

/*  Primary language IDs. */
#define LANG_NEUTRAL                              0x00
#define LANG_INVARIANT                            0x7f

#define LANG_AFRIKAANS                            0x36
#define LANG_ALBANIAN                             0x1c
#define LANG_ALSATIAN                             0x84
#define LANG_AMHARIC                              0x5e
#define LANG_ARABIC                               0x01
#define LANG_ARMENIAN                             0x2b
#define LANG_ASSAMESE                             0x4d
#define LANG_AZERI                                0x2c
#define LANG_AZERBAIJANI                          0x2c
#define LANG_BANGLA                               0x45
#define LANG_BASHKIR                              0x6d
#define LANG_BASQUE                               0x2d
#define LANG_BELARUSIAN                           0x23
#define LANG_BENGALI                              0x45
#define LANG_BRETON                               0x7e
#define LANG_BOSNIAN                              0x1a
#define LANG_BOSNIAN_NEUTRAL                    0x781a
#define LANG_BULGARIAN                            0x02
#define LANG_CATALAN                              0x03
#define LANG_CENTRAL_KURDISH                      0x92
#define LANG_CHEROKEE                             0x5c
#define LANG_CHINESE                              0x04
#define LANG_CHINESE_SIMPLIFIED                   0x04
#define LANG_CHINESE_TRADITIONAL                0x7c04
#define LANG_CORSICAN                             0x83
#define LANG_CROATIAN                             0x1a
#define LANG_CZECH                                0x05
#define LANG_DANISH                               0x06
#define LANG_DARI                                 0x8c
#define LANG_DIVEHI                               0x65
#define LANG_DUTCH                                0x13
#define LANG_ENGLISH                              0x09
#define LANG_ESTONIAN                             0x25
#define LANG_FAEROESE                             0x38
#define LANG_FARSI                                0x29
#define LANG_FILIPINO                             0x64
#define LANG_FINNISH                              0x0b
#define LANG_FRENCH                               0x0c
#define LANG_FRISIAN                              0x62
#define LANG_FULAH                                0x67
#define LANG_GALICIAN                             0x56
#define LANG_GEORGIAN                             0x37
#define LANG_GERMAN                               0x07
#define LANG_GREEK                                0x08
#define LANG_GREENLANDIC                          0x6f
#define LANG_GUJARATI                             0x47
#define LANG_HAUSA                                0x68
#define LANG_HAWAIIAN                             0x75
#define LANG_HEBREW                               0x0d
#define LANG_HINDI                                0x39
#define LANG_HUNGARIAN                            0x0e
#define LANG_ICELANDIC                            0x0f
#define LANG_IGBO                                 0x70
#define LANG_INDONESIAN                           0x21
#define LANG_INUKTITUT                            0x5d
#define LANG_IRISH                                0x3c
#define LANG_ITALIAN                              0x10
#define LANG_JAPANESE                             0x11
#define LANG_KANNADA                              0x4b
#define LANG_KASHMIRI                             0x60
#define LANG_KAZAK                                0x3f
#define LANG_KHMER                                0x53
#define LANG_KICHE                                0x86
#define LANG_KINYARWANDA                          0x87
#define LANG_KONKANI                              0x57
#define LANG_KOREAN                               0x12
#define LANG_KYRGYZ                               0x40
#define LANG_LAO                                  0x54
#define LANG_LATVIAN                              0x26
#define LANG_LITHUANIAN                           0x27
#define LANG_LOWER_SORBIAN                        0x2e
#define LANG_LUXEMBOURGISH                        0x6e
#define LANG_MACEDONIAN                           0x2f
#define LANG_MALAY                                0x3e
#define LANG_MALAYALAM                            0x4c
#define LANG_MALTESE                              0x3a
#define LANG_MANIPURI                             0x58
#define LANG_MAORI                                0x81
#define LANG_MAPUDUNGUN                           0x7a
#define LANG_MARATHI                              0x4e
#define LANG_MOHAWK                               0x7c
#define LANG_MONGOLIAN                            0x50
#define LANG_NEPALI                               0x61
#define LANG_NORWEGIAN                            0x14
#define LANG_OCCITAN                              0x82
#define LANG_ODIA                                 0x48
#define LANG_ORIYA                                0x48
#define LANG_PASHTO                               0x63
#define LANG_PERSIAN                              0x29
#define LANG_POLISH                               0x15
#define LANG_PORTUGUESE                           0x16
#define LANG_PULAR                                0x67
#define LANG_PUNJABI                              0x46
#define LANG_QUECHUA                              0x6b
#define LANG_ROMANIAN                             0x18
#define LANG_ROMANSH                              0x17
#define LANG_RUSSIAN                              0x19
#define LANG_SAKHA                                0x85
#define LANG_SAMI                                 0x3b
#define LANG_SANSKRIT                             0x4f
#define LANG_SCOTTISH_GAELIC                      0x91
#define LANG_SERBIAN                              0x1a
#define LANG_SERBIAN_NEUTRAL                    0x7c1a
#define LANG_SINDHI                               0x59
#define LANG_SINHALESE                            0x5b
#define LANG_SLOVAK                               0x1b
#define LANG_SLOVENIAN                            0x24
#define LANG_SOTHO                                0x6c
#define LANG_SPANISH                              0x0a
#define LANG_SWAHILI                              0x41
#define LANG_SWEDISH                              0x1d
#define LANG_SYRIAC                               0x5a
#define LANG_TAJIK                                0x28
#define LANG_TAMAZIGHT                            0x5f
#define LANG_TAMIL                                0x49
#define LANG_TATAR                                0x44
#define LANG_TELUGU                               0x4a
#define LANG_THAI                                 0x1e
#define LANG_TIBETAN                              0x51
#define LANG_TIGRIGNA                             0x73
#define LANG_TIGRINYA                             0x73
#define LANG_TSWANA                               0x32
#define LANG_TURKISH                              0x1f
#define LANG_TURKMEN                              0x42
#define LANG_UIGHUR                               0x80
#define LANG_UKRAINIAN                            0x22
#define LANG_UPPER_SORBIAN                        0x2e
#define LANG_URDU                                 0x20
#define LANG_UZBEK                                0x43
#define LANG_VALENCIAN                            0x03
#define LANG_VIETNAMESE                           0x2a
#define LANG_WELSH                                0x52
#define LANG_WOLOF                                0x88
#define LANG_XHOSA                                0x34
#define LANG_YAKUT                                0x85
#define LANG_YI                                   0x78
#define LANG_YORUBA                               0x6a
#define LANG_ZULU                                 0x35

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _NTDEF_ */
