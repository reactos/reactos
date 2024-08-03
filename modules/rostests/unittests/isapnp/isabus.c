/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ISA PnP bus register access helpers
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

/* GLOBALS ********************************************************************/

PISAPNP_CARD IsapCard;

static PISAPNP_CARD IsapConfigureCard = NULL;
static ULONG IsapCardCount = 0;
static UCHAR IsapAddressLatch = 0;

/* PRIVATE FUNCTIONS **********************************************************/

static
inline
UCHAR
IsaBusNextLFSR(
    _In_ UCHAR Lfsr,
    _In_ UCHAR InputBit)
{
    UCHAR NextLfsr = Lfsr >> 1;

    NextLfsr |= (((Lfsr ^ NextLfsr) ^ InputBit)) << 7;

    return NextLfsr;
}

static
VOID
IsaBusWriteAddressRegister(
    _In_ UCHAR Value)
{
    ULONG i;

    IsapAddressLatch = Value;

    for (i = 0; i < IsapCardCount; ++i)
    {
        PISAPNP_CARD Card = &IsapCard[i];

        if (Card->State != IsaWaitForKey)
            continue;

        /* Reset the LFSR contents */
        if (Card->Lfsr != Value)
        {
            Card->Lfsr = ISAPNP_LFSR_SEED;
            Card->LfsrCount = 0;
            continue;
        }

        /* Generate the next data pattern */
        Card->Lfsr = IsaBusNextLFSR(Card->Lfsr, 0);

        /* 32 bytes of the initiation key compared correctly */
        if (++Card->LfsrCount == 32)
        {
            Card->State = IsaSleep;
        }
    }
}

static
VOID
IsaBusWriteDataRegister(
    _In_ UCHAR Value)
{
    ULONG i, j;

    switch (IsapAddressLatch)
    {
        case ISAPNP_READPORT:
        {
            /* Update the address of the Read Data Port */
            for (i = 0; i < IsapCardCount; ++i)
            {
                PISAPNP_CARD Card = &IsapCard[i];

                if (Card->State != IsaIsolation)
                    continue;

                Card->ReadDataPort = (PUCHAR)(((ULONG_PTR)Value << 2) | 3);
            }
            break;
        }

        case ISAPNP_CONFIGCONTROL:
        {
            if (Value & ISAPNP_CONFIG_WAIT_FOR_KEY)
            {
                IsapConfigureCard = NULL;
            }

            for (i = 0; i < IsapCardCount; ++i)
            {
                PISAPNP_CARD Card = &IsapCard[i];

                if (Card->State != IsaWaitForKey)
                {
                    if (Value & ISAPNP_CONFIG_RESET)
                    {
                        for (j = 0; j < Card->LogicalDevices; ++j)
                        {
                            PISAPNP_CARD_LOGICAL_DEVICE LogDev = &Card->LogDev[j];

                            LogDev->Registers[ISAPNP_ACTIVATE] = 0;
                        }
                    }
                    if (Value & ISAPNP_CONFIG_RESET_CSN)
                    {
                        Card->SelectNumberReg = 0;
                    }
                }
                if (Value & ISAPNP_CONFIG_WAIT_FOR_KEY)
                {
                    Card->State = IsaWaitForKey;
                }
            }
            break;
        }

        case ISAPNP_WAKE:
        {
            for (i = 0; i < IsapCardCount; ++i)
            {
                PISAPNP_CARD Card = &IsapCard[i];

                if (Card->State == IsaWaitForKey)
                    continue;

                if (Card->SelectNumberReg != Value)
                {
                    if (Card->State == IsaConfgure || Card->State == IsaIsolation)
                    {
                        Card->State = IsaSleep;

                        if (IsapConfigureCard == Card)
                        {
                            IsapConfigureCard = NULL;
                        }
                    }

                    continue;
                }

                Card->RomIdx = 0;
                Card->SerialIsolationIdx = 0;

                if (Card->State == IsaSleep)
                {
                    if (Value == 0)
                    {
                        Card->State = IsaIsolation;

                        Card->IsolationRead = 0;
                    }
                    else
                    {
                        Card->State = IsaConfgure;

                        /* Only one card can be in the configure state */
                        IsapConfigureCard = Card;
                    }
                }
            }

            break;
        }

        case ISAPNP_CARDSELECTNUMBER:
        {
            ULONG CsnAssigned = 0;

            /* Assign the CSN */
            for (i = 0; i < IsapCardCount; ++i)
            {
                PISAPNP_CARD Card = &IsapCard[i];

                if (Card->State != IsaIsolation)
                    continue;

                ok(Value != 0, "The new CSN is zero\n");
                ok(Card->SelectNumberReg != Value, "CSNs must be assigned sequentially");

                Card->State = IsaConfgure;
                Card->SelectNumberReg = Value;

                /* Only one card can be in the configure state */
                IsapConfigureCard = Card;

                ++CsnAssigned;
                ok_eq_ulong(CsnAssigned, 1UL);
            }
            break;
        }

        case ISAPNP_LOGICALDEVICENUMBER:
        {
            ok(IsapConfigureCard != NULL, "Invalid write to a LDN register\n");

            if (IsapConfigureCard != NULL)
            {
                ok(IsapConfigureCard->LogicalDevices != 0, "Write to a read-only register\n");
                ok(Value < IsapConfigureCard->LogicalDevices, "Invalid write to a LDN register\n");

                IsapConfigureCard->DeviceNumberReg = Value;
            }
            break;
        }

        case ISAPNP_ACTIVATE:
        {
            Value &= 0x01;
            goto WriteDeviceRegister;
        }

        case ISAPNP_IORANGECHECK:
        {
            Value &= 0x03;
            goto WriteDeviceRegister;
        }

        case ISAPNP_SERIALISOLATION:
        case ISAPNP_RESOURCEDATA:
        case ISAPNP_STATUS:
        {
            ok(FALSE, "Write to a read-only register %02x\n", IsapAddressLatch);
            break;
        }

        default:
        {
            if (IsapAddressLatch >= 0x40)
            {
                PISAPNP_CARD_LOGICAL_DEVICE LogDev;

WriteDeviceRegister:
                ok(IsapConfigureCard != NULL, "Invalid write to device register\n");

                if (IsapConfigureCard != NULL)
                {
                    LogDev = &IsapConfigureCard->LogDev[IsapConfigureCard->DeviceNumberReg];

                    LogDev->Registers[IsapAddressLatch] = Value;
                }
            }
            else
            {
                ok(FALSE, "Unexpected write to register %02x\n", IsapAddressLatch);
            }
            break;
        }
    }
}

static
UCHAR
IsaBusReadSerialIsolationRegister(
    _In_ PUCHAR Port)
{
    ULONG i, ResponseMap = 0, ListenMap = 0;
    UCHAR Result = 0xFF;

    for (i = 0; i < IsapCardCount; ++i)
    {
        PISAPNP_CARD Card = &IsapCard[i];

        if (Card->State != IsaIsolation || Card->ReadDataPort != Port)
            continue;

        /* The hardware on each card expects 72 pairs of reads */
        if (Card->SerialIsolationIdx == RTL_BITS_OF(ISAPNP_IDENTIFIER))
            continue;

        Card->IsolationRead ^= 1;

        if (Card->IsolationRead)
        {
            if (Card->PnpRom[Card->SerialIsolationIdx / 8] & (1 << (Card->SerialIsolationIdx % 8)))
                Card->SerialIdResponse = 0x55;
            else
                Card->SerialIdResponse = 0x00;

            ++Card->RomIdx;
            ++Card->SerialIsolationIdx;
        }
        else
        {
            Card->SerialIdResponse <<= 1;

            if (Card->SerialIdResponse == 0xAA)
                ResponseMap |= (1 << i);
            else
                ListenMap |= (1 << i);
        }

        if ((Card->SerialIdResponse > Result) || (Result == 0xFF))
            Result = Card->SerialIdResponse;
    }

    /* Release passive cards from the isolation state */
    if (ResponseMap != 0 && ListenMap != 0)
    {
        for (i = 0; i < RTL_BITS_OF(ListenMap); ++i)
        {
            if (ListenMap & (1 << i))
            {
                PISAPNP_CARD Card = &IsapCard[i];

                Card->State = IsaSleep;
            }
        }
    }

    return Result;
}

static
UCHAR
IsaBusReadDataPortRegister(
    _In_ PUCHAR Port)
{
    if (IsapAddressLatch == ISAPNP_SERIALISOLATION)
        return IsaBusReadSerialIsolationRegister(Port);

    if (IsapConfigureCard == NULL || IsapConfigureCard->ReadDataPort != Port)
        return 0xFF;

    switch (IsapAddressLatch)
    {
        case ISAPNP_RESOURCEDATA:
        {
            if (IsapConfigureCard->RomIdx >= IsapConfigureCard->RomSize)
                break;

            /* The resource data register may return an invalid identifier checksum byte */
            if (IsapConfigureCard->RomIdx == FIELD_OFFSET(ISAPNP_IDENTIFIER, Checksum))
            {
                ++IsapConfigureCard->RomIdx;
                break;
            }

            return IsapConfigureCard->PnpRom[IsapConfigureCard->RomIdx++];
        }

        case ISAPNP_STATUS:
            return 0x01; /* Resource data byte available */

        case ISAPNP_CARDSELECTNUMBER:
            return IsapConfigureCard->SelectNumberReg;

        case ISAPNP_LOGICALDEVICENUMBER:
            return IsapConfigureCard->DeviceNumberReg;

        case ISAPNP_ACTIVATE:
        case ISAPNP_IORANGECHECK:
            goto ReadDeviceRegister;

        default:
        {
            if (IsapAddressLatch >= 0x40)
            {
                PISAPNP_CARD_LOGICAL_DEVICE LogDev;

ReadDeviceRegister:
                LogDev = &IsapConfigureCard->LogDev[IsapConfigureCard->DeviceNumberReg];

                return LogDev->Registers[IsapAddressLatch];
            }
            else
            {
                ok(FALSE, "Unexpected read from register %02x\n", IsapAddressLatch);
            }
            break;
        }
    }

    return 0xFF;
}

static
UCHAR
IsaBusPnpChecksum(
    _In_ PISAPNP_IDENTIFIER Identifier)
{
    UCHAR i, j, Lfsr;

    Lfsr = ISAPNP_LFSR_SEED;
    for (i = 0; i < FIELD_OFFSET(ISAPNP_IDENTIFIER, Checksum); ++i)
    {
        UCHAR Byte = ((PUCHAR)Identifier)[i];

        for (j = 0; j < RTL_BITS_OF(Byte); ++j)
        {
            Lfsr = IsaBusNextLFSR(Lfsr, Byte);
            Byte >>= 1;
        }
    }

    return Lfsr;
}

static
UCHAR
IsaBusResourceDataChecksum(
    _In_ PUCHAR PnpRom,
    _In_ ULONG RomSize)
{
    UNREFERENCED_PARAMETER(PnpRom);
    UNREFERENCED_PARAMETER(RomSize);

    /* This means "Checksummed properly" */
    return 0x00;
}

static
VOID
IsaBusPlugInCard(
    _Inout_ PISAPNP_CARD Card)
{
    Card->State = IsaWaitForKey;
    Card->Lfsr = ISAPNP_LFSR_SEED;
    Card->LfsrCount = 0;
    Card->SelectNumberReg = 0;
    Card->ReadDataPort = NULL;
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
IsaBusCreateCard(
    _Inout_ PISAPNP_CARD Card,
    _In_ PVOID PnpRom,
    _In_ ULONG RomSize,
    _In_ ULONG LogicalDevices)
{
    Card->RomSize = RomSize;
    Card->PnpRom = PnpRom;
    Card->PnpRom[FIELD_OFFSET(ISAPNP_IDENTIFIER, Checksum)] = IsaBusPnpChecksum(PnpRom);
    Card->PnpRom[RomSize - 1] = IsaBusResourceDataChecksum(PnpRom, RomSize);
    Card->LogicalDevices = LogicalDevices;

    IsaBusPlugInCard(Card);

    ++IsapCardCount;
}

VOID
NTAPI
WRITE_PORT_UCHAR(
    _In_ PUCHAR Port,
    _In_ UCHAR Value)
{
    switch ((ULONG_PTR)Port)
    {
        case 0x279:
            IsaBusWriteAddressRegister(Value);
            break;

        case 0xA79:
            IsaBusWriteDataRegister(Value);
            break;

        default:
            ok(FALSE, "Unexpected write to port %p %02x\n", Port, Value);
            break;
    }
}

UCHAR
NTAPI
READ_PORT_UCHAR(
    _In_ PUCHAR Port)
{
    UCHAR Result;

    /* We can write only to NT Read Data Ports */
    switch ((ULONG_PTR)Port)
    {
        case 0x2F4 | 3:
            Result = IsaBusReadDataPortRegister(Port);
            break;

        /* Indicate that the Read Data Port is in conflict */
        case 0x274 | 3:
        case 0x3E4 | 3:
        case 0x204 | 3:
        case 0x2E4 | 3:
        case 0x354 | 3:
            Result = 0x00;
            break;

        default:
            ok(FALSE, "Unexpected read from port %p\n", Port);
            Result = 0xFF;
            break;
    }

    return Result;
}
