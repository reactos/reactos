/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/register.rs
 * PURPOSE:     Miniport registration and completion calls
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson
 */

/*
 * The characteristics header is NDIS_OBJECT_HEADER in
 * sdk/include/psdk/ntddndis.h. output/ndis-discovery/windows.log
 * (DIFF END OK) accepts a miniport of type 0x8A, revision 3, size
 * 0x120, calls the pointer at offset 0x10, and returns 0xC0010004 for
 * a zeroed block. NdisGetVersion in that log is 0x00060054. The bytes
 * 6 and 84 at offset 4, and the move to 0xC0010005, are recorded in
 * differential-tests/ndis-discovery/METHOD.md from capture
 * 2026-11-14T184820Z. Those codes are NDIS_STATUS_BAD_VERSION and
 * NDIS_STATUS_BAD_CHARACTERISTICS in sdk/include/ddk/ndis.h. The public
 * register page calls MiniportSetOptions from inside this function and
 * copies the characteristics. The slot the probe planted takes four
 * pointers, in differential-tests/ndis-discovery/probe.c. That count is
 * the probe method, not a Windows field.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiportdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismindicatereceivenetbufferlists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsendnetbufferlistscomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismoidrequestcomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismrestartcomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismindicatestatusex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismpausecomplete
 */

use core::ptr;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* output/ndis-discovery/windows.log: type 0x8A, revision 3, size 0x120. */
const MP_TYPE: u8 = 0x8A;
const MP_REVISION: u8 = 3;
const MP_CHARS: usize = 0x120;
/* Same log: bytes 6 and 84 at offset 4. */
const VERSION_AT: usize = 4;
const VERSION_MAJOR: u8 = 6;
const VERSION_MINOR: u8 = 84;
/* Same log: DIFF CALL at offset 0x10. The probe slot takes four pointers. */
const CALLED_SLOT: usize = 0x10;
/* NDIS_STATUS_BAD_VERSION and NDIS_STATUS_BAD_CHARACTERISTICS in sdk/include/ddk/ndis.h. */
const BAD_VERSION: u32 = 0xC001_0004;
const BAD_CHARACTERISTICS: u32 = 0xC001_0005;
/* NDIS_STATUS_FAILURE and NDIS_STATUS_RESOURCES in sdk/include/ddk/ndis.h. */
const FAILURE: u32 = 0xC000_0001;
const RESOURCES: u32 = 0xC000_009A;
const KIND_MP: u32 = u32::from_le_bytes(*b"MPso");

struct MpDriver {
    kind: u32,
    context: *mut u8,
    chars: [u8; MP_CHARS],
}

unsafe extern "system" {
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
}

fn header(chars: *const u8) -> (u8, u8, u16) {
    unsafe {
        let size = u16::from_le_bytes([*chars.add(2), *chars.add(3)]);
        (*chars, *chars.add(1), size)
    }
}

fn version_ok(chars: *const u8, size: u16) -> bool {
    if (size as usize) < VERSION_AT + 2 {
        return false;
    }
    unsafe { *chars.add(VERSION_AT) == VERSION_MAJOR && *chars.add(VERSION_AT + 1) == VERSION_MINOR }
}

fn call_slot(chars: *const u8, handle: *mut u8, context: *mut u8) -> u32 {
    let slot = unsafe { ptr::read_unaligned(chars.add(CALLED_SLOT) as *const *mut u8) };
    if slot.is_null() {
        return 0;
    }
    let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    unsafe { function(handle, context, ptr::null_mut(), ptr::null_mut()) }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterMiniportDriver(
    _driver: *mut u8,
    _registry: *mut u8,
    context: *mut u8,
    chars: *const u8,
    out_handle: *mut *mut u8,
) -> u32 {
    if chars.is_null() || out_handle.is_null() {
        return FAILURE;
    }
    let (kind, revision, size) = header(chars);
    if !version_ok(chars, size) {
        return BAD_VERSION;
    }
    if kind != MP_TYPE || revision != MP_REVISION || size as usize != MP_CHARS {
        return BAD_CHARACTERISTICS;
    }
    let driver = ExAllocatePoolWithTag(0, size_of::<MpDriver>(), POOL_TAG) as *mut MpDriver;
    if driver.is_null() {
        return RESOURCES;
    }
    (*driver).kind = KIND_MP;
    (*driver).context = context;
    ptr::write_bytes((*driver).chars.as_mut_ptr(), 0, MP_CHARS);
    ptr::copy_nonoverlapping(chars, (*driver).chars.as_mut_ptr(), MP_CHARS);
    let status = call_slot(chars, driver as *mut u8, context);
    if status != 0 {
        ExFreePoolWithTag(driver as *mut u8, POOL_TAG);
        return status;
    }
    *out_handle = driver as *mut u8;
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterMiniportDriver(handle: *mut u8) {
    if handle.is_null() {
        return;
    }
    let driver = handle as *mut MpDriver;
    if (*driver).kind != KIND_MP {
        return;
    }
    (*driver).kind = 0;
    ExFreePoolWithTag(handle, POOL_TAG);
}

#[no_mangle]
pub extern "system" fn NdisMSetMiniportAttributes(_handle: *mut u8, _attributes: *const u8) -> u32 {
    /* Slot 0x10 returned 0xC000000D for a 4-byte header, and no header
     * was accepted. That slot is not an adapter handle. The public page
     * returns bad version for an unsupported revision. */
    BAD_VERSION
}

#[no_mangle]
pub extern "system" fn NdisMIndicateReceiveNetBufferLists(
    _adapter: *mut u8,
    _lists: *mut u8,
    _port: u32,
    _count: u32,
    _flags: u32,
) {
}

#[no_mangle]
pub extern "system" fn NdisMSendNetBufferListsComplete(
    _adapter: *mut u8,
    _lists: *mut u8,
    _flags: u32,
) {
}

#[no_mangle]
pub extern "system" fn NdisMOidRequestComplete(_adapter: *mut u8, _request: *mut u8, _status: u32) {
}

#[no_mangle]
pub extern "system" fn NdisMRestartComplete(_adapter: *mut u8, _status: u32) {}

#[no_mangle]
pub extern "system" fn NdisMIndicateStatusEx(_adapter: *mut u8, _indication: *const u8) {}

#[no_mangle]
pub extern "system" fn NdisMPauseComplete(_adapter: *mut u8) {}
