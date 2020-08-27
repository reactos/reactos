/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Virtual DOS Machine (VDM) support routines
 * COPYRIGHT:   Copyright Stefan Ginsberg (stefan.ginsberg@reactos.org)
 *              Copyright 2020 George Bișoc (george.bisoc@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ERESOURCE VdmIoListCreationResource;

/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Deletes Virtual DOS Machine (DOS) objects, given an Executive process (EPROCESS).
 * 
 * @param[in] Process
 * A pointer to an Executive process.
 * 
 * @return
 * Nothing.
 * 
 */
VOID
NTAPI
PspDeleteVdmObjects(IN PEPROCESS Process)
{
    /* If there are no VDM objects, just exit */
    if (Process->VdmObjects == NULL)
    {
        DPRINT1("PspDeleteVdmObjects(): No VDM objects could be found, exiting...\n");
        return;
    }

    /* Otherwise return the quota it was taking up and free them */
    PsReturnProcessNonPagedPoolQuota(Process->VdmObjects, sizeof(Process->VdmObjects));
    ExFreePoolWithTag(Process->VdmObjects, '  eK');
    Process->VdmObjects = NULL;

    /* Remove the VDM resource from the system's resources list and get our process quota */
    ExDeleteResourceLite(&VdmIoListCreationResource);
    PsReturnProcessPagedPoolQuota(Process, sizeof(Process));
    DPRINT1("PspDeleteVdmObjects(): Successfully deleted the VDM objects!\n");
}

/**
 * @brief
 * Initializes the Virtual DOS Machine (DOS) resource in boot phase.
 *
 * @return
 * The call returns STATUS_SUCCESS when the resources are fully initialised.
 *
 * @remarks
 * The call is invoked in PspInitPhase0, during Process structure (Ps) kernel
 * service initialisation phase. In that phase both VDM and LDT support mechanisms
 * are initialised. The VDM resource list is freed by calling PspDeleteVdmObjects.
 * 
 */
NTSTATUS
NTAPI
PspVdmInitialize(VOID)
{
    /* Call the Executive to initialise our VDM resource */
    return ExInitializeResourceLite(&VdmIoListCreationResource);
}
