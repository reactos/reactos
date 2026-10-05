/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Descriptor validation for devices and hubs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Outcome of one descriptor check */
enum class Verdict : UCHAR
{
    Ok,
    Failed,
    Fatal
};

/* When a finding fails the descriptor; it is always logged */
enum class Gate : UCHAR
{
    Always,
    Legacy,
    NewRule,
    Versioned,
    Cap20,
    CapSs,
    CapSsp,
    Reserved,
    LogOnly,
    Warning
};

/* Newest dwWindowsVersion an MS OS 2.0 descriptor set may target: the one the hub reports */
#define HUB_MSOS20_NEWEST_VERSION NTDDI_WIN11_GE

/* MS OS 2.0 subsets never nest this deep in a sane device */
#define HUB_MSOS20_MAX_DEPTH 8

static const GUID HubMsOs20PlatformUuid =
    { 0xD8DD60DF, 0x4589, 0x4CC7, { 0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F } };

static const GUID HubPlatformFeaturesUuid =
    { 0xA6C10D69, 0x5E2F, 0x480D, { 0xA5, 0x23, 0x52, 0xC0, 0x56, 0xFA, 0xD4, 0xB4 } };

/* Walk state for one configuration descriptor set */
struct ConfigWalk
{
    const HubDescContext* Ctx;
    PUCHAR Base;
    ULONG BufferLength;
    PUCHAR End;
    PUSB_CONFIGURATION_DESCRIPTOR Config;
    PBOOLEAN SupportsStreams;
    PUSB_ENDPOINT_DESCRIPTOR LastEndpoint;
    ULONG InterfaceSeen[8];
    ULONG AlternateSeen[8];
    ULONG EndpointSeen[8];
    ULONG InterfacesFound;
    ULONG EndpointsDeclared;
    ULONG EndpointsCounted;
    UCHAR Interface;
    UCHAR Alternate;
    BOOLEAN HadInterface;
    BOOLEAN HadEndpoint;
    BOOLEAN WantCompanion;
    BOOLEAN WantSspCompanion;
    BOOLEAN GotCompanion;
    BOOLEAN GotSspCompanion;
};

/* Walk state for one BOS descriptor set */
struct BosWalk
{
    const HubDescContext* Ctx;
    PUCHAR Base;
    ULONG BufferLength;
    HubBosInfo* Info;
    PUSHORT U1;
    PUSHORT U2;
    PBOOLEAN Ltm;
    HubLpmInfo* Lpm;
};

/* Context and logging */

VOID
NTAPI
HubDescInitDeviceContext(
    _Out_ HubDescContext* Context,
    _In_ USHORT BcdUsb,
    _In_ USB_DEVICE_SPEED Speed,
    _In_ BOOLEAN Win8Behavior,
    _In_ ULONG SspIsochBurstCount,
    _In_opt_ PULONG Bitmap)
{
    RtlZeroMemory(Context, sizeof(*Context));
    Context->BcdUsb = BcdUsb;
    Context->Speed = Speed;
    Context->LegacyStrict = HubDriver.HasFlag(HubGlobal::EnableExtendedValidation);
    Context->ReservedFields = HubDriver.HasFlag(HubGlobal::CheckReservedFields);
    Context->Strictest = HubDriver.HasFlag(HubGlobal::StrictDescriptorChecks);
    Context->Win8Behavior = Win8Behavior;
    Context->SspIsochBurstCount = SspIsochBurstCount;
    Context->Bitmap = Bitmap;
}

VOID
NTAPI
HubDescInitHubContext(
    _Out_ HubDescContext* Context,
    _In_ USHORT BcdUsb,
    _In_ USB_DEVICE_SPEED Speed)
{
    RtlZeroMemory(Context, sizeof(*Context));
    Context->BcdUsb = BcdUsb;
    Context->Speed = Speed;
    Context->ForHub = TRUE;
}

/* Name of a validation code for the debug log */
#define HUB_DESC_NAME(Code) case HubDescCode::Code: return #Code

PCSTR
NTAPI
HubDescCodeName(
    _In_ HubDescCode Code)
{
    switch (Code)
    {
        HUB_DESC_NAME(Hub20LengthTooLarge);
        HUB_DESC_NAME(Hub20LengthTooSmall);
        HUB_DESC_NAME(Hub20Null);
        HUB_DESC_NAME(Hub20BufferTooSmall);
        HUB_DESC_NAME(Hub20RemovableMissingPort);
        HUB_DESC_NAME(Hub20RemovableReserved);
        HUB_DESC_NAME(Hub20WrongType);
        HUB_DESC_NAME(Hub20NoPorts);
        HUB_DESC_NAME(Hub20PowerMaskNotOnes);
        HUB_DESC_NAME(Hub20CharacteristicsReserved);
        HUB_DESC_NAME(Hub30DecodeLatencyReserved);
        HUB_DESC_NAME(Hub30LengthTooLarge);
        HUB_DESC_NAME(Hub30LengthTooSmall);
        HUB_DESC_NAME(Hub30BufferTooSmall);
        HUB_DESC_NAME(Hub30RemovableMissingPort);
        HUB_DESC_NAME(Hub30RemovableReserved);
        HUB_DESC_NAME(Hub30WrongType);
        HUB_DESC_NAME(Hub30TooManyPorts);
        HUB_DESC_NAME(Hub30NoPorts);
        HUB_DESC_NAME(Hub30CharacteristicsReserved);
        HUB_DESC_NAME(BosLengthTooLarge);
        HUB_DESC_NAME(BosLengthTooSmall);
        HUB_DESC_NAME(BosBufferLargerThanTotal);
        HUB_DESC_NAME(BosBufferTooSmall);
        HUB_DESC_NAME(BosWrongType);
        HUB_DESC_NAME(BosCapCountMismatch);
        HUB_DESC_NAME(BosNoSuperSpeedCap);
        HUB_DESC_NAME(BosNull);
        HUB_DESC_NAME(BosUnknownType);
        HUB_DESC_NAME(BosTotalLengthTooSmall);
        HUB_DESC_NAME(BulkAtLowSpeed);
        HUB_DESC_NAME(BulkAttributesReserved);
        HUB_DESC_NAME(BulkMaxPacket);
        HUB_DESC_NAME(CompanionBulkReserved);
        HUB_DESC_NAME(CompanionBulkBurst);
        HUB_DESC_NAME(CompanionBulkStreams);
        HUB_DESC_NAME(CompanionBulkBytesPerInterval);
        HUB_DESC_NAME(CompanionControlAttributes);
        HUB_DESC_NAME(CompanionControlBurst);
        HUB_DESC_NAME(CompanionControlBytesPerInterval);
        HUB_DESC_NAME(CompanionNotSuperSpeed);
        HUB_DESC_NAME(CompanionUnexpected);
        HUB_DESC_NAME(CompanionLengthTooLarge);
        HUB_DESC_NAME(CompanionLengthTooSmall);
        HUB_DESC_NAME(CompanionBufferTooSmall);
        HUB_DESC_NAME(CompanionInterruptBurst);
        HUB_DESC_NAME(CompanionInterruptBurstPacket);
        HUB_DESC_NAME(CompanionInterruptAttributes);
        HUB_DESC_NAME(CompanionInterruptBytesPerInterval);
        HUB_DESC_NAME(CompanionIsochReserved);
        HUB_DESC_NAME(CompanionIsochBurst);
        HUB_DESC_NAME(CompanionIsochBurstPacket);
        HUB_DESC_NAME(CompanionIsochMult);
        HUB_DESC_NAME(CompanionIsochBytesPerInterval);
        HUB_DESC_NAME(ConfigLengthTooLarge);
        HUB_DESC_NAME(ConfigLengthTooSmall);
        HUB_DESC_NAME(ConfigBufferTooSmall);
        HUB_DESC_NAME(ConfigBufferTooSmallForInterfaces);
        HUB_DESC_NAME(ConfigWrongType);
        HUB_DESC_NAME(ConfigMissingCompanion);
        HUB_DESC_NAME(ConfigNull);
        HUB_DESC_NAME(ConfigTotalLengthTooLarge);
        HUB_DESC_NAME(ConfigTotalLengthTooSmall);
        HUB_DESC_NAME(ContainerIdLengthTooLarge);
        HUB_DESC_NAME(ContainerIdLengthTooSmall);
        HUB_DESC_NAME(ContainerIdBufferTooSmall);
        HUB_DESC_NAME(ContainerIdReserved);
        HUB_DESC_NAME(ControlAttributesReserved);
        HUB_DESC_NAME(ControlMaxPacket);
        HUB_DESC_NAME(DeviceVersionNotBcd);
        HUB_DESC_NAME(DeviceLengthTooSmall);
        HUB_DESC_NAME(DeviceBufferTooSmall);
        HUB_DESC_NAME(CapLengthTooSmall);
        HUB_DESC_NAME(CapBufferTooSmall);
        HUB_DESC_NAME(CapMultipleContainerId);
        HUB_DESC_NAME(CapMultipleSuperSpeed);
        HUB_DESC_NAME(CapMultipleUsb20);
        HUB_DESC_NAME(DeviceWrongType);
        HUB_DESC_NAME(DeviceMaxPacket0);
        HUB_DESC_NAME(DeviceNull);
        HUB_DESC_NAME(EndpointLengthTooLarge);
        HUB_DESC_NAME(EndpointLengthTooSmall);
        HUB_DESC_NAME(EndpointAttributesReserved);
        HUB_DESC_NAME(EndpointBufferTooSmall);
        HUB_DESC_NAME(EndpointAddressReserved);
        HUB_DESC_NAME(EndpointBeforeInterface);
        HUB_DESC_NAME(EndpointNumberZero);
        HUB_DESC_NAME(HeaderPastBuffer);
        HUB_DESC_NAME(HeaderPastTotalLength);
        HUB_DESC_NAME(HeaderLengthTooSmall);
        HUB_DESC_NAME(HeaderBufferTooSmall);
        HUB_DESC_NAME(HeaderLengthZero);
        HUB_DESC_NAME(IadLengthTooLarge);
        HUB_DESC_NAME(IadLengthTooSmall);
        HUB_DESC_NAME(IadBufferTooSmall);
        HUB_DESC_NAME(IadCountTooLarge);
        HUB_DESC_NAME(IadCountZero);
        HUB_DESC_NAME(IadFirstInterfaceTooLarge);
        HUB_DESC_NAME(InterfaceLengthTooLarge);
        HUB_DESC_NAME(InterfaceLengthTooSmall);
        HUB_DESC_NAME(InterfaceBufferTooSmall);
        HUB_DESC_NAME(DuplicateAlternate);
        HUB_DESC_NAME(DuplicateEndpoint);
        HUB_DESC_NAME(DuplicateInterface);
        HUB_DESC_NAME(FirstAlternateNotZero);
        HUB_DESC_NAME(AlternateOutOfOrder);
        HUB_DESC_NAME(InterfaceOutOfOrder);
        HUB_DESC_NAME(EndpointCountMismatch);
        HUB_DESC_NAME(InterruptAttributesReserved);
        HUB_DESC_NAME(InterruptInterval);
        HUB_DESC_NAME(InterruptMaxPacket);
        HUB_DESC_NAME(IsochAtLowSpeed);
        HUB_DESC_NAME(IsochInterval);
        HUB_DESC_NAME(IsochMaxPacket);
        HUB_DESC_NAME(MsOsContainerIdZero);
        HUB_DESC_NAME(MsOsContainerIdVersion);
        HUB_DESC_NAME(MsOsContainerIdIndex);
        HUB_DESC_NAME(MsOsContainerIdBufferSize);
        HUB_DESC_NAME(MsOsContainerIdLength);
        HUB_DESC_NAME(MsOsExtConfigFirstInterface);
        HUB_DESC_NAME(MsOsExtConfigVersion);
        HUB_DESC_NAME(MsOsExtConfigBufferSize);
        HUB_DESC_NAME(MsOsExtConfigCompatibleId);
        HUB_DESC_NAME(MsOsExtConfigSubCompatibleId);
        HUB_DESC_NAME(MsOsExtConfigIndex);
        HUB_DESC_NAME(MsOsExtConfigBufferVsLength);
        HUB_DESC_NAME(StringLengthOdd);
        HUB_DESC_NAME(StringLengthTooLarge);
        HUB_DESC_NAME(StringLengthTooSmall);
        HUB_DESC_NAME(StringBufferTooSmall);
        HUB_DESC_NAME(SerialNumberBadCharacter);
        HUB_DESC_NAME(StringWrongType);
        HUB_DESC_NAME(StringLengthVsBytes);
        HUB_DESC_NAME(SsCapLengthTooLarge);
        HUB_DESC_NAME(SsCapLengthTooSmall);
        HUB_DESC_NAME(SsCapAttributesReserved);
        HUB_DESC_NAME(SsCapU1TooLarge);
        HUB_DESC_NAME(SsCapU2TooLarge);
        HUB_DESC_NAME(SsCapBufferTooSmall);
        HUB_DESC_NAME(SsCapFunctionalityNotInSpeeds);
        HUB_DESC_NAME(SsCapFunctionalityReserved);
        HUB_DESC_NAME(SsCapSpeedsReserved);
        HUB_DESC_NAME(SsCapNoSpeeds);
        HUB_DESC_NAME(Usb20CapLengthTooLarge);
        HUB_DESC_NAME(Usb20CapLengthTooSmall);
        HUB_DESC_NAME(Usb20CapReserved);
        HUB_DESC_NAME(Usb20CapBufferTooSmall);
        HUB_DESC_NAME(BaselineBeslZero);
        HUB_DESC_NAME(BeslWithoutLpm);
        HUB_DESC_NAME(DeepBeslNotAboveBaseline);
        HUB_DESC_NAME(DeepBeslZero);
        HUB_DESC_NAME(BosContainerIdZero);
        HUB_DESC_NAME(MsOs20MultipleSetHeaders);
        HUB_DESC_NAME(MsOs20SetHeaderLength);
        HUB_DESC_NAME(ConfigSubsetLength);
        HUB_DESC_NAME(ConfigSubsetTotalTooSmall);
        HUB_DESC_NAME(ConfigSubsetTotalTooLarge);
        HUB_DESC_NAME(ConfigSubsetMisplacedFeature);
        HUB_DESC_NAME(ConfigSubsetFeatureLength);
        HUB_DESC_NAME(FunctionSubsetLength);
        HUB_DESC_NAME(FunctionSubsetTotalTooSmall);
        HUB_DESC_NAME(FunctionSubsetTotalTooLarge);
        HUB_DESC_NAME(FunctionSubsetMisplacedFeature);
        HUB_DESC_NAME(FunctionSubsetFeatureLength);
        HUB_DESC_NAME(MsOs20MultipleCompatibleId);
        HUB_DESC_NAME(MsOs20CompatibleIdLength);
        HUB_DESC_NAME(MsOs20CompatibleIdAfterNull);
        HUB_DESC_NAME(MsOs20RegistryTooSmall);
        HUB_DESC_NAME(MsOs20RegistryNameTooLarge);
        HUB_DESC_NAME(MsOs20RegistryNameOddOrZero);
        HUB_DESC_NAME(MsOs20RegistryDataTooLarge);
        HUB_DESC_NAME(MsOs20RegistryDataZero);
        HUB_DESC_NAME(MsOs20RegistryType);
        HUB_DESC_NAME(MsOs20MultipleResumeTime);
        HUB_DESC_NAME(MsOs20ResumeTimeLength);
        HUB_DESC_NAME(MsOs20ResumeRecovery);
        HUB_DESC_NAME(MsOs20ResumeSignaling);
        HUB_DESC_NAME(MsOs20MultipleModelId);
        HUB_DESC_NAME(MsOs20ModelIdLength);
        HUB_DESC_NAME(MsOs20ModelIdZero);
        HUB_DESC_NAME(MsOs20BytesVsSetInfo);
        HUB_DESC_NAME(MsOs20BytesTooSmall);
        HUB_DESC_NAME(MsOs20HeaderLength);
        HUB_DESC_NAME(MsOs20TotalLengthMismatch);
        HUB_DESC_NAME(MsOs20VersionMismatch);
        HUB_DESC_NAME(MsOs20FeatureOutOfOrder);
        HUB_DESC_NAME(MsOs20PastEnd);
        HUB_DESC_NAME(PlatformReserved);
        HUB_DESC_NAME(PlatformUuidZero);
        HUB_DESC_NAME(MsOs20MultiplePlatform);
        HUB_DESC_NAME(MsOs20PlatformLength);
        HUB_DESC_NAME(MsOs20DuplicateVersion);
        HUB_DESC_NAME(MsOs20NoVersionMatch);
        HUB_DESC_NAME(MsOs20MultipleCcgp);
        HUB_DESC_NAME(MsOs20CcgpLength);
        HUB_DESC_NAME(CapMultipleSuperSpeedPlus);
        HUB_DESC_NAME(CompanionIsochBytesNotOne);
        HUB_DESC_NAME(SspCompanionNotSuperSpeed);
        HUB_DESC_NAME(SspCompanionUnexpected);
        HUB_DESC_NAME(SspCompanionLengthTooLarge);
        HUB_DESC_NAME(SspCompanionLengthTooSmall);
        HUB_DESC_NAME(SspCompanionBufferTooSmall);
        HUB_DESC_NAME(SspCompanionReserved);
        HUB_DESC_NAME(SspCompanionBytesRange);
        HUB_DESC_NAME(ConfigMissingSspCompanion);
        HUB_DESC_NAME(SspCapLengthInvalid);
        HUB_DESC_NAME(SspCapLengthTooSmall);
        HUB_DESC_NAME(SspCapAttributesReserved);
        HUB_DESC_NAME(SspCapFunctionalityReserved);
        HUB_DESC_NAME(SspCapBufferTooSmall);
        HUB_DESC_NAME(SspCapReservedByte);
        HUB_DESC_NAME(SspCapReservedWord);
        HUB_DESC_NAME(SspAttrNotRx);
        HUB_DESC_NAME(SspAttrNotTx);
        HUB_DESC_NAME(SspAttrTxMissing);
        HUB_DESC_NAME(SspAttrPairMismatch);
        HUB_DESC_NAME(SspAttrSymmetricMismatch);
        HUB_DESC_NAME(SspMinLaneMismatch);
        HUB_DESC_NAME(SspSpeedIdDuplicate);
        HUB_DESC_NAME(SspMinSpeedMissing);
        HUB_DESC_NAME(PdLengthTooLarge);
        HUB_DESC_NAME(PdLengthTooSmall);
        HUB_DESC_NAME(PdBufferTooSmall);
        HUB_DESC_NAME(PdReserved);
        HUB_DESC_NAME(IsochPacketInAlternate0);
        HUB_DESC_NAME(DuplicateSerialNumber);
        HUB_DESC_NAME(BillboardLengthTooSmall);
        HUB_DESC_NAME(BillboardLengthInvalid);
        HUB_DESC_NAME(BillboardBufferTooSmall);
        HUB_DESC_NAME(BillboardNoModes);
        HUB_DESC_NAME(BillboardTooManyModes);
        HUB_DESC_NAME(BillboardPreferredInvalid);
        HUB_DESC_NAME(CapMultipleBillboard);
        HUB_DESC_NAME(BillboardDeviceVersion);
        HUB_DESC_NAME(PlatformFeaturesLength);
        HUB_DESC_NAME(PlatformFeaturesMultiple);
        HUB_DESC_NAME(PlatformFeaturesVersion);
        HUB_DESC_NAME(PlatformFeaturesTooSmall);
        HUB_DESC_NAME(PlatformLengthTooSmall);
        HUB_DESC_NAME(PlatformBufferTooSmall);

        default:
            return "Unknown";
    }
}

#undef HUB_DESC_NAME

VOID
NTAPI
HubDescLog(
    _In_ const HubDescContext* Context,
    _In_ HubDescCode Code,
    _In_ BOOLEAN Warning)
{
    ULONG Bit = (ULONG)Code;

    /* Warnings set the same bit as errors */
    if (Context->Bitmap != NULL && Bit < HUB_DESC_BITMAP_ULONGS * 32)
        Context->Bitmap[Bit / 32] |= 1UL << (Bit % 32);

    if (Context->Log != NULL)
    {
        Context->Log(Context->LogContext, Code, Warning);
        return;
    }

    /* Warning also covers findings whose gate does not apply to this device */
    if (Warning)
    {
        DPRINT("%s %p descriptor check %s (%lu) ignored\n",
               Context->ForHub ? "Hub" : "Device", Context->LogContext, HubDescCodeName(Code), Bit);
    }
    else
    {
        DPRINT1("%s %p descriptor check %s (%lu) failed\n",
                Context->ForHub ? "Hub" : "Device", Context->LogContext, HubDescCodeName(Code), Bit);
    }
}

/* Shared helpers */

static
BOOLEAN
NTAPI
GateApplies(
    _In_ const HubDescContext* Ctx,
    _In_ Gate Kind)
{
    USHORT Bcd = Ctx->BcdUsb;
    BOOLEAN Newer = Ctx->Strictest || Bcd > 0x0200;

    switch (Kind)
    {
        case Gate::Always:
            return TRUE;

        case Gate::Legacy:
            return Newer || Ctx->LegacyStrict;

        case Gate::NewRule:
            return Newer;

        case Gate::Versioned:
            return Ctx->Strictest ||
                   (Bcd >= 0x0201 && Bcd <= 0x0210) ||
                   (Bcd >= 0x0300 && Bcd <= 0x0310);

        /* QUIRK: 0x0210 itself is excluded for the USB 2.0 extension */
        case Gate::Cap20:
            return Ctx->Strictest || (Bcd >= 0x0201 && Bcd < 0x0210);

        case Gate::CapSs:
            return Ctx->Strictest || (Bcd >= 0x0300 && Bcd <= 0x0310);

        case Gate::CapSsp:
            return Ctx->Strictest || Bcd == 0x0310;

        case Gate::Reserved:
            return Ctx->ReservedFields;

        default:
            return FALSE;
    }
}

/** Logs a finding and fails the descriptor when the gate is enforced. */
static
VOID
NTAPI
Note(
    _In_ const HubDescContext* Ctx,
    _Inout_ Verdict* Result,
    _In_ HubDescCode Code,
    _In_ Gate Kind)
{
    BOOLEAN Enforced = GateApplies(Ctx, Kind);

    /* Only an enforced finding prints as an error */
    HubDescLog(Ctx, Code, !Enforced);
    if (Enforced)
        *Result = Verdict::Failed;
}

/** Logs a finding the walk cannot continue past. */
static
VOID
NTAPI
NoteFatal(
    _In_ const HubDescContext* Ctx,
    _Inout_ Verdict* Result,
    _In_ HubDescCode Code)
{
    HubDescLog(Ctx, Code, FALSE);
    *Result = Verdict::Fatal;
}

/** In compatibility mode each check starts its caller's slot over, forgiving earlier findings. */
static
VOID
NTAPI
StartCheck(
    _In_ const HubDescContext* Ctx,
    _Inout_ Verdict* Result)
{
    if (Ctx->Win8Behavior)
        *Result = Verdict::Ok;
}

/** Uses the standard size for a short descriptor when that many bytes remain. FALSE means stop. */
static
BOOLEAN
NTAPI
AssumeSize(
    _In_ const HubDescContext* Ctx,
    _Inout_ Verdict* Result,
    _In_ HubDescCode Code,
    _In_ Gate Kind,
    _In_ ULONG Size,
    _In_ ULONG Left,
    _Inout_ PULONG Advance)
{
    Note(Ctx, Result, Code, Kind);
    if (Size > Left)
    {
        *Result = Verdict::Fatal;
        return FALSE;
    }

    *Advance = Size;
    return TRUE;
}

static
BOOLEAN
NTAPI
BitIsSet(
    _In_reads_(8) const ULONG* Bits,
    _In_ UCHAR Index)
{
    return (Bits[Index / 32] & (1UL << (Index % 32))) != 0;
}

static
VOID
NTAPI
BitSet(
    _Inout_updates_(8) PULONG Bits,
    _In_ UCHAR Index)
{
    Bits[Index / 32] |= 1UL << (Index % 32);
}

static
BOOLEAN
NTAPI
IsAllZero(
    _In_reads_bytes_(Length) const VOID* Buffer,
    _In_ ULONG Length)
{
    const UCHAR* Bytes = (const UCHAR*)Buffer;
    ULONG i;

    for (i = 0; i < Length; i++)
    {
        if (Bytes[i] != 0)
            return FALSE;
    }

    return TRUE;
}

/** One of the packet sizes full speed allows for control and bulk. */
static
BOOLEAN
NTAPI
IsSmallPacket(
    _In_ ULONG Size)
{
    return Size == 8 || Size == 16 || Size == 32 || Size == 64;
}

/** Bytes from Descriptor to the end of the buffer the caller handed in. */
static
ULONG
NTAPI
BytesLeft(
    _In_ PUCHAR Base,
    _In_ ULONG BufferLength,
    _In_ const VOID* Descriptor)
{
    return BufferLength - (ULONG)((const UCHAR*)Descriptor - Base);
}

/* Common header of every descriptor inside a configuration or BOS set */
static
VOID
NTAPI
CheckHeader(
    _In_ const HubDescContext* Ctx,
    _In_ PUCHAR Base,
    _In_ ULONG BufferLength,
    _In_ PUCHAR End,
    _In_ PUSB_COMMON_DESCRIPTOR Desc,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    ULONG Left = BytesLeft(Base, BufferLength, Desc);

    StartCheck(Ctx, Result);
    if (Left < sizeof(*Desc))
    {
        *Advance = 0;
        NoteFatal(Ctx, Result, HubDescCode::HeaderBufferTooSmall);
        return;
    }

    *Advance = Desc->bLength;
    if (Desc->bLength > Left)
    {
        Note(Ctx, Result, HubDescCode::HeaderPastBuffer, Gate::Always);
        *Advance = Left;
    }

    if (Desc->bLength > (ULONG)(End - (PUCHAR)Desc))
        Note(Ctx, Result, HubDescCode::HeaderPastTotalLength, Gate::Always);

    if (Desc->bLength < sizeof(*Desc))
        Note(Ctx, Result, HubDescCode::HeaderLengthTooSmall, Gate::NewRule);

    if (Desc->bLength == 0)
    {
        HubDescLog(Ctx, HubDescCode::HeaderLengthZero, !GateApplies(Ctx, Gate::NewRule));
        if (GateApplies(Ctx, Gate::NewRule))
            *Result = Verdict::Fatal;
    }
}

/* Configuration descriptor set */

static
VOID
NTAPI
CheckConfigFixedPart(
    _Inout_ ConfigWalk* Walk,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    PUSB_CONFIGURATION_DESCRIPTOR Config = Walk->Config;
    ULONG Needed;

    /* A short or mistyped header is Fatal, but a later finding can still make it Failed */
    if (Config->bLength < sizeof(*Config))
        NoteFatal(Ctx, Result, HubDescCode::ConfigLengthTooSmall);

    if (Config->bLength > sizeof(*Config))
        Note(Ctx, Result, HubDescCode::ConfigLengthTooLarge, Gate::NewRule);

    if (Config->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE)
        NoteFatal(Ctx, Result, HubDescCode::ConfigWrongType);

    if (Walk->BufferLength < sizeof(*Config))
    {
        Note(Ctx, Result, HubDescCode::ConfigBufferTooSmall, Gate::Legacy);
        return;
    }

    Needed = sizeof(*Config) + Config->bNumInterfaces * sizeof(USB_INTERFACE_DESCRIPTOR);
    if (Needed > Walk->BufferLength)
        Note(Ctx, Result, HubDescCode::ConfigBufferTooSmallForInterfaces, Gate::Legacy);

    Walk->End = Walk->Base + Config->wTotalLength;
    if (Config->wTotalLength > Walk->BufferLength)
    {
        Note(Ctx, Result, HubDescCode::ConfigTotalLengthTooLarge, Gate::Legacy);
        Walk->End = Walk->Base + Walk->BufferLength;
    }

    if (Config->wTotalLength < Needed)
    {
        Note(Ctx, Result, HubDescCode::ConfigTotalLengthTooSmall, Gate::Legacy);
        Walk->End = Walk->Base + Walk->BufferLength;
    }
}

/** Companions the previous endpoint needed. Also resets the companion state. */
static
VOID
NTAPI
CheckPreviousEndpoint(
    _Inout_ ConfigWalk* Walk,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;

    StartCheck(Ctx, Result);
    if (!Walk->HadEndpoint)
    {
        Walk->HadEndpoint = TRUE;
    }
    else if (Ctx->Speed == UsbSuperSpeed)
    {
        if (!Walk->GotCompanion)
            Note(Ctx, Result, HubDescCode::ConfigMissingCompanion, Gate::Always);

        if (Walk->WantSspCompanion && !Walk->GotSspCompanion)
            Note(Ctx, Result, HubDescCode::ConfigMissingSspCompanion, Gate::Always);
    }

    Walk->GotCompanion = FALSE;
    Walk->WantCompanion = FALSE;
    Walk->GotSspCompanion = FALSE;
    Walk->WantSspCompanion = FALSE;
}

/** Wraps up the previous interface: its last endpoint and its endpoint count. */
static
VOID
NTAPI
CheckPreviousInterface(
    _Inout_ ConfigWalk* Walk,
    _Inout_ Verdict* Result)
{
    StartCheck(Walk->Ctx, Result);
    if (!Walk->HadInterface)
    {
        Walk->HadInterface = TRUE;
        return;
    }

    CheckPreviousEndpoint(Walk, Result);
    if (*Result == Verdict::Fatal)
        return;

    /* QUIRK: one code for too few and too many endpoints */
    if (Walk->EndpointsCounted != Walk->EndpointsDeclared)
        Note(Walk->Ctx, Result, HubDescCode::EndpointCountMismatch, Gate::Legacy);
}

static
VOID
NTAPI
CheckInterface(
    _Inout_ ConfigWalk* Walk,
    _In_ PUSB_INTERFACE_DESCRIPTOR Desc,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Desc);
    BOOLEAN First;
    BOOLEAN NewNumber;
    UCHAR Number;
    UCHAR Alternate;

    StartCheck(Ctx, Result);
    *Advance = Desc->bLength;

    if (Desc->bLength < sizeof(*Desc) &&
        !AssumeSize(Ctx, Result, HubDescCode::InterfaceLengthTooSmall, Gate::Legacy, sizeof(*Desc), Left, Advance))
    {
        return;
    }

    if (Desc->bLength > sizeof(*Desc))
        Note(Ctx, Result, HubDescCode::InterfaceLengthTooLarge, Gate::Legacy);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::InterfaceBufferTooSmall, Gate::Legacy);
        return;
    }

    Number = Desc->bInterfaceNumber;
    Alternate = Desc->bAlternateSetting;
    First = !Walk->HadInterface;
    NewNumber = First || Number != Walk->Interface;

    /* In compatibility mode this forgives the length findings above */
    CheckPreviousInterface(Walk, Result);
    if (*Result == Verdict::Fatal)
        return;

    if (NewNumber)
    {
        if (BitIsSet(Walk->InterfaceSeen, Number))
            Note(Ctx, Result, HubDescCode::DuplicateInterface, Gate::Legacy);

        if (!First && Walk->Interface > Number)
            Note(Ctx, Result, HubDescCode::InterfaceOutOfOrder, Gate::Legacy);

        if (Alternate != 0)
            Note(Ctx, Result, HubDescCode::FirstAlternateNotZero, Gate::Legacy);

        RtlZeroMemory(Walk->AlternateSeen, sizeof(Walk->AlternateSeen));
        BitSet(Walk->InterfaceSeen, Number);
        Walk->InterfacesFound++;
        Walk->Interface = Number;
    }
    else
    {
        if (BitIsSet(Walk->AlternateSeen, Alternate))
            Note(Ctx, Result, HubDescCode::DuplicateAlternate, Gate::Legacy);

        if (Walk->Alternate >= Alternate)
            Note(Ctx, Result, HubDescCode::AlternateOutOfOrder, Gate::Legacy);
    }

    RtlZeroMemory(Walk->EndpointSeen, sizeof(Walk->EndpointSeen));
    Walk->EndpointsCounted = 0;
    Walk->EndpointsDeclared = Desc->bNumEndpoints;
    Walk->HadEndpoint = FALSE;
    BitSet(Walk->AlternateSeen, Alternate);
    Walk->Alternate = Alternate;
}

/* Packet size, interval and attribute rules per transfer type and speed */
static
VOID
NTAPI
CheckEndpointType(
    _In_ ConfigWalk* Walk,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Desc,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    USB_DEVICE_SPEED Speed = Ctx->Speed;
    ULONG Packet = Desc->wMaxPacketSize;
    ULONG Size = Packet & 0x7FF;
    ULONG Mult = (Packet >> 11) & 0x3;
    ULONG Spare = Packet >> 13;
    UCHAR Extra = Desc->bmAttributes & ~USB_ENDPOINT_TYPE_MASK;
    BOOLEAN BadInterval = Desc->bInterval == 0 || Desc->bInterval > 16;

    switch (Desc->bmAttributes & USB_ENDPOINT_TYPE_MASK)
    {
        case USB_ENDPOINT_TYPE_CONTROL:
            if (Extra != 0)
                Note(Ctx, Result, HubDescCode::ControlAttributesReserved, Gate::Reserved);

            if (Speed == UsbLowSpeed && Packet != 8)
            {
                Note(Ctx, Result, HubDescCode::ControlMaxPacket,
                     (Packet > 8 || Packet == 0) ? Gate::Always : Gate::Versioned);
            }
            else if (Speed == UsbFullSpeed && !IsSmallPacket(Packet))
            {
                Note(Ctx, Result, HubDescCode::ControlMaxPacket, Packet == 0 ? Gate::Always : Gate::Versioned);
            }
            else if ((Speed == UsbHighSpeed && Packet != 64) || (Speed == UsbSuperSpeed && Packet != 512))
            {
                Note(Ctx, Result, HubDescCode::ControlMaxPacket, Gate::Versioned);
            }
            break;

        case USB_ENDPOINT_TYPE_INTERRUPT:
            /* SuperSpeed interrupt attribute bits are not looked at */
            if (Speed != UsbSuperSpeed && Extra != 0)
                Note(Ctx, Result, HubDescCode::InterruptAttributesReserved, Gate::Reserved);

            if (Speed == UsbLowSpeed)
            {
                if (Packet > 8)
                    Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Always);

                if (Packet == 0)
                    Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Always);
            }
            else if (Speed == UsbFullSpeed)
            {
                if (Packet > 64)
                    Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Always);
            }
            else if (Speed == UsbHighSpeed)
            {
                if (Size > 1024)
                    Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Versioned);

                if (Spare != 0)
                    Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Versioned);

                if (Mult == 3)
                    Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Versioned);
            }
            else if (Speed == UsbSuperSpeed && Packet > 1024)
            {
                Note(Ctx, Result, HubDescCode::InterruptMaxPacket, Gate::Versioned);
            }

            /* High speed intervals are not looked at */
            if ((Speed == UsbLowSpeed || Speed == UsbFullSpeed) && Desc->bInterval == 0)
                Note(Ctx, Result, HubDescCode::InterruptInterval, Gate::Versioned);
            break;

        case USB_ENDPOINT_TYPE_BULK:
            if (Extra != 0)
                Note(Ctx, Result, HubDescCode::BulkAttributesReserved, Gate::Reserved);

            if (Speed == UsbLowSpeed)
                Note(Ctx, Result, HubDescCode::BulkAtLowSpeed, Gate::Always);
            else if (Speed == UsbFullSpeed && !IsSmallPacket(Packet))
                Note(Ctx, Result, HubDescCode::BulkMaxPacket, Packet == 0 ? Gate::Always : Gate::Versioned);
            else if ((Speed == UsbHighSpeed && Packet != 512) || (Speed == UsbSuperSpeed && Packet != 1024))
                Note(Ctx, Result, HubDescCode::BulkMaxPacket, Gate::Versioned);
            break;

        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            /* Logged, never enforced */
            if (Walk->Alternate == 0 && Packet > 0)
                Note(Ctx, Result, HubDescCode::IsochPacketInAlternate0, Gate::LogOnly);

            if (Speed == UsbLowSpeed)
            {
                Note(Ctx, Result, HubDescCode::IsochAtLowSpeed, Gate::Always);
                break;
            }

            if (Speed == UsbFullSpeed)
            {
                if (Packet > 1023)
                    Note(Ctx, Result, HubDescCode::IsochMaxPacket, Gate::Always);
            }
            else if (Speed == UsbHighSpeed)
            {
                if (Size > 1024)
                    Note(Ctx, Result, HubDescCode::IsochMaxPacket, Gate::Versioned);

                if (Spare != 0)
                    Note(Ctx, Result, HubDescCode::IsochMaxPacket, Gate::Versioned);

                if (Mult == 3)
                    Note(Ctx, Result, HubDescCode::IsochMaxPacket, Gate::Versioned);
            }
            else if (Speed == UsbSuperSpeed)
            {
                if (Packet > 1024)
                    Note(Ctx, Result, HubDescCode::IsochMaxPacket, Gate::Versioned);
            }
            else
            {
                break;
            }

            if (BadInterval)
                Note(Ctx, Result, HubDescCode::IsochInterval, Gate::Versioned);
            break;
    }
}

static
VOID
NTAPI
CheckEndpoint(
    _Inout_ ConfigWalk* Walk,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Desc,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Desc);

    StartCheck(Ctx, Result);
    *Advance = Desc->bLength;

    CheckPreviousEndpoint(Walk, Result);
    if (*Result == Verdict::Fatal)
        return;

    if (Desc->bLength < sizeof(*Desc) &&
        !AssumeSize(Ctx, Result, HubDescCode::EndpointLengthTooSmall, Gate::Legacy, sizeof(*Desc), Left, Advance))
    {
        return;
    }

    /* Audio class endpoints are 9 bytes long */
    if (Desc->bLength > sizeof(*Desc))
        Note(Ctx, Result, HubDescCode::EndpointLengthTooLarge, Gate::Warning);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::EndpointBufferTooSmall, Gate::Legacy);
        return;
    }

    if (!Walk->HadInterface)
    {
        Note(Ctx, Result, HubDescCode::EndpointBeforeInterface, Gate::Legacy);
        return;
    }

    if ((Desc->bEndpointAddress & 0x0F) == 0)
        Note(Ctx, Result, HubDescCode::EndpointNumberZero, Gate::NewRule);

    if (Desc->bEndpointAddress & 0x70)
        Note(Ctx, Result, HubDescCode::EndpointAddressReserved, Gate::Versioned);

    if (Desc->bmAttributes & 0xC0)
        Note(Ctx, Result, HubDescCode::EndpointAttributesReserved, Gate::Reserved);

    CheckEndpointType(Walk, Desc, Result);

    if (BitIsSet(Walk->EndpointSeen, Desc->bEndpointAddress))
        Note(Ctx, Result, HubDescCode::DuplicateEndpoint, Gate::Legacy);

    BitSet(Walk->EndpointSeen, Desc->bEndpointAddress);
    Walk->EndpointsCounted++;
    Walk->GotCompanion = FALSE;
    Walk->WantCompanion = TRUE;
    Walk->LastEndpoint = Desc;
}

static
VOID
NTAPI
CheckCompanion(
    _Inout_ ConfigWalk* Walk,
    _In_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Desc,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Desc);
    PUSB_ENDPOINT_DESCRIPTOR Endpoint;
    ULONG Packet;
    ULONG Burst;
    ULONG Attributes;
    ULONG Mult;
    ULONG Ceiling;
    BOOLEAN SspFollows;

    StartCheck(Ctx, Result);
    *Advance = Desc->bLength;

    /* QUIRK: so the output only reflects the last companion in the set */
    if (Walk->SupportsStreams != NULL)
        *Walk->SupportsStreams = FALSE;

    /* USB 2.0 PHDC class descriptors share this type */
    if (Ctx->Speed != UsbSuperSpeed)
    {
        Note(Ctx, Result, HubDescCode::CompanionNotSuperSpeed, Gate::LogOnly);
        return;
    }

    if (Desc->bLength < sizeof(*Desc) &&
        !AssumeSize(Ctx, Result, HubDescCode::CompanionLengthTooSmall, Gate::Always, sizeof(*Desc), Left, Advance))
    {
        return;
    }

    if (Desc->bLength > sizeof(*Desc))
        Note(Ctx, Result, HubDescCode::CompanionLengthTooLarge, Gate::Versioned);

    if (!Walk->WantCompanion)
    {
        Note(Ctx, Result, HubDescCode::CompanionUnexpected, Gate::Always);
        return;
    }

    Walk->GotCompanion = TRUE;
    Walk->WantCompanion = FALSE;

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::CompanionBufferTooSmall, Gate::NewRule);
        return;
    }

    Endpoint = Walk->LastEndpoint;
    Packet = Endpoint->wMaxPacketSize;
    Burst = Desc->bMaxBurst;
    Attributes = Desc->bmAttributes.AsUchar;
    Mult = Attributes & 0x3;
    Ceiling = Packet * (Burst + 1) * (Mult + 1);

    switch (Endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK)
    {
        case USB_ENDPOINT_TYPE_CONTROL:
            if (Burst != 0)
                Note(Ctx, Result, HubDescCode::CompanionControlBurst, Gate::Versioned);

            if (Attributes != 0)
                Note(Ctx, Result, HubDescCode::CompanionControlAttributes, Gate::Reserved);

            if (Desc->wBytesPerInterval != 0)
                Note(Ctx, Result, HubDescCode::CompanionControlBytesPerInterval, Gate::Versioned);
            break;

        case USB_ENDPOINT_TYPE_BULK:
            if (Burst > 15)
                Note(Ctx, Result, HubDescCode::CompanionBulkBurst, Gate::Versioned);

            if ((Attributes & 0x1F) != 0 && Walk->SupportsStreams != NULL)
                *Walk->SupportsStreams = TRUE;

            if ((Attributes & 0x1F) > 16)
                Note(Ctx, Result, HubDescCode::CompanionBulkStreams, Gate::Versioned);

            if (Attributes & 0xE0)
                Note(Ctx, Result, HubDescCode::CompanionBulkReserved, Gate::Reserved);

            if (Desc->wBytesPerInterval != 0)
                Note(Ctx, Result, HubDescCode::CompanionBulkBytesPerInterval, Gate::Versioned);
            break;

        case USB_ENDPOINT_TYPE_INTERRUPT:
            if (Burst > 15)
                Note(Ctx, Result, HubDescCode::CompanionInterruptBurst, Gate::Versioned);

            if (Burst != 0 && Packet != 1024)
                Note(Ctx, Result, HubDescCode::CompanionInterruptBurstPacket, Gate::Versioned);

            if (Attributes != 0)
                Note(Ctx, Result, HubDescCode::CompanionInterruptAttributes, Gate::Reserved);

            if (Desc->wBytesPerInterval > Ceiling)
                Note(Ctx, Result, HubDescCode::CompanionInterruptBytesPerInterval, Gate::Warning);
            break;

        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            SspFollows = (Attributes & 0x80) != 0;
            Walk->GotSspCompanion = FALSE;
            Walk->WantSspCompanion = SspFollows;

            if (Burst > 15)
                Note(Ctx, Result, HubDescCode::CompanionIsochBurst, Gate::Versioned);

            if (Burst != 0 && Packet != 1024)
                Note(Ctx, Result, HubDescCode::CompanionIsochBurstPacket, Gate::Versioned);

            if (!SspFollows && Mult > 2)
                Note(Ctx, Result, HubDescCode::CompanionIsochMult, Gate::Versioned);

            if (Attributes & 0x7C)
                Note(Ctx, Result, HubDescCode::CompanionIsochReserved, Gate::Reserved);

            if (!SspFollows && Desc->wBytesPerInterval > Ceiling)
                Note(Ctx, Result, HubDescCode::CompanionIsochBytesPerInterval, Gate::Warning);

            if (SspFollows && Desc->wBytesPerInterval != 1)
                Note(Ctx, Result, HubDescCode::CompanionIsochBytesNotOne, Gate::Reserved);
            break;
    }
}

static
VOID
NTAPI
CheckSspCompanion(
    _Inout_ ConfigWalk* Walk,
    _In_ PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR Desc,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Desc);

    StartCheck(Ctx, Result);
    *Advance = Desc->bLength;

    if (Ctx->Speed != UsbSuperSpeed || Ctx->SspIsochBurstCount == 0)
    {
        Note(Ctx, Result, HubDescCode::SspCompanionNotSuperSpeed, Gate::Always);
        return;
    }

    if (Desc->bLength < sizeof(*Desc) &&
        !AssumeSize(Ctx, Result, HubDescCode::SspCompanionLengthTooSmall, Gate::Always, sizeof(*Desc), Left, Advance))
    {
        return;
    }

    if (Desc->bLength > sizeof(*Desc))
        Note(Ctx, Result, HubDescCode::SspCompanionLengthTooLarge, Gate::Versioned);

    if (!Walk->WantSspCompanion)
    {
        Note(Ctx, Result, HubDescCode::SspCompanionUnexpected, Gate::Always);
        return;
    }

    Walk->GotSspCompanion = TRUE;
    Walk->WantSspCompanion = FALSE;

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::SspCompanionBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Desc->wReserved != 0)
        Note(Ctx, Result, HubDescCode::SspCompanionReserved, Gate::Reserved);

    if (Desc->dwBytesPerInterval < 0xC001 || Desc->dwBytesPerInterval > 0xFFFFFF)
        Note(Ctx, Result, HubDescCode::SspCompanionBytesRange, Gate::Reserved);
}

static
VOID
NTAPI
CheckIad(
    _Inout_ ConfigWalk* Walk,
    _In_ PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR Desc,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Desc);
    ULONG Declared = Walk->Config->bNumInterfaces;

    StartCheck(Ctx, Result);
    *Advance = Desc->bLength;

    if (Desc->bLength < sizeof(*Desc) &&
        !AssumeSize(Ctx, Result, HubDescCode::IadLengthTooSmall, Gate::Legacy, sizeof(*Desc), Left, Advance))
    {
        return;
    }

    if (Desc->bLength > sizeof(*Desc))
        Note(Ctx, Result, HubDescCode::IadLengthTooLarge, Gate::Legacy);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::IadBufferTooSmall, Gate::Legacy);
        return;
    }

    if ((ULONG)Desc->bFirstInterface + Desc->bInterfaceCount > 256)
        Note(Ctx, Result, HubDescCode::IadFirstInterfaceTooLarge, Gate::Legacy);

    if (Desc->bInterfaceCount > Declared)
        Note(Ctx, Result, HubDescCode::IadCountTooLarge, Gate::Legacy);

    /* Same test USBCCGP makes */
    if (Desc->bInterfaceCount + Walk->InterfacesFound > Declared)
        Note(Ctx, Result, HubDescCode::IadCountTooLarge, Gate::NewRule);

    if (Desc->bInterfaceCount == 0)
        Note(Ctx, Result, HubDescCode::IadCountZero, Gate::Legacy);
}

BOOLEAN
NTAPI
HubDescCheckConfiguration(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Out_opt_ PBOOLEAN SupportsStreams)
{
    ConfigWalk Walk;
    Verdict Result;
    BOOLEAN Valid = TRUE;
    PUCHAR Cursor;

    if (SupportsStreams != NULL)
        *SupportsStreams = FALSE;

    if (Descriptor == NULL)
    {
        HubDescLog(Context, HubDescCode::ConfigNull, FALSE);
        return FALSE;
    }

    if (Length < sizeof(*Descriptor))
    {
        HubDescLog(Context, HubDescCode::ConfigBufferTooSmall, FALSE);
        return FALSE;
    }

    RtlZeroMemory(&Walk, sizeof(Walk));
    Walk.Ctx = Context;
    Walk.Base = (PUCHAR)Descriptor;
    Walk.BufferLength = Length;
    Walk.End = Walk.Base + Length;
    Walk.Config = Descriptor;
    Walk.SupportsStreams = SupportsStreams;

    Result = Verdict::Ok;
    CheckConfigFixedPart(&Walk, &Result);
    if (Result == Verdict::Fatal)
        return FALSE;

    if (Result != Verdict::Ok)
        Valid = FALSE;

    /* QUIRK: the walk starts after the 9 fixed bytes even when bLength is larger */
    Cursor = Walk.Base + sizeof(*Descriptor);
    while (Cursor < Walk.End)
    {
        PUSB_COMMON_DESCRIPTOR Desc = (PUSB_COMMON_DESCRIPTOR)Cursor;
        ULONG Advance;

        Result = Verdict::Ok;
        CheckHeader(Context, Walk.Base, Length, Walk.End, Desc, &Advance, &Result);
        if (Result == Verdict::Fatal)
            return FALSE;

        /* Compatibility mode lets header findings pass here (a Cypress mouse needs it) */
        if (Result == Verdict::Failed && !Context->Win8Behavior)
            Valid = FALSE;

        Result = Verdict::Ok;
        switch (Desc->bDescriptorType)
        {
            case USB_INTERFACE_DESCRIPTOR_TYPE:
                Walk.WantCompanion = FALSE;
                Walk.WantSspCompanion = FALSE;
                CheckInterface(&Walk, (PUSB_INTERFACE_DESCRIPTOR)Desc, &Advance, &Result);
                break;

            case USB_ENDPOINT_DESCRIPTOR_TYPE:
                Walk.WantSspCompanion = FALSE;
                CheckEndpoint(&Walk, (PUSB_ENDPOINT_DESCRIPTOR)Desc, &Advance, &Result);
                break;

            case USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE:
                Walk.WantSspCompanion = FALSE;
                CheckCompanion(&Walk, (PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR)Desc, &Advance, &Result);
                break;

            case USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR_TYPE:
                CheckSspCompanion(&Walk,
                                  (PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR)Desc,
                                  &Advance,
                                  &Result);
                break;

            case USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE:
                Walk.WantCompanion = FALSE;
                Walk.WantSspCompanion = FALSE;
                CheckIad(&Walk, (PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR)Desc, &Advance, &Result);
                break;

            default:
                Walk.WantCompanion = FALSE;
                Walk.WantSspCompanion = FALSE;
                break;
        }

        if (Result == Verdict::Fatal)
            return FALSE;

        if (Result == Verdict::Failed)
            Valid = FALSE;

        /* Zero padding at the end of the set is tolerated */
        if (Desc->bLength == 0)
            break;

        Cursor += Advance;
    }

    Result = Verdict::Ok;
    CheckPreviousInterface(&Walk, &Result);
    if (Result != Verdict::Ok)
        Valid = FALSE;

    /* No validation code for this one */
    if (Walk.InterfacesFound != Descriptor->bNumInterfaces && GateApplies(Context, Gate::Legacy))
    {
        DPRINT1("%s %p configuration has %lu interfaces, bNumInterfaces %u\n",
                Context->ForHub ? "Hub" : "Device",
                Context->LogContext,
                Walk.InterfacesFound,
                Descriptor->bNumInterfaces);
        Valid = FALSE;
    }

    return Valid;
}

/* Device descriptor */

BOOLEAN
NTAPI
HubDescCheckDevice(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_DEVICE_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Inout_opt_ PBOOLEAN IsBillboard)
{
    Verdict Result = Verdict::Ok;
    USHORT Version;
    ULONG Packet0;
    ULONG i;

    if (Descriptor == NULL)
    {
        HubDescLog(Context, HubDescCode::DeviceNull, FALSE);
        return FALSE;
    }

    if (Length < sizeof(USB_COMMON_DESCRIPTOR))
    {
        HubDescLog(Context, HubDescCode::DeviceBufferTooSmall, FALSE);
        return FALSE;
    }

    if (Descriptor->bLength < sizeof(*Descriptor))
        Note(Context, &Result, HubDescCode::DeviceLengthTooSmall, Gate::Always);

    if (Descriptor->bDescriptorType != USB_DEVICE_DESCRIPTOR_TYPE)
        Note(Context, &Result, HubDescCode::DeviceWrongType, Gate::Always);

    if (Length < 8)
    {
        HubDescLog(Context, HubDescCode::DeviceBufferTooSmall, FALSE);
        return FALSE;
    }

    /* Skipped on the 8 byte read, where these bytes are stale */
    if (Length >= RTL_SIZEOF_THROUGH_FIELD(USB_DEVICE_DESCRIPTOR, bcdDevice))
    {
        for (i = 0; i < 16; i += 4)
        {
            if (((Descriptor->bcdDevice >> i) & 0xF) > 9)
            {
                Note(Context, &Result, HubDescCode::DeviceVersionNotBcd, Gate::Warning);
                break;
            }
        }
    }

    /* The gates use the context version; this one is the descriptor's own */
    Version = Descriptor->bcdUSB;
    if (Descriptor->bDeviceClass == USB_DEVICE_CLASS_BILLBOARD &&
        Descriptor->bDeviceSubClass == 0 &&
        Descriptor->bDeviceProtocol == 0)
    {
        if (Version > 0x0200 && Version < 0x0300)
        {
            if (IsBillboard != NULL)
                *IsBillboard = TRUE;
        }
        else
        {
            /* QUIRK: a USB 3.x Billboard device fails enumeration */
            Note(Context, &Result, HubDescCode::BillboardDeviceVersion, Gate::Always);
        }
    }

    Packet0 = Descriptor->bMaxPacketSize0;
    switch (Context->Speed)
    {
        /* QUIRK: low and high speed can log the code twice */
        case UsbLowSpeed:
        case UsbHighSpeed:
            if (Packet0 != (Context->Speed == UsbLowSpeed ? 8UL : 64UL))
            {
                Note(Context, &Result, HubDescCode::DeviceMaxPacket0, Gate::Versioned);
                if (!IsSmallPacket(Packet0))
                    Note(Context, &Result, HubDescCode::DeviceMaxPacket0, Gate::Always);
            }
            break;

        case UsbFullSpeed:
            if (!IsSmallPacket(Packet0))
                Note(Context, &Result, HubDescCode::DeviceMaxPacket0, Gate::Always);
            break;

        case UsbSuperSpeed:
            if (Packet0 != 9)
                Note(Context, &Result, HubDescCode::DeviceMaxPacket0, Gate::Versioned);
            break;
    }

    return Result == Verdict::Ok;
}

/* BOS descriptor set */

static
VOID
NTAPI
CheckBosFixedPart(
    _In_ const HubDescContext* Ctx,
    _In_opt_ PUSB_BOS_DESCRIPTOR Bos,
    _In_ ULONG Length,
    _Out_ PUCHAR* End,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    ULONG Minimum;

    StartCheck(Ctx, Result);
    *Advance = sizeof(*Bos);
    *End = NULL;

    if (Bos == NULL)
    {
        NoteFatal(Ctx, Result, HubDescCode::BosNull);
        return;
    }

    /* Every caller supplies 5 bytes, so this never fails in practice */
    if (Length < sizeof(*Bos))
    {
        NoteFatal(Ctx, Result, HubDescCode::BosBufferTooSmall);
        return;
    }

    if (Bos->bDescriptorType != USB_BOS_DESCRIPTOR_TYPE)
        Note(Ctx, Result, HubDescCode::BosWrongType, Gate::Always);

    if (Bos->bLength < sizeof(*Bos))
        Note(Ctx, Result, HubDescCode::BosLengthTooSmall, Gate::Always);

    *End = (PUCHAR)Bos + Bos->wTotalLength;
    if (Bos->bLength > sizeof(*Bos))
    {
        Note(Ctx, Result, HubDescCode::BosLengthTooLarge, Gate::Versioned);
        *Advance = Bos->bLength;
    }

    Minimum = Bos->bLength + Bos->bNumDeviceCaps * sizeof(USB_COMMON_DESCRIPTOR);
    if (Bos->wTotalLength < sizeof(*Bos))
    {
        Note(Ctx, Result, HubDescCode::BosTotalLengthTooSmall, Gate::Always);
        *End = (PUCHAR)Bos + Length;
    }

    if (Bos->wTotalLength < Minimum)
    {
        Note(Ctx, Result, HubDescCode::BosTotalLengthTooSmall, Gate::Always);
        *End = (PUCHAR)Bos + Length;
    }

    /* QUIRK: reuses the wTotalLength code */
    if (Bos->bNumDeviceCaps == 0)
        Note(Ctx, Result, HubDescCode::BosTotalLengthTooSmall, Gate::Versioned);
}

static
VOID
NTAPI
CheckUsb20Cap(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_USB20_EXTENSION_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);
    HubLpmInfo* Lpm = Walk->Lpm;
    ULONG Attributes;
    UCHAR Baseline;
    UCHAR Deep;

    StartCheck(Ctx, Result);
    if (Cap->bLength < sizeof(*Cap) &&
        !AssumeSize(Ctx, Result, HubDescCode::Usb20CapLengthTooSmall, Gate::Always, sizeof(*Cap), Left, Advance))
    {
        return;
    }

    if (Cap->bLength > sizeof(*Cap))
        Note(Ctx, Result, HubDescCode::Usb20CapLengthTooLarge, Gate::Cap20);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::Usb20CapBufferTooSmall, Gate::NewRule);
        return;
    }

    Attributes = Cap->bmAttributes.AsUlong;
    if (Attributes & USB_DEVICE_CAPABILITY_USB20_EXTENSION_BMATTRIBUTES_RESERVED_MASK)
        Note(Ctx, Result, HubDescCode::Usb20CapReserved, Gate::Reserved);

    if (!Cap->bmAttributes.LPMCapable)
    {
        if (Cap->bmAttributes.BESLAndAlternateHIRDSupported)
            Note(Ctx, Result, HubDescCode::BeslWithoutLpm, Gate::NewRule);
        return;
    }

    /* QUIRK: written even for a duplicate capability, and never cleared */
    if (Lpm != NULL)
    {
        Lpm->Capable = TRUE;
        if (Cap->bmAttributes.BESLAndAlternateHIRDSupported)
            Lpm->BeslAndAltHird = TRUE;
    }

    Baseline = (UCHAR)Cap->bmAttributes.BaselineBESL;
    Deep = (UCHAR)Cap->bmAttributes.DeepBESL;

    if (Cap->bmAttributes.BaselineBESLValid)
    {
        if (Baseline == 0)
        {
            Note(Ctx, Result, HubDescCode::BaselineBeslZero, Gate::NewRule);
        }
        else if (Lpm != NULL)
        {
            Lpm->BaselineValid = TRUE;
            Lpm->Baseline = Baseline;
        }
    }

    if (!Cap->bmAttributes.DeepBESLValid)
        return;

    if (Cap->bmAttributes.BaselineBESLValid && Deep <= Baseline)
    {
        Note(Ctx, Result, HubDescCode::DeepBeslNotAboveBaseline, Gate::NewRule);
    }
    else if (Deep == 0)
    {
        Note(Ctx, Result, HubDescCode::DeepBeslZero, Gate::NewRule);
    }
    else if (Lpm != NULL)
    {
        Lpm->DeepValid = TRUE;
        Lpm->Deep = Deep;
    }
}

static
VOID
NTAPI
CheckSuperSpeedCap(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_SUPERSPEED_USB_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);
    ULONG Speeds;

    StartCheck(Ctx, Result);

    /* QUIRK: the LTM output is cleared and never set, so the BOS never makes a device LTM capable */
    if (Walk->Ltm != NULL)
        *Walk->Ltm = FALSE;

    if (Cap->bLength < sizeof(*Cap) &&
        !AssumeSize(Ctx, Result, HubDescCode::SsCapLengthTooSmall, Gate::Always, sizeof(*Cap), Left, Advance))
    {
        return;
    }

    if (Cap->bLength > sizeof(*Cap))
        Note(Ctx, Result, HubDescCode::SsCapLengthTooLarge, Gate::CapSs);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::SsCapBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Cap->bmAttributes & USB_DEVICE_CAPABILITY_SUPERSPEED_BMATTRIBUTES_RESERVED_MASK)
        Note(Ctx, Result, HubDescCode::SsCapAttributesReserved, Gate::Reserved);

    Speeds = Cap->wSpeedsSupported;
    if (Speeds & USB_DEVICE_CAPABILITY_SUPERSPEED_SPEEDS_SUPPORTED_RESERVED_MASK)
        Note(Ctx, Result, HubDescCode::SsCapSpeedsReserved, Gate::Reserved);

    if (Speeds == 0)
        Note(Ctx, Result, HubDescCode::SsCapNoSpeeds, Gate::Always);

    if (Cap->bFunctionalitySupport > 3)
        Note(Ctx, Result, HubDescCode::SsCapFunctionalityReserved, Gate::Reserved);
    else if ((Speeds & (1UL << Cap->bFunctionalitySupport)) == 0)
        Note(Ctx, Result, HubDescCode::SsCapFunctionalityNotInSpeeds, Gate::Always);

    if (Cap->bU1DevExitLat > USB_DEVICE_CAPABILITY_SUPERSPEED_U1_DEVICE_EXIT_MAX_VALUE)
        Note(Ctx, Result, HubDescCode::SsCapU1TooLarge, Gate::CapSs);
    else if (Walk->U1 != NULL)
        *Walk->U1 = Cap->bU1DevExitLat;

    if (Cap->wU2DevExitLat > USB_DEVICE_CAPABILITY_SUPERSPEED_U2_DEVICE_EXIT_MAX_VALUE)
        Note(Ctx, Result, HubDescCode::SsCapU2TooLarge, Gate::CapSs);
    else if (Walk->U2 != NULL)
        *Walk->U2 = Cap->wU2DevExitLat;
}

/** Sublink speed attribute pairs: RX then TX, matching, with unique IDs. */
static
VOID
NTAPI
CheckSublinkSpeeds(
    _In_ const HubDescContext* Ctx,
    _In_ PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR Cap,
    _In_ ULONG Present,
    _Inout_ Verdict* Result)
{
    ULONG Last = Cap->bmAttributes.SublinkSpeedAttrCount;
    ULONG MinimumId = Cap->wFunctionalitySupport.SublinkSpeedAttrID;
    BOOLEAN Symmetric;
    BOOLEAN MinimumFound = FALSE;
    ULONG SeenIds = 0;
    ULONG Unique = 0;
    ULONG i;

    for (i = 0; i <= Last; i += 2)
    {
        /* Stop at the last entry bLength covers */
        if (i >= Present)
            return;

        if (Cap->bmSublinkSpeedAttr[i].SublinkTypeDir != USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_DIR_RX)
        {
            Note(Ctx, Result, HubDescCode::SspAttrNotRx, Gate::NewRule);
            return;
        }

        if (i == Last)
        {
            Note(Ctx, Result, HubDescCode::SspAttrTxMissing, Gate::NewRule);
            return;
        }

        if (i + 1 >= Present)
            return;

        if (Cap->bmSublinkSpeedAttr[i + 1].SublinkTypeDir != USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_DIR_TX)
        {
            Note(Ctx, Result, HubDescCode::SspAttrNotTx, Gate::NewRule);
            return;
        }

        if (Cap->bmSublinkSpeedAttr[i].SublinkSpeedAttrID != Cap->bmSublinkSpeedAttr[i + 1].SublinkSpeedAttrID ||
            Cap->bmSublinkSpeedAttr[i].SublinkTypeMode != Cap->bmSublinkSpeedAttr[i + 1].SublinkTypeMode ||
            Cap->bmSublinkSpeedAttr[i].LinkProtocol != Cap->bmSublinkSpeedAttr[i + 1].LinkProtocol)
        {
            Note(Ctx, Result, HubDescCode::SspAttrPairMismatch, Gate::NewRule);
        }

        Symmetric = Cap->bmSublinkSpeedAttr[i].SublinkTypeMode == USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED_MODE_SYMMETRIC;
        if (Symmetric &&
            (Cap->bmSublinkSpeedAttr[i].LaneSpeedExponent != Cap->bmSublinkSpeedAttr[i + 1].LaneSpeedExponent ||
             Cap->bmSublinkSpeedAttr[i].LaneSpeedMantissa != Cap->bmSublinkSpeedAttr[i + 1].LaneSpeedMantissa))
        {
            Note(Ctx, Result, HubDescCode::SspAttrSymmetricMismatch, Gate::NewRule);
        }

        if (Cap->bmSublinkSpeedAttr[i].SublinkSpeedAttrID == MinimumId)
        {
            MinimumFound = TRUE;
            if (Symmetric &&
                Cap->wFunctionalitySupport.MinRxLaneCount != Cap->wFunctionalitySupport.MinTxLaneCount)
            {
                Note(Ctx, Result, HubDescCode::SspMinLaneMismatch, Gate::NewRule);
            }
        }

        if (SeenIds & (1UL << Cap->bmSublinkSpeedAttr[i].SublinkSpeedAttrID))
        {
            Note(Ctx, Result, HubDescCode::SspSpeedIdDuplicate, Gate::NewRule);
        }
        else
        {
            SeenIds |= 1UL << Cap->bmSublinkSpeedAttr[i].SublinkSpeedAttrID;
            Unique++;
        }
    }

    if (Cap->bmAttributes.SublinkSpeedIDCount + 1UL != Unique)
        Note(Ctx, Result, HubDescCode::SspSpeedIdCount, Gate::NewRule);

    if (!MinimumFound)
        Note(Ctx, Result, HubDescCode::SspMinSpeedMissing, Gate::NewRule);
}

static
VOID
NTAPI
CheckSuperSpeedPlusCap(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);
    ULONG Fixed = FIELD_OFFSET(USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR, bmSublinkSpeedAttr);
    ULONG Entry = sizeof(Cap->bmSublinkSpeedAttr[0]);

    StartCheck(Ctx, Result);
    if (Cap->bLength < sizeof(*Cap) &&
        !AssumeSize(Ctx, Result, HubDescCode::SspCapLengthTooSmall, Gate::Always, sizeof(*Cap), Left, Advance))
    {
        return;
    }

    if (Cap->bLength != sizeof(*Cap) + Entry * Cap->bmAttributes.SublinkSpeedAttrCount)
        Note(Ctx, Result, HubDescCode::SspCapLengthInvalid, Gate::CapSsp);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::SspCapBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Cap->bmAttributes.Reserved != 0)
        Note(Ctx, Result, HubDescCode::SspCapAttributesReserved, Gate::Reserved);

    if (Cap->wFunctionalitySupport.Reserved != 0)
        Note(Ctx, Result, HubDescCode::SspCapFunctionalityReserved, Gate::Reserved);

    if (Cap->bReserved != 0)
        Note(Ctx, Result, HubDescCode::SspCapReservedByte, Gate::Reserved);

    if (Cap->wReserved != 0)
        Note(Ctx, Result, HubDescCode::SspCapReservedWord, Gate::Reserved);

    CheckSublinkSpeeds(Ctx, Cap, (*Advance - Fixed) / Entry, Result);
}

static
VOID
NTAPI
CheckContainerIdCap(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_CONTAINER_ID_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);

    StartCheck(Ctx, Result);
    if (Cap->bLength < sizeof(*Cap) &&
        !AssumeSize(Ctx, Result, HubDescCode::ContainerIdLengthTooSmall, Gate::Always, sizeof(*Cap), Left, Advance))
    {
        return;
    }

    if (Cap->bLength > sizeof(*Cap))
        Note(Ctx, Result, HubDescCode::ContainerIdLengthTooLarge, Gate::Versioned);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::ContainerIdBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Cap->bReserved != 0)
        Note(Ctx, Result, HubDescCode::ContainerIdReserved, Gate::Reserved);

    /* These devices still enumerate; only the ID is dropped */
    if (IsAllZero(Cap->ContainerID, sizeof(Cap->ContainerID)))
    {
        Note(Ctx, Result, HubDescCode::BosContainerIdZero, Gate::Warning);
        Walk->Info->DiscardContainerId = TRUE;
    }
}

/** Version check standing in for RtlIsNtDdiVersionAvailable. */
static
BOOLEAN
NTAPI
MsOs20VersionUsable(
    _In_ ULONG Version)
{
    return Version <= HUB_MSOS20_NEWEST_VERSION;
}

static
VOID
NTAPI
CheckMsOs20Platform(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_PLATFORM_DESCRIPTOR Cap,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Fixed = FIELD_OFFSET(USB_DEVICE_CAPABILITY_PLATFORM_DESCRIPTOR, CapabililityData);
    HubMsOs20SetInfo* Sets = (HubMsOs20SetInfo*)((PUCHAR)Cap + Fixed);
    ULONG Count;
    ULONG Best = 0;
    ULONG i;
    ULONG j;

    if (Walk->Info->MsOs20SetInfo != NULL)
    {
        NoteFatal(Ctx, Result, HubDescCode::MsOs20MultiplePlatform);
        return;
    }

    if (Cap->bLength < Fixed + sizeof(*Sets) || (Cap->bLength - Fixed) % sizeof(*Sets) != 0)
    {
        NoteFatal(Ctx, Result, HubDescCode::MsOs20PlatformLength);
        return;
    }

    Count = (Cap->bLength - Fixed) / sizeof(*Sets);

    /* Logged once per equal pair */
    for (i = 0; i < Count; i++)
    {
        for (j = i + 1; j < Count; j++)
        {
            if (Sets[i].dwWindowsVersion == Sets[j].dwWindowsVersion)
                NoteFatal(Ctx, Result, HubDescCode::MsOs20DuplicateVersion);
        }
    }

    /* Newest set this OS can use */
    for (i = 0; i < Count; i++)
    {
        if (Sets[i].dwWindowsVersion > Best && MsOs20VersionUsable(Sets[i].dwWindowsVersion))
        {
            Best = Sets[i].dwWindowsVersion;
            Walk->Info->MsOs20SetInfo = &Sets[i];
        }
    }

    if (Best == 0)
        HubDescLog(Ctx, HubDescCode::MsOs20NoVersionMatch, TRUE);
}

static
VOID
NTAPI
CheckPlatformFeatures(
    _Inout_ BosWalk* Walk,
    _In_ HubPlatformFeatures* Cap,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;

    if (Cap->bLength < sizeof(*Cap))
    {
        NoteFatal(Ctx, Result, HubDescCode::PlatformFeaturesTooSmall);
        return;
    }

    /* Later versions may be longer */
    if (Cap->Version == 1 && Cap->bLength != sizeof(*Cap))
    {
        NoteFatal(Ctx, Result, HubDescCode::PlatformFeaturesLength);
        return;
    }

    if (Cap->Version == 0)
    {
        NoteFatal(Ctx, Result, HubDescCode::PlatformFeaturesVersion);
        return;
    }

    if (Walk->Info->PlatformFeatures != NULL)
    {
        NoteFatal(Ctx, Result, HubDescCode::PlatformFeaturesMultiple);
        return;
    }

    Walk->Info->PlatformFeatures = Cap;
}

static
VOID
NTAPI
CheckPlatformCap(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_PLATFORM_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);
    ULONG Fixed = FIELD_OFFSET(USB_DEVICE_CAPABILITY_PLATFORM_DESCRIPTOR, CapabililityData);
    PUCHAR Uuid = (PUCHAR)Cap + FIELD_OFFSET(USB_DEVICE_CAPABILITY_PLATFORM_DESCRIPTOR, PlatformCapabilityUuid);

    StartCheck(Ctx, Result);
    if (Cap->bLength < Fixed &&
        !AssumeSize(Ctx, Result, HubDescCode::PlatformLengthTooSmall, Gate::Always, Fixed, Left, Advance))
    {
        return;
    }

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::PlatformBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Cap->bReserved != 0)
        Note(Ctx, Result, HubDescCode::PlatformReserved, Gate::Reserved);

    if (IsAllZero(Uuid, sizeof(GUID)))
    {
        NoteFatal(Ctx, Result, HubDescCode::PlatformUuidZero);
        return;
    }

    /* Other platform UUIDs are accepted as they are */
    if (RtlCompareMemory(Uuid, &HubMsOs20PlatformUuid, sizeof(GUID)) == sizeof(GUID))
        CheckMsOs20Platform(Walk, Cap, Result);
    else if (RtlCompareMemory(Uuid, &HubPlatformFeaturesUuid, sizeof(GUID)) == sizeof(GUID))
        CheckPlatformFeatures(Walk, (HubPlatformFeatures*)Cap, Result);
}

static
VOID
NTAPI
CheckPowerDeliveryCap(
    _Inout_ BosWalk* Walk,
    _In_ PUSB_DEVICE_CAPABILITY_POWER_DELIVERY_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);

    StartCheck(Ctx, Result);
    if (Cap->bLength < sizeof(*Cap) &&
        !AssumeSize(Ctx, Result, HubDescCode::PdLengthTooSmall, Gate::Always, sizeof(*Cap), Left, Advance))
    {
        return;
    }

    if (Cap->bLength > sizeof(*Cap))
        Note(Ctx, Result, HubDescCode::PdLengthTooLarge, Gate::Versioned);

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::PdBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Cap->bReserved != 0 ||
        Cap->bmAttributes.Reserved1 != 0 ||
        Cap->bmAttributes.Reserved2 != 0 ||
        Cap->bmAttributes.Reserved3 != 0)
    {
        Note(Ctx, Result, HubDescCode::PdReserved, Gate::Reserved);
    }

    if (Cap->bmAttributes.ChargingPolicy)
        Walk->Info->ChargingPolicy = TRUE;
}

static
VOID
NTAPI
CheckBillboardCap(
    _Inout_ BosWalk* Walk,
    _Inout_ PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Cap,
    _Inout_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);
    ULONG Mode = sizeof(Cap->AlternateMode[0]);
    ULONG Modes = Cap->bNumberOfAlternateModes;
    ULONG Expected;
    BOOLEAN Repaired;

    StartCheck(Ctx, Result);
    if (Left < sizeof(*Cap))
    {
        NoteFatal(Ctx, Result, HubDescCode::BillboardBufferTooSmall);
        return;
    }

    /* The structure holds one mode; zero modes gives 44 */
    Expected = sizeof(*Cap) - Mode + Modes * Mode;
    if (Cap->bLength != Expected)
    {
        /* QUIRK: the walk moves by the computed length from here on */
        *Advance = Expected;
        Repaired = (Cap->bLength == 40 || Cap->bLength == 44);
        HubDescLog(Ctx, HubDescCode::BillboardLengthInvalid, Repaired || !GateApplies(Ctx, Gate::CapSsp));

        /* QUIRK: early spec revisions gave the base length; repair it in the caller's buffer */
        if (Repaired)
            Cap->bLength = (UCHAR)Expected;
        else if (GateApplies(Ctx, Gate::CapSsp))
            *Result = Verdict::Failed;
    }

    if (*Advance > Left)
    {
        Note(Ctx, Result, HubDescCode::BillboardBufferTooSmall, Gate::NewRule);
        return;
    }

    if (Modes == 0)
    {
        NoteFatal(Ctx, Result, HubDescCode::BillboardNoModes);
        return;
    }

    if (Modes > 128)
    {
        NoteFatal(Ctx, Result, HubDescCode::BillboardTooManyModes);
        return;
    }

    /* QUIRK: an index equal to the mode count passes */
    if (Cap->bPreferredAlternateMode > Modes)
        NoteFatal(Ctx, Result, HubDescCode::BillboardPreferredInvalid);
}

/** Generic capability header, then the type specific check and the duplicate rules. */
static
VOID
NTAPI
CheckCapability(
    _Inout_ BosWalk* Walk,
    _Inout_ PUSB_DEVICE_CAPABILITY_DESCRIPTOR Cap,
    _Out_ PULONG Advance,
    _Inout_ Verdict* Result)
{
    const HubDescContext* Ctx = Walk->Ctx;
    HubBosInfo* Info = Walk->Info;
    ULONG Left = BytesLeft(Walk->Base, Walk->BufferLength, Cap);

    StartCheck(Ctx, Result);

    /* QUIRK: latencies survive only when the SuperSpeed capability comes last */
    if (Walk->U1 != NULL)
        *Walk->U1 = 0;

    if (Walk->U2 != NULL)
        *Walk->U2 = 0;

    *Advance = Cap->bLength;
    if (Cap->bLength < sizeof(*Cap) &&
        !AssumeSize(Ctx, Result, HubDescCode::CapLengthTooSmall, Gate::Always, sizeof(*Cap), Left, Advance))
    {
        return;
    }

    if (sizeof(*Cap) > Left)
    {
        Note(Ctx, Result, HubDescCode::CapBufferTooSmall, Gate::NewRule);
        return;
    }

    /* QUIRK: in compatibility mode the specific checks below start over */
    switch (Cap->bDevCapabilityType)
    {
        case USB_DEVICE_CAPABILITY_USB20_EXTENSION:
            CheckUsb20Cap(Walk, (PUSB_DEVICE_CAPABILITY_USB20_EXTENSION_DESCRIPTOR)Cap, Advance, Result);
            if (*Result == Verdict::Fatal)
                return;

            if (Info->Usb20 != NULL)
                Note(Ctx, Result, HubDescCode::CapMultipleUsb20, Gate::Always);
            else
                Info->Usb20 = (PUSB_DEVICE_CAPABILITY_USB20_EXTENSION_DESCRIPTOR)Cap;
            break;

        case USB_DEVICE_CAPABILITY_SUPERSPEED_USB:
            CheckSuperSpeedCap(Walk, (PUSB_DEVICE_CAPABILITY_SUPERSPEED_USB_DESCRIPTOR)Cap, Advance, Result);
            if (*Result == Verdict::Fatal)
                return;

            if (Info->SuperSpeed != NULL)
                Note(Ctx, Result, HubDescCode::CapMultipleSuperSpeed, Gate::Always);
            else
                Info->SuperSpeed = (PUSB_DEVICE_CAPABILITY_SUPERSPEED_USB_DESCRIPTOR)Cap;
            break;

        case USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB:
            CheckSuperSpeedPlusCap(Walk, (PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR)Cap, Advance, Result);
            if (*Result == Verdict::Fatal)
                return;

            if (Info->SuperSpeedPlus != NULL)
                Note(Ctx, Result, HubDescCode::CapMultipleSuperSpeedPlus, Gate::Always);
            else
                Info->SuperSpeedPlus = (PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR)Cap;
            break;

        case USB_DEVICE_CAPABILITY_CONTAINER_ID:
            CheckContainerIdCap(Walk, (PUSB_DEVICE_CAPABILITY_CONTAINER_ID_DESCRIPTOR)Cap, Advance, Result);
            if (*Result == Verdict::Fatal)
                return;

            if (Info->ContainerId != NULL)
                Note(Ctx, Result, HubDescCode::CapMultipleContainerId, Gate::Always);
            else if (!Info->DiscardContainerId)
                Info->ContainerId = (PUSB_DEVICE_CAPABILITY_CONTAINER_ID_DESCRIPTOR)Cap;
            break;

        case USB_DEVICE_CAPABILITY_PLATFORM:
            CheckPlatformCap(Walk, (PUSB_DEVICE_CAPABILITY_PLATFORM_DESCRIPTOR)Cap, Advance, Result);
            break;

        case USB_DEVICE_CAPABILITY_POWER_DELIVERY:
            CheckPowerDeliveryCap(Walk, (PUSB_DEVICE_CAPABILITY_POWER_DELIVERY_DESCRIPTOR)Cap, Advance, Result);
            break;

        case USB_DEVICE_CAPABILITY_BILLBOARD:
            CheckBillboardCap(Walk, (PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR)Cap, Advance, Result);
            if (*Result == Verdict::Fatal)
                return;

            if (Info->Billboard != NULL)
                Note(Ctx, Result, HubDescCode::CapMultipleBillboard, Gate::Always);
            else
                Info->Billboard = (PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR)Cap;
            break;

        /* Wireless USB, battery, PD ports, PTM, firmware status and unknown types */
        default:
            break;
    }
}

BOOLEAN
NTAPI
HubDescCheckBosHeader(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_BOS_DESCRIPTOR Descriptor,
    _In_ ULONG Length)
{
    Verdict Result = Verdict::Ok;
    PUCHAR End;
    ULONG Advance;

    CheckBosFixedPart(Context, Descriptor, Length, &End, &Advance, &Result);
    return Result == Verdict::Ok;
}

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
NTAPI
HubDescCheckBos(
    _In_ const HubDescContext* Context,
    _Inout_updates_bytes_(Length) PUSB_BOS_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Out_opt_ HubBosInfo* Info,
    _Out_opt_ PUSHORT U1ExitLatency,
    _Out_opt_ PUSHORT U2ExitLatency,
    _Inout_opt_ PBOOLEAN LtmCapable,
    _Inout_opt_ HubLpmInfo* Lpm,
    _Inout_opt_ PULONG DeviceFlags)
{
    HubBosInfo LocalInfo;
    BosWalk Walk;
    Verdict Result;
    BOOLEAN Valid = TRUE;
    PUCHAR Cursor;
    PUCHAR End;
    PUCHAR Limit;
    ULONG Advance;
    ULONG Found = 0;

    if (Info == NULL)
        Info = &LocalInfo;

    if (U1ExitLatency != NULL)
        *U1ExitLatency = 0;

    if (U2ExitLatency != NULL)
        *U2ExitLatency = 0;

    RtlZeroMemory(Info, sizeof(*Info));

    if (Length < sizeof(*Descriptor))
    {
        HubDescLog(Context, HubDescCode::BosBufferTooSmall, FALSE);
        return FALSE;
    }

    Result = Verdict::Ok;
    CheckBosFixedPart(Context, Descriptor, Length, &End, &Advance, &Result);
    if (Result == Verdict::Fatal)
        return FALSE;

    if (Result != Verdict::Ok)
        Valid = FALSE;

    if (Length < Descriptor->wTotalLength)
    {
        HubDescLog(Context, HubDescCode::BosBufferTooSmall, FALSE);
        Valid = FALSE;
    }

    /* A Genesys 2.1 hub returns more than wTotalLength */
    if (Length > Descriptor->wTotalLength)
        HubDescLog(Context, HubDescCode::BosBufferLargerThanTotal, TRUE);

    Walk.Ctx = Context;
    Walk.Base = (PUCHAR)Descriptor;
    Walk.BufferLength = Length;
    Walk.Info = Info;
    Walk.U1 = U1ExitLatency;
    Walk.U2 = U2ExitLatency;
    Walk.Ltm = LtmCapable;
    Walk.Lpm = Lpm;

    /* Stop at the buffer end even when wTotalLength runs past it */
    Limit = Walk.Base + Length;

    Cursor = Walk.Base + Advance;
    while (Cursor < End)
    {
        PUSB_COMMON_DESCRIPTOR Desc = (PUSB_COMMON_DESCRIPTOR)Cursor;

        /* Code 23 was logged above when wTotalLength overran the buffer */
        if (Cursor >= Limit)
            return FALSE;

        Result = Verdict::Ok;
        CheckHeader(Context, Walk.Base, Length, End, Desc, &Advance, &Result);
        if (Result == Verdict::Fatal)
            return FALSE;

        if (Result != Verdict::Ok)
            Valid = FALSE;

        Result = Verdict::Ok;
        if (Desc->bDescriptorType == USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE)
            CheckCapability(&Walk, (PUSB_DEVICE_CAPABILITY_DESCRIPTOR)Desc, &Advance, &Result);
        else
            HubDescLog(Context, HubDescCode::BosUnknownType, TRUE);

        if (Result == Verdict::Fatal)
            return FALSE;

        if (Result != Verdict::Ok)
            Valid = FALSE;

        /* QUIRK: every descriptor counts toward bNumDeviceCaps, whatever its type */
        Found++;

        /* A zero length descriptor would otherwise loop forever */
        if (Advance == 0)
            break;

        Cursor += Advance;
    }

    if (Found != Descriptor->bNumDeviceCaps)
    {
        HubDescLog(Context, HubDescCode::BosCapCountMismatch, FALSE);
        Valid = FALSE;
    }

    if (Info->SuperSpeed != NULL)
    {
        if (DeviceFlags != NULL)
            *DeviceFlags |= HUB_BOS_SUPERSPEED_CAPABLE;
    }
    else if (Context->Speed == UsbSuperSpeed)
    {
        HubDescLog(Context, HubDescCode::BosNoSuperSpeedCap, FALSE);
        Valid = FALSE;
    }

    if (DeviceFlags != NULL)
    {
        if (Info->SuperSpeedPlus != NULL)
            *DeviceFlags |= HUB_BOS_ENHANCED_SUPERSPEED_CAPABLE;

        if (Info->ChargingPolicy)
            *DeviceFlags |= HUB_BOS_CHARGING_POLICY;
    }

    return Valid;
}

/* Strings */

static
Verdict
NTAPI
CheckStringCore(
    _In_ const HubDescContext* Ctx,
    _In_reads_bytes_(Length) PUSB_STRING_DESCRIPTOR Desc,
    _In_ ULONG Length,
    _Out_ PULONG Adjusted)
{
    Verdict Result = Verdict::Ok;

    if (Length < sizeof(USB_COMMON_DESCRIPTOR))
    {
        *Adjusted = 0;
        NoteFatal(Ctx, &Result, HubDescCode::StringBufferTooSmall);
        return Result;
    }

    *Adjusted = Desc->bLength;
    if (Desc->bLength > Length)
    {
        Note(Ctx, &Result, HubDescCode::StringLengthTooLarge, Gate::Always);
        *Adjusted = Length;
    }

    /* No characters at all */
    if (Desc->bLength <= sizeof(USB_COMMON_DESCRIPTOR))
    {
        Note(Ctx, &Result, HubDescCode::StringLengthTooSmall, Gate::Always);
        *Adjusted = Length;
    }

    if (Desc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
        Note(Ctx, &Result, HubDescCode::StringWrongType, Gate::Always);

    if (Desc->bLength & 1)
    {
        Note(Ctx, &Result, HubDescCode::StringLengthOdd, Gate::Always);
        *Adjusted &= ~1UL;
    }

    if (Desc->bLength != Length)
        Note(Ctx, &Result, HubDescCode::StringLengthVsBytes, Gate::LogOnly);

    return Result;
}

BOOLEAN
NTAPI
HubDescCheckString(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) PUSB_STRING_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Out_opt_ PULONG AdjustedLength)
{
    ULONG Adjusted;
    Verdict Result;

    Result = CheckStringCore(Context, Descriptor, Length, &Adjusted);
    if (AdjustedLength != NULL)
        *AdjustedLength = Adjusted;

    return Result == Verdict::Ok;
}

BOOLEAN
NTAPI
HubDescCheckSerialNumber(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(*Length) PUSB_STRING_DESCRIPTOR Descriptor,
    _Inout_ PULONG Length)
{
    ULONG Adjusted;
    ULONG Chars;
    ULONG i;
    BOOLEAN Valid = TRUE;

    if (CheckStringCore(Context, Descriptor, *Length, &Adjusted) != Verdict::Ok)
        return FALSE;

    *Length = Descriptor->bLength - sizeof(USB_COMMON_DESCRIPTOR);
    Chars = *Length / sizeof(WCHAR);
    for (i = 0; i < Chars; i++)
    {
        WCHAR Ch = Descriptor->bString[i];

        if (Ch == 0)
            break;

        /* Printable ASCII only, and no comma since it separates instance ID parts */
        if (Ch < L' ' || Ch > 0x7F || Ch == L',')
        {
            HubDescLog(Context, HubDescCode::SerialNumberBadCharacter, FALSE);
            Valid = FALSE;
        }
    }

    if (!Valid)
        return FALSE;

    /* Room for a terminating NUL */
    *Length += sizeof(WCHAR);
    return TRUE;
}

/* MS OS 1.0 descriptors */

/** Compatible and subcompatible IDs: upper case letters, digits, '_', then NUL padding. */
static
BOOLEAN
NTAPI
IsMsOsIdChar(
    _In_ UCHAR Ch)
{
    return (Ch >= 'A' && Ch <= 'Z') || (Ch >= '0' && Ch <= '9') || Ch == '_';
}

static
BOOLEAN
NTAPI
IsMsOs10IdValid(
    _In_reads_(8) const UCHAR* Id)
{
    BOOLEAN Ended = FALSE;
    ULONG i;

    for (i = 0; i < 8; i++)
    {
        if (Id[i] == 0)
            Ended = TRUE;
        else if (Ended || !IsMsOsIdChar(Id[i]))
            return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
HubDescCheckMsOsString(
    _In_ const HubDescContext* Context,
    _In_ const HubMsOsString* Descriptor)
{
    static const WCHAR Expected[7] = { 'M', 'S', 'F', 'T', '1', '0', '0' };

    if (RtlCompareMemory(Descriptor->Signature, Expected, sizeof(Expected)) == sizeof(Expected))
        return TRUE;

    DPRINT1("Device %p MS OS string has the wrong signature\n", Context->LogContext);
    return FALSE;
}

BOOLEAN
NTAPI
HubDescCheckMsOsContainerIdHeader(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) const HubMsOsHeader* Descriptor,
    _In_ ULONG Length)
{
    BOOLEAN Valid = TRUE;

    if (Length != sizeof(*Descriptor))
    {
        HubDescLog(Context, HubDescCode::MsOsContainerIdBufferSize, FALSE);
        return FALSE;
    }

    if (Descriptor->bcdVersion != 0x0100)
    {
        HubDescLog(Context, HubDescCode::MsOsContainerIdVersion, FALSE);
        Valid = FALSE;
    }

    if (Descriptor->wIndex != 6)
    {
        HubDescLog(Context, HubDescCode::MsOsContainerIdIndex, FALSE);
        Valid = FALSE;
    }

    if (Descriptor->dwLength != sizeof(HubMsOsContainerId))
    {
        HubDescLog(Context, HubDescCode::MsOsContainerIdLength, FALSE);
        Valid = FALSE;
    }

    return Valid;
}

BOOLEAN
NTAPI
HubDescCheckMsOsContainerId(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) const HubMsOsContainerId* Descriptor,
    _In_ ULONG Length)
{
    if (Length != sizeof(*Descriptor))
    {
        HubDescLog(Context, HubDescCode::MsOsContainerIdBufferSize, FALSE);
        return FALSE;
    }

    if (IsAllZero(Descriptor->ContainerId, sizeof(Descriptor->ContainerId)))
    {
        HubDescLog(Context, HubDescCode::MsOsContainerIdZero, FALSE);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
HubDescCheckMsOsExtConfigHeader(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) const HubMsOsExtConfigHeader* Descriptor,
    _In_ ULONG Length)
{
    BOOLEAN Valid = TRUE;

    if (Length != sizeof(*Descriptor))
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigBufferSize, FALSE);
        return FALSE;
    }

    if (Descriptor->bcdVersion != 0x0100)
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigVersion, FALSE);
        Valid = FALSE;
    }

    if (Descriptor->wIndex != 4)
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigIndex, FALSE);
        Valid = FALSE;
    }

    /* Only single function devices are supported */
    if (Descriptor->bCount != 1)
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigCount, FALSE);
        return FALSE;
    }

    if (Descriptor->dwLength != sizeof(*Descriptor) + Descriptor->bCount * sizeof(HubMsOsExtConfigFunction))
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigLengthVsCount, FALSE);
        return FALSE;
    }

    return Valid;
}

BOOLEAN
NTAPI
HubDescCheckMsOsExtConfig(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(sizeof(HubMsOsExtConfig)) const HubMsOsExtConfig* Descriptor,
    _In_ ULONG Length)
{
    const HubMsOsExtConfigFunction* Function = &Descriptor->Function[0];
    Verdict Result = Verdict::Ok;

    if (Length != Descriptor->Header.dwLength)
        Note(Context, &Result, HubDescCode::MsOsExtConfigBufferVsLength, Gate::NewRule);

    /* Interface numbers are not compared with the configuration */
    if (!IsMsOs10IdValid(Function->CompatibleId))
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigCompatibleId, FALSE);
        return FALSE;
    }

    if (!IsMsOs10IdValid(Function->SubCompatibleId))
    {
        HubDescLog(Context, HubDescCode::MsOsExtConfigSubCompatibleId, FALSE);
        return FALSE;
    }

    return Result == Verdict::Ok;
}

/* MS OS 2.0 descriptor set */

/* wDescriptorType values */
#define HUB_MSOS20_TYPE_SET_HEADER      0
#define HUB_MSOS20_TYPE_CONFIG_SUBSET   1
#define HUB_MSOS20_TYPE_FUNCTION_SUBSET 2
#define HUB_MSOS20_TYPE_COMPATIBLE_ID   3
#define HUB_MSOS20_TYPE_REGISTRY        4
#define HUB_MSOS20_TYPE_RESUME_TIME     5
#define HUB_MSOS20_TYPE_MODEL_ID        6
#define HUB_MSOS20_TYPE_CCGP            7

BOOLEAN
NTAPI
HubDescNextMsOs20(
    _In_ PUCHAR End,
    _Inout_ HubMsOs20Common** Current)
{
    HubMsOs20Common* Desc = *Current;
    PUCHAR At = (PUCHAR)Desc;
    PUCHAR Next;

    *Current = NULL;

    /* The current descriptor must itself fit before End */
    if (At + Desc->wLength > End)
        return FALSE;

    if (Desc->wDescriptorType == HUB_MSOS20_TYPE_CONFIG_SUBSET ||
        Desc->wDescriptorType == HUB_MSOS20_TYPE_FUNCTION_SUBSET)
    {
        HubMsOs20Subset* Subset = (HubMsOs20Subset*)Desc;

        if (Subset->wLength < sizeof(*Subset) || Subset->wTotalLength < sizeof(*Subset) + sizeof(*Desc))
            return FALSE;

        Next = At + Subset->wTotalLength;
    }
    else
    {
        if (Desc->wLength < sizeof(*Desc))
            return FALSE;

        Next = At + Desc->wLength;
    }

    if (Next == End)
        return TRUE;

    if (Next > End || Next + sizeof(*Desc) > End)
        return FALSE;

    if (Next + ((HubMsOs20Common*)Next)->wLength > End)
        return FALSE;

    *Current = (HubMsOs20Common*)Next;
    return TRUE;
}

static
BOOLEAN
NTAPI
CheckMsOs20Feature(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc,
    _In_ PUCHAR End,
    _In_ ULONG Depth);

static
BOOLEAN
NTAPI
CheckMsOs20SetHeader(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc)
{
    BOOLEAN Valid = TRUE;

    if (Info->Flags & HUB_MSOS20_SET_HEADER)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20MultipleSetHeaders, FALSE);
        Valid = FALSE;
    }

    if (Desc->wLength != sizeof(HubMsOs20SetHeader))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20SetHeaderLength, FALSE);
        Valid = FALSE;
    }

    if (Valid)
        Info->Flags |= HUB_MSOS20_SET_HEADER;

    return Valid;
}

/** Feature types a subset may hold. Configuration subsets only warn about others. */
static
BOOLEAN
NTAPI
MsOs20AllowedIn(
    _In_ BOOLEAN Function,
    _In_ ULONG Type)
{
    if (Type == HUB_MSOS20_TYPE_COMPATIBLE_ID || Type == HUB_MSOS20_TYPE_REGISTRY)
        return TRUE;

    return !Function && Type == HUB_MSOS20_TYPE_FUNCTION_SUBSET;
}

static
BOOLEAN
NTAPI
CheckMsOs20Subset(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc,
    _In_ PUCHAR End,
    _In_ ULONG Depth)
{
    HubMsOs20Subset* Subset = (HubMsOs20Subset*)Desc;
    BOOLEAN Function = Desc->wDescriptorType == HUB_MSOS20_TYPE_FUNCTION_SUBSET;
    HubDescCode LengthCode = Function ? HubDescCode::FunctionSubsetLength : HubDescCode::ConfigSubsetLength;
    HubDescCode SmallCode = Function ? HubDescCode::FunctionSubsetTotalTooSmall : HubDescCode::ConfigSubsetTotalTooSmall;
    HubDescCode LargeCode = Function ? HubDescCode::FunctionSubsetTotalTooLarge : HubDescCode::ConfigSubsetTotalTooLarge;
    HubDescCode StepCode = Function ? HubDescCode::FunctionSubsetFeatureLength : HubDescCode::ConfigSubsetFeatureLength;
    HubMsOs20Info Private;
    HubMsOs20Common* Cursor;
    PUCHAR SubsetEnd;
    BOOLEAN Valid = TRUE;

    if (Subset->wLength != sizeof(*Subset))
    {
        HubDescLog(Ctx, LengthCode, FALSE);
        return FALSE;
    }

    if (Subset->wTotalLength < sizeof(*Subset) + sizeof(*Desc))
    {
        HubDescLog(Ctx, SmallCode, FALSE);
        return FALSE;
    }

    SubsetEnd = (PUCHAR)Subset + Subset->wTotalLength;
    if (SubsetEnd > End)
    {
        HubDescLog(Ctx, LargeCode, FALSE);
        return FALSE;
    }

    /* Nested configuration subsets would otherwise recurse without bound */
    if (Depth >= HUB_MSOS20_MAX_DEPTH)
    {
        HubDescLog(Ctx, StepCode, FALSE);
        return FALSE;
    }

    /* QUIRK: what a subset holds is checked into a private record, then dropped */
    RtlZeroMemory(&Private, sizeof(Private));

    Cursor = (HubMsOs20Common*)((PUCHAR)Subset + sizeof(*Subset));
    do
    {
        ULONG Type = Cursor->wDescriptorType;

        if (Type <= HUB_MSOS20_TYPE_CCGP)
        {
            if (!MsOs20AllowedIn(Function, Type))
            {
                if (Function)
                {
                    HubDescLog(Ctx, HubDescCode::FunctionSubsetMisplacedFeature, FALSE);
                    Valid = FALSE;
                }
                else
                {
                    HubDescLog(Ctx, HubDescCode::ConfigSubsetMisplacedFeature, TRUE);
                }
            }

            /* Skip a first entry that runs past the subset; the step below fails it */
            if ((PUCHAR)Cursor + Cursor->wLength <= SubsetEnd &&
                !CheckMsOs20Feature(Ctx, &Private, Cursor, SubsetEnd, Depth + 1))
            {
                Valid = FALSE;
            }
        }

        if (!HubDescNextMsOs20(SubsetEnd, &Cursor))
        {
            HubDescLog(Ctx, StepCode, FALSE);
            return FALSE;
        }
    } while (Cursor != NULL);

    if (Valid)
        Info->Flags |= Function ? HUB_MSOS20_FUNCTION_SUBSET : HUB_MSOS20_CONFIG_SUBSET;

    return Valid;
}

/** Checks both 8 byte IDs. Bad characters log the length code (QUIRK). */
static
BOOLEAN
NTAPI
CheckMsOs20Id(
    _In_ const HubDescContext* Ctx,
    _In_reads_(8) const UCHAR* Id)
{
    BOOLEAN Ended = FALSE;
    ULONG i;

    for (i = 0; i < 8; i++)
    {
        if (Ended && Id[i] != 0)
        {
            HubDescLog(Ctx, HubDescCode::MsOs20CompatibleIdAfterNull, FALSE);
            return FALSE;
        }

        if (Id[i] == 0)
        {
            Ended = TRUE;
        }
        else if (!IsMsOsIdChar(Id[i]))
        {
            HubDescLog(Ctx, HubDescCode::MsOs20CompatibleIdLength, FALSE);
            return FALSE;
        }
    }

    return TRUE;
}

static
BOOLEAN
NTAPI
CheckMsOs20CompatibleId(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc)
{
    HubMsOs20CompatibleId* Id = (HubMsOs20CompatibleId*)Desc;
    BOOLEAN Valid = TRUE;

    if (Info->Flags & HUB_MSOS20_COMPATIBLE_ID)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20MultipleCompatibleId, FALSE);
        Valid = FALSE;
    }

    if (Id->wLength != sizeof(*Id))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20CompatibleIdLength, FALSE);
        return FALSE;
    }

    if (!CheckMsOs20Id(Ctx, Id->CompatibleId) || !CheckMsOs20Id(Ctx, Id->SubCompatibleId))
        return FALSE;

    if (Valid)
    {
        Info->CompatibleId = Id;
        Info->Flags |= HUB_MSOS20_COMPATIBLE_ID;
    }

    return Valid;
}

static
BOOLEAN
NTAPI
CheckMsOs20Registry(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc)
{
    HubMsOs20Registry* Reg = (HubMsOs20Registry*)Desc;
    ULONG Total = Reg->wLength;
    ULONG NameLength;
    USHORT DataLength;

    /* Header, a one character name, the data length and one data byte */
    if (Total < sizeof(*Reg) + sizeof(WCHAR) + sizeof(USHORT) + 1)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20RegistryTooSmall, FALSE);
        return FALSE;
    }

    NameLength = Reg->wPropertyNameLength;
    if (Total < sizeof(*Reg) + NameLength + sizeof(USHORT) + 1)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20RegistryNameTooLarge, FALSE);
        return FALSE;
    }

    if (NameLength == 0 || (NameLength % sizeof(WCHAR)) != 0)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20RegistryNameOddOrZero, FALSE);
        return FALSE;
    }

    RtlCopyMemory(&DataLength, (PUCHAR)Reg + sizeof(*Reg) + NameLength, sizeof(DataLength));
    if (Total < sizeof(*Reg) + NameLength + sizeof(USHORT) + DataLength)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20RegistryDataTooLarge, FALSE);
        return FALSE;
    }

    if (DataLength == 0)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20RegistryDataZero, FALSE);
        return FALSE;
    }

    if (Reg->wPropertyDataType < REG_SZ || Reg->wPropertyDataType > REG_MULTI_SZ)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20RegistryType, FALSE);
        return FALSE;
    }

    /* Any number of registry properties is fine */
    Info->Flags |= HUB_MSOS20_REGISTRY;
    return TRUE;
}

static
BOOLEAN
NTAPI
CheckMsOs20ResumeTime(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc)
{
    HubMsOs20ResumeTime* Resume = (HubMsOs20ResumeTime*)Desc;
    BOOLEAN Valid = TRUE;

    if (Info->Flags & HUB_MSOS20_RESUME_TIME)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20MultipleResumeTime, FALSE);
        Valid = FALSE;
    }

    if (Resume->wLength != sizeof(*Resume))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20ResumeTimeLength, FALSE);
        return FALSE;
    }

    if (Resume->bResumeRecoveryTime > 10)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20ResumeRecovery, FALSE);
        Valid = FALSE;
    }

    /* The hub drives resume signaling for at most 20 ms */
    if (Resume->bResumeSignalingTime == 0 || Resume->bResumeSignalingTime > 20)
        HubDescLog(Ctx, HubDescCode::MsOs20ResumeSignaling, TRUE);

    if (Valid)
    {
        Info->ResumeTime = Resume;
        Info->Flags |= HUB_MSOS20_RESUME_TIME;
    }

    return Valid;
}

static
BOOLEAN
NTAPI
CheckMsOs20ModelId(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc)
{
    HubMsOs20ModelId* Model = (HubMsOs20ModelId*)Desc;

    if (Info->Flags & HUB_MSOS20_MODEL_ID)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20MultipleModelId, FALSE);
        return FALSE;
    }

    if (Model->wLength != sizeof(*Model))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20ModelIdLength, FALSE);
        return FALSE;
    }

    if (IsAllZero(Model->ModelId, sizeof(Model->ModelId)))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20ModelIdZero, FALSE);
        return FALSE;
    }

    Info->ModelId = Model;
    Info->Flags |= HUB_MSOS20_MODEL_ID;
    return TRUE;
}

static
BOOLEAN
NTAPI
CheckMsOs20Ccgp(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc)
{
    if (Info->Flags & HUB_MSOS20_CCGP)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20MultipleCcgp, FALSE);
        return FALSE;
    }

    if (Desc->wLength != sizeof(HubMsOs20Ccgp))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20CcgpLength, FALSE);
        return FALSE;
    }

    Info->Ccgp = (HubMsOs20Ccgp*)Desc;
    Info->Flags |= HUB_MSOS20_CCGP;
    return TRUE;
}

static
BOOLEAN
NTAPI
CheckMsOs20Feature(
    _In_ const HubDescContext* Ctx,
    _Inout_ HubMsOs20Info* Info,
    _In_ HubMsOs20Common* Desc,
    _In_ PUCHAR End,
    _In_ ULONG Depth)
{
    switch (Desc->wDescriptorType)
    {
        case HUB_MSOS20_TYPE_SET_HEADER:
            return CheckMsOs20SetHeader(Ctx, Info, Desc);

        case HUB_MSOS20_TYPE_CONFIG_SUBSET:
        case HUB_MSOS20_TYPE_FUNCTION_SUBSET:
            return CheckMsOs20Subset(Ctx, Info, Desc, End, Depth);

        case HUB_MSOS20_TYPE_COMPATIBLE_ID:
            return CheckMsOs20CompatibleId(Ctx, Info, Desc);

        case HUB_MSOS20_TYPE_REGISTRY:
            return CheckMsOs20Registry(Ctx, Info, Desc);

        case HUB_MSOS20_TYPE_RESUME_TIME:
            return CheckMsOs20ResumeTime(Ctx, Info, Desc);

        case HUB_MSOS20_TYPE_MODEL_ID:
            return CheckMsOs20ModelId(Ctx, Info, Desc);

        case HUB_MSOS20_TYPE_CCGP:
            return CheckMsOs20Ccgp(Ctx, Info, Desc);

        default:
            return TRUE;
    }
}

static
BOOLEAN
NTAPI
WalkMsOs20Set(
    _In_ const HubDescContext* Ctx,
    _In_reads_bytes_(Length) PVOID Set,
    _In_ ULONG Length,
    _In_ const HubMsOs20SetInfo* SetInfo,
    _Inout_ HubMsOs20Info* Info)
{
    HubMsOs20SetHeader* Header = (HubMsOs20SetHeader*)Set;
    HubMsOs20Common* Cursor;
    PUCHAR End;
    BOOLEAN Valid = TRUE;

    if (Length != SetInfo->wLength)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20BytesVsSetInfo, FALSE);
        return FALSE;
    }

    if (Length < sizeof(*Header))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20BytesTooSmall, FALSE);
        return FALSE;
    }

    if (Header->wLength != sizeof(*Header))
    {
        HubDescLog(Ctx, HubDescCode::MsOs20HeaderLength, FALSE);
        return FALSE;
    }

    if (Header->wTotalLength != SetInfo->wLength)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20TotalLengthMismatch, FALSE);
        return FALSE;
    }

    if (Header->dwWindowsVersion > SetInfo->dwWindowsVersion)
    {
        HubDescLog(Ctx, HubDescCode::MsOs20VersionMismatch, FALSE);
        return FALSE;
    }

    /* The walk starts at the set header itself */
    End = (PUCHAR)Header + Header->wTotalLength;
    Cursor = (HubMsOs20Common*)Header;
    do
    {
        ULONG Type = Cursor->wDescriptorType;

        if (Type <= HUB_MSOS20_TYPE_CCGP)
        {
            if (!CheckMsOs20Feature(Ctx, Info, Cursor, End, 0))
                Valid = FALSE;

            /* Top level features must come before any subset */
            if (Type != HUB_MSOS20_TYPE_CONFIG_SUBSET &&
                Type != HUB_MSOS20_TYPE_FUNCTION_SUBSET &&
                (Info->Flags & (HUB_MSOS20_CONFIG_SUBSET | HUB_MSOS20_FUNCTION_SUBSET)))
            {
                HubDescLog(Ctx, HubDescCode::MsOs20FeatureOutOfOrder, FALSE);
                Valid = FALSE;
            }
        }

        if (!HubDescNextMsOs20(End, &Cursor))
        {
            HubDescLog(Ctx, HubDescCode::MsOs20PastEnd, FALSE);
            return FALSE;
        }
    } while (Cursor != NULL);

    return Valid;
}

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
NTAPI
HubDescCheckMsOs20Set(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) PVOID Set,
    _In_ ULONG Length,
    _In_ const HubMsOs20SetInfo* SetInfo,
    _Inout_ HubMsOs20Info* Info)
{
    if (WalkMsOs20Set(Context, Set, Length, SetInfo, Info))
        return TRUE;

    /* QUIRK: this also drops the set info and alternate enumeration bits from the BOS */
    Info->Flags = 0;
    return FALSE;
}

/* Hub descriptors */

BOOLEAN
NTAPI
HubDescCheck20Hub(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length)
{
    Verdict Result = Verdict::Ok;
    ULONG Fixed = FIELD_OFFSET(USB_HUB_DESCRIPTOR, bRemoveAndPowerMask);
    ULONG Ports;
    ULONG RemovableBytes;
    ULONG PowerBytes;
    ULONG Expected;
    ULONG Used;
    UCHAR Spare;
    ULONG i;

    if (Descriptor == NULL)
    {
        HubDescLog(Context, HubDescCode::Hub20Null, FALSE);
        return FALSE;
    }

    if (Length < Fixed)
    {
        HubDescLog(Context, HubDescCode::Hub20BufferTooSmall, FALSE);
        return FALSE;
    }

    Ports = Descriptor->bNumberOfPorts;
    if (Ports == 0)
        Note(Context, &Result, HubDescCode::Hub20NoPorts, Gate::Always);

    /* DeviceRemovable has a reserved bit 0 plus one bit per port; the power mask is legacy */
    RemovableBytes = (Ports + 8) / 8;
    PowerBytes = (Ports + 7) / 8;
    Expected = Fixed + RemovableBytes + PowerBytes;

    if (Descriptor->bDescriptorLength < Expected)
        Note(Context, &Result, HubDescCode::Hub20LengthTooSmall, Gate::NewRule);

    if (Descriptor->bDescriptorLength > Expected)
        Note(Context, &Result, HubDescCode::Hub20LengthTooLarge, Gate::Versioned);

    if (Descriptor->bDescriptorType != USB_20_HUB_DESCRIPTOR_TYPE)
        Note(Context, &Result, HubDescCode::Hub20WrongType, Gate::NewRule);

    if (Descriptor->wHubCharacteristics & 0x0002)
        Note(Context, &Result, HubDescCode::Hub20CharacteristicsReserved, Gate::Reserved);

    if (Descriptor->wHubCharacteristics & 0xFF00)
        Note(Context, &Result, HubDescCode::Hub20CharacteristicsReserved, Gate::Reserved);

    if (Descriptor->bRemoveAndPowerMask[0] & 0x01)
        Note(Context, &Result, HubDescCode::Hub20RemovableReserved, Gate::Reserved);

    /* Bits past the last port in the last DeviceRemovable byte */
    Used = (Ports + 1) % 8;
    if (Used != 0)
    {
        Spare = (UCHAR)(0xFF << Used);
        if (Descriptor->bRemoveAndPowerMask[RemovableBytes - 1] & Spare)
            Note(Context, &Result, HubDescCode::Hub20RemovableMissingPort, Gate::Versioned);
    }

    for (i = 0; i < PowerBytes; i++)
    {
        if (Descriptor->bRemoveAndPowerMask[RemovableBytes + i] != 0xFF)
            Note(Context, &Result, HubDescCode::Hub20PowerMaskNotOnes, Gate::Versioned);
    }

    return Result == Verdict::Ok;
}

BOOLEAN
NTAPI
HubDescCheck30Hub(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_30_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length)
{
    Verdict Result = Verdict::Ok;
    ULONG Ports;
    ULONG Spare;

    if (Descriptor == NULL)
    {
        DPRINT1("Hub %p has no SuperSpeed hub descriptor\n", Context->LogContext);
        return FALSE;
    }

    if (Length < sizeof(USB_COMMON_DESCRIPTOR))
    {
        HubDescLog(Context, HubDescCode::Hub30BufferTooSmall, FALSE);
        return FALSE;
    }

    if (Descriptor->bLength < sizeof(*Descriptor))
        Note(Context, &Result, HubDescCode::Hub30LengthTooSmall, Gate::Always);

    if (Descriptor->bLength > sizeof(*Descriptor))
        Note(Context, &Result, HubDescCode::Hub30LengthTooLarge, Gate::Versioned);

    if (Descriptor->bDescriptorType != USB_30_HUB_DESCRIPTOR_TYPE)
        Note(Context, &Result, HubDescCode::Hub30WrongType, Gate::Always);

    Ports = Descriptor->bNumberOfPorts;
    if (Ports > 15)
        Note(Context, &Result, HubDescCode::Hub30TooManyPorts, Gate::Always);

    if (Ports == 0)
        Note(Context, &Result, HubDescCode::Hub30NoPorts, Gate::Always);

    if (Descriptor->wHubCharacteristics & 0x0002)
        Note(Context, &Result, HubDescCode::Hub30CharacteristicsReserved, Gate::Reserved);

    if (Descriptor->wHubCharacteristics & 0xFFE0)
        Note(Context, &Result, HubDescCode::Hub30CharacteristicsReserved, Gate::Reserved);

    if (Descriptor->bHubHdrDecLat >= 11)
        Note(Context, &Result, HubDescCode::Hub30DecodeLatencyReserved, Gate::Versioned);

    if (Descriptor->DeviceRemovable & 0x0001)
        Note(Context, &Result, HubDescCode::Hub30RemovableReserved, Gate::Reserved);

    /* Bits above the last port; with 15 or more ports none are left */
    Spare = (Ports + 1 < 16) ? (0xFFFFUL << (Ports + 1)) & 0xFFFF : 0;
    if (Descriptor->DeviceRemovable & Spare)
        Note(Context, &Result, HubDescCode::Hub30RemovableMissingPort, Gate::Versioned);

    return Result == Verdict::Ok;
}

/* Search helpers */

PUSB_COMMON_DESCRIPTOR
NTAPI
HubDescFind(
    _In_reads_bytes_(TotalLength) PVOID Buffer,
    _In_ ULONG TotalLength,
    _In_ PVOID Position,
    _In_ UCHAR Type)
{
    PUCHAR End = (PUCHAR)Buffer + TotalLength;
    PUCHAR At = (PUCHAR)Position;

    /* bLength is trusted; only the two header bytes are kept inside the buffer */
    while (At + sizeof(USB_COMMON_DESCRIPTOR) <= End)
    {
        PUSB_COMMON_DESCRIPTOR Desc = (PUSB_COMMON_DESCRIPTOR)At;

        if (Desc->bLength == 0)
            return NULL;

        if (Desc->bDescriptorType == Type)
            return Desc;

        At += Desc->bLength;
    }

    return NULL;
}

static
BOOLEAN
NTAPI
FieldMatches(
    _In_ LONG Wanted,
    _In_ UCHAR Actual)
{
    return Wanted == -1 || Wanted == (LONG)Actual;
}

PUSB_INTERFACE_DESCRIPTOR
NTAPI
HubDescFindInterface(
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Config,
    _In_ PVOID Position,
    _In_ LONG Number,
    _In_ LONG Alternate,
    _In_ LONG Class,
    _In_ LONG SubClass,
    _In_ LONG Protocol,
    _Out_opt_ PBOOLEAN HasAlternates)
{
    PUSB_INTERFACE_DESCRIPTOR Match;
    PUSB_INTERFACE_DESCRIPTOR Next;
    ULONG SameNumber = 0;
    PUCHAR At = (PUCHAR)Position;

    if (HasAlternates != NULL)
        *HasAlternates = FALSE;

    if (Config->bLength < sizeof(*Config) ||
        Config->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE ||
        Config->wTotalLength < sizeof(*Config))
    {
        return NULL;
    }

    for (;;)
    {
        Match = (PUSB_INTERFACE_DESCRIPTOR)HubDescFind(Config, Config->wTotalLength, At, USB_INTERFACE_DESCRIPTOR_TYPE);
        if (Match == NULL)
            return NULL;

        /* Only the 2 byte header is known to be inside wTotalLength */
        if ((PUCHAR)Match + sizeof(*Match) > (PUCHAR)Config + Config->wTotalLength)
            return NULL;

        At = (PUCHAR)Match + Match->bLength;

        /* Every interface with the requested number counts, matching or not */
        if (Number != -1 && Match->bInterfaceNumber == Number)
            SameNumber++;

        if (FieldMatches(Number, Match->bInterfaceNumber) &&
            FieldMatches(Alternate, Match->bAlternateSetting) &&
            FieldMatches(Class, Match->bInterfaceClass) &&
            FieldMatches(SubClass, Match->bInterfaceSubClass) &&
            FieldMatches(Protocol, Match->bInterfaceProtocol))
        {
            break;
        }
    }

    if (HasAlternates == NULL)
        return Match;

    /* QUIRK: only the very next interface is looked at, and never for a wildcard number */
    if (SameNumber <= 1)
    {
        Next = (PUSB_INTERFACE_DESCRIPTOR)HubDescFind(Config, Config->wTotalLength, At, USB_INTERFACE_DESCRIPTOR_TYPE);
        if (Next != NULL && Number != -1 &&
            (PUCHAR)Next + sizeof(*Next) <= (PUCHAR)Config + Config->wTotalLength &&
            Next->bInterfaceNumber == Number)
            SameNumber++;
    }

    if (SameNumber > 1)
        *HasAlternates = TRUE;

    return Match;
}
