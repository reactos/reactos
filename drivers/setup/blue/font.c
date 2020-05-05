/*
 * PROJECT:     ReactOS Console Text-Mode Device Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Loading specific fonts into VGA.
 * COPYRIGHT:   Copyright 2008-2019 Aleksey Bragin (aleksey@reactos.org)
 *              Copyright 2008-2019 Colin Finck (mail@colinfinck.de)
 *              Copyright 2008-2019 Christoph von Wittich (christoph_vw@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include "blue.h"
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

NTSTATUS ExtractFont(_In_ ULONG CodePage, _In_ PUCHAR FontBitField);
VOID OpenBitPlane(VOID);
VOID CloseBitPlane(VOID);
VOID LoadFont(_In_ PUCHAR Bitplane, _In_ PUCHAR FontBitfield);

/* FUNCTIONS ****************************************************************/

VOID
ScrLoadFontTable(
    _In_ ULONG CodePage)
{
    PHYSICAL_ADDRESS BaseAddress;
    PUCHAR Bitplane;
    PUCHAR FontBitfield = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    FontBitfield = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, 2048, TAG_BLUE);
    if (FontBitfield == NULL)
    {
        DPRINT1("ExAllocatePoolWithTag failed\n");
        return;
    }

    /* open bit plane for font table access */
    OpenBitPlane();

    /* get pointer to video memory */
    BaseAddress.QuadPart = BITPLANE_BASE;
    Bitplane = (PUCHAR)MmMapIoSpace(BaseAddress, 0xFFFF, MmNonCached);

    Status = ExtractFont(CodePage, FontBitfield);
    if (NT_SUCCESS(Status))
    {
        LoadFont(Bitplane, FontBitfield);
    }
    else
    {
        DPRINT1("ExtractFont failed with Status 0x%lx\n", Status);
    }

    MmUnmapIoSpace(Bitplane, 0xFFFF);
    ExFreePoolWithTag(FontBitfield, TAG_BLUE);

    /* close bit plane */
    CloseBitPlane();
}

VOID
ScrSetFont(
    _In_ PUCHAR FontBitfield)
{
    PHYSICAL_ADDRESS BaseAddress;
    PUCHAR Bitplane;

    /* open bit plane for font table access */
    OpenBitPlane();

    /* get pointer to video memory */
    BaseAddress.QuadPart = BITPLANE_BASE;
    Bitplane = (PUCHAR)MmMapIoSpace(BaseAddress, 0xFFFF, MmNonCached);

    LoadFont(Bitplane, FontBitfield);

    MmUnmapIoSpace(Bitplane, 0xFFFF);

    /* close bit plane */
    CloseBitPlane();
}

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
ExtractFont(
    _In_ ULONG CodePage,
    _In_ PUCHAR FontBitField)
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
    WCHAR              SourceBuffer[MAX_PATH] = { L'\0' };
    ULONG              ReadCP;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        return STATUS_INVALID_DEVICE_STATE;

    RtlInitUnicodeString(&LinkName,
                         L"\\SystemRoot");

    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenSymbolicLinkObject(&Handle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenSymbolicLinkObject failed with Status 0x%lx\n", Status);
        return Status;
    }

    SourceName.Length = 0;
    SourceName.MaximumLength = MAX_PATH * sizeof(WCHAR);
    SourceName.Buffer = SourceBuffer;

    Status = ZwQuerySymbolicLinkObject(Handle,
                                       &SourceName,
                                       NULL);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQuerySymbolicLinkObject failed with Status 0x%lx\n", Status);
        return Status;
    }

    Status = RtlAppendUnicodeToString(&SourceName, L"\\vgafonts.cab");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAppendUnicodeToString failed with Status 0x%lx\n", Status);
        return Status;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &SourceName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateFile(&Handle,
                          GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN, 
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Cannot open vgafonts.cab (0x%lx)\n", Status);
        return Status;
    }

    ByteOffset.QuadPart = 0;
    Status = ZwReadFile(Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &CabFileHeader,
                        sizeof(CabFileHeader),
                        &ByteOffset,
                        NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Cannot read from file (0x%lx)\n", Status);
        goto Exit;
    }

    if (CabFileHeader.Signature != CAB_SIGNATURE)
    {
        DPRINT1("Invalid CAB signature: 0x%lx!\n", CabFileHeader.Signature);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    // We have a valid CAB file!
    // Read the file table now and decrement the file count on every file. When it's zero, we read the complete table.
    ByteOffset.QuadPart = CabFileHeader.FileTableOffset;

    while (CabFileHeader.FileCount)
    {
        Status = ZwReadFile(Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &CabFile,
                            sizeof(CabFile),
                            &ByteOffset,
                            NULL);

        if (NT_SUCCESS(Status))
        {
            ByteOffset.QuadPart += sizeof(CabFile);

            // We assume here that the file name is max. 19 characters (+ 1 NULL character) long.
            // This should be enough for our purpose.
            Status = ZwReadFile(Handle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FileName,
                                sizeof(FileName),
                                &ByteOffset,
                                NULL);

            if (NT_SUCCESS(Status))
            {
                if (!bFoundFile)
                {
                    Status = RtlCharToInteger(FileName, 0, &ReadCP);
                    if (NT_SUCCESS(Status) && ReadCP == CodePage)
                    {
                        // We got the correct file.
                        // Save the offset and loop through the rest of the file table to find the position, where the actual data starts.
                        CabFileOffset = CabFile.FileOffset;
                        bFoundFile = TRUE;
                    }
                }

                ByteOffset.QuadPart += strlen(FileName) + 1;
            }
        }

        CabFileHeader.FileCount--;
    }

    // 8 = Size of a CFFOLDER structure (see cabman). As we don't need the values of that structure, just increase the offset here.
    ByteOffset.QuadPart += 8;
    ByteOffset.QuadPart += CabFileOffset;

    // ByteOffset now contains the offset of the actual data, so we can read the RAW font
    Status = ZwReadFile(Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FontBitField,
                        2048,
                        &ByteOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwReadFile failed with Status 0x%lx\n", Status);
    }

Exit:

    ZwClose(Handle);
    return Status;
}

/* Font-load specific funcs */
VOID
OpenBitPlane(VOID)
{
    /* disable interrupts */
    _disable();

    /* sequence reg */
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x01);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_ENABLE_WRT_PLANE); WRITE_PORT_UCHAR(SEQ_DATA, 0x04);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_MEM_MODE); WRITE_PORT_UCHAR(SEQ_DATA, 0x07);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);

    /* graphic reg */
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_READ_PLANE); WRITE_PORT_UCHAR(GCT_DATA, 0x02);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_RW_MODES); WRITE_PORT_UCHAR(GCT_DATA, 0x00);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_GRAPH_MODE); WRITE_PORT_UCHAR(GCT_DATA, 0x00);

    /* enable interrupts */
    _enable();
}

VOID
CloseBitPlane(VOID)
{
    /* disable interrupts */
    _disable();

    /* sequence reg */
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x01);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_ENABLE_WRT_PLANE); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_MEM_MODE); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);

    /* graphic reg */
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_READ_PLANE); WRITE_PORT_UCHAR(GCT_DATA, 0x00);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_RW_MODES); WRITE_PORT_UCHAR(GCT_DATA, 0x10);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_GRAPH_MODE); WRITE_PORT_UCHAR(GCT_DATA, 0x0e);

    /* enable interrupts */
    _enable();
}

VOID
LoadFont(
    _In_ PUCHAR Bitplane,
    _In_ PUCHAR FontBitfield)
{
    UINT32 i, j;

    for (i = 0; i < 256; i++)
    {
        for (j = 0; j < 8; j++)
        {
            *Bitplane = FontBitfield[i * 8 + j];
            Bitplane++;
        }

        // padding
        for (j = 8; j < 32; j++)
        {
            *Bitplane = 0;
            Bitplane++;
        }
    }
}
