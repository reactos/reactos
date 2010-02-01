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
PCHAR BootLdrImageName    = "./output-arm/boot/freeldr/freeldr.sys";
PCHAR FsImageName         = "ReactOS.img";

/* NAND On-Disk Memory Map */
ULONG LlbStart     = 0x00000000,     LlbEnd = 0x00080000;
ULONG BootLdrStart = 0x00280000, BootLdrEnd = 0x00680000;
ULONG FsStart      = 0x00300000,      FsEnd = 0x10000000;

/* FUNCTIONS ******************************************************************/

ULONG
NTAPI
CreateFlashFile(VOID)
{
    ULONG FileDescriptor, i;
    CHAR Buffer[NAND_PAGE_SIZE + NAND_OOB_SIZE];

    /* Try open NAND image */
    FileDescriptor = open(NandImageName, O_RDWR);
    if (!FileDescriptor)
    {
        /* Create NAND image */
        FileDescriptor = open(NandImageName, O_RDWR | O_CREAT);
        if (!FileDescriptor) return (-1);

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
WriteToFlash(IN ULONG NandImageFile,
             IN ULONG ImageFile,
             IN ULONG ImageStart,
             IN ULONG ImageEnd)
{
    CHAR Data[NAND_PAGE_SIZE], Oob[NAND_OOB_SIZE];
    ULONG StartPage, EndPage, i;
    BOOLEAN KeepGoing = TRUE;

    /* Offset to NAND Page convert */
    StartPage = ImageStart / NAND_PAGE_SIZE;
    EndPage = ImageEnd / NAND_PAGE_SIZE;

    /* Jump to NAND offset */
    lseek(NandImageFile, StartPage * (NAND_PAGE_SIZE + NAND_OOB_SIZE), SEEK_SET);

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
        write(NandImageFile, Oob, NAND_OOB_SIZE);

        /* Next page if data continues */
        if (!KeepGoing) break;
    }
}

VOID
NTAPI
WriteLlb(IN ULONG NandImageFile)
{
    ULONG FileDescriptor;

    /* Open LLB and write it */
    FileDescriptor = open(LlbImageName, O_RDWR);
    WriteToFlash(NandImageFile, FileDescriptor, LlbStart, LlbEnd);
    close(FileDescriptor);
}

VOID
NTAPI
WriteBootLdr(IN ULONG NandImageFile)
{
    ULONG FileDescriptor;

    /* Open FreeLDR and write it */
    FileDescriptor = open(BootLdrImageName, O_RDWR);
    WriteToFlash(NandImageFile, FileDescriptor, BootLdrStart, BootLdrEnd);
    close(FileDescriptor);
}

VOID
NTAPI
WriteFileSystem(IN ULONG NandImageFile)
{
    ULONG FileDescriptor;

    /* Open FS image and write it */
    FileDescriptor = open(FsImageName, O_RDWR);
    WriteToFlash(NandImageFile, FileDescriptor, FsStart, FsEnd);
    close(FileDescriptor);
}

int
main(ULONG argc,
     char **argv)
{
    ULONG NandImageFile;

    /* Open or create NAND Image File */
    NandImageFile = CreateFlashFile();
    if (!NandImageFile) exit(-1);

    /* Write components */
    WriteLlb(NandImageFile);
    WriteBootLdr(NandImageFile);
    WriteFileSystem(NandImageFile);

    /* Close and return */
    close(NandImageFile);
    return 0;
}

/* EOF */
