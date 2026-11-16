/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/protocol.rs
 * PURPOSE:     Protocol registration, send, and return
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * The characteristics header is NDIS_OBJECT_HEADER in
 * sdk/include/psdk/ntddndis.h. output/ndis-discovery/windows.log
 * (DIFF END OK) accepts a protocol of type 0x95, revision 2, size
 * 0x80, and calls offsets 0x20 and 0x48. NdisGetVersion in that log is
 * 0x00060054. The bytes 6 and 84 at offset 4 are recorded in
 * differential-tests/ndis-discovery/METHOD.md from capture
 * 2026-11-14T184820Z. That file also records that offset 8 stays zero,
 * and that a pointer there returns 0xC000000D. output/netkvm-20261116T053850Z.log
 * printed NDIS PR 95 02 0080 and NDIS PVER 06 1e from NetKVM after
 * NdisRegisterDeviceEx, then DriverEntry returned 0xC0010004 from this
 * function. Capture 2026-11-16T054327Z reached DIFF END OK for that
 * header: status 0 and DIFF CALL at offset 0x48. This file accepts
 * bytes 6 and 0x1e at that size. NetKVM leaves offset 0x20 null, so
 * this path does not call 0x48. output/netkvm-20261116T054453Z.log
 * printed that accept and DriverEntry returned 0. The public register
 * page calls ProtocolSetOptions from inside this function and copies
 * the characteristics. Each slot the probe planted takes four pointers,
 * in differential-tests/ndis-discovery/probe.c. That count is the probe
 * method, not a Windows field.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterprotocoldriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissendnetbufferlists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreturnnetbufferlists
 * After MiniportRestart, NDIS calls BindAdapterHandlerEx at offset 0x28
 * (capture 2026-11-15T162555Z / METHOD.md). notify_adapter_ready walks
 * registered protocol drivers and calls that slot.
 */

use core::ptr;

use crate::prot6;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* output/ndis-discovery/windows.log: type 0x95, revision 2, size 0x80. */
const PR_TYPE: u8 = 0x95;
const PR_REVISION: u8 = 2;
const PR_CHARS: usize = 0x80;
/* METHOD.md, capture 2026-11-14T184820Z: bytes 6 and 84 at offset 4. */
const VERSION_AT: usize = 4;
const VERSION_MAJOR: u8 = 6;
const VERSION_MINOR: u8 = 84;
/* Capture 2026-11-16T054327Z: NetKVM protocol bytes 6 and 0x1e. */
const VERSION_MINOR_NETKVM: u8 = 0x1e;
/* METHOD.md: offset 8 stays zero. A pointer there returns 0xC000000D. */
const FLAGS_AT: usize = 8;
/* Same log, in the order DIFF CALL printed them. */
const CALLED_SLOTS: [usize; 2] = [0x20, 0x48];
/* Public register page: ProtocolSetOptions at this offset. */
const SET_OPTIONS_SLOT: usize = 0x20;
const BIND_SLOT: usize = 0x28;
/* NDIS_STATUS_BAD_VERSION and NDIS_STATUS_BAD_CHARACTERISTICS in sdk/include/ddk/ndis.h. */
const BAD_VERSION: u32 = 0xC001_0004;
const BAD_CHARACTERISTICS: u32 = 0xC001_0005;
/* METHOD.md measured 0xC000000D. NDIS_STATUS_INVALID_PARAMETER in sdk/include/ddk/ndis.h. */
const INVALID_PARAMETER: u32 = 0xC000_000D;
/* NDIS_STATUS_FAILURE and NDIS_STATUS_RESOURCES in sdk/include/ddk/ndis.h. */
const FAILURE: u32 = 0xC000_0001;
const RESOURCES: u32 = 0xC000_009A;
const KIND_PR: u32 = u32::from_le_bytes(*b"PRso");
const MAX_PROTOCOLS: usize = 8;

struct PrDriver {
    kind: u32,
    context: *mut u8,
    chars: [u8; PR_CHARS],
}

static mut PROTOCOLS: [*mut PrDriver; MAX_PROTOCOLS] = [ptr::null_mut(); MAX_PROTOCOLS];

unsafe extern "system" {
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
    fn DbgPrint(format: *const u8, ...) -> i32;
}

pub fn dbg(msg: &[u8]) {
    unsafe { DbgPrint(msg.as_ptr()) };
}

pub fn dbg_status(fmt: &[u8], status: u32) {
    unsafe { DbgPrint(fmt.as_ptr(), status) };
}

pub fn dbg_status_ptr(fmt: &[u8], status: u32, ptr_val: *mut u8) {
    unsafe { DbgPrint(fmt.as_ptr(), status, ptr_val) };
}

pub fn dbg_hdr(fmt: &[u8], kind: u8, revision: u8, size: u16) {
    unsafe { DbgPrint(fmt.as_ptr(), kind as u32, revision as u32, size as u32) };
}

pub fn alloc_bytes(bytes: usize) -> *mut u8 {
    unsafe { ExAllocatePoolWithTag(0, bytes, POOL_TAG) }
}

pub fn free_bytes(address: *mut u8) {
    if !address.is_null() {
        unsafe { ExFreePoolWithTag(address, POOL_TAG) };
    }
}

fn track(driver: *mut PrDriver) {
    unsafe {
        for slot in PROTOCOLS.iter_mut() {
            if slot.is_null() {
                *slot = driver;
                return;
            }
        }
    }
}

fn untrack(driver: *mut PrDriver) {
    unsafe {
        for slot in PROTOCOLS.iter_mut() {
            if *slot == driver {
                *slot = ptr::null_mut();
                return;
            }
        }
    }
}

pub fn notify_adapter_ready(adapter: *mut u8) {
    /* Call BindAdapterEx on each registered protocol that has a slot. */
    if adapter.is_null() {
        return;
    }
    unsafe { DbgPrint(b"NDIS BINDALL\n\0".as_ptr()) };
    let params = prot6::make_bind_parameters();
    for index in 0..MAX_PROTOCOLS {
        let driver = unsafe { PROTOCOLS[index] };
        if driver.is_null() {
            continue;
        }
        let chars = unsafe { (*driver).chars.as_ptr() };
        let slot = unsafe { ptr::read_unaligned(chars.add(BIND_SLOT) as *const *mut u8) };
        if slot.is_null() {
            continue;
        }
        let context = unsafe { (*driver).context };
        let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8) -> u32 =
            unsafe { core::mem::transmute(slot) };
        let status = unsafe { function(context, adapter, params) };
        unsafe { DbgPrint(b"NDIS BINDALL %08x\n\0".as_ptr(), status) };
    }
    prot6::free_bind_parameters(params);
    /* After bind and packet-filter OIDs, queue the interrupt DPC once so
     * receive path runs even when the line IRQ is quiet. */
    unsafe { crate::kick_interrupt_dpc(adapter) };
}

fn push_hex(line: &mut [u8], at: &mut usize, value: u8) {
    const DIGITS: &[u8] = b"0123456789abcdef";
    line[*at] = DIGITS[(value >> 4) as usize];
    line[*at + 1] = DIGITS[(value & 0xf) as usize];
    *at += 2;
}

fn log_body(chars: *const u8, size: u16) {
    let total = size as usize;
    let mut offset = 0usize;
    while offset < total {
        let mut line = [0u8; 80];
        let mut at = 0usize;
        for byte in b"NDIS PBODY " {
            line[at] = *byte;
            at += 1;
        }
        push_hex(&mut line, &mut at, (offset >> 8) as u8);
        push_hex(&mut line, &mut at, offset as u8);
        let count = (total - offset).min(16);
        for index in 0..count {
            line[at] = b' ';
            at += 1;
            let byte = unsafe { *chars.add(offset + index) };
            push_hex(&mut line, &mut at, byte);
        }
        line[at] = b'\n';
        line[at + 1] = 0;
        unsafe { DbgPrint(line.as_ptr()) };
        offset += 16;
    }
}

fn version_byte(chars: *const u8, size: u16, index: usize) -> Option<u8> {
    if (size as usize) < index + 1 {
        return None;
    }
    Some(unsafe { *chars.add(index) })
}

fn version_ok(chars: *const u8, size: u16) -> bool {
    version_byte(chars, size, VERSION_AT) == Some(VERSION_MAJOR)
        && version_byte(chars, size, VERSION_AT + 1) == Some(VERSION_MINOR)
}

fn netkvm_protocol(kind: u8, revision: u8, size: u16, chars: *const u8) -> bool {
    kind == PR_TYPE
        && revision == PR_REVISION
        && size as usize == PR_CHARS
        && version_byte(chars, size, VERSION_AT) == Some(VERSION_MAJOR)
        && version_byte(chars, size, VERSION_AT + 1) == Some(VERSION_MINOR_NETKVM)
}

fn call_slot(chars: *const u8, handle: *mut u8, context: *mut u8, offset: usize) -> u32 {
    let slot = unsafe { ptr::read_unaligned(chars.add(offset) as *const *mut u8) };
    if slot.is_null() {
        return 0;
    }
    let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    unsafe { function(handle, context, ptr::null_mut(), ptr::null_mut()) }
}

fn call_slots(chars: *const u8, handle: *mut u8, context: *mut u8) -> u32 {
    for offset in CALLED_SLOTS {
        let status = call_slot(chars, handle, context, offset);
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
    /* Capture 2026-11-16T054327Z accepted type 0x95, revision 2, size
     * 0x80, bytes 06 1e. output/netkvm-20261116T053850Z.log printed that
     * block from NetKVM after NdisRegisterDeviceEx.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterprotocoldriver */
    if chars.is_null() || out_handle.is_null() {
        return FAILURE;
    }
    let kind = *chars;
    let revision = *chars.add(1);
    let size = u16::from_le_bytes([*chars.add(2), *chars.add(3)]);
    unsafe {
        DbgPrint(
            b"NDIS PR %02x %02x %04x\n\0".as_ptr(),
            kind as u32,
            revision as u32,
            size as u32,
        );
    }
    if (size as usize) >= VERSION_AT + 2 {
        unsafe {
            DbgPrint(
                b"NDIS PVER %02x %02x\n\0".as_ptr(),
                *chars.add(VERSION_AT) as u32,
                *chars.add(VERSION_AT + 1) as u32,
            );
        }
    }
    let netkvm = netkvm_protocol(kind, revision, size, chars);
    if netkvm {
        log_body(chars, size);
    }
    if !version_ok(chars, size) && !netkvm {
        return BAD_VERSION;
    }
    if kind != PR_TYPE || revision != PR_REVISION || size as usize != PR_CHARS {
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
    let status = if netkvm {
        call_slot(chars, driver as *mut u8, context, SET_OPTIONS_SLOT)
    } else {
        call_slots(chars, driver as *mut u8, context)
    };
    if status != 0 {
        ExFreePoolWithTag(driver as *mut u8, POOL_TAG);
        return status;
    }
    track(driver);
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
    untrack(driver);
    (*driver).kind = 0;
    ExFreePoolWithTag(handle, POOL_TAG);
}

#[no_mangle]
pub unsafe extern "system" fn NdisSendNetBufferLists(
    binding: *mut u8,
    lists: *mut u8,
    port: u32,
    flags: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissendnetbufferlists
     * Binding kind and adapter offset match NdisOpenAdapterEx in lib.rs
     * (BINDING_KIND / BINDING_ADAPTER_AT). */
    const BINDING_KIND: u32 = u32::from_le_bytes(*b"BNso");
    const BINDING_ADAPTER_AT: usize = 8;
    if binding.is_null() || lists.is_null() {
        return;
    }
    if core::ptr::read_unaligned(binding as *const u32) != BINDING_KIND {
        return;
    }
    let adapter = core::ptr::read_unaligned(binding.add(BINDING_ADAPTER_AT) as *const *mut u8);
    crate::register::call_send_net_buffer_lists(adapter, lists, port, flags);
}

#[no_mangle]
pub unsafe extern "system" fn NdisReturnNetBufferLists(
    binding: *mut u8,
    lists: *mut u8,
    flags: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreturnnetbufferlists
     * Miniport ReturnNetBufferListsHandler is at characteristics 0x50
     * (capture 2026-11-15T161901Z INDICATE_SLOT). */
    const BINDING_KIND: u32 = u32::from_le_bytes(*b"BNso");
    const BINDING_ADAPTER_AT: usize = 8;
    if binding.is_null() || lists.is_null() {
        return;
    }
    if core::ptr::read_unaligned(binding as *const u32) != BINDING_KIND {
        return;
    }
    let adapter = core::ptr::read_unaligned(binding.add(BINDING_ADAPTER_AT) as *const *mut u8);
    crate::register::call_return_net_buffer_lists(adapter, lists, flags);
}
