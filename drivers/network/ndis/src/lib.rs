/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/lib.rs
 * PURPOSE:     NDIS 6.84 version and routine lookup
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * NdisGetVersion and NdisGetRoutineAddress match the declarations in
 * sdk/include/ddk/ndis.h. The counted string is UNICODE_STRING in
 * sdk/include/xdk/ntdef.template.h. The names and 0x00060054 come from
 * output/ndis-discovery/windows.log (DIFF END OK). The version split is
 * the public NdisGetVersion page: the high half is the major and the low
 * half is the minor.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisgetversion
 * output/netkvm-20261116T002404Z.log names NdisFreeRWLock.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreerwlock
 * output/netkvm-20261116T003238Z.log names NdisFreeIoWorkItem.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreeioworkitem
 * output/netkvm-20261116T003507Z.log names NdisQueueIoWorkItem.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisqueueioworkitem
 * output/netkvm-20261116T003735Z.log names NdisAllocateIoWorkItem.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocateioworkitem
 * The free and queue stubs log the call. Those pages return no status.
 * Allocate returns null, which that page returns when the allocation fails.
 * output/netkvm-20261116T004009Z.log names NdisMDeregisterScatterGatherDma.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterscattergatherdma
 * output/netkvm-20261116T004240Z.log names NdisMRegisterScatterGatherDma.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterscattergatherdma
 * The deregister page returns no status. Register returns a failure status from the stub.
 * output/netkvm-20261116T004517Z.log names NdisMSynchronizeWithInterruptEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsynchronizewithinterruptex
 * It returns false, which that page returns when the callback does not run.
 * output/netkvm-20261116T004752Z.log names NdisMDeregisterInterruptEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterinterruptex
 * output/netkvm-20261116T005020Z.log names NdisMRegisterInterruptEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterinterruptex
 * Deregister returns no status. Register returns the stub failure status.
 * output/netkvm-20261116T005259Z.log names NdisAcquireRWLockWrite.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisacquirerwlockwrite
 * output/netkvm-20261116T005640Z.log names NdisMRemoveMiniport.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismremoveminiport
 * Those pages return no status.
 * output/netkvm-20261116T005913Z.log names NdisSetOptionalHandlers.
 * output/netkvm-20261116T032525Z.log called it from registration and
 * DriverEntry returned 0xC0000002. register.rs logs its header.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissetoptionalhandlers
 * output/netkvm-20261116T010137Z.log names NdisOpenConfigurationEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenconfigurationex
 * output/netkvm-20261116T032858Z.log called it, then faulted at address
 * 0x60. This file returns success and a handle.
 * output/netkvm-20261116T043614Z.log printed NDIS HANDLE same and
 * DriverEntry returned 0xC0010004. This file prints the object passed
 * to NdisOpenConfigurationEx. output/netkvm-20261116T043930Z.log printed
 * NDIS COBJ type a9, revision 01, size 0014. Offset 4 is 80 f8 ff ff,
 * offset 8 is a pool pointer, and offset 0x10 is zero. DriverEntry
 * returned 0xC0010004. This file prints the attributes block passed to
 * NdisRegisterDeviceEx. output/netkvm-20261116T044705Z.log printed
 * NDIS ATTR type 85, revision 01, size 0038. Offset 4 is zero, three
 * pointers follow, and offset 0x20 is 08 00 00 00. DriverEntry returned
 * 0xC0010004. Capture 2026-11-16T050159Z called that header with probe
 * functions in those slots and returned 0xC0000033. This file prints
 * each slot as a UNICODE_STRING from sdk/include/xdk/ntdef.template.h.
 * output/netkvm-20261116T050231Z.log printed NDIS A8 \Device\NETKVMD,
 * NDIS A10 \DosDevices\NETKVMD, and an empty NDIS A18. Capture
 * 2026-11-16T050701Z returned 0 for those names and called no slot.
 * Offset 0x20 is extension size 8. This file calls IoCreateDevice and
 * IoCreateSymbolicLink with those measured names.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterdeviceex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iocreatedevice
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iocreatesymboliclink
 * FILE_DEVICE_NETWORK is 0x12 in sdk/include/xdk/iotypes.h.
 * DeviceExtension is at offset 0x40 in that same DEVICE_OBJECT.
 * output/netkvm-20261116T033320Z.log printed NDIS CONF, NDIS ALLOC
 * 00000080, and DriverEntry returned 0xC0000001.
 * output/netkvm-20261116T010402Z.log names NdisOpenConfigurationKeyByName.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenconfigurationkeybyname
 * output/netkvm-20261116T035548Z.log printed NDIS STR Parameters before
 * that call. sdk/include/ddk/ndis.h declares it VOID and writes the
 * status and the subkey handle. This file writes success and a handle.
 * output/netkvm-20261116T035852Z.log printed NDIS SUB Parameters, then
 * DriverEntry returned 0xC0010004.
 * output/netkvm-20261116T010626Z.log names NdisReleaseRWLock.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreleaserwlock
 * output/netkvm-20261116T010858Z.log names NdisAcquireRWLockRead.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisacquirerwlockread
 * output/netkvm-20261116T011315Z.log names NdisFreeCloneNetBufferList.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreeclonenetbufferlist
 * Those pages return no status.
 * output/netkvm-20261116T011547Z.log names NdisAllocateCloneNetBufferList.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocateclonenetbufferlist
 * It returns null, which that page returns when the allocation fails.
 * output/netkvm-20261116T011814Z.log names NdisMGetBusData.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismgetbusdata
 * output/netkvm-20261116T060635Z.log printed that stub returning 0,
 * then NDIS INIT c001001e. This file reads PCI config through the
 * stored PDO with IoGetDeviceProperty and HalGetBusDataByOffset from
 * sdk/include/xdk/iofuncs.h and sdk/include/xdk/halfuncs.h.
 * PCIConfiguration and PCI_WHICHSPACE_CONFIG are in
 * sdk/include/xdk/iotypes.h. DevicePropertyBusNumber and
 * DevicePropertyAddress are in that same header.
 * output/netkvm-20261116T012044Z.log names NdisReadConfiguration.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreadconfiguration
 * output/netkvm-20261116T033320Z.log called it and DriverEntry returned
 * 0xC0000001. The parameter block is NDIS_CONFIGURATION_PARAMETER in
 * sdk/include/ddk/ndis.h. This file writes NDIS_STATUS_SUCCESS.
 * output/netkvm-20261116T012404Z.log names NdisCloseConfiguration.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiscloseconfiguration
 * That page returns no status.
 * output/netkvm-20261116T012633Z.log names NdisReadNetworkAddress.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreadnetworkaddress
 * The status written is NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h.
 * The address pointer is left null and the length is 0.
 * output/netkvm-20261116T012904Z.log names NdisFreeMemory.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreememory
 * That page returns no status. This file frees with ExFreePoolWithTag.
 * output/netkvm-20261116T013204Z.log names NdisInitializeEvent.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisinitializeevent
 * output/netkvm-20261116T013429Z.log names NdisSetEvent.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissetevent
 * output/netkvm-20261116T013651Z.log names NdisResetEvent.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisresetevent
 * output/netkvm-20261116T013914Z.log names NdisWaitEvent.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiswaitevent
 * output/netkvm-20261116T060635Z.log printed those stubs during INIT,
 * with Wait returning 0, then NDIS INIT c001001e. NDIS_EVENT holds a
 * KEVENT in sdk/include/ddk/ndis.h. modules/rostests/kmtests/ntos_ke/KeMutex.c
 * sizes DISPATCHER_HEADER as 8 + 2 * sizeof(PVOID). KeInitializeEvent,
 * KeSetEvent, KeResetEvent, and KeWaitForSingleObject are in
 * sdk/include/xdk/kefuncs.h. NotificationEvent is in
 * sdk/include/xdk/ntdef.template.h. STATUS_TIMEOUT is in
 * sdk/include/psdk/ntstatus.h.
 * output/netkvm-20261116T014146Z.log names NdisInitializeString.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisinitializestring
 * That page returns no status. output/netkvm-20261116T035203Z.log printed
 * NDIS KEY with no characters after the stub. This file copies the
 * source into the UNICODE_STRING. output/netkvm-20261116T035548Z.log
 * printed NDIS STR Parameters, NDIS STR DisableMSI, and NDIS KEY DisableMSI.
 * output/netkvm-20261116T014406Z.log names NdisAllocateMemoryWithTagPriority.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocatememorywithtagpriority
 * output/netkvm-20261116T032858Z.log called it and then faulted at
 * address 0x60. This file returns the allocated block. NonPagedPool is 0.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_pool_type
 * output/netkvm-20261116T014631Z.log names NdisFreeMemoryWithTagPriority.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreememorywithtagpriority
 * That page returns no status. This file frees with ExFreePoolWithTag.
 * output/netkvm-20261116T014852Z.log names NdisMQueueDpcEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismqueuedpcex
 * It returns 0, which that page returns when the DPC is not queued.
 * output/netkvm-20261116T015125Z.log names NdisMSleep.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsleep
 * That page returns no status.
 * output/netkvm-20261116T015350Z.log names NdisRegisterDeviceEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterdeviceex
 * output/netkvm-20261116T033320Z.log called it and DriverEntry returned
 * 0xC0000001. This file returns success and two handles. The 0x80-byte
 * blocks are this file's buffers, not a Windows device object.
 * output/netkvm-20261116T015619Z.log names NdisDeregisterDeviceEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisderegisterdeviceex
 * That page returns no status. output/netkvm-20261116T040328Z.log called
 * it after NdisGetDeviceReservedExtension and DriverEntry returned
 * 0xC0010004. This file prints the device buffer and the extension.
 * output/netkvm-20261116T041534Z.log left both device buffers zero.
 * The extension's first qword is 50 4b 90 95 82 fa ff ff.
 * This file prints 0x80 bytes at that address. 0x80 is the
 * NDIS ALLOC size from output/netkvm-20261116T040328Z.log, not a
 * proven size of the pointer. output/netkvm-20261116T041854Z.log
 * printed NDIS AT for those bytes. DriverEntry returned 0xC0010004.
 * output/netkvm-20261116T015847Z.log names NdisGetDeviceReservedExtension.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisgetdevicereservedextension
 * output/netkvm-20261116T033717Z.log called it and faulted at address 0.
 * This file returns one page. That page is this file's buffer, not a
 * Windows extension size. output/netkvm-20261116T034055Z.log printed
 * NDIS EXT, then DriverEntry returned 0xC0010004.
 * output/netkvm-20261116T020120Z.log names NdisMUnmapIoSpace.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismunmapiospace
 * That page returns no status. sdk/include/xdk/mmfuncs.h declares
 * MmUnmapIoSpace.
 * output/netkvm-20261116T020355Z.log names NdisOpenAdapterEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenadapterex
 * The status returned is NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h.
 * The binding handle is left null.
 * output/netkvm-20261116T020819Z.log names NdisCloseAdapterEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiscloseadapterex
 * The status returned is NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h.
 * output/netkvm-20261116T021105Z.log names NdisMGetDeviceProperty.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismgetdeviceproperty
 * That page returns no status. register.rs stores the PDO, FDO, and
 * next-lower pointers on the adapter handle during AddDevice.
 * output/netkvm-20261116T021334Z.log names NdisAllocateRWLock.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocaterwlock
 * That page returns null when the allocation fails. output/netkvm-20261116T060247Z.log
 * printed NDIS INIT then that stub and NDIS INIT c000009a. This file
 * allocates an opaque lock block for that call. The block size is this
 * file's, not a Windows object size.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreerwlock
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisacquirerwlockwrite
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisacquirerwlockread
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreleaserwlock
 * output/netkvm-20261116T021602Z.log names NdisGroupActiveProcessorCount.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisgroupactiveprocessorcount
 * The count is KeQueryActiveProcessorCountEx. Group 0 and
 * ALL_PROCESSOR_GROUPS from sdk/include/xdk/ntbasedef.h return
 * KeNumberProcessors. Any other group returns 0.
 * output/netkvm-20261116T021850Z.log names NdisMAllocateSharedMemory.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismallocatesharedmemory
 * That page returns no status. The virtual address is left null and the
 * physical address is 0, which is the allocation-failure result.
 * output/netkvm-20261116T022127Z.log names NdisMFreeSharedMemory.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismfreesharedmemory
 * That page returns no status.
 * output/netkvm-20261116T022528Z.log names NdisMRegisterIoPortRange.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterioportrange
 * The status returned is NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h.
 * The port pointer is left null.
 * output/netkvm-20261116T022801Z.log names NdisMDeregisterIoPortRange.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterioportrange
 * That page returns no status.
 * output/netkvm-20261116T023033Z.log names NdisMMapIoSpace.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismmapiospace
 * output/netkvm-20261116T063023Z.log printed that stub then NDIS INIT
 * c0000010 (NDIS_STATUS_INVALID_DEVICE_REQUEST). This file maps with
 * MmMapIoSpace from sdk/include/xdk/mmfuncs.h. MmNonCached is 0 in
 * sdk/include/xdk/mmtypes.h.
 * output/netkvm-20261116T023400Z.log names NdisProcessorNumberToIndex.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisprocessornumbertoindex
 * The argument is PROCESSOR_NUMBER from sdk/include/xdk/ntbasedef.h.
 * The return is KeGetProcessorIndexFromNumber.
 */

#![no_std]

mod check;
mod filter;
mod ndis5;
mod pool;
mod protocol;
mod register;

use core::panic::PanicInfo;

#[repr(C)]
struct UnicodeString {
    length: u16,
    maximum_length: u16,
    buffer: *const u16,
}

struct Routine {
    name: &'static str,
    address: *const (),
}

unsafe impl Sync for Routine {}

/* sdk/include/xdk/iotypes.h */
const PCI_CONFIGURATION: u32 = 4;
const PCI_WHICHSPACE_CONFIG: u32 = 0;
const DEVICE_PROPERTY_BUS_NUMBER: u32 = 0xe;
const DEVICE_PROPERTY_ADDRESS: u32 = 0x10;
/* modules/rostests/kmtests/ntos_ke/KeMutex.c: 8 + 2 * sizeof(PVOID) on x64. */
const KEVENT_BYTES: usize = 24;
/* sdk/include/xdk/ntdef.template.h */
const NOTIFICATION_EVENT: u8 = 0;
/* sdk/include/xdk/ketypes.h */
const WAIT_EXECUTIVE: u8 = 0;
const KERNEL_MODE: u8 = 0;
/* sdk/include/psdk/ntstatus.h */
const STATUS_TIMEOUT: i32 = 0x0000_0102;

unsafe extern "system" {
    fn DbgPrint(format: *const u8, ...) -> i32;
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePool(address: *mut u8);
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
    fn KeQueryActiveProcessorCountEx(group: u16) -> u32;
    fn KeGetProcessorIndexFromNumber(processor: *const u8) -> u32;
    fn KeInitializeEvent(event: *mut u8, kind: u8, state: u8);
    fn KeSetEvent(event: *mut u8, increment: i32, wait: u8) -> i32;
    fn KeResetEvent(event: *mut u8) -> i32;
    fn KeWaitForSingleObject(
        object: *mut u8,
        reason: u8,
        wait_mode: u8,
        alertable: u8,
        timeout: *const i64,
    ) -> i32;
    fn IoCreateDevice(
        driver: *mut u8,
        extension: u32,
        name: *const u8,
        device_type: u32,
        characteristics: u32,
        exclusive: u8,
        device: *mut *mut u8,
    ) -> i32;
    fn IoCreateSymbolicLink(symbolic: *const u8, device: *const u8) -> i32;
    fn IoDeleteDevice(device: *mut u8);
    fn IoDeleteSymbolicLink(symbolic: *const u8) -> i32;
    fn IoGetDeviceProperty(
        device: *mut u8,
        property: u32,
        length: u32,
        buffer: *mut u8,
        result: *mut u32,
    ) -> i32;
    fn HalGetBusDataByOffset(
        bus_type: u32,
        bus: u32,
        slot: u32,
        buffer: *mut u8,
        offset: u32,
        length: u32,
    ) -> u32;
    fn MmMapIoSpace(physical: u64, bytes: usize, cache: u32) -> *mut u8;
    fn MmUnmapIoSpace(base: *mut u8, bytes: usize);
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "system" fn DriverEntry(_driver: *mut u8, _registry: *mut u8) -> i32 {
    /* STATUS_SUCCESS from sdk/include/psdk/ntstatus.h.
     * check.rs prints NDIS CHECK OK when the measured paths match. */
    check::run();
    0
}

#[no_mangle]
pub extern "system" fn NdisGetVersion() -> u32 {
    /* output/ndis-discovery/windows.log: DIFF NdisGetVersion 0x00060054.
     * output/netkvm-20261116T034749Z.log returned 0x00060053 from this
     * call and DriverEntry still returned 0xC0010004. */
    unsafe { DbgPrint(b"NDIS GETVER\n\0".as_ptr()) };
    0x0006_0054
}

#[no_mangle]
pub unsafe extern "system" fn NdisGetRoutineAddress(name: *const u8) -> *const () {
    if name.is_null() {
        return core::ptr::null();
    }
    let text = &*(name as *const UnicodeString);
    for routine in REAL
        .iter()
        .chain(POOLS.iter())
        .chain(MINIPORT.iter())
        .chain(FILTER.iter())
        .chain(PROTOCOL.iter())
        .chain(STUBS.iter())
    {
        if same(text, routine.name) {
            return routine.address;
        }
    }
    /* output/netkvm-20261116T041854Z.log: DriverEntry returned 0xC0010004
     * with no new stub. A miss from this lookup prints the name.
     * output/netkvm-20261116T042252Z.log printed no NDIS MISS line.
     * DriverEntry returned 0xC0010004. */
    log_keyword(b"NDIS MISS ", name);
    core::ptr::null()
}

fn same(text: &UnicodeString, name: &str) -> bool {
    let bytes = name.as_bytes();
    if text.buffer.is_null() || text.length as usize != bytes.len() * 2 {
        return false;
    }
    let mut index = 0;
    while index < bytes.len() {
        let unit = unsafe { *text.buffer.add(index) };
        if unit != bytes[index] as u16 {
            return false;
        }
        index += 1;
    }
    true
}

static POOLS: &[Routine] = &[
    Routine { name: "NdisAllocateNetBufferPool", address: pool::NdisAllocateNetBufferPool as *const () },
    Routine { name: "NdisFreeNetBufferPool", address: pool::NdisFreeNetBufferPool as *const () },
    Routine { name: "NdisAllocateNetBuffer", address: pool::NdisAllocateNetBuffer as *const () },
    Routine { name: "NdisFreeNetBuffer", address: pool::NdisFreeNetBuffer as *const () },
    Routine { name: "NdisAllocateNetBufferListPool", address: pool::NdisAllocateNetBufferListPool as *const () },
    Routine { name: "NdisFreeNetBufferListPool", address: pool::NdisFreeNetBufferListPool as *const () },
    Routine { name: "NdisAllocateNetBufferAndNetBufferList", address: pool::NdisAllocateNetBufferAndNetBufferList as *const () },
    Routine { name: "NdisFreeNetBufferList", address: pool::NdisFreeNetBufferList as *const () },
];

#[no_mangle]
pub extern "system" fn NdisAllocateIoWorkItem(_handle: *mut u8) -> *mut u8 {
    unsafe { DbgPrint(b"NDIS STUB NdisAllocateIoWorkItem\n\0".as_ptr()) };
    core::ptr::null_mut()
}

#[no_mangle]
pub extern "system" fn NdisAllocateCloneNetBufferList(
    _original: *mut u8,
    _list_pool: *mut u8,
    _buffer_pool: *mut u8,
    _flags: u32,
) -> *mut u8 {
    unsafe { DbgPrint(b"NDIS STUB NdisAllocateCloneNetBufferList\n\0".as_ptr()) };
    core::ptr::null_mut()
}

fn pci_slot_from_address(address: u32) -> u32 {
    /* sdk/include/xdk/iotypes.h PCI_SLOT_NUMBER: device is 5 bits, function
     * is the next 3. DevicePropertyAddress high word is the device and the
     * low word is the function. */
    let device = address >> 16;
    let function = address & 0xffff;
    device | (function << 5)
}

#[no_mangle]
pub unsafe extern "system" fn NdisMGetBusData(
    adapter: *mut u8,
    space: u32,
    offset: u32,
    buffer: *mut u8,
    length: u32,
) -> u32 {
    /* output/netkvm-20261116T060635Z.log: stub returned 0 then INIT
     * c001001e. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismgetbusdata */
    unsafe { DbgPrint(b"NDIS BUS %08x %08x %08x\n\0".as_ptr(), space, offset, length) };
    if space != PCI_WHICHSPACE_CONFIG || buffer.is_null() || length == 0 {
        return 0;
    }
    let (pdo, _, _) = register::miniport_devices(adapter);
    if pdo.is_null() {
        return 0;
    }
    let mut bus = 0u32;
    let mut bus_len = 0u32;
    let mut address = 0u32;
    let mut address_len = 0u32;
    let bus_status = IoGetDeviceProperty(
        pdo,
        DEVICE_PROPERTY_BUS_NUMBER,
        4,
        &mut bus as *mut u32 as *mut u8,
        &mut bus_len,
    );
    let address_status = IoGetDeviceProperty(
        pdo,
        DEVICE_PROPERTY_ADDRESS,
        4,
        &mut address as *mut u32 as *mut u8,
        &mut address_len,
    );
    if bus_status < 0 || address_status < 0 || bus_len < 4 || address_len < 4 {
        unsafe { DbgPrint(b"NDIS BUSPROP %08x %08x\n\0".as_ptr(), bus_status as u32, address_status as u32) };
        return 0;
    }
    let slot = pci_slot_from_address(address);
    let got = HalGetBusDataByOffset(
        PCI_CONFIGURATION,
        bus,
        slot,
        buffer,
        offset,
        length,
    );
    if got >= 4 {
        let vendor = u16::from_le_bytes(unsafe { [*buffer, *buffer.add(1)] });
        let device_id = u16::from_le_bytes(unsafe { [*buffer.add(2), *buffer.add(3)] });
        unsafe {
            DbgPrint(
                b"NDIS BUSGOT %08x bus %08x slot %08x id %04x:%04x\n\0".as_ptr(),
                got,
                bus,
                slot,
                vendor as u32,
                device_id as u32,
            )
        };
    } else {
        unsafe { DbgPrint(b"NDIS BUSGOT %08x\n\0".as_ptr(), got) };
    }
    got
}

#[no_mangle]
pub unsafe extern "system" fn NdisReadConfiguration(
    status: *mut u32,
    parameter: *mut *mut u8,
    _handle: *mut u8,
    _keyword: *mut u8,
    _kind: u32,
) {
    /* sdk/include/ddk/ndis.h NDIS_CONFIGURATION_PARAMETER: the type is
     * first, and the union holds an integer, an NDIS_STRING, or
     * BINARY_DATA. On x64 the pointer in that union puts the data at
     * offset 8. NDIS_STATUS_SUCCESS is 0 through STATUS_SUCCESS.
     * NDIS_STATUS_RESOURCES is 0xC000009A. */
    /* sdk/include/xdk/ntdef.template.h UNICODE_STRING: Length is the
     * first USHORT and Buffer follows it. output/netkvm-20261116T035548Z.log
     * printed NDIS KEY DisableMSI. That name is not in the registry
     * image, so this read writes NDIS_STATUS_FAILURE and a null
     * parameter. output/netkvm-20261116T040328Z.log did that and
     * DriverEntry still returned 0xC0010004. */
    unsafe { DbgPrint(b"NDIS READ %08x\n\0".as_ptr(), _kind) };
    log_keyword(b"NDIS KEY ", _keyword);
    if status.is_null() || parameter.is_null() {
        return;
    }
    if keyword_is(_keyword, b"DisableMSI") {
        *parameter = core::ptr::null_mut();
        *status = 0xC000_0001;
        return;
    }
    let block = ExAllocatePoolWithTag(0, 24, u32::from_le_bytes(*b"NsoV"));
    if block.is_null() {
        *status = 0xC000_009A;
        *parameter = core::ptr::null_mut();
        return;
    }
    core::ptr::write_bytes(block, 0, 24);
    core::ptr::write(block as *mut u32, _kind);
    *parameter = block;
    *status = 0;
}

#[no_mangle]
pub unsafe extern "system" fn NdisInitializeString(destination: *mut u8, source: *const u8) {
    /* sdk/include/xdk/ntdef.template.h UNICODE_STRING. The extra wide
     * character is this file's terminator. It is not a Windows size. */
    log_source(b"NDIS STR ", source);
    if destination.is_null() {
        return;
    }
    unsafe {
        core::ptr::write_unaligned(destination as *mut u16, 0);
        core::ptr::write_unaligned(destination.add(2) as *mut u16, 0);
        core::ptr::write_unaligned(destination.add(8) as *mut *mut u8, core::ptr::null_mut());
    }
    if source.is_null() {
        return;
    }
    let mut count = 0usize;
    while count < 128 && unsafe { *source.add(count) } != 0 {
        count += 1;
    }
    if count == 0 {
        return;
    }
    let bytes = count * 2;
    let buffer = unsafe { ExAllocatePoolWithTag(0, bytes + 2, u32::from_le_bytes(*b"NsoV")) };
    if buffer.is_null() {
        return;
    }
    for index in 0..count {
        let byte = unsafe { *source.add(index) } as u16;
        unsafe { core::ptr::write_unaligned(buffer.add(index * 2) as *mut u16, byte) };
    }
    unsafe {
        core::ptr::write_unaligned(buffer.add(bytes) as *mut u16, 0);
        core::ptr::write_unaligned(destination as *mut u16, bytes as u16);
        core::ptr::write_unaligned(destination.add(2) as *mut u16, (bytes + 2) as u16);
        core::ptr::write_unaligned(destination.add(8) as *mut *mut u8, buffer);
    }
}

fn log_source(prefix: &[u8], text: *const u8) {
    let mut line = [0u8; 80];
    let dest = line.as_mut_ptr();
    let mut at = 0isize;
    for byte in prefix {
        unsafe { *dest.offset(at) = *byte };
        at += 1;
    }
    if !text.is_null() {
        let mut index = 0isize;
        while index < 64 && at < 77 {
            let byte = unsafe { *text.offset(index) };
            if byte == 0 {
                break;
            }
            let shown = if byte < 0x80 { byte } else { b'?' };
            unsafe { *dest.offset(at) = shown };
            at += 1;
            index += 1;
        }
    }
    unsafe {
        *dest.offset(at) = b'\n';
        *dest.offset(at + 1) = 0;
        DbgPrint(dest);
    }
}

fn keyword_is(keyword: *const u8, text: &[u8]) -> bool {
    if keyword.is_null() {
        return false;
    }
    let length = unsafe { core::ptr::read_unaligned(keyword as *const u16) } as usize;
    let buffer = unsafe { core::ptr::read_unaligned(keyword.add(8) as *const *const u16) };
    if buffer.is_null() || length != text.len() * 2 {
        return false;
    }
    for index in 0..text.len() {
        let unit = unsafe { *buffer.add(index) };
        if unit != text[index] as u16 {
            return false;
        }
    }
    true
}

fn log_keyword(prefix: &[u8], keyword: *const u8) {
    if keyword.is_null() {
        return;
    }
    let length = unsafe { core::ptr::read_unaligned(keyword as *const u16) } as usize;
    let buffer = unsafe { core::ptr::read_unaligned(keyword.add(8) as *const *const u16) };
    let mut line = [0u8; 80];
    let dest = line.as_mut_ptr();
    let mut at = 0isize;
    for byte in prefix {
        unsafe { *dest.offset(at) = *byte };
        at += 1;
    }
    if !buffer.is_null() && length >= 2 {
        let chars = (length / 2).min(64);
        for index in 0..chars {
            if at >= 77 {
                break;
            }
            let unit = unsafe { *buffer.add(index) };
            let byte = if unit < 0x80 { unit as u8 } else { b'?' };
            unsafe { *dest.offset(at) = byte };
            at += 1;
        }
    }
    unsafe {
        *dest.offset(at) = b'\n';
        *dest.offset(at + 1) = 0;
        DbgPrint(dest);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisOpenConfigurationKeyByName(
    status: *mut u32,
    _handle: *mut u8,
    name: *const u8,
    subkey: *mut *mut u8,
) {
    /* output/netkvm-20261116T032858Z.log faulted at 0x60. The 0x80 bytes
     * are this file's buffer so that offset is inside the handle.
     * NDIS_STATUS_SUCCESS is 0. NDIS_STATUS_RESOURCES is 0xC000009A. */
    log_keyword(b"NDIS SUB ", name);
    if status.is_null() || subkey.is_null() {
        return;
    }
    let block = ExAllocatePoolWithTag(0, 0x80, u32::from_le_bytes(*b"NsoV"));
    if block.is_null() {
        *status = 0xC000_009A;
        *subkey = core::ptr::null_mut();
        return;
    }
    core::ptr::write_bytes(block, 0, 0x80);
    *subkey = block;
    *status = 0;
}

#[no_mangle]
pub unsafe extern "system" fn NdisReadNetworkAddress(
    status: *mut u32,
    address: *mut *mut u8,
    length: *mut u32,
    _handle: *mut u8,
) {
    /* NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h. */
    unsafe { DbgPrint(b"NDIS STUB NdisReadNetworkAddress\n\0".as_ptr()) };
    if !status.is_null() {
        *status = 0xC000_0001;
    }
    if !address.is_null() {
        *address = core::ptr::null_mut();
    }
    if !length.is_null() {
        *length = 0;
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateMemoryWithTagPriority(
    _handle: *mut u8,
    length: u32,
    tag: u32,
    _priority: u32,
) -> *mut u8 {
    unsafe { DbgPrint(b"NDIS ALLOC %08x\n\0".as_ptr(), length) };
    if length == 0 {
        return core::ptr::null_mut();
    }
    let block = ExAllocatePoolWithTag(0, length as usize, tag);
    if !block.is_null() {
        core::ptr::write_bytes(block, 0, length as usize);
    }
    block
}

#[no_mangle]
pub unsafe extern "system" fn NdisOpenConfigurationEx(
    config: *mut u8,
    handle: *mut *mut u8,
) -> u32 {
    /* output/netkvm-20261116T032858Z.log faulted at 0x60. The 0x80 bytes
     * are this file's buffer so that offset is inside the handle. They
     * are not a Windows structure size. NDIS_STATUS_INVALID_PARAMETER and
     * NDIS_STATUS_RESOURCES are 0xC000000D and 0xC000009A in
     * sdk/include/ddk/ndis.h. */
    unsafe { DbgPrint(b"NDIS CONF\n\0".as_ptr()) };
    if config.is_null() || handle.is_null() {
        return 0xC000_000D;
    }
    /* NDIS_OBJECT_HEADER size is the u16 at offset 2, in
     * sdk/include/psdk/ntddndis.h. 0x80 is this file's cap, not a
     * Windows size. output/netkvm-20261116T043614Z.log left the
     * miniport handle unchanged and DriverEntry returned 0xC0010004. */
    let mut dump = 16usize;
    let hdr_size = u16::from_le_bytes(unsafe { [*config.add(2), *config.add(3)] }) as usize;
    if hdr_size >= 4 && hdr_size <= 0x80 {
        dump = hdr_size;
    }
    log_block(b"NDIS COBJ ", config, dump, true);
    let block = ExAllocatePoolWithTag(0, 0x80, u32::from_le_bytes(*b"NsoV"));
    if block.is_null() {
        return 0xC000_009A;
    }
    core::ptr::write_bytes(block, 0, 0x80);
    *handle = block;
    0
}

#[no_mangle]
pub extern "system" fn NdisMQueueDpcEx(
    _interrupt: *mut u8,
    _message: u32,
    _processors: *mut u8,
    _importance: u32,
) -> u32 {
    unsafe { DbgPrint(b"NDIS STUB NdisMQueueDpcEx\n\0".as_ptr()) };
    0
}

/* FILE_DEVICE_NETWORK in sdk/include/xdk/iotypes.h. */
const FILE_DEVICE_NETWORK: u32 = 0x12;
/* DEVICE_OBJECT.DeviceExtension in sdk/include/xdk/iotypes.h on x64. */
const DEVICE_EXTENSION_AT: usize = 0x40;
/* output/netkvm-20261116T044705Z.log: dword at ATTR offset 0x20. */
const ATTR_EXTENSION_AT: usize = 0x20;
const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");

struct NdisDev {
    device_object: *mut u8,
    symlink_length: u16,
    symlink_chars: [u16; 32],
}

static mut DEVICE_OBJECT: *mut u8 = core::ptr::null_mut();
static mut NDIS_DEVICE: *mut u8 = core::ptr::null_mut();
static mut DEVICE_EXTENSION: *mut u8 = core::ptr::null_mut();
static mut SYMLINK_NAME: [u8; 16] = [0; 16];

#[no_mangle]
pub unsafe extern "system" fn NdisRegisterDeviceEx(
    handle: *mut u8,
    attributes: *mut u8,
    device: *mut *mut u8,
    ndis_device: *mut *mut u8,
) -> u32 {
    /* NDIS_OBJECT_HEADER size is the u16 at offset 2, in
     * sdk/include/psdk/ntddndis.h. NDIS_STATUS_INVALID_PARAMETER and
     * NDIS_STATUS_RESOURCES are in sdk/include/ddk/ndis.h. */
    unsafe { DbgPrint(b"NDIS DEV\n\0".as_ptr()) };
    if attributes.is_null() || device.is_null() || ndis_device.is_null() {
        return 0xC000_000D;
    }
    let mut dump = 16usize;
    let hdr_size = u16::from_le_bytes(unsafe { [*attributes.add(2), *attributes.add(3)] }) as usize;
    if hdr_size >= 4 && hdr_size <= 0x80 {
        dump = hdr_size;
    }
    log_block(b"NDIS ATTR ", attributes, dump, true);
    if dump >= 0x20 {
        log_keyword(b"NDIS A8 ", unsafe {
            core::ptr::read_unaligned(attributes.add(8) as *const *const u8)
        } as *const u8);
        log_keyword(b"NDIS A10 ", unsafe {
            core::ptr::read_unaligned(attributes.add(0x10) as *const *const u8)
        } as *const u8);
        log_keyword(b"NDIS A18 ", unsafe {
            core::ptr::read_unaligned(attributes.add(0x18) as *const *const u8)
        } as *const u8);
    }
    let driver_object = unsafe { register::mp_driver_object(handle) };
    if driver_object.is_null() {
        return 0xC000_000D;
    }
    let device_name = unsafe { core::ptr::read_unaligned(attributes.add(8) as *const *mut u8) };
    let symbolic = unsafe { core::ptr::read_unaligned(attributes.add(0x10) as *const *mut u8) };
    let extension = if dump >= ATTR_EXTENSION_AT + 4 {
        u32::from_le_bytes(unsafe {
            [
                *attributes.add(ATTR_EXTENSION_AT),
                *attributes.add(ATTR_EXTENSION_AT + 1),
                *attributes.add(ATTR_EXTENSION_AT + 2),
                *attributes.add(ATTR_EXTENSION_AT + 3),
            ]
        })
    } else {
        0
    };
    let mut object: *mut u8 = core::ptr::null_mut();
    let status = unsafe {
        IoCreateDevice(
            driver_object,
            extension,
            device_name,
            FILE_DEVICE_NETWORK,
            0,
            0,
            &mut object,
        )
    };
    if status < 0 || object.is_null() {
        unsafe { DbgPrint(b"NDIS DEVFAIL %08x\n\0".as_ptr(), status as u32) };
        return status as u32;
    }
    let wrapper = ExAllocatePoolWithTag(0, core::mem::size_of::<NdisDev>(), POOL_TAG) as *mut NdisDev;
    if wrapper.is_null() {
        unsafe { IoDeleteDevice(object) };
        return 0xC000_009A;
    }
    core::ptr::write_bytes(wrapper as *mut u8, 0, core::mem::size_of::<NdisDev>());
    (*wrapper).device_object = object;
    if !symbolic.is_null() {
        let length = unsafe { core::ptr::read_unaligned(symbolic as *const u16) } as usize;
        let buffer = unsafe { core::ptr::read_unaligned(symbolic.add(8) as *const *const u16) };
        let chars = (length / 2).min((*wrapper).symlink_chars.len());
        if !buffer.is_null() && chars > 0 {
            for index in 0..chars {
                (*wrapper).symlink_chars[index] = unsafe { *buffer.add(index) };
            }
            (*wrapper).symlink_length = (chars * 2) as u16;
            core::ptr::write_bytes(core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u8, 0, 16);
            core::ptr::write_unaligned(
                core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u16,
                (*wrapper).symlink_length,
            );
            core::ptr::write_unaligned(
                (core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u8).add(2) as *mut u16,
                (*wrapper).symlink_length,
            );
            core::ptr::write_unaligned(
                (core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u8).add(8) as *mut *mut u16,
                (*wrapper).symlink_chars.as_mut_ptr(),
            );
            let link = unsafe {
                IoCreateSymbolicLink(
                    core::ptr::addr_of!(SYMLINK_NAME) as *const u8,
                    device_name,
                )
            };
            if link < 0 {
                unsafe {
                    DbgPrint(b"NDIS LINKFAIL %08x\n\0".as_ptr(), link as u32);
                    IoDeleteDevice(object);
                    ExFreePoolWithTag(wrapper as *mut u8, POOL_TAG);
                }
                return link as u32;
            }
        }
    }
    DEVICE_OBJECT = object;
    NDIS_DEVICE = wrapper as *mut u8;
    DEVICE_EXTENSION = unsafe {
        core::ptr::read_unaligned(object.add(DEVICE_EXTENSION_AT) as *const *mut u8)
    };
    *device = object;
    *ndis_device = wrapper as *mut u8;
    0
}

fn hex_digit(nibble: u8) -> u8 {
    let nibble = nibble & 0xf;
    if nibble < 10 {
        b'0'.wrapping_add(nibble)
    } else {
        b'a'.wrapping_add(nibble - 10)
    }
}

fn log_block(prefix: &[u8], block: *const u8, size: usize, zeros: bool) {
    if block.is_null() {
        return;
    }
    let mut offset = 0usize;
    let mut printed = false;
    while offset < size {
        let mut any = false;
        let mut index = 0usize;
        while index < 16 && offset + index < size {
            if unsafe { *block.add(offset + index) } != 0 {
                any = true;
                break;
            }
            index += 1;
        }
        if any || zeros {
            let mut line = [0u8; 80];
            let dest = line.as_mut_ptr();
            let mut at = 0isize;
            for byte in prefix {
                unsafe { *dest.offset(at) = *byte };
                at += 1;
            }
            unsafe {
                *dest.offset(at) = hex_digit((offset >> 12) as u8);
                *dest.offset(at + 1) = hex_digit((offset >> 8) as u8);
                *dest.offset(at + 2) = hex_digit((offset >> 4) as u8);
                *dest.offset(at + 3) = hex_digit(offset as u8);
            }
            at += 4;
            index = 0;
            while index < 16 && offset + index < size {
                let byte = unsafe { *block.add(offset + index) };
                unsafe {
                    *dest.offset(at) = b' ';
                    *dest.offset(at + 1) = hex_digit(byte >> 4);
                    *dest.offset(at + 2) = hex_digit(byte);
                }
                at += 3;
                index += 1;
            }
            unsafe {
                *dest.offset(at) = b'\n';
                *dest.offset(at + 1) = 0;
                DbgPrint(dest);
            }
            printed = true;
        }
        offset += 16;
    }
    if !printed {
        let mut line = [0u8; 16];
        let dest = line.as_mut_ptr();
        let mut at = 0isize;
        for byte in prefix {
            unsafe { *dest.offset(at) = *byte };
            at += 1;
        }
        unsafe {
            *dest.offset(at) = b'\n';
            *dest.offset(at + 1) = 0;
            DbgPrint(dest);
        }
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisDeregisterDeviceEx(handle: *mut u8) {
    unsafe { DbgPrint(b"NDIS DROP\n\0".as_ptr()) };
    /* output/netkvm-20261116T042252Z.log: DriverEntry returned 0xC0010004
     * with no NDIS MISS line. register.rs prints the miniport handle.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisderegisterdeviceex
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iodeletedevice
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iodeletesymboliclink */
    unsafe { register::log_handle_later() };
    let extension = DEVICE_EXTENSION;
    log_block(b"NDIS DOBJ ", DEVICE_OBJECT, 0x80, true);
    log_block(b"NDIS DHND ", NDIS_DEVICE, core::mem::size_of::<NdisDev>(), true);
    if !extension.is_null() {
        log_block(b"NDIS EXTB ", extension, 0x10, true);
        let target = core::ptr::read_unaligned(extension as *const u64);
        if target >> 48 == 0xFFFF {
            log_block(b"NDIS AT ", target as *const u8, 0x80, true);
        }
    }
    if !handle.is_null() {
        let wrapper = handle as *mut NdisDev;
        if (*wrapper).symlink_length != 0 {
            core::ptr::write_bytes(core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u8, 0, 16);
            core::ptr::write_unaligned(
                core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u16,
                (*wrapper).symlink_length,
            );
            core::ptr::write_unaligned(
                (core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u8).add(2) as *mut u16,
                (*wrapper).symlink_length,
            );
            core::ptr::write_unaligned(
                (core::ptr::addr_of_mut!(SYMLINK_NAME) as *mut u8).add(8) as *mut *mut u16,
                (*wrapper).symlink_chars.as_mut_ptr(),
            );
            unsafe {
                IoDeleteSymbolicLink(core::ptr::addr_of!(SYMLINK_NAME) as *const u8);
            }
        }
        if !(*wrapper).device_object.is_null() {
            unsafe { IoDeleteDevice((*wrapper).device_object) };
        }
        ExFreePoolWithTag(handle, POOL_TAG);
    }
    DEVICE_OBJECT = core::ptr::null_mut();
    NDIS_DEVICE = core::ptr::null_mut();
    DEVICE_EXTENSION = core::ptr::null_mut();
}

#[no_mangle]
pub unsafe extern "system" fn NdisGetDeviceReservedExtension(device: *mut u8) -> *mut u8 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisgetdevicereservedextension */
    unsafe { DbgPrint(b"NDIS EXT\n\0".as_ptr()) };
    if device.is_null() {
        return core::ptr::null_mut();
    }
    let extension = unsafe {
        core::ptr::read_unaligned(device.add(DEVICE_EXTENSION_AT) as *const *mut u8)
    };
    DEVICE_EXTENSION = extension;
    extension
}

#[no_mangle]
pub unsafe extern "system" fn NdisOpenAdapterEx(
    _protocol: *mut u8,
    _context: *mut u8,
    _parameters: *mut u8,
    _bind: *mut u8,
    binding: *mut *mut u8,
) -> u32 {
    /* NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h. */
    unsafe { DbgPrint(b"NDIS STUB NdisOpenAdapterEx\n\0".as_ptr()) };
    if !binding.is_null() {
        *binding = core::ptr::null_mut();
    }
    0xC000_0001
}

#[no_mangle]
pub extern "system" fn NdisCloseAdapterEx(_binding: *mut u8) -> u32 {
    /* NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h. */
    unsafe { DbgPrint(b"NDIS STUB NdisCloseAdapterEx\n\0".as_ptr()) };
    0xC000_0001
}

#[no_mangle]
pub unsafe extern "system" fn NdisMGetDeviceProperty(
    handle: *mut u8,
    physical: *mut *mut u8,
    functional: *mut *mut u8,
    next: *mut *mut u8,
    resources: *mut *mut u8,
    translated: *mut *mut u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismgetdeviceproperty */
    let (pdo, fdo, lower) = register::miniport_devices(handle);
    unsafe { DbgPrint(b"NDIS PROP\n\0".as_ptr()) };
    if !physical.is_null() {
        *physical = pdo;
    }
    if !functional.is_null() {
        *functional = fdo;
    }
    if !next.is_null() {
        *next = lower;
    }
    if !resources.is_null() {
        *resources = core::ptr::null_mut();
    }
    if !translated.is_null() {
        *translated = core::ptr::null_mut();
    }
}

/* Opaque lock kind. The byte count is this file's, not a Windows size. */
const RW_KIND: u32 = u32::from_le_bytes(*b"RWso");
const RW_BYTES: usize = 16;

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateRWLock(_handle: *mut u8) -> *mut u8 {
    /* output/netkvm-20261116T060247Z.log: null from this stub made
     * MiniportInitializeEx return 0xC000009A.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocaterwlock */
    unsafe { DbgPrint(b"NDIS RWLOCK\n\0".as_ptr()) };
    let lock = ExAllocatePoolWithTag(0, RW_BYTES, u32::from_le_bytes(*b"NsoV"));
    if lock.is_null() {
        return core::ptr::null_mut();
    }
    core::ptr::write_bytes(lock, 0, RW_BYTES);
    core::ptr::write_unaligned(lock as *mut u32, RW_KIND);
    lock
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeRWLock(lock: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreerwlock */
    if lock.is_null() {
        return;
    }
    if core::ptr::read_unaligned(lock as *const u32) != RW_KIND {
        return;
    }
    core::ptr::write_unaligned(lock as *mut u32, 0);
    ExFreePoolWithTag(lock, u32::from_le_bytes(*b"NsoV"));
}

#[no_mangle]
pub extern "system" fn NdisAcquireRWLockWrite(
    _lock: *mut u8,
    _state: *mut u8,
    _flags: u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisacquirerwlockwrite */
}

#[no_mangle]
pub extern "system" fn NdisAcquireRWLockRead(
    _lock: *mut u8,
    _state: *mut u8,
    _flags: u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisacquirerwlockread */
}

#[no_mangle]
pub extern "system" fn NdisReleaseRWLock(_lock: *mut u8, _state: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreleaserwlock */
}

#[no_mangle]
pub extern "system" fn NdisGroupActiveProcessorCount(group: u16) -> u32 {
    unsafe {
        DbgPrint(b"NDIS STUB NdisGroupActiveProcessorCount\n\0".as_ptr());
        KeQueryActiveProcessorCountEx(group)
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMAllocateSharedMemory(
    _handle: *mut u8,
    _length: u32,
    _cached: u8,
    virtual_address: *mut *mut u8,
    physical: *mut u64,
) {
    unsafe { DbgPrint(b"NDIS STUB NdisMAllocateSharedMemory\n\0".as_ptr()) };
    if !virtual_address.is_null() {
        *virtual_address = core::ptr::null_mut();
    }
    if !physical.is_null() {
        *physical = 0;
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterIoPortRange(
    port: *mut *mut u8,
    _handle: *mut u8,
    _initial: u32,
    _count: u32,
) -> u32 {
    /* NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h. */
    unsafe { DbgPrint(b"NDIS STUB NdisMRegisterIoPortRange\n\0".as_ptr()) };
    if !port.is_null() {
        *port = core::ptr::null_mut();
    }
    0xC000_0001
}

#[no_mangle]
pub unsafe extern "system" fn NdisMMapIoSpace(
    virtual_address: *mut *mut u8,
    _handle: *mut u8,
    physical: u64,
    length: u32,
) -> u32 {
    /* output/netkvm-20261116T063023Z.log: stub then INIT c0000010.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismmapiospace */
    unsafe { DbgPrint(b"NDIS MAP %08x %08x\n\0".as_ptr(), (physical >> 32) as u32, physical as u32) };
    if virtual_address.is_null() || length == 0 {
        return 0xC000_000D;
    }
    let mapped = MmMapIoSpace(physical, length as usize, 0);
    if mapped.is_null() {
        *virtual_address = core::ptr::null_mut();
        return 0xC000_009A;
    }
    *virtual_address = mapped;
    unsafe { DbgPrint(b"NDIS MAPOK %p\n\0".as_ptr(), mapped) };
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMUnmapIoSpace(
    _handle: *mut u8,
    virtual_address: *mut u8,
    length: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismunmapiospace */
    if virtual_address.is_null() || length == 0 {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS UNMAP\n\0".as_ptr());
        MmUnmapIoSpace(virtual_address, length as usize);
    }
}

#[no_mangle]
pub extern "system" fn NdisProcessorNumberToIndex(processor: u32) -> u32 {
    let packed = processor;
    unsafe {
        DbgPrint(b"NDIS STUB NdisProcessorNumberToIndex\n\0".as_ptr());
        KeGetProcessorIndexFromNumber(&packed as *const u32 as *const u8)
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisInitializeEvent(event: *mut u8) {
    /* NDIS_EVENT is a KEVENT in sdk/include/ddk/ndis.h.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisinitializeevent */
    if event.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS EVENT init\n\0".as_ptr());
        core::ptr::write_bytes(event, 0, KEVENT_BYTES);
        KeInitializeEvent(event, NOTIFICATION_EVENT, 0);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisSetEvent(event: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissetevent */
    if event.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS EVENT set\n\0".as_ptr());
        KeSetEvent(event, 0, 0);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisResetEvent(event: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisresetevent */
    if event.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS EVENT reset\n\0".as_ptr());
        KeResetEvent(event);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisWaitEvent(event: *mut u8, milliseconds: u32) -> u8 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiswaitevent
     * Zero milliseconds waits without a timeout. A non-zero value is a
     * relative 100 ns count of that many milliseconds. */
    if event.is_null() {
        return 0;
    }
    unsafe { DbgPrint(b"NDIS EVENT wait %08x\n\0".as_ptr(), milliseconds) };
    let status = if milliseconds == 0 {
        unsafe { KeWaitForSingleObject(event, WAIT_EXECUTIVE, KERNEL_MODE, 0, core::ptr::null()) }
    } else {
        let timeout = -((milliseconds as i64) * 10_000);
        unsafe {
            KeWaitForSingleObject(event, WAIT_EXECUTIVE, KERNEL_MODE, 0, &timeout)
        }
    };
    if status == STATUS_TIMEOUT {
        0
    } else {
        1
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeMemory(address: *mut u8, _length: u32, _flags: u32) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreememory */
    if address.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS FREE\n\0".as_ptr());
        ExFreePool(address);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeMemoryWithTagPriority(
    _handle: *mut u8,
    address: *mut u8,
    tag: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreememorywithtagpriority */
    if address.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS FREETAG\n\0".as_ptr());
        ExFreePoolWithTag(address, tag);
    }
}

#[no_mangle]
pub extern "system" fn NdisMSynchronizeWithInterruptEx(
    _interrupt: *mut u8,
    _message: u32,
    _routine: *mut u8,
    _context: *mut u8,
) -> u8 {
    unsafe { DbgPrint(b"NDIS STUB NdisMSynchronizeWithInterruptEx\n\0".as_ptr()) };
    0
}

static MINIPORT: &[Routine] = &[
    Routine { name: "NdisAllocateIoWorkItem", address: NdisAllocateIoWorkItem as *const () },
    Routine { name: "NdisAllocateCloneNetBufferList", address: NdisAllocateCloneNetBufferList as *const () },
    Routine { name: "NdisMGetBusData", address: NdisMGetBusData as *const () },
    Routine { name: "NdisReadConfiguration", address: NdisReadConfiguration as *const () },
    Routine { name: "NdisReadNetworkAddress", address: NdisReadNetworkAddress as *const () },
    Routine { name: "NdisInitializeEvent", address: NdisInitializeEvent as *const () },
    Routine { name: "NdisSetEvent", address: NdisSetEvent as *const () },
    Routine { name: "NdisResetEvent", address: NdisResetEvent as *const () },
    Routine { name: "NdisWaitEvent", address: NdisWaitEvent as *const () },
    Routine { name: "NdisAllocateMemoryWithTagPriority", address: NdisAllocateMemoryWithTagPriority as *const () },
    Routine { name: "NdisFreeMemory", address: NdisFreeMemory as *const () },
    Routine { name: "NdisFreeMemoryWithTagPriority", address: NdisFreeMemoryWithTagPriority as *const () },
    Routine { name: "NdisOpenConfigurationEx", address: NdisOpenConfigurationEx as *const () },
    Routine { name: "NdisInitializeString", address: NdisInitializeString as *const () },
    Routine { name: "NdisOpenConfigurationKeyByName", address: NdisOpenConfigurationKeyByName as *const () },
    Routine { name: "NdisDeregisterDeviceEx", address: NdisDeregisterDeviceEx as *const () },
    Routine { name: "NdisMQueueDpcEx", address: NdisMQueueDpcEx as *const () },
    Routine { name: "NdisRegisterDeviceEx", address: NdisRegisterDeviceEx as *const () },
    Routine { name: "NdisGetDeviceReservedExtension", address: NdisGetDeviceReservedExtension as *const () },
    Routine { name: "NdisOpenAdapterEx", address: NdisOpenAdapterEx as *const () },
    Routine { name: "NdisCloseAdapterEx", address: NdisCloseAdapterEx as *const () },
    Routine { name: "NdisMGetDeviceProperty", address: NdisMGetDeviceProperty as *const () },
    Routine { name: "NdisAllocateRWLock", address: NdisAllocateRWLock as *const () },
    Routine { name: "NdisFreeRWLock", address: NdisFreeRWLock as *const () },
    Routine { name: "NdisAcquireRWLockWrite", address: NdisAcquireRWLockWrite as *const () },
    Routine { name: "NdisAcquireRWLockRead", address: NdisAcquireRWLockRead as *const () },
    Routine { name: "NdisReleaseRWLock", address: NdisReleaseRWLock as *const () },
    Routine { name: "NdisGroupActiveProcessorCount", address: NdisGroupActiveProcessorCount as *const () },
    Routine { name: "NdisMAllocateSharedMemory", address: NdisMAllocateSharedMemory as *const () },
    Routine { name: "NdisMRegisterIoPortRange", address: NdisMRegisterIoPortRange as *const () },
    Routine { name: "NdisMMapIoSpace", address: NdisMMapIoSpace as *const () },
    Routine { name: "NdisMUnmapIoSpace", address: NdisMUnmapIoSpace as *const () },
    Routine { name: "NdisProcessorNumberToIndex", address: NdisProcessorNumberToIndex as *const () },
    Routine { name: "NdisMSynchronizeWithInterruptEx", address: NdisMSynchronizeWithInterruptEx as *const () },
    Routine { name: "NdisSetOptionalHandlers", address: register::NdisSetOptionalHandlers as *const () },
    Routine { name: "NdisMRegisterMiniportDriver", address: register::NdisMRegisterMiniportDriver as *const () },
    Routine { name: "NdisMDeregisterMiniportDriver", address: register::NdisMDeregisterMiniportDriver as *const () },
    Routine { name: "NdisMSetMiniportAttributes", address: register::NdisMSetMiniportAttributes as *const () },
    Routine { name: "NdisMIndicateReceiveNetBufferLists", address: register::NdisMIndicateReceiveNetBufferLists as *const () },
    Routine { name: "NdisMSendNetBufferListsComplete", address: register::NdisMSendNetBufferListsComplete as *const () },
    Routine { name: "NdisMOidRequestComplete", address: register::NdisMOidRequestComplete as *const () },
    Routine { name: "NdisMRestartComplete", address: register::NdisMRestartComplete as *const () },
    Routine { name: "NdisMIndicateStatusEx", address: register::NdisMIndicateStatusEx as *const () },
    Routine { name: "NdisMPauseComplete", address: register::NdisMPauseComplete as *const () },
];

static FILTER: &[Routine] = &[
    Routine { name: "NdisFRegisterFilterDriver", address: filter::NdisFRegisterFilterDriver as *const () },
    Routine { name: "NdisFDeregisterFilterDriver", address: filter::NdisFDeregisterFilterDriver as *const () },
    Routine { name: "NdisFSetAttributes", address: filter::NdisFSetAttributes as *const () },
    Routine { name: "NdisFIndicateReceiveNetBufferLists", address: filter::NdisFIndicateReceiveNetBufferLists as *const () },
    Routine { name: "NdisFSendNetBufferLists", address: filter::NdisFSendNetBufferLists as *const () },
    Routine { name: "NdisFSendNetBufferListsComplete", address: filter::NdisFSendNetBufferListsComplete as *const () },
    Routine { name: "NdisFOidRequestComplete", address: filter::NdisFOidRequestComplete as *const () },
    Routine { name: "NdisFPauseComplete", address: filter::NdisFPauseComplete as *const () },
    Routine { name: "NdisFRestartComplete", address: filter::NdisFRestartComplete as *const () },
    Routine { name: "NdisFIndicateStatus", address: filter::NdisFIndicateStatus as *const () },
];

static PROTOCOL: &[Routine] = &[
    Routine { name: "NdisRegisterProtocolDriver", address: protocol::NdisRegisterProtocolDriver as *const () },
    Routine { name: "NdisDeregisterProtocolDriver", address: protocol::NdisDeregisterProtocolDriver as *const () },
    Routine { name: "NdisSendNetBufferLists", address: protocol::NdisSendNetBufferLists as *const () },
    Routine { name: "NdisReturnNetBufferLists", address: protocol::NdisReturnNetBufferLists as *const () },
];

static REAL: &[Routine] = &[
    Routine {
        name: "NdisGetVersion",
        address: NdisGetVersion as *const (),
    },
    Routine {
        name: "NdisGetRoutineAddress",
        address: NdisGetRoutineAddress as *const (),
    },
];

macro_rules! stubs {
    ($($name:ident),* $(,)?) => {
        $(
            #[no_mangle]
            pub extern "system" fn $name() -> u32 {
                /* STATUS_NOT_IMPLEMENTED from sdk/include/psdk/ntstatus.h. */
                unsafe { DbgPrint(concat!("NDIS STUB ", stringify!($name), "\n\0").as_ptr()) };
                0xC000_0002
            }
        )*

        static STUBS: &[Routine] = &[
            $(
                Routine {
                    name: stringify!($name),
                    address: $name as *const (),
                },
            )*
        ];
    };
}

stubs!(
    NdisAllocateNetBufferList,
    NdisMAllocateNetBufferSGList,
    NdisMFreeNetBufferSGList,
    NdisAllocateMdl,
    NdisFreeMdl,
    NdisGetDataBuffer,
    NdisOidRequest,
    NdisFOidRequest,
    NdisFreeIoWorkItem,
    NdisQueueIoWorkItem,
    NdisMDeregisterScatterGatherDma,
    NdisMRegisterScatterGatherDma,
    NdisMDeregisterInterruptEx,
    NdisMRegisterInterruptEx,
    NdisMRemoveMiniport,
    NdisFreeCloneNetBufferList,
    NdisCloseConfiguration,
    NdisMSleep,
    NdisMFreeSharedMemory,
    NdisMDeregisterIoPortRange
);
