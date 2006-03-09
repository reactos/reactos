/******************************************************************************
 *
 * Module Name: evxface - External interfaces for ACPI events
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
#include "achware.h"
#include "acnamesp.h"
#include "acevents.h"
#include "amlcode.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_EVENTS
	 MODULE_NAME         ("evxface")


/******************************************************************************
 *
 * FUNCTION:    Acpi_install_fixed_event_handler
 *
 * PARAMETERS:  Event           - Event type to enable.
 *              Handler         - Pointer to the handler function for the
 *                                event
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Saves the pointer to the handler function and then enables the
 *              event.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_install_fixed_event_handler (
	u32                     event,
	FIXED_EVENT_HANDLER     handler,
	void                    *context)
{
	ACPI_STATUS             status;


	/* Parameter validation */

	if (event >= NUM_FIXED_EVENTS) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_EVENTS);

	/* Don't allow two handlers. */

	if (NULL != acpi_gbl_fixed_event_handlers[event].handler) {
		status = AE_EXIST;
		goto cleanup;
	}


	/* Install the handler before enabling the event - just in case... */

	acpi_gbl_fixed_event_handlers[event].handler = handler;
	acpi_gbl_fixed_event_handlers[event].context = context;

	status = acpi_enable_event (event, ACPI_EVENT_FIXED);
	if (!ACPI_SUCCESS (status)) {
		/* Remove the handler */

		acpi_gbl_fixed_event_handlers[event].handler = NULL;
		acpi_gbl_fixed_event_handlers[event].context = NULL;
	}



cleanup:
	acpi_cm_release_mutex (ACPI_MTX_EVENTS);
	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_remove_fixed_event_handler
 *
 * PARAMETERS:  Event           - Event type to disable.
 *              Handler         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disables the event and unregisters the event handler.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_remove_fixed_event_handler (
	u32                     event,
	FIXED_EVENT_HANDLER     handler)
{
	ACPI_STATUS             status = AE_OK;


	/* Parameter validation */

	if (event >= NUM_FIXED_EVENTS) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_EVENTS);

	/* Disable the event before removing the handler - just in case... */

	status = acpi_disable_event(event, ACPI_EVENT_FIXED);

	/* Always Remove the handler */

	acpi_gbl_fixed_event_handlers[event].handler = NULL;
	acpi_gbl_fixed_event_handlers[event].context = NULL;




	acpi_cm_release_mutex (ACPI_MTX_EVENTS);
	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_install_notify_handler
 *
 * PARAMETERS:  Device          - The device for which notifies will be handled
 *              Handler_type    - The type of handler:
 *                                  ACPI_SYSTEM_NOTIFY: System_handler (00-7f)
 *                                  ACPI_DEVICE_NOTIFY: Driver_handler (80-ff)
 *              Handler         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for notifies on an ACPI device
 *
 ******************************************************************************/

ACPI_STATUS
acpi_install_notify_handler (
	ACPI_HANDLE             device,
	u32                     handler_type,
	NOTIFY_HANDLER          handler,
	void                    *context)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *notify_obj;
	ACPI_NAMESPACE_NODE     *device_node;
	ACPI_STATUS             status = AE_OK;


	/* Parameter validation */

	if ((!handler) ||
		(handler_type > ACPI_MAX_NOTIFY_HANDLER_TYPE)) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);

	/* Convert and validate the device handle */

	device_node = acpi_ns_convert_handle_to_entry (device);
	if (!device_node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/*
	 * Root Object:
	 * ------------
	 * Registering a notify handler on the root object indicates that the
	 * caller wishes to receive notifications for all objects.  Note that
	 * only one <external> global handler can be regsitered (per notify type).
	 */
	if (device == ACPI_ROOT_OBJECT) {
		/* Make sure the handler is not already installed */

		if (((handler_type == ACPI_SYSTEM_NOTIFY) &&
			  acpi_gbl_sys_notify.handler) ||
			((handler_type == ACPI_DEVICE_NOTIFY) &&
			  acpi_gbl_drv_notify.handler)) {
			status = AE_EXIST;
			goto unlock_and_exit;
		}

		if (handler_type == ACPI_SYSTEM_NOTIFY) {
			acpi_gbl_sys_notify.node = device_node;
			acpi_gbl_sys_notify.handler = handler;
			acpi_gbl_sys_notify.context = context;
		}
		else /* ACPI_DEVICE_NOTIFY */ {
			acpi_gbl_drv_notify.node = device_node;
			acpi_gbl_drv_notify.handler = handler;
			acpi_gbl_drv_notify.context = context;
		}

		/* Global notify handler installed */
	}

	/*
	 * Other Objects:
	 * --------------
	 * Caller will only receive notifications specific to the target object.
	 * Note that only certain object types can receive notifications.
	 */
	else {
		/*
		 * These are the ONLY objects that can receive ACPI notifications
		 */
		if ((device_node->type != ACPI_TYPE_DEVICE)    &&
			(device_node->type != ACPI_TYPE_PROCESSOR) &&
			(device_node->type != ACPI_TYPE_POWER)     &&
			(device_node->type != ACPI_TYPE_THERMAL)) {
			status = AE_BAD_PARAMETER;
			goto unlock_and_exit;
		}

		/* Check for an existing internal object */

		obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) device_node);
		if (obj_desc) {

			/* Object exists - make sure there's no handler */

			if (((handler_type == ACPI_SYSTEM_NOTIFY) &&
				  obj_desc->device.sys_handler) ||
				((handler_type == ACPI_DEVICE_NOTIFY) &&
				  obj_desc->device.drv_handler)) {
				status = AE_EXIST;
				goto unlock_and_exit;
			}
		}

		else {
			/* Create a new object */

			obj_desc = acpi_cm_create_internal_object (device_node->type);
			if (!obj_desc) {
				status = AE_NO_MEMORY;
				goto unlock_and_exit;
			}

			/* Attach new object to the Node */

			status = acpi_ns_attach_object (device, obj_desc, (u8) device_node->type);

			if (ACPI_FAILURE (status)) {
				goto unlock_and_exit;
			}
		}

		/* Install the handler */

		notify_obj = acpi_cm_create_internal_object (INTERNAL_TYPE_NOTIFY);
		if (!notify_obj) {
			status = AE_NO_MEMORY;
			goto unlock_and_exit;
		}

		notify_obj->notify_handler.node = device_node;
		notify_obj->notify_handler.handler = handler;
		notify_obj->notify_handler.context = context;


		if (handler_type == ACPI_SYSTEM_NOTIFY) {
			obj_desc->device.sys_handler = notify_obj;
		}
		else /* ACPI_DEVICE_NOTIFY */ {
			obj_desc->device.drv_handler = notify_obj;
		}
	}

unlock_and_exit:
	acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_remove_notify_handler
 *
 * PARAMETERS:  Device          - The device for which notifies will be handled
 *              Handler_type    - The type of handler:
 *                                  ACPI_SYSTEM_NOTIFY: System_handler (00-7f)
 *                                  ACPI_DEVICE_NOTIFY: Driver_handler (80-ff)
 *              Handler         - Address of the handler
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a handler for notifies on an ACPI device
 *
 ******************************************************************************/

ACPI_STATUS
acpi_remove_notify_handler (
	ACPI_HANDLE             device,
	u32                     handler_type,
	NOTIFY_HANDLER          handler)
{
	ACPI_OPERAND_OBJECT     *notify_obj;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_NAMESPACE_NODE     *device_node;
	ACPI_STATUS             status = AE_OK;

	/* Parameter validation */

	if ((!handler) ||
		(handler_type > ACPI_MAX_NOTIFY_HANDLER_TYPE)) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);

	/* Convert and validate the device handle */

	device_node = acpi_ns_convert_handle_to_entry (device);
	if (!device_node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/*
	 * Root Object:
	 * ------------
	 */
	if (device == ACPI_ROOT_OBJECT) {

		if (((handler_type == ACPI_SYSTEM_NOTIFY) &&
			  !acpi_gbl_sys_notify.handler) ||
			((handler_type == ACPI_DEVICE_NOTIFY) &&
			  !acpi_gbl_drv_notify.handler)) {
			status = AE_NOT_EXIST;
			goto unlock_and_exit;
		}

		if (handler_type == ACPI_SYSTEM_NOTIFY) {
			acpi_gbl_sys_notify.node = NULL;
			acpi_gbl_sys_notify.handler = NULL;
			acpi_gbl_sys_notify.context = NULL;
		}
		else {
			acpi_gbl_drv_notify.node = NULL;
			acpi_gbl_drv_notify.handler = NULL;
			acpi_gbl_drv_notify.context = NULL;
		}
	}

	/*
	 * Other Objects:
	 * --------------
	 */
	else {
		/*
		 * These are the ONLY objects that can receive ACPI notifications
		 */
		if ((device_node->type != ACPI_TYPE_DEVICE)    &&
			(device_node->type != ACPI_TYPE_PROCESSOR) &&
			(device_node->type != ACPI_TYPE_POWER)     &&
			(device_node->type != ACPI_TYPE_THERMAL)) {
			status = AE_BAD_PARAMETER;
			goto unlock_and_exit;
		}

		/* Check for an existing internal object */

		obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) device_node);
		if (!obj_desc) {
			status = AE_NOT_EXIST;
			goto unlock_and_exit;
		}

		/* Object exists - make sure there's an existing handler */

		if (handler_type == ACPI_SYSTEM_NOTIFY) {
			notify_obj = obj_desc->device.sys_handler;
		}
		else {
			notify_obj = obj_desc->device.drv_handler;
		}

		if ((!notify_obj) ||
			(notify_obj->notify_handler.handler != handler)) {
			status = AE_BAD_PARAMETER;
			goto unlock_and_exit;
		}

		/* Remove the handler */

		if (handler_type == ACPI_SYSTEM_NOTIFY) {
			obj_desc->device.sys_handler = NULL;
		}
		else {
			obj_desc->device.drv_handler = NULL;
		}

		acpi_cm_remove_reference (notify_obj);
	}


unlock_and_exit:
	acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_install_gpe_handler
 *
 * PARAMETERS:  Gpe_number      - The GPE number.  The numbering scheme is
 *                                bank 0 first, then bank 1.
 *              Type            - Whether this GPE should be treated as an
 *                                edge- or level-triggered interrupt.
 *              Handler         - Address of the handler
 *              Context         - Value passed to the handler on each GPE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for a General Purpose Event.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_install_gpe_handler (
	u32                     gpe_number,
	u32                     type,
	GPE_HANDLER             handler,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;

	/* Parameter validation */

	if (!handler || (gpe_number > NUM_GPE)) {
		return (AE_BAD_PARAMETER);
	}

	/* Ensure that we have a valid GPE number */

	if (acpi_gbl_gpe_valid[gpe_number] == ACPI_GPE_INVALID) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_EVENTS);

	/* Make sure that there isn't a handler there already */

	if (acpi_gbl_gpe_info[gpe_number].handler) {
		status = AE_EXIST;
		goto cleanup;
	}

	/* Install the handler */

	acpi_gbl_gpe_info[gpe_number].handler = handler;
	acpi_gbl_gpe_info[gpe_number].context = context;
	acpi_gbl_gpe_info[gpe_number].type = (u8) type;

	/* Clear the GPE (of stale events), the enable it */

	acpi_hw_clear_gpe (gpe_number);
	acpi_hw_enable_gpe (gpe_number);

cleanup:
	acpi_cm_release_mutex (ACPI_MTX_EVENTS);
	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_remove_gpe_handler
 *
 * PARAMETERS:  Gpe_number      - The event to remove a handler
 *              Handler         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a handler for a General Purpose Acpi_event.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_remove_gpe_handler (
	u32                     gpe_number,
	GPE_HANDLER             handler)
{
	ACPI_STATUS             status = AE_OK;


	/* Parameter validation */

	if (!handler || (gpe_number > NUM_GPE)) {
		return (AE_BAD_PARAMETER);
	}

	/* Ensure that we have a valid GPE number */

	if (acpi_gbl_gpe_valid[gpe_number] == ACPI_GPE_INVALID) {
		return (AE_BAD_PARAMETER);
	}

	/* Disable the GPE before removing the handler */

	acpi_hw_disable_gpe (gpe_number);

	acpi_cm_acquire_mutex (ACPI_MTX_EVENTS);

	/* Make sure that the installed handler is the same */

	if (acpi_gbl_gpe_info[gpe_number].handler != handler) {
		acpi_hw_enable_gpe (gpe_number);
		status = AE_BAD_PARAMETER;
		goto cleanup;
	}

	/* Remove the handler */

	acpi_gbl_gpe_info[gpe_number].handler = NULL;
	acpi_gbl_gpe_info[gpe_number].context = NULL;

cleanup:
	acpi_cm_release_mutex (ACPI_MTX_EVENTS);
	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_acquire_global_lock
 *
 * PARAMETERS:  Timeout         - How long the caller is willing to wait
 *              Out_handle      - A handle to the lock if acquired
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire the ACPI Global Lock
 *
 ******************************************************************************/
ACPI_STATUS
acpi_acquire_global_lock (
	void)
{
	ACPI_STATUS             status;


	status = acpi_aml_enter_interpreter ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/*
	 * TBD: [Restructure] add timeout param to internal interface, and
	 * perhaps INTERPRETER_LOCKED
	 */

	status = acpi_ev_acquire_global_lock ();
	acpi_aml_exit_interpreter ();

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_release_global_lock
 *
 * PARAMETERS:  Handle      - Returned from Acpi_acquire_global_lock
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release the ACPI Global Lock
 *
 ******************************************************************************/

ACPI_STATUS
acpi_release_global_lock (
	void)
{
	acpi_ev_release_global_lock ();
	return (AE_OK);
}


