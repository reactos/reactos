/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    UdfData.h

Abstract:

    This module declares the global data used by the Udfs file system.

Author:

    Dan Lovinger    [DanLo]   20-May-1996

Revision History:

--*/

#ifndef _UDFDATA_
#define _UDFDATA_

//
//  Global data structures
//

extern UDF_DATA UdfData;
extern FAST_IO_DISPATCH UdfFastIoDispatch;

//
//  Global constants
//

//
//  These are the number of times a mounted Vcb will be referenced on behalf
//  of the system.  The counts include the following references.
//
//      1 reference - shows the volume is mounted
//      1 reference - 1 for VolumeDasdFcb.
//      2 references - 1 for RootIndexFcb, 1 for internal stream.
//      2 references - 1 for MetadataFcb, 1 for internal stream.
//
//  AND THEN, IF THIS IS CD-UDF
//
//      2 references - 1 for the VatFcb, 1 for the internal stream.
//
//  For user references we add one for the reference in each of the internal
//  Fcbs.
//

#define UDFS_BASE_RESIDUAL_REFERENCE                (6)
#define UDFS_BASE_RESIDUAL_USER_REFERENCE           (3)

#define UDFS_CDUDF_RESIDUAL_REFERENCE               (2)
#define UDFS_CDUDF_RESIDUAL_USER_REFERENCE          (1)

//
//  The UDFS signature for thread contexts
//

#define UDFS_SIGNATURE                              0x53464455 

//
//  Reserved directory strings
//

#define SELF_ENTRY   0
#define PARENT_ENTRY 1

extern WCHAR UdfUnicodeSelfArray[];
extern WCHAR UdfUnicodeParentArray[];

extern UNICODE_STRING UdfUnicodeDirectoryNames[];

//
//  Static Identifier strings
//

extern STRING UdfCS0Identifier;
extern STRING UdfDomainIdentifier;
extern STRING UdfVirtualPartitionDomainIdentifier;
extern STRING UdfVatTableIdentifier;
extern STRING UdfSparablePartitionDomainIdentifier;
extern STRING UdfSparingTableIdentifier;
extern STRING UdfNSR02Identifier;

//
//  Lookup tables for rudimentary parsing of strings we will
//  discover in on-disk structures
//

extern PARSE_KEYVALUE VsdIdentParseTable[];
extern PARSE_KEYVALUE NsrPartContIdParseTable[];

//
//  Lookaside lists
//

extern NPAGED_LOOKASIDE_LIST UdfFcbNonPagedLookasideList;
extern NPAGED_LOOKASIDE_LIST UdfIrpContextLookasideList;

extern PAGED_LOOKASIDE_LIST UdfCcbLookasideList;
extern PAGED_LOOKASIDE_LIST UdfFcbIndexLookasideList;
extern PAGED_LOOKASIDE_LIST UdfFcbDataLookasideList;
extern PAGED_LOOKASIDE_LIST UdfLcbLookasideList;

//
//  16bit CRC table
//

extern PUSHORT UdfCrcTable;

//
//  Turn on pseudo-asserts if UDFS_FREE_ASSERTS is defined.
//

#if (!DBG && defined( UDFS_FREE_ASSERTS )) || defined( UDFSDBG )
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)                                             \
    ((exp) ? TRUE :                                             \
             (DbgPrint( "%s:%d %s\n",__FILE__,__LINE__,#exp ),  \
              DbgBreakPoint(),                                  \
              TRUE))
#define ASSERTMSG(msg,exp)                                              \
    ((exp) ? TRUE :                                                     \
             (DbgPrint( "%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp ),   \
              DbgBreakPoint(),                                          \
              TRUE))
#endif


//
//  McDebugging Stuff
//

//
//  The following assertion macros ensure that the indicated structure
//  is valid
//
//      ASSERT_STRUCT( IN PVOID Struct, IN CSHORT NodeType );
//      ASSERT_OPTIONAL_STRUCT( IN PVOID Struct OPTIONAL, IN CSHORT NodeType );
//
//      ASSERT_VCB( IN PVCB Vcb );
//      ASSERT_OPTIONAL_VCB( IN PVCB Vcb OPTIONAL );
//
//      ASSERT_FCB( IN PFCB Fcb );
//      ASSERT_OPTIONAL_FCB( IN PFCB Fcb OPTIONAL );
//
//      ASSERT_LCB( IN PLCB Lcb );
//      ASSERT_OPTIONAL_LCB( IN PLCB Lcb OPTIONAL );
//
//      ASSERT_PCB( IN PFCB Pcb );
//      ASSERT_OPTIONAL_PCB( IN PPCB Pcb OPTIONAL );
//
//      ASSERT_FCB_NONPAGED( IN PFCB_NONPAGED FcbNonpaged );
//      ASSERT_OPTIONAL_FCB( IN PFCB_NONPAGED FcbNonpaged OPTIONAL );
//
//      ASSERT_CCB( IN PSCB Ccb );
//      ASSERT_OPTIONAL_CCB( IN PSCB Ccb OPTIONAL );
//
//      ASSERT_IRP_CONTEXT( IN PIRP_CONTEXT IrpContext );
//      ASSERT_OPTIONAL_IRP_CONTEXT( IN PIRP_CONTEXT IrpContext OPTIONAL );
//
//      ASSERT_IRP( IN PIRP Irp );
//      ASSERT_OPTIONAL_IRP( IN PIRP Irp OPTIONAL );
//
//      ASSERT_FILE_OBJECT( IN PFILE_OBJECT FileObject );
//      ASSERT_OPTIONAL_FILE_OBJECT( IN PFILE_OBJECT FileObject OPTIONAL );
//
//  The following macros are used to check the current thread owns
//  the indicated resource
//
//      ASSERT_EXCLUSIVE_RESOURCE( IN PERESOURCE Resource );
//
//      ASSERT_SHARED_RESOURCE( IN PERESOURCE Resource );
//
//      ASSERT_RESOURCE_NOT_MINE( IN PERESOURCE Resource );
//
//  The following macros are used to check whether the current thread
//  owns the resoures in the given structures.
//
//      ASSERT_EXCLUSIVE_CDDATA
//
//      ASSERT_EXCLUSIVE_VCB( IN PVCB Vcb );
//
//      ASSERT_SHARED_VCB( IN PVCB Vcb );
//
//      ASSERT_EXCLUSIVE_FCB( IN PFCB Fcb );
//
//      ASSERT_SHARED_FCB( IN PFCB Fcb );
//
//      ASSERT_EXCLUSIVE_FILE( IN PFCB Fcb );
//
//      ASSERT_SHARED_FILE( IN PFCB Fcb );
//
//      ASSERT_LOCKED_VCB( IN PVCB Vcb );
//
//      ASSERT_NOT_LOCKED_VCB( IN PVCB Vcb );
//
//      ASSERT_LOCKED_FCB( IN PFCB Fcb );
//
//      ASSERT_NOT_LOCKED_FCB( IN PFCB Fcb );
//

//
//  Turn on the sanity checks if this is DBG or UDF_FREE_ASSERTS
//

#if DBG || UDF_FREE_ASSERTS
#undef UDF_SANITY
#define UDF_SANITY
#endif

#ifdef UDF_SANITY

extern LONG UdfDebugTraceLevel;
extern LONG UdfDebugTraceIndent;
extern BOOLEAN UdfNoisyVerifyDescriptor;
extern BOOLEAN UdfTestRaisedStatus;

BOOLEAN
UdfDebugTrace (
    LONG IndentIncrement,
    ULONG TraceMask,
    PCHAR Format,
    ...
    );

#define DebugTrace(x) UdfDebugTrace x

#define DebugUnwind(X) {                                                            \
    if (AbnormalTermination()) {                                                    \
        DebugTrace(( -1, UDFS_DEBUG_LEVEL_UNWIND, #X ", Abnormal termination.\n" )); \
    }                                                                               \
}

#define DebugBreakOnStatus(S) {                                                       \
    if (UdfTestRaisedStatus) {                                                        \
        if ((S) == STATUS_DISK_CORRUPT_ERROR ||                                       \
            (S) == STATUS_FILE_CORRUPT_ERROR ||                                       \
            (S) == STATUS_CRC_ERROR) {                                                \
            DbgPrint( "UDFS: Breaking on interesting raised status (%08x)\n", (S) );  \
            DbgPrint( "UDFS: Set UdfTestRaisedStatus @ %08x to 0 to disable\n",       \
                      &UdfTestRaisedStatus );                                         \
            DbgBreakPoint();                                                          \
        }                                                                             \
    }                                                                                 \
}    

#define ASSERT_STRUCT(S,T)                       ASSERT( SafeNodeType( S ) == (T) )
#define ASSERT_OPTIONAL_STRUCT(S,T)              ASSERT( ((S) == NULL) ||  (SafeNodeType( S ) == (T)) )

#define ASSERT_VCB(V)                            ASSERT_STRUCT( (V), UDFS_NTC_VCB )
#define ASSERT_OPTIONAL_VCB(V)                   ASSERT_OPTIONAL_STRUCT( (V), UDFS_NTC_VCB )

#define ASSERT_FCB(F)                                           \
    ASSERT( (SafeNodeType( F ) == UDFS_NTC_FCB_DATA ) ||        \
            (SafeNodeType( F ) == UDFS_NTC_FCB_INDEX ) )

#define ASSERT_OPTIONAL_FCB(F)                                  \
    ASSERT( ((F) == NULL) ||                                    \
            (SafeNodeType( F ) == UDFS_NTC_FCB_DATA ) ||        \
            (SafeNodeType( F ) == UDFS_NTC_FCB_INDEX ) )

#define ASSERT_FCB_DATA(F)                       ASSERT( (SafeNodeType( F ) == UDFS_NTC_FCB_DATA ) )

#define ASSERT_OPTIONAL_FCB_DATA(F)                             \
    ASSERT( ((F) == NULL) ||                                    \
            (SafeNodeType( F ) == UDFS_NTC_FCB_DATA ) )

#define ASSERT_FCB_INDEX(F)                      ASSERT( (SafeNodeType( F ) == UDFS_NTC_FCB_INDEX ) )

#define ASSERT_OPTIONAL_FCB_INDEX(F)                            \
    ASSERT( ((F) == NULL) ||                                    \
            (SafeNodeType( F ) == UDFS_NTC_FCB_INDEX ) )

#define ASSERT_FCB_NONPAGED(FN)                  ASSERT_STRUCT( (FN), UDFS_NTC_FCB_NONPAGED )
#define ASSERT_OPTIONAL_FCB_NONPAGED(FN)         ASSERT_OPTIONAL_STRUCT( (FN), UDFS_NTC_FCB_NONPAGED )

#define ASSERT_CCB(C)                            ASSERT_STRUCT( (C), UDFS_NTC_CCB )
#define ASSERT_OPTIONAL_CCB(C)                   ASSERT_OPTIONAL_STRUCT( (C), UDFS_NTC_CCB )

#define ASSERT_PCB(C)                            ASSERT_STRUCT( (C), UDFS_NTC_PCB )
#define ASSERT_OPTIONAL_PCB(C)                   ASSERT_OPTIONAL_STRUCT( (C), UDFS_NTC_PCB )

#define ASSERT_LCB(C)                            ASSERT_STRUCT( (C), UDFS_NTC_LCB )
#define ASSERT_OPTIONAL_LCB(C)                   ASSERT_OPTIONAL_STRUCT( (C), UDFS_NTC_LCB )

#define ASSERT_IRP_CONTEXT(IC)                   ASSERT_STRUCT( (IC), UDFS_NTC_IRP_CONTEXT )
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC)          ASSERT_OPTIONAL_STRUCT( (IC), UDFS_NTC_IRP_CONTEXT )

#define ASSERT_IRP_CONTEXT_LITE(IC)              ASSERT_STRUCT( (IC), UDFS_NTC_IRP_CONTEXT_LITE )
#define ASSERT_OPTIONAL_IRP_CONTEXT_LITE(IC)     ASSERT_OPTIONAL_STRUCT( (IC), UDFS_NTC_IRP_CONTEXT_LITE )

#define ASSERT_IRP(I)                            ASSERT_STRUCT( (I), IO_TYPE_IRP )
#define ASSERT_OPTIONAL_IRP(I)                   ASSERT_OPTIONAL_STRUCT( (I), IO_TYPE_IRP )

#define ASSERT_FILE_OBJECT(FO)                   ASSERT_STRUCT( (FO), IO_TYPE_FILE )
#define ASSERT_OPTIONAL_FILE_OBJECT(FO)          ASSERT_OPTIONAL_STRUCT( (FO), IO_TYPE_FILE )

#define ASSERT_EXCLUSIVE_RESOURCE(R)             ASSERT( ExIsResourceAcquiredExclusive( R ))

#define ASSERT_SHARED_RESOURCE(R)                ASSERT( ExIsResourceAcquiredShared( R ))

#define ASSERT_RESOURCE_NOT_MINE(R)              ASSERT( !ExIsResourceAcquiredShared( R ))

#define ASSERT_EXCLUSIVE_UDFDATA                 ASSERT( ExIsResourceAcquiredExclusive( &UdfData.DataResource ))
#define ASSERT_EXCLUSIVE_VCB(V)                  ASSERT( ExIsResourceAcquiredExclusive( &(V)->VcbResource ))
#define ASSERT_SHARED_VCB(V)                     ASSERT( ExIsResourceAcquiredShared( &(V)->VcbResource ))

#define ASSERT_EXCLUSIVE_FCB_OR_VCB(F)           ASSERT( ExIsResourceAcquiredExclusive( &(F)->FcbNonpaged->FcbResource ) || \
                                                         ExIsResourceAcquiredExclusive( &(F)->Vcb->VcbResource ))

#define ASSERT_EXCLUSIVE_FCB(F)                  ASSERT( ExIsResourceAcquiredExclusive( &(F)->FcbNonpaged->FcbResource ))
#define ASSERT_SHARED_FCB(F)                     ASSERT( ExIsResourceAcquiredShared( &(F)->FcbNonpaged->FcbResource ))

#define ASSERT_EXCLUSIVE_FILE(F)                 ASSERT( ExIsResourceAcquiredExclusive( (F)->Resource ))
#define ASSERT_SHARED_FILE(F)                    ASSERT( ExIsResourceAcquiredShared( (F)->Resource ))

#define ASSERT_LOCKED_VCB(V)                     ASSERT( (V)->VcbLockThread == PsGetCurrentThread() )
#define ASSERT_NOT_LOCKED_VCB(V)                 ASSERT( (V)->VcbLockThread != PsGetCurrentThread() )

#define ASSERT_LOCKED_FCB(F)                     ASSERT( (F)->FcbLockThread == PsGetCurrentThread() )
#define ASSERT_NOT_LOCKED_FCB(F)                 ASSERT( (F)->FcbLockThread != PsGetCurrentThread() )

#else

#define DebugTrace(X)                            TRUE
#define DebugUnwind(X)                           { NOTHING; }
#define DebugBreakOnStatus(S)                    { NOTHING; }

#define ASSERT_STRUCT(S,T)                       { NOTHING; }
#define ASSERT_OPTIONAL_STRUCT(S,T)              { NOTHING; }
#define ASSERT_VCB(V)                            { NOTHING; }
#define ASSERT_OPTIONAL_VCB(V)                   { NOTHING; }
#define ASSERT_FCB(F)                            { NOTHING; }
#define ASSERT_OPTIONAL_FCB(F)                   { NOTHING; }
#define ASSERT_FCB_DATA                          { NOTHING; }
#define ASSERT_OPTIONAL_FCB_DATA(F)              { NOTHING; }
#define ASSERT_FCB_INDEX(F)                      { NOTHING; }
#define ASSERT_OPTIONAL_FCB_INDEX(F)             { NOTHING; }
#define ASSERT_FCB_NONPAGED(FN)                  { NOTHING; }
#define ASSERT_OPTIONAL_FCB_NONPAGED(FN)         { NOTHING; }
#define ASSERT_CCB(C)                            { NOTHING; }
#define ASSERT_OPTIONAL_CCB(C)                   { NOTHING; }
#define ASSERT_PCB(C)                            { NOTHING; }
#define ASSERT_OPTIONAL_PCB(C)                   { NOTHING; }
#define ASSERT_LCB(C)                            { NOTHING; }
#define ASSERT_OPTIONAL_LCB(C)                   { NOTHING; }
#define ASSERT_IRP_CONTEXT(IC)                   { NOTHING; }
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC)          { NOTHING; }
#define ASSERT_IRP_CONTEXT_LITE(IC)              { NOTHING; }
#define ASSERT_OPTIONAL_IRP_CONTEXT_LITE(IC)     { NOTHING; }
#define ASSERT_IRP(I)                            { NOTHING; }
#define ASSERT_OPTIONAL_IRP(I)                   { NOTHING; }
#define ASSERT_FILE_OBJECT(FO)                   { NOTHING; }
#define ASSERT_OPTIONAL_FILE_OBJECT(FO)          { NOTHING; }
#define ASSERT_EXCLUSIVE_RESOURCE(R)             { NOTHING; }
#define ASSERT_SHARED_RESOURCE(R)                { NOTHING; }
#define ASSERT_RESOURCE_NOT_MINE(R)              { NOTHING; }
#define ASSERT_EXCLUSIVE_UDFDATA                 { NOTHING; }
#define ASSERT_EXCLUSIVE_VCB(V)                  { NOTHING; }
#define ASSERT_SHARED_VCB(V)                     { NOTHING; }
#define ASSERT_EXCLUSIVE_FCB_OR_VCB(F)           { NOTHING; }
#define ASSERT_EXCLUSIVE_FCB(F)                  { NOTHING; }
#define ASSERT_SHARED_FCB(F)                     { NOTHING; }
#define ASSERT_EXCLUSIVE_FILE(F)                 { NOTHING; }
#define ASSERT_SHARED_FILE(F)                    { NOTHING; }
#define ASSERT_LOCKED_VCB(V)                     { NOTHING; }
#define ASSERT_NOT_LOCKED_VCB(V)                 { NOTHING; }
#define ASSERT_LOCKED_FCB(F)                     { NOTHING; }
#define ASSERT_NOT_LOCKED_FCB(F)                 { NOTHING; }

#endif

#endif // _UDFDATA_

