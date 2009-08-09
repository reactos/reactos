/*
* PROJECT:         ReactOS Setup Driver
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/setup/blue/font.c
* PURPOSE:         Loading specific fonts into VGA
* PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
*                  Colin Finck (mail@colinfinck.de)
*                  Christoph von Wittich (christoph_vw@reactos.org)
*/

/* INCLUDES ***************************************************************/

#include <ntddk.h>
#include "blue.h"

#define NDEBUG
#include <debug.h>

VOID OpenBitPlane();
VOID CloseBitPlane();
VOID LoadFont(PUCHAR Bitplane, PUCHAR FontBitfield);

/* FUNCTIONS ****************************************************************/

VOID
ScrLoadFontTable(UINT32 CodePage)
{
    PHYSICAL_ADDRESS BaseAddress;
    PUCHAR Bitplane;
    PUCHAR FontBitfield = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    FontBitfield = (PUCHAR) ExAllocatePoolWithTag(NonPagedPool, 2048, TAG_BLUE);
    if(FontBitfield)
    {
        /* open bit plane for font table access */
        OpenBitPlane();

        /* get pointer to video memory */
        BaseAddress.QuadPart = BITPLANE_BASE;
        Bitplane = (PUCHAR)MmMapIoSpace (BaseAddress, 0xFFFF, MmNonCached);

        Status = ExtractFont(CodePage, FontBitfield);
        if (NT_SUCCESS(Status))
            LoadFont(Bitplane, FontBitfield);

        MmUnmapIoSpace(Bitplane, 0xFFFF);
        ExFreePool(FontBitfield);

        /* close bit plane */
        CloseBitPlane();
    }
}

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS ExtractFont(UINT32 CodePage, PUCHAR FontBitField)
{
    BOOLEAN            bFoundFile = FALSE;
    HANDLE             Handle;
    NTSTATUS           Status;
    CHAR               FileName[20];
    IO_STATUS_BLOCK    IoStatusBlock;
    OBJECT_ATTRIBUTES  ObjectAttributes;
    UNICODE_STRING     LinkName;
    UNICODE_STRING     SourceName;
    CFHEADER           CabFileHeader;
    CFFILE             CabFile;
    ULONG              CabFileOffset = 0;
    LARGE_INTEGER      ByteOffset;
    WCHAR              SourceBuffer[_MAX_PATH] = {L'\0'};

    if(KeGetCurrentIrql() != PASSIVE_LEVEL)
        return STATUS_INVALID_DEVICE_STATE;

    RtlInitUnicodeString(&LinkName,
                         L"\\SystemRoot");

    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenSymbolicLinkObject(&Handle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);

    if (!NT_SUCCESS(Status))
        return(Status);

    SourceName.Length = 0;
    SourceName.MaximumLength = _MAX_PATH * sizeof(WCHAR);
    SourceName.Buffer = SourceBuffer;

    Status = ZwQuerySymbolicLinkObject(Handle,
                                      &SourceName,
                                      NULL);
    ZwClose(Handle);

    Status = RtlAppendUnicodeToString(&SourceName, L"\\vgafonts.cab");
    InitializeObjectAttributes(&ObjectAttributes, &SourceName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);

    Status = ZwCreateFile(&Handle,
                          GENERIC_READ,
                          &ObjectAttributes, &IoStatusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN, 
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL, 0);

    ByteOffset.LowPart = ByteOffset.HighPart = 0;

    if(NT_SUCCESS(Status))
    {
        Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                            &CabFileHeader, sizeof(CabFileHeader), &ByteOffset, NULL);

        if(NT_SUCCESS(Status))
        {
            if(CabFileHeader.Signature == CAB_SIGNATURE)
            {
                // We have a valid CAB file!
                // Read the file table now and decrement the file count on every file. When it's zero, we read the complete table.
                ByteOffset.LowPart = CabFileHeader.FileTableOffset;

                while(CabFileHeader.FileCount)
                {
                    Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                                        &CabFile, sizeof(CabFile), &ByteOffset, NULL);

                    if(NT_SUCCESS(Status))
                    {
                        ByteOffset.LowPart += sizeof(CabFile);

                        // We assume here that the file name is max. 19 characters (+ 1 NULL character) long.
                        // This should be enough for our purpose.
                        Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                                            FileName, sizeof(FileName), &ByteOffset, NULL);

                        if(NT_SUCCESS(Status))
                        {
                            if(!bFoundFile && (UINT32)atoi(FileName) == CodePage)
                            {
                                // We got the correct file.
                                // Save the offset and loop through the rest of the file table to find the position, where the actual data starts.
                                CabFileOffset = CabFile.FileOffset;
                                bFoundFile = TRUE;
                            }

                            ByteOffset.LowPart += strlen(FileName) + 1;
                        }
                    }

                    CabFileHeader.FileCount--;
                }

                // 8 = Size of a CFFOLDER structure (see cabman). As we don't need the values of that structure, just increase the offset here.
                ByteOffset.LowPart += 8;
                ByteOffset.LowPart += CabFileOffset;

                // ByteOffset now contains the offset of the actual data, so we can read the RAW font
                Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                                    FontBitField, 2048, &ByteOffset, NULL);
                ZwClose(Handle);
                return STATUS_SUCCESS;
            }
            else
            {
                DPRINT1("Error: CAB signature is missing!\n");
                Status = STATUS_UNSUCCESSFUL;
            }
        }
        else
            DPRINT1("Error: Cannot read from file\n");

        ZwClose(Handle);
        return Status;
    }
    else
    {
        DPRINT1("Error: Cannot open vgafonts.cab\n");
        return Status;
    }
}

/* Font-load specific funcs */
VOID
OpenBitPlane()
{
    /* disable interrupts */
    _disable();

    /* sequence reg */
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR (SEQ_DATA, 0x01);
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_ENABLE_WRT_PLANE); WRITE_PORT_UCHAR (SEQ_DATA, 0x04);
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_MEM_MODE); WRITE_PORT_UCHAR (SEQ_DATA, 0x07);
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR (SEQ_DATA, 0x03);

    /* graphic reg */
    WRITE_PORT_UCHAR (GCT_COMMAND, GCT_READ_PLANE); WRITE_PORT_UCHAR (GCT_DATA, 0x02);
    WRITE_PORT_UCHAR (GCT_COMMAND, GCT_RW_MODES); WRITE_PORT_UCHAR (GCT_DATA, 0x00);
    WRITE_PORT_UCHAR (GCT_COMMAND, GCT_GRAPH_MODE); WRITE_PORT_UCHAR (GCT_DATA, 0x00);

    /* enable interrupts */
    _enable();
}

VOID
CloseBitPlane()
{
    /* disable interrupts */
    _disable();

    /* sequence reg */
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR (SEQ_DATA, 0x01);
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_ENABLE_WRT_PLANE); WRITE_PORT_UCHAR (SEQ_DATA, 0x03);
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_MEM_MODE); WRITE_PORT_UCHAR (SEQ_DATA, 0x03);
    WRITE_PORT_UCHAR (SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR (SEQ_DATA, 0x03);

    /* graphic reg */
    WRITE_PORT_UCHAR (GCT_COMMAND, GCT_READ_PLANE); WRITE_PORT_UCHAR (GCT_DATA, 0x00);
    WRITE_PORT_UCHAR (GCT_COMMAND, GCT_RW_MODES); WRITE_PORT_UCHAR (GCT_DATA, 0x10);
    WRITE_PORT_UCHAR (GCT_COMMAND, GCT_GRAPH_MODE); WRITE_PORT_UCHAR (GCT_DATA, 0x0e);

    /* enable interrupts */
    _enable();
}

VOID
LoadFont(PUCHAR Bitplane, PUCHAR FontBitfield)
{
    UINT32 i,j;

    for (i=0; i<256; i++)
    {
        for (j=0; j<8; j++)
        {
            *Bitplane = FontBitfield[i*8+j];
            Bitplane++;
        }

        // padding
        for (j=8; j<32; j++)
        {
            *Bitplane = 0;
            Bitplane++;
        }
    }
}

