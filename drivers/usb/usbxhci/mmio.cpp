/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Register block mapping, capability parsing and halt/reset/run
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* Vendor registers touched by errata and internal fixes */
#define XHCI_VENDOR_ETRON_FIRMWARE      0x4000
#define XHCI_VENDOR_ETRON_CONTROL       0x4074
#define XHCI_VENDOR_TI_FIRMWARE         0xC120
#define XHCI_VENDOR_FRESCO_CONTROL      0x8090
#define XHCI_VENDOR_PPT_BULK            0x8094
#define XHCI_VENDOR_FRESCO_SHADOW       0x80DC
#define XHCI_VENDOR_PME                 0x80A4
#define XHCI_VENDOR_SSIC_PORT           0x883C
#define XHCI_SSIC_PROG_DONE             0x40000000
#define XHCI_SSIC_PROG_UNUSED           0x80000000
#define XHCI_VENDOR_SAVED_BIT           0x02000000

static const ULONG XhciVendorBitRegisters[] = { 0x8904, 0x8A14, 0x8B24 };

/* Timing limits */
#define XHCI_HANDOFF_WAITS              20
#define XHCI_HANDOFF_WAIT_MS            100
#define XHCI_CNR_WAITS                  100
#define XHCI_CNR_WAIT_MS                100
#define XHCI_HALT_WAITS                 16
#define XHCI_RESET_WAITS                100
#define XHCI_RESET_MAX_STEP_MS          16
#define XHCI_RESET_SLOW_MS              50
#define XHCI_RUN_STALLS                 50
#define XHCI_RUN_STALL_US               100
#define XHCI_RUN_WAITS                  7
#define XHCI_RUN_WAIT_MS                5
#define XHCI_STATE_WAITS                20

static
VOID
NTAPI
XhciSleepMs(
    _In_ ULONG Milliseconds)
{
    LARGE_INTEGER Interval = XhciRelativeMs(Milliseconds);

    KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

/* Mapping ********************************************************************/

NTSTATUS
XhciRegisters::MapRegisters(
    _In_ WDFCMRESLIST Translated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG Count = WdfCmResourceListGetCount(Translated);
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
    {
        Descriptor = WdfCmResourceListGetDescriptor(Translated, Index);
        if (Descriptor == NULL || Descriptor->Type != CmResourceTypeMemory)
            continue;

        /* Only the first memory range is the register BAR */
        m_Base = (PUCHAR)MmMapIoSpace(Descriptor->u.Memory.Start,
                                      Descriptor->u.Memory.Length,
                                      MmNonCached);
        if (m_Base == NULL)
        {
            DPRINT1("Mapping %lu register bytes at 0x%I64x failed\n",
                    Descriptor->u.Memory.Length, Descriptor->u.Memory.Start.QuadPart);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        m_Length = Descriptor->u.Memory.Length;
        m_Mapped = TRUE;
        return STATUS_SUCCESS;
    }

    /* QUIRK: a missing memory resource reports STATUS_UNSUCCESSFUL */
    DPRINT1("No memory resource for the register BAR\n");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
XhciRegisters::Prepare(
    _In_ XhciController* Controller,
    _In_ WDFCMRESLIST Translated)
{
    NTSTATUS Status;

    m_Controller = Controller;

    Status = MapRegisters(Translated);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ParseCapabilities();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Capability registers rejected 0x%lx\n", Status);
        return Status;
    }

    Status = BiosHandoff();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("BIOS handoff failed 0x%lx\n", Status);
        return Status;
    }

    Status = ResetController(FALSE);
    if (!NT_SUCCESS(Status))
        DPRINT1("Initial controller reset failed 0x%lx\n", Status);

    return Status;
}

VOID
XhciRegisters::Release()
{
    if (m_Mapped)
    {
        MmUnmapIoSpace(m_Base, m_Length);
        m_Mapped = FALSE;
    }

    m_Base = NULL;
    m_Length = 0;
    m_Operational = NULL;
    m_Runtime = NULL;
    m_Doorbells = NULL;
    m_ExtendedList = 0;
    m_LegacySupport = 0;
    m_DebugCapability = 0;
}

/* Capability parsing *********************************************************/

VOID
XhciRegisters::ReadFirmwareVersion()
{
    XhciIdentity* Id = &m_Controller->m_Identity;
    BOOLEAN Qualcomm;

    if (Id->FirmwareVersion != XHCI_FIRMWARE_UNKNOWN)
        return;

    Qualcomm = (Id->ParentBus == UcxControllerParentBusTypeAcpi &&
                _strnicmp(Id->AcpiVendorId, "QCOM", 4) == 0);

    if (Id->PciVendorId == 0x1B6F)
    {
        if (InRange(XHCI_VENDOR_ETRON_FIRMWARE, sizeof(ULONG)))
            Id->FirmwareVersion = ReadCapability32(XHCI_VENDOR_ETRON_FIRMWARE);
    }
    else if (Id->PciVendorId == 0x104C || Qualcomm)
    {
        if (InRange(XHCI_VENDOR_TI_FIRMWARE, sizeof(ULONG)))
            Id->FirmwareVersion = ReadCapability32(XHCI_VENDOR_TI_FIRMWARE) & 0xFFFF;
    }
}

NTSTATUS
XhciRegisters::ParseCapabilities()
{
    ULONG Value;
    ULONG Psa;
    ULONG Offset;

    ReadFirmwareVersion();

    Value = ReadCapability32(XHCI_CAP_LENGTH_VERSION);
    m_CapLength = Value & XHCI_CAPLENGTH_MASK;
    m_VersionMajor = (UCHAR)(Value >> XHCI_HCIVERSION_MAJOR_SHIFT);
    m_VersionMinor = (UCHAR)(Value >> XHCI_HCIVERSION_MINOR_SHIFT);

    if (m_VersionMajor == 0)
        m_Controller->SetErrata(XhciErrata::IntPrimaryOnly);
    else if (m_VersionMinor >= 0x10 || m_VersionMajor > 1)
        m_MseLengthUsable = TRUE;

    m_Operational = m_Base + m_CapLength;

    Value = ReadCapability32(XHCI_CAP_HCSPARAMS1);
    m_MaxSlots = Value & XHCI_HCS1_MAX_SLOTS_MASK;
    m_MaxInterrupters = (Value & XHCI_HCS1_MAX_INTRS_MASK) >> XHCI_HCS1_MAX_INTRS_SHIFT;
    m_MaxPorts = Value >> XHCI_HCS1_MAX_PORTS_SHIFT;
    if (m_MaxSlots == 0)
    {
        DPRINT1("HCSPARAMS1 0x%08lx reports no device slots\n", Value);
        return STATUS_INVALID_PARAMETER;
    }
    if (m_MaxInterrupters == 0 || m_MaxInterrupters > 0x400)
    {
        DPRINT1("HCSPARAMS1 0x%08lx reports %lu interrupters\n", Value, m_MaxInterrupters);
        return STATUS_INVALID_PARAMETER;
    }
    if (m_MaxPorts == 0)
    {
        DPRINT1("HCSPARAMS1 0x%08lx reports no ports\n", Value);
        return STATUS_INVALID_PARAMETER;
    }

    Value = ReadCapability32(XHCI_CAP_HCSPARAMS2);
    m_MaxEventRingSegments = 1UL << ((Value & XHCI_HCS2_ERST_MAX_MASK) >> XHCI_HCS2_ERST_MAX_SHIFT);
    m_Scratchpads = (((Value & XHCI_HCS2_SCRATCH_HI_MASK) >> XHCI_HCS2_SCRATCH_HI_SHIFT) << 5) |
                    (Value >> XHCI_HCS2_SCRATCH_LO_SHIFT);

    m_Hcsparams3 = ReadCapability32(XHCI_CAP_HCSPARAMS3);
    if ((m_Hcsparams3 & XHCI_HCS3_U1_LATENCY_MASK) > 0x0A)
    {
        DPRINT1("HCSPARAMS3 0x%08lx U1 exit latency out of range\n", m_Hcsparams3);
        return STATUS_INVALID_PARAMETER;
    }
    if ((m_Hcsparams3 >> XHCI_HCS3_U2_LATENCY_SHIFT) > 0x7FF)
    {
        DPRINT1("HCSPARAMS3 0x%08lx U2 exit latency out of range\n", m_Hcsparams3);
        return STATUS_INVALID_PARAMETER;
    }

    m_Hccparams1 = ReadCapability32(XHCI_CAP_HCCPARAMS1);
    m_Hccparams2 = (m_CapLength >= 0x20) ? ReadCapability32(XHCI_CAP_HCCPARAMS2) : 0;

    /* A 4 KB stream context array holds at most 256 entries */
    Psa = (m_Hccparams1 & XHCI_HCC1_MAX_PSA_MASK) >> XHCI_HCC1_MAX_PSA_SHIFT;
    if (Psa > 7)
        Psa = 7;
    if (Psa == 0 || m_Controller->HasErrata(XhciErrata::StrmUnsupported))
        m_Streams = 0;
    else
        m_Streams = (2UL << Psa) - 1;

    Offset = (m_Hccparams1 >> XHCI_HCC1_XECP_SHIFT) * 4;
    if (Offset == 0 || Offset >= m_Length)
    {
        DPRINT1("Extended capability pointer 0x%lx outside the %lu byte BAR\n", Offset, m_Length);
        return STATUS_INVALID_PARAMETER;
    }
    m_ExtendedList = Offset;
    m_LegacySupport = FindExtendedCapability(XHCI_EXTCAP_LEGACY, 0);
    m_DebugCapability = FindExtendedCapability(XHCI_EXTCAP_DEBUG, 0);

    /* DBOFF and RTSOFF must fall inside the mapping */
    Offset = ReadCapability32(XHCI_CAP_DBOFF);
    if (Offset == 0 || !InRange(Offset, XHCI_DOORBELL_STRIDE * (m_MaxSlots + 1)))
    {
        DPRINT1("Doorbell offset 0x%lx invalid for a %lu byte BAR\n", Offset, m_Length);
        return STATUS_INVALID_PARAMETER;
    }
    m_Doorbells = m_Base + Offset;

    Offset = ReadCapability32(XHCI_CAP_RTSOFF);
    if (Offset == 0 ||
        !InRange(Offset, XHCI_RUNTIME_INTERRUPTER_BASE + XHCI_INTERRUPTER_STRIDE * m_MaxInterrupters))
    {
        DPRINT1("Runtime offset 0x%lx invalid for a %lu byte BAR\n", Offset, m_Length);
        return STATUS_INVALID_PARAMETER;
    }
    m_Runtime = m_Base + Offset;

    Value = ReadOperational32(XHCI_OP_PAGESIZE);
    if (Value != XHCI_PAGESIZE_4K)
    {
        DPRINT1("PAGESIZE 0x%lx is not 4 KB only\n", Value);
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT("xHCI %x.%02x: %lu slots, %lu interrupters, %lu ports, %lu scratchpads, HCC 0x%08lx/0x%08lx\n",
           m_VersionMajor, m_VersionMinor, m_MaxSlots, m_MaxInterrupters, m_MaxPorts,
           m_Scratchpads, m_Hccparams1, m_Hccparams2);
    return STATUS_SUCCESS;
}

/* Extended capabilities ******************************************************/

ULONG
XhciRegisters::FindExtendedCapability(
    _In_ ULONG Id,
    _In_ ULONG Start) const
{
    ULONG Offset;
    ULONG Header;
    ULONG Next;

    if (!m_Mapped || m_ExtendedList == 0)
        return 0;

    Offset = (Start == 0) ? m_ExtendedList : Start;
    Header = ReadCapability32(Offset);

    /* A given start is skipped; only entries after it are candidates */
    if (Start == 0 && (Header & XHCI_EXTCAP_ID_MASK) == Id)
        return Offset;

    for (;;)
    {
        Next = ((Header & XHCI_EXTCAP_NEXT_MASK) >> XHCI_EXTCAP_NEXT_SHIFT) * 4;
        if (Next == 0)
            return 0;

        Offset += Next;
        if (Offset >= m_Length)
            return 0;

        Header = ReadCapability32(Offset);
        if ((Header & XHCI_EXTCAP_ID_MASK) == Id)
            return Offset;
    }
}

ULONG
XhciRegisters::ReadExtended32(
    _In_ ULONG Offset) const
{
    if (!InRange(Offset, sizeof(ULONG)))
    {
        DPRINT1("Extended capability read at 0x%lx outside the BAR\n", Offset);
        return MAXULONG;
    }

    return READ_REGISTER_ULONG((PULONG)(m_Base + Offset));
}

VOID
XhciRegisters::WriteExtended32(
    _In_ ULONG Offset,
    _In_ ULONG Value)
{
    if (!InRange(Offset, sizeof(ULONG)))
    {
        DPRINT1("Extended capability write at 0x%lx outside the BAR\n", Offset);
        return;
    }

    WRITE_REGISTER_ULONG((PULONG)(m_Base + Offset), Value);
}

/* Raw access *****************************************************************/

BOOLEAN
XhciRegisters::InRange(
    _In_ ULONG Offset,
    _In_ ULONG Size) const
{
    return m_Mapped && Offset < m_Length && Size <= m_Length - Offset;
}

UCHAR
XhciRegisters::ReadByte(
    _In_ ULONG Offset) const
{
    return READ_REGISTER_UCHAR(m_Base + Offset);
}

VOID
XhciRegisters::WriteByte(
    _In_ ULONG Offset,
    _In_ UCHAR Value)
{
    WRITE_REGISTER_UCHAR(m_Base + Offset, Value);
}

VOID
XhciRegisters::ModifyCapability32(
    _In_ ULONG Offset,
    _In_ ULONG Clear,
    _In_ ULONG Set)
{
    PULONG Register;

    if (!InRange(Offset, sizeof(ULONG)))
        return;

    Register = (PULONG)(m_Base + Offset);
    WRITE_REGISTER_ULONG(Register, (READ_REGISTER_ULONG(Register) & ~Clear) | Set);
}

ULONG64
XhciRegisters::Read64(
    _In_ PUCHAR Address) const
{
    ULONG Low;
    ULONG High;

#ifdef _WIN64
    if (!m_Controller->HasErrata(XhciErrata::RegSplit64BitAccess))
        return READ_REGISTER_ULONG64((PULONG64)Address);
#endif

    Low = READ_REGISTER_ULONG((PULONG)Address);
    High = READ_REGISTER_ULONG((PULONG)(Address + sizeof(ULONG)));
    return ((ULONG64)High << 32) | Low;
}

VOID
XhciRegisters::Write64(
    _In_ PUCHAR Address,
    _In_ ULONG64 Value)
{
#ifdef _WIN64
    if (!m_Controller->HasErrata(XhciErrata::RegSplit64BitAccess))
    {
        WRITE_REGISTER_ULONG64((PULONG64)Address, Value);
        return;
    }
#endif

    /* Low dword first; CRCR, ERSTBA and ERDP latch on the high dword write */
    WRITE_REGISTER_ULONG((PULONG)Address, (ULONG)Value);
    WRITE_REGISTER_ULONG((PULONG)(Address + sizeof(ULONG)), (ULONG)(Value >> 32));
}

ULONG
XhciRegisters::ReadCapability32(
    _In_ ULONG Offset) const
{
    if (!m_Mapped)
        return MAXULONG;

    return READ_REGISTER_ULONG((PULONG)(m_Base + Offset));
}

ULONG
XhciRegisters::ReadOperational32(
    _In_ ULONG Offset) const
{
    if (!m_Mapped)
        return MAXULONG;

    return READ_REGISTER_ULONG((PULONG)(m_Operational + Offset));
}

VOID
XhciRegisters::WriteOperational32(
    _In_ ULONG Offset,
    _In_ ULONG Value)
{
    if (m_Mapped)
        WRITE_REGISTER_ULONG((PULONG)(m_Operational + Offset), Value);
}

ULONG64
XhciRegisters::ReadOperational64(
    _In_ ULONG Offset) const
{
    if (!m_Mapped)
        return ~0ULL;

    return Read64(m_Operational + Offset);
}

VOID
XhciRegisters::WriteOperational64(
    _In_ ULONG Offset,
    _In_ ULONG64 Value)
{
    if (m_Mapped)
        Write64(m_Operational + Offset, Value);
}

ULONG
XhciRegisters::ReadRuntime32(
    _In_ ULONG Offset) const
{
    if (!m_Mapped)
        return MAXULONG;

    return READ_REGISTER_ULONG((PULONG)(m_Runtime + Offset));
}

VOID
XhciRegisters::WriteRuntime32(
    _In_ ULONG Offset,
    _In_ ULONG Value)
{
    if (m_Mapped)
        WRITE_REGISTER_ULONG((PULONG)(m_Runtime + Offset), Value);
}

ULONG64
XhciRegisters::ReadRuntime64(
    _In_ ULONG Offset) const
{
    if (!m_Mapped)
        return ~0ULL;

    return Read64(m_Runtime + Offset);
}

VOID
XhciRegisters::WriteRuntime64(
    _In_ ULONG Offset,
    _In_ ULONG64 Value)
{
    if (m_Mapped)
        Write64(m_Runtime + Offset, Value);
}

VOID
XhciRegisters::RingDoorbell(
    _In_ ULONG Index,
    _In_ ULONG Value)
{
    if (m_Mapped && Index <= m_MaxSlots)
        WRITE_REGISTER_ULONG((PULONG)(m_Doorbells + Index * XHCI_DOORBELL_STRIDE), Value);
}

ULONG
XhciRegisters::ReadPort32(
    _In_ ULONG Port,
    _In_ ULONG Offset) const
{
    if (!m_Mapped || Port == 0 || Port > m_MaxPorts)
        return MAXULONG;

    return READ_REGISTER_ULONG((PULONG)(m_Operational + XHCI_OP_PORT_BASE +
                                        XHCI_OP_PORT_STRIDE * (Port - 1) + Offset));
}

VOID
XhciRegisters::WritePort32(
    _In_ ULONG Port,
    _In_ ULONG Offset,
    _In_ ULONG Value)
{
    if (!m_Mapped || Port == 0 || Port > m_MaxPorts)
        return;

    WRITE_REGISTER_ULONG((PULONG)(m_Operational + XHCI_OP_PORT_BASE +
                                  XHCI_OP_PORT_STRIDE * (Port - 1) + Offset), Value);
}

/* Parsed values **************************************************************/

ULONG
XhciRegisters::MaxSlots() const
{
    return m_MaxSlots;
}

ULONG
XhciRegisters::MaxInterrupters() const
{
    return m_MaxInterrupters;
}

ULONG
XhciRegisters::MaxPorts() const
{
    return m_MaxPorts;
}

ULONG
XhciRegisters::EventRingSegmentTableMax() const
{
    return m_MaxEventRingSegments;
}

ULONG
XhciRegisters::MaxEventRingSegments() const
{
    return m_MaxEventRingSegments;
}

ULONG
XhciRegisters::ScratchpadCount() const
{
    return m_Scratchpads;
}

ULONG
XhciRegisters::PageSize() const
{
    return PAGE_SIZE;
}

ULONG
XhciRegisters::SupportedStreams() const
{
    return m_Streams;
}

ULONG
XhciRegisters::Hcsparams3() const
{
    return m_Hcsparams3;
}

ULONG
XhciRegisters::Hccparams1() const
{
    return m_Hccparams1;
}

ULONG
XhciRegisters::Hccparams2() const
{
    return m_Hccparams2;
}

ULONG
XhciRegisters::CapLength() const
{
    return m_CapLength;
}

UCHAR
XhciRegisters::VersionMajor() const
{
    return m_VersionMajor;
}

UCHAR
XhciRegisters::VersionMinor() const
{
    return m_VersionMinor;
}

BOOLEAN
XhciRegisters::MseLengthUsable() const
{
    return m_MseLengthUsable;
}

BOOLEAN
XhciRegisters::IsMapped() const
{
    return m_Mapped;
}

ULONG
XhciRegisters::LegacySupportOffset() const
{
    return m_LegacySupport;
}

ULONG
XhciRegisters::DebugCapabilityOffset() const
{
    return m_DebugCapability;
}

ULONG
XhciRegisters::ContextSize() const
{
    return (m_Hccparams1 & XHCI_HCC1_CSZ) ? 64 : 32;
}

USHORT
XhciRegisters::InterfaceVersion() const
{
    return (USHORT)((m_VersionMajor << 8) | m_VersionMinor);
}

BOOLEAN
XhciRegisters::Addressing64() const
{
    /* Read live, as the reference does */
    return (ReadCapability32(XHCI_CAP_HCCPARAMS1) & XHCI_HCC1_AC64) != 0;
}

BOOLEAN
XhciRegisters::ContiguousFrameId() const
{
    if (m_Controller->HasErrata(XhciErrata::IsochNoFrameIdContinuity))
        return FALSE;

    return (m_Hccparams1 & XHCI_HCC1_CFC) != 0;
}

ULONG
XhciRegisters::CurrentFrame(
    _In_ ULONG Increment) const
{
    ULONG Index = ReadRuntime32(XHCI_RT_MFINDEX) & XHCI_MFINDEX_MASK;

    return ((Index + Increment) >> 3) & 0x7FF;
}

/* Sequences ******************************************************************/

NTSTATUS
XhciRegisters::WaitForReady()
{
    ULONG Waits;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    for (Waits = 0; ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_CNR; Waits++)
    {
        if (Waits == XHCI_CNR_WAITS)
        {
            DPRINT1("Controller Not Ready still set after %lu ms\n", XHCI_CNR_WAITS * XHCI_CNR_WAIT_MS);
            return STATUS_UNSUCCESSFUL;
        }

        XhciSleepMs(XHCI_CNR_WAIT_MS);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciRegisters::BiosHandoff()
{
    ULONG Waits;
    NTSTATUS Status;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    if (m_LegacySupport != 0)
    {
        WriteByte(m_LegacySupport + XHCI_LEGSUP_OS_OWNED_BYTE,
                  ReadByte(m_LegacySupport + XHCI_LEGSUP_OS_OWNED_BYTE) | XHCI_LEGSUP_OWNED);

        for (Waits = 0; ReadByte(m_LegacySupport + XHCI_LEGSUP_BIOS_OWNED_BYTE) & XHCI_LEGSUP_OWNED; Waits++)
        {
            if (Waits == XHCI_HANDOFF_WAITS)
            {
                if (!m_Controller->HasErrata(XhciErrata::BootTolerateBiosHold))
                {
                    DPRINT1("BIOS kept ownership of the controller\n");
                    return STATUS_UNSUCCESSFUL;
                }

                DPRINT1("BIOS kept ownership of the controller, continuing\n");
                break;
            }

            XhciSleepMs(XHCI_HANDOFF_WAIT_MS);
        }
    }
    else
    {
        DPRINT1("No USB legacy support capability, skipping the BIOS handoff\n");
    }

    if (!(ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_HCH))
    {
        if (m_Controller->HasErrata(XhciErrata::BootStrictBiosRelease))
        {
            DPRINT1("Controller still running after the BIOS handoff\n");
            return STATUS_UNSUCCESSFUL;
        }

        Status = Stop();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Stopping the controller after the BIOS handoff failed 0x%lx\n", Status);
            return Status;
        }
    }

    /* Keeps the BIOS SMI handler quiet, notably on S3 resume */
    if (m_LegacySupport != 0)
    {
        ULONG Offset = m_LegacySupport + XHCI_LEGSUP_CTLSTS;

        WriteExtended32(Offset, ReadExtended32(Offset) & XHCI_LEGCTLSTS_KEEP_MASK);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciRegisters::Stop()
{
    ULONG Waits;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    WriteOperational32(XHCI_OP_USBCMD, ReadOperational32(XHCI_OP_USBCMD) & ~XHCI_USBCMD_RS);

    /* QUIRK: each 1 ms wait lasts a full timer tick */
    for (Waits = 0; !(ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_HCH); Waits++)
    {
        if (Waits == XHCI_HALT_WAITS)
        {
            DPRINT1("Controller did not halt, USBSTS 0x%08lx\n", ReadOperational32(XHCI_OP_USBSTS));
            return STATUS_UNSUCCESSFUL;
        }

        XhciSleepMs(1);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciRegisters::Reset()
{
    return ResetController(FALSE);
}

NTSTATUS
XhciRegisters::ResetController(
    _In_ BOOLEAN AllowRunning)
{
    ULONG Waits;
    ULONG Step = 1;
    ULONG Elapsed = 0;
    NTSTATUS Status;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    Status = WaitForReady();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller not ready before reset\n");
        return Status;
    }

    if (!AllowRunning && !(ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_HCH))
    {
        DPRINT1("Reset refused, controller not halted\n");
        return STATUS_UNSUCCESSFUL;
    }

    ProgramSsicPortUnused(FALSE);

    WriteOperational32(XHCI_OP_USBCMD, XHCI_USBCMD_HCRST);

    for (Waits = 0; ReadOperational32(XHCI_OP_USBCMD) & XHCI_USBCMD_HCRST; Waits++)
    {
        if (Waits == XHCI_RESET_WAITS)
        {
            DPRINT1("Host controller reset did not finish in %lu ms\n", Elapsed);
            return STATUS_UNSUCCESSFUL;
        }

        XhciSleepMs(Step);
        Elapsed += Step;
        if (Step < XHCI_RESET_MAX_STEP_MS)
            Step *= 2;
    }

    if (Elapsed > XHCI_RESET_SLOW_MS)
        DPRINT1("Host controller reset took %lu ms, over the %u ms limit\n", Elapsed, XHCI_RESET_SLOW_MS);

    Status = WaitForReady();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller not ready after reset\n");
        return Status;
    }

    if (m_Controller->m_InternalFlags & XhciController::FixAsmediaResetDelay)
        XhciSleepMs(100);

    return STATUS_SUCCESS;
}

NTSTATUS
XhciRegisters::Run()
{
    ULONG Command;
    ULONG Index;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    if (!(ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_HCH))
    {
        DPRINT1("Start refused, controller not halted\n");
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    WriteOperational32(XHCI_OP_DNCTRL, XHCI_DNCTRL_FUNCTION_WAKE);

    /* QUIRK: HSEE is left as it reads */
    Command = ReadOperational32(XHCI_OP_USBCMD) | XHCI_USBCMD_RS | XHCI_USBCMD_INTE;
    Command &= ~(XHCI_USBCMD_CME | XHCI_USBCMD_ETE);
    if (m_Hccparams2 & XHCI_HCC2_CMC)
        Command |= XHCI_USBCMD_CME;
    if ((m_Hccparams2 & XHCI_HCC2_LEC) && (m_Hccparams2 & XHCI_HCC2_ETC))
        Command |= XHCI_USBCMD_ETE;
    WriteOperational32(XHCI_OP_USBCMD, Command);

    for (Index = 0; Index < XHCI_RUN_STALLS; Index++)
    {
        KeStallExecutionProcessor(XHCI_RUN_STALL_US);
        if (!(ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_HCH))
            return STATUS_SUCCESS;
    }

    /* Then poll with 5 ms sleeps */
    for (Index = 0; Index < XHCI_RUN_WAITS; Index++)
    {
        XhciSleepMs(XHCI_RUN_WAIT_MS);
        if (!(ReadOperational32(XHCI_OP_USBSTS) & XHCI_USBSTS_HCH))
            return STATUS_SUCCESS;
    }

    /* QUIRK: still halted is only reported */
    DPRINT1("Controller still halted after start, USBSTS 0x%08lx\n", ReadOperational32(XHCI_OP_USBSTS));
    return STATUS_SUCCESS;
}

NTSTATUS
XhciRegisters::CheckStateOperationAllowed() const
{
    ULONG Usbsts = ReadOperational32(XHCI_OP_USBSTS);

    if (!(Usbsts & XHCI_USBSTS_HCH))
    {
        DPRINT1("State save or restore refused, controller running\n");
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    if (Usbsts & (XHCI_USBSTS_SSS | XHCI_USBSTS_RSS))
    {
        DPRINT1("State save or restore already in progress, USBSTS 0x%08lx\n", Usbsts);
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciRegisters::WaitForStateOperation(
    _In_ ULONG BusyBit)
{
    LARGE_INTEGER Interval;
    ULONG Usbsts;
    ULONG Waits = 0;

    Interval.QuadPart = -2000;

    for (;;)
    {
        Usbsts = ReadOperational32(XHCI_OP_USBSTS);
        if (Usbsts & XHCI_USBSTS_SRE)
        {
            DPRINT1("Save/Restore Error reported, USBSTS 0x%08lx\n", Usbsts);
            WriteOperational32(XHCI_OP_USBSTS, XHCI_USBSTS_SRE);
            return STATUS_ADAPTER_HARDWARE_ERROR;
        }

        if (!(Usbsts & BusyBit))
            return STATUS_SUCCESS;

        if (Waits == XHCI_STATE_WAITS)
        {
            DPRINT1("State save or restore timed out, USBSTS 0x%08lx\n", Usbsts);
            return STATUS_ADAPTER_HARDWARE_ERROR;
        }

        Waits++;
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }
}

NTSTATUS
XhciRegisters::SaveState()
{
    NTSTATUS Status;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    Status = CheckStateOperationAllowed();
    if (!NT_SUCCESS(Status))
        return Status;

    WriteOperational32(XHCI_OP_USBCMD, ReadOperational32(XHCI_OP_USBCMD) | XHCI_USBCMD_CSS);
    return WaitForStateOperation(XHCI_USBSTS_SSS);
}

NTSTATUS
XhciRegisters::RestoreState()
{
    NTSTATUS Status;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    Status = CheckStateOperationAllowed();
    if (!NT_SUCCESS(Status))
        return Status;

    /* DNCTRL is not part of the saved state and must be back before CRS */
    WriteOperational32(XHCI_OP_DNCTRL, XHCI_DNCTRL_FUNCTION_WAKE);
    WriteOperational32(XHCI_OP_USBCMD, ReadOperational32(XHCI_OP_USBCMD) | XHCI_USBCMD_CRS);
    return WaitForStateOperation(XHCI_USBSTS_RSS);
}

/* Vendor sequences ***********************************************************/

VOID
XhciRegisters::ProgramSsicPortUnused(
    _In_ BOOLEAN PowerDown)
{
    if (!m_Controller->HasErrata(XhciErrata::PortParkIdleSsic))
        return;

    ModifyCapability32(XHCI_VENDOR_SSIC_PORT, XHCI_SSIC_PROG_DONE, 0);
    if (PowerDown)
        ModifyCapability32(XHCI_VENDOR_SSIC_PORT, 0, XHCI_SSIC_PROG_UNUSED);
    else
        ModifyCapability32(XHCI_VENDOR_SSIC_PORT, XHCI_SSIC_PROG_UNUSED, 0);
    ModifyCapability32(XHCI_VENDOR_SSIC_PORT, 0, XHCI_SSIC_PROG_DONE);
}

VOID
XhciRegisters::SaveVendorBits()
{
    ULONG Index;

    if (!m_Controller->HasErrata(XhciErrata::PwrPreserveVendorRegs))
        return;

    m_VendorBits = 0;
    for (Index = 0; Index < RTL_NUMBER_OF(XhciVendorBitRegisters); Index++)
    {
        if (ReadExtended32(XhciVendorBitRegisters[Index]) & XHCI_VENDOR_SAVED_BIT)
            m_VendorBits |= 1UL << Index;
    }
}

VOID
XhciRegisters::RestoreVendorBits()
{
    ULONG Index;

    if (!m_Controller->HasErrata(XhciErrata::PwrPreserveVendorRegs))
        return;

    for (Index = 0; Index < RTL_NUMBER_OF(XhciVendorBitRegisters); Index++)
    {
        if (m_VendorBits & (1UL << Index))
            ModifyCapability32(XhciVendorBitRegisters[Index], 0, XHCI_VENDOR_SAVED_BIT);
        else
            ModifyCapability32(XhciVendorBitRegisters[Index], XHCI_VENDOR_SAVED_BIT, 0);
    }
}

VOID
XhciRegisters::ApplyD0Workarounds()
{
    ULONG64 Fixes = m_Controller->m_InternalFlags;

    if (Fixes & XhciController::FixFrescoControl)
        ModifyCapability32(XHCI_VENDOR_FRESCO_CONTROL, 1UL << 12, 0);
    if (Fixes & XhciController::FixFrescoShadow)
        ModifyCapability32(XHCI_VENDOR_FRESCO_SHADOW, (1UL << 1) | (1UL << 7), 0);
    if (m_Controller->HasErrata(XhciErrata::IntelPchAsyncOrderingFix))
        ModifyCapability32(XHCI_VENDOR_PPT_BULK, 0, 1UL << 21);
    if (Fixes & XhciController::FixEtronSet)
        ModifyCapability32(XHCI_VENDOR_ETRON_CONTROL, 0, 1UL << 15);
    if (Fixes & XhciController::FixEtronClear)
        ModifyCapability32(XHCI_VENDOR_ETRON_CONTROL, 1UL << 23, 0);

    if (m_Controller->HasErrata(XhciErrata::PwrTogglePmeEnable))
    {
        ModifyCapability32(XHCI_VENDOR_PME, 0, 1UL << 28);
        KeStallExecutionProcessor(50);
        ModifyCapability32(XHCI_VENDOR_PME, 1UL << 28, 0);
    }
}
