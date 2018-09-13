//////////////////////////////////////////////////////////////////////////
// WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING  //
//                                                                      //
// This test file is not current with the security implementation.      //
// This file contains references to data types and APIs that do not     //
// exist.                                                               //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tacl.c

Abstract:

    Test program for the acl editing package

Author:

    Gary Kimura     [GaryKi]    19-Nov-1989

Revision History:

    v4: robertre
        updated ACL_REVISION
    RichardW  - updated ACE_HEADER

--*/

#include <stdio.h>

#include "nt.h"
#include "ntrtl.h"

VOID
RtlDumpAcl(
    IN PACL Acl
    );

UCHAR FredAclBuffer[128];
UCHAR WilmaAclBuffer[128];
UCHAR PebbleAclBuffer[128];
UCHAR DinoAclBuffer[128];
UCHAR BarneyAclBuffer[128];
UCHAR BettyAclBuffer[128];
UCHAR BambamAclBuffer[128];

UCHAR GuidMaskBuffer[512];
STANDARD_ACE AceListBuffer[2];

int
main(
    int argc,
    char *argv[]
    )
{
    PACL FredAcl = (PACL)FredAclBuffer;
    PACL WilmaAcl = (PACL)WilmaAclBuffer;
    PACL PebbleAcl = (PACL)PebbleAclBuffer;
    PACL DinoAcl = (PACL)DinoAclBuffer;
    PACL BarneyAcl = (PACL)BarneyAclBuffer;
    PACL BettyAcl = (PACL)BettyAclBuffer;
    PACL BambamAcl = (PACL)BambamAclBuffer;

    PMASK_GUID_PAIRS GuidMasks = (PMASK_GUID_PAIRS)GuidMaskBuffer;

    ACL_REVISION_INFORMATION AclRevisionInfo;
    ACL_SIZE_INFORMATION AclSizeInfo;

    //
    //  We're starting the test
    //

    DbgPrint("Start Acl Test\n");

    //
    //  test create acl
    //

    if (!NT_SUCCESS(RtlCreateAcl(FredAcl, 128, 1))) {
        DbgPrint("RtlCreateAcl Error\n");
    }

    RtlDumpAcl(FredAcl);
    DbgPrint("\n");

    //
    //  test add ace to add two aces to an empty acl
    //

    AceListBuffer[0].Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AceListBuffer[0].Header.AceSize = sizeof(STANDARD_ACE);
    AceListBuffer[0].Header.AceFlags = 0;
    AceListBuffer[0].Mask = 0x22222222;
    CopyGuid(&AceListBuffer[0].Guid, &FredGuid);

    AceListBuffer[1].Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AceListBuffer[1].Header.AceSize = sizeof(STANDARD_ACE);
    AceListBuffer[1].Header.AceFlags = 0;
    AceListBuffer[1].Mask = 0x44444444;
    CopyGuid(&AceListBuffer[1].Guid, &WilmaGuid);

    if (!NT_SUCCESS(RtlAddAce(FredAcl, 1, 0, AceListBuffer, 2*sizeof(STANDARD_ACE)))) {
        DbgPrint("RtlAddAce Error\n");
    }

    RtlDumpAcl(FredAcl);
    DbgPrint("\n");

    //
    //  test add ace to add one to the beginning of an acl
    //

    AceListBuffer[0].Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
    AceListBuffer[0].Header.AceSize = sizeof(STANDARD_ACE);
    AceListBuffer[0].Header.AceFlags = 0;
    AceListBuffer[0].Mask = 0x11111111;
    CopyGuid(&AceListBuffer[0].Guid, &PebbleGuid);

    if (!NT_SUCCESS(RtlAddAce(FredAcl, 1, 0, AceListBuffer, sizeof(STANDARD_ACE)))) {
        DbgPrint("RtlAddAce Error\n");
    }

    RtlDumpAcl(FredAcl);
    DbgPrint("\n");

    //
    //  test add ace to add one to the middle of an acl
    //

    AceListBuffer[0].Header.AceType = ACCESS_DENIED_ACE_TYPE;
    AceListBuffer[0].Header.AceSize = sizeof(STANDARD_ACE);
    AceListBuffer[0].Header.AceFlags = 0;
    AceListBuffer[0].Mask = 0x33333333;
    CopyGuid(&AceListBuffer[0].Guid, &DinoGuid);

    if (!NT_SUCCESS(RtlAddAce(FredAcl, 1, 2, AceListBuffer, sizeof(STANDARD_ACE)))) {
        DbgPrint("RtlAddAce Error\n");
    }

    RtlDumpAcl(FredAcl);
    DbgPrint("\n");

    //
    //  test add ace to add one to the end of an acl
    //

    AceListBuffer[0].Header.AceType = ACCESS_DENIED_ACE_TYPE;
    AceListBuffer[0].Header.AceSize = sizeof(STANDARD_ACE);
    AceListBuffer[0].Header.AceFlags = 0;
    AceListBuffer[0].Mask = 0x55555555;
    CopyGuid(&AceListBuffer[0].Guid, &FlintstoneGuid);

    if (!NT_SUCCESS(RtlAddAce(FredAcl, 1, MAXULONG, AceListBuffer, sizeof(STANDARD_ACE)))) {
        DbgPrint("RtlAddAce Error\n");
    }

    RtlDumpAcl(FredAcl);
    DbgPrint("\n");

    //
    //  Test get ace
    //

    {
        PSTANDARD_ACE Ace;

        if (!NT_SUCCESS(RtlGetAce(FredAcl, 2, (PVOID *)(&Ace)))) {
            DbgPrint("RtlGetAce Error\n");
        }

        if ((Ace->Header.AceType != ACCESS_DENIED_ACE_TYPE) ||
            (Ace->Mask != 0x33333333)) {
            DbgPrint("Got bad ace from RtlGetAce\n");
        }
    }

    //
    //  test delete ace middle ace
    //

    if (!NT_SUCCESS(RtlDeleteAce(FredAcl, 2))) {
        DbgPrint("RtlDeleteAce Error\n");
    }

    RtlDumpAcl(FredAcl);
    DbgPrint("\n");

    //
    //  Test query information acl
    //

    if (!NT_SUCCESS(RtlQueryInformationAcl( FredAcl,
                                         (PVOID)&AclRevisionInfo,
                                         sizeof(ACL_REVISION_INFORMATION),
                                         AclRevisionInformation))) {
        DbgPrint("RtlQueryInformationAcl Error\n");
    }
    if (AclRevisionInfo.AclRevision != ACL_REVISION) {
        DbgPrint("RtlAclRevision Error\n");
    }

    if (!NT_SUCCESS(RtlQueryInformationAcl( FredAcl,
                                         (PVOID)&AclSizeInfo,
                                         sizeof(ACL_SIZE_INFORMATION),
                                         AclSizeInformation))) {
        DbgPrint("RtlQueryInformationAcl Error\n");
    }
    if ((AclSizeInfo.AceCount != 4) ||
        (AclSizeInfo.AclBytesInUse != (sizeof(ACL)+4*sizeof(STANDARD_ACE))) ||
        (AclSizeInfo.AclBytesFree != 128 - AclSizeInfo.AclBytesInUse)) {
        DbgPrint("RtlAclSize Error\n");
        DbgPrint("AclSizeInfo.AceCount = %8lx\n", AclSizeInfo.AceCount);
        DbgPrint("AclSizeInfo.AclBytesInUse = %8lx\n", AclSizeInfo.AclBytesInUse);
        DbgPrint("AclSizeInfo.AclBytesFree = %8lx\n", AclSizeInfo.AclBytesFree);
        DbgPrint("\n");
    }

    //
    //  Test make Mask from Acl
    //

    GuidMasks->PairCount = 11;
    CopyGuid(&GuidMasks->MaskGuid[ 0].Guid, &FredGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 1].Guid, &WilmaGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 2].Guid, &PebbleGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 3].Guid, &DinoGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 4].Guid, &BarneyGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 5].Guid, &BettyGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 6].Guid, &BambamGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 7].Guid, &FlintstoneGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 8].Guid, &RubbleGuid);
    CopyGuid(&GuidMasks->MaskGuid[ 9].Guid, &AdultGuid);
    CopyGuid(&GuidMasks->MaskGuid[10].Guid, &ChildGuid);
    if (!NT_SUCCESS(RtlMakeMaskFromAcl(FredAcl, GuidMasks))) {
        DbgPrint("RtlMakeMaskFromAcl Error\n");
    }
    if ((GuidMasks->MaskGuid[ 0].Mask != 0x22222222) ||
        (GuidMasks->MaskGuid[ 1].Mask != 0x44444444) ||
        (GuidMasks->MaskGuid[ 2].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 3].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 4].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 5].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 6].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 7].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 8].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[ 9].Mask != 0x00000000) ||
        (GuidMasks->MaskGuid[10].Mask != 0x00000000)) {
        DbgPrint("Make Mask Error\n");
        DbgPrint("Fred gets       %8lx\n", GuidMasks->MaskGuid[ 0].Mask);
        DbgPrint("Wilma gets      %8lx\n", GuidMasks->MaskGuid[ 1].Mask);
        DbgPrint("Pebble gets     %8lx\n", GuidMasks->MaskGuid[ 2].Mask);
        DbgPrint("Dino gets       %8lx\n", GuidMasks->MaskGuid[ 3].Mask);
        DbgPrint("Barney gets     %8lx\n", GuidMasks->MaskGuid[ 4].Mask);
        DbgPrint("Betty gets      %8lx\n", GuidMasks->MaskGuid[ 5].Mask);
        DbgPrint("Banbam gets     %8lx\n", GuidMasks->MaskGuid[ 6].Mask);
        DbgPrint("Flintstone gets %8lx\n", GuidMasks->MaskGuid[ 7].Mask);
        DbgPrint("Rubble gets     %8lx\n", GuidMasks->MaskGuid[ 8].Mask);
        DbgPrint("Adult gets      %8lx\n", GuidMasks->MaskGuid[ 9].Mask);
        DbgPrint("Child gets      %8lx\n", GuidMasks->MaskGuid[10].Mask);
    }

    //
    //  test make acl from mask
    //

    GuidMasks->PairCount = 2;
    GuidMasks->MaskGuid[0].Mask = 0x55555555;
    CopyGuid(&GuidMasks->MaskGuid[0].Guid, &BarneyGuid);
    GuidMasks->MaskGuid[1].Mask = 0xaaaa5555;
    CopyGuid(&GuidMasks->MaskGuid[1].Guid, &RubbleGuid);

    //
    //  Initialize and dump a posix style acl
    //

    if (!NT_SUCCESS(RtlMakeAclFromMask(GuidMasks, AclPosixEnvironment, BarneyAcl, 128, 1))) {
        DbgPrint("RtlMakeAclFromMask Error\n");
    }

    RtlDumpAcl(BarneyAcl);
    DbgPrint("\n");

    //
    //  Initialize and dump a OS/2 style acl
    //

    if (!NT_SUCCESS(RtlMakeAclFromMask(GuidMasks, AclOs2Environment, BettyAcl, 128, 1))) {
        DbgPrint("RtlMakeAclFromMask Error\n");
    }

    RtlDumpAcl(BettyAcl);
    DbgPrint("\n");

    //
    //  We're done with the test
    //

    DbgPrint("End Acl Test\n");

    return TRUE;
}


