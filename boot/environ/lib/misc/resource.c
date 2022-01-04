/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/resource.c
 * PURPOSE:         Boot Library Resource Functions
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID ResPeImageBase;
PVOID ResPeImageEnd;
PVOID ResRootDirectory;

PVOID ResPeImageBasePrimary;
PVOID ResPeImageEndPrimary;
PVOID ResRootDirectoryPrimary;
ULONG_PTR ResRootDirectoryPrimaryOffset;
ULONG_PTR ResRootDirectoryOffset;
ULONG_PTR ResRootDirectoryFallbackOffset;
PVOID ResPeImageBaseFallback;
PVOID ResPeImageEndFallback;
PVOID ResRootDirectoryFallback;

BOOLEAN ResLoadedFontFiles;
PVOID ResMuiImageBase;
ULONG_PTR ResMuiImageSize;

PWCHAR ResLocale;

/* FUNCTIONS *****************************************************************/

NTSTATUS
ResSelectLocale (
    _In_ BOOLEAN Primary
    )
{
    NTSTATUS Status;

    /* Check if we're using the primary (MUI) or fallback resources */
    if (Primary)
    {
        /* Use the primary ones */
        ResRootDirectory = ResRootDirectoryPrimary;
        ResRootDirectoryOffset = ResRootDirectoryPrimaryOffset;
        ResPeImageBase = ResPeImageBasePrimary;
        ResPeImageEnd = ResPeImageEndPrimary;

        /* Register the locale with the display */
        Status = BlpDisplayRegisterLocale(ResLocale);
    }

    /* Check if that failed, or if we're using fallback */
    if (!(Primary) || !(NT_SUCCESS(Status)))
    {
        /* Set the fallback pointers */
        ResRootDirectory = ResRootDirectoryFallback;
        ResRootDirectoryOffset = ResRootDirectoryFallbackOffset;
        ResPeImageBase = ResPeImageBaseFallback;
        ResPeImageEnd = ResPeImageEndFallback;

        /* Register the fallback (America baby!) locale */
        Status = BlpDisplayRegisterLocale(L"en-US");
        if (!NT_SUCCESS(Status))
        {
            /* Fallback to text mode (yes, this is the API...) */
            return BlDisplaySetScreenResolution();
        }
    }

    /* No fonts loaded -- return failure code */
    ResLoadedFontFiles = FALSE;
    return Status;
}

PIMAGE_RESOURCE_DIRECTORY_ENTRY
ResFindDirectoryEntry (
    _In_ PIMAGE_RESOURCE_DIRECTORY Directory,
    _In_opt_ PUSHORT Id,
    _In_opt_ PWCHAR Name,
    _In_ ULONG_PTR SectionStart
    )
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY EntryTable, IdEntryTable;
    ULONG i;
    SIZE_T NameLength;
    PIMAGE_RESOURCE_DIRECTORY_STRING NameString;

    /* Are we looking by ID or name? */
    if (Id)
    {
        /* By ID, so were we passed a name? */
        if (Name)
        {
            /* That doesn't make sense */
            return NULL;
        }
    }
    else if (!Name)
    {
        /* By name, but we weren't given one. Also bad. */
        return NULL;
    }

    /* Get the table of names */
    EntryTable = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(Directory + 1);

    /* Check if we are doing ID lookup instead */
    if (Id)
    {
        /* The IDs come after the names */
        IdEntryTable = &EntryTable[Directory->NumberOfNamedEntries];

        /* Parse them */
        for (i = 0; i < Directory->NumberOfIdEntries; i++)
        {
            /* Check if the ID matches, or if the wildcard is being used*/
            if ((IdEntryTable[i].Id == *Id) || (*Id == 0xFFFF))
            {
                /* Return a pointer to the data */
                return (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(SectionStart + IdEntryTable[i].OffsetToDirectory);
            }
        }

        /* ID was not found */
        return NULL;
    }

    /* Searching by name, so parse them */
    for (i = 0; i < Directory->NumberOfNamedEntries; i++)
    {
        /* Get the name itself and count its length */
        NameString = (PIMAGE_RESOURCE_DIRECTORY_STRING)(SectionStart + EntryTable[i].NameOffset);
        NameLength = wcslen(Name);

        /* If the length matches, compare the bytes */
        if ((NameLength == NameString->Length) &&
            (RtlCompareMemory(NameString->NameString, Name, NameLength) == NameLength))
        {
            /* They both match, so this is our entry. Return it */
            return (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(SectionStart + EntryTable[i].OffsetToDirectory);
        }
    }

    /* Name was not found */
    return NULL;
}

NTSTATUS
ResFindDataEntryFromImage (
    _In_opt_ PVOID ImageBase,
    _In_opt_ ULONG ImageSize,
    _In_ USHORT DirectoryId,
    _In_ PUSHORT EntryId,
    _In_ PWCHAR Name,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *DataEntryOut,
    _Out_ PVOID* ResourceOut
    )
{
    NTSTATUS Status;
    PIMAGE_SECTION_HEADER ResourceSection;
    PIMAGE_RESOURCE_DIRECTORY ResourceDir, RootDir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY DirEntry;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PVOID Data, DataEnd, ImageEnd;
    BOOLEAN UseFallbackDirectory;

    /* Assume nothing found */
    UseFallbackDirectory = TRUE;
    Status = STATUS_NOT_FOUND;

    /* Are we looking at a particular image? */
    if (ImageBase)
    {
        /* Then make sure we know its size */
        if (!ImageSize)
        {
            return Status;
        }

        /* Find the resource section for it */
        ResourceSection = BlImgFindSection(ImageBase, ImageSize);
        if (!ResourceSection)
        {
            return STATUS_INVALID_IMAGE_FORMAT;
        }

        /* Remember how big the image is, and find the resource directory */
        ImageEnd = (PVOID)((ULONG_PTR)ImageBase + ImageSize);
        RootDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG_PTR)ImageBase +
                                              ResourceSection->VirtualAddress);
        if ((PVOID)RootDir < ImageBase)
        {
            /* It's out of bounds, so bail out */
            return STATUS_INVALID_PARAMETER;
        }

        /* We have a valid directory, don't use fallback for now */
        UseFallbackDirectory = FALSE;
    }
    else
    {
        /* We are using the current library settings instead */
        ImageBase = ResPeImageBase;
        RootDir = ResRootDirectory;
        ImageEnd = ResPeImageEnd;
    }

    /* If we don't have a resource directory, there's nothing to find */
    if (!RootDir)
    {
        return Status;
    }

    /* Try two loops, once for primary, once for fallback */
    while (1)
    {
        /* Find the directory first */
        ResourceDir = (PIMAGE_RESOURCE_DIRECTORY)ResFindDirectoryEntry(RootDir,
                                                                       &DirectoryId,
                                                                       NULL,
                                                                       (ULONG_PTR)RootDir);
        if (ResourceDir)
        {
            break;
        }

        /* We didn't find it -- is it time to use the fallback? */
        if (UseFallbackDirectory)
        {
            /* Were were not using the fallback already? */
            if (RootDir != ResRootDirectoryFallback)
            {
                /* Then attempt with the fallback instead*/
                RootDir = ResRootDirectoryFallback;
                ImageBase = ResPeImageBaseFallback;
                ImageEnd = ResPeImageEndFallback;

                /* Making sure we have one... */
                if (RootDir)
                {
                    continue;
                }
            }
        }

        /* Otherwise, return failure here */
        return Status;
    }

    /* Now that we are in the right directory, lookup the resource */
    ResourceDir = (PIMAGE_RESOURCE_DIRECTORY)ResFindDirectoryEntry(ResourceDir,
                                                                   EntryId,
                                                                   Name,
                                                                   (ULONG_PTR)RootDir);
    if (!ResourceDir)
    {
        return Status;
    }

    /* The entry is right after */
    DirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResourceDir + 1);
    if ((PVOID)DirEntry < (PVOID)ResourceDir)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the data entry for it */
    DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)((ULONG_PTR)RootDir +
                                             DirEntry->OffsetToData);

    /* Check if the data entry is out of bounds */
    if (((PVOID)DataEntry < ImageBase) || ((PVOID)DataEntry > ImageEnd))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Finally read the data offset */
    Data = (PVOID)((ULONG_PTR)ImageBase + DataEntry->OffsetToData);

    /* Check if the data is out of bounds */
    if (((PVOID)Data < ImageBase) || ((PVOID)Data > ImageEnd))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure the data end isn't out of bounds either */
    DataEnd = (PVOID)((ULONG_PTR)Data + DataEntry->Size);
    if (((PVOID)DataEnd < ImageBase) || ((PVOID)DataEnd > ImageEnd))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* We finally made it. Return the entry and the raw data */
    *DataEntryOut = DataEntry;
    *ResourceOut = Data;
    return STATUS_SUCCESS;
}

PWCHAR
BlResourceFindHtml (
    VOID
    )
{
    NTSTATUS Status;
    PIMAGE_RESOURCE_DATA_ENTRY HtmlDataEntry;
    PWCHAR Stylesheet;

    /* Assume failure */
    Stylesheet = NULL;

    /* Look for an RT_HTML resource called BOOTMGR.XSL */
    Status = ResFindDataEntryFromImage(NULL,
                                       0,
                                       23,
                                       NULL,
                                       L"BOOTMGR.XSL",
                                       &HtmlDataEntry,
                                       (PVOID*)&Stylesheet);
    if (!NT_SUCCESS(Status))
    {
        return Stylesheet;
    }

    /* Check for Unicode BOM */
    if (*Stylesheet == 0xFEFF)
    {
        /* Overwrite it, and NULL-terminate */
        RtlMoveMemory(Stylesheet,
                      Stylesheet + 1,
                      HtmlDataEntry->Size - sizeof(WCHAR));
        Stylesheet[(HtmlDataEntry->Size / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    }
    else if (Stylesheet[(HtmlDataEntry->Size / sizeof(WCHAR)) - 1] != UNICODE_NULL)
    {
        /* If it's not NULL-terminated, fail */
        Stylesheet = NULL;
    }

    /* Return it back */
    return Stylesheet;
}

PWCHAR
BlResourceFindMessage (
    _In_ ULONG MsgId
    )
{
    PWCHAR Message;
    PIMAGE_RESOURCE_DIRECTORY ResourceDir;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PMESSAGE_RESOURCE_DATA MsgData;
    PMESSAGE_RESOURCE_ENTRY MsgEntry;
    ULONG i, j;
    USHORT Id;
    PVOID MsgEnd;
    NTSTATUS Status;

    /* Bail out if there's no resource directory */
    Message = NULL;
    if (!ResRootDirectory)
    {
        return Message;
    }

    /* Check if we've loaded fonts already */
    if (!ResLoadedFontFiles)
    {
        /* Nope, load them now */
        Status = BfLoadDeferredFontFiles();
        if (!NT_SUCCESS(Status))
        {
            /* We failed to load fonts, fallback to fallback locale */
            Status = ResSelectLocale(FALSE);
            if (NT_SUCCESS(Status))
            {
                /* Try fonts now */
                Status = BfLoadDeferredFontFiles();
                if (!NT_SUCCESS(Status))
                {
                    /* Still didn't work -- fallback to text mode */
                    EfiPrintf(L"Font loading failed, falling back to text mode\r\n");
                    Status = BlDisplaySetScreenResolution();
                    if (!NT_SUCCESS(Status))
                    {
                        /* That didn't work either. F*ck it. */
                        return Message;
                    }
                }
            }
        }

        /* Now we have a resource directory, and fonts are loaded */
        NT_ASSERT(ResRootDirectory != NULL);
        ResLoadedFontFiles = TRUE;
    }

    /* Go look for RT_MESSAGETABLE */
    Id = 11;
    ResourceDir = (PIMAGE_RESOURCE_DIRECTORY)ResFindDirectoryEntry(ResRootDirectory,
                                                                   &Id,
                                                                   NULL,
                                                                   (ULONG_PTR)ResRootDirectory);
    if (!ResourceDir)
    {
        return Message;
    }

    /* Go look for the first directory in the table */
    Id = 1;
    ResourceDir = (PIMAGE_RESOURCE_DIRECTORY)ResFindDirectoryEntry(ResourceDir,
                                                                   &Id,
                                                                   NULL,
                                                                   (ULONG_PTR)ResRootDirectory);
    if (!ResourceDir)
    {
        return Message;
    }

    /* Go look for any language entry in the table */
    Id = -1;
    DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)ResFindDirectoryEntry(ResourceDir,
                                                                  &Id,
                                                                  NULL,
                                                                  (ULONG_PTR)ResRootDirectory);
    if (!DataEntry)
    {
        return Message;
    }

    /* Get the message data*/
    MsgData = (PMESSAGE_RESOURCE_DATA)((ULONG_PTR)ResRootDirectory +
                                        DataEntry->OffsetToData -
                                        ResRootDirectoryOffset);

    /* Loop through the message blocks */
    for (j = 0; j < MsgData->NumberOfBlocks; j++)
    {
        /* Check if the ID is within this range */
        if ((MsgId >= MsgData->Blocks[j].LowId) &&
            (MsgId <= MsgData->Blocks[j].HighId))
        {
            /* Get the first entry */
            MsgEntry = (PMESSAGE_RESOURCE_ENTRY)((ULONG_PTR)MsgData +
                                                 MsgData->Blocks[j].OffsetToEntries);

            /* Loop till we find the right one */
            for (i = MsgId - MsgData->Blocks[j].LowId; i; --i)
            {
                MsgEntry = (PMESSAGE_RESOURCE_ENTRY)((ULONG_PTR)MsgEntry +
                                                     MsgEntry->Length);
            }

            /* Find where this message ends */
            MsgEnd = (PVOID)((ULONG_PTR)MsgEntry + MsgEntry->Length);

            /* Now make sure that the message is within bounds */
            if ((MsgEnd >= (PVOID)MsgEntry) &&
                ((PVOID)MsgEntry >= ResPeImageBase) &&
                (MsgEnd <= ResPeImageEnd))
            {
                /* If so, read the text associated with it */
                Message = (PWCHAR)MsgEntry->Text;
                break;
            }
        }
    }

    /* Return the text, if one was found */
    return Message;
}

NTSTATUS
BlpResourceInitialize (
    VOID
    )
{
    NTSTATUS Status;
    PIMAGE_SECTION_HEADER ResourceSection;
    PVOID ImageBase;
    ULONG ImageSize, VRes, HRes;
    BOOLEAN UsePrimary;

    /* Default to using fallback */
    UsePrimary = FALSE;

    /* Initialize all globals */
    ResMuiImageBase = 0;
    ResMuiImageSize = 0;
    ResRootDirectoryPrimary = 0;
    ResRootDirectoryPrimaryOffset = 0;
    ResPeImageBasePrimary = 0;
    ResPeImageEndPrimary = 0;
    ResRootDirectoryFallback = 0;
    ResRootDirectoryFallbackOffset = 0;
    ResPeImageBaseFallback = 0;
    ResPeImageEndFallback = 0;
    ResRootDirectory = 0;
    ResRootDirectoryOffset = 0;
    ResPeImageBase = 0;
    ResPeImageEnd = 0;
    ResLoadedFontFiles = 0;

    /* Check if we had allocated a locale already */
    if (ResLocale)
    {
        /* Free it and reset */
        BlMmFreeHeap(ResLocale);
        ResLocale = 0;
    }

    /* Get our base address and size*/
    Status = BlGetApplicationBaseAndSize(&ImageBase, &ImageSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Find our resource section */
    ResourceSection = BlImgFindSection(ImageBase, ImageSize);
    if (ResourceSection)
    {
        /* The resource section will be our fallback. Save down its details */
        ResRootDirectoryFallbackOffset = ResourceSection->VirtualAddress;
        ResPeImageBaseFallback = ImageBase;
        ResPeImageEndFallback = (PVOID)((ULONG_PTR)ImageBase + ImageSize);
        ResRootDirectoryFallback = (PIMAGE_RESOURCE_DIRECTORY)((ULONG_PTR)ImageBase +
                                            ResRootDirectoryFallbackOffset);
    }

    /* Get the current screen resolution and check if we're in graphics mode */
    Status = BlDisplayGetScreenResolution(&HRes, &VRes);
    if ((NT_SUCCESS(Status)) && ((HRes != 640) || (VRes != 200)))
    {
        /* We are... we should load MUI data */
        Status = STATUS_NOT_IMPLEMENTED;//ResInitializeMuiResources();
        if (NT_SUCCESS(Status))
        {
            /* And not rely on the fallback */
            UsePrimary = TRUE;
        }
    }

    /* Load the locale resources */
    return ResSelectLocale(UsePrimary);
}
