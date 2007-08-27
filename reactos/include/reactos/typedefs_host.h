/*
  PROJECT:    ReactOS
  LICENSE:    GPL v2 or any later version
  FILE:       include/reactos/typedefs_host.h
  PURPOSE:    Type definitions and useful macros for host tools
  COPYRIGHT:  Copyright 2007 Hervé Poussineau
*/

#ifndef _TYPEDEFS_HOST_H
#define _TYPEDEFS_HOST_H

#include <limits.h>

#define UNIMPLEMENTED { printf("%s unimplemented\n", __FUNCTION__); exit(1); }
#define ASSERT(x) { if (!(x)) { printf("ASSERT at %s:%d failed\n", __FILE__, __LINE__); exit(1); } }

#define NTAPI __stdcall
#define WINAPI __stdcall

#define IN
#define OUT
#define OPTIONAL

#define FALSE 0
#define TRUE (!(FALSE))

typedef void VOID, *PVOID, *HANDLE;
typedef HANDLE HKEY, *PHKEY;
typedef size_t SIZE_T, *PSIZE_T;
typedef unsigned char UCHAR, *PUCHAR, BYTE, *LPBYTE;
typedef char CHAR, *PCHAR, *PSTR;
typedef const char CCHAR;
typedef const char *PCSTR, *LPCSTR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned int LONG, *PLONG;
typedef unsigned int ULONG, *PULONG, DWORD;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef UCHAR BOOLEAN, *PBOOLEAN;
typedef int BOOL;
typedef int LONG_PTR, *PLONG_PTR;
typedef unsigned int ULONG_PTR, *PULONG_PTR;
typedef wchar_t WCHAR, *PWCHAR, *PWSTR, *LPWSTR;
typedef const wchar_t *PCWSTR, *LPCWSTR;
typedef int NTSTATUS;
typedef int POOL_TYPE;

#define MAXUSHORT USHRT_MAX

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
typedef const UNICODE_STRING *PCUNICODE_STRING;

#define NT_SUCCESS(x) ((x)>=0)
#define RTL_CONSTANT_STRING(s) { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#define RtlZeroMemory(Destination, Length) memset(Destination, 0, Length)
#define RtlCopyMemory(Destination, Source, Length) memcpy(Destination, Source, Length)
#define RtlMoveMemory(Destination, Source, Length) memmove(Destination, Source, Length)

/* Prevent inclusion of some other headers */
#define __INTERNAL_DEBUG
#define RTL_H
#define _TYPEDEFS64_H

#endif
