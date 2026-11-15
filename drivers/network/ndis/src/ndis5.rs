/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/ndis5.rs
 * PURPOSE:     NDIS 4 exports that tcpip.sys imports
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * drivers/network/tcpip/tcpip.sys imports these names from ndis.sys.
 * sdk/include/ddk/ndis.h declares them. tcpip is built with NDIS40, so
 * the characteristics block is NDIS40_PROTOCOL_CHARACTERISTICS.
 * BindAdapterHandler follows the NDIS_STRING name in that struct.
 * sdk/include/xdk/mmtypes.h is the MDL. sdk/include/xdk/mmfuncs.h is
 * ADDRESS_AND_SIZE_TO_SPAN_PAGES and MmBuildMdlForNonPagedPool.
 * MmGetSystemAddressForMdlSafe maps a buffer that lacks
 * MDL_SOURCE_IS_NONPAGED_POOL. Capture 2026-11-15T215311Z stopped in
 * MmMapLockedPagesSpecifyCache on that path. A zero-length buffer returns 1.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndis_buffer_to_span_pages
 * OID values are in sdk/include/psdk/ntddndis.h. The station address is
 * the RTL8139 address printed by the disk boot.
 * VerosNoteTransmit is the miniport hook for NdisSend. It is not a
 * Windows export. Capture 2026-11-15T220043Z printed NDIS5 CHECK send
 * and the frame stayed in this driver. Capture 2026-11-15T220700Z
 * printed RTL8139 CHECK stack 0806 and 0800, so the frames left the
 * NIC. VerosIndicateFrame calls ReceiveHandler in
 * NDIS40_PROTOCOL_CHARACTERISTICS. That pointer follows the five
 * completion handlers and RequestCompleteHandler. The characteristics
 * block is the caller's stack, so the handler address is copied
 * during NdisRegisterProtocol. Capture 2026-11-15T221120Z bugchecked
 * 0x50 when the indication read that block later. Capture
 * 2026-11-15T221629Z then asserted DISPATCH_LEVEL in lan.c.
 */

use core::ptr;

const PAGE_SHIFT: usize = 12;
const PAGE_SIZE: usize = 1 << PAGE_SHIFT;
const SUCCESS: u32 = 0;
const NOT_SUPPORTED: u32 = 0xC000_00BB;
const RESOURCES: u32 = 0xC000_009A;
const POOL_TAG: u32 = u32::from_le_bytes(*b"5sdN");
const POOL_KIND: u32 = u32::from_le_bytes(*b"PKso");
/* NDIS40_PROTOCOL_CHARACTERISTICS: header, ten handlers, NDIS_STRING, ReceivePacketHandler. */
const BIND_AT: usize = 112;
const RECEIVE_AT: usize = 56;
const RECEIVE_COMPLETE_AT: usize = 64;
const OID_GEN_LINK_SPEED: u32 = 0x0001_0107;
const OID_GEN_MEDIA_CONNECT_STATUS: u32 = 0x0001_0114;
const OID_GEN_MEDIA_SUPPORTED: u32 = 0x0001_0103;
const OID_GEN_MAXIMUM_TOTAL_SIZE: u32 = 0x0001_0111;
const OID_GEN_PHYSICAL_MEDIUM: u32 = 0x0001_0202;
const OID_GEN_XMIT_ERROR: u32 = 0x0002_0103;
const OID_GEN_RCV_ERROR: u32 = 0x0002_0104;
const OID_GEN_MAXIMUM_FRAME_SIZE: u32 = 0x0001_0106;
const OID_GEN_MAXIMUM_SEND_PACKETS: u32 = 0x0001_0115;
const OID_802_3_CURRENT_ADDRESS: u32 = 0x0101_0102;
const MAC: [u8; 6] = [0x52, 0x54, 0x00, 0x12, 0x34, 0x56];

#[repr(C)]
struct Mdl {
    next: *mut Mdl,
    size: u16,
    flags: u16,
    _pad: u32,
    process: *mut u8,
    mapped: *mut u8,
    start: *mut u8,
    byte_count: u32,
    byte_offset: u32,
}

#[repr(C)]
struct Unicode {
    length: u16,
    maximum: u16,
    _pad: u32,
    buffer: *const u16,
}

struct Pool {
    kind: u32,
    reserved: u32,
}

static mut LAST_OID: u32 = 0xFFFF_FFFF;
static mut PROTOCOL: *mut u8 = ptr::null_mut();
static mut TCPIP_CONTEXT: *mut u8 = ptr::null_mut();
static mut RECEIVE: Option<
    unsafe extern "system" fn(*mut u8, *mut u8, *const u8, u32, *const u8, u32, u32) -> u32,
> = None;
static mut RECEIVE_DONE: Option<unsafe extern "system" fn(*mut u8)> = None;
static mut BINDING: u8 = 0;
static mut TRANSMIT: Option<unsafe extern "system" fn(*const u8, u32) -> u32> = None;

const FAILURE: u32 = 0xC000_0001;

unsafe extern "system" {
    fn DbgPrint(format: *const u8, ...) -> i32;
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
    fn IoAllocateMdl(
        address: *mut u8,
        length: u32,
        secondary: u8,
        charge: u8,
        irp: *mut u8,
    ) -> *mut u8;
    fn MmBuildMdlForNonPagedPool(mdl: *mut u8);
}

fn write_irql(irql: u64) {
    unsafe {
        core::arch::asm!("mov cr8, {}", in(reg) irql, options(nostack, preserves_flags));
    }
}

fn current_irql() -> u64 {
    let irql: u64;
    unsafe {
        core::arch::asm!("mov {}, cr8", out(reg) irql, options(nostack, preserves_flags));
    }
    irql
}
fn line(text: &[u8]) {
    unsafe { DbgPrint(text.as_ptr()); }
}

fn line_status(text: &[u8], status: u32) {
    unsafe { DbgPrint(text.as_ptr(), status); }
}

fn span_pages(buffer: *const Mdl) -> u32 {
    if buffer.is_null() {
        return 1;
    }
    let count = unsafe { (*buffer).byte_count } as usize;
    if count == 0 {
        return 1;
    }
    let address = unsafe { (*buffer).start as usize + (*buffer).byte_offset as usize };
    (((address & (PAGE_SIZE - 1)) + count + PAGE_SIZE - 1) >> PAGE_SHIFT) as u32
}

fn write_status(status: *mut u32, value: u32) {
    if !status.is_null() {
        unsafe { *status = value; }
    }
}

fn query_field(request: *mut u8, at: usize) -> u32 {
    unsafe { ptr::read_unaligned(request.add(at) as *const u32) }
}

fn query_buffer(request: *mut u8) -> *mut u8 {
    unsafe { ptr::read_unaligned(request.add(48) as *const *mut u8) }
}

fn answer_query(request: *mut u8) -> u32 {
    /* NDIS_REQUEST: MacReserved, RequestType, then the query union. */
    let oid = query_field(request, 40);
    let length = query_field(request, 56);
    let buffer = query_buffer(request);
    if oid != unsafe { LAST_OID } {
        line_status(b"NDIS5 CHECK oid %08x\n\0", oid);
        unsafe { LAST_OID = oid; }
    }
    let (bytes, value) = match oid {
        OID_GEN_MAXIMUM_SEND_PACKETS => (4u32, 1u32),
        OID_GEN_MAXIMUM_FRAME_SIZE => (4, 1500),
        OID_GEN_LINK_SPEED => (4, 1_000_000),
        OID_GEN_MEDIA_CONNECT_STATUS => (4, 0),
        OID_GEN_MEDIA_SUPPORTED => (4, 0),
        OID_GEN_MAXIMUM_TOTAL_SIZE => (4, 1514),
        OID_GEN_PHYSICAL_MEDIUM => (4, 14),
        OID_GEN_XMIT_ERROR => (4, 0),
        OID_GEN_RCV_ERROR => (4, 0),
        OID_802_3_CURRENT_ADDRESS => (6, 0),
        _ => return NOT_SUPPORTED,
    };
    if buffer.is_null() || length < bytes {
        return NOT_SUPPORTED;
    }
    if oid == OID_802_3_CURRENT_ADDRESS {
        unsafe { ptr::copy_nonoverlapping(MAC.as_ptr(), buffer, 6); }
    } else {
        unsafe { ptr::write_unaligned(buffer as *mut u32, value); }
    }
    unsafe { ptr::write_unaligned(request.add(60) as *mut u32, bytes); }
    SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn NDIS_BUFFER_TO_SPAN_PAGES(buffer: *const Mdl) -> u32 {
    span_pages(buffer)
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateBufferPool(
    status: *mut u32,
    handle: *mut *mut u8,
    _count: u32,
) {
    write_status(status, SUCCESS);
    if !handle.is_null() {
        *handle = 1 as *mut u8;
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateBuffer(
    status: *mut u32,
    buffer: *mut *mut u8,
    _pool: *mut u8,
    address: *mut u8,
    length: u32,
) {
    let mdl = IoAllocateMdl(address, length, 0, 0, ptr::null_mut());
    if mdl.is_null() {
        write_status(status, RESOURCES);
        return;
    }
    if length != 0 {
        MmBuildMdlForNonPagedPool(mdl);
    }
    if !buffer.is_null() {
        *buffer = mdl;
    }
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeBufferPool(_pool: *mut u8) {}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocatePacketPool(
    status: *mut u32,
    handle: *mut *mut u8,
    count: u32,
    reserved: u32,
) {
    NdisAllocatePacketPoolEx(status, handle, count, 0, reserved);
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocatePacketPoolEx(
    status: *mut u32,
    handle: *mut *mut u8,
    _count: u32,
    _overflow: u32,
    reserved: u32,
) {
    let pool = ExAllocatePoolWithTag(0, size_of::<Pool>(), POOL_TAG) as *mut Pool;
    if pool.is_null() {
        write_status(status, RESOURCES);
        return;
    }
    (*pool).kind = POOL_KIND;
    (*pool).reserved = reserved;
    if !handle.is_null() {
        *handle = pool as *mut u8;
    }
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreePacketPool(pool: *mut u8) {
    if pool.is_null() {
        return;
    }
    let header = pool as *mut Pool;
    if (*header).kind != POOL_KIND {
        return;
    }
    (*header).kind = 0;
    ExFreePoolWithTag(pool, POOL_TAG);
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocatePacket(
    status: *mut u32,
    packet: *mut *mut u8,
    pool: *mut u8,
) {
    let mut bytes = 256usize;
    if !pool.is_null() {
        let header = pool as *mut Pool;
        if (*header).kind == POOL_KIND {
            bytes += (*header).reserved as usize;
        }
    }
    let block = ExAllocatePoolWithTag(0, bytes, POOL_TAG);
    if block.is_null() {
        write_status(status, RESOURCES);
        return;
    }
    ptr::write_bytes(block, 0, bytes);
    if !packet.is_null() {
        *packet = block;
    }
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreePacket(packet: *mut u8) {
    if !packet.is_null() {
        ExFreePoolWithTag(packet, POOL_TAG);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisGetFirstBufferFromPacket(
    packet: *mut u8,
    first: *mut *mut u8,
    address: *mut *mut u8,
    length: *mut u32,
    total: *mut u32,
) {
    let head = if packet.is_null() {
        ptr::null_mut()
    } else {
        ptr::read_unaligned(packet.add(8) as *const *mut Mdl)
    };
    if !first.is_null() {
        *first = head as *mut u8;
    }
    let (va, count) = if head.is_null() {
        (ptr::null_mut(), 0)
    } else {
        (
            unsafe { (*head).start.add((*head).byte_offset as usize) },
            unsafe { (*head).byte_count },
        )
    };
    if !address.is_null() {
        *address = va;
    }
    if !length.is_null() {
        *length = count;
    }
    if !total.is_null() {
        *total = if packet.is_null() {
            0
        } else {
            ptr::read_unaligned(packet.add(4) as *const u32)
        };
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisRegisterProtocol(
    status: *mut u32,
    handle: *mut *mut u8,
    chars: *mut u8,
    length: u32,
) {
    line_status(b"NDIS5 CHECK register %08x\n\0", length);
    if chars.is_null() || length < (BIND_AT + 8) as u32 {
        write_status(status, NOT_SUPPORTED);
        return;
    }
    PROTOCOL = chars;
    if RECEIVE.is_none() {
        let receive = ptr::read_unaligned(chars.add(RECEIVE_AT) as *const *mut u8);
        let complete = ptr::read_unaligned(chars.add(RECEIVE_COMPLETE_AT) as *const *mut u8);
        if !receive.is_null() {
            RECEIVE = Some(core::mem::transmute(receive));
            if !complete.is_null() {
                RECEIVE_DONE = Some(core::mem::transmute(complete));
            }
        }
    }
    line_status(b"NDIS5 CHECK protocol %08x\n\0", PROTOCOL as usize as u32);
    if !handle.is_null() {
        *handle = chars;
    }
    write_status(status, SUCCESS);
    let slot = ptr::read_unaligned(chars.add(BIND_AT) as *const *mut u8);
    if slot.is_null() {
        line(b"NDIS5 CHECK bind none\n\0");
        return;
    }
    let bind: unsafe extern "system" fn(*mut u32, *mut u8, *mut Unicode, *mut u8, *mut u8) =
        core::mem::transmute(slot);
    let mut device: [u16; 18] = [
        '\\' as u16, 'D' as u16, 'e' as u16, 'v' as u16, 'i' as u16, 'c' as u16, 'e' as u16,
        '\\' as u16, 'R' as u16, 't' as u16, 'l' as u16, '8' as u16, '1' as u16, '3' as u16,
        '9' as u16, 0, 0, 0,
    ];
    let mut registry: [u16; 64] = [0; 64];
    let path = "\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Tcpip";
    let mut index = 0;
    for unit in path.encode_utf16() {
        registry[index] = unit;
        index += 1;
    }
    let mut device_name = Unicode {
        length: 15 * 2,
        maximum: 16 * 2,
        _pad: 0,
        buffer: device.as_ptr(),
    };
    let mut registry_name = Unicode {
        length: (index * 2) as u16,
        maximum: (index * 2) as u16,
        _pad: 0,
        buffer: registry.as_ptr(),
    };
    let mut bind_status = NOT_SUPPORTED;
    bind(
        &mut bind_status,
        ptr::null_mut(),
        &mut device_name,
        &mut registry_name as *mut Unicode as *mut u8,
        ptr::null_mut(),
    );
    line_status(b"NDIS5 CHECK bind %08x\n\0", bind_status);
    let _ = device[15];
}

#[no_mangle]
pub unsafe extern "system" fn NdisDeregisterProtocol(status: *mut u32, _handle: *mut u8) {
    PROTOCOL = ptr::null_mut();
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisOpenAdapter(
    status: *mut u32,
    open_status: *mut u32,
    handle: *mut *mut u8,
    medium: *mut u32,
    _media: *mut u32,
    _count: u32,
    _protocol: *mut u8,
    context: *mut u8,
    _name: *mut Unicode,
    _options: u32,
    _addressing: *mut u8,
) {
    line(b"NDIS5 CHECK open\n\0");
    write_status(status, SUCCESS);
    write_status(open_status, SUCCESS);
    if !handle.is_null() {
        *handle = ptr::addr_of_mut!(BINDING);
    }
    if !medium.is_null() {
        *medium = 0;
    }
    if TCPIP_CONTEXT.is_null() {
        TCPIP_CONTEXT = context;
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisCloseAdapter(status: *mut u32, _handle: *mut u8) {
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisRequest(
    status: *mut u32,
    _binding: *mut u8,
    request: *mut u8,
) {
    if request.is_null() {
        write_status(status, NOT_SUPPORTED);
        return;
    }
    let kind = query_field(request, 32);
    let result = if kind == 1 {
        SUCCESS
    } else {
        answer_query(request)
    };
    write_status(status, result);
}

#[no_mangle]
fn copy_packet(packet: *mut u8, frame: &mut [u8]) -> Option<usize> {
    if packet.is_null() {
        return None;
    }
    let mut mdl = unsafe { ptr::read_unaligned(packet.add(8) as *const *const Mdl) };
    let mut filled = 0usize;
    while !mdl.is_null() {
        let count = unsafe { (*mdl).byte_count } as usize;
        let source = unsafe { (*mdl).mapped };
        if source.is_null() || filled + count > frame.len() {
            return None;
        }
        unsafe { ptr::copy_nonoverlapping(source, frame.as_mut_ptr().add(filled), count); }
        filled += count;
        mdl = unsafe { (*mdl).next };
    }
    if filled == 0 {
        None
    } else {
        Some(filled)
    }
}

#[no_mangle]
pub unsafe extern "system" fn VerosIndicateFrame(frame: *const u8, length: u32) {
    if frame.is_null() || length < 14 || TCPIP_CONTEXT.is_null() {
        line(b"NDIS5 CHECK recv none\n\0");
        return;
    }
    let Some(indicate) = RECEIVE else {
        line(b"NDIS5 CHECK recv none\n\0");
        return;
    };
    let kind = u16::from_be_bytes([*frame.add(12), *frame.add(13)]);
    let payload = length - 14;
    /* sdk/include/xdk/amd64/ke.h reads IRQL from cr8. DISPATCH_LEVEL is 2.
     * lan.c ProtocolTransferDataComplete asserts that level. */
    let old_irql = current_irql();
    write_irql(2);
    let status = indicate(
        TCPIP_CONTEXT,
        ptr::null_mut(),
        frame,
        14,
        frame.add(14),
        payload,
        payload,
    );
    if let Some(done) = RECEIVE_DONE {
        done(TCPIP_CONTEXT);
    }
    write_irql(old_irql);
    line_status(b"NDIS5 CHECK recv %04x\n\0", kind as u32);
    line_status(b"NDIS5 CHECK recvst %08x\n\0", status);
    if kind == 0x0800 && length >= 34 {
        let proto = *frame.add(23);
        line_status(b"NDIS5 CHECK proto %02x\n\0", proto as u32);
        if proto == 17 && length >= 42 {
            let source = u16::from_be_bytes([*frame.add(34), *frame.add(35)]);
            let dest = u16::from_be_bytes([*frame.add(36), *frame.add(37)]);
            line_status(b"NDIS5 CHECK sport %04x\n\0", source as u32);
            line_status(b"NDIS5 CHECK dport %04x\n\0", dest as u32);
        }
    }
}

#[no_mangle]
pub unsafe extern "system" fn VerosNoteTransmit(
    transmit: Option<unsafe extern "system" fn(*const u8, u32) -> u32>,
) {
    TRANSMIT = transmit;
}

#[no_mangle]
pub unsafe extern "system" fn NdisSend(status: *mut u32, _binding: *mut u8, packet: *mut u8) {
    let mut frame = [0u8; 1514];
    let Some(length) = copy_packet(packet, &mut frame) else {
        line(b"NDIS5 CHECK send none\n\0");
        write_status(status, FAILURE);
        return;
    };
    let kind = u16::from_be_bytes([frame[12], frame[13]]);
    line_status(b"NDIS5 CHECK send %04x\n\0", kind as u32);
    let transmit = TRANSMIT;
    let Some(transmit) = transmit else {
        write_status(status, FAILURE);
        return;
    };
    let result = transmit(frame.as_ptr(), length as u32);
    write_status(status, result);
}

#[no_mangle]
pub unsafe extern "system" fn NdisTransferData(
    status: *mut u32,
    _binding: *mut u8,
    _context: *mut u8,
    _offset: u32,
    _bytes: u32,
    _packet: *mut u8,
    transferred: *mut u32,
) {
    if !transferred.is_null() {
        *transferred = 0;
    }
    write_status(status, NOT_SUPPORTED);
}

#[no_mangle]
pub unsafe extern "system" fn NdisUnchainBufferAtFront(packet: *mut u8, buffer: *mut *mut u8) {
    if packet.is_null() {
        return;
    }
    let head = ptr::read_unaligned(packet.add(8) as *const *mut u8);
    if !buffer.is_null() {
        *buffer = head;
    }
    let next = if head.is_null() {
        ptr::null_mut()
    } else {
        ptr::read_unaligned(head as *const *mut u8)
    };
    ptr::write_unaligned(packet.add(8) as *mut *mut u8, next);
    if next.is_null() {
        ptr::write_unaligned(packet.add(16) as *mut *mut u8, ptr::null_mut());
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisReturnPackets(_packets: *mut *mut u8, _count: u32) {}
