/******************************************************************************
 *
 * Module Name: evmisc - ACPI device notification handler dispatch
 *                       and ACPI Global Lock support
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
#include "acevents.h"
#include "acnamesp.h"
#include "acinterp.h"
#include "achware.h"

#define _COMPONENT          ACPI_EVENTS
	 MODULE_NAME         ("evmisc")


/**************************************************************************
 *
 * FUNCTION:    Acpi_ev_queue_notify_request
 *
 * PARAMETERS:
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Dispatch a device notification event to a previously
 *              installed handler.
 *
 *************************************************************************/

ACPI_STATUS
acpi_ev_queue_notify_request (
	ACPI_NAMESPACE_NODE     *node,
	u32                     notify_value)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *handler_obj = NULL;
	ACPI_GENERIC_STATE      *notify_info;
	ACPI_STATUS             status = AE_OK;


	/*
	 * For value 1 (Ejection Request), some device method may need to be run.
	 * For value 2 (Device Wake) if _PRW exists, the _PS0 method may need to be run.
	 * For value 0x80 (Status Change) on the power button or sleep button,
	 * initiate soft-off or sleep operation?
	 */

	switch (notify_value) {
	case 0:
		break;

	case 1:
		break;

	case 2:
		break;

	case 0x80:
		break;

	default:
		break;
	}


	/*
	 * Get the notify object attached to the device Node
	 */

	obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) node);
	if (obj_desc) {

		/* We have the notify object, Get the right handler */

		switch (node->type) {
		case ACPI_TYPE_DEVICE:
			if (notify_value <= MAX_SYS_NOTIFY) {
				handler_obj = obj_desc->device.sys_handler;
			}
			else {
				handler_obj = obj_desc->device.drv_handler;
			}
			break;

		case ACPI_TYPE_THERMAL:
			if (notify_value <= MAX_SYS_NOTIFY) {
				handler_obj = obj_desc->thermal_zone.sys_handler;
			}
			else {
				handler_obj = obj_desc->thermal_zone.drv_handler;
			}
			break;
		}
	}


	/* If there is any handler to run, schedule the dispatcher */

	if ((acpi_gbl_sys_notify.handler && (notify_value <= MAX_SYS_NOTIFY)) ||
		(acpi_gbl_drv_notify.handler && (notify_value > MAX_SYS_NOTIFY)) ||
		handler_obj) {

		notify_info = acpi_cm_create_generic_state ();
		if (!notify_info) {
			return (AE_NO_MEMORY);
		}

		notify_info->notify.node      = node;
		notify_info->notify.value     = (u16) notify_value;
		notify_info->notify.handler_obj = handler_obj;

		status = acpi_os_queue_for_execution (OSD_PRIORITY_HIGH,
				  acpi_ev_notify_dispatch, notify_info);
		if (ACPI_FAILURE (status)) {
			acpi_cm_delete_generic_state (notify_info);
		}
	}

	if (!handler_obj) {
		/* There is no per-device notify handler for this device */

	}


	return (status);
}


/**************************************************************************
 *
 * FUNCTION:    Acpi_ev_notify_dispatch
 *
 * PARAMETERS:
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Dispatch a device notification event to a previously
 *              installed handler.
 *
 *************************************************************************/

void
acpi_ev_notify_dispatch (
	void                    *context)
{
	ACPI_GENERIC_STATE      *notify_info = (ACPI_GENERIC_STATE *) context;
	NOTIFY_HANDLER          global_handler = NULL;
	void                    *global_context = NULL;
	ACPI_OPERAND_OBJECT     *handler_obj;


	/*
	 * We will invoke a global notify handler if installed.
	 * This is done _before_ we invoke the per-device handler attached to the device.
	 */

	if (notify_info->notify.value <= MAX_SYS_NOTIFY) {
		/* Global system notification handler */

		if (acpi_gbl_sys_notify.handler) {
			global_handler = acpi_gbl_sys_notify.handler;
			global_context = acpi_gbl_sys_notify.context;
		}
	}

	else {
		/* Global driver notification handler */

		if (acpi_gbl_drv_notify.handler) {
			global_handler = acpi_gbl_drv_notify.handler;
			global_context = acpi_gbl_drv_notify.context;
		}
	}


	/* Invoke the system handler first, if present */

	if (global_handler) {
		global_handler (notify_info->notify.node, notify_info->notify.value, global_context);
	}

	/* Now invoke the per-device handler, if present */

	handler_obj = notify_info->notify.handler_obj;
	if (handler_obj) {
		handler_obj->notify_handler.handler (notify_info->notify.node, notify_info->notify.value,
				  handler_obj->notify_handler.context);
	}


	/* All done with the info object */

	acpi_cm_delete_generic_state (notify_info);
}


/***************************************************************************
 *
 * FUNCTION:    Acpi_ev_global_lock_thread
 *
 * RETURN:      None
 *
 * DESCRIPTION: Invoked by SCI interrupt handler upon acquisition of the
 *              Global Lock.  Simply signal all threads that are waiting
 *              for the lock.
 *
 **************************************************************************/

static void
acpi_ev_global_lock_thread (
	void                    *context)
{

	/* Signal threads that are waiting for the lock */

	if (acpi_gbl_global_lock_thread_count) {
		/* Send sufficient units to the semaphore */

		acpi_os_signal_semaphore (acpi_gbl_global_lock_semaphore,
				 acpi_gbl_global_lock_thread_count);
	}
}


/***************************************************************************
 *
 * FUNCTION:    Acpi_ev_global_lock_handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Invoked directly from the SCI handler when a global lock
 *              release interrupt occurs.  Grab the global lock and queue
 *              the global lock thread for execution
 *
 **************************************************************************/

static u32
acpi_ev_global_lock_handler (
	void                    *context)
{
	u8                      acquired = FALSE;
	void                    *global_lock;


	/*
	 * Attempt to get the lock
	 * If we don't get it now, it will be marked pending and we will
	 * take another interrupt when it becomes free.
	 */

	global_lock = acpi_gbl_FACS->global_lock;
	ACPI_ACQUIRE_GLOBAL_LOCK (global_lock, acquired);
	if (acquired) {
		/* Got the lock, now wake all threads waiting for it */

		acpi_gbl_global_lock_acquired = TRUE;

		/* Run the Global Lock thread which will signal all waiting threads */

		acpi_os_queue_for_execution (OSD_PRIORITY_HIGH, acpi_ev_global_lock_thread,
				  context);
	}

	return (INTERRUPT_HANDLED);
}


/***************************************************************************
 *
 * FUNCTION:    Acpi_ev_init_global_lock_handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for the global lock release event
 *
 **************************************************************************/

ACPI_STATUS
acpi_ev_init_global_lock_handler (void)
{
	ACPI_STATUS             status;


	acpi_gbl_global_lock_present = TRUE;
	status = acpi_install_fixed_event_handler (ACPI_EVENT_GLOBAL,
			  acpi_ev_global_lock_handler, NULL);

	/*
	 * If the global lock does not exist on this platform, the attempt
	 * to enable GBL_STS will fail (the GBL_EN bit will not stick)
	 * Map to AE_OK, but mark global lock as not present.
	 * Any attempt to actually use the global lock will be flagged
	 * with an error.
	 */
	if (status == AE_NO_HARDWARE_RESPONSE) {
		acpi_gbl_global_lock_present = FALSE;
		status = AE_OK;
	}

	return (status);
}


/***************************************************************************
 *
 * FUNCTION:    Acpi_ev_acquire_global_lock
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Attempt to gain ownership of the Global Lock.
 *
 **************************************************************************/

ACPI_STATUS
acpi_ev_acquire_global_lock(void)
{
	ACPI_STATUS             status = AE_OK;
	u8                      acquired = FALSE;
	void                    *global_lock;


	/* Make sure that we actually have a global lock */

	if (!acpi_gbl_global_lock_present) {
		return (AE_NO_GLOBAL_LOCK);
	}

	/* One more thread wants the global lock */

	acpi_gbl_global_lock_thread_count++;


	/* If we (OS side) have the hardware lock already, we are done */

	if (acpi_gbl_global_lock_acquired) {
		return (AE_OK);
	}

	/* Only if the FACS is valid */

	if (!acpi_gbl_FACS) {
		return (AE_OK);
	}


	/* We must acquire the actual hardware lock */

	global_lock = acpi_gbl_FACS->global_lock;
	ACPI_ACQUIRE_GLOBAL_LOCK (global_lock, acquired);
	if (acquired) {
	   /* We got the lock */

		acpi_gbl_global_lock_acquired = TRUE;

		return (AE_OK);
	}


	/*
	 * Did not get the lock.  The pending bit was set above, and we must now
	 * wait until we get the global lock released interrupt.
	 */

	 /*
	  * Acquire the global lock semaphore first.
	  * Since this wait will block, we must release the interpreter
	  */

	status = acpi_aml_system_wait_semaphore (acpi_gbl_global_lock_semaphore,
			  ACPI_UINT32_MAX);

	return (status);
}


/***************************************************************************
 *
 * FUNCTION:    Acpi_ev_release_global_lock
 *
 * DESCRIPTION: Releases ownership of the Global Lock.
 *
 **************************************************************************/

void
acpi_ev_release_global_lock (void)
{
	u8                      pending = FALSE;
	void                    *global_lock;


	if (!acpi_gbl_global_lock_thread_count) {
		REPORT_WARNING(("Global Lock has not be acquired, cannot release\n"));
		return;
	}

   /* One fewer thread has the global lock */

	acpi_gbl_global_lock_thread_count--;

	/* Have all threads released the lock? */

	if (!acpi_gbl_global_lock_thread_count) {
		/*
		 * No more threads holding lock, we can do the actual hardware
		 * release
		 */

		global_lock = acpi_gbl_FACS->global_lock;
		ACPI_RELEASE_GLOBAL_LOCK (global_lock, pending);
		acpi_gbl_global_lock_acquired = FALSE;

		/*
		 * If the pending bit was set, we must write GBL_RLS to the control
		 * register
		 */
		if (pending) {
			acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK,
					 GBL_RLS, 1);
		}
	}

	return;
}
