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
 * output/netkvm-20261116T071822Z.log printed NDIS IRQ 84 01 0060, NDIS
 * IRQOK, then NDIS STUB NdisMRegisterScatterGatherDma and NDIS INIT
 * c0000002. NDIS_STATUS_SUCCESS is 0 in sdk/include/ddk/ndis.h. The
 * deregister page returns no status. ScatterGatherListSize OUT is the
 * one-element SCATTER_GATHER_LIST size from sdk/include/xdk/iotypes.h.
 * ProcessSGListHandler is at description offset 0x10 on that register
 * page; size 0x24 matches output/netkvm-20261116T072305Z.log.
 * output/netkvm-20261116T101041Z.log printed NDIS STUB
 * NdisMAllocateNetBufferSGList on every send.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismallocatenetbuffersglist
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismfreenetbuffersglist
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_process_sg_list
 * NET_BUFFER Mdl/DataLength/DataOffset are capture 2026-11-15T232142Z.
 * MDL.MappedSystemVa is sdk/include/xdk/mmtypes.h.
 * output/netkvm-20261116T004517Z.log names NdisMSynchronizeWithInterruptEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsynchronizewithinterruptex
 * KeSynchronizeExecution is
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kesynchronizeexecution
 * and sdk/include/xdk/kefuncs.h. Soft-only (no IoConnectInterrupt yet)
 * runs the sync routine directly; a connected object uses
 * KeSynchronizeExecution.
 * output/netkvm-20261116T004752Z.log names NdisMDeregisterInterruptEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterinterruptex
 * output/netkvm-20261116T005020Z.log names NdisMRegisterInterruptEx.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterinterruptex
 * output/netkvm-20261116T071255Z.log printed NDIS POOL 80 01 0010, then
 * NDIS STUB NdisMRegisterInterruptEx, then NDIS INIT c0000002
 * (STATUS_NOT_IMPLEMENTED in sdk/include/psdk/ntstatus.h). Public
 * NetKVM ParaNdis_FinishSpecificInitialization calls that register
 * after the pool allocate. output/netkvm-20261116T071822Z.log printed
 * NDIS IRQ 84 01 0060 and NDIS IRQOK. NDIS_STATUS_SUCCESS is 0 in
 * sdk/include/ddk/ndis.h. Deregister returns no status. Capture
 * 2026-11-16T080419Z reached DIFF END OK: after NdisMRegisterInterruptEx
 * the dword at characteristics offset 0x50 became 1 and the pointer at
 * 0x58 stayed null (DIFF IRQA0050). Those OUT offsets are the probe's
 * count from the public characteristics page, confirmed by that dump.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_interrupt_characteristics
 * Soft register stores handlers and parses CM resources without
 * IoConnectInterrupt during InitializeEx. Connect runs after Restart
 * (output/netkvm-20261116T103245Z.log and
 * output/netkvm-20261116T103516Z.log connected mid-init, printed
 * NDIS IRQDPC, then asserted KeGetCurrentIrql() <= APC_LEVEL in
 * ntoskrnl\mm\ARM3\miarm.h). Soft-only boot
 * output/netkvm-20261116T103750Z.log printed NDIS IRQ soft.
 * output/netkvm-20261116T183657Z.log: BINDALL skip and post-bind
 * RECVIND work, but the second gateway echo TX timed out with no
 * RECVIND; the next TX drained a stranded RECVIND. RX poll uses
 * KeSetTimerEx (sdk/include/xdk/kefuncs.h) while armed so replies that
 * arrive during IcmpSendEcho2 are drained without a SENDDONE kick.
 * ScatterGatherListSize was unchanged on that same Windows call.
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
 * output/netkvm-20261116T072305Z.log printed NDIS SGOK, then that stub
 * twice, then NDIS INIT c000009a. MmAllocateContiguousMemory,
 * MmGetPhysicalAddress, and MmFreeContiguousMemory are in
 * sdk/include/xdk/mmfuncs.h. That page returns no status.
 * output/netkvm-20261116T022127Z.log names NdisMFreeSharedMemory.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismfreesharedmemory
 * That page returns no status.
 * output/netkvm-20261116T022528Z.log names NdisMRegisterIoPortRange.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterioportrange
 * PortOffset receives InitialPort so NdisRawReadPort / WritePort use that
 * bus-relative base (drivers/network/dd/e1000/hardware.c,
 * drivers/network/dd/netkvm/Common/ParaNdis-VirtIO.c).
 * output/netkvm-20261116T022801Z.log names NdisMDeregisterIoPortRange.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterioportrange
 * That page returns no status.
 * output/netkvm-20261116T015125Z.log names NdisMSleep.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsleep
 * KeDelayExecutionThread is in sdk/include/xdk/kefuncs.h. A negative
 * interval is relative 100 ns units (drivers/network/rtl8139n/src/lib.rs).
 * output/netkvm-20261116T023033Z.log names NdisMMapIoSpace.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismmapiospace
 * output/netkvm-20261116T063023Z.log printed that stub then NDIS INIT
 * c0000010 (NDIS_STATUS_INVALID_DEVICE_REQUEST). This file maps with
 * MmMapIoSpace from sdk/include/xdk/mmfuncs.h. MmNonCached is 0 in
 * sdk/include/xdk/mmtypes.h. MAP logs the length so a short BAR map can
 * be told from a full one (output/netkvm-20261116T133444Z.log).
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
mod prot6;
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
    fn MmAllocateContiguousMemory(bytes: usize, highest: u64) -> *mut u8;
    fn MmFreeContiguousMemory(base: *mut u8);
    fn MmGetPhysicalAddress(base: *mut u8) -> u64;
    fn KeDelayExecutionThread(wait_mode: i32, alertable: u8, interval: *const i64) -> i32;
    fn IoAllocateMdl(
        virtual_address: *mut u8,
        length: u32,
        secondary: u8,
        charge_quota: u8,
        irp: *mut u8,
    ) -> *mut u8;
    fn IoFreeMdl(mdl: *mut u8);
    fn MmBuildMdlForNonPagedPool(mdl: *mut u8);
    fn IoConnectInterrupt(
        interrupt_object: *mut *mut u8,
        service_routine: *const (),
        service_context: *mut u8,
        spin_lock: *mut u8,
        vector: u32,
        irql: u8,
        synchronize_irql: u8,
        mode: u32,
        share_vector: u8,
        affinity: u64,
        floating_save: u8,
    ) -> i32;
    fn IoDisconnectInterrupt(interrupt_object: *mut u8);
    fn KeSynchronizeExecution(
        interrupt_object: *mut u8,
        synchronize_routine: *const (),
        synchronize_context: *mut u8,
    ) -> u8;
    fn KeInitializeDpc(dpc: *mut u8, routine: *const (), context: *mut u8);
    fn KeInsertQueueDpc(dpc: *mut u8, arg1: *mut u8, arg2: *mut u8) -> u8;
    /* sdk/include/xdk/kefuncs.h / ntoskrnl.spec. */
    fn KeInitializeTimer(timer: *mut u8);
    fn KeSetTimerEx(timer: *mut u8, due_time: i64, period: i32, dpc: *mut u8) -> u8;
    fn KeCancelTimer(timer: *mut u8) -> u8;
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn rust_eh_personality() {}

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
    Routine { name: "NdisAllocateNetBufferList", address: pool::NdisAllocateNetBufferList as *const () },
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
pub unsafe extern "system" fn NdisCloseConfiguration(handle: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiscloseconfiguration
     * That page returns no status. NdisOpenConfigurationEx and
     * NdisOpenConfigurationKeyByName allocate the 0x80-byte handle with
     * tag NsoV; free that block here. output/netkvm-20261116T081944Z.log
     * printed NDIS STUB NdisCloseConfiguration after those opens. */
    if handle.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS CLOSE\n\0".as_ptr());
        ExFreePoolWithTag(handle, u32::from_le_bytes(*b"NsoV"));
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisReadNetworkAddress(
    status: *mut u32,
    address: *mut *mut u8,
    length: *mut u32,
    _handle: *mut u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisreadnetworkaddress
     * NDIS_STATUS_FAILURE from sdk/include/ddk/ndis.h when NetworkAddress
     * is not in the registry. output/netkvm-20261116T081944Z.log printed
     * the stub before this return. */
    unsafe { DbgPrint(b"NDIS NETADDR\n\0".as_ptr()) };
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
pub unsafe extern "system" fn NdisMQueueDpcEx(
    interrupt: *mut u8,
    _message: u32,
    _processors: *mut u8,
    dpc_context: *mut u8,
) -> usize {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismqueuedpcex
     * Returns a KAFFINITY bitmap of scheduled CPUs. Line interrupts use
     * MessageId 0. Store MiniportDpcContext for the next DPC. */
    if interrupt.is_null()
        || core::ptr::read_unaligned(interrupt as *const u32) != IRQ_KIND
    {
        return 0;
    }
    core::ptr::write_unaligned(
        interrupt.add(IRQ_DPC_CONTEXT_AT) as *mut *mut u8,
        dpc_context,
    );
    KeInsertQueueDpc(
        interrupt.add(IRQ_DPC_AT),
        core::ptr::null_mut(),
        core::ptr::null_mut(),
    );
    1
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

const BINDING_BYTES: usize = 0x40;
const BINDING_KIND: u32 = u32::from_le_bytes(*b"BNso");
const BINDING_ADAPTER_AT: usize = 8;
const BINDING_CONTEXT_AT: usize = 16;
const BINDING_PROTOCOL_AT: usize = 24;

#[no_mangle]
pub unsafe extern "system" fn NdisOpenAdapterEx(
    protocol: *mut u8,
    context: *mut u8,
    parameters: *mut u8,
    bind: *mut u8,
    binding: *mut *mut u8,
) -> u32 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisopenadapterex
     * BindContext from BindAdapterEx is the miniport adapter handle on
     * this path (notify_adapter_ready). Open-parameters Header type is
     * not measured; dump Size from offset 2. SelectedMediumIndex at
     * derived offset 32 gets NdisMedium802_3 index 0. */
    unsafe { DbgPrint(b"NDIS OPENEX\n\0".as_ptr()) };
    if !parameters.is_null() {
        let kind = *parameters;
        let revision = *parameters.add(1);
        let size = u16::from_le_bytes([*parameters.add(2), *parameters.add(3)]);
        unsafe {
            DbgPrint(
                b"NDIS OPENHDR %02x %02x %04x\n\0".as_ptr(),
                kind as u32,
                revision as u32,
                size as u32,
            )
        };
        let selected = core::ptr::read_unaligned(parameters.add(32) as *const *mut u32);
        if !selected.is_null() {
            *selected = 0;
        }
    }
    if binding.is_null() {
        return 0xC000_000D;
    }
    let block = ExAllocatePoolWithTag(0, BINDING_BYTES, u32::from_le_bytes(*b"NsoV"));
    if block.is_null() {
        *binding = core::ptr::null_mut();
        return 0xC000_009A;
    }
    core::ptr::write_bytes(block, 0, BINDING_BYTES);
    core::ptr::write_unaligned(block as *mut u32, BINDING_KIND);
    let adapter = if !bind.is_null() {
        bind
    } else {
        core::ptr::null_mut()
    };
    core::ptr::write_unaligned(block.add(BINDING_ADAPTER_AT) as *mut *mut u8, adapter);
    core::ptr::write_unaligned(block.add(BINDING_CONTEXT_AT) as *mut *mut u8, context);
    core::ptr::write_unaligned(block.add(BINDING_PROTOCOL_AT) as *mut *mut u8, protocol);
    *binding = block;
    unsafe { DbgPrint(b"NDIS OPENEXOK %p\n\0".as_ptr(), block) };
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisOidRequest(binding: *mut u8, request: *mut u8) -> u32 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisoidrequest
     * Binding stores the miniport adapter handle at BINDING_ADAPTER_AT.
     * output/netkvm-20261116T091655Z.log printed NDIS STUB NdisOidRequest
     * after NDIS OPENEXOK on the second BindAdapterEx (OPENHDR 87 01 0034). */
    if binding.is_null() || request.is_null() {
        return 0xC000_000D;
    }
    if core::ptr::read_unaligned(binding as *const u32) != BINDING_KIND {
        return 0xC000_0001;
    }
    let adapter = core::ptr::read_unaligned(binding.add(BINDING_ADAPTER_AT) as *const *mut u8);
    register::call_oid_request(adapter, request)
}

#[no_mangle]
pub extern "system" fn NdisCloseAdapterEx(binding: *mut u8) -> u32 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndiscloseadapterex */
    if binding.is_null() {
        return 0xC000_000D;
    }
    unsafe {
        if core::ptr::read_unaligned(binding as *const u32) != BINDING_KIND {
            return 0xC000_0001;
        }
        DbgPrint(b"NDIS CLOSEEX\n\0".as_ptr());
        core::ptr::write_unaligned(binding as *mut u32, 0);
        ExFreePoolWithTag(binding, u32::from_le_bytes(*b"NsoV"));
    }
    0
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
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisgroupactiveprocessorcount
     * KeQueryActiveProcessorCountEx in ntoskrnl/ke/processor.c. */
    let count = unsafe { KeQueryActiveProcessorCountEx(group) };
    unsafe { DbgPrint(b"NDIS GROUP %04x %08x\n\0".as_ptr(), group as u32, count) };
    count
}

#[no_mangle]
pub unsafe extern "system" fn NdisAllocateMdl(
    _handle: *mut u8,
    virtual_address: *mut u8,
    length: u32,
) -> *mut u8 {
    /* output/netkvm-20261116T072815Z.log printed NDIS STUB NdisAllocateMdl
     * then faulted at 0xC0000002 (the stub's STATUS_NOT_IMPLEMENTED return
     * used as an MDL pointer).
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisallocatemdl
     * IoAllocateMdl and MmBuildMdlForNonPagedPool are in
     * sdk/include/xdk/mmfuncs.h / iofuncs. SecondaryBuffer and ChargeQuota
     * are 0; Irp is null. */
    if virtual_address.is_null() || length == 0 {
        return core::ptr::null_mut();
    }
    unsafe {
        DbgPrint(b"NDIS MDL %08x\n\0".as_ptr(), length);
        let mdl = IoAllocateMdl(virtual_address, length, 0, 0, core::ptr::null_mut());
        if mdl.is_null() {
            return core::ptr::null_mut();
        }
        MmBuildMdlForNonPagedPool(mdl);
        DbgPrint(b"NDIS MDLOK %p\n\0".as_ptr(), mdl);
        mdl
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisFreeMdl(mdl: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisfreemdl
     * IoFreeMdl is in ntoskrnl. */
    if mdl.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS MDLFREE\n\0".as_ptr());
        IoFreeMdl(mdl);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMAllocateSharedMemory(
    _handle: *mut u8,
    length: u32,
    _cached: u8,
    virtual_address: *mut *mut u8,
    physical: *mut u64,
) {
    /* output/netkvm-20261116T072305Z.log: null from this stub then INIT
     * c000009a.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismallocatesharedmemory
     * MmAllocateContiguousMemory and MmGetPhysicalAddress are in
     * sdk/include/xdk/mmfuncs.h. HighestAcceptableAddress all-ones is
     * this file's no-limit value for that call.
     * MmAllocateContiguousMemory may satisfy small sizes from
     * NonPagedPoolCacheAligned (ntoskrnl/mm/ARM3/contmem.c), which is
     * not page-aligned. output/netkvm-20261116T132341Z.log printed
     * NDIS SHMEMOK with physical addresses ending in 0x010 for 0x800
     * NetKVM RX buffers while IOAPIC IRQ11 stayed quiet and RX never
     * indicated. Round the allocation up to a whole page so the GPA
     * NetKVM posts on the virtqueue is page-aligned. */
    unsafe { DbgPrint(b"NDIS SHMEM %08x\n\0".as_ptr(), length) };
    if virtual_address.is_null() || physical.is_null() || length == 0 {
        if !virtual_address.is_null() {
            *virtual_address = core::ptr::null_mut();
        }
        if !physical.is_null() {
            *physical = 0;
        }
        return;
    }
    let bytes = ((length as usize) + 0xfff) & !0xfff;
    let va = unsafe { MmAllocateContiguousMemory(bytes, u64::MAX) };
    if va.is_null() {
        *virtual_address = core::ptr::null_mut();
        *physical = 0;
        return;
    }
    unsafe {
        core::ptr::write_bytes(va, 0, bytes);
        *virtual_address = va;
        *physical = MmGetPhysicalAddress(va);
        DbgPrint(
            b"NDIS SHMEMOK %p %08x %08x len %08x\n\0".as_ptr(),
            va,
            (*physical >> 32) as u32,
            *physical as u32,
            bytes as u32,
        );
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMFreeSharedMemory(
    _handle: *mut u8,
    _length: u32,
    _cached: u8,
    virtual_address: *mut u8,
    _physical: u64,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismfreesharedmemory
     * MmFreeContiguousMemory is in sdk/include/xdk/mmfuncs.h. */
    if virtual_address.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS SHMEMFREE\n\0".as_ptr());
        MmFreeContiguousMemory(virtual_address);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterIoPortRange(
    port: *mut *mut u8,
    _handle: *mut u8,
    initial: u32,
    count: u32,
) -> u32 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterioportrange
     * PortOffset is the bus-relative InitialPort for NdisRaw*Port.
     * drivers/network/dd/netkvm/Common/ParaNdis-VirtIO.c. */
    unsafe { DbgPrint(b"NDIS IOPORT %08x %08x\n\0".as_ptr(), initial, count) };
    if port.is_null() || count == 0 {
        return 0xC000_000D;
    }
    *port = initial as usize as *mut u8;
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterIoPortRange(
    _handle: *mut u8,
    _initial: u32,
    _count: u32,
    _port: *mut u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterioportrange */
    unsafe { DbgPrint(b"NDIS IOPORTFREE\n\0".as_ptr()) };
}

#[no_mangle]
pub unsafe extern "system" fn NdisMSleep(microseconds: u32) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsleep
     * KeDelayExecutionThread in sdk/include/xdk/kefuncs.h. Negative
     * interval is relative 100 ns (drivers/network/rtl8139n/src/lib.rs). */
    if microseconds == 0 {
        return;
    }
    let interval: i64 = -((microseconds as i64) * 10);
    unsafe {
        DbgPrint(b"NDIS SLEEP %08x\n\0".as_ptr(), microseconds);
        KeDelayExecutionThread(0, 0, &interval);
    }
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
    unsafe {
        DbgPrint(
            b"NDIS MAP %08x %08x len %08x\n\0".as_ptr(),
            (physical >> 32) as u32,
            physical as u32,
            length,
        )
    };
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

/* DMA object kind and field offsets. The 0x40-byte block is this file's
 * buffer, not a Windows object size. ProcessSGListHandler is at
 * description offset 0x10 on the public NDIS_SG_DMA_DESCRIPTION page
 * (size 0x24 matches output/netkvm-20261116T072305Z.log NDIS SG 83 01 0024).
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterscattergatherdma
 * FDO is HANDLE_FDO_AT from register.rs (AddDevice). */
const DMA_KIND: u32 = u32::from_le_bytes(*b"DMso");
const DMA_BYTES: usize = 0x40;
const DMA_PROCESS_AT: usize = 8;
const DMA_FDO_AT: usize = 16;
const SG_DESC_PROCESS_AT: usize = 0x10;
const SG_DESC_SIZE_AT: usize = 0x20;
/* One-element SCATTER_GATHER_LIST from sdk/include/xdk/iotypes.h on x64:
 * NumberOfElements + Reserved (0x10) plus one SCATTER_GATHER_ELEMENT (0x18). */
const SG_LIST_BYTES: usize = 0x28;
const SG_OWNED: u32 = u32::from_le_bytes(*b"SGow");
const SG_OWN_HEADER: usize = 16;
/* NET_BUFFER DataLength / Mdl / DataOffset: capture 2026-11-15T232142Z.
 * CurrentMdl / CurrentMdlOffset precede DataLength in the public
 * NET_BUFFER member order. */
const NB_CURRENT_MDL_AT: usize = 0x08;
const NB_CURRENT_MDL_OFFSET_AT: usize = 0x10;
const NB_LENGTH_AT: usize = 0x18;
const NB_MDL_AT: usize = 0x20;
const NB_OFFSET_AT: usize = 0x28;
/* MDL.MappedSystemVa in sdk/include/xdk/mmtypes.h on x64. */
const MDL_MAPPED_VA_AT: usize = 0x18;

type ProcessSgFn = unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8);

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterScatterGatherDma(
    adapter: *mut u8,
    description: *mut u8,
    dma_handle: *mut *mut u8,
) -> u32 {
    /* output/netkvm-20261116T071822Z.log: stub returned 0xC0000002 then
     * INIT c0000002.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterscattergatherdma
     * NDIS_STATUS_SUCCESS and NDIS_STATUS_RESOURCES are 0 and 0xC000009A
     * in sdk/include/ddk/ndis.h. ScatterGatherListSize OUT is the
     * one-element list size from sdk/include/xdk/iotypes.h. */
    if dma_handle.is_null() {
        return 0xC000_000D;
    }
    unsafe {
        let mut process: *mut u8 = core::ptr::null_mut();
        if !description.is_null() {
            let kind = *description;
            let revision = *description.add(1);
            let size = u16::from_le_bytes([*description.add(2), *description.add(3)]);
            DbgPrint(
                b"NDIS SG %02x %02x %04x\n\0".as_ptr(),
                kind as u32,
                revision as u32,
                size as u32,
            );
            if (size as usize) >= SG_DESC_PROCESS_AT + 8 {
                process = core::ptr::read_unaligned(
                    description.add(SG_DESC_PROCESS_AT) as *const *mut u8,
                );
            }
            if (size as usize) >= SG_DESC_SIZE_AT + 4 {
                let bytes = (SG_LIST_BYTES as u32).to_le_bytes();
                core::ptr::copy_nonoverlapping(
                    bytes.as_ptr(),
                    description.add(SG_DESC_SIZE_AT),
                    4,
                );
            }
        }
        let block = ExAllocatePoolWithTag(0, DMA_BYTES, POOL_TAG);
        if block.is_null() {
            *dma_handle = core::ptr::null_mut();
            return 0xC000_009A;
        }
        core::ptr::write_bytes(block, 0, DMA_BYTES);
        core::ptr::write_unaligned(block as *mut u32, DMA_KIND);
        core::ptr::write_unaligned(block.add(DMA_PROCESS_AT) as *mut *mut u8, process);
        let (_pdo, fdo, _next) = register::miniport_devices(adapter);
        core::ptr::write_unaligned(block.add(DMA_FDO_AT) as *mut *mut u8, fdo);
        *dma_handle = block;
        DbgPrint(b"NDIS SGOK\n\0".as_ptr());
    }
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterScatterGatherDma(dma: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterscattergatherdma */
    if dma.is_null() {
        return;
    }
    unsafe {
        DbgPrint(b"NDIS SGFREE\n\0".as_ptr());
        ExFreePoolWithTag(dma, POOL_TAG);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMAllocateNetBufferSGList(
    dma: *mut u8,
    net_buffer: *mut u8,
    context: *mut u8,
    _flags: u32,
    scatter_buffer: *mut u8,
    scatter_bytes: u32,
) -> u32 {
    /* output/netkvm-20261116T101041Z.log printed NDIS STUB
     * NdisMAllocateNetBufferSGList on every NDIS SEND, then SENDDONE
     * with no wire traffic (PING CHECK fail 000004cf for 10.0.2.2).
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismallocatenetbuffersglist
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_process_sg_list
     * SCATTER_GATHER_LIST is sdk/include/xdk/iotypes.h. NET_BUFFER
     * Mdl/DataLength/DataOffset are capture 2026-11-15T232142Z.
     * MDL.MappedSystemVa is sdk/include/xdk/mmtypes.h. */
    if dma.is_null()
        || net_buffer.is_null()
        || core::ptr::read_unaligned(dma as *const u32) != DMA_KIND
    {
        return 0xC000_000D;
    }
    let process_ptr =
        core::ptr::read_unaligned(dma.add(DMA_PROCESS_AT) as *const *mut u8);
    if process_ptr.is_null() {
        unsafe { DbgPrint(b"NDIS SGALLOC null\n\0".as_ptr()) };
        return 0xC000_009A;
    }
    /* Public allocate page builds the list from CurrentMdl /
     * CurrentMdlOffset; fall back to MdlChain / DataOffset. */
    let mut mdl =
        core::ptr::read_unaligned(net_buffer.add(NB_CURRENT_MDL_AT) as *const *mut u8);
    let mut mdl_offset =
        core::ptr::read_unaligned(net_buffer.add(NB_CURRENT_MDL_OFFSET_AT) as *const u32);
    if mdl.is_null() {
        mdl = core::ptr::read_unaligned(net_buffer.add(NB_MDL_AT) as *const *mut u8);
        mdl_offset = core::ptr::read_unaligned(net_buffer.add(NB_OFFSET_AT) as *const u32);
    }
    if mdl.is_null() {
        unsafe { DbgPrint(b"NDIS SGALLOC mdl\n\0".as_ptr()) };
        return 0xC000_009A;
    }
    let data_length = core::ptr::read_unaligned(net_buffer.add(NB_LENGTH_AT) as *const u64);
    let mapped =
        core::ptr::read_unaligned(mdl.add(MDL_MAPPED_VA_AT) as *const *mut u8);
    if mapped.is_null() || data_length == 0 || data_length > u32::MAX as u64 {
        unsafe { DbgPrint(b"NDIS SGALLOC va\n\0".as_ptr()) };
        return 0xC000_009A;
    }
    /* List covers the MDL from its start; CurrentMdlOffset is applied by
     * the miniport (public allocate page). */
    let physical = MmGetPhysicalAddress(mapped);
    let element_length = (mdl_offset as u64).saturating_add(data_length);
    if element_length > u32::MAX as u64 {
        unsafe { DbgPrint(b"NDIS SGALLOC len\n\0".as_ptr()) };
        return 0xC000_009A;
    }
    /* Always allocate. The public allocate page says NDIS may allocate
     * even when ScatterGatherListBuffer is set; Free then owns the list. */
    let _ = (scatter_buffer, scatter_bytes);
    let block = ExAllocatePoolWithTag(0, SG_OWN_HEADER + SG_LIST_BYTES, POOL_TAG);
    if block.is_null() {
        unsafe { DbgPrint(b"NDIS SGALLOC nomem\n\0".as_ptr()) };
        return 0xC000_009A;
    }
    core::ptr::write_bytes(block, 0, SG_OWN_HEADER + SG_LIST_BYTES);
    core::ptr::write_unaligned(block as *mut u32, SG_OWNED);
    let list = block.add(SG_OWN_HEADER);
    core::ptr::write_bytes(list, 0, SG_LIST_BYTES);
    /* NumberOfElements at 0; Elements[0].Address at 0x10; Length at 0x18. */
    core::ptr::write_unaligned(list as *mut u32, 1);
    core::ptr::write_unaligned(list.add(0x10) as *mut u64, physical);
    core::ptr::write_unaligned(list.add(0x18) as *mut u32, element_length as u32);

    let fdo = core::ptr::read_unaligned(dma.add(DMA_FDO_AT) as *const *mut u8);
    let process: ProcessSgFn = core::mem::transmute(process_ptr);
    unsafe {
        DbgPrint(
            b"NDIS SGALLOC %p len %u pa %p\n\0".as_ptr(),
            list,
            element_length as u32,
            physical as *mut u8,
        );
        process(fdo, core::ptr::null_mut(), list, context);
    }
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMFreeNetBufferSGList(
    _dma: *mut u8,
    scatter_list: *mut u8,
    _net_buffer: *mut u8,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismfreenetbuffersglist */
    if scatter_list.is_null() {
        return;
    }
    let header = scatter_list.sub(SG_OWN_HEADER);
    if core::ptr::read_unaligned(header as *const u32) == SG_OWNED {
        unsafe {
            DbgPrint(b"NDIS SGLISTFREE\n\0".as_ptr());
            ExFreePoolWithTag(header, POOL_TAG);
        }
    }
}

/* Interrupt object layout is this file's, not a Windows size. Handler
 * offsets match the public NDIS_MINIPORT_INTERRUPT_CHARACTERISTICS page
 * with InterruptType at 0x50 / MessageInfoTable at 0x58 from capture
 * 2026-11-16T080419Z DIFF IRQA0050. Size 0x60 is output/netkvm-20261116T071822Z.log.
 * DisableInterruptHandler / EnableInterruptHandler follow InterruptDpc
 * on that same public member list (offsets 0x18 / 0x20).
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_interrupt_characteristics
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_enable_interrupt
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_disable_interrupt
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_isr
 * CM_PARTIAL_RESOURCE_LIST / CmResourceTypeInterrupt /
 * CmResourceShareShared are sdk/include/xdk/cmtypes.h.
 * IoConnectInterrupt is
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ioconnectinterrupt
 * ShareVector matches drivers/network/dd/rtl8139/ndis.c
 * (ShareDisposition == CmResourceShareShared).
 * output/netkvm-20261116T102602Z.log printed SGALLOC/SENDDONE with no
 * RECVLAN: RegisterInterruptEx never connected an ISR.
 * output/netkvm-20261116T103245Z.log connected mid-init, printed
 * NDIS IRQDPC, then asserted KeGetCurrentIrql() <= APC_LEVEL.
 * output/netkvm-20261116T105034Z.log and output/netkvm-20261116T111136Z.log
 * deferred IoConnectInterrupt until after Restart: IRQCONN and
 * IRQENABLE printed, then SENDDONE with no IRQDPC and PING CHECK fail
 * 000004cf. Connect during RegisterInterruptEx again; hold delivery
 * until after Restart (IRQ_ARMED). Armed DPC with a null
 * ReceiveThrottleParameters faulted in netkvm.sys
 * (output/netkvm-20261116T112400Z.log) and at DISPATCH asserted
 * APC_LEVEL (output/netkvm-20261116T113013Z.log). Public NetKVM
 * MiniportInterruptDPC reads MaxNblsToIndicate from that pointer
 * (https://github.com/virtio-win/kvm-guest-drivers-windows/blob/6340662c/NetKVM/wlh/ParaNdis6_Impl.cpp).
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_interrupt_dpc
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_receive_throttle_parameters */
const IRQ_KIND: u32 = u32::from_le_bytes(*b"IRso");
/* This file's interrupt block: KDPC at 0x40 (sdk/include/xdk/ketypes.h
 * KDPC is 0x40 on x64), throttle at 0x90, then a second KDPC + KTIMER
 * for RX poll. KTIMER is sdk/include/xdk/ketypes.h; size uses
 * KTIMER_ACTUAL_LENGTH there (Period + sizeof LONG). */
const IRQ_BYTES: usize = 0x180;
const IRQ_ARMED_AT: usize = 4;
const IRQ_CONTEXT_AT: usize = 8;
const IRQ_ISR_AT: usize = 0x10;
const IRQ_DPC_HANDLER_AT: usize = 0x18;
const IRQ_OBJECT_AT: usize = 0x20;
const IRQ_ADAPTER_AT: usize = 0x28;
const IRQ_DISABLE_AT: usize = 0x30;
const IRQ_ENABLE_AT: usize = 0x38;
const IRQ_DPC_AT: usize = 0x40;
/* NDIS_RECEIVE_THROTTLE_PARAMETERS: MaxNblsToIndicate ULONG then
 * MoreNblsPending bit. This file's 8-byte buffer, not a Windows size.
 * Past the KDPC at IRQ_DPC_AT (sdk/include/xdk/ketypes.h KDPC). */
const IRQ_THROTTLE_AT: usize = 0x90;
const IRQ_THROTTLE_MAX_AT: usize = IRQ_THROTTLE_AT;
const IRQ_THROTTLE_MORE_AT: usize = IRQ_THROTTLE_AT + 4;
const IRQ_DPC_CONTEXT_AT: usize = 0x98;
/* Set by kick_interrupt_dpc / RX timer; cleared in the DPC. This file. */
const IRQ_SYN_AT: usize = 0x9c;
const IRQ_SYN_SEND: u8 = 1;
const IRQ_SYN_TIMER: u8 = 2;
/* Second KDPC + KTIMER past the first DPC block (this file's layout). */
const IRQ_RX_TIMER_DPC_AT: usize = 0xa0;
const IRQ_RX_TIMER_AT: usize = 0xe0;
/* Relative 50 ms in 100 ns units; period ms for KeSetTimerEx
 * (sdk/include/xdk/kefuncs.h). output/netkvm-20261116T183657Z.log:
 * second gateway TX then PING CHECK timeout with no RECVIND; next TX
 * drained a stranded RECVIND. IRQISR stayed unused; SENDDONE kick
 * alone misses replies that arrive while IcmpSendEcho2 blocks. */
const RX_POLL_DUE_100NS: i64 = -500_000;
const RX_POLL_PERIOD_MS: i32 = 50;
/* NDIS_INDICATE_ALL_NBLS on the public throttle page. */
const INDICATE_ALL_NBLS: u32 = 0xFFFF_FFFF;
static mut ISR_HITS: u32 = 0;
static mut RX_POLL_LOGS: u32 = 0;
type SyncRoutineFn = unsafe extern "system" fn(*mut u8) -> u8;
const IRQ_CHARS_ISR_AT: usize = 0x08;
const IRQ_CHARS_DPC_AT: usize = 0x10;
const IRQ_CHARS_DISABLE_AT: usize = 0x18;
const IRQ_CHARS_ENABLE_AT: usize = 0x20;
const IRQ_CHARS_TYPE_AT: usize = 0x50;
const IRQ_CHARS_MSG_AT: usize = 0x58;
const CM_RES_COUNT_AT: usize = 4;
const CM_RES_DESC_AT: usize = 8;
const CM_DESC_BYTES: usize = 0x14;
const CM_DESC_TYPE_AT: usize = 0;
const CM_DESC_SHARE_AT: usize = 1;
const CM_DESC_FLAGS_AT: usize = 2;
const CM_DESC_LEVEL_AT: usize = 4;
const CM_DESC_VECTOR_AT: usize = 8;
const CM_DESC_AFFINITY_AT: usize = 12;
const CM_TYPE_INTERRUPT: u8 = 2;
const CM_SHARE_SHARED: u8 = 3;
const CM_INTERRUPT_LATCHED: u16 = 1;
const LEVEL_SENSITIVE: u32 = 0;
const LATCHED: u32 = 1;

type MiniportIsrFn =
    unsafe extern "system" fn(*mut u8, *mut u8, *mut u32) -> u8;
type MiniportDpcFn =
    unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8);
type MiniportLineIrqFn = unsafe extern "system" fn(*mut u8);

unsafe fn call_line_irq_handler(block: *mut u8, slot: usize) {
    let handler =
        core::ptr::read_unaligned(block.add(slot) as *const *mut u8);
    let miniport_ctx =
        core::ptr::read_unaligned(block.add(IRQ_CONTEXT_AT) as *const *mut u8);
    if handler.is_null() {
        return;
    }
    let function: MiniportLineIrqFn = core::mem::transmute(handler);
    function(miniport_ctx);
}

unsafe extern "system" fn ndis_interrupt_service(
    _interrupt: *mut u8,
    context: *mut u8,
) -> u8 {
    if context.is_null() || core::ptr::read_unaligned(context as *const u32) != IRQ_KIND {
        return 0;
    }
    let isr_ptr =
        core::ptr::read_unaligned(context.add(IRQ_ISR_AT) as *const *mut u8);
    let miniport_ctx =
        core::ptr::read_unaligned(context.add(IRQ_CONTEXT_AT) as *const *mut u8);
    if isr_ptr.is_null() {
        return 0;
    }
    let mut queue_dpc: u8 = 0;
    let mut targets: u32 = 0;
    let isr: MiniportIsrFn = core::mem::transmute(isr_ptr);
    let claimed = isr(miniport_ctx, &mut queue_dpc, &mut targets);
    /* Public MiniportInterrupt page: queue from QueueDefaultInterruptDpc
     * / TargetProcessors regardless of the BOOLEAN return.
     * Rate-limited IRQISR separates real line interrupts from synthetic
     * post-send DPC kicks (empty-RX fork after host ARP replies in
     * output/netkvm-20261116T124245Z.pcap). */
    let hits = ISR_HITS.wrapping_add(1);
    ISR_HITS = hits;
    if hits <= 4 || (hits & 0x3f) == 0 {
        DbgPrint(b"NDIS IRQISR %u\n\0".as_ptr(), hits);
    }
    if queue_dpc != 0 || targets != 0 {
        KeInsertQueueDpc(context.add(IRQ_DPC_AT), core::ptr::null_mut(), core::ptr::null_mut());
    }
    claimed
}

unsafe extern "system" fn ndis_interrupt_dpc(
    _dpc: *mut u8,
    context: *mut u8,
    _arg1: *mut u8,
    _arg2: *mut u8,
) {
    if context.is_null() || core::ptr::read_unaligned(context as *const u32) != IRQ_KIND {
        return;
    }
    /* Before Restart, skip the miniport DPC.
     * output/netkvm-20261116T103245Z.log asserted APC_LEVEL mid-init. */
    if *context.add(IRQ_ARMED_AT) == 0 {
        return;
    }
    let dpc_ptr = core::ptr::read_unaligned(
        context.add(IRQ_DPC_HANDLER_AT) as *const *mut u8,
    );
    let miniport_ctx =
        core::ptr::read_unaligned(context.add(IRQ_CONTEXT_AT) as *const *mut u8);
    if dpc_ptr.is_null() {
        return;
    }
    /* Fresh throttle block each DPC. MaxNblsToIndicate =
     * NDIS_INDICATE_ALL_NBLS; MoreNblsPending cleared. */
    core::ptr::write_unaligned(
        context.add(IRQ_THROTTLE_MAX_AT) as *mut u32,
        INDICATE_ALL_NBLS,
    );
    core::ptr::write_unaligned(context.add(IRQ_THROTTLE_MORE_AT) as *mut u32, 0);
    let throttle = context.add(IRQ_THROTTLE_AT);
    let dpc_context = core::ptr::read_unaligned(
        context.add(IRQ_DPC_CONTEXT_AT) as *const *mut u8,
    );
    let syn = *context.add(IRQ_SYN_AT);
    *context.add(IRQ_SYN_AT) = 0;
    if syn == IRQ_SYN_SEND {
        DbgPrint(b"NDIS IRQDPC syn\n\0".as_ptr());
    } else if syn == IRQ_SYN_TIMER {
        let hits = RX_POLL_LOGS.wrapping_add(1);
        RX_POLL_LOGS = hits;
        if hits <= 4 || (hits & 0x3f) == 0 {
            DbgPrint(b"NDIS IRQDPC poll %u\n\0".as_ptr(), hits);
        }
    } else {
        DbgPrint(b"NDIS IRQDPC hw\n\0".as_ptr());
    }
    let dpc: MiniportDpcFn = core::mem::transmute(dpc_ptr);
    dpc(miniport_ctx, dpc_context, throttle, core::ptr::null_mut());
    /* Public NetKVM MiniportInterrupt disables the virtqueues; NDIS must
     * call EnableInterruptEx after the DPC returns
     * (https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_interrupt_dpc
     * https://github.com/virtio-win/kvm-guest-drivers-windows/blob/master/NetKVM/wlh/ParaNdis6_Impl.cpp).
     * output/netkvm-20261116T131159Z.log printed 0 IRQISR / 266 IRQDPC syn. */
    call_line_irq_handler(context, IRQ_ENABLE_AT);
    let more = core::ptr::read_unaligned(context.add(IRQ_THROTTLE_MORE_AT) as *const u32);
    if (more & 1) != 0 {
        KeInsertQueueDpc(context.add(IRQ_DPC_AT), core::ptr::null_mut(), core::ptr::null_mut());
    }
}

fn find_interrupt_resource(partial: *mut u8) -> Option<(u32, u8, u64, u16, u8)> {
    /* Returns Vector, Irql, Affinity, Flags, ShareDisposition. */
    if partial.is_null() {
        return None;
    }
    unsafe {
        let count = core::ptr::read_unaligned(partial.add(CM_RES_COUNT_AT) as *const u32);
        DbgPrint(b"NDIS IRQRES count %u\n\0".as_ptr(), count);
        let mut index = 0u32;
        while index < count && index < 16 {
            let desc = partial.add(CM_RES_DESC_AT + (index as usize) * CM_DESC_BYTES);
            let kind = *desc.add(CM_DESC_TYPE_AT);
            if kind == CM_TYPE_INTERRUPT {
                let share = *desc.add(CM_DESC_SHARE_AT);
                let flags =
                    core::ptr::read_unaligned(desc.add(CM_DESC_FLAGS_AT) as *const u16);
                let level =
                    core::ptr::read_unaligned(desc.add(CM_DESC_LEVEL_AT) as *const u32);
                let vector =
                    core::ptr::read_unaligned(desc.add(CM_DESC_VECTOR_AT) as *const u32);
                let affinity = core::ptr::read_unaligned(
                    desc.add(CM_DESC_AFFINITY_AT) as *const u64,
                );
                DbgPrint(
                    b"NDIS IRQRES v%u l%u a%p f%u s%u\n\0".as_ptr(),
                    vector,
                    level,
                    affinity as *mut u8,
                    flags as u32,
                    share as u32,
                );
                return Some((vector, level as u8, affinity, flags, share));
            }
            index += 1;
        }
    }
    None
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterInterruptEx(
    adapter: *mut u8,
    context: *mut u8,
    characteristics: *mut u8,
    interrupt_handle: *mut *mut u8,
) -> u32 {
    /* output/netkvm-20261116T071255Z.log: stub returned 0xC0000002 then
     * INIT c0000002.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterinterruptex
     * NDIS_STATUS_SUCCESS and NDIS_STATUS_RESOURCES are 0 and 0xC000009A
     * in sdk/include/ddk/ndis.h. Capture 2026-11-16T080419Z DIFF IRQA0050:
     * InterruptType dword at offset 0x50 became 1; MessageInfoTable at
     * 0x58 stayed null. */
    if interrupt_handle.is_null() {
        return 0xC000_000D;
    }
    unsafe {
        let mut isr: *mut u8 = core::ptr::null_mut();
        let mut dpc_handler: *mut u8 = core::ptr::null_mut();
        let mut disable: *mut u8 = core::ptr::null_mut();
        let mut enable: *mut u8 = core::ptr::null_mut();
        if !characteristics.is_null() {
            let kind = *characteristics;
            let revision = *characteristics.add(1);
            let size = u16::from_le_bytes([*characteristics.add(2), *characteristics.add(3)]);
            DbgPrint(
                b"NDIS IRQ %02x %02x %04x\n\0".as_ptr(),
                kind as u32,
                revision as u32,
                size as u32,
            );
            if (size as usize) >= IRQ_CHARS_DPC_AT + 8 {
                isr = core::ptr::read_unaligned(
                    characteristics.add(IRQ_CHARS_ISR_AT) as *const *mut u8,
                );
                dpc_handler = core::ptr::read_unaligned(
                    characteristics.add(IRQ_CHARS_DPC_AT) as *const *mut u8,
                );
            }
            if (size as usize) >= IRQ_CHARS_ENABLE_AT + 8 {
                disable = core::ptr::read_unaligned(
                    characteristics.add(IRQ_CHARS_DISABLE_AT) as *const *mut u8,
                );
                enable = core::ptr::read_unaligned(
                    characteristics.add(IRQ_CHARS_ENABLE_AT) as *const *mut u8,
                );
            }
            if size as usize >= 0x60 {
                let interrupt_type = 1u32.to_le_bytes();
                core::ptr::copy_nonoverlapping(
                    interrupt_type.as_ptr(),
                    characteristics.add(IRQ_CHARS_TYPE_AT),
                    4,
                );
                core::ptr::write_bytes(characteristics.add(IRQ_CHARS_MSG_AT), 0, 8);
            }
        }
        let block = ExAllocatePoolWithTag(0, IRQ_BYTES, POOL_TAG);
        if block.is_null() {
            *interrupt_handle = core::ptr::null_mut();
            return 0xC000_009A;
        }
        core::ptr::write_bytes(block, 0, IRQ_BYTES);
        core::ptr::write_unaligned(block as *mut u32, IRQ_KIND);
        core::ptr::write_unaligned(block.add(IRQ_CONTEXT_AT) as *mut *mut u8, context);
        core::ptr::write_unaligned(block.add(IRQ_ISR_AT) as *mut *mut u8, isr);
        core::ptr::write_unaligned(
            block.add(IRQ_DPC_HANDLER_AT) as *mut *mut u8,
            dpc_handler,
        );
        core::ptr::write_unaligned(block.add(IRQ_DISABLE_AT) as *mut *mut u8, disable);
        core::ptr::write_unaligned(block.add(IRQ_ENABLE_AT) as *mut *mut u8, enable);
        DbgPrint(
            b"NDIS IRQEN %p %p\n\0".as_ptr(),
            enable,
            disable,
        );
        KeInitializeDpc(
            block.add(IRQ_DPC_AT),
            ndis_interrupt_dpc as *const (),
            block,
        );
        KeInitializeDpc(
            block.add(IRQ_RX_TIMER_DPC_AT),
            ndis_rx_poll_timer_dpc as *const (),
            block,
        );
        KeInitializeTimer(block.add(IRQ_RX_TIMER_AT));

        core::ptr::write_unaligned(block.add(IRQ_ADAPTER_AT) as *mut *mut u8, adapter);
        core::ptr::write_unaligned(block.add(IRQ_OBJECT_AT) as *mut *mut u8, core::ptr::null_mut());
        /* Soft register during InitializeEx. Early connect+arm reached
         * IRQDPC (output/netkvm-20261116T115118Z.log) then faulted in
         * netkvm.sys after a second OpenAdapterEx. Defer connect to
         * after Restart; pass a real throttle block on every DPC. */
        *block.add(IRQ_ARMED_AT) = 0;
        if find_interrupt_resource(register::miniport_resources(adapter)).is_none() {
            DbgPrint(b"NDIS IRQRES none\n\0".as_ptr());
        }
        register::store_interrupt(adapter, block);
        DbgPrint(b"NDIS IRQ soft\n\0".as_ptr());
        *interrupt_handle = block;
        DbgPrint(b"NDIS IRQOK\n\0".as_ptr());
    }
    0
}

unsafe fn connect_interrupt_block(block: *mut u8, partial: *mut u8) {
    if block.is_null() {
        return;
    }
    let existing = core::ptr::read_unaligned(block.add(IRQ_OBJECT_AT) as *const *mut u8);
    if !existing.is_null() {
        return;
    }
    let Some((vector, irql, affinity, flags, share)) = find_interrupt_resource(partial) else {
        DbgPrint(b"NDIS IRQRES none\n\0".as_ptr());
        return;
    };
    let mode = if (flags & CM_INTERRUPT_LATCHED) != 0 {
        LATCHED
    } else {
        LEVEL_SENSITIVE
    };
    /* BOOLEAN ShareVector, not the raw ShareDisposition enum.
     * drivers/network/dd/rtl8139/ndis.c and
     * win32ss/drivers/videoprt/resource.c. */
    let share_vector: u8 = if share == CM_SHARE_SHARED { 1 } else { 0 };
    let mut object: *mut u8 = core::ptr::null_mut();
    let status = IoConnectInterrupt(
        &mut object,
        ndis_interrupt_service as *const (),
        block,
        core::ptr::null_mut(),
        vector,
        irql,
        irql,
        mode,
        share_vector,
        affinity,
        0,
    );
    DbgPrint(
        b"NDIS IRQCONN %08x %p\n\0".as_ptr(),
        status as u32,
        object,
    );
    if status >= 0 && !object.is_null() {
        core::ptr::write_unaligned(block.add(IRQ_OBJECT_AT) as *mut *mut u8, object);
        call_line_irq_handler(block, IRQ_ENABLE_AT);
        DbgPrint(b"NDIS IRQENABLE\n\0".as_ptr());
    }
}

unsafe extern "system" fn ndis_rx_poll_timer_dpc(
    _dpc: *mut u8,
    context: *mut u8,
    _arg1: *mut u8,
    _arg2: *mut u8,
) {
    /* output/netkvm-20261116T183657Z.log: ICMP replies on the wire and
     * a RECVIND only after the next TX. Poll queues the same miniport
     * DPC the SENDDONE kick uses. */
    if context.is_null() || core::ptr::read_unaligned(context as *const u32) != IRQ_KIND {
        return;
    }
    if *context.add(IRQ_ARMED_AT) == 0 {
        return;
    }
    *context.add(IRQ_SYN_AT) = IRQ_SYN_TIMER;
    KeInsertQueueDpc(
        context.add(IRQ_DPC_AT),
        core::ptr::null_mut(),
        core::ptr::null_mut(),
    );
}

unsafe fn start_rx_poll_timer(block: *mut u8) {
    if block.is_null() || core::ptr::read_unaligned(block as *const u32) != IRQ_KIND {
        return;
    }
    KeSetTimerEx(
        block.add(IRQ_RX_TIMER_AT),
        RX_POLL_DUE_100NS,
        RX_POLL_PERIOD_MS,
        block.add(IRQ_RX_TIMER_DPC_AT),
    );
    DbgPrint(b"NDIS RXPOLL on\n\0".as_ptr());
}

unsafe fn stop_rx_poll_timer(block: *mut u8) {
    if block.is_null() || core::ptr::read_unaligned(block as *const u32) != IRQ_KIND {
        return;
    }
    KeCancelTimer(block.add(IRQ_RX_TIMER_AT));
}

pub(crate) unsafe fn kick_interrupt_dpc(adapter: *mut u8) {
    let block = register::miniport_interrupt(adapter);
    if block.is_null() {
        return;
    }
    if core::ptr::read_unaligned(block as *const u32) != IRQ_KIND {
        return;
    }
    if *block.add(IRQ_ARMED_AT) == 0 {
        return;
    }
    *block.add(IRQ_SYN_AT) = IRQ_SYN_SEND;
    KeInsertQueueDpc(
        block.add(IRQ_DPC_AT),
        core::ptr::null_mut(),
        core::ptr::null_mut(),
    );
}

pub(crate) unsafe fn connect_interrupt(adapter: *mut u8) {
    /* After MiniportRestart: arm, connect, enable, then queue one DPC so
     * KickRX work from Restart is not lost when the ISR was not yet
     * connected (deferred path, output/netkvm-20261116T114639Z.log). */
    let block = register::miniport_interrupt(adapter);
    if block.is_null() {
        return;
    }
    unsafe {
        if core::ptr::read_unaligned(block as *const u32) != IRQ_KIND {
            return;
        }
        *block.add(IRQ_ARMED_AT) = 1;
        connect_interrupt_block(block, register::miniport_resources(adapter));
        call_line_irq_handler(block, IRQ_ENABLE_AT);
        kick_interrupt_dpc(adapter);
        start_rx_poll_timer(block);
        DbgPrint(b"NDIS IRQARM\n\0".as_ptr());
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterInterruptEx(interrupt: *mut u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismderegisterinterruptex */
    if interrupt.is_null() {
        return;
    }
    unsafe {
        if core::ptr::read_unaligned(interrupt as *const u32) == IRQ_KIND {
            *interrupt.add(IRQ_ARMED_AT) = 0;
            stop_rx_poll_timer(interrupt);
            let object = core::ptr::read_unaligned(
                interrupt.add(IRQ_OBJECT_AT) as *const *mut u8,
            );
            if !object.is_null() {
                IoDisconnectInterrupt(object);
            }
            let adapter = core::ptr::read_unaligned(
                interrupt.add(IRQ_ADAPTER_AT) as *const *mut u8,
            );
            if !adapter.is_null() && register::miniport_interrupt(adapter) == interrupt {
                register::clear_interrupt(adapter);
            }
        }
        DbgPrint(b"NDIS IRQFREE\n\0".as_ptr());
        ExFreePoolWithTag(interrupt, POOL_TAG);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMSynchronizeWithInterruptEx(
    interrupt: *mut u8,
    _message: u32,
    routine: *mut u8,
    context: *mut u8,
) -> u8 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsynchronizewithinterruptex
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kesynchronizeexecution */
    if interrupt.is_null() || routine.is_null() {
        return 0;
    }
    unsafe {
        if core::ptr::read_unaligned(interrupt as *const u32) != IRQ_KIND {
            return 0;
        }
        let object = core::ptr::read_unaligned(
            interrupt.add(IRQ_OBJECT_AT) as *const *mut u8,
        );
        if !object.is_null() {
            return KeSynchronizeExecution(object, routine as *const (), context);
        }
        let sync: SyncRoutineFn = core::mem::transmute(routine);
        sync(context)
    }
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
    Routine { name: "NdisCloseConfiguration", address: NdisCloseConfiguration as *const () },
    Routine { name: "NdisInitializeString", address: NdisInitializeString as *const () },
    Routine { name: "NdisOpenConfigurationKeyByName", address: NdisOpenConfigurationKeyByName as *const () },
    Routine { name: "NdisDeregisterDeviceEx", address: NdisDeregisterDeviceEx as *const () },
    Routine { name: "NdisMQueueDpcEx", address: NdisMQueueDpcEx as *const () },
    Routine { name: "NdisRegisterDeviceEx", address: NdisRegisterDeviceEx as *const () },
    Routine { name: "NdisGetDeviceReservedExtension", address: NdisGetDeviceReservedExtension as *const () },
    Routine { name: "NdisOpenAdapterEx", address: NdisOpenAdapterEx as *const () },
    Routine { name: "NdisCloseAdapterEx", address: NdisCloseAdapterEx as *const () },
    Routine { name: "NdisOidRequest", address: NdisOidRequest as *const () },
    Routine { name: "NdisMGetDeviceProperty", address: NdisMGetDeviceProperty as *const () },
    Routine { name: "NdisAllocateRWLock", address: NdisAllocateRWLock as *const () },
    Routine { name: "NdisFreeRWLock", address: NdisFreeRWLock as *const () },
    Routine { name: "NdisAcquireRWLockWrite", address: NdisAcquireRWLockWrite as *const () },
    Routine { name: "NdisAcquireRWLockRead", address: NdisAcquireRWLockRead as *const () },
    Routine { name: "NdisReleaseRWLock", address: NdisReleaseRWLock as *const () },
    Routine { name: "NdisGroupActiveProcessorCount", address: NdisGroupActiveProcessorCount as *const () },
    Routine { name: "NdisMAllocateSharedMemory", address: NdisMAllocateSharedMemory as *const () },
    Routine { name: "NdisMFreeSharedMemory", address: NdisMFreeSharedMemory as *const () },
    Routine { name: "NdisAllocateMdl", address: NdisAllocateMdl as *const () },
    Routine { name: "NdisFreeMdl", address: NdisFreeMdl as *const () },
    Routine { name: "NdisMRegisterIoPortRange", address: NdisMRegisterIoPortRange as *const () },
    Routine { name: "NdisMDeregisterIoPortRange", address: NdisMDeregisterIoPortRange as *const () },
    Routine { name: "NdisMSleep", address: NdisMSleep as *const () },
    Routine { name: "NdisMMapIoSpace", address: NdisMMapIoSpace as *const () },
    Routine { name: "NdisMUnmapIoSpace", address: NdisMUnmapIoSpace as *const () },
    Routine { name: "NdisProcessorNumberToIndex", address: NdisProcessorNumberToIndex as *const () },
    Routine { name: "NdisMSynchronizeWithInterruptEx", address: NdisMSynchronizeWithInterruptEx as *const () },
    Routine { name: "NdisMRegisterInterruptEx", address: NdisMRegisterInterruptEx as *const () },
    Routine { name: "NdisMDeregisterInterruptEx", address: NdisMDeregisterInterruptEx as *const () },
    Routine { name: "NdisMRegisterScatterGatherDma", address: NdisMRegisterScatterGatherDma as *const () },
    Routine { name: "NdisMDeregisterScatterGatherDma", address: NdisMDeregisterScatterGatherDma as *const () },
    Routine { name: "NdisMAllocateNetBufferSGList", address: NdisMAllocateNetBufferSGList as *const () },
    Routine { name: "NdisMFreeNetBufferSGList", address: NdisMFreeNetBufferSGList as *const () },
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
    NdisGetDataBuffer,
    NdisFOidRequest,
    NdisFreeIoWorkItem,
    NdisQueueIoWorkItem,
    NdisMRemoveMiniport,
    NdisFreeCloneNetBufferList,
);
