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

/* HELPERS FOR DISK AND PARTITION DESCRIPTIONS ******************************/

VOID
GetPartitionTypeString(
    IN PPARTENTRY PartEntry,
    OUT PSTR strBuffer,
    IN ULONG cchBuffer)
{
    if (PartEntry->PartitionType == PARTITION_ENTRY_UNUSED)
    {
        RtlStringCchCopyA(strBuffer, cchBuffer,
                          MUIGetString(STRING_FORMATUNUSED));
    }
    else if (IsContainerPartition(PartEntry->PartitionType))
    {
        RtlStringCchCopyA(strBuffer, cchBuffer,
                          MUIGetString(STRING_EXTENDED_PARTITION));
    }
    else
    {
        UINT i;

        /* Do the table lookup */
        if (PartEntry->DiskEntry->DiskStyle == PARTITION_STYLE_MBR)
        {
            for (i = 0; i < ARRAYSIZE(MbrPartitionTypes); ++i)
            {
                if (PartEntry->PartitionType == MbrPartitionTypes[i].Type)
                {
                    RtlStringCchCopyA(strBuffer, cchBuffer,
                                      MbrPartitionTypes[i].Description);
                    return;
                }
            }
        }
#if 0 // TODO: GPT support!
        else if (PartEntry->DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
        {
            for (i = 0; i < ARRAYSIZE(GptPartitionTypes); ++i)
            {
                if (IsEqualPartitionType(PartEntry->PartitionType,
                                         GptPartitionTypes[i].Guid))
                {
                    RtlStringCchCopyA(strBuffer, cchBuffer,
                                      GptPartitionTypes[i].Description);
                    return;
                }
            }
        }
#endif

        /* We are here because the partition type is unknown */
        if (cchBuffer > 0) *strBuffer = '\0';
    }

    if ((cchBuffer > 0) && (*strBuffer == '\0'))
    {
        RtlStringCchPrintfA(strBuffer, cchBuffer,
                            MUIGetString(STRING_PARTTYPE),
                            PartEntry->PartitionType);
    }
}

VOID
PrettifySize1(
    IN OUT PULONGLONG Size,
    OUT PCSTR* Unit)
{
    ULONGLONG DiskSize = *Size;

    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = RoundingDivide(DiskSize, GB);
        *Unit = MUIGetString(STRING_GB);
    }
    else
    {
        DiskSize = RoundingDivide(DiskSize, MB);
        if (DiskSize == 0)
            DiskSize = 1;
        *Unit = MUIGetString(STRING_MB);
    }

    *Size = DiskSize;
}

VOID
PrettifySize2(
    IN OUT PULONGLONG Size,
    OUT PCSTR* Unit)
{
    ULONGLONG PartSize = *Size;

#if 0
    if (PartSize >= 10 * GB) /* 10 GB */
    {
        PartSize = RoundingDivide(PartSize, GB);
        *Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    if (PartSize >= 10 * MB) /* 10 MB */
    {
        PartSize = RoundingDivide(PartSize, MB);
        *Unit = MUIGetString(STRING_MB);
    }
    else
    {
        PartSize = RoundingDivide(PartSize, KB);
        *Unit = MUIGetString(STRING_KB);
    }

    *Size = PartSize;
}

VOID
PartitionDescription(
    IN PPARTENTRY PartEntry,
    OUT PSTR strBuffer,
    IN SIZE_T cchBuffer)
{
    PSTR pBuffer = strBuffer;
    size_t cchBufferSize = cchBuffer;
    ULONGLONG PartSize;
    PCSTR Unit;
    PVOLINFO VolInfo = (PartEntry->Volume ? &PartEntry->Volume->Info : NULL);

    /* Get the partition size */
    PartSize = GetPartEntrySizeInBytes(PartEntry);
    PrettifySize2(&PartSize, &Unit);

    if (PartEntry->IsPartitioned == FALSE)
    {
        /* Unpartitioned space: Just display the description and size */
        RtlStringCchPrintfExA(pBuffer, cchBufferSize,
                              &pBuffer, &cchBufferSize, 0,
                              "     %s%-.30s",
                              PartEntry->LogicalPartition ? "  " : "", // Optional indentation
                              MUIGetString(STRING_UNPSPACE));

        RtlStringCchPrintfA(pBuffer, cchBufferSize,
                            "%*s%6I64u %s",
                            38 - min(strlen(strBuffer), 38), "", // Indentation
                            PartSize,
                            Unit);
        return;
    }

//
// NOTE: This could be done with the next case.
//
    if ((PartEntry->DiskEntry->DiskStyle == PARTITION_STYLE_MBR) &&
        IsContainerPartition(PartEntry->PartitionType))
    {
        /* Extended partition container: Just display the partition's type and size */
        RtlStringCchPrintfExA(pBuffer, cchBufferSize,
                              &pBuffer, &cchBufferSize, 0,
                              "     %-.30s",
                              MUIGetString(STRING_EXTENDED_PARTITION));

        RtlStringCchPrintfA(pBuffer, cchBufferSize,
                            "%*s%6I64u %s",
                            38 - min(strlen(strBuffer), 38), "", // Indentation
                            PartSize,
                            Unit);
        return;
    }

    /*
     * Not an extended partition container.
     */

    /* Drive letter and partition number */
    RtlStringCchPrintfExA(pBuffer, cchBufferSize,
                          &pBuffer, &cchBufferSize, 0,
                          "%c%c %c %s(%lu) ",
                          !(VolInfo && VolInfo->DriveLetter) ? '-' : (CHAR)VolInfo->DriveLetter,
                          !(VolInfo && VolInfo->DriveLetter) ? '-' : ':',
                          PartEntry->BootIndicator ? '*' : ' ',
                          PartEntry->LogicalPartition ? "  " : "", // Optional indentation
                          PartEntry->PartitionNumber);

    /*
     * If the volume's file system is recognized, display the volume label
     * (if any) and the file system name. Otherwise, display the partition
     * type if it's not a new partition.
     */
    if (VolInfo && IsFormatted(VolInfo))
    {
        size_t cchLabelSize = 0;
        if (*VolInfo->VolumeLabel)
        {
            RtlStringCchPrintfExA(pBuffer, cchBufferSize,
                                  &pBuffer, &cchLabelSize, 0,
                                  "\"%-.11S\" ",
                                  VolInfo->VolumeLabel);
            cchLabelSize = cchBufferSize - cchLabelSize; // Actual length of the label part.
            cchBufferSize -= cchLabelSize; // And reset cchBufferSize to what it should be.
        }

        // TODO: Group this part together with the similar one
        // from below once the strings are in the same encoding...
        RtlStringCchPrintfExA(pBuffer, cchBufferSize,
                              &pBuffer, &cchBufferSize, 0,
                              "[%-.*S]",
                              /* The minimum length can be at most 11 since
                               * cchLabelSize can be at most == 11 + 3 == 14 */
                              25 - min(cchLabelSize, 25),
                              VolInfo->FileSystem);
    }
    else
    {
        CHAR PartTypeString[32];
        PCSTR PartType = PartTypeString;

        if (PartEntry->New)
        {
            /* Use this description if the partition is new (and thus, not formatted) */
            PartType = MUIGetString(STRING_UNFORMATTED);
        }
        else
        {
            /* If the partition is not new but its file system is not recognized
             * (or is not formatted), use the partition type description. */
            GetPartitionTypeString(PartEntry,
                                   PartTypeString,
                                   ARRAYSIZE(PartTypeString));
            PartType = PartTypeString;
        }
        if (!PartType || !*PartType)
        {
            PartType = MUIGetString(STRING_FORMATUNKNOWN);
        }

        // TODO: Group this part together with the similar one
        // from above once the strings are in the same encoding...
        RtlStringCchPrintfExA(pBuffer, cchBufferSize,
                              &pBuffer, &cchBufferSize, 0,
                              "[%-.*s]",
                              25,
                              PartType);
    }

    /* Show the remaining free space only if a FS is mounted */
    // FIXME: We don't support that yet!
#if 0
    if (VolInfo && *VolInfo->FileSystem)
    {
        RtlStringCchPrintfA(pBuffer, cchBufferSize,
                            "%*s%6I64u %s (%6I64u %s %s)",
                            38 - min(strlen(strBuffer), 38), "", // Indentation
                            PartSize,
                            Unit,
                            PartFreeSize,
                            Unit,
                            "free");
    }
    else
#endif
    {
        RtlStringCchPrintfA(pBuffer, cchBufferSize,
                            "%*s%6I64u %s",
                            38 - min(strlen(strBuffer), 38), "", // Indentation
                            PartSize,
                            Unit);
    }
}

VOID
DiskDescription(
    IN PDISKENTRY DiskEntry,
    OUT PSTR strBuffer,
    IN SIZE_T cchBuffer)
{
    ULONGLONG DiskSize;
    PCSTR Unit;

    /* Get the disk size */
    DiskSize = GetDiskSizeInBytes(DiskEntry);
    PrettifySize1(&DiskSize, &Unit);

    //
    // FIXME: We *MUST* use TXTSETUP.SIF strings from section "DiskDriverMap" !!
    //
    if (DiskEntry->DriverName.Length > 0)
    {
        RtlStringCchPrintfA(strBuffer, cchBuffer,
                            MUIGetString(STRING_HDDINFO1),
                            DiskSize,
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
        RtlStringCchPrintfA(strBuffer, cchBuffer,
                            MUIGetString(STRING_HDDINFO2),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            DiskEntry->DiskStyle == PARTITION_STYLE_MBR ? "MBR" :
                            DiskEntry->DiskStyle == PARTITION_STYLE_GPT ? "GPT" :
                                                                          "RAW");
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
    COORD coPos;
    ULONG Written;
    USHORT Width;
    USHORT Height;
    UCHAR Attribute;
    CHAR LineBuffer[100];

    PartitionDescription(PartEntry, LineBuffer, ARRAYSIZE(LineBuffer));

    Width = ListUi->Right - ListUi->Left - 1;
    Height = ListUi->Bottom - ListUi->Top - 2;

    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top + 1 + ListUi->Line;

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
    COORD coPos;
    ULONG Written;
    USHORT Width;
    USHORT Height;
    CHAR LineBuffer[100];

    DiskDescription(DiskEntry, LineBuffer, ARRAYSIZE(LineBuffer));

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
                                CharUpperLeftCorner, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw upper edge */
    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Top;
    if (ListUi->Offset == 0)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    CharHorizontalLine, // '-',
                                    Width,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    CharHorizontalLine, // '-',
                                    Width - 4,
                                    coPos,
                                    &Written);
        {
            CHAR szBuff[] = "(.)"; // "(up)"
            szBuff[1] = CharUpArrow;
            coPos.X = ListUi->Right - 5;
            WriteConsoleOutputCharacterA(StdOutput,
                                         szBuff,
                                         3,
                                         coPos,
                                         &Written);
        }
        coPos.X = ListUi->Right - 2;
        FillConsoleOutputCharacterA(StdOutput,
                                    CharHorizontalLine, // '-',
                                    2,
                                    coPos,
                                    &Written);
    }

    /* Draw upper right corner */
    coPos.X = ListUi->Right;
    coPos.Y = ListUi->Top;
    FillConsoleOutputCharacterA(StdOutput,
                                CharUpperRightCorner, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw left and right edge */
    for (i = ListUi->Top + 1; i < ListUi->Bottom; i++)
    {
        coPos.X = ListUi->Left;
        coPos.Y = i;
        FillConsoleOutputCharacterA(StdOutput,
                                    CharVerticalLine, // '|',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = ListUi->Right;
        FillConsoleOutputCharacterA(StdOutput,
                                    CharVerticalLine, //'|',
                                    1,
                                    coPos,
                                    &Written);
    }

    /* Draw lower left corner */
    coPos.X = ListUi->Left;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                CharLowerLeftCorner, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw lower edge */
    coPos.X = ListUi->Left + 1;
    coPos.Y = ListUi->Bottom;
    if (LastLine - ListUi->Offset <= Height)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    CharHorizontalLine, // '-',
                                    Width,
                                    coPos,
                                    &Written);
    }
    else
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    CharHorizontalLine, // '-',
                                    Width - 4,
                                    coPos,
                                    &Written);
        {
            CHAR szBuff[] = "(.)"; // "(down)"
            szBuff[1] = CharDownArrow;
            coPos.X = ListUi->Right - 5;
            WriteConsoleOutputCharacterA(StdOutput,
                                         szBuff,
                                         3,
                                         coPos,
                                         &Written);
        }
       coPos.X = ListUi->Right - 2;
       FillConsoleOutputCharacterA(StdOutput,
                                   CharHorizontalLine, // '-',
                                   2,
                                   coPos,
                                   &Written);
    }

    /* Draw lower right corner */
    coPos.X = ListUi->Right;
    coPos.Y = ListUi->Bottom;
    FillConsoleOutputCharacterA(StdOutput,
                                CharLowerRightCorner, // '+',
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

/**
 * @param[in]   Direction
 * TRUE or FALSE to scroll to the next (down) or previous (up) entry, respectively.
 **/
VOID
ScrollUpDownPartitionList(
    _In_ PPARTLIST_UI ListUi,
    _In_ BOOLEAN Direction)
{
    PPARTENTRY PartEntry =
        (Direction ? GetNextPartition
                   : GetPrevPartition)(ListUi->List, ListUi->CurrentPartition);
    if (PartEntry)
    {
        ListUi->CurrentPartition = PartEntry;
        ListUi->CurrentDisk = PartEntry->DiskEntry;
        DrawPartitionList(ListUi);
    }
}

/* EOF */
