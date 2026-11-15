/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/check.rs
 * PURPOSE:     Run the measured NDIS calls when the driver loads
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * DriverEntry calls this once. The serial line NDIS CHECK OK means the
 * miniport attribute accept and both indicate dispatches matched the
 * captures below. A failed step prints NDIS CHECK FAIL and the boot
 * continues. The list address is a byte in this file. It is the check's
 * marker, not a Windows object.
 * Capture 2026-11-15T143730Z: type 0x9E returns 0. Other types at size 4
 * returned 0xC000000D.
 * Capture 2026-11-15T161901Z: miniport indicate delivers the list to
 * offset 0x50. Argument 0 was null.
 * Capture 2026-11-15T160339Z: filter indicate delivers the list to
 * offset 0x88. Argument 0 was null.
 * Capture 2026-11-15T161622Z: with offset 0x88 empty, the list arrives
 * at offset 0x90. Argument 0 was null.
 * differential-tests/ndis-discovery/METHOD.md records the registration
 * headers and that this check is the VerOS result for those paths.
 * output/ndis-discovery/windows.log (DIFF END OK) accepts a protocol of
 * type 0x95, revision 2, size 0x80, and calls offsets 0x20 and 0x48.
 * A version other than bytes 6 and 84 returns 0xC0010004. A non-zero
 * value at offset 8 returns 0xC000000D.
 * output/rtl8139n-20261115T210425Z.log printed NDIS CHECK pr-version
 * c0010004, NDIS CHECK pr-flags c000000d, NDIS CHECK pr-register
 * 00000000, NDIS CHECK pr-calls 00000002, and NDIS CHECK OK.
 */

use core::ptr;
use core::sync::atomic::{AtomicPtr, AtomicU32, AtomicU8, Ordering};

use crate::filter;
use crate::protocol;
use crate::register;

unsafe extern "system" {
    fn DbgPrint(format: *const u8, ...) -> i32;
}

const MP_CHARS: usize = 0x120;
const FI_CHARS: usize = 0xF0;
const PR_CHARS: usize = 0x80;
const INVALID_PARAMETER: u32 = 0xC000_000D;
const BAD_VERSION: u32 = 0xC001_0004;

static LIST: u8 = 0x5A;
static MP_FIRST_NULL: AtomicU8 = AtomicU8::new(0);
static MP_LIST: AtomicPtr<u8> = AtomicPtr::new(ptr::null_mut());
static MP_FLAGS: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
static FI_FIRST_NULL: AtomicU8 = AtomicU8::new(0);
static FI_LIST: AtomicPtr<u8> = AtomicPtr::new(ptr::null_mut());
static FI_PORT: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
static FI_COUNT: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
static FI_FLAGS: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
static BACK_FIRST_NULL: AtomicU8 = AtomicU8::new(0);
static BACK_LIST: AtomicPtr<u8> = AtomicPtr::new(ptr::null_mut());
static BACK_FLAGS: AtomicU32 = AtomicU32::new(0xFFFF_FFFF);
static PR_CALLS: AtomicU32 = AtomicU32::new(0);

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

fn put_ptr(buf: &mut [u8], at: usize, value: usize) {
    let bytes = value.to_le_bytes();
    buf[at..at + bytes.len()].copy_from_slice(&bytes);
}

unsafe extern "system" fn mp_options(
    _handle: *mut u8,
    _context: *mut u8,
    _third: *mut u8,
    _fourth: *mut u8,
) -> u32 {
    0
}

unsafe extern "system" fn mp_indicate(first: *mut u8, list: *mut u8, flags: u32) {
    MP_FIRST_NULL.store(u8::from(first.is_null()), Ordering::Relaxed);
    MP_LIST.store(list, Ordering::Relaxed);
    MP_FLAGS.store(flags, Ordering::Relaxed);
}

unsafe extern "system" fn fi_receive(
    first: *mut u8,
    list: *mut u8,
    port: u32,
    count: u32,
    flags: u32,
) {
    FI_FIRST_NULL.store(u8::from(first.is_null()), Ordering::Relaxed);
    FI_LIST.store(list, Ordering::Relaxed);
    FI_PORT.store(port, Ordering::Relaxed);
    FI_COUNT.store(count, Ordering::Relaxed);
    FI_FLAGS.store(flags, Ordering::Relaxed);
}

unsafe extern "system" fn pr_slot(
    _handle: *mut u8,
    _context: *mut u8,
    _third: *mut u8,
    _fourth: *mut u8,
) -> u32 {
    PR_CALLS.fetch_add(1, Ordering::Relaxed);
    0
}

unsafe extern "system" fn fi_return(first: *mut u8, list: *mut u8, flags: u32) {
    BACK_FIRST_NULL.store(u8::from(first.is_null()), Ordering::Relaxed);
    BACK_LIST.store(list, Ordering::Relaxed);
    BACK_FLAGS.store(flags, Ordering::Relaxed);
}

fn list_ptr() -> *mut u8 {
    &LIST as *const u8 as *mut u8
}

fn miniport() -> bool {
    let mut chars = [0u8; MP_CHARS];
    chars[0] = 0x8A;
    chars[1] = 3;
    put_u16(&mut chars, 2, MP_CHARS as u16);
    chars[4] = 6;
    chars[5] = 84;
    put_ptr(&mut chars, 0x10, mp_options as *const () as usize);
    put_ptr(&mut chars, 0x50, mp_indicate as *const () as usize);
    let mut handle: *mut u8 = ptr::null_mut();
    let status = unsafe {
        register::NdisMRegisterMiniportDriver(
            ptr::null_mut(),
            ptr::null_mut(),
            ptr::null_mut(),
            chars.as_ptr(),
            &mut handle,
        )
    };
    line_status(b"NDIS CHECK mp-register %08x\n\0", status);
    if status != 0 || handle.is_null() {
        return false;
    }
    let rejected = [0u8; 4];
    let bad = unsafe { register::NdisMSetMiniportAttributes(handle, rejected.as_ptr()) };
    line_status(b"NDIS CHECK mp-attr-other %08x\n\0", bad);
    let accepted = [0x9Eu8, 0, 4, 0];
    let good = unsafe { register::NdisMSetMiniportAttributes(handle, accepted.as_ptr()) };
    line_status(b"NDIS CHECK mp-attr %08x\n\0", good);
    MP_LIST.store(ptr::null_mut(), Ordering::Relaxed);
    unsafe { register::NdisMIndicateReceiveNetBufferLists(handle, list_ptr(), 0, 1, 0) };
    let indicated = MP_FIRST_NULL.load(Ordering::Relaxed) == 1
        && MP_LIST.load(Ordering::Relaxed) == list_ptr()
        && MP_FLAGS.load(Ordering::Relaxed) == 0;
    if indicated {
        line(b"NDIS CHECK mp-indicate ok\n\0");
    } else {
        line(b"NDIS CHECK mp-indicate fail\n\0");
    }
    unsafe { register::NdisMDeregisterMiniportDriver(handle) };
    bad == INVALID_PARAMETER && good == 0 && indicated
}

fn filter_block(receive: bool) -> bool {
    let mut chars = [0u8; FI_CHARS];
    chars[0] = 0x8B;
    chars[1] = 3;
    put_u16(&mut chars, 2, FI_CHARS as u16);
    chars[4] = 6;
    chars[5] = 84;
    if receive {
        put_ptr(&mut chars, 0x88, fi_receive as *const () as usize);
    } else {
        put_ptr(&mut chars, 0x90, fi_return as *const () as usize);
    }
    let mut handle: *mut u8 = ptr::null_mut();
    let status = unsafe {
        filter::NdisFRegisterFilterDriver(
            ptr::null_mut(),
            ptr::null_mut(),
            chars.as_ptr(),
            &mut handle,
        )
    };
    if receive {
        line_status(b"NDIS CHECK fi-register %08x\n\0", status);
    } else {
        line_status(b"NDIS CHECK fi-return-register %08x\n\0", status);
    }
    if status != 0 || handle.is_null() {
        return false;
    }
    if receive {
        FI_LIST.store(ptr::null_mut(), Ordering::Relaxed);
    } else {
        BACK_LIST.store(ptr::null_mut(), Ordering::Relaxed);
    }
    unsafe { filter::NdisFIndicateReceiveNetBufferLists(handle, list_ptr(), 0, 1, 0) };
    let indicated = if receive {
        FI_FIRST_NULL.load(Ordering::Relaxed) == 1
            && FI_LIST.load(Ordering::Relaxed) == list_ptr()
            && FI_PORT.load(Ordering::Relaxed) == 0
            && FI_COUNT.load(Ordering::Relaxed) == 1
            && FI_FLAGS.load(Ordering::Relaxed) == 0
    } else {
        BACK_FIRST_NULL.load(Ordering::Relaxed) == 1
            && BACK_LIST.load(Ordering::Relaxed) == list_ptr()
            && BACK_FLAGS.load(Ordering::Relaxed) == 0
    };
    if receive && indicated {
        line(b"NDIS CHECK fi-indicate ok\n\0");
    } else if receive {
        line(b"NDIS CHECK fi-indicate fail\n\0");
    } else if indicated {
        line(b"NDIS CHECK fi-return ok\n\0");
    } else {
        line(b"NDIS CHECK fi-return fail\n\0");
    }
    unsafe { filter::NdisFDeregisterFilterDriver(handle) };
    indicated
}

fn protocol_block() -> bool {
    let mut chars = [0u8; PR_CHARS];
    chars[0] = 0x95;
    chars[1] = 2;
    put_u16(&mut chars, 2, PR_CHARS as u16);
    chars[4] = 5;
    chars[5] = 84;
    let mut handle: *mut u8 = ptr::null_mut();
    let bad_version = unsafe {
        protocol::NdisRegisterProtocolDriver(ptr::null_mut(), chars.as_ptr(), &mut handle)
    };
    line_status(b"NDIS CHECK pr-version %08x\n\0", bad_version);
    chars[4] = 6;
    chars[8] = 1;
    let bad_flags = unsafe {
        protocol::NdisRegisterProtocolDriver(ptr::null_mut(), chars.as_ptr(), &mut handle)
    };
    line_status(b"NDIS CHECK pr-flags %08x\n\0", bad_flags);
    chars[8] = 0;
    put_ptr(&mut chars, 0x20, pr_slot as *const () as usize);
    put_ptr(&mut chars, 0x48, pr_slot as *const () as usize);
    PR_CALLS.store(0, Ordering::Relaxed);
    let status = unsafe {
        protocol::NdisRegisterProtocolDriver(ptr::null_mut(), chars.as_ptr(), &mut handle)
    };
    line_status(b"NDIS CHECK pr-register %08x\n\0", status);
    let called = PR_CALLS.load(Ordering::Relaxed);
    line_status(b"NDIS CHECK pr-calls %08x\n\0", called);
    if status == 0 && !handle.is_null() {
        unsafe { protocol::NdisDeregisterProtocolDriver(handle) };
    }
    bad_version == BAD_VERSION && bad_flags == INVALID_PARAMETER && status == 0 && called == 2
}

pub fn run() {
    let miniport_ok = miniport();
    let receive_ok = filter_block(true);
    let return_ok = filter_block(false);
    let protocol_ok = protocol_block();
    if miniport_ok && receive_ok && return_ok && protocol_ok {
        line(b"NDIS CHECK OK\n\0");
    } else {
        line(b"NDIS CHECK FAIL\n\0");
    }
}
