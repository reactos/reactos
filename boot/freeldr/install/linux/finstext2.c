#include <stdio.h>
#include <string.h>
#include "finstext2.h"
#include "ext2.h"    // #defines ext2_data
#include <linux/hdreg.h>
#include <linux/ext3_fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAJOR_VERSION    1
#define MINOR_VERSION    0

PEXT2_BOOTCODE            Ext2BootCode = (PEXT2_BOOTCODE)ext2_data;
struct ext3_super_block    Ext2SuperBlock;
struct hd_geometry        Ext2DriveGeometry;
unsigned char            Ext2ExistingBootCode[1024];

char                    BlockDevice[260];
int                        BlockDeviceSpecified = 0;
int                        BlockDeviceFileDescriptor;

int main(int argc, char *argv[])
{
    int        Index;
    char    ch;

    // Verify that we got enough command line parameters
    if (argc < 2)
    {
        printf("finstext2 block_device [-v]\n\n");
        printf("block_device    Specifies the ext2/3 volume to install to (i.e. /dev/hda1)\n");
        printf("-v              Displays the version\n\n");
    }

    // Parse the command line parameters
    for (Index=1; Index<argc; Index++)
    {
        if (strcmp(argv[Index], "-v") == 0)
        {
            printf("FreeLoader Ext2/3 Installer v%d.%d\n", MAJOR_VERSION, MINOR_VERSION);
        }
        else
        {
            strcpy(BlockDevice, argv[Index]);
            BlockDeviceSpecified = 1;
        }
    }

    if (BlockDeviceSpecified)
    {
        BlockDeviceFileDescriptor = open(BlockDevice, O_RDWR|O_SYNC);

        if (BlockDeviceFileDescriptor == -1)
        {
            printf("Couldn't open block device %s\n", BlockDevice);
            return 1;
        }

        if (read(BlockDeviceFileDescriptor, Ext2ExistingBootCode, (size_t)1024) != (ssize_t)1024)
        {
            close(BlockDeviceFileDescriptor);
            printf("Couldn't read existing boot code from %s\n", BlockDevice);
            return 1;
        }

        for (Index=0; Index<1024; Index++)
        {
            if (Ext2ExistingBootCode[Index] != 0x00)
            {
                printf("This EXT2/3 volume has existing boot code.\n");
                printf("Do you want to overwrite it? [y/n] ");
                scanf("%c", &ch);

                if (ch == 'n' || ch == 'N')
                {
                    close(BlockDeviceFileDescriptor);
                    printf("Cancelled.\n");
                    return 0;
                }

                break;
            }
        }

        if (read(BlockDeviceFileDescriptor, &Ext2SuperBlock, (size_t)1024) != (ssize_t)1024)
        {
            close(BlockDeviceFileDescriptor);
            printf("Couldn't read super block from %s\n", BlockDevice);
            return 1;
        }

        if (Ext2SuperBlock.s_magic != EXT3_SUPER_MAGIC)
        {
            close(BlockDeviceFileDescriptor);
            printf("Block device %s is not a EXT2/3 volume or has an invalid super block.\n", BlockDevice);
            return 1;
        }

        if (ioctl(BlockDeviceFileDescriptor, HDIO_GETGEO, &Ext2DriveGeometry) != 0)
        {
            close(BlockDeviceFileDescriptor);
            printf("Couldn't get drive geometry from block device %s\n", BlockDevice);
            return 1;
        }

        printf("Heads: %d\n", Ext2DriveGeometry.heads);
        printf("Sectors: %d\n", Ext2DriveGeometry.sectors);
        printf("Cylinders: %d\n", Ext2DriveGeometry.cylinders);
        printf("Start: %d\n", Ext2DriveGeometry.start);

        Ext2BootCode->BootDrive = 0xff;
        Ext2BootCode->BootPartition = 0x00;
        //Ext2BootCode->SectorsPerTrack = Ext2DriveGeometry.sectors;
        //Ext2BootCode->NumberOfHeads = Ext2DriveGeometry.heads;
        Ext2BootCode->Ext2VolumeStartSector = Ext2DriveGeometry.start;
        Ext2BootCode->Ext2BlockSizeInBytes = 1024 << Ext2SuperBlock.s_log_block_size;
        Ext2BootCode->Ext2BlockSize = Ext2BootCode->Ext2BlockSizeInBytes / 512;
        Ext2BootCode->Ext2PointersPerBlock = Ext2BootCode->Ext2BlockSizeInBytes / 4;
        Ext2BootCode->Ext2GroupDescPerBlock = Ext2BootCode->Ext2BlockSizeInBytes / 32;
        Ext2BootCode->Ext2FirstDataBlock = Ext2SuperBlock.s_first_data_block;
        Ext2BootCode->Ext2InodesPerGroup = Ext2SuperBlock.s_inodes_per_group;
        Ext2BootCode->Ext2InodesPerBlock = Ext2BootCode->Ext2BlockSizeInBytes / 128;

        if (lseek(BlockDeviceFileDescriptor, (off_t)0, SEEK_SET) == (off_t)-1)
        {
            close(BlockDeviceFileDescriptor);
            printf("Couldn't write boot code on %s\n", BlockDevice);
            return 1;
        }

        if (write(BlockDeviceFileDescriptor, Ext2BootCode, (size_t)1024) != (ssize_t)1024)
        {
            close(BlockDeviceFileDescriptor);
            printf("Couldn't write boot code on %s\n", BlockDevice);
            return 1;
        }

        close(BlockDeviceFileDescriptor);

        printf("Boot code written successfully!\n");
    }

    return 0;
}
