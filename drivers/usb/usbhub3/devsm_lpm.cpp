/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, SuperSpeed U1 and U2 link power sub machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* FUNCTIONS ******************************************************************/

/* The exit latency is raised before the timeouts change and lowered after */
SM_RESULT
DeviceMachine::LinkPowerStart()
{
    InitLinkStates();
    return LpmCompute();
}

SM_RESULT
DeviceMachine::LpmCompute()
{
    CalcU1Timeout();
    CalcU2Timeout();
    ComputeExitLatency();
    if (!ExitLatencyMustRise())
        return LpmU1();

    if (EndpointsNeedProgramming())
    {
        return Request(&RequestingCritical, &DeviceMachine::ProgramEndpoints,
                       &DeviceMachine::LpmProgrammedBeforeRaise);
    }

    return Request(&RequestingCritical, &DeviceMachine::UpdateExitLatency,
                   &DeviceMachine::LpmLatencyRaised);
}

SM_RESULT
DeviceMachine::LpmProgrammedBeforeRaise(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return Request(&RequestingCritical, &DeviceMachine::UpdateExitLatency,
                           &DeviceMachine::LpmLatencyRaised);

        case DsmEvent::ControllerExitLatencyTooLarge:
            return LpmLatencyTooLarge();

        case DsmEvent::ControllerIoctlFailed:
            return LpmFailCleanup();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::LpmLatencyRaised(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
        case DsmEvent::ControllerIoctlFailed:
            return LpmU1();

        case DsmEvent::ControllerExitLatencyTooLarge:
            return LpmLatencyTooLarge();

        default:
            return SmUnhandled();
    }
}

/* The controller cannot meet the exit latency: try again without U1 and U2 */
SM_RESULT
DeviceMachine::LpmLatencyTooLarge()
{
    if (DisableLinkStatesForLatency())
        return LpmCompute();

    return LpmFailCleanup();
}

SM_RESULT
DeviceMachine::LpmFailCleanup()
{
    if (EndpointsNeedDisableOnFailure())
    {
        return Request(&RequestingCritical, &DeviceMachine::ProgramEndpoints,
                       &DeviceMachine::LpmCleanedUpAfterFailure);
    }

    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::LpmCleanedUpAfterFailure(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::LpmDetachCleanup()
{
    if (EndpointsNeedDisableOnFailure())
    {
        return Request(&RequestingCritical, &DeviceMachine::ProgramEndpoints,
                       &DeviceMachine::LpmCleanedUpAfterDetach);
    }

    return EndWith(DsmEvent::PortDetached);
}

SM_RESULT
DeviceMachine::LpmCleanedUpAfterDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return EndWith(DsmEvent::PortDetached);
}

SM_RESULT
DeviceMachine::LpmU1()
{
    if (!U1TimeoutChanged())
        return LpmEnableU1();

    RequestU1Timeout();
    return Wait(&AwaitingPortYielding, &DeviceMachine::LpmU1TimeoutSet);
}

SM_RESULT
DeviceMachine::LpmU1TimeoutSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortTimeoutSet:
        case DsmEvent::PortFault:
            return LpmEnableU1();

        case DsmEvent::PortDetached:
            return LpmDetachCleanup();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::LpmEnableU1()
{
    switch (U1Change())
    {
        case DsmFeatureDisable:
            return Request(&RequestingCritical, &DeviceMachine::DisableU1,
                           &DeviceMachine::LpmU1Disabled);

        case DsmFeatureEnable:
            return Request(&RequestingCritical, &DeviceMachine::EnableU1,
                           &DeviceMachine::LpmU1Enabled);

        default:
            return LpmU2();
    }
}

SM_RESULT
DeviceMachine::LpmU1Disabled(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    MarkU1Disabled();
    return LpmU2();
}

SM_RESULT
DeviceMachine::LpmU1Enabled(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    MarkU1Enabled();
    return LpmU2();
}

SM_RESULT
DeviceMachine::LpmU2()
{
    if (!U2TimeoutChanged())
        return LpmEnableU2();

    RequestU2Timeout();
    return Wait(&AwaitingPortYielding, &DeviceMachine::LpmU2TimeoutSet);
}

SM_RESULT
DeviceMachine::LpmU2TimeoutSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortTimeoutSet:
        case DsmEvent::PortFault:
            return LpmEnableU2();

        case DsmEvent::PortDetached:
            return LpmDetachCleanup();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::LpmEnableU2()
{
    switch (U2Change())
    {
        case DsmFeatureDisable:
            return Request(&RequestingCritical, &DeviceMachine::DisableU2,
                           &DeviceMachine::LpmU2Disabled);

        case DsmFeatureEnable:
            return Request(&RequestingCritical, &DeviceMachine::EnableU2,
                           &DeviceMachine::LpmU2Enabled);

        default:
            return LpmLower();
    }
}

SM_RESULT
DeviceMachine::LpmU2Disabled(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    MarkU2Disabled();
    return LpmLower();
}

SM_RESULT
DeviceMachine::LpmU2Enabled(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    MarkU2Enabled();
    return LpmLower();
}

SM_RESULT
DeviceMachine::LpmLower()
{
    if (ExitLatencyMayDrop())
    {
        return Request(&RequestingCritical, &DeviceMachine::UpdateExitLatency,
                       &DeviceMachine::LpmLatencyLowered);
    }

    return LpmFinalProgram();
}

SM_RESULT
DeviceMachine::LpmLatencyLowered(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
        case DsmEvent::ControllerIoctlFailed:
            return LpmFinalProgram();

        case DsmEvent::ControllerExitLatencyTooLarge:
            return LpmLatencyTooLarge();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::LpmFinalProgram()
{
    if (EndpointsNeedProgramming())
    {
        return Request(&RequestingCritical, &DeviceMachine::ProgramEndpoints,
                       &DeviceMachine::LpmFinalProgrammed);
    }

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::LpmFinalProgrammed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::ControllerExitLatencyTooLarge:
            return LpmLatencyTooLarge();

        case DsmEvent::ControllerIoctlFailed:
            return LpmFailCleanup();

        default:
            return SmUnhandled();
    }
}
