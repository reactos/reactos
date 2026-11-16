/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/filter.rs
 * PURPOSE:     Filter registration and completion calls
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * The characteristics header is NDIS_OBJECT_HEADER in
 * sdk/include/psdk/ntddndis.h. output/ndis-discovery/windows.log
 * (DIFF END OK) accepts a filter of type 0x8B, revision 3, size 0xF0.
 * Capture 2026-11-15T163753Z reached DIFF END OK and called offsets
 * 0x40, 0x48, 0x50, 0x60, 0x70, 0x98, and 0xC0. It did not call 0x58
 * or 0x68. NdisGetVersion in the discovery log is 0x00060054. The bytes 6 and
 * 84 at offset 4 are recorded in differential-tests/ndis-discovery/METHOD.md
 * from capture 2026-11-14T184820Z. That file also records that offset 8
 * stays zero, and that a pointer there returns 0xC000000D. That code is
 * NDIS_STATUS_INVALID_PARAMETER in sdk/include/ddk/ndis.h. The public
 * register page calls FilterSetOptions from inside this function and
 * copies the characteristics. output/ndis-discovery/windows.log
 * (DIFF END OK) returned 0 from NdisFSetAttributes for a 4-byte header
 * and for a type 0xFF revision 4 size 0x120 header, both on the handle
 * from slot 0x50. The header bytes are not the check. Each slot the
 * probe planted takes four
 * pointers, in differential-tests/ndis-discovery/probe.c. That count is
 * the probe method, not a Windows field.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfregisterfilterdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfsetattributes
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfindicatereceivenetbufferlists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfsendnetbufferlists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfsendnetbufferlistscomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfoidrequestcomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfpausecomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfrestartcomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfindicatestatus
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-filter_receive_net_buffer_lists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-filter_return_net_buffer_lists
 * Capture 2026-11-15T160339Z reached DIFF END OK: indicate delivers the
 * list to offset 0x88, argument 1. Argument 0 and argument 2 were null.
 * Capture 2026-11-15T161622Z reached DIFF END OK: with that slot empty,
 * the list arrives at offset 0x90, argument 1.
 */

use core::ptr;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* output/ndis-discovery/windows.log: type 0x8B, revision 3, size 0xF0. */
const FI_TYPE: u8 = 0x8B;
const FI_REVISION: u8 = 3;
const FI_CHARS: usize = 0xF0;
/* METHOD.md, capture 2026-11-14T184820Z: bytes 6 and 84 at offset 4. */
const VERSION_AT: usize = 4;
const VERSION_MAJOR: u8 = 6;
const VERSION_MINOR: u8 = 84;
/* METHOD.md: offset 8 stays zero. A pointer there returns 0xC000000D. */
const FLAGS_AT: usize = 8;
/* Capture 2026-11-15T163753Z. Registration called these and not 0x58 or 0x68. */
const CALLED_SLOTS: [usize; 7] = [0x40, 0x48, 0x50, 0x60, 0x70, 0x98, 0xC0];
/* NDIS_STATUS_BAD_VERSION, NDIS_STATUS_BAD_CHARACTERISTICS, NDIS_STATUS_INVALID_PARAMETER. */
const BAD_VERSION: u32 = 0xC001_0004;
const BAD_CHARACTERISTICS: u32 = 0xC001_0005;
const INVALID_PARAMETER: u32 = 0xC000_000D;
/* NDIS_STATUS_FAILURE and NDIS_STATUS_RESOURCES in sdk/include/ddk/ndis.h. */
const FAILURE: u32 = 0xC000_0001;
const RESOURCES: u32 = 0xC000_009A;
const KIND_FI: u32 = u32::from_le_bytes(*b"FIso");
/* Capture 2026-11-15T160339Z and capture 2026-11-15T161622Z. */
const RECEIVE_SLOT: usize = 0x88;
const RETURN_SLOT: usize = 0x90;

struct FiDriver {
    kind: u32,
    context: *mut u8,
    module_context: *mut u8,
    chars: [u8; FI_CHARS],
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

fn flags_clear(chars: *const u8) -> bool {
    unsafe { ptr::read_unaligned(chars.add(FLAGS_AT) as *const u64) == 0 }
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
pub unsafe extern "system" fn NdisFRegisterFilterDriver(
    _driver: *mut u8,
    context: *mut u8,
    chars: *const u8,
    out_handle: *mut *mut u8,
) -> u32 {
    if chars.is_null() || out_handle.is_null() {
        return INVALID_PARAMETER;
    }
    let revision = *chars.add(1);
    let size = u16::from_le_bytes([*chars.add(2), *chars.add(3)]);
    if !version_ok(chars, size) {
        return BAD_VERSION;
    }
    if *chars != FI_TYPE || revision != FI_REVISION || size as usize != FI_CHARS {
        return BAD_CHARACTERISTICS;
    }
    if !flags_clear(chars) {
        return INVALID_PARAMETER;
    }
    let driver = ExAllocatePoolWithTag(0, size_of::<FiDriver>(), POOL_TAG) as *mut FiDriver;
    if driver.is_null() {
        return RESOURCES;
    }
    (*driver).kind = KIND_FI;
    (*driver).context = context;
    (*driver).module_context = ptr::null_mut();
    ptr::write_bytes((*driver).chars.as_mut_ptr(), 0, FI_CHARS);
    ptr::copy_nonoverlapping(chars, (*driver).chars.as_mut_ptr(), FI_CHARS);
    let status = call_slots(chars, driver as *mut u8, context);
    if status != 0 {
        ExFreePoolWithTag(driver as *mut u8, POOL_TAG);
        return status;
    }
    *out_handle = driver as *mut u8;
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisFDeregisterFilterDriver(handle: *mut u8) {
    if handle.is_null() {
        return;
    }
    let driver = handle as *mut FiDriver;
    if (*driver).kind != KIND_FI {
        return;
    }
    (*driver).kind = 0;
    ExFreePoolWithTag(handle, POOL_TAG);
}

#[no_mangle]
pub unsafe extern "system" fn NdisFSetAttributes(
    filter: *mut u8,
    module_context: *mut u8,
    attributes: *const u8,
) -> u32 {
    if filter.is_null() || attributes.is_null() {
        return FAILURE;
    }
    let driver = filter as *mut FiDriver;
    if (*driver).kind != KIND_FI {
        return FAILURE;
    }
    (*driver).module_context = module_context;
    0
}

fn slot_at(chars: &[u8], offset: usize) -> *mut u8 {
    if offset + size_of::<*mut u8>() > chars.len() {
        return ptr::null_mut();
    }
    unsafe { ptr::read_unaligned(chars.as_ptr().add(offset) as *const *mut u8) }
}

#[no_mangle]
pub unsafe extern "system" fn NdisFIndicateReceiveNetBufferLists(
    filter: *mut u8,
    lists: *mut u8,
    port: u32,
    count: u32,
    flags: u32,
) {
    if filter.is_null() {
        return;
    }
    let driver = filter as *mut FiDriver;
    if (*driver).kind != KIND_FI {
        return;
    }
    let receive = slot_at(&(*driver).chars, RECEIVE_SLOT);
    if !receive.is_null() {
        /* Argument 0 was null. Argument 1 was the list. Argument 2 was
         * null when the port was 0. The fourth captured value was the count. */
        let function: unsafe extern "system" fn(*mut u8, *mut u8, u32, u32, u32) =
            unsafe { core::mem::transmute(receive) };
        unsafe { function(ptr::null_mut(), lists, port, count, flags) }
        return;
    }
    let give_back = slot_at(&(*driver).chars, RETURN_SLOT);
    if give_back.is_null() {
        return;
    }
    let function: unsafe extern "system" fn(*mut u8, *mut u8, u32) =
        unsafe { core::mem::transmute(give_back) };
    unsafe { function(ptr::null_mut(), lists, flags) }
}

#[no_mangle]
pub extern "system" fn NdisFSendNetBufferLists(
    _filter: *mut u8,
    _lists: *mut u8,
    _port: u32,
    _flags: u32,
) {
}

#[no_mangle]
pub extern "system" fn NdisFSendNetBufferListsComplete(
    _filter: *mut u8,
    _lists: *mut u8,
    _flags: u32,
) {
}

#[no_mangle]
pub extern "system" fn NdisFOidRequestComplete(_filter: *mut u8, _request: *mut u8, _status: u32) {}

#[no_mangle]
pub extern "system" fn NdisFPauseComplete(_filter: *mut u8) {}

#[no_mangle]
pub extern "system" fn NdisFRestartComplete(_filter: *mut u8, _status: u32) {}

#[no_mangle]
pub extern "system" fn NdisFIndicateStatus(_filter: *mut u8, _indication: *const u8) {}
