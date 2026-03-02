/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Minimal write-only FAT32 populator for in-memory ramdisks
 * COPYRIGHT:   Copyright 2025 Ahmed Arif <arif.ing@outlook.com>
 *
 * This module provides a lightweight FAT32 writer that operates directly on
 * an in-memory buffer that has already been formatted by RamDiskFormatFat32().
 * It replaces the third-party FatFs library for the specific use case of
 * sequentially populating a FAT32 volume from an ISO source.
 *
 * Key assumptions:
 *   - Single-threaded, write-once population (no concurrent access)
 *   - Volume is already formatted (boot sector, FAT tables, FSINFO present)
 *   - Files are written sequentially and never modified after creation
 *   - Cluster allocation is sequential (no fragmentation)
 */

#include <freeldr.h>
#include <debug.h>
#include <ctype.h>
#include <string.h>
#include <fs/fat.h>
#include <ramdisk_fatwrite.h>

DBG_DEFAULT_CHANNEL(DISK);

/* ========================================================================== */
/*  Internal helpers                                                          */
/* ========================================================================== */

/*
 * Convert a cluster number to a direct memory pointer within the volume.
 */
static
PUCHAR
Fat32ClusterToPointer(
    _In_ PFAT32_WRITER Writer,
    _In_ ULONG Cluster)
{
    ULONGLONG Offset = ((ULONGLONG)Writer->FirstDataSector +
                       (ULONGLONG)(Cluster - 2) * Writer->SectorsPerCluster) *
                       Writer->BytesPerSector;
    return Writer->VolumeBase + Offset;
}

/*
 * Allocate a contiguous run of clusters. Returns the first cluster number.
 * Since we always allocate sequentially from NextFreeCluster, the clusters
 * are guaranteed to be contiguous in the data area.
 */
static
BOOLEAN
Fat32AllocateClusters(
    _Inout_ PFAT32_WRITER Writer,
    _In_ ULONG Count,
    _Out_ PULONG FirstCluster)
{
    ULONG Start;
    ULONG i;

    if (Count == 0)
    {
        *FirstCluster = 0;
        return TRUE;
    }

    Start = Writer->NextFreeCluster;

    /* Check we have enough free clusters (cluster numbers are 2-based) */
    if (Start + Count - 1 > Writer->TotalClusters + 1)
    {
        WARN("Fat32AllocateClusters: need %lu clusters at %lu but only %lu total\n",
             Count, Start, Writer->TotalClusters);
        return FALSE;
    }

    /* Build the cluster chain in both FAT copies */
    for (i = 0; i < Count; i++)
    {
        ULONG Cluster = Start + i;
        ULONG Value = (i < Count - 1) ? (Cluster + 1) : 0x0FFFFFFF;

        Writer->Fat0[Cluster] = Value;
        if (Writer->Fat1)
            Writer->Fat1[Cluster] = Value;
    }

    Writer->NextFreeCluster = Start + Count;
    *FirstCluster = Start;
    return TRUE;
}

/*
 * Characters invalid in FAT 8.3 short names.
 */
static const CHAR InvalidShortNameChars[] = "\"*+,./:;<=>?[\\]| ";

static
BOOLEAN
IsValidShortNameChar(CHAR c)
{
    if ((UCHAR)c < 0x21 || (UCHAR)c > 0x7E)
        return FALSE;
    if (strchr(InvalidShortNameChars, c))
        return FALSE;
    return TRUE;
}

/*
 * Generate a FAT 8.3 short name from a leaf filename.
 *
 * Returns TRUE if the name fits in 8.3 format (possibly with case folding).
 * Sets *NeedsLfn to TRUE if an LFN entry is required.
 * Sets *CaseFlags for the ReservedNT byte (0x08 = base lowercase, 0x10 = ext lowercase).
 */
static
BOOLEAN
Fat32GenerateShortName(
    _In_ PCSTR LeafName,
    _Out_writes_(11) PUCHAR ShortName,
    _Out_ PBOOLEAN NeedsLfn,
    _Out_ PUCHAR CaseFlags)
{
    const CHAR *Dot = NULL;
    const CHAR *p;
    SIZE_T BaseLen, ExtLen;
    SIZE_T i;
    BOOLEAN BaseLower = FALSE, BaseUpper = FALSE;
    BOOLEAN ExtLower = FALSE, ExtUpper = FALSE;

    *NeedsLfn = FALSE;
    *CaseFlags = 0;
    RtlFillMemory(ShortName, 11, ' ');

    if (!LeafName || !*LeafName)
        return FALSE;

    /* Find the last dot (not leading dot) */
    for (p = LeafName; *p; p++)
    {
        if (*p == '.' && p != LeafName)
            Dot = p;
    }

    BaseLen = Dot ? (SIZE_T)(Dot - LeafName) : strlen(LeafName);
    ExtLen = Dot ? strlen(Dot + 1) : 0;

    /* Check if it fits 8.3 with only case folding needed */
    if (BaseLen >= 1 && BaseLen <= 8 && ExtLen <= 3)
    {
        BOOLEAN AllValid = TRUE;

        /* Check base characters */
        for (i = 0; i < BaseLen; i++)
        {
            CHAR c = LeafName[i];
            if (!IsValidShortNameChar(c) && !(c >= 'a' && c <= 'z'))
            {
                AllValid = FALSE;
                break;
            }
            if (c >= 'a' && c <= 'z') BaseLower = TRUE;
            if (c >= 'A' && c <= 'Z') BaseUpper = TRUE;
        }

        /* Check extension characters */
        if (AllValid && Dot)
        {
            for (i = 0; i < ExtLen; i++)
            {
                CHAR c = Dot[1 + i];
                if (!IsValidShortNameChar(c) && !(c >= 'a' && c <= 'z'))
                {
                    AllValid = FALSE;
                    break;
                }
                if (c >= 'a' && c <= 'z') ExtLower = TRUE;
                if (c >= 'A' && c <= 'Z') ExtUpper = TRUE;
            }
        }

        if (AllValid)
        {
            /* Mixed case in base or extension requires LFN */
            BOOLEAN BaseMixed = BaseLower && BaseUpper;
            BOOLEAN ExtMixed = ExtLower && ExtUpper;

            if (!BaseMixed && !ExtMixed)
            {
                /* Pure 8.3 — use case bits in ReservedNT */
                for (i = 0; i < BaseLen; i++)
                    ShortName[i] = (UCHAR)toupper((UCHAR)LeafName[i]);
                for (i = 0; i < ExtLen; i++)
                    ShortName[8 + i] = (UCHAR)toupper((UCHAR)Dot[1 + i]);

                if (BaseLower) *CaseFlags |= 0x08;
                if (ExtLower) *CaseFlags |= 0x10;

                *NeedsLfn = FALSE;
                return TRUE;
            }
        }
    }

    /* Name needs LFN — generate a lossy 8.3 basis name */
    *NeedsLfn = TRUE;
    {
        SIZE_T Pos = 0;

        /* Take up to 6 valid uppercase characters from the base */
        for (i = 0; i < BaseLen && Pos < 6; i++)
        {
            CHAR c = LeafName[i];
            if (c == ' ' || c == '.')
                continue;
            if (IsValidShortNameChar(c))
                ShortName[Pos++] = (UCHAR)toupper((UCHAR)c);
            else if (c >= 'a' && c <= 'z')
                ShortName[Pos++] = (UCHAR)toupper((UCHAR)c);
        }

        /* Pad with underscores if too short */
        while (Pos < 6)
            ShortName[Pos++] = '_';

        /* Append ~1 (caller will handle collisions) */
        ShortName[6] = '~';
        ShortName[7] = '1';

        /* Extension: up to 3 valid uppercase characters */
        if (Dot)
        {
            for (i = 0; i < ExtLen && i < 3; i++)
            {
                CHAR c = Dot[1 + i];
                if (IsValidShortNameChar(c) || (c >= 'a' && c <= 'z'))
                    ShortName[8 + i] = (UCHAR)toupper((UCHAR)c);
                else
                    ShortName[8 + i] = '_';
            }
        }
    }

    return TRUE;
}

/*
 * Compute the LFN checksum from an 11-byte short name.
 */
static
UCHAR
Fat32LfnChecksum(
    _In_reads_(11) const UCHAR *ShortName)
{
    UCHAR Sum = 0;
    int i;
    for (i = 0; i < 11; i++)
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + ShortName[i];
    return Sum;
}

/*
 * Get the next cluster in a chain. Returns 0 if end-of-chain or invalid.
 */
static
ULONG
Fat32GetNextCluster(
    _In_ PFAT32_WRITER Writer,
    _In_ ULONG Cluster)
{
    ULONG Value;

    if (Cluster < 2 || Cluster > Writer->TotalClusters + 1)
        return 0;

    Value = Writer->Fat0[Cluster] & 0x0FFFFFFF;
    if (Value >= 0x0FFFFFF8)
        return 0; /* End of chain */
    return Value;
}

/*
 * Compare a short name entry's 11-byte name against a target leaf name.
 * Reconstructs "BASE.EXT" and does case-insensitive comparison.
 */
static
BOOLEAN
Fat32MatchShortName(
    _In_ const DIRENTRY *Entry,
    _In_ PCSTR LeafName)
{
    CHAR Reconstructed[13]; /* "FILENAME.EXT" + null */
    SIZE_T Pos = 0;
    SIZE_T i;

    for (i = 0; i < 8 && Entry->FileName[i] != ' '; i++)
        Reconstructed[Pos++] = Entry->FileName[i];

    if (Entry->FileName[8] != ' ')
    {
        Reconstructed[Pos++] = '.';
        for (i = 8; i < 11 && Entry->FileName[i] != ' '; i++)
            Reconstructed[Pos++] = Entry->FileName[i];
    }
    Reconstructed[Pos] = '\0';

    return (_stricmp(Reconstructed, LeafName) == 0);
}

/*
 * Reconstruct a long filename from collected LFN entries and compare
 * against a target name. LFN entries are stored in reverse order
 * (highest sequence first), but we collected them in directory order
 * (lowest sequence first in our buffer).
 */
static
BOOLEAN
Fat32MatchLfnName(
    _In_ const LFN_DIRENTRY *LfnEntries,
    _In_ ULONG LfnCount,
    _In_ PCSTR LeafName)
{
    CHAR Reconstructed[256];
    SIZE_T Pos = 0;
    ULONG Seq;

    for (Seq = 0; Seq < LfnCount; Seq++)
    {
        const LFN_DIRENTRY *Lfn = &LfnEntries[Seq];
        ULONG j;
        WCHAR Chars[13];

        /* Extract 13 UTF-16LE characters from the LFN entry */
        for (j = 0; j < 5; j++) Chars[j] = Lfn->Name0_4[j];
        for (j = 0; j < 6; j++) Chars[5 + j] = Lfn->Name5_10[j];
        for (j = 0; j < 2; j++) Chars[11 + j] = Lfn->Name11_12[j];

        for (j = 0; j < 13; j++)
        {
            if (Chars[j] == 0x0000)
                goto done_reconstructing;
            if (Chars[j] == 0xFFFF)
                goto done_reconstructing;
            if (Pos >= sizeof(Reconstructed) - 1)
                return FALSE; /* Name too long */
            /* Simple UTF-16 to ASCII (our names are ASCII) */
            Reconstructed[Pos++] = (CHAR)(Chars[j] & 0xFF);
        }
    }

done_reconstructing:
    Reconstructed[Pos] = '\0';
    return (_stricmp(Reconstructed, LeafName) == 0);
}

/*
 * Search a directory's cluster chain for an entry matching LeafName.
 * Matches against both the short name and the reconstructed long name.
 * Returns the starting cluster of the found entry, or 0 if not found.
 * If IsDir is not NULL, sets it to TRUE if the found entry is a directory.
 */
static
ULONG
Fat32FindEntryInDirectory(
    _In_ PFAT32_WRITER Writer,
    _In_ ULONG DirCluster,
    _In_ PCSTR LeafName,
    _Out_opt_ PBOOLEAN IsDir)
{
    ULONG Cluster = DirCluster;
    /* Buffer to collect LFN entries for the current sequence */
    LFN_DIRENTRY LfnBuffer[20]; /* Max 20 LFN entries = 260 chars */
    ULONG LfnCount = 0;

    while (Cluster != 0)
    {
        PDIRENTRY Entries = (PDIRENTRY)Fat32ClusterToPointer(Writer, Cluster);
        ULONG Count = Writer->BytesPerCluster / sizeof(DIRENTRY);
        ULONG i;

        for (i = 0; i < Count; i++)
        {
            PDIRENTRY Entry = &Entries[i];

            /* End of directory */
            if ((UCHAR)Entry->FileName[0] == 0x00)
                return 0;

            /* Deleted entry */
            if ((UCHAR)Entry->FileName[0] == 0xE5)
            {
                LfnCount = 0;
                continue;
            }

            /* Collect LFN entries */
            if (Entry->Attr == FAT_ATTR_LONG_NAME)
            {
                PLFN_DIRENTRY Lfn = (PLFN_DIRENTRY)Entry;
                UCHAR SeqNum = Lfn->SequenceNumber & 0x3F;

                if (Lfn->SequenceNumber & 0x40)
                {
                    /* First LFN entry (highest sequence number) — start fresh */
                    LfnCount = 0;
                }

                if (SeqNum >= 1 && SeqNum <= 20)
                {
                    /* Store in order: sequence 1 at index 0, sequence 2 at index 1, etc. */
                    if (SeqNum - 1 < 20)
                    {
                        RtlCopyMemory(&LfnBuffer[SeqNum - 1], Lfn, sizeof(LFN_DIRENTRY));
                        if (SeqNum > LfnCount)
                            LfnCount = SeqNum;
                    }
                }
                continue;
            }

            /* Skip volume label */
            if (Entry->Attr & FAT_ATTR_VOLUMENAME)
            {
                LfnCount = 0;
                continue;
            }

            /* This is a short name entry — try matching */
            {
                BOOLEAN Matched = FALSE;

                /* First try matching the long name if we have LFN entries */
                if (LfnCount > 0)
                    Matched = Fat32MatchLfnName(LfnBuffer, LfnCount, LeafName);

                /* Also try matching the short name */
                if (!Matched)
                    Matched = Fat32MatchShortName(Entry, LeafName);

                LfnCount = 0;

                if (Matched)
                {
                    ULONG StartCluster = ((ULONG)Entry->ClusterHigh << 16) |
                                         (ULONG)Entry->ClusterLow;
                    if (IsDir)
                        *IsDir = !!(Entry->Attr & FAT_ATTR_DIRECTORY);
                    return StartCluster;
                }
            }
        }

        Cluster = Fat32GetNextCluster(Writer, Cluster);
    }

    return 0;
}

/*
 * Check if a specific 11-byte short name already exists in a directory.
 */
static
BOOLEAN
Fat32ShortNameExists(
    _In_ PFAT32_WRITER Writer,
    _In_ ULONG DirCluster,
    _In_reads_(11) const UCHAR *ShortName)
{
    ULONG Cluster = DirCluster;

    while (Cluster != 0)
    {
        PDIRENTRY Entries = (PDIRENTRY)Fat32ClusterToPointer(Writer, Cluster);
        ULONG Count = Writer->BytesPerCluster / sizeof(DIRENTRY);
        ULONG i;

        for (i = 0; i < Count; i++)
        {
            PDIRENTRY Entry = &Entries[i];

            if ((UCHAR)Entry->FileName[0] == 0x00)
                return FALSE;
            if ((UCHAR)Entry->FileName[0] == 0xE5)
                continue;
            if (Entry->Attr == FAT_ATTR_LONG_NAME)
                continue;
            if (Entry->Attr & FAT_ATTR_VOLUMENAME)
                continue;

            if (RtlEqualMemory(Entry->FileName, ShortName, 11))
                return TRUE;
        }

        Cluster = Fat32GetNextCluster(Writer, Cluster);
    }

    return FALSE;
}

/*
 * Find N consecutive free directory entry slots in a directory's cluster chain.
 * If not enough space, extends the chain by allocating a new cluster.
 *
 * Returns a pointer to the first free slot, or NULL on failure.
 * The slots are guaranteed to be consecutive within the same cluster.
 *
 * Note: If the required slots span a cluster boundary, we start from the
 * beginning of a new cluster rather than splitting across clusters for
 * simplicity (LFN + short name entries should stay together when possible).
 */
static
PDIRENTRY
Fat32FindFreeSlots(
    _Inout_ PFAT32_WRITER Writer,
    _In_ ULONG DirCluster,
    _In_ ULONG SlotsNeeded)
{
    ULONG Cluster = DirCluster;
    ULONG PrevCluster = 0;

    while (Cluster != 0)
    {
        PDIRENTRY Entries = (PDIRENTRY)Fat32ClusterToPointer(Writer, Cluster);
        ULONG Count = Writer->BytesPerCluster / sizeof(DIRENTRY);
        ULONG i;
        ULONG RunStart = 0;
        ULONG RunLength = 0;

        for (i = 0; i < Count; i++)
        {
            if ((UCHAR)Entries[i].FileName[0] == 0x00 ||
                (UCHAR)Entries[i].FileName[0] == 0xE5)
            {
                if (RunLength == 0)
                    RunStart = i;
                RunLength++;

                if (RunLength >= SlotsNeeded)
                    return &Entries[RunStart];

                /* If this is the end-of-directory marker (0x00), all remaining
                 * slots in this cluster are also free */
                if ((UCHAR)Entries[i].FileName[0] == 0x00)
                {
                    ULONG Remaining = Count - RunStart;
                    if (Remaining >= SlotsNeeded)
                        return &Entries[RunStart];
                    /* Not enough in this cluster, need a new one */
                    break;
                }
            }
            else
            {
                RunLength = 0;
            }
        }

        PrevCluster = Cluster;
        Cluster = Fat32GetNextCluster(Writer, Cluster);
    }

    /* Need to extend the directory with a new cluster */
    {
        ULONG NewCluster;

        if (!Fat32AllocateClusters(Writer, 1, &NewCluster))
            return NULL;

        RtlZeroMemory(Fat32ClusterToPointer(Writer, NewCluster),
                       Writer->BytesPerCluster);

        /* Chain the new cluster to the end */
        if (PrevCluster != 0)
        {
            Writer->Fat0[PrevCluster] = NewCluster;
            if (Writer->Fat1)
                Writer->Fat1[PrevCluster] = NewCluster;
        }

        /* The new cluster FAT entry is already set to 0x0FFFFFFF by Fat32AllocateClusters */

        if (Writer->BytesPerCluster / sizeof(DIRENTRY) >= SlotsNeeded)
            return (PDIRENTRY)Fat32ClusterToPointer(Writer, NewCluster);

        /* This should never happen for reasonable slot counts */
        WARN("Fat32FindFreeSlots: cluster too small for %lu slots\n", SlotsNeeded);
        return NULL;
    }
}

/*
 * Write a complete directory entry (LFN entries + short name entry)
 * into a parent directory.
 */
static
BOOLEAN
Fat32AddDirectoryEntry(
    _Inout_ PFAT32_WRITER Writer,
    _In_ ULONG DirCluster,
    _In_ PCSTR LeafName,
    _In_ ULONG StartCluster,
    _In_ ULONG FileSize,
    _In_ UCHAR Attributes)
{
    UCHAR ShortName[11];
    BOOLEAN NeedsLfn;
    UCHAR CaseFlags;
    ULONG SlotsNeeded;
    PDIRENTRY Slots;

    if (!Fat32GenerateShortName(LeafName, ShortName, &NeedsLfn, &CaseFlags))
        return FALSE;

    /* Handle short name collisions */
    if (NeedsLfn)
    {
        CHAR Digit;
        for (Digit = '1'; Digit <= '9'; Digit++)
        {
            if (!Fat32ShortNameExists(Writer, DirCluster, ShortName))
                break;
            ShortName[7] = (UCHAR)Digit;
        }

        if (Digit > '9')
        {
            WARN("Fat32AddDirectoryEntry: too many short name collisions for '%s'\n",
                 LeafName);
            return FALSE;
        }
    }

    /* Calculate how many LFN entries we need */
    if (NeedsLfn)
    {
        SIZE_T NameLen = strlen(LeafName);
        SlotsNeeded = (ULONG)((NameLen + 12) / 13) + 1; /* LFN entries + 1 short name entry */
    }
    else
    {
        SlotsNeeded = 1; /* Just the short name entry */
    }

    /* Find free slots in the directory */
    Slots = Fat32FindFreeSlots(Writer, DirCluster, SlotsNeeded);
    if (!Slots)
    {
        WARN("Fat32AddDirectoryEntry: no free slots for '%s'\n", LeafName);
        return FALSE;
    }

    /* Write LFN entries (if needed), in order from highest sequence to lowest */
    if (NeedsLfn)
    {
        UCHAR Checksum = Fat32LfnChecksum(ShortName);
        SIZE_T NameLen = strlen(LeafName);
        ULONG LfnCount = SlotsNeeded - 1;
        ULONG Seq;

        for (Seq = LfnCount; Seq >= 1; Seq--)
        {
            PLFN_DIRENTRY Lfn = (PLFN_DIRENTRY)&Slots[LfnCount - Seq];
            SIZE_T CharOffset = (Seq - 1) * 13;
            ULONG j;

            RtlZeroMemory(Lfn, sizeof(LFN_DIRENTRY));

            Lfn->SequenceNumber = (UCHAR)Seq;
            if (Seq == LfnCount)
                Lfn->SequenceNumber |= 0x40; /* Last LFN entry marker */

            Lfn->EntryAttributes = FAT_ATTR_LONG_NAME;
            Lfn->AliasChecksum = Checksum;
            Lfn->StartCluster = 0;

            /* Fill the 13 UTF-16LE characters across the three name fields */
            for (j = 0; j < 5; j++)
            {
                SIZE_T Idx = CharOffset + j;
                if (Idx < NameLen)
                    Lfn->Name0_4[j] = (WCHAR)(UCHAR)LeafName[Idx];
                else if (Idx == NameLen)
                    Lfn->Name0_4[j] = 0x0000;
                else
                    Lfn->Name0_4[j] = 0xFFFF;
            }
            for (j = 0; j < 6; j++)
            {
                SIZE_T Idx = CharOffset + 5 + j;
                if (Idx < NameLen)
                    Lfn->Name5_10[j] = (WCHAR)(UCHAR)LeafName[Idx];
                else if (Idx == NameLen)
                    Lfn->Name5_10[j] = 0x0000;
                else
                    Lfn->Name5_10[j] = 0xFFFF;
            }
            for (j = 0; j < 2; j++)
            {
                SIZE_T Idx = CharOffset + 11 + j;
                if (Idx < NameLen)
                    Lfn->Name11_12[j] = (WCHAR)(UCHAR)LeafName[Idx];
                else if (Idx == NameLen)
                    Lfn->Name11_12[j] = 0x0000;
                else
                    Lfn->Name11_12[j] = 0xFFFF;
            }
        }
    }

    /* Write the short name entry */
    {
        PDIRENTRY ShortEntry = &Slots[SlotsNeeded - 1];

        RtlZeroMemory(ShortEntry, sizeof(DIRENTRY));
        RtlCopyMemory(ShortEntry->FileName, ShortName, 11);
        ShortEntry->Attr = Attributes;
        ShortEntry->ReservedNT = CaseFlags;
        ShortEntry->CreateTime = Writer->FatTime;
        ShortEntry->CreateDate = Writer->FatDate;
        ShortEntry->LastAccessDate = Writer->FatDate;
        ShortEntry->Time = Writer->FatTime;
        ShortEntry->Date = Writer->FatDate;
        ShortEntry->ClusterHigh = (USHORT)(StartCluster >> 16);
        ShortEntry->ClusterLow = (USHORT)(StartCluster & 0xFFFF);
        ShortEntry->Size = FileSize;
    }

    return TRUE;
}

/*
 * Resolve a path like "/reactos/system32/file.exe" to find the parent
 * directory cluster and extract the leaf name.
 *
 * The path components are separated by '/'. Leading '/' is stripped.
 */
static
BOOLEAN
Fat32ResolvePath(
    _In_ PFAT32_WRITER Writer,
    _In_ PCSTR Path,
    _Out_ PULONG ParentCluster,
    _Out_writes_(LeafNameSize) PCHAR LeafName,
    _In_ SIZE_T LeafNameSize)
{
    CHAR Component[256];
    PCSTR p = Path;
    PCSTR SlashPos;
    ULONG CurrentCluster;

    /* Skip leading slashes */
    while (*p == '/')
        p++;

    /* Empty path means root directory itself */
    if (*p == '\0')
    {
        *ParentCluster = Writer->RootDirCluster;
        LeafName[0] = '\0';
        return TRUE;
    }

    CurrentCluster = Writer->RootDirCluster;

    while (*p)
    {
        SIZE_T Len;

        /* Find next slash or end */
        SlashPos = strchr(p, '/');
        if (SlashPos)
            Len = (SIZE_T)(SlashPos - p);
        else
            Len = strlen(p);

        if (Len == 0)
        {
            p++;
            continue;
        }

        if (Len >= sizeof(Component))
            return FALSE;

        RtlCopyMemory(Component, p, Len);
        Component[Len] = '\0';

        if (SlashPos)
        {
            /* This is an intermediate directory — look it up */
            BOOLEAN IsDir = FALSE;
            ULONG ChildCluster = Fat32FindEntryInDirectory(Writer, CurrentCluster,
                                                            Component, &IsDir);
            if (ChildCluster == 0 || !IsDir)
            {
                WARN("Fat32ResolvePath: component '%s' not found or not a directory\n",
                     Component);
                return FALSE;
            }
            CurrentCluster = ChildCluster;
            p = SlashPos + 1;
        }
        else
        {
            /* This is the leaf name */
            if (Len >= LeafNameSize)
                return FALSE;
            RtlCopyMemory(LeafName, Component, Len);
            LeafName[Len] = '\0';
            *ParentCluster = CurrentCluster;
            return TRUE;
        }
    }

    /* Path ended with a slash — treat as directory reference */
    *ParentCluster = CurrentCluster;
    LeafName[0] = '\0';
    return TRUE;
}

/* ========================================================================== */
/*  Public API                                                                */
/* ========================================================================== */

BOOLEAN
Fat32WriterInit(
    _Out_ PFAT32_WRITER Writer,
    _In_ PVOID VolumeBase,
    _In_ ULONGLONG VolumeSize,
    _In_ PRAMDISK_FAT32_LAYOUT Layout)
{
    ULONGLONG DataSectors;

    if (!Writer || !VolumeBase || VolumeSize == 0 || !Layout)
        return FALSE;

    RtlZeroMemory(Writer, sizeof(*Writer));

    Writer->VolumeBase = (PUCHAR)VolumeBase;
    Writer->VolumeSize = VolumeSize;
    Writer->BytesPerSector = Layout->BytesPerSector;
    Writer->SectorsPerCluster = Layout->SectorsPerCluster;
    Writer->BytesPerCluster = Layout->BytesPerSector * Layout->SectorsPerCluster;
    Writer->ReservedSectors = Layout->ReservedSectors;
    Writer->NumberOfFats = Layout->NumberOfFats;
    Writer->FatSizeSectors = Layout->FatSizeSectors;
    Writer->FirstDataSector = Layout->FirstDataSector;
    Writer->RootDirCluster = Layout->RootDirFirstCluster;

    /* Calculate total data clusters */
    DataSectors = (ULONGLONG)Layout->TotalSectors - Layout->FirstDataSector;
    Writer->TotalClusters = (ULONG)(DataSectors / Layout->SectorsPerCluster);

    /* Set up direct FAT pointers */
    Writer->Fat0 = (PULONG)(Writer->VolumeBase +
                   (ULONGLONG)Layout->ReservedSectors * Layout->BytesPerSector);

    if (Layout->NumberOfFats >= 2)
    {
        Writer->Fat1 = (PULONG)((PUCHAR)Writer->Fat0 +
                       (ULONGLONG)Layout->FatSizeSectors * Layout->BytesPerSector);
    }

    /* Cluster 2 is root dir (already allocated by format), start at 3 */
    Writer->NextFreeCluster = 3;

    /* Fixed timestamp: 2025-01-01 00:00:00 */
    Writer->FatDate = (USHORT)(((2025 - 1980) << 9) | (1 << 5) | 1);
    Writer->FatTime = 0;

    TRACE("Fat32WriterInit: %lu clusters, %lu bytes/cluster, first data sector %lu\n",
          Writer->TotalClusters, Writer->BytesPerCluster, Writer->FirstDataSector);

    return TRUE;
}

BOOLEAN
Fat32CreateDirectory(
    _Inout_ PFAT32_WRITER Writer,
    _In_ PCSTR Path)
{
    CHAR LeafName[256];
    ULONG ParentCluster;
    ULONG NewCluster;
    PDIRENTRY DotEntries;
    BOOLEAN IsDir;

    if (!Writer || !Path)
        return FALSE;

    /* Resolve the path to get parent cluster and leaf name */
    if (!Fat32ResolvePath(Writer, Path, &ParentCluster, LeafName, sizeof(LeafName)))
        return FALSE;

    /* Empty leaf means the directory already exists (it's the root or trailing slash) */
    if (LeafName[0] == '\0')
        return TRUE;

    /* Check if it already exists */
    {
        ULONG Existing = Fat32FindEntryInDirectory(Writer, ParentCluster,
                                                    LeafName, &IsDir);
        if (Existing != 0)
        {
            if (IsDir)
                return TRUE; /* Already exists as a directory */
            WARN("Fat32CreateDirectory: '%s' exists as a file\n", Path);
            return FALSE;
        }
    }

    /* Allocate one cluster for the new directory */
    if (!Fat32AllocateClusters(Writer, 1, &NewCluster))
        return FALSE;

    /* Zero-fill and write . and .. entries */
    RtlZeroMemory(Fat32ClusterToPointer(Writer, NewCluster), Writer->BytesPerCluster);

    DotEntries = (PDIRENTRY)Fat32ClusterToPointer(Writer, NewCluster);

    /* "." entry — points to itself */
    RtlFillMemory(DotEntries[0].FileName, 11, ' ');
    DotEntries[0].FileName[0] = '.';
    DotEntries[0].Attr = FAT_ATTR_DIRECTORY;
    DotEntries[0].CreateTime = Writer->FatTime;
    DotEntries[0].CreateDate = Writer->FatDate;
    DotEntries[0].LastAccessDate = Writer->FatDate;
    DotEntries[0].Time = Writer->FatTime;
    DotEntries[0].Date = Writer->FatDate;
    DotEntries[0].ClusterHigh = (USHORT)(NewCluster >> 16);
    DotEntries[0].ClusterLow = (USHORT)(NewCluster & 0xFFFF);
    DotEntries[0].Size = 0;

    /* ".." entry — points to parent (0 if parent is root) */
    RtlFillMemory(DotEntries[1].FileName, 11, ' ');
    DotEntries[1].FileName[0] = '.';
    DotEntries[1].FileName[1] = '.';
    DotEntries[1].Attr = FAT_ATTR_DIRECTORY;
    DotEntries[1].CreateTime = Writer->FatTime;
    DotEntries[1].CreateDate = Writer->FatDate;
    DotEntries[1].LastAccessDate = Writer->FatDate;
    DotEntries[1].Time = Writer->FatTime;
    DotEntries[1].Date = Writer->FatDate;
    {
        /* FAT spec: ".." cluster is 0 when parent is root */
        ULONG ParentRef = (ParentCluster == Writer->RootDirCluster) ? 0 : ParentCluster;
        DotEntries[1].ClusterHigh = (USHORT)(ParentRef >> 16);
        DotEntries[1].ClusterLow = (USHORT)(ParentRef & 0xFFFF);
    }
    DotEntries[1].Size = 0;

    /* Add directory entry in parent */
    return Fat32AddDirectoryEntry(Writer, ParentCluster, LeafName,
                                  NewCluster, 0, FAT_ATTR_DIRECTORY);
}

BOOLEAN
Fat32CreateFileEx(
    _Inout_ PFAT32_WRITER Writer,
    _In_ PCSTR Path,
    _In_ ULONG FileSize,
    _Out_ PUCHAR *DataPointer)
{
    CHAR LeafName[256];
    ULONG ParentCluster;
    ULONG FirstCluster = 0;
    ULONG ClustersNeeded;

    if (!Writer || !Path || !DataPointer)
        return FALSE;

    *DataPointer = NULL;

    if (!Fat32ResolvePath(Writer, Path, &ParentCluster, LeafName, sizeof(LeafName)))
        return FALSE;

    if (LeafName[0] == '\0')
    {
        WARN("Fat32CreateFileEx: empty leaf name for path '%s'\n", Path);
        return FALSE;
    }

    /* Allocate clusters for file data */
    if (FileSize > 0)
    {
        ClustersNeeded = (FileSize + Writer->BytesPerCluster - 1) / Writer->BytesPerCluster;
        if (!Fat32AllocateClusters(Writer, ClustersNeeded, &FirstCluster))
            return FALSE;

        /* Zero-fill the last cluster's tail to avoid leaking stale data */
        {
            ULONG TailPadding = (ClustersNeeded * Writer->BytesPerCluster) - FileSize;
            if (TailPadding > 0)
            {
                PUCHAR TailStart = Fat32ClusterToPointer(Writer, FirstCluster) + FileSize;
                RtlZeroMemory(TailStart, TailPadding);
            }
        }

        *DataPointer = Fat32ClusterToPointer(Writer, FirstCluster);
    }

    /* Add directory entry */
    if (!Fat32AddDirectoryEntry(Writer, ParentCluster, LeafName,
                                FirstCluster, FileSize, FAT_ATTR_ARCHIVE))
    {
        /* Note: clusters are "leaked" but this is a fatal error path anyway */
        return FALSE;
    }

    return TRUE;
}
