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
#define ASSERTMSG(x, m) assert(x)
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
typedef void VOID, *PVOID;
typedef char CHAR, *PCHAR, *PSTR;
typedef const char CCHAR, *PCSTR, *LPCSTR;
typedef unsigned char UCHAR, *PUCHAR, BYTE, *LPBYTE, BOOLEAN, *PBOOLEAN;
typedef short SHORT, *PSHORT;
typedef unsigned short USHORT, *PUSHORT, WORD, *PWORD, *LPWORD, WCHAR, *PWCHAR, *PWSTR, *LPWSTR;
typedef const unsigned short *PCWSTR, *LPCWSTR;
typedef int INT, LONG, *PLONG, *LPLONG, BOOL;
typedef unsigned int UINT, *PUINT, *LPUINT, ULONG, *PULONG, DWORD, *LPDWORD;
typedef long LONG_PTR, *PLONG_PTR, INT_PTR, *PINT_PTR;
typedef unsigned long ULONG_PTR, DWORD_PTR, *PULONG_PTR, UINT_PTR, *PUINT_PTR;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;

/* Derived types */
typedef PVOID HANDLE, HKEY, *PHKEY;
typedef INT NTSTATUS, POOL_TYPE;
typedef LONG HRESULT;
typedef ULONG_PTR SIZE_T, *PSIZE_T;

#define MAXUSHORT USHRT_MAX

/* Widely used structures */
#include <host/pshpack4.h>
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

typedef union _LARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        LONG  HighPart;
    };
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY,*PLIST_ENTRY;

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
#include <host/poppack.h>

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

BOOLEAN
static __inline
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

typedef const UNICODE_STRING *PCUNICODE_STRING;

/* Widely used macros */
#define LOBYTE(w)               ((BYTE)(w))
#define HIBYTE(w)               ((BYTE)(((WORD)(w)>>8)&0xFF))
#define LOWORD(l)               ((WORD)((DWORD_PTR)(l)))
#define HIWORD(l)               ((WORD)(((DWORD_PTR)(l)>>16)&0xFFFF))
#define MAKEWORD(a,b)           ((WORD)(((BYTE)(a))|(((WORD)((BYTE)(b)))<<8)))
#define MAKELONG(a,b)           ((LONG)(((WORD)(a))|(((DWORD)((WORD)(b)))<<16)))

#define NT_SUCCESS(x)           ((x)>=0)
#define FIELD_OFFSET(t,f)       ((LONG_PTR)&(((t*)0)->f))
#define RTL_CONSTANT_STRING(s)  { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#define CONTAINING_RECORD(address, type, field)  ((type *)(((ULONG_PTR)address) - (ULONG_PTR)(&(((type *)0)->field))))

#define RtlZeroMemory(Destination, Length)            memset(Destination, 0, Length)
#define RtlCopyMemory(Destination, Source, Length)    memcpy(Destination, Source, Length)
#define RtlMoveMemory(Destination, Source, Length)    memmove(Destination, Source, Length)

/* Prevent inclusion of some other headers */
#define __INTERNAL_DEBUG
#define RTL_H

#endif
