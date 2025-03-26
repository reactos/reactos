#pragma once

#define PACKED            __attribute__((packed))

typedef struct
{
    unsigned char    JmpBoot[3];
    unsigned char    BootDrive;
    //unsigned char    BootPartition;
    //unsigned char    SectorsPerTrack;
    //unsigned short    NumberOfHeads;
    //unsigned long    Reserved1;
    //unsigned long    Reserved2;

    unsigned long    Ext2VolumeStartSector;    // Start sector of the ext2 volume
    unsigned long    Ext2BlockSize;            // Block size in sectors
    unsigned long    Ext2BlockSizeInBytes;    // Block size in bytes
    unsigned long    Ext2PointersPerBlock;    // Number of block pointers that can be contained in one block
    unsigned long    Ext2GroupDescPerBlock;    // Number of group descriptors per block
    unsigned long    Ext2FirstDataBlock;        // First data block (1 for 1024-byte blocks, 0 for bigger sizes)
    unsigned long    Ext2InodesPerGroup;        // Number of inodes per group
    unsigned long    Ext2InodesPerBlock;        // Number of inodes per block

    unsigned char    BootCodeAndData[459];

    unsigned char    BootPartition;
    unsigned short    BootSignature;

} PACKED EXT2_BOOTCODE, *PEXT2_BOOTCODE;
