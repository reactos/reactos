/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Novell Eagle 2000 driver
 * FILE:        ne2000/8390.c
 * PURPOSE:     DP8390 NIC specific routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 27/08-2000 Created
 */
#include <ne2000.h>


BOOLEAN NICCheck(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Tests for a NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     TRUE if NIC is believed to be present, FALSE if not
 * NOTES:
 *     If the adapter responds correctly to a
 *     stop command we assume it is present
 */
{
    UCHAR Tmp;

    /* Disable interrupts */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_IMR, 0);

    /* Stop the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2);

    /* Pause for 1.6ms */
    NdisStallExecution(1600);

    /* Read NIC response */
    NdisRawReadPortUchar(Adapter->IOBase + PG0_CR, &Tmp);

    if ((Tmp == (CR_RD2 | CR_STP)) || (Tmp == (CR_RD2 | CR_STP | CR_STA)))
        return TRUE;
    else
        return FALSE;
}


BOOLEAN NICTestAddress(
    PNIC_ADAPTER Adapter,
    ULONG Address)
/*
 * FUNCTION: Tests if an address is writable
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     TRUE if the address is writable, FALSE if not
 */
{
    USHORT Data;
    USHORT Tmp;

    /* Read one word */
    NICReadDataAlign(Adapter, &Data, Address, 0x02);

    /* Alter it */
    Data ^= 0xFFFF;

    /* Write it back */
    NICWriteDataAlign(Adapter, Address, &Data, 0x02);

    /* Check if it has changed on the NIC */
    NICReadDataAlign(Adapter, &Tmp, Address, 0x02);

    return (Data == Tmp);
}


BOOLEAN NICTestRAM(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Finds out how much RAM a NIC has
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     TRUE if the RAM size was found, FALSE if not
 * NOTES:
 *     Start at 1KB and test for every 1KB up to 64KB
 */
{
    ULONG Base;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Locate RAM base address */
    for (Base = 0x0400; Base < 0x10000; Base += 0x0400) {
        if (NICTestAddress(Adapter, Base))
            break;
    }

    if (Base == 0x10000) {
        /* No RAM on this board */
        NDIS_DbgPrint(MIN_TRACE, ("No RAM found on board.\n"));
        return FALSE;
    }

    Adapter->RamBase = (PUCHAR)Base;

    /* Find RAM size */
    for (; Base < 0x10000; Base += 0x0400) {
        if (!NICTestAddress(Adapter, Base))
            break;
    }

    Adapter->RamSize = (UINT)(Base - (ULONG_PTR)Adapter->RamBase);

    NDIS_DbgPrint(MID_TRACE, ("RAM is at (0x%X). Size is (0x%X).\n",
        Adapter->RamBase, Adapter->RamSize));

    return TRUE;
}


VOID NICSetPhysicalAddress(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Initializes the physical address on the NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * NOTES:
 *     The physical address is taken from Adapter.
 *     The NIC is stopped by this operation
 */
{
    UINT i;

    /* Select page 1 */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE1);

    /* Initialize PAR - Physical Address Registers */
    for (i = 0; i < 0x06; i++)
        NdisRawWritePortUchar(Adapter->IOBase + PG1_PAR + i, Adapter->StationAddress[i]);

    /* Go back to page 0 */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE0);
}


VOID NICSetMulticastAddressMask(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Initializes the multicast address mask on the NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * NOTES:
 *     The multicast address mask is taken from Adapter.
 *     The NIC is stopped by this operation
 */
{
    UINT i;

    /* Select page 1 */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE1);

    /* Initialize MAR - Multicast Address Registers */
    for (i = 0; i < 0x08; i++)
        NdisRawWritePortUchar(Adapter->IOBase + PG1_MAR + i, Adapter->MulticastAddressMask[i]);

    /* Go back to page 0 */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE0);
}


BOOLEAN NICReadSAPROM(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Reads the Station Address PROM data from the NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     TRUE if a the NIC is an NE2000
 * NOTES:
 *    This routine also determines if the NIC can support word mode transfers
 *    and if it does initializes the NIC for word mode.
 *    The station address in the adapter structure is initialized with
 *    the address from the SAPROM
 */
{
    UINT i;
    UCHAR Buffer[32];
    UCHAR WordLength;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Read Station Address PROM (SAPROM) which is 16 bytes at remote DMA address 0.
       Some cards double the data read which we must compensate for */

    /* Initialize RBCR0 and RBCR1 - Remote Byte Count Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, 0x20);
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, 0x00);

    /* Initialize RSAR0 and RSAR1 - Remote Start Address Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR0, 0x00);
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR1, 0x00);

    /* Select page 0, read and start the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD0 | CR_PAGE0);

    /* Read one byte at a time */
    WordLength = 2; /* Assume a word is two bytes */
    for (i = 0; i < 32; i += 2) {
        NdisRawReadPortUchar(Adapter->IOBase + NIC_DATA, &Buffer[i]);
        NdisRawReadPortUchar(Adapter->IOBase + NIC_DATA, &Buffer[i + 1]);
		if (Buffer[i] != Buffer[i + 1])
			WordLength = 1; /* A word is one byte long */
	}

    /* If WordLength is 2 the data read before was doubled. We must compensate for this */
    if (WordLength == 2) {
        DbgPrint("NE2000 or compatible network adapter found.\n");

        Adapter->WordMode = TRUE;

        /* Move the SAPROM data to the adapter object */
        for (i = 0; i < 16; i++)
            Adapter->SAPROM[i] = Buffer[i * 2];

        /* Copy the station address */
        NdisMoveMemory(
            (PVOID)&Adapter->StationAddress,
            (PVOID)&Adapter->SAPROM,
            DRIVER_LENGTH_OF_ADDRESS);

        /* Initialize DCR - Data Configuration Register (word mode/4 words FIFO) */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_DCR, DCR_WTS | DCR_LS | DCR_FT10);

        return TRUE;
    } else {
        DbgPrint("NE1000 or compatible network adapter found.\n");

        Adapter->WordMode = FALSE;

        return FALSE;
    }
}


NDIS_STATUS NICInitialize(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Initializes a NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     Status of NIC initialization
 * NOTES:
 *     The NIC is put into loopback mode
 */
{
    UCHAR Tmp;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (!NICCheck(Adapter)) {
        NDIS_DbgPrint(MID_TRACE, ("No adapter found at (0x%X).\n", Adapter->IOBase));
        return NDIS_STATUS_ADAPTER_NOT_FOUND;
    } else
        NDIS_DbgPrint(MID_TRACE, ("Adapter found at (0x%X).\n", Adapter->IOBase));

    /* Reset the NIC */
    NdisRawReadPortUchar(Adapter->IOBase + NIC_RESET, &Tmp);

    /* Wait for 1.6ms */
    NdisStallExecution(1600);

    /* Write the value back  */
    NdisRawWritePortUchar(Adapter->IOBase + NIC_RESET, Tmp);

    /* Select page 0 and stop NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE0);
   
    /* Initialize DCR - Data Configuration Register (byte mode/8 bytes FIFO) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_DCR, DCR_LS | DCR_FT10);

    /* Clear RBCR0 and RBCR1 - Remote Byte Count Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, 0x00);
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, 0x00);

    /* Initialize RCR - Receive Configuration Register (monitor mode) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RCR, RCR_MON);

    /* Enter loopback mode (internal NIC module loopback) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TCR, TCR_LOOP);

    /* Read the Station Address PROM */
    if (!NICReadSAPROM(Adapter))
        return NDIS_STATUS_ADAPTER_NOT_FOUND;

    NDIS_DbgPrint(MID_TRACE, ("Station address is (%02X %02X %02X %02X %02X %02X).\n",
        Adapter->StationAddress[0], Adapter->StationAddress[1],
        Adapter->StationAddress[2], Adapter->StationAddress[3],
        Adapter->StationAddress[4], Adapter->StationAddress[5]));

    /* Select page 0 and start NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2 | CR_PAGE0);

    /* Clear ISR - Interrupt Status Register */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, 0xFF);

    /* Find NIC RAM size */
    NICTestRAM(Adapter);

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS NICSetup(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Sets up a NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     The NIC is put into loopback mode
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (Adapter->WordMode ) {
        /* Initialize DCR - Data Configuration Register (word mode/4 words FIFO) */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_DCR, DCR_WTS | DCR_LS | DCR_FT10);
    } else {
        /* Initialize DCR - Data Configuration Register (byte mode/8 bytes FIFO) */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_DCR, DCR_LS | DCR_FT10);
    }

    /* Clear RBCR0 and RBCR1 - Remote Byte Count Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, 0x00);
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, 0x00);

    /* Initialize RCR - Receive Configuration Register (monitor mode) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RCR, RCR_MON);

    /* Enter loopback mode (internal NIC module loopback) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TCR, TCR_LOOP);

    /* Set boundary page */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_BNRY, Adapter->NextPacket);

    /* Set start page */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_PSTART, Adapter->PageStart);

    /* Set stop page */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_PSTOP, Adapter->PageStop);

    /* Program our address on the NIC */
    NICSetPhysicalAddress(Adapter);

    /* Program the multicast address mask on the NIC */
    NICSetMulticastAddressMask(Adapter);

    /* Select page 1 and stop NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE1);

    /* Initialize current page register */
    NdisRawWritePortUchar(Adapter->IOBase + PG1_CURR, Adapter->PageStart + 1);

    /* Select page 0 and stop NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE0);

    /* Clear ISR - Interrupt Status Register */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, 0xFF);

    /* Initialize IMR - Interrupt Mask Register */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_IMR, Adapter->InterruptMask);

    /* Select page 0 and start NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2 | CR_PAGE0);

    Adapter->CurrentPage            = Adapter->PageStart + 1;
    Adapter->NextPacket             = Adapter->PageStart + 1;
    Adapter->BufferOverflow         = FALSE;
    Adapter->ReceiveError           = FALSE;
    Adapter->TransmitError          = FALSE;

    NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS NICStart(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Starts a NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Take NIC out of loopback mode */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TCR, 0x00);

    /* Initialize RCR - Receive Configuration Register (accept all) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RCR, RCR_AB | RCR_AM | RCR_PRO);

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS NICStop(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Stops a NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     Status of operation
 */
{
    UCHAR Tmp;
    UINT i;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Select page 0 and stop NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE0);

    /* Clear Remote Byte Count Register so ISR_RST will be set */
    NdisRawWritePortUchar( Adapter->IOBase + PG0_RBCR0, 0x00);
    NdisRawWritePortUchar( Adapter->IOBase + PG0_RBCR0, 0x00);

    /* Wait for ISR_RST to be set, but timeout after 2ms */
    for (i = 0; i < 4; i++) {
        NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &Tmp);
        if (Tmp & ISR_RST)
            break;

        NdisStallExecution(500);
    }

#ifdef DBG
    if (i == 4)
        NDIS_DbgPrint(MIN_TRACE, ("NIC was not reset after 2ms.\n"));
#endif

    /* Initialize RCR - Receive Configuration Register (monitor mode) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RCR, RCR_MON);

    /* Initialize TCR - Transmit Configuration Register (loopback mode) */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TCR, TCR_LOOP);

    /* Start NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2);

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS NICReset(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Resets a NIC
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * RETURNS:
 *     Status of operation
 */
{
    UCHAR Tmp;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Stop the NIC */
    NICStop(Adapter);

    /* Reset the NIC */
    NdisRawReadPortUchar(Adapter->IOBase + NIC_RESET, &Tmp);

    /* Wait for 1.6ms */
    NdisStallExecution(1600);

    /* Write the value back  */
    NdisRawWritePortUchar(Adapter->IOBase + NIC_RESET, Tmp);

    /* Restart the NIC */
    NICStart(Adapter);

    return NDIS_STATUS_SUCCESS;
}


VOID NICStartTransmit(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Starts transmitting a packet
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    UINT Length;

    /* Set start of frame */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TPSR,
        Adapter->TXStart + Adapter->TXCurrent * DRIVER_BLOCK_SIZE);

    /* Set length of frame */
    Length = Adapter->TXSize[Adapter->TXCurrent];
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TBCR0, Length & 0xFF);
    NdisRawWritePortUchar(Adapter->IOBase + PG0_TBCR1, Length >> 8);

    /* Start transmitting */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_TXP | CR_RD2);

    NDIS_DbgPrint(MAX_TRACE, ("Transmitting. Buffer (%d)  Size (%d).\n",
        Adapter->TXCurrent,
        Length));

}


VOID NICSetBoundaryPage(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Sets the boundary page on the adapter to be one less than NextPacket
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    if (Adapter->NextPacket == Adapter->PageStart) {
        NdisRawWritePortUchar(Adapter->IOBase + PG0_BNRY,
            (UCHAR)(Adapter->PageStop - 1));
    } else {
        NdisRawWritePortUchar(Adapter->IOBase + PG0_BNRY,
            (UCHAR)(Adapter->NextPacket - 1));
    }
}


VOID NICGetCurrentPage(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Retrieves the current page from the adapter
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    UCHAR Current;

    /* Select page 1 */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2 | CR_PAGE1);

    /* Read current page */
    NdisRawReadPortUchar(Adapter->IOBase + PG1_CURR, &Current);

    /* Select page 0 */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2 | CR_PAGE0);

    Adapter->CurrentPage = Current;
}


VOID NICUpdateCounters(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Updates counters
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    UCHAR Tmp;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    NdisRawReadPortUchar(Adapter->IOBase + PG0_CNTR0, &Tmp);
    Adapter->FrameAlignmentErrors += Tmp;

    NdisRawReadPortUchar(Adapter->IOBase + PG0_CNTR1, &Tmp);
    Adapter->CrcErrors += Tmp;

    NdisRawReadPortUchar(Adapter->IOBase + PG0_CNTR2, &Tmp);
    Adapter->MissedPackets += Tmp;
}


VOID NICReadDataAlign(
    PNIC_ADAPTER Adapter,
    PUSHORT Target,
    ULONG Source,
    USHORT Length)
/*
 * FUNCTION: Copies data from a NIC's RAM into a buffer
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 *     Target  = Pointer to buffer to copy data into (in host memory)
 *     Source  = Offset into NIC's RAM (must be an even number)
 *     Length  = Number of bytes to copy from NIC's RAM (must be an even number)
 */
{
    UCHAR Tmp;
    USHORT Count;

    Count = Length;

    /* Select page 0 and start the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2 | CR_PAGE0);

    /* Initialize RSAR0 and RSAR1 - Remote Start Address Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR0, (UCHAR)(Source & 0xFF));
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR1, (UCHAR)(Source >> 8));

    /* Initialize RBCR0 and RBCR1 - Remote Byte Count Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, (UCHAR)(Count & 0xFF));
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, (UCHAR)(Count >> 8));

    /* Select page 0, read and start the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD0 | CR_PAGE0);
    
    if (Adapter->WordMode)
        NdisRawReadPortBufferUshort(Adapter->IOBase + NIC_DATA, Target, Count >> 1);
    else
        NdisRawReadPortBufferUchar(Adapter->IOBase + NIC_DATA, Target, Count);

    /* Wait for remote DMA to complete, but timeout after some time */
    for (Count = 0; Count < 0xFFFF; Count++) {
        NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &Tmp);
        if (Tmp & ISR_RDC)
            break;

        NdisStallExecution(4);
    }

#ifdef DBG
    if (Count == 0xFFFF)
        NDIS_DbgPrint(MIN_TRACE, ("Remote DMA did not complete.\n"));
#endif

    /* Clear remote DMA bit in ISR - Interrupt Status Register */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, ISR_RDC);
}


VOID NICWriteDataAlign(
    PNIC_ADAPTER Adapter,
    ULONG Target,
    PUSHORT Source,
    USHORT Length)
/*
 * FUNCTION: Copies data from a buffer into the NIC's RAM
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 *     Target  = Offset into NIC's RAM (must be an even number)
 *     Source  = Pointer to buffer to copy data from (in host memory)
 *     Length  = Number of bytes to copy from the buffer (must be an even number)
 */
{
    UCHAR Tmp;
    USHORT Count;

    /* Select page 0 and start the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2 | CR_PAGE0);

    /* Handle read-before-write bug */

    /* Initialize RSAR0 and RSAR1 - Remote Start Address Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR0, (UCHAR)(Target & 0xFF));
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR1, (UCHAR)(Target >> 8));

    /* Initialize RBCR0 and RBCR1 - Remote Byte Count Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, 0x02);
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, 0x00);

    /* Read and start the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD0 | CR_PAGE0);

    /* Read data */
    NdisRawReadPortUshort(Adapter->IOBase + NIC_DATA, &Count);

    /* Wait for remote DMA to complete, but timeout after some time */
    for (Count = 0; Count < 0xFFFF; Count++) {
        NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &Tmp);
        if (Tmp & ISR_RDC)
            break;

        NdisStallExecution(4);
    }

#ifdef DBG
    if (Count == 0xFFFF)
        NDIS_DbgPrint(MIN_TRACE, ("Remote DMA did not complete.\n"));
#endif

    /* Clear remote DMA bit in ISR - Interrupt Status Register */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, ISR_RDC);


    /* Now output some data */

    Count = Length;

    /* Initialize RSAR0 and RSAR1 - Remote Start Address Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR0, (UCHAR)(Target & 0xFF));
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RSAR1, (UCHAR)(Target >> 8));

    /* Initialize RBCR0 and RBCR1 - Remote Byte Count Registers */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, (UCHAR)(Count & 0xFF));
    NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, (UCHAR)(Count >> 8));

    /* Write and start the NIC */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD1 | CR_PAGE0);
    
    if (Adapter->WordMode)
        NdisRawWritePortBufferUshort(Adapter->IOBase + NIC_DATA, Source, Count >> 1);
    else
        NdisRawWritePortBufferUchar(Adapter->IOBase + NIC_DATA, Source, Count);

    /* Wait for remote DMA to complete, but timeout after some time */
    for (Count = 0; Count < 0xFFFF; Count++) {
        NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &Tmp);
        if (Tmp & ISR_RDC)
            break;

        NdisStallExecution(4);
    }

#ifdef DBG
    if (Count == 0xFFFF)
        NDIS_DbgPrint(MIN_TRACE, ("Remote DMA did not complete.\n"));
#endif

    /* Clear remote DMA bit in ISR - Interrupt Status Register */
    NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, ISR_RDC);
}


VOID NICReadData(
    PNIC_ADAPTER Adapter,
    PUCHAR Target,
    ULONG Source,
    USHORT Length)
/*
 * FUNCTION: Copies data from a NIC's RAM into a buffer
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 *     Target  = Pointer to buffer to copy data into (in host memory)
 *     Source  = Offset into NIC's RAM
 *     Length  = Number of bytes to copy from NIC's RAM
 */
{
    USHORT Tmp;

    /* Avoid transfers to odd addresses */
    if (Source & 0x01) {
        /* Transfer one word and use the MSB */
        NICReadDataAlign(Adapter, &Tmp, Source - 1, 0x02);
        *Target = (UCHAR)(Tmp >> 8);
        Source++;
        Target++;
        Length--;
    }

    if (Length & 0x01) {
        /* Transfer as many words as we can without exceeding the buffer length */
        Tmp = Length & 0xFFFE;
        NICReadDataAlign(Adapter, (PUSHORT)Target, Source, Tmp);
        Source            += Tmp;
        (ULONG_PTR)Target += Tmp;

        /* Read one word and keep the LSB */
        NICReadDataAlign(Adapter, &Tmp, Source, 0x02);
        *Target = (UCHAR)(Tmp & 0x00FF);
    } else
        /* Transfer the rest of the data */
        NICReadDataAlign(Adapter, (PUSHORT)Target, Source, Length);
}


VOID NICWriteData(
    PNIC_ADAPTER Adapter,
    ULONG Target,
    PUCHAR Source,
    USHORT Length)
/*
 * FUNCTION: Copies data from a buffer into NIC's RAM
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 *     Target  = Offset into NIC's RAM to store data
 *     Source  = Pointer to buffer to copy data from (in host memory)
 *     Length  = Number of bytes to copy from buffer
 */
{
    USHORT Tmp;

    /* Avoid transfers to odd addresses */
    if (Target & 0x01) {
        /* Read one word */
        NICReadDataAlign(Adapter, &Tmp, Target - 1, 0x02);

        /* Merge LSB with the new byte which become the new MSB */
        Tmp = (Tmp & 0x00FF) | (*Source << 8);

        /* Finally write the value back */
        NICWriteDataAlign(Adapter, Target - 1, &Tmp, 0x02);

        /* Update pointers */
        (ULONG_PTR)Source += 1;
        (ULONG_PTR)Target += 1;
        Length--;
    }

    if (Length & 0x01) {
        /* Transfer as many words as we can without exceeding the transfer length */
        Tmp = Length & 0xFFFE;
        NICWriteDataAlign(Adapter, Target, (PUSHORT)Source, Tmp);
        Source            += Tmp;
        (ULONG_PTR)Target += Tmp;

        /* Read one word */
        NICReadDataAlign(Adapter, &Tmp, Target, 0x02);

        /* Merge MSB with the new byte which become the new LSB */
        Tmp = (Tmp & 0xFF00) | (*Source);

        /* Finally write the value back */
        NICWriteDataAlign(Adapter, Target, &Tmp, 0x02);
    } else
        /* Transfer the rest of the data */
        NICWriteDataAlign(Adapter, Target, (PUSHORT)Source, Length);
}


VOID NICIndicatePacket(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Indicates a packet to the wrapper
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    UINT IndicateLength;

    IndicateLength = (Adapter->PacketHeader.PacketLength <
        (Adapter->LookaheadSize + DRIVER_HEADER_SIZE))?
        (Adapter->PacketHeader.PacketLength) :
        (Adapter->LookaheadSize + DRIVER_HEADER_SIZE);

    /* Fill the lookahead buffer */
    NICReadData(Adapter,
                (PUCHAR)&Adapter->Lookahead,
                Adapter->PacketOffset + sizeof(PACKET_HEADER),
                IndicateLength + DRIVER_HEADER_SIZE);

    NDIS_DbgPrint(MAX_TRACE, ("Indicating (%d) bytes.\n", IndicateLength));

#if 0
    NDIS_DbgPrint(MAX_TRACE, ("FRAME:\n"));
    for (i = 0; i < (IndicateLength + 7) / 8; i++) {
        NDIS_DbgPrint(MAX_TRACE, ("%02X %02X %02X %02X %02X %02X %02X %02X\n",
            Adapter->Lookahead[i*8+0],
            Adapter->Lookahead[i*8+1],
            Adapter->Lookahead[i*8+2],
            Adapter->Lookahead[i*8+3],
            Adapter->Lookahead[i*8+4],
            Adapter->Lookahead[i*8+5],
            Adapter->Lookahead[i*8+6],
            Adapter->Lookahead[i*8+7]));
    }
#endif

    if (IndicateLength >= DRIVER_HEADER_SIZE) {
        NdisMEthIndicateReceive(Adapter->MiniportAdapterHandle,
                                NULL,
                                (PVOID)&Adapter->Lookahead,
                                DRIVER_HEADER_SIZE,
                                (PVOID)&Adapter->Lookahead[DRIVER_HEADER_SIZE],
                                IndicateLength - DRIVER_HEADER_SIZE,
                                Adapter->PacketHeader.PacketLength - DRIVER_HEADER_SIZE);
    } else {
        NdisMEthIndicateReceive(Adapter->MiniportAdapterHandle,
                                NULL,
                                (PVOID)&Adapter->Lookahead,
                                IndicateLength,
                                NULL,
                                0,
                                0);
    }
}


VOID NICReadPacket(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Reads a full packet from the receive buffer ring
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    BOOLEAN SkipPacket = FALSE;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Get the header of the next packet in the receive ring */
    Adapter->PacketOffset = Adapter->NextPacket << 8;
    NICReadData(Adapter,
                (PUCHAR)&Adapter->PacketHeader,
                Adapter->PacketOffset,
                sizeof(PACKET_HEADER));

    NDIS_DbgPrint(MAX_TRACE, ("HEADER: (Status)       (0x%X)\n", Adapter->PacketHeader.Status));
    NDIS_DbgPrint(MAX_TRACE, ("HEADER: (NextPacket)   (0x%X)\n", Adapter->PacketHeader.NextPacket));
    NDIS_DbgPrint(MAX_TRACE, ("HEADER: (PacketLength) (0x%X)\n", Adapter->PacketHeader.PacketLength));

    if (Adapter->PacketHeader.PacketLength < 64  ||
        Adapter->PacketHeader.PacketLength > 1518) {
        NDIS_DbgPrint(MAX_TRACE, ("Bogus packet size (%d).\n",
            Adapter->PacketHeader.PacketLength));
        SkipPacket = TRUE;
    }

    if (SkipPacket) {
        /* Skip packet */
        Adapter->NextPacket = Adapter->CurrentPage;
    } else {
        NICIndicatePacket(Adapter);

        /* Go to the next free buffer in receive ring */
        Adapter->NextPacket = Adapter->PacketHeader.NextPacket;
    }

    /* Update boundary page */
    NICSetBoundaryPage(Adapter);
}


VOID NICWritePacket(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Writes a full packet to the transmit buffer ring
 * ARGUMENTS:
 *     Adapter  = Pointer to adapter information
 * NOTES:
 *     There must be enough free buffers available in the transmit buffer ring.
 *     The packet is taken from the head of the transmit queue and the position
 *     into the transmit buffer ring is taken from TXNext
 */
{
    PNDIS_BUFFER SrcBuffer;
    UINT BytesToCopy, SrcSize, DstSize;
    PUCHAR SrcData;
    ULONG DstData;
    UINT TXStart;
    UINT TXStop;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    TXStart = Adapter->TXStart * DRIVER_BLOCK_SIZE;
    TXStop  = (Adapter->TXStart + Adapter->TXCount) * DRIVER_BLOCK_SIZE;

    NdisQueryPacket(Adapter->TXQueueHead,
                    NULL,
                    NULL,
                    &SrcBuffer,
                    &Adapter->TXSize[Adapter->TXNext]);

    NDIS_DbgPrint(MAX_TRACE, ("Packet Size (%d) is now (%d).\n",
        Adapter->TXNext,
        Adapter->TXSize[Adapter->TXNext]));

    NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);

    DstData = TXStart + Adapter->TXNext * DRIVER_BLOCK_SIZE;
    DstSize = TXStop - DstData;

    /* Start copying the data */
    for (;;) {
        BytesToCopy = (SrcSize < DstSize)? SrcSize : DstSize;

        NICWriteData(Adapter, DstData, SrcData, BytesToCopy);

        (ULONG_PTR)SrcData += BytesToCopy;
        SrcSize            -= BytesToCopy;
        DstData            += BytesToCopy;
        DstSize            -= BytesToCopy;

        if (SrcSize == 0) {
            /* No more bytes in source buffer. Proceed to
               the next buffer in the source buffer chain */
            NdisGetNextBuffer(SrcBuffer, &SrcBuffer);
            if (!SrcBuffer)
                break;

            NdisQueryBuffer(SrcBuffer, (PVOID)&SrcData, &SrcSize);
        }

        if (DstSize == 0) {
            /* Wrap around the end of the transmit buffer ring */
            DstData = TXStart;
            DstSize = Adapter->TXCount * DRIVER_BLOCK_SIZE;
        }
    }
}


BOOLEAN NICPrepareForTransmit(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Prepares a packet for transmission
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * NOTES:
 *     There must be at least one packet in the transmit queue
 * RETURNS:
 *     TRUE if a packet was prepared, FALSE if not
 */
{
    UINT Length;
    UINT BufferCount;
    PNDIS_PACKET Packet;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Calculate number of buffers needed to transmit packet */
    NdisQueryPacket(Adapter->TXQueueHead,
                    NULL,
                    NULL,
                    NULL,
                    &Length);

    BufferCount = (Length + DRIVER_BLOCK_SIZE - 1) / DRIVER_BLOCK_SIZE;

    if (BufferCount > Adapter->TXFree) {
        NDIS_DbgPrint(MID_TRACE, ("No transmit resources. Have (%d) buffers, need (%d).\n",
            Adapter->TXFree, BufferCount));
        /* We don't have the resources to transmit this packet right now */
        return FALSE;
    }

    /* Write the packet to the card */
    NICWritePacket(Adapter);

    /* If the NIC is not transmitting, reset the current transmit pointer */
    if (Adapter->TXCurrent == -1)
        Adapter->TXCurrent = Adapter->TXNext;

    Adapter->TXNext  = (Adapter->TXNext + BufferCount) % Adapter->TXCount;
    Adapter->TXFree -= BufferCount;

    /* Remove the packet from the queue */
    Packet = Adapter->TXQueueHead;
    Adapter->TXQueueHead = RESERVED(Packet)->Next;

    if (Packet == Adapter->TXQueueTail)
        Adapter->TXQueueTail = NULL;

    /* Assume the transmit went well */
    NdisMSendComplete(Adapter->MiniportAdapterHandle,
                      Packet,
                      NDIS_STATUS_SUCCESS);

    return TRUE;
}


VOID NICTransmit(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Starts transmitting packets in the transmit queue
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * NOTES:
 *     There must be at least one packet in the transmit queue
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (Adapter->TXCurrent == -1) {
        /* NIC is not transmitting, so start transmitting now */

        /* Load next packet onto the card, and start transmitting */
        if (NICPrepareForTransmit(Adapter))
            NICStartTransmit(Adapter);
    }
}


VOID HandleReceive(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Handles reception of a packet
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 * NOTES:
 *     Buffer overflows are also handled here
 */
{
    UINT i;
    UCHAR Tmp;
    UINT PacketCount;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    Adapter->DoneIndicating = FALSE;
    PacketCount = 0;

    NICGetCurrentPage(Adapter);

    if (Adapter->BufferOverflow) {

        NDIS_DbgPrint(MAX_TRACE, ("Receive ring overflow.\n"));

        /* Select page 0 and stop the NIC */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STP | CR_RD2 | CR_PAGE0);
        
        /* Clear RBCR0,RBCR1 - Remote Byte Count Registers */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR0, 0x00);
        NdisRawWritePortUchar(Adapter->IOBase + PG0_RBCR1, 0x00);

        /* Wait for ISR_RST to be set, but timeout after 2ms */
        for (i = 0; i < 4; i++) {
            NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &Tmp);
            if (Tmp & ISR_RST)
                break;

            NdisStallExecution(500);
        }

#ifdef DBG
        if (i == 4)
            NDIS_DbgPrint(MIN_TRACE, ("NIC was not reset after 2ms.\n"));
#endif

        if ((Adapter->InterruptStatus & (ISR_PTX | ISR_TXE)) == 0) {
            /* We may need to restart the transmitter */
            Adapter->TransmitPending = TRUE;
        }

        /* Initialize TCR - Transmit Configuration Register to loopback mode 1 */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_TCR, TCR_LOOP);

        /* Start NIC */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_CR, CR_STA | CR_RD2);

        NICStart(Adapter);

        Adapter->BufferOverflow = FALSE;
    }

    if (Adapter->ReceiveError) {
        NDIS_DbgPrint(MAX_TRACE, ("Receive error.\n"));

        /* Skip this packet */
        Adapter->NextPacket = Adapter->CurrentPage;
        NICSetBoundaryPage(Adapter);

        Adapter->ReceiveError = FALSE;
    }

    for (;;) {
        NICGetCurrentPage(Adapter);

        NDIS_DbgPrint(MAX_TRACE, ("Current page (0x%X)  NextPacket (0x%X).\n",
            Adapter->CurrentPage,
            Adapter->NextPacket));

        if (Adapter->CurrentPage == Adapter->NextPacket) {
            NDIS_DbgPrint(MAX_TRACE, ("No more packets.\n"));
            break;
        } else {
            NDIS_DbgPrint(MAX_TRACE, ("Got a packet in the receive ring.\n"));

            /* Read packet from receive buffer ring */
            NICReadPacket(Adapter);

            Adapter->DoneIndicating = TRUE;

            PacketCount++;
            if (PacketCount == 10) {
                /* Don't starve transmit interrupts */
                break;
            }
        }
    }

    if ((Adapter->TransmitPending) && (Adapter->TXCurrent != -1)) {
        NDIS_DbgPrint(MAX_TRACE, ("Retransmitting current packet at (%d).\n", Adapter->TXCurrent));
        /* Retransmit packet */
        NICStartTransmit(Adapter);
        Adapter->TransmitPending = FALSE;
    }

    if (Adapter->DoneIndicating)
        NdisMEthIndicateReceiveComplete(Adapter->MiniportAdapterHandle);
}


VOID HandleTransmit(
    PNIC_ADAPTER Adapter)
/*
 * FUNCTION: Handles transmission of a packet
 * ARGUMENTS:
 *     Adapter = Pointer to adapter information
 */
{
    UINT Length;
    UINT BufferCount;

    if (Adapter->TransmitError) {
        /* FIXME: Retransmit now or let upper layer protocols handle retransmit? */
        Adapter->TransmitError = FALSE;
    }

    /* Free transmit buffers */
    Length      = Adapter->TXSize[Adapter->TXCurrent];
    BufferCount = (Length + DRIVER_BLOCK_SIZE - 1) / DRIVER_BLOCK_SIZE;

    NDIS_DbgPrint(MAX_TRACE, ("Freeing (%d) buffers at (%d).\n",
        BufferCount,
        Adapter->TXCurrent));

    Adapter->TXFree += BufferCount;
    Adapter->TXSize[Adapter->TXCurrent] = 0;
    Adapter->TXCurrent = (Adapter->TXCurrent + BufferCount) % Adapter->TXCount;

    if (Adapter->TXSize[Adapter->TXCurrent] == 0) {
        NDIS_DbgPrint(MAX_TRACE, ("No more packets in transmit buffer.\n"));

        Adapter->TXCurrent = -1;
    }

    if (Adapter->TXQueueTail) {
        if (NICPrepareForTransmit(Adapter))
            NICStartTransmit(Adapter);
    }
}


VOID MiniportHandleInterrupt(
    IN  NDIS_HANDLE MiniportAdapterContext)
/*
 * FUNCTION: Handler for deferred processing of interrupts
 * ARGUMENTS:
 *     MiniportAdapterContext = Pointer to adapter context area
 * NOTES:
 *     Interrupt Service Register is read to determine which interrupts
 *     are pending. All pending interrupts are handled
 */
{
    UCHAR ISRValue;
    UCHAR ISRMask;
    UCHAR Mask;
    PNIC_ADAPTER Adapter = (PNIC_ADAPTER)MiniportAdapterContext;

    ISRMask = Adapter->InterruptMask;
    NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &ISRValue);

    NDIS_DbgPrint(MAX_TRACE, ("ISRValue (0x%X).\n", ISRValue));

    Adapter->InterruptStatus |= (ISRValue & ISRMask);

    if (ISRValue != 0x00)
        /* Acknowledge interrupts */
        NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, ISRValue);

    Mask = 0x01;
    while (Adapter->InterruptStatus != 0x00) {

        NDIS_DbgPrint(MAX_TRACE, ("Adapter->InterruptStatus (0x%X)  Mask (0x%X).\n",
            Adapter->InterruptStatus, Mask));

        /* Find next interrupt type */
        while (((Adapter->InterruptStatus & Mask) == 0) && (Mask < ISRMask))
            Mask = (Mask << 1);

        switch (Adapter->InterruptStatus & Mask) {
        case ISR_OVW:   
            NDIS_DbgPrint(MAX_TRACE, ("Overflow interrupt.\n"));
            /* Overflow. Handled almost the same way as a receive interrupt */
            Adapter->BufferOverflow = TRUE;

            HandleReceive(Adapter);

            Adapter->InterruptStatus &= ~ISR_OVW;
            break;

        case ISR_RXE:
            NDIS_DbgPrint(MAX_TRACE, ("Receive error interrupt.\n"));
            NICUpdateCounters(Adapter);

            Adapter->ReceiveError = TRUE;
            
        case ISR_PRX:
            NDIS_DbgPrint(MAX_TRACE, ("Receive interrupt.\n"));

            HandleReceive(Adapter);

            Adapter->InterruptStatus &= ~(ISR_PRX | ISR_RXE);
            break;  

        case ISR_TXE:
            NDIS_DbgPrint(MAX_TRACE, ("Transmit error interrupt.\n"));
            NICUpdateCounters(Adapter);

            Adapter->TransmitError = TRUE;

        case ISR_PTX:
            NDIS_DbgPrint(MAX_TRACE, ("Transmit interrupt.\n"));

            HandleTransmit(Adapter);

            Adapter->InterruptStatus &= ~(ISR_PTX | ISR_TXE);
            break;

        case ISR_CNT:
            NDIS_DbgPrint(MAX_TRACE, ("Counter interrupt.\n"));
            /* Counter overflow. Read counters from the NIC */
            NICUpdateCounters(Adapter);

            Adapter->InterruptStatus &= ~ISR_CNT;
            break;

        default:
            NDIS_DbgPrint(MAX_TRACE, ("Unknown interrupt. Adapter->InterruptStatus (0x%X).\n", Adapter->InterruptStatus));
            Adapter->InterruptStatus &= ~Mask;
            break;
        }

        Mask = (Mask << 1);

        /* Check if new interrupts are generated */

        NdisRawReadPortUchar(Adapter->IOBase + PG0_ISR, &ISRValue);

        NDIS_DbgPrint(MAX_TRACE, ("ISRValue (0x%X).\n", ISRValue));

        Adapter->InterruptStatus |= (ISRValue & ISRMask);

        if (ISRValue != 0x00) {
            /* Acknowledge interrupts */
            NdisRawWritePortUchar(Adapter->IOBase + PG0_ISR, ISRValue);
            Mask = 0x01;
        }
    }

    NICEnableInterrupts((PNIC_ADAPTER)MiniportAdapterContext);
}

/* EOF */
