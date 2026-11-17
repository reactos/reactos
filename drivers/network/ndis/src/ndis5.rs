/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/ndis5.rs
 * PURPOSE:     Legacy NDIS packet pools tcpip still allocates
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * drivers/network/tcpip/tcpip.sys still calls NdisAllocatePacketPoolEx from
 * main.c and NdisAllocatePacket from buffer.c. sdk/include/ddk/ndis.h
 * declares these NDIS 4 packet APIs. The NDIS 5 tcpip bind bridge
 * VerosNoteTransmit / VerosIndicateFrame and tcpip's NDIS 5 bind are gone;
 * tcpip uses LanRegisterProtocol6 only. Phase C acceptance:
 * output/ndis-disk-20261116T203348Z.log and output/netkvm-20261116T203635Z.log
 * (gateway done 2 2, DNS done 1 1). NdisRegisterProtocol still succeeds so
 * in-tree ndisuio.sys can load for raw NDIS smoke (Phase H); BINDALL for
 * that legacy path is not restored. sdk/include/xdk/mmtypes.h is the MDL.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndis_buffer_to_span_pages
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterprotocol
 * NDIS 5 miniports enter through NdisMRegisterMiniport. Characteristics
 * slots and struct _NDIS_MINIPORT_BLOCK field order are the x64 layout
 * in sdk/include/ddk/ndis.h. KTIMER and KDPC are 0x40 bytes in
 * sdk/include/xdk/ketypes.h, matching the interrupt block in src/lib.rs.
 * NdisMedium802_3 is the first enumerator in sdk/include/psdk/ntddndis.h.
 * Indicated NBLs use the DataLength/Mdl offsets lan.c already reads.
 * NDIS 6 drivers do not call this path.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiport
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetattributesex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterinterrupt
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsynchronizewithinterrupt
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismindicatestatus
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisminitializetimer
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissettimer
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismcanceltimer
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenconfiguration
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismqueryadapterresources
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreadpcislotinformation
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiswritepcislotinformation
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocatememorywithtag
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisminitializescattergatherdma
 * Dma64Addresses 0 asks for a 32-bit device. output/ndis-disk-20261117T150052Z.log
 * printed NDIS5 SGDMA 00000000 then NDIS SHMEMOK with physical high
 * dword 00000001, and the pcap beside that log carried the miniport's
 * transmits as zero frames of the right length. The scatter/gather list
 * is SCATTER_GATHER_LIST in sdk/include/xdk/iotypes.h, reached through
 * NDIS_PER_PACKET_INFO_FROM_PACKET in sdk/include/ddk/ndis.h.
 * Attaching that list and a private bounce bugchecked 0x19
 * (BAD_POOL_HEADER) in output/ndis-disk-20261117T152123Z.log after one
 * NDIS5 SEND. Shared-memory DMA stays 32-bit; the packet bytes stay in
 * a pool buffer the miniport already copied on that boot.
 * SendComplete from the interrupt DPC asserted APC_LEVEL and later
 * bugchecked 0x19 on tag 5sdN when Sent lived at WrapperReserved 0x40
 * (output/ndis-disk-20261117T152636Z.log,
 * output/ndis-disk-20261117T154605Z.log). MiniportReservedEx overwrote
 * that slot. Sent now trails the packet in the same pool block.
 * Leak probe: finish_send completes the NBL but does not free the
 * packet, copy, or MDL (output/ndis-disk-20261117T154925Z.log reached
 * MDLFREE then 0x19 on tag 5sdN). Copy-only and copy+packet frees
 * passed; IoFreeMdl alone passed. Freeing all three with
 * NdisPacketOobOffset left at 0 still risked OOB/extension writes into
 * the descriptor. build_packet now sets NdisPacketOobOffset to 96
 * (sdk/include/ddk/ndis.h NDIS_PACKET_PRIVATE) and fPACKET_ALLOCATED_BY_NDIS.
 * output/ndis-disk-20261117T160927Z.log: NDIS5 FREEA, gateway done 2 2,
 * loopback done 1 1, no Fatal System Error.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismfreemapregisters
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiswriteerrorlogentry
 */

use core::mem::size_of;
use core::ptr;

const PAGE_SHIFT: usize = 12;
const PAGE_SIZE: usize = 1 << PAGE_SHIFT;
const SUCCESS: u32 = 0;
const NOT_SUPPORTED: u32 = 0xC000_00BB;
const RESOURCES: u32 = 0xC000_009A;
const POOL_TAG: u32 = u32::from_le_bytes(*b"5sdN");
const COPY_TAG: u32 = u32::from_le_bytes(*b"5sdC");
const PACKET_TAG: u32 = u32::from_le_bytes(*b"5sdP");
const POOL_KIND: u32 = u32::from_le_bytes(*b"PKso");
/* Private (44) + reserved union (32) + Reserved[2] (16) = 92. */
const PACKET_OOB_AT: u16 = 96;

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

struct Pool {
    kind: u32,
    reserved: u32,
}

unsafe extern "system" {
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
    /* Opaque protocol block so ndisuio DriverEntry can finish. The
     * characteristics buffer is not walked for BINDALL. */
    if chars.is_null() || length == 0 {
        write_status(status, NOT_SUPPORTED);
        if !handle.is_null() {
            *handle = ptr::null_mut();
        }
        return;
    }
    let block = ExAllocatePoolWithTag(0, size_of::<Pool>(), POOL_TAG) as *mut Pool;
    if block.is_null() {
        write_status(status, RESOURCES);
        if !handle.is_null() {
            *handle = ptr::null_mut();
        }
        return;
    }
    (*block).kind = u32::from_le_bytes(*b"PRot");
    (*block).reserved = length;
    if !handle.is_null() {
        *handle = block as *mut u8;
    }
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisDeregisterProtocol(status: *mut u32, _handle: *mut u8) {
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisOpenAdapter(
    status: *mut u32,
    open_status: *mut u32,
    _handle: *mut *mut u8,
    _medium: *mut u32,
    _media: *mut u32,
    _count: u32,
    _protocol: *mut u8,
    _context: *mut u8,
    _name: *mut u8,
    _options: u32,
    _addressing: *mut u8,
) {
    write_status(status, NOT_SUPPORTED);
    write_status(open_status, NOT_SUPPORTED);
}

#[no_mangle]
pub unsafe extern "system" fn NdisCloseAdapter(status: *mut u32, _handle: *mut u8) {
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisRequest(status: *mut u32, _binding: *mut u8, _request: *mut u8) {
    write_status(status, NOT_SUPPORTED);
}

#[no_mangle]
pub unsafe extern "system" fn NdisSend(status: *mut u32, _binding: *mut u8, _packet: *mut u8) {
    write_status(status, NOT_SUPPORTED);
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

const LEGACY_KIND: u32 = u32::from_le_bytes(*b"L5so");
const INVALID_PARAMETER: u32 = 0xC000_000D;
const BAD_VERSION: u32 = 0xC001_0004;
const PENDING: u32 = 0x0000_0103;
const BUFFER_TOO_SHORT: u32 = 0xC001_0016;
const BLOCK_BYTES: usize = 0x400;
const CHARS_KEEP: usize = 0x100;
const NBL_BYTES: usize = 512;
const FRAME_LIMIT: usize = 2048;
/* NDIS51_MINIPORT_CHARACTERISTICS in sdk/include/ddk/ndis.h, x64. */
const CH_DISABLE: usize = 16;
const CH_ENABLE: usize = 24;
const CH_HANDLE_IRQ: usize = 40;
const CH_INIT: usize = 48;
const CH_ISR: usize = 56;
const CH_QUERY: usize = 64;
const CH_SEND: usize = 88;
const CH_SET: usize = 96;
const CH_TRANSFER: usize = 104;
const CH_SEND_PACKETS: usize = 120;
/* struct _NDIS_MINIPORT_BLOCK in sdk/include/ddk/ndis.h, x64.
 * KTIMER + KDPC are 0x40 in sdk/include/xdk/ketypes.h. */
const BL_ETH: usize = 0x190;
const BL_PACKET: usize = 0x1b0;
const BL_SEND_DONE: usize = 0x1b8;
const BL_SEND_RES: usize = 0x1c0;
const BL_RESET_DONE: usize = 0x1c8;
const BL_ETH_RX: usize = 0x280;
const BL_ETH_DONE: usize = 0x298;
const BL_STATUS: usize = 0x2b0;
const BL_STATUS_DONE: usize = 0x2b8;
const BL_TD_DONE: usize = 0x2c0;
const BL_QUERY_DONE: usize = 0x2c8;
const BL_SET_DONE: usize = 0x2d0;
/* NDIS_MINIPORT_INTERRUPT in sdk/include/ddk/ndis.h. */
const MI_OBJECT: usize = 0;
const MI_RESERVED: usize = 16;
const MI_ISR: usize = 24;
const MI_DPC: usize = 32;
/* NDIS_MINIPORT_TIMER: KTIMER then KDPC, then the function and context. */
const TIMER_DPC: usize = 0x40;
const TIMER_FN: usize = 0x80;
const TIMER_CTX: usize = 0x88;
/* NDIS_PACKET wrapper reserved, after NDIS_PACKET_PRIVATE. */
/* Packet descriptor the miniport may write through MiniportReservedEx.
 * Sent trails the descriptor in the same pool block so the miniport
 * cannot overwrite it. output/ndis-disk-20261117T152636Z.log and
 * output/ndis-disk-20261117T154756Z.log completed with a bad NBL when
 * Sent lived inside the descriptor. sdk/include/ddk/ndis.h NDIS_PACKET. */
const PACKET_BYTES: usize = 0x100;
const DMA32_LIMIT: u64 = 0xFFFF_FFFF;
/* lan.c NBL_DATA_* / FirstNetBuffer. Bookkeeping at 0x10 and 0x18 is
 * this file's frame and MDL, not a Windows field. */
const NBL_NEXT: usize = 0;
const NBL_FRAME: usize = 0x10;
const NBL_OWN_MDL: usize = 0x18;
const NBL_FIRST_NB: usize = 8;
const NB_LENGTH: usize = 0x18;
const NB_MDL: usize = 0x20;
const NB_SKIP: usize = 0x28;
const NBL_LENGTH: usize = 0x198;
const NBL_MDL: usize = 0x1A0;
const NBL_SKIP: usize = 0x1A8;
const OID_TYPE_AT: usize = 4;
const OID_OID_AT: usize = 0x20;
const OID_BUFFER_AT: usize = 0x28;
const OID_LENGTH_AT: usize = 0x30;
const IRQ_CHARS: usize = 0x60;
const IRQ_ISR_AT: usize = 0x08;
const IRQ_DPC_AT: usize = 0x10;
const IRQ_DISABLE_AT: usize = 0x18;
const IRQ_ENABLE_AT: usize = 0x20;
const CM_COUNT_AT: usize = 4;
const CM_DESC_AT: usize = 8;
const CM_DESC_BYTES: usize = 0x14;
const DEVICE_PROPERTY_BUS_NUMBER: u32 = 0x0e;
const DEVICE_PROPERTY_ADDRESS: u32 = 0x10;
const PCI_CONFIGURATION: u32 = 4;

struct Legacy {
    kind: u32,
    mp: *mut u8,
    block: *mut u8,
    driver_ctx: *mut u8,
    chars: *mut u8,
    chars_len: u32,
    mp_interrupt: *mut u8,
    pending_oid: *mut u8,
    shutdown: *const (),
    shutdown_ctx: *mut u8,
    dma32: u8,
}

const SENT_KIND: u32 = u32::from_le_bytes(*b"S5so");

struct Sent {
    kind: u32,
    nbl: *mut u8,
    copy: *mut u8,
    mdl: *mut u8,
    mp: *mut u8,
}

static mut ACTIVE: *mut Legacy = ptr::null_mut();

unsafe extern "system" {
    fn DbgPrint(format: *const u8, ...) -> i32;
    fn KeInitializeDpc(dpc: *mut u8, routine: *const (), context: *mut u8);
    fn KeInitializeTimer(timer: *mut u8);
    fn KeSetTimerEx(timer: *mut u8, due: i64, period: i32, dpc: *mut u8) -> u8;
    fn KeCancelTimer(timer: *mut u8) -> u8;
    fn IoFreeMdl(mdl: *mut u8);
    fn KeSynchronizeExecution(
        interrupt_object: *mut u8,
        synchronize_routine: *const (),
        synchronize_context: *mut u8,
    ) -> u8;
    fn HalSetBusDataByOffset(
        bus_type: u32,
        bus: u32,
        slot: u32,
        buffer: *mut u8,
        offset: u32,
        length: u32,
    ) -> u32;
    fn IoGetDeviceProperty(
        device: *mut u8,
        property: u32,
        length: u32,
        buffer: *mut u8,
        result: *mut u32,
    ) -> i32;
}

fn active() -> *mut Legacy {
    unsafe { ACTIVE }
}

fn state_for(handle: *mut u8) -> *mut Legacy {
    let state = active();
    if state.is_null() || handle.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        if (*state).block == handle || (*state).mp == handle {
            state
        } else {
            ptr::null_mut()
        }
    }
}

fn write_fn(block: *mut u8, at: usize, function: *const ()) {
    unsafe { ptr::write_unaligned(block.add(at) as *mut *const (), function) };
}

fn read_fn(chars: *const u8, len: u32, at: usize) -> *const () {
    if chars.is_null() || (at + size_of::<*const ()>()) > len as usize {
        return ptr::null();
    }
    unsafe { ptr::read_unaligned(chars.add(at) as *const *const ()) }
}

fn read_u32(base: *const u8, at: usize) -> u32 {
    unsafe {
        u32::from_le_bytes([
            *base.add(at),
            *base.add(at + 1),
            *base.add(at + 2),
            *base.add(at + 3),
        ])
    }
}

fn slot_fn(state: *const Legacy, at: usize) -> *const () {
    unsafe { read_fn((*state).chars, (*state).chars_len, at) }
}

unsafe extern "system" fn legacy_isr(ctx: *mut u8, queue: *mut u8, targets: *mut u32) -> u8 {
    let _ = ctx;
    let state = active();
    if state.is_null() {
        return 0;
    }
    let isr = slot_fn(state, CH_ISR);
    if isr.is_null() {
        return 0;
    }
    let mut recognized: u8 = 0;
    let mut queue_mp: u8 = 0;
    let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8) =
        core::mem::transmute(isr);
    let driver_ctx = unsafe { (*state).driver_ctx };
    function(&mut recognized, &mut queue_mp, driver_ctx);
    if !queue.is_null() {
        unsafe { *queue = queue_mp };
    }
    if !targets.is_null() {
        unsafe { *targets = 0 };
    }
    recognized
}

unsafe extern "system" fn legacy_dpc(ctx: *mut u8, _a: *mut u8, _b: *mut u8, _c: *mut u8) {
    let _ = ctx;
    let state = active();
    if state.is_null() {
        return;
    }
    let handler = slot_fn(state, CH_HANDLE_IRQ);
    if handler.is_null() {
        return;
    }
    let function: unsafe extern "system" fn(*mut u8) = core::mem::transmute(handler);
    function(unsafe { (*state).driver_ctx });
}

unsafe extern "system" fn legacy_enable(ctx: *mut u8) {
    let _ = ctx;
    let state = active();
    if state.is_null() {
        return;
    }
    let handler = slot_fn(state, CH_ENABLE);
    if handler.is_null() {
        return;
    }
    let function: unsafe extern "system" fn(*mut u8) = core::mem::transmute(handler);
    function(unsafe { (*state).driver_ctx });
}

unsafe extern "system" fn legacy_disable(ctx: *mut u8) {
    let _ = ctx;
    let state = active();
    if state.is_null() {
        return;
    }
    let handler = slot_fn(state, CH_DISABLE);
    if handler.is_null() {
        return;
    }
    let function: unsafe extern "system" fn(*mut u8) = core::mem::transmute(handler);
    function(unsafe { (*state).driver_ctx });
}

unsafe extern "system" fn timer_dpc(
    _dpc: *mut u8,
    context: *mut u8,
    arg1: *mut u8,
    arg2: *mut u8,
) {
    if context.is_null() {
        return;
    }
    let function = unsafe { ptr::read_unaligned(context.add(TIMER_FN) as *const *const ()) };
    let timer_ctx = unsafe { ptr::read_unaligned(context.add(TIMER_CTX) as *const *mut u8) };
    if function.is_null() {
        return;
    }
    let call: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8) =
        core::mem::transmute(function);
    call(arg1, timer_ctx, arg2, ptr::null_mut());
}

unsafe extern "system" fn eth_indicate(
    filter: *mut u8,
    _context: *mut u8,
    _address: *mut u8,
    header: *mut u8,
    header_size: u32,
    lookahead: *mut u8,
    lookahead_size: u32,
    packet_size: u32,
) {
    let _ = filter;
    indicate_frame(header, header_size, lookahead, lookahead_size, packet_size);
}

unsafe extern "system" fn packet_indicate(miniport: *mut u8, packets: *mut *mut u8, count: u32) {
    let _ = miniport;
    if packets.is_null() || count == 0 {
        return;
    }
    let mut index = 0u32;
    while index < count {
        let packet = unsafe { *packets.add(index as usize) };
        indicate_packet(packet);
        index += 1;
    }
}

unsafe extern "system" fn send_complete(miniport: *mut u8, packet: *mut u8, status: u32) {
    let _ = (miniport, status);
    finish_send(packet);
}

unsafe extern "system" fn send_resources(_miniport: *mut u8) {}

unsafe extern "system" fn reset_complete(_miniport: *mut u8, _status: u32, _reset: u8) {}

unsafe extern "system" fn eth_complete(_filter: *mut u8) {}

unsafe extern "system" fn td_complete(_miniport: *mut u8, _packet: *mut u8, _status: u32, _bytes: u32) {}

unsafe extern "system" fn query_complete(miniport: *mut u8, status: u32) {
    finish_oid(miniport, status);
}

unsafe extern "system" fn set_complete(miniport: *mut u8, status: u32) {
    finish_oid(miniport, status);
}

fn finish_oid(miniport: *mut u8, status: u32) {
    let state = state_for(miniport);
    if state.is_null() {
        return;
    }
    unsafe {
        let request = (*state).pending_oid;
        (*state).pending_oid = ptr::null_mut();
        if !request.is_null() {
            crate::prot6::complete_oid((*state).mp, request, status);
        }
    }
}

fn mdl_bytes(mdl: *const Mdl) -> (*mut u8, u32) {
    if mdl.is_null() {
        return (ptr::null_mut(), 0);
    }
    unsafe {
        let va = (*mdl).start.add((*mdl).byte_offset as usize);
        (va, (*mdl).byte_count)
    }
}

fn copy_from_mdl(mdl: *const Mdl, skip: u32, dest: *mut u8, need: u32) -> u32 {
    let mut left_skip = skip;
    let mut filled = 0u32;
    let mut current = mdl;
    while !current.is_null() && filled < need {
        let (va, count) = mdl_bytes(current);
        unsafe { current = (*current).next };
        if count == 0 || va.is_null() {
            continue;
        }
        if left_skip >= count {
            left_skip -= count;
            continue;
        }
        let take = (count - left_skip).min(need - filled) as usize;
        unsafe {
            ptr::copy_nonoverlapping(va.add(left_skip as usize), dest.add(filled as usize), take);
        }
        filled += take as u32;
        left_skip = 0;
    }
    filled
}

fn nbl_view(nbl: *mut u8) -> (*mut Mdl, u32, u32) {
    if nbl.is_null() {
        return (ptr::null_mut(), 0, 0);
    }
    unsafe {
        let nb = ptr::read_unaligned(nbl.add(NBL_FIRST_NB) as *const *mut u8);
        let (length, mdl, skip) = if !nb.is_null() {
            (
                ptr::read_unaligned(nb.add(NB_LENGTH) as *const u64) as u32,
                ptr::read_unaligned(nb.add(NB_MDL) as *const *mut Mdl),
                ptr::read_unaligned(nb.add(NB_SKIP) as *const u32),
            )
        } else {
            (
                ptr::read_unaligned(nbl.add(NBL_LENGTH) as *const u64) as u32,
                ptr::read_unaligned(nbl.add(NBL_MDL) as *const *mut Mdl),
                ptr::read_unaligned(nbl.add(NBL_SKIP) as *const u32),
            )
        };
        (mdl, skip, length)
    }
}

fn sent_of(packet: *mut u8) -> *mut Sent {
    if packet.is_null() {
        return ptr::null_mut();
    }
    unsafe { packet.add(PACKET_BYTES) as *mut Sent }
}

fn finish_send(packet: *mut u8) {
    if packet.is_null() {
        return;
    }
    let sent = sent_of(packet);
    unsafe {
        if (*sent).kind != SENT_KIND {
            DbgPrint(b"NDIS5 SENTBAD %p\n\0".as_ptr(), packet);
            return;
        }
        let nbl = (*sent).nbl;
        let mp = (*sent).mp;
        let mdl = (*sent).mdl;
        let copy = (*sent).copy;
        (*sent).kind = 0;
        (*sent).nbl = ptr::null_mut();
        (*sent).mp = ptr::null_mut();
        (*sent).mdl = ptr::null_mut();
        (*sent).copy = ptr::null_mut();
        ptr::write_unaligned(packet.add(8) as *mut *mut u8, ptr::null_mut());
        ptr::write_unaligned(packet.add(16) as *mut *mut u8, ptr::null_mut());
        DbgPrint(
            b"NDIS5 FREE %p nbl %p mdl %p copy %p\n\0".as_ptr(),
            packet,
            nbl,
            mdl,
            copy,
        );
        if !nbl.is_null() && !mp.is_null() {
            crate::prot6::complete_send(mp, nbl, 0);
        }
        /* Individually IoFreeMdl, copy, and packet each passed. Free all
         * with OobOffset set and Head unlinked. Tags 5sdC/5sdP name the
         * 0x19 block if the combination still fails. */
        if !mdl.is_null() {
            IoFreeMdl(mdl);
        }
        if !copy.is_null() {
            ExFreePoolWithTag(copy, COPY_TAG);
        }
        ExFreePoolWithTag(packet, PACKET_TAG);
        DbgPrint(b"NDIS5 FREEA\n\0".as_ptr());
    }
}

fn indicate_bytes(frame: *mut u8, length: u32) {
    let state = active();
    if state.is_null() || frame.is_null() || length == 0 {
        if !frame.is_null() {
            unsafe { ExFreePoolWithTag(frame, POOL_TAG) };
        }
        return;
    }
    let mdl = unsafe { IoAllocateMdl(frame, length, 0, 0, ptr::null_mut()) };
    if mdl.is_null() {
        unsafe { ExFreePoolWithTag(frame, POOL_TAG) };
        return;
    }
    unsafe { MmBuildMdlForNonPagedPool(mdl) };
    let nbl = unsafe { ExAllocatePoolWithTag(0, NBL_BYTES, POOL_TAG) };
    if nbl.is_null() {
        unsafe {
            IoFreeMdl(mdl);
            ExFreePoolWithTag(frame, POOL_TAG);
        }
        return;
    }
    unsafe {
        ptr::write_bytes(nbl, 0, NBL_BYTES);
        ptr::write_unaligned(nbl.add(NBL_LENGTH) as *mut u64, length as u64);
        ptr::write_unaligned(nbl.add(NBL_MDL) as *mut *mut u8, mdl);
        ptr::write_unaligned(nbl.add(NBL_SKIP) as *mut u32, 0);
        ptr::write_unaligned(nbl.add(NBL_FRAME) as *mut *mut u8, frame);
        ptr::write_unaligned(nbl.add(NBL_OWN_MDL) as *mut *mut u8, mdl);
        DbgPrint(b"NDIS5 RECV %u\n\0".as_ptr(), length);
        crate::prot6::indicate_receive((*state).mp, nbl, 0, 1, 0);
    }
}

fn indicate_frame(
    header: *mut u8,
    header_size: u32,
    lookahead: *mut u8,
    lookahead_size: u32,
    packet_size: u32,
) {
    let total = header_size as usize + packet_size as usize;
    if total == 0 || total > FRAME_LIMIT || header.is_null() || header_size == 0 {
        return;
    }
    let frame = unsafe { ExAllocatePoolWithTag(0, total, POOL_TAG) };
    if frame.is_null() {
        return;
    }
    unsafe { ptr::copy_nonoverlapping(header, frame, header_size as usize) };
    let data_have = lookahead_size.min(packet_size);
    if data_have != 0 && !lookahead.is_null() {
        unsafe {
            ptr::copy_nonoverlapping(
                lookahead,
                frame.add(header_size as usize),
                data_have as usize,
            );
        }
    }
    if data_have < packet_size {
        let state = active();
        let transfer = if state.is_null() {
            ptr::null()
        } else {
            slot_fn(state, CH_TRANSFER)
        };
        if transfer.is_null() {
            unsafe { ExFreePoolWithTag(frame, POOL_TAG) };
            return;
        }
        let rest = packet_size - data_have;
        let function: unsafe extern "system" fn(
            *mut u8,
            *mut u32,
            *mut u8,
            *mut u8,
            u32,
            u32,
        ) -> u32 = unsafe { core::mem::transmute(transfer) };
        let mut moved = 0u32;
        let status = unsafe {
            function(
                ptr::null_mut(),
                &mut moved,
                (*state).driver_ctx,
                ptr::null_mut(),
                data_have,
                rest,
            )
        };
        if status != SUCCESS || moved == 0 {
            unsafe { ExFreePoolWithTag(frame, POOL_TAG) };
            return;
        }
    }
    indicate_bytes(frame, total as u32);
}

fn indicate_packet(packet: *mut u8) {
    if packet.is_null() {
        return;
    }
    let total = unsafe { ptr::read_unaligned(packet.add(4) as *const u32) } as usize;
    let head = unsafe { ptr::read_unaligned(packet.add(8) as *const *const Mdl) };
    if total == 0 || total > FRAME_LIMIT || head.is_null() {
        return;
    }
    let frame = unsafe { ExAllocatePoolWithTag(0, total, POOL_TAG) };
    if frame.is_null() {
        return;
    }
    let got = copy_from_mdl(head, 0, frame, total as u32);
    if got == 0 {
        unsafe { ExFreePoolWithTag(frame, POOL_TAG) };
        return;
    }
    indicate_bytes(frame, got);
}

fn build_packet(mp: *mut u8, nbl: *mut u8) -> *mut u8 {
    let (mdl, skip, length) = nbl_view(nbl);
    if mdl.is_null() || length == 0 || length as usize > FRAME_LIMIT {
        return ptr::null_mut();
    }
    let copy = unsafe { ExAllocatePoolWithTag(0, length as usize, COPY_TAG) };
    let packet =
        unsafe { ExAllocatePoolWithTag(0, PACKET_BYTES + size_of::<Sent>(), PACKET_TAG) };
    if copy.is_null() || packet.is_null() {
        unsafe {
            if !copy.is_null() {
                ExFreePoolWithTag(copy, COPY_TAG);
            }
            if !packet.is_null() {
                ExFreePoolWithTag(packet, PACKET_TAG);
            }
        }
        return ptr::null_mut();
    }
    let got = copy_from_mdl(mdl, skip, copy, length);
    if got == 0 {
        unsafe {
            ExFreePoolWithTag(copy, COPY_TAG);
            ExFreePoolWithTag(packet, PACKET_TAG);
        }
        return ptr::null_mut();
    }
    let owned = unsafe { IoAllocateMdl(copy, got, 0, 0, ptr::null_mut()) };
    if owned.is_null() {
        unsafe {
            ExFreePoolWithTag(copy, COPY_TAG);
            ExFreePoolWithTag(packet, PACKET_TAG);
        }
        return ptr::null_mut();
    }
    let sent = sent_of(packet);
    unsafe {
        MmBuildMdlForNonPagedPool(owned);
        ptr::write_bytes(packet, 0, PACKET_BYTES + size_of::<Sent>());
        ptr::write_unaligned(packet as *mut u32, span_pages(owned as *const Mdl));
        ptr::write_unaligned(packet.add(4) as *mut u32, got);
        ptr::write_unaligned(packet.add(8) as *mut *mut u8, owned);
        ptr::write_unaligned(packet.add(16) as *mut *mut u8, owned);
        ptr::write_unaligned(packet.add(32) as *mut u32, 1);
        *packet.add(40) = 1;
        /* NdisPacketOobOffset — sdk/include/ddk/ndis.h NDIS_PACKET_PRIVATE. */
        ptr::write_unaligned(packet.add(42) as *mut u16, PACKET_OOB_AT);
        *packet.add(41) = 0x80;
        (*sent).kind = SENT_KIND;
        (*sent).nbl = nbl;
        (*sent).copy = copy;
        (*sent).mdl = owned;
        (*sent).mp = mp;
    }
    packet
}

pub fn legacy_dma_limit(handle: *mut u8) -> u64 {
    let state = state_for(handle);
    if state.is_null() || unsafe { (*state).dma32 } == 0 {
        u64::MAX
    } else {
        DMA32_LIMIT
    }
}

pub unsafe fn start_legacy(mp: *mut u8) {
    let state = crate::register::legacy_state(mp) as *mut Legacy;
    if state.is_null() || unsafe { (*state).kind } != LEGACY_KIND {
        return;
    }
    unsafe {
        (*state).mp = mp;
        ACTIVE = state;
        DbgPrint(b"NDIS5 START\n\0".as_ptr());
    }
    let block = unsafe { ExAllocatePoolWithTag(0, BLOCK_BYTES, POOL_TAG) };
    if block.is_null() {
        return;
    }
    unsafe {
        ptr::write_bytes(block, 0, BLOCK_BYTES);
        (*state).block = block;
        write_fn(block, BL_ETH, block as *const ());
        write_fn(block, BL_PACKET, packet_indicate as *const ());
        write_fn(block, BL_SEND_DONE, send_complete as *const ());
        write_fn(block, BL_SEND_RES, send_resources as *const ());
        write_fn(block, BL_RESET_DONE, reset_complete as *const ());
        write_fn(block, BL_ETH_RX, eth_indicate as *const ());
        write_fn(block, BL_ETH_DONE, eth_complete as *const ());
        write_fn(block, BL_STATUS, NdisMIndicateStatus as *const ());
        write_fn(block, BL_STATUS_DONE, NdisMIndicateStatusComplete as *const ());
        write_fn(block, BL_TD_DONE, td_complete as *const ());
        write_fn(block, BL_QUERY_DONE, query_complete as *const ());
        write_fn(block, BL_SET_DONE, set_complete as *const ());
    }
    let init = slot_fn(state, CH_INIT);
    if init.is_null() {
        unsafe { DbgPrint(b"NDIS5 INIT null\n\0".as_ptr()) };
        return;
    }
    let mut open_error = 0u32;
    let mut selected = 0u32;
    let mut medium = 0u32;
    let function: unsafe extern "system" fn(*mut u32, *mut u32, *mut u32, u32, *mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(init) };
    let status = unsafe { function(&mut open_error, &mut selected, &mut medium, 1, block, mp) };
    unsafe { DbgPrint(b"NDIS5 INIT %08x\n\0".as_ptr(), status) };
    if status != SUCCESS {
        return;
    }
    unsafe {
        crate::connect_line_only(mp);
        if !(*state).mp_interrupt.is_null() {
            let object = crate::line_interrupt_object(mp);
            ptr::write_unaligned((*state).mp_interrupt.add(MI_OBJECT) as *mut *mut u8, object);
        }
        crate::protocol::notify_adapter_ready(mp);
    }
}

pub unsafe fn send_legacy(mp: *mut u8, lists: *mut u8) {
    let state = crate::register::legacy_state(mp) as *mut Legacy;
    if state.is_null() {
        return;
    }
    let mut nbl = lists;
    while !nbl.is_null() {
        let next = unsafe { ptr::read_unaligned(nbl.add(NBL_NEXT) as *const *mut u8) };
        let packet = build_packet(mp, nbl);
        if packet.is_null() {
            crate::prot6::complete_send(mp, nbl, 0);
            nbl = next;
            continue;
        }
        let packets_fn = slot_fn(state, CH_SEND_PACKETS);
        if !packets_fn.is_null() {
            let call: unsafe extern "system" fn(*mut u8, *mut *mut u8, u32) =
                unsafe { core::mem::transmute(packets_fn) };
            let mut packet_slot = packet;
            unsafe {
                DbgPrint(b"NDIS5 SEND\n\0".as_ptr());
                call((*state).driver_ctx, &mut packet_slot, 1);
            }
        } else {
            let send_fn = slot_fn(state, CH_SEND);
            if send_fn.is_null() {
                finish_send(packet);
            } else {
                let call: unsafe extern "system" fn(*mut u8, *mut u8, u32) -> u32 =
                    unsafe { core::mem::transmute(send_fn) };
                let status = unsafe { call((*state).driver_ctx, packet, 0) };
                unsafe { DbgPrint(b"NDIS5 SEND %08x\n\0".as_ptr(), status) };
                if status != PENDING {
                    finish_send(packet);
                }
            }
        }
        nbl = next;
    }
}

pub unsafe fn oid_legacy(mp: *mut u8, request: *mut u8) -> u32 {
    let state = crate::register::legacy_state(mp) as *mut Legacy;
    if state.is_null() || unsafe { (*state).driver_ctx.is_null() } {
        return INVALID_PARAMETER;
    }
    let kind = read_u32(request, OID_TYPE_AT);
    let oid = read_u32(request, OID_OID_AT);
    let buffer = unsafe { ptr::read_unaligned(request.add(OID_BUFFER_AT) as *const *mut u8) };
    let length = read_u32(request, OID_LENGTH_AT);
    let slot = if kind == 1 { CH_SET } else { CH_QUERY };
    let handler = slot_fn(state, slot);
    unsafe { DbgPrint(b"NDIS5 OID %08x %08x\n\0".as_ptr(), kind, oid) };
    if handler.is_null() {
        return NOT_SUPPORTED;
    }
    let mut done = 0u32;
    let mut needed = 0u32;
    let status = if kind == 1 {
        let call: unsafe extern "system" fn(*mut u8, u32, *mut u8, u32, *mut u32, *mut u32) -> u32 =
            unsafe { core::mem::transmute(handler) };
        unsafe { call((*state).driver_ctx, oid, buffer, length, &mut done, &mut needed) }
    } else {
        let call: unsafe extern "system" fn(*mut u8, u32, *mut u8, u32, *mut u32, *mut u32) -> u32 =
            unsafe { core::mem::transmute(handler) };
        unsafe { call((*state).driver_ctx, oid, buffer, length, &mut done, &mut needed) }
    };
    unsafe { DbgPrint(b"NDIS5 OIDDONE %08x\n\0".as_ptr(), status) };
    if status == PENDING {
        unsafe { (*state).pending_oid = request };
    }
    status
}

pub unsafe fn return_legacy(lists: *mut u8) {
    let mut nbl = lists;
    while !nbl.is_null() {
        let next = unsafe { ptr::read_unaligned(nbl.add(NBL_NEXT) as *const *mut u8) };
        unsafe {
            let frame = ptr::read_unaligned(nbl.add(NBL_FRAME) as *const *mut u8);
            let mdl = ptr::read_unaligned(nbl.add(NBL_OWN_MDL) as *const *mut u8);
            if !mdl.is_null() {
                IoFreeMdl(mdl);
            }
            if !frame.is_null() {
                ExFreePoolWithTag(frame, POOL_TAG);
            }
            ExFreePoolWithTag(nbl, POOL_TAG);
        }
        nbl = next;
    }
}

fn pci_slot(address: u32) -> u32 {
    /* drivers/network/ndis/src/lib.rs pci_slot_from_address. */
    let device = address >> 16;
    let function = address & 0xffff;
    device | (function << 5)
}

fn pci_location(mp: *mut u8) -> Option<(u32, u32)> {
    let (pdo, _, _) = crate::register::miniport_devices(mp);
    if pdo.is_null() {
        return None;
    }
    let mut bus = 0u32;
    let mut bus_len = 0u32;
    let mut address = 0u32;
    let mut address_len = 0u32;
    let bus_status = unsafe {
        IoGetDeviceProperty(
            pdo,
            DEVICE_PROPERTY_BUS_NUMBER,
            4,
            &mut bus as *mut u32 as *mut u8,
            &mut bus_len,
        )
    };
    let address_status = unsafe {
        IoGetDeviceProperty(
            pdo,
            DEVICE_PROPERTY_ADDRESS,
            4,
            &mut address as *mut u32 as *mut u8,
            &mut address_len,
        )
    };
    if bus_status < 0 || address_status < 0 || bus_len < 4 || address_len < 4 {
        return None;
    }
    Some((bus, pci_slot(address)))
}

#[no_mangle]
pub unsafe extern "system" fn NdisInitializeWrapper(
    wrapper: *mut *mut u8,
    driver: *mut u8,
    _registry: *mut u8,
    _unused: *mut u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisinitializewrapper */
    if wrapper.is_null() {
        return;
    }
    unsafe { DbgPrint(b"NDIS5 WRAP\n\0".as_ptr()) };
    *wrapper = driver;
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterMiniport(
    wrapper: *mut u8,
    chars: *mut u8,
    length: u32,
) -> u32 {
    unsafe { DbgPrint(b"NDIS5 REG %u\n\0".as_ptr(), length) };
    if wrapper.is_null() || chars.is_null() || length < (CH_INIT as u32 + 8) {
        return INVALID_PARAMETER;
    }
    if unsafe { *chars } != 5 {
        return BAD_VERSION;
    }
    let keep = (length as usize).min(CHARS_KEEP);
    let copy = unsafe { ExAllocatePoolWithTag(0, CHARS_KEEP, POOL_TAG) };
    let state = unsafe { ExAllocatePoolWithTag(0, size_of::<Legacy>(), POOL_TAG) } as *mut Legacy;
    if copy.is_null() || state.is_null() {
        unsafe {
            if !copy.is_null() {
                ExFreePoolWithTag(copy, POOL_TAG);
            }
            if !state.is_null() {
                ExFreePoolWithTag(state as *mut u8, POOL_TAG);
            }
        }
        return RESOURCES;
    }
    unsafe {
        ptr::write_bytes(copy, 0, CHARS_KEEP);
        ptr::copy_nonoverlapping(chars, copy, keep);
        ptr::write_bytes(state as *mut u8, 0, size_of::<Legacy>());
        (*state).kind = LEGACY_KIND;
        (*state).chars = copy;
        (*state).chars_len = keep as u32;
        let mp = crate::register::register_legacy_driver(wrapper, state as *mut u8);
        if mp.is_null() {
            ExFreePoolWithTag(copy, POOL_TAG);
            ExFreePoolWithTag(state as *mut u8, POOL_TAG);
            return RESOURCES;
        }
        (*state).mp = mp;
        ACTIVE = state;
        DbgPrint(b"NDIS5 REGOK\n\0".as_ptr());
    }
    SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn NdisMSetAttributesEx(
    handle: *mut u8,
    context: *mut u8,
    _hang_seconds: u32,
    flags: u32,
    _adapter_type: u32,
) {
    unsafe { DbgPrint(b"NDIS5 ATTR %08x %p\n\0".as_ptr(), flags, context) };
    let state = state_for(handle);
    if !state.is_null() {
        unsafe { (*state).driver_ctx = context };
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterInterrupt(
    interrupt: *mut u8,
    handle: *mut u8,
    vector: u32,
    level: u32,
    _request_isr: u32,
    _shared: u32,
    mode: u32,
) -> u32 {
    unsafe { DbgPrint(b"NDIS5 IRQ %08x %08x %08x\n\0".as_ptr(), vector, level, mode) };
    let state = state_for(handle);
    if state.is_null() || interrupt.is_null() {
        return INVALID_PARAMETER;
    }
    let mut chars = [0u8; IRQ_CHARS];
    chars[2] = IRQ_CHARS as u8;
    write_fn(chars.as_mut_ptr(), IRQ_ISR_AT, legacy_isr as *const ());
    write_fn(chars.as_mut_ptr(), IRQ_DPC_AT, legacy_dpc as *const ());
    write_fn(chars.as_mut_ptr(), IRQ_DISABLE_AT, legacy_disable as *const ());
    write_fn(chars.as_mut_ptr(), IRQ_ENABLE_AT, legacy_enable as *const ());
    let mut irq_handle: *mut u8 = ptr::null_mut();
    let status = unsafe {
        crate::NdisMRegisterInterruptEx(
            (*state).mp,
            (*state).driver_ctx,
            chars.as_mut_ptr(),
            &mut irq_handle,
        )
    };
    if status != SUCCESS {
        return status;
    }
    unsafe {
        (*state).mp_interrupt = interrupt;
        ptr::write_bytes(interrupt, 0, 64);
        write_fn(interrupt, MI_ISR, slot_fn(state, CH_ISR));
        write_fn(interrupt, MI_DPC, slot_fn(state, CH_HANDLE_IRQ));
        ptr::write_unaligned(interrupt.add(MI_RESERVED) as *mut *mut u8, irq_handle);
        crate::connect_line_only((*state).mp);
        let object = crate::line_interrupt_object((*state).mp);
        ptr::write_unaligned(interrupt.add(MI_OBJECT) as *mut *mut u8, object);
        DbgPrint(b"NDIS5 IRQOK %p\n\0".as_ptr(), object);
    }
    SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterInterrupt(interrupt: *mut u8) {
    if interrupt.is_null() {
        return;
    }
    let irq = unsafe { ptr::read_unaligned(interrupt.add(MI_RESERVED) as *const *mut u8) };
    unsafe { crate::NdisMDeregisterInterruptEx(irq) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMSynchronizeWithInterrupt(
    interrupt: *mut u8,
    function: *const (),
    context: *mut u8,
) -> u8 {
    if function.is_null() {
        return 0;
    }
    let call: unsafe extern "system" fn(*mut u8) -> u8 = unsafe { core::mem::transmute(function) };
    let object = if interrupt.is_null() {
        ptr::null_mut()
    } else {
        unsafe { ptr::read_unaligned(interrupt.add(MI_OBJECT) as *const *mut u8) }
    };
    if object.is_null() {
        return unsafe { call(context) };
    }
    unsafe { KeSynchronizeExecution(object, function, context) }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMIndicateStatus(
    handle: *mut u8,
    status: u32,
    _buffer: *mut u8,
    _length: u32,
) {
    let _ = handle;
    unsafe { DbgPrint(b"NDIS5 STAT %08x\n\0".as_ptr(), status) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMIndicateStatusComplete(handle: *mut u8) {
    let _ = handle;
    unsafe { DbgPrint(b"NDIS5 STATDONE\n\0".as_ptr()) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMInitializeTimer(
    timer: *mut u8,
    handle: *mut u8,
    function: *const (),
    context: *mut u8,
) {
    let _ = handle;
    if timer.is_null() {
        return;
    }
    unsafe {
        KeInitializeTimer(timer);
        KeInitializeDpc(timer.add(TIMER_DPC), timer_dpc as *const (), timer);
        ptr::write_unaligned(timer.add(TIMER_FN) as *mut *const (), function);
        ptr::write_unaligned(timer.add(TIMER_CTX) as *mut *mut u8, context);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisSetTimer(timer: *mut u8, milliseconds: u32) {
    if timer.is_null() {
        return;
    }
    let due = if milliseconds == 0 {
        -1i64
    } else {
        -((milliseconds as i64) * 10_000)
    };
    unsafe { KeSetTimerEx(timer, due, 0, timer.add(TIMER_DPC)) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMCancelTimer(timer: *mut u8, cancelled: *mut u8) {
    let was = if timer.is_null() {
        0
    } else {
        unsafe { KeCancelTimer(timer) }
    };
    if !cancelled.is_null() {
        unsafe { *cancelled = was };
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisOpenConfiguration(
    status: *mut u32,
    handle: *mut *mut u8,
    context: *mut u8,
) {
    let state = state_for(context);
    let config = if state.is_null() {
        context
    } else {
        unsafe { (*state).mp }
    };
    if !handle.is_null() {
        unsafe { *handle = config };
    }
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisMInitializeScatterGatherDma(
    _handle: *mut u8,
    dma64: u32,
    mapping: u32,
) -> u32 {
    unsafe { DbgPrint(b"NDIS5 SGDMA %08x %08x\n\0".as_ptr(), dma64, mapping) };
    let state = state_for(_handle);
    if !state.is_null() && dma64 == 0 {
        unsafe { (*state).dma32 = 1 };
    }
    SUCCESS
}

#[no_mangle]
pub unsafe extern "system" fn NdisMFreeMapRegisters(_handle: *mut u8) {
    unsafe { DbgPrint(b"NDIS5 MAPFREE\n\0".as_ptr()) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMQueryAdapterResources(
    status: *mut u32,
    context: *mut u8,
    list: *mut u8,
    bytes: *mut u32,
) {
    let state = state_for(context);
    let mp = if state.is_null() { context } else { unsafe { (*state).mp } };
    let partial = crate::register::miniport_resources(mp);
    if partial.is_null() || bytes.is_null() {
        write_status(status, RESOURCES);
        return;
    }
    let count = read_u32(partial, CM_COUNT_AT) as usize;
    let need = (CM_DESC_AT + count * CM_DESC_BYTES) as u32;
    let have = unsafe { *bytes };
    unsafe { *bytes = need };
    if list.is_null() || have < need {
        write_status(status, BUFFER_TOO_SHORT);
        return;
    }
    unsafe { ptr::copy_nonoverlapping(partial, list, need as usize) };
    write_status(status, SUCCESS);
}

#[no_mangle]
pub unsafe extern "system" fn NdisReadPciSlotInformation(
    handle: *mut u8,
    _slot: u32,
    offset: u32,
    buffer: *mut u8,
    length: u32,
) -> u32 {
    let state = state_for(handle);
    if state.is_null() {
        return 0;
    }
    unsafe { crate::NdisMGetBusData((*state).mp, 0, offset, buffer, length) }
}

#[no_mangle]
pub unsafe extern "system" fn NdisWritePciSlotInformation(
    handle: *mut u8,
    _slot: u32,
    offset: u32,
    buffer: *mut u8,
    length: u32,
) -> u32 {
    let state = state_for(handle);
    if state.is_null() || buffer.is_null() || length == 0 {
        return 0;
    }
    let Some((bus, slot)) = pci_location(unsafe { (*state).mp }) else {
        return 0;
    };
    unsafe { HalSetBusDataByOffset(PCI_CONFIGURATION, bus, slot, buffer, offset, length) }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateMemoryWithTag(
    address: *mut *mut u8,
    length: u32,
    tag: u32,
) -> u32 {
    if address.is_null() || length == 0 {
        return INVALID_PARAMETER;
    }
    let block = unsafe { ExAllocatePoolWithTag(0, length as usize, tag) };
    if block.is_null() {
        unsafe { *address = ptr::null_mut() };
        return RESOURCES;
    }
    unsafe {
        ptr::write_bytes(block, 0, length as usize);
        *address = block;
    }
    SUCCESS
}

#[no_mangle]
pub unsafe extern "C" fn NdisWriteErrorLogEntry(handle: *mut u8, code: u32, count: u32) {
    let _ = (handle, count);
    unsafe { DbgPrint(b"NDIS5 ERR %08x\n\0".as_ptr(), code) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterAdapterShutdownHandler(
    handle: *mut u8,
    context: *mut u8,
    handler: *const (),
) {
    let state = state_for(handle);
    if state.is_null() {
        return;
    }
    unsafe {
        (*state).shutdown = handler;
        (*state).shutdown_ctx = context;
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterAdapterShutdownHandler(handle: *mut u8) {
    let state = state_for(handle);
    if state.is_null() {
        return;
    }
    unsafe {
        (*state).shutdown = ptr::null();
        (*state).shutdown_ctx = ptr::null_mut();
    }
}