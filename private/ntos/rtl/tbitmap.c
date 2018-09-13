/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tbitmap.c

Abstract:

    Test program for the Bitmap Procedures

Author:

    Gary Kimura     [GaryKi]    30-Jan-1989

Revision History:

--*/

#include <stdio.h>

#include "nt.h"
#include "ntrtl.h"

ULONG Buffer[512];
RTL_BITMAP BitMapHeader;
PRTL_BITMAP BitMap;

int
main(
    int argc,
    char *argv[]
    )
{
    ULONG j;

    DbgPrint("Start BitMapTest()\n");

    //
    //  First create a new bitmap
    //

    BitMap = &BitMapHeader;
    RtlInitializeBitMap( BitMap, Buffer, 2048*8 );

    //
    //  >>>> Test setting bits
    //

    //
    //  Now clear all bits
    //

    RtlClearAllBits( BitMap );
    if (RtlNumberOfClearBits( BitMap ) != 2048*8) { DbgPrint("Number of Clear bits error 1\n" ); }
    if (RtlNumberOfSetBits( BitMap ) != 0)        { DbgPrint("Number of Set bits error 1\n" ); }

    //
    //  Now set some bit patterns, and test them
    //

    RtlSetBits( BitMap,   0,  1 );
    RtlSetBits( BitMap,  63,  1 );
    RtlSetBits( BitMap,  65, 30 );
    RtlSetBits( BitMap, 127,  2 );
    RtlSetBits( BitMap, 191, 34 );

    if ((BitMap->Buffer[0] != 0x00000001) ||
        (BitMap->Buffer[1] != 0x80000000) ||
        (BitMap->Buffer[2] != 0x7ffffffe) ||
        (BitMap->Buffer[3] != 0x80000000) ||
        (BitMap->Buffer[4] != 0x00000001) ||
        (BitMap->Buffer[5] != 0x80000000) ||
        (BitMap->Buffer[6] != 0xffffffff) ||
        (BitMap->Buffer[7] != 0x00000001)) {

        DbgPrint("RtlSetBits Error\n");
        return FALSE;
    }

    if (RtlNumberOfClearBits( BitMap ) != 2048*8 - 68) { DbgPrint("Number of Clear bits error 2\n" ); }
    if (RtlNumberOfSetBits( BitMap ) != 68)        { DbgPrint("Number of Set bits error 2\n" ); }

    //
    //  Now test some RtlFindClearBitsAndSet
    //

    RtlSetAllBits( BitMap );

    RtlClearBits( BitMap, 0 +  10*32,  1 );
    RtlClearBits( BitMap, 5 +  11*32,  1 );
    RtlClearBits( BitMap, 7 +  12*32,  1 );

    RtlClearBits( BitMap, 0 +  13*32,  9 );
    RtlClearBits( BitMap, 4 +  14*32,  9 );
    RtlClearBits( BitMap, 7 +  15*32,  9 );

    RtlClearBits( BitMap, 0 +  16*32, 10 );
    RtlClearBits( BitMap, 4 +  17*32, 10 );
    RtlClearBits( BitMap, 6 +  18*32, 10 );
    RtlClearBits( BitMap, 7 +  19*32, 10 );

    RtlClearBits( BitMap, 0 + 110*32, 14 );
    RtlClearBits( BitMap, 1 + 111*32, 14 );
    RtlClearBits( BitMap, 2 + 112*32, 14 );

    RtlClearBits( BitMap, 0 + 113*32, 15 );
    RtlClearBits( BitMap, 1 + 114*32, 15 );
    RtlClearBits( BitMap, 2 + 115*32, 15 );

    //DbgPrint("ClearBits = %08lx, ", RtlNumberOfClearBits( BitMap ));
    //DbgPrint("SetBits   = %08lx\n", RtlNumberOfSetBits( BitMap ));

//    {
//        ULONG i;
//        for (i = 0; i < 16; i++) {
//            DbgPrint("%2d: ", i);
//            DbgPrint("%08lx\n", BitMap->Buffer[i]);
//        }
//    }

    if (RtlFindClearBitsAndSet(BitMap, 15, 0) != 0 + 113*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 113*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 15, 0) != 1 + 114*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    1 + 114*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 15, 0) != 2 + 115*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    2 + 115*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap, 14, 0) != 0 + 110*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 110*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 14, 0) != 1 + 111*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    1 + 111*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 14, 0) != 2 + 112*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    2 + 112*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 0 + 16*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 16*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 4 + 17*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    4 + 17*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 6 + 18*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    6 + 18*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 7 + 19*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    7 + 19*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap,  9, 0) != 0 + 13*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 13*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  9, 0) != 4 + 14*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    4 + 14*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  9, 0) != 7 + 15*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    7 + 15*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap,  1, 0) != 0 + 10*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 10*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  1, 0) != 5 + 11*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    5 + 11*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  1, 0) != 7 + 12*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    7 + 12*32\n");
        return FALSE;
    }

    if (RtlNumberOfClearBits( BitMap ) != 0) { DbgPrint("Number of Clear bits error 3\n" ); }
    if (RtlNumberOfSetBits( BitMap ) != 2048*8)        { DbgPrint("Number of Set bits error 3\n" ); }

    //
    //  Now test some RtlFindClearBitsAndSet
    //

    RtlSetAllBits( BitMap );

    RtlClearBits( BitMap, 0 +  0*32,  1 );
    RtlClearBits( BitMap, 5 +  1*32,  1 );
    RtlClearBits( BitMap, 7 +  2*32,  1 );

    RtlClearBits( BitMap, 0 +  3*32,  9 );
    RtlClearBits( BitMap, 4 +  4*32,  9 );
    RtlClearBits( BitMap, 7 +  5*32,  9 );

    RtlClearBits( BitMap, 0 +  6*32, 10 );
    RtlClearBits( BitMap, 4 +  7*32, 10 );
    RtlClearBits( BitMap, 6 +  8*32, 10 );
    RtlClearBits( BitMap, 7 +  9*32, 10 );

    RtlClearBits( BitMap, 0 + 10*32, 14 );
    RtlClearBits( BitMap, 1 + 11*32, 14 );
    RtlClearBits( BitMap, 2 + 12*32, 14 );

    RtlClearBits( BitMap, 0 + 13*32, 15 );
    RtlClearBits( BitMap, 1 + 14*32, 15 );
    RtlClearBits( BitMap, 2 + 15*32, 15 );

    //DbgPrint("ClearBits = %08lx, ", RtlNumberOfClearBits( BitMap ));
    //DbgPrint("SetBits   = %08lx\n", RtlNumberOfSetBits( BitMap ));

//    {
//        ULONG i;
//        for (i = 0; i < 16; i++) {
//            DbgPrint("%2d: ", i);
//            DbgPrint("%08lx\n", BitMap->Buffer[i]);
//        }
//    }

    if (RtlFindClearBitsAndSet(BitMap, 15, 0) != 0 + 13*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 13*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 15, 0) != 1 + 14*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    1 + 14*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 15, 0) != 2 + 15*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    2 + 15*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap, 14, 0) != 0 + 10*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 10*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 14, 0) != 1 + 11*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    1 + 11*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 14, 0) != 2 + 12*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    2 + 12*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 0 + 6*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 6*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 4 + 7*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    4 + 7*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 6 + 8*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    6 + 8*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap, 10, 0) != 7 + 9*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    7 + 9*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap,  9, 0) != 0 + 3*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 3*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  9, 0) != 4 + 4*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    4 + 4*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  9, 0) != 7 + 5*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    7 + 5*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet(BitMap,  1, 0) != 0 + 0*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    0 + 0*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  1, 0) != 5 + 1*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    5 + 1*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet(BitMap,  1, 0) != 7 + 2*32) {
        DbgPrint("RtlFindClearBitsAndSet Error    7 + 2*32\n");
        return FALSE;
    }

    if (RtlNumberOfClearBits( BitMap ) != 0) { DbgPrint("Number of Clear bits error 4\n" ); }
    if (RtlNumberOfSetBits( BitMap ) != 2048*8)        { DbgPrint("Number of Set bits error 4\n" ); }

    //
    //  Test RtlAreBitsClear and AreBitsSet
    //

    DbgPrint("Start bit query tests\n");

    RtlClearAllBits( BitMap );
    if (!RtlAreBitsClear( BitMap, 0, 2048*8 )) { DbgPrint("RtlAreBitsClear  Error 0\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 8, 8 )) { DbgPrint("AreBitsClear Error 1\n"); }
    RtlClearBits( BitMap, 9, 6 );
    if (RtlAreBitsClear( BitMap, 8, 8 )) { DbgPrint("AreBitsClear Error 2\n"); }
    RtlClearBits( BitMap, 8, 1 );
    if (RtlAreBitsClear( BitMap, 8, 8 )) { DbgPrint("AreBitsClear Error 3\n"); }
    RtlClearBits( BitMap, 15, 1 );
    if (!RtlAreBitsClear( BitMap, 8, 8 )) { DbgPrint("AreBitsClear Error 4\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 8, 7 )) { DbgPrint("AreBitsClear Error 5\n"); }
    RtlClearBits( BitMap, 9, 5 );
    if (RtlAreBitsClear( BitMap, 8, 7 )) { DbgPrint("AreBitsClear Error 6\n"); }
    RtlClearBits( BitMap, 8, 1 );
    if (RtlAreBitsClear( BitMap, 8, 7 )) { DbgPrint("AreBitsClear Error 7\n"); }
    RtlClearBits( BitMap, 14, 1 );
    if (!RtlAreBitsClear( BitMap, 8, 7 )) { DbgPrint("AreBitsClear Error 8\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 9, 7 )) { DbgPrint("AreBitsClear Error 9\n"); }
    RtlClearBits( BitMap, 10, 5 );
    if (RtlAreBitsClear( BitMap, 9, 7 )) { DbgPrint("AreBitsClear Error 10\n"); }
    RtlClearBits( BitMap, 9, 1 );
    if (RtlAreBitsClear( BitMap, 9, 7 )) { DbgPrint("AreBitsClear Error 11\n"); }
    RtlClearBits( BitMap, 15, 1 );
    if (!RtlAreBitsClear( BitMap, 9, 7 )) { DbgPrint("AreBitsClear Error 12\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 9, 5 )) { DbgPrint("AreBitsClear Error 13\n"); }
    RtlClearBits( BitMap, 10, 3 );
    if (RtlAreBitsClear( BitMap, 9, 5 )) { DbgPrint("AreBitsClear Error 14\n"); }
    RtlClearBits( BitMap, 9, 1 );
    if (RtlAreBitsClear( BitMap, 9, 5 )) { DbgPrint("AreBitsClear Error 15\n"); }
    RtlClearBits( BitMap, 13, 1 );
    if (!RtlAreBitsClear( BitMap, 9, 5 )) { DbgPrint("AreBitsClear Error 16\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 8, 24 )) { DbgPrint("AreBitsClear Error 17\n"); }
    RtlClearBits( BitMap, 9, 22 );
    if (RtlAreBitsClear( BitMap, 8, 24 )) { DbgPrint("AreBitsClear Error 18\n"); }
    RtlClearBits( BitMap, 8, 1 );
    if (RtlAreBitsClear( BitMap, 8, 24 )) { DbgPrint("AreBitsClear Error 19\n"); }
    RtlClearBits( BitMap, 31, 1 );
    if (!RtlAreBitsClear( BitMap, 8, 24 )) { DbgPrint("AreBitsClear Error 20\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 8, 23 )) { DbgPrint("AreBitsClear Error 21\n"); }
    RtlClearBits( BitMap, 9, 21 );
    if (RtlAreBitsClear( BitMap, 8, 23 )) { DbgPrint("AreBitsClear Error 22\n"); }
    RtlClearBits( BitMap, 8, 1 );
    if (RtlAreBitsClear( BitMap, 8, 23 )) { DbgPrint("AreBitsClear Error 23\n"); }
    RtlClearBits( BitMap, 30, 1 );
    if (!RtlAreBitsClear( BitMap, 8, 23 )) { DbgPrint("AreBitsClear Error 24\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 9, 23 )) { DbgPrint("AreBitsClear Error 25\n"); }
    RtlClearBits( BitMap, 10, 21 );
    if (RtlAreBitsClear( BitMap, 9, 23 )) { DbgPrint("AreBitsClear Error 26\n"); }
    RtlClearBits( BitMap, 9, 1 );
    if (RtlAreBitsClear( BitMap, 9, 23 )) { DbgPrint("AreBitsClear Error 27\n"); }
    RtlClearBits( BitMap, 31, 1 );
    if (!RtlAreBitsClear( BitMap, 9, 23 )) { DbgPrint("AreBitsClear Error 28\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 9, 21 )) { DbgPrint("AreBitsClear Error 29\n"); }
    RtlClearBits( BitMap, 10, 19 );
    if (RtlAreBitsClear( BitMap, 9, 21 )) { DbgPrint("AreBitsClear Error 30\n"); }
    RtlClearBits( BitMap, 9, 1 );
    if (RtlAreBitsClear( BitMap, 9, 21 )) { DbgPrint("AreBitsClear Error 31\n"); }
    RtlClearBits( BitMap, 29, 1 );
    if (!RtlAreBitsClear( BitMap, 9, 21 )) { DbgPrint("AreBitsClear Error 32\n"); }

    RtlSetAllBits( BitMap );
    if (RtlAreBitsClear( BitMap, 10, 1 )) { DbgPrint("AreBitsClear Error 33\n"); }
    RtlClearBits( BitMap, 9, 1 );
    if (RtlAreBitsClear( BitMap, 10, 1 )) { DbgPrint("AreBitsClear Error 34\n"); }
    RtlClearBits( BitMap, 11, 1 );
    if (RtlAreBitsClear( BitMap, 10, 1 )) { DbgPrint("AreBitsClear Error 35\n"); }
    RtlClearBits( BitMap, 10, 1 );
    if (!RtlAreBitsClear( BitMap, 10, 1 )) { DbgPrint("AreBitsClear Error 36\n"); }


    RtlSetAllBits( BitMap );
    if (!RtlAreBitsSet( BitMap, 0, 2048*8 )) { DbgPrint("RtlAreBitsSet  Error 0\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 8, 8 )) { DbgPrint("AreBitsSet Error 1\n"); }
    RtlSetBits( BitMap, 9, 6 );
    if (RtlAreBitsSet( BitMap, 8, 8 )) { DbgPrint("AreBitsSet Error 2\n"); }
    RtlSetBits( BitMap, 8, 1 );
    if (RtlAreBitsSet( BitMap, 8, 8 )) { DbgPrint("AreBitsSet Error 3\n"); }
    RtlSetBits( BitMap, 15, 1 );
    if (!RtlAreBitsSet( BitMap, 8, 8 )) { DbgPrint("AreBitsSet Error 4\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 8, 7 )) { DbgPrint("AreBitsSet Error 5\n"); }
    RtlSetBits( BitMap, 9, 5 );
    if (RtlAreBitsSet( BitMap, 8, 7 )) { DbgPrint("AreBitsSet Error 6\n"); }
    RtlSetBits( BitMap, 8, 1 );
    if (RtlAreBitsSet( BitMap, 8, 7 )) { DbgPrint("AreBitsSet Error 7\n"); }
    RtlSetBits( BitMap, 14, 1 );
    if (!RtlAreBitsSet( BitMap, 8, 7 )) { DbgPrint("AreBitsSet Error 8\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 9, 7 )) { DbgPrint("AreBitsSet Error 9\n"); }
    RtlSetBits( BitMap, 10, 5 );
    if (RtlAreBitsSet( BitMap, 9, 7 )) { DbgPrint("AreBitsSet Error 10\n"); }
    RtlSetBits( BitMap, 9, 1 );
    if (RtlAreBitsSet( BitMap, 9, 7 )) { DbgPrint("AreBitsSet Error 11\n"); }
    RtlSetBits( BitMap, 15, 1 );
    if (!RtlAreBitsSet( BitMap, 9, 7 )) { DbgPrint("AreBitsSet Error 12\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 9, 5 )) { DbgPrint("AreBitsSet Error 13\n"); }
    RtlSetBits( BitMap, 10, 3 );
    if (RtlAreBitsSet( BitMap, 9, 5 )) { DbgPrint("AreBitsSet Error 14\n"); }
    RtlSetBits( BitMap, 9, 1 );
    if (RtlAreBitsSet( BitMap, 9, 5 )) { DbgPrint("AreBitsSet Error 15\n"); }
    RtlSetBits( BitMap, 13, 1 );
    if (!RtlAreBitsSet( BitMap, 9, 5 )) { DbgPrint("AreBitsSet Error 16\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 8, 24 )) { DbgPrint("AreBitsSet Error 17\n"); }
    RtlSetBits( BitMap, 9, 22 );
    if (RtlAreBitsSet( BitMap, 8, 24 )) { DbgPrint("AreBitsSet Error 18\n"); }
    RtlSetBits( BitMap, 8, 1 );
    if (RtlAreBitsSet( BitMap, 8, 24 )) { DbgPrint("AreBitsSet Error 19\n"); }
    RtlSetBits( BitMap, 31, 1 );
    if (!RtlAreBitsSet( BitMap, 8, 24 )) { DbgPrint("AreBitsSet Error 20\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 8, 23 )) { DbgPrint("AreBitsSet Error 21\n"); }
    RtlSetBits( BitMap, 9, 21 );
    if (RtlAreBitsSet( BitMap, 8, 23 )) { DbgPrint("AreBitsSet Error 22\n"); }
    RtlSetBits( BitMap, 8, 1 );
    if (RtlAreBitsSet( BitMap, 8, 23 )) { DbgPrint("AreBitsSet Error 23\n"); }
    RtlSetBits( BitMap, 30, 1 );
    if (!RtlAreBitsSet( BitMap, 8, 23 )) { DbgPrint("AreBitsSet Error 24\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 9, 23 )) { DbgPrint("AreBitsSet Error 25\n"); }
    RtlSetBits( BitMap, 10, 21 );
    if (RtlAreBitsSet( BitMap, 9, 23 )) { DbgPrint("AreBitsSet Error 26\n"); }
    RtlSetBits( BitMap, 9, 1 );
    if (RtlAreBitsSet( BitMap, 9, 23 )) { DbgPrint("AreBitsSet Error 27\n"); }
    RtlSetBits( BitMap, 31, 1 );
    if (!RtlAreBitsSet( BitMap, 9, 23 )) { DbgPrint("AreBitsSet Error 28\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 9, 21 )) { DbgPrint("AreBitsSet Error 29\n"); }
    RtlSetBits( BitMap, 10, 19 );
    if (RtlAreBitsSet( BitMap, 9, 21 )) { DbgPrint("AreBitsSet Error 30\n"); }
    RtlSetBits( BitMap, 9, 1 );
    if (RtlAreBitsSet( BitMap, 9, 21 )) { DbgPrint("AreBitsSet Error 31\n"); }
    RtlSetBits( BitMap, 29, 1 );
    if (!RtlAreBitsSet( BitMap, 9, 21 )) { DbgPrint("AreBitsSet Error 32\n"); }

    RtlClearAllBits( BitMap );
    if (RtlAreBitsSet( BitMap, 10, 1 )) { DbgPrint("AreBitsSet Error 33\n"); }
    RtlSetBits( BitMap, 9, 1 );
    if (RtlAreBitsSet( BitMap, 10, 1 )) { DbgPrint("AreBitsSet Error 34\n"); }
    RtlSetBits( BitMap, 11, 1 );
    if (RtlAreBitsSet( BitMap, 10, 1 )) { DbgPrint("AreBitsSet Error 35\n"); }
    RtlSetBits( BitMap, 10, 1 );
    if (!RtlAreBitsSet( BitMap, 10, 1 )) { DbgPrint("AreBitsSet Error 36\n"); }

    DbgPrint("End BitMapTest()\n");

    return TRUE;
}
