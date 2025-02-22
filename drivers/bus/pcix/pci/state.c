/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/state.c
 * PURPOSE:         Bus/Device State Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCHAR PciTransitionText[PciMaxObjectState + 1] =
{
    "PciNotStarted",
    "PciStarted",
    "PciDeleted",
    "PciStopped",
    "PciSurpriseRemoved",
    "PciSynchronizedOperation",
    "PciMaxObjectState"
};

NTSTATUS PnpStateCancelArray[PciMaxObjectState] =
{
    STATUS_INVALID_DEVICE_REQUEST,
    STATUS_FAIL_CHECK,
    STATUS_INVALID_DEVICE_STATE,
    STATUS_INVALID_DEVICE_STATE,
    STATUS_FAIL_CHECK,
    STATUS_FAIL_CHECK
};

NTSTATUS PnpStateTransitionArray[PciMaxObjectState * PciMaxObjectState] =
{
    STATUS_SUCCESS,                 // Not Started -> Not Started
    STATUS_SUCCESS,                 // Started -> Not Started
    STATUS_FAIL_CHECK,              // Deleted -> Not Started
    STATUS_SUCCESS,                 // Stopped -> Not Started
    STATUS_FAIL_CHECK,              // Surprise Removed -> Not Started
    STATUS_FAIL_CHECK,              // Synchronized Operation -> Not Started

    STATUS_SUCCESS,                 // Not Started -> Started
    STATUS_FAIL_CHECK,              // Started -> Started
    STATUS_FAIL_CHECK,              // Deleted -> Started
    STATUS_SUCCESS,                 // Stopped -> Started
    STATUS_FAIL_CHECK,              // Surprise Removed -> Started
    STATUS_FAIL_CHECK,              // Synchronized Operation -> Started

    STATUS_SUCCESS,                 // Not Started -> Deleted
    STATUS_SUCCESS,                 // Started -> Deleted
    STATUS_FAIL_CHECK,              // Deleted -> Deleted
    STATUS_FAIL_CHECK,              // Stopped -> Deleted
    STATUS_SUCCESS,                 // Surprise Removed -> Deleted
    STATUS_FAIL_CHECK,              // Synchronized Operation -> Deleted

    STATUS_INVALID_DEVICE_REQUEST,  // Not Started -> Stopped
    STATUS_SUCCESS,                 // Started -> Stopped
    STATUS_FAIL_CHECK,              // Deleted -> Stopped
    STATUS_FAIL_CHECK,              // Stopped -> Stopped
    STATUS_FAIL_CHECK,              // Surprise Removed -> Stopped
    STATUS_FAIL_CHECK,              // Synchronized Operation -> Stopped

    STATUS_SUCCESS,                 // Not Started -> Surprise Removed
    STATUS_SUCCESS,                 // Started -> Surprise Removed
    STATUS_FAIL_CHECK,              // Deleted -> Surprise Removed
    STATUS_SUCCESS,                 // Stopped -> Surprise Removed
    STATUS_FAIL_CHECK,              // Surprise Removed -> Surprise Removed
    STATUS_FAIL_CHECK,              // Synchronized Operation -> Surprise Removed

    STATUS_SUCCESS,                 // Not Started -> Synchronized Operation
    STATUS_SUCCESS,                 // Started -> Synchronized Operation
    STATUS_INVALID_DEVICE_STATE,    // Deleted -> Synchronized Operation
    STATUS_SUCCESS,                 // Stopped -> Synchronized Operation
    STATUS_INVALID_DEVICE_STATE,    // Surprise Removed -> Synchronized Operation
    STATUS_FAIL_CHECK               // Synchronized Operation -> Synchronized Operation
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
PciInitializeState(IN PPCI_FDO_EXTENSION DeviceExtension)
{
    /* Set the initial state */
    DeviceExtension->DeviceState = PciNotStarted;
    DeviceExtension->TentativeNextState = PciNotStarted;
}

NTSTATUS
NTAPI
PciBeginStateTransition(IN PPCI_FDO_EXTENSION DeviceExtension,
                        IN PCI_STATE NewState)
{
    PCI_STATE CurrentState;
    NTSTATUS Status;
    DPRINT1("PCI Request to begin transition of Extension %p to %s ->",
            DeviceExtension,
            PciTransitionText[NewState]);

    /* Assert the device isn't already in a pending transition */
    ASSERT(DeviceExtension->TentativeNextState == DeviceExtension->DeviceState);

    /* Assert this is a valid state */
    CurrentState = DeviceExtension->DeviceState;
    ASSERT(CurrentState < PciMaxObjectState);
    ASSERT(NewState < PciMaxObjectState);

    /* Lookup if this state transition is valid */
    Status = PnpStateTransitionArray[CurrentState + 6 * NewState];
    if (Status == STATUS_FAIL_CHECK)
    {
        /* Invalid transition (logical fault) */
        DPRINT1("ERROR\nPCI: Error trying to enter state \"%s\" "
                "from state \"%s\"\n",
                PciTransitionText[NewState],
                PciTransitionText[CurrentState]);
        DbgBreakPoint();
    }
    else if (Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        /* Invalid transition (illegal request) */
        DPRINT1("ERROR\nPCI: Illegal request to try to enter state \"%s\" "
                "from state \"%s\", rejecting",
                PciTransitionText[NewState],
                PciTransitionText[CurrentState]);
    }

    /* New state must be different from current, unless request is at fault */
    ASSERT((NewState != DeviceExtension->DeviceState) || (!NT_SUCCESS(Status)));

    /* Enter the new state if successful, and return state status */
    if (NT_SUCCESS(Status)) DeviceExtension->TentativeNextState = NewState;
    DbgPrint("%x\n", Status);
    return Status;
}

NTSTATUS
NTAPI
PciCancelStateTransition(IN PPCI_FDO_EXTENSION DeviceExtension,
                         IN PCI_STATE StateNotEntered)
{
    NTSTATUS Status;
    DPRINT1("PCI Request to cancel transition of Extension %p to %s ->",
            DeviceExtension,
            PciTransitionText[StateNotEntered]);

    /* The next state can't be the state the device is already in */
    if (DeviceExtension->TentativeNextState == DeviceExtension->DeviceState)
    {
        /* It's too late since the state was already committed */
        ASSERT(StateNotEntered < PciMaxObjectState);
        ASSERT(PnpStateCancelArray[StateNotEntered] != STATUS_FAIL_CHECK);

        /* Return failure */
        Status = STATUS_INVALID_DEVICE_STATE;
        DbgPrint("%x\n", Status);
    }
    else
    {
        /* The device hasn't yet entered the state, so it's still possible to cancel */
        ASSERT(DeviceExtension->TentativeNextState == StateNotEntered);
        DeviceExtension->TentativeNextState = DeviceExtension->DeviceState;

        /* Return success */
        Status = STATUS_SUCCESS;
        DbgPrint("%x\n", Status);
    }

    /* Return the cancel state */
    return Status;
}

VOID
NTAPI
PciCommitStateTransition(IN PPCI_FDO_EXTENSION DeviceExtension,
                         IN PCI_STATE NewState)
{
    DPRINT1("PCI Commit transition of Extension %p to %s\n",
            DeviceExtension, PciTransitionText[NewState]);

    /* Make sure this is a valid commit */
    ASSERT(NewState != PciSynchronizedOperation);
    ASSERT(DeviceExtension->TentativeNextState == NewState);

    /* Enter the new state */
    DeviceExtension->DeviceState = NewState;
}

/* EOF */
