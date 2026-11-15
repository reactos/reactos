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
 * in sdk/include/xdk/mmfuncs.h.
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
 * PCI_COMMON_HEADER. sdk/include/xdk/halfuncs.h declares HalGetBusData.
 * drivers/network/dd/rtl8139/rtlhw.h defines R_MAC as 0x00, R_CMD as 0x37,
 * B_CMD_RST as 0x10, R_CFG1 as 0x52, R_MS as 0x58, R_MS_LINKDWN as 0x04,
 * and R_MS_SPEED_10 as 0x08. rtlhw.h also defines R_RXSA as 0x30, R_TC as
 * 0x40, R_RC as 0x44, B_CMD_TXE as 0x04, and B_CMD_RXE as 0x08.
 * drivers/network/dd/rtl8139/hardware.c writes 0 to R_CFG1, writes
 * B_CMD_RST to R_CMD until that bit clears, NICUpdateLinkStatus reads
 * R_MS, NICRegisterReceiveBuffer writes the low 32 bits of the buffer
 * address to R_RXSA, and NICEnableTxRx writes B_CMD_TXE | B_CMD_RXE
 * before RC_VAL and TC_VAL. drivers/network/dd/rtl8139/nic.h sets
 * MAX_RESET_ATTEMPTS to 25, FULL_RECEIVE_BUFFER_SIZE to 32768 + 16 + 2048,
 * RC_VAL to 0x800B780, and TC_VAL to 0x700.
 * output/rtl8139n-20261115T182029Z.log printed RTL8139 CHECK bar 0000c001
 * and mac bytes 52 54 00 12 34 56. output/rtl8139n-20261115T183403Z.log
 * printed RTL8139 CHECK cmd 01 and RTL8139 CHECK reset ok.
 * output/rtl8139n-20261115T184127Z.log printed RTL8139 CHECK media d0,
 * RTL8139 CHECK link up, and RTL8139 CHECK speed 100.
 * output/rtl8139n-20261115T185118Z.log printed RTL8139 CHECK rxpa 00208000,
 * RTL8139 CHECK txrx 0d, and RTL8139 CHECK txrx ok.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiportdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
 */

#![no_std]

use core::panic::PanicInfo;
use core::ptr;

const MP_CHARS: usize = 0x120;
/* sdk/include/xdk/iotypes.h: PCIConfiguration is the fifth enumerator. */
const PCI_CONFIGURATION: u32 = 4;
const PCI_HEADER: u32 = 64;
const PCI_VENDOR: u16 = 0x10EC;
const PCI_DEVICE: u16 = 0x8139;
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
/* drivers/network/dd/rtl8139/nic.h */
const MAX_RESET_ATTEMPTS: u32 = 25;
const FULL_RECEIVE_BUFFER_SIZE: usize = 32768 + 16 + 2048;
const RC_VAL: u32 = 0x800B780;
const TC_VAL: u32 = 0x700;
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
    fn MmAllocateContiguousMemory(bytes: usize, highest: u64) -> *mut u8;
    fn MmGetPhysicalAddress(base: *mut u8) -> u64;
}

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
            options(nostack, nomem, preserves_flags)
        );
    }
}

fn write_port_u32(port: u16, value: u32) {
    unsafe {
        core::arch::asm!(
            "out dx, eax",
            in("dx") port,
            in("eax") value,
            options(nostack, nomem, preserves_flags)
        );
    }
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

fn arm_receive(base: u16) {
    let buffer = unsafe { MmAllocateContiguousMemory(FULL_RECEIVE_BUFFER_SIZE, HIGHEST_PA) };
    if buffer.is_null() {
        line(b"RTL8139 CHECK rx none\n\0");
        return;
    }
    let physical = unsafe { MmGetPhysicalAddress(buffer) };
    line_status(b"RTL8139 CHECK rxpa %08x\n\0", physical as u32);
    if physical > HIGHEST_PA {
        line(b"RTL8139 CHECK rx high\n\0");
        return;
    }
    unsafe { ptr::write_bytes(buffer, 0, FULL_RECEIVE_BUFFER_SIZE); }
    /* hardware.c NICRegisterReceiveBuffer, then NICEnableTxRx. */
    write_port_u32(base + R_RXSA, physical as u32);
    write_port(base + R_CMD, B_CMD_TXE | B_CMD_RXE);
    write_port_u32(base + R_RC, RC_VAL);
    write_port_u32(base + R_TC, TC_VAL);
    let command = read_port(base + R_CMD);
    line_status(b"RTL8139 CHECK txrx %02x\n\0", command as u32);
    if (command & (B_CMD_TXE | B_CMD_RXE)) == (B_CMD_TXE | B_CMD_RXE) {
        line(b"RTL8139 CHECK txrx ok\n\0");
    } else {
        line(b"RTL8139 CHECK txrx fail\n\0");
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
                arm_receive(base);
            }
        }
        None => line(b"RTL8139 CHECK pci none\n\0"),
    }
    0
}
