/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    AllocSup.c

Abstract:

    This module implements mappings to physical blocks on UDF media.  The basic
    structure used here is the Pcb, which contains lookup information for each
    partition reference in the volume.

Author:

    Dan Lovinger    [DanLo]   5-Sep-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_ALLOCSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_ALLOCSUP)

//
//  Local support routines.
//

PPCB
UdfCreatePcb (
    IN ULONG NumberOfPartitions
    );

NTSTATUS
UdfLoadSparingTables(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PPCB Pcb,
    ULONG Reference
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfAddToPcb)
#pragma alloc_text(PAGE, UdfCompletePcb)
#pragma alloc_text(PAGE, UdfCreatePcb)
#pragma alloc_text(PAGE, UdfDeletePcb)
#pragma alloc_text(PAGE, UdfEquivalentPcb)
#pragma alloc_text(PAGE, UdfInitializePcb)
#pragma alloc_text(PAGE, UdfLookupAllocation)
#pragma alloc_text(PAGE, UdfLookupMetaVsnOfExtent)
#pragma alloc_text(PAGE, UdfLookupPsnOfExtent)
#endif


BOOLEAN
UdfLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG FileOffset,
    OUT PLONGLONG DiskOffset,
    OUT PULONG ByteCount
    )

/*++

Routine Description:

    This routine looks through the mapping information for the file
    to find the logical diskoffset and number of bytes at that offset.

    This routine assumes we are looking up a valid range in the file.  If
    a mapping does not exist,

Arguments:

    Fcb - Fcb representing this stream.

    FileOffset - Lookup the allocation beginning at this point.

    DiskOffset - Address to store the logical disk offset.

    ByteCount - Address to store the number of contiguous bytes beginning
        at DiskOffset above.

Return Value:

    BOOLEAN - whether the extent is unrecorded data

--*/

{
    PVCB Vcb;

    BOOLEAN Recorded = TRUE;

    BOOLEAN Result;

    LARGE_INTEGER LocalPsn;
    LARGE_INTEGER LocalSectorCount;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    //
    //  We will never be looking up the allocations of embedded objects.
    //

    ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_EMBEDDED_DATA ));

    Vcb = Fcb->Vcb;

    LocalPsn.QuadPart = LocalSectorCount.QuadPart = 0;

    //
    //  Lookup the entry containing this file offset.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_VMCB_MAPPING )) {

        //
        //  Map this offset into the metadata stream.
        //

        ASSERT( SectorOffset( Vcb, FileOffset ) == 0 );

        Result = UdfVmcbVbnToLbn( &Vcb->Vmcb,
                                  SectorsFromBytes( Vcb, FileOffset ),
                                  &LocalPsn.LowPart,
                                  &LocalSectorCount.LowPart );

        ASSERT( Result );

    } else {

        //
        //  Map this offset in a regular stream.
        //

        ASSERT( FlagOn( Fcb->FcbState, FCB_STATE_MCB_INITIALIZED ));

        Result = FsRtlLookupLargeMcbEntry( &Fcb->Mcb,
                                           LlSectorsFromBytes( Vcb, FileOffset ),
                                           &LocalPsn.QuadPart,
                                           &LocalSectorCount.QuadPart,
                                           NULL,
                                           NULL,
                                           NULL );
    }

    //
    //  If within the Mcb then we use the data out of this entry and are nearly done.
    //

    if (Result) {

        if ( LocalPsn.QuadPart == -1 ) {

            //
            //  Regular files can have holey allocations which represent unrecorded extents.  For
            //  such extents which are sandwiched in between recorded extents of the file, the Mcb
            //  package tells us that it found a valid mapping but that it doesn't correspond to
            //  any extents on the media yet.  In this case, simply fake the disk offset.  The
            //  returned sector count is accurate.
            //

            *DiskOffset = 0;

            Recorded = FALSE;

        } else {

            //
            //  Now mimic the effects of physical sector sparing.  This may shrink the size of the
            //  returned run if sparing interrupted the extent on disc.
            //

            ASSERT( LocalPsn.HighPart == 0 );

            if (Vcb->Pcb->SparingMcb) {

                LONGLONG SparingPsn;
                LONGLONG SparingSectorCount;

                if (FsRtlLookupLargeMcbEntry( Vcb->Pcb->SparingMcb,
                                              LocalPsn.LowPart,
                                              &SparingPsn,
                                              &SparingSectorCount,
                                              NULL,
                                              NULL,
                                              NULL )) {

                    //
                    //  Only emit noise if we will really change anything as a result
                    //  of the sparing table.
                    //

                    if (SparingPsn != -1 ||
                        SparingSectorCount < LocalSectorCount.QuadPart) {

                        DebugTrace(( 0, Dbg, "UdfLookupAllocation, spared [%x, +%x) onto [%x, +%x)\n",
                                             LocalPsn.LowPart,
                                             LocalSectorCount.LowPart,
                                             (ULONG) SparingPsn,
                                             (ULONG) SparingSectorCount ));
                    }

                    //
                    //  If we did not land in a hole, map the sector.
                    //

                    if (SparingPsn != -1) {

                        LocalPsn.QuadPart = SparingPsn;
                    }

                    //
                    //  The returned sector count now reduces the previous sector count.
                    //  If we landed in a hole, this indicates that the trailing edge of
                    //  the extent is spared, if not this indicates that the leading
                    //  edge is spared.
                    //

                    if (SparingSectorCount < LocalSectorCount.QuadPart) {

                        LocalSectorCount.QuadPart = SparingSectorCount;
                    }
                }
            }

            *DiskOffset = LlBytesFromSectors( Vcb, LocalPsn.QuadPart ) + SectorOffset( Vcb, FileOffset );

            //
            //  Now we can apply method 2 fixups, which will again interrupt the size of the extent.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_METHOD_2_FIXUP )) {

                LARGE_INTEGER SectorsToRunout;

                SectorsToRunout.QuadPart= UdfMethod2NextRunoutInSectors( Vcb, *DiskOffset );

                if (SectorsToRunout.QuadPart < LocalSectorCount.QuadPart) {

                    LocalSectorCount.QuadPart = SectorsToRunout.QuadPart;
                }

                *DiskOffset = UdfMethod2TransformByteOffset( Vcb, *DiskOffset );
            }
        }

    } else {

        //
        //  We know that prior to this call the system has restricted IO to points within the
        //  the file data.  Since we failed to find a mapping this is an unrecorded extent at
        //  the end of the file, so just conjure up a proper representation.
        //

        ASSERT( FileOffset < Fcb->FileSize.QuadPart );

        *DiskOffset = 0;

        LocalSectorCount.QuadPart = LlSectorsFromBytes( Vcb, Fcb->FileSize.QuadPart ) -
                                    LlSectorsFromBytes( Vcb, FileOffset ) +
                                    1;

        Recorded = FALSE;
    }

    //
    //  Restrict to MAXULONG bytes of allocation
    //

    if (LocalSectorCount.QuadPart > SectorsFromBytes( Vcb, MAXULONG )) {

        *ByteCount = MAXULONG;

    } else {

        *ByteCount = BytesFromSectors( Vcb, LocalSectorCount.LowPart );
    }

    *ByteCount -= SectorOffset( Vcb, FileOffset );

    return Recorded;
}


VOID
UdfDeletePcb (
    IN PPCB Pcb
    )

/*++

Routine Description:

    This routine deallocates a Pcb and all ancilliary structures.

Arguments:

    Pcb - Pcb being deleted

Return Value:

    None

--*/

{
    PPARTITION Partition;

    if (Pcb->SparingMcb) {

        FsRtlUninitializeLargeMcb( Pcb->SparingMcb );
        UdfFreePool( &Pcb->SparingMcb );
    }

    for (Partition = Pcb->Partition;
         Partition < &Pcb->Partition[Pcb->Partitions];
         Partition++) {

        switch (Partition->Type) {

            case Physical:

                UdfFreePool( &Partition->Physical.PartitionDescriptor );
                UdfFreePool( &Partition->Physical.SparingMap );                

                break;

            case Virtual:
            case Uninitialized:
                break;

            default:

                ASSERT( FALSE );
                break;
        }
    }

    ExFreePool( Pcb );
}


NTSTATUS
UdfInitializePcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PPCB *Pcb,
    IN PNSR_LVOL LVD
    )

/*++

Routine Description:

    This routine walks through the partition map of a Logical Volume Descriptor
    and builds an intializing Pcb from it.  The Pcb will be ready to be used
    in searching for the partition descriptors of a volume.

Arguments:

    Vcb - The volume this Pcb will pertain to

    Pcb - Caller's pointer to the Pcb

    LVD - The Logical Volume Descriptor being used

Return Value:

    STATUS_SUCCESS if the partition map is good and the Pcb is built

    STATUS_DISK_CORRUPT_ERROR if corrupt maps are found

    STATUS_UNRECOGNIZED_VOLUME if noncompliant maps are found

--*/

{
    PPARTMAP_UDF_GENERIC Map;
    PPARTITION Partition;

    BOOLEAN Found;

    PAGED_CODE();

    //
    //  Check the input parameters
    //

    ASSERT_OPTIONAL_PCB( *Pcb );

    DebugTrace(( +1, Dbg,
                 "UdfInitializePcb, Lvd %08x\n",
                 LVD ));

    //
    //  Delete a pre-existing (partially initialized from a failed
    //  crawl of a VDS) Pcb.
    //

    if (*Pcb != NULL) {

        UdfDeletePcb( *Pcb );
        *Pcb = NULL;
    }

    *Pcb = UdfCreatePcb( LVD->MapTableCount );

    //
    //  Walk the table of partition maps intializing the Pcb for the descriptor
    //  initialization pass.
    //

    for (Map = (PPARTMAP_UDF_GENERIC) LVD->MapTable,
         Partition = (*Pcb)->Partition;

         Partition < &(*Pcb)->Partition[(*Pcb)->Partitions];

         Map = Add2Ptr( Map, Map->Length, PPARTMAP_UDF_GENERIC ),
         Partition++) {

        //
        //  Now check that this LVD can actually contain this map entry.  First check that
        //  the descriptor can contain the first few fields, then check that it can hold
        //  all of the bytes claimed by the descriptor.
        //

        if (Add2Ptr( Map, sizeof( PARTMAP_GENERIC ), PCHAR ) > Add2Ptr( LVD, ISONsrLvolSize( LVD ), PCHAR ) ||
            Add2Ptr( Map, Map->Length,               PCHAR ) > Add2Ptr( LVD, ISONsrLvolSize( LVD ), PCHAR )) {

            DebugTrace(( 0, Dbg,
                         "UdfInitializePcb, map at +%04x beyond Lvd size %04x\n",
                         (PCHAR) Map - (PCHAR) LVD,
                         ISONsrLvolSize( LVD )));

            DebugTrace(( -1, Dbg,
                         "UdfInitializePcb -> STATUS_DISK_CORRUPT_ERROR\n" ));

            return STATUS_DISK_CORRUPT_ERROR;
        }

        //
        //  Now load up this map entry.
        //

        switch (Map->Type) {

            case PARTMAP_TYPE_PHYSICAL:

                {
                    PPARTMAP_PHYSICAL MapPhysical = (PPARTMAP_PHYSICAL) Map;

                    //
                    //  Type 1 - Physical Partition
                    //

                    DebugTrace(( 0, Dbg,
                                 "UdfInitializePcb, map reference %02x is Physical (Partition # %08x)\n",
                                 (Partition - (*Pcb)->Partition)/sizeof(PARTITION),
                                 MapPhysical->Partition ));

                    //
                    //  It must be the case that the volume the partition is on is the first
                    //  one since we only do single disc UDF.  This will have already been
                    //  checked by the caller.
                    //

                    if (MapPhysical->VolSetSeq > 1) {

                        DebugTrace(( 0, Dbg,
                                     "UdfInitializePcb, ... but physical partition resides on volume set volume # %08x (> 1)!\n",
                                     MapPhysical->VolSetSeq ));

                        DebugTrace(( -1, Dbg,
                                     "UdfInitializePcb -> STATUS_DISK_CORRUPT_ERROR\n" ));

                        return STATUS_DISK_CORRUPT_ERROR;
                    }

                    SetFlag( (*Pcb)->Flags, PCB_FLAG_PHYSICAL_PARTITION );
                    Partition->Type = Physical;
                    Partition->Physical.PartitionNumber = MapPhysical->Partition;
                }

                break;

            case PARTMAP_TYPE_PROXY:

                //
                //  Type 2 - a Proxy Partition, something not explicitly physical.
                //

                DebugTrace(( 0, Dbg,
                             "UdfInitializePcb, map reference %02x is a proxy\n",
                             (Partition - (*Pcb)->Partition)/sizeof(PARTITION)));

                //
                //  Handle the various types of proxy partitions we recognize
                //

                if (UdfDomainIdentifierContained( &Map->PartID,
                                                  &UdfVirtualPartitionDomainIdentifier,
                                                  UDF_VERSION_150,
                                                  UDF_VERSION_RECOGNIZED )) {

                    {
                        PPARTMAP_VIRTUAL MapVirtual = (PPARTMAP_VIRTUAL) Map;

                        //
                        //  Only one of these guys can exist, since there can be only one VAT per media surface.
                        //

                        if (FlagOn( (*Pcb)->Flags, PCB_FLAG_VIRTUAL_PARTITION )) {

                            DebugTrace(( 0, Dbg,
                                         "UdfInitializePcb, ... but this is a second virtual partition!?!!\n" ));

                            DebugTrace(( -1, Dbg,
                                         "UdfInitializePcb -> STATUS_UNCRECOGNIZED_VOLUME\n" ));

                            return STATUS_UNRECOGNIZED_VOLUME;
                        }

                        DebugTrace(( 0, Dbg,
                                     "UdfInitializePcb, ... Virtual (Partition # %08x)\n",
                                     MapVirtual->Partition ));

                        SetFlag( (*Pcb)->Flags, PCB_FLAG_VIRTUAL_PARTITION );
                        Partition->Type = Virtual;

                        //
                        //  We will convert the partition number to a partition reference
                        //  before returning.
                        //

                        Partition->Virtual.RelatedReference = MapVirtual->Partition;
                    }

                } else if (UdfDomainIdentifierContained( &Map->PartID,
                                                         &UdfSparablePartitionDomainIdentifier,
                                                         UDF_VERSION_150,
                                                         UDF_VERSION_RECOGNIZED )) {

                    {
                        NTSTATUS Status;
                        PPARTMAP_SPARABLE MapSparable = (PPARTMAP_SPARABLE) Map;

                        //
                        //  It must be the case that the volume the partition is on is the first
                        //  one since we only do single disc UDF.  This will have already been
                        //  checked by the caller.
                        //

                        if (MapSparable->VolSetSeq > 1) {

                            DebugTrace(( 0, Dbg,
                                         "UdfInitializePcb, ... but sparable partition resides on volume set volume # %08x (> 1)!\n",
                                         MapSparable->VolSetSeq ));

                            DebugTrace(( -1, Dbg,
                                         "UdfInitializePcb -> STATUS_DISK_CORRUPT_ERROR\n" ));

                            return STATUS_DISK_CORRUPT_ERROR;
                        }

                        DebugTrace(( 0, Dbg,
                                     "UdfInitializePcb, ... Sparable (Partition # %08x)\n",
                                     MapSparable->Partition ));

                        //
                        //  We pretend that sparable partitions are basically the same as
                        //  physical partitions.  Since we are not r/w (and will never be
                        //  on media that requires host-based sparing in any case), this
                        //  is a good simplification.
                        //

                        SetFlag( (*Pcb)->Flags, PCB_FLAG_SPARABLE_PARTITION );
                        Partition->Type = Physical;
                        Partition->Physical.PartitionNumber = MapSparable->Partition;

                        //
                        //  Save this map for use when the partition descriptor is found.
                        //  We can't load the sparing table at this time because we have
                        //  to turn the Lbn->Psn mapping into a Psn->Psn mapping.  UDF
                        //  believes that the way sparing will be used in concert with
                        //  the Lbn->Psn mapping engine (like UdfLookupPsnOfExtent).
                        //
                        //  Unfortunately, this would be a bit painful at this time.
                        //  The users of UdfLookupPsnOfExtent would need to iterate
                        //  over a new interface (not so bad) but the Vmcb package
                        //  would need to be turned inside out so that it didn't do
                        //  the page-filling alignment of blocks in the metadata
                        //  stream - instead, UdfLookupMetaVsnOfExtent would need to
                        //  do this itself.  I choose to lay the sparing engine into
                        //  the read path and raw sector read engine instead.
                        //

                        Partition->Physical.SparingMap = FsRtlAllocatePoolWithTag( PagedPool,
                                                                                   sizeof(PARTMAP_SPARABLE),
                                                                                   TAG_NSR_FSD);
                        RtlCopyMemory( Partition->Physical.SparingMap,
                                       MapSparable,
                                       sizeof(PARTMAP_SPARABLE));
                    }

                } else {

                    DebugTrace(( 0, Dbg,
                                 "UdfInitializePcb, ... but we don't recognize this proxy!\n" ));

                    DebugTrace(( -1, Dbg,
                                 "UdfInitializePcb -> STATUS_UNRECOGNIZED_VOLUME\n" ));

                    return STATUS_UNRECOGNIZED_VOLUME;
                }

                break;

            default:

                DebugTrace(( 0, Dbg,
                             "UdfInitializePcb, map reference %02x is of unknown type %02x\n",
                             Map->Type ));

                DebugTrace(( -1, Dbg,
                             "UdfInitializePcb -> STATUS_UNRECOGNIZED_VOLUME\n" ));

                return STATUS_UNRECOGNIZED_VOLUME;
                break;
        }
    }

    if (!FlagOn( (*Pcb)->Flags, PCB_FLAG_PHYSICAL_PARTITION | PCB_FLAG_SPARABLE_PARTITION )) {

        DebugTrace(( 0, Dbg,
                     "UdfInitializePcb, no physical partition seen on this logical volume!\n" ));

        DebugTrace(( -1, Dbg,
                     "UdfInitializePcb -> STATUS_UNRECOGNIZED_VOLUME\n" ));

        return STATUS_UNRECOGNIZED_VOLUME;
    }

    if (FlagOn( (*Pcb)->Flags, PCB_FLAG_VIRTUAL_PARTITION )) {

        PPARTITION Host;

        //
        //  Confirm the validity of any type 2 virtual maps on this volume
        //  and convert partition numbers to partition references that will
        //  immediately index an element of the Pcb.
        //

        for (Partition = (*Pcb)->Partition;
             Partition < &(*Pcb)->Partition[(*Pcb)->Partitions];
             Partition++) {

            if (Partition->Type == Virtual) {

                //
                //  Go find the partition this thing is talking about
                //

                Found = FALSE;

                for (Host = (*Pcb)->Partition;
                     Host < &(*Pcb)->Partition[(*Pcb)->Partitions];
                     Host++) {

                    if (Host->Type == Physical &&
                        Host->Physical.PartitionNumber ==
                        Partition->Virtual.RelatedReference) {

                        Partition->Virtual.RelatedReference =
                            (USHORT)(Host - (*Pcb)->Partition)/sizeof(PARTITION);
                        Found = TRUE;
                        break;
                    }
                }

                //
                //  Failure to find a physical partition for this virtual guy
                //  is not a good sign.
                //

                if (!Found) {

                    return STATUS_DISK_CORRUPT_ERROR;
                }
            }
        }
    }

    DebugTrace(( -1, Dbg,
             "UdfInitializePcb -> STATUS_SUCCESS\n" ));

    return STATUS_SUCCESS;
}


VOID
UdfAddToPcb (
    IN PPCB Pcb,
    IN PNSR_PART PartitionDescriptor
)

/*++

Routine Description:

    This routine possibly adds a partition descriptor into a Pcb if it
    turns out to be of higher precendence than a descriptor already
    present.  Used in building a Pcb already initialized in preperation
    for UdfCompletePcb.

Arguments:

    Vcb - Vcb of the volume the Pcb describes

    Pcb - Pcb being filled in

Return Value:

    None. An old partition descriptor may be returned in the input field.

--*/

{
    USHORT Reference;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_PCB( Pcb );
    ASSERT( PartitionDescriptor );

    for (Reference = 0;
         Reference < Pcb->Partitions;
         Reference++) {

        DebugTrace(( 0, Dbg, "UdfAddToPcb,  considering partition reference %d (type %d)\n", (ULONG)Reference, Pcb->Partition[Reference].Type));
        
        switch (Pcb->Partition[Reference].Type) {

            case Physical:

                //
                //  Now possibly store this descriptor in the Pcb if it is
                //  the partition number for this partition reference.
                //

                if (Pcb->Partition[Reference].Physical.PartitionNumber == PartitionDescriptor->Number) {

                    //
                    //  It seems to be legal (if questionable) for multiple partition maps to reference 
                    //  the same partition descriptor.  So we make a copy of the descriptor for each 
                    //  referencing partitionmap to make life easier when it comes to freeing it.
                    //

                    UdfStoreVolumeDescriptorIfPrevailing( (PNSR_VD_GENERIC *) &Pcb->Partition[Reference].Physical.PartitionDescriptor,
                                                          (PNSR_VD_GENERIC) PartitionDescriptor );
                }
                
                break;

            case Virtual:
                break;

            default:

                ASSERT(FALSE);
                break;
        }
    }
}


NTSTATUS
UdfCompletePcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PPCB Pcb
    )

/*++

Routine Description:

    This routine completes initialization of a Pcb which has been filled
    in with partition descriptors.  Initialization-time data such as the
    physical partition descriptors will be returned to the system.

Arguments:

    Vcb - Vcb of the volume the Pcb describes

    Pcb - Pcb being completed

Return Value:

    NTSTATUS according to whether intialization completion was succesful

--*/

{
    ULONG Reference;

    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    ASSERT_PCB( Pcb );

    DebugTrace(( +1, Dbg, "UdfCompletePcb, Vcb %08x Pcb %08x\n", Vcb, Pcb ));

    //
    //  Complete intialization all physical partitions
    //

    for (Reference = 0;
         Reference < Pcb->Partitions;
         Reference++) {

        DebugTrace(( 0, Dbg, "UdfCompletePcb, Examining Ref %u (type %u)!\n", Reference, Pcb->Partition[Reference].Type));

        switch (Pcb->Partition[Reference].Type) {

            case Physical:

                if (Pcb->Partition[Reference].Physical.PartitionDescriptor == NULL) {

                    DebugTrace(( 0, Dbg,
                                 "UdfCompletePcb, ... but didn't find Partition# %u!\n",
                                 Pcb->Partition[Reference].Physical.PartitionNumber ));

                    DebugTrace(( -1, Dbg, "UdfCompletePcb -> STATUS_DISK_CORRUPT_ERROR\n" ));

                    return STATUS_DISK_CORRUPT_ERROR;
                }

                Pcb->Partition[Reference].Physical.Start =
                    Pcb->Partition[Reference].Physical.PartitionDescriptor->Start;
                Pcb->Partition[Reference].Physical.Length =
                    Pcb->Partition[Reference].Physical.PartitionDescriptor->Length;


                //
                //  Retrieve the sparing information at this point if appropriate.
                //  We have to do this when we can map logical -> physical blocks.
                //

                if (Pcb->Partition[Reference].Physical.SparingMap) {

                    Status = UdfLoadSparingTables( IrpContext,
                                                   Vcb,
                                                   Pcb,
                                                   Reference );

                    if (!NT_SUCCESS( Status )) {

                        DebugTrace(( -1, Dbg,
                                     "UdfCompletePcb -> %08x\n", Status ));
                        return Status;
                    }
                }

                //
                //  We will not need the descriptor or sparing map anymore, so drop them.  
                //

                UdfFreePool( &Pcb->Partition[Reference].Physical.PartitionDescriptor );
                UdfFreePool( &Pcb->Partition[Reference].Physical.SparingMap );
                break;

            case Virtual:
                break;

            default:

                ASSERT(FALSE);
                break;
        }
    }

    DebugTrace(( -1, Dbg, "UdfCompletePcb -> STATUS_SUCCESS\n" ));

    return STATUS_SUCCESS;
}


BOOLEAN
UdfEquivalentPcb (
    IN PIRP_CONTEXT IrpContext,
    IN PPCB Pcb1,
    IN PPCB Pcb2
    )

/*++

Routine Description:

    This routine compares two completed Pcbs to see if they appear equivalent.

Arguments:

    Pcb1 - Pcb being compared

    Pcb2 - Pcb being compared

Return Value:

    BOOLEAN according to whether they are equivalent (TRUE, else FALSE)

--*/

{
    ULONG Index;

    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    if (Pcb1->Partitions != Pcb2->Partitions) {

        return FALSE;
    }

    for (Index = 0;
         Index < Pcb1->Partitions;
         Index++) {

        //
        //  First check that the partitions are of the same type.
        //

        if (Pcb1->Partition[Index].Type != Pcb2->Partition[Index].Type) {

            return FALSE;
        }

        //
        //  Now the map content must be the same ...
        //

        switch (Pcb1->Partition[Index].Type) {

            case Physical:

                if (Pcb1->Partition[Index].Physical.PartitionNumber != Pcb2->Partition[Index].Physical.PartitionNumber ||
                    Pcb1->Partition[Index].Physical.Length != Pcb2->Partition[Index].Physical.Length ||
                    Pcb1->Partition[Index].Physical.Start != Pcb2->Partition[Index].Physical.Start) {

                    return FALSE;
                }
                break;

            case Virtual:

                if (Pcb1->Partition[Index].Virtual.RelatedReference != Pcb2->Partition[Index].Virtual.RelatedReference) {

                    return FALSE;
                }
                break;

            default:

                ASSERT( FALSE);
                return FALSE;
                break;
        }
    }

    //
    //  All map elements were equivalent.
    //

    return TRUE;
}


ULONG
UdfLookupPsnOfExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT Reference,
    IN ULONG Lbn,
    IN ULONG Len
    )

/*++

Routine Description:

    This routine maps the input logical block extent on a given partition to
    a starting physical sector.  It doubles as a bounds checker - if the routine
    does not raise, the caller is guaranteed that the extent lies within the
    partition.

Arguments:

    Vcb - Vcb of logical volume

    Reference - Partition reference to use in the mapping

    Lbn - Logical block number

    Len - Length of extent in bytes

Return Value:

    ULONG physical sector number

--*/

{
    PPCB Pcb = Vcb->Pcb;
    ULONG Psn;

    PBCB Bcb;
    LARGE_INTEGER Offset;
    PULONG MappedLbn;

    PAGED_CODE();

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    ASSERT_PCB( Pcb );

    DebugTrace(( +1, Dbg, "UdfLookupPsnOfExtent, [%04x/%08x, +%08x)\n", Reference, Lbn, Len ));

    if (Reference < Pcb->Partitions) {

        while (TRUE) {

            switch (Pcb->Partition[Reference].Type) {

                case Physical:

                    //
                    //  Check that the input extent lies inside the partition.  Calculate the
                    //  Lbn of the last block and see that it is interior.
                    //

                    if (SectorsFromBlocks( Vcb, Lbn ) + SectorsFromBytes( Vcb, Len ) >
                        Pcb->Partition[Reference].Physical.Length) {

                        goto NoGood;
                    }

                    Psn = Pcb->Partition[Reference].Physical.Start + SectorsFromBlocks( Vcb, Lbn );

                    DebugTrace(( -1, Dbg, "UdfLookupPsnOfExtent -> %08x\n", Psn ));
                    return Psn;

                case Virtual:

                    //
                    //  Bounds check.  Per UDF 2.00 2.3.10 and implied in UDF 1.50, virtual
                    //  extent lengths cannot be greater than one block in size.
                    //

                    if (Lbn + BlocksFromBytes( Vcb, Len ) > Pcb->Partition[Reference].Virtual.Length ||
                        Len > BlockSize( Vcb )) {

                        goto NoGood;
                    }

                    try {

                        //
                        //  Calculate the location of the mapping element in the VAT
                        //  and retrieve.
                        //

                        Offset.QuadPart = Lbn * sizeof(ULONG);

                        CcMapData( Vcb->VatFcb->FileObject,
                                   &Offset,
                                   sizeof(ULONG),
                                   TRUE,
                                   &Bcb,
                                   &MappedLbn );

                        //
                        //  Now rewrite the inputs in terms of the virtual mapping.  We
                        //  will reloop to perform the logical -> physical mapping.
                        //

                        DebugTrace(( 0, Dbg,
                                     "UdfLookupPsnOfExtent, Mapping V %04x/%08x -> L %04x/%08x\n",
                                     Reference,
                                     Lbn,
                                     Pcb->Partition[Reference].Virtual.RelatedReference,
                                     *MappedLbn ));

                        Lbn = *MappedLbn;
                        Reference = Pcb->Partition[Reference].Virtual.RelatedReference;

                    } finally {

                        DebugUnwind( UdfLookupPsnOfExtent );

                        UdfUnpinData( IrpContext, &Bcb );
                    }

                    //
                    //  An Lbn of ~0 in the VAT is defined to indicate that the sector is unused,
                    //  so we should never see such a thing.
                    //

                    if (Lbn == ~0) {

                        goto NoGood;
                    }

                    break;

                default:

                    ASSERT(FALSE);
                    break;
            }
        }
    }

    NoGood:

    //
    //  Some people have misinterpreted a partition number to equal a
    //  partition reference, or perhaps this is just corrupt media.
    //

    UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
}


ULONG
UdfLookupMetaVsnOfExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT Reference,
    IN ULONG Lbn,
    IN ULONG Len,
    IN BOOLEAN ExactEnd
    )

/*++

Routine Description:

    This routine maps the input logical block extent on a given partition to
    a starting virtual block in the metadata stream.  If a mapping does not
    exist, one will be created and the metadata stream extended.

Arguments:

    Vcb - Vcb of logical volume

    Reference - Partition reference to use in the mapping

    Lbn - Logical block number

    Len - Length of extent in bytes
    
    ExactEnd - Indicates the extension policy if these blocks are not mapped.

Return Value:

    ULONG virtual sector number

    Raised status if the Lbn extent is split across multiple Vbn extents.

--*/

{
    ULONG Vsn;
    ULONG Psn;
    ULONG SectorCount;

    BOOLEAN Result;

    BOOLEAN UnwindExtension = FALSE;
    LONGLONG UnwindAllocationSize;

    PFCB Fcb = NULL;

    //
    //  Check inputs
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    //
    //  The extent must be an integral number of logical blocks in length.
    //

    if (Len == 0 || BlockOffset( Vcb, Len )) {

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }


    //
    //  Get the physical mapping of the extent.  The Mcb package operates on ULONG/ULONG
    //  keys and values so we must render our 48bit address into 32.  We can do this since
    //  this is a single surface implementation, and it is guaranteed that a surface cannot
    //  contain more than MAXULONG physical sectors.
    //

    Psn = UdfLookupPsnOfExtent( IrpContext,
                                Vcb,
                                Reference,
                                Lbn,
                                Len );

    //
    //  Use try-finally for cleanup
    //

    try {

        //
        //  We must safely establish a mapping and extend the metadata stream so that cached
        //  reads can occur on this new extent.
        //

        Fcb = Vcb->MetadataFcb;
        UdfLockFcb( IrpContext, Fcb );
        
        Result = UdfVmcbLbnToVbn( &Vcb->Vmcb,
                                  Psn,
                                  &Vsn,
                                  &SectorCount );

        if (Result) {

            //
            //  If the mapping covers the extent, we can give this back.
            //

            if (BlocksFromSectors( Vcb, SectorCount ) >= BlocksFromBytes( Vcb, Len )) {

                try_leave( NOTHING );

            }

            //
            //  It is a fatal error if the extent we are mapping is not wholly contained
            //  by an extent of Vsns in the Vmcb.  This will indicate that some structure
            //  is trying to overlap another.
            //

            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        //  Add the new mapping.  We know that it is being added to the end of the stream.
        //

        UdfAddVmcbMapping( &Vcb->Vmcb,
                           Psn,
                           SectorsFromBytes( Vcb, Len ),
                           ExactEnd,
                           &Vsn,
                           &SectorCount );

        UnwindAllocationSize = Fcb->AllocationSize.QuadPart;
        UnwindExtension = TRUE;

        Fcb->AllocationSize.QuadPart =
        Fcb->FileSize.QuadPart =
        Fcb->ValidDataLength.QuadPart = LlBytesFromSectors( Vcb, Vsn + SectorCount);

        CcSetFileSizes( Fcb->FileObject, (PCC_FILE_SIZES) &Fcb->AllocationSize );
        UnwindExtension = FALSE;

        //
        //  We do not need to purge the cache maps since the Vmcb will always be
        //  page aligned, and thus any reads will have filled it with valid data.
        //

    } finally {

        if (UnwindExtension) {

            ULONG FirstZappedVsn;

            //
            //  Strip off the additional mappings we made.
            //

            Fcb->AllocationSize.QuadPart =
            Fcb->FileSize.QuadPart =
            Fcb->ValidDataLength.QuadPart = UnwindAllocationSize;

            FirstZappedVsn = SectorsFromBytes( Vcb, UnwindAllocationSize );

            UdfRemoveVmcbMapping( &Vcb->Vmcb,
                                  FirstZappedVsn,
                                  Vsn + SectorCount - FirstZappedVsn );

            CcSetFileSizes( Fcb->FileObject, (PCC_FILE_SIZES) &Fcb->AllocationSize );
        }

        if (Fcb) { UdfUnlockFcb( IrpContext, Fcb ); }
    }

    return Vsn;
}


//
//  Local support routine.
//

PPCB
UdfCreatePcb (
    IN ULONG NumberOfPartitions
    )

/*++

Routine Description:

    This routine creates a new Pcb of the indicated size.

Arguments:

    NumberOfPartitions - Number of partitions this Pcb will describe

Return Value:

    PPCB - the Pcb created

--*/

{
    PPCB Pcb;
    ULONG Size = sizeof(PCB) + sizeof(PARTITION)*NumberOfPartitions;

    PAGED_CODE();

    ASSERT( NumberOfPartitions );
    ASSERT( NumberOfPartitions < MAXUSHORT );

    Pcb = (PPCB) FsRtlAllocatePoolWithTag( UdfPagedPool,
                                           Size,
                                           TAG_PCB );

    RtlZeroMemory( Pcb, Size );

    Pcb->NodeTypeCode = UDFS_NTC_PCB;
    Pcb->NodeByteSize = (USHORT) Size;

    Pcb->Partitions = (USHORT)NumberOfPartitions;

    return Pcb;
}


//
//  Internal support routine
//

NTSTATUS
UdfLoadSparingTables(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PPCB Pcb,
    ULONG Reference
    )

/*++

Routine Description:

    This routine reads the sparing tables for a partition and fills
    in the sparing Mcb.

Arguments:

    Vcb - the volume hosting the spared partition

    Pcb - the partion block corresponding to the volume

    Reference - the partition reference being pulled in

Return Value:

    NTSTATUS according to whether the sparing tables were loaded

--*/

{
    NTSTATUS Status;

    ULONG SparingTable;
    PULONG SectorBuffer;
    ULONG Psn;

    ULONG RemainingBytes;
    ULONG ByteOffset;
    ULONG TotalBytes;

    BOOLEAN Complete;

    PSPARING_TABLE_HEADER Header;
    PSPARING_TABLE_ENTRY Entry;

    PPARTITION Partition = &Pcb->Partition[Reference];
    PPARTMAP_SPARABLE Map = Partition->Physical.SparingMap;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    ASSERT( Map != NULL );

    DebugTrace(( +1, Dbg, "UdfLoadSparingTables, Vcb %08x, PcbPartition %08x, Map @ %08x\n", Vcb, Partition, Map ));

    DebugTrace(( 0, Dbg, "UdfLoadSparingTables, Map sez: PacketLen %u, NTables %u, TableSize %u\n",
                         Map->PacketLength,
                         Map->NumSparingTables,
                         Map->TableSize));


    //
    //  Check that the sparale map appears sane.  If there are no sparing tables that
    //  is pretty OK, and it'll wind up looking like a regular physical partition.
    //

    if (Map->NumSparingTables == 0) {

        DebugTrace((  0, Dbg, "UdfLoadSparingTables, no sparing tables claimed!\n" ));
        DebugTrace(( -1, Dbg, "UdfLoadSparingTables -> STATUS_SUCCESS\n" ));
        return STATUS_SUCCESS;
    }

    if (Map->NumSparingTables > sizeof(Map->TableLocation)/sizeof(ULONG)) {

        DebugTrace((  0, Dbg, "UdfLoadSparingTables, too many claimed tables to fit! (max %u)\n",
                              sizeof(Map->TableLocation)/sizeof(ULONG)));
        DebugTrace(( -1, Dbg, "UdfLoadSparingTables -> STATUS_DISK_CORRUPT_ERROR\n" ));
        return  STATUS_DISK_CORRUPT_ERROR;
    }

    if (Map->PacketLength != UDF_SPARING_PACKET_LENGTH) {

        DebugTrace((  0, Dbg, "UdfLoadSparingTables, packet size is %u (not %u!\n",
                              Map->PacketLength,
                              UDF_SPARING_PACKET_LENGTH ));
        DebugTrace(( -1, Dbg, "UdfLoadSparingTables -> STATUS_DISK_CORRUPT_ERROR\n" ));
        return  STATUS_DISK_CORRUPT_ERROR;
    }

    if (Map->TableSize < sizeof(SPARING_TABLE_HEADER) ||
        (Map->TableSize - sizeof(SPARING_TABLE_HEADER)) % sizeof(SPARING_TABLE_ENTRY) != 0) {

        DebugTrace((  0, Dbg, "UdfLoadSparingTables, sparing table size is too small or unaligned!\n" ));
        DebugTrace(( -1, Dbg, "UdfLoadSparingTables -> STATUS_DISK_CORRUPT_ERROR\n" ));
        return  STATUS_DISK_CORRUPT_ERROR;
    }

#ifdef UDF_SANITY
    DebugTrace(( 0, Dbg, "UdfLoadSparingTables" ));
    for (SparingTable = 0; SparingTable < Map->NumSparingTables; SparingTable++) {

        DebugTrace(( 0, Dbg, ", Table %u @ %x", SparingTable, Map->TableLocation[SparingTable] ));
    }
    DebugTrace(( 0, Dbg, "\n" ));
#endif

    //
    //  If a sparing mcb doesn't exist, manufacture one.
    //

    if (Pcb->SparingMcb == NULL) {

        Pcb->SparingMcb = FsRtlAllocatePoolWithTag( PagedPool, sizeof(LARGE_MCB), TAG_SPARING_MCB );
        FsRtlInitializeLargeMcb( Pcb->SparingMcb, PagedPool );
    }

    SectorBuffer = FsRtlAllocatePoolWithTag( PagedPool, PAGE_SIZE, TAG_NSR_FSD );

    //
    //  Now loop across the sparing tables and pull the data in.
    //

    try {

        for (Complete = FALSE, SparingTable = 0;

             SparingTable < Map->NumSparingTables;

             SparingTable++) {

            DebugTrace((  0, Dbg, "UdfLoadSparingTables, loading sparing table %u!\n",
                                  SparingTable ));

            ByteOffset = 0;
            TotalBytes = 0;
            RemainingBytes = 0;

            do {

                if (RemainingBytes == 0) {

                    (VOID) UdfReadSectors( IrpContext,
                                           BytesFromSectors( Vcb, Map->TableLocation[SparingTable] ) + ByteOffset,
                                           SectorSize( Vcb ),
                                           FALSE,
                                           SectorBuffer,
                                           Vcb->TargetDeviceObject );

                    //
                    //  Verify the descriptor at the head of the sparing table.  If it is not
                    //  valid, we just break out for a chance at the next table, if any.
                    //

                    if (ByteOffset == 0) {

                        Header = (PSPARING_TABLE_HEADER) SectorBuffer;

                        if (!UdfVerifyDescriptor( IrpContext,
                                                  &Header->Destag,
                                                  0,
                                                  SectorSize( Vcb ),
                                                  Header->Destag.Lbn,
                                                  TRUE )) {

                            DebugTrace((  0, Dbg, "UdfLoadSparingTables, sparing table %u didn't verify destag!\n",
                                                  SparingTable ));
                            break;
                        }

                        if (!UdfUdfIdentifierContained( &Header->RegID,
                                                        &UdfSparingTableIdentifier,
                                                        UDF_VERSION_150,
                                                        UDF_VERSION_RECOGNIZED,
                                                        OSCLASS_INVALID,
                                                        OSIDENTIFIER_INVALID)) {

                            DebugTrace((  0, Dbg, "UdfLoadSparingTables, sparing table %u didn't verify regid!\n",
                                                  SparingTable ));
                            break;
                        }

                        //
                        //  Calculate the total number bytes this map spans and check it against what
                        //  we were told the sparing table sizes are.
                        //

                        DebugTrace(( 0, Dbg, "UdfLoadSparingTables, Sparing table %u has %u entries\n",
                                             SparingTable,
                                             Header->TableEntries ));

                        TotalBytes = sizeof(SPARING_TABLE_HEADER) + Header->TableEntries * sizeof(SPARING_TABLE_ENTRY);

                        if (Map->TableSize < TotalBytes) {

                            DebugTrace((  0, Dbg, "UdfLoadSparingTables, sparing table #ents %u overflows allocation!\n",
                                                  Header->TableEntries ));
                            break;
                        }

                        //
                        //  So far so good, advance past the header.
                        //

                        ByteOffset = sizeof(SPARING_TABLE_HEADER);
                        Entry = Add2Ptr( SectorBuffer, sizeof(SPARING_TABLE_HEADER), PSPARING_TABLE_ENTRY );

                    } else {

                        //
                        //  Pick up in the new sector.
                        //

                        Entry = (PSPARING_TABLE_ENTRY) SectorBuffer;
                    }

                    RemainingBytes = Min( SectorSize( Vcb ), TotalBytes - ByteOffset );
                }

                //
                //  Add the mapping.  Since sparing tables are an Lbn->Psn mapping,
                //  very odd, and I want to simplify things by putting the sparing
                //  in right at IO dispatch, translate this to a Psn->Psn mapping.
                //

                if (Entry->Original != UDF_SPARING_AVALIABLE &&
                    Entry->Original != UDF_SPARING_DEFECTIVE) {

                    Psn = Partition->Physical.Start + SectorsFromBlocks( Vcb, Entry->Original );

                    DebugTrace((  0, Dbg, "UdfLoadSparingTables, mapping from Psn %x (Lbn %x) -> Psn %x\n",
                                          Psn,
                                          Entry->Original,
                                          Entry->Mapped ));

                    FsRtlAddLargeMcbEntry( Pcb->SparingMcb,
                                           Psn,
                                           Entry->Mapped,
                                           UDF_SPARING_PACKET_LENGTH );
                }

                //
                //  Advance to the next, and drop out if we've hit the end.
                //

                ByteOffset += sizeof(SPARING_TABLE_ENTRY);
                RemainingBytes -= sizeof(SPARING_TABLE_ENTRY);
                Entry++;

            } while ( ByteOffset < TotalBytes );
        }

    } finally {

        DebugUnwind( UdfLoadSparingTables );

        UdfFreePool( &SectorBuffer );
    }

    DebugTrace(( -1, Dbg, "UdfLoadSparingTables -> STATUS_SUCCESS\n" ));

    return STATUS_SUCCESS;
}
