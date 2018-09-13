/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    TMcbSup.c

Abstract:

    This module tests the Pinball Map Control Block support routines

Author:

    Gary Kimura     [GaryKi]    5-Feb-1990

Revision History:

--*/

#include <stdio.h>
#include <string.h>

#include "PbProcs.h"

VOID
PbDumpMcb (
    IN PMCB Mcb
    );



#ifndef SIMULATOR
ULONG IoInitIncludeDevices;
#endif // SIMULATOR

BOOLEAN McbTest();

int
main(
    int argc,
    char *argv[]
    )
{
    extern ULONG IoInitIncludeDevices;
    VOID KiSystemStartup();

    DbgPrint("sizeof(MCB) = %d\n", sizeof(MCB));

    IoInitIncludeDevices = 0; // IOINIT_FATFS |
                              // IOINIT_PINBALLFS |
                              // IOINIT_DDFS;
    TestFunction = McbTest;

    KiSystemStartup();

    return( 0 );
}

BOOLEAN
McbTest()
{
    BOOLEAN TestAddEntry();
    BOOLEAN TestRemoveEntry();
    BOOLEAN TestLookupEntry();
    BOOLEAN TestGetEntry();
    BOOLEAN TestLookupLastEntry();

    if (!TestAddEntry()) {
        return FALSE;
    }

    if (!TestRemoveEntry()) {
        return FALSE;
    }

    if (!TestLookupEntry()) {
        return FALSE;
    }

    if (!TestGetEntry()) {
        return FALSE;
    }

    if (!TestLookupLastEntry()) {
        return FALSE;
    }

    return TRUE;

}

BOOLEAN
TestAddEntry()
{
    MCB Mcb;
    ULONG i;
    ULONG Vbn,Lbn,Length;

    DbgPrint("\n\n\n>>>> Test PbAddMcbEntry <<<<\n");

    //
    //  Build the following runs
    //
    //  [0-9|10-19][20-29] [40-49]
    //

    PbInitializeMcb(Mcb, NonPagedPool);

    DbgPrint("\nTest 0:|--NewRun--|\n");
    if (!PbAddMcbEntry(Mcb,  0, 1000, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --LastRun--|--NewRun--|\n");
    if (!PbAddMcbEntry(Mcb, 10, 1010, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --LastRun--||--NewRun--|\n");
    if (!PbAddMcbEntry(Mcb, 20, 2020, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --LastRun--|  hole  |--NewRun--|\n");
    if (!PbAddMcbEntry(Mcb, 40, 1040, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest over writing an existing run\n");
    if (PbAddMcbEntry(Mcb, 40, 1190, 5)) {DbgPrint("Error\n");return FALSE;}
    if (PbAddMcbEntry(Mcb, 25, 1190, 10)) {DbgPrint("Error\n");return FALSE;}
    if (PbAddMcbEntry(Mcb, 15, 1190, 10)) {DbgPrint("Error\n");return FALSE;}

    PbUninitializeMcb(Mcb);

    //
    //  Build the following runs
    //
    //  [0-9] [30-39][40-49|50-59|60-64] [70-79][80-84] [90-99]
    //

    PbInitializeMcb(Mcb, NonPagedPool);

    DbgPrint("\nTest 0:  hole  |--NewRun--|\n");
    if (!PbAddMcbEntry(Mcb, 90, 1090, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:  hole  |--NewRun--|  hole  |--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 50, 1050, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --PreviousRun--|  hole  |--NewRun--|  hole  |--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 70, 1070, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --PreviousRun--|--NewRun--|  hole  |--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 60, 1060,  5)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --PreviousRun--||--NewRun--|  hole  |--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 80, 1180,  5)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:|--NewRun--|  hole  |--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb,  0, 1000, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --PreviousRun--|  hole  |--NewRun--|--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 40, 1040, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --PreviousRun--|  hole  |--NewRun--||--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 30, 1130, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    PbUninitializeMcb(Mcb);

    //
    //  Build the following runs
    //
    //  [0-9|10-19|20-29][30-39][40-49|50-59|60-69|70-79][80-89|90-99]
    //

    PbInitializeMcb(Mcb, NonPagedPool);
    if (!PbAddMcbEntry(Mcb, 90, 1090, 10)) {DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest 0:  hole  |--NewRun--|--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 80, 1080, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:  hole  |--NewRun--||--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 70, 1170, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    if (!PbAddMcbEntry(Mcb, 50, 1150, 10)) {DbgPrint("Error\n");return FALSE;}
    DbgPrint("\nTest --PreviousRun--|--NewRun--|--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 60, 1160, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    if (!PbAddMcbEntry(Mcb, 30, 1030, 10)) {DbgPrint("Error\n");return FALSE;}
    DbgPrint("\nTest --PreviousRun--||--NewRun--|--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 40, 1140, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    if (!PbAddMcbEntry(Mcb, 10, 1110, 10)) {DbgPrint("Error\n");return FALSE;}
    DbgPrint("\nTest --PreviousRun--|--NewRun--||--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 20, 1120, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:|--NewRun--|--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb,  0, 1100, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    PbUninitializeMcb(Mcb);

    //
    //  Build the following runs
    //
    //  [0-69][80-79|80-89|90-99]
    //

    PbInitializeMcb(Mcb, NonPagedPool);
    if (!PbAddMcbEntry(Mcb, 90, 1090, 10)) {DbgPrint("Error\n");return FALSE;}

    if (!PbAddMcbEntry(Mcb, 70, 1070, 10)) {DbgPrint("Error\n");return FALSE;}
    DbgPrint("\nTest --PreviousRun--||--NewRun--||--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb, 80, 1010, 10)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:|--NewRun--||--FollowingRun--\n");
    if (!PbAddMcbEntry(Mcb,  0, 1100, 70)) {DbgPrint("Error\n");return FALSE;}
    PbDumpMcb( Mcb );

    PbUninitializeMcb(Mcb);

    return TRUE;

}


BOOLEAN
TestRemoveEntry()
{
    MCB Mcb;
    ULONG i;
    ULONG Vbn,Lbn,Length;

    DbgPrint("\n\n\n>>>> Test PbRemoveMcbEntry <<<<\n");

    PbInitializeMcb(Mcb, NonPagedPool);
    if (!PbAddMcbEntry(Mcb, 0, 1000, 100)) {DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest --Previous--|  Hole\n");
    PbRemoveMcbEntry(Mcb, 90, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --Previous--|  Hole  |--Following--\n");
    PbRemoveMcbEntry(Mcb, 50, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --Previous--|  Hole  |--Hole--\n");
    PbRemoveMcbEntry(Mcb, 40, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --hole--|  Hole  |--Following--\n");
    PbRemoveMcbEntry(Mcb, 60, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --hole--|  Hole\n");
    PbRemoveMcbEntry(Mcb, 70, 20);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:  Hole  |--Following--\n");
    PbRemoveMcbEntry(Mcb, 0, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest over remove\n");
    PbRemoveMcbEntry(Mcb, 0, 100);
    PbDumpMcb( Mcb );

    PbUninitializeMcb(Mcb);



    PbInitializeMcb(Mcb, NonPagedPool);
    if (!PbAddMcbEntry(Mcb, 0, 1000, 100)) {DbgPrint("Error\n");return FALSE;}

    PbRemoveMcbEntry(Mcb, 10, 10);
    DbgPrint("\nTest 0:  Hole |--Hole--\n");
    PbRemoveMcbEntry(Mcb,  0, 10);
    PbDumpMcb( Mcb );

    PbRemoveMcbEntry(Mcb, 30, 10);
    DbgPrint("\nTest --Hole--|  Hole  |--Hole--\n");
    PbRemoveMcbEntry(Mcb, 20, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest over remove\n");
    PbRemoveMcbEntry(Mcb, 0, 100);
    PbDumpMcb( Mcb );

    PbUninitializeMcb(Mcb);



    PbInitializeMcb(Mcb, NonPagedPool);
    if (!PbAddMcbEntry(Mcb, 0, 1000, 100)){DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest 0:  Hole\n");
    PbRemoveMcbEntry(Mcb, 0, 100);
    PbDumpMcb( Mcb );

    if (!PbAddMcbEntry(Mcb,  0, 1000,  30)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 30, 1130,  30)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 60, 1060,  30)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 90, 1190,  10)) {DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest --Previous--|  Hole  |--Index--||--Following--\n");
    PbRemoveMcbEntry(Mcb, 30, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --Hole--|  Hole  |--Index--||--Following--\n");
    PbRemoveMcbEntry(Mcb, 40, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --Previous--||--Index--|  Hole  |--Following--\n");
    PbRemoveMcbEntry(Mcb, 80, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --Previous--||--Index--|  Hole  |--Hole--\n");
    PbRemoveMcbEntry(Mcb, 70, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:|--Index--|  Hole  |--Index--||--Following--\n");
    PbRemoveMcbEntry(Mcb, 10, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest over remove\n");
    PbRemoveMcbEntry(Mcb, 0, 100);
    PbDumpMcb( Mcb );



    if (!PbAddMcbEntry(Mcb,  0, 1000,  30)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 30, 1130,  30)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 60, 1060,  30)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 90, 1190,  10)) {DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest --Previous--||--Index--|  Hole  |--Index--||--Following--\n");
    PbRemoveMcbEntry(Mcb, 40, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest 0:|--Index--|  Hole  |--Following--\n");
    PbRemoveMcbEntry(Mcb, 20, 10);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest --Previous--||--Index--|  Hole\n");
    PbRemoveMcbEntry(Mcb, 80, 20);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest over remove\n");
    PbRemoveMcbEntry(Mcb, 0, 100);
    PbDumpMcb( Mcb );


    if (!PbAddMcbEntry(Mcb,  0, 1000,  100)){DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest 0:--Index--|  Hole\n");
    PbRemoveMcbEntry(Mcb, 50, 50);
    PbDumpMcb( Mcb );

    DbgPrint("\nTest over remove\n");
    PbRemoveMcbEntry(Mcb, 0, 100);
    PbDumpMcb( Mcb );



    if (!PbAddMcbEntry(Mcb,  0, 1000,  50)) {DbgPrint("Error\n");return FALSE;}
    if (!PbAddMcbEntry(Mcb, 50, 1150,  50)) {DbgPrint("Error\n");return FALSE;}

    DbgPrint("\nTest 0:  hole  |--Index--||--Following--\n");
    PbRemoveMcbEntry(Mcb,  0, 20);
    PbDumpMcb( Mcb );

    PbUninitializeMcb(Mcb);

    return TRUE;

}


BOOLEAN
TestLookupEntry()
{
    MCB Mcb;
    ULONG i;
    ULONG Vbn,Lbn,Length;

    DbgPrint("\n\n\n>>>> Test PbLookupMcbEntry <<<<\n");

    PbInitializeMcb(Mcb, NonPagedPool);

    for (i =  0; i < 100; i += 30) {
        if (!PbAddMcbEntry(Mcb, i, 1000+i, 10))
            {DbgPrint("Add1Error\n");return FALSE;}
    }
    for (i = 10; i < 100; i += 30) {
        if (!PbAddMcbEntry(Mcb, i, 1100+i, 10))
            {DbgPrint("Add2Error\n");return FALSE;}
    }

    PbDumpMcb( Mcb );

    for (i =  0; i < 100; i += 30) {
        if (!PbLookupMcbEntry(Mcb,i,&Lbn,&Length))
            {DbgPrint("Lookup1Error %d\n", i);return FALSE;}
        if ((Lbn != 1000+i) || (Length != 10))
            {DbgPrint("Result1Error %d, %d, %d\n", i, Lbn, Length);return FALSE;}
        if (!PbLookupMcbEntry(Mcb,i+5,&Lbn,&Length))
            {DbgPrint("Lookup2Error %d\n", i);return FALSE;}
        if ((Lbn != 1000+i+5) || (Length != 5))
            {DbgPrint("Result2Error %d, %d, %d\n", i, Lbn, Length);return FALSE;}
    }

    for (i = 20; i < 100; i += 30) {
        if (!PbLookupMcbEntry(Mcb,i,&Lbn,&Length))
            {DbgPrint("Lookup3Error %d\n", i);return FALSE;}
        if ((Lbn != 0) || (Length != 10))
            {DbgPrint("Result3Error %d, %d, %d\n", i, Lbn, Length);return FALSE;}

        if (!PbLookupMcbEntry(Mcb,i+5,&Lbn,&Length))
            {DbgPrint("Lookup4Error %d\n", i);return FALSE;}
        if ((Lbn != 0) || (Length != 5)) {
            DbgPrint("Result4Error %d", i+5);
            DbgPrint(", %08lx",   Lbn);
            DbgPrint(", %08lx\n", Length);
            return FALSE;
        }
    }

    PbUninitializeMcb(Mcb);

    return TRUE;

}


BOOLEAN
TestGetEntry()
{
    MCB Mcb;
    ULONG i;
    ULONG Vbn,Lbn,Length;

    DbgPrint("\n\n\n>>>> TestPbNumberOfRunsInMcb <<<<\n");

    PbInitializeMcb(Mcb, NonPagedPool);

    for (i =  0; i < 100; i += 30) {
        if (!PbAddMcbEntry(Mcb, i, 1000+i, 10))
            {DbgPrint("Add1Error\n");return FALSE;}
    }
    for (i = 10; i < 100; i += 30) {
        if (!PbAddMcbEntry(Mcb, i, 1100+i, 10))
            {DbgPrint("Add2Error\n");return FALSE;}
    }

    PbDumpMcb( Mcb );

    i = PbNumberOfRunsInMcb(Mcb);
    if (i != 10) {DbgPrint("Error\n");return FALSE;}
    for (i = 0; i < 10; i += 1) {
        if (!PbGetNextMcbEntry(Mcb,i,&Vbn,&Lbn,&Length))
            {DbgPrint("Error\n");return FALSE;}
        DbgPrint("%d", i);
        DbgPrint(", %ld", Vbn);
        DbgPrint(", %ld", Lbn);
        DbgPrint(", %ld\n", Length);
    }

    PbUninitializeMcb(Mcb);

    return TRUE;

}


BOOLEAN
TestLookupLastEntry()
{
    MCB Mcb;
    ULONG i;
    ULONG Vbn,Lbn,Length;


    DbgPrint("\n\n\n>>>> Test PbLookupLastMcbEntry <<<<\n");

    PbInitializeMcb(Mcb, NonPagedPool);

    if (!PbAddMcbEntry(Mcb,  0, 1000, 100)) {DbgPrint("Error\n");return FALSE;}

    if (!PbLookupLastMcbEntry(Mcb, &Vbn, &Lbn)) {DbgPrint("Error\n");return FALSE;}
    if ((Vbn != 99) && (Lbn != 1099)) {DbgPrint("Lookup Error\n");return FALSE;}

    PbUninitializeMcb(Mcb);

    return TRUE;
}

