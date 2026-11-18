/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/prot6.rs
 * PURPOSE:     NDIS 6.84 protocol bind for tcpip
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * Windows 10 compatible NDIS 6 protocol path for tcpip. Registration uses
 * the measured characteristics header type 0x95, revision 2, size 0x80,
 * version bytes 6 and 84 from output/ndis-discovery/windows.log
 * (DIFF END OK) and differential-tests/ndis-discovery/METHOD.md.
 * Handler offsets follow
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_protocol_driver_characteristics
 * with SetOptionsHandler at 0x20 (capture 2026-11-15T162555Z) and
 * BindAdapterHandlerEx at 0x28. output/netkvm-20261116T083539Z.log
 * printed that layout in NDIS PBODY. BindAdapterEx calls NdisOpenAdapterEx
 * per
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-protocol_bind_adapter_ex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenadapterex
 * NDIS_OPEN_PARAMETERS member order is Header, AdapterName, MediumArray,
 * MediumArraySize, SelectedMediumIndex
 * (https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_open_parameters).
 * On x64 AdapterName is at offset 8 and MediumArray at 16; those offsets
 * are this derivation, not a Windows dump. The open-parameters Header
 * type byte is not measured yet; Size is this file's buffer length.
 * NDIS_BIND_PARAMETERS puts AdapterName after ProtocolSection
 * (https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_bind_parameters);
 * on x64 AdapterName is at offset 16. NdisMedium802_3 is 0 in
 * sdk/include/ddk/ndis.h. LanRegisterProtocol6 is the VerOS tcpip entry
 * that calls NdisRegisterProtocolDriver. Phase C removed the NDIS 5
 * tcpip bridge; output/ndis-disk-20261116T203348Z.log and
 * output/netkvm-20261116T203635Z.log carry PingCheck on NDIS 6.
 */

use core::ptr;

use crate::protocol;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
const PR_TYPE: u8 = 0x95;
const PR_REVISION: u8 = 2;
const PR_CHARS: usize = 0x80;
const VERSION_AT: usize = 4;
const NAME_AT: usize = 0x10;
const SET_OPTIONS_AT: usize = 0x20;
const BIND_AT: usize = 0x28;
const UNBIND_AT: usize = 0x30;
const OPEN_COMPLETE_AT: usize = 0x38;
const CLOSE_COMPLETE_AT: usize = 0x40;
const OID_COMPLETE_AT: usize = 0x58;
const STATUS_AT: usize = 0x60;
const RECEIVE_AT: usize = 0x68;
const SEND_COMPLETE_AT: usize = 0x70;
const SUCCESS: u32 = 0;
const FAILURE: u32 = 0xC000_0001;
const RESOURCES: u32 = 0xC000_009A;
const OPEN_BYTES: usize = 0x40;
const OPEN_NAME_AT: usize = 8;
const OPEN_MEDIUM_AT: usize = 16;
const OPEN_MEDIUM_COUNT_AT: usize = 24;
const OPEN_SELECTED_AT: usize = 32;
const BIND_BYTES: usize = 0x100;
const BIND_NAME_AT: usize = 16;
const BIND_MEDIA_AT: usize = 32;
const NDIS_MEDIUM_802_3: u32 = 0;

#[repr(C)]
pub struct UnicodeString {
    pub length: u16,
    pub maximum_length: u16,
    pub buffer: *mut u16,
}

static mut PROTOCOL_HANDLE: *mut u8 = ptr::null_mut();
static mut NAME_BUF: [u16; 16] = [0; 16];
static mut NAME_STR: UnicodeString = UnicodeString {
    length: 0,
    maximum_length: 0,
    buffer: ptr::null_mut(),
};
static mut MEDIUM: u32 = NDIS_MEDIUM_802_3;
static mut SELECTED: u32 = 0;
static mut ADAPTER_NAME_BUF: [u16; 32] = [0; 32];
static mut ADAPTER_NAME: UnicodeString = UnicodeString {
    length: 0,
    maximum_length: 0,
    buffer: ptr::null_mut(),
};

/* tcpip registers these so ndis.sys does not import tcpip.sys. */
type Bind6Fn = unsafe extern "system" fn(*mut u8, *mut UnicodeString) -> i32;
type SendDone6Fn = unsafe extern "system" fn(*mut u8, *mut u8, u32);
type OidDone6Fn = unsafe extern "system" fn(*mut u8, u32);
type ReceiveNbl6Fn = unsafe extern "system" fn(*mut u8, *mut u8, u32, u32, u32);
static mut BIND6: Option<Bind6Fn> = None;
static mut SEND_DONE6: Option<SendDone6Fn> = None;
static mut OID_DONE6: Option<OidDone6Fn> = None;
static mut RECEIVE_NBL6: Option<ReceiveNbl6Fn> = None;
/* adapter handle -> binding handle for send-complete. */
const MAP_SLOTS: usize = 8;
static mut BINDING_MAP: [(*mut u8, *mut u8); MAP_SLOTS] =
    [(ptr::null_mut(), ptr::null_mut()); MAP_SLOTS];
const BINDING_KIND: u32 = u32::from_le_bytes(*b"BNso");
const BINDING_ADAPTER_AT: usize = 8;
const BINDING_CONTEXT_AT: usize = 16;

#[no_mangle]
pub unsafe extern "system" fn LanSetBindAdapter6(handler: Bind6Fn) {
    BIND6 = Some(handler);
}

#[no_mangle]
pub unsafe extern "system" fn LanSetSendComplete6(handler: SendDone6Fn) {
    SEND_DONE6 = Some(handler);
}

#[no_mangle]
pub unsafe extern "system" fn LanSetOidComplete6(handler: OidDone6Fn) {
    OID_DONE6 = Some(handler);
}

#[no_mangle]
pub unsafe extern "system" fn LanSetReceiveNbl6(handler: ReceiveNbl6Fn) {
    RECEIVE_NBL6 = Some(handler);
}

#[no_mangle]
pub unsafe extern "system" fn LanSetBindingContext6(binding: *mut u8, context: *mut u8) {
    /* Binding layout from NdisOpenAdapterEx in lib.rs. */
    if binding.is_null() || ptr::read_unaligned(binding as *const u32) != BINDING_KIND {
        return;
    }
    ptr::write_unaligned(binding.add(BINDING_CONTEXT_AT) as *mut *mut u8, context);
    let adapter = ptr::read_unaligned(binding.add(BINDING_ADAPTER_AT) as *const *mut u8);
    if adapter.is_null() {
        return;
    }
    for slot in BINDING_MAP.iter_mut() {
        if slot.0.is_null() || slot.0 == adapter {
            *slot = (adapter, binding);
            return;
        }
    }
}

pub fn binding_for_adapter(adapter: *mut u8) -> *mut u8 {
    if adapter.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        for slot in BINDING_MAP.iter() {
            if slot.0 == adapter {
                return slot.1;
            }
        }
    }
    ptr::null_mut()
}

pub fn binding_context(binding: *mut u8) -> *mut u8 {
    if binding.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        if ptr::read_unaligned(binding as *const u32) != BINDING_KIND {
            return ptr::null_mut();
        }
        ptr::read_unaligned(binding.add(BINDING_CONTEXT_AT) as *const *mut u8)
    }
}

fn put_u16(chars: &mut [u8], at: usize, value: u16) {
    chars[at] = value as u8;
    chars[at + 1] = (value >> 8) as u8;
}

fn put_ptr(chars: &mut [u8], at: usize, value: usize) {
    let bytes = value.to_le_bytes();
    chars[at..at + 8].copy_from_slice(&bytes);
}

fn write_u16(block: *mut u8, at: usize, value: u16) {
    unsafe {
        *block.add(at) = value as u8;
        *block.add(at + 1) = (value >> 8) as u8;
    }
}

unsafe extern "system" fn set_options(_driver: *mut u8, _context: *mut u8) -> u32 {
    protocol::dbg(b"NDIS6 SETOPT\n\0");
    SUCCESS
}

unsafe extern "system" fn bind_adapter(
    _driver_context: *mut u8,
    bind_context: *mut u8,
    bind_parameters: *mut u8,
) -> u32 {
    protocol::dbg(b"NDIS6 BIND\n\0");
    if PROTOCOL_HANDLE.is_null() {
        return FAILURE;
    }
    let name = if !bind_parameters.is_null() {
        ptr::read_unaligned(bind_parameters.add(BIND_NAME_AT) as *const *mut UnicodeString)
    } else {
        ptr::null_mut()
    };
    let open = protocol::alloc_bytes(OPEN_BYTES);
    if open.is_null() {
        return RESOURCES;
    }
    ptr::write_bytes(open, 0, OPEN_BYTES);
    write_u16(open, 2, OPEN_BYTES as u16);
    let name_ptr = if name.is_null() {
        ptr::addr_of_mut!(ADAPTER_NAME) as *mut u8
    } else {
        name as *mut u8
    };
    ptr::write_unaligned(open.add(OPEN_NAME_AT) as *mut *mut u8, name_ptr);
    ptr::write_unaligned(
        open.add(OPEN_MEDIUM_AT) as *mut *mut u32,
        ptr::addr_of_mut!(MEDIUM),
    );
    ptr::write_unaligned(open.add(OPEN_MEDIUM_COUNT_AT) as *mut u32, 1);
    ptr::write_unaligned(
        open.add(OPEN_SELECTED_AT) as *mut *mut u32,
        ptr::addr_of_mut!(SELECTED),
    );
    let mut binding: *mut u8 = ptr::null_mut();
    let status = crate::NdisOpenAdapterEx(
        PROTOCOL_HANDLE,
        bind_context,
        open,
        bind_context,
        &mut binding,
    );
    protocol::dbg_status_ptr(b"NDIS6 OPEN %08x %p\n\0", status, binding);
    protocol::free_bytes(open);
    if status == 0 && !binding.is_null() {
        if let Some(bind6) = BIND6 {
            let name_for_bind = if name.is_null() {
                ptr::addr_of_mut!(ADAPTER_NAME)
            } else {
                name
            };
            let bind_status = bind6(binding, name_for_bind);
            protocol::dbg_status(b"NDIS6 LAN %08x\n\0", bind_status as u32);
            if bind_status != 0 {
                return bind_status as u32;
            }
        }
    }
    status
}

unsafe extern "system" fn unbind_adapter(_binding_context: *mut u8) -> u32 {
    protocol::dbg(b"NDIS6 UNBIND\n\0");
    SUCCESS
}

unsafe extern "system" fn open_complete(_binding_context: *mut u8, status: u32) {
    protocol::dbg_status(b"NDIS6 OPENDONE %08x\n\0", status);
}

unsafe extern "system" fn close_complete(_binding_context: *mut u8) {
    protocol::dbg(b"NDIS6 CLOSEDONE\n\0");
}

unsafe extern "system" fn oid_complete(
    binding_context: *mut u8,
    _request: *mut u8,
    status: u32,
) {
    protocol::dbg_status(b"NDIS6 OIDDONE %08x\n\0", status);
    if let Some(done) = OID_DONE6 {
        done(binding_context, status);
    }
}

unsafe extern "system" fn status_ex(_binding_context: *mut u8, indication: *const u8) {
    if indication.is_null() {
        protocol::dbg(b"NDIS6 STAT null\n\0");
        return;
    }
    let kind = *indication;
    let revision = *indication.add(1);
    let size = u16::from_le_bytes([*indication.add(2), *indication.add(3)]);
    protocol::dbg_hdr(b"NDIS6 STAT %02x %02x %04x\n\0", kind, revision, size);
}

unsafe extern "system" fn receive_nbls(
    binding_context: *mut u8,
    lists: *mut u8,
    port: u32,
    count: u32,
    flags: u32,
) {
    protocol::dbg(b"NDIS6 RECV\n\0");
    if let Some(recv) = RECEIVE_NBL6 {
        recv(binding_context, lists, port, count, flags);
    }
}

pub fn indicate_receive(adapter: *mut u8, lists: *mut u8, port: u32, count: u32, flags: u32) {
    /* Return lists to the miniport when no protocol binding is ready.
     * output/netkvm-20261116T094739Z.log printed NDIS RECVIND before
     * NDIS6 LAN; LanReceiveNbl6 saw a null context and returned without
     * NdisReturnNetBufferLists, which starved later indicates.
     * call_return_net_buffer_lists no-ops when the return slot is null
     * (NDIS CHECK throwaway handles).
     * Fan out to secondary opens (lldp.sys) before tcpip so they can
     * copy EtherType 0x88CC; tcpip still owns NdisReturnNetBufferLists
     * (output/ndis-disk-20261116T214838Z.log had LLDP TX with no RX). */
    let binding = binding_for_adapter(adapter);
    let context = binding_context(binding);
    if context.is_null() || lists.is_null() {
        unsafe { protocol::dbg(b"NDIS6 RECV drop\n\0") };
        if !lists.is_null() {
            crate::register::call_return_net_buffer_lists(adapter, lists, flags);
        }
        return;
    }
    let primary_protocol = unsafe {
        if !binding.is_null() {
            ptr::read_unaligned(binding.add(24) as *const *mut u8) /* BINDING_PROTOCOL_AT */
        } else {
            ptr::null_mut()
        }
    };
    protocol::indicate_secondary(adapter, lists, port, count, flags, primary_protocol);
    unsafe {
        receive_nbls(context, lists, port, count, flags);
    }
}

unsafe extern "system" fn send_complete(
    binding_context: *mut u8,
    lists: *mut u8,
    flags: u32,
) {
    protocol::dbg(b"NDIS6 SENDDONE\n\0");
    if let Some(done) = SEND_DONE6 {
        done(binding_context, lists, flags);
    }
}

pub fn complete_send(adapter: *mut u8, lists: *mut u8, flags: u32) {
    let binding = binding_for_adapter(adapter);
    let context = binding_context(binding);
    unsafe {
        send_complete(context, lists, flags);
    }
}

pub fn complete_oid(adapter: *mut u8, request: *mut u8, status: u32) {
    let binding = binding_for_adapter(adapter);
    let context = binding_context(binding);
    unsafe {
        oid_complete(context, request, status);
    }
}

fn fill_protocol_name(name: *const UnicodeString) {
    unsafe {
        NAME_BUF = [0; 16];
        let len = if name.is_null() || (*name).buffer.is_null() {
            let fallback: [u16; 6] = [
                b'T' as u16, b'C' as u16, b'P' as u16, b'I' as u16, b'P' as u16, 0,
            ];
            NAME_BUF[..6].copy_from_slice(&fallback);
            10u16
        } else {
            let units = ((*name).length / 2).min(15) as usize;
            ptr::copy_nonoverlapping((*name).buffer, NAME_BUF.as_mut_ptr(), units);
            (*name).length.min(30)
        };
        NAME_STR.length = len;
        NAME_STR.maximum_length = 32;
        NAME_STR.buffer = NAME_BUF.as_mut_ptr();
    }
}

fn fill_adapter_name() {
    /* NdisOpenAdapterEx may ignore the name; keep a generic device string. */
    let text: [u16; 16] = [
        b'\\' as u16, b'D' as u16, b'e' as u16, b'v' as u16, b'i' as u16, b'c' as u16,
        b'e' as u16, b'\\' as u16, b'E' as u16, b't' as u16, b'h' as u16, b'e' as u16,
        b'r' as u16, b'n' as u16, b'e' as u16, b't' as u16,
    ];
    unsafe {
        ADAPTER_NAME_BUF = [0; 32];
        ADAPTER_NAME_BUF[..16].copy_from_slice(&text);
        ADAPTER_NAME.length = 32;
        ADAPTER_NAME.maximum_length = 64;
        ADAPTER_NAME.buffer = ADAPTER_NAME_BUF.as_mut_ptr();
    }
}

#[no_mangle]
pub unsafe extern "system" fn LanRegisterProtocol6(name: *const UnicodeString) -> i32 {
    protocol::dbg(b"NDIS6 REG\n\0");
    if !PROTOCOL_HANDLE.is_null() {
        return SUCCESS as i32;
    }
    fill_protocol_name(name);
    fill_adapter_name();
    let mut chars = [0u8; PR_CHARS];
    chars[0] = PR_TYPE;
    chars[1] = PR_REVISION;
    put_u16(&mut chars, 2, PR_CHARS as u16);
    chars[VERSION_AT] = 6;
    chars[VERSION_AT + 1] = 84;
    put_u16(&mut chars, NAME_AT, NAME_STR.length);
    put_u16(&mut chars, NAME_AT + 2, NAME_STR.maximum_length);
    put_ptr(&mut chars, NAME_AT + 8, NAME_STR.buffer as usize);
    put_ptr(&mut chars, SET_OPTIONS_AT, set_options as *const () as usize);
    put_ptr(&mut chars, BIND_AT, bind_adapter as *const () as usize);
    put_ptr(&mut chars, UNBIND_AT, unbind_adapter as *const () as usize);
    put_ptr(
        &mut chars,
        OPEN_COMPLETE_AT,
        open_complete as *const () as usize,
    );
    put_ptr(
        &mut chars,
        CLOSE_COMPLETE_AT,
        close_complete as *const () as usize,
    );
    put_ptr(&mut chars, OID_COMPLETE_AT, oid_complete as *const () as usize);
    put_ptr(&mut chars, STATUS_AT, status_ex as *const () as usize);
    put_ptr(&mut chars, RECEIVE_AT, receive_nbls as *const () as usize);
    put_ptr(
        &mut chars,
        SEND_COMPLETE_AT,
        send_complete as *const () as usize,
    );
    let mut handle: *mut u8 = ptr::null_mut();
    let status = protocol::NdisRegisterProtocolDriver(ptr::null_mut(), chars.as_ptr(), &mut handle);
    protocol::dbg_status_ptr(b"NDIS6 REG %08x %p\n\0", status, handle);
    if status == 0 {
        PROTOCOL_HANDLE = handle;
    }
    status as i32
}

#[no_mangle]
pub unsafe extern "system" fn VerosBindReadyProtocols() {
    /* Call from tcpip after LanSetBindAdapter6 / LanSetReceiveNbl6.
     * output/ndis-disk-20261116T200400Z.log: BINDALL succeeded but
     * NDIS6 LAN never ran because BIND6 was still null. */
    protocol::bind_ready_protocols();
}

#[no_mangle]
pub unsafe extern "system" fn LanDeregisterProtocol6() {
    if PROTOCOL_HANDLE.is_null() {
        return;
    }
    protocol::NdisDeregisterProtocolDriver(PROTOCOL_HANDLE);
    PROTOCOL_HANDLE = ptr::null_mut();
}

pub fn make_bind_parameters() -> *mut u8 {
    let block = protocol::alloc_bytes(BIND_BYTES);
    if block.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        ptr::write_bytes(block, 0, BIND_BYTES);
        write_u16(block, 2, BIND_BYTES as u16);
        ptr::write_unaligned(
            block.add(BIND_NAME_AT) as *mut *mut UnicodeString,
            ptr::addr_of_mut!(ADAPTER_NAME),
        );
        ptr::write_unaligned(block.add(BIND_MEDIA_AT) as *mut u32, NDIS_MEDIUM_802_3);
    }
    block
}

pub fn free_bind_parameters(block: *mut u8) {
    protocol::free_bytes(block);
}
