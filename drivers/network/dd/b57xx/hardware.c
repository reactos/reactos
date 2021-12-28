/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific functions
 * COPYRIGHT:   Copyright 2021-2022 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#define NDEBUG
#include <debug.h>

static const struct _B57XX_DEVICE
{
    USHORT DeviceID1;
    USHORT DeviceID2;
    USHORT DeviceID3;
    USHORT SubsystemID;
} Devices[] = 
{
    [B5700]  = { 0x1644 },
    [B5701]  = { 0x1645 },
    [B5702]  = { 0x16A6, 0x1646, 0x16C6 },
    [B5703S] = { 0x16C7, 0x1647, 0x16A7, 0x000A },
    [B5703C] = { 0x16C7, 0x1647, 0x16A7 },
    [B5704C] = { 0x1648 },
    [B5704S] = { 0x16A8, 0x1649 },
    [B5705]  = { 0x1653, 0x1654 },
    [B5705M] = { 0x165D, 0x165E },
    [B5788]  = { 0x169C },
    [B5721]  = { 0x1659 },
    [B5751]  = { 0x1677 },
    [B5751M] = { 0x167D },
    [B5752]  = { 0x1600 },
    [B5752M] = { 0x1601 },
    [B5714C] = { 0x1668 },
    [B5714S] = { 0x1669 },
    [B5715C] = { 0x1678 },
    [B5715S] = { 0x1679 },
    [B5722] =  { 0x165A },
    [B5755] =  { 0x167B },
    [B5755M] = { 0x1673 },
    [B5754] =  { 0x167A },
    [B5754M] = { 0x1672 },
    [B5756M] = { 0x1674 },
    [B5757] =  { 0x1670 },
    [B5786] =  { 0x169A },
    [B5787] =  { 0x169B },
    [B5787M] = { 0x1693 },
    [B5784M] = { 0x1698 },
    [B5764M] = { 0x1684 },
};

/* NIC FUNCTIONS **********************************************************************************/

BOOLEAN
NTAPI
NICIsDevice(_In_ PB57XX_ADAPTER Adapter,
            _In_ ULONG DeviceIDListLength,
            _In_reads_(DeviceIDListLength) const B57XX_DEVICE_ID DeviceIDList[])
{
    for (UINT i = 0; i < DeviceIDListLength; i++)
    {
        if (DeviceIDList[i] == Adapter->DeviceID)
        {
            return TRUE;
        }
    }
    return FALSE;
}


_Check_return_
NDIS_STATUS
NTAPI
NICConfigureAdapter(_In_ PB57XX_ADAPTER Adapter)
{
    UINT i;
    ULONG AdditionalFrameSize = sizeof(ETH_HEADER) + 4;
    ULONG Alignment = Adapter->PciState.CacheLineSize;
    
    ASSERT(Adapter->PciState.DeviceID != 0);
    
    for (i = 0; i < ARRAYSIZE(Devices); i++)
    {
        if ((Devices[i].DeviceID1 == Adapter->PciState.DeviceID ||
            Devices[i].DeviceID2 == Adapter->PciState.DeviceID ||
            Devices[i].DeviceID3 == Adapter->PciState.DeviceID) &&
            (Devices[i].SubsystemID == 0 ||
            Devices[i].SubsystemID == Adapter->PciState.SubSystemID))
        {
            Adapter->DeviceID = i;
            break;
        }
    }
    if (i == ARRAYSIZE(Devices))
    {
        NDIS_MinDbgPrint("Could not identify device\n");
        return NDIS_STATUS_FAILURE;
    }
    
    Adapter->B5705Plus = !NICISDEVICE(Adapter, B5700, B5701, B5702, B5703C, B5703S, B5704C, B5704S);
    Adapter->B5721Plus = Adapter->B5705Plus && !NICISDEVICE(Adapter, B5705, B5705M, B5788);
    Adapter->IsSerDes = NICISDEVICE(Adapter, B5703S, B5704S, B5714S, B5715S);
    
    if (Alignment % 8 != 0)
    {
        Alignment += 8 - (Alignment % 8);
    }
    
    /* Standard receive producer ring */
    Adapter->MaxFrameSize = PAYLOAD_SIZE_STANDARD + AdditionalFrameSize;
    
    Adapter->StandardProducer.FrameBufferLength = Adapter->MaxFrameSize;
    if (Adapter->StandardProducer.FrameBufferLength % Alignment != 0)
    {
        Adapter->StandardProducer.FrameBufferLength += Alignment - (
                                                       Adapter->StandardProducer.FrameBufferLength %
                                                       Alignment);
    }
    Adapter->StandardProducer.Count = NICISDEVICE(Adapter, B5705) ? 512 : NUM_RX_BUFFER_DESCRIPTORS;
    Adapter->StandardProducer.Index = Adapter->StandardProducer.Count - 1;
    
    /* Receive consumer rings */
    Adapter->ReturnConsumer[0].Count = NICISDEVICE(Adapter, B5705) ?
                                       512 : NUM_RX_RETURN_DESCRIPTORS;
    
    /* Send producer rings */
    Adapter->SendProducer[0].Count = NUM_TX_BUFFER_DESCRIPTORS;
    
    return NDIS_STATUS_SUCCESS;
}

_Check_return_
NDIS_STATUS
NTAPI
NICPowerOn(_In_ PB57XX_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    ULONG Value;
    UINT n;
    
    Adapter->HardwareStatus = NdisHardwareStatusNotReady;

    /* Initialization Procedure step 1: Enable MAC memory space decode and bus mastering */
    B57XXEnableUShort(Adapter,
                      B57XX_REG_COMMAND,
                      &Value,
                      B57XX_COMMAND_MEMSPACE | B57XX_COMMAND_BUSMASTER);

    /* Initialization Procedure step 2: Disable interrupts */
    B57XXEnableULong(Adapter, B57XX_REG_MISC_HOST_CTRL, &Value, B57XX_MISC_HOST_CTRL_CINTA);
    NICDisableInterrupts(Adapter);

    /* Initialization Procedure step 3 should be done already (Save select PCI registers) */

    /* Initialization Procedure step 4: Acquire the NVRAM lock */
    if (!NICISDEVICE(Adapter, B5700, B5701))
    {
        B57XXEnableULong(Adapter, B57XX_REG_SOFT_ARB, &Value, B57XX_SOFT_ARB_REQSET1);
        for (n = 0; n < MAX_ATTEMPTS; n++)
        {
            NdisStallExecution(1);
            B57XXReadULong(Adapter, B57XX_REG_SOFT_ARB, &Value);
            if (Value & B57XX_SOFT_ARB_ARBWON1)
            {
                break;
            }
        }
        if (n == MAX_ATTEMPTS)
        {
            NDIS_MinDbgPrint("Could not aquire device arbitration register\n");
            return NDIS_STATUS_DEVICE_FAILED;
        }
    }
    
    /* Initialization Procedure step 5: Prepare the chip for writing T3_MAGIC_NUMBER */
    B57XXEnableULong(Adapter, B57XX_REG_MEM_ARB_MODE, &Value, B57XX_MEM_ARB_MODE_ENABLE);

    B57XXEnableULong(Adapter,
                     B57XX_REG_MISC_HOST_CTRL,
                     &Value,
                     B57XX_MISC_HOST_CTRL_INDIRECTACCESS |
                     B57XX_MISC_HOST_CTRL_CLOCKCTRL |
                     B57XX_ENDIAN_SWAP(B57XX_ENDIAN_BYTESWAP,
                                       B57XX_MISC_HOST_CTRL_ENDIANBYTESWAP,
                                       B57XX_ENDIAN_WORDSWAP,
                                       B57XX_MISC_HOST_CTRL_ENDIANWORDSWAP));
    B57XXEnableULong(Adapter,
                     B57XX_REG_MODE_CTRL,
                     &Value,
                     B57XX_ENDIAN_SWAP(B57XX_ENDIAN_BYTESWAP_NFDATA,
                                       B57XX_MODE_CTRL_BYTESWAPNF,
                                       B57XX_ENDIAN_WORDSWAP_NFDATA,
                                       B57XX_MODE_CTRL_WORDSWAPNF));
    
    /* Initialization Procedure steps 6+ */
    B57XXWriteMemoryULong(Adapter, B57XX_ADDR_FW_DRV_STATE_MAILBOX, B57XX_CONST_DRV_STATE_START);
    
    Status = NICSoftReset(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return Status;
    }

    
    return NDIS_STATUS_SUCCESS;
}

_Check_return_
NDIS_STATUS
NTAPI
NICSoftReset(_In_ PB57XX_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    ULONG Value;
    B57XX_RING_CONTROL_BLOCK RCBValue;
    UINT n;
    
    Adapter->HardwareStatus = NdisHardwareStatusReset;

    /* Initialization Procedure step 6: Prepare firmware for reset */
    B57XXWriteMemoryULong(Adapter, B57XX_ADDR_FW_MAILBOX, B57XX_CONST_T3_MAGIC_NUMBER);
    
    if (NICISDEVICE(Adapter, B5752))
    {
        B57XXWriteULong(Adapter, B57XX_REG_FASTBOOT_COUNTER, 0);
    }

    /* Initialization Procedure step 7: Reset the core clocks */
    if (NICISDEVICE(Adapter, B5751, B5751M, B5721, B5752, B5752M))
    {
        B57XXEnableULong(Adapter,
                         B57XX_REG_PCI_CLOCK,
                         &Value,
                         B57XX_PCI_CLOCK_ALTCLOCKFINAL | B57XX_PCI_CLOCK_ALTCLOCK);
    }
    
    B57XXReadULong(Adapter, B57XX_REG_MISC_CONFIG, &Value);
    Value |= B57XX_MISC_CONFIG_CLCKRESET;
    if (Adapter->B5705Plus)
    {
        Value |= B57XX_MISC_CONFIG_POVERRIDE;
    }
    if (Adapter->B5721Plus)
    {
        Value |= B57XX_MISC_CONFIG_DGRPCIEB;
    }
    B57XXWriteULong(Adapter, B57XX_REG_MISC_CONFIG, Value);

    /* Initialization Procedure step 8: Wait for core-clock reset to complete. */
    /* We can't poll the core-clock reset bit to de-assert,
       since the local memory interface is disabled by the reset. */
    NdisStallExecution(90000);
    B57XXReadPciConfigULong(Adapter, B57XX_REG_COMMAND, &Value);
    NdisStallExecution(10000);
    
    /* Initialization Procedure step 9: Disable interrupts */
    NICDisableInterrupts(Adapter);

    /* Initialization Procedure step 10: Enable MAC memory space decode and bus mastering */ 
    NdisWriteRegisterUshort((PUSHORT)(Adapter->IoBase + B57XX_REG_COMMAND),
                            Adapter->PciState.Command |
                            B57XX_COMMAND_MEMSPACE |
                            B57XX_COMMAND_BUSMASTER);
    
    /* Initialization Procedure step 11: Disable PCI-X Relaxed Ordering */
    NdisReadRegisterUshort((PUSHORT)(Adapter->IoBase + B57XX_REG_PCIX_COMMAND), &Value);
    Value &= ~B57XX_PCIX_COMMAND_RELAXEDORDER;
    NdisWriteRegisterUshort((PUSHORT)(Adapter->IoBase + B57XX_REG_PCIX_COMMAND), Value);
    
    /* Initialization Procedure step 12: Enable the MAC memory arbiter */
    B57XXEnableULong(Adapter, B57XX_REG_MEM_ARB_MODE, &Value, B57XX_MEM_ARB_MODE_ENABLE);
    
    /* Initialization Procedure step 13: Enable External Memory */
    if (NICISDEVICE(Adapter, B5700))
    {
        // TODO B5700 External memory config
        // If you have one of these, please let me know
    }
    
    /* Initialization Procedure step 14: Initialize the Miscellaneous Host Control register */
    B57XXEnableULong(Adapter,
                     B57XX_REG_MISC_HOST_CTRL,
                     &Value,
                     B57XX_MISC_HOST_CTRL_INDIRECTACCESS |
                     B57XX_MISC_HOST_CTRL_PCISTATE |
                     B57XX_MISC_HOST_CTRL_CLOCKCTRL |
                     B57XX_ENDIAN_SWAP(B57XX_ENDIAN_BYTESWAP,
                                       B57XX_MISC_HOST_CTRL_ENDIANBYTESWAP,
                                       B57XX_ENDIAN_WORDSWAP,
                                       B57XX_MISC_HOST_CTRL_ENDIANWORDSWAP));
    
    /* Initialization Procedure step 15-16: Initialize the General Mode Control register */
    B57XXEnableULong(Adapter,
                     B57XX_REG_MODE_CTRL,
                     &Value,
                     B57XX_ENDIAN_SWAP(B57XX_ENDIAN_BYTESWAP_DATA,
                                       B57XX_MODE_CTRL_BYTESWAPDATA,
                                       B57XX_ENDIAN_WORDSWAP_DATA,
                                       B57XX_MODE_CTRL_WORDSWAPDATA) |
                     B57XX_ENDIAN_SWAP(B57XX_ENDIAN_BYTESWAP_NFDATA,
                                       B57XX_MODE_CTRL_BYTESWAPNF,
                                       B57XX_ENDIAN_WORDSWAP_NFDATA,
                                       B57XX_MODE_CTRL_WORDSWAPNF));
    
    /* Initialization Procedure step 17: Poll for bootcode completion */
    for (n = 0; n < MAX_ATTEMPTS; n++)
    {
        B57XXReadMemoryULong(Adapter, B57XX_ADDR_FW_MAILBOX, &Value);
        if (Value == ~B57XX_CONST_T3_MAGIC_NUMBER)
        {
            break;
        }
        NdisStallExecution(100);
    }
    if (n == MAX_ATTEMPTS)
    {
        NDIS_MinDbgPrint("Device did not recover reset (Firmware mailbox: 0x%x)\n", Value);
        return NDIS_STATUS_FAILURE;
    }
    Adapter->HardwareStatus = NdisHardwareStatusInitializing;
    NDIS_MinDbgPrint("Device is back from reset (%u)\n", n);
    
    /* Initialization Procedure step 18: Initialize the Ethernet MAC Mode register */
    if (Adapter->IsSerDes)
    {
        B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_MODE, B57XX_ETH_MAC_MODE_PORT_TBI);
    }
    else
    {
        B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_MODE, B57XX_ETH_MAC_MODE_PORT_GMI);
    }
    
    /* Initialization Procedure step 19-20: Enable the PCIe bug fix & Enable data FIFO protection */
    if (Adapter->B5721Plus)
    {
        B57XXEnableRegister(Adapter,
                            B57XX_REG_TLP_CTRL,
                            &Value,
                            B57XX_TLP_CTRL_FIFOPROTECT | B57XX_TLP_CTRL_INTMODEFIX);
    }
    
    /* Initialization Procedure step 21: Enable the hardware fixes */
    if (Adapter->B5705Plus)
    {
        B57XXEnableRegister(Adapter,
                            B57XX_REG_HW_FIX,
                            &Value,
                            B57XX_HW_FIX_HWFIX1 | B57XX_HW_FIX_HWFIX3 | B57XX_HW_FIX_HWFIX4);
    }
    
    /* Initialization Procedure step 22: Enable Tagged Status Mode */
    B57XXEnableRegister(Adapter,
                       B57XX_REG_MISC_HOST_CTRL,
                       &Value,
                       B57XX_MISC_HOST_CTRL_TAGGEDSTATUS);
    
    /* Initialization Procedure step 23: Restore select PCI registers */
    NdisWriteRegisterUchar((PUCHAR)(Adapter->IoBase + B57XX_REG_CACHE_LINE_SIZE),
                           Adapter->PciState.CacheLineSize);
    NdisWriteRegisterUshort((PUSHORT)(Adapter->IoBase + B57XX_REG_SUBSY_VENDORID),
                            Adapter->PciState.SubVendorID);
    
    /* Initialization Procedure steps 24-25: Clear the MAC/driver statistics (not applicable) */
    
    /* Initialization Procedure steps 26: Clear the MAC/driver status (done earlier) */
    
    /* Initialization Procedure step 27: Set the default PCI Command Encoding R/W Transactions */
    B57XXReadRegister(Adapter, B57XX_REG_PCI_STATE, &Value);
    if (NICISDEVICE(Adapter, B5700, B5701))
    {
        Value = (Value & B57XX_PCI_STATE_BUSMODE) ? 0x761B000F : 0x763F000F;
    }
    else if (NICISDEVICE(Adapter, B5702))
    {
        Value = 0x763F000F;
    }
    else if (NICISDEVICE(Adapter, B5703C, B5703S, B5704C, B5704S))
    {
        if (Value & B57XX_PCI_STATE_BUSMODE)
        {
            Value = (Value & B57XX_PCI_STATE_HIGHSPEED) ? 0x769F4000 : 0x769F0000;
        }
        Value = 0x763F0000;
    }
    else if (NICISDEVICE(Adapter, B5705, B5705M, B5788))
    {
        Value = 0x763F0000;
    }
    else if (NICISDEVICE(Adapter, B5714C, B5714S, B5715C, B5715S))
    {
        Value = 0x76144000;
    }
    else
    {
        NdisReadRegisterUshort((PUSHORT)(Adapter->IoBase + B57XX_REG_DEVICE_CTRL), &Value);
        Value = (Value & (0b111 << 5)) ? 0x76380000 : 0x76180000;
    }
    B57XXWriteRegister(Adapter, B57XX_REG_DMA_CTRL, Value);
    
    /* Initialization Procedure step 28 done in step 15-16 (Set DMA byte swapping) */
    
    /* Initialization Procedure step 29-31: Host-based send rings, Ready RX traffic, Checksums */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_MODE_CTRL,
                        &Value,
                        B57XX_MODE_CTRL_INTMACATTN |
                        B57XX_MODE_CTRL_HOSTSENDBDS |
                        B57XX_MODE_CTRL_HOSTSTACKUP |
                        B57XX_MODE_CTRL_NOTXPSHCS);
    
    /* Initialization Procedure step 32: Configure timer */
    B57XXReadRegister(Adapter, B57XX_REG_MISC_CONFIG, &Value);
    Value &= ~(0x7F << 1);
    Value |= (65 << 1); // Increment every 1 uS
    B57XXWriteRegister(Adapter, B57XX_REG_MISC_CONFIG, Value);
    
    /* Initialization Procedure step 33: MAC local memory pool */
    if (NICISDEVICE(Adapter, B5700))
    {
        B57XXEnableRegister(Adapter,
                            B57XX_REG_MISC_LOCAL_CTRL,
                            &Value,
                            B57XX_MISC_LOCAL_CTRL_EXTMEM);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_BASE, 0x20000);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_LENGTH, 0); // FIXME for B5700
    }
    if (NICISDEVICE(Adapter, B5701, B5702, B5703C, B5703S))
    {
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_BASE, 0x8000);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_LENGTH, 0x18000);
    }
    else if (NICISDEVICE(Adapter, B5704C, B5704S))
    {
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_BASE, 0x10000);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_LENGTH, 0x10000);
    }
    
    /* Initialization Procedure step 34: MAC DMA resource pool */
    if (!Adapter->B5705Plus)
    {
        B57XXWriteRegister(Adapter, B57XX_REG_DMA_DESC_POOL_BASE, 0x2000);
        B57XXWriteRegister(Adapter, B57XX_REG_DMA_DESC_POOL_LENGTH, 0x2000);
    }
    
    /* Initialization Procedure step 35: MAC memory pool watermarks (Standard frames) */
    if (!Adapter->B5705Plus)
    {
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_DMA_LO_WM, 0x50);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_RX_LO_WM, 0x20);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_HI_WM, 0x60);
    }
    else
    {
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_DMA_LO_WM, 0x00);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_RX_LO_WM, 0x10);
        B57XXWriteRegister(Adapter, B57XX_REG_MBUF_POOL_HI_WM, 0x60);
    }
    
    /* Initialization Procedure step 36: MAC DMA pool watermarks */
    B57XXWriteRegister(Adapter, B57XX_REG_DMA_DESC_POOL_LO_WM, 5);
    B57XXWriteRegister(Adapter, B57XX_REG_DMA_DESC_POOL_HI_WM, 10);
    
    /* Initialization Procedure step 37: Configure flow control behavior */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_RX_LOW_WM_MAX, 2);
    
    /* Initialization Procedure step 38-39: Enable the buffer manager */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_BUF_MAN_MODE,
                        &Value,
                        B57XX_BUF_MAN_MODE_ENABLE | B57XX_BUF_MAN_MODE_ATTN);
    
    for (n = 0; n < MAX_ATTEMPTS; n++)
    {
        NdisStallExecution(10);
        B57XXReadRegister(Adapter, B57XX_REG_BUF_MAN_MODE, &Value);
        if (Value & B57XX_BUF_MAN_MODE_ENABLE)
        {
            break;
        }
    }
    if (n == MAX_ATTEMPTS)
    {
        NDIS_MinDbgPrint("Unable to start NIC buffer manager\n");
        return NDIS_STATUS_FAILURE;
    }
    
    /* Initialization Procedure step 40: Enable internal hardware queues */
    B57XXWriteRegister(Adapter, B57XX_REG_FTQ_RESET, 0xFFFFFFFF);
    B57XXWriteRegister(Adapter, B57XX_REG_FTQ_RESET, 0x00000000);
    
    /* Initialization Procedure step 41: Standard Receive Buffer Ring (Internal memory) */
    Adapter->StandardProducer.RingControlBlock.MaxLength = !Adapter->B5705Plus ? 
                                                           Adapter->MaxFrameSize :
                                                           Adapter->StandardProducer.Count;
    Adapter->StandardProducer.RingControlBlock.NICRingAddress = B57XX_ADDR_RECEIVE_STD_RINGS;
    
    B57XXWriteRegister(Adapter,
                       B57XX_REG_STD_RXP_RING_HOST_HI,
                       Adapter->StandardProducer.RingControlBlock.HostRingAddress.HighPart);
    B57XXWriteRegister(Adapter,
                       B57XX_REG_STD_RXP_RING_HOST_LO,
                       Adapter->StandardProducer.RingControlBlock.HostRingAddress.LowPart);
    B57XXWriteRegister(Adapter,
                       B57XX_REG_STD_RXP_RING_ATTR,
                       Adapter->StandardProducer.RingControlBlock.MaxLength << 16);
    B57XXWriteRegister(Adapter,
                       B57XX_REG_STD_RXP_RING_NIC,
                       Adapter->StandardProducer.RingControlBlock.NICRingAddress);
    B57XXWriteMailbox(Adapter, B57XX_MBOX_STD_RXP_RING_INDEX, 0);
    
    /* Initialization Procedure step 42: Jumbo Receive Buffer Ring (disable) */
    if (!Adapter->B5705Plus)
    {
        Adapter->JumboProducer.RingControlBlock.Flags = B57XX_RCB_FLAG_RING_DISABLED;
        Adapter->JumboProducer.RingControlBlock.NICRingAddress = B57XX_ADDR_RECEIVE_JUMBO_RINGS;
        
        B57XXWriteRegister(Adapter,
                           B57XX_REG_JUMB_RXP_RING_ATTR,
                           Adapter->JumboProducer.RingControlBlock.Flags);
        B57XXWriteRegister(Adapter,
                           B57XX_REG_JUMB_RXP_RING_NIC,
                           Adapter->JumboProducer.RingControlBlock.NICRingAddress);
        B57XXWriteMailbox(Adapter, B57XX_MBOX_JUMBO_RXP_RING_INDEX, 0);
    }
    
    /* Initialization Procedure step 43: Mini Receive Buffer Ring (disable) */
    if (NICISDEVICE(Adapter, B5700))
    {
        Adapter->MiniProducer.RingControlBlock.Flags = B57XX_RCB_FLAG_RING_DISABLED;
        Adapter->MiniProducer.RingControlBlock.NICRingAddress = B57XX_ADDR_RECEIVE_MINI_RINGS;
        
        B57XXWriteRegister(Adapter,
                           B57XX_REG_MINI_RXP_RING_ATTR,
                           Adapter->MiniProducer.RingControlBlock.Flags);
        B57XXWriteRegister(Adapter,
                           B57XX_REG_MINI_RXP_RING_NIC,
                           Adapter->MiniProducer.RingControlBlock.NICRingAddress);
        B57XXWriteMailbox(Adapter, B57XX_MBOX_MINI_RXP_RING_INDEX, 0);
    }
    
    /* Initialization Procedure step 44: Set the BD Ring replenish thresholds for RX Rings */
    B57XXWriteRegister(Adapter, B57XX_REG_STD_RX_RING_REPL, 25);
    
    /* Initialization Procedure step 45-47: Initialize send rings & Disable unused send rings */
    Adapter->SendProducer[0].RingControlBlock.MaxLength = Adapter->SendProducer[0].Count;
    Adapter->SendProducer[0].RingControlBlock.NICRingAddress = B57XX_ADDR_SEND_RINGS;
    B57XXWriteMemoryRCB(Adapter, B57XX_ADDR_SEND_RCBS, &Adapter->SendProducer[0].RingControlBlock);
    B57XXWriteMailbox(Adapter, B57XX_MBOX_TXP_HOST_RING_INDEX1, 0);
    B57XXWriteMailbox(Adapter, B57XX_MBOX_TXP_NIC_RING_INDEX1, 0);
    
    if (!Adapter->B5705Plus)
    {
        NdisZeroMemory(&RCBValue, sizeof(B57XX_RING_CONTROL_BLOCK));
        RCBValue.Flags = B57XX_RCB_FLAG_RING_DISABLED;
        for (n = 1; n < 4; n++)
        {
            B57XXWriteMemoryRCB(Adapter,
                                B57XX_ADDR_SEND_RCBS + n * sizeof(B57XX_RING_CONTROL_BLOCK),
                                &RCBValue);
            B57XXWriteMailbox(Adapter, B57XX_MBOX_TXP_HOST_RING_INDEX1 + n * 8, 0);
        }
    }

    /* Initialization Procedure step 48-50: Initialize Receive Return Rings & Disable unused ones */
    Adapter->ReturnConsumer[0].RingControlBlock.MaxLength = Adapter->ReturnConsumer[0].Count;
    Adapter->ReturnConsumer[0].RingControlBlock.NICRingAddress = 0;
    B57XXWriteMemoryRCB(Adapter,
                        B57XX_ADDR_RECEIVE_RETURN_RCBS,
                        &Adapter->ReturnConsumer[0].RingControlBlock);
    B57XXWriteMailbox(Adapter, B57XX_MBOX_RXC_RET_RING_INDEX1, 0);
    
    if (!Adapter->B5705Plus)
    {
        NdisZeroMemory(&RCBValue, sizeof(B57XX_RING_CONTROL_BLOCK));
        RCBValue.Flags = B57XX_RCB_FLAG_RING_DISABLED;
        for (n = 1; n < 16; n++)
        {
            B57XXWriteMemoryRCB(Adapter,
                                B57XX_ADDR_RECEIVE_RETURN_RCBS +
                                n * sizeof(B57XX_RING_CONTROL_BLOCK),
                                &RCBValue);
            B57XXWriteMailbox(Adapter, B57XX_MBOX_RXC_RET_RING_INDEX1 + n * 8, 0);
        }
    }
    
    /* Initialization Procedure step 51: Receive Producer Ring mailbox */
    B57XXWriteMailbox(Adapter, B57XX_MBOX_STD_RXP_RING_INDEX, Adapter->StandardProducer.Index);
    
    /* Initialization Procedure step 52: Configure the MAC unicast address */
    NICUpdateMACAddresses(Adapter);
    
    /* Initialization Procedure step 53: Configure random backoff seed for transmit */
    Value = (Adapter->MACAddresses[0][0] + 
             Adapter->MACAddresses[0][1] +
             Adapter->MACAddresses[0][2] +
             Adapter->MACAddresses[0][3] +
             Adapter->MACAddresses[0][4] +
             Adapter->MACAddresses[0][5]) & 0x3FF;
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_TX_RND_BACKOFF, Value);
    
    /* Initialization Procedure step 54: Configure the Message Transfer Unit MTU size */
    // We need to include VLAN tags for the MTU
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_RX_MTU_SIZE, Adapter->MaxFrameSize + 4);
    
    /* Initialization Procedure step 55: Configure IPG for transmit */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_TX_LENGTH, 0x2620);
    
    /* Initialization Procedure step 56: Configure default RX return ring for non-matched packets */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_RX_RULES_CONFIG, (1 << 3));
    
    /* Initialization Procedure step 57: Configure the number of Receive Lists */
    B57XXWriteRegister(Adapter, B57XX_REG_RX_LIST_PLACE_CONFIG, 0x181);
    
    /* Initialization Procedure step 58: Receive List Placement Statistics mask */
    B57XXWriteRegister(Adapter, B57XX_REG_RX_LIST_STAT_MASK, 0xFFFFFF);
    
    /* Initialization Procedure step 59: Enable RX statistics */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_LIST_STAT_CTRL,
                        &Value,
                        B57XX_STAT_CTRL_ENABLE);
    
    /* Initialization Procedure step 60: Enable the Send Data Initiator mask */
    B57XXWriteRegister(Adapter, B57XX_REG_TX_DATA_STAT_MASK, 0xFFFFFF);
    
    /* Initialization Procedure step 61: Enable TX statistics */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_TX_DATA_STAT_CTRL,
                        &Value,
                        B57XX_STAT_CTRL_ENABLE | B57XX_STAT_CTRL_FASTSTAT);
    
    /* Initialization Procedure step 62: Disable the host coalescing eng */
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_HOST_MODE, 0x0000);
    
    /* Initialization Procedure step 63: Poll 20 ms for the host coalescing engine to stop */
    for (n = 0; n < MAX_ATTEMPTS; n++)
    {
        NdisStallExecution(20);
        B57XXReadRegister(Adapter, B57XX_REG_COAL_HOST_MODE, &Value);
        if (Value == 0x0000)
        {
            break;
        }
    }
    if (n == MAX_ATTEMPTS)
    {
        NDIS_MinDbgPrint("Unable to start NIC buffer manager\n");
        return NDIS_STATUS_FAILURE;
    }
    
    /* Initialization Procedure step 64: Configure the host coalescing tick count */
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_RX_TICKS, 300);
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_TX_TICKS, 150);
    
    /* Initialization Procedure step 65: Configure the host coalescing BD count */
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_RX_MAX_BD, 20);
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_TX_MAX_BD, 10);
    
    /* Initialization Procedure step 66: Configure during interrupt tick counter */
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_RX_INT_TICKS, 0);
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_TX_INT_TICKS, 0);
    
    /* Initialization Procedure step 67: Configure the max-coalesced frames during int. counter */
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_RX_MAX_INT_BD, 0);
    B57XXWriteRegister(Adapter, B57XX_REG_COAL_TX_MAX_INT_BD, 0);
    
    /* Initialization Procedure step 68: Initialize host status block address */
    B57XXWriteRegister(Adapter,
                       B57XX_REG_STATUS_BLCK_HOST_ADDR,
                       Adapter->Status.HostAddress.HighPart);
    B57XXWriteRegister(Adapter,
                       B57XX_REG_STATUS_BLCK_HOST_ADDR + 4, 
                       Adapter->Status.HostAddress.LowPart);
    
    /* Initialization Procedure step 69-71: Initialize statistics block */
    if (!Adapter->B5705Plus)
    {
        B57XXWriteRegister(Adapter,
                           B57XX_REG_STAT_HOST_ADDR,
                           Adapter->Statistics.HostAddress.HighPart);
        B57XXWriteRegister(Adapter,
                           B57XX_REG_STAT_HOST_ADDR + 4, 
                           Adapter->Statistics.HostAddress.LowPart);
        B57XXWriteRegister(Adapter, B57XX_REG_STAT_TICKS, 1000000);
        B57XXWriteRegister(Adapter, B57XX_REG_STAT_BASE_ADDR, B57XX_ADDR_STAT_BLCK);
    }
    
    /* Initialization Procedure step 72: Configure the status block address in NIC local memory */
    if (!Adapter->B5705Plus)
    {
        B57XXWriteRegister(Adapter, B57XX_REG_STATUS_BLCK_BASE_ADDR, B57XX_ADDR_STATUS_BLCK);
    }
    
    /* Initialization Procedure step 73: Enable the host coalescing engine */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_COAL_HOST_MODE,
                        &Value,
                        B57XX_COAL_HOST_MODE_ENABLE |
                        B57XX_COAL_HOST_MODE_ATTN);
    
    /* Initialization Procedure step 74: Enable the receive BD completion functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_BD_COMPL_MODE,
                        &Value,
                        B57XX_BD_MODE_ENABLE | B57XX_BD_MODE_ATTN);
    
    /* Initialization Procedure step 75: Enable the receive list placement functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_LIST_PLACE_MODE,
                        &Value,
                        B57XX_RX_LIST_PLACE_MODE_ENABLE);
    
    /* Initialization Procedure step 76: Enable the receive list selector functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_LIST_SELECT_MODE,
                        &Value,
                        B57XX_RX_LIST_SELECT_MODE_ENABLE | B57XX_RX_LIST_SELECT_MODE_ATTN);
    
    /* Initialization Procedure step 77-78: Enable DMA engines & Enable and clear statistics */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_ETH_MAC_MODE,
                        &Value,
                        B57XX_ETH_MAC_MODE_TXDMA |
                        B57XX_ETH_MAC_MODE_RXDMA |
                        B57XX_ETH_MAC_MODE_RXFHDDMA |
                        B57XX_ETH_MAC_MODE_RXSTATCLEAR | 
                        B57XX_ETH_MAC_MODE_RXSTATENABLE |
                        B57XX_ETH_MAC_MODE_TXSTATCLEAR | 
                        B57XX_ETH_MAC_MODE_TXSTATENABLE);
    NdisStallExecution(40);
    
    /* Initialization Procedure step 79: Configure General Miscellaneous Local Control */
    Value = B57XX_MISC_LOCAL_CTRL_INTATTN | B57XX_MISC_LOCAL_CTRL_SEEPROM;
    if (NICISDEVICE(Adapter, B5752))
    {
        Value |= B57XX_MISC_LOCAL_CTRL_GPIOO3;
    }
    B57XXWriteRegister(Adapter, B57XX_REG_MISC_LOCAL_CTRL, Value);
    NdisStallExecution(100);
    
    /* Initialization Procedure step 80: Write a value of zero to the Interrupt Mailbox 0 */
    B57XXWriteMailbox(Adapter, B57XX_MBOX_INT_MAILBOX0, 0);
    
    /* Initialization Procedure step 81: Enable DMA completion functional block */
    if (!Adapter->B5705Plus)
    {
        B57XXEnableRegister(Adapter, B57XX_REG_DMA_COMPLETION, &Value, B57XX_DMA_COMPLETION_ENABLE);
    }
    
    /* Initialization Procedure step 82: Configure the Write DMA Mode register */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_DMA_WRITE_MODE, 
                        &Value,
                        B57XX_DMA_MODE_ENABLE |
                        B57XX_DMA_MODE_TARGETABORT |
                        B57XX_DMA_MODE_MASTERABORT |
                        B57XX_DMA_MODE_PARITYERROR |
                        B57XX_DMA_MODE_HOSTADDROF |
                        B57XX_DMA_MODE_FIFOOR |
                        B57XX_DMA_MODE_FIFOUR |
                        B57XX_DMA_MODE_FIFOOVER |
                        B57XX_DMA_MODE_OVERLENGTH |
                        B57XX_DMA_MODE_STATUSTAGFIX);
    NdisStallExecution(40);
    
    /* Initialization Procedure step 83: Configure the Read DMA Mode register */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_DMA_READ_MODE, 
                        &Value,
                        B57XX_DMA_MODE_ENABLE |
                        B57XX_DMA_MODE_TARGETABORT |
                        B57XX_DMA_MODE_MASTERABORT |
                        B57XX_DMA_MODE_PARITYERROR |
                        B57XX_DMA_MODE_HOSTADDROF |
                        B57XX_DMA_MODE_FIFOOR |
                        B57XX_DMA_MODE_FIFOUR |
                        B57XX_DMA_MODE_FIFOOVER |
                        B57XX_DMA_MODE_OVERLENGTH);
    NdisStallExecution(40);
    
    /* Initialization Procedure step 84: Enable the receive data completion functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_DATA_COMPL_MODE,
                        &Value,
                        B57XX_RX_DATA_COMPL_MODE_ENABLE | B57XX_RX_DATA_COMPL_MODE_ATTN);
    
    /* Initialization Procedure step 85: Enable the Mbuf cluster free functional block */
    if (!Adapter->B5705Plus)
    {
        B57XXEnableRegister(Adapter,
                            B57XX_REG_MBUF_CLUSTER_FREE,
                            &Value,
                            B57XX_MBUF_CLUSTER_FREE_ENABLE);
    }
    
    /* Initialization Procedure step 86: Enable the send data completion functional block */
    B57XXEnableRegister(Adapter, B57XX_REG_TX_COMPL_MODE, &Value, B57XX_TX_COMPL_MODE_ENABLE);
    
    /* Initialization Procedure step 87: Enable the send BD completion functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_TX_BD_COMPL_MODE,
                        &Value,
                        B57XX_BD_MODE_ENABLE | B57XX_BD_MODE_ATTN);
    
    /* Initialization Procedure step 88: Enable the Receive BD Initiator Functional Block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_BD_MODE,
                        &Value,
                        B57XX_BD_MODE_ENABLE | B57XX_BD_MODE_ATTN);
    
    /* Initialization Procedure step 89: Enable the receive data & BD initiator functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_RX_DATA_BD_MODE,
                        &Value,
                        B57XX_RX_DATA_BD_MODE_ENABLE | B57XX_RX_DATA_BD_MODE_ILLEGALSZ);
    
    /* Initialization Procedure step 90: Enable the send data initiator functional block */
    B57XXEnableRegister(Adapter, B57XX_REG_TX_DATA_MODE, &Value, B57XX_TX_DATA_MODE_ENABLE);
    
    /* Initialization Procedure step 91: Enable the send BD initiator functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_TX_INITIATOR_MODE,
                        &Value,
                        B57XX_BD_MODE_ENABLE | B57XX_BD_MODE_ATTN);
    
    /* Initialization Procedure step 92: Enable the send BD selector functional block */
    B57XXEnableRegister(Adapter,
                        B57XX_REG_TX_RING_SELECTOR_MODE,
                        &Value,
                        B57XX_BD_MODE_ENABLE | B57XX_BD_MODE_ATTN);
    
    /* Initialization Procedure step 93 not applicable: Download firmware */
    
    /* Initialization Procedure step 94: Enable the transmit MAC */
    B57XXWriteRegister(Adapter,
                       B57XX_REG_ETH_TX_MODE,
                       B57XX_ETH_TX_MODE_ENABLE | B57XX_ETH_TX_MODE_LOCKUPFIX);
    NdisStallExecution(100);
    
    /* Initialization Procedure step 95: Enable the receive MAC */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_RX_MODE, B57XX_ETH_RX_MODE_ENABLE);
    NdisStallExecution(10);
    
    /* Initialization Procedure step 96: Disable auto-polling on management */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MI_MODE, 0xC0000);
    
    /* Initialization Procedure step 97: Configure D0 power state in PMSCR */
    B57XXEnableUShort(Adapter, B57XX_REG_POWER_MANAGEMENT, &Value, B57XX_POWER_MANAGEMENT_STATE_D0);
    
    /* Initialization Procedure step 98: Program Hardware to control LEDs */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_LED, 0);
    
    /* Initialization Procedure step 99-100: Setup the physical layer & MAC functional blocks */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MI_STATUS, B57XX_ETH_MI_STATUS_LINK);
    Status = NICSetupPHY(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return Status;
    }
    
    /* Initialization Procedure step 101: Setup multicast filters */
    NICUpdateMulticastList(Adapter);
    NICApplyPacketFilter(Adapter);
    
    /* Initialization Procedure step 102: Enable interrupts (done later) */
    B57XXDisableRegister(Adapter, B57XX_REG_MSI_MODE, &Value, B57XX_MSI_MODE_ENABLE);
    
    Adapter->HardwareStatus = NdisHardwareStatusReady;
    
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICSetupPHY(_In_ PB57XX_ADAPTER Adapter)
{
    /* PHY Initialization Procedure step 1: Disable link events */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_EVENT, 0x00);
    
    /* PHY Initialization Procedure step 2: Clear link attentions */
    B57XXClearMACAttentions(Adapter);
    
    /* PHY Initialization Procedure step 3: Disable Autopolling mode */
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MI_MODE, 0xC0000);
    
    /* PHY Initialization Procedure step 4: Wait for the Auto Poll disable step to complete */
    NdisStallExecution(40);
    
    /* PHY Initialization Procedure step 5: Disable WOL mode */
    B57XXWritePHY(Adapter, B57XX_MII_REG_AUX_CTRL, 0x02);
    
    /* PHY Initialization Procedure step 6: Errata for the physical layer component */
    
    /* PHY Initialization Procedure step 8: Configure the PHY interrupt mask */
    B57XXWritePHY(Adapter, B57XX_MII_REG_INT_MASK, ~B57XX_PHY_INT_LINKSTATUS);
    
    /* PHY Initialization Procedure step 7-16 (not 8): Link status update */
    if (NICUpdateLinkStatus(Adapter) == NDIS_STATUS_MEDIA_CONNECT)
    {
        NDIS_MinDbgPrint("Link connected\n");
    }
    
    /* PHY Initialization Procedure step 17: Enable port polling */
    //B57XXEnableRegister(Adapter, B57XX_REG_ETH_MI_MODE, &Value, B57XX_ETH_MI_MODE_PORTPOLLING);
    
    /* PHY Initialization Procedure step 18: Enable link attentions */
    B57XXWriteRegister(Adapter,
                       B57XX_REG_ETH_MAC_EVENT,
                       B57XX_ETH_MAC_EVENT_LINKSTATE | B57XX_ETH_MAC_EVENT_MIINT);
     
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICUpdateLinkStatus(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    USHORT Value16;
    
    B57XXClearMACAttentions(Adapter);
    
    /* PHY Initialization Procedure step 7: Acknowledge outstanding interrupts */
    B57XXReadAndClearPHYInterrupts(Adapter, &Value16);
    
    /* PHY Initialization Procedure step 9: Configure the Three LINK LED Mode bit (optional) */
    
    /* PHY Initialization Procedure step 10: Determine link status */
    B57XXReadPHY(Adapter, B57XX_MII_REG_STATUS, &Value16);
    B57XXReadPHY(Adapter, B57XX_MII_REG_STATUS, &Value16);
    
    /* PHY Initialization Procedure step 11-12: Determine speed and duplex operation & store */
    NdisStallExecution(20000);
    B57XXReadPHY(Adapter, B57XX_MII_REG_AUX_STATUS, &Value16);
    Adapter->LinkStatus = Value16;
    if (!(Value16 & B57XX_PHY_AUX_STATUS_LINKSTATUS))
    {
        NDIS_MinDbgPrint("Link disconnected (0x%x)\n", Value16);
        
        B57XXReadPHY(Adapter, B57XX_MII_REG_AUTONEG_ADVERT, &Value16);
        Value16 &= ~B57XX_PHY_AUTONEG_PAUSE;
        B57XXWritePHY(Adapter, B57XX_MII_REG_AUTONEG_ADVERT, Value16);
        
        B57XXDisableRegister(Adapter, B57XX_REG_ETH_TX_MODE, &Value, B57XX_ETH_TX_MODE_FLOWCTRL);
        B57XXDisableRegister(Adapter, B57XX_REG_ETH_RX_MODE, &Value, B57XX_ETH_RX_MODE_FLOWCTRL);
        
        B57XXClearMACAttentions(Adapter);
        
        return NDIS_STATUS_MEDIA_DISCONNECT;
    }
    
    /* PHY Initialization Procedure step 13: Enable Flow Control */
    B57XXReadPHY(Adapter, B57XX_MII_REG_AUTONEG_ADVERT, &Value16);
    Value16 |= B57XX_PHY_AUTONEG_PAUSE;
    B57XXWritePHY(Adapter, B57XX_MII_REG_AUTONEG_ADVERT, Value16);
    NICSetupFlowControl(Adapter);
    
    /* PHY Initialization Procedure step 14-16: Configure MAC port mode, duplex, & polarity */
    B57XXReadRegister(Adapter, B57XX_REG_ETH_MAC_MODE, &Value);
    Value &= ~(B57XX_ETH_MAC_MODE_HALFDUPLEX | B57XX_MASK_ETH_MAC_MODE_PORT);
    switch (Adapter->LinkStatus & B57XX_MASK_PHY_AUX_STATUS_MODE)
    {
        case B57XX_PHY_AUX_STATUS_MODE_10TH:
            Value |= B57XX_ETH_MAC_MODE_HALFDUPLEX;
            NDIS_MinDbgPrint("Half duplex\n");
        case B57XX_PHY_AUX_STATUS_MODE_10TF:
            Value |= B57XX_ETH_MAC_MODE_PORT_MII;
            NDIS_MinDbgPrint("10BASE-T (MII)\n");
            break;
        case B57XX_PHY_AUX_STATUS_MODE_100TXH:
        case B57XX_PHY_AUX_STATUS_MODE_100T4:
            Value |= B57XX_ETH_MAC_MODE_HALFDUPLEX;
            NDIS_MinDbgPrint("Half duplex\n");
        case B57XX_PHY_AUX_STATUS_MODE_100TXF:
            Value |= B57XX_ETH_MAC_MODE_PORT_MII;
            NDIS_MinDbgPrint("100BASE-T (MII)\n");
            break;
        case B57XX_PHY_AUX_STATUS_MODE_1000TH:
            Value |= B57XX_ETH_MAC_MODE_HALFDUPLEX;
            NDIS_MinDbgPrint("Half duplex\n");
        case B57XX_PHY_AUX_STATUS_MODE_1000TF:
            Value |= B57XX_ETH_MAC_MODE_PORT_GMI;
            NDIS_MinDbgPrint("1000BASE-T (GMI)\n");
            break;
        default:
            Value |= B57XX_ETH_MAC_MODE_PORT_NONE;
    }
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_MODE, Value);
    
    B57XXClearMACAttentions(Adapter);
    return NDIS_STATUS_MEDIA_CONNECT;
}

VOID
NTAPI
NICSetupFlowControl(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    USHORT AutonegLocal;
    USHORT AutonegRemote;
    BOOLEAN FlowControlTX = FALSE;
    BOOLEAN FlowControlRX = FALSE;
    
    if (!((Adapter->LinkStatus & B57XX_PHY_AUX_STATUS_MODE_10TF) ||
        (Adapter->LinkStatus & B57XX_PHY_AUX_STATUS_MODE_100TXF) ||
        (Adapter->LinkStatus & B57XX_PHY_AUX_STATUS_MODE_1000TF)))
    {
        NDIS_MinDbgPrint("Flow control only available in full-duplex mode.\n");
        goto Finish;
    }
    
    if (!(Adapter->LinkStatus & B57XX_PHY_AUX_STATUS_AUTONEGCOMP))
    {
        NDIS_MinDbgPrint("Autonegotiation not complete, cannot configure Flow control\n");
        goto Finish;
    }
    
    B57XXReadPHY(Adapter, B57XX_MII_REG_AUTONEG_ADVERT, &AutonegLocal);
    B57XXReadPHY(Adapter, B57XX_MII_REG_AUTONEG_PARTNER, &AutonegRemote);
    
    if ((AutonegLocal & B57XX_PHY_AUTONEG_PAUSE) && (AutonegLocal & B57XX_PHY_AUTONEG_ASYMPAUSE))
    {
        FlowControlTX = (AutonegRemote & B57XX_PHY_AUTONEG_PAUSE) ? TRUE : FALSE;
        FlowControlRX = (AutonegRemote & B57XX_PHY_AUTONEG_ASYMPAUSE) ? TRUE : FALSE;
    }
    else if (AutonegLocal & B57XX_PHY_AUTONEG_PAUSE)
    {
        FlowControlTX = FlowControlRX = (AutonegRemote & B57XX_PHY_AUTONEG_PAUSE) ? TRUE : FALSE;
    }
    else if (AutonegLocal & B57XX_PHY_AUTONEG_ASYMPAUSE)
    {
        FlowControlTX = (AutonegRemote & B57XX_PHY_AUTONEG_PAUSE) ? TRUE : FALSE;
    }
    
Finish:
    B57XXReadRegister(Adapter, B57XX_REG_ETH_TX_MODE, &Value);
    if (FlowControlTX)
    {
        Value |= B57XX_ETH_TX_MODE_FLOWCTRL;
    }
    else
    {
        Value &= ~B57XX_ETH_TX_MODE_FLOWCTRL;
    }
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_TX_MODE, Value);
    
    B57XXReadRegister(Adapter, B57XX_REG_ETH_RX_MODE, &Value);
    if (FlowControlRX)
    {
        Value |= B57XX_ETH_RX_MODE_FLOWCTRL;
    }
    else
    {
        Value &= ~B57XX_ETH_RX_MODE_FLOWCTRL;
    }
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_RX_MODE, Value);
}

NDIS_STATUS
NTAPI
NICShutdown(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    if (Adapter->HardwareStatus != NdisHardwareStatusReady)
    {
        return NDIS_STATUS_FAILURE;
    }
    Adapter->HardwareStatus = NdisHardwareStatusClosing;
    
    /* Receive path shutdown sequence. */
    B57XXDisableRegister(Adapter, B57XX_REG_ETH_RX_MODE, &Value, B57XX_ETH_RX_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_RX_BD_MODE, &Value, B57XX_BD_MODE_ENABLE);
    B57XXDisableRegister(Adapter,
                         B57XX_REG_RX_LIST_PLACE_MODE,
                         &Value,
                         B57XX_RX_LIST_PLACE_MODE_ENABLE);
    if (!Adapter->B5705Plus)
    {
        B57XXDisableRegister(Adapter,
                             B57XX_REG_RX_LIST_SELECT_MODE,
                             &Value,
                             B57XX_RX_LIST_SELECT_MODE_ENABLE);
    }
    B57XXDisableRegister(Adapter, B57XX_REG_RX_DATA_BD_MODE, &Value, B57XX_RX_DATA_BD_MODE_ENABLE);
    B57XXDisableRegister(Adapter,
                         B57XX_REG_RX_DATA_COMPL_MODE,
                         &Value,
                         B57XX_RX_DATA_COMPL_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_RX_BD_COMPL_MODE, &Value, B57XX_BD_MODE_ENABLE);
    
    /* Transmit path shutdown sequence. */
    B57XXDisableRegister(Adapter, B57XX_REG_TX_RING_SELECTOR_MODE, &Value, B57XX_BD_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_TX_INITIATOR_MODE, &Value, B57XX_BD_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_TX_DATA_MODE, &Value, B57XX_TX_DATA_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_DMA_READ_MODE, &Value, B57XX_DMA_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_TX_COMPL_MODE, &Value, B57XX_TX_COMPL_MODE_ENABLE);
    if (!Adapter->B5705Plus)
    {
        B57XXDisableRegister(Adapter,
                             B57XX_REG_DMA_COMPLETION,
                             &Value,
                             B57XX_DMA_COMPLETION_ENABLE);
    }
    B57XXDisableRegister(Adapter, B57XX_REG_TX_BD_COMPL_MODE, &Value, B57XX_BD_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_ETH_MAC_MODE, &Value, B57XX_ETH_MAC_MODE_TXDMA);
    B57XXDisableRegister(Adapter, B57XX_REG_ETH_TX_MODE, &Value, B57XX_ETH_TX_MODE_ENABLE);
    
    /* Memory related state machines shutdown. */
    B57XXDisableRegister(Adapter, B57XX_REG_COAL_HOST_MODE, &Value, B57XX_COAL_HOST_MODE_ENABLE);
    B57XXDisableRegister(Adapter, B57XX_REG_DMA_WRITE_MODE, &Value, B57XX_DMA_MODE_ENABLE);
    if (!Adapter->B5705Plus)
    {
        B57XXDisableRegister(Adapter,
                             B57XX_REG_MBUF_CLUSTER_FREE,
                             &Value,
                             B57XX_MBUF_CLUSTER_FREE_ENABLE);
    }
    B57XXWriteRegister(Adapter, B57XX_REG_FTQ_RESET, 0xFFFFFFFF);
    B57XXWriteRegister(Adapter, B57XX_REG_FTQ_RESET, 0x00000000);
    if (!Adapter->B5705Plus)
    {
        B57XXDisableRegister(Adapter, B57XX_REG_BUF_MAN_MODE, &Value, B57XX_BUF_MAN_MODE_ENABLE);
        B57XXDisableRegister(Adapter, B57XX_REG_MEM_ARB_MODE, &Value, B57XX_MEM_ARB_MODE_ENABLE);
    }
    
    return NDIS_STATUS_SUCCESS;
}

VOID
NTAPI
NICUpdateMACAddresses(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXReadRegister(Adapter, B57XX_REG_ETH_MAC_ADDR1_HI, &Value);
    Adapter->MACAddresses[0][0] = (Value >> 8) & 0xFF;
    Adapter->MACAddresses[0][1] = (Value >> 0) & 0xFF;
    
    B57XXReadRegister(Adapter, B57XX_REG_ETH_MAC_ADDR1_LO, &Value);
    Adapter->MACAddresses[0][2] = (Value >> 24) & 0xFF;
    Adapter->MACAddresses[0][3] = (Value >> 16) & 0xFF;
    Adapter->MACAddresses[0][4] = (Value >> 8) & 0xFF;
    Adapter->MACAddresses[0][5] = (Value >> 0) & 0xFF;
    
    NDIS_MinDbgPrint("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                     Adapter->MACAddresses[0][0],
                     Adapter->MACAddresses[0][1],
                     Adapter->MACAddresses[0][2],
                     Adapter->MACAddresses[0][3],
                     Adapter->MACAddresses[0][4],
                     Adapter->MACAddresses[0][5]);
}

NDIS_STATUS
NTAPI
NICUpdateMulticastList(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    ULONG RegIndex;
    ULONG BitPos;
    
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 0, 0);
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 4, 0);
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 8, 0);
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 12, 0);
    
    for (UINT i = 0; i < Adapter->MulticastListSize; i++)
    {
        BitPos = B57XXComputeInverseCrc32(Adapter->MulticastList[i], IEEE_802_ADDR_LENGTH) & 0x7F;
        RegIndex = (BitPos & 0x60) >> 5;
        BitPos &= 0x1F;
        
        ASSERT(RegIndex < 4);
        
        B57XXEnableRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 4 * RegIndex, &Value, (1 << BitPos));
    }
    
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICApplyPacketFilter(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXReadRegister(Adapter, B57XX_REG_ETH_RX_MODE, &Value);
    Value &= ~(B57XX_ETH_RX_MODE_PROMISCUOUS | B57XX_ETH_RX_MODE_FILTBROAD);
    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        Value |= B57XX_ETH_RX_MODE_PROMISCUOUS;
    }
    if (!(Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST))
    {
        Value |= B57XX_ETH_RX_MODE_FILTBROAD;
    }
    B57XXWriteRegister(Adapter, B57XX_REG_ETH_RX_MODE, Value);
    
    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
    {
        B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 0,  0xFFFFFFFF);
        B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 4,  0xFFFFFFFF);
        B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 8,  0xFFFFFFFF);
        B57XXWriteRegister(Adapter, B57XX_REG_ETH_MAC_HASH + 12, 0xFFFFFFFF);
    }
    else
    {
        NICUpdateMulticastList(Adapter);
    }
    
    return NDIS_STATUS_SUCCESS;
}

_Check_return_
NDIS_STATUS
NTAPI
NICTransmitPacket(_In_ PB57XX_ADAPTER Adapter,
                  _In_ PHYSICAL_ADDRESS PhysicalAddress,
                  _In_ ULONG Length,
                  _In_ USHORT Flags)
{
    ULONG Index = Adapter->SendProducer[0].ProducerIndex;
    
    if (Adapter->SendProducer[0].RingFull == TRUE)
    {
        return NDIS_STATUS_RESOURCES;
    }
    
    Adapter->SendProducer[0].pRing[Index].Length = Length;
    Adapter->SendProducer[0].pRing[Index].Flags = Flags;
    B57XXConvertAddress(&Adapter->SendProducer[0].pRing[Index].HostAddress, &PhysicalAddress);
    
    Index = (Index + 1) % Adapter->SendProducer[0].Count;
    Adapter->SendProducer[0].ProducerIndex = Index;
    B57XXWriteMailbox(Adapter, B57XX_MBOX_TXP_HOST_RING_INDEX1, Index);
    
    if (Index == Adapter->SendProducer[0].ConsumerIndex)
    {
        NDIS_MinDbgPrint("Send ring is full\n");
        Adapter->SendProducer[0].RingFull = TRUE;
    }

    return NDIS_STATUS_SUCCESS;
}

VOID
NTAPI
NICEnableInterrupts(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXReadULong(Adapter, B57XX_REG_MISC_HOST_CTRL, &Value);
    Value &= ~B57XX_MISC_HOST_CTRL_MASKINTOUT;
    B57XXWriteULong(Adapter, B57XX_REG_MISC_HOST_CTRL, Value);
}

VOID
NTAPI
NICDisableInterrupts(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXEnableULong(Adapter, B57XX_REG_MISC_HOST_CTRL, &Value, B57XX_MISC_HOST_CTRL_MASKINTOUT);
}

VOID
NTAPI
NICInterruptAcknowledge(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXWriteMailbox(Adapter, B57XX_MBOX_INT_MAILBOX0, 1);
    B57XXEnableRegister(Adapter, B57XX_REG_MISC_LOCAL_CTRL, &Value, B57XX_MISC_LOCAL_CTRL_CLEARINT);
}

BOOLEAN
NTAPI
NICInterruptCheckAvailability(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXReadULong(Adapter, B57XX_MBOX_INT_MAILBOX0 + 4, &Value);
    
    return Adapter->Status.pBlock->Status & B57XX_SB_UPDATED;
}

VOID
NTAPI
NICInterruptSignalComplete(_In_ PB57XX_ADAPTER Adapter)
{
    B57XXWriteMailbox(Adapter, B57XX_MBOX_INT_MAILBOX0, (Adapter->Status.LastTag & 0xFF) << 24);
}

VOID
NTAPI
NICReceiveSignalComplete(_In_ PB57XX_ADAPTER Adapter)
{
    B57XXWriteMailbox(Adapter, B57XX_MBOX_RXC_RET_RING_INDEX1, Adapter->ReturnConsumer[0].Index);
    B57XXWriteMailbox(Adapter, B57XX_MBOX_STD_RXP_RING_INDEX, Adapter->StandardProducer.Index);
}

VOID
NTAPI
NICQueryStatisticCounter(_In_  PB57XX_ADAPTER Adapter,
                         _In_  NDIS_OID Oid,
                         _Out_ PULONG64 pValue64)
{
    *pValue64 = 0;
    
    if (Adapter->HardwareStatus != NdisHardwareStatusReady)
    {
        return;
    }
    
    switch (Oid)
    {
        case OID_GEN_XMIT_OK:
            *pValue64 = Adapter->Statistics.TransmitSuccesses;
            break;
        case OID_GEN_RCV_OK:
            *pValue64 = Adapter->Statistics.ReceiveSuccesses;
            break;
        case OID_GEN_XMIT_ERROR:
            *pValue64 = Adapter->Statistics.TransmitErrors;
            break;
        case OID_GEN_RCV_ERROR:
            *pValue64 = Adapter->Statistics.ReceiveErrors;
            break;
        case OID_GEN_RCV_NO_BUFFER:
            *pValue64 = Adapter->Statistics.ReceiveBufferErrors;
            break;
        case OID_GEN_DIRECTED_FRAMES_XMIT:  // ifHCOutUcastPkts
            B57XXReadStatistic(Adapter, 0x3D8, 0x086C, FALSE, pValue64);
            break;
        case OID_GEN_MULTICAST_FRAMES_XMIT: // ifHCOutMulticastPkts
            B57XXReadStatistic(Adapter, 0x3E0, 0x0870, FALSE, pValue64);
            break;
        case OID_GEN_BROADCAST_FRAMES_XMIT: // ifHCOutBroadcastPkts
            B57XXReadStatistic(Adapter, 0x3E8, 0x0874, FALSE, pValue64);
            break;
        case OID_GEN_DIRECTED_FRAMES_RCV:   // ifHCInUcastPkts
            B57XXReadStatistic(Adapter, 0x118, 0x088C, FALSE, pValue64);
            break;
        case OID_GEN_MULTICAST_FRAMES_RCV:   // ifHCInMulticastPkts
            B57XXReadStatistic(Adapter, 0x120, 0x0890, FALSE, pValue64);
            break;
        case OID_GEN_BROADCAST_FRAMES_RCV:   // ifHCInBroadcastPkts
            B57XXReadStatistic(Adapter, 0x120, 0x0894, FALSE, pValue64);
            break;
        default:
            NDIS_MinDbgPrint("Unknown statistic (0x%x)", Oid);
    }
}

NDIS_STATUS
NTAPI
NICFillPowerManagementCapabilities(_In_  PB57XX_ADAPTER Adapter,
                                   _Out_ PNDIS_PNP_CAPABILITIES Capabilities)
{
    /* TODO Fill with real power management capabilities. 
       See the chapter "Power management" in corresponding manuals */ 
    Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
    Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateUnspecified;
    
    return NDIS_STATUS_SUCCESS;
}

VOID
NTAPI
NICOutputDebugInfo(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    ULONG ExtraValue;
    
    B57XXReadULong(Adapter, B57XX_REG_FLOW_ATTENTION, &Value);
    NDIS_MinDbgPrint("Flow attention: 0x%x\n", Value);
    
    B57XXReadULong(Adapter, B57XX_REG_ETH_MAC_STATUS, &Value);
    NDIS_MinDbgPrint("Ethernet MAC Status: 0x%x\n", Value);
    
    B57XXReadULong(Adapter, B57XX_REG_MSI_MODE, &Value);
    B57XXReadULong(Adapter, B57XX_REG_MSI_STATUS, &ExtraValue);
    NDIS_MinDbgPrint("MSI: (Mode: 0x%x, Status: 0x%x)\n", Value, ExtraValue);
    
    B57XXReadULong(Adapter, B57XX_REG_DMA_READ_STATUS, &Value);
    B57XXReadULong(Adapter, B57XX_REG_DMA_WRITE_STATUS, &ExtraValue);
    NDIS_MinDbgPrint("DMA: (Read: 0x%x, Write: 0x%x)\n", Value, ExtraValue);
    
    B57XXReadRegister(Adapter, B57XX_REG_RX_RISC_STATE, &Value);
    B57XXReadRegister(Adapter, B57XX_REG_RX_RISC_MODE, &ExtraValue);
    NDIS_MinDbgPrint("RX RISC: (State: 0x%x, Mode: 0x%x)\n", Value, ExtraValue);
    
    B57XXReadRegister(Adapter, B57XX_REG_TX_RISC_STATE, &Value);
    B57XXReadRegister(Adapter, B57XX_REG_TX_RISC_MODE, &ExtraValue);
    NDIS_MinDbgPrint("TX RISC: (State: 0x%x, Mode: 0x%x)\n", Value, ExtraValue);
}

/* B57XX FUNCTIONS ********************************************************************************/

VOID
B57XXWriteMemoryRCB(_In_ PB57XX_ADAPTER Adapter,
                    _In_ ULONG Address,
                    _In_ volatile PB57XX_RING_CONTROL_BLOCK pRCB)
{
    ULONG AddressBase = Address & 0xFFFF0000;
    ULONG Offset = B57XX_REG_MEM_WINDOW + (Address & 0x0000FFFF);
    
    B57XXWritePciConfigULong(Adapter, B57XX_REG_MEM_BASE, AddressBase);
    *(PB57XX_RING_CONTROL_BLOCK)(Adapter->IoBase + Offset) = *pRCB;
    
    B57XXWritePciConfigULong(Adapter, B57XX_REG_MEM_BASE, 0);
}

NDIS_STATUS
B57XXReadPHY(_In_  PB57XX_ADAPTER Adapter,
             _In_  USHORT PHYRegOffset,
             _Out_ PUSHORT pValue)
{
    ULONG Value32;
    USHORT PHYAddress = 0x01;
    
    B57XXWriteULong(Adapter,
                    B57XX_REG_ETH_MI_COMMUNICATION,
                    ((ULONG)PHYAddress << 21) |
                    ((ULONG)PHYRegOffset << 16) |
                    B57XX_ETH_MI_COMMUNICATION_START |
                    (0b10 << 26));
    
    for (UINT n = 0; n < MAX_ATTEMPTS_PHY; n++)
    {
        B57XXReadULong(Adapter, B57XX_REG_ETH_MI_COMMUNICATION, &Value32);
        if (!(Value32 & B57XX_ETH_MI_COMMUNICATION_START))
        {
            break;
        }
        NdisStallExecution(1);
    }
    if (Value32 & B57XX_ETH_MI_COMMUNICATION_START)
    {
        NDIS_MinDbgPrint("Unable to read to PHY, timed out (0x%x)\n", Value32);
        return NDIS_STATUS_FAILURE;
    }
    
    *pValue = Value32 & 0xFFFF;
    
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
B57XXWritePHY(_In_ PB57XX_ADAPTER Adapter,
              _In_ USHORT PHYRegOffset,
              _In_ USHORT Value)
{
    ULONG Value32;
    USHORT PHYAddress = 0x01;
    
    B57XXWriteULong(Adapter,
                    B57XX_REG_ETH_MI_COMMUNICATION,
                    ((ULONG)PHYAddress << 21) |
                    ((ULONG)PHYRegOffset << 16) |
                    (ULONG)Value |
                    B57XX_ETH_MI_COMMUNICATION_START |
                    (0b01 << 26));
    
    for (UINT n = 0; n < MAX_ATTEMPTS_PHY; n++)
    {
        B57XXReadULong(Adapter, B57XX_REG_ETH_MI_COMMUNICATION, &Value32);
        if (!(Value32 & B57XX_ETH_MI_COMMUNICATION_START))
        {
            break;
        }
        NdisStallExecution(1);
    }
    if (Value32 & B57XX_ETH_MI_COMMUNICATION_START)
    {
        NDIS_MinDbgPrint("Unable to write to PHY, timed out (0x%x)\n", Value32);
        return NDIS_STATUS_FAILURE;
    }
    
    return NDIS_STATUS_SUCCESS;
}

VOID
B57XXClearMACAttentions(_In_ PB57XX_ADAPTER Adapter)
{
    ULONG Value;
    
    B57XXEnableRegister(Adapter,
                        B57XX_REG_ETH_MAC_STATUS,
                        &Value,
                        B57XX_ETH_MAC_STATUS_CONFIGCHANGED |
                        B57XX_ETH_MAC_STATUS_SYNCCHANGED |
                        B57XX_ETH_MAC_STATUS_MICOMPLETE);
}

VOID
B57XXReadAndClearPHYInterrupts(_In_  PB57XX_ADAPTER Adapter,
                               _Out_ PUSHORT Interrupts)
{
    USHORT Dummy;
    
    B57XXReadPHY(Adapter, B57XX_MII_REG_INT_STATUS, Interrupts);
    // Read again to clear
    B57XXReadPHY(Adapter, B57XX_MII_REG_INT_STATUS, &Dummy);
}

VOID
B57XXReadStatistic(_In_  PB57XX_ADAPTER Adapter,
                   _In_  ULONG BlockOffset,
                   _In_  ULONG RegOffset,
                   _In_  BOOLEAN Is64Bit,
                   _Out_ PULONG64 pValue64)
{
    PULARGE_INTEGER pLargeInt = (PULARGE_INTEGER)pValue64;
    
    if (!Adapter->B5705Plus)
    {
        if (Is64Bit)
        {
            pLargeInt->HighPart = *(PULONG)((PUCHAR)Adapter->Statistics.pBlock + BlockOffset + 0);
            pLargeInt->LowPart = *(PULONG)((PUCHAR)Adapter->Statistics.pBlock + BlockOffset + 4);
        }
        else
        {
            pLargeInt->LowPart = *(PULONG)((PUCHAR)Adapter->Statistics.pBlock + BlockOffset);
        }
    }
    else
    {
        if (Is64Bit)
        {
            B57XXReadULong(Adapter, RegOffset + 0, &pLargeInt->HighPart);
            B57XXReadULong(Adapter, RegOffset + 4, &pLargeInt->LowPart);
        }
        else
        {
            B57XXReadULong(Adapter, RegOffset, &pLargeInt->LowPart);
        }
    }
}

VOID
B57XXAccumulateAddress(_Out_ PB57XX_PHYSICAL_ADDRESS pDestination,
                       _In_ LONG Offset)
{
    NDIS_PHYSICAL_ADDRESS Address;
    
    Address.HighPart = pDestination->HighPart;
    Address.LowPart = pDestination->LowPart;
    
    Address.QuadPart += Offset;
    
    pDestination->HighPart = Address.HighPart;
    pDestination->LowPart = Address.LowPart;
}

ULONG
B57XXComputeInverseCrc32(_In_ PUCHAR pBuffer,
                         _In_ ULONG BufferSize)
{
    ULONG Tmp;
    ULONG Reg = 0xFFFFFFFF;
    
    for(ULONG i = 0; i < BufferSize; i++)
    {
        Reg ^= pBuffer[i];
        
        for(ULONG j = 0; j < 8; j++)
        {
            Tmp = Reg & 0x01;
            Reg >>= 1;
            if(Tmp)
            {
                Reg ^= 0xEDB88320;
            }
        }
    }
    return Reg;
}
