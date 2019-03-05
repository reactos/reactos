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
/*
 * COPYRIGHT:       See COPYING in the top level directory
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
    IN PPARTENTRY CurrentEntry OPTIONAL,
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

    /* Search for first usable disk and partition */
    if (!CurrentEntry)
    {
        ListUi->CurrentDisk = NULL;
        ListUi->CurrentPartition = NULL;

        if (!IsListEmpty(&List->DiskListHead))
        {
            ListUi->CurrentDisk = CONTAINING_RECORD(List->DiskListHead.Flink,
                                                    DISKENTRY, ListEntry);

            if (!IsListEmpty(&ListUi->CurrentDisk->PrimaryPartListHead))
            {
                ListUi->CurrentPartition = CONTAINING_RECORD(ListUi->CurrentDisk->PrimaryPartListHead.Flink,
                                                             PARTENTRY, ListEntry);
            }
        }
    }
    else
    {
        /*
         * The CurrentEntry must belong to the associated partition list,
         * and the latter must therefore not be empty.
         */
        ASSERT(!IsListEmpty(&List->DiskListHead));
        ASSERT(CurrentEntry->DiskEntry->PartList == List);

        ListUi->CurrentPartition = CurrentEntry;
        ListUi->CurrentDisk = CurrentEntry->DiskEntry;
    }
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

    Attribute = (ListUi->CurrentDisk == DiskEntry &&
                 ListUi->CurrentPartition == PartEntry) ?
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
                DiskEntry->DiskStyle == PARTITION_STYLE_MBR ? "MBR" :
                DiskEntry->DiskStyle == PARTITION_STYLE_GPT ? "GPT" :
                                                              "RAW");
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
                DiskEntry->DiskStyle == PARTITION_STYLE_MBR ? "MBR" :
                DiskEntry->DiskStyle == PARTITION_STYLE_GPT ? "GPT" :
                                                              "RAW");
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
    for (PrimaryEntry = DiskEntry->PrimaryPartListHead.Flink;
         PrimaryEntry != &DiskEntry->PrimaryPartListHead;
         PrimaryEntry = PrimaryEntry->Flink)
    {
        PrimaryPartEntry = CONTAINING_RECORD(PrimaryEntry, PARTENTRY, ListEntry);

        PrintPartitionData(ListUi,
                           DiskEntry,
                           PrimaryPartEntry);

        if (IsContainerPartition(PrimaryPartEntry->PartitionType))
        {
            for (LogicalEntry = DiskEntry->LogicalPartListHead.Flink;
                 LogicalEntry != &DiskEntry->LogicalPartListHead;
                 LogicalEntry = LogicalEntry->Flink)
            {
                LogicalPartEntry = CONTAINING_RECORD(LogicalEntry, PARTENTRY, ListEntry);

                PrintPartitionData(ListUi,
                                   DiskEntry,
                                   LogicalPartEntry);
            }
        }
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
    USHORT Width;
    USHORT Height;
    SHORT i;
    SHORT CurrentDiskLine;
    SHORT CurrentPartLine;
    SHORT LastLine;
    BOOLEAN CurrentPartLineFound = FALSE;
    BOOLEAN CurrentDiskLineFound = FALSE;

    Width = ListUi->Right - ListUi->Left - 1;
    Height = ListUi->Bottom - ListUi->Top - 2;

    /* Calculate the line of the current disk and partition */
    CurrentDiskLine = 0;
    CurrentPartLine = 0;
    LastLine = 0;

    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        LastLine += 2;
        if (CurrentPartLineFound == FALSE)
        {
            CurrentPartLine += 2;
        }

        for (Entry2 = DiskEntry->PrimaryPartListHead.Flink;
             Entry2 != &DiskEntry->PrimaryPartListHead;
             Entry2 = Entry2->Flink)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry == ListUi->CurrentPartition)
            {
                CurrentPartLineFound = TRUE;
            }

            if (CurrentPartLineFound == FALSE)
            {
                CurrentPartLine++;
            }

            LastLine++;
        }

        if (CurrentPartLineFound == FALSE)
        {
            for (Entry2 = DiskEntry->LogicalPartListHead.Flink;
                 Entry2 != &DiskEntry->LogicalPartListHead;
                 Entry2 = Entry2->Flink)
            {
                PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
                if (PartEntry == ListUi->CurrentPartition)
                {
                    CurrentPartLineFound = TRUE;
                }

                if (CurrentPartLineFound == FALSE)
                {
                    CurrentPartLine++;
                }

                LastLine++;
            }
        }

        if (DiskEntry == ListUi->CurrentDisk)
        {
            CurrentDiskLineFound = TRUE;
        }

        if (Entry->Flink != &List->DiskListHead)
        {
            if (CurrentDiskLineFound == FALSE)
            {
                CurrentPartLine++;
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
    else if (CurrentPartLine - ListUi->Offset > Height)
    {
        ListUi->Offset = CurrentPartLine - Height;
    }

    if (CurrentDiskLine < ListUi->Offset && CurrentPartLine - CurrentDiskLine < Height)
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
                                    Width,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    Width - 4,
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
    if (LastLine - ListUi->Offset <= Height)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    Width,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    Width - 4,
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

    /* Print list entries */
    ListUi->Line = -ListUi->Offset;

    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* Print disk entry */
        PrintDiskData(ListUi, DiskEntry);
    }
}

VOID
ScrollDownPartitionList(
    IN PPARTLIST_UI ListUi)
{
    PPARTENTRY NextPart = GetNextPartition(ListUi->List, ListUi->CurrentPartition);
    if (NextPart)
    {
        ListUi->CurrentPartition = NextPart;
        ListUi->CurrentDisk = NextPart->DiskEntry;
        DrawPartitionList(ListUi);
    }
}

VOID
ScrollUpPartitionList(
    IN PPARTLIST_UI ListUi)
{
    PPARTENTRY PrevPart = GetPrevPartition(ListUi->List, ListUi->CurrentPartition);
    if (PrevPart)
    {
        ListUi->CurrentPartition = PrevPart;
        ListUi->CurrentDisk = PrevPart->DiskEntry;
        DrawPartitionList(ListUi);
    }
}

/* EOF */
