#ifndef _EXT2_COMMON_INCLUDE_
#define _EXT2_COMMON_INCLUDE_

/* global ioctl */
#define IOCTL_APP_VOLUME_PROPERTY \
CTL_CODE(FILE_DEVICE_UNKNOWN, 2000, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_APP_QUERY_PERFSTAT \
CTL_CODE(FILE_DEVICE_UNKNOWN, 2001, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_APP_MOUNT_POINT \
CTL_CODE(FILE_DEVICE_UNKNOWN, 2002, METHOD_BUFFERED, FILE_ANY_ACCESS)


/* performance / memory allocaiton statistics */
#define PS_IRP_CONTEXT  0x00
#define PS_VCB          0x01
#define PS_FCB          0x02
#define PS_CCB          0x03
#define PS_MCB          0x04
#define PS_EXTENT       0x05
#define PS_RW_CONTEXT   0x06
#define PS_VPB          0x07
#define PS_FILE_NAME    0x08
#define PS_MCB_NAME     0x09
#define PS_INODE_NAME   0x0A
#define PS_DIR_ENTRY    0x0B
#define PS_DIR_PATTERN  0x0C
#define PS_DISK_EVENT   0x0D
#define PS_DISK_BUFFER  0x0E
#define PS_BLOCK_DATA   0x0F

#define PS_EXT2_INODE   0x10
#define PS_DENTRY       0x11
#define PS_BUFF_HEAD    0x12

#define PS_MAX_TYPE_V1  (0x10)
#define PS_MAX_TYPE_V2  (0x30)

typedef union {

    ULONG           Slot[PS_MAX_TYPE_V1];

    struct {
        ULONG       IrpContext;
        ULONG       Vcb;
        ULONG       Fcb;
        ULONG       Ccb;
        ULONG       Mcb;
        ULONG       Extent;
        ULONG       RwContext;   /* rw context */
        ULONG       Vpb;
        ULONG       FileName;
        ULONG       McbName;
        ULONG       InodeName;
        ULONG       DirEntry;    /* pDir */
        ULONG       DirPattern;  /* Ccb-> in Ext2QeuryDir */
        ULONG       ReadDiskEvent;
        ULONG       ReadDiskBuffer;
        ULONG       BlockData;   /* Ext2Expand&TruncateFile*/
    };

} EXT2_STAT_ARRAY_V1;

typedef union {

    ULONG           Slot[PS_MAX_TYPE_V2];

    struct {
        ULONG       IrpContext;
        ULONG       Vcb;
        ULONG       Fcb;
        ULONG       Ccb;
        ULONG       Mcb;
        ULONG       Extent;
        ULONG       RwContext;  /* rw context */
        ULONG       Vpb;
        ULONG       FileName;
        ULONG       McbName;
        ULONG       InodeName;
        ULONG       DirEntry;   /* pDir */
        ULONG       DirPattern; /* Ccb-> in Ext2QeuryDir */
        ULONG       ReadDiskEvent;
        ULONG       ReadDiskBuffer;
        ULONG       BlockData;  /* Ext2Expand&TruncateFile*/
        ULONG       Inodes;     /* inodes */
        ULONG       NameEntries;    /* name dentry */
        ULONG       BufferHead; /* Buffer Header allocations */
    };

} EXT2_STAT_ARRAY_V2;

typedef struct _EXT2_PERF_STATISTICS_V1 {

    /* totoal number of processed/being processed requests */
    struct {
        ULONG           Processed;
        ULONG           Current;
    } Irps [IRP_MJ_MAXIMUM_FUNCTION + 1];

    /* structure size */
    EXT2_STAT_ARRAY_V1  Unit;

    /* current memory allocation statistics */
    EXT2_STAT_ARRAY_V1  Current;

    /* memory allocated in bytes */
    EXT2_STAT_ARRAY_V1  Size;

    /* totoal memory allocation statistics */
    EXT2_STAT_ARRAY_V1  Total;

} EXT2_PERF_STATISTICS_V1, *PEXT2_PERF_STATISTICS_V1;

#define EXT2_PERF_STAT_MAGIC '2SPE'
#define EXT2_PERF_STAT_VER2   2

typedef struct _EXT2_PERF_STATISTICS_V2 {

    ULONG               Magic;      /* EPS2 */
    USHORT              Version;    /* 02 */
    USHORT              Length;     /* sizeof(EXT2_PERF_STATISTICS_V2) */

    /* totoal number of processed/being processed requests */
    struct {
        ULONG           Processed;
        ULONG           Current;
    } Irps [IRP_MJ_MAXIMUM_FUNCTION + 1];

    /* structure size */
    EXT2_STAT_ARRAY_V2  Unit;

    /* current memory allocation statistics */
    EXT2_STAT_ARRAY_V2  Current;

    /* memory allocated in bytes */
    EXT2_STAT_ARRAY_V2  Size;

    /* totoal memory allocation statistics */
    EXT2_STAT_ARRAY_V2  Total;

} EXT2_PERF_STATISTICS_V2, *PEXT2_PERF_STATISTICS_V2;

/* volume property ... */

#define EXT2_VOLUME_PROPERTY_MAGIC 'EVPM'

#define EXT2_FLAG_VP_SET_GLOBAL   0x00000001

#define APP_CMD_QUERY_VERSION     0x00000000 /* with global flag set */
#define APP_CMD_QUERY_CODEPAGES   0x00000001
#define APP_CMD_QUERY_PROPERTY    0x00000002
#define APP_CMD_SET_PROPERTY      0x00000003
#define APP_CMD_QUERY_PROPERTY2   0x00000004
#define APP_CMD_SET_PROPERTY2     0x00000005
#define APP_CMD_QUERY_PROPERTY3   0x00000006
#define APP_CMD_SET_PROPERTY3     0x00000007

#define CODEPAGE_MAXLEN     0x20
#define HIDINGPAT_LEN       0x20

typedef struct _EXT2_VOLUME_PROPERTY {
    ULONG               Magic;
    ULONG               Flags;
    ULONG               Command;
    BOOLEAN             bReadonly;
    BOOLEAN             bExt3Writable;
    BOOLEAN             bExt2;
    BOOLEAN             bExt3;
    UCHAR               Codepage[CODEPAGE_MAXLEN];
} EXT2_VOLUME_PROPERTY;

#ifdef __cplusplus
typedef struct _EXT2_VOLUME_PROPERTY2:EXT2_VOLUME_PROPERTY {
#else   // __cplusplus
typedef struct _EXT2_VOLUME_PROPERTY2 {
    EXT2_VOLUME_PROPERTY ;
#endif  // __cplusplus

    /* new volume properties added after version 0.35 */

    /* volume uuid */
    __u8	                    UUID[16];

    /* mount point: driver letter only */
    UCHAR                       DrvLetter;

    /* checking bitmap */
    BOOLEAN                     bCheckBitmap;

    /* global hiding patterns */
    BOOLEAN                     bHidingPrefix;
    BOOLEAN                     bHidingSuffix;
    CHAR                        sHidingPrefix[HIDINGPAT_LEN];
    CHAR                        sHidingSuffix[HIDINGPAT_LEN];

} EXT2_VOLUME_PROPERTY2, *PEXT2_VOLUME_PROPERTY2;

#define EXT2_VPROP3_AUTOMOUNT 0x0000000000000001

typedef struct _EXT2_VOLUME_PROPERTY3 {
    EXT2_VOLUME_PROPERTY2  Prop2;
    unsigned __int64       Flags;
    int                    AutoMount:1;
    int                    Reserved1:31;
    int                    Reserved2[31];
} EXT2_VOLUME_PROPERTY3, *PEXT2_VOLUME_PROPERTY3;

/* Ext2Fsd driver version and built time */
typedef struct _EXT2_VOLUME_PROPERTY_VERSION {
    ULONG               Magic;
    ULONG               Flags;
    ULONG               Command;
    CHAR                Version[0x1C];
    CHAR                Time[0x20];
    CHAR                Date[0x20];
} EXT2_VOLUME_PROPERTY_VERSION, *PEXT2_VOLUME_PROPERTY_VERSION;

/* performance statistics */
#define EXT2_QUERY_PERFSTAT_MAGIC 'EVPM'
#define EXT2_QUERY_PERFSTAT_VER2  0x8000000

typedef struct _EXT2_QUERY_PERFSTAT {
    ULONG                   Magic;
    ULONG                   Flags;
    ULONG                   Command;
    union {
        EXT2_PERF_STATISTICS_V1 PerfStatV1;
        EXT2_PERF_STATISTICS_V2 PerfStatV2;
    };
} EXT2_QUERY_PERFSTAT, *PEXT2_QUERY_PERFSTAT;

#define EXT2_QUERY_PERFSTAT_SZV1 (FIELD_OFFSET(EXT2_QUERY_PERFSTAT, PerfStatV1) + sizeof(EXT2_PERF_STATISTICS_V1))
#define EXT2_QUERY_PERFSTAT_SZV2 (FIELD_OFFSET(EXT2_QUERY_PERFSTAT, PerfStatV1) + sizeof(EXT2_PERF_STATISTICS_V2))

/* mountpoint management  */
#define EXT2_APP_MOUNTPOINT_MAGIC 'EAMM'
typedef struct _EXT2_MOUNT_POINT {
    ULONG                   Magic;
    ULONG                   Flags;
    ULONG                   Size;
    ULONG                   Command;
    USHORT                  Link[256];
    USHORT                  Name[256];
} EXT2_MOUNT_POINT, *PEXT2_MOUNT_POINT;

#define APP_CMD_ADD_DOS_SYMLINK    0x00000001
#define APP_CMD_DEL_DOS_SYMLINK    0x00000002


#endif /* _EXT2_COMMON_INCLUDE_ */
