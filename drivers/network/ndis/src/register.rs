/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/ndis/src/register.rs
 * PURPOSE:     Miniport registration and completion calls
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * The characteristics header is NDIS_OBJECT_HEADER in
 * sdk/include/psdk/ntddndis.h. output/ndis-discovery/windows.log
 * (DIFF END OK) accepts a miniport of type 0x8A, revision 3, size
 * 0x120, calls the pointer at offset 0x10, and returns 0xC0010004 for
 * a zeroed block. NdisGetVersion in that log is 0x00060054. The bytes
 * 6 and 84 at offset 4, and the move to 0xC0010005, are recorded in
 * differential-tests/ndis-discovery/METHOD.md from capture
 * 2026-11-14T184820Z. Those codes are NDIS_STATUS_BAD_VERSION and
 * NDIS_STATUS_BAD_CHARACTERISTICS in sdk/include/ddk/ndis.h. The public
 * register page calls MiniportSetOptions from inside this function and
 * copies the characteristics. The slot the probe planted takes four
 * pointers, in differential-tests/ndis-discovery/probe.c. That count is
 * the probe method, not a Windows field.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismregisterminiportdriver
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismindicatereceivenetbufferlists
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsendnetbufferlistscomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismoidrequestcomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismrestartcomplete
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismindicatestatusex
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismpausecomplete
 * Capture 2026-11-15T143730Z reached DIFF END OK: type 0x9E, size 4,
 * returns 0 on the initialize handle. Other types at size 4 returned
 * 0xC000000D. Capture 2026-11-15T161901Z reached DIFF END OK: indicate
 * delivers the list to offset 0x50, argument 1. Argument 0 and
 * argument 2 were null. differential-tests/ndis-discovery/METHOD.md
 * records that same capture: slot 0x18 ran and its argument 0 was the
 * handle passed to NdisMSetMiniportAttributes.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_initialize_ex
 * names that handler's three parameters. The 256-byte block passed as
 * the third parameter is this file's buffer so the pointer is not null.
 * It is not a Windows structure size.
 * output/netkvm-20261116T023655Z.log printed NDIS REG 8a 03 00a0 and
 * DriverEntry returned 0xC0010004. Capture 2026-11-16T025328Z called
 * the same type, revision, and size with bytes 6 and 84 and reached
 * DIFF END OK with status 0xC0010005. output/netkvm-20261116T025725Z.log
 * printed NDIS VER 06 53 for that registration. Capture
 * 2026-11-16T030735Z called that header with a zeroed body and reached
 * DIFF END OK with status 0xC0010005. output/netkvm-20261116T031154Z.log
 * printed that block: bytes 68 dc follow 06 53, eight flag bytes are
 * zero, and the slots from 0x10 through 0x90 are this boot's addresses.
 * output/ndis-discovery/windows.log (DIFF END OK) in capture
 * 2026-11-16T032247Z accepted that size with bytes 06 53, returned 0,
 * and called offset 0x10. A zeroed body at that size returned
 * 0xC0010005. This file accepts the 06 53 size and calls offset 0x10.
 * output/netkvm-20261116T032525Z.log then called NdisSetOptionalHandlers
 * and DriverEntry returned 0xC0000002. This file logs that block's
 * header and returns 0. output/netkvm-20261116T032858Z.log printed
 * NDIS OPT 92 01 002c. This file prints that block and still returns 0.
 * output/netkvm-20261116T041014Z.log printed the 0x2C bytes. Offset 4
 * is 80 f8 ff ff, offset 0x28 is a0 00 00 00, and the other slots are
 * this boot's addresses. DriverEntry returned 0xC0010004.
 * output/netkvm-20261116T042252Z.log printed no NDIS MISS line and
 * DriverEntry returned 0xC0010004. This file prints the miniport handle
 * after offset 0x10 returns, and again from NdisDeregisterDeviceEx.
 * output/netkvm-20261116T043051Z.log printed NDIS SLOT 00000000 and
 * wrote the registration at byte 0 of that handle. The row at offset
 * 0x120 ends in 4d 50 73 6f 67 00 5d 00. NDIS HEND same followed.
 * DriverEntry returned 0xC0010004. Passing the characteristics pointer
 * instead left that image unchanged in output/netkvm-20261116T043614Z.log
 * and DriverEntry still returned 0xC0010004. output/netkvm-20261116T043930Z.log
 * then printed the optional-handlers tail as 20 b4 d8 72. The slot
 * receives the driver allocation again. Capture 2026-11-16T050701Z
 * returned 0 from NdisRegisterDeviceEx for the measured names. This
 * file keeps the DRIVER_OBJECT pointer past the characteristics copy.
 * Capture 2026-11-16T052229Z dumped the Windows miniport handle: bytes
 * 02 01 a8 04 at offset 0, 06 53 at offset 0x18, 84 00 86 00 at offset
 * 0x30, and the characteristics header at offset 0x70. BEFORE and AFTER
 * NdisSetOptionalHandlers matched. Capture 2026-11-16T053108Z dumped
 * the full 0x4A8 bytes: zeros from 0x110 through 0x16F, 06 00 at
 * offset 0x172, dword 1 at 0x190, and dc 00 68 00 24 00 24 00 at
 * 0x1E0. Guest pointers are not replayed. This file's KIND, context,
 * and DRIVER_OBJECT sit in that zero run at 0x110. The optional-handlers
 * copy sits at 0x128 in that same zero run. Capture 2026-11-16T045900Z
 * returned 0 from NdisSetOptionalHandlers for type 0x92, revision 1,
 * size 0x2C. output/netkvm-20261116T054453Z.log reached DriverEntry 0
 * with no MiniportInitializeEx. This file stores that block and sets
 * DRIVER_OBJECT.DriverExtension.AddDevice from
 * sdk/include/xdk/iotypes.h so PnP can call MiniportAddDeviceHandler.
 * output/netkvm-20261116T055653Z.log printed NDIS ADDMP 00000000, then
 * IopInitiatePnpIrp(IRP_MN_FILTER_RESOURCE_REQUIREMENTS) failed. This
 * file's PnP dispatch calls the FilterResourceRequirements and
 * StartDevice slots from the stored optional-handlers block.
 * output/netkvm-20261116T060635Z.log printed NDIS FILTER 00000000,
 * NDIS START 00000000, NDIS INIT, NDIS RWLOCK, then NDIS INIT c001001e
 * (NDIS_STATUS_RESOURCE_CONFLICT in sdk/include/ddk/ndis.h) after
 * NdisMGetBusData returned 0. This file stores the physical, functional,
 * and next-lower device objects on the adapter handle for
 * NdisMGetDeviceProperty and NdisMGetBusData.
 * output/netkvm-20261116T061555Z.log printed NDIS BUSGOT 00000040 then
 * faulted in netkvm.sys at address 4. StartDevice now forwards to the
 * PDO with IoForwardIrpSynchronously before the miniport start slot.
 * Public NetKVM CPciResources::Init
 * (https://github.com/virtio-win/kvm-guest-drivers-windows/blob/master/NetKVM/Common/ParaNdis_VirtIO.cpp)
 * reads NdisMGetBusData then RList->Count. Count is at offset 4 of
 * CM_PARTIAL_RESOURCE_LIST in sdk/include/xdk/cmtypes.h. A null RList
 * faults at address 4. Public NetKVM MiniportInitializeEx passes
 * miniportInitParameters->AllocatedResources into that Init
 * (https://github.com/virtio-win/kvm-guest-drivers-windows/blob/master/NetKVM/wlh/ParaNdis6_Driver.cpp).
 * The third InitializeEx argument was this file's zeroed INIT_BYTES
 * block, so that pointer was null. The public init-parameters page lists
 * Header, then Flags, then AllocatedResources
 * (https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_init_parameters).
 * NDIS_OBJECT_HEADER is 4 bytes in sdk/include/psdk/ntddndis.h. Flags is
 * a ULONG. On x64 that puts AllocatedResources at offset 8. That offset
 * is this derivation, not a Windows dump of that structure.
 * NDIS_RESOURCE_LIST is CM_PARTIAL_RESOURCE_LIST in
 * sdk/include/ddk/ndis.h. The StartDevice IRP carries CM_RESOURCE_LIST;
 * PartialResourceList sits after Count, InterfaceType, and BusNumber in
 * sdk/include/xdk/cmtypes.h.
 * After MiniportInitializeEx returns success the adapter is paused.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_restart
 * NDIS then calls MiniportRestart. output/netkvm-20261116T083539Z.log
 * printed NDIS INIT 00000000 and no Restart. The Restart handler is
 * four pointer slots after SetOptionsHandler at 0x10 in
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_driver_characteristics
 * so the slot is 0x38. output/netkvm-20261116T083539Z.log printed that
 * pointer in NDIS BODY 0030. MiniportRestart takes
 * MiniportAdapterContext from
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_adapter_registration_attributes
 * which is the pointer at offset 8 of the type 0x9E size 0x1C block in
 * that same log. The restart-parameters page lists Header, then
 * RestartAttributes, then Flags
 * (https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_restart_parameters).
 * On x64 RestartAttributes is at offset 8. A null RestartAttributes is
 * the case that page says the miniport must not modify. The 0x20-byte
 * zeroed block is this file's buffer, not a Windows size.
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndissetoptionalhandlers
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_pnp_characteristics
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_add_device
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nc-wdm-driver_add_device
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismgetdeviceproperty
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismgetbusdata
 * output/netkvm-20261116T142828Z.log: OIDFILTER 0000000b then
 * OIDFILTER 00000000, then a page fault in netkvm.sys. NDIS_PACKET_TYPE_*
 * bits are in sdk/include/ddk/ndis.h. Public ParaNdis_DeviceFiltersUpdateRxMode
 * (https://github.com/virtio-win/kvm-guest-drivers-windows/blob/master/NetKVM/Common/ParaNdis_Common.cpp)
 * enables CTRL_RX NOUNI when DIRECTED is clear.
 */

use core::ptr;

const POOL_TAG: u32 = u32::from_le_bytes(*b"NsoV");
/* output/ndis-discovery/windows.log: type 0x8A, revision 3, size 0x120. */
const MP_TYPE: u8 = 0x8A;
const MP_REVISION: u8 = 3;
const MP_CHARS: usize = 0x120;
/* Capture 2026-11-16T032247Z: type 0x8A, revision 3, size 0xA0, bytes 06 53. */
const MP_CHARS_A0: u16 = 0xA0;
const VERSION_MINOR_A0: u8 = 0x53;
/* Same log: bytes 6 and 84 at offset 4. */
const VERSION_AT: usize = 4;
const VERSION_MAJOR: u8 = 6;
const VERSION_MINOR: u8 = 84;
/* Same log: DIFF CALL at offset 0x10. The probe slot takes four pointers. */
const CALLED_SLOT: usize = 0x10;
/* NDIS_STATUS_BAD_VERSION and NDIS_STATUS_BAD_CHARACTERISTICS in sdk/include/ddk/ndis.h. */
const BAD_VERSION: u32 = 0xC001_0004;
const BAD_CHARACTERISTICS: u32 = 0xC001_0005;
/* Capture 2026-11-15T143730Z: type 0x9E. Other types at size 4 returned this. */
const ATTR_TYPE: u8 = 0x9E;
/* Capture 2026-11-16T055625Z: type 0xA4, revision 1, size 0x14. */
const ATTR_ADD_TYPE: u8 = 0xA4;
const ATTR_ADD_REVISION: u8 = 1;
const ATTR_ADD_SIZE: u16 = 0x14;
/* output/netkvm-20261116T063334Z.log printed NDIS HDR 9f 02 00e0 from
 * NetKVM during MiniportInitializeEx, then NDIS INIT c000000d. This
 * file returned INVALID_PARAMETER for every attributes type other than
 * the measured 0x9E and 0xA4 headers. That status matched. The type,
 * revision, and size are the bytes NetKVM passed on that VerOS boot,
 * not a Windows accept. Capture 2026-11-16T064151Z dumped the 0xE0 body.
 * An earlier Windows try of type 0x9F revision 0 size 4 did not return.
 * A later Windows try of type 0x9F revision 2 size 0xE0 after an
 * accepted 0x9E size 0x1C also did not return. Do not call either
 * 0x9F shape again. */
const ATTR_GENERAL_TYPE: u8 = 0x9F;
const ATTR_GENERAL_REVISION: u8 = 2;
const ATTR_GENERAL_SIZE: u16 = 0xE0;
const INVALID_PARAMETER: u32 = 0xC000_000D;
/* Capture 2026-11-15T161901Z: ReturnNetBufferListsHandler / indicate
 * delivery offset on the miniport characteristics. */
const RETURN_SLOT: usize = 0x50;
/* METHOD.md capture 2026-11-15T143730Z: initialize is slot 0x18. */
const INIT_SLOT: usize = 0x18;
/* SetOptions at 0x10; Restart is four pointers later: 0x38. */
const RESTART_SLOT: usize = 0x38;
/* OidRequestHandler is the next pointer after Restart on
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_driver_characteristics
 * Eight-byte pointers from 0x10: Init 0x18, Halt 0x20, Unload 0x28,
 * Pause 0x30, Restart 0x38, OidRequest 0x40. Restart at 0x38 is measured
 * (output/netkvm-20261116T085039Z.log). OidRequest at 0x40 is that
 * public member order from Restart, not a Windows dump of the slot. */
const OID_SLOT: usize = 0x40;
/* SendNetBufferListsHandler is the next pointer after OidRequest on the
 * same public characteristics page. Eight-byte pointers: Oid 0x40, Send
 * 0x48. */
const SEND_SLOT: usize = 0x48;
/* This file's buffer for the third initialize parameter, not a Windows size. */
const INIT_BYTES: usize = 256;
/* This file's buffer for MiniportRestart parameters, not a Windows size. */
const RESTART_BYTES: usize = 0x20;
/* Type 0x9E size 0x1C: MiniportAdapterContext pointer at offset 8. */
const ATTR_CONTEXT_AT: usize = 8;
const ATTR_REG_SIZE: u16 = 0x1C;
/* NDIS_STATUS_FAILURE and NDIS_STATUS_RESOURCES in sdk/include/ddk/ndis.h. */
const FAILURE: u32 = 0xC000_0001;
const RESOURCES: u32 = 0xC000_009A;
const KIND_MP: u32 = u32::from_le_bytes(*b"MPso");
/* Capture 2026-11-16T052229Z: type 0x02, revision 1, size 0x4A8. */
const HANDLE_TYPE: u8 = 0x02;
const HANDLE_REVISION: u8 = 1;
const HANDLE_BYTES: usize = 0x4A8;
const HANDLE_VERSION_AT: usize = 0x18;
const HANDLE_MARK_AT: usize = 0x30;
const HANDLE_CHARS_AT: usize = 0x70;
/* Capture 2026-11-16T053108Z: zeros from 0x110 through 0x16F. */
const HANDLE_KIND_AT: usize = 0x110;
const HANDLE_CONTEXT_AT: usize = 0x118;
const HANDLE_DRIVER_AT: usize = 0x120;
/* Same capture: 06 00 at 0x172, and dc 00 68 00 24 00 24 00 at 0x1E0. */
const HANDLE_FLAG_AT: usize = 0x172;
const HANDLE_TAIL_AT: usize = 0x1E0;
const HANDLE_ONE_AT: usize = 0x190;
/* Zero run after KIND/context/driver: store the measured 0x2C OPT block. */
const HANDLE_OPT_AT: usize = 0x128;
/* Same zero run after the 0x2C OPT copy: PDO, FDO, and next-lower. */
const HANDLE_PDO_AT: usize = 0x158;
const HANDLE_FDO_AT: usize = 0x160;
const HANDLE_NEXT_AT: usize = 0x168;
/* After the measured 06 00 at 0x172, before dword 1 at 0x190. */
const HANDLE_RES_AT: usize = 0x178;
/* Free pointer slot between resources and the measured dword at 0x190:
 * soft-registered interrupt block from NdisMRegisterInterruptEx. */
const HANDLE_IRQ_AT: usize = 0x180;
/* Zero run after dword 1 at 0x190: MiniportAdapterContext from 0x9E. */
const HANDLE_ADAPTER_CTX_AT: usize = 0x198;
/* Free dword after the adapter context pointer, before the measured
 * tail at 0x1E0 (capture 2026-11-16T053108Z zero run). Stores the last
 * non-zero OID_GEN_CURRENT_PACKET_FILTER applied to this adapter. */
const HANDLE_PACKET_FILTER_AT: usize = 0x1A0;
/* IO_STACK_LOCATION.Parameters starts after the 4 UCHAR header fields on
 * x64 with pointer alignment, in sdk/include/xdk/iotypes.h. StartDevice
 * stores AllocatedResources then AllocatedResourcesTranslated. */
const STACK_PARAMETERS_AT: usize = 8;
const STACK_START_RES_AT: usize = STACK_PARAMETERS_AT;
const STACK_START_REST_AT: usize = STACK_PARAMETERS_AT + 8;
/* CM_RESOURCE_LIST.List[0].PartialResourceList in sdk/include/xdk/cmtypes.h. */
const CM_PARTIAL_AT: usize = 12;
/* AllocatedResources at offset 8: see file header. Public member order
 * after NDIS_OBJECT_HEADER and Flags; not a Windows-measured layout. */
const INIT_RESOURCES_AT: usize = 8;
/* Capture 2026-11-16T045900Z: type 0x92, revision 1, size 0x2C. */
const OPT_TYPE: u8 = 0x92;
const OPT_REVISION: u8 = 1;
const OPT_BYTES: usize = 0x2C;
/* Public member order after the 4-byte header: first pointer at 0x08. */
const OPT_ADD_DEVICE_AT: usize = 0x08;
const OPT_REMOVE_DEVICE_AT: usize = 0x10;
const OPT_FILTER_AT: usize = 0x18;
const OPT_START_DEVICE_AT: usize = 0x20;
/* DRIVER_OBJECT.DriverExtension and DRIVER_EXTENSION.AddDevice in
 * sdk/include/xdk/iotypes.h on x64. */
const DRIVER_EXTENSION_AT: usize = 0x30;
const ADD_DEVICE_AT: usize = 0x08;
/* DRIVER_OBJECT.MajorFunction in sdk/include/xdk/iotypes.h on x64. */
const MAJOR_FUNCTION_AT: usize = 0x70;
/* IRP_MJ_PNP, IRP_MN_START_DEVICE, IRP_MN_REMOVE_DEVICE, and
 * IRP_MN_FILTER_RESOURCE_REQUIREMENTS in sdk/include/xdk/iotypes.h. */
const IRP_MJ_PNP: usize = 0x1b;
const IRP_MN_START_DEVICE: u8 = 0x00;
const IRP_MN_REMOVE_DEVICE: u8 = 0x02;
const IRP_MN_FILTER_RESOURCE_REQUIREMENTS: u8 = 0x0d;
/* IRP.IoStatus, CurrentLocation, and Tail.Overlay.CurrentStackLocation
 * from sdk/include/xdk/iotypes.h on x64. */
const IRP_IO_STATUS_AT: usize = 0x30;
const IRP_CURRENT_LOCATION_AT: usize = 0x43;
const IRP_STACK_AT: usize = 0xb8;
/* modules/rostests/kmtests/kmtest_drv/structs.c: sizeof(IO_STACK_LOCATION) == 72 on x64. */
const STACK_LOCATION_BYTES: usize = 72;
/* DEVICE_OBJECT.DeviceExtension in sdk/include/xdk/iotypes.h on x64. */
const DEVICE_EXTENSION_AT: usize = 0x40;
const FDO_EXT_BYTES: u32 = 16;
const FDO_ADAPTER_AT: usize = 0;
const FDO_LOWER_AT: usize = 8;
/* FILE_DEVICE_PHYSICAL_NETCARD in sdk/include/xdk/iotypes.h. */
const FILE_DEVICE_PHYSICAL_NETCARD: u32 = 0x17;
const IO_NO_INCREMENT: i32 = 0;
/* Same capture: first 0xA0 bytes were dumped. */
const SLOT_IMAGE: usize = 0xA0;
static mut SAVED_HANDLE: *mut u8 = ptr::null_mut();
static mut HANDLE_SNAP: [u8; SLOT_IMAGE] = [0; SLOT_IMAGE];
static mut MP_DRIVER_HANDLE: *mut u8 = ptr::null_mut();

unsafe extern "system" {
    fn ExAllocatePoolWithTag(kind: u32, bytes: usize, tag: u32) -> *mut u8;
    fn ExFreePoolWithTag(address: *mut u8, tag: u32);
    fn DbgPrint(format: *const u8, ...) -> i32;
    fn IoCreateDevice(
        driver: *mut u8,
        extension: u32,
        name: *const u8,
        device_type: u32,
        characteristics: u32,
        exclusive: u8,
        device: *mut *mut u8,
    ) -> i32;
    fn IoAttachDeviceToDeviceStack(source: *mut u8, target: *mut u8) -> *mut u8;
    fn IoDeleteDevice(device: *mut u8);
    fn IoDetachDevice(device: *mut u8);
    fn IoCompleteRequest(irp: *mut u8, boost: i32);
    fn IoCallDriver(device: *mut u8, irp: *mut u8) -> i32;
    fn IoForwardIrpSynchronously(device: *mut u8, irp: *mut u8) -> u8;
}

fn log_bytes(text: &[u8], kind: u8, revision: u8, size: u16) {
    unsafe { DbgPrint(text.as_ptr(), kind as u32, revision as u32, size as u32) };
}

fn log_version(chars: *const u8, size: u16) {
    if (size as usize) < VERSION_AT + 2 {
        return;
    }
    let major = unsafe { *chars.add(VERSION_AT) };
    let minor = unsafe { *chars.add(VERSION_AT + 1) };
    unsafe { DbgPrint(b"NDIS VER %02x %02x\n\0".as_ptr(), major as u32, minor as u32) };
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
        for byte in b"NDIS BODY " {
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

fn log_opt(chars: *const u8, size: u16) {
    let total = size as usize;
    let mut offset = 0usize;
    while offset < total {
        let mut line = [0u8; 80];
        let mut at = 0usize;
        for byte in b"NDIS OPTB " {
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

fn header(chars: *const u8) -> (u8, u8, u16) {
    unsafe {
        let size = u16::from_le_bytes([*chars.add(2), *chars.add(3)]);
        (*chars, *chars.add(1), size)
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

fn netkvm_chars(kind: u8, revision: u8, size: u16, chars: *const u8) -> bool {
    kind == MP_TYPE
        && revision == MP_REVISION
        && size == MP_CHARS_A0
        && version_byte(chars, size, VERSION_AT) == Some(VERSION_MAJOR)
        && version_byte(chars, size, VERSION_AT + 1) == Some(VERSION_MINOR_A0)
}

fn planted_byte(saved: *const u8, copy_len: usize, major: u8, minor: u8, index: usize) -> u8 {
    /* Capture 2026-11-16T052229Z layout for the first 0xA0 bytes. */
    if index == 0 {
        return HANDLE_TYPE;
    }
    if index == 1 {
        return HANDLE_REVISION;
    }
    if index == 2 {
        return (HANDLE_BYTES & 0xff) as u8;
    }
    if index == 3 {
        return ((HANDLE_BYTES >> 8) & 0xff) as u8;
    }
    if index == HANDLE_VERSION_AT {
        return major;
    }
    if index == HANDLE_VERSION_AT + 1 {
        return minor;
    }
    if index == HANDLE_MARK_AT {
        return 0x84;
    }
    if index == HANDLE_MARK_AT + 2 {
        return 0x86;
    }
    if index >= HANDLE_CHARS_AT && index - HANDLE_CHARS_AT < copy_len {
        return unsafe { *saved.add(index - HANDLE_CHARS_AT) };
    }
    0
}

fn write_u16(handle: *mut u8, offset: usize, value: u16) {
    unsafe {
        *handle.add(offset) = value as u8;
        *handle.add(offset + 1) = (value >> 8) as u8;
    }
}

fn write_ptr(handle: *mut u8, offset: usize, value: *mut u8) {
    unsafe {
        ptr::write_unaligned(handle.add(offset) as *mut *mut u8, value);
    }
}

fn read_ptr(handle: *mut u8, offset: usize) -> *mut u8 {
    unsafe { ptr::read_unaligned(handle.add(offset) as *const *mut u8) }
}

fn chars_ptr(handle: *mut u8) -> *mut u8 {
    unsafe { handle.add(HANDLE_CHARS_AT) }
}

fn is_mp_handle(handle: *mut u8) -> bool {
    if handle.is_null() {
        return false;
    }
    unsafe { ptr::read_unaligned(handle.add(HANDLE_KIND_AT) as *const u32) == KIND_MP }
}

pub fn miniport_devices(handle: *mut u8) -> (*mut u8, *mut u8, *mut u8) {
    if !is_mp_handle(handle) {
        return (ptr::null_mut(), ptr::null_mut(), ptr::null_mut());
    }
    (
        read_ptr(handle, HANDLE_PDO_AT),
        read_ptr(handle, HANDLE_FDO_AT),
        read_ptr(handle, HANDLE_NEXT_AT),
    )
}

pub fn miniport_resources(handle: *mut u8) -> *mut u8 {
    /* CM_PARTIAL_RESOURCE_LIST pointer stored in store_start_resources. */
    if !is_mp_handle(handle) {
        return ptr::null_mut();
    }
    read_ptr(handle, HANDLE_RES_AT)
}

pub fn store_interrupt(handle: *mut u8, interrupt: *mut u8) {
    if !is_mp_handle(handle) {
        return;
    }
    write_ptr(handle, HANDLE_IRQ_AT, interrupt);
}

pub fn miniport_interrupt(handle: *mut u8) -> *mut u8 {
    if !is_mp_handle(handle) {
        return ptr::null_mut();
    }
    read_ptr(handle, HANDLE_IRQ_AT)
}

pub fn clear_interrupt(handle: *mut u8) {
    store_interrupt(handle, ptr::null_mut());
}

fn log_rows(prefix: &[u8; 10], base: *const u8, total: usize, changed: &mut bool, expect: &mut dyn FnMut(usize) -> u8) {
    let mut offset = 0usize;
    while offset < total {
        let count = (total - offset).min(16);
        let mut row = false;
        let mut index = 0usize;
        while index < count {
            let actual = unsafe { *base.add(offset + index) };
            if actual != expect(offset + index) {
                row = true;
                *changed = true;
            }
            index += 1;
        }
        if row {
            let mut line = [0u8; 80];
            let mut at = 0usize;
            let mut prefix_at = 0usize;
            while prefix_at < prefix.len() {
                line[at] = prefix[prefix_at];
                at += 1;
                prefix_at += 1;
            }
            push_hex(&mut line, &mut at, (offset >> 8) as u8);
            push_hex(&mut line, &mut at, offset as u8);
            index = 0;
            while index < count {
                line[at] = b' ';
                at += 1;
                push_hex(&mut line, &mut at, unsafe { *base.add(offset + index) });
                index += 1;
            }
            line[at] = b'\n';
            line[at + 1] = 0;
            unsafe { DbgPrint(line.as_ptr()) };
        }
        offset += 16;
    }
}

fn log_handle(handle: *mut u8, saved: *const u8, copy_len: usize, major: u8, minor: u8) {
    let mut changed = false;
    log_rows(
        b"NDIS HNDL ",
        handle,
        SLOT_IMAGE,
        &mut changed,
        &mut |index| planted_byte(saved, copy_len, major, minor, index),
    );
    if !changed {
        unsafe { DbgPrint(b"NDIS HANDLE same\n\0".as_ptr()) };
    }
    unsafe {
        SAVED_HANDLE = handle;
        ptr::copy_nonoverlapping(handle, core::ptr::addr_of_mut!(HANDLE_SNAP) as *mut u8, SLOT_IMAGE);
    }
}

pub(crate) unsafe fn log_handle_later() {
    let driver = unsafe { SAVED_HANDLE };
    if driver.is_null() {
        return;
    }
    let mut changed = false;
    log_rows(
        b"NDIS HEND ",
        driver,
        SLOT_IMAGE,
        &mut changed,
        &mut |index| unsafe { *core::ptr::addr_of!(HANDLE_SNAP).cast::<u8>().add(index) },
    );
    if !changed {
        unsafe { DbgPrint(b"NDIS HEND same\n\0".as_ptr()) };
    }
}

fn call_slot(chars: *const u8, handle: *mut u8, context: *mut u8) -> u32 {
    let slot = unsafe { ptr::read_unaligned(chars.add(CALLED_SLOT) as *const *mut u8) };
    if slot.is_null() {
        return 0;
    }
    let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    unsafe { function(handle, context, ptr::null_mut(), ptr::null_mut()) }
}

pub(crate) unsafe fn mp_driver_object(handle: *mut u8) -> *mut u8 {
    if !is_mp_handle(handle) {
        return ptr::null_mut();
    }
    read_ptr(handle, HANDLE_DRIVER_AT)
}

fn set_add_device(driver_object: *mut u8) {
    if driver_object.is_null() {
        return;
    }
    let extension = read_ptr(driver_object, DRIVER_EXTENSION_AT);
    if extension.is_null() {
        return;
    }
    write_ptr(
        extension,
        ADD_DEVICE_AT,
        ndis_add_device as *mut u8,
    );
    write_ptr(
        driver_object,
        MAJOR_FUNCTION_AT + IRP_MJ_PNP * size_of::<*mut u8>(),
        ndis_pnp as *mut u8,
    );
}

fn irp_stack(irp: *mut u8) -> *mut u8 {
    read_ptr(irp, IRP_STACK_AT)
}

fn irp_minor(irp: *mut u8) -> u8 {
    let stack = irp_stack(irp);
    if stack.is_null() {
        return 0xff;
    }
    unsafe { *stack.add(1) }
}

fn complete_irp(irp: *mut u8, status: i32) -> i32 {
    unsafe {
        ptr::write_unaligned(irp.add(IRP_IO_STATUS_AT) as *mut i32, status);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    status
}

fn skip_stack(irp: *mut u8) {
    unsafe {
        *irp.add(IRP_CURRENT_LOCATION_AT) =
            (*irp.add(IRP_CURRENT_LOCATION_AT)).wrapping_add(1);
    }
    let stack = read_ptr(irp, IRP_STACK_AT);
    if !stack.is_null() {
        write_ptr(irp, IRP_STACK_AT, unsafe { stack.add(STACK_LOCATION_BYTES) });
    }
}

fn opt_slot(adapter: *mut u8, offset: usize) -> *mut u8 {
    if !is_mp_handle(adapter) {
        return ptr::null_mut();
    }
    read_ptr(unsafe { adapter.add(HANDLE_OPT_AT) }, offset)
}

fn call_opt_irp(adapter: *mut u8, offset: usize, irp: *mut u8) -> u32 {
    let slot = opt_slot(adapter, offset);
    if slot.is_null() {
        return 0;
    }
    let function: unsafe extern "system" fn(*mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    unsafe { function(adapter, irp) }
}

unsafe extern "system" fn ndis_pnp(device: *mut u8, irp: *mut u8) -> i32 {
    /* output/netkvm-20261116T055653Z.log: FILTER_RESOURCE_REQUIREMENTS
     * failed after AddDevice. IRP_MN_* in sdk/include/xdk/iotypes.h.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_pnp_characteristics */
    if device.is_null() || irp.is_null() {
        return FAILURE as i32;
    }
    let extension = read_ptr(device, DEVICE_EXTENSION_AT);
    let adapter = if extension.is_null() {
        ptr::null_mut()
    } else {
        read_ptr(extension, FDO_ADAPTER_AT)
    };
    let lower = if extension.is_null() {
        ptr::null_mut()
    } else {
        read_ptr(extension, FDO_LOWER_AT)
    };
    let minor = irp_minor(irp);
    unsafe { DbgPrint(b"NDIS PNP %02x\n\0".as_ptr(), minor as u32) };
    match minor {
        IRP_MN_FILTER_RESOURCE_REQUIREMENTS => {
            let status = call_opt_irp(adapter, OPT_FILTER_AT, irp);
            unsafe { DbgPrint(b"NDIS FILTER %08x\n\0".as_ptr(), status) };
            complete_irp(irp, status as i32)
        }
        IRP_MN_START_DEVICE => {
            /* Pass StartDevice to the PDO first so resources are on the
             * IRP before MiniportStartDevice and MiniportInitializeEx.
             * sdk/include/xdk/iofuncs.h IoForwardIrpSynchronously.
             * output/netkvm-20261116T061555Z.log faulted in netkvm.sys
             * after NdisMGetBusData returned 0x40. */
            let mut status = 0u32;
            if !lower.is_null() {
                if IoForwardIrpSynchronously(lower, irp) == 0 {
                    unsafe { DbgPrint(b"NDIS STARTFWD fail\n\0".as_ptr()) };
                    return complete_irp(irp, FAILURE as i32);
                }
                status = unsafe { ptr::read_unaligned(irp.add(IRP_IO_STATUS_AT) as *const i32) as u32 };
                unsafe { DbgPrint(b"NDIS STARTPDO %08x\n\0".as_ptr(), status) };
            }
            if status == 0 {
                store_start_resources(adapter, irp);
                status = call_opt_irp(adapter, OPT_START_DEVICE_AT, irp);
                unsafe { DbgPrint(b"NDIS START %08x\n\0".as_ptr(), status) };
                if status == 0 && is_mp_handle(adapter) {
                    call_initialize(adapter);
                }
            }
            complete_irp(irp, status as i32)
        }
        IRP_MN_REMOVE_DEVICE => {
            let _ = call_opt_irp(adapter, OPT_REMOVE_DEVICE_AT, irp);
            if !lower.is_null() {
                IoDetachDevice(device);
            }
            if is_mp_handle(adapter) {
                ptr::write_unaligned(adapter.add(HANDLE_KIND_AT) as *mut u32, 0);
                ExFreePoolWithTag(adapter, POOL_TAG);
            }
            IoDeleteDevice(device);
            complete_irp(irp, 0)
        }
        _ => {
            if lower.is_null() {
                complete_irp(irp, FAILURE as i32)
            } else {
                skip_stack(irp);
                IoCallDriver(lower, irp)
            }
        }
    }
}

unsafe extern "system" fn ndis_add_device(driver_object: *mut u8, physical: *mut u8) -> i32 {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nc-wdm-driver_add_device
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_add_device */
    unsafe { DbgPrint(b"NDIS ADDDEV\n\0".as_ptr()) };
    let driver_handle = unsafe { MP_DRIVER_HANDLE };
    if !is_mp_handle(driver_handle) || physical.is_null() {
        return FAILURE as i32;
    }
    if read_ptr(driver_handle, HANDLE_DRIVER_AT) != driver_object {
        return FAILURE as i32;
    }
    let add = read_ptr(driver_handle.add(HANDLE_OPT_AT), OPT_ADD_DEVICE_AT);
    let context = read_ptr(driver_handle, HANDLE_CONTEXT_AT);
    let adapter = ExAllocatePoolWithTag(0, HANDLE_BYTES, POOL_TAG);
    if adapter.is_null() {
        return RESOURCES as i32;
    }
    ptr::copy_nonoverlapping(driver_handle, adapter, HANDLE_BYTES);
    let mut device: *mut u8 = ptr::null_mut();
    let create = IoCreateDevice(
        driver_object,
        FDO_EXT_BYTES,
        ptr::null(),
        FILE_DEVICE_PHYSICAL_NETCARD,
        0,
        0,
        &mut device,
    );
    if create < 0 || device.is_null() {
        unsafe { DbgPrint(b"NDIS ADDFAIL %08x\n\0".as_ptr(), create as u32) };
        ExFreePoolWithTag(adapter, POOL_TAG);
        return create;
    }
    let attached = IoAttachDeviceToDeviceStack(device, physical);
    if attached.is_null() {
        unsafe { DbgPrint(b"NDIS ADDATTACH\n\0".as_ptr()) };
        IoDeleteDevice(device);
        ExFreePoolWithTag(adapter, POOL_TAG);
        return FAILURE as i32;
    }
    let extension = read_ptr(device, DEVICE_EXTENSION_AT);
    if !extension.is_null() {
        write_ptr(extension, FDO_ADAPTER_AT, adapter);
        write_ptr(extension, FDO_LOWER_AT, attached);
    }
    write_ptr(adapter, HANDLE_PDO_AT, physical);
    write_ptr(adapter, HANDLE_FDO_AT, device);
    write_ptr(adapter, HANDLE_NEXT_AT, attached);
    let status = if add.is_null() {
        0
    } else {
        let function: unsafe extern "system" fn(*mut u8, *mut u8) -> u32 =
            core::mem::transmute(add);
        let status = function(adapter, context);
        unsafe { DbgPrint(b"NDIS ADDMP %08x\n\0".as_ptr(), status) };
        status
    };
    if status != 0 {
        IoDetachDevice(device);
        IoDeleteDevice(device);
        ExFreePoolWithTag(adapter, POOL_TAG);
        return status as i32;
    }
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMRegisterMiniportDriver(
    driver_object: *mut u8,
    _registry: *mut u8,
    context: *mut u8,
    chars: *const u8,
    out_handle: *mut *mut u8,
) -> u32 {
    if chars.is_null() || out_handle.is_null() {
        return FAILURE;
    }
    let (kind, revision, size) = header(chars);
    log_bytes(b"NDIS REG %02x %02x %04x\n\0", kind, revision, size);
    log_version(chars, size);
    let netkvm = netkvm_chars(kind, revision, size, chars);
    if netkvm {
        log_body(chars, size);
    }
    if !version_ok(chars, size) && !netkvm {
        return BAD_VERSION;
    }
    if !netkvm && (kind != MP_TYPE || revision != MP_REVISION || size as usize != MP_CHARS) {
        return BAD_CHARACTERISTICS;
    }
    let copy_len = if netkvm { MP_CHARS_A0 as usize } else { MP_CHARS };
    let major = unsafe { *chars.add(VERSION_AT) };
    let minor = unsafe { *chars.add(VERSION_AT + 1) };
    let handle = ExAllocatePoolWithTag(0, HANDLE_BYTES, POOL_TAG);
    if handle.is_null() {
        return RESOURCES;
    }
    ptr::write_bytes(handle, 0, HANDLE_BYTES);
    *handle = HANDLE_TYPE;
    *handle.add(1) = HANDLE_REVISION;
    write_u16(handle, 2, HANDLE_BYTES as u16);
    *handle.add(HANDLE_VERSION_AT) = major;
    *handle.add(HANDLE_VERSION_AT + 1) = minor;
    /* Capture 2026-11-16T052229Z: 84 00 86 00 at offset 0x30. */
    *handle.add(HANDLE_MARK_AT) = 0x84;
    *handle.add(HANDLE_MARK_AT + 2) = 0x86;
    ptr::copy_nonoverlapping(chars, chars_ptr(handle), copy_len);
    ptr::write_unaligned(handle.add(HANDLE_KIND_AT) as *mut u32, KIND_MP);
    write_ptr(handle, HANDLE_CONTEXT_AT, context);
    write_ptr(handle, HANDLE_DRIVER_AT, driver_object);
    write_u16(handle, HANDLE_FLAG_AT, 6);
    /* Capture 2026-11-16T053108Z: dword 1 at offset 0x190. */
    ptr::write_unaligned(handle.add(HANDLE_ONE_AT) as *mut u32, 1);
    *handle.add(HANDLE_TAIL_AT) = 0xdc;
    *handle.add(HANDLE_TAIL_AT + 1) = 0x00;
    *handle.add(HANDLE_TAIL_AT + 2) = 0x68;
    *handle.add(HANDLE_TAIL_AT + 3) = 0x00;
    *handle.add(HANDLE_TAIL_AT + 4) = 0x24;
    *handle.add(HANDLE_TAIL_AT + 5) = 0x00;
    *handle.add(HANDLE_TAIL_AT + 6) = 0x24;
    *handle.add(HANDLE_TAIL_AT + 7) = 0x00;
    let mut saved = [0u8; MP_CHARS];
    ptr::copy_nonoverlapping(chars, saved.as_mut_ptr(), copy_len);
    let status = call_slot(chars, handle, context);
    if netkvm {
        unsafe { DbgPrint(b"NDIS SLOT %08x\n\0".as_ptr(), status) };
        log_handle(handle, saved.as_ptr(), copy_len, major, minor);
    }
    if status != 0 {
        ExFreePoolWithTag(handle, POOL_TAG);
        return status;
    }
    set_add_device(driver_object);
    unsafe { MP_DRIVER_HANDLE = handle };
    *out_handle = handle;
    if !netkvm {
        call_initialize(handle);
    }
    0
}

fn store_start_resources(adapter: *mut u8, irp: *mut u8) {
    if !is_mp_handle(adapter) {
        return;
    }
    let stack = irp_stack(irp);
    if stack.is_null() {
        return;
    }
    let cm_list = read_ptr(stack, STACK_START_REST_AT);
    let cm_raw = read_ptr(stack, STACK_START_RES_AT);
    let chosen = if !cm_list.is_null() { cm_list } else { cm_raw };
    if chosen.is_null() {
        unsafe { DbgPrint(b"NDIS STARTRES null\n\0".as_ptr()) };
        return;
    }
    let partial = unsafe { chosen.add(CM_PARTIAL_AT) };
    write_ptr(adapter, HANDLE_RES_AT, partial);
    unsafe { DbgPrint(b"NDIS STARTRES %p\n\0".as_ptr(), partial) };
}

fn call_initialize(handle: *mut u8) {
    let chars_len = chars_len(handle);
    let slot = unsafe { slot_at(chars_ptr(handle), chars_len, INIT_SLOT) };
    if slot.is_null() {
        return;
    }
    let params = unsafe { ExAllocatePoolWithTag(0, INIT_BYTES, POOL_TAG) };
    if params.is_null() {
        return;
    }
    unsafe { ptr::write_bytes(params, 0, INIT_BYTES) };
    let resources = read_ptr(handle, HANDLE_RES_AT);
    write_ptr(params, INIT_RESOURCES_AT, resources);
    let context = read_ptr(handle, HANDLE_CONTEXT_AT);
    let function: unsafe extern "system" fn(*mut u8, *mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    unsafe { DbgPrint(b"NDIS INIT\n\0".as_ptr()) };
    let status = unsafe { function(handle, context, params) };
    unsafe { DbgPrint(b"NDIS INIT %08x\n\0".as_ptr(), status) };
    unsafe { ExFreePoolWithTag(params, POOL_TAG) };
    if status == 0 {
        call_restart(handle);
    }
}

fn chars_len(handle: *mut u8) -> usize {
    let chars = chars_ptr(handle);
    if chars.is_null() {
        return MP_CHARS;
    }
    let size = u16::from_le_bytes(unsafe { [*chars.add(2), *chars.add(3)] }) as usize;
    if size >= RESTART_SLOT + size_of::<*mut u8>() && size <= MP_CHARS {
        size
    } else {
        MP_CHARS
    }
}

fn call_restart(handle: *mut u8) {
    let chars_len = chars_len(handle);
    let slot = unsafe { slot_at(chars_ptr(handle), chars_len, RESTART_SLOT) };
    if slot.is_null() {
        unsafe { DbgPrint(b"NDIS RESTART null\n\0".as_ptr()) };
        return;
    }
    let adapter_ctx = read_ptr(handle, HANDLE_ADAPTER_CTX_AT);
    if adapter_ctx.is_null() {
        unsafe { DbgPrint(b"NDIS RESTART ctx\n\0".as_ptr()) };
        return;
    }
    let params = unsafe { ExAllocatePoolWithTag(0, RESTART_BYTES, POOL_TAG) };
    if params.is_null() {
        return;
    }
    /* RestartAttributes at offset 8 stays null. */
    unsafe { ptr::write_bytes(params, 0, RESTART_BYTES) };
    let function: unsafe extern "system" fn(*mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    unsafe { DbgPrint(b"NDIS RESTART\n\0".as_ptr()) };
    let status = unsafe { function(adapter_ctx, params) };
    unsafe { DbgPrint(b"NDIS RESTART %08x\n\0".as_ptr(), status) };
    unsafe { ExFreePoolWithTag(params, POOL_TAG) };
    if status == 0 {
        /* Connect the soft-registered interrupt before bind. Connecting
         * during InitializeEx ran the miniport DPC mid-init and asserted
         * KeGetCurrentIrql() <= APC_LEVEL (output/netkvm-20261116T103245Z.log,
         * output/netkvm-20261116T103516Z.log). */
        unsafe { crate::connect_interrupt(handle) };
        crate::protocol::notify_adapter_ready(handle);
    }
}

#[no_mangle]
pub unsafe extern "system" fn NdisSetOptionalHandlers(handle: *mut u8, handlers: *const u8) -> u32 {
    /* A null argument is NDIS_STATUS_INVALID_PARAMETER on the public page.
     * Capture 2026-11-16T045900Z returned 0 for type 0x92 size 0x2C.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_pnp_characteristics */
    if handle.is_null() || handlers.is_null() {
        return INVALID_PARAMETER;
    }
    let (kind, revision, size) = header(handlers);
    log_bytes(b"NDIS OPT %02x %02x %04x\n\0", kind, revision, size);
    if size == OPT_BYTES as u16 {
        log_opt(handlers, size);
    }
    if is_mp_handle(handle)
        && kind == OPT_TYPE
        && revision == OPT_REVISION
        && size as usize == OPT_BYTES
    {
        ptr::copy_nonoverlapping(handlers, handle.add(HANDLE_OPT_AT), OPT_BYTES);
    }
    0
}

#[no_mangle]
pub unsafe extern "system" fn NdisMDeregisterMiniportDriver(handle: *mut u8) {
    if !is_mp_handle(handle) {
        return;
    }
    if unsafe { MP_DRIVER_HANDLE } == handle {
        unsafe { MP_DRIVER_HANDLE = ptr::null_mut() };
    }
    ptr::write_unaligned(handle.add(HANDLE_KIND_AT) as *mut u32, 0);
    ExFreePoolWithTag(handle, POOL_TAG);
}

fn slot_at(chars: *const u8, chars_len: usize, offset: usize) -> *mut u8 {
    if offset + size_of::<*mut u8>() > chars_len {
        return ptr::null_mut();
    }
    unsafe { ptr::read_unaligned(chars.add(offset) as *const *mut u8) }
}

#[no_mangle]
pub unsafe extern "system" fn NdisMSetMiniportAttributes(
    handle: *mut u8,
    attributes: *const u8,
) -> u32 {
    /* Capture 2026-11-15T143730Z returned 0 for type 0x9E, size 4.
     * Capture 2026-11-15T150727Z returned 0 for the same type at the
     * probe's larger size. Other types at size 4 returned 0xC000000D.
     * output/netkvm-20261116T055159Z.log printed NDIS HDR a4 01 0014
     * from MiniportAddDevice. Capture 2026-11-16T055625Z returned 0 for
     * that header. output/netkvm-20261116T063334Z.log printed
     * NDIS HDR 9f 02 00e0 then NDIS INIT c000000d. Accepting that VerOS
     * triple let InitializeEx continue. A Windows call with that 0x9F
     * buffer after an accepted 0x9E size 0x1C did not return; do not
     * call it again. Capture 2026-11-16T082709Z reached DIFF END OK:
     * type 0x9E revision 1 size 0x1C with AttributeFlags 0x165 from
     * output/netkvm-20261116T081944Z.log was ACCEPTED, then
     * NdisMRegisterScatterGatherDma returned 0xC000009A.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsetminiportattributes
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_add_device
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_miniport_adapter_general_attributes */
    if handle.is_null() || attributes.is_null() {
        return INVALID_PARAMETER;
    }
    let (kind, revision, size) = header(attributes);
    log_bytes(b"NDIS HDR %02x %02x %04x\n\0", kind, revision, size);
    /* Capture 2026-11-16T064151Z dumped the 0x9F body. Dump every
     * attributes block so the 0x9E size 0x1C AttributeFlags are on the
     * serial log for the Windows SG probe. */
    log_body(attributes, size);
    if kind == ATTR_TYPE {
        /* Size 4 was the probe accept; size 0x1C carries the context. */
        if size >= ATTR_REG_SIZE && is_mp_handle(handle) {
            let adapter_ctx = read_ptr(attributes as *mut u8, ATTR_CONTEXT_AT);
            write_ptr(handle, HANDLE_ADAPTER_CTX_AT, adapter_ctx);
            unsafe {
                DbgPrint(b"NDIS CTX %p\n\0".as_ptr(), adapter_ctx);
            };
        }
        return 0;
    }
    if kind == ATTR_ADD_TYPE && revision == ATTR_ADD_REVISION && size == ATTR_ADD_SIZE {
        return 0;
    }
    if kind == ATTR_GENERAL_TYPE && revision == ATTR_GENERAL_REVISION && size == ATTR_GENERAL_SIZE {
        return 0;
    }
    INVALID_PARAMETER
}

#[no_mangle]
pub unsafe extern "system" fn NdisMIndicateReceiveNetBufferLists(
    adapter: *mut u8,
    lists: *mut u8,
    port: u32,
    count: u32,
    flags: u32,
) {
    /* Deliver to the NDIS 6 protocol binding first. Capture
     * 2026-11-15T161901Z measured miniport return at 0x50 for a probe
     * indicate; live traffic uses the protocol receive path instead. */
    unsafe {
        DbgPrint(b"NDIS RECVIND %p %u\n\0".as_ptr(), adapter, count);
    }
    if !is_mp_handle(adapter) {
        unsafe { DbgPrint(b"NDIS RECVIND bad\n\0".as_ptr()) };
        return;
    }
    crate::prot6::indicate_receive(adapter, lists, port, count, flags);
}

pub fn call_return_net_buffer_lists(adapter: *mut u8, lists: *mut u8, flags: u32) {
    if !is_mp_handle(adapter) || lists.is_null() {
        return;
    }
    let chars_len = chars_len(adapter);
    let slot = unsafe { slot_at(chars_ptr(adapter), chars_len, RETURN_SLOT) };
    if slot.is_null() {
        return;
    }
    let adapter_ctx = read_ptr(adapter, HANDLE_ADAPTER_CTX_AT);
    let function: unsafe extern "system" fn(*mut u8, *mut u8, u32) =
        unsafe { core::mem::transmute(slot) };
    unsafe { function(adapter_ctx, lists, flags) };
}

pub fn call_send_net_buffer_lists(
    adapter: *mut u8,
    lists: *mut u8,
    port: u32,
    flags: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_send_net_buffer_lists */
    if !is_mp_handle(adapter) || lists.is_null() {
        return;
    }
    let chars_len = chars_len(adapter);
    let slot = unsafe { slot_at(chars_ptr(adapter), chars_len, SEND_SLOT) };
    if slot.is_null() {
        unsafe { DbgPrint(b"NDIS SEND null\n\0".as_ptr()) };
        return;
    }
    let adapter_ctx = read_ptr(adapter, HANDLE_ADAPTER_CTX_AT);
    if adapter_ctx.is_null() {
        unsafe { DbgPrint(b"NDIS SEND ctx\n\0".as_ptr()) };
        return;
    }
    unsafe { DbgPrint(b"NDIS SEND\n\0".as_ptr()) };
    let function: unsafe extern "system" fn(*mut u8, *mut u8, u32, u32) =
        unsafe { core::mem::transmute(slot) };
    unsafe { function(adapter_ctx, lists, port, flags) };
}

#[no_mangle]
pub extern "system" fn NdisMSendNetBufferListsComplete(
    adapter: *mut u8,
    lists: *mut u8,
    flags: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismsendnetbufferlistscomplete
     * names a Status member on each NET_BUFFER_LIST. That offset is not
     * measured yet. Scan the first NBL for known completion codes from
     * sdk/include/ddk/ndis.h / sdk/include/psdk/ntstatus.h so a NetKVM
     * media-disconnect complete
     * (https://github.com/virtio-win/kvm-guest-drivers-windows/blob/master/NetKVM/wlh/ParaNdis6_Impl.cpp
     * ParaNdis_ExactSendFailureStatus) can be told apart from success
     * without inventing a Status offset. The 256-byte window matches the
     * NET_BUFFER probe read in differential-tests/ndis-discovery/probe.c;
     * it is not a Windows object size. Print each distinct hit once. */
    unsafe {
        DbgPrint(
            b"NDIS SENDDONE %p %p %08x\n\0".as_ptr(),
            adapter,
            lists,
            flags,
        );
        scan_send_complete_status(lists);
    };
    crate::prot6::complete_send(adapter, lists, flags);
    /* Deferred IoConnectInterrupt accepts but the line stays quiet for
     * RX (output/netkvm-20261116T115651Z.log: two synthetic IRQDPC then
     * many SENDDONE with no further IRQDPC). Kick the miniport DPC after
     * each send complete so ARP/ICMP replies can be indicated. */
    unsafe { crate::kick_interrupt_dpc(adapter) };
}

/* STATUS_NDIS_MEDIA_DISCONNECTED in sdk/include/psdk/ntstatus.h;
 * NDIS_STATUS_MEDIA_DISCONNECTED in sdk/include/ddk/ndis.h. */
const SEND_STAT_MEDIA_DISC: u32 = 0xC023_001F;
/* STATUS_UNSUCCESSFUL / NDIS_STATUS_FAILURE in sdk/include/ddk/ndis.h. */
const SEND_STAT_FAILURE: u32 = 0xC000_0001;
/* NDIS_STATUS_RESOURCES in sdk/include/ddk/ndis.h. */
const SEND_STAT_RESOURCES: u32 = 0xC000_009A;
/* STATUS_NDIS_PAUSED in sdk/include/psdk/ntstatus.h. */
const SEND_STAT_PAUSED: u32 = 0xC023_002A;
const SEND_STAT_SCAN: usize = 256;

static mut SEND_STAT_SEEN: u32 = 0;

unsafe fn scan_send_complete_status(lists: *mut u8) {
    if lists.is_null() {
        return;
    }
    let codes: [u32; 4] = [
        SEND_STAT_MEDIA_DISC,
        SEND_STAT_FAILURE,
        SEND_STAT_RESOURCES,
        SEND_STAT_PAUSED,
    ];
    let mut off = 0usize;
    while off + 4 <= SEND_STAT_SCAN {
        let value = core::ptr::read_unaligned(lists.add(off) as *const u32);
        let mut i = 0usize;
        while i < codes.len() {
            if value == codes[i] {
                let bit = 1u32 << (i as u32);
                if (SEND_STAT_SEEN & bit) == 0 {
                    SEND_STAT_SEEN |= bit;
                    DbgPrint(
                        b"NDIS SENDSTAT %08x @%02x\n\0".as_ptr(),
                        value,
                        off as u32,
                    );
                }
                return;
            }
            i += 1;
        }
        off += 4;
    }
    if (SEND_STAT_SEEN & 0x8000_0000) == 0 {
        SEND_STAT_SEEN |= 0x8000_0000;
        DbgPrint(b"NDIS SENDSTAT none\n\0".as_ptr());
    }
}

pub fn call_oid_request(adapter: *mut u8, request: *mut u8) -> u32 {
    /* Forward to MiniportOidRequest. The request buffer stays opaque
     * until a guest dump names its body offsets.
     * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-miniport_oid_request
     * output/netkvm-20261116T091655Z.log printed NDIS STUB NdisOidRequest
     * three times after NDIS OPENEXOK on the NetKVM bind path. */
    if !is_mp_handle(adapter) || request.is_null() {
        return INVALID_PARAMETER;
    }
    let chars_len = chars_len(adapter);
    let slot = unsafe { slot_at(chars_ptr(adapter), chars_len, OID_SLOT) };
    if slot.is_null() {
        unsafe { DbgPrint(b"NDIS OID null\n\0".as_ptr()) };
        return 0xC000_00BB; /* STATUS_NOT_SUPPORTED in sdk/include/psdk/ntstatus.h */
    }
    let adapter_ctx = read_ptr(adapter, HANDLE_ADAPTER_CTX_AT);
    if adapter_ctx.is_null() {
        unsafe { DbgPrint(b"NDIS OID ctx\n\0".as_ptr()) };
        return INVALID_PARAMETER;
    }
    let kind = unsafe { *request };
    let revision = unsafe { *request.add(1) };
    let size = u16::from_le_bytes(unsafe { [*request.add(2), *request.add(3)] });
    /* output/netkvm-20261116T092548Z.log NDIS OIDBODY: RequestType dword
     * at offset 4 (0 = NdisRequestQueryInformation, 1 =
     * NdisRequestSetInformation in sdk/include/ddk/ndis.h); Oid dword at
     * offset 0x20 (0x0001010E = OID_GEN_CURRENT_PACKET_FILTER in
     * sdk/include/psdk/ntddndis.h). */
    let request_type = if (size as usize) >= 8 {
        u32::from_le_bytes(unsafe {
            [
                *request.add(4),
                *request.add(5),
                *request.add(6),
                *request.add(7),
            ]
        })
    } else {
        0xffff_ffff
    };
    let oid = if (size as usize) >= 0x24 {
        u32::from_le_bytes(unsafe {
            [
                *request.add(0x20),
                *request.add(0x21),
                *request.add(0x22),
                *request.add(0x23),
            ]
        })
    } else {
        0
    };
    unsafe {
        DbgPrint(
            b"NDIS OID %02x %02x %04x t%u %08x\n\0".as_ptr(),
            kind as u32,
            revision as u32,
            size as u32,
            request_type,
            oid,
        )
    };
    /* SET OID_GEN_CURRENT_PACKET_FILTER: InformationBuffer at 0x28 and
     * length at 0x30 are output/netkvm-20261116T092548Z.log NDIS OIDBODY
     * (lan.c OID_REQUEST_BUFFER_AT). QMP on output/netkvm-20261116T141444Z
     * showed RX shadow-avail-idx 256 with used-idx 0 — host has RX
     * buffers but never fills; CTRL_RX NOUNI drops unicast when
     * DIRECTED is clear (public ParaNdis_DeviceFiltersUpdateRxMode).
     * output/netkvm-20261116T142828Z.log: tcpip set 0x0B
     * (NDIS_PACKET_TYPE_DIRECTED|MULTICAST|BROADCAST in
     * sdk/include/ddk/ndis.h), then a second open set 0 and netkvm.sys
     * page-faulted. Keep the live filter when a later SET clears
     * DIRECTED after it was already applied. */
    if request_type == 1 && oid == 0x0001_010E && (size as usize) >= 0x34 {
        let buffer = read_ptr(request, 0x28);
        let length = u32::from_le_bytes(unsafe {
            [
                *request.add(0x30),
                *request.add(0x31),
                *request.add(0x32),
                *request.add(0x33),
            ]
        });
        let filter = if !buffer.is_null() && length >= 4 {
            u32::from_le_bytes(unsafe {
                [*buffer, *buffer.add(1), *buffer.add(2), *buffer.add(3)]
            })
        } else {
            0xffff_ffff
        };
        unsafe {
            DbgPrint(
                b"NDIS OIDFILTER %08x len %08x\n\0".as_ptr(),
                filter,
                length,
            )
        };
        /* NDIS_PACKET_TYPE_DIRECTED in sdk/include/ddk/ndis.h. */
        const DIRECTED: u32 = 0x0000_0001;
        let prior = unsafe {
            ptr::read_unaligned(adapter.add(HANDLE_PACKET_FILTER_AT) as *const u32)
        };
        if filter == 0 && (prior & DIRECTED) != 0 {
            unsafe { DbgPrint(b"NDIS OIDFILTER keep\n\0".as_ptr()) };
            return 0;
        }
        if filter != 0xffff_ffff {
            unsafe {
                ptr::write_unaligned(
                    adapter.add(HANDLE_PACKET_FILTER_AT) as *mut u32,
                    filter,
                )
            };
        }
    }
    let function: unsafe extern "system" fn(*mut u8, *mut u8) -> u32 =
        unsafe { core::mem::transmute(slot) };
    let status = unsafe { function(adapter_ctx, request) };
    unsafe { DbgPrint(b"NDIS OID %08x\n\0".as_ptr(), status) };
    status
}

#[no_mangle]
pub extern "system" fn NdisMOidRequestComplete(
    adapter: *mut u8,
    request: *mut u8,
    status: u32,
) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismoidrequestcomplete */
    unsafe { DbgPrint(b"NDIS OIDDONE %08x\n\0".as_ptr(), status) };
    crate::prot6::complete_oid(adapter, request, status);
}

#[no_mangle]
pub extern "system" fn NdisMRestartComplete(_adapter: *mut u8, _status: u32) {}

#[no_mangle]
pub extern "system" fn NdisMIndicateStatusEx(_adapter: *mut u8, indication: *const u8) {
    /* https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndismindicatestatusex
     * Log the indication header so the next Windows call has the bytes. */
    if indication.is_null() {
        unsafe { DbgPrint(b"NDIS STAT null\n\0".as_ptr()) };
        return;
    }
    let kind = unsafe { *indication };
    let revision = unsafe { *indication.add(1) };
    let size = u16::from_le_bytes(unsafe { [*indication.add(2), *indication.add(3)] });
    unsafe {
        DbgPrint(
            b"NDIS STAT %02x %02x %04x\n\0".as_ptr(),
            kind as u32,
            revision as u32,
            size as u32,
        )
    };
}

#[no_mangle]
pub extern "system" fn NdisMPauseComplete(_adapter: *mut u8) {}
