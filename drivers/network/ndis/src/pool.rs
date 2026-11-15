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
 * accepts the list pool when parameter byte 5 is 1. Capture
 * 2026-11-15T232142Z reached DIFF END OK. NET_BUFFER DataLength is at
 * 0x18, Mdl at 0x20, and DataOffset at 0x28. NET_BUFFER_LIST DataLength
 * is at 0x198, Mdl at 0x1A0, and DataOffset at 0x1A8. Each value hit
 * once. The 256-byte and 512-byte lengths are that probe's reads in
 * differential-tests/ndis-discovery/probe.c, not a Windows object size.
 * Pool type 0 is NonPagedPool on
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
/* Capture 2026-11-15T232142Z: one hit for each field. */
const NB_LENGTH_AT: usize = 0x18;
const NB_MDL_AT: usize = 0x20;
const NB_OFFSET_AT: usize = 0x28;
const NBL_LENGTH_AT: usize = 0x198;
const NBL_MDL_AT: usize = 0x1A0;
const NBL_OFFSET_AT: usize = 0x1A8;

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

fn store_u32(object: *mut u8, at: usize, value: u32) {
    unsafe {
        let bytes = value.to_le_bytes();
        ptr::copy_nonoverlapping(bytes.as_ptr(), object.add(at), 4);
    }
}

fn store_u64(object: *mut u8, at: usize, value: u64) {
    unsafe {
        let bytes = value.to_le_bytes();
        ptr::copy_nonoverlapping(bytes.as_ptr(), object.add(at), 8);
    }
}

fn alloc_object(
    bytes: usize,
    mdl: *mut u8,
    data_offset: u32,
    data_length: u64,
    length_at: usize,
    mdl_at: usize,
    offset_at: usize,
) -> *mut u8 {
    unsafe {
        let object = ExAllocatePoolWithTag(0, bytes, POOL_TAG);
        if object.is_null() {
            return ptr::null_mut();
        }
        ptr::write_bytes(object, 0, bytes);
        store_u64(object, length_at, data_length);
        store_u64(object, mdl_at, mdl as u64);
        store_u32(object, offset_at, data_offset);
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
    mdl: *mut u8,
    data_offset: u32,
    data_length: u64,
) -> *mut u8 {
    if pool.is_null() || (*(pool as *mut Pool)).kind != KIND_NB {
        return ptr::null_mut();
    }
    alloc_object(
        NB_SCAN,
        mdl,
        data_offset,
        data_length,
        NB_LENGTH_AT,
        NB_MDL_AT,
        NB_OFFSET_AT,
    )
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
    mdl: *mut u8,
    data_offset: u32,
    data_length: u64,
) -> *mut u8 {
    if pool.is_null() {
        return ptr::null_mut();
    }
    let header = &*(pool as *mut Pool);
    if header.kind != KIND_NBL || header.allow_both == 0 {
        return ptr::null_mut();
    }
    alloc_object(
        NBL_SCAN,
        mdl,
        data_offset,
        data_length,
        NBL_LENGTH_AT,
        NBL_MDL_AT,
        NBL_OFFSET_AT,
    )
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeNetBufferList(list: *mut u8, _flags: u32) {
    if !list.is_null() {
        ExFreePoolWithTag(list, POOL_TAG);
    }
}
