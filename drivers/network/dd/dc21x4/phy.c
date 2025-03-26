/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PHY layer setup and management
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* GLOBALS ********************************************************************/

#define MII_READ(Adapter, Data)  \
    do { \
        *Data = DC_READ((Adapter), DcCsr9_SerialInterface); \
        NdisStallExecution(2); \
    } while (0)

#define MII_WRITE(Adapter, Value)  \
    do { \
        DC_WRITE((Adapter), DcCsr9_SerialInterface, Value); \
        NdisStallExecution(2); \
    } while (0)

/* FUNCTIONS ******************************************************************/

static
VOID
MiiMdioPacket(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Sequence,
    _In_ ULONG BitCount)
{
    LONG i;

    for (i = BitCount - 1; i >= 0; --i)
    {
        ULONG Mdo = ((Sequence >> i) & 1) << DC_SERIAL_MII_MDO_SHIFT;

        MII_WRITE(Adapter, Mdo);
        MII_WRITE(Adapter, Mdo | DC_SERIAL_MII_MDC);
    }
}

static
ULONG
MiiMdioShiftIn(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i, Csr;
    ULONG Result = 0;

    for (i = 0; i < RTL_BITS_OF(USHORT); ++i)
    {
        MII_WRITE(Adapter, DC_SERIAL_MII_MII);
        MII_WRITE(Adapter, DC_SERIAL_MII_MII | DC_SERIAL_MII_MDC);

        MII_READ(Adapter, &Csr);
        Result = (Result << 1) | ((Csr >> DC_SERIAL_MII_MDI_SHIFT) & 1);
    }

    return Result;
}

static
VOID
MiiMdioClearExtraBits(
    _In_ PDC21X4_ADAPTER Adapter)
{
    MII_WRITE(Adapter, DC_SERIAL_MII_MII);
    MII_WRITE(Adapter, DC_SERIAL_MII_MII | DC_SERIAL_MII_MDC);
}

BOOLEAN
MiiWrite(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _In_ ULONG Data)
{
    MiiMdioPacket(Adapter, MDIO_PREAMBLE, 32);
    MiiMdioPacket(Adapter,
                  (MDIO_START << 30) |
                  (MDIO_WRITE << 28) |
                  (PhyAddress << 23) |
                  (RegAddress << 18) |
                  (MDIO_TA    << 16) |
                  Data,
                  32);

    /* Idle state */
    MiiMdioClearExtraBits(Adapter);

    return TRUE;
}

BOOLEAN
MiiRead(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _Out_ PULONG Data)
{
    ULONG Csr;
    BOOLEAN Success;

    MiiMdioPacket(Adapter, MDIO_PREAMBLE, 32);
    MiiMdioPacket(Adapter,
                  (MDIO_START << 12) |
                  (MDIO_READ  << 10) |
                  (PhyAddress << 5) |
                  RegAddress,
                  14);

    /* Turnaround */
    MiiMdioClearExtraBits(Adapter);

    Csr = DC_READ(Adapter, DcCsr9_SerialInterface);
    Success = !(Csr & DC_SERIAL_MII_MDI);

    *Data = MiiMdioShiftIn(Adapter);

    /* Idle state */
    MiiMdioClearExtraBits(Adapter);

    return Success;
}

static
CODE_SEG("PAGE")
VOID
HpnaSpiClose(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    DC_WRITE(Adapter, DcCsr9_SerialInterface, 0);
    DC_WRITE(Adapter, DcCsr9_SerialInterface, DC_SERIAL_SPI_SK);
}

static
CODE_SEG("PAGE")
VOID
HpnaSpiShiftOut(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Sequence,
    _In_ ULONG BitCount)
{
    LONG i;

    PAGED_CODE();

    DC_WRITE(Adapter, DcCsr9_SerialInterface, 0);
    DC_WRITE(Adapter, DcCsr9_SerialInterface, DC_SERIAL_SPI_CS);

    for (i = BitCount - 1; i >= 0; --i)
    {
        ULONG DataIn = ((Sequence >> i) & 1) << DC_SERIAL_SPI_DI_SHIFT;

        DC_WRITE(Adapter, DcCsr9_SerialInterface, DataIn | DC_SERIAL_SPI_CS);
        DC_WRITE(Adapter, DcCsr9_SerialInterface, DataIn | DC_SERIAL_SPI_CS | DC_SERIAL_SPI_SK);
    }

    DC_WRITE(Adapter, DcCsr9_SerialInterface, 0);
    DC_WRITE(Adapter, DcCsr9_SerialInterface, DC_SERIAL_SPI_SK);
}

static
CODE_SEG("PAGE")
VOID
HpnaWrite(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG RegAddress,
    _In_ ULONG Data)
{
    PAGED_CODE();

    HpnaSpiShiftOut(Adapter, DC_SPI_SET_WRITE_ENABLE, 8);
    HpnaSpiClose(Adapter);

    HpnaSpiShiftOut(Adapter,
                    (Data << 16) |
                    (RegAddress << 8) |
                    DC_SPI_BYTE_WRITE_OPERATION,
                    RTL_BITS_OF(UCHAR) * 3);
    HpnaSpiClose(Adapter);
}

CODE_SEG("PAGE")
VOID
HpnaPhyInit(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG SiaConn, i;

    PAGED_CODE();

    /* Select the HPNA interface */
    SiaConn = DC_READ(Adapter, DcCsr13_SiaConnectivity);
    SiaConn |= DC_SIA_CONN_HPNA;
    DC_WRITE(Adapter, DcCsr13_SiaConnectivity, SiaConn);

    for (i = 0; i < RTL_NUMBER_OF(Adapter->HpnaRegister); ++i)
    {
        if (Adapter->HpnaInitBitmap & (1 << i))
        {
            HpnaWrite(Adapter, i, Adapter->HpnaRegister[i]);
        }
    }
}

CODE_SEG("PAGE")
BOOLEAN
DcFindMiiPhy(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG Phy;

    PAGED_CODE();

    /* Look for the first connected PHY */
    for (Phy = 1; Phy <= MII_MAX_PHY_ADDRESSES; ++Phy)
    {
        ULONG PhyAddress = Phy % MII_MAX_PHY_ADDRESSES; /* Check the PHY 0 last */
        ULONG MiiStatus;
#if DBG
        ULONG PhyIdLow, PhyIdHigh, MiiControl, MiiAdvertise;
#endif

        /*
         * Read the status register. Some PHYs, such as the ML6692,
         * don't implement the IEEE ID registers.
         */
        if (!MiiRead(Adapter, PhyAddress, MII_STATUS, &MiiStatus))
            continue;
        if (MiiStatus == 0xFFFF || MiiStatus == 0)
            continue;

#if DBG
        MiiRead(Adapter, PhyAddress, MII_PHY_ID1, &PhyIdLow);
        MiiRead(Adapter, PhyAddress, MII_PHY_ID2, &PhyIdHigh);
        MiiRead(Adapter, PhyAddress, MII_CONTROL, &MiiControl);
        MiiRead(Adapter, PhyAddress, MII_AUTONEG_ADVERTISE, &MiiAdvertise);

        INFO_VERB("Found PHY at address %u: ID %04lx:%04lx, Ctrl %04lx, Status %04lx, Adv %04lx\n",
                  PhyAddress,
                  PhyIdLow,
                  PhyIdHigh,
                  MiiControl,
                  MiiStatus,
                  MiiAdvertise);
#endif

        Adapter->PhyAddress = PhyAddress;

        return TRUE;
    }

    return FALSE;
}
