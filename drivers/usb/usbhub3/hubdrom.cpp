/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USB4 router DROM read hooks of the hub machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Vendor requests that reach the router mailbox registers over the hub's default pipe */
#define HUB_MAILBOX_READ_TYPE           0xC0
#define HUB_MAILBOX_WRITE_TYPE          0x40
#define HUB_MAILBOX_READ_REQUEST        0x40
#define HUB_MAILBOX_WRITE_REQUEST       0x41
#define HUB_MAILBOX_SPACE               0x0400

/* Hub hardware verifier bit that stops in the debugger on a failed control transfer */
#define HUB_VERIFIER_CONTROL_FAILURE    0x00000002

/* USB4 DROM layout the identity lookup uses */
#define DROM_VERSION_OFFSET             13
#define DROM_LENGTH_OFFSET              14
#define DROM_LENGTH_MASK                0x0FFF
#define DROM_FIRST_ENTRY                22
#define DROM_ENTRY_HEADER               2
#define DROM_ENTRY_ADAPTER              0x80
#define DROM_ENTRY_TYPE_MASK            0x3F
#define DROM_ENTRY_TYPE_IDENTITY        9

/* Field offsets inside the identity entry, and the length that holds all of them */
#define DROM_IDENTITY_VENDOR            4
#define DROM_IDENTITY_PRODUCT           6
#define DROM_IDENTITY_BCD               8
#define DROM_IDENTITY_REVISION          14
#define DROM_IDENTITY_LENGTH            15

static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubMailboxComplete;

static
VOID
NTAPI
HubMailboxComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    NTSTATUS Status = Params->IoStatus.Status;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    Hub->m_Control.Reuse();

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p mailbox register %u access failed 0x%lx, URB status 0x%lx\n",
                Hub,
                Hub->m_Control.Urb.SetupPacket[4],
                Status,
                Hub->m_Control.Urb.Hdr.Status);

        if (Hub->m_VerifierFlags & HUB_VERIFIER_CONTROL_FAILURE)
        {
            DPRINT1("USB hardware verifier break: hub control transfer failed\n");
            DbgBreakPoint();
        }

        Hub->Post(HubEvent::TransferFailed);
        return;
    }

    Hub->Post(HubEvent::TransferDone);
}

/* Four byte transfer of m_DromMailbox; a refused send is reported to the machine at once */
static
VOID
NTAPI
HubMailboxTransfer(
    _In_ HubFdo* Hub,
    _In_ BOOLEAN Write,
    _In_ USHORT Register)
{
    NTSTATUS Status;

    Hub->m_Control.SetSetup(Write ? HUB_MAILBOX_WRITE_TYPE : HUB_MAILBOX_READ_TYPE,
                            Write ? HUB_MAILBOX_WRITE_REQUEST : HUB_MAILBOX_READ_REQUEST,
                            HUB_MAILBOX_SPACE,
                            Register,
                            sizeof(Hub->m_DromMailbox));

    Status = Hub->m_Control.Send(Hub->m_RootHubTarget,
                                 Hub->UsbDevice(),
                                 HubMailboxComplete,
                                 Hub,
                                 &Hub->m_DromMailbox,
                                 sizeof(Hub->m_DromMailbox),
                                 FALSE,
                                 Hub->NeedsForwardProgress());
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p mailbox register %u %s not sent 0x%lx\n", Hub, Register, Write ? "write" : "read", Status);
        Hub->Post(HubEvent::TransferFailed);
    }
}

static
USHORT
NTAPI
HubDromReadUshort(
    _In_reads_bytes_(2) const UCHAR* Bytes)
{
    return (USHORT)(Bytes[0] | (Bytes[1] << 8));
}

/* QUIRK: the firmware update protocol gates the read, not tunneling; the identity only names the update device */
BOOLEAN
HubMachine::IsDromReadWanted()
{
    return m_Hub->m_Parent.HubDepth != 0 && m_Hub->m_FwUpdateProtocol == 1;
}

ULONG
HubMachine::MailboxValue()
{
    return m_Hub->m_DromMailbox;
}

ULONG
HubMachine::DromDataLength()
{
    const UCHAR* Header = (const UCHAR*)m_Hub->m_DromHeader;

    return HubDromReadUshort(&Header[DROM_LENGTH_OFFSET]) & DROM_LENGTH_MASK;
}

VOID
HubMachine::ReadDromRegister(
    _In_ USHORT Register)
{
    HubMailboxTransfer(m_Hub, FALSE, Register);
}

VOID
HubMachine::WriteDromRegister(
    _In_ USHORT Register,
    _In_ ULONG Value)
{
    m_Hub->m_DromMailbox = Value;
    HubMailboxTransfer(m_Hub, TRUE, Register);
}

VOID
HubMachine::UseDromHeaderBuffer()
{
    RtlZeroMemory(m_Hub->m_DromHeader, sizeof(m_Hub->m_DromHeader));
    m_Hub->m_DromBuffer = m_Hub->m_DromHeader;
}

/* Whole DWORDs are stored, so the buffer is rounded up to them */
BOOLEAN
HubMachine::AllocateDromBuffer(
    _In_ ULONG Length)
{
    ULONG Bytes = ALIGN_UP_BY(Length, sizeof(ULONG));
    PULONG Buffer;

    Buffer = (PULONG)ExAllocatePoolWithTag(NonPagedPool, Bytes, HUB_TAG_HUB);
    if (Buffer == NULL)
    {
        DPRINT1("Hub %p no memory for a %lu byte router DROM\n", m_Hub, Length);
        return FALSE;
    }

    RtlZeroMemory(Buffer, Bytes);
    m_Hub->m_DromPool = Buffer;
    m_Hub->m_DromBuffer = Buffer;
    m_Hub->m_DromLength = Length;
    return TRUE;
}

VOID
HubMachine::ReleaseDromBuffer()
{
    if (m_Hub->m_DromPool != NULL)
        ExFreePoolWithTag(m_Hub->m_DromPool, HUB_TAG_HUB);

    m_Hub->m_DromPool = NULL;
    m_Hub->m_DromBuffer = NULL;
    m_Hub->m_DromLength = 0;
}

VOID
HubMachine::StoreDromWord(
    _In_ ULONG Index)
{
    NT_ASSERT(m_Hub->m_DromBuffer != NULL);
    m_Hub->m_DromBuffer[Index] = m_Hub->m_DromMailbox;
}

/**
 * @brief
 * Finds the router identity entry of a version 1 DROM, bounds checking each entry.
 */
VOID
HubMachine::ParseDromIdentity()
{
    HubRouterIdentity* Identity = &m_Hub->m_RouterIdentity;
    const UCHAR* Drom = (const UCHAR*)m_Hub->m_DromPool;
    const UCHAR* Entry;
    ULONG Length = m_Hub->m_DromLength;
    ULONG Offset;

    if (Drom == NULL || Drom[DROM_VERSION_OFFSET] != 1)
        return;

    for (Offset = DROM_FIRST_ENTRY; Offset + DROM_ENTRY_HEADER <= Length; Offset += Entry[0])
    {
        Entry = &Drom[Offset];

        if (Entry[0] == 0)
        {
            DPRINT1("Hub %p router DROM has a zero length entry at %lu\n", m_Hub, Offset);
            return;
        }

        if (Entry[0] > Length - Offset)
        {
            DPRINT1("Hub %p router DROM entry at %lu overruns the DROM\n", m_Hub, Offset);
            return;
        }

        if ((Entry[1] & DROM_ENTRY_ADAPTER) || (Entry[1] & DROM_ENTRY_TYPE_MASK) != DROM_ENTRY_TYPE_IDENTITY)
            continue;

        if (Entry[0] < DROM_IDENTITY_LENGTH)
        {
            DPRINT1("Hub %p router DROM identity entry is only %u bytes\n", m_Hub, Entry[0]);
            return;
        }

        /* QUIRK: a later read that fails keeps this identity */
        Identity->VendorId = HubDromReadUshort(&Entry[DROM_IDENTITY_VENDOR]);
        Identity->ProductId = HubDromReadUshort(&Entry[DROM_IDENTITY_PRODUCT]);
        Identity->BcdDevice = HubDromReadUshort(&Entry[DROM_IDENTITY_BCD]);
        Identity->Revision = Entry[DROM_IDENTITY_REVISION];
        Identity->Known = TRUE;

        DPRINT("Hub %p router VID %04X PID %04X BCD %04X REV %04X\n",
               m_Hub,
               Identity->VendorId,
               Identity->ProductId,
               Identity->BcdDevice,
               Identity->Revision);
        return;
    }

    DPRINT1("Hub %p router DROM has no identity entry\n", m_Hub);
}

/**
 * @brief
 * The firmware update child is not supported, so the IDs it would report are only printed.
 */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubDromReportFirmwareUpdateDevice(
    _In_ HubFdo* Hub)
{
    const HubRouterIdentity* Identity = &Hub->m_RouterIdentity;

    if (!Identity->Known || Hub->IsRootHub() || Hub->m_FwUpdateProtocol == 0)
        return;

    DPRINT1("Hub %p firmware update device USB\\VID_%04X&PID_%04X&BCD_%04X&REV_%04X&GFU is not supported\n",
            Hub,
            Identity->VendorId,
            Identity->ProductId,
            Identity->BcdDevice,
            Identity->Revision);
}
