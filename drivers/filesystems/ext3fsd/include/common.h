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

#define PS_MAX_TYPE     (0x10)

typedef union {

    ULONG           Slot[PS_MAX_TYPE];

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
        ULONG       BlockData;  /* Ext2Expand&TruncateFile*/
    };

} EXT2_STAT_ARRAY;


typedef struct _EXT2_PERF_STATISTICS {

    /* totoal number of processed/being processed requests */
    struct {
        ULONG           Processed;
        ULONG           Current;
    } Irps [IRP_MJ_MAXIMUM_FUNCTION + 1];

    /* structure size */
    EXT2_STAT_ARRAY     Unit;

    /* current memory allocation statistics */
    EXT2_STAT_ARRAY     Current;

    /* memory allocated in bytes */
    EXT2_STAT_ARRAY     Size;

    /* totoal memory allocation statistics */
    EXT2_STAT_ARRAY     Total;

} EXT2_PERF_STATISTICS, *PEXT2_PERF_STATISTICS;

/* volume property ... */

#define EXT2_VOLUME_PROPERTY_MAGIC 'EVPM'

#define EXT2_FLAG_VP_SET_GLOBAL   0x00000001

#define APP_CMD_QUERY_VERSION     0x00000000 /* with global flag set */
#define APP_CMD_QUERY_CODEPAGES   0x00000001
#define APP_CMD_QUERY_PROPERTY    0x00000002
#define APP_CMD_SET_PROPERTY      0x00000003
#define APP_CMD_QUERY_PROPERTY2   0x00000004
#define APP_CMD_SET_PROPERTY2     0x00000005

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
typedef struct _EXT2_QUERY_PERFSTAT {
    ULONG                   Magic;
    ULONG                   Flags;
    ULONG                   Command;
    EXT2_PERF_STATISTICS    PerfStat;
} EXT2_QUERY_PERFSTAT, *PEXT2_QUERY_PERFSTAT;

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