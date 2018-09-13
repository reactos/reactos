/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heaptag.c

Abstract:

    This module implements the support routines needed for FLG_HEAP_ENABLE_TAG_BY_DLL

Author:

    Steve Wood (stevewo) 07-Apr-1995

Revision History:

--*/

#include <ntos.h>
#include "ldrp.h"
#include <stktrace.h>
#include <heap.h>
#include <stdio.h>

#define LDRP_MAXIMUM_DLL_TAGS 64

BOOLEAN LdrpDllTagsInitialized;
ULONG LdrpNumberOfDllTags;
ULONG LdrpBaseDllTag;
ULONG LdrpDllTags[ LDRP_MAXIMUM_DLL_TAGS ];

#define DEFINE_HEAPTAG_ENTRY( n ) \
PVOID LdrpTagAllocateHeap##n( PVOID h, ULONG f, ULONG s ) {return LdrpTagAllocateHeap( h, f, s, n ); }

PVOID
LdrpTagAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN ULONG Size,
    IN ULONG n
    )
{
    if ((Flags & HEAP_TAG_MASK) == 0) {
        Flags |= LdrpDllTags[ n ];
        }

    return RtlAllocateHeap( HeapHandle, Flags, Size );
}

DEFINE_HEAPTAG_ENTRY(  0 );
DEFINE_HEAPTAG_ENTRY(  1 );
DEFINE_HEAPTAG_ENTRY(  2 );
DEFINE_HEAPTAG_ENTRY(  3 );
DEFINE_HEAPTAG_ENTRY(  4 );
DEFINE_HEAPTAG_ENTRY(  5 );
DEFINE_HEAPTAG_ENTRY(  6 );
DEFINE_HEAPTAG_ENTRY(  7 );
DEFINE_HEAPTAG_ENTRY(  8 );
DEFINE_HEAPTAG_ENTRY(  9 );
DEFINE_HEAPTAG_ENTRY( 10 );
DEFINE_HEAPTAG_ENTRY( 11 );
DEFINE_HEAPTAG_ENTRY( 12 );
DEFINE_HEAPTAG_ENTRY( 13 );
DEFINE_HEAPTAG_ENTRY( 14 );
DEFINE_HEAPTAG_ENTRY( 15 );
DEFINE_HEAPTAG_ENTRY( 16 );
DEFINE_HEAPTAG_ENTRY( 17 );
DEFINE_HEAPTAG_ENTRY( 18 );
DEFINE_HEAPTAG_ENTRY( 19 );
DEFINE_HEAPTAG_ENTRY( 20 );
DEFINE_HEAPTAG_ENTRY( 21 );
DEFINE_HEAPTAG_ENTRY( 22 );
DEFINE_HEAPTAG_ENTRY( 23 );
DEFINE_HEAPTAG_ENTRY( 24 );
DEFINE_HEAPTAG_ENTRY( 25 );
DEFINE_HEAPTAG_ENTRY( 26 );
DEFINE_HEAPTAG_ENTRY( 27 );
DEFINE_HEAPTAG_ENTRY( 28 );
DEFINE_HEAPTAG_ENTRY( 29 );
DEFINE_HEAPTAG_ENTRY( 30 );
DEFINE_HEAPTAG_ENTRY( 31 );
DEFINE_HEAPTAG_ENTRY( 32 );
DEFINE_HEAPTAG_ENTRY( 33 );
DEFINE_HEAPTAG_ENTRY( 34 );
DEFINE_HEAPTAG_ENTRY( 35 );
DEFINE_HEAPTAG_ENTRY( 36 );
DEFINE_HEAPTAG_ENTRY( 37 );
DEFINE_HEAPTAG_ENTRY( 38 );
DEFINE_HEAPTAG_ENTRY( 39 );
DEFINE_HEAPTAG_ENTRY( 40 );
DEFINE_HEAPTAG_ENTRY( 41 );
DEFINE_HEAPTAG_ENTRY( 42 );
DEFINE_HEAPTAG_ENTRY( 43 );
DEFINE_HEAPTAG_ENTRY( 44 );
DEFINE_HEAPTAG_ENTRY( 45 );
DEFINE_HEAPTAG_ENTRY( 46 );
DEFINE_HEAPTAG_ENTRY( 47 );
DEFINE_HEAPTAG_ENTRY( 48 );
DEFINE_HEAPTAG_ENTRY( 49 );
DEFINE_HEAPTAG_ENTRY( 50 );
DEFINE_HEAPTAG_ENTRY( 51 );
DEFINE_HEAPTAG_ENTRY( 52 );
DEFINE_HEAPTAG_ENTRY( 53 );
DEFINE_HEAPTAG_ENTRY( 54 );
DEFINE_HEAPTAG_ENTRY( 55 );
DEFINE_HEAPTAG_ENTRY( 56 );
DEFINE_HEAPTAG_ENTRY( 57 );
DEFINE_HEAPTAG_ENTRY( 58 );
DEFINE_HEAPTAG_ENTRY( 59 );
DEFINE_HEAPTAG_ENTRY( 60 );
DEFINE_HEAPTAG_ENTRY( 61 );
DEFINE_HEAPTAG_ENTRY( 62 );
DEFINE_HEAPTAG_ENTRY( 63 );

typedef PVOID (*PLDRP_DLL_TAG_PROCEDURE)(
    PVOID HeapHandle,
    ULONG Flags,
    ULONG Size
    );

const PLDRP_DLL_TAG_PROCEDURE LdrpDllTagProcedures[ LDRP_MAXIMUM_DLL_TAGS ] = {
    LdrpTagAllocateHeap0,
    LdrpTagAllocateHeap1,
    LdrpTagAllocateHeap2,
    LdrpTagAllocateHeap3,
    LdrpTagAllocateHeap4,
    LdrpTagAllocateHeap5,
    LdrpTagAllocateHeap6,
    LdrpTagAllocateHeap7,
    LdrpTagAllocateHeap8,
    LdrpTagAllocateHeap9,
    LdrpTagAllocateHeap10,
    LdrpTagAllocateHeap11,
    LdrpTagAllocateHeap12,
    LdrpTagAllocateHeap13,
    LdrpTagAllocateHeap14,
    LdrpTagAllocateHeap15,
    LdrpTagAllocateHeap16,
    LdrpTagAllocateHeap17,
    LdrpTagAllocateHeap18,
    LdrpTagAllocateHeap19,
    LdrpTagAllocateHeap20,
    LdrpTagAllocateHeap21,
    LdrpTagAllocateHeap22,
    LdrpTagAllocateHeap23,
    LdrpTagAllocateHeap24,
    LdrpTagAllocateHeap25,
    LdrpTagAllocateHeap26,
    LdrpTagAllocateHeap27,
    LdrpTagAllocateHeap28,
    LdrpTagAllocateHeap29,
    LdrpTagAllocateHeap30,
    LdrpTagAllocateHeap31,
    LdrpTagAllocateHeap32,
    LdrpTagAllocateHeap33,
    LdrpTagAllocateHeap34,
    LdrpTagAllocateHeap35,
    LdrpTagAllocateHeap36,
    LdrpTagAllocateHeap37,
    LdrpTagAllocateHeap38,
    LdrpTagAllocateHeap39,
    LdrpTagAllocateHeap40,
    LdrpTagAllocateHeap41,
    LdrpTagAllocateHeap42,
    LdrpTagAllocateHeap43,
    LdrpTagAllocateHeap44,
    LdrpTagAllocateHeap45,
    LdrpTagAllocateHeap46,
    LdrpTagAllocateHeap47,
    LdrpTagAllocateHeap48,
    LdrpTagAllocateHeap49,
    LdrpTagAllocateHeap50,
    LdrpTagAllocateHeap51,
    LdrpTagAllocateHeap52,
    LdrpTagAllocateHeap53,
    LdrpTagAllocateHeap54,
    LdrpTagAllocateHeap55,
    LdrpTagAllocateHeap56,
    LdrpTagAllocateHeap57,
    LdrpTagAllocateHeap58,
    LdrpTagAllocateHeap59,
    LdrpTagAllocateHeap60,
    LdrpTagAllocateHeap61,
    LdrpTagAllocateHeap62,
    LdrpTagAllocateHeap63
};

PVOID
LdrpDefineDllTag(
    PWSTR TagName,
    PUSHORT TagIndex
    )
{
    PVOID Result;
    WCHAR TagNameBuffer[ 260 ];

    if (RtlpGlobalTagHeap == NULL) {
        RtlpGlobalTagHeap = RtlAllocateHeap( RtlProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( HEAP ));
        if (RtlpGlobalTagHeap == NULL) {
            return NULL;
        }
    }
    
    if (!LdrpDllTagsInitialized) {
        //
        // Keep QUERY.C happy
        //
        InitializeListHead( &RtlpGlobalTagHeap->VirtualAllocdBlocks );
        LdrpDllTagsInitialized = TRUE;
        }

    Result = NULL;
    if (LdrpNumberOfDllTags < LDRP_MAXIMUM_DLL_TAGS) {
        memset( TagNameBuffer, 0, sizeof( TagNameBuffer ) );
        wcscpy( TagNameBuffer, TagName );
        LdrpDllTags[ LdrpNumberOfDllTags ] =
            RtlCreateTagHeap( NULL,
                              0,
                              NULL,
                              TagNameBuffer
                            );

        if (LdrpDllTags[ LdrpNumberOfDllTags ] != 0) {
            Result = LdrpDllTagProcedures[ LdrpNumberOfDllTags ];
            }

        if (Result != NULL) {
            *TagIndex = (USHORT)(LdrpDllTags[ LdrpNumberOfDllTags ] >> HEAP_TAG_SHIFT);
            LdrpNumberOfDllTags += 1;
            }
        }

    return Result;
}
