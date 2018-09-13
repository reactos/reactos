/*++ BUILD Version: 0001

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:    perfutil.h

Abstract:

    This file supports routines used to parse and create Performance Monitor Data
    Structures. It actually supports Performance Object types with multiple instances

Revision History:
    Sept 97 Borrowed from the SDK samples and slightly modified

--*/

#ifndef _PERFUTIL_H_
#define _PERFUTIL_H_

//
//  Utility macro.  This is used to reserve a DWORD multiple of bytes for Unicode strings
//  embedded in the definitional data, viz., object instance names.
//

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

//    (assumes dword is 4 bytes long and pointer is a dword in size)

#define ALIGN_ON_DWORD(x) ((VOID *)( ((DWORD) x & 0x00000003) ? ( ((DWORD) x & 0xFFFFFFFC) + 4 ) : ( (DWORD) x ) ))

extern WCHAR  GLOBAL_STRING[];          // Global command (get all local ctrs)
extern WCHAR  FOREIGN_STRING[];         // get data from foreign computers
extern WCHAR  COSTLY_STRING[];
extern WCHAR  NULL_STRING[];

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

//
// The definition of the only routine of perfutil.c, It builds part of a performance data
// instance (PERF_INSTANCE_DEFINITION) as described in winperf.h
//

HANDLE  MonOpenEventLog ();
VOID    MonCloseEventLog ();
DWORD   GetQueryType (IN LPWSTR);

//
// To Do:
// Add a flag like this for additional objects added.
// Used by IsNumberInUnicodeListand Collect.
//

// special value: query all counters but the counter values doesn't matter
#define     QUERY_NOCOUNTERS  0x0001
#define     QUERY_USER        0x0002
#define     QUERY_CS          0x0004
DWORD   IsNumberInUnicodeList (LPWSTR);

#define ALLOC(size)      HeapAlloc (GetProcessHeap(), 0, size)
#define ALLOCZERO(size)  HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, size)
#define REALLOC(pointer, newsize) \
                         HeapReAlloc (GetProcessHeap(), 0, pointer, newsize)
#define FREE(pointer)    HeapFree (GetProcessHeap(), 0, pointer)

#endif  //_PERFUTIL_H_
