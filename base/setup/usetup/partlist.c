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
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* HELPERS FOR PARTITION TYPES **********************************************/

VOID
GetPartTypeStringFromPartitionType(
    IN UCHAR partitionType,
    OUT PCHAR strPartType,
    IN ULONG cchPartType)
{
    /* Determine partition type */

    if (IsContainerPartition(partitionType))
    {
        RtlStringCchCopyA(strPartType, cchPartType, MUIGetString(STRING_EXTENDED_PARTITION));
    }
    else if (partitionType == PARTITION_ENTRY_UNUSED)
    {
        RtlStringCchCopyA(strPartType, cchPartType, MUIGetString(STRING_FORMATUNUSED));
    }
    else
    {
        UINT i;

        /* Do the table lookup */
        for (i = 0; i < ARRAYSIZE(PartitionTypes); i++)
        {
            if (partitionType == PartitionTypes[i].Type)
            {
                RtlStringCchCopyA(strPartType, cchPartType, PartitionTypes[i].Description);
                return;
            }
        }

        /* We are here because the partition type is unknown */
        RtlStringCchCopyA(strPartType, cchPartType, MUIGetString(STRING_FORMATUNKNOWN));
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

    /* Get the partition size */
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

    if (PartEntry->IsPartitioned == FALSE)
    {
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
        if (PartEntry->New != FALSE)
        {
            PartType = MUIGetString(STRING_UNFORMATTED);
        }
        else if (PartEntry->IsPartitioned != FALSE)
        {
            GetPartTypeStringFromPartitionType(PartEntry->PartitionType,
                                               PartTypeString,
                                               ARRAYSIZE(PartTypeString));
            PartType = PartTypeString;
        }

        if (strcmp(PartType, MUIGetString(STRING_FORMATUNKNOWN)) == 0)
        {
            sprintf(LineBuffer,
                    MUIGetString(STRING_HDDINFOUNK5),
                    (PartEntry->DriveLetter == 0) ? '-' : (CHAR)PartEntry->DriveLetter,
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
                    (PartEntry->DriveLetter == 0) ? '-' : (CHAR)PartEntry->DriveLetter,
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

    //
    // FIXME: We *MUST* use TXTSETUP.SIF strings from section "DiskDriverMap" !!
    //
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
