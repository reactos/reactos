/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/reactos/typedefs_host.h
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

typedef void VOID, *PVOID, *HANDLE;
typedef HANDLE HKEY, *PHKEY;
typedef unsigned char UCHAR, *PUCHAR, BYTE, *LPBYTE;
typedef char CHAR, *PCHAR, *PSTR;
typedef const char CCHAR;
typedef const char *PCSTR, *LPCSTR;
typedef short SHORT;
typedef unsigned short USHORT;
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

#include <pshpack4.h>
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
    } u;
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
#include <poppack.h>

typedef const UNICODE_STRING *PCUNICODE_STRING;

#define NT_SUCCESS(x) ((x)>=0)
#define FIELD_OFFSET(t,f) ((LONG_PTR)&(((t*)0)->f))
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#define RtlZeroMemory(Destination, Length) memset(Destination, 0, Length)
#define RtlCopyMemory(Destination, Source, Length) memcpy(Destination, Source, Length)
#define RtlMoveMemory(Destination, Source, Length) memmove(Destination, Source, Length)

/* Prevent inclusion of some other headers */
#define __INTERNAL_DEBUG
#define RTL_H
#define _TYPEDEFS64_H

#endif
