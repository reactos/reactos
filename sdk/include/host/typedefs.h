/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/host/typedefs.h
  PURPOSE:    Type definitions and useful macros for host tools
  COPYRIGHT:  Copyright 2007 Hervé Poussineau
              Copyright 2007 Colin Finck <mail@colinfinck.de>
*/

#ifndef _TYPEDEFS_HOST_H
#define _TYPEDEFS_HOST_H

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;

typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

/* Function attributes for GCC */
#if !defined(_MSC_VER) && !defined(__fastcall)
#define __fastcall __attribute__((fastcall))
#endif
#if !defined(_MSC_VER) && !defined(__cdecl)
#define __cdecl __attribute__((cdecl))
#endif
#if !defined(_MSC_VER) && !defined(__stdcall)
#define __stdcall __attribute__((stdcall))
#endif

/* Basic definitions */
#define UNIMPLEMENTED { printf("%s unimplemented\n", __FUNCTION__); exit(1); }
#define ASSERT(x) assert(x)
#define ASSERTMSG(m, x) assert(x)
#define DPRINT if (0) printf
#define DPRINT1 printf

#define NTAPI
#define WINAPI

#define IN
#define OUT
#define OPTIONAL

#define FALSE 0
#define TRUE  1

#define ANYSIZE_ARRAY 1

/* Basic types
   Emulate a LLP64 memory model using a LP64 compiler */
typedef void VOID, *PVOID, *LPVOID;
typedef char CHAR, CCHAR, *PCHAR, *PSTR, *LPSTR;
typedef const char *PCSTR, *LPCSTR;
typedef unsigned char UCHAR, *PUCHAR, BYTE, *LPBYTE, BOOLEAN, *PBOOLEAN;
typedef int16_t SHORT, *PSHORT;
typedef uint16_t USHORT, *PUSHORT, WORD, *PWORD, *LPWORD, WCHAR, *PWCHAR, *PWSTR, *LPWSTR, UINT16;
typedef const uint16_t *PCWSTR, *LPCWSTR;
typedef int32_t INT, LONG, *PLONG, *LPLONG, BOOL, WINBOOL;
typedef uint32_t UINT, *PUINT, *LPUINT, ULONG, *PULONG, DWORD, *PDWORD, *LPDWORD, UINT32;
#if defined(_LP64) || defined(_WIN64)
typedef int64_t LONG_PTR, *PLONG_PTR, INT_PTR, *PINT_PTR;
typedef uint64_t ULONG_PTR, DWORD_PTR, *PULONG_PTR, UINT_PTR, *PUINT_PTR;
#else
typedef int32_t LONG_PTR, *PLONG_PTR, INT_PTR, *PINT_PTR;
typedef uint32_t ULONG_PTR, DWORD_PTR, *PULONG_PTR, UINT_PTR, *PUINT_PTR;
#endif
typedef uint64_t ULONG64, DWORD64, *PDWORD64, UINT64, ULONGLONG;
typedef int64_t LONGLONG, LONG64;
typedef float FLOAT;
typedef double DOUBLE;

/* Derived types */
typedef PVOID HANDLE;
#ifndef _HAVE_HKEY
typedef HANDLE HKEY, *PHKEY;
#endif
typedef HANDLE HMODULE, HINSTANCE;
typedef INT NTSTATUS, POOL_TYPE;
typedef LONG HRESULT;
typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef WORD LANGID;

#define MAXUSHORT USHRT_MAX

/* Widely used structures */
#include <pshpack4.h>
#ifndef _HAVE_RTL_BITMAP
typedef struct _RTL_BITMAP
{
    ULONG  SizeOfBitMap;
    PULONG  Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN
{
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;
#endif

#ifndef _HAVE_LARGE_INTEGER
typedef union _LARGE_INTEGER
{
    struct
    {
        ULONG LowPart;
        LONG HighPart;
    };
    struct
    {
        ULONG LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
#endif

#ifndef _HAVE_LIST_ENTRY
typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY,*PLIST_ENTRY;
#endif

#ifndef _HAVE_ANSI_STRING
typedef struct _ANSI_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PSTR   Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

#include <poppack.h>

#ifndef _HAVE_LIST_ENTRY
/* List Functions */
static __inline
VOID
InitializeListHead(
                   IN PLIST_ENTRY ListHead
                   )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

static __inline
VOID
InsertHeadList(
               IN PLIST_ENTRY ListHead,
               IN PLIST_ENTRY Entry
               )
{
    PLIST_ENTRY OldFlink;
    OldFlink = ListHead->Flink;
    Entry->Flink = OldFlink;
    Entry->Blink = ListHead;
    OldFlink->Blink = Entry;
    ListHead->Flink = Entry;
}

static __inline
VOID
InsertTailList(
               IN PLIST_ENTRY ListHead,
               IN PLIST_ENTRY Entry
               )
{
    PLIST_ENTRY OldBlink;
    OldBlink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = OldBlink;
    OldBlink->Flink = Entry;
    ListHead->Blink = Entry;
}

static __inline
BOOLEAN
IsListEmpty(
            IN const LIST_ENTRY * ListHead
            )
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

static __inline
BOOLEAN
RemoveEntryList(
                IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY OldFlink;
    PLIST_ENTRY OldBlink;

    OldFlink = Entry->Flink;
    OldBlink = Entry->Blink;
    OldFlink->Blink = OldBlink;
    OldBlink->Flink = OldFlink;
    return (BOOLEAN)(OldFlink == OldBlink);
}

static __inline
PLIST_ENTRY
RemoveHeadList(
               IN PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

static __inline
PLIST_ENTRY
RemoveTailList(
               IN PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}
#endif

#ifndef _HAVE_ANSI_STRING
typedef const UNICODE_STRING *PCUNICODE_STRING;
#endif

/* Widely used macros */
#define LOBYTE(w)               ((BYTE)(w))
#define HIBYTE(w)               ((BYTE)(((WORD)(w)>>8)&0xFF))
#define LOWORD(l)               ((WORD)((DWORD_PTR)(l)))
#define HIWORD(l)               ((WORD)(((DWORD_PTR)(l)>>16)&0xFFFF))
#define MAKEWORD(a,b)           ((WORD)(((BYTE)(a))|(((WORD)((BYTE)(b)))<<8)))
#define MAKELONG(a,b)           ((LONG)(((WORD)(a))|(((DWORD)((WORD)(b)))<<16)))

#define MAXULONG 0xFFFFFFFF

#define NT_SUCCESS(x)           ((x)>=0)
#if !defined(__GNUC__)
#define FIELD_OFFSET(t,f)       ((LONG)(LONG_PTR)&(((t*) 0)->f))
#else
#define FIELD_OFFSET(t,f)       ((LONG)__builtin_offsetof(t,f))
#endif
#define RTL_CONSTANT_STRING(s)  { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#define CONTAINING_RECORD(address, type, field)  ((type *)(((ULONG_PTR)address) - (ULONG_PTR)(&(((type *)0)->field))))

#define RtlZeroMemory(Destination, Length)            memset(Destination, 0, Length)
#define RtlCopyMemory(Destination, Source, Length)    memcpy(Destination, Source, Length)
#define RtlMoveMemory(Destination, Source, Length)    memmove(Destination, Source, Length)

#define MAKELANGID(p,s)         ((((WORD)(s))<<10)|(WORD)(p))
#define PRIMARYLANGID(l)        ((WORD)(l)&0x3ff)
#define SUBLANGID(l)            ((WORD)(l)>>10)
#define SUBLANG_NEUTRAL         0x00

/* Prevent inclusion of some other headers */
#define __INTERNAL_DEBUG
#define RTL_H

#endif

