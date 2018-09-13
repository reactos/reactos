/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    DirSup.c

Abstract:

    This module implements the support for walking across on-disk directory
    structures.

Author:

    Dan Lovinger    [DanLo]   11-Jun-1996

Revision History:

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_DIRSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_DIRSUP)

//
//  Local support routines.
//

BOOLEAN
UdfLookupDirEntryPostProcessing (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN ReturnError
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UdfCleanupDirContext)
#pragma alloc_text(PAGE, UdfFindDirEntry)
#pragma alloc_text(PAGE, UdfInitializeDirContext)
#pragma alloc_text(PAGE, UdfLookupDirEntryPostProcessing)
#pragma alloc_text(PAGE, UdfLookupInitialDirEntry)
#pragma alloc_text(PAGE, UdfLookupNextDirEntry)
#pragma alloc_text(PAGE, UdfUpdateDirNames)
#endif


VOID
UdfInitializeDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PDIR_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine initializes a directory enumeartion context.
    
    Call this exactly once in the lifetime of a context.

Arguments:

    DirContext - a context to initialize

Return Value:

    None.

--*/

{
    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    //
    //  Provide defaults for fields, nothing too special.
    //

    RtlZeroMemory( DirContext, sizeof(DIR_ENUM_CONTEXT) );
}


VOID
UdfCleanupDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PDIR_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine cleans up a directory enumeration context for reuse.

Arguments:

    DirContext - a context to clean.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    
    //
    //  Dump the allocation we store the triple of names in.
    //

    UdfFreePool( &DirContext->NameBuffer );

    //
    //  And the short name.
    //

    UdfFreePool( &DirContext->ShortObjectName.Buffer );

    //
    //  Unpin the view.
    //

    UdfUnpinData( IrpContext, &DirContext->Bcb );

    //
    //  Free a buffered Fid that may remain.
    //
    
    if (FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_FID_BUFFERED )) {

        UdfFreePool( &DirContext->Fid );
    }
    
    //
    //  Zero everything else out.
    //

    RtlZeroMemory( DirContext, sizeof( DIR_ENUM_CONTEXT ) );
}


BOOLEAN
UdfLookupInitialDirEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN PLONGLONG InitialOffset OPTIONAL
    )

/*++

Routine Description:

    This routine begins the enumeration of a directory by setting the context
    at the first avaliable directory entry.

Arguments:

    Fcb - the directory being enumerated.
    
    DirContext - a corresponding context for the enumeration.
    
    InitialOffset - an optional starting byte offset to base the enumeration.

Return Value:

    If InitialOffset is unspecified, TRUE will always be returned.  Failure will result
    in a raised status indicating corruption.
    
    If InitialOffset is specified, TRUE will be returned if a valid entry is found at this
    offset, FALSE otherwise.

--*/

{
    BOOLEAN Result;

    PAGED_CODE();
    
    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB_INDEX( Fcb );
    
    //
    //  Create the internal stream if it isn't already in place.
    //

    if (Fcb->FileObject == NULL) {

        UdfCreateInternalStream( IrpContext, Fcb->Vcb, Fcb );
    }

    //
    //  Reset the flags.
    //

    DirContext->Flags = 0;
    
    if (InitialOffset) {

        //
        //  If we are beginning in the middle of the stream, adjust the sanity check flags.
        //
        
        if (*InitialOffset != 0) {

            DirContext->Flags = DIR_CONTEXT_FLAG_SEEN_NONCONSTANT | DIR_CONTEXT_FLAG_SEEN_PARENT;
        }

        //
        //  Now set up the range we will map.  This is constrained by the size of a cache view.
        //
        
        DirContext->BaseOffset.QuadPart = GenericTruncate( *InitialOffset, VACB_MAPPING_GRANULARITY );
        DirContext->ViewOffset = (ULONG) GenericOffset( *InitialOffset, VACB_MAPPING_GRANULARITY );

    } else {
        
        //
        //  Map at the beginning.
        //
    
        DirContext->BaseOffset.QuadPart = 0;
        DirContext->ViewOffset = 0;
    }

    //
    //  Contain the view length by the size of the stream and map.
    //

    DirContext->ViewLength = VACB_MAPPING_GRANULARITY;

    if (DirContext->BaseOffset.QuadPart + DirContext->ViewLength > Fcb->FileSize.QuadPart) {

        DirContext->ViewLength = (ULONG) (Fcb->FileSize.QuadPart - DirContext->BaseOffset.QuadPart);
    }
    
    UdfUnpinData( IrpContext, &DirContext->Bcb );
    
    CcMapData( Fcb->FileObject,
               &DirContext->BaseOffset,
               DirContext->ViewLength,
               TRUE,
               &DirContext->Bcb,
               &DirContext->View );

    DirContext->Fid = Add2Ptr( DirContext->View, DirContext->ViewOffset, PNSR_FID );

    //
    //  The state of the context is now valid.  Tail off into our common post-processor
    //  to finish the work.
    //

    return UdfLookupDirEntryPostProcessing( IrpContext,
                                            Fcb,
                                            DirContext,
                                            (BOOLEAN) (InitialOffset != NULL));
}


BOOLEAN
UdfLookupNextDirEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIR_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine advances the enumeration of a directory by one entry.

Arguments:

    Fcb - the directory being enumerated.
    
    DirContext - a corresponding context for the enumeration.

Return Value:

    BOOLEAN True if another Fid is avaliable, False if we are at the end.

--*/

{
    PAGED_CODE();
    
    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB_INDEX( Fcb );

    //
    //  If we have reached the end, stop.
    //
    
    if (DirContext->BaseOffset.QuadPart + DirContext->NextFidOffset == Fcb->FileSize.QuadPart) {

        return FALSE;
    }

    //
    //  If the previous Fid was buffered, dismantle it now.
    //
    
    if (FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_FID_BUFFERED )) {

        ClearFlag( DirContext->Flags, DIR_CONTEXT_FLAG_FID_BUFFERED );
        UdfFreePool( &DirContext->Fid );
    }
    
    //
    //  Move the pointers based on the knowledge generated in the previous iteration.
    //

    DirContext->ViewOffset = DirContext->NextFidOffset;
    DirContext->Fid = Add2Ptr( DirContext->View, DirContext->ViewOffset, PNSR_FID );

    //
    //  The state of the context is now valid.  Tail off into our common post-processor
    //  to finish the work.
    //

    return UdfLookupDirEntryPostProcessing( IrpContext,
                                            Fcb,
                                            DirContext,
                                            FALSE );
}


VOID
UdfUpdateDirNames (
    IN PIRP_CONTEXT IrpContext,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine fills in the non-short names of a directory enumeration context
    for the Fid currently referenced.

Arguments:

    DirContext - a corresponding context to fill in.
    
    IgnoreCase - whether the caller wants to be insensitive to case.

Return Value:

    None.
    
--*/

{
    PUCHAR NameDstring;
    BOOLEAN ContainsIllegal;
    
    USHORT NameLength;
    USHORT BufferLength;
    USHORT PresentLength;
     
    PAGED_CODE();

    //
    //  Check input.
    //

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace(( +1, Dbg, "UdfUpdateDirNames\n" ));

    //
    //  Handle the case of the self directory entry.
    //

    if (DirContext->Fid == NULL) {

        //
        //  Simply synthesize
        //
        
        //
        //  It doesn't hurt to be pedantic about initialization, so do it all.
        //
        
        DirContext->PureObjectName.Length =
        DirContext->CaseObjectName.Length =
        DirContext->ObjectName.Length = UdfUnicodeDirectoryNames[SELF_ENTRY].Length;
        
        DirContext->PureObjectName.MaximumLength =
        DirContext->CaseObjectName.MaximumLength =
        DirContext->ObjectName.MaximumLength = UdfUnicodeDirectoryNames[SELF_ENTRY].MaximumLength;

        DirContext->PureObjectName.Buffer = 
        DirContext->CaseObjectName.Buffer = 
        DirContext->ObjectName.Buffer = UdfUnicodeDirectoryNames[SELF_ENTRY].Buffer;

        //
        //  All done.
        //

        DebugTrace((  0, Dbg, "Self Entry case\n" ));
        DebugTrace(( -1, Dbg, "UdfUpdateDirNames -> VOID\n" ));
        
        return;
    }
    
    //
    //  Handle the case of the parent directory entry.
    //

    if (FlagOn( DirContext->Fid->Flags, NSR_FID_F_PARENT )) {

        //
        //  Parent entries must occur at the front of the directory and
        //  have a fid length of zero (13346 4/14.4.4).
        //

        if (FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_NONCONSTANT ) ||
            DirContext->Fid->FileIDLen != 0) {

            UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        //
        //  Note that we have seen the parent entry.
        //

        SetFlag( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_PARENT );
        
        //
        //  It doesn't hurt to be pedantic about initialization, so do it all.
        //
        
        DirContext->PureObjectName.Length =
        DirContext->CaseObjectName.Length =
        DirContext->ObjectName.Length = UdfUnicodeDirectoryNames[PARENT_ENTRY].Length;
        
        DirContext->PureObjectName.MaximumLength =
        DirContext->CaseObjectName.MaximumLength =
        DirContext->ObjectName.MaximumLength = UdfUnicodeDirectoryNames[PARENT_ENTRY].MaximumLength;

        DirContext->PureObjectName.Buffer = 
        DirContext->CaseObjectName.Buffer = 
        DirContext->ObjectName.Buffer = UdfUnicodeDirectoryNames[PARENT_ENTRY].Buffer;

        //
        //  All done.
        //

        DebugTrace((  0, Dbg, "Parent Entry case\n" ));
        DebugTrace(( -1, Dbg, "UdfUpdateDirNames -> VOID\n" ));
        
        return;
    }

    //
    //  We now know that we will need to convert the name in a real FID, so figure out where
    //  it sits in the descriptor.
    //
    
    NameDstring = Add2Ptr( DirContext->Fid, ISONsrFidConstantSize + DirContext->Fid->ImpUseLen, PUCHAR );
     
    //
    //  Every directory must record a parent entry.
    //
    
    if (!FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_PARENT)) {
    
        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }
    
    //
    //  Note that we are proceeding into the non-constant portion of a directory.
    //
    
    SetFlag( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_NONCONSTANT );
    
    //
    //  Make sure the dstring is good CS0
    //
    
    UdfCheckLegalCS0Dstring( IrpContext,
                             NameDstring,
                             DirContext->Fid->FileIDLen,
                             0,
                             FALSE );
    
    //
    //  Don't bother allocating tiny buffers - always make sure we get enough for an 8.3 name.
    //

    BufferLength =
    NameLength = Max( BYTE_COUNT_8_DOT_3, UdfCS0DstringUnicodeSize( IrpContext,
                                                                    NameDstring,
                                                                    DirContext->Fid->FileIDLen) );

    //
    //  Illegality is both actual illegal characters and too many characters.
    //
    
    ContainsIllegal = (!UdfCS0DstringContainsLegalCharacters( NameDstring, DirContext->Fid->FileIDLen ) ||
                       (NameLength / sizeof( WCHAR )) > MAX_LEN);

    
    //
    //  If we're illegal, we will need more characters to hold the uniqifying stamp.
    //
    
    if (ContainsIllegal) {

        BufferLength = (NameLength += (CRC_LEN * sizeof(WCHAR)));
    }
    
    
    //
    //  If we need to build a case insensitive name, need more space.
    //
        
    if (IgnoreCase) {

        BufferLength += NameLength;
    }
    
    //
    //  If we need to render the names due to illegal characters, more space again.
    //
        
    if (ContainsIllegal) {

        BufferLength += NameLength;
    
    } else {

        //
        //  Make sure the names aren't seperated. If more illegal names are found we can
        //  resplit the buffer but until then avoid the expense of having to copy bytes
        //  ... odds are that illegal characters are going to be a rarish occurance.
        //
        
        DirContext->PureObjectName.Buffer = DirContext->ObjectName.Buffer;
    }

    DebugTrace(( 0, Dbg,
                 "Ob %s%sneeds %d bytes (%d byte chunks), have %d\n",
                 (IgnoreCase? "Ic " : ""),
                 (ContainsIllegal? "Ci " : ""),
                 BufferLength,
                 NameLength,
                 DirContext->AllocLength ));

    //
    //  Check if we need more space for the names.  We will need more if the name size is greater
    //  than the maximum we can currently store, or if we have stumbled across illegal characters
    //  and the current Pure name is not seperated from the exposed Object name.
    //
    //  Note that IgnoreCase remains constant across usage of a context so we don't have to wonder
    //  if it has been seperated from the ObjectName - it'll always be correct.
    //
    
    if ((NameLength > DirContext->ObjectName.MaximumLength) ||
        (ContainsIllegal && DirContext->ObjectName.Buffer == DirContext->PureObjectName.Buffer)) {

        DebugTrace(( 0, Dbg, "Resizing buffers\n" ));
        
        //
        //  For some reason the sizing is not good for the current name we have to unroll.  Figure
        //  out if we can break up the current allocation in a different way before falling back
        //  to a new allocation.
        //

        if (DirContext->AllocLength >= BufferLength) {

            //
            //  So we can still use the current allocation.  Chop it up into the required number
            //  of chunks.
            //

            DirContext->PureObjectName.MaximumLength =
            DirContext->CaseObjectName.MaximumLength =
            DirContext->ObjectName.MaximumLength = DirContext->AllocLength / (1 +
                                                                              (IgnoreCase? 1 : 0) +
                                                                              (ContainsIllegal? 1 : 0));

            DebugTrace(( 0, Dbg, 
                         "... by resplit into %d byte chunks\n",
                         DirContext->ObjectName.MaximumLength ));
            
            //
            //  Set the buffer pointers up.  Required adjustment will occur below.
            //
                
            DirContext->PureObjectName.Buffer = 
            DirContext->CaseObjectName.Buffer = 
            DirContext->ObjectName.Buffer = DirContext->NameBuffer;
        
        } else {

            DebugTrace(( 0, Dbg, "... by allocating new pool\n" ));
            
            //
            //  Oh well, no choice but to fall back into the pool.  Drop our previous hunk.
            //
            
            UdfFreePool( &DirContext->NameBuffer );
            DirContext->AllocLength = 0;
            
            //
            //  The names share an allocation for efficiency.
            //
            
            DirContext->PureObjectName.MaximumLength =
            DirContext->CaseObjectName.MaximumLength =
            DirContext->ObjectName.MaximumLength = NameLength;
    
            DirContext->NameBuffer =
            DirContext->PureObjectName.Buffer = 
            DirContext->CaseObjectName.Buffer = 
            DirContext->ObjectName.Buffer = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                                      BufferLength,
                                                                      TAG_FILE_NAME );

            DirContext->AllocLength = BufferLength;
        }
        
        //
        //  In the presence of the "as appropriate" names, adjust the buffer locations.  Note
        //  that ObjectName.Buffer is always the base of the allocated space.
        //
        
        if (IgnoreCase) {

            DirContext->CaseObjectName.Buffer = Add2Ptr( DirContext->ObjectName.Buffer, 
                                                         DirContext->ObjectName.MaximumLength,
                                                         PWCHAR );
        }

        if (ContainsIllegal) {
            
            DirContext->PureObjectName.Buffer = Add2Ptr( DirContext->CaseObjectName.Buffer,
                                                         DirContext->CaseObjectName.MaximumLength,
                                                         PWCHAR );
        }
    }

    ASSERT( BufferLength <= DirContext->AllocLength );

    //
    //  Convert the dstring.
    //
    
    UdfConvertCS0DstringToUnicode( IrpContext,
                                   NameDstring,
                                   DirContext->Fid->FileIDLen,
                                   0,
                                   &DirContext->PureObjectName );

    //
    //  If illegal characters were present, run the name through the UDF transmogrifier.
    //

    if (ContainsIllegal) {

        UdfRenderNameToLegalUnicode( IrpContext,
                                     &DirContext->PureObjectName,
                                     &DirContext->ObjectName );

    //
    //  The ObjectName is the same as the PureObjectName.
    //

    } else {

        DirContext->ObjectName.Length = DirContext->PureObjectName.Length;
    }

    //
    //  Upcase the result if required.
    //

    if (IgnoreCase) {

        UdfUpcaseName( IrpContext,
                       &DirContext->ObjectName,
                       &DirContext->CaseObjectName );
    }

    DebugTrace(( -1, Dbg, "UdfUpdateDirNames -> VOID\n" ));
    
    return;
}


BOOLEAN
UdfFindDirEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortName,
    IN PDIR_ENUM_CONTEXT DirContext
    )

/*++

Routine Description:

    This routine walks the directory specified for an entry which matches the input
    criteria.

Arguments:

    Fcb - the directory to search
    
    Name - name to search for
    
    IgnoreCase - whether this search should be case-insensitive (Name will already
        be upcased)
        
    ShortName - whether the name should be searched for according to short name rules
    
    DirContext - context structure to use and return results in

Return Value:

    BOOLEAN True if a matching directory entry is being returned, False otherwise.

--*/

{
    PUNICODE_STRING MatchName;

    PAGED_CODE();

    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB_INDEX( Fcb );

    DebugTrace(( +1, Dbg,
                 "UdfFindDirEntry, Fcb=%08x Name=\"%wZ\" Ignore=%u Short=%u, DC=%08x\n",
                 Fcb,
                 Name,
                 IgnoreCase,
                 ShortName,
                 DirContext ));

    //
    //  Depending on the kind of search we are performing a different flavor of the found name
    //  wil be used in the comparison.
    //
    
    if (ShortName) {

        MatchName = &DirContext->ShortObjectName;
    
    } else {

        MatchName = &DirContext->CaseObjectName;
    }


    //
    //  Go get the first entry.
    //

    UdfLookupInitialDirEntry( IrpContext,
                              Fcb,
                              DirContext,
                              NULL );

    //
    //  Now loop looking for a good match.
    //
    
    do {

        //
        //  If it is deleted, we obviously aren't interested in it.
        //
        
        if (FlagOn( DirContext->Fid->Flags, NSR_FID_F_DELETED )) {

            continue;
        }

        UdfUpdateDirNames( IrpContext,
                           DirContext,
                           IgnoreCase );
            
        
        //
        //  If this is a constant entry, just keep going.
        //
        
        if (!FlagOn( DirContext->Flags, DIR_CONTEXT_FLAG_SEEN_NONCONSTANT )) {
            
            continue;
        }

        DebugTrace(( 0, Dbg,
                     "\"%wZ\" (pure \"%wZ\") @ +%08x\n",
                     &DirContext->ObjectName,
                     &DirContext->PureObjectName,
                     DirContext->ViewOffset ));

        //
        //  If we are searching for generated shortnames, a small subset of the names
        //  in the directory are actually candidates for a match.  Go get the name.
        //
        
        if (ShortName) {

            //
            //  Now, only if this Fid's name is non 8.3 is it neccesary to work with it.
            //
            
            if (!UdfIs8dot3Name( IrpContext, DirContext->ObjectName )) {

                //
                //  Allocate the shortname if it isn't already done.
                //
                
                if (DirContext->ShortObjectName.Buffer == NULL) {

                    DirContext->ShortObjectName.Buffer = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                                                   BYTE_COUNT_8_DOT_3,
                                                                                   TAG_SHORT_FILE_NAME );
                    DirContext->ShortObjectName.MaximumLength = BYTE_COUNT_8_DOT_3;
                }

                UdfGenerate8dot3Name( IrpContext,
                                      &DirContext->PureObjectName,
                                      &DirContext->ShortObjectName );

                DebugTrace(( 0, Dbg,
                             "built shortname \"%wZ\"\n", &DirContext->ShortObjectName ));

            } else {

                //
                //  As an 8.3 name already, this name will not have caused us to have to generate
                //  a short name, so it can't be the case that the caller is looking for it.
                //
                
                continue;
            }
        }

        if (UdfFullCompareNames( IrpContext,
                                 MatchName,
                                 Name ) == EqualTo) {

            //
            //  Got a match, so give it up.
            //

            DebugTrace((  0, Dbg, "HIT\n" ));
            DebugTrace(( -1, Dbg, "UdfFindDirEntry -> TRUE\n" ));

            return TRUE;
        }

    } while ( UdfLookupNextDirEntry( IrpContext,
                                     Fcb,
                                     DirContext ));

    //
    //  No match was found.
    //

    DebugTrace(( -1, Dbg, "UdfFindDirEntry -> FALSE\n" ));

    return FALSE;
}


//
//  Local support routine
//

BOOLEAN
UdfLookupDirEntryPostProcessing (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN ReturnError
    )

/*++

Routine Description:

    This routine is the core engine of directory stream enumeration. It receives
    a context which has been advanced and does the integrity checks and final
    extraction of the Fid with respect to file cache granularity restrictions.

    NOTE: we assume that a Fid cannot span a cache view.  The maximum size of a
    Fid is just over 32k, so this is a good and likely permanent assumption.

Arguments:

    Fcb - the directory being enumerated.
    
    DirContext - a corresponding context for the enumeration.
    
    ReturnError - whether errors should be returned (or raised)

Return Value:

    BOOLEAN according to the successful extraction of the Fid.  If ReturnError is
    FALSE, then failure will result in a raised status.

--*/

{
    BOOLEAN Result = TRUE;
    
    PNSR_FID FidBufferC = NULL;
    PNSR_FID FidBuffer = NULL;

    PNSR_FID FidC;
    PNSR_FID Fid;

    ULONG FidBytesInPreviousView = 0;
    
    PAGED_CODE();
    
    //
    //  Check inputs.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB_INDEX( Fcb );
    
    try {
        
        //
        //  First check that the stream can contain another FID.
        //
    
        if (DirContext->BaseOffset.QuadPart +
            DirContext->ViewOffset +
            ISONsrFidConstantSize > Fcb->FileSize.QuadPart) {
    
            DebugTrace(( 0, Dbg,
                         "UdfLookupDirEntryPostProcessing: DC %p, constant header overlaps end of dir\n",
                         DirContext ));

            try_leave( Result = FALSE );
        }
            
        //
        //  We now build up the constant portion of the FID for use.  It may be the case that
        //  this spans a view boundary and must be buffered, or is entirely in the next view
        //  and we simply need to advance.
        //
    
        if (GenericTruncatePtr( Add2Ptr( DirContext->Fid, ISONsrFidConstantSize - 1, PUCHAR ), VACB_MAPPING_GRANULARITY ) !=
            DirContext->View) {
            
            FidBytesInPreviousView = GenericRemainderPtr( DirContext->Fid, VACB_MAPPING_GRANULARITY );
            
            //
            //  Only buffer if there are really bytes in the previous view.
            //
            
            if (FidBytesInPreviousView) {
                
                FidC =
                FidBufferC = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                       ISONsrFidConstantSize,
                                                       TAG_FID_BUFFER );
        
                RtlCopyMemory( FidBufferC,
                               DirContext->Fid,
                               FidBytesInPreviousView );
            }
    
            //
            //  Now advance into the next view to pick up the rest.
            //
            
            DirContext->BaseOffset.QuadPart += VACB_MAPPING_GRANULARITY;
            DirContext->ViewOffset = 0;
            
            //
            //  Contain the view length by the size of the stream and map.
            //
        
            DirContext->ViewLength = VACB_MAPPING_GRANULARITY;
        
            if (DirContext->BaseOffset.QuadPart + DirContext->ViewLength > Fcb->FileSize.QuadPart) {
        
                DirContext->ViewLength = (ULONG) (Fcb->FileSize.QuadPart - DirContext->BaseOffset.QuadPart);
            }
            
            UdfUnpinData( IrpContext, &DirContext->Bcb );
            
            CcMapData( Fcb->FileObject,
                       &DirContext->BaseOffset,
                       DirContext->ViewLength,
                       TRUE,
                       &DirContext->Bcb,
                       &DirContext->View );

            //
            //  We are guaranteed that this much lies in the stream.  Build the rest of the
            //  constant header.
            //
    
            if (FidBytesInPreviousView) {
                
                RtlCopyMemory( Add2Ptr( FidBufferC, FidBytesInPreviousView, PUCHAR ),
                               DirContext->View,
                               ISONsrFidConstantSize - FidBytesInPreviousView );
            
            //
            //  In fact, this FID is perfectly aligned to the front of this view.  No buffering
            //  is required, and we just set the FID pointer.
            //

            } else {


                DirContext->Fid = DirContext->View;
            }
        }
         
        //
        //  If no buffering was required, we can use the cache directly.
        //
            
        if (!FidBytesInPreviousView) {
    
            FidC = DirContext->Fid;
        }
    
        //
        //  Now we can check that the Fid data lies within the stream bounds and is sized
        //  within a logical block (per UDF).  This will complete the size-wise integrity
        //  verification.
        //

        if (((DirContext->BaseOffset.QuadPart +
              DirContext->ViewOffset -
              FidBytesInPreviousView +
              ISONsrFidSize( FidC ) > Fcb->FileSize.QuadPart) &&
             DebugTrace(( 0, Dbg,
                          "UdfLookupDirEntryPostProcessing: DC %p, FID (FidC %p, FBIPV %u) overlaps end of dir\n",
                          DirContext,
                          FidC,
                          FidBytesInPreviousView )))
              ||

            (ISONsrFidSize( FidC ) > BlockSize( Fcb->Vcb ) &&
             DebugTrace(( 0, Dbg,
             "UdfLookupDirEntryPostProcessing: DC %p, FID (FidC %p) larger than a logical block\n",
                          DirContext,
                          FidC )))) {

            try_leave( Result = FALSE );

        }

        //
        //  Final Fid rollup.
        //
        
        //
        //  The Fid may span a view boundary and should be buffered.  If we already buffered, we know
        //  we have to do this.
        //

        if (FidBytesInPreviousView ||
            GenericTruncatePtr( Add2Ptr( DirContext->Fid, ISONsrFidSize( FidC ) - 1, PUCHAR ), VACB_MAPPING_GRANULARITY ) !=
            DirContext->View) {
        
            Fid =
            FidBuffer = FsRtlAllocatePoolWithTag( UdfPagedPool,
                                                  ISONsrFidSize( FidC ),
                                                  TAG_FID_BUFFER );

            
            //
            //  If we already buffered and advanced for the header, just prefill
            //  the final Fid buffer with the bytes that are now unavaliable.
            //
            
            if (FidBytesInPreviousView) {

                RtlCopyMemory( FidBuffer,
                               FidBufferC,
                               FidBytesInPreviousView );

            } else {
                
                //
                //  Buffer and advance the view.
                //
                
                FidBytesInPreviousView = GenericRemainderPtr( DirContext->Fid, VACB_MAPPING_GRANULARITY );
                
                RtlCopyMemory( FidBuffer,
                               DirContext->Fid,
                               FidBytesInPreviousView );
                
                //
                //  Now advance into the next view to pick up the rest.
                //
                
                DirContext->BaseOffset.QuadPart += VACB_MAPPING_GRANULARITY;
                DirContext->ViewOffset = 0;
                
                //
                //  Contain the view length by the size of the stream and map.
                //
            
                DirContext->ViewLength = VACB_MAPPING_GRANULARITY;
            
                if (DirContext->BaseOffset.QuadPart + DirContext->ViewLength > Fcb->FileSize.QuadPart) {
            
                    DirContext->ViewLength = (ULONG) (Fcb->FileSize.QuadPart - DirContext->BaseOffset.QuadPart);
                }
                
                UdfUnpinData( IrpContext, &DirContext->Bcb );
                
                CcMapData( Fcb->FileObject,
                           &DirContext->BaseOffset,
                           DirContext->ViewLength,
                           TRUE,
                           &DirContext->Bcb,
                           &DirContext->View );
            }
    
            //
            //  We are guaranteed that this much lies in the stream.
            //
    
            RtlCopyMemory( Add2Ptr( FidBuffer, FidBytesInPreviousView, PUCHAR ),
                           DirContext->View,
                           ISONsrFidSize( FidC ) - FidBytesInPreviousView );
    
        } else {

            Fid = DirContext->Fid;
        }
        
        //
        //  We finally have the whole Fid safely extracted from the cache, so the
        //  integrity check is now the last step before success.  For simplicity's
        //  sake we trust the Lbn field.
        //
    
        Result = UdfVerifyDescriptor( IrpContext,
                                      &Fid->Destag,
                                      DESTAG_ID_NSR_FID,
                                      ISONsrFidSize( Fid ),
                                      Fid->Destag.Lbn,
                                      ReturnError );

        //
        //  Prepare to return a buffered Fid.
        //
        
        if (FidBuffer && Result) {

            SetFlag( DirContext->Flags, DIR_CONTEXT_FLAG_FID_BUFFERED );
            DirContext->Fid = FidBuffer;
            FidBuffer = NULL;
        }
        
    } finally {

        UdfFreePool( &FidBuffer );
        UdfFreePool( &FidBufferC );
    }

    if (!ReturnError && !Result) {

        UdfRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    //
    //  On success update the next Fid information in the context.
    //  Note that  we must drop in a hint as to where the next Fid
    //  will be found so that the next advance will know how much
    //  of a buffered Fid isn't in this view.
    //

    if (Result) {

        DirContext->NextFidOffset = DirContext->ViewOffset +
                                    ISONsrFidSize( Fid );
        
        if (FidBytesInPreviousView) {
            
            DirContext->NextFidOffset -= FidBytesInPreviousView;
        }
    }

    return Result;
}


