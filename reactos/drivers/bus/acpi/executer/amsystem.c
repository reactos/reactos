
/******************************************************************************
 *
 * Module Name: amsystem - Interface to OS services
 *              $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "acpi.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "achware.h"
#include "acevents.h"

#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amsystem")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_wait_semaphore
 *
 * PARAMETERS:  Semaphore           - OSD semaphore to wait on
 *              Timeout             - Max time to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Implements a semaphore wait with a check to see if the
 *              semaphore is available immediately.  If it is not, the
 *              interpreter is released.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_wait_semaphore (
	ACPI_HANDLE             semaphore,
	u32                     timeout)
{
	ACPI_STATUS             status;


	status = acpi_os_wait_semaphore (semaphore, 1, 0);
	if (ACPI_SUCCESS (status)) {
		return (status);
	}

	if (status == AE_TIME) {
		/* We must wait, so unlock the interpreter */

		acpi_aml_exit_interpreter ();

		status = acpi_os_wait_semaphore (semaphore, 1, timeout);

		/* Reacquire the interpreter */

		status = acpi_aml_enter_interpreter ();
		if (ACPI_SUCCESS (status)) {
			/* Restore the timeout exception */

			status = AE_TIME;
		}
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_do_stall
 *
 * PARAMETERS:  How_long            - The amount of time to stall
 *
 * RETURN:      None
 *
 * DESCRIPTION: Suspend running thread for specified amount of time.
 *
 ******************************************************************************/

void
acpi_aml_system_do_stall (
	u32                     how_long)
{

	if (how_long > 1000) /* 1 millisecond */ {
		/* Since this thread will sleep, we must release the interpreter */

		acpi_aml_exit_interpreter ();

		acpi_os_sleep_usec (how_long);

		/* And now we must get the interpreter again */

		acpi_aml_enter_interpreter ();
	}

	else {
		acpi_os_sleep_usec (how_long);
	}
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_do_suspend
 *
 * PARAMETERS:  How_long            - The amount of time to suspend
 *
 * RETURN:      None
 *
 * DESCRIPTION: Suspend running thread for specified amount of time.
 *
 ******************************************************************************/

void
acpi_aml_system_do_suspend (
	u32                     how_long)
{
	/* Since this thread will sleep, we must release the interpreter */

	acpi_aml_exit_interpreter ();

	acpi_os_sleep ((u16) (how_long / (u32) 1000),
			  (u16) (how_long % (u32) 1000));

	/* And now we must get the interpreter again */

	acpi_aml_enter_interpreter ();
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_acquire_mutex
 *
 * PARAMETERS:  *Time_desc          - The 'time to delay' object descriptor
 *              *Obj_desc           - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML.  This function will cause a lock to be generated
 *              for the Mutex pointed to by Obj_desc.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_acquire_mutex (
	ACPI_OPERAND_OBJECT     *time_desc,
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_STATUS             status = AE_OK;


	if (!obj_desc) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Support for the _GL_ Mutex object -- go get the global lock
	 */

	if (obj_desc->mutex.semaphore == acpi_gbl_global_lock_semaphore) {
		status = acpi_ev_acquire_global_lock ();
		return (status);
	}

	status = acpi_aml_system_wait_semaphore (obj_desc->mutex.semaphore,
			  (u32) time_desc->integer.value);
	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_release_mutex
 *
 * PARAMETERS:  *Obj_desc           - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML.  This operation is a request to release a
 *              previously acquired Mutex.  If the Mutex variable is set then
 *              it will be decremented.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_release_mutex (
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_STATUS             status = AE_OK;


	if (!obj_desc) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Support for the _GL_ Mutex object -- release the global lock
	 */
	if (obj_desc->mutex.semaphore == acpi_gbl_global_lock_semaphore) {
		acpi_ev_release_global_lock ();
		return (AE_OK);
	}

	status = acpi_os_signal_semaphore (obj_desc->mutex.semaphore, 1);
	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_signal_event
 *
 * PARAMETERS:  *Obj_desc           - The object descriptor for this op
 *
 * RETURN:      AE_OK
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_signal_event (
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_STATUS             status = AE_OK;


	if (obj_desc) {
		status = acpi_os_signal_semaphore (obj_desc->event.semaphore, 1);
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_wait_event
 *
 * PARAMETERS:  *Time_desc          - The 'time to delay' object descriptor
 *              *Obj_desc           - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Provides an access point to perform synchronization operations
 *              within the AML.  This operation is a request to wait for an
 *              event.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_wait_event (
	ACPI_OPERAND_OBJECT     *time_desc,
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_STATUS             status = AE_OK;


	if (obj_desc) {
		status = acpi_aml_system_wait_semaphore (obj_desc->event.semaphore,
				  (u32) time_desc->integer.value);
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_reset_event
 *
 * PARAMETERS:  *Obj_desc           - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reset an event to a known state.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_reset_event (
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_STATUS             status = AE_OK;
	void                    *temp_semaphore;


	/*
	 * We are going to simply delete the existing semaphore and
	 * create a new one!
	 */

	status = acpi_os_create_semaphore (ACPI_NO_UNIT_LIMIT, 0, &temp_semaphore);
	if (ACPI_SUCCESS (status)) {
		acpi_os_delete_semaphore (obj_desc->event.semaphore);
		obj_desc->event.semaphore = temp_semaphore;
	}

	return (status);
}

