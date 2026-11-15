/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/pool.rs
 * PURPOSE:     Net buffer pools at the offsets the probe measured
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * The parameter header is NDIS_OBJECT_HEADER in
 * sdk/include/psdk/ntddndis.h: Type, Revision, Size.
 * output/ndis-discovery/windows.log (DIFF END OK) accepts type 0x80,
 * revision 1, size 4 for both pool allocators. The combined allocate
 * accepts the list pool when parameter byte 5 is 1. NET_BUFFER
 * DataLength is at 0x18. NET_BUFFER_LIST DataLength is at 0x198.
 * DataOffset and Mdl were AMBIGUOUS in that log, so they are not stored
 * in the bytes the probe reads. The 256-byte and 512-byte lengths are
 * that probe's reads in differential-tests/ndis-discovery/probe.c, not
 * a Windows object size. Pool type 0 is NonPagedPool on
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_pool_type
 */

use core::ptr;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* Probe read length in differential-tests/ndis-discovery/probe.c. Not a Windows size. */
const NB_SCAN: usize = 256;
/* Same probe, the NET_BUFFER_LIST read. Not a Windows size. */
const NBL_SCAN: usize = 512;
const KIND_NB: u32 = 1;
const KIND_NBL: u32 = 2;

#[repr(C)]
struct Pool {
    kind: u32,
    allow_both: u32,
}

unsafe extern "system" {
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
}

fn header_ok(params: *const u8) -> bool {
    if params.is_null() {
        return false;
    }
    unsafe {
        let kind = *params;
        let revision = *params.add(1);
        let size = u16::from_le_bytes([*params.add(2), *params.add(3)]);
        kind == 0x80 && revision == 1 && size == 4
    }
}

fn alloc_pool(kind: u32, allow_both: u32) -> *mut u8 {
    unsafe {
        let pool = ExAllocatePoolWithTag(0, size_of::<Pool>(), POOL_TAG) as *mut Pool;
        if pool.is_null() {
            return ptr::null_mut();
        }
        (*pool).kind = kind;
        (*pool).allow_both = allow_both;
        pool as *mut u8
    }
}

fn alloc_object(bytes: usize, data_at: usize, data_length: u64) -> *mut u8 {
    unsafe {
        let object = ExAllocatePoolWithTag(0, bytes, POOL_TAG);
        if object.is_null() {
            return ptr::null_mut();
        }
        ptr::write_bytes(object, 0, bytes);
        let slot = object.add(data_at) as *mut u64;
        *slot = data_length;
        object
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateNetBufferPool(
    _handle: *mut u8,
    params: *const u8,
) -> *mut u8 {
    if !header_ok(params) {
        return ptr::null_mut();
    }
    alloc_pool(KIND_NB, 0)
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeNetBufferPool(pool: *mut u8) {
    if !pool.is_null() {
        ExFreePoolWithTag(pool, POOL_TAG);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateNetBuffer(
    pool: *mut u8,
    _mdl: *mut u8,
    _data_offset: u32,
    data_length: u64,
) -> *mut u8 {
    if pool.is_null() || (*(pool as *mut Pool)).kind != KIND_NB {
        return ptr::null_mut();
    }
    alloc_object(NB_SCAN, 0x18, data_length)
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeNetBuffer(buffer: *mut u8) {
    if !buffer.is_null() {
        ExFreePoolWithTag(buffer, POOL_TAG);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateNetBufferListPool(
    _handle: *mut u8,
    params: *const u8,
) -> *mut u8 {
    if !header_ok(params) {
        return ptr::null_mut();
    }
    /* Byte 5 is past the 4-byte header. The log accepts value 1 there. */
    let allow = unsafe { *params.add(5) };
    alloc_pool(KIND_NBL, if allow == 1 { 1 } else { 0 })
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeNetBufferListPool(pool: *mut u8) {
    if !pool.is_null() {
        ExFreePoolWithTag(pool, POOL_TAG);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateNetBufferAndNetBufferList(
    pool: *mut u8,
    _context: u16,
    _backfill: u16,
    _mdl: *mut u8,
    _data_offset: u32,
    data_length: u64,
) -> *mut u8 {
    if pool.is_null() {
        return ptr::null_mut();
    }
    let header = &*(pool as *mut Pool);
    if header.kind != KIND_NBL || header.allow_both == 0 {
        return ptr::null_mut();
    }
    alloc_object(NBL_SCAN, 0x198, data_length)
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeNetBufferList(list: *mut u8, _flags: u32) {
    if !list.is_null() {
        ExFreePoolWithTag(list, POOL_TAG);
    }
}
