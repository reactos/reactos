/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/rtl8139n/src/lib.rs
 * PURPOSE:     Register an RTL8139 miniport with the measured NDIS header
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * The C driver in drivers/network/dd/rtl8139 stays in place. This file
 * calls the NDIS exports the live CD already implements. The receive
 * buffer comes from MmAllocateContiguousMemory and MmGetPhysicalAddress
 * in sdk/include/xdk/mmfuncs.h. The same header declares MmMapIoSpace.
 * output/ndis-discovery/windows.log (DIFF END OK) accepts a miniport of
 * type 0x8A, revision 3, size 0x120, with bytes 6 and 84 at offset 4,
 * and calls the pointer at offset 0x10. Capture 2026-11-15T143730Z
 * reached DIFF END OK: type 0x9E returns 0 from
 * NdisMSetMiniportAttributes. Capture 2026-11-15T173703Z printed
 * NDIS CHECK OK for that same pair of calls.
 * drivers/network/dd/rtl8139/netrtl.inf names PCI\VEN_10EC&DEV_8139.
 * sdk/include/xdk/iotypes.h defines PCI_SLOT_NUMBER, PCIConfiguration,
 * PCI_COMMON_HDR_LENGTH, PCI_INVALID_VENDORID, PCI_ADDRESS_IO_SPACE, and
 * PCI_ADDRESS_IO_ADDRESS_MASK. type0.BaseAddresses starts at byte 0x10 of
 * PCI_COMMON_HEADER. sdk/include/xdk/halfuncs.h declares HalGetBusData and
 * HalSetBusDataByOffset. iotypes.h defines PCI_ENABLE_BUS_MASTER as 0x0004.
 * drivers/network/dd/rtl8139/rtlhw.h defines R_MAC as 0x00, R_CMD as 0x37,
 * B_CMD_RST as 0x10, R_CFG1 as 0x52, R_MS as 0x58, R_MS_LINKDWN as 0x04,
 * and R_MS_SPEED_10 as 0x08. rtlhw.h also defines R_RXSA as 0x30, R_TC as
 * 0x40, R_RC as 0x44, B_CMD_TXE as 0x04, and B_CMD_RXE as 0x08.
 * drivers/network/dd/rtl8139/hardware.c writes 0 to R_CFG1, writes
 * B_CMD_RST to R_CMD until that bit clears, NICUpdateLinkStatus reads
 * R_MS, NICRegisterReceiveBuffer writes the low 32 bits of the buffer
 * address to R_RXSA, and NICEnableTxRx writes B_CMD_TXE | B_CMD_RXE
 * before RC_VAL and TC_VAL. NICTransmitPacket writes the physical
 * address to R_TXSAD0 and the length to R_TXSTS0. drivers/network/dd/rtl8139/interrupt.c
 * reads R_CMD, and when R_CMD_RXEMPTY is clear it reads PACKET_HEADER at the
 * receive offset, then writes the aligned offset minus 0x10 to R_CAPR.
 * At offset 0 that write is 0xFFF0. R_IS is 0x3E. R_MPC is 0x4C.
 * The port writes clobber memory so the frame stores finish first.
 * sdk/include/xdk/kefuncs.h declares KeDelayExecutionThread. The receive
 * check waits 200 milliseconds, counted in 100 ns units, so a reply can arrive.
 * rtlhw.h PACKET_HEADER is a USHORT status, then a USHORT length.
 * RSR_ROK is 0x0001. NICApplyPacketFilter ORs B_RC_APM and B_RC_AB onto
 * RC_VAL. B_RC_APM is 0x02 and B_RC_AB is 0x08. R_CAPR is 0x38.
 * R_CMD_RXEMPTY is 0x01. The frame is an ARP request.
 * drivers/network/tcpip/lwip/src/include/lwip/prot/etharp.h defines
 * etharp_hdr and ARP_REQUEST 1. SIZEOF_ETHARP_HDR is 28.
 * drivers/network/tcpip/lwip/src/include/lwip/prot/ieee.h defines
 * ETHTYPE_ARP 0x0806 and ETHTYPE_IP 0x0800.
 * drivers/network/tcpip/lwip/src/include/lwip/prot/iana.h defines
 * LWIP_IANA_HWTYPE_ETHERNET as 1.
 * https://www.qemu.org/docs/master/system/devices/net.html gives the user
 * network guest 10.0.2.15 and the virtual host 10.0.2.2.
 * ip.h, ip4.h, and icmp.h name the IPv4 and ICMP echo headers.
 * opt.h sets IP_DEFAULT_TTL to 255. The checksum is the ones' complement
 * sum in RFC 1071, https://www.rfc-editor.org/rfc/rfc1071.
 * drivers/network/dd/rtl8139/ndis.c
 * sends a buffer whose high half is 0, whose address is 4-byte aligned, and
 * whose length is from MINIMUM_FRAME_SIZE 60 through MAXIMUM_FRAME_SIZE 1514.
 * rtlhw.h defines R_TXSTS0 as 0x10, R_TXSAD0 as 0x20, and R_TXS_STATOK as
 * 0x00008000. drivers/network/dd/rtl8139/nic.h sets
 * MAX_RESET_ATTEMPTS to 25, FULL_RECEIVE_BUFFER_SIZE to 32768 + 16 + 2048,
 * RC_VAL to 0x800B780, TC_VAL to 0x700, MINIMUM_FRAME_SIZE to 60, and
 * MAXIMUM_FRAME_SIZE to 1514. CHECK_TX_READS and CHECK_RX_READS are this
 * driver's status read counts, not register fields.
 * output/rtl8139n-20261115T182029Z.log printed RTL8139 CHECK bar 0000c001
 * and mac bytes 52 54 00 12 34 56. output/rtl8139n-20261115T183403Z.log
 * printed RTL8139 CHECK cmd 01 and RTL8139 CHECK reset ok.
 * output/rtl8139n-20261115T184127Z.log printed RTL8139 CHECK media d0,
 * RTL8139 CHECK link up, and RTL8139 CHECK speed 100.
 * output/rtl8139n-20261115T185118Z.log printed RTL8139 CHECK rxpa 00208000,
 * RTL8139 CHECK txrx 0d, and RTL8139 CHECK txrx ok.
 * output/rtl8139n-20261115T185912Z.log printed RTL8139 CHECK txst 0000a03c
 * and RTL8139 CHECK tx ok.
 * output/rtl8139n-20261115T201701Z.log printed RTL8139 CHECK cmdreg 0107,
 * RTL8139 CHECK isr 0005, RTL8139 CHECK rxst 4001, RTL8139 CHECK rxlen 0044,
 * and RTL8139 CHECK rx ok.
 * output/rtl8139n-20261115T203413Z.log printed RTL8139 CHECK peer 0a000202,
 * RTL8139 CHECK rxtype 0800, RTL8139 CHECK icmp 00, and RTL8139 CHECK ping ok.
 * output/rtl8139n-20261115T204612Z.log printed RTL8139 CHECK returned, then
 * RTL8139 CHECK running, RTL8139 CHECK ping ok, and RTL8139 CHECK serving.
 * sdk/include/xdk/psfuncs.h declares PsCreateSystemThread. pstypes.h defines
 * THREAD_ALL_ACCESS from STANDARD_RIGHTS_REQUIRED and SYNCHRONIZE in
 * sdk/include/xdk/setypes.h, which is 0x001F0FFF. ntoskrnl.spec exports ZwClose.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiportdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
 */

#![no_std]

use core::panic::PanicInfo;
use core::ptr;
use core::sync::atomic::{AtomicU32, Ordering};

const MP_CHARS: usize = 0x120;
/* sdk/include/xdk/iotypes.h: PCIConfiguration is the fifth enumerator. */
const PCI_CONFIGURATION: u32 = 4;
const PCI_HEADER: u32 = 64;
const PCI_VENDOR: u16 = 0x10EC;
const PCI_DEVICE: u16 = 0x8139;
/* sdk/include/xdk/iotypes.h */
const PCI_ENABLE_BUS_MASTER: u16 = 0x0004;
const R_MAC: u16 = 0;
/* drivers/network/dd/rtl8139/rtlhw.h */
const R_CMD: u16 = 0x37;
const B_CMD_RST: u8 = 0x10;
const R_CFG1: u16 = 0x52;
const R_MS: u16 = 0x58;
const R_MS_LINKDWN: u8 = 0x04;
const R_MS_SPEED_10: u8 = 0x08;
const R_RXSA: u16 = 0x30;
const R_TC: u16 = 0x40;
const R_RC: u16 = 0x44;
const B_CMD_TXE: u8 = 0x04;
const B_CMD_RXE: u8 = 0x08;
const R_CMD_RXEMPTY: u8 = 0x01;
const R_CAPR: u16 = 0x38;
const R_IS: u16 = 0x3E;
const R_MPC: u16 = 0x4C;
const B_RC_APM: u32 = 0x02;
const B_RC_AB: u32 = 0x08;
const RSR_ROK: u16 = 0x0001;
/* drivers/network/tcpip/lwip/src/include/lwip/prot/etharp.h */
const ARP_REQUEST: u16 = 1;
const ARP_REPLY: u16 = 2;
const SIZEOF_ETHARP_HDR: usize = 28;
/* drivers/network/tcpip/lwip/src/include/lwip/prot/ieee.h */
const ETHTYPE_ARP: u16 = 0x0806;
const ETHTYPE_IP: u16 = 0x0800;
/* drivers/network/tcpip/lwip/src/include/lwip/prot/iana.h */
const HWTYPE_ETHERNET: u16 = 1;
/* drivers/network/tcpip/lwip/src/include/lwip/prot/ip.h */
const IP_PROTO_ICMP: u8 = 1;
/* drivers/network/tcpip/lwip/src/include/lwip/prot/ip4.h */
const IP_HLEN: usize = 20;
/* drivers/network/tcpip/lwip/src/include/lwip/opt.h */
const IP_DEFAULT_TTL: u8 = 255;
/* drivers/network/tcpip/lwip/src/include/lwip/prot/icmp.h */
const ICMP_ER: u8 = 0;
const ICMP_ECHO: u8 = 8;
const ICMP_HLEN: usize = 8;
/* drivers/network/dd/rtl8139/nic.h */
const RECEIVE_BUFFER_SIZE: usize = 32768;
/* https://www.qemu.org/docs/master/system/devices/net.html */
const GUEST_IP: [u8; 4] = [10, 0, 2, 15];
const HOST_IP: [u8; 4] = [10, 0, 2, 2];
/* drivers/network/dd/rtl8139/nic.h */
const MAX_RESET_ATTEMPTS: u32 = 25;
const FULL_RECEIVE_BUFFER_SIZE: usize = 32768 + 16 + 2048;
const RC_VAL: u32 = 0x800B780;
const TC_VAL: u32 = 0x700;
const MINIMUM_FRAME_SIZE: usize = 60;
const MAXIMUM_FRAME_SIZE: usize = 1514;
const R_TXSTS0: u16 = 0x10;
const R_TXSAD0: u16 = 0x20;
const R_TXS_STATOK: u32 = 0x0000_8000;
/* This driver's status read count, not a register field. */
const CHECK_TX_READS: u32 = 100_000;
const CHECK_RX_READS: u32 = 100_000;
/* drivers/network/dd/rtl8139/ndis.c rejects a physical address whose HighPart is not 0. */
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
    fn MmMapIoSpace(physical: u64, bytes: usize, cache: u32) -> *mut u8;
    fn KeDelayExecutionThread(wait_mode: i32, alertable: u8, interval: *const i64) -> i32;
    fn PsCreateSystemThread(
        handle: *mut *mut u8,
        access: u32,
        object: *mut u8,
        process: *mut u8,
        client: *mut u8,
        start: unsafe extern "system" fn(*mut u8),
        context: *mut u8,
    ) -> i32;
    fn ZwClose(handle: *mut u8) -> i32;
}

/* sdk/include/xdk/pstypes.h and setypes.h. */
const THREAD_ALL_ACCESS: u32 = 0x001F_0FFF;

struct Live {
    base: u16,
    mac: [u8; 6],
    receive: *mut u8,
}

static mut LIVE: Live = Live {
    base: 0,
    mac: [0; 6],
    receive: ptr::null_mut(),
};
static GO: AtomicU32 = AtomicU32::new(0);

fn line(text: &[u8]) {
    unsafe { DbgPrint(text.as_ptr()); }
}

fn line_status(text: &[u8], status: u32) {
    unsafe { DbgPrint(text.as_ptr(), status); }
}

unsafe extern "system" fn set_options(
    _handle: *mut u8,
    _context: *mut u8,
    _third: *mut u8,
    _fourth: *mut u8,
) -> u32 {
    0
}

fn put_u16(buf: &mut [u8], at: usize, value: u16) {
    let bytes = value.to_le_bytes();
    buf[at] = bytes[0];
    buf[at + 1] = bytes[1];
}

fn put_ptr(buf: &mut [u8], at: usize, value: usize) {
    let bytes = value.to_le_bytes();
    buf[at..at + bytes.len()].copy_from_slice(&bytes);
}

fn write_port(port: u16, value: u8) {
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

fn read_port_u16(port: u16) -> u16 {
    let value: u16;
    unsafe {
        core::arch::asm!(
            "in ax, dx",
            in("dx") port,
            out("ax") value,
            options(nostack, preserves_flags)
        );
    }
    value
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
    /* PCI_SLOT_NUMBER: device is 5 bits, function is the next 3. */
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
                /* PCI_COMMON_HEADER type0.BaseAddresses[0]. */
                let bar = u32::from_le_bytes([header[0x10], header[0x11], header[0x12], header[0x13]]);
                line_status(b"RTL8139 CHECK bar %08x\n\0", bar);
                /* PCI_ADDRESS_IO_SPACE and PCI_ADDRESS_IO_ADDRESS_MASK. */
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
                line_status(b"RTL8139 CHECK cmdreg %04x\n\0", command as u32);
                return Some((bar & 0xFFFF_FFFC) as u16);
            }
        }
    }
    None
}

fn power_and_reset(base: u16) -> u8 {
    /* hardware.c NICPowerOn, then NICSoftReset. */
    write_port(base + R_CFG1, 0);
    write_port(base + R_CMD, B_CMD_RST);
    let mut command = B_CMD_RST;
    let mut attempt = 0;
    while attempt < MAX_RESET_ATTEMPTS {
        command = read_port(base + R_CMD);
        if command & B_CMD_RST == 0 {
            break;
        }
        attempt += 1;
    }
    command
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
        line(b"RTL8139 CHECK rx high\n\0");
        return ptr::null_mut();
    }
    unsafe { ptr::write_bytes(buffer, 0, FULL_RECEIVE_BUFFER_SIZE); }
    /* hardware.c NICRegisterReceiveBuffer, then NICEnableTxRx. */
    write_port_u32(base + R_RXSA, physical as u32);
    write_port(base + R_CMD, B_CMD_TXE | B_CMD_RXE);
    /* NICApplyPacketFilter for directed and broadcast. */
    write_port_u32(base + R_RC, RC_VAL | B_RC_APM | B_RC_AB);
    write_port_u32(base + R_TC, TC_VAL);
    line_status(b"RTL8139 CHECK tc %08x\n\0", read_port_u32(base + R_TC));
    /* interrupt.c writes ReceiveOffset - 0x10. Offset 0 is 0xFFF0. */
    write_port_u16(base + R_CAPR, 0u16.wrapping_sub(0x10));
    let command = read_port(base + R_CMD);
    line_status(b"RTL8139 CHECK txrx %02x\n\0", command as u32);
    if (command & (B_CMD_TXE | B_CMD_RXE)) == (B_CMD_TXE | B_CMD_RXE) {
        line(b"RTL8139 CHECK txrx ok\n\0");
        buffer
    } else {
        line(b"RTL8139 CHECK txrx fail\n\0");
        ptr::null_mut()
    }
}

fn put_be16(buffer: *mut u8, at: usize, value: u16) {
    let bytes = value.to_be_bytes();
    unsafe {
        *buffer.add(at) = bytes[0];
        *buffer.add(at + 1) = bytes[1];
    }
}

fn send_one(base: u16, mac: &[u8; 6]) {
    let buffer = unsafe { MmAllocateContiguousMemory(MAXIMUM_FRAME_SIZE, HIGHEST_PA) };
    if buffer.is_null() {
        line(b"RTL8139 CHECK tx none\n\0");
        return;
    }
    let physical = unsafe { MmGetPhysicalAddress(buffer) };
    if physical > HIGHEST_PA || (physical & 3) != 0 {
        line(b"RTL8139 CHECK tx align\n\0");
        return;
    }
    unsafe { ptr::write_bytes(buffer, 0, MINIMUM_FRAME_SIZE); }
    let mut index = 0;
    while index < mac.len() {
        unsafe {
            /* Broadcast destination, accepted by B_RC_AB. */
            *buffer.add(index) = 0xFF;
            *buffer.add(6 + index) = mac[index];
            *buffer.add(22 + index) = mac[index];
        }
        index += 1;
    }
    put_be16(buffer, 12, ETHTYPE_ARP);
    put_be16(buffer, 14, HWTYPE_ETHERNET);
    put_be16(buffer, 16, ETHTYPE_IP);
    unsafe {
        *buffer.add(18) = 6;
        *buffer.add(19) = 4;
    }
    put_be16(buffer, 20, ARP_REQUEST);
    index = 0;
    while index < 4 {
        unsafe {
            *buffer.add(28 + index) = GUEST_IP[index];
            *buffer.add(38 + index) = HOST_IP[index];
        }
        index += 1;
    }
    let frame_len = 14 + SIZEOF_ETHARP_HDR;
    if frame_len > MINIMUM_FRAME_SIZE {
        line(b"RTL8139 CHECK tx none\n\0");
        return;
    }
    let ethertype = u16::from_be_bytes([unsafe { *buffer.add(12) }, unsafe { *buffer.add(13) }]);
    line_status(b"RTL8139 CHECK txpa %08x\n\0", physical as u32);
    line_status(b"RTL8139 CHECK txtype %04x\n\0", ethertype as u32);
    let mapped = unsafe { MmMapIoSpace(physical, MINIMUM_FRAME_SIZE, 0) };
    if mapped.is_null() {
        line(b"RTL8139 CHECK io none\n\0");
    } else {
        let mapped_type = u16::from_be_bytes([unsafe { *mapped.add(12) }, unsafe { *mapped.add(13) }]);
        line_status(b"RTL8139 CHECK iotype %04x\n\0", mapped_type as u32);
    }
    /* hardware.c NICTransmitPacket, descriptor 0. */
    write_port_u32(base + R_TXSAD0, physical as u32);
    line_status(b"RTL8139 CHECK txrd %08x\n\0", read_port_u32(base + R_TXSAD0));
    write_port_u32(base + R_TXSAD0, physical as u32);
    write_port_u32(base + R_TXSTS0, MINIMUM_FRAME_SIZE as u32);
    let mut status = 0u32;
    let mut read = 0u32;
    while read < CHECK_TX_READS {
        status = read_port_u32(base + R_TXSTS0);
        if (status & R_TXS_STATOK) != 0 {
            break;
        }
        read += 1;
    }
    line_status(b"RTL8139 CHECK txst %08x\n\0", status);
    if (status & R_TXS_STATOK) != 0 {
        line(b"RTL8139 CHECK tx ok\n\0");
    } else {
        line(b"RTL8139 CHECK tx wait\n\0");
    }
}

fn checksum(data: *const u8, len: usize) -> u16 {
    /* RFC 1071 ones' complement sum. */
    let mut sum = 0u32;
    let mut index = 0;
    while index + 1 < len {
        let word = u16::from_be_bytes([
            unsafe { *data.add(index) },
            unsafe { *data.add(index + 1) },
        ]);
        sum += word as u32;
        index += 2;
    }
    if index < len {
        sum += (unsafe { *data.add(index) } as u32) << 8;
    }
    while (sum >> 16) != 0 {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    !(sum as u16)
}

fn transmit(base: u16, physical: u64, desc: u16) {
    /* ndis.c advances CurrentTxDesc after NICTransmitPacket. */
    write_port_u32(base + R_TXSAD0 + (desc * 4), physical as u32);
    write_port_u32(base + R_TXSTS0 + (desc * 4), MINIMUM_FRAME_SIZE as u32);
    let mut status = 0u32;
    let mut read = 0u32;
    while read < CHECK_TX_READS {
        status = read_port_u32(base + R_TXSTS0);
        if (status & R_TXS_STATOK) != 0 {
            break;
        }
        read += 1;
    }
    line_status(b"RTL8139 CHECK txst %08x\n\0", status);
    if (status & R_TXS_STATOK) != 0 {
        line(b"RTL8139 CHECK tx ok\n\0");
    } else {
        line(b"RTL8139 CHECK tx wait\n\0");
    }
}

fn send_echo(base: u16, mac: &[u8; 6], peer_mac: &[u8; 6], peer_ip: &[u8; 4]) {
    let buffer = unsafe { MmAllocateContiguousMemory(MAXIMUM_FRAME_SIZE, HIGHEST_PA) };
    if buffer.is_null() {
        line(b"RTL8139 CHECK echo none\n\0");
        return;
    }
    let physical = unsafe { MmGetPhysicalAddress(buffer) };
    if physical > HIGHEST_PA || (physical & 3) != 0 {
        line(b"RTL8139 CHECK echo align\n\0");
        return;
    }
    unsafe { ptr::write_bytes(buffer, 0, MINIMUM_FRAME_SIZE); }
    let mut index = 0;
    while index < 6 {
        unsafe {
            *buffer.add(index) = peer_mac[index];
            *buffer.add(6 + index) = mac[index];
        }
        index += 1;
    }
    put_be16(buffer, 12, ETHTYPE_IP);
    unsafe {
        *buffer.add(14) = 0x45;
        *buffer.add(15) = 0;
    }
    put_be16(buffer, 16, (IP_HLEN + ICMP_HLEN) as u16);
    put_be16(buffer, 18, 1);
    put_be16(buffer, 20, 0);
    unsafe {
        *buffer.add(22) = IP_DEFAULT_TTL;
        *buffer.add(23) = IP_PROTO_ICMP;
    }
    index = 0;
    while index < 4 {
        unsafe {
            *buffer.add(26 + index) = GUEST_IP[index];
            *buffer.add(30 + index) = peer_ip[index];
        }
        index += 1;
    }
    let ip_sum = checksum(unsafe { buffer.add(14) }, IP_HLEN);
    put_be16(buffer, 24, ip_sum);
    unsafe {
        *buffer.add(34) = ICMP_ECHO;
        *buffer.add(35) = 0;
    }
    put_be16(buffer, 38, 1);
    put_be16(buffer, 40, 1);
    let icmp_sum = checksum(unsafe { buffer.add(34) }, ICMP_HLEN);
    put_be16(buffer, 36, icmp_sum);
    transmit(base, physical, 1);
}

fn take_receive(base: u16, buffer: *mut u8, offset: usize) -> (usize, Option<([u8; 6], [u8; 4])>) {
    /* kefuncs.h: a negative interval is relative. 200 ms is 2_000_000 units of 100 ns. */
    let interval: i64 = -2_000_000;
    unsafe { KeDelayExecutionThread(0, 0, &interval); }
    let mut command = R_CMD_RXEMPTY;
    let mut read = 0u32;
    while read < CHECK_RX_READS {
        command = read_port(base + R_CMD);
        if (command & R_CMD_RXEMPTY) == 0 {
            break;
        }
        read += 1;
    }
    line_status(b"RTL8139 CHECK rxcmd %02x\n\0", command as u32);
    line_status(b"RTL8139 CHECK isr %04x\n\0", read_port_u16(base + R_IS) as u32);
    line_status(b"RTL8139 CHECK mpc %08x\n\0", read_port_u32(base + R_MPC));
    let header = unsafe { buffer.add(offset) };
    let status = u16::from_le_bytes([unsafe { *header }, unsafe { *header.add(1) }]);
    let length = u16::from_le_bytes([unsafe { *header.add(2) }, unsafe { *header.add(3) }]);
    line_status(b"RTL8139 CHECK rxst %04x\n\0", status as u32);
    line_status(b"RTL8139 CHECK rxlen %04x\n\0", length as u32);
    if (command & R_CMD_RXEMPTY) != 0 {
        line(b"RTL8139 CHECK rx empty\n\0");
        return (offset, None);
    }
    if (status & RSR_ROK) == 0 {
        line(b"RTL8139 CHECK rx fail\n\0");
        return (offset, None);
    }
    line(b"RTL8139 CHECK rx ok\n\0");
    if (length as usize) > FULL_RECEIVE_BUFFER_SIZE {
        return (offset, None);
    }
    let mut peer = None;
    let ethertype = u16::from_be_bytes([unsafe { *header.add(16) }, unsafe { *header.add(17) }]);
    line_status(b"RTL8139 CHECK rxtype %04x\n\0", ethertype as u32);
    if ethertype == ETHTYPE_ARP {
        let opcode = u16::from_be_bytes([unsafe { *header.add(24) }, unsafe { *header.add(25) }]);
        line_status(b"RTL8139 CHECK arp %04x\n\0", opcode as u32);
        if opcode == ARP_REPLY {
            let mut peer_mac = [0u8; 6];
            let mut peer_ip = [0u8; 4];
            let mut index = 0;
            while index < 6 {
                peer_mac[index] = unsafe { *header.add(26 + index) };
                index += 1;
            }
            index = 0;
            while index < 4 {
                peer_ip[index] = unsafe { *header.add(32 + index) };
                index += 1;
            }
            line_status(
                b"RTL8139 CHECK peer %08x\n\0",
                u32::from_be_bytes(peer_ip),
            );
            peer = Some((peer_mac, peer_ip));
        }
    }
    if ethertype == ETHTYPE_IP {
        let icmp_type = unsafe { *header.add(38) };
        line_status(b"RTL8139 CHECK icmp %02x\n\0", icmp_type as u32);
        if icmp_type == ICMP_ER {
            line(b"RTL8139 CHECK ping ok\n\0");
        }
    }
    /* interrupt.c advances CAPR from the header length. */
    let mut next = (length as usize) + 4;
    next = (next + 3) & !3;
    next = (offset + next) % RECEIVE_BUFFER_SIZE;
    write_port_u16(base + R_CAPR, (next as u16).wrapping_sub(0x10));
    (next, peer)
}

fn exchange(base: u16, mac: &[u8; 6], receive: *mut u8) {
    send_one(base, mac);
    let (next, peer) = take_receive(base, receive, 0);
    if let Some((peer_mac, peer_ip)) = peer {
        send_echo(base, mac, &peer_mac, &peer_ip);
        take_receive(base, receive, next);
    }
}

unsafe extern "system" fn service(_context: *mut u8) {
    while GO.load(Ordering::Acquire) == 0 {
        let interval: i64 = -10_000;
        KeDelayExecutionThread(0, 0, &interval);
    }
    line(b"RTL8139 CHECK running\n\0");
    let live = unsafe { &mut *ptr::addr_of_mut!(LIVE) };
    exchange(live.base, &live.mac, live.receive);
    line(b"RTL8139 CHECK serving\n\0");
    loop {
        let interval: i64 = -2_000_000;
        KeDelayExecutionThread(0, 0, &interval);
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
    put_ptr(&mut chars, 0x10, set_options as *const () as usize);
    let mut handle: *mut u8 = ptr::null_mut();
    let registered = NdisMRegisterMiniportDriver(
        driver,
        registry,
        ptr::null_mut(),
        chars.as_ptr(),
        &mut handle,
    );
    line_status(b"RTL8139 CHECK register %08x\n\0", registered);
    if registered != 0 || handle.is_null() {
        line(b"RTL8139 CHECK FAIL\n\0");
        return 0;
    }
    let attributes = [0x9Eu8, 0, 4, 0];
    let accepted = NdisMSetMiniportAttributes(handle, attributes.as_ptr());
    line_status(b"RTL8139 CHECK attr %08x\n\0", accepted);
    if accepted == 0 {
        line(b"RTL8139 CHECK OK\n\0");
    } else {
        line(b"RTL8139 CHECK FAIL\n\0");
        return 0;
    }
    match find_io_base() {
        Some(base) => {
            let mut mac = [0u8; 6];
            let mut index = 0;
            while index < mac.len() {
                mac[index] = read_port(base + R_MAC + index as u16);
                index += 1;
            }
            line_status(b"RTL8139 CHECK mac0 %02x\n\0", mac[0] as u32);
            line_status(b"RTL8139 CHECK mac1 %02x\n\0", mac[1] as u32);
            line_status(b"RTL8139 CHECK mac2 %02x\n\0", mac[2] as u32);
            line_status(b"RTL8139 CHECK mac3 %02x\n\0", mac[3] as u32);
            line_status(b"RTL8139 CHECK mac4 %02x\n\0", mac[4] as u32);
            line_status(b"RTL8139 CHECK mac5 %02x\n\0", mac[5] as u32);
            let command = power_and_reset(base);
            line_status(b"RTL8139 CHECK cmd %02x\n\0", command as u32);
            if command & B_CMD_RST == 0 {
                line(b"RTL8139 CHECK reset ok\n\0");
            } else {
                line(b"RTL8139 CHECK reset busy\n\0");
            }
            let media = read_port(base + R_MS);
            line_status(b"RTL8139 CHECK media %02x\n\0", media as u32);
            if media & R_MS_LINKDWN == 0 {
                line(b"RTL8139 CHECK link up\n\0");
            } else {
                line(b"RTL8139 CHECK link down\n\0");
            }
            if media & R_MS_SPEED_10 == 0 {
                line(b"RTL8139 CHECK speed 100\n\0");
            } else {
                line(b"RTL8139 CHECK speed 10\n\0");
            }
            if command & B_CMD_RST == 0 {
                let receive = arm_receive(base);
                if !receive.is_null() {
                    let live = unsafe { &mut *ptr::addr_of_mut!(LIVE) };
                    live.base = base;
                    live.mac = mac;
                    live.receive = receive;
                    let mut handle: *mut u8 = ptr::null_mut();
                    let started = PsCreateSystemThread(
                        &mut handle,
                        THREAD_ALL_ACCESS,
                        ptr::null_mut(),
                        ptr::null_mut(),
                        ptr::null_mut(),
                        service,
                        ptr::null_mut(),
                    );
                    line_status(b"RTL8139 CHECK thread %08x\n\0", started as u32);
                    if started == 0 {
                        ZwClose(handle);
                        line(b"RTL8139 CHECK returned\n\0");
                        GO.store(1, Ordering::Release);
                    } else {
                        exchange(base, &mac, receive);
                    }
                }
            }
        }
        None => line(b"RTL8139 CHECK pci none\n\0"),
    }
    0
}
