#include "precomp.h"
#pragma hdrstop

#define READ_SIZE   65536

typedef struct {
    UCHAR   IntelNearJumpCommand[1];    // Intel Jump command
    UCHAR   BootStrapJumpOffset[2];     // offset of boot strap code
    UCHAR   OemData[8];                     // OEM data
    UCHAR   BytesPerSector[2];          // BPB
    UCHAR   SectorsPerCluster[1];       //
    UCHAR   ReservedSectors[2];         //
    UCHAR   Fats[1];                    //
    UCHAR   RootEntries[2];             //
    UCHAR   Sectors[2];                 //
    UCHAR   Media[1];                   //
    UCHAR   SectorsPerFat[2];           //
    UCHAR   SectorsPerTrack[2];         //
    UCHAR   Heads[2];                   //
    UCHAR   HiddenSectors[4];           //
    UCHAR   LargeSectors[4];            //
        UCHAR   PhysicalDrive[1];                   // 0 = removable, 80h = fixed
        UCHAR   CurrentHead[1];                     // not used by fs utils
        UCHAR   Signature[1];                       // boot signature
        UCHAR   SerialNumber[4];            // serial number
        UCHAR   Label[11];                          // volume label, aligned padded
        UCHAR   SystemIdText[8];            // system ID, FAT for example
} UNALIGNED_SECTOR_ZERO, *PUNALIGNED_SECTOR_ZERO;


#define CSEC_FAT32MEG   65536
#define CSEC_FAT16BIT   32680
#define SYSID_FAT12BIT  1
#define SYSID_FAT16BIT  4
#define SYSID_FAT32MEG  6
#define MIN_CLUS_BIG    4085    // Minimum clusters for a big FAT.
#define MAX_CLUS_BIG    65525   // Maximum + 1 clusters for big FAT.



ARC_STATUS
FmtIsFatPartition(
    IN  ULONG       PartitionId,
    IN  ULONG       SectorSize,
    OUT PBOOLEAN    IsFatPartition
    )
/*++

Routine Description:

    This routine computes whether or not the given partition is a FAT
    partition.

Arguments:

    VolumeId        - Supplies the volume to check.
    SectorSize      - Supplies the number of bytes per sector.
    IsFatPartition  - Returns whether or not the partition is FAT.

Return Value:

    LowReadSectors, ENOMEM, ESUCCESS

--*/
{
    PUCHAR      Buffer;
    ARC_STATUS  r;

    if (!(Buffer = AllocateMemory(SectorSize*2))) {
        return ENOMEM;
    }

    r = LowReadSectors(PartitionId, SectorSize, 0, 2, Buffer);

    if (r != ESUCCESS) {
        FreeMemory(Buffer);
        return r;
    }

    *IsFatPartition = Buffer[510] == 0x55 &&
                      Buffer[511] == 0xAA &&
                      Buffer[0x10] != 0 &&
                      Buffer[0x15] == Buffer[SectorSize] &&
                      Buffer[SectorSize + 1] == 0xFF &&
                      Buffer[SectorSize + 2] == 0xFF;

    FreeMemory(Buffer);
    return ESUCCESS;
}


ARC_STATUS
FmtIsFat(
    IN  PCHAR       PartitionPath,
    OUT PBOOLEAN    IsFatPartition
    )
/*++

Routine Description:

    This routine computes whether or not the given partition is a FAT
    partition.

Arguments:

    PartitionPath   - Supplies a path to the partition.
    IsFatPartition  - Returns whether or not the partition is FAT.

Return Value:

    ArcOpen, ArcClose, LowGetPartitionGeometry, FmtIsFatPartition

--*/
{
    ULONG       partition_id;
    ARC_STATUS  r;
    ULONG       total_sectors, sector_size, sec_per_track, heads;

    r = LowGetPartitionGeometry(PartitionPath, &total_sectors, &sector_size,
                                &sec_per_track, &heads);

    if (r != ESUCCESS) {
        return r;
    }

    r = ArcOpen(PartitionPath, ArcOpenReadOnly, &partition_id);

    if (r != ESUCCESS) {
        return r;
    }

    r = FmtIsFatPartition(partition_id, sector_size, IsFatPartition);

    if (r != ESUCCESS) {
        ArcClose(partition_id);
        return r;
    }

    return ArcClose(partition_id);
}




USHORT
ComputeSecPerCluster(
    IN  ULONG   NumSectors,
    IN  BOOLEAN SmallFat
    )
/*++

Routine Description:

    This routine computes the number of sectors per cluster.

Arguments:

    NumSectors  - Supplies the number of sectors on the disk.
    SmallFat    - Supplies whether or not the FAT should be small.

Return Value:

    The number of sectors per cluster necessary.

--*/
{
    ULONG   threshold;
    USHORT  sec_per_clus;
    USHORT  min_sec_per_clus;

    threshold = SmallFat ? MIN_CLUS_BIG : MAX_CLUS_BIG;
    sec_per_clus = 1;

    while (NumSectors >= threshold) {
        sec_per_clus *= 2;
        threshold *= 2;
    }

    if (SmallFat) {
        min_sec_per_clus = 8;
    } else {
        min_sec_per_clus = 4;
    }

    return max(sec_per_clus, min_sec_per_clus);
}


ULONG
ComputeNewSerialNumber(
    IN  ULONG   Seed
    )
/*++

Routine Description:

    This routine computes a new serial number for a volume.

Arguments:

    Seed    - Supplies a seed for the serial number.

Return Value:

    A new volume serial number.

--*/
{
    PUCHAR  p;
    ULONG   i;

    p = (PUCHAR) &Seed;

    for (i = 0; i < sizeof(ULONG); i++) {

        Seed += p[i];
        Seed = (Seed >> 2) + (Seed << 30);
    }

    return Seed;
}


VOID
EditFat(
    IN      USHORT  ClusterNumber,
    IN      USHORT  ClusterEntry,
    IN OUT  PUCHAR  Fat,
    IN      BOOLEAN SmallFat
    )
/*++

Routine Description:

    This routine edits the FAT entry 'ClusterNumber' with 'ClusterEntry'.

Arguments:

    ClusterNumber   - Supplies the number of the cluster to edit.
    ClusterEntry    - Supplies the new value for that cluster number.
    Fat             - Supplies the FAT to edit.
    SmallFat        - Supplies whether or not the FAT is small.

Return Value:

    None.

--*/
{
    ULONG   n;

    if (SmallFat) {

        n = ClusterNumber*3;
        if (n%2) {
                Fat[n/2] = (UCHAR) ((Fat[n/2]&0x0F) | ((ClusterEntry&0x000F)<<4));
            Fat[n/2 + 1] = (UCHAR) ((ClusterEntry&0x0FF0)>>4);
        } else {
            Fat[n/2] = (UCHAR) (ClusterEntry&0x00FF);
                Fat[n/2 + 1] = (UCHAR) ((Fat[n/2 + 1]&0xF0) |
                                    ((ClusterEntry&0x0F00)>>8));
        }

    } else {

        ((PUSHORT) Fat)[ClusterNumber] = ClusterEntry;

    }
}


ARC_STATUS
FmtFillFormatBuffer(
    IN  ULONG   NumberOfSectors,
    IN  ULONG   SectorSize,
    IN  ULONG   SectorsPerTrack,
    IN  ULONG   NumberOfHeads,
    IN  ULONG   NumberOfHiddenSectors,
    OUT PVOID   FormatBuffer,
    IN  ULONG   FormatBufferSize,
    OUT PULONG  SuperAreaSize,
    IN  ULONG   TimeSeed,
    IN  PULONG  BadSectorsList,
    IN  ULONG   NumberOfBadSectors
    )
/*++

Routine Description:

    This routine computes a FAT super area based on the disk size,
    disk geometry, and bad sectors of the volume.

Arguments:

    NumberOfSectors         - Supplies the number of sectors on the volume.
    SectorSize              - Supplies the number of bytes per sector.
    SectorsPerTrack         - Supplies the number of sectors per track.
    NumberOfHeads           - Supplies the number of heads.
    NumberOfHiddenSectors   - Supplies the number of hidden sectors.
    FormatBuffer            - Returns the super area for the volume.
    FormatBufferSize        - Supplies the number of bytes in the supplied
                                buffer.
    SuperAreaSize           - Returns the number of bytes in the super area.
    TimeSeed                - Supplies a time seed for serial number.
    BadSectorsList          - Supplies the list of bad sectors on the volume.
    NumberOfBadSectors      - Supplies the number of bad sectors in the list.

Return Value:

    ENOMEM  - The buffer wasn't big enough.
    E2BIG   - The disk is too large to be formatted.
    EIO     - There is a bad sector in the super area.
    EINVAL  - There is a bad sector off the end of the disk.
    ESUCCESS

--*/
{
    PUNALIGNED_SECTOR_ZERO  psecz;
    PUCHAR                  puchar;
    USHORT                  tmp_ushort;
    ULONG                   tmp_ulong;
    BOOLEAN                 small_fat;
    ULONG                   num_sectors;
    ULONG                   partition_id;
    ULONG                   sec_per_fat;
    ULONG                   sec_per_root;
    ULONG                   sec_per_clus;
    ULONG                   i;
    ULONG                   sec_per_sa;


    // First memset the buffer to all zeros;
    memset(FormatBuffer, 0, (unsigned int) FormatBufferSize);


    // Make sure that there's enough room for the BPB.

    if (!FormatBuffer || FormatBufferSize < SectorSize) {
        return ENOMEM;
    }

    // Compute the number of sectors on disk.
    num_sectors = NumberOfSectors;

    // Compute the partition identifier.
    partition_id = num_sectors < CSEC_FAT16BIT ? SYSID_FAT12BIT :
                   num_sectors < CSEC_FAT32MEG ? SYSID_FAT16BIT :
                                                 SYSID_FAT32MEG;

    // Compute whether or not to have a big or small FAT.
    small_fat = (BOOLEAN) (partition_id == SYSID_FAT12BIT);


    psecz = (PUNALIGNED_SECTOR_ZERO) FormatBuffer;
    puchar = (PUCHAR) FormatBuffer;

    // Set up the jump instruction.
    psecz->IntelNearJumpCommand[0] = 0xEB;
    tmp_ushort = 0x903C;
    memcpy(psecz->BootStrapJumpOffset, &tmp_ushort, sizeof(USHORT));

    // Set up the OEM data.
    memcpy(psecz->OemData, "WINNT1.0", 8); // BUGBUG norbertk

    // Set up the bytes per sector.
    tmp_ushort = (USHORT) SectorSize;
    memcpy(psecz->BytesPerSector, &tmp_ushort, sizeof(USHORT));

    // Set up the number of sectors per cluster.
    sec_per_clus = ComputeSecPerCluster(num_sectors, small_fat);
    if (sec_per_clus > 128) {

        // The disk is too large to be formatted.
        return E2BIG;
    }
    psecz->SectorsPerCluster[0] = (UCHAR) sec_per_clus;

    // Set up the number of reserved sectors.
    tmp_ushort = 1;
    memcpy(psecz->ReservedSectors, &tmp_ushort, sizeof(USHORT));

    // Set up the number of FATs.
    psecz->Fats[0] = 2;

    // Set up the number of root entries and number of sectors for the root.
    tmp_ushort = 512;
    memcpy(psecz->RootEntries, &tmp_ushort, sizeof(USHORT));
    sec_per_root = (512*32 - 1)/SectorSize + 1;

    // Set up the number of sectors.
    if (num_sectors >= 1<<16) {
        tmp_ushort = 0;
        tmp_ulong = num_sectors;
    } else {
        tmp_ushort = (USHORT) num_sectors;
        tmp_ulong = 0;
    }
    memcpy(psecz->Sectors, &tmp_ushort, sizeof(USHORT));
    memcpy(psecz->LargeSectors, &tmp_ulong, sizeof(ULONG));

    // Set up the media byte.
    psecz->Media[0] = 0xF8;

    // Set up the number of sectors per FAT.
    if (small_fat) {
        sec_per_fat = num_sectors/(2 + SectorSize*sec_per_clus*2/3);
    } else {
        sec_per_fat = num_sectors/(2 + SectorSize*sec_per_clus/2);
    }
    sec_per_fat++;
    tmp_ushort = (USHORT) sec_per_fat;
    memcpy(psecz->SectorsPerFat, &tmp_ushort, sizeof(USHORT));

    // Set up the number of sectors per track.
    tmp_ushort = (USHORT) SectorsPerTrack;
    memcpy(psecz->SectorsPerTrack, &tmp_ushort, sizeof(USHORT));

    // Set up the number of heads.
    tmp_ushort = (USHORT) NumberOfHeads;
    memcpy(psecz->Heads, &tmp_ushort, sizeof(USHORT));

    // Set up the number of hidden sectors.
    memcpy(psecz->HiddenSectors, &NumberOfHiddenSectors, sizeof(ULONG));

    // Set up the physical drive number.
    psecz->PhysicalDrive[0] = 0x80;

    // Set up the BPB signature.
    psecz->Signature[0] = 0x29;

    // Set up the serial number.
    tmp_ulong = ComputeNewSerialNumber(TimeSeed);
    memcpy(psecz->SerialNumber, &tmp_ulong, sizeof(ULONG));

    // Set up the system id.
    memcpy(psecz->SystemIdText, "FAT     ", 8);

    // Set up the boot signature.
    puchar[510] = 0x55;
    puchar[511] = 0xAA;


    // Now make sure that the buffer has enough room for both of the
    // FATs and the root directory.

    sec_per_sa = 1 + 2*sec_per_fat + sec_per_root;
    *SuperAreaSize = SectorSize*sec_per_sa;
    if (*SuperAreaSize > FormatBufferSize) {
        return ENOMEM;
    }


    // Set up the first FAT.

    puchar[SectorSize] = 0xF8;
    puchar[SectorSize + 1] = 0xFF;
    puchar[SectorSize + 2] = 0xFF;

    if (!small_fat) {
        puchar[SectorSize + 3] = 0xFF;
    }


    for (i = 0; i < NumberOfBadSectors; i++) {

        if (BadSectorsList[i] < sec_per_sa) {
            // There's a bad sector in the super area.
            return EIO;
        }

        if (BadSectorsList[i] >= num_sectors) {
            // Bad sector out of range.
            return EINVAL;
        }

        // Compute the bad cluster number;
        tmp_ushort = (USHORT)
                     ((BadSectorsList[i] - sec_per_sa)/sec_per_clus + 2);

        EditFat(tmp_ushort, (USHORT) 0xFFF7, &puchar[SectorSize], small_fat);
    }


    // Copy the first FAT onto the second.

    memcpy(&puchar[SectorSize*(1 + sec_per_fat)],
           &puchar[SectorSize],
           (unsigned int) SectorSize*sec_per_fat);

    return ESUCCESS;
}


ARC_STATUS
FmtVerifySectors(
    IN  ULONG       PartitionId,
    IN  ULONG       NumberOfSectors,
    IN  ULONG       SectorSize,
    OUT PULONG*     BadSectorsList,
    OUT PULONG      NumberOfBadSectors
    )
/*++

Routine Description:

    This routine verifies all of the sectors on the volume.
    It returns a pointer to a list of bad sectors.  The pointer
    will be NULL if there was an error detected.

Arguments:

    PartitionId         - Supplies a handle to the partition for reading.
    NumberOfSectors     - Supplies the number of partition sectors.
    SectorSize          - Supplies the number of bytes per sector.
    BadSectorsList      - Returns the list of bad sectors.
    NumberOfBadSectors  - Returns the number of bad sectors in the list.

Return Value:

    ENOMEM, ESUCCESS

--*/
{
    ULONG           num_read_sec;
    PVOID           read_buffer;
    ULONG           i, j;
    PULONG          bad_sec_buf;
    ULONG           max_num_bad;
    ARC_STATUS      r;
    ULONG           percent;

    if (!(read_buffer = AllocateMemory(READ_SIZE))) {
        return ENOMEM;
    }

    max_num_bad = 100;
    if (!(bad_sec_buf = (PULONG) AllocateMemory(max_num_bad*sizeof(ULONG)))) {
        FreeMemory(read_buffer);
        return ENOMEM;
    }

    *NumberOfBadSectors = 0;

    num_read_sec = READ_SIZE/SectorSize;


    percent = 1;
    for (i = 0; i < NumberOfSectors; i += num_read_sec) {

        if (percent != i*100/NumberOfSectors) {
            percent = i*100/NumberOfSectors;
            AlPrint("%s%d percent formatted.\r", MSGMARGIN, percent);
        }

        if (i + num_read_sec > NumberOfSectors) {
            num_read_sec = NumberOfSectors - i;
        }

        r = LowReadSectors(PartitionId, SectorSize, i,
                           num_read_sec, read_buffer);

        if (r != ESUCCESS) {

            for (j = 0; j < num_read_sec; j++) {

                r = LowReadSectors(PartitionId, SectorSize, i+j, 1,
                                   read_buffer);

                if (r != ESUCCESS) {

                    if (*NumberOfBadSectors == max_num_bad) {

                        max_num_bad += 100;
                        if (!(bad_sec_buf = (PULONG)
                              ReallocateMemory(bad_sec_buf,
                                               max_num_bad*sizeof(ULONG)))) {

                            FreeMemory(read_buffer);
                            FreeMemory(bad_sec_buf);
                            return ENOMEM;
                        }
                    }

                    bad_sec_buf[(*NumberOfBadSectors)++] = i + j;
                }
            }
        }
    }

    AlPrint("%s100 percent formatted.\r\n",MSGMARGIN);

    *BadSectorsList = bad_sec_buf;

    FreeMemory(read_buffer);

    return ESUCCESS;
}


ARC_STATUS
FmtFatFormat(
    IN  PCHAR   PartitionPath,
    IN  ULONG   HiddenSectorCount
    )
/*++

Routine Description:

    This routine does a FAT format on the given partition.

Arguments:

    PartitionPath   - Supplies a path to the partition to format.

Return Value:

    LowGetPartitionGeometry, ArcOpen,
    FmtVerifySectors, FmtFillFormatBuffer, LowWriteSectors,
    ArcClose, ENOMEM

--*/
{
    ULONG           num_sectors;
    ULONG           sector_size;
    ULONG           sec_per_track;
    ULONG           heads;
    ULONG           hidden_sectors;
    PULONG          bad_sectors;
    ULONG           num_bad_sectors;
    PVOID           format_buffer;
    ULONG           max_sec_per_sa;
    ULONG           super_area_size;
    ARC_STATUS      r;
    ULONG           partition_id;
    ULONG           time_seed;
    PTIME_FIELDS    ptime_fields;


    r = LowGetPartitionGeometry(PartitionPath, &num_sectors,
                                &sector_size, &sec_per_track, &heads);

    if (r != ESUCCESS) {
        return r;
    }

    hidden_sectors = HiddenSectorCount;

    r = ArcOpen(PartitionPath, ArcOpenReadWrite, &partition_id);

    if (r != ESUCCESS) {
        return r;
    }

    bad_sectors = NULL;

    r = FmtVerifySectors(partition_id, num_sectors, sector_size,
                         &bad_sectors, &num_bad_sectors);

    if (r != ESUCCESS) {
        ArcClose(partition_id);
        return r;
    }

    max_sec_per_sa = 1 +
                     2*((2*65536 - 1)/sector_size + 1) +
                     ((512*32 - 1)/sector_size + 1);

    ptime_fields = ArcGetTime();

    time_seed = (ptime_fields->Year - 1970)*366*24*60*60 +
                (ptime_fields->Month)*31*24*60*60 +
                (ptime_fields->Day)*24*60*60 +
                (ptime_fields->Hour)*60*60 +
                (ptime_fields->Minute)*60 +
                (ptime_fields->Second);

    if (!(format_buffer = AllocateMemory(max_sec_per_sa*sector_size))) {

        if (bad_sectors) {
            FreeMemory(bad_sectors);
        }
        ArcClose(partition_id);
        return ENOMEM;
    }

    r = FmtFillFormatBuffer(num_sectors,
                            sector_size,
                            sec_per_track,
                            heads,
                            hidden_sectors,
                            format_buffer,
                            max_sec_per_sa*sector_size,
                            &super_area_size,
                            time_seed,
                            bad_sectors,
                            num_bad_sectors);


    if (bad_sectors) {
        FreeMemory(bad_sectors);
    }

    if (r != ESUCCESS) {
        ArcClose(partition_id);
        FreeMemory(format_buffer);
        return r;
    }

    r = LowWriteSectors(partition_id, sector_size, 0,
                        super_area_size/sector_size, format_buffer);

    FreeMemory(format_buffer);

    if (r != ESUCCESS) {
        ArcClose(partition_id);
        return r;
    }

    return ArcClose(partition_id);
}


ARC_STATUS
FmtQueryFatPartitionList(
    OUT PCHAR** FatPartitionList,
    OUT PULONG  ListLength
    )
/*++

Routine Description:

    This routine browses the component tree for all disk peripherals
    and the attempts to open partitions on all of them.  It add all
    FAT partitions to the list and returns it.

Arguments:

    FatPartitionList    - Returns a list of FAT partitions.
    ListLength          - Returns the length of the list.

Return Value:

    LowQueryPathList, ArcOpen, ArcClose, FmtIsFat, EIO, ENOMEM, ESUCCESS

--*/
{
    CONFIGURATION_TYPE          config_type;
    ARC_STATUS                  r;
    PCHAR*                      peripheral_list;
    ULONG                       peripheral_list_length;
    ULONG                       i, j;
    ULONG                       num_partitions;
    CHAR                        partition_buf[21];
    ULONG                       partition_id;
    PCHAR*                      fat_partition_list;
    ULONG                       fat_partition_list_length;
    BOOLEAN                     is_fat;
    PCHAR                       partition_name;


    // First get a list of the all of the disk peripheral paths.

    config_type = DiskPeripheral;
    r = LowQueryPathList(NULL, &config_type, &peripheral_list,
                         &peripheral_list_length);

    if (r != ESUCCESS) {
        return r;
    }

    // Now we have a list of disk peripheral paths.



    // Next, count how many partitions there are.

    num_partitions = 0;
    for (i = 0; i < peripheral_list_length; i++) {

        partition_name = AllocateMemory(strlen(peripheral_list[i]) + 21);

        if (!partition_name) {
            LowFreePathList(peripheral_list, peripheral_list_length);
            return ENOMEM;
        }

        for (j = 1; ; j++) {

            strcpy(partition_name, peripheral_list[i]);
            sprintf(partition_buf, "partition(%d)", j);
            strcat(partition_name, partition_buf);

            r = ArcOpen(partition_name, ArcOpenReadOnly, &partition_id);

            if (r != ESUCCESS) {
                break;
            }

            num_partitions++;

            r = ArcClose(partition_id);

            if (r != ESUCCESS) {
                LowFreePathList(peripheral_list, peripheral_list_length);
                FreeMemory(partition_name);
                return r;
            }
        }

        FreeMemory(partition_name);
    }

    // 'num_partitions' indicates the number of partitions on the disk.


    // Allocate a buffer for the FAT partitions list.  There can be
    // no more FAT partitions then there are partitions.

    fat_partition_list = (PCHAR*) AllocateMemory(num_partitions*sizeof(PCHAR));

    if (!fat_partition_list) {
        LowFreePathList(peripheral_list, peripheral_list_length);
        return ENOMEM;
    }

    for (i = 0; i < num_partitions; i++) {
        fat_partition_list[i] = NULL;
    }

    fat_partition_list_length = 0;

    // 'fat_partition_list_length' indicates the number of FAT partitions.



    // Now go through all of the peripherals trying all possible
    // partitions on each.  Test these to see if they are FAT and
    // put the FAT ones in 'fat_partitions_list'.

    for (i = 0; i < peripheral_list_length; i++) {

        partition_name = AllocateMemory(strlen(peripheral_list[i]) + 21);

        if (!partition_name) {
            LowFreePathList(peripheral_list, peripheral_list_length);
            LowFreePathList(fat_partition_list, fat_partition_list_length);
            return ENOMEM;
        }

        for (j = 1; ; j++) {

            strcpy(partition_name, peripheral_list[i]);
            sprintf(partition_buf, "partition(%d)", j);
            strcat(partition_name, partition_buf);

            r = ArcOpen(partition_name, ArcOpenReadOnly, &partition_id);

            if (r != ESUCCESS) {
                break;
            }

            r = ArcClose(partition_id);

            if (r != ESUCCESS) {
                LowFreePathList(peripheral_list, peripheral_list_length);
                LowFreePathList(fat_partition_list, fat_partition_list_length);
                FreeMemory(partition_name);
                return r;
            }

            r = FmtIsFat(partition_name, &is_fat);

            if (r != ESUCCESS) {
                LowFreePathList(peripheral_list, peripheral_list_length);
                LowFreePathList(fat_partition_list, fat_partition_list_length);
                FreeMemory(partition_name);
                return r;
            }

            if (is_fat) {

                if (fat_partition_list_length == num_partitions) {
                    // This can't happen.
                    LowFreePathList(peripheral_list, peripheral_list_length);
                    LowFreePathList(fat_partition_list, fat_partition_list_length);
                    FreeMemory(partition_name);
                    return EIO;
                }

                fat_partition_list[fat_partition_list_length] =
                        AllocateMemory(strlen(partition_name) + 1);

                if (!fat_partition_list[fat_partition_list_length]) {
                    LowFreePathList(peripheral_list, peripheral_list_length);
                    LowFreePathList(fat_partition_list, fat_partition_list_length);
                    FreeMemory(partition_name);
                    return ENOMEM;
                }

                strcpy(fat_partition_list[fat_partition_list_length], partition_name);

                fat_partition_list_length++;
            }
        }

        FreeMemory(partition_name);
    }

    LowFreePathList(peripheral_list, peripheral_list_length);

    *FatPartitionList = fat_partition_list;
    *ListLength = fat_partition_list_length;

    return ESUCCESS;
}


ARC_STATUS
FmtFreeFatPartitionList(
    IN OUT  PCHAR*  FatPartitionList,
    IN      ULONG   ListLength
    )
/*++

Routine Description:

    This routine frees up the heap space taken by the FAT partition
    list.

Arguments:

    FatPartitionList    - Supplies the buffer to free.
    ListLength          - Supplies the buffer length.

Return Value:

    LowFreePathList

--*/
{
    return LowFreePathList(FatPartitionList, ListLength);
}
