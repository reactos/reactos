/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     EEPROM support code
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* GLOBASLS *******************************************************************/

#define EEPROM_READ(Adapter, Data)  \
    do { \
        *Data = CSR_READ_16((Adapter), FXP_CSR_EEPROMCONTROL); \
        NdisStallExecution(1); \
    } while (0)

#define EEPROM_WRITE(Adapter, Value)  \
    do { \
        CSR_WRITE_16((Adapter), FXP_CSR_EEPROMCONTROL, Value); \
        NdisStallExecution(1); \
    } while (0)

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
EeEepromShiftOut(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG Sequence,
    _In_ ULONG BitCount)
{
    LONG i, Bit = 0;

    PAGED_CODE();

    /* Shift the data out to the EEPROM */
    for (i = BitCount - 1; i >= 0; i--)
    {
        USHORT DataIn = ((Sequence >> i) & 1) << FXP_EEPROM_EEDI_SHIFT;

        EEPROM_WRITE(Adapter, DataIn | FXP_EEPROM_EECS);
        EEPROM_WRITE(Adapter, DataIn | FXP_EEPROM_EECS | FXP_EEPROM_EESK);
        EEPROM_WRITE(Adapter, DataIn | FXP_EEPROM_EECS);

        if (Adapter->EepromAddressBusWidth == (ULONG)-1)
        {
            Bit++;

            /* Check the preceding dummy zero bit */
            if (!(CSR_READ_16(Adapter, FXP_CSR_EEPROMCONTROL) & FXP_EEPROM_EEDO))
            {
                Adapter->EepromAddressBusWidth = Bit;
                break;
            }
        }
    }
}

static
CODE_SEG("PAGE")
USHORT
EeEepromShiftIn(
    _In_ PE100_ADAPTER Adapter)
{
    USHORT SerialData, DataOut;
    ULONG i;

    /* Shift the data in from the EEPROM */
    SerialData = 0;
    for (i = 0; i < RTL_BITS_OF(USHORT); ++i)
    {
        EEPROM_WRITE(Adapter, FXP_EEPROM_EECS | FXP_EEPROM_EESK);

        EEPROM_READ(Adapter, &DataOut);
        SerialData = (SerialData << 1) | ((DataOut >> FXP_EEPROM_EEDO_SHIFT) & 1);

        EEPROM_WRITE(Adapter, FXP_EEPROM_EECS);
    }

    /* End the read cycle */
    EEPROM_WRITE(Adapter, 0);

    return SerialData;
}

static
CODE_SEG("PAGE")
BOOLEAN
EeEepromDetectAddressBusWidth(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    ASSERT(Adapter->EepromAddressBusWidth == 0);

    EEPROM_WRITE(Adapter, FXP_EEPROM_EECS);

    EeEepromShiftOut(Adapter, FXP_EEPROM_OPC_READ, FXP_EEPROM_OPC_LENGTH);

    /* Detect EEPROM size (64 or 256 words) */
    Adapter->EepromAddressBusWidth = (ULONG)-1;
    EeEepromShiftOut(Adapter, 0, 8);

    /* Complete the read cycle */
    (VOID)EeEepromShiftIn(Adapter);

    return (Adapter->EepromAddressBusWidth != (ULONG)-1);
}

static
CODE_SEG("PAGE")
BOOLEAN
EeEepromChecksumValid(
    _In_reads_(Size) const USHORT* Data,
    _In_ ULONG Size)
{
    ULONG i;
    USHORT Crc = 0;

    PAGED_CODE();

    for (i = 0; i < Size; ++i)
    {
        Crc += Data[i];
    }
    Crc = 0xBABA - Crc;

    return (Crc == Data[Size]);
}

static
BOOLEAN
CODE_SEG("PAGE")
EeEepromRead(
    _In_ PE100_ADAPTER Adapter,
    _Out_ PUSHORT EepromImage)
{
    ULONG Address, EepromWords;

    if (!EeEepromDetectAddressBusWidth(Adapter))
    {
        ERR("Bad EEPROM\n");
        return FALSE;
    }

    EepromWords = 1 << Adapter->EepromAddressBusWidth;
    INFO("EEPROM Size %lu\n", EepromWords);
    ASSERT(EepromWords <= FXP_EEPROM_MAX_WORDS);

    for (Address = 0; Address < EepromWords; ++Address)
    {
        EEPROM_WRITE(Adapter, FXP_EEPROM_EECS);

        EeEepromShiftOut(Adapter, FXP_EEPROM_OPC_READ, FXP_EEPROM_OPC_LENGTH);
        EeEepromShiftOut(Adapter, Address, Adapter->EepromAddressBusWidth);

        EepromImage[Address] = EeEepromShiftIn(Adapter);

        TRACE("Data %03X: %4X\n", Address, EepromImage[Address]);
    }

    if (!EeEepromChecksumValid(EepromImage, EepromWords - 1))
    {
        ERR("EEPROM checksum failed\n");
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
CODE_SEG("PAGE")
EeEepromParse(
    _In_ PE100_ADAPTER Adapter,
    _In_ USHORT* __restrict EepromImage)
{
    PAGED_CODE();

    /* Read MAC address */
    Adapter->PermanentMacAddress[0] = EepromImage[FXP_EEPROM_MAP_IA0] & 0xFF;
    Adapter->PermanentMacAddress[1] = EepromImage[FXP_EEPROM_MAP_IA0] >> 8;
    Adapter->PermanentMacAddress[2] = EepromImage[FXP_EEPROM_MAP_IA1] & 0xFF;
    Adapter->PermanentMacAddress[3] = EepromImage[FXP_EEPROM_MAP_IA1] >> 8;
    Adapter->PermanentMacAddress[4] = EepromImage[FXP_EEPROM_MAP_IA2] & 0xFF;
    Adapter->PermanentMacAddress[5] = EepromImage[FXP_EEPROM_MAP_IA2] >> 8;

    INFO("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
         Adapter->PermanentMacAddress[0],
         Adapter->PermanentMacAddress[1],
         Adapter->PermanentMacAddress[2],
         Adapter->PermanentMacAddress[3],
         Adapter->PermanentMacAddress[4],
         Adapter->PermanentMacAddress[5]);

    if (ETH_IS_BROADCAST(Adapter->PermanentMacAddress) ||
        ETH_IS_EMPTY(Adapter->PermanentMacAddress) ||
        ETH_IS_MULTICAST(Adapter->PermanentMacAddress))
    {
        ERR("Invalid permanent MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
            Adapter->PermanentMacAddress[0],
            Adapter->PermanentMacAddress[1],
            Adapter->PermanentMacAddress[2],
            Adapter->PermanentMacAddress[3],
            Adapter->PermanentMacAddress[4],
            Adapter->PermanentMacAddress[5]);

        NdisWriteErrorLogEntry(Adapter->AdapterHandle, NDIS_ERROR_CODE_NETWORK_ADDRESS, 0);

        return FALSE;
    }

    /* Assign revision ID */
    if (Adapter->Flags & E100_FLAG_IS_ICH)
    {
        /* Treat ICH based controllers as 82559 devices */
        Adapter->RevisionID = FXP_REV_82559_A0;

        /* Do not load microcode for ICH devices */
        Adapter->Flags |= E100_FLAG_NO_UCODE;
    }
    else
    {
        /* Lump all 82557 revisions together */
        if ((EepromImage[FXP_EEPROM_MAP_CNTR] >> 8) == 1)
            Adapter->RevisionID = FXP_REV_82557;
    }

    /* Check availability of WOL. 82559ER does not support WOL */
    if ((Adapter->RevisionID >= FXP_REV_82558_A4) &&
        (Adapter->RevisionID != FXP_REV_82559S_A))
    {
        if (EepromImage[FXP_EEPROM_MAP_ID] & 0x20)
            Adapter->Flags |= E100_FLAG_HAS_WOL;
    }

    if (Adapter->RevisionID == FXP_REV_82550_C)
    {
       /*
        * 82550C with server extension requires microcode to
        * receive fragmented UDP datagrams. However if the
        * microcode is used for client-only featured 82550C
        * it locks up controller.
        */
        if (!(EepromImage[FXP_EEPROM_MAP_COMPAT] & 0x0400))
            Adapter->Flags |= E100_FLAG_NO_UCODE;
    }

    if (Adapter->RevisionID < FXP_REV_82558_A4)
    {
        if ((EepromImage[FXP_EEPROM_MAP_COMPAT] & 0x03) != 0x03)
            Adapter->Flags |= E100_FLAG_RX_LOCKUP_BUG;
    }

    /* Detect serial interface PHY (Intel 82503 or Seeq 82C24) */
    if (Adapter->RevisionID == FXP_REV_82557)
    {
        if ((EepromImage[FXP_EEPROM_MAP_PRI_PHY] & FXP_PHY_DEVICE_MASK) &&
            (EepromImage[FXP_EEPROM_MAP_PRI_PHY] & FXP_PHY_SERIAL_ONLY))
        {
            /* Connected to a non-MII PHY device */
            Adapter->PhyType = E100_PHY_TYPE_503;
        }
    }

    return TRUE;
}

CODE_SEG("PAGE")
BOOLEAN
EeReadEeprom(
    _In_ PE100_ADAPTER Adapter)
{
    PUSHORT EepromImage;
    NDIS_STATUS Status;
    BOOLEAN Success = FALSE;

    PAGED_CODE();

    Status = NdisAllocateMemoryWithTag((PVOID*)&EepromImage, FXP_EEPROM_MAX_WORDS * 2, E100_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
        return FALSE;

    if (!EeEepromRead(Adapter, EepromImage))
        goto Cleanup;

    if (!EeEepromParse(Adapter, EepromImage))
        goto Cleanup;

    Success = TRUE;

Cleanup:
    NdisFreeMemory(EepromImage, FXP_EEPROM_MAX_WORDS * 2, 0);
    return Success;
}
