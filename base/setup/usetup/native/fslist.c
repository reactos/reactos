#include <usetup.h>

BOOLEAN
NATIVE_CreateFileSystemList(
    IN PFILE_SYSTEM_LIST List)
{
    FS_AddProvider(List, L"FAT", VfatFormat, VfatChkdsk);
#if 0
    FS_AddProvider(List, L"EXT2", Ext2Format, Ext2Chkdsk);
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
