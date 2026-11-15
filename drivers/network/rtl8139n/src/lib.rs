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
 * only calls the NDIS exports the live CD already implements.
 * output/ndis-discovery/windows.log (DIFF END OK) accepts a miniport of
 * type 0x8A, revision 3, size 0x120, with bytes 6 and 84 at offset 4,
 * and calls the pointer at offset 0x10. Capture 2026-11-15T143730Z
 * reached DIFF END OK: type 0x9E returns 0 from
 * NdisMSetMiniportAttributes. Capture 2026-11-15T173703Z printed
 * NDIS CHECK OK for that same pair of calls.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiportdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
 */

#![no_std]

use core::panic::PanicInfo;
use core::ptr;

const MP_CHARS: usize = 0x120;

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
    }
    0
}
