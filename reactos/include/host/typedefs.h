/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/host/typedefs.h
  PURPOSE:    Type definitions and useful macros for host tools
  COPYRIGHT:  Copyright 2007 Hervé Poussineau
*/

#ifndef _TYPEDEFS_HOST_H
#define _TYPEDEFS_HOST_H

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#if defined(_MSC_VER)
#define W64 __w64
#elif defined(__GNUC__)
#define W64
#else
#error Unknown compiler
#endif

#if !defined(_MSC_VER) && !defined(__fastcall)
#define __fastcall __attribute__((fastcall))
#endif
#if !defined(_MSC_VER) && !defined(__cdecl)
#define __cdecl __attribute__((cdecl))
#endif
#if !defined(_MSC_VER) && !defined(__stdcall)
#define __stdcall __attribute__((stdcall))
#endif

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
#define TRUE (!(FALSE))

/* FIXME: this value is target specific, host tools MUST not use it
 * and this line has to be removed */
#define PAGE_SIZE 4096

#define ANYSIZE_ARRAY 1

typedef void VOID, *PVOID, *HANDLE;
typedef HANDLE HKEY, *PHKEY;
typedef unsigned char UCHAR, *PUCHAR, BYTE, *LPBYTE;
typedef char CHAR, *PCHAR, *PSTR;
typedef const char CCHAR;
typedef const char *PCSTR, *LPCSTR;
typedef short SHORT, *PSHORT;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned short WORD, *PWORD, *LPWORD;
typedef int LONG, *PLONG, *LPLONG;
typedef unsigned int ULONG, *PULONG, DWORD, *LPDWORD;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef UCHAR BOOLEAN, *PBOOLEAN;
typedef int BOOL;
typedef long int W64 LONG_PTR, *PLONG_PTR;
typedef long unsigned int W64 ULONG_PTR, DWORD_PTR, *PULONG_PTR;
typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef unsigned short WCHAR, *PWCHAR, *PWSTR, *LPWSTR;
typedef const unsigned short *PCWSTR, *LPCWSTR;
typedef int NTSTATUS;
typedef int POOL_TYPE;

#define MAXUSHORT USHRT_MAX

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

//
// List Functions
//
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

#define NT_SUCCESS(x) ((x)>=0)
#define FIELD_OFFSET(t,f) ((LONG_PTR)&(((t*)0)->f))
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#define CONTAINING_RECORD(address, type, field) ((type *)(((ULONG_PTR)address) - (ULONG_PTR)(&(((type *)0)->field))))
#define RtlZeroMemory(Destination, Length) memset(Destination, 0, Length)
#define RtlCopyMemory(Destination, Source, Length) memcpy(Destination, Source, Length)
#define RtlMoveMemory(Destination, Source, Length) memmove(Destination, Source, Length)

/* Prevent inclusion of some other headers */
#define __INTERNAL_DEBUG
#define RTL_H

#endif
