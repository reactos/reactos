/*
 *  FreeLoader
 *
 *  Copyright (C) 2014  Timo Kreuzer <timo.kreuzer@reactos.org>
 *                2022  George Bișoc <george.bisoc@reactos.org>
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

#include <freeldr.h>
#include <cmlib.h>
#include "registry.h"
#include <internal/cmboot.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(REGISTRY);

static PCMHIVE CmSystemHive;
static HCELL_INDEX SystemRootCell;

PHHIVE SystemHive = NULL;
HKEY CurrentControlSetKey = NULL;

#define HCI_TO_HKEY(CellIndex)          ((HKEY)(ULONG_PTR)(CellIndex))
#ifndef HKEY_TO_HCI // See also registry.h
#define HKEY_TO_HCI(hKey)               ((HCELL_INDEX)(ULONG_PTR)(hKey))
#endif

#define GET_HHIVE(CmHive)               (&((CmHive)->Hive))
#define GET_HHIVE_FROM_HKEY(hKey)       GET_HHIVE(CmSystemHive)
#define GET_CM_KEY_NODE(hHive, hKey)    ((PCM_KEY_NODE)HvGetCell(hHive, HKEY_TO_HCI(hKey)))

#define GET_HBASE_BLOCK(ChunkBase)      ((PHBASE_BLOCK)ChunkBase)

PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    UNREFERENCED_PARAMETER(Paged);
    return FrLdrHeapAlloc(Size, Tag);
}

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota)
{
    UNREFERENCED_PARAMETER(Quota);
    FrLdrHeapFree(Ptr, 0);
}

/**
 * @brief
 * Initializes a flat hive descriptor for the
 * hive and validates the registry hive.
 * Volatile data is purged during this procedure
 * for initialization.
 *
 * @param[in] CmHive
 * A pointer to a CM (in-memory) hive descriptor
 * containing the hive descriptor to be initialized.
 *
 * @param[in] ChunkBase
 * An arbitrary pointer that points to the registry
 * chunk base. This pointer serves as the base block
 * containing the hive file header data.
 *
 * @param[in] LoadAlternate
 * If set to TRUE, the function will initialize the
 * hive as an alternate hive, otherwise FALSE to initialize
 * it as primary.
 *
 * @return
 * Returns TRUE if the hive has been initialized
 * and registry data inside the hive is valid, FALSE
 * otherwise.
 */
static
BOOLEAN
RegInitializeHive(
    _In_ PCMHIVE CmHive,
    _In_ PVOID ChunkBase,
    _In_ BOOLEAN LoadAlternate)
{
    NTSTATUS Status;
    CM_CHECK_REGISTRY_STATUS CmStatusCode;

    /* Initialize the hive */
    Status = HvInitialize(GET_HHIVE(CmHive),
                          HINIT_FLAT, // HINIT_MEMORY_INPLACE
                          0,
                          LoadAlternate ? HFILE_TYPE_ALTERNATE : HFILE_TYPE_PRIMARY,
                          ChunkBase,
                          CmpAllocate,
                          CmpFree,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to initialize the flat hive (Status 0x%lx)\n", Status);
        return FALSE;
    }

    /* Now check the hive and purge volatile data */
    CmStatusCode = CmCheckRegistry(CmHive, CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        ERR("CmCheckRegistry detected problems with the loaded flat hive (check code %lu)\n", CmStatusCode);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * Loads and reads a hive log at specified
 * file offset.
 *
 * @param[in] DirectoryPath
 * A pointer to a string that denotes the directory
 * path of the hives and logs location.
 *
 * @param[in] LogFileOffset
 * The file offset of which this function uses to
 * seek at specific position during read.
 *
 * @param[in] LogName
 * A pointer to a string that denotes the name of
 * the desired hive log (e.g. "SYSTEM").
 *
 * @param[out] LogData
 * A pointer to the returned hive log data that was
 * read. The following data varies depending on the
 * specified offset set up by the caller, that is used
 * to where to start reading from the hive log.
 *
 * @return
 * Returns TRUE if the hive log was loaded and read
 * successfully, FALSE otherwise.
 *
 * @remarks
 * The returned log data pointer to the caller is a
 * virtual address. You must use VaToPa that converts
 * the address to a physical one in order to actually
 * use it!
 */
static
BOOLEAN
RegLoadHiveLog(
    _In_ PCSTR DirectoryPath,
    _In_ ULONG LogFileOffset,
    _In_ PCSTR LogName,
    _Out_ PVOID *LogData)
{
    ARC_STATUS Status;
    ULONG LogId;
    CHAR LogPath[MAX_PATH];
    ULONG LogFileSize;
    FILEINFORMATION FileInfo;
    LARGE_INTEGER Position;
    ULONG BytesRead;
    PVOID LogDataVirtual;
    PVOID LogDataPhysical;

    /* Build the full path to the hive log */
    RtlStringCbCopyA(LogPath, sizeof(LogPath), DirectoryPath);
    RtlStringCbCatA(LogPath, sizeof(LogPath), LogName);

    /* Open the file */
    Status = ArcOpen(LogPath, OpenReadOnly, &LogId);
    if (Status != ESUCCESS)
    {
        ERR("Failed to open %s (ARC code %lu)\n", LogName, Status);
        return FALSE;
    }

    /* Get the file length */
    Status = ArcGetFileInformation(LogId, &FileInfo);
    if (Status != ESUCCESS)
    {
        ERR("Failed to get file information from %s (ARC code %lu)\n", LogName, Status);
        ArcClose(LogId);
        return FALSE;
    }

    /* Capture the size of the hive log file */
    LogFileSize = FileInfo.EndingAddress.LowPart;
    if (LogFileSize == 0)
    {
        ERR("LogFileSize is 0, %s is corrupt\n", LogName);
        ArcClose(LogId);
        return FALSE;
    }

    /* Allocate memory blocks for our log data */
    LogDataPhysical = MmAllocateMemoryWithType(
        MM_SIZE_TO_PAGES(LogFileSize + MM_PAGE_SIZE - 1) << MM_PAGE_SHIFT,
        LoaderRegistryData);
    if (LogDataPhysical == NULL)
    {
        ERR("Failed to allocate memory for log data\n");
        ArcClose(LogId);
        return FALSE;
    }

    /* Convert the address to virtual so that it can be useable */
    LogDataVirtual = PaToVa(LogDataPhysical);

    /* Seek within the log file at desired position */
    Position.QuadPart = LogFileOffset;
    Status = ArcSeek(LogId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        ERR("Failed to seek at %s (ARC code %lu)\n", LogName, Status);
        ArcClose(LogId);
        return FALSE;
    }

    /* And read the actual data from the log */
    Status = ArcRead(LogId, LogDataPhysical, LogFileSize, &BytesRead);
    if (Status != ESUCCESS)
    {
        ERR("Failed to read %s (ARC code %lu)\n", LogName, Status);
        ArcClose(LogId);
        return FALSE;
    }

    *LogData = LogDataVirtual;
    ArcClose(LogId);
    return TRUE;
}

/**
 * @brief
 * Recovers the header base block of a flat
 * registry hive.
 *
 * @param[in] ChunkBase
 * A pointer to the registry hive chunk base of
 * which the damaged header block is to be recovered.
 *
 * @param[in] DirectoryPath
 * A pointer to a string that denotes the directory
 * path of the hives and logs location.
 *
 * @param[in] LogName
 * A pointer to a string that denotes the name of
 * the desired hive log (e.g. "SYSTEM").
 *
 * @return
 * Returns TRUE if the header base block was successfully
 * recovered, FALSE otherwise.
 */
static
BOOLEAN
RegRecoverHeaderHive(
    _Inout_ PVOID ChunkBase,
    _In_ PCSTR DirectoryPath,
    _In_ PCSTR LogName)
{
    BOOLEAN Success;
    CHAR FullLogFileName[MAX_PATH];
    PVOID LogData;
    PHBASE_BLOCK HiveBaseBlock;
    PHBASE_BLOCK LogBaseBlock;

    /* Build the complete path of the hive log */
    RtlStringCbCopyA(FullLogFileName, sizeof(FullLogFileName), LogName);
    RtlStringCbCatA(FullLogFileName, sizeof(FullLogFileName), ".LOG");
    Success = RegLoadHiveLog(DirectoryPath, 0, FullLogFileName, &LogData);
    if (!Success)
    {
        ERR("Failed to read the hive log\n");
        return FALSE;
    }

    /* Make sure the header from the hive log is actually sane  */
    LogData = VaToPa(LogData);
    LogBaseBlock = GET_HBASE_BLOCK(LogData);
    if (!HvpVerifyHiveHeader(LogBaseBlock, HFILE_TYPE_LOG))
    {
        ERR("The hive log has corrupt base block\n");
        return FALSE;
    }

    /* Copy the healthy header base block into the primary hive */
    HiveBaseBlock = GET_HBASE_BLOCK(ChunkBase);
    WARN("Recovering the hive base block...\n");
    RtlCopyMemory(HiveBaseBlock,
                  LogBaseBlock,
                  LogBaseBlock->Cluster * HSECTOR_SIZE);
    HiveBaseBlock->Type = HFILE_TYPE_PRIMARY;
    return TRUE;
}

/**
 * @brief
 * Recovers the corrupt data of a primary flat
 * registry hive.
 *
 * @param[in] ChunkBase
 * A pointer to the registry hive chunk base of
 * which the damaged hive data is to be replaced
 * with healthy data from the corresponding hive log.
 *
 * @param[in] DirectoryPath
 * A pointer to a string that denotes the directory
 * path of the hives and logs location.
 *
 * @param[in] LogName
 * A pointer to a string that denotes the name of
 * the desired hive log (e.g. "SYSTEM").
 *
 * @return
 * Returns TRUE if the hive data was successfully
 * recovered, FALSE otherwise.
 *
 * @remarks
 * Data recovery of the target hive does not always
 * guarantee the primary hive is fully recovered.
 * It could happen a block from a hive log is not
 * marked dirty (pending to be written to disk) that
 * has healthy data therefore the following bad block
 * would still remain in corrupt state in the main primary
 * hive. In such scenarios an alternate hive must be replayed.
 */
static
BOOLEAN
RegRecoverDataHive(
    _Inout_ PVOID ChunkBase,
    _In_ PCSTR DirectoryPath,
    _In_ PCSTR LogName)
{
    BOOLEAN Success;
    ULONG StorageLength;
    ULONG BlockIndex, LogIndex;
    PUCHAR BlockPtr, BlockDest;
    CHAR FullLogFileName[MAX_PATH];
    PVOID LogData;
    PUCHAR LogDataPhysical;
    PHBASE_BLOCK HiveBaseBlock;

    /* Build the complete path of the hive log */
    RtlStringCbCopyA(FullLogFileName, sizeof(FullLogFileName), LogName);
    RtlStringCbCatA(FullLogFileName, sizeof(FullLogFileName), ".LOG");
    Success = RegLoadHiveLog(DirectoryPath, HV_LOG_HEADER_SIZE, FullLogFileName, &LogData);
    if (!Success)
    {
        ERR("Failed to read the hive log\n");
        return FALSE;
    }

    /* Make sure the dirty vector signature is there otherwise the hive log is corrupt */
    LogDataPhysical = (PUCHAR)VaToPa(LogData);
    if (*((PULONG)LogDataPhysical) != HV_LOG_DIRTY_SIGNATURE)
    {
        ERR("The hive log dirty signature could not be found\n");
        return FALSE;
    }

    /* Copy the dirty data into the primary hive */
    LogIndex = 0;
    BlockIndex = 0;
    HiveBaseBlock = GET_HBASE_BLOCK(ChunkBase);
    StorageLength = HiveBaseBlock->Length / HBLOCK_SIZE;
    for (; BlockIndex < StorageLength; ++BlockIndex)
    {
        /* Skip this block if it's not dirty and go to the next one */
        if (LogDataPhysical[BlockIndex + sizeof(HV_LOG_DIRTY_SIGNATURE)] != HV_LOG_DIRTY_BLOCK)
        {
            continue;
        }

        /* Read the dirty block and copy it at right offsets */
        BlockPtr = (PUCHAR)((ULONG_PTR)LogDataPhysical + 2 * HSECTOR_SIZE + LogIndex * HBLOCK_SIZE);
        BlockDest = (PUCHAR)((ULONG_PTR)ChunkBase + (BlockIndex + 1) * HBLOCK_SIZE);
        RtlCopyMemory(BlockDest, BlockPtr, HBLOCK_SIZE);

        /* Increment the index in log as we continue further */
        LogIndex++;
    }

    /* Fix the secondary sequence of the primary hive and compute a new checksum */
    HiveBaseBlock->Sequence2 = HiveBaseBlock->Sequence1;
    HiveBaseBlock->CheckSum = HvpHiveHeaderChecksum(HiveBaseBlock);
    return TRUE;
}

/**
 * @brief
 * Imports the SYSTEM binary hive from
 * the registry base chunk that's been
 * provided by the loader block.
 *
 * @param[in] ChunkBase
 * A pointer to the registry base chunk
 * that serves for SYSTEM hive initialization.
 *
 * @param[in] ChunkSize
 * The size of the registry base chunk. This
 * parameter refers to the actual size of
 * the SYSTEM hive. This parameter is currently
 * unused.
 *
 * @param[in] LoadAlternate
 * If set to TRUE, the function will initialize the
 * hive as an alternate hive, otherwise FALSE to initialize
 * it as primary.
 *
 * @return
 * Returns TRUE if hive importing and initialization
 * have succeeded, FALSE otherwise.
 */
BOOLEAN
RegImportBinaryHive(
    _In_ PVOID ChunkBase,
    _In_ ULONG ChunkSize,
    _In_ PCSTR SearchPath,
    _In_ BOOLEAN LoadAlternate)
{
    BOOLEAN Success;
    PCM_KEY_NODE KeyNode;

    TRACE("RegImportBinaryHive(%p, 0x%lx)\n", ChunkBase, ChunkSize);

    /* Assume that we don't need boot recover, unless we have to */
    ((PHBASE_BLOCK)ChunkBase)->BootRecover = HBOOT_NO_BOOT_RECOVER;

    /* Allocate and initialize the hive */
    CmSystemHive = FrLdrTempAlloc(sizeof(CMHIVE), 'eviH');
    Success = RegInitializeHive(CmSystemHive, ChunkBase, LoadAlternate);
    if (!Success)
    {
        /* Free the buffer and retry again */
        FrLdrTempFree(CmSystemHive, 'eviH');
        CmSystemHive = NULL;

        if (!RegRecoverHeaderHive(ChunkBase, SearchPath, "SYSTEM"))
        {
            ERR("Failed to recover the hive header block\n");
            return FALSE;
        }

        if (!RegRecoverDataHive(ChunkBase, SearchPath, "SYSTEM"))
        {
            ERR("Failed to recover the hive data\n");
            return FALSE;
        }

        /* Now retry initializing the hive again */
        CmSystemHive = FrLdrTempAlloc(sizeof(CMHIVE), 'eviH');
        Success = RegInitializeHive(CmSystemHive, ChunkBase, LoadAlternate);
        if (!Success)
        {
            ERR("Corrupted hive (despite recovery) %p\n", ChunkBase);
            FrLdrTempFree(CmSystemHive, 'eviH');
            return FALSE;
        }

        /*
         * Acknowledge the kernel we recovered the SYSTEM hive
         * on our side by applying log data.
         */
        ((PHBASE_BLOCK)ChunkBase)->BootRecover = HBOOT_BOOT_RECOVERED_BY_HIVE_LOG;
    }

    /* Save the root key node */
    SystemHive = GET_HHIVE(CmSystemHive);
    SystemRootCell = SystemHive->BaseBlock->RootCell;
    ASSERT(SystemRootCell != HCELL_NIL);

    /* Verify it is accessible */
    KeyNode = (PCM_KEY_NODE)HvGetCell(SystemHive, SystemRootCell);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    HvReleaseCell(SystemHive, SystemRootCell);

    return TRUE;
}

BOOLEAN
RegInitCurrentControlSet(
    _In_ BOOLEAN LastKnownGood)
{
    UNICODE_STRING ControlSetName;
    HCELL_INDEX ControlCell;
    PCM_KEY_NODE KeyNode;
    BOOLEAN AutoSelect;

    TRACE("RegInitCurrentControlSet\n");

    /* Choose which control set to open and set it as the new "Current" */
    RtlInitUnicodeString(&ControlSetName,
                         LastKnownGood ? L"LastKnownGood"
                                       : L"Default");

    ControlCell = CmpFindControlSet(SystemHive,
                                    SystemRootCell,
                                    &ControlSetName,
                                    &AutoSelect);
    if (ControlCell == HCELL_NIL)
    {
        ERR("CmpFindControlSet('%wZ') failed\n", &ControlSetName);
        return FALSE;
    }

    CurrentControlSetKey = HCI_TO_HKEY(ControlCell);

    /* Verify it is accessible */
    KeyNode = (PCM_KEY_NODE)HvGetCell(SystemHive, ControlCell);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    HvReleaseCell(SystemHive, ControlCell);

    return TRUE;
}

static
BOOLEAN
GetNextPathElement(
    _Out_ PUNICODE_STRING NextElement,
    _Inout_ PUNICODE_STRING RemainingPath)
{
    /* Check if there are any characters left */
    if (RemainingPath->Length < sizeof(WCHAR))
    {
        /* Nothing left, bail out early */
        return FALSE;
    }

    /* The next path elements starts with the remaining path */
    NextElement->Buffer = RemainingPath->Buffer;

    /* Loop until the path element ends */
    while ((RemainingPath->Length >= sizeof(WCHAR)) &&
           (RemainingPath->Buffer[0] != '\\'))
    {
        /* Skip this character */
        RemainingPath->Buffer++;
        RemainingPath->Length -= sizeof(WCHAR);
    }

    NextElement->Length = (USHORT)(RemainingPath->Buffer - NextElement->Buffer) * sizeof(WCHAR);
    NextElement->MaximumLength = NextElement->Length;

    /* Check if the path element ended with a path separator */
    if (RemainingPath->Length >= sizeof(WCHAR))
    {
        /* Skip the path separator */
        ASSERT(RemainingPath->Buffer[0] == '\\');
        RemainingPath->Buffer++;
        RemainingPath->Length -= sizeof(WCHAR);
    }

    /* Return whether we got any characters */
    return TRUE;
}

#if 0
LONG
RegEnumKey(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR Name,
    _Inout_ PULONG NameSize,
    _Out_opt_ PHKEY SubKey)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode, SubKeyNode;
    HCELL_INDEX CellIndex;
    USHORT NameLength;

    TRACE("RegEnumKey(%p, %lu, %p, %p->%u)\n",
          Key, Index, Name, NameSize, NameSize ? *NameSize : 0);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, Index);
    if (CellIndex == HCELL_NIL)
    {
        TRACE("RegEnumKey index out of bounds (%d) in key (%.*s)\n",
              Index, KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, HKEY_TO_HCI(Key));
        return ERROR_NO_MORE_ITEMS;
    }
    HvReleaseCell(Hive, HKEY_TO_HCI(Key));

    /* Get the value cell */
    SubKeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
    ASSERT(SubKeyNode != NULL);
    ASSERT(SubKeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    if (SubKeyNode->Flags & KEY_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(SubKeyNode->Name, SubKeyNode->NameLength);

        /* Compressed name */
        CmpCopyCompressedName(Name,
                              *NameSize,
                              SubKeyNode->Name,
                              SubKeyNode->NameLength);
    }
    else
    {
        NameLength = SubKeyNode->NameLength;

        /* Normal name */
        RtlCopyMemory(Name, SubKeyNode->Name,
                      min(*NameSize, SubKeyNode->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        Name[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    HvReleaseCell(Hive, CellIndex);

    if (SubKey != NULL)
        *SubKey = HCI_TO_HKEY(CellIndex);

    TRACE("RegEnumKey done -> %u, '%.*S'\n", *NameSize, *NameSize, Name);
    return ERROR_SUCCESS;
}
#endif

LONG
RegOpenKey(
    _In_ HKEY ParentKey,
    _In_z_ PCWSTR KeyName,
    _Out_ PHKEY Key)
{
    UNICODE_STRING RemainingPath, SubKeyName;
    UNICODE_STRING CurrentControlSet = RTL_CONSTANT_STRING(L"CurrentControlSet");
    PHHIVE Hive = (ParentKey ? GET_HHIVE_FROM_HKEY(ParentKey) : GET_HHIVE(CmSystemHive));
    PCM_KEY_NODE KeyNode;
    HCELL_INDEX CellIndex;

    TRACE("RegOpenKey(%p, '%S', %p)\n", ParentKey, KeyName, Key);

    /* Initialize the remaining path name */
    RtlInitUnicodeString(&RemainingPath, KeyName);

    /* Check if we have a parent key */
    if (ParentKey == NULL)
    {
        UNICODE_STRING SubKeyName1, SubKeyName2, SubKeyName3;
        UNICODE_STRING RegistryPath = RTL_CONSTANT_STRING(L"Registry");
        UNICODE_STRING MachinePath = RTL_CONSTANT_STRING(L"MACHINE");
        UNICODE_STRING SystemPath = RTL_CONSTANT_STRING(L"SYSTEM");

        TRACE("RegOpenKey: absolute path\n");

        if ((RemainingPath.Length < sizeof(WCHAR)) ||
            RemainingPath.Buffer[0] != '\\')
        {
            /* The key path is not absolute */
            ERR("RegOpenKey: invalid path '%S' (%wZ)\n", KeyName, &RemainingPath);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Skip initial path separator */
        RemainingPath.Buffer++;
        RemainingPath.Length -= sizeof(WCHAR);

        /* Get the first 3 path elements */
        GetNextPathElement(&SubKeyName1, &RemainingPath);
        GetNextPathElement(&SubKeyName2, &RemainingPath);
        GetNextPathElement(&SubKeyName3, &RemainingPath);
        TRACE("RegOpenKey: %wZ / %wZ / %wZ\n", &SubKeyName1, &SubKeyName2, &SubKeyName3);

        /* Check if we have the correct path */
        if (!RtlEqualUnicodeString(&SubKeyName1, &RegistryPath, TRUE) ||
            !RtlEqualUnicodeString(&SubKeyName2, &MachinePath, TRUE) ||
            !RtlEqualUnicodeString(&SubKeyName3, &SystemPath, TRUE))
        {
            /* The key path is not inside HKLM\Machine\System */
            ERR("RegOpenKey: invalid path '%S' (%wZ)\n", KeyName, &RemainingPath);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Use the root key */
        CellIndex = SystemRootCell;
    }
    else
    {
        /* Use the parent key */
        CellIndex = HKEY_TO_HCI(ParentKey);
    }

    /* Check if this is the root key */
    if (CellIndex == SystemRootCell)
    {
        UNICODE_STRING TempPath = RemainingPath;

        /* Get the first path element */
        GetNextPathElement(&SubKeyName, &TempPath);

        /* Check if this is CurrentControlSet */
        if (RtlEqualUnicodeString(&SubKeyName, &CurrentControlSet, TRUE))
        {
            /* Use the CurrentControlSetKey and update the remaining path */
            CellIndex = HKEY_TO_HCI(CurrentControlSetKey);
            RemainingPath = TempPath;
        }
    }

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    TRACE("RegOpenKey: RemainingPath '%wZ'\n", &RemainingPath);

    /* Loop while there are path elements */
    while (GetNextPathElement(&SubKeyName, &RemainingPath))
    {
        HCELL_INDEX NextCellIndex;

        TRACE("RegOpenKey: next element '%wZ'\n", &SubKeyName);

        /* Get the next sub key */
        NextCellIndex = CmpFindSubKeyByName(Hive, KeyNode, &SubKeyName);
        HvReleaseCell(Hive, CellIndex);
        CellIndex = NextCellIndex;
        if (CellIndex == HCELL_NIL)
        {
            WARN("Did not find sub key '%wZ' (full: %S)\n", &SubKeyName, KeyName);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Get the found key */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
        ASSERT(KeyNode);
        ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    }

    HvReleaseCell(Hive, CellIndex);
    *Key = HCI_TO_HKEY(CellIndex);

    return ERROR_SUCCESS;
}

static
VOID
RepGetValueData(
    _In_ PHHIVE Hive,
    _In_ PCM_KEY_VALUE ValueCell,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    ULONG DataLength;
    PVOID DataCell;

    /* Does the caller want the type? */
    if (Type != NULL)
        *Type = ValueCell->Type;

    /* Does the caller provide DataSize? */
    if (DataSize != NULL)
    {
        // NOTE: CmpValueToData doesn't support big data (the function will
        // bugcheck if so), FreeLdr is not supposed to read such data.
        // If big data is needed, use instead CmpGetValueData.
        // CmpGetValueData(Hive, ValueCell, DataSize, &DataCell, ...);
        DataCell = CmpValueToData(Hive, ValueCell, &DataLength);

        /* Does the caller want the data? */
        if ((Data != NULL) && (*DataSize != 0))
        {
            RtlCopyMemory(Data,
                          DataCell,
                          min(*DataSize, DataLength));
        }

        /* Return the actual data length */
        *DataSize = DataLength;
    }
}

LONG
RegQueryValue(
    _In_ HKEY Key,
    _In_z_ PCWSTR ValueName,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    TRACE("RegQueryValue(%p, '%S', %p, %p, %p)\n",
          Key, ValueName, Type, Data, DataSize);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, ValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
    {
        TRACE("RegQueryValue value not found in key (%.*s)\n",
              KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, HKEY_TO_HCI(Key));
        return ERROR_FILE_NOT_FOUND;
    }
    HvReleaseCell(Hive, HKEY_TO_HCI(Key));

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
    ASSERT(ValueCell != NULL);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, CellIndex);

    return ERROR_SUCCESS;
}

/*
 * NOTE: This function is currently unused in FreeLdr; however it is kept here
 * as an implementation reference of RegEnumValue using CMLIB that may be used
 * elsewhere in ReactOS.
 */
#if 0
LONG
RegEnumValue(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR ValueName,
    _Inout_ PULONG NameSize,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode;
    PCELL_DATA ValueListCell;
    PCM_KEY_VALUE ValueCell;
    USHORT NameLength;

    TRACE("RegEnumValue(%p, %lu, %S, %p, %p, %p, %p (%lu))\n",
          Key, Index, ValueName, NameSize, Type, Data, DataSize, *DataSize);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if the index is valid */
    if ((KeyNode->ValueList.Count == 0) ||
        (KeyNode->ValueList.List == HCELL_NIL) ||
        (Index >= KeyNode->ValueList.Count))
    {
        ERR("RegEnumValue: index invalid\n");
        HvReleaseCell(Hive, HKEY_TO_HCI(Key));
        return ERROR_NO_MORE_ITEMS;
    }

    ValueListCell = (PCELL_DATA)HvGetCell(Hive, KeyNode->ValueList.List);
    ASSERT(ValueListCell != NULL);

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, ValueListCell->KeyList[Index]);
    ASSERT(ValueCell != NULL);
    ASSERT(ValueCell->Signature == CM_KEY_VALUE_SIGNATURE);

    if (ValueCell->Flags & VALUE_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(ValueCell->Name, ValueCell->NameLength);

        /* Compressed name */
        CmpCopyCompressedName(ValueName,
                              *NameSize,
                              ValueCell->Name,
                              ValueCell->NameLength);
    }
    else
    {
        NameLength = ValueCell->NameLength;

        /* Normal name */
        RtlCopyMemory(ValueName, ValueCell->Name,
                      min(*NameSize, ValueCell->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        ValueName[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, ValueListCell->KeyList[Index]);
    HvReleaseCell(Hive, KeyNode->ValueList.List);
    HvReleaseCell(Hive, HKEY_TO_HCI(Key));

    TRACE("RegEnumValue done -> %u, '%.*S'\n", *NameSize, *NameSize, ValueName);
    return ERROR_SUCCESS;
}
#endif

/* EOF */
