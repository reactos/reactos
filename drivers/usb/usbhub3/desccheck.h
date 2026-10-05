/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Descriptor validation for devices and hubs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/*
 * Validation codes, used as the ETW payload and the validation bitmap bit
 * index. The numbering must never change; gaps are reserved codes.
 */
enum class HubDescCode : ULONG
{
    Hub20LengthTooLarge                 = 0,
    Hub20LengthTooSmall                 = 1,
    Hub20Null                           = 2,
    Hub20BufferTooSmall                 = 3,
    Hub20RemovableMissingPort           = 4,
    Hub20RemovableReserved              = 5,
    Hub20WrongType                      = 6,
    Hub20NoPorts                        = 7,
    Hub20PowerMaskNotOnes               = 8,
    Hub20CharacteristicsReserved        = 9,
    Hub30DecodeLatencyReserved          = 10,
    Hub30LengthTooLarge                 = 11,
    Hub30LengthTooSmall                 = 12,
    Hub30BufferTooSmall                 = 13,
    Hub30RemovableMissingPort           = 14,
    Hub30RemovableReserved              = 15,
    Hub30WrongType                      = 16,
    Hub30TooManyPorts                   = 17,
    Hub30NoPorts                        = 18,
    Hub30CharacteristicsReserved        = 19,
    BosLengthTooLarge                   = 20,
    BosLengthTooSmall                   = 21,
    BosBufferLargerThanTotal            = 22,
    BosBufferTooSmall                   = 23,
    BosWrongType                        = 24,
    BosCapCountMismatch                 = 25,
    BosNoSuperSpeedCap                  = 26,
    BosNull                             = 27,
    BosUnknownType                      = 28,
    BosTotalLengthTooSmall              = 29,
    BulkAtLowSpeed                      = 30,
    BulkAttributesReserved              = 31,
    BulkMaxPacket                       = 32,
    CompanionBulkReserved               = 33,
    CompanionBulkBurst                  = 34,
    CompanionBulkStreams                = 35,
    CompanionBulkBytesPerInterval       = 36,
    CompanionControlAttributes          = 37,
    CompanionControlBurst               = 38,
    CompanionControlBytesPerInterval    = 39,
    CompanionNotSuperSpeed              = 40,
    CompanionUnexpected                 = 41,
    CompanionLengthTooLarge             = 42,
    CompanionLengthTooSmall             = 43,
    CompanionBufferTooSmall             = 44,
    CompanionInterruptBurst             = 45,
    CompanionInterruptBurstPacket       = 46,
    CompanionInterruptAttributes        = 47,
    CompanionInterruptBytesPerInterval  = 48,
    CompanionIsochReserved              = 49,
    CompanionIsochBurst                 = 50,
    CompanionIsochBurstPacket           = 51,
    CompanionIsochMult                  = 52,
    CompanionIsochBytesPerInterval      = 53,
    ConfigLengthTooLarge                = 54,
    ConfigLengthTooSmall                = 55,
    ConfigBufferTooSmall                = 56,
    ConfigBufferTooSmallForInterfaces   = 57,
    ConfigWrongType                     = 58,
    ConfigMissingCompanion              = 59,
    ConfigNull                          = 60,
    ConfigTotalLengthTooLarge           = 61,
    ConfigTotalLengthTooSmall           = 62,
    ContainerIdLengthTooLarge           = 63,
    ContainerIdLengthTooSmall           = 64,
    ContainerIdBufferTooSmall           = 65,
    ContainerIdReserved                 = 66,
    ControlAttributesReserved           = 67,
    ControlMaxPacket                    = 68,
    DeviceVersionNotBcd                 = 69,
    DeviceLengthTooSmall                = 70,
    DeviceBufferTooSmall                = 71,
    CapLengthTooSmall                   = 72,
    CapBufferTooSmall                   = 73,
    CapMultipleContainerId              = 74,
    CapMultipleSuperSpeed               = 75,
    CapMultipleUsb20                    = 76,
    DeviceWrongType                     = 77,
    DeviceMaxPacket0                    = 78,
    DeviceNull                          = 79,
    EndpointLengthTooLarge              = 80,
    EndpointLengthTooSmall              = 81,
    EndpointAttributesReserved          = 82,
    EndpointBufferTooSmall              = 83,
    EndpointAddressReserved             = 84,
    EndpointBeforeInterface             = 85,
    EndpointNumberZero                  = 86,
    HeaderPastBuffer                    = 87,
    HeaderPastTotalLength               = 88,
    HeaderLengthTooSmall                = 89,
    HeaderBufferTooSmall                = 90,
    HeaderLengthZero                    = 91,
    IadLengthTooLarge                   = 92,
    IadLengthTooSmall                   = 93,
    IadBufferTooSmall                   = 94,
    IadCountTooLarge                    = 95,
    IadCountZero                        = 96,
    IadFirstInterfaceTooLarge           = 97,
    InterfaceLengthTooLarge             = 98,
    InterfaceLengthTooSmall             = 99,
    InterfaceBufferTooSmall             = 100,
    DuplicateAlternate                  = 101,
    DuplicateEndpoint                   = 102,
    DuplicateInterface                  = 103,
    FirstAlternateNotZero               = 104,
    AlternateOutOfOrder                 = 105,
    InterfaceOutOfOrder                 = 106,
    EndpointCountMismatch               = 107,
    InterruptAttributesReserved         = 108,
    InterruptInterval                   = 109,
    InterruptMaxPacket                  = 110,
    IsochAtLowSpeed                     = 111,
    IsochInterval                       = 112,
    IsochMaxPacket                      = 113,
    MsOsContainerIdZero                 = 114,
    MsOsContainerIdVersion              = 115,
    MsOsContainerIdIndex                = 116,
    MsOsContainerIdBufferSize           = 117,
    MsOsContainerIdLength               = 118,
    MsOsExtConfigFirstInterface         = 119,
    MsOsExtConfigVersion                = 120,
    MsOsExtConfigCount                  = 121,
    MsOsExtConfigBufferSize             = 122,
    MsOsExtConfigCompatibleId           = 123,
    MsOsExtConfigSubCompatibleId        = 124,
    MsOsExtConfigIndex                  = 125,
    MsOsExtConfigBufferVsLength         = 126,
    MsOsExtConfigLengthVsCount          = 127,
    StringLengthOdd                     = 128,
    StringLengthTooLarge                = 129,
    StringLengthTooSmall                = 130,
    StringBufferTooSmall                = 131,
    SerialNumberBadCharacter            = 132,
    StringWrongType                     = 133,
    StringLengthVsBytes                 = 134,
    SsCapLengthTooLarge                 = 135,
    SsCapLengthTooSmall                 = 136,
    SsCapAttributesReserved             = 137,
    SsCapU1TooLarge                     = 138,
    SsCapU2TooLarge                     = 139,
    SsCapBufferTooSmall                 = 140,
    SsCapFunctionalityNotInSpeeds       = 141,
    SsCapFunctionalityReserved          = 142,
    SsCapSpeedsReserved                 = 143,
    SsCapNoSpeeds                       = 144,
    Usb20CapLengthTooLarge              = 145,
    Usb20CapLengthTooSmall              = 146,
    Usb20CapReserved                    = 147,
    Usb20CapBufferTooSmall              = 148,
    BaselineBeslZero                    = 149,
    BeslWithoutLpm                      = 150,
    DeepBeslNotAboveBaseline            = 151,
    DeepBeslZero                        = 152,
    BosContainerIdZero                  = 153,
    MsOs20MultipleSetHeaders            = 154,
    MsOs20SetHeaderLength               = 155,
    ConfigSubsetLength                  = 156,
    ConfigSubsetTotalTooSmall           = 157,
    ConfigSubsetTotalTooLarge           = 158,
    ConfigSubsetMisplacedFeature        = 159,
    ConfigSubsetFeatureLength           = 160,
    FunctionSubsetLength                = 161,
    FunctionSubsetTotalTooSmall         = 162,
    FunctionSubsetTotalTooLarge         = 163,
    FunctionSubsetMisplacedFeature      = 164,
    FunctionSubsetFeatureLength         = 165,
    MsOs20MultipleCompatibleId          = 166,
    MsOs20CompatibleIdLength            = 167,
    MsOs20CompatibleIdAfterNull         = 168,
    MsOs20RegistryTooSmall              = 169,
    MsOs20RegistryNameTooLarge          = 170,
    MsOs20RegistryNameOddOrZero         = 171,
    MsOs20RegistryDataTooLarge          = 172,
    MsOs20RegistryDataZero              = 173,
    MsOs20RegistryType                  = 174,
    MsOs20MultipleResumeTime            = 175,
    MsOs20ResumeTimeLength              = 176,
    MsOs20ResumeRecovery                = 177,
    MsOs20ResumeSignaling               = 178,
    MsOs20MultipleModelId               = 181,
    MsOs20ModelIdLength                 = 182,
    MsOs20ModelIdZero                   = 183,
    MsOs20BytesVsSetInfo                = 188,
    MsOs20BytesTooSmall                 = 189,
    MsOs20HeaderLength                  = 190,
    MsOs20TotalLengthMismatch           = 191,
    MsOs20VersionMismatch               = 192,
    MsOs20FeatureOutOfOrder             = 193,
    MsOs20PastEnd                       = 194,
    PlatformReserved                    = 195,
    PlatformUuidZero                    = 196,
    MsOs20MultiplePlatform              = 197,
    MsOs20PlatformLength                = 198,
    MsOs20DuplicateVersion              = 199,
    MsOs20NoVersionMatch                = 200,
    MsOs20MultipleCcgp                  = 201,
    MsOs20CcgpLength                    = 202,
    CapMultipleSuperSpeedPlus           = 203,
    CompanionIsochBytesNotOne           = 204,
    SspCompanionNotSuperSpeed           = 205,
    SspCompanionUnexpected              = 206,
    SspCompanionLengthTooLarge          = 207,
    SspCompanionLengthTooSmall          = 208,
    SspCompanionBufferTooSmall          = 209,
    SspCompanionReserved                = 210,
    SspCompanionBytesRange              = 211,
    ConfigMissingSspCompanion           = 212,
    SspCapLengthInvalid                 = 213,
    SspCapLengthTooSmall                = 214,
    SspCapAttributesReserved            = 215,
    SspCapFunctionalityReserved         = 216,
    SspCapBufferTooSmall                = 217,
    SspCapReservedByte                  = 218,
    SspCapReservedWord                  = 219,
    SspAttrNotRx                        = 220,
    SspAttrNotTx                        = 221,
    SspAttrTxMissing                    = 222,
    SspAttrPairMismatch                 = 223,
    SspAttrSymmetricMismatch            = 224,
    SspMinLaneMismatch                  = 225,
    SspSpeedIdDuplicate                 = 226,
    SspSpeedIdCount                     = 227,
    SspMinSpeedMissing                  = 228,
    PdLengthTooLarge                    = 229,
    PdLengthTooSmall                    = 230,
    PdBufferTooSmall                    = 231,
    PdReserved                          = 232,
    IsochPacketInAlternate0             = 233,
    DuplicateSerialNumber               = 234,
    BillboardLengthTooSmall             = 235,
    BillboardLengthInvalid              = 236,
    BillboardBufferTooSmall             = 237,
    BillboardNoModes                    = 238,
    BillboardTooManyModes               = 239,
    BillboardPreferredInvalid           = 241,
    CapMultipleBillboard                = 245,
    BillboardDeviceVersion              = 246,
    PlatformFeaturesLength              = 247,
    PlatformFeaturesMultiple            = 248,
    PlatformFeaturesVersion             = 249,
    PlatformFeaturesTooSmall            = 250,
    PlatformLengthTooSmall              = 251,
    PlatformBufferTooSmall              = 253,
    Count                               = 254
};

/* The device validation bitmap: 256 bits */
#define HUB_DESC_BITMAP_ULONGS 8

/** Logging hook. Called for every finding after the bitmap bit is set. */
typedef
VOID
(NTAPI *PFN_HUB_DESC_LOG)(
    _In_opt_ PVOID Context,
    _In_ HubDescCode Code,
    _In_ BOOLEAN Warning);

/** What every validator needs to know. Build it with one of the init routines. */
struct HubDescContext
{
    /* Version the strictness gates use, not necessarily the one in the descriptor */
    USHORT BcdUsb;

    /* SuperSpeedPlus devices report UsbSuperSpeed */
    USB_DEVICE_SPEED Speed;

    /* From the hub global flags; always FALSE for hub descriptors */
    BOOLEAN Strictest;
    BOOLEAN LegacyStrict;
    BOOLEAN ReservedFields;

    /* Per device lenient validation mode (UseWin8DescriptorValidation errata) */
    BOOLEAN Win8Behavior;

    /* Findings belong to the hub itself; only changes the default log text */
    BOOLEAN ForHub;

    /* Nonzero when the port supports SuperSpeedPlus isochronous bursting */
    ULONG SspIsochBurstCount;

    /* HUB_DESC_BITMAP_ULONGS words, or NULL */
    PULONG Bitmap;

    /* NULL logs with DPRINT */
    PFN_HUB_DESC_LOG Log;
    PVOID LogContext;
};

/** USB 2.0 LPM attributes collected from the BOS. Only ever set, never cleared. */
struct HubLpmInfo
{
    BOOLEAN Capable;
    BOOLEAN BeslAndAltHird;
    BOOLEAN BaselineValid;
    BOOLEAN DeepValid;
    UCHAR Baseline;
    UCHAR Deep;
};

/* Device flags the BOS walk sets (in/out, only ever set) */
#define HUB_BOS_SUPERSPEED_CAPABLE          0x00000001
#define HUB_BOS_ENHANCED_SUPERSPEED_CAPABLE 0x00000002
#define HUB_BOS_CHARGING_POLICY             0x00000004

#include <pshpack1.h>

/* MS OS 2.0 descriptor set information entry in the BOS platform capability */
struct HubMsOs20SetInfo
{
    ULONG dwWindowsVersion;
    USHORT wLength;
    UCHAR bVendorCode;
    UCHAR bAltEnumCode;
};

/* USB platform features (dual role) capability, version 1 */
struct HubPlatformFeatures
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    UCHAR bDevCapabilityType;
    UCHAR bReserved;
    GUID PlatformCapabilityUuid;
    UCHAR Version;
    UCHAR VendorCommand;
    ULONG Features;
};

/* MS OS 2.0 descriptors */
struct HubMsOs20Common
{
    USHORT wLength;
    USHORT wDescriptorType;
};

struct HubMsOs20SetHeader
{
    USHORT wLength;
    USHORT wDescriptorType;
    ULONG dwWindowsVersion;
    USHORT wTotalLength;
};

/* Configuration subset (bValue = configuration) or function subset (bValue = first interface) */
struct HubMsOs20Subset
{
    USHORT wLength;
    USHORT wDescriptorType;
    UCHAR bValue;
    UCHAR bReserved;
    USHORT wTotalLength;
};

struct HubMsOs20CompatibleId
{
    USHORT wLength;
    USHORT wDescriptorType;
    UCHAR CompatibleId[8];
    UCHAR SubCompatibleId[8];
};

/* Followed by the name, a USHORT data length and the data */
struct HubMsOs20Registry
{
    USHORT wLength;
    USHORT wDescriptorType;
    USHORT wPropertyDataType;
    USHORT wPropertyNameLength;
};

struct HubMsOs20ResumeTime
{
    USHORT wLength;
    USHORT wDescriptorType;
    UCHAR bResumeRecoveryTime;
    UCHAR bResumeSignalingTime;
};

struct HubMsOs20ModelId
{
    USHORT wLength;
    USHORT wDescriptorType;
    UCHAR ModelId[16];
};

struct HubMsOs20Ccgp
{
    USHORT wLength;
    USHORT wDescriptorType;
};

/* MS OS 1.0 descriptors */
struct HubMsOsString
{
    UCHAR bLength;
    UCHAR bDescriptorType;
    WCHAR Signature[7];
    UCHAR bVendorCode;
    UCHAR bPad;
};

struct HubMsOsHeader
{
    ULONG dwLength;
    USHORT bcdVersion;
    USHORT wIndex;
};

struct HubMsOsContainerId
{
    HubMsOsHeader Header;
    UCHAR ContainerId[16];
};

struct HubMsOsExtConfigHeader
{
    ULONG dwLength;
    USHORT bcdVersion;
    USHORT wIndex;
    UCHAR bCount;
    UCHAR Reserved[7];
};

struct HubMsOsExtConfigFunction
{
    UCHAR bFirstInterfaceNumber;
    UCHAR bInterfaceCount;
    UCHAR CompatibleId[8];
    UCHAR SubCompatibleId[8];
    UCHAR Reserved[6];
};

struct HubMsOsExtConfig
{
    HubMsOsExtConfigHeader Header;
    HubMsOsExtConfigFunction Function[1];
};

#include <poppack.h>

C_ASSERT(sizeof(HubMsOs20SetInfo) == 8);
C_ASSERT(sizeof(HubPlatformFeatures) == 26);
C_ASSERT(sizeof(HubMsOs20SetHeader) == 10);
C_ASSERT(sizeof(HubMsOs20Subset) == 8);
C_ASSERT(sizeof(HubMsOs20CompatibleId) == 20);
C_ASSERT(sizeof(HubMsOsString) == 18);
C_ASSERT(sizeof(HubMsOsContainerId) == 24);
C_ASSERT(sizeof(HubMsOsExtConfigHeader) == 16);
C_ASSERT(sizeof(HubMsOsExtConfig) == 40);

/** Capability summary of a validated BOS. Pointers point into the validated buffer. */
struct HubBosInfo
{
    PUSB_DEVICE_CAPABILITY_USB20_EXTENSION_DESCRIPTOR Usb20;
    PUSB_DEVICE_CAPABILITY_SUPERSPEED_USB_DESCRIPTOR SuperSpeed;
    PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR SuperSpeedPlus;

    /* NULL when the device reported an all zero container ID */
    PUSB_DEVICE_CAPABILITY_CONTAINER_ID_DESCRIPTOR ContainerId;

    PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Billboard;

    /* Entry chosen for this OS version, or NULL */
    HubMsOs20SetInfo* MsOs20SetInfo;

    HubPlatformFeatures* PlatformFeatures;

    BOOLEAN DiscardContainerId;
    BOOLEAN ChargingPolicy;
};

/* HubMsOs20Info.Flags */
#define HUB_MSOS20_SET_INFO         0x00000001
#define HUB_MSOS20_SET_HEADER       0x00000002
#define HUB_MSOS20_ALT_ENUM         0x00000004
#define HUB_MSOS20_CONFIG_SUBSET    0x00000008
#define HUB_MSOS20_FUNCTION_SUBSET  0x00000010
#define HUB_MSOS20_COMPATIBLE_ID    0x00000020
#define HUB_MSOS20_REGISTRY         0x00000040
#define HUB_MSOS20_RESUME_TIME      0x00000080
#define HUB_MSOS20_MODEL_ID         0x00000100
#define HUB_MSOS20_CCGP             0x00000200

/** What the MS OS 2.0 set validation found. Pointers point into the set. */
struct HubMsOs20Info
{
    ULONG Flags;
    HubMsOs20CompatibleId* CompatibleId;
    HubMsOs20ResumeTime* ResumeTime;
    HubMsOs20ModelId* ModelId;
    HubMsOs20Ccgp* Ccgp;
};

/* Context setup */

/** Device context: reads the global strictness flags. Set Log and LogContext afterwards. */
VOID
NTAPI
HubDescInitDeviceContext(
    _Out_ HubDescContext* Context,
    _In_ USHORT BcdUsb,
    _In_ USB_DEVICE_SPEED Speed,
    _In_ BOOLEAN Win8Behavior,
    _In_ ULONG SspIsochBurstCount,
    _In_opt_ PULONG Bitmap);

/** Hub context: version and speed only, global flags are never applied. */
VOID
NTAPI
HubDescInitHubContext(
    _Out_ HubDescContext* Context,
    _In_ USHORT BcdUsb,
    _In_ USB_DEVICE_SPEED Speed);

/** Name of a finding for debug prints. */
PCSTR
NTAPI
HubDescCodeName(
    _In_ HubDescCode Code);

/** Records one finding: sets the bitmap bit and calls the log hook. */
VOID
NTAPI
HubDescLog(
    _In_ const HubDescContext* Context,
    _In_ HubDescCode Code,
    _In_ BOOLEAN Warning);

/* Validators. FALSE means the caller fails the operation. */

/** Device descriptor. Length is the bytes returned; IsBillboard is only ever set TRUE. */
BOOLEAN
NTAPI
HubDescCheckDevice(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_DEVICE_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Inout_opt_ PBOOLEAN IsBillboard);

/** Whole configuration descriptor set. SupportsStreams reflects the last companion. */
BOOLEAN
NTAPI
HubDescCheckConfiguration(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Out_opt_ PBOOLEAN SupportsStreams);

/** The 5 byte BOS header on its own. BosHeaderValid. */
BOOLEAN
NTAPI
HubDescCheckBosHeader(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_BOS_DESCRIPTOR Descriptor,
    _In_ ULONG Length);

/**
 * Whole BOS set. Outputs are filled even on FALSE, and a Billboard capability
 * with bLength 40 or 44 gets its bLength fixed in the buffer.
 */
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
    _Inout_opt_ PULONG DeviceFlags);

/** String descriptor. AdjustedLength is the usable byte count. */
BOOLEAN
NTAPI
HubDescCheckString(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) PUSB_STRING_DESCRIPTOR Descriptor,
    _In_ ULONG Length,
    _Out_opt_ PULONG AdjustedLength);

/**
 * Serial number string. Length is the bytes returned on entry and the string
 * bytes plus room for a NUL on success.
 */
BOOLEAN
NTAPI
HubDescCheckSerialNumber(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(*Length) PUSB_STRING_DESCRIPTOR Descriptor,
    _Inout_ PULONG Length);

/** MS OS string descriptor (index 0xEE): signature only. MsOsDescriptorValid. */
BOOLEAN
NTAPI
HubDescCheckMsOsString(
    _In_ const HubDescContext* Context,
    _In_ const HubMsOsString* Descriptor);

/** MS OS 1.0 container ID header. ContainerIdHeaderValid. */
BOOLEAN
NTAPI
HubDescCheckMsOsContainerIdHeader(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) const HubMsOsHeader* Descriptor,
    _In_ ULONG Length);

/** MS OS 1.0 container ID. ContainerIdKnown. */
BOOLEAN
NTAPI
HubDescCheckMsOsContainerId(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) const HubMsOsContainerId* Descriptor,
    _In_ ULONG Length);

/** MS OS 1.0 extended configuration header. ExtendedConfigHeaderValid. */
BOOLEAN
NTAPI
HubDescCheckMsOsExtConfigHeader(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) const HubMsOsExtConfigHeader* Descriptor,
    _In_ ULONG Length);

/**
 * MS OS 1.0 extended configuration. The buffer must hold the 40 bytes the
 * validated header promised, whatever Length says.
 */
BOOLEAN
NTAPI
HubDescCheckMsOsExtConfig(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(sizeof(HubMsOsExtConfig)) const HubMsOsExtConfig* Descriptor,
    _In_ ULONG Length);

/**
 * MS OS 2.0 descriptor set. SetInfo is the entry the BOS walk selected; on
 * failure Info->Flags is cleared.
 */
_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
NTAPI
HubDescCheckMsOs20Set(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_(Length) PVOID Set,
    _In_ ULONG Length,
    _In_ const HubMsOs20SetInfo* SetInfo,
    _Inout_ HubMsOs20Info* Info);

/** USB 2.0 hub descriptor. Hub ParseHubDescriptor for Full and High speed hubs. */
BOOLEAN
NTAPI
HubDescCheck20Hub(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length);

/** USB 3.0 hub descriptor. Hub ParseHubDescriptor for SuperSpeed hubs. */
BOOLEAN
NTAPI
HubDescCheck30Hub(
    _In_ const HubDescContext* Context,
    _In_reads_bytes_opt_(Length) PUSB_30_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length);

/* Search helpers */

/** First descriptor of Type at or after Position, or NULL. Stops on a zero bLength. */
PUSB_COMMON_DESCRIPTOR
NTAPI
HubDescFind(
    _In_reads_bytes_(TotalLength) PVOID Buffer,
    _In_ ULONG TotalLength,
    _In_ PVOID Position,
    _In_ UCHAR Type);

/**
 * First matching interface at or after Position; -1 matches anything.
 * HasAlternates is TRUE when that interface number has other settings.
 */
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
    _Out_opt_ PBOOLEAN HasAlternates);

/** Steps to the next MS OS 2.0 descriptor, or NULL at End. FALSE when a length runs past End. */
BOOLEAN
NTAPI
HubDescNextMsOs20(
    _In_ PUCHAR End,
    _Inout_ HubMsOs20Common** Current);
