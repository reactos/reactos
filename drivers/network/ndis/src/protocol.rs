/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/protocol.rs
 * PURPOSE:     Protocol registration, send, and return
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson
 */

/*
 * The characteristics header is NDIS_OBJECT_HEADER in
 * sdk/include/psdk/ntddndis.h. output/ndis-discovery/windows.log
 * (DIFF END OK) accepts a protocol of type 0x95, revision 2, size
 * 0x80, and calls offsets 0x20 and 0x48. NdisGetVersion in that log is
 * 0x00060054. The bytes 6 and 84 at offset 4 are recorded in
 * differential-tests/ndis-discovery/METHOD.md from capture
 * 2026-11-14T184820Z. That file also records that offset 8 stays zero,
 * and that a pointer there returns 0xC000000D. The public register page
 * calls ProtocolSetOptions from inside this function and copies the
 * characteristics. Each slot the probe planted takes four pointers, in
 * differential-tests/ndis-discovery/probe.c. That count is the probe
 * method, not a Windows field.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterprotocoldriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissendnetbufferlists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreturnnetbufferlists
 */

use core::ptr;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* output/ndis-discovery/windows.log: type 0x95, revision 2, size 0x80. */
const PR_TYPE: u8 = 0x95;
const PR_REVISION: u8 = 2;
const PR_CHARS: usize = 0x80;
/* METHOD.md, capture 2026-11-14T184820Z: bytes 6 and 84 at offset 4. */
const VERSION_AT: usize = 4;
const VERSION_MAJOR: u8 = 6;
const VERSION_MINOR: u8 = 84;
/* METHOD.md: offset 8 stays zero. A pointer there returns 0xC000000D. */
const FLAGS_AT: usize = 8;
/* Same log, in the order DIFF CALL printed them. */
const CALLED_SLOTS: [usize; 2] = [0x20, 0x48];
/* NDIS_STATUS_BAD_VERSION and NDIS_STATUS_BAD_CHARACTERISTICS in sdk/include/ddk/ndis.h. */
const BAD_VERSION: u32 = 0xC001_0004;
const BAD_CHARACTERISTICS: u32 = 0xC001_0005;
/* METHOD.md measured 0xC000000D. NDIS_STATUS_INVALID_PARAMETER in sdk/include/ddk/ndis.h. */
const INVALID_PARAMETER: u32 = 0xC000_000D;
/* NDIS_STATUS_FAILURE and NDIS_STATUS_RESOURCES in sdk/include/ddk/ndis.h. */
const FAILURE: u32 = 0xC000_0001;
const RESOURCES: u32 = 0xC000_009A;
const KIND_PR: u32 = u32::from_le_bytes(*b"PRso");

struct PrDriver {
    kind: u32,
    context: *mut u8,
    chars: [u8; PR_CHARS],
}

unsafe extern "system" {
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
}

fn version_ok(chars: *const u8, size: u16) -> bool {
    if (size as usize) < VERSION_AT + 2 {
        return false;
    }
    unsafe { *chars.add(VERSION_AT) == VERSION_MAJOR && *chars.add(VERSION_AT + 1) == VERSION_MINOR }
}

fn call_slots(chars: *const u8, handle: *mut u8, context: *mut u8) -> u32 {
    for offset in CALLED_SLOTS {
        let slot = unsafe { ptr::read_unaligned(chars.add(offset) as *const *mut u8) };
        if slot.is_null() {
            continue;
        }
        let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8) -> u32 =
            unsafe { core::mem::transmute(slot) };
        let status = unsafe { function(handle, context, ptr::null_mut(), ptr::null_mut()) };
        if status != 0 {
            return status;
        }
    }
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisRegisterProtocolDriver(
    context: *mut u8,
    chars: *const u8,
    out_handle: *mut *mut u8,
) -> u32 {
    if chars.is_null() || out_handle.is_null() {
        return FAILURE;
    }
    let revision = *chars.add(1);
    let size = u16::from_le_bytes([*chars.add(2), *chars.add(3)]);
    if !version_ok(chars, size) {
        return BAD_VERSION;
    }
    if *chars != PR_TYPE || revision != PR_REVISION || size as usize != PR_CHARS {
        return BAD_CHARACTERISTICS;
    }
    if ptr::read_unaligned(chars.add(FLAGS_AT) as *const u64) != 0 {
        return INVALID_PARAMETER;
    }
    let driver = ExAllocatePoolWithTag(0, size_of::<PrDriver>(), POOL_TAG) as *mut PrDriver;
    if driver.is_null() {
        return RESOURCES;
    }
    (*driver).kind = KIND_PR;
    (*driver).context = context;
    ptr::write_bytes((*driver).chars.as_mut_ptr(), 0, PR_CHARS);
    ptr::copy_nonoverlapping(chars, (*driver).chars.as_mut_ptr(), PR_CHARS);
    let status = call_slots(chars, driver as *mut u8, context);
    if status != 0 {
        ExFreePoolWithTag(driver as *mut u8, POOL_TAG);
        return status;
    }
    *out_handle = driver as *mut u8;
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisDeregisterProtocolDriver(handle: *mut u8) {
    if handle.is_null() {
        return;
    }
    let driver = handle as *mut PrDriver;
    if (*driver).kind != KIND_PR {
        return;
    }
    (*driver).kind = 0;
    ExFreePoolWithTag(handle, POOL_TAG);
}

#[no_mangle]
pub extern "system" fn NdisSendNetBufferLists(
    _binding: *mut u8,
    _lists: *mut u8,
    _port: u32,
    _flags: u32,
) {
}

#[no_mangle]
pub extern "system" fn NdisReturnNetBufferLists(_binding: *mut u8, _lists: *mut u8, _flags: u32) {}
