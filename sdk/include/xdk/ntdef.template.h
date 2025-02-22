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
$if(0)
//#include <winapifamily.h>
$endif()
#include <basetsd.h>
#include <guiddef.h>
#include <excpt.h>
#include <sdkddkver.h>
#include <specstrings.h>
#include <kernelspecs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default to strict */
#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
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

#define ARGUMENT_PRESENT(ArgumentPointer) \
  ((CHAR*)((ULONG_PTR)(ArgumentPointer)) != (CHAR*)NULL)

#if defined(_MANAGED)
 #define FASTCALL __stdcall
#elif defined(_M_IX86)
 #define FASTCALL __fastcall
#else
 #define FASTCALL
#endif /* _MANAGED */

/* min/max helper macros */
#ifndef NOMINMAX
# ifndef min
#  define min(a,b) (((a) < (b)) ? (a) : (b))
# endif
# ifndef max
#  define max(a,b) (((a) > (b)) ? (a) : (b))
# endif
#endif /* NOMINMAX */

/* Tell windef.h that we have defined some basic types */
#define BASETYPES

$define(_NTDEF_)
$define(ULONG=ULONG)
$define(USHORT=USHORT)
$define(UCHAR=UCHAR)
$include(ntbasedef.h)

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS *PNTSTATUS;

#ifndef __SECSTATUS_DEFINED__
typedef long SECURITY_STATUS;
#define __SECSTATUS_DEFINED__
#endif

/* Physical Addresses are always treated as 64-bit wide */
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

#define TIME LARGE_INTEGER
#define _TIME _LARGE_INTEGER
#define PTIME PLARGE_INTEGER
#define LowTime LowPart
#define HighTime HighPart

/* Used to store a non-float 8 byte aligned structure */
typedef struct _QUAD
{
    _ANONYMOUS_UNION union
    {
        __GNU_EXTENSION __int64 UseThisFieldToCopy;
        double DoNotUseThisField;
    } DUMMYUNIONNAME;
} QUAD, *PQUAD, UQUAD, *PUQUAD;

#if (_WIN32_WINNT >= 0x0600) || (defined(__cplusplus) && defined(WINDOWS_ENABLE_CPLUSPLUS))
typedef CONST UCHAR *PCUCHAR;
typedef CONST USHORT *PCUSHORT;
typedef CONST ULONG *PCULONG;
typedef CONST UQUAD *PCUQUAD;
typedef CONST SCHAR *PCSCHAR;
#endif /* (/_WIN32_WINNT >= 0x0600) */
#if (_WIN32_WINNT >= 0x0600)
typedef CONST NTSTATUS *PCNTSTATUS;
#endif /* (/_WIN32_WINNT >= 0x0600) */

/* String Types */
typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
#ifdef MIDL_PASS
  [size_is(MaximumLength), length_is(Length) ]
#endif
  _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING, *PSTRING,
  ANSI_STRING, *PANSI_STRING,
  OEM_STRING, *POEM_STRING;

typedef CONST STRING* PCOEM_STRING;
typedef STRING CANSI_STRING;
typedef PSTRING PCANSI_STRING;

typedef struct _STRING32 {
  USHORT   Length;
  USHORT   MaximumLength;
  $ULONG  Buffer;
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

typedef struct _CSTRING {
  USHORT Length;
  USHORT MaximumLength;
  CONST CHAR *Buffer;
} CSTRING, *PCSTRING;

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

typedef USHORT RTL_STRING_LENGTH_TYPE;

#ifdef __cplusplus
extern "C++" template<typename _Type> struct _RTL_remove_const_template;
extern "C++" template<typename _Type> struct _RTL_remove_const_template<const _Type&> { typedef _Type type; };
#define _RTL_CONSTANT_STRING_remove_const_macro(s) \
    (const_cast<_RTL_remove_const_template<decltype((s)[0])>::type*>(s))
extern "C++" template<class _Ty> struct _RTL_CONSTANT_STRING_type_check_template;
extern "C++" template<class _Ty, int _Count>	struct _RTL_CONSTANT_STRING_type_check_template<const _Ty (&)[_Count]> { typedef char type; };
#define _RTL_CONSTANT_STRING_type_check(s) _RTL_CONSTANT_STRING_type_check_template<decltype(s)>::type
#else
# define _RTL_CONSTANT_STRING_remove_const_macro(s) (s)
char _RTL_CONSTANT_STRING_type_check(const void *s);
#endif
#define RTL_CONSTANT_STRING(s) { \
    sizeof(s)-sizeof((s)[0]), \
    sizeof(s) / (sizeof(_RTL_CONSTANT_STRING_type_check(s))), \
    _RTL_CONSTANT_STRING_remove_const_macro(s) }

#ifdef _MSC_VER
#define DECLARE_UNICODE_STRING_SIZE(_var, _size) \
  WCHAR _var ## _buffer[_size]; \
  __pragma(warning(push)) __pragma(warning(disable:4221)) __pragma(warning(disable:4204)) \
  UNICODE_STRING _var = { 0, (_size) * sizeof(WCHAR) , _var ## _buffer } \
  __pragma(warning(pop))

#define DECLARE_CONST_UNICODE_STRING(_var, _string) \
  const WCHAR _var##_buffer[] = _string; \
  __pragma(warning(push)) __pragma(warning(disable:4221)) __pragma(warning(disable:4204)) \
  const UNICODE_STRING _var = { sizeof(_string) - sizeof(WCHAR), sizeof(_string), (PWCH)_var##_buffer } \
  __pragma(warning(pop))
#else
#define DECLARE_UNICODE_STRING_SIZE(_var, _size) \
  WCHAR _var ## _buffer[_size]; \
  UNICODE_STRING _var = { 0, (_size) * sizeof(WCHAR) , _var ## _buffer }

#define DECLARE_CONST_UNICODE_STRING(_var, _string) \
  const WCHAR _var##_buffer[] = _string; \
  const UNICODE_STRING _var = { sizeof(_string) - sizeof(WCHAR), sizeof(_string), (PWCH)_var##_buffer }
#endif

#define DECLARE_GLOBAL_CONST_UNICODE_STRING(_var, _str) \
  extern const __declspec(selectany) UNICODE_STRING _var = RTL_CONSTANT_STRING(_str)

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

#define OBJ_HANDLE_TAGBITS      0x00000003L

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

#ifdef _MSC_VER
 #pragma warning(push)
 #pragma warning(disable:4214) /* Bit fields of other types than int */
#endif /* _MSC_VER */
typedef struct _RTL_BALANCED_NODE
{
    _ANONYMOUS_UNION union
    {
        struct _RTL_BALANCED_NODE *Children[2];
        _ANONYMOUS_STRUCT struct
        {
            struct _RTL_BALANCED_NODE *Left;
            struct _RTL_BALANCED_NODE *Right;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    _ANONYMOUS_UNION union
    {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    } DUMMYUNIONNAME2;
} RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;
#ifdef _MSC_VER
 #pragma warning(pop)
#endif /* _MSC_VER */

#define RTL_BALANCED_NODE_RESERVED_PARENT_MASK 3
#define RTL_BALANCED_NODE_GET_PARENT_POINTER(Node) \
    ((PRTL_BALANCED_NODE)((Node)->ParentValue & \
                          ~RTL_BALANCED_NODE_RESERVED_PARENT_MASK))

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

#ifndef MIDL_PASS
FORCEINLINE
VOID
ListEntry32To64(
    _In_ PLIST_ENTRY32 ListEntry32,
    _Out_ PLIST_ENTRY64 ListEntry64)
{
    ListEntry64->Flink = ListEntry32->Flink;
    ListEntry64->Blink = ListEntry32->Blink;
}

FORCEINLINE
VOID
ListEntry64To32(
    _In_ PLIST_ENTRY64 ListEntry64,
    _Out_ PLIST_ENTRY32 ListEntry32)
{
    /* ASSERT without ASSERT or intrinsics ... */
    if (((ListEntry64->Flink >> 32) != 0) ||
        ((ListEntry64->Blink >> 32) != 0))
    {
        (VOID)*(volatile LONG*)(LONG_PTR)-1;
    }
    ListEntry32->Flink = ListEntry64->Flink & 0xFFFFFFFF;
    ListEntry32->Blink = ListEntry64->Blink & 0xFFFFFFFF;
}
#endif /* !MIDL_PASS */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _NTDEF_ */
