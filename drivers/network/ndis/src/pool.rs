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
 * once. CurrentMdl at 0x08 and CurrentMdlOffset at 0x10 sit before the
 * measured DataLength in the public NET_BUFFER member order
 * (https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nbl/ns-nbl-net_buffer).
 * output/netkvm-20261116T102019Z.log printed NDIS SGALLOC then
 * SGLISTFREE with no RECVLAN. The 256-byte and 512-byte lengths are
 * that probe's reads in differential-tests/ndis-discovery/probe.c, not
 * a Windows object size.
 * Pool type 0 is NonPagedPool on
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_pool_type
 *
 * output/netkvm-20261116T064229Z.log printed NDIS HDR 9f 02 00e0, then
 * NDIS EVENT reset/set/wait, NDIS UNMAP, and NDIS INIT c000009a with no
 * NDIS STUB. NDIS_STATUS_RESOURCES is 0xC000009A in
 * sdk/include/ddk/ndis.h. Size 4 is the measured Windows accept.
 * Size 0x10 with the same type and revision is also measured. Capture
 * 2026-11-16T074503Z reached DIFF END OK with ACCEPTED for that header.
 * output/vmxnet3-20261117T163947Z.log: Missing API
 * 'NdisQueryNetBufferPhysicalCount' while loading vmxnet3.sys.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisquerynetbufferphysicalcount
 * output/vmxnet3-20261117T164218Z.log: Missing API
 * 'NdisCopyFromNetBufferToNetBuffer'.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndiscopyfromnetbuffertonetbuffer
 * output/vmxnet3-20261117T165650Z.log: NDIS POOL 80 01 000c then
 * NDIS5 ERR c0001389 and NDIS INIT c000009a after SHMEMOK.
 * output/vmxnet3-20261117T183737Z.log: Missing API
 * 'NdisAdvanceNetBufferDataStart'.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisadvancenetbufferdatastart
 * output/vmxnet3-20261117T183927Z.log: Missing API
 * 'NdisRetreatNetBufferDataStart'.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisretreatnetbufferdatastart
 * output/vmxnet3-20261117T184128Z.log: Missing API
 * 'NdisAllocateNetBufferMdlAndData'.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisallocatenetbuffermdlanddata
 * output/e1000e-20261118T044018Z.log: after ASHMEMOK / NDIS POOL 80 01
 * 0010 / NDIS MDL, MM FAULT va 0xA8 with rsi 0. Combined allocate had
 * stuffed NB length/MDL onto the NBL at 0x198 and left FirstNetBuffer
 * at 8 null (lan.c NBL_FIRST_NET_BUFFER_AT /
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nbl/ns-nbl-net_buffer_list_data).
 * Pool size 0x10 matches
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/ns-nblapi-net_buffer_list_pool_parameters
 * (Header, ProtocolId, fAllocateNetBuffer, ContextSize, PoolTag,
 * DataSize).
 */

use core::ptr;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* Probe read length in differential-tests/ndis-discovery/probe.c. Not a Windows size. */
const NB_SCAN: usize = 256;
/* Same probe, the NET_BUFFER_LIST read. Not a Windows size. */
const NBL_SCAN: usize = 512;
const KIND_NB: u32 = 1;
const KIND_NBL: u32 = 2;
/* Capture 2026-11-15T232142Z: one hit for each field. CurrentMdl and
 * CurrentMdlOffset precede DataLength in the public NET_BUFFER order. */
const NB_CURRENT_MDL_AT: usize = 0x08;
const NB_CURRENT_MDL_OFFSET_AT: usize = 0x10;
const NB_LENGTH_AT: usize = 0x18;
const NB_MDL_AT: usize = 0x20;
const NB_OFFSET_AT: usize = 0x28;
const NBL_LENGTH_AT: usize = 0x198;
const NBL_MDL_AT: usize = 0x1A0;
const NBL_OFFSET_AT: usize = 0x1A8;
/* lan.c / NET_BUFFER_LIST_DATA: FirstNetBuffer at 8. */
const NBL_FIRST_NB_AT: usize = 8;

#[repr(C)]
struct Pool {
    kind: u32,
    allow_both: u32,
}

unsafe extern "system" {
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
    fn DbgPrint(format: *const u8, ...) -> i32;
}

fn header_ok(params: *const u8) -> bool {
    if params.is_null() {
        return false;
    }
    unsafe {
        let kind = *params;
        let revision = *params.add(1);
        let size = u16::from_le_bytes([*params.add(2), *params.add(3)]);
        DbgPrint(
            b"NDIS POOL %02x %02x %04x\n\0".as_ptr(),
            kind as u32,
            revision as u32,
            size as u32,
        );
        /* Capture 2026-11-15T232142Z: type 0x80, revision 1, size 4.
         * output/netkvm-20261116T071255Z.log printed NDIS POOL 80 01 0010
         * from NetKVM before NdisMRegisterInterruptEx. Capture
         * 2026-11-16T074503Z reached DIFF END OK with ACCEPTED for type
         * 0x80, revision 1, size 0x10. output/vmxnet3-20261117T165650Z.log
         * printed NDIS POOL 80 01 000c then INIT c000009a. */
        if size as usize >= 0x10 {
            DbgPrint(
                b"NDIS POOLB %02x %02x %04x %08x %08x\n\0".as_ptr(),
                *params.add(4) as u32,
                *params.add(5) as u32,
                u16::from_le_bytes([*params.add(6), *params.add(7)]) as u32,
                u32::from_le_bytes([
                    *params.add(8),
                    *params.add(9),
                    *params.add(10),
                    *params.add(11),
                ]),
                u32::from_le_bytes([
                    *params.add(12),
                    *params.add(13),
                    *params.add(14),
                    *params.add(15),
                ]),
            );
        }
        kind == 0x80 && revision == 1 && (size == 4 || size == 0x0c || size == 0x10)
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
    set_current: bool,
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
        if set_current {
            store_u64(object, NB_CURRENT_MDL_AT, mdl as u64);
            store_u32(object, NB_CURRENT_MDL_OFFSET_AT, data_offset);
        }
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
        true,
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
pub unsafe extern "system" fn NdisAllocateNetBufferList(
    pool: *mut u8,
    _context: u16,
    _backfill: u16,
) -> *mut u8 {
    /* Empty NET_BUFFER_LIST. FirstNetBuffer stays null until the caller
     * writes it. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisallocatenetbufferlist */
    if pool.is_null() {
        return ptr::null_mut();
    }
    let header = &*(pool as *mut Pool);
    if header.kind != KIND_NBL {
        return ptr::null_mut();
    }
    let object = ExAllocatePoolWithTag(0, NBL_SCAN, POOL_TAG);
    if !object.is_null() {
        ptr::write_bytes(object, 0, NBL_SCAN);
    }
    object
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
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisallocatenetbufferandnetbufferlist
     * output/e1000e-20261118T044018Z.log faulted at va 0xA8 after MDL
     * when FirstNetBuffer stayed null. Allocate a NET_BUFFER, link it at
     * NBL_FIRST_NB_AT (lan.c), and keep the NBL+0x198 MDL/length mirror
     * NetKVM used on output/netkvm-20261116T145342Z.log. */
    DbgPrint(
        b"NDIS NBLCOMBO pool %p mdl %p off %08x len %08x\n\0".as_ptr(),
        pool,
        mdl,
        data_offset,
        data_length as u32,
    );
    if pool.is_null() {
        return ptr::null_mut();
    }
    let header = &*(pool as *mut Pool);
    if header.kind != KIND_NBL || header.allow_both == 0 {
        DbgPrint(
            b"NDIS NBLCOMBO reject kind %08x both %08x\n\0".as_ptr(),
            header.kind,
            header.allow_both,
        );
        return ptr::null_mut();
    }
    let nbl = ExAllocatePoolWithTag(0, NBL_SCAN, POOL_TAG);
    if nbl.is_null() {
        return ptr::null_mut();
    }
    ptr::write_bytes(nbl, 0, NBL_SCAN);
    let nb = alloc_object(
        NB_SCAN,
        mdl,
        data_offset,
        data_length,
        NB_LENGTH_AT,
        NB_MDL_AT,
        NB_OFFSET_AT,
        true,
    );
    if nb.is_null() {
        ExFreePoolWithTag(nbl, POOL_TAG);
        return ptr::null_mut();
    }
    store_u64(nbl, NBL_FIRST_NB_AT, nb as u64);
    store_u64(nbl, NBL_LENGTH_AT, data_length);
    store_u64(nbl, NBL_MDL_AT, mdl as u64);
    store_u32(nbl, NBL_OFFSET_AT, data_offset);
    DbgPrint(b"NDIS NBLCOMBOOK %p nb %p\n\0".as_ptr(), nbl, nb);
    nbl
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeNetBufferList(list: *mut u8, _flags: u32) {
    if !list.is_null() {
        ExFreePoolWithTag(list, POOL_TAG);
    }
}

/* MDL.MappedSystemVa in sdk/include/xdk/mmtypes.h on x64. */
const MDL_MAPPED_VA_AT: usize = 0x18;
const STATUS_SUCCESS: u32 = 0;
const STATUS_RESOURCES: u32 = 0xC000_009A;
const STATUS_FAILURE: u32 = 0xC000_0001;
/* This file's data buffer for AllocateNetBufferMdlAndData, not a Windows size. */
const NB_DATA_BYTES: usize = 2048;
const NB_COMBINED: usize = NB_SCAN + 0x40 + NB_DATA_BYTES;

unsafe extern "system" {
    fn IoAllocateMdl(
        virtual_address: *mut u8,
        length: u32,
        secondary: u8,
        charge: u8,
        irp: *mut u8,
    ) -> *mut u8;
    fn MmBuildMdlForNonPagedPool(mdl: *mut u8);
}

fn nb_current(net_buffer: *mut u8) -> (*mut u8, u32) {
    unsafe {
        let mut mdl = ptr::read_unaligned(net_buffer.add(NB_CURRENT_MDL_AT) as *const *mut u8);
        let mut offset =
            ptr::read_unaligned(net_buffer.add(NB_CURRENT_MDL_OFFSET_AT) as *const u32);
        if mdl.is_null() {
            mdl = ptr::read_unaligned(net_buffer.add(NB_MDL_AT) as *const *mut u8);
            offset = ptr::read_unaligned(net_buffer.add(NB_OFFSET_AT) as *const u32);
        }
        (mdl, offset)
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisQueryNetBufferPhysicalCount(net_buffer: *mut u8) -> u32 {
    /* output/vmxnet3-20261117T163947Z.log named this import.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisquerynetbufferphysicalcount
     * Count physical breaks in the MDL chain; one page-aligned buffer is 1. */
    if net_buffer.is_null() {
        return 0;
    }
    let mdl = ptr::read_unaligned(net_buffer.add(NB_MDL_AT) as *const *mut u8);
    if mdl.is_null() {
        0
    } else {
        1
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisCopyFromNetBufferToNetBuffer(
    destination: *mut u8,
    destination_offset: u32,
    bytes_to_copy: u32,
    source: *const u8,
    source_offset: u32,
    bytes_copied: *mut u32,
) -> u32 {
    /* output/vmxnet3-20261117T164218Z.log named this import.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndiscopyfromnetbuffertonetbuffer */
    if !bytes_copied.is_null() {
        *bytes_copied = 0;
    }
    if destination.is_null() || source.is_null() || bytes_to_copy == 0 {
        return STATUS_FAILURE;
    }
    let src = source as *mut u8;
    let (src_mdl, src_cur) = nb_current(src);
    let (dst_mdl, dst_cur) = nb_current(destination);
    if src_mdl.is_null() || dst_mdl.is_null() {
        return STATUS_RESOURCES;
    }
    let src_va = ptr::read_unaligned(src_mdl.add(MDL_MAPPED_VA_AT) as *const *mut u8);
    let dst_va = ptr::read_unaligned(dst_mdl.add(MDL_MAPPED_VA_AT) as *const *mut u8);
    if src_va.is_null() || dst_va.is_null() {
        return STATUS_RESOURCES;
    }
    let src_len = ptr::read_unaligned(src.add(NB_LENGTH_AT) as *const u64) as u32;
    let dst_len = ptr::read_unaligned(destination.add(NB_LENGTH_AT) as *const u64) as u32;
    let from = src_cur.saturating_add(source_offset);
    let to = dst_cur.saturating_add(destination_offset);
    let avail_src = src_len.saturating_sub(source_offset);
    let avail_dst = dst_len.saturating_sub(destination_offset);
    let n = bytes_to_copy.min(avail_src).min(avail_dst);
    if n == 0 {
        return STATUS_SUCCESS;
    }
    ptr::copy_nonoverlapping(src_va.add(from as usize), dst_va.add(to as usize), n as usize);
    if !bytes_copied.is_null() {
        *bytes_copied = n;
    }
    STATUS_SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn NdisAdvanceNetBufferDataStart(
    net_buffer: *mut u8,
    data_offset_delta: u32,
    _free_mdl: u8,
    _free_mdl_handler: *mut u8,
) {
    /* output/vmxnet3-20261117T183737Z.log named this import.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisadvancenetbufferdatastart
     * Adjusts DataOffset / DataLength / CurrentMdlOffset at the offsets
     * capture 2026-11-15T232142Z measured. FreeMdl stays unused. */
    if net_buffer.is_null() || data_offset_delta == 0 {
        return;
    }
    let length = ptr::read_unaligned(net_buffer.add(NB_LENGTH_AT) as *const u64);
    let offset = ptr::read_unaligned(net_buffer.add(NB_OFFSET_AT) as *const u32);
    let cur = ptr::read_unaligned(net_buffer.add(NB_CURRENT_MDL_OFFSET_AT) as *const u32);
    let delta = data_offset_delta as u64;
    if delta > length {
        return;
    }
    store_u64(net_buffer, NB_LENGTH_AT, length - delta);
    store_u32(net_buffer, NB_OFFSET_AT, offset.saturating_add(data_offset_delta));
    store_u32(
        net_buffer,
        NB_CURRENT_MDL_OFFSET_AT,
        cur.saturating_add(data_offset_delta),
    );
}

#[no_mangle]
pub unsafe extern "system" fn NdisRetreatNetBufferDataStart(
    net_buffer: *mut u8,
    data_offset_delta: u32,
    _data_back_fill: u32,
    _allocate_mdl_handler: *mut u8,
) -> u32 {
    /* output/vmxnet3-20261117T183927Z.log named this import.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisretreatnetbufferdatastart
     * Reduce DataOffset when unused space covers the delta; no new MDL. */
    if net_buffer.is_null() || data_offset_delta == 0 {
        return STATUS_SUCCESS;
    }
    let offset = ptr::read_unaligned(net_buffer.add(NB_OFFSET_AT) as *const u32);
    let cur = ptr::read_unaligned(net_buffer.add(NB_CURRENT_MDL_OFFSET_AT) as *const u32);
    if offset < data_offset_delta || cur < data_offset_delta {
        return STATUS_RESOURCES;
    }
    let length = ptr::read_unaligned(net_buffer.add(NB_LENGTH_AT) as *const u64);
    store_u64(net_buffer, NB_LENGTH_AT, length + data_offset_delta as u64);
    store_u32(net_buffer, NB_OFFSET_AT, offset - data_offset_delta);
    store_u32(net_buffer, NB_CURRENT_MDL_OFFSET_AT, cur - data_offset_delta);
    STATUS_SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateNetBufferMdlAndData(pool: *mut u8) -> *mut u8 {
    /* output/vmxnet3-20261117T184128Z.log named this import.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/nblapi/nf-nblapi-ndisallocatenetbuffermdlanddata
     * IoAllocateMdl / MmBuildMdlForNonPagedPool match NdisAllocateMdl in lib.rs. */
    if pool.is_null() || (*(pool as *mut Pool)).kind != KIND_NB {
        return ptr::null_mut();
    }
    let block = ExAllocatePoolWithTag(0, NB_COMBINED, POOL_TAG);
    if block.is_null() {
        return ptr::null_mut();
    }
    ptr::write_bytes(block, 0, NB_COMBINED);
    let data = block.add(NB_SCAN + 0x40);
    let mdl = IoAllocateMdl(data, NB_DATA_BYTES as u32, 0, 0, ptr::null_mut());
    if mdl.is_null() {
        ExFreePoolWithTag(block, POOL_TAG);
        return ptr::null_mut();
    }
    MmBuildMdlForNonPagedPool(mdl);
    store_u64(block, NB_LENGTH_AT, NB_DATA_BYTES as u64);
    store_u64(block, NB_MDL_AT, mdl as u64);
    store_u32(block, NB_OFFSET_AT, 0);
    store_u64(block, NB_CURRENT_MDL_AT, mdl as u64);
    store_u32(block, NB_CURRENT_MDL_OFFSET_AT, 0);
    block
}
