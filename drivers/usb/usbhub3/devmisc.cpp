/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device kind, speed and reset decisions, and the duplicate serial list
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Retry limits */
#define DEVICE_DUPLICATE_RETRY_LIMIT    10

/* Two resets closer than a second apart make the next one a warm reset */
#define DEVICE_FAST_RESET_WINDOW        (1LL * 1000 * 1000 * 10)

/* Validation code for a serial number another connected device already uses */
#define HUB_VALIDATION_DUPLICATE_SERIAL 234

/* Four attempts in all before the device is given up on */
#define DEVICE_ENUM_RETRY_LIMIT         3

/* USB 2 port status bits for the attached speed */
#define PS_LOW_SPEED                    0x0200
#define PS_HIGH_SPEED                   0x0400

BOOLEAN
DeviceMachine::IsFirstEnumTry()
{
    return m_Device->m_EnumRetryCount == 0;
}

/** Forgets everything learned from the MS OS 2.0 descriptor set so the next attempt starts clean. */
static
VOID
NTAPI
HubForgetMsOs20(
    _Inout_ HubChild* Child)
{
    if (Child->m_MsOs20Set != NULL)
        ExFreePoolWithTag(Child->m_MsOs20Set, HUB_TAG_DEVICE);

    Child->m_MsOs20Set = NULL;
    Child->m_MsOs20SetLength = 0;
    RtlZeroMemory(&Child->m_MsOs20, sizeof(Child->m_MsOs20));
    RtlZeroMemory(&Child->m_MsOs20SetInfo, sizeof(Child->m_MsOs20SetInfo));
    RtlZeroMemory(&Child->m_OriginalDeviceDescriptor, sizeof(Child->m_OriginalDeviceDescriptor));
    Child->m_AltEnumCached = FALSE;
    Child->m_HasResumeRecoveryTime = FALSE;
    Child->m_ResumeRecoveryTime = 0;
    Child->m_ResumeSignalingTime = 0;
    Child->ClearState(ChildState::AltEnumCompleted);
    Child->ClearState(ChildState::AltEnumCommandSent);
}

VOID
DeviceMachine::StartEnumRetries()
{
    m_Device->ClearState(ChildState::ConfigurationValid);
    m_Device->ClearState(ChildState::AltEnumCompleted);
    m_Device->ClearState(ChildState::AltEnumCommandSent);
    m_Device->ClearState(ChildState::U1EnabledUpstream);
    m_Device->ClearState(ChildState::U2EnabledUpstream);
    m_Device->m_EnumMessageId = 0;
    m_Device->m_EnumRetryCount = 0;
    m_Device->m_U1Timeout = 0;
    m_Device->m_U2Timeout = 0;
    m_Device->m_EffectiveExitLatency = 0;
}

BOOLEAN
DeviceMachine::EnumRetriesUsedUp()
{
    m_Device->SetState(ChildState::FailedAnEnumeration);

    if (++m_Device->m_EnumRetryCount > DEVICE_ENUM_RETRY_LIMIT)
    {
        DPRINT1("Device %p failed %lu enumeration attempts\n", m_Device, m_Device->m_EnumRetryCount);
        return TRUE;
    }

    DPRINT1("Device %p enumeration retry %lu\n", m_Device, m_Device->m_EnumRetryCount);
    m_Device->ClearState(ChildState::ConfigurationValid);
    m_Device->m_EnumMessageId = 0;
    HubForgetMsOs20(m_Device);
    return FALSE;
}

VOID
DeviceMachine::StartDuplicateRetries()
{
    m_Device->m_DuplicateRetryCount = 0;
}

BOOLEAN
DeviceMachine::DuplicateRetriesUsedUp()
{
    ULONG Count = ++m_Device->m_DuplicateRetryCount;

    DPRINT1("Device %p duplicate serial number retry %lu\n", m_Device, Count);

    if (Count > DEVICE_DUPLICATE_RETRY_LIMIT)
    {
        DPRINT1("Device %p duplicate is not going away, serial number dropped\n", m_Device);
        return TRUE;
    }

    return FALSE;
}

/* A device that was high speed and now reports another speed is enumerated again */
BOOLEAN
DeviceMachine::SetSpeedFor20()
{
    USHORT Status = m_Device->m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    ULONG OldSpeed = m_Device->m_Kind & DSM_KIND_ANY_SPEED;
    ULONG NewSpeed;

    if (Status & PS_HIGH_SPEED)
    {
        NewSpeed = DSM_KIND_HIGH_SPEED;
        m_Device->SetProperty(ChildProperty::HighSpeedCapable);
    }
    else if (Status & PS_LOW_SPEED)
    {
        NewSpeed = DSM_KIND_LOW_SPEED;
    }
    else
    {
        NewSpeed = DSM_KIND_FULL_SPEED;
    }

    m_Device->m_Kind = (m_Device->m_Kind & ~DSM_KIND_ANY_SPEED) | NewSpeed;

    if (OldSpeed != 0 && OldSpeed != NewSpeed)
    {
        DPRINT1("Device %p changed speed 0x%lx to 0x%lx\n", m_Device, OldSpeed, NewSpeed);
        if (OldSpeed == DSM_KIND_HIGH_SPEED)
            return FALSE;
    }

    return TRUE;
}

/* A USB 3 bcdUSB on a USB 2 port counts as USB 2.x */
VOID
DeviceMachine::RecordDeviceVersion()
{
    USHORT BcdUsb = m_Device->m_DeviceDescriptor.bcdUSB;
    ULONG Version;

    if (BcdUsb == 0x0100)
    {
        Version = DSM_KIND_USB10;
    }
    else if (BcdUsb < 0x0200)
    {
        if (BcdUsb < 0x0100)
            DPRINT1("Device %p reports bad bcdUSB 0x%x\n", m_Device, BcdUsb);
        Version = DSM_KIND_USB1X;
    }
    else if (BcdUsb == 0x0200)
    {
        Version = DSM_KIND_USB20;
    }
    else if (BcdUsb < 0x0300)
    {
        Version = DSM_KIND_USB2X;
    }
    else if (m_Device->m_Kind & DSM_KIND_PORT30)
    {
        Version = DSM_KIND_USB3X;
    }
    else
    {
        DPRINT1("Device %p on a USB 2 port reports bcdUSB 0x%x\n", m_Device, BcdUsb);
        Version = DSM_KIND_USB2X;
    }

    m_Device->m_Kind = (m_Device->m_Kind & ~DSM_KIND_ANY_VERSION) | Version;
}

/* Both reset requests are consumed, whatever the answer */
DSM_RESET_KIND
DeviceMachine::ResetKindNeeded()
{
    HubChild* Child = m_Device;
    DSM_RESET_KIND Kind = DsmResetNone;
    LARGE_INTEGER Now;

    if (Child->m_EnumRetryCount > 0)
    {
        Kind = DsmResetWarm;
    }
    else
    {
        KeQuerySystemTime(&Now);

        if (Now.QuadPart - Child->m_LastResetTime.QuadPart < DEVICE_FAST_RESET_WINDOW)
            Kind = DsmResetWarm;
        else if (Child->HasState(ChildState::WarmResetOnEnumeration))
            Kind = DsmResetWarm;
        else if (Child->HasState(ChildState::HotResetOnEnumeration))
            Kind = Child->HasHack(ChildHack::DisableHotReset) ? DsmResetWarm : DsmResetHot;

        Child->m_LastResetTime = Now;
    }

    Child->ClearState(ChildState::WarmResetOnEnumeration);
    Child->ClearState(ChildState::HotResetOnEnumeration);
    return Kind;
}

/* Only asked for devices below SuperSpeed */
BOOLEAN
DeviceMachine::DeviceSpeedChanged()
{
    USHORT Status = m_Device->m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USB_DEVICE_SPEED Speed = m_Device->Speed();

    if (Speed == UsbSuperSpeed)
    {
        NT_ASSERT(FALSE);
        return FALSE;
    }

    if (Status & PS_HIGH_SPEED)
        return Speed != UsbHighSpeed;
    if (Status & PS_LOW_SPEED)
        return Speed != UsbLowSpeed;
    return Speed != UsbFullSpeed;
}

/* A device that switched descriptors through MS OS 2.0 reports its pre-switch descriptor after a reset */
BOOLEAN
DeviceMachine::SameDeviceConnected()
{
    const USB_DEVICE_DESCRIPTOR* Known = &m_Device->m_DeviceDescriptor;

    if (m_Device->m_MsOs20.Flags & HUB_MSOS20_ALT_ENUM)
        Known = &m_Device->m_OriginalDeviceDescriptor;

    if (!RtlEqualMemory(m_Device->m_ScratchBuffer, Known, sizeof(*Known)))
    {
        DPRINT("Device %p descriptor differs from the cached one\n", m_Device);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DeviceMachine::ConfigReadOnReset()
{
    return m_Device->HasHack(ChildHack::RequestConfigDescOnReset);
}

/* Fast enumeration skips the second reset of a USB 2 device on its first try */
BOOLEAN
DeviceMachine::NeedsSecondReset()
{
    if (m_Device->HasHack(ChildHack::AlwaysSecondReset))
        return TRUE;

    if (m_Device->m_Kind & DSM_KIND_USB2X)
        return FALSE;

    if ((m_Device->m_Kind & DSM_KIND_HIGH_SPEED) &&
        !HubDriver.HasFlag(HubGlobal::AlwaysSecondReset) &&
        m_Device->m_EnumRetryCount == 0)
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DeviceMachine::WaitNeededAfterAddress()
{
    return !(m_Device->m_Kind & DSM_KIND_SUPER_SPEED) || m_Device->HasState(ChildState::FailedAnEnumeration);
}

BOOLEAN
DeviceMachine::SuperSpeedMustBeDisabled()
{
    return m_Device->HasHack(ChildHack::DisableSuperSpeed) && (m_Device->m_Kind & DSM_KIND_SUPER_SPEED);
}

BOOLEAN
DeviceMachine::SupportsPdCharging()
{
    return m_Device->HasProperty(ChildProperty::ChargingPolicySupported);
}

BOOLEAN
DeviceMachine::DualRoleSupported()
{
    return m_Device->HasProperty(ChildProperty::DualRole);
}

BOOLEAN
DeviceMachine::ProductStringIndexZero()
{
    return m_Device->m_DeviceDescriptor.iProduct == 0;
}

BOOLEAN
DeviceMachine::SelSkipped()
{
    return m_Device->HasHack(ChildHack::NoExitLatencyRequest);
}

BOOLEAN
DeviceMachine::IsochDelaySkipped()
{
    return m_Device->HasHack(ChildHack::NoIsochDelayRequest);
}

BOOLEAN
DeviceMachine::LtmWanted()
{
    return m_Device->HasProperty(ChildProperty::LtmCapable) &&
           m_Device->HasProperty(ChildProperty::NotRemovable) &&
           HubDriver.HasFlag(HubGlobal::EnableUsbLtm);
}

BOOLEAN
DeviceMachine::DisableWhenUnused()
{
    return m_Device->HasHack(ChildHack::DisableOnSoftRemove) &&
           m_Device->m_Hub->HasFlag(HubFlag::DisableOnSoftRemove) &&
           HubDriver.HasFlag(HubGlobal::DisableOnSoftRemove);
}

BOOLEAN
DeviceMachine::ResetOnResumeFromS0()
{
    return m_Device->HasHack(ChildHack::ResetAfterIdleResume);
}

BOOLEAN
DeviceMachine::ResetOnResumeFromSx()
{
    return m_Device->HasHack(ChildHack::ResetAfterSystemResume);
}

/* The controller lost the device: every enabled pipe has to be programmed again */
BOOLEAN
DeviceMachine::DeviceProgrammingLost()
{
    HubConfiguration* Config = m_Device->m_CurrentConfig;
    PLIST_ENTRY Entry;
    HubInterface* Interface;
    ULONG Index;

    if (!m_Device->HasState(ChildState::ReprogrammingPending))
        return FALSE;

    m_Device->ClearState(ChildState::ReprogrammingPending);

    if (m_Device->m_Pdo != NULL)
        InterlockedOr(&m_Device->m_Pdo->m_Flags, (LONG)PdoFlag::ProgrammingLostOnReset);

    if (Config != NULL)
    {
        for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
        {
            Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

            for (Index = 0; Index < Interface->PipeCount; Index++)
            {
                if (Interface->Pipes[Index].State == PipeState::Enabled)
                    Interface->Pipes[Index].State = PipeState::Disabled;
            }
        }
    }

    return TRUE;
}

VOID
DeviceMachine::ClearReprogramNeeded()
{
    m_Device->ClearState(ChildState::ReprogrammingPending);
}

/* PnP does not see this enumeration */
VOID
DeviceMachine::LogReenumeration()
{
    DPRINT1("Device %p enumerated again behind PnP\n", m_Device);
}

VOID
DeviceMachine::RecordResetTimeout()
{
    DPRINT1("Device %p port %u reset timed out\n", m_Device, m_Device->m_Port->Number());
    m_Device->m_EnumMessageId = HUB_ENUM_PORT_RESET_FAILED;
}

VOID
DeviceMachine::BugCheckForBootDevice()
{
    DPRINT1("Boot device %p failed to enumerate again\n", m_Device);
    KeBugCheckEx(HUB_BUGCHECK_USB3,
                 HUB_USB3_BOOT_DEVICE_FAILED,
                 (ULONG_PTR)WdfDeviceWdmGetDeviceObject(m_Device->m_Pdo->m_Device),
                 m_Device->HasProperty(ChildProperty::IsHub) ? HUB_BOOT_DEVICE_IS_HUB : HUB_BOOT_DEVICE_IS_STORAGE,
                 0);
}

/* Duplicate serial number list in usbd.sys */

/* The other device with this serial number is going away: poll until it has */
DSM_ADD_CHILD
DeviceMachine::AddChildToHub()
{
    HubChild* Child = m_Device;
    USB_ID_STRING Serial;

    if (Child->m_SerialNumber == NULL)
        return DsmAddChildDone;

    RtlZeroMemory(&Serial, sizeof(Serial));
    Serial.LanguageId = Child->m_LanguageId;
    Serial.LengthInBytes = Child->m_SerialNumberLength;
    Serial.Buffer = Child->m_SerialNumber;

    switch (USBD_AddDeviceToGlobalList(Child,
                                       Child->m_Hub,
                                       Child->m_Port->Number(),
                                       HubConnectorNodeForPort(Child->m_Port),
                                       Child->m_DeviceDescriptor.idVendor,
                                       Child->m_DeviceDescriptor.idProduct,
                                       &Serial))
    {
        case USBD_GLOBAL_LIST_ADDED:
            return DsmAddChildDone;

        case USBD_GLOBAL_LIST_DUPLICATE_PENDING:
            DPRINT("Device %p waits for a departing duplicate\n", Child);
            return DsmAddChildDuplicate;

        case USBD_GLOBAL_LIST_DUPLICATE:
            DPRINT1("Device %p serial number already in use\n", Child);
            HubLogDeviceValidationError(Child, HUB_VALIDATION_DUPLICATE_SERIAL);
            return DsmAddChildFailed;

        case USBD_GLOBAL_LIST_NO_MEMORY:
            DPRINT1("Device %p could not be added to the device list\n", Child);
            return DsmAddChildFailed;

        default:
            DPRINT1("Device %p unexpected device list answer\n", Child);
            NT_ASSERT(FALSE);
            return DsmAddChildFailed;
    }
}

VOID
DeviceMachine::ForgetChild()
{
    if (m_Device->m_SerialNumber != NULL)
        USBD_RemoveDeviceFromGlobalList(m_Device);
}

VOID
DeviceMachine::MarkDisconnected()
{
    if (m_Device->m_SerialNumber != NULL)
        USBD_MarkDeviceAsDisconnected(m_Device);
}

/* Codes land in the per device bitmap the registry module writes out */
VOID
NTAPI
HubLogDeviceValidationError(
    _In_ HubChild* Child,
    _In_ ULONG Code)
{
    RtlSetBit(&Child->m_ValidationBitmap, Code);
}

/* Gated on the serial number errata */
BOOLEAN
DeviceMachine::ProductNameWanted()
{
    if (m_Device->HasHack(ChildHack::DisableSerialNumber))
        return FALSE;

    return HubIdLanguageSupported(m_Device, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
}

BOOLEAN
DeviceMachine::ArmForWakeWanted()
{
    HubPdo* Pdo = m_Device->m_Pdo;

    return Pdo != NULL && Pdo->HasFlag(PdoFlag::WakeArmRequested);
}

BOOLEAN
DeviceMachine::DeviceArmedForWake()
{
    HubPdo* Pdo = m_Device->m_Pdo;

    return Pdo != NULL && Pdo->HasFlag(PdoFlag::ArmedForWake);
}

/* Function suspend clients manage their own remote wake */
BOOLEAN
DeviceMachine::DisarmOnResume()
{
    HubPdo* Pdo = m_Device->m_Pdo;

    if (!m_Device->HasProperty(ChildProperty::RemoteWakeCapable))
        return FALSE;

    return Pdo == NULL || !Pdo->HasFlag(PdoFlag::ClientDoesFunctionSuspend);
}
