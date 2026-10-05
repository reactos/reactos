/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Registry helpers shared by the registry and MS OS descriptor code
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Enumeration failure text for a usbflags read error */
#define HUB_MSG_REGISTRY_FAILURE    0x40011006

/** VID, PID and revision as 4 upper case hex digits each, the usbflags key name parts. */
struct HubIdStrings
{
    CHAR Vendor[MAX_VENDOR_ID_STRING_LENGTH];
    CHAR Product[MAX_DEVICE_ID_STRING_LENGTH];
    CHAR Revision[MAX_REVISION_ID_STRING_LENGTH];
};

VOID
NTAPI
HubFormatIdStrings(
    _In_ const USB_DEVICE_DESCRIPTOR* Descriptor,
    _Out_ HubIdStrings* Ids);

/** Reads up to Size bytes of a value into a zeroed buffer; the type is not checked. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubRegReadValue(
    _In_ WDFKEY Key,
    _In_ PCWSTR Name,
    _In_ ULONG Size,
    _Out_writes_bytes_(Size) PVOID Data);

/** Writes a REG_BINARY value into the device's usbflags subkey. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubWriteUsbflagsValue(
    _In_ HubChild* Child,
    _In_ PCWSTR Name,
    _In_reads_bytes_(Size) PVOID Data,
    _In_ ULONG Size);

/** Sets or clears ExtPropertiesInstalled from ExtPropDescSemaphore in the hardware key. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubRefreshExtPropertiesInstalled(
    _In_ HubChild* Child);
