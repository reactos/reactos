/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device side of the UCX contract, configuration bookkeeping and link power
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* HubChild::m_LpmPolicy, as the PDO power setting callback writes it */
#define HUB_LPM_POLICY_U1_ENABLED       0x00000001
#define HUB_LPM_POLICY_U2_ENABLED       0x00000002
#define HUB_LPM_POLICY_U1_ACCEPT        0x00000004
#define HUB_LPM_POLICY_U2_ACCEPT        0x00000008
#define HUB_LPM_POLICY_U1_INITIATE      0x00000010
#define HUB_LPM_POLICY_U2_INITIATE      0x00000020
#define HUB_LPM_POLICY_CONSERVATIVE     0x00000040
#define HUB_LPM_POLICY_AGGRESSIVE       0x00000080

/* HubChild::m_LinkFlags */
#define HUB_LINK_TARGET_U1              0x00000001
#define HUB_LINK_TARGET_U2              0x00000002
#define HUB_LINK_U1_OFF_FOR_LATENCY     0x00000004
#define HUB_LINK_U2_OFF_FOR_LATENCY     0x00000008
#define HUB_LINK_OFF_FOR_NO_PING        0x00000010

/* HubChild::m_Lpm20Status */
enum class HubLpm20Status : ULONG
{
    Unknown,
    Enabled,
    DeviceIsHub,
    DeviceHack,
    HubHack,
    GloballyDisabled,
    DeviceNotCapable,
    FirstGenerationDevice,
    PortNotCapable,
    RunningAtSuperSpeed
};

/* devhcd.cpp */

/**
 * Sends the payload already in the device's UCX buffer. On a send failure
 * the failure event is posted here and the completion never runs.
 */
NTSTATUS
NTAPI
HubChildSubmitUcxIoctl(
    _In_ HubChild* Child,
    _In_ ULONG IoControlCode);

/** Moves every pipe of a configuration in state From to state To. */
VOID
NTAPI
HubConfigMovePipes(
    _In_opt_ HubConfiguration* Config,
    _In_ PipeState From,
    _In_ PipeState To);

/** Deletes the UCX endpoints of the disabled pipes, then the interface record. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubChildDeleteInterface(
    _In_ HubChild* Child,
    _In_ HubInterface* Interface);

/* devconfig.cpp */

/** Enabled pipes go to the disable list as PendingDisable, disabled ones stay unchanged. */
VOID
NTAPI
HubChildCollectForDisable(
    _In_ HubChild* Child,
    _In_ HubInterface* Interface);

/* devlpm.cpp */

/** TRUE when an enumerated, unconfigured device gets a U2 timeout; sets that timeout. */
BOOLEAN
NTAPI
HubChildU2ForEnumerated(
    _In_ HubChild* Child);

/* Provided by the child PDO module */

/** NT to USBD status mapping shared by the hub and the child PDO. */
USBD_STATUS
NTAPI
HubNtStatusToUsbd(
    _In_ NTSTATUS Status);
