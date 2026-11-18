/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/lldp/src/lib.rs
 * PURPOSE:     IEEE 802.1AB LLDP as an NDIS 6.84 protocol
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * LLDPDU layout, EtherType 0x88CC, nearest-bridge multicast destination
 * 01-80-C2-00-00-0E, and mandatory TLV types Chassis ID (1), Port ID (2),
 * and TTL (3) come from IEEE Std 802.1AB-2016 (Station and Media Access
 * Control Connectivity Discovery), a public IEEE standard.
 * https://standards.ieee.org/ieee/802.1AB/6047/
 * NDIS 6 protocol registration uses the measured characteristics header
 * type 0x95, revision 2, size 0x80, version 6.84 from
 * drivers/network/ndis/src/prot6.rs and differential-tests/ndis-discovery/METHOD.md.
 * Handler offsets follow
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_protocol_driver_characteristics
 * BindAdapterEx / NdisOpenAdapterEx follow
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-protocol_bind_adapter_ex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenadapterex
 * NdisSendNetBufferLists:
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissendnetbufferlists
 * NET_BUFFER / NET_BUFFER_LIST field offsets match drivers/network/ndis/src/pool.rs
 * and drivers/network/tcpip/datalink/lan.c.
 */

#![no_std]

use core::panic::PanicInfo;
use core::ptr;
use core::sync::atomic::{AtomicU32, Ordering};

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

const OPEN_BYTES: usize = 0x40;
const OPEN_NAME_AT: usize = 8;
const OPEN_MEDIUM_AT: usize = 16;
const OPEN_MEDIUM_COUNT_AT: usize = 24;
const OPEN_SELECTED_AT: usize = 32;
const NDIS_MEDIUM_802_3: u32 = 0;

const NBL_NEXT_AT: usize = 0;
const NBL_FIRST_NB_AT: usize = 8;
const NB_LENGTH_AT: usize = 0x18;
const NB_MDL_AT: usize = 0x20;
const NB_OFFSET_AT: usize = 0x28;
const NBL_LENGTH_AT: usize = 0x198;
const NBL_MDL_AT: usize = 0x1A0;
const NBL_OFFSET_AT: usize = 0x1A8;

const ETH_DST: usize = 0;
const ETH_SRC: usize = 6;
const ETH_TYPE: usize = 12;
const ETH_HDR: usize = 14;
const ETHERTYPE_LLDP: u16 = 0x88CC;
const TTL_SECONDS: u16 = 120;
const TLV_CHASSIS: u8 = 1;
const TLV_PORT: u8 = 2;
const TLV_TTL: u8 = 3;
const TLV_END: u8 = 0;
const CHASSIS_SUBTYPE_LOCAL: u8 = 7;
const PORT_SUBTYPE_LOCAL: u8 = 7;

const SUCCESS: u32 = 0;
const PENDING: u32 = 0x0000_0103;
const RESOURCES: u32 = 0xC000_009A;
const POOL_PARAMS_BYTES: usize = 0x10;

#[repr(C)]
struct UnicodeString {
    length: u16,
    maximum_length: u16,
    buffer: *mut u16,
}

#[repr(C)]
struct ListEntry {
    flink: *mut ListEntry,
    blink: *mut ListEntry,
}

#[repr(C)]
struct KTimer {
    header: [u8; 16],
    due_time: i64,
    timer_list_entry: ListEntry,
    kdpc: *mut u8,
    period: i32,
}

#[repr(C)]
struct KDpc {
    typ: u8,
    importance: u8,
    number: u16,
    dpc_list_entry: ListEntry,
    deferred_routine: *const (),
    deferred_context: *mut u8,
    system_argument1: *mut u8,
    system_argument2: *mut u8,
    dpc_data: *mut u8,
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

unsafe extern "system" {
    fn DbgPrint(format: *const u8, ...) -> i32;
    fn NdisRegisterProtocolDriver(
        context: *mut u8,
        chars: *const u8,
        out_handle: *mut *mut u8,
    ) -> u32;
    fn NdisOpenAdapterEx(
        protocol: *mut u8,
        context: *mut u8,
        open_params: *mut u8,
        bind_context: *mut u8,
        out_handle: *mut *mut u8,
    ) -> u32;
    fn NdisSendNetBufferLists(
        binding: *mut u8,
        lists: *mut u8,
        port: u32,
        flags: u32,
    );
    fn NdisReturnNetBufferLists(
        binding: *mut u8,
        lists: *mut u8,
        flags: u32,
    );
    fn NdisAllocateNetBufferListPool(handle: *mut u8, params: *const u8) -> *mut u8;
    fn NdisAllocateNetBufferAndNetBufferList(
        pool: *mut u8,
        context_size: u16,
        context_backfill: u16,
        mdl: *mut u8,
        data_offset: u32,
        data_length: u64,
    ) -> *mut u8;
    fn NdisFreeNetBufferList(nbl: *mut u8, flags: u32);
    fn NdisAllocateMdl(handle: *mut u8, buffer: *mut u8, length: u32) -> *mut u8;
    fn VerosBindReadyProtocols();
    fn KeInitializeTimer(timer: *mut KTimer);
    fn KeInitializeDpc(dpc: *mut KDpc, routine: *const (), context: *mut u8);
    fn KeSetTimerEx(timer: *mut KTimer, due_time: i64, period: i32, dpc: *mut KDpc) -> u8;
    fn KeCancelTimer(timer: *mut KTimer) -> u8;
}

static mut PROTOCOL_HANDLE: *mut u8 = ptr::null_mut();
static mut NAME_BUF: [u16; 8] = [0; 8];
static mut NAME_STR: UnicodeString = UnicodeString {
    length: 0,
    maximum_length: 0,
    buffer: ptr::null_mut(),
};
static mut MEDIUM: u32 = NDIS_MEDIUM_802_3;
static mut SELECTED: u32 = 0;
static mut BINDING: *mut u8 = ptr::null_mut();
static mut NBL_POOL: *mut u8 = ptr::null_mut();
static mut TX_FRAME: [u8; 128] = [0; 128];
static mut TX_TIMER: KTimer = unsafe { core::mem::zeroed() };
static mut TX_DPC: KDpc = unsafe { core::mem::zeroed() };
static mut LOCAL_MAC: [u8; 6] = [0x52, 0x54, 0x00, 0x12, 0x34, 0x56];
static STARTED: AtomicU32 = AtomicU32::new(0);
static NEIGHBOR_SEEN: AtomicU32 = AtomicU32::new(0);

fn put_u16(buf: &mut [u8], at: usize, value: u16) {
    buf[at] = (value & 0xff) as u8;
    buf[at + 1] = (value >> 8) as u8;
}

fn put_ptr(buf: &mut [u8], at: usize, value: usize) {
    let bytes = value.to_le_bytes();
    buf[at..at + 8].copy_from_slice(&bytes);
}

fn put_be16(buf: &mut [u8], at: usize, value: u16) {
    buf[at] = (value >> 8) as u8;
    buf[at + 1] = (value & 0xff) as u8;
}

fn write_tlv(buf: &mut [u8], at: usize, typ: u8, info: &[u8]) -> usize {
    let len = info.len();
    let header = ((typ as u16) << 9) | (len as u16);
    put_be16(buf, at, header);
    buf[at + 2..at + 2 + len].copy_from_slice(info);
    at + 2 + len
}

unsafe fn fill_name() {
    let text: [u16; 4] = [
        b'L' as u16,
        b'L' as u16,
        b'D' as u16,
        b'P' as u16,
    ];
    NAME_BUF[..4].copy_from_slice(&text);
    NAME_STR.length = 8;
    NAME_STR.maximum_length = 16;
    NAME_STR.buffer = NAME_BUF.as_mut_ptr();
}

unsafe extern "system" fn set_options(_driver: *mut u8, _context: *mut u8) -> u32 {
    SUCCESS
}

unsafe extern "system" fn open_complete(
    _context: *mut u8,
    status: u32,
    _open_status: u32,
) {
    DbgPrint(b"LLDP OPENDONE %08x\n\0".as_ptr(), status);
}

unsafe extern "system" fn close_complete(_context: *mut u8) {}

unsafe extern "system" fn oid_complete(_context: *mut u8, _request: *mut u8, _status: u32) {}

unsafe extern "system" fn status_ex(_context: *mut u8, _status: *mut u8) {}

unsafe extern "system" fn send_complete(
    _context: *mut u8,
    lists: *mut u8,
    _count: u32,
) {
    let mut cur = lists;
    while !cur.is_null() {
        let next = ptr::read_unaligned(cur.add(NBL_NEXT_AT) as *const *mut u8);
        NdisFreeNetBufferList(cur, 0);
        cur = next;
    }
}

unsafe fn frame_ethertype(frame: *const u8, len: usize) -> Option<u16> {
    if frame.is_null() || len < ETH_HDR {
        return None;
    }
    Some(u16::from_be_bytes([*frame.add(ETH_TYPE), *frame.add(ETH_TYPE + 1)]))
}

unsafe fn copy_from_nbl(nbl: *mut u8, out: &mut [u8]) -> usize {
    if nbl.is_null() {
        return 0;
    }
    let mut len = ptr::read_unaligned(nbl.add(NBL_LENGTH_AT) as *const u32) as usize;
    let mut mdl = ptr::read_unaligned(nbl.add(NBL_MDL_AT) as *const *mut u8);
    let mut offset = ptr::read_unaligned(nbl.add(NBL_OFFSET_AT) as *const u32) as usize;
    let first_nb = ptr::read_unaligned(nbl.add(NBL_FIRST_NB_AT) as *const *mut u8);
    if !first_nb.is_null() {
        len = ptr::read_unaligned(first_nb.add(NB_LENGTH_AT) as *const u32) as usize;
        mdl = ptr::read_unaligned(first_nb.add(NB_MDL_AT) as *const *mut u8);
        offset = ptr::read_unaligned(first_nb.add(NB_OFFSET_AT) as *const u32) as usize;
    }
    if mdl.is_null() || len == 0 {
        return 0;
    }
    /* MDL: MappedSystemVa at +0x18, StartVa +0x20, ByteOffset +0x2c (mmtypes.h). */
    let mapped = ptr::read_unaligned(mdl.add(0x18) as *const *mut u8);
    let start = ptr::read_unaligned(mdl.add(0x20) as *const *mut u8);
    let byte_off = ptr::read_unaligned(mdl.add(0x2c) as *const u32) as usize;
    let base = if !mapped.is_null() {
        mapped
    } else if !start.is_null() {
        start.add(byte_off)
    } else {
        return 0;
    };
    let src = base.add(offset);
    let take = core::cmp::min(len, out.len());
    ptr::copy_nonoverlapping(src, out.as_mut_ptr(), take);
    take
}

unsafe fn parse_neighbor(frame: &[u8]) {
    if frame.len() < ETH_HDR + 6 {
        return;
    }
    let mut at = ETH_HDR;
    let mut chassis_ok = false;
    let mut port_ok = false;
    while at + 2 <= frame.len() {
        let header = u16::from_be_bytes([frame[at], frame[at + 1]]);
        let typ = (header >> 9) as u8;
        let len = (header & 0x1ff) as usize;
        at += 2;
        if typ == TLV_END || len == 0 {
            break;
        }
        if at + len > frame.len() {
            break;
        }
        match typ {
            TLV_CHASSIS => chassis_ok = true,
            TLV_PORT => port_ok = true,
            _ => {}
        }
        at += len;
    }
    if chassis_ok && port_ok {
        NEIGHBOR_SEEN.store(1, Ordering::SeqCst);
        DbgPrint(b"LLDP RX neighbor\n\0".as_ptr());
    } else {
        DbgPrint(b"LLDP RX\n\0".as_ptr());
    }
}

unsafe extern "system" fn receive_nbls(
    _context: *mut u8,
    lists: *mut u8,
    _port: u32,
    _count: u32,
    flags: u32,
) {
    let mut cur = lists;
    let mut scratch = [0u8; 256];
    while !cur.is_null() {
        let next = ptr::read_unaligned(cur.add(NBL_NEXT_AT) as *const *mut u8);
        let n = copy_from_nbl(cur, &mut scratch);
        if let Some(et) = frame_ethertype(scratch.as_ptr(), n) {
            if et == ETHERTYPE_LLDP {
                parse_neighbor(&scratch[..n]);
            }
        }
        cur = next;
    }
    /* Do not return the chain here. NDIS fans this indicate to LLDP
     * before tcpip; LanReceiveNbl6 owns NdisReturnNetBufferLists
     * (output/ndis-disk-20261116T214838Z.log: LLDP OPEN with tcpip
     * still returning RECVLAN). Returning from both starved PingCheck
     * in output/ndis-disk-20261116T212749Z.log (gateway done 2 0). */
    let _ = flags;
}

unsafe fn build_lldpdu(frame: &mut [u8], src_mac: &[u8; 6]) -> usize {
    let dest: [u8; 6] = [0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e];
    frame[ETH_DST..ETH_DST + 6].copy_from_slice(&dest);
    frame[ETH_SRC..ETH_SRC + 6].copy_from_slice(src_mac);
    put_be16(frame, ETH_TYPE, ETHERTYPE_LLDP);
    let mut at = ETH_HDR;
    let mut chassis = [0u8; 16];
    chassis[0] = CHASSIS_SUBTYPE_LOCAL;
    let name = b"VerOS";
    chassis[1..1 + name.len()].copy_from_slice(name);
    at = write_tlv(frame, at, TLV_CHASSIS, &chassis[..1 + name.len()]);
    let mut port = [0u8; 16];
    port[0] = PORT_SUBTYPE_LOCAL;
    let pname = b"eth0";
    port[1..1 + pname.len()].copy_from_slice(pname);
    at = write_tlv(frame, at, TLV_PORT, &port[..1 + pname.len()]);
    let mut ttl = [0u8; 2];
    put_be16(&mut ttl, 0, TTL_SECONDS);
    at = write_tlv(frame, at, TLV_TTL, &ttl);
    at = write_tlv(frame, at, TLV_END, &[]);
    at
}

unsafe fn send_one_lldpdu() {
    if BINDING.is_null() || NBL_POOL.is_null() {
        return;
    }
    let len = build_lldpdu(&mut TX_FRAME, &LOCAL_MAC);
    let mdl = NdisAllocateMdl(BINDING, TX_FRAME.as_mut_ptr(), len as u32);
    if mdl.is_null() {
        DbgPrint(b"LLDP TX mdl fail\n\0".as_ptr());
        return;
    }
    let nbl = NdisAllocateNetBufferAndNetBufferList(
        NBL_POOL,
        0,
        0,
        mdl,
        0,
        len as u64,
    );
    if nbl.is_null() {
        DbgPrint(b"LLDP TX nbl fail\n\0".as_ptr());
        return;
    }
    NdisSendNetBufferLists(BINDING, nbl, 0, 0);
    DbgPrint(b"LLDP TX\n\0".as_ptr());
}

unsafe extern "system" fn tx_timer_dpc(
    _dpc: *mut u8,
    _context: *mut u8,
    _arg1: *mut u8,
    _arg2: *mut u8,
) {
    send_one_lldpdu();
}

unsafe fn start_tx_timer() {
    KeInitializeDpc(
        ptr::addr_of_mut!(TX_DPC),
        tx_timer_dpc as *const (),
        ptr::null_mut(),
    );
    KeInitializeTimer(ptr::addr_of_mut!(TX_TIMER));
    /* Wait 90s before the first LLDPDU so PingCheck can finish on the
     * same boot (output/ndis-disk-20261116T212749Z.log: immediate LLDP TX
     * left gateway done 2 0; with Lldp Start 4, done 2 2 / 1 1). Period
     * 30s afterward. Due times are 100 ns units. */
    let due: i64 = -900_000_000;
    KeSetTimerEx(ptr::addr_of_mut!(TX_TIMER), due, 30_000, ptr::addr_of_mut!(TX_DPC));
}

unsafe extern "system" fn bind_adapter(
    _driver_context: *mut u8,
    bind_context: *mut u8,
    bind_params: *mut u8,
) -> u32 {
    /* Three-argument BindAdapterEx matches prot6.rs / NDIS
     * ProtocolBindAdapterEx (driver context, bind context, bind
     * parameters). A four-argument stub shifted bind_context onto the
     * parameters pointer so NdisOpenAdapterEx stored the wrong adapter
     * and indicate_secondary never matched (RXET 88cc with no LLDP RX
     * in output/ndis-disk-20261116T222756Z.log). */
    DbgPrint(b"LLDP BIND\n\0".as_ptr());
    if bind_params.is_null() {
        return RESOURCES;
    }
    let mut open = [0u8; OPEN_BYTES];
    open[0] = 0x87;
    open[1] = 1;
    put_u16(&mut open, 2, OPEN_BYTES as u16);
    /* AdapterName UNICODE_STRING* at bind+16 (prot6.rs). */
    let name_ptr = ptr::read_unaligned(bind_params.add(16) as *const *mut UnicodeString);
    put_ptr(&mut open, OPEN_NAME_AT, name_ptr as usize);
    put_ptr(&mut open, OPEN_MEDIUM_AT, ptr::addr_of_mut!(MEDIUM) as usize);
    put_u16(&mut open, OPEN_MEDIUM_COUNT_AT, 1);
    put_ptr(
        &mut open,
        OPEN_SELECTED_AT,
        ptr::addr_of_mut!(SELECTED) as usize,
    );
    let mut binding: *mut u8 = ptr::null_mut();
    let status = NdisOpenAdapterEx(
        PROTOCOL_HANDLE,
        ptr::null_mut(),
        open.as_mut_ptr(),
        bind_context,
        &mut binding,
    );
    DbgPrint(b"LLDP OPEN %08x\n\0".as_ptr(), status);
    if status == SUCCESS || status == PENDING {
        BINDING = binding;
        if NBL_POOL.is_null() {
            let mut params = [0u8; POOL_PARAMS_BYTES];
            params[0] = 0x80;
            params[1] = 1;
            put_u16(&mut params, 2, POOL_PARAMS_BYTES as u16);
            params[5] = 1;
            NBL_POOL = NdisAllocateNetBufferListPool(PROTOCOL_HANDLE, params.as_ptr());
        }
        if STARTED.swap(1, Ordering::SeqCst) == 0 {
            start_tx_timer();
        }
        return SUCCESS;
    }
    status
}

unsafe extern "system" fn unbind_adapter(
    _unbind_context: *mut u8,
    _protocol_binding: *mut u8,
) -> u32 {
    KeCancelTimer(ptr::addr_of_mut!(TX_TIMER));
    BINDING = ptr::null_mut();
    SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn DriverEntry(_driver: *mut u8, _registry: *mut u8) -> i32 {
    DbgPrint(b"LLDP LOAD\n\0".as_ptr());
    fill_name();
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
    let status = NdisRegisterProtocolDriver(ptr::null_mut(), chars.as_ptr(), &mut handle);
    DbgPrint(b"LLDP REG %08x\n\0".as_ptr(), status);
    if status != SUCCESS {
        return status as i32;
    }
    PROTOCOL_HANDLE = handle;
    VerosBindReadyProtocols();
    0
}
