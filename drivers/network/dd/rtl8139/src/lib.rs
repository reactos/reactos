/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/dd/rtl8139/src/lib.rs
 * PURPOSE:     RTL8139 NDIS 6.84 miniport for tcpip BINDALL
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * Hardware bring-up and ring logic below come from this file's earlier
 * RTL8139 register checks in the same driver.
 * Miniport registration offsets, OID request layout, and attribute type
 * 0x9E size 0x1C are in drivers/network/ndis/src/register.rs.
 * OID values match sdk/include/psdk/ntddndis.h. NET_BUFFER and NET_BUFFER_LIST
 * field offsets match drivers/network/tcpip/datalink/lan.c and
 * drivers/network/ndis/src/pool.rs. Pool parameter header type 0x80
 * revision 1 is measured in drivers/network/ndis/src/pool.rs.
 * MDL layout and MappedSystemVa / StartVa+ByteOffset follow
 * sdk/include/xdk/mmtypes.h and the MmGetSystemAddressForMdlSafe macro
 * in sdk/include/xdk/mmfuncs.h (that macro is not an ntoskrnl export).
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiportdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_initialize_ex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_restart
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_oid_request
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_send_net_buffer_lists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_return_net_buffer_lists
 */

#![no_std]

use core::panic::PanicInfo;
use core::ptr;
use core::sync::atomic::{AtomicU32, Ordering};

const MP_CHARS: usize = 0x120;
const INIT_SLOT: usize = 0x18;
const HALT_SLOT: usize = 0x20;
const PAUSE_SLOT: usize = 0x30;
const RESTART_SLOT: usize = 0x38;
const OID_SLOT: usize = 0x40;
const SEND_SLOT: usize = 0x48;
const RETURN_SLOT: usize = 0x50;
const SET_OPTIONS_SLOT: usize = 0x10;

const ATTR_REG_SIZE: usize = 0x1C;
const ATTR_CONTEXT_AT: usize = 8;
const ATTR_FLAGS_AT: usize = 0x10;
const ATTR_CHECK_HANG_AT: usize = 0x14;
const ATTR_IFACE_AT: usize = 0x18;
const ATTR_FLAGS_VAL: u32 = 0x165;
const ATTR_CHECK_HANG: u32 = 4;
const ATTR_IFACE_PCI: u32 = 5;

const REQ_TYPE_AT: usize = 4;
const REQ_OID_AT: usize = 0x20;
const REQ_BUFFER_AT: usize = 0x28;
const REQ_LENGTH_AT: usize = 0x30;

const NBL_NEXT_AT: usize = 0;
const NBL_FIRST_NB_AT: usize = 8;
const NB_LENGTH_AT: usize = 0x18;
const NB_MDL_AT: usize = 0x20;
const NB_OFFSET_AT: usize = 0x28;
const NBL_LENGTH_AT: usize = 0x198;
const NBL_MDL_AT: usize = 0x1A0;
const NBL_OFFSET_AT: usize = 0x1A8;
const NBL_SIDE_FRAME_AT: usize = 0x100;
const NBL_SIDE_MDL_AT: usize = 0x108;

const IRQ_CHARS_BYTES: usize = 0x60;
const IRQ_CHARS_ISR_AT: usize = 0x08;
const IRQ_CHARS_DPC_AT: usize = 0x10;
const IRQ_CHARS_DISABLE_AT: usize = 0x18;
const IRQ_CHARS_ENABLE_AT: usize = 0x20;

const POOL_PARAMS_BYTES: usize = 0x10;

const SUCCESS: u32 = 0;
const NOT_SUPPORTED: u32 = 0xC000_00BB;
const RESOURCES: u32 = 0xC000_009A;
const FAILURE: u32 = 0xC000_0001;
const FRAME_POOL_TAG: u32 = u32::from_le_bytes(*b"39rT");
/* sdk/include/xdk/mmtypes.h MDL_MAPPED_TO_SYSTEM_VA / MDL_SOURCE_IS_NONPAGED_POOL */
const MDL_MAPPED_TO_SYSTEM_VA: u16 = 0x0001;
const MDL_SOURCE_IS_NONPAGED_POOL: u16 = 0x0004;

#[repr(C)]
struct Mdl {
    next: *mut Mdl,
    size: u16,
    flags: u16,
    _pad: u32,
    process: *mut u8,
    mapped: *mut u8,
    start: *mut u8,
    byte_count: u32,
    byte_offset: u32,
}

/* sdk/include/psdk/ntddndis.h */
const OID_GEN_HARDWARE_STATUS: u32 = 0x0001_0102;
const OID_GEN_MEDIA_SUPPORTED: u32 = 0x0001_0103;
const OID_GEN_MEDIA_IN_USE: u32 = 0x0001_0104;
const OID_GEN_MAXIMUM_FRAME_SIZE: u32 = 0x0001_0106;
const OID_GEN_LINK_SPEED: u32 = 0x0001_0107;
const OID_GEN_VENDOR_DESCRIPTION: u32 = 0x0001_010D;
const OID_GEN_CURRENT_PACKET_FILTER: u32 = 0x0001_010E;
const OID_GEN_CURRENT_LOOKAHEAD: u32 = 0x0001_010F;
const OID_GEN_MAXIMUM_TOTAL_SIZE: u32 = 0x0001_0111;
const OID_GEN_MEDIA_CONNECT_STATUS: u32 = 0x0001_0114;
const OID_GEN_MAXIMUM_SEND_PACKETS: u32 = 0x0001_0115;
const OID_802_3_CURRENT_ADDRESS: u32 = 0x0101_0102;
const NDIS_MEDIUM_802_3: u32 = 0;
const NDIS_HW_STATUS_READY: u32 = 0;
const NDIS_MEDIA_CONNECTED: u32 = 0;
const ETH_HEADER: usize = 14;

/* sdk/include/xdk/iotypes.h: PCIConfiguration is the fifth enumerator. */
const PCI_CONFIGURATION: u32 = 4;
const PCI_HEADER: u32 = 64;
const PCI_VENDOR: u16 = 0x10EC;
const PCI_DEVICE: u16 = 0x8139;
const PCI_ENABLE_BUS_MASTER: u16 = 0x0004;
const R_MAC: u16 = 0;
/* drivers/network/dd/rtl8139/rtlhw.h */
const R_CMD: u16 = 0x37;
const B_CMD_RST: u8 = 0x10;
const R_CFG1: u16 = 0x52;
const R_RXSA: u16 = 0x30;
const R_TC: u16 = 0x40;
const R_RC: u16 = 0x44;
const B_CMD_TXE: u8 = 0x04;
const B_CMD_RXE: u8 = 0x08;
const R_CMD_RXEMPTY: u8 = 0x01;
const R_CAPR: u16 = 0x38;
const B_RC_APM: u32 = 0x02;
/* Accept Multicast — RTL8139 RxConfig bit 2 (Realtek RTL8139 datasheet
 * Rx Configuration Register). Needed for IEEE 802.1AB nearest-bridge
 * dest 01-80-C2-00-00-0E (Boot-NdisDisk-LldpPeer peer inject reached
 * the hub but produced no RTL8139 RXET 88cc with only APM|AB). */
const B_RC_AM: u32 = 0x04;
const B_RC_AB: u32 = 0x08;
const B_RC_AAP: u32 = 0x01;
const R_MAR0: u16 = 0x08;
/* NDIS_PACKET_TYPE_* in sdk/include/psdk/ntddndis.h. */
const NDIS_PACKET_TYPE_DIRECTED: u32 = 0x0000_0001;
const NDIS_PACKET_TYPE_MULTICAST: u32 = 0x0000_0002;
const NDIS_PACKET_TYPE_ALL_MULTICAST: u32 = 0x0000_0004;
const NDIS_PACKET_TYPE_BROADCAST: u32 = 0x0000_0008;
const NDIS_PACKET_TYPE_PROMISCUOUS: u32 = 0x0000_0020;
const RSR_ROK: u16 = 0x0001;
/* drivers/network/dd/rtl8139/nic.h */
const RECEIVE_BUFFER_SIZE: usize = 32768;
const MAX_RESET_ATTEMPTS: u32 = 25;
const FULL_RECEIVE_BUFFER_SIZE: usize = 32768 + 16 + 2048;
const RC_VAL: u32 = 0x800B780;
const TC_VAL: u32 = 0x700;
const MINIMUM_FRAME_SIZE: usize = 60;
const MAXIMUM_FRAME_SIZE: usize = 1514;
/* Interrupt Mask and Interrupt Status, Realtek RTL8139C(L) datasheet
 * (https://www.cs.columbia.edu/~sedwards/classes/2005/emsys-summer/8139cp.pdf).
 * Writing a set bit back to the status register clears it. */
const R_IMR: u16 = 0x3C;
const R_ISR: u16 = 0x3E;
const I_ROK: u16 = 0x0001;
const I_RER: u16 = 0x0002;
const I_TOK: u16 = 0x0004;
const I_TER: u16 = 0x0008;
const I_RXOVW: u16 = 0x0010;
const I_MASK: u16 = I_ROK | I_RER | I_TOK | I_TER | I_RXOVW;
const R_TXSTS0: u16 = 0x10;
const R_TXSAD0: u16 = 0x20;
const R_TXS_STATOK: u32 = 0x0000_8000;
const TX_DESC_COUNT: u32 = 4;
const CHECK_TX_READS: u32 = 100_000;
const HIGHEST_PA: u64 = 0xFFFF_FFFF;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

unsafe extern "system" {
    fn DbgPrint(format: *const u8, ...) -> i32;
    fn NdisMRegisterMiniportDriver(
        driver: *mut u8,
        registry: *mut u8,
        context: *mut u8,
        chars: *const u8,
        out_handle: *mut *mut u8,
    ) -> u32;
    fn NdisMSetMiniportAttributes(handle: *mut u8, attributes: *const u8) -> u32;
    fn NdisMRegisterInterruptEx(
        adapter: *mut u8,
        context: *mut u8,
        characteristics: *mut u8,
        interrupt_handle: *mut *mut u8,
    ) -> u32;
    fn NdisAllocateNetBufferListPool(handle: *mut u8, params: *const u8) -> *mut u8;
    fn NdisAllocateNetBufferAndNetBufferList(
        pool: *mut u8,
        context: u16,
        backfill: u16,
        mdl: *mut u8,
        data_offset: u32,
        data_length: u64,
    ) -> *mut u8;
    fn NdisFreeNetBufferList(list: *mut u8, flags: u32);
    fn NdisAllocateMdl(handle: *mut u8, virtual_address: *mut u8, length: u32) -> *mut u8;
    fn NdisFreeMdl(mdl: *mut u8);
    fn NdisMIndicateReceiveNetBufferLists(
        adapter: *mut u8,
        lists: *mut u8,
        port: u32,
        count: u32,
        flags: u32,
    );
    fn NdisMSendNetBufferListsComplete(adapter: *mut u8, lists: *mut u8, flags: u32);
    fn HalGetBusData(
        bus_type: u32,
        bus: u32,
        slot: u32,
        buffer: *mut u8,
        length: u32,
    ) -> u32;
    fn HalSetBusDataByOffset(
        bus_type: u32,
        bus: u32,
        slot: u32,
        buffer: *const u8,
        offset: u32,
        length: u32,
    ) -> u32;
    fn MmAllocateContiguousMemory(bytes: usize, highest: u64) -> *mut u8;
    fn MmGetPhysicalAddress(base: *mut u8) -> u64;
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
}

struct TxBuf {
    data: *mut u8,
    physical: u64,
}

struct Adapter {
    ndis_handle: *mut u8,
    base: u16,
    mac: [u8; 6],
    receive: *mut u8,
    rx_cursor: usize,
    nbl_pool: *mut u8,
    packet_filter: u32,
    tx_bufs: [TxBuf; 4],
    next_tx: AtomicU32,
}

static mut ADAPTER: Adapter = Adapter {
    ndis_handle: ptr::null_mut(),
    base: 0,
    mac: [0; 6],
    receive: ptr::null_mut(),
    rx_cursor: 0,
    nbl_pool: ptr::null_mut(),
    packet_filter: 0,
    tx_bufs: [
        TxBuf {
            data: ptr::null_mut(),
            physical: 0,
        },
        TxBuf {
            data: ptr::null_mut(),
            physical: 0,
        },
        TxBuf {
            data: ptr::null_mut(),
            physical: 0,
        },
        TxBuf {
            data: ptr::null_mut(),
            physical: 0,
        },
    ],
    next_tx: AtomicU32::new(0),
};
/* RXPOLL and line IRQ can queue drain_rx together; a second walker
 * desyncs CAPR (output/ndis-disk-20261116T202149Z.log RTL8139 RX 0800 len 1). */
static DRAINING: AtomicU32 = AtomicU32::new(0);
/* Disk may start this miniport more than once against the same ImagePath
 * (output/ndis-disk-20261116T203000Z.log: two NDIS6 LAN, RXARP on the
 * second handle, PingCheck timeout on the first). */
static STARTED: AtomicU32 = AtomicU32::new(0);

fn line(text: &[u8]) {
    unsafe { DbgPrint(text.as_ptr()); }
}

fn line_status(text: &[u8], status: u32) {
    unsafe { DbgPrint(text.as_ptr(), status); }
}

fn put_u16(buf: &mut [u8], at: usize, value: u16) {
    let bytes = value.to_le_bytes();
    buf[at] = bytes[0];
    buf[at + 1] = bytes[1];
}

fn put_u32(buf: &mut [u8], at: usize, value: u32) {
    let bytes = value.to_le_bytes();
    buf[at..at + 4].copy_from_slice(&bytes);
}

fn put_ptr(buf: &mut [u8], at: usize, value: usize) {
    let bytes = value.to_le_bytes();
    buf[at..at + bytes.len()].copy_from_slice(&bytes);
}

fn read_u32(block: *const u8, at: usize) -> u32 {
    unsafe { ptr::read_unaligned(block.add(at) as *const u32) }
}

fn read_ptr(block: *const u8, at: usize) -> *mut u8 {
    unsafe { ptr::read_unaligned(block.add(at) as *const *mut u8) }
}

fn write_port(value: u8, port: u16) {
    unsafe {
        core::arch::asm!(
            "out dx, al",
            in("dx") port,
            in("al") value,
            options(nostack, preserves_flags)
        );
    }
}

fn write_port_u16(port: u16, value: u16) {
    unsafe {
        core::arch::asm!(
            "out dx, ax",
            in("dx") port,
            in("ax") value,
            options(nostack, preserves_flags)
        );
    }
}

fn write_port_u32(port: u16, value: u32) {
    unsafe {
        core::arch::asm!(
            "out dx, eax",
            in("dx") port,
            in("eax") value,
            options(nostack, preserves_flags)
        );
    }
}

fn read_port_u32(port: u16) -> u32 {
    let value: u32;
    unsafe {
        core::arch::asm!(
            "in eax, dx",
            in("dx") port,
            out("eax") value,
            options(nostack, nomem, preserves_flags)
        );
    }
    value
}

fn read_port_u16(port: u16) -> u16 {
    let value: u16;
    unsafe {
        core::arch::asm!(
            "in ax, dx",
            in("dx") port,
            out("ax") value,
            options(nostack, nomem, preserves_flags)
        );
    }
    value
}

fn read_port(port: u16) -> u8 {
    let value: u8;
    unsafe {
        core::arch::asm!(
            "in al, dx",
            in("dx") port,
            out("al") value,
            options(nostack, nomem, preserves_flags)
        );
    }
    value
}

fn pci_slot(device: u32, function: u32) -> u32 {
    device | (function << 5)
}

fn find_io_base() -> Option<u16> {
    let mut header = [0u8; PCI_HEADER as usize];
    for bus in 0..8u32 {
        for device in 0..32u32 {
            for function in 0..8u32 {
                let got = unsafe {
                    HalGetBusData(
                        PCI_CONFIGURATION,
                        bus,
                        pci_slot(device, function),
                        header.as_mut_ptr(),
                        PCI_HEADER,
                    )
                };
                if got < 4 {
                    if function == 0 {
                        break;
                    }
                    continue;
                }
                let vendor = u16::from_le_bytes([header[0], header[1]]);
                let device_id = u16::from_le_bytes([header[2], header[3]]);
                if vendor == 0xFFFF && function == 0 {
                    break;
                }
                if vendor != PCI_VENDOR || device_id != PCI_DEVICE {
                    continue;
                }
                let bar = u32::from_le_bytes([header[0x10], header[0x11], header[0x12], header[0x13]]);
                line_status(b"RTL8139 CHECK bar %08x\n\0", bar);
                if bar & 1 == 0 {
                    return None;
                }
                let mut command = u16::from_le_bytes([header[4], header[5]]);
                command |= PCI_ENABLE_BUS_MASTER;
                let command_bytes = command.to_le_bytes();
                let slot = pci_slot(device, function);
                unsafe {
                    HalSetBusDataByOffset(
                        PCI_CONFIGURATION,
                        bus,
                        slot,
                        command_bytes.as_ptr(),
                        4,
                        2,
                    );
                }
                return Some((bar & 0xFFFF_FFFC) as u16);
            }
        }
    }
    None
}

fn power_and_reset(base: u16) -> bool {
    write_port(0, base + R_CFG1);
    write_port(B_CMD_RST, base + R_CMD);
    let mut attempt = 0u32;
    while attempt < MAX_RESET_ATTEMPTS {
        let command = read_port(base + R_CMD);
        if command & B_CMD_RST == 0 {
            line(b"RTL8139 CHECK reset ok\n\0");
            return true;
        }
        attempt += 1;
    }
    line(b"RTL8139 CHECK reset busy\n\0");
    false
}

fn arm_receive(base: u16) -> *mut u8 {
    let buffer = unsafe { MmAllocateContiguousMemory(FULL_RECEIVE_BUFFER_SIZE, HIGHEST_PA) };
    if buffer.is_null() {
        line(b"RTL8139 CHECK rx none\n\0");
        return ptr::null_mut();
    }
    let physical = unsafe { MmGetPhysicalAddress(buffer) };
    line_status(b"RTL8139 CHECK rxpa %08x\n\0", physical as u32);
    if physical > HIGHEST_PA {
        return ptr::null_mut();
    }
    unsafe { ptr::write_bytes(buffer, 0, FULL_RECEIVE_BUFFER_SIZE) };
    write_port_u32(base + R_RXSA, physical as u32);
    write_port(B_CMD_TXE | B_CMD_RXE, base + R_CMD);
    /* MAR all-ones + AM so 802.1AB multicast is not dropped before the
     * protocol sets OID_GEN_CURRENT_PACKET_FILTER. */
    write_port_u32(base + R_MAR0, 0xFFFF_FFFF);
    write_port_u32(base + R_MAR0 + 4, 0xFFFF_FFFF);
    write_port_u32(base + R_RC, RC_VAL | B_RC_APM | B_RC_AM | B_RC_AB);
    write_port_u32(base + R_TC, TC_VAL);
    write_port_u16(base + R_CAPR, 0u16.wrapping_sub(0x10));
    let command = read_port(base + R_CMD);
    if (command & (B_CMD_TXE | B_CMD_RXE)) == (B_CMD_TXE | B_CMD_RXE) {
        line(b"RTL8139 CHECK txrx ok\n\0");
        buffer
    } else {
        ptr::null_mut()
    }
}

fn prepare_tx(adapter: &mut Adapter) -> bool {
    let mut index = 0u32;
    while index < TX_DESC_COUNT {
        let buffer = unsafe { MmAllocateContiguousMemory(MAXIMUM_FRAME_SIZE, HIGHEST_PA) };
        if buffer.is_null() {
            return false;
        }
        let physical = unsafe { MmGetPhysicalAddress(buffer) };
        if physical > HIGHEST_PA || (physical & 3) != 0 {
            return false;
        }
        adapter.tx_bufs[index as usize].data = buffer;
        adapter.tx_bufs[index as usize].physical = physical;
        index += 1;
    }
    true
}

fn copy_ring(buffer: *const u8, offset: usize, len: usize, out: &mut [u8]) {
    let mut index = 0;
    while index < len {
        let at = (offset + index) % RECEIVE_BUFFER_SIZE;
        out[index] = unsafe { *buffer.add(at) };
        index += 1;
    }
}

fn advance_rx(base: u16, length: u16, offset: usize) -> usize {
    let mut next = length as usize + 4;
    next = (next + 3) & !3;
    next = (offset + next) % RECEIVE_BUFFER_SIZE;
    write_port_u16(base + R_CAPR, (next as u16).wrapping_sub(0x10));
    next
}

fn mdl_system_va(mdl: *mut u8) -> *mut u8 {
    /* sdk/include/xdk/mmfuncs.h MmGetSystemAddressForMdlSafe for
     * MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL. tcpip send
     * MDLs come from NdisAllocateMdl + MmBuildMdlForNonPagedPool. */
    if mdl.is_null() {
        return ptr::null_mut();
    }
    let desc = mdl as *const Mdl;
    let flags = unsafe { (*desc).flags };
    if (flags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) != 0 {
        let mapped = unsafe { (*desc).mapped };
        if !mapped.is_null() {
            return mapped;
        }
        return unsafe { (*desc).start.add((*desc).byte_offset as usize) };
    }
    ptr::null_mut()
}

fn frame_va_from_nb(nbl: *mut u8, nb: *mut u8) -> Option<(*mut u8, u32, usize)> {
    if !nb.is_null() {
        let length =
            unsafe { ptr::read_unaligned(nb.add(NB_LENGTH_AT) as *const u64) };
        let mdl = read_ptr(nb, NB_MDL_AT);
        let offset = read_u32(nb, NB_OFFSET_AT);
        if mdl.is_null() || length == 0 || length > MAXIMUM_FRAME_SIZE as u64 {
            return None;
        }
        let va = mdl_system_va(mdl);
        if va.is_null() {
            return None;
        }
        return Some((va, offset, length as usize));
    }
    let length = unsafe { ptr::read_unaligned(nbl.add(NBL_LENGTH_AT) as *const u64) };
    let mdl = read_ptr(nbl, NBL_MDL_AT);
    let offset = read_u32(nbl, NBL_OFFSET_AT);
    if mdl.is_null() || length == 0 || length > MAXIMUM_FRAME_SIZE as u64 {
        return None;
    }
    let va = mdl_system_va(mdl);
    if va.is_null() {
        return None;
    }
    Some((va, offset, length as usize))
}

fn transmit_frame(adapter: &Adapter, frame: *const u8, length: usize) -> bool {
    if adapter.base == 0 || length == 0 || length > MAXIMUM_FRAME_SIZE {
        return false;
    }
    let desc = adapter.next_tx.fetch_add(1, Ordering::Relaxed) % TX_DESC_COUNT;
    let slot = &adapter.tx_bufs[desc as usize];
    if slot.data.is_null() {
        return false;
    }
    let wire = if length < MINIMUM_FRAME_SIZE {
        MINIMUM_FRAME_SIZE
    } else {
        length
    };
    unsafe {
        ptr::write_bytes(slot.data, 0, wire);
        ptr::copy_nonoverlapping(frame, slot.data, length);
    }
    let status_port = adapter.base + R_TXSTS0 + (desc as u16 * 4);
    write_port_u32(adapter.base + R_TXSAD0 + (desc as u16 * 4), slot.physical as u32);
    write_port_u32(status_port, wire as u32);
    let mut status = 0u32;
    let mut read = 0u32;
    while read < CHECK_TX_READS {
        status = read_port_u32(status_port);
        if (status & R_TXS_STATOK) != 0 {
            return true;
        }
        read += 1;
    }
    (status & R_TXS_STATOK) != 0
}

fn free_indicated_nbl(nbl: *mut u8) {
    if nbl.is_null() {
        return;
    }
    let frame = read_ptr(nbl, NBL_SIDE_FRAME_AT);
    let mdl = read_ptr(nbl, NBL_SIDE_MDL_AT);
    unsafe {
        if !mdl.is_null() {
            NdisFreeMdl(mdl);
        }
        if !frame.is_null() {
            ExFreePoolWithTag(frame, FRAME_POOL_TAG);
        }
        NdisFreeNetBufferList(nbl, 0);
    }
}

fn drain_rx(adapter: &mut Adapter) {
    if adapter.receive.is_null() || adapter.nbl_pool.is_null() || adapter.base == 0 {
        return;
    }
    if DRAINING
        .compare_exchange(0, 1, Ordering::Acquire, Ordering::Relaxed)
        .is_err()
    {
        return;
    }
    let mut head: *mut u8 = ptr::null_mut();
    let mut tail: *mut u8 = ptr::null_mut();
    let mut count = 0u32;
    while (read_port(adapter.base + R_CMD) & R_CMD_RXEMPTY) == 0 {
        let header = unsafe { adapter.receive.add(adapter.rx_cursor) };
        let status = u16::from_le_bytes([unsafe { *header }, unsafe { *header.add(1) }]);
        let length = u16::from_le_bytes([unsafe { *header.add(2) }, unsafe { *header.add(3) }]);
        if (status & RSR_ROK) == 0 || length < 18 || (length as usize) > MAXIMUM_FRAME_SIZE + 4 {
            adapter.rx_cursor = advance_rx(adapter.base, length.max(4), adapter.rx_cursor);
            continue;
        }
        let frame_len = (length as usize) - 4;
        let frame = unsafe { ExAllocatePoolWithTag(0, frame_len, FRAME_POOL_TAG) };
        if frame.is_null() {
            break;
        }
        let mut scratch = [0u8; MAXIMUM_FRAME_SIZE];
        copy_ring(adapter.receive, adapter.rx_cursor + 4, frame_len, &mut scratch[..frame_len]);
        if frame_len >= 22 {
            let et = u16::from_be_bytes([scratch[12], scratch[13]]);
            line_status(b"RTL8139 RXET %04x\n\0", et as u32);
            line_status(b"RTL8139 RXL %u\n\0", frame_len as u32);
            if et == 0x0806 {
                let op = u16::from_be_bytes([scratch[20], scratch[21]]);
                line_status(b"RTL8139 RXARP %u\n\0", op as u32);
            }
        }
        unsafe {
            ptr::copy_nonoverlapping(scratch.as_ptr(), frame, frame_len);
        }
        let mdl = unsafe { NdisAllocateMdl(adapter.ndis_handle, frame, frame_len as u32) };
        if mdl.is_null() {
            unsafe { ExFreePoolWithTag(frame, FRAME_POOL_TAG) };
            break;
        }
        let nbl = unsafe {
            NdisAllocateNetBufferAndNetBufferList(
                adapter.nbl_pool,
                0,
                0,
                mdl,
                0,
                frame_len as u64,
            )
        };
        if nbl.is_null() {
            unsafe {
                NdisFreeMdl(mdl);
                ExFreePoolWithTag(frame, FRAME_POOL_TAG);
            }
            break;
        }
        unsafe {
            ptr::write_unaligned(
                nbl.add(NBL_SIDE_FRAME_AT) as *mut *mut u8,
                frame,
            );
            ptr::write_unaligned(nbl.add(NBL_SIDE_MDL_AT) as *mut *mut u8, mdl);
        }
        if head.is_null() {
            head = nbl;
            tail = nbl;
        } else {
            unsafe {
                ptr::write_unaligned(tail.add(NBL_NEXT_AT) as *mut *mut u8, nbl);
            }
            tail = nbl;
        }
        count += 1;
        adapter.rx_cursor = advance_rx(adapter.base, length, adapter.rx_cursor);
        if count >= 16 {
            break;
        }
    }
    if !head.is_null() {
        unsafe {
            NdisMIndicateReceiveNetBufferLists(adapter.ndis_handle, head, 0, count, 0);
        }
    }
    DRAINING.store(0, Ordering::Release);
}

unsafe extern "system" fn set_options(
    _handle: *mut u8,
    _context: *mut u8,
    _third: *mut u8,
    _fourth: *mut u8,
) -> u32 {
    SUCCESS
}

unsafe extern "system" fn halt(_ctx: *mut u8) {}

unsafe extern "system" fn pause(_ctx: *mut u8, _pause: *mut u8) -> u32 {
    SUCCESS
}

unsafe extern "system" fn initialize_ex(
    handle: *mut u8,
    _driver_context: *mut u8,
    _init_params: *mut u8,
) -> u32 {
    line(b"RTL8139 INIT\n\0");
    if STARTED.swap(1, Ordering::AcqRel) != 0 {
        line(b"RTL8139 INIT skip\n\0");
        return FAILURE;
    }
    let adapter = unsafe { &mut *ptr::addr_of_mut!(ADAPTER) };
    adapter.ndis_handle = handle;
    adapter.packet_filter = 0;
    adapter.rx_cursor = 0;

    let base = match find_io_base() {
        Some(base) => base,
        None => {
            line(b"RTL8139 CHECK pci none\n\0");
            return FAILURE;
        }
    };
    adapter.base = base;
    let mut index = 0;
    while index < 6 {
        adapter.mac[index] = read_port(base + R_MAC + index as u16);
        index += 1;
    }
    if !power_and_reset(base) {
        return FAILURE;
    }
    let receive = arm_receive(base);
    if receive.is_null() {
        return RESOURCES;
    }
    adapter.receive = receive;
    if !prepare_tx(adapter) {
        return RESOURCES;
    }

    let mut attrs = [0u8; ATTR_REG_SIZE];
    attrs[0] = 0x9E;
    attrs[1] = 1;
    put_u16(&mut attrs, 2, ATTR_REG_SIZE as u16);
    put_ptr(&mut attrs, ATTR_CONTEXT_AT, adapter as *mut Adapter as usize);
    put_u32(&mut attrs, ATTR_FLAGS_AT, ATTR_FLAGS_VAL);
    put_u32(&mut attrs, ATTR_CHECK_HANG_AT, ATTR_CHECK_HANG);
    put_u32(&mut attrs, ATTR_IFACE_AT, ATTR_IFACE_PCI);
    let attr_status = NdisMSetMiniportAttributes(handle, attrs.as_ptr());
    line_status(b"RTL8139 CHECK attr %08x\n\0", attr_status);
    if attr_status != 0 {
        return attr_status;
    }

    let mut pool_params = [0u8; POOL_PARAMS_BYTES];
    pool_params[0] = 0x80;
    pool_params[1] = 1;
    put_u16(&mut pool_params, 2, POOL_PARAMS_BYTES as u16);
    pool_params[5] = 1;
    let pool = NdisAllocateNetBufferListPool(handle, pool_params.as_ptr());
    if pool.is_null() {
        line(b"RTL8139 CHECK pool none\n\0");
        return RESOURCES;
    }
    adapter.nbl_pool = pool;

    let mut irq_chars = [0u8; IRQ_CHARS_BYTES];
    irq_chars[0] = 0x81;
    irq_chars[1] = 1;
    put_u16(&mut irq_chars, 2, IRQ_CHARS_BYTES as u16);
    put_ptr(&mut irq_chars, IRQ_CHARS_ISR_AT, miniport_isr as *const () as usize);
    put_ptr(&mut irq_chars, IRQ_CHARS_DPC_AT, miniport_dpc as *const () as usize);
    put_ptr(
        &mut irq_chars,
        IRQ_CHARS_DISABLE_AT,
        disable_interrupt as *const () as usize,
    );
    put_ptr(
        &mut irq_chars,
        IRQ_CHARS_ENABLE_AT,
        enable_interrupt as *const () as usize,
    );
    let mut irq_handle: *mut u8 = ptr::null_mut();
    let irq_status = NdisMRegisterInterruptEx(
        handle,
        adapter as *mut Adapter as *mut u8,
        irq_chars.as_mut_ptr(),
        &mut irq_handle,
    );
    line_status(b"RTL8139 CHECK irq %08x\n\0", irq_status);
    if irq_status != 0 {
        return irq_status;
    }
    line(b"RTL8139 INIT ok\n\0");
    SUCCESS
}

unsafe extern "system" fn restart(_ctx: *mut u8, _params: *mut u8) -> u32 {
    line(b"RTL8139 RESTART\n\0");
    SUCCESS
}

unsafe extern "system" fn oid_request(_ctx: *mut u8, request: *mut u8) -> u32 {
    if request.is_null() {
        return FAILURE;
    }
    let request_type = read_u32(request, REQ_TYPE_AT);
    let oid = read_u32(request, REQ_OID_AT);
    let buflen = read_u32(request, REQ_LENGTH_AT);
    let buffer = read_ptr(request, REQ_BUFFER_AT);
    let adapter = unsafe { &*ptr::addr_of!(ADAPTER) };

    if request_type == 1 {
        if oid == OID_GEN_CURRENT_PACKET_FILTER {
            if buffer.is_null() || buflen < 4 {
                return FAILURE;
            }
            let filter = read_u32(buffer, 0);
            unsafe {
                let live = &mut *ptr::addr_of_mut!(ADAPTER);
                live.packet_filter = filter;
                if live.base != 0 {
                    let mut rc = RC_VAL;
                    if (filter & NDIS_PACKET_TYPE_PROMISCUOUS) != 0 {
                        rc |= B_RC_AAP | B_RC_APM | B_RC_AM | B_RC_AB;
                    } else {
                        if (filter & NDIS_PACKET_TYPE_DIRECTED) != 0 {
                            rc |= B_RC_APM;
                        }
                        if (filter
                            & (NDIS_PACKET_TYPE_MULTICAST | NDIS_PACKET_TYPE_ALL_MULTICAST))
                            != 0
                        {
                            rc |= B_RC_AM;
                            write_port_u32(live.base + R_MAR0, 0xFFFF_FFFF);
                            write_port_u32(live.base + R_MAR0 + 4, 0xFFFF_FFFF);
                        }
                        if (filter & NDIS_PACKET_TYPE_BROADCAST) != 0 {
                            rc |= B_RC_AB;
                        }
                    }
                    write_port_u32(live.base + R_RC, rc);
                }
            }
            line_status(b"RTL8139 OID filter %08x\n\0", filter);
            return SUCCESS;
        }
        if oid == OID_GEN_CURRENT_LOOKAHEAD {
            /* lan.c BindAdapter sets lookahead before NDIS6 LAN.
             * output/ndis-disk-20261116T200930Z.log: SET 0001010f returned
             * c00000bb then NDIS6 LAN 00010003. */
            return SUCCESS;
        }
        return NOT_SUPPORTED;
    }
    if request_type != 0 {
        return NOT_SUPPORTED;
    }
    if buffer.is_null() {
        return FAILURE;
    }

    match oid {
        OID_802_3_CURRENT_ADDRESS => {
            if buflen < 6 {
                return FAILURE;
            }
            unsafe { ptr::copy_nonoverlapping(adapter.mac.as_ptr(), buffer, 6) };
        }
        OID_GEN_MAXIMUM_FRAME_SIZE => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, 1500) };
        }
        OID_GEN_MAXIMUM_TOTAL_SIZE => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, 1514) };
        }
        OID_GEN_LINK_SPEED => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, 1_000_000) };
        }
        OID_GEN_MEDIA_CONNECT_STATUS => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, NDIS_MEDIA_CONNECTED) };
        }
        OID_GEN_CURRENT_PACKET_FILTER => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, adapter.packet_filter) };
        }
        OID_GEN_CURRENT_LOOKAHEAD => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe {
                ptr::write_unaligned(
                    buffer as *mut u32,
                    (MAXIMUM_FRAME_SIZE - ETH_HEADER) as u32,
                );
            }
        }
        OID_GEN_MAXIMUM_SEND_PACKETS => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, 1) };
        }
        OID_GEN_HARDWARE_STATUS => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, NDIS_HW_STATUS_READY) };
        }
        OID_GEN_MEDIA_SUPPORTED | OID_GEN_MEDIA_IN_USE => {
            if buflen < 4 {
                return FAILURE;
            }
            unsafe { ptr::write_unaligned(buffer as *mut u32, NDIS_MEDIUM_802_3) };
        }
        OID_GEN_VENDOR_DESCRIPTION => {
            const TEXT: &[u8] = b"VerOS RTL8139";
            let copy = TEXT.len().min(buflen as usize);
            if copy == 0 {
                return FAILURE;
            }
            unsafe {
                ptr::copy_nonoverlapping(TEXT.as_ptr(), buffer, copy);
            }
        }
        _ => return NOT_SUPPORTED,
    }
    SUCCESS
}

unsafe extern "system" fn send_net_buffer_lists(
    _ctx: *mut u8,
    lists: *mut u8,
    _port: u32,
    _flags: u32,
) {
    let adapter = unsafe { &*ptr::addr_of!(ADAPTER) };
    let mut nbl = lists;
    while !nbl.is_null() {
        let next = read_ptr(nbl, NBL_NEXT_AT);
        let nb = read_ptr(nbl, NBL_FIRST_NB_AT);
        if let Some((va, offset, length)) = frame_va_from_nb(nbl, nb) {
            let frame = unsafe { va.add(offset as usize) };
            let _ok = transmit_frame(adapter, frame, length);
        }
        nbl = next;
    }
    unsafe {
        NdisMSendNetBufferListsComplete(adapter.ndis_handle, lists, 0);
    }
}

unsafe extern "system" fn return_net_buffer_lists(_ctx: *mut u8, lists: *mut u8, _flags: u32) {
    let mut nbl = lists;
    while !nbl.is_null() {
        let next = read_ptr(nbl, NBL_NEXT_AT);
        free_indicated_nbl(nbl);
        nbl = next;
    }
}

unsafe extern "system" fn miniport_isr(
    _ctx: *mut u8,
    queue_dpc: *mut u8,
    _targets: *mut u32,
) -> u8 {
    let adapter = unsafe { &*ptr::addr_of!(ADAPTER) };
    if adapter.base == 0 {
        return 0;
    }
    let status = read_port_u16(adapter.base + R_ISR);
    if (status & I_MASK) == 0 {
        return 0;
    }
    write_port_u16(adapter.base + R_ISR, status & I_MASK);
    if !queue_dpc.is_null() {
        unsafe { *queue_dpc = 1 };
    }
    1
}

unsafe extern "system" fn miniport_dpc(
    _ctx: *mut u8,
    _dpc_context: *mut u8,
    _throttle: *mut u8,
    _reserved: *mut u8,
) {
    let adapter = unsafe { &mut *ptr::addr_of_mut!(ADAPTER) };
    drain_rx(adapter);
}

unsafe extern "system" fn disable_interrupt(_ctx: *mut u8) {
    let adapter = unsafe { &*ptr::addr_of!(ADAPTER) };
    if adapter.base != 0 {
        write_port_u16(adapter.base + R_IMR, 0);
    }
}

unsafe extern "system" fn enable_interrupt(_ctx: *mut u8) {
    let adapter = unsafe { &*ptr::addr_of!(ADAPTER) };
    if adapter.base != 0 {
        write_port_u16(adapter.base + R_IMR, I_MASK);
    }
}

#[no_mangle]
pub unsafe extern "system" fn DriverEntry(driver: *mut u8, registry: *mut u8) -> i32 {
  let mut chars = [0u8; MP_CHARS];
    chars[0] = 0x8A;
    chars[1] = 3;
    put_u16(&mut chars, 2, MP_CHARS as u16);
    chars[4] = 6;
    chars[5] = 84;
    put_ptr(&mut chars, SET_OPTIONS_SLOT, set_options as *const () as usize);
    put_ptr(&mut chars, INIT_SLOT, initialize_ex as *const () as usize);
    put_ptr(&mut chars, HALT_SLOT, halt as *const () as usize);
    put_ptr(&mut chars, 0x28, halt as *const () as usize);
    put_ptr(&mut chars, PAUSE_SLOT, pause as *const () as usize);
    put_ptr(&mut chars, RESTART_SLOT, restart as *const () as usize);
    put_ptr(&mut chars, OID_SLOT, oid_request as *const () as usize);
    put_ptr(&mut chars, SEND_SLOT, send_net_buffer_lists as *const () as usize);
    put_ptr(
        &mut chars,
        RETURN_SLOT,
        return_net_buffer_lists as *const () as usize,
    );

    let mut handle: *mut u8 = ptr::null_mut();
    let registered = NdisMRegisterMiniportDriver(
        driver,
        registry,
        ptr::null_mut(),
        chars.as_ptr(),
        &mut handle,
    );
    line_status(b"RTL8139 CHECK register %08x\n\0", registered);
    if registered != 0 {
        line(b"RTL8139 CHECK FAIL\n\0");
        return registered as i32;
    }
    line(b"RTL8139 CHECK OK\n\0");
    0
}