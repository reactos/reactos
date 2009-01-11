/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/dirwr.c
 * PURPOSE:          VFAT Filesystem : write in directory
 *
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/*
 * update an existing FAT entry
 */
NTSTATUS
VfatUpdateEntry(
    IN PVFATFCB pFcb)
{
    PVOID Context;
    PDIR_ENTRY PinEntry;
    LARGE_INTEGER Offset;
    ULONG SizeDirEntry;
    ULONG dirIndex;

    ASSERT(pFcb);

    if (pFcb->Flags & FCB_IS_FATX_ENTRY)
    {
        SizeDirEntry = sizeof(FATX_DIR_ENTRY);
        dirIndex = pFcb->startIndex;
    }
    else
    {
        SizeDirEntry = sizeof(FAT_DIR_ENTRY);
        dirIndex = pFcb->dirIndex;
    }

    DPRINT("updEntry dirIndex %d, PathName \'%wZ\'\n", dirIndex, &pFcb->PathNameU);

    if (vfatFCBIsRoot(pFcb) || (pFcb->Flags & (FCB_IS_FAT|FCB_IS_VOLUME)))
    {
        return STATUS_SUCCESS;
    }

    ASSERT(pFcb->parentFcb);

    Offset.u.HighPart = 0;
    Offset.u.LowPart = dirIndex * SizeDirEntry;
    if (CcPinRead(pFcb->parentFcb->FileObject, &Offset, SizeDirEntry,
        TRUE, &Context, (PVOID*)&PinEntry))
    {
        pFcb->Flags &= ~FCB_IS_DIRTY;
        RtlCopyMemory(PinEntry, &pFcb->entry, SizeDirEntry);
        CcSetDirtyPinnedData(Context, NULL);
        CcUnpinData(Context);
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("Failed write to \'%wZ\'.\n", &pFcb->parentFcb->PathNameU);
        return STATUS_UNSUCCESSFUL;
    }
}

/*
 * try to find contiguous entries frees in directory,
 * extend a directory if is neccesary
 */
BOOLEAN
vfatFindDirSpace(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB pDirFcb,
    IN ULONG nbSlots,
    OUT PULONG start)
{
    LARGE_INTEGER FileOffset;
    ULONG i, count, size, nbFree = 0;
    PDIR_ENTRY pFatEntry;
    PVOID Context = NULL;
    NTSTATUS Status;
    ULONG SizeDirEntry;
    FileOffset.QuadPart = 0;

    if (DeviceExt->Flags & VCB_IS_FATX)
        SizeDirEntry = sizeof(FATX_DIR_ENTRY);
    else
        SizeDirEntry = sizeof(FAT_DIR_ENTRY);

    count = pDirFcb->RFCB.FileSize.u.LowPart / SizeDirEntry;
    size = DeviceExt->FatInfo.BytesPerCluster / SizeDirEntry;
    for (i = 0; i < count; i++, pFatEntry = (PDIR_ENTRY)((ULONG_PTR)pFatEntry + SizeDirEntry))
    {
        if (Context == NULL || (i % size) == 0)
        {
            if (Context)
            {
                CcUnpinData(Context);
            }
            /* FIXME: check return value */
            CcPinRead(pDirFcb->FileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster,
                      TRUE, &Context, (PVOID*)&pFatEntry);
            FileOffset.u.LowPart += DeviceExt->FatInfo.BytesPerCluster;
        }
        if (ENTRY_END(DeviceExt, pFatEntry))
        {
            break;
        }
        if (ENTRY_DELETED(DeviceExt, pFatEntry))
        {
            nbFree++;
        }
        else
        {
            nbFree = 0;
        }
        if (nbFree == nbSlots)
        {
            break;
        }
    }
    if (Context)
    {
        CcUnpinData(Context);
        Context = NULL;
    }
    if (nbFree == nbSlots)
    {
        /* found enough contiguous free slots */
        *start = i - nbSlots + 1;
    }
    else
    {
        *start = i - nbFree;
        if (*start + nbSlots > count)
        {
            LARGE_INTEGER AllocationSize;
            /* extend the directory */
            if (vfatFCBIsRoot(pDirFcb) && DeviceExt->FatInfo.FatType != FAT32)
            {
                /* We can't extend a root directory on a FAT12/FAT16/FATX partition */
                return FALSE;
            }
            AllocationSize.QuadPart = pDirFcb->RFCB.FileSize.u.LowPart + DeviceExt->FatInfo.BytesPerCluster;
            Status = VfatSetAllocationSizeInformation(pDirFcb->FileObject, pDirFcb,
                                                      DeviceExt, &AllocationSize);
            if (!NT_SUCCESS(Status))
            {
                return FALSE;
            }
            /* clear the new dir cluster */
            FileOffset.u.LowPart = (ULONG)(pDirFcb->RFCB.FileSize.QuadPart -
                                           DeviceExt->FatInfo.BytesPerCluster);
            CcPinRead(pDirFcb->FileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster,
                      TRUE, &Context, (PVOID*)&pFatEntry);
            if (DeviceExt->Flags & VCB_IS_FATX)
                memset(pFatEntry, 0xff, DeviceExt->FatInfo.BytesPerCluster);
            else
                RtlZeroMemory(pFatEntry, DeviceExt->FatInfo.BytesPerCluster);
        }
        else if (*start + nbSlots < count)
        {
            /* clear the entry after the last new entry */
            FileOffset.u.LowPart = (*start + nbSlots) * SizeDirEntry;
            CcPinRead(pDirFcb->FileObject, &FileOffset, SizeDirEntry,
                TRUE, &Context, (PVOID*)&pFatEntry);
            if (DeviceExt->Flags & VCB_IS_FATX)
                memset(pFatEntry, 0xff, SizeDirEntry);
            else
                RtlZeroMemory(pFatEntry, SizeDirEntry);
        }
        if (Context)
        {
            CcSetDirtyPinnedData(Context, NULL);
            CcUnpinData(Context);
        }
    }
    DPRINT("nbSlots %d nbFree %d, entry number %d\n", nbSlots, nbFree, *start);
    return TRUE;
}

/*
  create a new FAT entry
*/
static NTSTATUS
FATAddEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PUNICODE_STRING NameU,
    IN PVFATFCB* Fcb,
    IN PVFATFCB ParentFcb,
    IN ULONG RequestedOptions,
    IN UCHAR ReqAttr)
{
    PVOID Context = NULL;
    PFAT_DIR_ENTRY pFatEntry;
    slot *pSlots;
    USHORT nbSlots = 0, j, posCar;
    PUCHAR Buffer;
    BOOLEAN needTilde = FALSE, needLong = FALSE;
    BOOLEAN lCaseBase = FALSE, uCaseBase, lCaseExt = FALSE, uCaseExt;
    ULONG CurrentCluster;
    LARGE_INTEGER SystemTime, FileOffset;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG size;
    long i;

    OEM_STRING NameA;
    CHAR aName[13];
    BOOLEAN IsNameLegal;
    BOOLEAN SpacesFound;

    VFAT_DIRENTRY_CONTEXT DirContext;
    WCHAR LongNameBuffer[LONGNAME_MAX_LENGTH + 1];
    WCHAR ShortNameBuffer[13];

    DPRINT("addEntry: Name='%wZ', Dir='%wZ'\n", NameU, &ParentFcb->PathNameU);

    DirContext.LongNameU = *NameU;

    /* nb of entry needed for long name+normal entry */
    nbSlots = (DirContext.LongNameU.Length / sizeof(WCHAR) + 12) / 13 + 1;
    DPRINT("NameLen= %d, nbSlots =%d\n", DirContext.LongNameU.Length / sizeof(WCHAR), nbSlots);
    Buffer = ExAllocatePoolWithTag(NonPagedPool, (nbSlots - 1) * sizeof(FAT_DIR_ENTRY), TAG_VFAT);
    if (Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Buffer, (nbSlots - 1) * sizeof(FAT_DIR_ENTRY));
    pSlots = (slot *) Buffer;

    NameA.Buffer = aName;
    NameA.Length = 0;
    NameA.MaximumLength = sizeof(aName);

    DirContext.ShortNameU.Buffer = ShortNameBuffer;
    DirContext.ShortNameU.Length = 0;
    DirContext.ShortNameU.MaximumLength = sizeof(ShortNameBuffer);

    RtlZeroMemory(&DirContext.DirEntry.Fat, sizeof(FAT_DIR_ENTRY));

    IsNameLegal = RtlIsNameLegalDOS8Dot3(&DirContext.LongNameU, &NameA, &SpacesFound);

    if (!IsNameLegal || SpacesFound)
    {
        GENERATE_NAME_CONTEXT NameContext;
        VFAT_DIRENTRY_CONTEXT SearchContext;
        WCHAR ShortSearchName[13];
        needTilde = TRUE;
        needLong = TRUE;
        RtlZeroMemory(&NameContext, sizeof(GENERATE_NAME_CONTEXT));
        SearchContext.LongNameU.Buffer = LongNameBuffer;
        SearchContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
        SearchContext.ShortNameU.Buffer = ShortSearchName;
        SearchContext.ShortNameU.MaximumLength = sizeof(ShortSearchName);

        for (i = 0; i < 100; i++)
        {
            RtlGenerate8dot3Name(&DirContext.LongNameU, FALSE, &NameContext, &DirContext.ShortNameU);
            DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
                                         SearchContext.DirIndex = 0;
            Status = FindFile(DeviceExt, ParentFcb, &DirContext.ShortNameU, &SearchContext, TRUE);
            if (!NT_SUCCESS(Status))
            {
                break;
            }
        }
        if (i == 100) /* FIXME : what to do after this ? */
        {
            ExFreePoolWithTag(Buffer, TAG_VFAT);
            return STATUS_UNSUCCESSFUL;
        }
        IsNameLegal = RtlIsNameLegalDOS8Dot3(&DirContext.ShortNameU, &NameA, &SpacesFound);
        aName[NameA.Length]=0;
    }
    else
    {
        aName[NameA.Length] = 0;
        for (posCar = 0; posCar < DirContext.LongNameU.Length / sizeof(WCHAR); posCar++)
        {
            if (DirContext.LongNameU.Buffer[posCar] == L'.')
            {
                break;
            }
        }
        /* check if the name and the extension contains upper case characters */
        RtlDowncaseUnicodeString(&DirContext.ShortNameU, &DirContext.LongNameU, FALSE);
        DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
        uCaseBase = wcsncmp(DirContext.LongNameU.Buffer,
                            DirContext.ShortNameU.Buffer, posCar) ? TRUE : FALSE;
        if (posCar < DirContext.LongNameU.Length/sizeof(WCHAR))
        {
            i = DirContext.LongNameU.Length / sizeof(WCHAR) - posCar;
            uCaseExt = wcsncmp(DirContext.LongNameU.Buffer + posCar,
                               DirContext.ShortNameU.Buffer + posCar, i) ? TRUE : FALSE;
        }
        else
        {
            uCaseExt = FALSE;
        }
        /* check if the name and the extension contains lower case characters */
        RtlUpcaseUnicodeString(&DirContext.ShortNameU, &DirContext.LongNameU, FALSE);
        DirContext.ShortNameU.Buffer[DirContext.ShortNameU.Length / sizeof(WCHAR)] = 0;
        lCaseBase = wcsncmp(DirContext.LongNameU.Buffer,
                            DirContext.ShortNameU.Buffer, posCar) ? TRUE : FALSE;
        if (posCar < DirContext.LongNameU.Length / sizeof(WCHAR))
        {
            i = DirContext.LongNameU.Length / sizeof(WCHAR) - posCar;
            lCaseExt = wcsncmp(DirContext.LongNameU.Buffer + posCar,
                               DirContext.ShortNameU.Buffer + posCar, i) ? TRUE : FALSE;
        }
        else
        {
            lCaseExt = FALSE;
        }
        if ((lCaseBase && uCaseBase) || (lCaseExt && uCaseExt))
        {
            needLong = TRUE;
        }
    }
    DPRINT("'%s', '%wZ', needTilde=%d, needLong=%d\n",
           aName, &DirContext.LongNameU, needTilde, needLong);
    memset(DirContext.DirEntry.Fat.ShortName, ' ', 11);
    for (i = 0; i < 8 && aName[i] && aName[i] != '.'; i++)
    {
        DirContext.DirEntry.Fat.Filename[i] = aName[i];
    }
    if (aName[i] == '.')
    {
        i++;
        for (j = 0; j < 3 && aName[i]; j++, i++)
        {
            DirContext.DirEntry.Fat.Ext[j] = aName[i];
        }
    }
    if (DirContext.DirEntry.Fat.Filename[0] == 0xe5)
    {
        DirContext.DirEntry.Fat.Filename[0] = 0x05;
    }

    if (needLong)
    {
        RtlCopyMemory(LongNameBuffer, DirContext.LongNameU.Buffer, DirContext.LongNameU.Length);
        DirContext.LongNameU.Buffer = LongNameBuffer;
        DirContext.LongNameU.MaximumLength = sizeof(LongNameBuffer);
        DirContext.LongNameU.Buffer[DirContext.LongNameU.Length / sizeof(WCHAR)] = 0;
        memset(DirContext.LongNameU.Buffer + DirContext.LongNameU.Length / sizeof(WCHAR) + 1, 0xff,
               DirContext.LongNameU.MaximumLength - DirContext.LongNameU.Length - sizeof(WCHAR));
    }
    else
    {
        nbSlots = 1;
        if (lCaseBase)
        {
            DirContext.DirEntry.Fat.lCase |= VFAT_CASE_LOWER_BASE;
        }
        if (lCaseExt)
        {
            DirContext.DirEntry.Fat.lCase |= VFAT_CASE_LOWER_EXT;
        }
    }

    DPRINT ("dos name=%11.11s\n", DirContext.DirEntry.Fat.Filename);

    /* set attributes */
    DirContext.DirEntry.Fat.Attrib = ReqAttr;
    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
        DirContext.DirEntry.Fat.Attrib |= FILE_ATTRIBUTE_DIRECTORY;
    }
    /* set dates and times */
    KeQuerySystemTime(&SystemTime);
    FsdSystemTimeToDosDateTime(DeviceExt, &SystemTime, &DirContext.DirEntry.Fat.CreationDate,
                               &DirContext.DirEntry.Fat.CreationTime);
    DirContext.DirEntry.Fat.UpdateDate = DirContext.DirEntry.Fat.CreationDate;
    DirContext.DirEntry.Fat.UpdateTime = DirContext.DirEntry.Fat.CreationTime;
    DirContext.DirEntry.Fat.AccessDate = DirContext.DirEntry.Fat.CreationDate;

    if (needLong)
    {
        /* calculate checksum for 8.3 name */
        for (pSlots[0].alias_checksum = 0, i = 0; i < 11; i++)
        {
            pSlots[0].alias_checksum = (((pSlots[0].alias_checksum & 1) << 7
                                     | ((pSlots[0].alias_checksum & 0xfe) >> 1))
                                     + DirContext.DirEntry.Fat.ShortName[i]);
        }
        /* construct slots and entry */
        for (i = nbSlots - 2; i >= 0; i--)
        {
            DPRINT("construct slot %d\n", i);
            pSlots[i].attr = 0xf;
            if (i)
            {
                pSlots[i].id = (unsigned char)(nbSlots - i - 1);
            }
            else
            {
                pSlots[i].id = (unsigned char)(nbSlots - i - 1 + 0x40);
            }
            pSlots[i].alias_checksum = pSlots[0].alias_checksum;
            RtlCopyMemory(pSlots[i].name0_4, DirContext.LongNameU.Buffer + (nbSlots - i - 2) * 13, 10);
            RtlCopyMemory(pSlots[i].name5_10, DirContext.LongNameU.Buffer + (nbSlots - i - 2) * 13 + 5, 12);
            RtlCopyMemory(pSlots[i].name11_12, DirContext.LongNameU.Buffer + (nbSlots - i - 2) * 13 + 11, 4);
        }
    }
    /* try to find nbSlots contiguous entries frees in directory */
    if (!vfatFindDirSpace(DeviceExt, ParentFcb, nbSlots, &DirContext.StartIndex))
    {
        ExFreePoolWithTag(Buffer, TAG_VFAT);
        return STATUS_DISK_FULL;
    }
    DirContext.DirIndex = DirContext.StartIndex + nbSlots - 1;
    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
        CurrentCluster = 0;
        Status = NextCluster(DeviceExt, 0, &CurrentCluster, TRUE);
        if (CurrentCluster == 0xffffffff || !NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(Buffer, TAG_VFAT);
            if (!NT_SUCCESS(Status))
            {
                return Status;
            }
            return STATUS_DISK_FULL;
        }
        if (DeviceExt->FatInfo.FatType == FAT32)
        {
            DirContext.DirEntry.Fat.FirstClusterHigh = (unsigned short)(CurrentCluster >> 16);
        }
        DirContext.DirEntry.Fat.FirstCluster = (unsigned short)CurrentCluster;
    }

    i = DeviceExt->FatInfo.BytesPerCluster / sizeof(FAT_DIR_ENTRY);
    FileOffset.u.HighPart = 0;
    FileOffset.u.LowPart = DirContext.StartIndex * sizeof(FAT_DIR_ENTRY);
    if (DirContext.StartIndex / i == DirContext.DirIndex / i)
    {
        /* one cluster */
        CcPinRead(ParentFcb->FileObject, &FileOffset, nbSlots * sizeof(FAT_DIR_ENTRY),
                  TRUE, &Context, (PVOID*)&pFatEntry);
        if (nbSlots > 1)
        {
            RtlCopyMemory(pFatEntry, Buffer, (nbSlots - 1) * sizeof(FAT_DIR_ENTRY));
        }
        RtlCopyMemory(pFatEntry + (nbSlots - 1), &DirContext.DirEntry.Fat, sizeof(FAT_DIR_ENTRY));
    }
    else
    {
        /* two clusters */
        size = DeviceExt->FatInfo.BytesPerCluster -
               (DirContext.StartIndex * sizeof(FAT_DIR_ENTRY)) % DeviceExt->FatInfo.BytesPerCluster;
        i = size / sizeof(FAT_DIR_ENTRY);
        CcPinRead(ParentFcb->FileObject, &FileOffset, size, TRUE,
                  &Context, (PVOID*)&pFatEntry);
        RtlCopyMemory(pFatEntry, Buffer, size);
        CcSetDirtyPinnedData(Context, NULL);
        CcUnpinData(Context);
        FileOffset.u.LowPart += size;
        CcPinRead(ParentFcb->FileObject, &FileOffset,
                  nbSlots * sizeof(FAT_DIR_ENTRY) - size,
                  TRUE, &Context, (PVOID*)&pFatEntry);
        if (nbSlots - 1 > i)
        {
            RtlCopyMemory(pFatEntry, (PVOID)(Buffer + size), (nbSlots - 1 - i) * sizeof(FAT_DIR_ENTRY));
        }
        RtlCopyMemory(pFatEntry + nbSlots - 1 - i, &DirContext.DirEntry.Fat, sizeof(FAT_DIR_ENTRY));
    }
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);

    /* FIXME: check status */
    vfatMakeFCBFromDirEntry(DeviceExt, ParentFcb, &DirContext, Fcb);

    DPRINT("new : entry=%11.11s\n", (*Fcb)->entry.Fat.Filename);
    DPRINT("new : entry=%11.11s\n", DirContext.DirEntry.Fat.Filename);

    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
        FileOffset.QuadPart = 0;
        CcPinRead((*Fcb)->FileObject, &FileOffset, DeviceExt->FatInfo.BytesPerCluster, TRUE,
                  &Context, (PVOID*)&pFatEntry);
        /* clear the new directory cluster */
        RtlZeroMemory(pFatEntry, DeviceExt->FatInfo.BytesPerCluster);
        /* create '.' and '..' */
        RtlCopyMemory(&pFatEntry[0].Attrib, &DirContext.DirEntry.Fat.Attrib, sizeof(FAT_DIR_ENTRY) - 11);
        RtlCopyMemory(pFatEntry[0].ShortName, ".          ", 11);
        RtlCopyMemory(&pFatEntry[1].Attrib, &DirContext.DirEntry.Fat.Attrib, sizeof(FAT_DIR_ENTRY) - 11);
        RtlCopyMemory(pFatEntry[1].ShortName, "..         ", 11);
        pFatEntry[1].FirstCluster = ParentFcb->entry.Fat.FirstCluster;
        pFatEntry[1].FirstClusterHigh = ParentFcb->entry.Fat.FirstClusterHigh;
        if (vfatFCBIsRoot(ParentFcb))
        {
            pFatEntry[1].FirstCluster = 0;
            pFatEntry[1].FirstClusterHigh = 0;
        }
        CcSetDirtyPinnedData(Context, NULL);
        CcUnpinData(Context);
    }
    ExFreePoolWithTag(Buffer, TAG_VFAT);
    DPRINT("addentry ok\n");
    return STATUS_SUCCESS;
}

/*
  create a new FAT entry
*/
static NTSTATUS
FATXAddEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PUNICODE_STRING NameU,
    IN PVFATFCB* Fcb,
    IN PVFATFCB ParentFcb,
    IN ULONG RequestedOptions,
    IN UCHAR ReqAttr)
{
    PVOID Context = NULL;
    LARGE_INTEGER SystemTime, FileOffset;
    OEM_STRING NameA;
    VFAT_DIRENTRY_CONTEXT DirContext;
    PFATX_DIR_ENTRY pFatXDirEntry;
    ULONG Index;

    DPRINT("addEntry: Name='%wZ', Dir='%wZ'\n", NameU, &ParentFcb->PathNameU);

    DirContext.LongNameU = *NameU;

    if (DirContext.LongNameU.Length / sizeof(WCHAR) > 42)
    {
        /* name too long */
        return STATUS_NAME_TOO_LONG;
    }

    /* try to find 1 entry free in directory */
    if (!vfatFindDirSpace(DeviceExt, ParentFcb, 1, &DirContext.StartIndex))
    {
        return STATUS_DISK_FULL;
    }
    Index = DirContext.DirIndex = DirContext.StartIndex;
    if (!vfatFCBIsRoot(ParentFcb))
    {
        DirContext.DirIndex += 2;
        DirContext.StartIndex += 2;
    }

    DirContext.ShortNameU.Buffer = 0;
    DirContext.ShortNameU.Length = 0;
    DirContext.ShortNameU.MaximumLength = 0;
    RtlZeroMemory(&DirContext.DirEntry.FatX, sizeof(FATX_DIR_ENTRY));
    memset(DirContext.DirEntry.FatX.Filename, 0xff, 42);
    DirContext.DirEntry.FatX.FirstCluster = 0;
    DirContext.DirEntry.FatX.FileSize = 0;

    /* set file name */
    NameA.Buffer = (PCHAR)DirContext.DirEntry.FatX.Filename;
    NameA.Length = 0;
    NameA.MaximumLength = 42;
    RtlUnicodeStringToOemString(&NameA, &DirContext.LongNameU, FALSE);
    DirContext.DirEntry.FatX.FilenameLength = (unsigned char)NameA.Length;

    /* set attributes */
    DirContext.DirEntry.FatX.Attrib = ReqAttr;
    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
        DirContext.DirEntry.FatX.Attrib |= FILE_ATTRIBUTE_DIRECTORY;
    }

    /* set dates and times */
    KeQuerySystemTime(&SystemTime);
    FsdSystemTimeToDosDateTime(DeviceExt, &SystemTime, &DirContext.DirEntry.FatX.CreationDate,
                               &DirContext.DirEntry.FatX.CreationTime);
    DirContext.DirEntry.FatX.UpdateDate = DirContext.DirEntry.FatX.CreationDate;
    DirContext.DirEntry.FatX.UpdateTime = DirContext.DirEntry.FatX.CreationTime;
    DirContext.DirEntry.FatX.AccessDate = DirContext.DirEntry.FatX.CreationDate;
    DirContext.DirEntry.FatX.AccessTime = DirContext.DirEntry.FatX.CreationTime;

    /* add entry into parent directory */
    FileOffset.u.HighPart = 0;
    FileOffset.u.LowPart = Index * sizeof(FATX_DIR_ENTRY);
    CcPinRead(ParentFcb->FileObject, &FileOffset, sizeof(FATX_DIR_ENTRY),
              TRUE, &Context, (PVOID*)&pFatXDirEntry);
    RtlCopyMemory(pFatXDirEntry, &DirContext.DirEntry.FatX, sizeof(FATX_DIR_ENTRY));
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);

    /* FIXME: check status */
    vfatMakeFCBFromDirEntry(DeviceExt, ParentFcb, &DirContext, Fcb);

    DPRINT("addentry ok\n");
    return STATUS_SUCCESS;
}

NTSTATUS
VfatAddEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PUNICODE_STRING NameU,
    IN PVFATFCB *Fcb,
    IN PVFATFCB ParentFcb,
    IN ULONG RequestedOptions,
    IN UCHAR ReqAttr)
{
    if (DeviceExt->Flags & VCB_IS_FATX)
        return FATXAddEntry(DeviceExt, NameU, Fcb, ParentFcb, RequestedOptions, ReqAttr);
    else
        return FATAddEntry(DeviceExt, NameU, Fcb, ParentFcb, RequestedOptions, ReqAttr);
}

/*
 * deleting an existing FAT entry
 */
static NTSTATUS
FATDelEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB pFcb)
{
    ULONG CurrentCluster = 0, NextCluster, i;
    PVOID Context = NULL;
    LARGE_INTEGER Offset;
    PFAT_DIR_ENTRY pDirEntry;

    ASSERT(pFcb);
    ASSERT(pFcb->parentFcb);

    DPRINT("delEntry PathName \'%wZ\'\n", &pFcb->PathNameU);
    DPRINT("delete entry: %d to %d\n", pFcb->startIndex, pFcb->dirIndex);
    Offset.u.HighPart = 0;
    for (i = pFcb->startIndex; i <= pFcb->dirIndex; i++)
    {
        if (Context == NULL || ((i * sizeof(FAT_DIR_ENTRY)) % PAGE_SIZE) == 0)
        {
            if (Context)
            {
                CcSetDirtyPinnedData(Context, NULL);
                CcUnpinData(Context);
            }
            Offset.u.LowPart = (i * sizeof(FAT_DIR_ENTRY) / PAGE_SIZE) * PAGE_SIZE;
            CcPinRead(pFcb->parentFcb->FileObject, &Offset, PAGE_SIZE, TRUE,
                      &Context, (PVOID*)&pDirEntry);
        }
        pDirEntry[i % (PAGE_SIZE / sizeof(FAT_DIR_ENTRY))].Filename[0] = 0xe5;
        if (i == pFcb->dirIndex)
        {
            CurrentCluster =
            vfatDirEntryGetFirstCluster(DeviceExt,
                                        (PDIR_ENTRY)&pDirEntry[i % (PAGE_SIZE / sizeof(FAT_DIR_ENTRY))]);
        }
    }
    if (Context)
    {
        CcSetDirtyPinnedData(Context, NULL);
        CcUnpinData(Context);
    }

    while (CurrentCluster && CurrentCluster != 0xffffffff)
    {
        GetNextCluster(DeviceExt, CurrentCluster, &NextCluster);
        /* FIXME: check status */
        WriteCluster(DeviceExt, CurrentCluster, 0);
        CurrentCluster = NextCluster;
    }
    return STATUS_SUCCESS;
}

/*
 * deleting an existing FAT entry
 */
static NTSTATUS
FATXDelEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB pFcb)
{
    ULONG CurrentCluster = 0, NextCluster;
    PVOID Context = NULL;
    LARGE_INTEGER Offset;
    PFATX_DIR_ENTRY pDirEntry;
    ULONG StartIndex;

    ASSERT(pFcb);
    ASSERT(pFcb->parentFcb);
    ASSERT(pFcb->Flags & FCB_IS_FATX_ENTRY);

    StartIndex = pFcb->startIndex;

    DPRINT("delEntry PathName \'%wZ\'\n", &pFcb->PathNameU);
    DPRINT("delete entry: %d\n", StartIndex);
    Offset.u.HighPart = 0;
    Offset.u.LowPart = (StartIndex * sizeof(FATX_DIR_ENTRY) / PAGE_SIZE) * PAGE_SIZE;
    if (!CcPinRead(pFcb->parentFcb->FileObject, &Offset, PAGE_SIZE, TRUE,
                   &Context, (PVOID*)&pDirEntry))
    {
        DPRINT1("CcPinRead(Offset %x:%x, Length %d) failed\n", Offset.u.HighPart, Offset.u.LowPart, PAGE_SIZE);
        return STATUS_UNSUCCESSFUL;
    }
    pDirEntry = &pDirEntry[StartIndex % (PAGE_SIZE / sizeof(FATX_DIR_ENTRY))];
    pDirEntry->FilenameLength = 0xe5;
    CurrentCluster = vfatDirEntryGetFirstCluster(DeviceExt,
                                                 (PDIR_ENTRY)pDirEntry);
    CcSetDirtyPinnedData(Context, NULL);
    CcUnpinData(Context);

    while (CurrentCluster && CurrentCluster != 0xffffffff)
    {
        GetNextCluster(DeviceExt, CurrentCluster, &NextCluster);
        /* FIXME: check status */
        WriteCluster(DeviceExt, CurrentCluster, 0);
        CurrentCluster = NextCluster;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
VfatDelEntry(
    IN PDEVICE_EXTENSION DeviceExt,
    IN PVFATFCB pFcb)
{
    if (DeviceExt->Flags & VCB_IS_FATX)
        return FATXDelEntry(DeviceExt, pFcb);
    else
        return FATDelEntry(DeviceExt, pFcb);
}

/* EOF */
