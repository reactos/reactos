/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Register block mapping, capability parsing and halt/reset/run
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;

/** Owns the MMIO mapping and every raw register access. */
class XhciRegisters
{
public:
    /* Lifetime, called by the controller in PrepareHardware order */
    NTSTATUS
    Prepare(
        _In_ XhciController* Controller,
        _In_ WDFCMRESLIST Translated);
    VOID Release();

    /* Sequences */
    NTSTATUS BiosHandoff();
    NTSTATUS WaitForReady();
    NTSTATUS Stop();
    NTSTATUS Reset();
    NTSTATUS Run();
    NTSTATUS SaveState();
    NTSTATUS RestoreState();

    /* Raw access. 64 bit writes split into two 32 bit writes when the errata asks for it. */
    ULONG
    ReadCapability32(
        _In_ ULONG Offset) const;
    ULONG
    ReadOperational32(
        _In_ ULONG Offset) const;
    VOID
    WriteOperational32(
        _In_ ULONG Offset,
        _In_ ULONG Value);
    ULONG64
    ReadOperational64(
        _In_ ULONG Offset) const;
    VOID
    WriteOperational64(
        _In_ ULONG Offset,
        _In_ ULONG64 Value);
    ULONG
    ReadRuntime32(
        _In_ ULONG Offset) const;
    VOID
    WriteRuntime32(
        _In_ ULONG Offset,
        _In_ ULONG Value);
    ULONG64
    ReadRuntime64(
        _In_ ULONG Offset) const;
    VOID
    WriteRuntime64(
        _In_ ULONG Offset,
        _In_ ULONG64 Value);
    VOID
    RingDoorbell(
        _In_ ULONG Index,
        _In_ ULONG Value);

    /** PORTSC and the three registers after it for a 1 based port number. */
    ULONG
    ReadPort32(
        _In_ ULONG Port,
        _In_ ULONG Offset) const;
    VOID
    WritePort32(
        _In_ ULONG Port,
        _In_ ULONG Offset,
        _In_ ULONG Value);

    /** Next extended capability with Id after Start (0 starts at the first); 0 when none. */
    ULONG
    FindExtendedCapability(
        _In_ ULONG Id,
        _In_ ULONG Start) const;
    ULONG
    ReadExtended32(
        _In_ ULONG Offset) const;
    VOID
    WriteExtended32(
        _In_ ULONG Offset,
        _In_ ULONG Value);

    /* Parsed capability values */
    ULONG MaxSlots() const;
    ULONG MaxInterrupters() const;
    ULONG MaxPorts() const;
    ULONG EventRingSegmentTableMax() const;
    ULONG ScratchpadCount() const;
    ULONG ContextSize() const;
    ULONG PageSize() const;
    USHORT InterfaceVersion() const;
    BOOLEAN Addressing64() const;
    BOOLEAN ContiguousFrameId() const;
    ULONG SupportedStreams() const;
    ULONG Hcsparams3() const;


    /*
     * Extended capability offsets taken and returned above are byte offsets from
     * the BAR, so 0 never names a capability.
     */

    /** HCRST sequence. Reset() is this with AllowRunning FALSE. */
    NTSTATUS
    ResetController(
        _In_ BOOLEAN AllowRunning);

    BOOLEAN IsMapped() const;
    ULONG CapLength() const;
    ULONG Hccparams1() const;
    ULONG Hccparams2() const;
    UCHAR VersionMajor() const;
    UCHAR VersionMinor() const;
    BOOLEAN MseLengthUsable() const;
    ULONG MaxEventRingSegments() const;
    ULONG LegacySupportOffset() const;
    ULONG DebugCapabilityOffset() const;

    /** Current 11 bit frame, Increment microframes ahead. */
    ULONG
    CurrentFrame(
        _In_ ULONG Increment) const;

    /* Vendor sequences */
    VOID
    ProgramSsicPortUnused(
        _In_ BOOLEAN PowerDown);
    VOID SaveVendorBits();
    VOID RestoreVendorBits();
    VOID ApplyD0Workarounds();

private:
    NTSTATUS
    MapRegisters(
        _In_ WDFCMRESLIST Translated);
    NTSTATUS ParseCapabilities();
    VOID ReadFirmwareVersion();

    BOOLEAN
    InRange(
        _In_ ULONG Offset,
        _In_ ULONG Size) const;

    UCHAR
    ReadByte(
        _In_ ULONG Offset) const;
    VOID
    WriteByte(
        _In_ ULONG Offset,
        _In_ UCHAR Value);
    VOID
    ModifyCapability32(
        _In_ ULONG Offset,
        _In_ ULONG Clear,
        _In_ ULONG Set);

    ULONG64
    Read64(
        _In_ PUCHAR Address) const;
    VOID
    Write64(
        _In_ PUCHAR Address,
        _In_ ULONG64 Value);

    /** Polls USBSTS for a save or restore to finish. */
    NTSTATUS
    WaitForStateOperation(
        _In_ ULONG BusyBit);

    NTSTATUS CheckStateOperationAllowed() const;

    XhciController* m_Controller;
    PUCHAR m_Base;
    ULONG m_Length;
    BOOLEAN m_Mapped;
    PUCHAR m_Operational;
    PUCHAR m_Runtime;
    PUCHAR m_Doorbells;

    ULONG m_ExtendedList;
    ULONG m_LegacySupport;
    ULONG m_DebugCapability;

    ULONG m_CapLength;
    UCHAR m_VersionMajor;
    UCHAR m_VersionMinor;
    BOOLEAN m_MseLengthUsable;
    ULONG m_MaxSlots;
    ULONG m_MaxInterrupters;
    ULONG m_MaxPorts;
    ULONG m_MaxEventRingSegments;
    ULONG m_Scratchpads;
    ULONG m_Hcsparams3;
    ULONG m_Hccparams1;
    ULONG m_Hccparams2;
    ULONG m_Streams;

    /** Bit 25 of the three errata 55 registers, one bit each. */
    ULONG m_VendorBits;
};
