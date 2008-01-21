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
VOID LoadFont(PBYTE Bitplane, PUCHAR FontBitfield);

/* FUNCTIONS ****************************************************************/

VOID
ScrLoadFontTable(UINT CodePage)
{
    PHYSICAL_ADDRESS BaseAddress;
    PBYTE Bitplane;
    PUCHAR FontBitfield = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    FontBitfield = (PUCHAR) ExAllocatePoolWithTag(NonPagedPool, 2048, TAG_BLUE);
    if(FontBitfield)
    {
        /* open bit plane for font table access */
        OpenBitPlane();

        /* get pointer to video memory */
        BaseAddress.QuadPart = BITPLANE_BASE;
        Bitplane = (PBYTE)MmMapIoSpace (BaseAddress, 0xFFFF, MmNonCached);

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

NTSTATUS ExtractFont(UINT CodePage, PUCHAR FontBitField)
{
    HANDLE             Handle;
    NTSTATUS           Status = STATUS_SUCCESS;
    CHAR               FileHeader[5];
    CHAR               Header[5];
    CHAR               FileName[BUFFER_SIZE];
    ULONG              Length;
    IO_STATUS_BLOCK    IoStatusBlock;
    OBJECT_ATTRIBUTES  ObjectAttributes;
    UNICODE_STRING     LinkName;
    UNICODE_STRING     SourceName;
    ZIP_LOCAL_HEADER   LocalHeader;
    LARGE_INTEGER      ByteOffset;
    WCHAR              SourceBuffer[MAX_PATH] = {L'\0'};

    if(KeGetCurrentIrql() != PASSIVE_LEVEL)
        return STATUS_INVALID_DEVICE_STATE; 

    RtlZeroMemory(FileHeader, sizeof(FileHeader));
    RtlZeroMemory(Header, sizeof(Header));

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
    SourceName.MaximumLength = MAX_PATH * sizeof(WCHAR);
    SourceName.Buffer = SourceBuffer;

    Status = ZwQuerySymbolicLinkObject(Handle,
                                      &SourceName,
                                      &Length);
    ZwClose(Handle);

    Status = RtlAppendUnicodeToString(&SourceName, L"\\vgafont.bin");
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
    if(NT_SUCCESS(Status)) {
        sprintf(Header, "PK%c%c", 3, 4);

        Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                              FileHeader, 4, &ByteOffset, NULL);
        ByteOffset.LowPart += 4;

        if(NT_SUCCESS(Status))
        {
            while(strcmp(FileHeader, Header) == 0)
            {
                Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                              &LocalHeader, sizeof(ZIP_LOCAL_HEADER), &ByteOffset, NULL);
                ByteOffset.LowPart += sizeof(ZIP_LOCAL_HEADER);
                if (LocalHeader.FileNameLength < BUFFER_SIZE)
                {
                    RtlZeroMemory(FileName, BUFFER_SIZE);
                    Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                              FileName, LocalHeader.FileNameLength, &ByteOffset, NULL);
                }
                ByteOffset.LowPart += LocalHeader.FileNameLength;
                if (LocalHeader.ExtraFieldLength > 0)
                    ByteOffset.LowPart += LocalHeader.ExtraFieldLength;
                if (atoi(FileName) == CodePage)
                {
                    if (LocalHeader.CompressedSize == 2048)
                        Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                              FontBitField, LocalHeader.CompressedSize, &ByteOffset, NULL);
                    ZwClose(Handle);
                    return STATUS_SUCCESS;
                }
                ByteOffset.LowPart += LocalHeader.CompressedSize;
                Status = ZwReadFile(Handle, NULL, NULL, NULL, &IoStatusBlock,
                              FileHeader, 4, &ByteOffset, NULL);
                ByteOffset.LowPart += 4;
                DbgPrint("%s\n", FileHeader);
            }
        }
        ZwClose(Handle);
    }
    else
    {
        DbgPrint("Error: Can not open vgafont.bin\n");
        return Status;
    }
    return STATUS_NO_MATCH;
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
LoadFont(PBYTE Bitplane, PUCHAR FontBitfield)
{
    UINT i,j;

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

