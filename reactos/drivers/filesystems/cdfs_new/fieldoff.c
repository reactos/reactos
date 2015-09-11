#include "CdProcs.h"
#include <stdio.h>

#define doit(a,b) { printf("%s %04lx %4lx %s\n", #a, FIELD_OFFSET(a,b), sizeof(d.b), #b); }

VOID
__cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    printf("<Record>  <offset>  <size>  <field>\n\n");
    {
        CD_MCB d;
        doit( CD_MCB, MaximumEntryCount );
        doit( CD_MCB, CurrentEntryCount );
        doit( CD_MCB, McbArray );
    }
    printf("\n");
    {
        CD_MCB_ENTRY d;
        doit( CD_MCB_ENTRY, DiskOffset );
        doit( CD_MCB_ENTRY, ByteCount );
        doit( CD_MCB_ENTRY, FileOffset );
        doit( CD_MCB_ENTRY, DataBlockByteCount );
        doit( CD_MCB_ENTRY, TotalBlockByteCount );
    }
    printf("\n");
    {
        CD_NAME d;
        doit( CD_NAME, FileName );
        doit( CD_NAME, VersionString );
    }
    printf("\n");
    {
        NAME_LINK d;
        doit( NAME_LINK, Links );
        doit( NAME_LINK, FileName );
    }
    printf("\n");
    {
        PREFIX_ENTRY d;
        doit( PREFIX_ENTRY, Fcb );
        doit( PREFIX_ENTRY, PrefixFlags );
        doit( PREFIX_ENTRY, ExactCaseName );
        doit( PREFIX_ENTRY, IgnoreCaseName );
        doit( PREFIX_ENTRY, FileNameBuffer );
    }
    printf("\n");
    {
        CD_DATA d;
        doit( CD_DATA, NodeTypeCode );
        doit( CD_DATA, NodeByteSize );
        doit( CD_DATA, DriverObject );
        doit( CD_DATA, VcbQueue );
        doit( CD_DATA, IrpContextDepth );
        doit( CD_DATA, IrpContextMaxDepth );
        doit( CD_DATA, IrpContextList );
        doit( CD_DATA, FileSystemDeviceObject );
        doit( CD_DATA, AsyncCloseQueue );
        doit( CD_DATA, AsyncCloseCount );
        doit( CD_DATA, FspCloseActive );
        doit( CD_DATA, ReduceDelayedClose );
        doit( CD_DATA, PadUshort );
        doit( CD_DATA, DelayedCloseQueue );
        doit( CD_DATA, DelayedCloseCount );
        doit( CD_DATA, MinDelayedCloseCount );
        doit( CD_DATA, MaxDelayedCloseCount );
        doit( CD_DATA, CdDataLockThread );
        doit( CD_DATA, CdDataMutex );
        doit( CD_DATA, DataResource );
        doit( CD_DATA, CacheManagerCallbacks );
        doit( CD_DATA, CacheManagerVolumeCallbacks );
        doit( CD_DATA, CloseItem );
    }
    printf("\n");
    {
        VCB d;
        doit( VCB, NodeTypeCode );
        doit( VCB, NodeByteSize );
        doit( VCB, Vpb );
        doit( VCB, TargetDeviceObject );
        doit( VCB, VolumeLockFileObject );
        doit( VCB, VcbLinks );
        doit( VCB, VcbState );
        doit( VCB, VcbCondition );
        doit( VCB, VcbCleanup );
        doit( VCB, VcbReference );
        doit( VCB, VcbUserReference );
        doit( VCB, VolumeDasdFcb );
        doit( VCB, RootIndexFcb );
        doit( VCB, PathTableFcb );
        doit( VCB, BaseSector );
        doit( VCB, VdSectorOffset );
        doit( VCB, PrimaryVdSectorOffset );
        doit( VCB, XASector );
        doit( VCB, XADiskOffset );
        doit( VCB, VcbResource );
        doit( VCB, FileResource );
        doit( VCB, VcbMutex );
        doit( VCB, VcbLockThread );
        doit( VCB, NotifySync );
        doit( VCB, DirNotifyList );
        doit( VCB, BlockSize );
        doit( VCB, BlockToSectorShift );
        doit( VCB, BlockToByteShift );
        doit( VCB, BlocksPerSector );
        doit( VCB, BlockMask );
        doit( VCB, BlockInverseMask );
        doit( VCB, FcbTable );
        doit( VCB, CdromToc );
        doit( VCB, TocLength );
        doit( VCB, TrackCount );
        doit( VCB, DiskFlags );
        doit( VCB, BlockFactor );
    }
    printf("\n");
    {
        VOLUME_DEVICE_OBJECT d;
        doit( VOLUME_DEVICE_OBJECT, DeviceObject );
        doit( VOLUME_DEVICE_OBJECT, PostedRequestCount );
        doit( VOLUME_DEVICE_OBJECT, OverflowQueueCount );
        doit( VOLUME_DEVICE_OBJECT, OverflowQueue );
        doit( VOLUME_DEVICE_OBJECT, OverflowQueueSpinLock );
        doit( VOLUME_DEVICE_OBJECT, Vcb );
    }
    printf("\n");
    {
        FCB_DATA d;
        doit( FCB_DATA, Oplock );
        doit( FCB_DATA, FileLock );
    }
    printf("\n");
    {
        FCB_INDEX d;
        doit( FCB_INDEX, FileObject );
        doit( FCB_INDEX, StreamOffset );
        doit( FCB_INDEX, FcbQueue );
        doit( FCB_INDEX, Ordinal );
        doit( FCB_INDEX, ChildPathTableOffset );
        doit( FCB_INDEX, ChildOrdinal );
        doit( FCB_INDEX, ExactCaseRoot );
        doit( FCB_INDEX, IgnoreCaseRoot );
    }
    printf("\n");
    {
        FCB_NONPAGED d;
        doit( FCB_NONPAGED, NodeTypeCode );
        doit( FCB_NONPAGED, NodeByteSize );
        doit( FCB_NONPAGED, SegmentObject );
        doit( FCB_NONPAGED, FcbResource );
        doit( FCB_NONPAGED, FcbMutex );
    }
    printf("\n");
    {
        FCB d;
        doit( FCB, Header );
        doit( FCB, Vcb );
        doit( FCB, ParentFcb );
        doit( FCB, FcbLinks );
        doit( FCB, FileId );
        doit( FCB, FcbCleanup );
        doit( FCB, FcbReference );
        doit( FCB, FcbUserReference );
        doit( FCB, FcbState );
        doit( FCB, FileAttributes );
        doit( FCB, XAAttributes );
        doit( FCB, XAFileNumber );
        doit( FCB, FcbLockThread );
        doit( FCB, FcbLockCount );
        doit( FCB, FcbNonpaged );
        doit( FCB, ShareAccess );
        doit( FCB, McbEntry );
        doit( FCB, Mcb );
        doit( FCB, ShortNamePrefix );
        doit( FCB, FileNamePrefix );
        doit( FCB, CreationTime );
        doit( FCB, FcbType );
    }
    printf("\n");
    {
        CCB d;
        doit( CCB, NodeTypeCode );
        doit( CCB, NodeByteSize );
        doit( CCB, Flags );
        doit( CCB, Fcb );
        doit( CCB, CurrentDirentOffset );
        doit( CCB, SearchExpression );
    }
    printf("\n");
    {
        IRP_CONTEXT d;
        doit( IRP_CONTEXT, NodeTypeCode );
        doit( IRP_CONTEXT, NodeByteSize );
        doit( IRP_CONTEXT, Irp );
        doit( IRP_CONTEXT, Vcb );
        doit( IRP_CONTEXT, ExceptionStatus );
        doit( IRP_CONTEXT, Flags );
        doit( IRP_CONTEXT, RealDevice );
        doit( IRP_CONTEXT, IoContext );
        doit( IRP_CONTEXT, TeardownFcb );
        doit( IRP_CONTEXT, TopLevel );
        doit( IRP_CONTEXT, MajorFunction );
        doit( IRP_CONTEXT, MinorFunction );
        doit( IRP_CONTEXT, ThreadContext );
        doit( IRP_CONTEXT, WorkQueueItem );
    }
    printf("\n");
    {
        IRP_CONTEXT_LITE d;
        doit( IRP_CONTEXT_LITE, NodeTypeCode );
        doit( IRP_CONTEXT_LITE, NodeByteSize );
        doit( IRP_CONTEXT_LITE, Fcb );
        doit( IRP_CONTEXT_LITE, DelayedCloseLinks );
        doit( IRP_CONTEXT_LITE, UserReference );
        doit( IRP_CONTEXT_LITE, RealDevice );
    }
    printf("\n");
    {
        CD_IO_CONTEXT d;
        doit( CD_IO_CONTEXT, IrpCount );
        doit( CD_IO_CONTEXT, MasterIrp );
        doit( CD_IO_CONTEXT, Status );
        doit( CD_IO_CONTEXT, AllocatedContext );
        doit( CD_IO_CONTEXT, Resource );
        doit( CD_IO_CONTEXT, ResourceThreadId );
        doit( CD_IO_CONTEXT, SyncEvent );
    }
    printf("\n");
    {
        THREAD_CONTEXT d;
        doit( THREAD_CONTEXT, Cdfs );
        doit( THREAD_CONTEXT, SavedTopLevelIrp );
        doit( THREAD_CONTEXT, TopLevelIrpContext );
    }
    printf("\n");
    {
        PATH_ENUM_CONTEXT d;
        doit( PATH_ENUM_CONTEXT, Data );
        doit( PATH_ENUM_CONTEXT, BaseOffset );
        doit( PATH_ENUM_CONTEXT, DataLength );
        doit( PATH_ENUM_CONTEXT, Bcb );
        doit( PATH_ENUM_CONTEXT, DataOffset );
        doit( PATH_ENUM_CONTEXT, AllocatedData );
        doit( PATH_ENUM_CONTEXT, LastDataBlock );
    }
    printf("\n");
    {
        PATH_ENTRY d;
        doit( PATH_ENTRY, Ordinal );
        doit( PATH_ENTRY, PathTableOffset );
        doit( PATH_ENTRY, DiskOffset );
        doit( PATH_ENTRY, PathEntryLength );
        doit( PATH_ENTRY, ParentOrdinal );
        doit( PATH_ENTRY, DirNameLen );
        doit( PATH_ENTRY, DirName );
        doit( PATH_ENTRY, Flags );
        doit( PATH_ENTRY, CdDirName );
        doit( PATH_ENTRY, CdCaseDirName );
        doit( PATH_ENTRY, NameBuffer );
    }
    printf("\n");
    {
        COMPOUND_PATH_ENTRY d;
        doit( COMPOUND_PATH_ENTRY, PathContext );
        doit( COMPOUND_PATH_ENTRY, PathEntry );
    }
    printf("\n");
    {
        DIRENT_ENUM_CONTEXT d;
        doit( DIRENT_ENUM_CONTEXT, Sector );
        doit( DIRENT_ENUM_CONTEXT, BaseOffset );
        doit( DIRENT_ENUM_CONTEXT, DataLength );
        doit( DIRENT_ENUM_CONTEXT, Bcb );
        doit( DIRENT_ENUM_CONTEXT, SectorOffset );
        doit( DIRENT_ENUM_CONTEXT, NextDirentOffset );
    }
    printf("\n");
    {
        DIRENT d;
        doit( DIRENT, DirentOffset );
        doit( DIRENT, DirentLength );
        doit( DIRENT, StartingOffset );
        doit( DIRENT, DataLength );
        doit( DIRENT, CdTime );
        doit( DIRENT, DirentFlags );
        doit( DIRENT, Flags );
        doit( DIRENT, FileUnitSize );
        doit( DIRENT, InterleaveGapSize );
        doit( DIRENT, SystemUseOffset );
        doit( DIRENT, XAAttributes );
        doit( DIRENT, XAFileNumber );
        doit( DIRENT, FileNameLen );
        doit( DIRENT, FileName );
        doit( DIRENT, CdFileName );
        doit( DIRENT, CdCaseFileName );
        doit( DIRENT, ExtentType );
        doit( DIRENT, NameBuffer );
    }
    printf("\n");
    {
        COMPOUND_DIRENT d;
        doit( COMPOUND_DIRENT, DirContext );
        doit( COMPOUND_DIRENT, Dirent );
    }
    printf("\n");
    {
        FILE_ENUM_CONTEXT d;
        doit( FILE_ENUM_CONTEXT, PriorDirent );
        doit( FILE_ENUM_CONTEXT, InitialDirent );
        doit( FILE_ENUM_CONTEXT, CurrentDirent );
        doit( FILE_ENUM_CONTEXT, Flags );
        doit( FILE_ENUM_CONTEXT, FileSize );
        doit( FILE_ENUM_CONTEXT, ShortName );
        doit( FILE_ENUM_CONTEXT, ShortNameBuffer );
        doit( FILE_ENUM_CONTEXT, Dirents );
    }
    printf("\n");
    {
        RIFF_HEADER d;
        doit( RIFF_HEADER, ChunkId );
        doit( RIFF_HEADER, ChunkSize );
        doit( RIFF_HEADER, SignatureCDXA );
        doit( RIFF_HEADER, SignatureFMT );
        doit( RIFF_HEADER, XAChunkSize );
        doit( RIFF_HEADER, OwnerId );
        doit( RIFF_HEADER, Attributes );
        doit( RIFF_HEADER, SignatureXA );
        doit( RIFF_HEADER, FileNumber );
        doit( RIFF_HEADER, Reserved );
        doit( RIFF_HEADER, SignatureData );
        doit( RIFF_HEADER, RawSectors );
    }
    printf("\n");
    {
        AUDIO_PLAY_HEADER d;
        doit( AUDIO_PLAY_HEADER, Chunk );
        doit( AUDIO_PLAY_HEADER, ChunkSize );
        doit( AUDIO_PLAY_HEADER, SignatureCDDA );
        doit( AUDIO_PLAY_HEADER, SignatureFMT );
        doit( AUDIO_PLAY_HEADER, FMTChunkSize );
        doit( AUDIO_PLAY_HEADER, FormatTag );
        doit( AUDIO_PLAY_HEADER, TrackNumber );
        doit( AUDIO_PLAY_HEADER, DiskID );
        doit( AUDIO_PLAY_HEADER, StartingSector );
        doit( AUDIO_PLAY_HEADER, SectorCount );
        doit( AUDIO_PLAY_HEADER, TrackAddress );
        doit( AUDIO_PLAY_HEADER, TrackLength );
    }
    printf("\n");
    {
        RAW_ISO_VD d;
        doit( RAW_ISO_VD, DescType );
        doit( RAW_ISO_VD, StandardId );
        doit( RAW_ISO_VD, Version );
        doit( RAW_ISO_VD, VolumeFlags );
        doit( RAW_ISO_VD, SystemId );
        doit( RAW_ISO_VD, VolumeId );
        doit( RAW_ISO_VD, Reserved );
        doit( RAW_ISO_VD, VolSpaceI );
        doit( RAW_ISO_VD, VolSpaceM );
        doit( RAW_ISO_VD, CharSet );
        doit( RAW_ISO_VD, VolSetSizeI );
        doit( RAW_ISO_VD, VolSetSizeM );
        doit( RAW_ISO_VD, VolSeqNumI );
        doit( RAW_ISO_VD, VolSeqNumM );
        doit( RAW_ISO_VD, LogicalBlkSzI );
        doit( RAW_ISO_VD, LogicalBlkSzM );
        doit( RAW_ISO_VD, PathTableSzI );
        doit( RAW_ISO_VD, PathTableSzM );
        doit( RAW_ISO_VD, PathTabLocI );
        doit( RAW_ISO_VD, PathTabLocM );
        doit( RAW_ISO_VD, RootDe );
        doit( RAW_ISO_VD, VolSetId );
        doit( RAW_ISO_VD, PublId );
        doit( RAW_ISO_VD, PreparerId );
        doit( RAW_ISO_VD, AppId );
        doit( RAW_ISO_VD, Copyright );
        doit( RAW_ISO_VD, Abstract );
        doit( RAW_ISO_VD, Bibliograph );
        doit( RAW_ISO_VD, CreateDate );
        doit( RAW_ISO_VD, ModDate );
        doit( RAW_ISO_VD, ExpireDate );
        doit( RAW_ISO_VD, EffectDate );
        doit( RAW_ISO_VD, FileStructVer );
        doit( RAW_ISO_VD, Reserved3 );
        doit( RAW_ISO_VD, ResApp );
        doit( RAW_ISO_VD, Reserved4 );
    }
    printf("\n");
    {
        RAW_HSG_VD d;
        doit( RAW_HSG_VD, BlkNumI );
        doit( RAW_HSG_VD, BlkNumM );
        doit( RAW_HSG_VD, DescType );
        doit( RAW_HSG_VD, StandardId );
        doit( RAW_HSG_VD, Version );
        doit( RAW_HSG_VD, VolumeFlags );
        doit( RAW_HSG_VD, SystemId );
        doit( RAW_HSG_VD, VolumeId );
        doit( RAW_HSG_VD, Reserved );
        doit( RAW_HSG_VD, VolSpaceI );
        doit( RAW_HSG_VD, VolSpaceM );
        doit( RAW_HSG_VD, CharSet );
        doit( RAW_HSG_VD, VolSetSizeI );
        doit( RAW_HSG_VD, VolSetSizeM );
        doit( RAW_HSG_VD, VolSeqNumI );
        doit( RAW_HSG_VD, VolSeqNumM );
        doit( RAW_HSG_VD, LogicalBlkSzI );
        doit( RAW_HSG_VD, LogicalBlkSzM );
        doit( RAW_HSG_VD, PathTableSzI );
        doit( RAW_HSG_VD, PathTableSzM );
        doit( RAW_HSG_VD, PathTabLocI );
        doit( RAW_HSG_VD, PathTabLocM );
        doit( RAW_HSG_VD, RootDe );
        doit( RAW_HSG_VD, VolSetId );
        doit( RAW_HSG_VD, PublId );
        doit( RAW_HSG_VD, PreparerId );
        doit( RAW_HSG_VD, AppId );
        doit( RAW_HSG_VD, Copyright );
        doit( RAW_HSG_VD, Abstract );
        doit( RAW_HSG_VD, CreateDate );
        doit( RAW_HSG_VD, ModDate );
        doit( RAW_HSG_VD, ExpireDate );
        doit( RAW_HSG_VD, EffectDate );
        doit( RAW_HSG_VD, FileStructVer );
        doit( RAW_HSG_VD, Reserved3 );
        doit( RAW_HSG_VD, ResApp );
        doit( RAW_HSG_VD, Reserved4 );
    }
    printf("\n");
    {
        RAW_DIRENT d;
        doit( RAW_DIRENT, DirLen );
        doit( RAW_DIRENT, XarLen );
        doit( RAW_DIRENT, FileLoc );
        doit( RAW_DIRENT, FileLocMot );
        doit( RAW_DIRENT, DataLen );
        doit( RAW_DIRENT, DataLenMot );
        doit( RAW_DIRENT, RecordTime );
        doit( RAW_DIRENT, FlagsHSG );
        doit( RAW_DIRENT, FlagsISO );
        doit( RAW_DIRENT, IntLeaveSize );
        doit( RAW_DIRENT, IntLeaveSkip );
        doit( RAW_DIRENT, Vssn );
        doit( RAW_DIRENT, VssnMot );
        doit( RAW_DIRENT, FileIdLen );
        doit( RAW_DIRENT, FileId );
    }
    printf("\n");
    {
        RAW_PATH_ISO d;
        doit( RAW_PATH_ISO, DirIdLen );
        doit( RAW_PATH_ISO, XarLen );
        doit( RAW_PATH_ISO, DirLoc );
        doit( RAW_PATH_ISO, ParentNum );
        doit( RAW_PATH_ISO, DirId );
    }
    printf("\n");
    {
        RAW_PATH_HSG d;
        doit( RAW_PATH_HSG, DirLoc );
        doit( RAW_PATH_HSG, XarLen );
        doit( RAW_PATH_HSG, DirIdLen );
        doit( RAW_PATH_HSG, ParentNum );
        doit( RAW_PATH_HSG, DirId );
    }
    printf("\n");
    {
        SYSTEM_USE_XA d;
        doit( SYSTEM_USE_XA, OwnerId );
        doit( SYSTEM_USE_XA, Attributes );
        doit( SYSTEM_USE_XA, Signature );
        doit( SYSTEM_USE_XA, FileNumber );
        doit( SYSTEM_USE_XA, Reserved );
    }
}

