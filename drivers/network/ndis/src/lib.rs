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
    /* output/ndis-discovery/windows.log: DIFF NdisGetVersion 0x00060054. */
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

static MINIPORT: &[Routine] = &[
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
    NdisFOidRequest
);
