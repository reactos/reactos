/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     The device attached to a hub port
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* registry */
#include "desccheck.h"

/* Mux bookkeeping per device, under the hub mux lock */
enum class ChildMuxFlag : ULONG
{
    Stopped                  = 0x00000001,
    PowerReference           = 0x00000002,
    PnpOperationPending      = 0x00000004
};

/* Device errata from usbflags and the shim engine; bit positions are fixed */
enum class ChildHack : ULONG
{
    DisableSerialNumber              = 0x00000001,
    AlwaysQueryMsOs           = 0x00000002,
    ResetAfterSystemResume                  = 0x00000004,
    DisableOnSoftRemove              = 0x00000008,
    RequestConfigDescOnReset         = 0x00000010,
    SkipContainerIdQuery             = 0x00000020,
    IgnoreBosValidationFailure       = 0x00000040,
    DisableLpm                       = 0x00000080,
    NoExitLatencyRequest                       = 0x00000100,
    ResetAfterSuperSpeedResume        = 0x00000200,
    TolerateStalePipes          = 0x00000400,
    DisableUasp                      = 0x00000800,
    NoIsochDelayRequest                = 0x00001000,
    ResetAfterIdleResume                  = 0x00002000,
    DisableHotReset                  = 0x00004000,
    SkipBosQuery                     = 0x00008000,
    NonFunctional                    = 0x00010000,
    DisableUsb20Lpm                  = 0x00020000,
    DisableRemoteWakeForUsb20Lpm     = 0x00040000,
    DisableSuperSpeed                = 0x00080000,
    BlockedDevice          = 0x00100000,
    UseWin8DescriptorValidation      = 0x00200000,
    AlwaysSecondReset           = 0x00400000
};

/* USB4 tunnel state of a device; 1 means the port has no USB4 host and the controller was not asked */
#define HUB_TUNNEL_STATE_NONE           UCXHUB_TUNNEL_STATE_NOT_ANSWERED
#define HUB_TUNNEL_STATE_NOT_USB4       UCXHUB_TUNNEL_STATE_NO_INFORMATION
#define HUB_TUNNEL_STATE_TUNNELED       UCXHUB_TUNNEL_STATE_TUNNELED
#define HUB_TUNNEL_STATE_NATIVE         UCXHUB_TUNNEL_STATE_NATIVE
#define HUB_TUNNEL_STATE_UNKNOWN        UCXHUB_TUNNEL_STATE_UNKNOWN

/* What the device is, learned during enumeration */
enum class ChildProperty : ULONG
{
    NotRemovable                     = 0x00000001,
    IsHub                            = 0x00000002,
    HighSpeedCapable                 = 0x00000004,
    MsOsNotSupported                 = 0x00000080,
    LtmCapable                       = 0x00000100,
    DualRole                         = 0x00000200,
    ChargingPolicySupported          = 0x00000400,
    AcpiAllowsD3Cold            = 0x00000800,
    ExtPropertiesInstalled           = 0x00001000,
    IsComposite                      = 0x00002000,
    HasBillboard                     = 0x00004000,
    SupportsStreams                  = 0x00008000,
    RemoteWakeCapable                = 0x00010000,
    SelfPowered                      = 0x00020000,
    Usb20LpmCapable                  = 0x00040000,
    BeslCapable                      = 0x00080000,

    /* identity */
    ContainerIdKnown                 = 0x10000000,  /**< From MS OS, or generated from the serial number */
    ContainerIdFromBos              = 0x20000000,

    /* registry */
    UxdReserved                      = 0x01000000,  /**< A UXD settings record redirects the device to a VM */
    SupportsSelectiveSuspend         = 0x02000000,
    AllowIdleIrpInD3                 = 0x04000000
};

/* Device state changed by several machines, always interlocked */
enum class ChildState : ULONG
{
    ReprogrammingPending             = 0x00000001,
    DifferentDeviceOnBootPort        = 0x00000002,
    ActivityIdSet                    = 0x00000004,
    AttachSucceeded                  = 0x00000008,
    WarmResetOnEnumeration           = 0x00000010,
    LastSetAddressFailed             = 0x00000020,
    AltSettingFiltered             = 0x00000040,
    ConfigurationValid               = 0x00000080,
    WasResetAtResume                = 0x00000100,
    D3ColdEnabledByDriver            = 0x00000200,
    FailedAnEnumeration              = 0x00000400,
    U1EnabledUpstream                = 0x00000800,
    U2EnabledUpstream                = 0x00001000,
    AltEnumCompleted                 = 0x00002000,
    AltEnumCommandSent               = 0x00004000,
    MsOsInstallHandled               = 0x00008000,
    Disconnected                     = 0x00010000,
    PendingNoPingResponse            = 0x00020000,
    HotResetOnEnumeration            = 0x00040000,

    /* identity */
    WorkingDevice                    = 0x10000000,  /**< The PDO describes a working device, not a placeholder */
    PrefixedSerial             = 0x20000000,

    /* devxfer */
    DualRoleCommandSent              = 0x00800000
};

/* registry */

/* Telemetry bits of the Ceip DeviceInformation value; layout fixed by its readers */
enum class ChildSqm : ULONG
{
    Virtualized                      = 0x00000004,
    IsOnXhci                         = 0x00000008,
    HoldsPagingFiles                 = 0x00000010,
    ValidBos                         = 0x00000020,
    SupportsStreams                  = 0x00000040,
    MultipleConfigurations           = 0x00000080,
    SuperSpeedCapable                = 0x00000200
};

/* UXD settings record written by the VM redirection policy tool */
struct HubUxdSettings
{
    ULONG Version;
    GUID PnpGuid;
    GUID OwnerGuid;
    ULONG DeleteOnShutdown;
    ULONG DeleteOnReload;
    ULONG DeleteOnDisconnect;
    ULONG Reserved[5];
};

C_ASSERT(sizeof(HubUxdSettings) == 68);

/* AlternateSettingFilter from usbflags: alternate settings SELECT_CONFIGURATION picks */
struct HubAltSettingFilter
{
    ULONG Count;
    struct
    {
        UCHAR InterfaceNumber;
        UCHAR AlternateSetting;
    } Entries[ANYSIZE_ARRAY];
};

/* UCX endpoint state of one pipe */
enum class PipeState : UCHAR
{
    Invalid,
    NotCreated,
    PendingCreate,
    PendingEnable,
    Enabled,
    PendingDisable,
    Disabled,
    Deleted
};

/* One endpoint of an interface; its address is the client's pipe handle owner */
struct HubPipe
{
    PUSB_ENDPOINT_DESCRIPTOR Descriptor;
    ULONG BytesToEnd;
    PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion;
    PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR IsochCompanion;
    ULONG PipeFlags;
    PipeState State;
    BOOLEAN ZeroBandwidth;
    UCXENDPOINT Endpoint;
    USBD_PIPE_HANDLE PipeHandle;
};

/* One interface of a configuration; its address is the client's interface handle */
struct HubInterface
{
    WDFMEMORY Memory;
    LIST_ENTRY Link;
    PUSB_INTERFACE_DESCRIPTOR Descriptor;
    ULONG PipeCount;
    BOOLEAN HasAlternateSettings;
    BOOLEAN NeedsSetInterface;
    HubPipe Pipes[ANYSIZE_ARRAY];
};

/* A selected configuration; its address is the client's configuration handle */
struct HubConfiguration
{
    WDFMEMORY Memory;
    LIST_ENTRY Interfaces;
    ULONG EndpointCount;
    USB_CONFIGURATION_DESCRIPTOR Descriptor;
};

/* Every device management request a device sends to UCX, one at a time */
union HubUcxPayload
{
    USBDEVICE_MGMT_HEADER Header;
    ADDRESS0_OWNERSHIP_ACQUIRE Address0;
    USBDEVICE_ENABLE Enable;
    USBDEVICE_RESET Reset;
    USBDEVICE_ADDRESS Address;
    USBDEVICE_UPDATE Update;
    USBDEVICE_HUB_INFO HubInfo;
    USBDEVICE_DISABLE Disable;
    USBDEVICE_PURGEIO Purge;
    USBDEVICE_STARTIO Start;
    USBDEVICE_ABORTIO Abort;
    USBDEVICE_TREE_PURGEIO TreePurge;
    ENDPOINTS_CONFIGURE Configure;
    DEFAULT_ENDPOINT_UPDATE DefaultEndpoint;
    ENDPOINT_RESET EndpointReset;
};

/* devxfer */

/* No alternate mode is being walked */
#define HUB_BILLBOARD_NO_MODE 0xFF

/** Billboard capability of the device; allocated with HUB_TAG_HUB, as is its descriptor copy. */
struct HubBillboardInfo
{
    PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Descriptor;
    UCHAR CurrentMode;
    BOOLEAN ModeError;
    BOOLEAN ModeSuccess;
};

class HubChild
{
public:
    /** Creates the device object on a port and publishes it there. */
    static
    HubChild*
    Create(
        _In_ HubPort* Port);

    static
    HubChild*
    FromObject(
        _In_ WDFOBJECT Object);

    VOID
    Post(
        _In_ DsmEvent Event)
    {
        m_Machine.SmPost(Event);
    }

    BOOLEAN
    HasState(
        _In_ ChildState Flag) const
    {
        return (m_StateFlags & (LONG)Flag) != 0;
    }

    VOID
    SetState(
        _In_ ChildState Flag)
    {
        InterlockedOr(&m_StateFlags, (LONG)Flag);
    }

    VOID
    ClearState(
        _In_ ChildState Flag)
    {
        InterlockedAnd(&m_StateFlags, ~(LONG)Flag);
    }

    BOOLEAN
    HasHack(
        _In_ ChildHack Flag) const
    {
        return (m_Hacks & (ULONG)Flag) != 0;
    }

    BOOLEAN
    HasProperty(
        _In_ ChildProperty Flag) const
    {
        return (m_Properties & (LONG)Flag) != 0;
    }

    VOID
    SetProperty(
        _In_ ChildProperty Flag)
    {
        InterlockedOr(&m_Properties, (LONG)Flag);
    }

    VOID
    ClearProperty(
        _In_ ChildProperty Flag)
    {
        InterlockedAnd(&m_Properties, ~(LONG)Flag);
    }

    USB_DEVICE_SPEED
    Speed() const;

public:
    DeviceMachine m_Machine;

    /* Links */
    WDFOBJECT m_Object;
    HubFdo* m_Hub;
    HubPort* m_Port;
    HubPdo* m_Pdo;
    UCXUSBDEVICE m_UsbDevice;

    /* Mux: flags under the hub mux lock, and the three list links */
    ULONG m_MuxFlags;
    LIST_ENTRY m_HubLink;
    LIST_ENTRY m_FanOutLink;
    LIST_ENTRY m_SurpriseRemoveLink;

    /* Flag words */
    ULONG m_Kind;
    ULONG m_Hacks;
    volatile LONG m_Properties;
    volatile LONG m_StateFlags;
    ULONG m_VerifierFlags;

    /* Machine resources */
    HubTimer m_Timer;
    WDFTIMER m_BandwidthRetryTimer;
    PUCXHUB_WORKITEM m_WorkItem;
    BOOLEAN m_NeedsForwardProgress;

    /* PnP handshakes with the child PDO and the hub FDO */
    KEVENT m_PnpEvent;
    NTSTATUS m_PnpStatus;
    KEVENT m_PreStartEvent;
    KEVENT m_QueryTextEvent;
    PWDFDEVICE_INIT m_PdoInit;

    /* Enumeration */
    ULONG m_EnumRetryCount;
    ULONG m_DuplicateRetryCount;
    UCHAR m_Address;
    USHORT m_MaxPacketSize0;
    UCXHUB_DEVICE_CREATE_INFO m_CreateInfo;
    NTSTATUS m_LastNtStatus;
    USBD_STATUS m_LastUsbdStatus;
    ULONG m_EnumMessageId;
    LARGE_INTEGER m_LastResetTime;

    /* Requests */
    HubControlRequest m_Control;
    HubControlRequest m_BootControl;
    WDFREQUEST m_UcxRequest;
    WDFMEMORY m_UcxPayloadMemory;
    HubUcxPayload* m_UcxPayload;
    ULONG m_UcxIoctl;
    WDFREQUEST m_ClientRequest;
    WDFREQUEST m_FdoRequest;

    /* Configuration, under m_ConfigLock against the FDO's connection info IOCTL */
    KSPIN_LOCK m_ConfigLock;
    HubConfiguration* m_CurrentConfig;
    HubConfiguration* m_OldConfig;
    HubInterface* m_NewInterface;
    HubInterface* m_OldInterface;
    HubInterface* m_NextInterface;
    UCXENDPOINT m_DefaultEndpoint;
    UCXENDPOINT* m_EndpointsToEnable;
    UCXENDPOINT* m_EndpointsToDisable;
    UCXENDPOINT* m_EndpointsUnchanged;
    ULONG m_EnableCount;
    ULONG m_DisableCount;
    ULONG m_UnchangedCount;
    ULONG m_EndpointArrayCapacity;
    PVOID m_AlternateSettingFilter;

    /* Descriptors */
    USB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    USB_DEVICE_QUALIFIER_DESCRIPTOR m_Qualifier;
    PUSB_CONFIGURATION_DESCRIPTOR m_ConfigDescriptor;
    PUSB_BOS_DESCRIPTOR m_Bos;
    PUSB_STRING_DESCRIPTOR m_LanguageIds;
    PUSB_STRING_DESCRIPTOR m_ProductString;
    USHORT m_LanguageId;
    UCHAR m_ScratchBuffer[256];

    /* Validation codes this device hit; words 0 to 6 are written to the registry */
    RTL_BITMAP m_ValidationBitmap;
    ULONG m_ValidationBits[8];

    /* Serial number as cached, the instance id */
    PWCHAR m_SerialNumber;
    ULONG m_SerialNumberLength;

    /* identity */

    /* Bytes of MSFT20 or MSFT30 decoration in front of the serial number text */
    ULONG m_SerialPrefixLength;

    /* Written by the BOS and MS OS readers, or generated when the PDO is created */
    GUID m_ContainerId;

    /* Replacement device id of a VM reserved device; Length counts its NUL */
    UNICODE_STRING m_VmReservedId;

    /* Set by the configuration descriptor reader: interface the class compatible ids come from */
    PUSB_INTERFACE_DESCRIPTOR m_CompatIdInterface;

    /* Owned by the MS OS 1.0 extended configuration reader; NULL when not cached */
    HubMsOsExtConfig* m_MsOsExtConfig;

    /* U1 and U2 timeouts chosen for the device, sent to its upstream port */
    UCHAR m_U1Timeout;
    UCHAR m_U2Timeout;

    /* From the MS OS 2.0 minimum resume time descriptor, in ms */
    BOOLEAN m_HasResumeRecoveryTime;
    UCHAR m_ResumeRecoveryTime;

    /* Boot device support */
    PVOID m_BootHandle;
    volatile LONG m_BootReportedMissing;

    /* Current WDM power state, to tell D2 to D3 apart */
    DEVICE_POWER_STATE m_PowerState;

    /* devucx */

    /* Pipe of the client's pipe request, set by FindClientPipe */
    HubPipe* m_TargetPipe;

    /* HUB_LPM_POLICY_* bits, written by the PDO power setting callback */
    ULONG m_LpmPolicy;

    /* HUB_LINK_* bits; only the device machine touches them */
    ULONG m_LinkFlags;

    /* U1 and U2 timeouts the next link power pass wants on the port */
    UCHAR m_TargetU1Timeout;
    UCHAR m_TargetU2Timeout;

    /* Extra exit latency in percent after missed ping responses */
    UCHAR m_LatencyAdjustPercent;

    /* Exit latency the controller has, the one wanted next, and the cap UCX reported, in us */
    USHORT m_EffectiveExitLatency;
    USHORT m_TargetExitLatency;
    ULONG m_MaxExitLatencyFromEld;

    /* From the device BOS (BosValid) and SET_SEL, in us */
    USHORT m_U1ExitLatency;
    USHORT m_U2ExitLatency;
    USHORT m_HostU1ExitLatency;
    USHORT m_HostU2ExitLatency;

    /* SET_SEL data stage, sent from here as is */
    struct
    {
        UCHAR U1Sel;
        UCHAR U1Pel;
        USHORT U2Sel;
        USHORT U2Pel;
    } m_Sel;

    /* TP link delays of the device link, in ns */
    USHORT m_RxTpDelay;
    USHORT m_TxTpDelay;

    /* Why USB 2.0 hardware LPM is on or off (HubLpm20Status), diagnostics only */
    ULONG m_Lpm20Status;

    /* bResumeSignalingTime of the MS OS 2.0 minimum resume time descriptor */
    UCHAR m_ResumeSignalingTime;

    /* HUB_TUNNEL_STATE_*: not asked for ports without a USB4 host, else the controller's answer */
    ULONG m_TunnelState;

    /* registry */

    /* MS OS 1.0: string 0xEE as read, and the vendor code for feature requests */
    HubMsOsString m_MsOsString;
    UCHAR m_MsOsVendorCode;

    /* MS OS 2.0: selected set info, the cached set and what its validation found */
    HubMsOs20SetInfo m_MsOs20SetInfo;
    HubMsOs20Info m_MsOs20;
    PVOID m_MsOs20Set;
    ULONG m_MsOs20SetLength;

    /* The set info is in usbflags, so the alternate enumeration command goes out early */
    BOOLEAN m_AltEnumCached;

    /* Device descriptor from before the alternate enumeration command */
    USB_DEVICE_DESCRIPTOR m_OriginalDeviceDescriptor;

    /* MS OS 1.0 extended properties, between AllocateExtPropertiesBuffer and its free */
    PVOID m_ExtProperties;

    /* FriendlyName from the hardware key; counted, no NUL */
    PWCHAR m_FriendlyName;
    USHORT m_FriendlyNameLength;

    /* UXD record for the device; all zero when it is not reserved */
    HubUxdSettings m_Uxd;

    /* ChildSqm bits, interlocked */
    volatile LONG m_SqmFlags;

    /* devxfer */

    /* Billboard capability from the BOS, or NULL; the PDO frees it after publishing */
    HubBillboardInfo* m_Billboard;

    /* SuperSpeedPlus sublink speed attributes, cached by the first BOS read only */
    PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED m_SublinkSpeedAttr;
    ULONG m_SublinkSpeedAttrCount;

    /* Dual role partner: vendor command from the BOS, and the features of both sides */
    UCHAR m_DualRoleRequestCode;
    ULONG m_DualRoleLocalFeatures;
    ULONG m_DualRolePartnerFeatures;

    /* SET_SEL found the device link slower than the path; only ever set */
    BOOLEAN m_SlowestLinkU1;
    BOOLEAN m_SlowestLinkU2;

    /* hubpdo */

    /* ReferenceHubPower got a power reference on the hub FDO */
    BOOLEAN m_HubPowerReferenceHeld;

private:
    static
    VOID
    NTAPI
    TimerFired(
        _In_ PVOID Context);

    static EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;
    static UCXHUB_WORKITEM_ROUTINE MachineWorkItem;

    friend class DeviceMachine;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubChild, HubGetChildContext);
