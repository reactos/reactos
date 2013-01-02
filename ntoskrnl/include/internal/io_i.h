/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/io_i.h
 * PURPOSE:         Info Classes for the I/O Manager
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

//
// File Information Classes
//
UCHAR IopQueryOperationLength[] =
{
    0,
    0,
    0,
    0,
    sizeof(FILE_BASIC_INFORMATION),
    sizeof(FILE_STANDARD_INFORMATION),
    sizeof(FILE_INTERNAL_INFORMATION),
    sizeof(FILE_EA_INFORMATION),
    sizeof(FILE_ACCESS_INFORMATION),
    sizeof(FILE_NAME_INFORMATION),
    0,
    0,
    0,
    0,
    sizeof(FILE_POSITION_INFORMATION),
    0,
    sizeof(FILE_MODE_INFORMATION),
    sizeof(FILE_ALIGNMENT_INFORMATION),
    sizeof(FILE_ALL_INFORMATION),
    0,
    0,
    sizeof(FILE_NAME_INFORMATION),
    sizeof(FILE_STREAM_INFORMATION),
    sizeof(FILE_PIPE_INFORMATION),
    sizeof(FILE_PIPE_LOCAL_INFORMATION),
    sizeof(FILE_PIPE_REMOTE_INFORMATION),
    sizeof(FILE_MAILSLOT_QUERY_INFORMATION),
    0,
    sizeof(FILE_COMPRESSION_INFORMATION),
    sizeof(FILE_OBJECTID_INFORMATION),
    0,
    0,
    sizeof(FILE_QUOTA_INFORMATION),
    sizeof(FILE_REPARSE_POINT_INFORMATION),
    sizeof(FILE_NETWORK_OPEN_INFORMATION),
    sizeof(FILE_ATTRIBUTE_TAG_INFORMATION),
    0,
    0,
    0,
    0,
    0,
#if 0 // VISTA
    sizeof(FILE_IO_COMPLETION_NOTIFICATION_INFORMATION),
    sizeof(FILE_IOSTATUSBLOCK_RANGE_INFORMATION),
    sizeof(FILE_IO_PRIORITY_HINT_INFORMATION),
    sizeof(FILE_SFIO_RESERVE_INFORMATION),
    sizeof(FILE_SFIO_VOLUME_INFORMATION),
    0,
    sizeof(FILE_PROCESS_IDS_USING_FILE_INFORMATION),
    0,
    sizeof(FILE_NETWORK_PHYSICAL_NAME_INFORMATION),
#endif
    0xFF
};

UCHAR IopSetOperationLength[] =
{
    0,
    0,
    0,
    0,
    sizeof(FILE_BASIC_INFORMATION),
    0,
    0,
    0,
    0,
    0,
    sizeof(FILE_RENAME_INFORMATION),
    sizeof(FILE_LINK_INFORMATION),
    0,
    sizeof(FILE_DISPOSITION_INFORMATION),
    sizeof(FILE_POSITION_INFORMATION),
    0,
    sizeof(FILE_MODE_INFORMATION),
    0,
    0,
    sizeof(FILE_ALLOCATION_INFORMATION),
    sizeof(FILE_END_OF_FILE_INFORMATION),
    0,
    0,
    sizeof(FILE_PIPE_INFORMATION),
    0,
    0,
    0,
    sizeof(FILE_MAILSLOT_SET_INFORMATION),
    0,
    sizeof(FILE_OBJECTID_INFORMATION),
    sizeof(FILE_COMPLETION_INFORMATION),
    sizeof(FILE_MOVE_CLUSTER_INFORMATION),
    sizeof(FILE_QUOTA_INFORMATION),
    0,
    0,
    0,
    sizeof(FILE_TRACKING_INFORMATION),
    0,
    0,
    sizeof(FILE_VALID_DATA_LENGTH_INFORMATION),
    sizeof(UNICODE_STRING),
    0xFF
};

ACCESS_MASK IopQueryOperationAccess[] =
{
    0,
    0,
    0,
    0,
    FILE_READ_ATTRIBUTES,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    FILE_READ_EA,
    0,
    0,
    FILE_READ_ATTRIBUTES,
    0,
    0,
    0,
    0,
    FILE_READ_ATTRIBUTES,
    FILE_READ_ATTRIBUTES,
    FILE_READ_ATTRIBUTES,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    FILE_READ_ATTRIBUTES,
    FILE_READ_ATTRIBUTES,
    0,
    0,
    0,
    0,
    0,
    0xFFFFFFFF
};

ACCESS_MASK IopSetOperationAccess[] =
{
    0,
    0,
    0,
    0,
    FILE_WRITE_ATTRIBUTES,
    0,
    0,
    0,
    0,
    0,
    DELETE,
    0,
    0,
    DELETE,
    0,
    FILE_WRITE_EA,
    0,
    0,
    0,
    FILE_WRITE_DATA,
    FILE_WRITE_DATA,
    0,
    0,
    FILE_WRITE_ATTRIBUTES,
    0,
    FILE_WRITE_ATTRIBUTES,
    0,
    0,
    0,
    0,
    0,
    FILE_WRITE_DATA,
    0,
    0,
    0,
    0,
    FILE_WRITE_DATA,
    0,
    0,
    FILE_WRITE_DATA,
    DELETE,
    0xFFFFFFFF
};

//
// Volume Information Classes
//
UCHAR IopQueryFsOperationLength[] =
{
    0,
    sizeof(FILE_FS_VOLUME_INFORMATION),
    0,
    sizeof(FILE_FS_SIZE_INFORMATION),
    sizeof(FILE_FS_DEVICE_INFORMATION),
    sizeof(FILE_FS_ATTRIBUTE_INFORMATION),
    sizeof(FILE_FS_CONTROL_INFORMATION),
    sizeof(FILE_FS_FULL_SIZE_INFORMATION),
    sizeof(FILE_FS_OBJECTID_INFORMATION),
    sizeof(FILE_FS_DRIVER_PATH_INFORMATION),
#if 0 // VISTA
    sizeof(FILE_FS_VOLUME_FLAGS_INFORMATION),
#endif
    0xFF
};

UCHAR IopSetFsOperationLength[] =
{
    0,
    0,
    sizeof(FILE_FS_LABEL_INFORMATION),
    0,
    0,
    0,
    sizeof(FILE_FS_CONTROL_INFORMATION),
    0,
    sizeof(FILE_FS_OBJECTID_INFORMATION),
    0,
#if 0 // VISTA
    sizeof(FILE_FS_VOLUME_FLAGS_INFORMATION),
#endif
    0xFF
};

ULONG IopQueryFsOperationAccess[] =
{
    0,
    0,
    0,
    0,
    0,
    0,
    FILE_READ_DATA,
    0,
    0,
    0,
#if 0 // VISTA
    0,
#endif
    0xFFFFFFFF
};

ULONG IopSetFsOperationAccess[] =
{
    0,
    0,
    FILE_WRITE_DATA,
    0,
    0,
    0,
    FILE_WRITE_DATA,
    0,
    FILE_WRITE_DATA,
    0,
#if 0 // VISTA
    0,
#endif
    0xFFFFFFFF
};
