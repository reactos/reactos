/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, descriptor reads after addressing and
 *              SuperSpeed link power setup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

/* A detach or hub stop or suspend cancels the transfer and ends the sub machine */
const DeviceMachine::State DeviceMachine::CollectingDescriptors =
    { NULL, "CollectingDescriptors", 0, &DeviceMachine::OnCollectingDescriptors, NULL };

const DeviceMachine::State DeviceMachine::ReadingDeviceDescriptor =
    { &CollectingDescriptors, "ReadingDeviceDescriptor", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnReadingDeviceDescriptor, &DeviceMachine::EnterReadingDeviceDescriptor };

const DeviceMachine::State DeviceMachine::ReadingConfigDescriptor =
    { &CollectingDescriptors, "ReadingConfigDescriptor", 0,
      &DeviceMachine::OnReadingConfigDescriptor, &DeviceMachine::EnterReadingConfigDescriptor };

/* Asked the port machine to bring the device back at high speed; the caller decides */
const DeviceMachine::State DeviceMachine::DisablingSuperSpeed =
    { NULL, "DisablingSuperSpeed", SM_STATE_YIELDS_TO_CALLER,
      NULL, &DeviceMachine::EnterDisablingSuperSpeed };

/* Runs the sub machine that reads everything after the configuration descriptor */
const DeviceMachine::State DeviceMachine::RemainingDescriptors =
    { NULL, "RemainingDescriptors", 0,
      &DeviceMachine::OnRemainingDescriptors, &DeviceMachine::EnterRemainingDescriptors };

const DeviceMachine::State DeviceMachine::ReadingBos =
    { &CollectingDescriptors, "ReadingBos", 0,
      &DeviceMachine::OnReadingBos, &DeviceMachine::EnterReadingBos };

const DeviceMachine::State DeviceMachine::ReadingMsOs =
    { &CollectingDescriptors, "ReadingMsOs", 0,
      &DeviceMachine::OnReadingMsOs, &DeviceMachine::EnterReadingMsOs };

const DeviceMachine::State DeviceMachine::ReadingLanguages =
    { &CollectingDescriptors, "ReadingLanguages", 0,
      &DeviceMachine::OnReadingLanguages, &DeviceMachine::EnterReadingLanguages };

const DeviceMachine::State DeviceMachine::ReadingProductName =
    { &CollectingDescriptors, "ReadingProductName", 0,
      &DeviceMachine::OnReadingProductName, &DeviceMachine::EnterReadingProductName };

const DeviceMachine::State DeviceMachine::ReadingQualifier =
    { &CollectingDescriptors, "ReadingQualifier", 0,
      &DeviceMachine::OnReadingQualifier, &DeviceMachine::EnterReadingQualifier };

/* FUNCTIONS ******************************************************************/

SM_RESULT
DeviceMachine::OnCollectingDescriptors(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            return CancelTransferAndEnd(Event);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterReadingDescriptors()
{
    return SmCall(&ReadingDeviceDescriptor);
}

SM_RESULT
DeviceMachine::EnterReadingDeviceDescriptor()
{
    ReadDeviceDescriptor();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnReadingDeviceDescriptor(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::TransferDone:
            if (!DeviceDescriptorValid())
                return EndWith(DsmEvent::Failed);

            RecordDeviceVersion();
            return AtPassive(&DeviceMachine::DescriptorsQueryRegistry);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::DescriptorsQueryRegistry()
{
    if (!QueryRegistryValues())
        return EndWith(DsmEvent::Failed);

    if (AltEnumNeededInEnum())
    {
        return Request(&Requesting, &DeviceMachine::SendAltEnumCommand,
                       &DeviceMachine::DescriptorsAltEnumSent);
    }

    if (SuperSpeedMustBeDisabled())
        return SmTransition(&DisablingSuperSpeed);

    m_ConfigFullLength = FALSE;
    return SmTransition(&ReadingConfigDescriptor);
}

SM_RESULT
DeviceMachine::DescriptorsAltEnumSent(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return SmTransition(&ReadingDeviceDescriptor);

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterDisablingSuperSpeed()
{
    RequestSuperSpeedDisable();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterReadingConfigDescriptor()
{
    if (m_ConfigFullLength)
        ReadFullConfigDescriptor();
    else
        ReadConfigDescriptorHeader();

    return SmHandled();
}

SM_RESULT
DeviceMachine::OnReadingConfigDescriptor(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::TransferDone:
            if (!m_ConfigFullLength && ConfigLongerThanRead())
            {
                m_ConfigFullLength = TRUE;
                return SmTransition(&ReadingConfigDescriptor);
            }

            if (!ConfigDescriptorValid())
                return EndWith(DsmEvent::Failed);

            return SmTransition(&RemainingDescriptors);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterRemainingDescriptors()
{
    return SmCall(&ReadingBos);
}

SM_RESULT
DeviceMachine::OnRemainingDescriptors(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
        case DsmEvent::Succeeded:
        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
        case DsmEvent::PortDetached:
            return EndWith(Event);

        default:
            return SmUnhandled();
    }
}

/* BOS DESCRIPTOR SUB MACHINE *************************************************/

SM_RESULT
DeviceMachine::EnterReadingBos()
{
    if (KindWithin(DSM_KIND_USB2X | DSM_KIND_USB3X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::BosStart);

    return SmTransition(&ReadingMsOs);
}

SM_RESULT
DeviceMachine::OnReadingBos(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&ReadingMsOs);

        case DsmEvent::Failed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BosStart()
{
    if (BosQuerySkipped())
        return EndWith(DsmEvent::Succeeded);

    return Request(&RequestingYielding, &DeviceMachine::ReadBosHeader,
                   &DeviceMachine::BosHeaderRead);
}

SM_RESULT
DeviceMachine::BosHeaderRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (!BosHeaderValid())
                return BosError();

            if (BosComplete())
                return AtPassive(&DeviceMachine::BosValidate);

            return Request(&RequestingYielding, &DeviceMachine::ReadBos,
                           &DeviceMachine::BosRead);

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

/* A device may get away with a bad or missing BOS, the error policy decides */
SM_RESULT
DeviceMachine::BosError()
{
    if (IgnoreDescriptorError())
        return EndWith(DsmEvent::Succeeded);

    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::BosRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return AtPassive(&DeviceMachine::BosValidate);

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BosValidate()
{
    if (!BosValid())
        return BosError();

    if (DualRoleSupported())
    {
        return Request(&RequestingYielding, &DeviceMachine::SendUsbFeatures,
                       &DeviceMachine::BosFeaturesSent);
    }

    return BosBillboard();
}

SM_RESULT
DeviceMachine::BosFeaturesSent(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return BosBillboard();

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BosBillboard()
{
    if (!HasBillboard())
        return BosAltEnum();

    if (BillboardStringWanted())
    {
        return Request(&RequestingYielding, &DeviceMachine::ReadBillboardString,
                       &DeviceMachine::BosBillboardRead);
    }

    return BosAltModes();
}

SM_RESULT
DeviceMachine::BosBillboardRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (BillboardStringValid())
                return BosAltModes();

            return BosError();

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BosAltModes()
{
    if (AltModeStringWanted())
    {
        return Request(&RequestingYielding, &DeviceMachine::ReadAltModeString,
                       &DeviceMachine::BosAltModeRead);
    }

    return BosAltEnum();
}

SM_RESULT
DeviceMachine::BosAltModeRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (AltModeStringValid())
                return BosAltModes();

            return BosError();

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BosAltEnum()
{
    if (AltEnumNeededAfterBos())
    {
        return Request(&RequestingYielding, &DeviceMachine::SendAltEnumCommand,
                       &DeviceMachine::BosAltEnumSent);
    }

    return BosMsOs20();
}

SM_RESULT
DeviceMachine::BosMsOs20()
{
    if (MsOs20Supported())
    {
        return Request(&RequestingYielding, &DeviceMachine::ReadMsOs20Set,
                       &DeviceMachine::BosMsOs20Read);
    }

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::BosMsOs20Read(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return AtPassive(&DeviceMachine::BosMsOs20Validate);

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BosMsOs20Validate()
{
    if (MsOs20SetValid())
        return EndWith(DsmEvent::Succeeded);

    return BosError();
}

/* The device switched to its alternate enumeration: read it again from the top */
SM_RESULT
DeviceMachine::BosAltEnumSent(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return Request(&RequestingYielding, &DeviceMachine::ReadDeviceDescriptor,
                           &DeviceMachine::AltDeviceDescriptorRead);

        case DsmEvent::TransferFailed:
            return BosError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AltDeviceDescriptorRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (!DeviceDescriptorValid())
                return EndWith(DsmEvent::Failed);

            RecordDeviceVersion();
            return AtPassive(&DeviceMachine::AltQueryRegistry);

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AltQueryRegistry()
{
    if (!QueryRegistryValues())
        return EndWith(DsmEvent::Failed);

    return Request(&RequestingYielding, &DeviceMachine::ReadConfigDescriptorHeader,
                   &DeviceMachine::AltConfigHeaderRead);
}

/* Unlike the first read, a short alternate configuration is validated as is */
SM_RESULT
DeviceMachine::AltConfigHeaderRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (ConfigLongerThanRead())
                return AltValidateConfig();

            return Request(&RequestingYielding, &DeviceMachine::ReadFullConfigDescriptor,
                           &DeviceMachine::AltConfigRead);

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AltConfigRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return AltValidateConfig();

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AltValidateConfig()
{
    if (!ConfigDescriptorValid())
        return EndWith(DsmEvent::Failed);

    if (BosQuerySkipped())
        return EndWith(DsmEvent::Succeeded);

    return Request(&RequestingYielding, &DeviceMachine::ReadBosHeader,
                   &DeviceMachine::AltBosHeaderRead);
}

SM_RESULT
DeviceMachine::AltBosHeaderRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (!BosHeaderValid())
                return EndWith(DsmEvent::Failed);

            if (BosComplete())
                return AtPassive(&DeviceMachine::AltBosValidate);

            return Request(&RequestingYielding, &DeviceMachine::ReadBos,
                           &DeviceMachine::AltBosRead);

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AltBosRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return AtPassive(&DeviceMachine::AltBosValidate);

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AltBosValidate()
{
    if (!BosValid())
        return EndWith(DsmEvent::Failed);

    return BosMsOs20();
}

/* MICROSOFT OS AND SERIAL NUMBER SUB MACHINE *********************************/

SM_RESULT
DeviceMachine::EnterReadingMsOs()
{
    return CallStep(&DeviceMachine::MsOsStart);
}

SM_RESULT
DeviceMachine::OnReadingMsOs(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            if (ProductStringIndexZero())
                return SmTransition(&ReadingQualifier);

            return SmTransition(&ReadingLanguages);

        case DsmEvent::Failed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsStart()
{
    if (MsOsQueryWanted())
    {
        return Request(&RequestingYielding, &DeviceMachine::ReadMsOsDescriptor,
                       &DeviceMachine::MsOsRead);
    }

    return MsOsSerial();
}

SM_RESULT
DeviceMachine::MsOsRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return AtPassive(&DeviceMachine::MsOsMarkUnsupported);

        case DsmEvent::TransferDone:
            if (MsOsDescriptorValid())
                return AtPassive(&DeviceMachine::MsOsStoreVendorCode);

            return AtPassive(&DeviceMachine::MsOsMarkUnsupported);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsMarkUnsupported()
{
    MarkMsOsUnsupported();
    return MsOsSerial();
}

SM_RESULT
DeviceMachine::MsOsStoreVendorCode()
{
    StoreMsOsVendorCode();
    if (!MsOsContainerIdSupported())
        MarkContainerIdUnsupported();

    return MsOsSerial();
}

SM_RESULT
DeviceMachine::MsOsSerial()
{
    if (IgnoreSerialNumber() || SerialNumberIndexZero())
        return MsOsExtendedConfig();

    return Request(&RequestingYielding, &DeviceMachine::ReadSerialNumber,
                   &DeviceMachine::MsOsSerialRead);
}

SM_RESULT
DeviceMachine::MsOsSerialRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return MsOsError(&DeviceMachine::MsOsExtendedConfig);

        case DsmEvent::TransferDone:
            if (SerialNumberValid())
                return MsOsExtendedConfig();

            return MsOsError(&DeviceMachine::MsOsExtendedConfig);

        default:
            return SmUnhandled();
    }
}

/* A bad optional descriptor fails enumeration unless the error policy says go on */
SM_RESULT
DeviceMachine::MsOsError(
    _In_ DSM_THEN GoOn)
{
    if (IgnoreDescriptorError())
        return (this->*GoOn)();

    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::MsOsExtendedConfig()
{
    if (MsOsExtendedConfigSupported())
    {
        return Request(&RequestingYielding, &DeviceMachine::ReadExtendedConfigHeader,
                       &DeviceMachine::MsOsExtendedHeaderRead);
    }

    return MsOsContainerId();
}

SM_RESULT
DeviceMachine::MsOsExtendedHeaderRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (!ExtendedConfigHeaderValid())
                return MsOsError(&DeviceMachine::MsOsContainerId);

            return Request(&RequestingYielding, &DeviceMachine::ReadExtendedConfig,
                           &DeviceMachine::MsOsExtendedRead);

        case DsmEvent::TransferFailed:
            return MsOsContainerId();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsExtendedRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (ExtendedConfigValid())
                return MsOsContainerId();

            return MsOsError(&DeviceMachine::MsOsContainerId);

        case DsmEvent::TransferFailed:
            return MsOsError(&DeviceMachine::MsOsContainerId);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsContainerId()
{
    if (ContainerIdWanted())
    {
        return Request(&RequestingYielding, &DeviceMachine::ReadContainerIdHeader,
                       &DeviceMachine::MsOsContainerHeaderRead);
    }

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::MsOsContainerHeaderRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (!ContainerIdHeaderValid())
                return MsOsError(&DeviceMachine::MsOsNoContainerId);

            return Request(&RequestingYielding, &DeviceMachine::ReadContainerId,
                           &DeviceMachine::MsOsContainerRead);

        case DsmEvent::TransferFailed:
            return MsOsError(&DeviceMachine::MsOsNoContainerId);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsContainerRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (ContainerIdKnown())
                return EndWith(DsmEvent::Succeeded);

            return MsOsError(&DeviceMachine::MsOsNoContainerId);

        case DsmEvent::TransferFailed:
            return MsOsError(&DeviceMachine::MsOsNoContainerId);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsNoContainerId()
{
    return AtPassive(&DeviceMachine::MsOsMarkNoContainerId);
}

SM_RESULT
DeviceMachine::MsOsMarkNoContainerId()
{
    MarkContainerIdUnsupported();
    return EndWith(DsmEvent::Succeeded);
}

/* STRINGS AND QUALIFIER ******************************************************/

SM_RESULT
DeviceMachine::EnterReadingLanguages()
{
    ReadLanguageIds();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnReadingLanguages(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return StringError();

        case DsmEvent::TransferDone:
            if (!LanguageIdsValid())
                return StringError();

            if (ProductNameWanted())
                return SmTransition(&ReadingProductName);

            return SmTransition(&ReadingQualifier);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StringError()
{
    if (IgnoreDescriptorError())
        return SmTransition(&ReadingQualifier);

    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::EnterReadingProductName()
{
    ReadProductName();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnReadingProductName(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (ProductNameValid())
                return SmTransition(&ReadingQualifier);

            return StringError();

        case DsmEvent::TransferFailed:
            return StringError();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterReadingQualifier()
{
    if (KindWithin(DSM_KIND_USB2X | DSM_KIND_USB20 | DSM_KIND_FULL_SPEED | DSM_KIND_ANY_PORT))
    {
        return CallRequest(&RequestingYielding, &DeviceMachine::ReadQualifier,
                           &DeviceMachine::QualifierRead);
    }

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::OnReadingQualifier(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
        case DsmEvent::FailEnumeration:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::Succeeded:
            return EndWith(DsmEvent::Succeeded);

        default:
            return SmUnhandled();
    }
}

/* A full speed 2.0 device may be high speed capable; a bad qualifier only matters if the policy says so */
SM_RESULT
DeviceMachine::QualifierRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (QualifierValid())
                return EndWith(DsmEvent::Succeeded);

            if (IgnoreDescriptorError())
                return EndWith(DsmEvent::Succeeded);

            return EndWith(DsmEvent::FailEnumeration);

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Succeeded);

        default:
            return SmUnhandled();
    }
}

/* SUPERSPEED LINK POWER SUB MACHINE ******************************************/

SM_RESULT
DeviceMachine::LinkStart()
{
    if (IsochDelaySkipped())
        return LinkExitLatency();

    return Request(&RequestingCritical, &DeviceMachine::SetIsochDelay,
                   &DeviceMachine::LinkIsochDelaySet);
}

SM_RESULT
DeviceMachine::LinkIsochDelaySet(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    return LinkExitLatency();
}

SM_RESULT
DeviceMachine::LinkExitLatency()
{
    if (SelSkipped())
        return LinkLatencyTolerance();

    return Request(&RequestingCritical, &DeviceMachine::SetSel, &DeviceMachine::LinkSelSet);
}

SM_RESULT
DeviceMachine::LinkSelSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
        case DsmEvent::TransferStalled:
            return LinkLatencyTolerance();

        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::LinkLatencyTolerance()
{
    if (LtmWanted())
        return Request(&RequestingCritical, &DeviceMachine::EnableLtm, &DeviceMachine::LinkLtmSet);

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::LinkLtmSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::TransferDone:
        case DsmEvent::TransferStalled:
            return EndWith(DsmEvent::Succeeded);

        default:
            return SmUnhandled();
    }
}
