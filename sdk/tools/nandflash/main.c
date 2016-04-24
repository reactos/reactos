/*
 * PROJECT:         OMAP3 NAND Flashing Utility
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            tools/nandflash/main.c
 * PURPOSE:         Flashes OmapLDR, FreeLDR and a Root FS into a NAND image
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "nandflash.h"

/* GLOBALS ********************************************************************/

/* File Names */
PCHAR NandImageName       = "reactos.bin";
PCHAR LlbImageName        = "./output-arm/boot/armllb/armllb.bin";
PCHAR BootLdrImageName    = "./output-arm/boot/freeldr/freeldr/freeldr.sys";
PCHAR FsImageName         = "ramdisk.img";
PCHAR RamImageName        = "ramdisk.bin";

/* NAND On-Disk Memory Map */
ULONG LlbStart     = 0x00000000,     LlbEnd = 0x00010000;   // 64  KB
ULONG BootLdrStart = 0x00010000, BootLdrEnd = 0x00090000;   // 512 KB
ULONG FsStart      = 0x00090000,      FsEnd = 0x10000000;   // 255 MB

/* Should we write OOB data? */
ULONG NeedsOob = TRUE;

/* FUNCTIONS ******************************************************************/

INT
NTAPI
CreateFlashFile(VOID)
{
    INT FileDescriptor, i;
    CHAR Buffer[NAND_PAGE_SIZE + NAND_OOB_SIZE];

    /* Try open NAND image */
    FileDescriptor = open(NandImageName, O_RDWR);
    if (FileDescriptor)
    {
        /* Create NAND image */
        FileDescriptor = open(NandImageName, O_RDWR | O_CREAT);
        if (FileDescriptor) return FileDescriptor;

        /* Create zero buffer */
        memset(Buffer, 0xff, sizeof(Buffer));

        /* Write zero buffer */
        for (i = 0; i < NAND_PAGES; i++) write(FileDescriptor, Buffer, sizeof(Buffer));
    }

    /* Return NAND descriptor */
    return FileDescriptor;
}

VOID
NTAPI
WriteToFlash(IN INT NandImageFile,
             IN INT ImageFile,
             IN ULONG ImageStart,
             IN ULONG ImageEnd)
{
    CHAR Data[NAND_PAGE_SIZE], Oob[NAND_OOB_SIZE];
    ULONG StartPage, EndPage, i, OobSize = 0;
    BOOLEAN KeepGoing = TRUE;

    /* Offset to NAND Page convert */
    StartPage = ImageStart / NAND_PAGE_SIZE;
    EndPage = ImageEnd / NAND_PAGE_SIZE;

    /* Jump to NAND offset */
    if (NeedsOob) OobSize = NAND_OOB_SIZE;
    lseek(NandImageFile, StartPage * (NAND_PAGE_SIZE + OobSize), SEEK_SET);

    /* Set input image offset */
    lseek(ImageFile, 0, SEEK_SET);

    /* Create zero buffer */
    memset(Data, 0xff, NAND_PAGE_SIZE);
    memset(Oob, 0xff, NAND_OOB_SIZE);

    /* Parse NAND Pages */
    for (i = StartPage; i < EndPage; i++)
    {
        /* Read NAND page from input image */
        if (read(ImageFile, Data, NAND_PAGE_SIZE) < NAND_PAGE_SIZE)
        {
            /* Do last write and quit after */
            KeepGoing = FALSE;
        }

        /* Write OOB and NAND Data */
        write(NandImageFile, Data, NAND_PAGE_SIZE);
        if (NeedsOob) write(NandImageFile, Oob, NAND_OOB_SIZE);

        /* Next page if data continues */
        if (!KeepGoing) break;
    }
}

VOID
NTAPI
WriteLlb(IN INT NandImageFile)
{
    INT FileDescriptor;

    /* Open LLB and write it */
    FileDescriptor = open(LlbImageName, O_RDWR);
    WriteToFlash(NandImageFile, FileDescriptor, LlbStart, LlbEnd);
    close(FileDescriptor);
}

VOID
NTAPI
WriteBootLdr(IN INT NandImageFile)
{
    INT FileDescriptor;

    /* Open FreeLDR and write it */
    FileDescriptor = open(BootLdrImageName, O_RDWR);
    WriteToFlash(NandImageFile, FileDescriptor, BootLdrStart, BootLdrEnd);
    close(FileDescriptor);
}

VOID
NTAPI
WriteFileSystem(IN INT NandImageFile)
{
    INT FileDescriptor;

    /* Open FS image and write it */
    FileDescriptor = open(FsImageName, O_RDWR);
    WriteToFlash(NandImageFile, FileDescriptor, FsStart, FsEnd);
    close(FileDescriptor);
}

VOID
NTAPI
WriteRamDisk(IN INT RamDiskFile)
{
    INT FileDescriptor;

    /* Open FS image and write it 16MB later */
    FileDescriptor = open(FsImageName, O_RDWR);
    WriteToFlash(RamDiskFile, FileDescriptor, 16 * 1024 * 1024, (32 + 16) * 1024 * 1024);
    close(FileDescriptor);
}

int
main(ULONG argc,
     char **argv)
{
    INT NandImageFile, RamDiskFile;

    /* Flat NAND, no OOB */
    if (argc == 2) NeedsOob = FALSE;

    /* Open or create NAND Image File */
    NandImageFile = CreateFlashFile();
    if (!NandImageFile) exit(-1);

    /* Write components */
    WriteLlb(NandImageFile);
    WriteBootLdr(NandImageFile);
    if (NeedsOob)
    {
        /* Write the ramdisk normaly */
        WriteFileSystem(NandImageFile);
    }
    else
    {
        /* Open a new file for the ramdisk */
        RamDiskFile = open(RamImageName, O_RDWR | O_CREAT);
        if (!RamDiskFile) exit(-1);

        /* Write it */
        WriteRamDisk(RamDiskFile);

        /* Close */
        close(RamDiskFile);
    }


    /* Close and return */
    close(NandImageFile);
    return 0;
}

/* EOF */
