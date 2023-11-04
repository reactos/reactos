/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    CdData.c

Abstract:

    This module declares the global data used by the Cdfs file system.


--*/

#ifndef _CDDATA_
#define _CDDATA_

//
//  Global data structures
//

extern CD_DATA CdData;
extern FAST_IO_DISPATCH CdFastIoDispatch;

//
//  Global constants
//

//
//  This is the number of times a mounted Vcb will be referenced on behalf
//  of the system.  The counts include the following references.
//
//      1 reference - shows the volume is mounted
//      1 reference - 1 for VolumeDasdFcb.
//      2 references - 1 for RootIndexFcb, 1 for internal stream.
//      2 references - 1 for PathTableFcb, 1 for internal stream.
//
//  For user references we add one for the reference in each of the internal
//  Fcb's.
//

#define CDFS_RESIDUAL_REFERENCE                     (6)
#define CDFS_RESIDUAL_USER_REFERENCE                (3)

//
//  Reserved directory strings
//

extern WCHAR CdUnicodeSelfArray[];
extern WCHAR CdUnicodeParentArray[];

extern UNICODE_STRING CdUnicodeDirectoryNames[];

//
//  Volume descriptor identifier strings.
//

extern CHAR CdHsgId[];
extern CHAR CdIsoId[];
extern CHAR CdXaId[];

//
//  Volume label for audio disks.
//

extern WCHAR CdAudioLabel[];
extern USHORT CdAudioLabelLength;

//
//  Pseudo file names for audio disks.
//

extern CHAR CdAudioFileName[];
extern UCHAR CdAudioFileNameLength;
extern ULONG CdAudioDirentSize;
extern ULONG CdAudioDirentsPerSector;
extern ULONG CdAudioSystemUseOffset;

#define AUDIO_NAME_ONES_OFFSET              (6)
#define AUDIO_NAME_TENS_OFFSET              (5)

//
//  Escape sequences for mounting Unicode volumes.
//

extern PCHAR CdJolietEscape[];

//
//  Hardcoded header for RIFF files.
//

extern LONG CdXAFileHeader[];
extern LONG CdAudioPlayHeader[];
extern LONG CdXAAudioPhileHeader[];

#ifdef CDFS_TELEMETRY_DATA

//
//  Globals for Telemetry data.
//

extern CDFS_TELEMETRY_DATA_CONTEXT CdTelemetryData;

#endif // CDFS_TELEMETRY_DATA

//
//  The following assertion macros ensure that the indicated structure
//  is valid
//
//      ASSERT_STRUCT( _In_ PVOID Struct, _In_ CSHORT NodeType );
//      ASSERT_OPTIONAL_STRUCT( _In_opt_ PVOID Struct, _In_ CSHORT NodeType );
//
//      ASSERT_VCB( _In_ PVCB Vcb );
//      ASSERT_OPTIONAL_VCB( _In_opt_ PVCB Vcb );
//
//      ASSERT_FCB( _In_ PFCB Fcb );
//      ASSERT_OPTIONAL_FCB( _In_opt_ PFCB Fcb );
//
//      ASSERT_FCB_NONPAGED( _In_ PFCB_NONPAGED FcbNonpaged );
//      ASSERT_OPTIONAL_FCB( _In_opt_ PFCB_NONPAGED FcbNonpaged );
//
//      ASSERT_CCB( _In_ PSCB Ccb );
//      ASSERT_OPTIONAL_CCB( _In_opt_ PSCB Ccb );
//
//      ASSERT_IRP_CONTEXT( _In_ PIRP_CONTEXT IrpContext );
//      ASSERT_OPTIONAL_IRP_CONTEXT( _In_opt_ PIRP_CONTEXT IrpContext );
//
//      ASSERT_IRP( _In_ PIRP Irp );
//      ASSERT_OPTIONAL_IRP( _In_opt_ PIRP Irp );
//
//      ASSERT_FILE_OBJECT( _In_ PFILE_OBJECT FileObject );
//      ASSERT_OPTIONAL_FILE_OBJECT( _In_opt_ PFILE_OBJECT FileObject );
//
//  The following macros are used to check the current thread owns
//  the indicated resource
//
//      ASSERT_EXCLUSIVE_RESOURCE( _In_ PERESOURCE Resource );
//
//      ASSERT_SHARED_RESOURCE( _In_ PERESOURCE Resource );
//
//      ASSERT_RESOURCE_NOT_MINE( _In_ PERESOURCE Resource );
//
//  The following macros are used to check whether the current thread
//  owns the resoures in the given structures.
//
//      ASSERT_EXCLUSIVE_CDDATA
//
//      ASSERT_EXCLUSIVE_VCB( _In_ PVCB Vcb );
//
//      ASSERT_SHARED_VCB( _In_ PVCB Vcb );
//
//      ASSERT_EXCLUSIVE_FCB( _In_ PFCB Fcb );
//
//      ASSERT_SHARED_FCB( _In_ PFCB Fcb );
//
//      ASSERT_EXCLUSIVE_FILE( _In_ PFCB Fcb );
//
//      ASSERT_SHARED_FILE( _In_ PFCB Fcb );
//
//      ASSERT_LOCKED_VCB( _In_ PVCB Vcb );
//
//      ASSERT_NOT_LOCKED_VCB( _In_ PVCB Vcb );
//
//      ASSERT_LOCKED_FCB( _In_ PFCB Fcb );
//
//      ASSERT_NOT_LOCKED_FCB( _In_ PFCB Fcb );
//

//
//  Turn on the sanity checks if this is DBG or CD_FREE_ASSERTS
//

#if DBG
#undef CD_SANITY
//#define CD_SANITY
#endif

#ifdef CD_SANITY

#define ASSERT_STRUCT(S,T)                  NT_ASSERT( SafeNodeType( S ) == (T) )
#define ASSERT_OPTIONAL_STRUCT(S,T)         NT_ASSERT( ((S) == NULL) ||  (SafeNodeType( S ) == (T)) )

#define ASSERT_VCB(V)                       ASSERT_STRUCT( (V), CDFS_NTC_VCB )
#define ASSERT_OPTIONAL_VCB(V)              ASSERT_OPTIONAL_STRUCT( (V), CDFS_NTC_VCB )

#define ASSERT_FCB(F)                                           \
    NT_ASSERT( (SafeNodeType( F ) == CDFS_NTC_FCB_DATA ) ||        \
            (SafeNodeType( F ) == CDFS_NTC_FCB_INDEX ) ||       \
            (SafeNodeType( F ) == CDFS_NTC_FCB_PATH_TABLE ) )

#define ASSERT_OPTIONAL_FCB(F)                                  \
    NT_ASSERT( ((F) == NULL) ||                                    \
            (SafeNodeType( F ) == CDFS_NTC_FCB_DATA ) ||        \
            (SafeNodeType( F ) == CDFS_NTC_FCB_INDEX ) ||       \
            (SafeNodeType( F ) == CDFS_NTC_FCB_PATH_TABLE ) )

#define ASSERT_FCB_NONPAGED(FN)             ASSERT_STRUCT( (FN), CDFS_NTC_FCB_NONPAGED )
#define ASSERT_OPTIONAL_FCB_NONPAGED(FN)    ASSERT_OPTIONAL_STRUCT( (FN), CDFS_NTC_FCB_NONPAGED )

#define ASSERT_CCB(C)                       ASSERT_STRUCT( (C), CDFS_NTC_CCB )
#define ASSERT_OPTIONAL_CCB(C)              ASSERT_OPTIONAL_STRUCT( (C), CDFS_NTC_CCB )

#define ASSERT_IRP_CONTEXT(IC)              ASSERT_STRUCT( (IC), CDFS_NTC_IRP_CONTEXT )
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC)     ASSERT_OPTIONAL_STRUCT( (IC), CDFS_NTC_IRP_CONTEXT )

#define ASSERT_IRP(I)                       ASSERT_STRUCT( (I), IO_TYPE_IRP )
#define ASSERT_OPTIONAL_IRP(I)              ASSERT_OPTIONAL_STRUCT( (I), IO_TYPE_IRP )

#define ASSERT_FILE_OBJECT(FO)              ASSERT_STRUCT( (FO), IO_TYPE_FILE )
#define ASSERT_OPTIONAL_FILE_OBJECT(FO)     ASSERT_OPTIONAL_STRUCT( (FO), IO_TYPE_FILE )

#define ASSERT_EXCLUSIVE_RESOURCE(R)        NT_ASSERT( ExIsResourceAcquiredExclusiveLite( R ))

#define ASSERT_SHARED_RESOURCE(R)           NT_ASSERT( ExIsResourceAcquiredSharedLite( R ))

#define ASSERT_RESOURCE_NOT_MINE(R)         NT_ASSERT( !ExIsResourceAcquiredSharedLite( R ))

#define ASSERT_EXCLUSIVE_CDDATA             NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &CdData.DataResource ))
#define ASSERT_EXCLUSIVE_VCB(V)             NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &(V)->VcbResource ))
#define ASSERT_SHARED_VCB(V)                NT_ASSERT( ExIsResourceAcquiredSharedLite( &(V)->VcbResource ))

#define ASSERT_EXCLUSIVE_FCB(F)             NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &(F)->FcbNonpaged->FcbResource ))
#define ASSERT_SHARED_FCB(F)                NT_ASSERT( ExIsResourceAcquiredSharedLite( &(F)->FcbNonpaged->FcbResource ))

#define ASSERT_EXCLUSIVE_FILE(F)            NT_ASSERT( ExIsResourceAcquiredExclusiveLite( (F)->Resource ))
#define ASSERT_SHARED_FILE(F)               NT_ASSERT( ExIsResourceAcquiredSharedLite( (F)->Resource ))

#define ASSERT_LOCKED_VCB(V)                NT_ASSERT( (V)->VcbLockThread == PsGetCurrentThread() )
#define ASSERT_NOT_LOCKED_VCB(V)            NT_ASSERT( (V)->VcbLockThread != PsGetCurrentThread() )

#define ASSERT_LOCKED_FCB(F)                NT_ASSERT( !FlagOn( (F)->FcbState, FCB_STATE_IN_FCB_TABLE) || ((F)->FcbLockThread == PsGetCurrentThread()))
#define ASSERT_NOT_LOCKED_FCB(F)            NT_ASSERT( (F)->FcbLockThread != PsGetCurrentThread() )

#else

#define DebugBreakOnStatus(S)           { NOTHING; }

#define ASSERT_STRUCT(S,T)              { NOTHING; }
#define ASSERT_OPTIONAL_STRUCT(S,T)     { NOTHING; }
#define ASSERT_VCB(V)                   { NOTHING; }
#define ASSERT_OPTIONAL_VCB(V)          { NOTHING; }
#define ASSERT_FCB(F)                   { NOTHING; }
#define ASSERT_FCB_NONPAGED(FN)         { NOTHING; }
#define ASSERT_OPTIONAL_FCB(FN)         { NOTHING; }
#define ASSERT_CCB(C)                   { NOTHING; }
#define ASSERT_OPTIONAL_CCB(C)          { NOTHING; }
#define ASSERT_IRP_CONTEXT(IC)          { NOTHING; }
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC) { NOTHING; }
#define ASSERT_IRP(I)                   { NOTHING; }
#define ASSERT_OPTIONAL_IRP(I)          { NOTHING; }
#define ASSERT_FILE_OBJECT(FO)          { NOTHING; }
#define ASSERT_OPTIONAL_FILE_OBJECT(FO) { NOTHING; }
#define ASSERT_EXCLUSIVE_RESOURCE(R)    { NOTHING; }
#define ASSERT_SHARED_RESOURCE(R)       { NOTHING; }
#define ASSERT_RESOURCE_NOT_MINE(R)     { NOTHING; }
#define ASSERT_EXCLUSIVE_CDDATA         { NOTHING; }
#define ASSERT_EXCLUSIVE_VCB(V)         { NOTHING; }
#define ASSERT_SHARED_VCB(V)            { NOTHING; }
#define ASSERT_EXCLUSIVE_FCB(F)         { NOTHING; }
#define ASSERT_SHARED_FCB(F)            { NOTHING; }
#define ASSERT_EXCLUSIVE_FILE(F)        { NOTHING; }
#define ASSERT_SHARED_FILE(F)           { NOTHING; }
#define ASSERT_LOCKED_VCB(V)            { NOTHING; }
#define ASSERT_NOT_LOCKED_VCB(V)        { NOTHING; }
#define ASSERT_LOCKED_FCB(F)            { NOTHING; }
#define ASSERT_NOT_LOCKED_FCB(F)        { NOTHING; }

#endif

#endif // _CDDATA_

