/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003, 2004, 2005 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/partlist.c
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* HELPERS FOR PARTITION TYPES **********************************************/

typedef struct _PARTITION_TYPE
{
    UCHAR Type;
    PCHAR Description;
} PARTITION_TYPE, *PPARTITION_TYPE;

/*
 * This partition type list was ripped off the kernelDisk.c module from:
 *
 * Visopsys Operating System
 * Copyright (C) 1998-2015 J. Andrew McLaughlin
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * See also https://en.wikipedia.org/wiki/Partition_type#List_of_partition_IDs
 * and http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
 * for a complete list.
 */

/* This is a table for keeping known partition type codes and descriptions */
static PARTITION_TYPE PartitionTypes[] =
{
    { 0x00, "(Empty)" },
    { 0x01, "FAT12" },
    { 0x02, "XENIX root" },
    { 0x03, "XENIX /usr" },
    { 0x04, "FAT16 (small)" },
    { 0x05, "Extended" },
    { 0x06, "FAT16" },
    { 0x07, "NTFS/HPFS/exFAT" },
    { 0x08, "OS/2 or AIX boot" },
    { 0x09, "AIX data" },
    { 0x0A, "OS/2 Boot Manager" },
    { 0x0B, "FAT32" },
    { 0x0C, "FAT32 (LBA)" },
    { 0x0E, "FAT16 (LBA)" },
    { 0x0F, "Extended (LBA)" },
    { 0x11, "Hidden FAT12" },
    { 0x12, "FAT diagnostic" },
    { 0x14, "Hidden FAT16 (small)" },
    { 0x16, "Hidden FAT16" },
    { 0x17, "Hidden HPFS or NTFS" },
    { 0x1B, "Hidden FAT32" },
    { 0x1C, "Hidden FAT32 (LBA)" },
    { 0x1E, "Hidden FAT16 (LBA)" },
    { 0x35, "JFS" },
    { 0x39, "Plan 9" },
    { 0x3C, "PartitionMagic" },
    { 0x3D, "Hidden Netware" },
    { 0x41, "PowerPC PReP" },
    { 0x42, "Win2K dynamic extended" },
    { 0x43, "Old Linux" },
    { 0x44, "GoBack" },
    { 0x4D, "QNX4.x" },
    { 0x4D, "QNX4.x 2nd" },
    { 0x4D, "QNX4.x 3rd" },
    { 0x50, "Ontrack R/O" },
    { 0x51, "Ontrack R/W or Novell" },
    { 0x52, "CP/M" },
    { 0x63, "GNU HURD or UNIX SysV" },
    { 0x64, "Netware 2" },
    { 0x65, "Netware 3/4" },
    { 0x66, "Netware SMS" },
    { 0x67, "Novell" },
    { 0x68, "Novell" },
    { 0x69, "Netware 5+" },
    { 0x7E, "Veritas VxVM public" },
    { 0x7F, "Veritas VxVM private" },
    { 0x80, "Minix" },
    { 0x81, "Linux or Minix" },
    { 0x82, "Linux swap or Solaris" },
    { 0x83, "Linux" },
    { 0x84, "Hibernation" },
    { 0x85, "Linux extended" },
    { 0x86, "HPFS or NTFS mirrored" },
    { 0x87, "HPFS or NTFS mirrored" },
    { 0x8E, "Linux LVM" },
    { 0x93, "Hidden Linux" },
    { 0x9F, "BSD/OS" },
    { 0xA0, "Laptop hibernation" },
    { 0xA1, "Laptop hibernation" },
    { 0xA5, "BSD, NetBSD, FreeBSD" },
    { 0xA6, "OpenBSD" },
    { 0xA7, "NeXTSTEP" },
    { 0xA8, "OS-X UFS" },
    { 0xA9, "NetBSD" },
    { 0xAB, "OS-X boot" },
    { 0xAF, "OS-X HFS" },
    { 0xB6, "NT corrupt mirror" },
    { 0xB7, "BSDI" },
    { 0xB8, "BSDI swap" },
    { 0xBE, "Solaris 8 boot" },
    { 0xBF, "Solaris x86" },
    { 0xC0, "NTFT" },
    { 0xC1, "DR-DOS FAT12" },
    { 0xC2, "Hidden Linux" },
    { 0xC3, "Hidden Linux swap" },
    { 0xC4, "DR-DOS FAT16 (small)" },
    { 0xC5, "DR-DOS Extended" },
    { 0xC6, "DR-DOS FAT16" },
    { 0xC7, "HPFS mirrored" },
    { 0xCB, "DR-DOS FAT32" },
    { 0xCC, "DR-DOS FAT32 (LBA)" },
    { 0xCE, "DR-DOS FAT16 (LBA)" },
    { 0xD0, "MDOS" },
    { 0xD1, "MDOS FAT12" },
    { 0xD4, "MDOS FAT16 (small)" },
    { 0xD5, "MDOS Extended" },
    { 0xD6, "MDOS FAT16" },
    { 0xD8, "CP/M-86" },
    { 0xDF, "BootIt EMBRM(FAT16/32)" },
    { 0xEB, "BeOS BFS" },
    { 0xEE, "EFI GPT protective" },
    { 0xEF, "EFI filesystem" },
    { 0xF0, "Linux/PA-RISC boot" },
    { 0xF2, "DOS 3.3+ second" },
    { 0xFA, "Bochs" },
    { 0xFB, "VmWare" },
    { 0xFC, "VmWare swap" },
    { 0xFD, "Linux RAID" },
    { 0xFE, "NT hidden" },
};

VOID
GetPartTypeStringFromPartitionType(
    IN UCHAR partitionType,
    OUT PCHAR strPartType,
    IN ULONG cchPartType)
{
    /* Determine partition type */

    if (IsContainerPartition(partitionType))
    {
        StringCchCopyA(strPartType, cchPartType, MUIGetString(STRING_EXTENDED_PARTITION));
    }
    else if (partitionType == PARTITION_ENTRY_UNUSED)
    {
        StringCchCopyA(strPartType, cchPartType, MUIGetString(STRING_FORMATUNUSED));
    }
    else
    {
        UINT i;

        /* Do the table lookup */
        for (i = 0; i < ARRAYSIZE(PartitionTypes); i++)
        {
            if (partitionType == PartitionTypes[i].Type)
            {
                StringCchCopyA(strPartType, cchPartType, PartitionTypes[i].Description);
                return;
            }
        }

        /* We are here because the partition type is unknown */
        StringCchCopyA(strPartType, cchPartType, MUIGetString(STRING_FORMATUNKNOWN));
    }
}


/* FUNCTIONS ****************************************************************/

VOID
InitPartitionListUi(
    IN OUT PPARTLIST_UI ListUi,
    IN PPARTLIST List,
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom)
{
    ListUi->List = List;
    // ListUi->FirstShown = NULL;
    // ListUi->LastShown = NULL;

    ListUi->Left = Left;
    ListUi->Top = Top;
    ListUi->Right = Right;
    ListUi->Bottom = Bottom;

    ListUi->Line = 0;
    ListUi->Offset = 0;

    // ListUi->Redraw = TRUE;
}

static
VOID
PrintEmptyLine(
    IN PPARTLIST_UI ListUi)
{
    COORD coPos;
    ULONG Written;
    USHORT Width;
    USHORT Height;

    Width = ListUi->Right - ListUi->Left - 1;
    Height = ListUi->Bottom - ListUi->Top - 2;

    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top + 1 + ListUi->Line;

    if (ListUi->Line >= 0 && ListUi->Line <= Height)
    {
        FillConsoleOutputAttribute(StdOutput,
                                   FOREGROUND_WHITE | BACKGROUND_BLUE,
                                   Width,
                                   coPos,
                                   &Written);

        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Width,
                                    coPos,
                                    &Written);
    }

    ListUi->Line++;
}

static
VOID
PrintPartitionData(
    IN PPARTLIST_UI ListUi,
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry)
{
    PPARTLIST List = ListUi->List;
    CHAR LineBuffer[128];
    COORD coPos;
    ULONG Written;
    USHORT Width;
    USHORT Height;
    LARGE_INTEGER PartSize;
    PCHAR Unit;
    UCHAR Attribute;
    CHAR PartTypeString[32];
    PCHAR PartType = PartTypeString;

    Width = ListUi->Right - ListUi->Left - 1;
    Height = ListUi->Bottom - ListUi->Top - 2;

    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top + 1 + ListUi->Line;

    if (PartEntry->IsPartitioned == FALSE)
    {
        PartSize.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
        if (PartSize.QuadPart >= 10 * GB) /* 10 GB */
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, GB);
            Unit = MUIGetString(STRING_GB);
        }
        else
#endif
        if (PartSize.QuadPart >= 10 * MB) /* 10 MB */
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, MB);
            Unit = MUIGetString(STRING_MB);
        }
        else
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, KB);
            Unit = MUIGetString(STRING_KB);
        }

        sprintf(LineBuffer,
                MUIGetString(STRING_UNPSPACE),
                PartEntry->LogicalPartition ? "  " : "",
                PartEntry->LogicalPartition ? "" : "  ",
                PartSize.u.LowPart,
                Unit);
    }
    else
    {
        /* Determine partition type */
        PartTypeString[0] = '\0';
        if (PartEntry->New == TRUE)
        {
            PartType = MUIGetString(STRING_UNFORMATTED);
        }
        else if (PartEntry->IsPartitioned == TRUE)
        {
           GetPartTypeStringFromPartitionType(PartEntry->PartitionType,
                                              PartTypeString,
                                              ARRAYSIZE(PartTypeString));
           PartType = PartTypeString;
        }

        PartSize.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
        if (PartSize.QuadPart >= 10 * GB) /* 10 GB */
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, GB);
            Unit = MUIGetString(STRING_GB);
        }
        else
#endif
        if (PartSize.QuadPart >= 10 * MB) /* 10 MB */
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, MB);
            Unit = MUIGetString(STRING_MB);
        }
        else
        {
            PartSize.QuadPart = RoundingDivide(PartSize.QuadPart, KB);
            Unit = MUIGetString(STRING_KB);
        }

        if (strcmp(PartType, MUIGetString(STRING_FORMATUNKNOWN)) == 0)
        {
            sprintf(LineBuffer,
                    MUIGetString(STRING_HDDINFOUNK5),
                    (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
                    (PartEntry->DriveLetter == 0) ? '-' : ':',
                    PartEntry->BootIndicator ? '*' : ' ',
                    PartEntry->LogicalPartition ? "  " : "",
                    PartEntry->PartitionType,
                    PartEntry->LogicalPartition ? "" : "  ",
                    PartSize.u.LowPart,
                    Unit);
        }
        else
        {
            sprintf(LineBuffer,
                    "%c%c %c %s%-24s%s     %6lu %s",
                    (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
                    (PartEntry->DriveLetter == 0) ? '-' : ':',
                    PartEntry->BootIndicator ? '*' : ' ',
                    PartEntry->LogicalPartition ? "  " : "",
                    PartType,
                    PartEntry->LogicalPartition ? "" : "  ",
                    PartSize.u.LowPart,
                    Unit);
        }
    }

    Attribute = (List->CurrentDisk == DiskEntry &&
                 List->CurrentPartition == PartEntry) ?
                 FOREGROUND_BLUE | BACKGROUND_WHITE :
                 FOREGROUND_WHITE | BACKGROUND_BLUE;

    if (ListUi->Line >= 0 && ListUi->Line <= Height)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Width,
                                    coPos,
                                    &Written);
    }
    coPos.X += 4;
    Width -= 8;
    if (ListUi->Line >= 0 && ListUi->Line <= Height)
    {
        FillConsoleOutputAttribute(StdOutput,
                                   Attribute,
                                   Width,
                                   coPos,
                                   &Written);
    }
    coPos.X++;
    Width -= 2;
    if (ListUi->Line >= 0 && ListUi->Line <= Height)
    {
        WriteConsoleOutputCharacterA(StdOutput,
                                     LineBuffer,
                                     min(strlen(LineBuffer), Width),
                                     coPos,
                                     &Written);
    }

    ListUi->Line++;
}

static
VOID
PrintDiskData(
    IN PPARTLIST_UI ListUi,
    IN PDISKENTRY DiskEntry)
{
    // PPARTLIST List = ListUi->List;
    PPARTENTRY PrimaryPartEntry, LogicalPartEntry;
    PLIST_ENTRY PrimaryEntry, LogicalEntry;
    CHAR LineBuffer[128];
    COORD coPos;
    ULONG Written;
    USHORT Width;
    USHORT Height;
    ULARGE_INTEGER DiskSize;
    PCHAR Unit;

    Width = ListUi->Right - ListUi->Left - 1;
    Height = ListUi->Bottom - ListUi->Top - 2;

    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top + 1 + ListUi->Line;

    DiskSize.QuadPart = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
    if (DiskSize.QuadPart >= 10 * GB) /* 10 GB */
    {
        DiskSize.QuadPart = RoundingDivide(DiskSize.QuadPart, GB);
        Unit = MUIGetString(STRING_GB);
    }
    else
    {
        DiskSize.QuadPart = RoundingDivide(DiskSize.QuadPart, MB);
        if (DiskSize.QuadPart == 0)
            DiskSize.QuadPart = 1;
        Unit = MUIGetString(STRING_MB);
    }

    if (DiskEntry->DriverName.Length > 0)
    {
        sprintf(LineBuffer,
                MUIGetString(STRING_HDINFOPARTSELECT_1),
                DiskSize.u.LowPart,
                Unit,
                DiskEntry->DiskNumber,
                DiskEntry->Port,
                DiskEntry->Bus,
                DiskEntry->Id,
                &DiskEntry->DriverName,
                DiskEntry->NoMbr ? "GPT" : "MBR");
    }
    else
    {
        sprintf(LineBuffer,
                MUIGetString(STRING_HDINFOPARTSELECT_2),
                DiskSize.u.LowPart,
                Unit,
                DiskEntry->DiskNumber,
                DiskEntry->Port,
                DiskEntry->Bus,
                DiskEntry->Id,
                DiskEntry->NoMbr ? "GPT" : "MBR");
    }

    if (ListUi->Line >= 0 && ListUi->Line <= Height)
    {
        FillConsoleOutputAttribute(StdOutput,
                                   FOREGROUND_WHITE | BACKGROUND_BLUE,
                                   Width,
                                   coPos,
                                   &Written);

        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Width,
                                    coPos,
                                    &Written);
    }

    coPos.X++;
    if (ListUi->Line >= 0 && ListUi->Line <= Height)
    {
        WriteConsoleOutputCharacterA(StdOutput,
                                     LineBuffer,
                                     min((USHORT)strlen(LineBuffer), Width - 2),
                                     coPos,
                                     &Written);
    }

    ListUi->Line++;

    /* Print separator line */
    PrintEmptyLine(ListUi);

    /* Print partition lines */
    PrimaryEntry = DiskEntry->PrimaryPartListHead.Flink;
    while (PrimaryEntry != &DiskEntry->PrimaryPartListHead)
    {
        PrimaryPartEntry = CONTAINING_RECORD(PrimaryEntry, PARTENTRY, ListEntry);

        PrintPartitionData(ListUi,
                           DiskEntry,
                           PrimaryPartEntry);

        if (IsContainerPartition(PrimaryPartEntry->PartitionType))
        {
            LogicalEntry = DiskEntry->LogicalPartListHead.Flink;
            while (LogicalEntry != &DiskEntry->LogicalPartListHead)
            {
                LogicalPartEntry = CONTAINING_RECORD(LogicalEntry, PARTENTRY, ListEntry);

                PrintPartitionData(ListUi,
                                   DiskEntry,
                                   LogicalPartEntry);

                LogicalEntry = LogicalEntry->Flink;
            }
        }

        PrimaryEntry = PrimaryEntry->Flink;
    }

    /* Print separator line */
    PrintEmptyLine(ListUi);
}

VOID
DrawPartitionList(
    IN PPARTLIST_UI ListUi)
{
    PPARTLIST List = ListUi->List;
    PLIST_ENTRY Entry, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry = NULL;
    COORD coPos;
    ULONG Written;
    SHORT i;
    SHORT CurrentDiskLine;
    SHORT CurrentPartLine;
    SHORT LastLine;
    BOOLEAN CurrentPartLineFound = FALSE;
    BOOLEAN CurrentDiskLineFound = FALSE;

    /* Calculate the line of the current disk and partition */
    CurrentDiskLine = 0;
    CurrentPartLine = 0;
    LastLine = 0;

    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        LastLine += 2;
        if (CurrentPartLineFound == FALSE)
        {
            CurrentPartLine += 2;
        }

        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry == List->CurrentPartition)
            {
                CurrentPartLineFound = TRUE;
            }

            Entry2 = Entry2->Flink;
            if (CurrentPartLineFound == FALSE)
            {
                CurrentPartLine++;
            }

            LastLine++;
        }

        if (CurrentPartLineFound == FALSE)
        {
            Entry2 = DiskEntry->LogicalPartListHead.Flink;
            while (Entry2 != &DiskEntry->LogicalPartListHead)
            {
                PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
                if (PartEntry == List->CurrentPartition)
                {
                    CurrentPartLineFound = TRUE;
                }

                Entry2 = Entry2->Flink;
                if (CurrentPartLineFound == FALSE)
                {
                    CurrentPartLine++;
                }

                LastLine++;
            }
        }

        if (DiskEntry == List->CurrentDisk)
        {
            CurrentDiskLineFound = TRUE;
        }

        Entry = Entry->Flink;
        if (Entry != &List->DiskListHead)
        {
            if (CurrentDiskLineFound == FALSE)
            {
                CurrentPartLine ++;
                CurrentDiskLine = CurrentPartLine;
            }

            LastLine++;
        }
        else
        {
            LastLine--;
        }
    }

    /* If it possible, make the disk name visible */
    if (CurrentPartLine < ListUi->Offset)
    {
        ListUi->Offset = CurrentPartLine;
    }
    else if (CurrentPartLine - ListUi->Offset > ListUi->Bottom - ListUi->Top - 2)
    {
        ListUi->Offset = CurrentPartLine - (ListUi->Bottom - ListUi->Top - 2);
    }

    if (CurrentDiskLine < ListUi->Offset && CurrentPartLine - CurrentDiskLine < ListUi->Bottom - ListUi->Top - 2)
    {
        ListUi->Offset = CurrentDiskLine;
    }

    /* Draw upper left corner */
    coPos.X = ListUi->Left;
    coPos.Y = ListUi->Top;
    FillConsoleOutputCharacterA(StdOutput,
                                0xDA, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw upper edge */
    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top;
    if (ListUi->Offset == 0)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    ListUi->Right - ListUi->Left - 1,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    ListUi->Right - ListUi->Left - 5,
                                    coPos,
                                    &Written);
        coPos.X = ListUi->Right - 5;
        WriteConsoleOutputCharacterA(StdOutput,
                                     "(\x18)", // "(up)"
                                     3,
                                     coPos,
                                     &Written);
        coPos.X = ListUi->Right - 2;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    2,
                                    coPos,
                                    &Written);
    }

    /* Draw upper right corner */
    coPos.X = ListUi->Right;
    coPos.Y = ListUi->Top;
    FillConsoleOutputCharacterA(StdOutput,
                                0xBF, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw left and right edge */
    for (i = ListUi->Top + 1; i < ListUi->Bottom; i++)
    {
        coPos.X = ListUi->Left;
        coPos.Y = i;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB3, // '|',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = ListUi->Right;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB3, //'|',
                                    1,
                                    coPos,
                                    &Written);
    }

    /* Draw lower left corner */
    coPos.X = ListUi->Left;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC0, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw lower edge */
    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Bottom;
    if (LastLine - ListUi->Offset <= ListUi->Bottom - ListUi->Top - 2)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    ListUi->Right - ListUi->Left - 1,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    ListUi->Right - ListUi->Left - 5,
                                    coPos,
                                    &Written);
        coPos.X = ListUi->Right - 5;
        WriteConsoleOutputCharacterA(StdOutput,
                                     "(\x19)", // "(down)"
                                     3,
                                     coPos,
                                     &Written);
       coPos.X = ListUi->Right - 2;
       FillConsoleOutputCharacterA(StdOutput,
                                   0xC4, // '-',
                                   2,
                                   coPos,
                                   &Written);
    }

    /* Draw lower right corner */
    coPos.X = ListUi->Right;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                0xD9, // '+',
                                1,
                                coPos,
                                &Written);

    /* print list entries */
    ListUi->Line = - ListUi->Offset;

    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* Print disk entry */
        PrintDiskData(ListUi, DiskEntry);

        Entry = Entry->Flink;
    }
}

VOID
ScrollDownPartitionList(
    IN PPARTLIST_UI ListUi)
{
    if (GetNextPartition(ListUi->List))
        DrawPartitionList(ListUi);
}

VOID
ScrollUpPartitionList(
    IN PPARTLIST_UI ListUi)
{
    if (GetPrevPartition(ListUi->List))
        DrawPartitionList(ListUi);
}

/* EOF */
