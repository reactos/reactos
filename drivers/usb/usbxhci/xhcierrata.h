/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller errata flag bit positions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/*
 * Bit positions of the 64 bit errata value. They match the layout the errata
 * database uses, so a value from there can be ORed in unchanged.
 */
enum class XhciErrata : ULONG
{
    RegSplit64BitAccess = 0,
    PwrSkipSaveRestore = 1,
    BootTolerateBiosHold = 2,
    RingKeepTdInOneSegment = 3,
    HostRefuseToStart = 4,
    EpStopContextErrorIsStopped = 5,
    CtlSplitDataAt512 = 6,
    XferFlattenMdlChain = 7,
    UnusedBit8 = 8,
    IntLegacyLineOnly = 9,
    UnusedBit10 = 10,
    RingLinkTrbChain = 11,
    IntPrimaryOnly = 12,
    EvtDropVendorCode199 = 13,
    EpDeferFirstStop = 14,
    EpResetViaSlotCycle = 15,
    EpClampIntervalTo7 = 16,
    XferNoInlineData = 17,
    StrmUnsupported = 18,
    PortWakeToU0BeforeSuspend = 19,
    PwrStayInD0 = 20,
    StrmReconfigOnStop = 21,
    PortHighSpeedLpmOff = 22,
    IntelPchBandwidthCap = 23,
    HostStaleFirmware = 24,
    PortHighSpeedDisableSuspends = 25,
    PortRetrySuspendEntry = 26,
    PortHighSpeedLpmOffForSuspend = 27,
    EvtResetOnRingOverflow = 28,
    PortHoldWakeMaskAwake = 29,
    PortAckCscOnPowerDown = 30,
    CmdOneInFlight = 31,
    XferFlushTtOnAbort = 32,
    IsochInterruptEveryTd = 33,
    XferPacketAlignChunks = 34,
    StrmByteCountLayoutA = 35,
    StrmByteCountLayoutB = 36,
    EvtCheckTrbAddress = 37,
    EvtDiscardRepeatedEd0 = 38,
    XferOneAsyncTdAtATime = 39,
    StrmByteCountTruncated = 40,
    IsochSkipPastFrames = 41,
    IsochMissedServiceSilent = 42,
    PwrResetBeforeShutdown = 43,
    StrmByteCountRearm = 44,
    StrmByteCountHasValidFlag = 45,
    IntelPchAsyncOrderingFix = 46,
    IsochFsNoBareLinkTd = 47,
    EvtTwoSegmentRingCap = 48,
    XferPadSsBulkInTail = 49,
    RingClearNewSegments = 50,
    PortMaskDebugPortNoise = 51,
    EpInfiniteCerr = 52,
    IsochStarveEventNoEd = 53,
    AcpiNotifyBeforeIntOff = 54,
    PwrPreserveVendorRegs = 55,
    PwrTogglePmeEnable = 56,
    PortParkIdleSsic = 57,
    AcpiDsmAllowD3Cold = 58,
    IsochNoFrameIdContinuity = 59,
    AcpiDsmHsicSuspendDetach = 60,
    CfgMultiTtFromHubData = 61,
    Reserved62 = 62,
    BootStrictBiosRelease = 63
};

FORCEINLINE
ULONG64
XhciErrataBit(
    _In_ XhciErrata Bit)
{
    return 1ULL << static_cast<ULONG>(Bit);
}
