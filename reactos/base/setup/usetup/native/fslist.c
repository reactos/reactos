#include <usetup.h>

BOOLEAN
NATIVE_CreateFileSystemList(
    IN PFILE_SYSTEM_LIST List,
    IN UCHAR PartitionType)
{
    if (PartitionType == PARTITION_ENTRY_UNUSED ||
        PartitionType == PARTITION_FAT_12 ||
        PartitionType == PARTITION_FAT_16 ||
        PartitionType == PARTITION_HUGE ||
        PartitionType == PARTITION_XINT13 ||
        PartitionType == PARTITION_FAT32 ||
        PartitionType == PARTITION_FAT32_XINT13)
    {
        FS_AddProvider(List, L"FAT", VfatFormat, VfatChkdsk);
    }

#if 0
    if (PartitionType == PARTITION_ENTRY_UNUSED ||
        PartitionType == PARTITION_EXT2)
    {
        FS_AddProvider(List, L"EXT2", Ext2Format, Ext2Chkdsk);
    }
#endif

#if 0
    if (PartitionType == PARTITION_ENTRY_UNUSED ||
        PartitionType == PARTITION_IFS)
    {
        FS_AddProvider(List, L"NTFS", NtfsFormat, NtfsChkdsk);
    }
#endif

    return TRUE;
}


BOOLEAN
NATIVE_FormatPartition(
    IN PFILE_SYSTEM_ITEM FileSystem,
    IN PCUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;

    Status = FileSystem->FormatFunc((PUNICODE_STRING)DriveRoot,
                                    FMIFS_HARDDISK,          /* MediaFlag */
                                    NULL,                    /* Label */
                                    FileSystem->QuickFormat, /* QuickFormat */
                                    0,                       /* ClusterSize */
                                    Callback);               /* Callback */
    return NT_SUCCESS(Status);
}
