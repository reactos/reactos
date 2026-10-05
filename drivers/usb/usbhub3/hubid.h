/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP identity, ACPI port description and connector map internals
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Enumeration failure reasons the ids are chosen by (Child->m_EnumMessageId) */
#define HUB_ENUM_DEVICE_DESCRIPTOR_FAILED   0x40010000
#define HUB_ENUM_SET_ADDRESS_FAILED         0x40010001
#define HUB_ENUM_PORT_RESET_FAILED          0x40010002
#define HUB_ENUM_BAD_CONFIG_DESCRIPTOR      0x40010004
#define HUB_ENUM_BAD_DEVICE_DESCRIPTOR      0x40010005
#define HUB_ENUM_CONFIG_DESCRIPTOR_FAILED   0x40010007
#define HUB_ENUM_SET_SEL_FAILED             0x4001000A
#define HUB_ENUM_BOS_FAILED                 0x4001000B
#define HUB_ENUM_QUALIFIER_FAILED           0x4001000C
#define HUB_ENUM_SERIAL_NUMBER_FAILED       0x4001000D
#define HUB_ENUM_LANGUAGE_IDS_FAILED        0x4001000E
#define HUB_ENUM_PRODUCT_STRING_FAILED      0x4001000F
#define HUB_ENUM_MSOS_EXT_CONFIG_FAILED     0x40010010
#define HUB_ENUM_MSOS_CONTAINER_ID_FAILED   0x40010011
#define HUB_ENUM_BAD_BOS                    0x40010012
#define HUB_ENUM_BAD_QUALIFIER              0x40010013
#define HUB_ENUM_BAD_LANGUAGE_IDS           0x40010014
#define HUB_ENUM_BAD_MSOS_CONTAINER_ID      0x40010015
#define HUB_ENUM_BAD_MSOS_EXT_CONFIG        0x40010016
#define HUB_ENUM_BAD_PRODUCT_STRING         0x40010017
#define HUB_ENUM_BAD_SERIAL_NUMBER          0x40010018
#define HUB_ENUM_LINK_SS_INACTIVE           0x40010019
#define HUB_ENUM_LINK_COMPLIANCE            0x4001001A
#define HUB_ENUM_INCOMPATIBLE               0x4001001B
#define HUB_ENUM_MSOS20_SET_FAILED          0x4001001C
#define HUB_ENUM_BAD_MSOS20_SET             0x4001001D
#define HUB_ENUM_ALT_ENUM_FAILED            0x4001001E
#define HUB_ENUM_BAD_BILLBOARD_URL          0x4001001F
#define HUB_ENUM_BAD_ALT_MODE_STRING        0x40010020

/* Serial number decoration, in bytes */
#define HUB_SERIAL_PREFIX_BYTES             (6 * sizeof(WCHAR))

/** FIPS 180-4 SHA-1, enough for hashing container id names. */
struct HubSha1
{
    ULONG State[5];
    ULONG64 TotalBytes;
    ULONG BlockUsed;
    UCHAR Block[64];
};

#define HUB_SHA1_DIGEST_BYTES               20

VOID
NTAPI
HubSha1Init(
    _Out_ HubSha1* Context);

VOID
NTAPI
HubSha1Update(
    _Inout_ HubSha1* Context,
    _In_reads_bytes_(Length) const VOID* Data,
    _In_ SIZE_T Length);

VOID
NTAPI
HubSha1Final(
    _Inout_ HubSha1* Context,
    _Out_writes_bytes_all_(HUB_SHA1_DIGEST_BYTES) PUCHAR Digest);

/* Port properties change after creation only here, so the updates are interlocked */
FORCEINLINE
VOID
NTAPI
HubSetPortProperty(
    _In_ HubPort* Port,
    _In_ PortProperty Property)
{
    InterlockedOr((volatile LONG*)&Port->m_Info.Properties, (LONG)Property);
}

FORCEINLINE
VOID
NTAPI
HubClearPortProperty(
    _In_ HubPort* Port,
    _In_ PortProperty Property)
{
    InterlockedAnd((volatile LONG*)&Port->m_Info.Properties, ~(LONG)Property);
}

/* Connector map, portconn.cpp */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubConnectorRegisterPort(
    _In_ HubPort* Port);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubConnectorUnregisterPort(
    _In_ HubPort* Port);
