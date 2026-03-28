/*
 * PROJECT:     ReactOS Disk Image Creator
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Assembles a bootable MBR+FAT32 disk image from a partition image
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.img@outlook.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512
#define MBR_BOOT_CODE_SIZE 440
#define MBR_PARTITION_TABLE_OFFSET 446
#define MBR_SIGNATURE_OFFSET 510
#define PARTITION_ENTRY_SIZE 16

/* FAT/FAT32 BPB field offsets (from start of VBR) */
#define FAT_BPB_BYTES_PER_SECTOR_OFFSET 11
#define FAT_BPB_RESERVED_SECTORS_OFFSET 14
#define FAT_BPB_NUMBER_OF_FATS_OFFSET 16
#define FAT32_MEDIA_DESCRIPTOR_OFFSET 21
#define FAT_BPB_SECTORS_PER_FAT16_OFFSET 22
#define FAT32_HIDDEN_SECTORS_OFFSET 28
#define FAT32_SECTORS_PER_FAT_OFFSET 36
#define FAT32_BACKUP_BOOT_SECTOR_OFFSET 50
#define FAT32_BOOT_DRIVE_OFFSET 64

/* Fixed disk media descriptor (0xF8) vs removable (0xF0) */
#define MEDIA_DESCRIPTOR_FIXED 0xF8
/* 0xFF tells boot code to use BIOS-provided drive number in DL */
#define BOOT_DRIVE_AUTO 0xFF

/* Conventional translation geometry for LBA-assisted partition tables */
#define CHS_SECTORS_PER_TRACK 63
#define CHS_HEADS 255

/* Default values */
#define DEFAULT_START_SECTOR 2048
#define DEFAULT_PARTITION_TYPE 0x0C /* FAT32 LBA */

#pragma pack(push, 1)
typedef struct _PARTITION_ENTRY
{
    unsigned char Status;       /* 0x80 = active */
    unsigned char CHSFirst[3];  /* CHS of first sector */
    unsigned char Type;         /* Partition type */
    unsigned char CHSLast[3];   /* CHS of last sector */
    unsigned int  LBAStart;     /* LBA of first sector */
    unsigned int  LBASize;      /* Number of sectors */
} PARTITION_ENTRY;
#pragma pack(pop)

static void print_usage(const char* name)
{
    printf("Usage: %s -o <output> -mbr <mbr.bin> -partition <part.img> [-start <sector>] [-type <hex>]\n\n", name);
    printf("  -o <output>         Output disk image file\n");
    printf("  -mbr <mbr.bin>      MBR boot code binary (first 440 bytes used)\n");
    printf("  -partition <img>    Raw FAT32 partition image\n");
    printf("  -start <sector>     Partition start sector (default: %d)\n", DEFAULT_START_SECTOR);
    printf("  -type <hex>         Partition type ID (default: 0x%02X)\n", DEFAULT_PARTITION_TYPE);
}

static int copy_file_data(FILE* dst, FILE* src, long size)
{
    unsigned char buf[32768];
    long remaining = size;

    while (remaining > 0)
    {
        size_t chunk = remaining < (long)sizeof(buf) ? (size_t)remaining : sizeof(buf);
        size_t rd = fread(buf, 1, chunk, src);
        if (rd == 0)
            return -1;
        if (fwrite(buf, 1, rd, dst) != rd)
            return -1;
        remaining -= (long)rd;
    }
    return remaining == 0 ? 0 : -1;
}

static unsigned short read_le16(const unsigned char* ptr)
{
    return (unsigned short)((unsigned short)ptr[0] |
                            ((unsigned short)ptr[1] << 8));
}

static unsigned int read_le32(const unsigned char* ptr)
{
    return (unsigned int)((unsigned int)ptr[0] |
                          ((unsigned int)ptr[1] << 8) |
                          ((unsigned int)ptr[2] << 16) |
                          ((unsigned int)ptr[3] << 24));
}

static void write_chs(unsigned int lba, unsigned char chs[3])
{
    unsigned int cylinders;
    unsigned int heads;
    unsigned int sectors;
    unsigned int temp;

    cylinders = lba / (CHS_HEADS * CHS_SECTORS_PER_TRACK);
    if (cylinders > 1023)
    {
        chs[0] = 0xFE;
        chs[1] = 0xFF;
        chs[2] = 0xFF;
        return;
    }

    temp = lba % (CHS_HEADS * CHS_SECTORS_PER_TRACK);
    heads = temp / CHS_SECTORS_PER_TRACK;
    sectors = (temp % CHS_SECTORS_PER_TRACK) + 1;

    chs[0] = (unsigned char)heads;
    chs[1] = (unsigned char)((sectors & 0x3F) | ((cylinders >> 2) & 0xC0));
    chs[2] = (unsigned char)(cylinders & 0xFF);
}

static int write_byte_at(FILE* file, long offset, unsigned char value)
{
    if (fseek(file, offset, SEEK_SET) != 0)
        return -1;
    return fwrite(&value, 1, 1, file) == 1 ? 0 : -1;
}

static int write_le32_at(FILE* file, long offset, unsigned int value)
{
    unsigned char data[4];

    data[0] = (unsigned char)(value & 0xFF);
    data[1] = (unsigned char)((value >> 8) & 0xFF);
    data[2] = (unsigned char)((value >> 16) & 0xFF);
    data[3] = (unsigned char)((value >> 24) & 0xFF);

    if (fseek(file, offset, SEEK_SET) != 0)
        return -1;
    return fwrite(data, sizeof(data), 1, file) == 1 ? 0 : -1;
}

int main(int argc, char* argv[])
{
    const char* output_path = NULL;
    const char* mbr_path = NULL;
    const char* partition_path = NULL;
    unsigned int start_sector = DEFAULT_START_SECTOR;
    unsigned char partition_type = DEFAULT_PARTITION_TYPE;

    FILE* f_output = NULL;
    FILE* f_mbr = NULL;
    FILE* f_partition = NULL;

    unsigned char mbr_sector[SECTOR_SIZE];
    unsigned char partition_vbr[SECTOR_SIZE];
    PARTITION_ENTRY* entry;
    long partition_size;
    long total_size;
    long gap_size;
    long vbr_offset;
    unsigned short bytes_per_sector;
    unsigned short reserved_sectors;
    unsigned short backup_boot_sector;
    unsigned char number_of_fats;
    unsigned int sectors_per_fat;
    unsigned int partition_sectors;
    unsigned int end_sector;
    int i, ret = 1;

    /* Parse arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
            output_path = argv[++i];
        else if (strcmp(argv[i], "-mbr") == 0 && i + 1 < argc)
            mbr_path = argv[++i];
        else if (strcmp(argv[i], "-partition") == 0 && i + 1 < argc)
            partition_path = argv[++i];
        else if (strcmp(argv[i], "-start") == 0 && i + 1 < argc)
            start_sector = (unsigned int)atoi(argv[++i]);
        else if (strcmp(argv[i], "-type") == 0 && i + 1 < argc)
            partition_type = (unsigned char)strtol(argv[++i], NULL, 16);
        else
        {
            fprintf(stderr, "Error: Unknown argument '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!output_path || !mbr_path || !partition_path)
    {
        fprintf(stderr, "Error: Missing required arguments.\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Read MBR boot code */
    f_mbr = fopen(mbr_path, "rb");
    if (!f_mbr)
    {
        fprintf(stderr, "Error: Cannot open MBR file '%s'.\n", mbr_path);
        goto cleanup;
    }

    /* Initialize MBR sector to zero */
    memset(mbr_sector, 0, SECTOR_SIZE);

    /* Read boot code (up to 440 bytes) */
    if (fread(mbr_sector, 1, MBR_BOOT_CODE_SIZE, f_mbr) != MBR_BOOT_CODE_SIZE)
    {
        fprintf(stderr, "Error: Cannot read MBR boot code from '%s'.\n", mbr_path);
        goto cleanup;
    }
    fclose(f_mbr);
    f_mbr = NULL;

    /* Get partition image size */
    f_partition = fopen(partition_path, "rb");
    if (!f_partition)
    {
        fprintf(stderr, "Error: Cannot open partition image '%s'.\n", partition_path);
        goto cleanup;
    }

    fseek(f_partition, 0, SEEK_END);
    partition_size = ftell(f_partition);
    fseek(f_partition, 0, SEEK_SET);

    if (partition_size <= 0 || (partition_size % SECTOR_SIZE) != 0)
    {
        fprintf(stderr, "Error: Partition image size (%ld) is not a multiple of %d.\n",
                partition_size, SECTOR_SIZE);
        goto cleanup;
    }

    if (fread(partition_vbr, 1, sizeof(partition_vbr), f_partition) != sizeof(partition_vbr))
    {
        fprintf(stderr, "Error: Cannot read partition boot sector from '%s'.\n", partition_path);
        goto cleanup;
    }

    if (fseek(f_partition, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error: Cannot rewind partition image '%s'.\n", partition_path);
        goto cleanup;
    }

    partition_sectors = (unsigned int)(partition_size / SECTOR_SIZE);
    if ((unsigned long long)start_sector + partition_sectors - 1 > 0xFFFFFFFFULL)
    {
        fprintf(stderr, "Error: Partition start/size exceeds MBR addressing limits.\n");
        goto cleanup;
    }

    /* Build partition table entry #1 */
    entry = (PARTITION_ENTRY*)(mbr_sector + MBR_PARTITION_TABLE_OFFSET);
    entry->Status = 0x80;  /* Active/bootable */
    entry->Type = partition_type;
    entry->LBAStart = start_sector;
    entry->LBASize = partition_sectors;
    write_chs(start_sector, entry->CHSFirst);
    end_sector = start_sector + partition_sectors - 1;
    write_chs(end_sector, entry->CHSLast);

    /* Write boot signature */
    mbr_sector[MBR_SIGNATURE_OFFSET]     = 0x55;
    mbr_sector[MBR_SIGNATURE_OFFSET + 1] = 0xAA;

    /* Create output image */
    f_output = fopen(output_path, "w+b");
    if (!f_output)
    {
        fprintf(stderr, "Error: Cannot create output file '%s'.\n", output_path);
        goto cleanup;
    }

    /* Write MBR sector */
    if (fwrite(mbr_sector, SECTOR_SIZE, 1, f_output) != 1)
    {
        fprintf(stderr, "Error: Cannot write MBR sector.\n");
        goto cleanup;
    }

    /* Write gap (zero-filled sectors between MBR and partition start) */
    gap_size = ((long)start_sector - 1) * SECTOR_SIZE;
    if (gap_size > 0)
    {
        unsigned char zero[SECTOR_SIZE];
        long remaining = gap_size;
        memset(zero, 0, SECTOR_SIZE);
        while (remaining > 0)
        {
            if (fwrite(zero, SECTOR_SIZE, 1, f_output) != 1)
            {
                fprintf(stderr, "Error: Cannot write gap sectors.\n");
                goto cleanup;
            }
            remaining -= SECTOR_SIZE;
        }
    }

    /* Copy partition image */
    if (copy_file_data(f_output, f_partition, partition_size) != 0)
    {
        fprintf(stderr, "Error: Cannot write partition data.\n");
        goto cleanup;
    }

    bytes_per_sector = read_le16(partition_vbr + FAT_BPB_BYTES_PER_SECTOR_OFFSET);
    reserved_sectors = read_le16(partition_vbr + FAT_BPB_RESERVED_SECTORS_OFFSET);
    number_of_fats = partition_vbr[FAT_BPB_NUMBER_OF_FATS_OFFSET];
    sectors_per_fat = read_le16(partition_vbr + FAT_BPB_SECTORS_PER_FAT16_OFFSET);
    if (sectors_per_fat == 0)
        sectors_per_fat = read_le32(partition_vbr + FAT32_SECTORS_PER_FAT_OFFSET);
    backup_boot_sector = read_le16(partition_vbr + FAT32_BACKUP_BOOT_SECTOR_OFFSET);

    if (bytes_per_sector == 0 || reserved_sectors == 0 || number_of_fats == 0 || sectors_per_fat == 0)
    {
        fprintf(stderr, "Error: Partition image '%s' has an invalid FAT BPB.\n", partition_path);
        goto cleanup;
    }

    vbr_offset = (long)start_sector * SECTOR_SIZE;
    total_size = (long)start_sector * SECTOR_SIZE + partition_size;

    /* Patch HiddenSectors in FAT32 BPB to match partition start */
    if (write_le32_at(f_output, vbr_offset + FAT32_HIDDEN_SECTORS_OFFSET, start_sector) != 0)
    {
        fprintf(stderr, "Error: Cannot patch HiddenSectors in BPB.\n");
        goto cleanup;
    }

    if (backup_boot_sector != 0)
    {
        long backup_vbr_offset = vbr_offset + (long)backup_boot_sector * bytes_per_sector;

        if (backup_vbr_offset + FAT32_HIDDEN_SECTORS_OFFSET + 4 > vbr_offset + partition_size ||
            write_le32_at(f_output, backup_vbr_offset + FAT32_HIDDEN_SECTORS_OFFSET, start_sector) != 0)
        {
            fprintf(stderr, "Error: Cannot patch HiddenSectors in backup BPB.\n");
            goto cleanup;
        }
    }

    /* Patch MediaDescriptor to 0xF8 (fixed disk) -- FatFs may set 0xF0 (removable) */
    {
        unsigned char media_byte = MEDIA_DESCRIPTOR_FIXED;
        long fat_offset;
        long fat_size;
        unsigned int fat_index;

        /* Primary VBR */
        if (write_byte_at(f_output, vbr_offset + FAT32_MEDIA_DESCRIPTOR_OFFSET, media_byte) != 0)
        {
            fprintf(stderr, "Error: Cannot patch media descriptor in BPB.\n");
            goto cleanup;
        }

        if (backup_boot_sector != 0)
        {
            long backup_vbr_offset = vbr_offset + (long)backup_boot_sector * bytes_per_sector;

            if (backup_vbr_offset + FAT32_MEDIA_DESCRIPTOR_OFFSET + 1 > vbr_offset + partition_size ||
                write_byte_at(f_output, backup_vbr_offset + FAT32_MEDIA_DESCRIPTOR_OFFSET, media_byte) != 0)
            {
                fprintf(stderr, "Error: Cannot patch media descriptor in backup BPB.\n");
                goto cleanup;
            }
        }

        fat_offset = vbr_offset + (long)reserved_sectors * bytes_per_sector;
        fat_size = (long)sectors_per_fat * bytes_per_sector;

        for (fat_index = 0; fat_index < number_of_fats; fat_index++)
        {
            long current_fat_offset = fat_offset + (long)fat_index * fat_size;

            if (current_fat_offset + 1 > vbr_offset + partition_size ||
                write_byte_at(f_output, current_fat_offset, media_byte) != 0)
            {
                fprintf(stderr, "Error: Cannot patch FAT%u media descriptor.\n", fat_index + 1);
                goto cleanup;
            }
        }
    }

    /* Patch BootDrive to 0xFF (auto-detect from BIOS DL register) */
    {
        unsigned char boot_drive = BOOT_DRIVE_AUTO;

        if (write_byte_at(f_output, vbr_offset + FAT32_BOOT_DRIVE_OFFSET, boot_drive) != 0)
        {
            fprintf(stderr, "Error: Cannot patch BootDrive in BPB.\n");
            goto cleanup;
        }

        if (backup_boot_sector != 0)
        {
            long backup_vbr_offset = vbr_offset + (long)backup_boot_sector * bytes_per_sector;

            if (backup_vbr_offset + FAT32_BOOT_DRIVE_OFFSET + 1 > vbr_offset + partition_size ||
                write_byte_at(f_output, backup_vbr_offset + FAT32_BOOT_DRIVE_OFFSET, boot_drive) != 0)
            {
                fprintf(stderr, "Error: Cannot patch BootDrive in backup BPB.\n");
                goto cleanup;
            }
        }
    }

    printf("Created disk image '%s' (%ld bytes, partition at sector %u)\n",
           output_path, total_size, start_sector);
    ret = 0;

cleanup:
    if (f_output) fclose(f_output);
    if (f_mbr) fclose(f_mbr);
    if (f_partition) fclose(f_partition);
    return ret;
}
