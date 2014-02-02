/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/list.c
 * PURPOSE:         Manages all the partitions of the OS in
 *                  an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

static VOID list_disk(VOID)
{
    /* Header labels */
    PrintResourceString(IDS_LIST_DISK_HEAD);
    PrintResourceString(IDS_LIST_DISK_LINE);

    printf("\n\n");
}

static VOID list_partition(VOID)
{
    printf("List Partition!!\n");
}

static VOID list_volume(VOID)
{
    PrintResourceString(IDS_LIST_VOLUME_HEAD);
}

static VOID list_vdisk(VOID)
{
    printf("List VDisk!!\n");
}

BOOL list_main(INT argc, LPWSTR *argv)
{
    /* gets the first word from the string */
    if (argc == 1)
    {
        PrintResourceString(IDS_HELP_CMD_LIST);
        return TRUE;
    }

    /* determines which to list (disk, partition, etc.) */
    if(!wcsicmp(argv[1], L"disk"))
        list_disk();
    else if(!wcsicmp(argv[1], L"partition"))
        list_partition();
    else if(!wcsicmp(argv[1], L"volume"))
        list_volume();
    else if(!wcsicmp(argv[1], L"vdisk"))
        list_vdisk();
    else
        PrintResourceString(IDS_HELP_CMD_LIST);

    return TRUE;
}
