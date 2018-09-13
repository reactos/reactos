/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ctsertl.c

Abstract:

    Common security RTL test routines.

    These routines are used in both the kernel and user mode RTL tests.



Author:

    Jim Kelly       (JimK)     23-Mar-1990

Environment:

    Test of security.

Revision History:

--*/

#include "tsecomm.c"




////////////////////////////////////////////////////////////////
//                                                            //
// Test routines                                              //
//                                                            //
////////////////////////////////////////////////////////////////


BOOLEAN
TestSeSid()
{

#define TARGET_SID_ARRAY_LENGTH   1024

typedef struct _TALT_SID1 {
    ULONG Value[3];
} TALT_SID1;
typedef TALT_SID1 *PTALT_SID1;

    NTSTATUS Status;

    PVOID Ignore;

    PSID TFredSid;
    PSID TBarneySid;
    PSID TWilmaSid;
    PSID TWilmaSubSid;
    PSID TNoSubSid;
    PSID TTempSid;

    PSID_AND_ATTRIBUTES SourceArray;
    PSID_AND_ATTRIBUTES TargetArray;
    ULONG TargetLength;
    ULONG OriginalTargetLength;

    ULONG NormalGroupAttributes;
    ULONG OwnerGroupAttributes;

    // Temporary Hack ...
    NormalGroupAttributes = 7;
    OwnerGroupAttributes = 15;
    // End Temporary Hack...


    TFredSid     = (PSID)TstAllocatePool( PagedPool, 256 );
    TBarneySid   = (PSID)TstAllocatePool( PagedPool, 256 );
    TWilmaSid    = (PSID)TstAllocatePool( PagedPool, 256 );
    TWilmaSubSid = (PSID)TstAllocatePool( PagedPool, 256 );
    TNoSubSid    = (PSID)TstAllocatePool( PagedPool, 256 );
    TTempSid     = (PSID)TstAllocatePool( PagedPool, 256 );


    //
    // Valid SID structure test
    //

    if (!RtlValidSid( TFredSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, TFredSid\n");
        return FALSE;
    }

    if (!RtlValidSid( TBarneySid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, TBarneySid\n");
        return FALSE;
    }

    if (!RtlValidSid( TWilmaSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, TWilmaSid\n");
        return FALSE;
    }

    if (!RtlValidSid( TWilmaSubSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, TWilmaSubSid\n");
        return FALSE;
    }

    if (!RtlValidSid( TNoSubSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, TNoSubSid\n");
        return FALSE;
    }


    //
    // Equal SIDs Test
    //

    if (RtlEqualSid( TFredSid, TBarneySid )) {
        DbgPrint("*Se**     Failure: RtlEqualSid, TFredSid - TBarneySid\n");
        DbgPrint("**** Failed **** \n");
        return FALSE;
    }

    if (!RtlEqualSid( TFredSid, TFredSid )) {
        DbgPrint("*Se**     Failure: RtlEqualSid, TFredSid - TFredSid\n");
        return FALSE;
    }

    if (RtlEqualSid( TWilmaSid, TWilmaSubSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlEqualSid, TWilmaSid - TWilmaSubSid\n");
        return FALSE;
    }

    if (RtlEqualSid( TWilmaSid, TNoSubSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlEqualSid, TWilmaSid - TNoSubSid\n");
        return FALSE;
    }


    //
    // Length Required test
    //

    if (RtlLengthRequiredSid( 0 ) != 8) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthRequiredSid, 0 SubAuthorities\n");
        return FALSE;
    }

    if (RtlLengthRequiredSid( 1 ) != 12) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthRequiredSid, 1 SubAuthorities\n");
        return FALSE;
    }

    if (RtlLengthRequiredSid( 2 ) != 16) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthRequiredSid, 2 SubAuthorities\n");
        return FALSE;
    }


    //
    // Length of SID test
    //

    if (SeLengthSid( TNoSubSid ) != 8) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: SeLengthSid, TNoSubSid\n");
        return FALSE;
    }

    if (SeLengthSid( TFredSid ) != 12) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: SeLengthSid, TFredSid\n");
        return FALSE;
    }

    if (SeLengthSid( TWilmaSubSid ) != 16) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: SeLengthSid, TWilmaSubSid\n");
        return FALSE;
    }


    //
    // Copy SID Test
    //

    if (NT_SUCCESS(RtlCopySid( 7, TTempSid, TNoSubSid ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid, insufficient TNoSubSid\n");
        return FALSE;
    }


    if (!NT_SUCCESS(RtlCopySid( 256, TTempSid, TNoSubSid ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid, TNoSubSid\n");
        return FALSE;
    }

    if (!RtlEqualSid( TTempSid, TNoSubSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid compare, TNoSubSid\n");
        return FALSE;
    }


    if (NT_SUCCESS(RtlCopySid( 11, TTempSid, TBarneySid ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid, insufficient TBarneySid\n");
        return FALSE;
    }


    if (!NT_SUCCESS(RtlCopySid( 256, TTempSid, TBarneySid ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid, TBarneySid\n");
        return FALSE;
    }

    if (!RtlEqualSid( TTempSid, TBarneySid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid compare, TBarneySid\n");
        return FALSE;
    }

    if (NT_SUCCESS(RtlCopySid( 15, TTempSid, TWilmaSubSid ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid, insufficient TWilmaSubSid\n");
        return FALSE;
    }

    if (!NT_SUCCESS(RtlCopySid( 256, TTempSid, TWilmaSubSid ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid, TNoSubSid\n");
        return FALSE;
    }

    if (!RtlEqualSid( TTempSid, TWilmaSubSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCopySid compare, TWilmaSubSid\n");
        return FALSE;
    }


    //
    // Validate all the tsevars SIDs
    //

    //
    //  Bedrock SIDs
    //


    if (!RtlValidSid( BedrockDomainSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, BedrockDomainSid\n");
        return FALSE;
    }

    if (!RtlValidSid( FredSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, FredSid\n");
        return FALSE;
    }

    if (!RtlValidSid( WilmaSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, WilmaSid\n");
        return FALSE;
    }

    if (!RtlValidSid( PebblesSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, PebblesSid\n");
        return FALSE;
    }

    if (!RtlValidSid( DinoSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, DinoSid\n");
        return FALSE;
    }

    if (!RtlValidSid( BarneySid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, BarneySid\n");
        return FALSE;
    }

    if (!RtlValidSid( BettySid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, BettySid\n");
        return FALSE;
    }

    if (!RtlValidSid( BambamSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, BambamSid\n");
        return FALSE;
    }

    if (!RtlValidSid( FlintstoneSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, FlintstoneSid\n");
        return FALSE;
    }

    if (!RtlValidSid( RubbleSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, RubbleSid\n");
        return FALSE;
    }

    if (!RtlValidSid( AdultSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, AdultSid\n");
        return FALSE;
    }

    if (!RtlValidSid( ChildSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, ChildSid\n");
        return FALSE;
    }

    if (!RtlValidSid( NeandertholSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, NeandertholSid\n");
        return FALSE;
    }

    //
    //  Well known SIDs
    //

    if (!RtlValidSid( NullSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, NullSid\n");
        return FALSE;
    }

    if (!RtlValidSid( WorldSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, WorldSid\n");
        return FALSE;
    }

    if (!RtlValidSid( LocalSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, CreatorSid\n");
        return FALSE;
    }

    if (!RtlValidSid( NtAuthoritySid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, NtAuthoritySid\n");
        return FALSE;
    }

    if (!RtlValidSid( DialupSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, DialupSid\n");
        return FALSE;
    }

    if (!RtlValidSid( NetworkSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, NetworkSid\n");
        return FALSE;
    }

    if (!RtlValidSid( BatchSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, BatchSid\n");
        return FALSE;
    }

    if (!RtlValidSid( InteractiveSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, InteractiveSid\n");
        return FALSE;
    }


    if (!RtlValidSid( LocalSystemSid )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSid, LocalSystemSid\n");
        return FALSE;
    }




    //
    //  Test SidAndAttributesArray copy routine
    //

    SourceArray = (PSID_AND_ATTRIBUTES)TstAllocatePool( PagedPool, 100 );
    TargetArray = (PSID_AND_ATTRIBUTES)TstAllocatePool( PagedPool,
                                                        TARGET_SID_ARRAY_LENGTH
                                                        );
    TargetLength = TARGET_SID_ARRAY_LENGTH - (5 * sizeof(PSID_AND_ATTRIBUTES));
    OriginalTargetLength = TargetLength;

    SourceArray[0].Sid = &PebblesSid;
    SourceArray[0].Attributes = 0;
    SourceArray[1].Sid = &FlintstoneSid;
    SourceArray[1].Attributes = OwnerGroupAttributes;
    SourceArray[2].Sid = &ChildSid;
    SourceArray[2].Attributes = NormalGroupAttributes;
    SourceArray[3].Sid = &NeandertholSid;
    SourceArray[3].Attributes = NormalGroupAttributes;
    SourceArray[4].Sid = &WorldSid;
    SourceArray[4].Attributes = NormalGroupAttributes;

    Status = RtlCopySidAndAttributesArray(
                 0,
                 SourceArray,
                 TargetLength,
                 TargetArray,
                 &(TargetArray[5]),
                 &(PSID)Ignore,
                 &TargetLength
                 );

    if (!NT_SUCCESS(Status) || TargetLength != OriginalTargetLength ) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtLCopySidAndAttributesArray, Zero length.\n");
        return FALSE;
    }



    Status = RtlCopySidAndAttributesArray(
                 1,
                 SourceArray,
                 1,                 // too short buffer
                 TargetArray,
                 &(TargetArray[1]),
                 &(PSID)Ignore,
                 &TargetLength
                 );

    if (NT_SUCCESS(Status)) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtLCopySidAndAttributesArray,\n");
        DbgPrint("*Se**              Buffer Too Short Test.\n");
        return FALSE;
    }


    TargetLength = TARGET_SID_ARRAY_LENGTH - (5 * sizeof(PSID_AND_ATTRIBUTES));
    OriginalTargetLength = TargetLength;

    Status = RtlCopySidAndAttributesArray(
                 5,
                 SourceArray,
                 TargetLength,
                 TargetArray,
                 &(TargetArray[5]),
                 &(PSID)Ignore,
                 &TargetLength
                 );

    if (!NT_SUCCESS(Status) ||
        !RtlEqualSid( SourceArray[0].Sid, TargetArray[0].Sid ) ||
        !RtlEqualSid( SourceArray[1].Sid, TargetArray[1].Sid ) ||
        !RtlEqualSid( SourceArray[2].Sid, TargetArray[2].Sid ) ||
        !RtlEqualSid( SourceArray[3].Sid, TargetArray[3].Sid ) ||
        !RtlEqualSid( SourceArray[4].Sid, TargetArray[4].Sid ) ||
        ( SourceArray[0].Attributes != TargetArray[0].Attributes )  ||
        ( SourceArray[1].Attributes != TargetArray[1].Attributes )  ||
        ( SourceArray[2].Attributes != TargetArray[2].Attributes )  ||
        ( SourceArray[3].Attributes != TargetArray[3].Attributes )  ||
        ( SourceArray[4].Attributes != TargetArray[4].Attributes ) ) {

        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtLCopySidAndAttributesArray,\n");
        DbgPrint("*Se**              Valid copy of 5 SIDs test.\n");
        return FALSE;
    }



    return TRUE;


}


BOOLEAN
TestSeSecurityDescriptor()
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR TFredDescriptor;
    PSECURITY_DESCRIPTOR TBarneyDescriptor;
    PSECURITY_DESCRIPTOR TWilmaDescriptor;

    PSECURITY_DESCRIPTOR TTempDescriptor;

    SECURITY_DESCRIPTOR_CONTROL Control;
    ULONG Revision;

    PACL TDacl;
    BOOLEAN TDaclPresent;
    BOOLEAN TDaclDefaulted;

    PACL TSacl;
    BOOLEAN TSaclPresent;
    BOOLEAN TSaclDefaulted;

    PSID TOwner;
    BOOLEAN TOwnerDefaulted;
    PSID TGroup;
    BOOLEAN TGroupDefaulted;


    PSID TFredSid;
    PSID TBarneySid;
    PSID TWilmaSid;
    PSID TWilmaSubSid;
    PSID TNoSubSid;
    PSID TTempSid;


    TFredDescriptor   = (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );
    TBarneyDescriptor = (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );
    TWilmaDescriptor  = (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );
    TTempDescriptor   = (PSECURITY_DESCRIPTOR)TstAllocatePool( PagedPool, 1024 );


    TFredSid     = (PSID)TstAllocatePool( PagedPool, 256 );
    TBarneySid   = (PSID)TstAllocatePool( PagedPool, 256 );
    TWilmaSid    = (PSID)TstAllocatePool( PagedPool, 256 );
    TWilmaSubSid = (PSID)TstAllocatePool( PagedPool, 256 );
    TNoSubSid    = (PSID)TstAllocatePool( PagedPool, 256 );
    TTempSid     = (PSID)TstAllocatePool( PagedPool, 256 );


    //
    // Build an ACL or two for use.

    TDacl        = (PACL)TstAllocatePool( PagedPool, 256 );
    TSacl        = (PACL)TstAllocatePool( PagedPool, 256 );

    TDacl->AclRevision=TSacl->AclRevision=ACL_REVISION;
    TDacl->Sbz1=TSacl->Sbz1=0;
    TDacl->Sbz2=TSacl->Sbz2=0;
    TDacl->AclSize=256;
    TSacl->AclSize=8;
    TDacl->AceCount=TSacl->AceCount=0;


    //
    // Create Security Descriptor test
    //

    if (NT_SUCCESS(RtlCreateSecurityDescriptor( TTempDescriptor, 0 ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCreateSecurityDescriptor, Rev=0\n");
        return FALSE;
    }

    if (NT_SUCCESS(RtlCreateSecurityDescriptor( TTempDescriptor, 2 ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCreateSecurityDescriptor, Rev=2\n");
        return FALSE;
    }

    if (!NT_SUCCESS(RtlCreateSecurityDescriptor( TTempDescriptor, 1 ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlCreateSecurityDescriptor, Rev=1\n");
        return FALSE;
    }

#ifdef NOT_YET_DEBUGGED
    //
    // Make sure fields have been set properly
    //

    if (!NT_SUCCESS(RtlGetControlSecurityDescriptor( TTempDescriptor,
                                               &Control,
                                               &Revision))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetControlSecurityDescriptor\n");
        DbgPrint("*Se**     Call failed.  Status = 0x%lx\n", Status);
        return FALSE;
    }

    if ( (Control != 0) || (Revision != 1) ) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetControlSecurityDescriptor\n");
        DbgPrint("*Se**     Bad Control or Revision value. \n");
        DbgPrint("*Se**     Status = 0x%lx\n", Status);
        DbgPrint("*Se**     Returned Revision = 0x%lx\n",Revision );
        DbgPrint("*Se**     Returned Control  = 0x%lx\n", (ULONG)Control);
        return FALSE;
    }
#else
    DBG_UNREFERENCED_LOCAL_VARIABLE( Status );
    DBG_UNREFERENCED_LOCAL_VARIABLE( Revision );
    DBG_UNREFERENCED_LOCAL_VARIABLE( Control );
#endif //NOT_YET_DEFINED

    if (!NT_SUCCESS(RtlGetDaclSecurityDescriptor( TTempDescriptor,
                                               &TDaclPresent,
                                               &TDacl,
                                               &TDaclDefaulted))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetDaclSecurityDescriptor, Empty\n");
        return FALSE;
    }

    if (TDaclPresent) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetDaclSecurityDescriptor, Empty-TDaclPresent\n");
        return FALSE;
    }

    if (!NT_SUCCESS(RtlGetSaclSecurityDescriptor( TTempDescriptor,
                                               &TSaclPresent,
                                               &TSacl,
                                               &TSaclDefaulted))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetSaclSecurityDescriptor, Empty\n");
        return FALSE;
    }

    if (TSaclPresent) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetSaclSecurityDescriptor, Empty-TSaclPresent\n");
        return FALSE;
    }

    if (!NT_SUCCESS(RtlGetOwnerSecurityDescriptor( TTempDescriptor,
                                                &TOwner,
                                                &TOwnerDefaulted))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetOwnerSecurityDescriptor, Empty\n");
        return FALSE;
    }

    if (TOwner != NULL) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetOwnerSecurityDescriptor, Empty-TOwner\n");
        return FALSE;
    }

    if (!NT_SUCCESS(RtlGetGroupSecurityDescriptor( TTempDescriptor,
                                                &TGroup,
                                                &TGroupDefaulted))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetGroupSecurityDescriptor, Empty\n");
        return FALSE;
    }

    if (TGroup != NULL) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlGetGroupSecurityDescriptor, Empty-TGroup\n");
        return FALSE;
    }

    //
    // Valid Security Descriptor test
    //

    ((SECURITY_DESCRIPTOR *)TTempDescriptor)->Revision=0;
    if (RtlValidSecurityDescriptor( TTempDescriptor )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSecurityDescriptor, Rev=0\n");
        return FALSE;
    }
    ((SECURITY_DESCRIPTOR *)TTempDescriptor)->Revision=1;

    if (!RtlValidSecurityDescriptor( TTempDescriptor )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlValidSecurityDescriptor, Empty\n");
        return FALSE;
    }


    //
    // Length test
    //

    if (RtlLengthSecurityDescriptor( TTempDescriptor ) != 20) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthSecurityDescriptor, Empty\n");
        return FALSE;
    }

    //
    // Add in an owner
    //

    if (!NT_SUCCESS(RtlSetOwnerSecurityDescriptor( TTempDescriptor, TWilmaSid, FALSE ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlSetOwnerSecurityDescriptor, TWilmaSid\n");
        return FALSE;
    }
    if (RtlLengthSecurityDescriptor( TTempDescriptor ) != 32) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthSecurityDescriptor, Wilma Owner\n");
        return FALSE;
    }

    //
    // Add in a Dacl
    //

    if (!NT_SUCCESS(RtlSetDaclSecurityDescriptor( TTempDescriptor, TRUE,
                                              TDacl, FALSE ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlSetDaclSecurityDescriptor, TDacl\n");
        return FALSE;
    }
    if (RtlLengthSecurityDescriptor( TTempDescriptor ) != 40) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthSecurityDescriptor, TDacl Dacl\n");
        return FALSE;
    }

    //
    // Add in a Sacl
    //

    if (!NT_SUCCESS(RtlSetSaclSecurityDescriptor( TTempDescriptor, TRUE,
                                              TSacl, FALSE ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlSetSaclSecurityDescriptor, TSacl\n");
        return FALSE;
    }
    if (RtlLengthSecurityDescriptor( TTempDescriptor ) != 48) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthSecurityDescriptor, TSacl Sacl\n");
        return FALSE;
    }

    //
    // Add in a Group (with 2 sub-authorities)
    //

    if (!NT_SUCCESS(RtlSetGroupSecurityDescriptor( TTempDescriptor, TWilmaSubSid, FALSE ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlSetGroupSecurityDescriptor, TWilmaSubSid\n");
        return FALSE;
    }
    if (RtlLengthSecurityDescriptor( TTempDescriptor ) != 64) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("*Se**     Failure: RtlLengthSecurityDescriptor, WilmaSub Group\n");
        return FALSE;
    }


    return TRUE;

}


BOOLEAN
TestSeAccessMask()
{
    return TRUE;
}




VOID
DumpAclSizeInfo(PACL_SIZE_INFORMATION AclSizeInfo)
{
    DbgPrint("\n");
    DbgPrint("Acl size info:\n");
    DbgPrint("AceCount = %d\n",AclSizeInfo->AceCount);
    DbgPrint("AclBytesInUse = %d\n",AclSizeInfo->AclBytesInUse);
    DbgPrint("AclBytesFree = %d\n",AclSizeInfo->AclBytesFree);
    return;
}

#define NUM_ACE 6

typedef struct _SIMPLE_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    SID Sid;
    } SIMPLE_ACE, *PSIMPLE_ACE;


BOOLEAN
TestSeAclRtl()
{

    PACL    TDacl;
    NTSTATUS Status;

    ACL_REVISION_INFORMATION AclInformation;
    ACL_REVISION_INFORMATION AclInformationOut;

    ACL_SIZE_INFORMATION AclSizeInfo;

    PVOID AceList;
    PVOID Ace;

    ULONG AceSize;

//
// Define the Dead domain
//
//     Dead Domain         S-1-54399-23-18-02
//     Bobby               S-1-54399-23-18-02-2
//     Jerry               S-1-54399-23-18-02-3
//     Phil                S-1-54399-23-18-02-4
//     Kreutzman           S-1-54399-23-18-02-5
//     Brent               S-1-54399-23-18-02-6
//     Micky               S-1-54399-23-18-02-7
//

#define DEAD_AUTHORITY               {0,0,0,0,212,127}
#define DEAD_SUBAUTHORITY_0          0x00000017L
#define DEAD_SUBAUTHORITY_1          0x00000012L
#define DEAD_SUBAUTHORITY_2          0x00000002L

#define BOBBY_RID               0x00000002
#define JERRY_RID               0x00000003
#define PHIL_RID                0x00000004
#define KREUTZMAN_RID           0x00000005
#define BRENT_RID               0x00000006
#define MICKY_RID               0x00000007

    PSID DeadDomainSid;

    PSID BobbySid;
    PSID JerrySid;
    PSID PhilSid;
    PSID KreutzmanSid;
    PSID BrentSid;
    PSID MickySid;

    ULONG SidWithZeroSubAuthorities;
    ULONG SidWithOneSubAuthority;
    ULONG SidWithThreeSubAuthorities;
    ULONG SidWithFourSubAuthorities;

    SID_IDENTIFIER_AUTHORITY DeadAuthority = DEAD_AUTHORITY;


    //
    //  The following SID sizes need to be allocated
    //

    SidWithZeroSubAuthorities  = RtlLengthRequiredSid( 0 );
    SidWithOneSubAuthority     = RtlLengthRequiredSid( 1 );
    SidWithThreeSubAuthorities = RtlLengthRequiredSid( 3 );
    SidWithFourSubAuthorities  = RtlLengthRequiredSid( 4 );

    DeadDomainSid   = (PSID)TstAllocatePool(PagedPool,SidWithThreeSubAuthorities);

    BobbySid    = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    JerrySid    = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    PhilSid     = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    KreutzmanSid    = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    BrentSid    = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);
    MickySid    = (PSID)TstAllocatePool(PagedPool,SidWithFourSubAuthorities);

    RtlInitializeSid( DeadDomainSid,   &DeadAuthority, 3 );
    *(RtlSubAuthoritySid( DeadDomainSid, 0)) = DEAD_SUBAUTHORITY_0;
    *(RtlSubAuthoritySid( DeadDomainSid, 1)) = DEAD_SUBAUTHORITY_1;
    *(RtlSubAuthoritySid( DeadDomainSid, 2)) = DEAD_SUBAUTHORITY_2;

    RtlCopySid( SidWithFourSubAuthorities, BobbySid, DeadDomainSid);
    *(RtlSubAuthorityCountSid( BobbySid )) += 1;
    *(RtlSubAuthoritySid( BobbySid, 3)) = BOBBY_RID;

    RtlCopySid( SidWithFourSubAuthorities, JerrySid, DeadDomainSid);
    *(RtlSubAuthorityCountSid( JerrySid )) += 1;
    *(RtlSubAuthoritySid( JerrySid, 3)) = JERRY_RID;

    RtlCopySid( SidWithFourSubAuthorities, PhilSid, DeadDomainSid);
    *(RtlSubAuthorityCountSid( PhilSid )) += 1;
    *(RtlSubAuthoritySid( PhilSid, 3)) = PHIL_RID;

    RtlCopySid( SidWithFourSubAuthorities, KreutzmanSid, DeadDomainSid);
    *(RtlSubAuthorityCountSid( KreutzmanSid )) += 1;
    *(RtlSubAuthoritySid( KreutzmanSid, 3)) = KREUTZMAN_RID;

    RtlCopySid( SidWithFourSubAuthorities, BrentSid, DeadDomainSid);
    *(RtlSubAuthorityCountSid( BrentSid )) += 1;
    *(RtlSubAuthoritySid( BrentSid, 3)) = BRENT_RID;

    RtlCopySid( SidWithFourSubAuthorities, MickySid, DeadDomainSid);
    *(RtlSubAuthorityCountSid( MickySid )) += 1;
    *(RtlSubAuthoritySid( MickySid, 3)) = MICKY_RID;

    TDacl = (PACL)TstAllocatePool( PagedPool, 256 );

    //DbgBreakPoint();

    if (!NT_SUCCESS(Status = RtlCreateAcl( TDacl, 256, ACL_REVISION ))) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlCreateAcl returned %X \n",Status);
        return(FALSE);
    }

    //DbgBreakPoint();

    if (!NT_SUCCESS( Status = RtlValidAcl( TDacl ) )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlValidAcl returned %X \n",Status);
        return(FALSE);
    }

    //DbgBreakPoint();

    AclInformation.AclRevision = ACL_REVISION;

    if (!NT_SUCCESS( Status = RtlSetInformationAcl( TDacl, &AclInformation,
                sizeof(AclInformation), AclRevisionInformation ) )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlSetInformation returned %X \n",Status);
        return(FALSE);
    }

    if (!NT_SUCCESS( Status = RtlQueryInformationAcl( TDacl, (PVOID)&AclInformationOut,
                sizeof(AclInformationOut), AclRevisionInformation ) )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlQueryInformation returned %X during revision query \n",Status);
        return(FALSE);
    }

    if (AclInformationOut.AclRevision != ACL_REVISION) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlQueryInformation returned incorrect revision \n");
        return(FALSE);
    }

    if (!NT_SUCCESS( Status = RtlQueryInformationAcl( TDacl, (PVOID)&AclSizeInfo,
                sizeof(AclSizeInfo), AclSizeInformation ) )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlQueryInformation returned %X during size query \n",Status);
        return(FALSE);
    }

    // DumpAclSizeInfo(&AclSizeInfo);

    AceSize = 6 * SidWithFourSubAuthorities + 1 * SidWithThreeSubAuthorities
                    + 7 * (sizeof( ACE_HEADER ) + sizeof( ACCESS_MASK ));

    AceList = (PVOID)TstAllocatePool(PagedPool, AceSize);

    Ace = AceList;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithThreeSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithThreeSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,DeadDomainSid);

    (ULONG)Ace += ((PSIMPLE_ACE)Ace)->Header.AceSize;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithFourSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithFourSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,BobbySid);

    (ULONG)Ace += ((PSIMPLE_ACE)Ace)->Header.AceSize;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithFourSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithFourSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,JerrySid);

    (ULONG)Ace += ((PSIMPLE_ACE)Ace)->Header.AceSize;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithFourSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithFourSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,PhilSid);

    (ULONG)Ace += ((PSIMPLE_ACE)Ace)->Header.AceSize;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithFourSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithFourSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,KreutzmanSid);

    (ULONG)Ace += ((PSIMPLE_ACE)Ace)->Header.AceSize;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithFourSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithFourSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,BrentSid);

    (ULONG)Ace += ((PSIMPLE_ACE)Ace)->Header.AceSize;

    ((PSIMPLE_ACE)Ace)->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ((PSIMPLE_ACE)Ace)->Header.AceSize = (USHORT)SidWithFourSubAuthorities +
                  (USHORT)sizeof(ACE_HEADER) + (USHORT)sizeof( ACCESS_MASK );
    ((PSIMPLE_ACE)Ace)->Header.AceFlags = OBJECT_INHERIT_ACE;
    ((PSIMPLE_ACE)Ace)->Mask = DELETE;
    RtlCopySid(SidWithFourSubAuthorities,&((PSIMPLE_ACE)Ace)->Sid,MickySid);

    //DbgBreakPoint();

    RtlAddAce(TDacl, ACL_REVISION, 0, AceList, AceSize);

    if (!NT_SUCCESS( Status = RtlQueryInformationAcl( TDacl, (PVOID)&AclSizeInfo,
                sizeof(AclSizeInfo), AclSizeInformation ) )) {
        DbgPrint("**** Failed **** \n");
        DbgPrint("RtlQueryInformation returned %X during size query \n",Status);
        return(FALSE);
    }

#if 0
    RtlDumpAcl(TDacl);
#endif

    RtlGetAce( TDacl, 5, &Ace );

    if ( !RtlEqualSid( &((PSIMPLE_ACE)Ace)->Sid, BrentSid) ) {
        DbgPrint("\n **** Failed **** \n");
        DbgPrint("RtlGetAce returned wrong Ace\n");
        return(FALSE);
    }

    if (!NT_SUCCESS(RtlDeleteAce (TDacl, 5))) {
        DbgPrint("\n **** Failed **** \n");
        DbgPrint("RtlDeleteAce failed\n");
        return(FALSE);
    }

#if 0
    RtlDumpAcl(TDacl);
#endif

    return(TRUE);
}


BOOLEAN
TestSeRtl()
{

    BOOLEAN Result = TRUE;

    DbgPrint("Se:   Global Variable Initialization...                        ");
    if (TSeVariableInitialization()) {
        DbgPrint("Succeeded.\n");
    } else {
        Result = FALSE;
    }

    DbgPrint("Se:   SID test...                                              ");
    if (TestSeSid()) {
        DbgPrint("Succeeded.\n");
    } else {
        Result = FALSE;
    }

    DbgPrint("Se:   SECURITY_DESCRIPTOR test...                              ");
    if (TestSeSecurityDescriptor()) {
        DbgPrint("Succeeded.\n");
    } else {
        Result = FALSE;
    }

    DbgPrint("Se:   ACCESS_MASK test...                                      ");
    if (TestSeAccessMask()) {
        DbgPrint("Succeeded.\n");
    } else {
        Result = FALSE;
    }

    DbgPrint("Se:   ACL test...                                              ");
    if (TestSeAclRtl()) {
        DbgPrint("Succeeded.\n");
    } else {
        Result = FALSE;
    }

    DbgPrint("\n");
    DbgPrint("\n");
    DbgPrint("    ********************\n");
    DbgPrint("    **                **\n");

    if (Result = TRUE) {
        DbgPrint("    ** Test Succeeded **\n");
    } else {
        DbgPrint("    **  Test Failed   **\n");
    }

    DbgPrint("    **                **\n");
    DbgPrint("    ********************\n");
    DbgPrint("\n");
    DbgPrint("\n");

    return Result;
}
