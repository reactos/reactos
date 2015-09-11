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


//
//  Turn on pseudo-asserts if CD_FREE_ASSERTS is defined.
//

#if !DBG
#ifdef CD_FREE_ASSERTS
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)        if (!(exp)) { extern BOOLEAN KdDebuggerEnabled; DbgPrint("%s:%d %s\n",__FILE__,__LINE__,#exp); if (KdDebuggerEnabled) { DbgBreakPoint(); } }
#define ASSERTMSG(msg,exp) if (!(exp)) { extern BOOLEAN KdDebuggerEnabled; DbgPrint("%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp); if (KdDebuggerEnabled) { DbgBreakPoint(); } }
#endif
#endif


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
//  Turn on the sanity checks if this is DBG or CD_FREE_ASSERTS
//

#if DBG
#undef CD_SANITY
#define CD_SANITY
#endif

#ifdef CD_SANITY

#define ASSERT_STRUCT(S,T)                  ASSERT( SafeNodeType( S ) == (T) )
#define ASSERT_OPTIONAL_STRUCT(S,T)         ASSERT( ((S) == NULL) ||  (SafeNodeType( S ) == (T)) )

#define ASSERT_VCB(V)                       ASSERT_STRUCT( (V), CDFS_NTC_VCB )
#define ASSERT_OPTIONAL_VCB(V)              ASSERT_OPTIONAL_STRUCT( (V), CDFS_NTC_VCB )

#define ASSERT_FCB(F)                                           \
    ASSERT( (SafeNodeType( F ) == CDFS_NTC_FCB_DATA ) ||        \
            (SafeNodeType( F ) == CDFS_NTC_FCB_INDEX ) ||       \
            (SafeNodeType( F ) == CDFS_NTC_FCB_PATH_TABLE ) )

#define ASSERT_OPTIONAL_FCB(F)                                  \
    ASSERT( ((F) == NULL) ||                                    \
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

#define ASSERT_EXCLUSIVE_RESOURCE(R)        ASSERT( ExIsResourceAcquiredExclusiveLite( R ))

#define ASSERT_SHARED_RESOURCE(R)           ASSERT( ExIsResourceAcquiredSharedLite( R ))

#define ASSERT_RESOURCE_NOT_MINE(R)         ASSERT( !ExIsResourceAcquiredSharedLite( R ))

#define ASSERT_EXCLUSIVE_CDDATA             ASSERT( ExIsResourceAcquiredExclusiveLite( &CdData.DataResource ))
#define ASSERT_EXCLUSIVE_VCB(V)             ASSERT( ExIsResourceAcquiredExclusiveLite( &(V)->VcbResource ))
#define ASSERT_SHARED_VCB(V)                ASSERT( ExIsResourceAcquiredSharedLite( &(V)->VcbResource ))

#define ASSERT_EXCLUSIVE_FCB(F)             ASSERT( ExIsResourceAcquiredExclusiveLite( &(F)->FcbNonpaged->FcbResource ))
#define ASSERT_SHARED_FCB(F)                ASSERT( ExIsResourceAcquiredSharedLite( &(F)->FcbNonpaged->FcbResource ))

#define ASSERT_EXCLUSIVE_FILE(F)            ASSERT( ExIsResourceAcquiredExclusiveLite( (F)->Resource ))
#define ASSERT_SHARED_FILE(F)               ASSERT( ExIsResourceAcquiredSharedLite( (F)->Resource ))

#define ASSERT_LOCKED_VCB(V)                ASSERT( (V)->VcbLockThread == PsGetCurrentThread() )
#define ASSERT_NOT_LOCKED_VCB(V)            ASSERT( (V)->VcbLockThread != PsGetCurrentThread() )

#define ASSERT_LOCKED_FCB(F)                ASSERT( !FlagOn( (F)->FcbState, FCB_STATE_IN_FCB_TABLE) || ((F)->FcbLockThread == PsGetCurrentThread()))
#define ASSERT_NOT_LOCKED_FCB(F)            ASSERT( (F)->FcbLockThread != PsGetCurrentThread() )

#else

#define DebugBreakOnStatus(S)           { NOTHING; }

#define ASSERT_STRUCT(S,T)              { NOTHING; }
#define ASSERT_OPTIONAL_STRUCT(S,T)     { NOTHING; }
#define ASSERT_VCB(V)                   { NOTHING; }
#define ASSERT_OPTIONAL_VCB(V)          { NOTHING; }
#define ASSERT_FCB(F)                   { NOTHING; }
#define ASSERT_OPTIONAL_FCB(F)          { NOTHING; }
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

