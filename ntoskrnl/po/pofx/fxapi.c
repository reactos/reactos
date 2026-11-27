/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager Framework API (PoFx) support routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Registers a device with the Power Framework (PoFx).
 *
 * @param[in] Pdo
 * A pointer to a physical device object that wants to be
 * registered with the Power Framework.
 *
 * @param[in] Device
 * A pointer to a Framework device that points to the actual
 * information about the PDO that is to be registered.
 *
 * @param[out] Handle
 * A pointer to a Framework handle returned to the caller after
 * the registration of the device.
 */
NTSTATUS
NTAPI
PoFxRegisterDevice(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PPO_FX_DEVICE Device,
    _Out_ POHANDLE *Handle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Unregisters a device from the Power Framework.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 */
VOID
NTAPI
PoFxUnregisterDevice(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Finish the registration done with a call to PoFxRegisterDevice and
 * puts all the components to an idle state condition
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx, of which device power management
 * is to be started.
 */
VOID
NTAPI
PoFxStartDevicePowerManagement(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Activates a component of a device, after being idle.
 * The function will perform a state transition to the active
 * state if the component cannot be currently accessed.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that is to be activated.
 *
 * @param[in] Flags
 * Flag bitmask provided by the caller that changes the behavior
 * of this function. The following flags are:
 *
 * PO_FX_FLAG_BLOCKING -- The operation is synchronous, the control is
 *                        returned to the caller only when the operation
 *                        has completed.
 *
 * PO_FX_FLAG_ASYNC_ONLY -- The operation is asynchronous, the operation
 *                          will be handled in a separate thread other than
 *                          the calling thread. The control is returned to the
 *                          device driver immediately.
 */
VOID
NTAPI
PoFxActivateComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Acknowledges the Power Framework the device driver has fully executed the
 * DevicePowerNotRequiredCallback callback function.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx, that fully responded to the
 * DevicePowerNotRequiredCallback callback routine.
 */
VOID
NTAPI
PoFxCompleteDevicePowerNotRequired(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Turns a component of a device into the idle state,
 * after being activated previously by a call to PoFxActivateComponent.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that is to be turned into the idle state.
 *
 * @param[in] Flags
 * Flag bitmask provided by the caller that changes the behavior
 * of this function. The following flags are:
 *
 * PO_FX_FLAG_BLOCKING -- The operation is synchronous, the control is
 *                        returned to the caller only when the operation
 *                        has completed.
 *
 * PO_FX_FLAG_ASYNC_ONLY -- The operation is asynchronous, the operation
 *                          will be handled in a separate thread other than
 *                          the calling thread. The control is returned to the
 *                          device driver immediately.
 */
VOID
NTAPI
PoFxIdleComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Acknowledges the Power Framework the component of a device has finished
 * transitioning to the idle state condition.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that has completed the transition to the
 * idle state condition.
 */
VOID
NTAPI
PoFxCompleteIdleCondition(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Acknowledges the Power Framework the component of a device has finished
 * transitioning to a Fx state.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that has completed the transition to the
 * Fx state.
 */
VOID
NTAPI
PoFxCompleteIdleState(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Specifies the minimum time interval from when the last component of
 * the device enters the idle condition to when the Power Framework calls
 * DevicePowerNotRequiredCallback.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] IdleTimeout
 * The idle timeout interval to be supplied.
 */
VOID
NTAPI
PoFxSetDeviceIdleTimeout(
    _In_ POHANDLE Handle,
    _In_ ULONGLONG IdleTimeout)
{
    UNIMPLEMENTED;
}

/**
 * @brief
 * Acknowledges the Power Framework the following device has fully
 * transitioned to the D0 power state.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 */
VOID
NTAPI
PoFxReportDevicePoweredOn(
    _In_ POHANDLE Handle)
{
    UNIMPLEMENTED;
}

/* EOF */
