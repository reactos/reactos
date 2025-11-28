/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA IDENTIFY DEVICE and IDENTIFY PACKET DEVICE data helper functions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

FORCEINLINE
BOOLEAN
AtaDevHasCdbInterrupt(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 5:6 of word 0 */
    return (IdentifyPacketData->GeneralConfiguration.DrqDelay == 1);
}

FORCEINLINE
UCHAR
AtaDevCdbSizeInWords(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 0:2 of word 0 */
    return (IdentifyPacketData->GeneralConfiguration.PacketType != 0) ? 8 : 6;
}

FORCEINLINE
BOOLEAN
AtaDevMaxLun(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 0:2 of word 126 */
    USHORT LastLunIdentifier = IdentifyPacketData->ReservedWord126 & 7;

    /*
     * We perform additional validation because
     * most ATAPI devices ignore the LUN field in the CDB and respond to each LUN.
     */

    /* Make sure this field has no value that represents all bits set */
    if (LastLunIdentifier != 7)
        return LastLunIdentifier + 1;

    return 1;
}

FORCEINLINE
BOOLEAN
AtaDevIsDmaDirectionRequired(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bit 15 of word 62 */
    if (IdentifyPacketData->DMADIR.DMADIRBitRequired)
    {
        return !(IdentifyPacketData->MultiWordDMASupport & 0x7) &&          // Bits 0:2 of word 63
               !IdentifyPacketData->Capabilities.DmaSupported &&            // Bit 8 of word 49
               !IdentifyPacketData->Capabilities.InterleavedDmaSupported && // Bit 15 of word 49
               !(IdentifyPacketData->UltraDMASupport & 0x7F);               // Bits 0:6 of word 88
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevIsTape(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 8:12 of word 0 (sequential-access device) */
    return (IdentifyPacketData->GeneralConfiguration.CommandPacketType == 1);
}

FORCEINLINE
BOOLEAN
AtaDevIsIdentifyDataValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    ULONG i;
    UCHAR Crc;

    /* Bits 0:8 of word 255 */
    if (IdentifyData->Signature != 0xA5)
    {
        /* The integrity word is missing, assume the data provided by the device is valid */
        return TRUE;
    }

    /* Verify the checksum */
    Crc = 0;
    for (i = 0; i < sizeof(*IdentifyData); ++i)
    {
        Crc += ((PUCHAR)IdentifyData)[i];
    }

    return (Crc == 0);
}

FORCEINLINE
BOOLEAN
AtaDevHasLbaTranslation(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 9 of word 49 */
    return IdentifyData->Capabilities.LbaSupported;
}

FORCEINLINE
ULONG
AtaDevUserAddressableSectors28Bit(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Words 60-61 */
    return IdentifyData->UserAddressableSectors;
}

FORCEINLINE
ULONG64
AtaDevUserAddressableSectors48Bit(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Words 100-103 */
    return ((ULONG64)IdentifyData->Max48BitLBA[1] << 32) | IdentifyData->Max48BitLBA[0];
}

FORCEINLINE
BOOLEAN
AtaDevHas48BitAddressFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
   if (IdentifyData->CommandSetSupport.WordValid83 == 1)
   {
        /* Bit 10 of word 83 */
        return IdentifyData->CommandSetSupport.BigLba;
   }

   return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevIsCurrentGeometryValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    return ((IdentifyData->TranslationFieldsValid & 1) &&
            (IdentifyData->NumberOfCurrentCylinders != 0) &&
            (IdentifyData->NumberOfCurrentCylinders <= 63) &&
            (IdentifyData->NumberOfCurrentHeads != 0) &&
            (IdentifyData->NumberOfCurrentHeads <= 16) &&
            (IdentifyData->CurrentSectorsPerTrack != 0));
}

FORCEINLINE
VOID
AtaDevDefaultChsTranslation(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PUSHORT Cylinders,
    _Out_ PUSHORT Heads,
    _Out_ PUSHORT SectorsPerTrack)
{
    /* Word 1 */
    *Cylinders = IdentifyData->NumCylinders;
    /* Word 3 */
    *Heads = IdentifyData->NumHeads;
    /* Word 6 */
    *SectorsPerTrack = IdentifyData->NumSectorsPerTrack;
}

FORCEINLINE
VOID
AtaDevCurrentChsTranslation(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PUSHORT Cylinders,
    _Out_ PUSHORT Heads,
    _Out_ PUSHORT SectorsPerTrack)
{
    /* Word 54 */
    *Cylinders = IdentifyData->NumberOfCurrentCylinders;
    /* Word 55 */
    *Heads = IdentifyData->NumberOfCurrentHeads;
    /* Word 55 */
    *SectorsPerTrack = IdentifyData->CurrentSectorsPerTrack;
}

FORCEINLINE
UCHAR
AtaDevCurrentSectorsPerDrq(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    UCHAR MultiSectorCurrent;

    /* Bit 8 of word 59 */
    if (!(IdentifyData->MultiSectorSettingValid))
        return 0;

    /* The word 59 should be a power of 2 */
    MultiSectorCurrent = IdentifyData->CurrentMultiSectorSetting;
    if ((MultiSectorCurrent > 0) && ((MultiSectorCurrent & (MultiSectorCurrent - 1)) == 0))
        return MultiSectorCurrent;

    return 0;
}

FORCEINLINE
UCHAR
AtaDevMaximumSectorsPerDrq(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    UCHAR MultiSectorMax;

    /* The word 47 should be a power of 2 */
    MultiSectorMax = IdentifyData->MaximumBlockTransfer;
    if ((MultiSectorMax > 0) && ((MultiSectorMax & (MultiSectorMax - 1)) == 0))
        return MultiSectorMax;

    return 0;
}

FORCEINLINE
ULONG
AtaDevBytesPerLogicalSector(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    ULONG WordCount;

    /* Word 106: 15 = 0, 14 = 1, 12 = 1 */
    if (IdentifyData->PhysicalLogicalSectorSize.Reserved1 == 1 &&
        IdentifyData->PhysicalLogicalSectorSize.LogicalSectorLongerThan256Words)
    {
        /* Words 116-117 */
        WordCount = IdentifyData->WordsPerLogicalSector[0];
        WordCount |= (ULONG)IdentifyData->WordsPerLogicalSector[1] << 16;
    }
    else
    {
        /* 256 words = 512 bytes */
        WordCount = 256;
    }

    return WordCount * sizeof(USHORT);
}

FORCEINLINE
ULONG
AtaDevLogicalSectorsPerPhysicalSector(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PULONG Exponent)
{
    /* Word 106: 15 = 0, 14 = 1, 13 = 1 */
    if (IdentifyData->PhysicalLogicalSectorSize.Reserved1 == 1 &&
        IdentifyData->PhysicalLogicalSectorSize.MultipleLogicalSectorsPerPhysicalSector)
    {
        /* Bits 0:3 of word 106 */
        *Exponent = IdentifyData->PhysicalLogicalSectorSize.LogicalSectorsPerPhysicalSector;

        return 1 << *Exponent;
    }

    *Exponent = 0;
    return 1 << 0;
}

FORCEINLINE
ULONG
AtaDevLogicalSectorAlignment(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 209: 15 = 0, 14 = 1 */
    if (IdentifyData->BlockAlignment.Word209Supported &&
        IdentifyData->BlockAlignment.Reserved0 == 0)
    {
        /* Bits 0:13 of word 209 */
        return IdentifyData->BlockAlignment.AlignmentOfLogicalWithinPhysical;
    }

    return 0;
}

FORCEINLINE
BOOLEAN
AtaDevInPuisState(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 2 */
    return (IdentifyData->SpecificConfiguration == 0x37C8) ||
           (IdentifyData->SpecificConfiguration == 0x738C);
}

FORCEINLINE
BOOLEAN
AtaDevIsIdentifyDataIncomplete(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 2 of word 0 */
    return IdentifyData->GeneralConfiguration.ResponseIncomplete;
}

FORCEINLINE
BOOLEAN
AtaDevIsRemovable(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 7 of word 0 */
    return IdentifyData->GeneralConfiguration.RemovableMedia;
}

FORCEINLINE
BOOLEAN
AtaDevHasRemovableMediaFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    if (AtaDevIsRemovable(IdentifyData))
    {
        /* Word 83: 15 = 0, 14 = 1 */
        if (IdentifyData->CommandSetSupport.WordValid == 1)
        {
            /* Bit 2 of word 82 */
            return IdentifyData->CommandSetSupport.RemovableMediaFeature;
        }
    }

    return FALSE;
}

FORCEINLINE
ULONG
AtaDevQueueDepth(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 8 of word 76 */
    if (IdentifyData->SerialAtaCapabilities.NCQ)
    {
        /* Bits 0:4 of word 75 */
        return IdentifyData->QueueDepth + 1;
    }

    return 0;
}

FORCEINLINE
BOOLEAN
AtaDevHasNcqAutosense(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    USHORT Word76 = ((PUSHORT)IdentifyData)[76]; // IdentifyData->SerialAtaCapabilities

    if (Word76 != 0x0000 && Word76 != 0xFFFF)
    {
        /* Bit 7 of word 78 */
        return IdentifyData->SerialAtaFeaturesSupported.NCQAutosense;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevHasSenseDataReporting(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    if (IdentifyData->CommandSetActive.Words119_120Valid && // Word 86: bit 15 = 1
        IdentifyData->CommandSetSupportExt.WordValid == 1)  // Word 119: 15 = 0, 14 = 1
    {
        /* Bit 6 of word 119 */
        return IdentifyData->CommandSetSupportExt.SenseDataReporting;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevHasRemovableMediaStatusNotification(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 127: bit 0 = 1, bit 1 = 0 */
    return (IdentifyData->MsnSupport == 1);
}

FORCEINLINE
BOOLEAN
AtaDevHasIeee1667(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    // TODO: Verify word 48

    /* Bit 0 of word 48 */
    if (IdentifyData->TrustedComputing.FeatureSupported)
    {
        /* Bit 7 of word 69 */
        return IdentifyData->AdditionalSupported.IEEE1667;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevHasWorldWideName(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 87: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetActive.Reserved4 == 1)
    {
        /* Bit 8 of word 87 */
        return IdentifyData->CommandSetActive.WWN64Bit;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevIsZonedDevice(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bits 0:1 of word 69 */
    return (IdentifyData->AdditionalSupported.ZonedCapabilities != 0);
}

FORCEINLINE
UCHAR
AtaDevZonedCapabilities(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bits 0:1 of word 69 */
    return IdentifyData->AdditionalSupported.ZonedCapabilities;
}

FORCEINLINE
BOOLEAN
AtaDevHasSecurityModeFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 1 of word 82 */
        return IdentifyData->CommandSetSupport.SecurityMode;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevHasSmartFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 0 of word 82 */
    return IdentifyData->CommandSetSupport.SmartCommands;
}

FORCEINLINE
BOOLEAN
AtaDevIsVolatileWriteCacheEnabled(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 5 of word 82 and bit 5 of word 85 */
        return (IdentifyData->CommandSetSupport.WriteCache &&
                IdentifyData->CommandSetActive.WriteCache);
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevIsReadLookAHeadEnabled(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 6 of word 82 and bit 6 of word 85 */
        return (IdentifyData->CommandSetSupport.LookAhead &&
                IdentifyData->CommandSetActive.LookAhead);
    }

    return FALSE;
}

FORCEINLINE
UCHAR
AtaDevNominalFormFactor(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bits 0:3 of word 168 */
    return IdentifyData->NominalFormFactor;
}

FORCEINLINE
USHORT
AtaDevMediumRotationRate(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 217 */
    return IdentifyData->NominalMediaRotationRate;
}

FORCEINLINE
BOOLEAN
AtaDevIsRotatingDevice(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 217 */
    return (IdentifyData->NominalMediaRotationRate >= 0x0401 &&
            IdentifyData->NominalMediaRotationRate <= 0xFFFE);
}

FORCEINLINE
BOOLEAN
AtaDevIsSsd(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 217 */
    return (IdentifyData->NominalMediaRotationRate == 1);
}

FORCEINLINE
BOOLEAN
AtaDevHasTrimFunction(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 0 of word 169 */
    return IdentifyData->DataSetManagementFeature.SupportsTrim;
}

FORCEINLINE
BOOLEAN
AtaDevHasRzatFunction(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 5 of word 69 */
    return IdentifyData->AdditionalSupported.ReadZeroAfterTrimSupported;
}

FORCEINLINE
BOOLEAN
AtaDevHasDratFunction(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 14 of word 69 */
    return IdentifyData->AdditionalSupported.DeterministicReadAfterTrimSupported;
}

FORCEINLINE
BOOLEAN
AtaDevHasForceUnitAccessCommands(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid83 == 1)
    {
        /* Bit 6 of word 84 */
        return IdentifyData->CommandSetSupport.WriteFua;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevHasFlushCache(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 12 of word 83 */
        return IdentifyData->CommandSetSupport.FlushCache;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevHasFlushCacheExt(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
    if (IdentifyData->CommandSetSupport.WordValid == 1)
    {
        /* Bit 13 of word 83 */
        return IdentifyData->CommandSetSupport.FlushCacheExt;
    }

    return FALSE;
}
