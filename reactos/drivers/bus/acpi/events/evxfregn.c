/******************************************************************************
 *
 * Module Name: evxfregn - External Interfaces, ACPI Operation Regions and
 *                         Address Spaces.
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
	 MODULE_NAME         ("evxfregn")


/******************************************************************************
 *
 * FUNCTION:    Acpi_install_address_space_handler
 *
 * PARAMETERS:  Device          - Handle for the device
 *              Space_id        - The address space ID
 *              Handler         - Address of the handler
 *              Setup           - Address of the setup function
 *              Context         - Value passed to the handler on each access
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for all Op_regions of a given Space_id.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_install_address_space_handler (
	ACPI_HANDLE             device,
	ACPI_ADDRESS_SPACE_TYPE space_id,
	ADDRESS_SPACE_HANDLER   handler,
	ADDRESS_SPACE_SETUP     setup,
	void                    *context)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *handler_obj;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_STATUS             status = AE_OK;
	OBJECT_TYPE_INTERNAL    type;
	u16                     flags = 0;


	/* Parameter validation */

	if ((!device)   ||
		((!handler)  && (handler != ACPI_DEFAULT_HANDLER)) ||
		(space_id > ACPI_MAX_ADDRESS_SPACE)) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);

	/* Convert and validate the device handle */

	node = acpi_ns_convert_handle_to_entry (device);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	/*
	 *  This registration is valid for only the types below
	 *  and the root.  This is where the default handlers
	 *  get placed.
	 */

	if ((node->type != ACPI_TYPE_DEVICE)     &&
		(node->type != ACPI_TYPE_PROCESSOR)  &&
		(node->type != ACPI_TYPE_THERMAL)    &&
		(node != acpi_gbl_root_node)) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}

	if (handler == ACPI_DEFAULT_HANDLER) {
		flags = ADDR_HANDLER_DEFAULT_INSTALLED;

		switch (space_id) {
		case ADDRESS_SPACE_SYSTEM_MEMORY:
			handler = acpi_aml_system_memory_space_handler;
			setup = acpi_ev_system_memory_region_setup;
			break;

		case ADDRESS_SPACE_SYSTEM_IO:
			handler = acpi_aml_system_io_space_handler;
			setup = acpi_ev_io_space_region_setup;
			break;

		case ADDRESS_SPACE_PCI_CONFIG:
			handler = acpi_aml_pci_config_space_handler;
			setup = acpi_ev_pci_config_region_setup;
			break;

		default:
			status = AE_NOT_EXIST;
			goto unlock_and_exit;
			break;
		}
	}

	/*
	 *  If the caller hasn't specified a setup routine, use the default
	 */
	if (!setup) {
		setup = acpi_ev_default_region_setup;
	}

	/*
	 *  Check for an existing internal object
	 */

	obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) node);
	if (obj_desc) {
		/*
		 *  The object exists.
		 *  Make sure the handler is not already installed.
		 */

		/* check the address handler the user requested */

		handler_obj = obj_desc->device.addr_handler;
		while (handler_obj) {
			/*
			 *  We have an Address handler, see if user requested this
			 *  address space.
			 */
			if(handler_obj->addr_handler.space_id == space_id) {
				status = AE_EXIST;
				goto unlock_and_exit;
			}

			/*
			 *  Move through the linked list of handlers
			 */
			handler_obj = handler_obj->addr_handler.next;
		}
	}

	else {
		/* Obj_desc does not exist, create one */

		if (node->type == ACPI_TYPE_ANY) {
			type = ACPI_TYPE_DEVICE;
		}

		else {
			type = node->type;
		}

		obj_desc = acpi_cm_create_internal_object (type);
		if (!obj_desc) {
			status = AE_NO_MEMORY;
			goto unlock_and_exit;
		}

		/* Init new descriptor */

		obj_desc->common.type = (u8) type;

		/* Attach the new object to the Node */

		status = acpi_ns_attach_object (node, obj_desc, (u8) type);
		if (ACPI_FAILURE (status)) {
			acpi_cm_remove_reference (obj_desc);
			goto unlock_and_exit;
		}
	}

	/*
	 *  Now we can install the handler
	 *
	 *  At this point we know that there is no existing handler.
	 *  So, we just allocate the object for the handler and link it
	 *  into the list.
	 */
	handler_obj = acpi_cm_create_internal_object (INTERNAL_TYPE_ADDRESS_HANDLER);
	if (!handler_obj) {
		status = AE_NO_MEMORY;
		goto unlock_and_exit;
	}

	handler_obj->addr_handler.space_id  = (u8) space_id;
	handler_obj->addr_handler.hflags    = flags;
	handler_obj->addr_handler.next      = obj_desc->device.addr_handler;
	handler_obj->addr_handler.region_list = NULL;
	handler_obj->addr_handler.node      = node;
	handler_obj->addr_handler.handler   = handler;
	handler_obj->addr_handler.context   = context;
	handler_obj->addr_handler.setup     = setup;

	/*
	 *  Now walk the namespace finding all of the regions this
	 *  handler will manage.
	 *
	 *  We start at the device and search the branch toward
	 *  the leaf nodes until either the leaf is encountered or
	 *  a device is detected that has an address handler of the
	 *  same type.
	 *
	 *  In either case we back up and search down the remainder
	 *  of the branch
	 */
	status = acpi_ns_walk_namespace (ACPI_TYPE_ANY, device,
			 ACPI_UINT32_MAX, NS_WALK_UNLOCK,
			 acpi_ev_addr_handler_helper,
			 handler_obj, NULL);

	/*
	 *  Place this handler 1st on the list
	 */

	handler_obj->common.reference_count =
			 (u16) (handler_obj->common.reference_count +
			 obj_desc->common.reference_count - 1);
	obj_desc->device.addr_handler = handler_obj;


unlock_and_exit:
	acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_remove_address_space_handler
 *
 * PARAMETERS:  Space_id        - The address space ID
 *              Handler         - Address of the handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a handler for accesses on an Operation Region
 *
 ******************************************************************************/

ACPI_STATUS
acpi_remove_address_space_handler (
	ACPI_HANDLE             device,
	ACPI_ADDRESS_SPACE_TYPE space_id,
	ADDRESS_SPACE_HANDLER   handler)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *handler_obj;
	ACPI_OPERAND_OBJECT     *region_obj;
	ACPI_OPERAND_OBJECT     **last_obj_ptr;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_STATUS             status = AE_OK;


	/* Parameter validation */

	if ((!device)   ||
		((!handler)  && (handler != ACPI_DEFAULT_HANDLER)) ||
		(space_id > ACPI_MAX_ADDRESS_SPACE)) {
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);

	/* Convert and validate the device handle */

	node = acpi_ns_convert_handle_to_entry (device);
	if (!node) {
		status = AE_BAD_PARAMETER;
		goto unlock_and_exit;
	}


	/* Make sure the internal object exists */

	obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) node);
	if (!obj_desc) {
		/*
		 *  The object DNE.
		 */
		status = AE_NOT_EXIST;
		goto unlock_and_exit;
	}

	/*
	 *  find the address handler the user requested
	 */

	handler_obj = obj_desc->device.addr_handler;
	last_obj_ptr = &obj_desc->device.addr_handler;
	while (handler_obj) {
		/*
		 *  We have a handler, see if user requested this one
		 */

		if(handler_obj->addr_handler.space_id == space_id) {
			/*
			 *  Got it, first dereference this in the Regions
			 */
			region_obj = handler_obj->addr_handler.region_list;

			/* Walk the handler's region list */

			while (region_obj) {
				/*
				 *  First disassociate the handler from the region.
				 *
				 *  NOTE: this doesn't mean that the region goes away
				 *  The region is just inaccessible as indicated to
				 *  the _REG method
				 */
				acpi_ev_disassociate_region_from_handler(region_obj, FALSE);

				/*
				 *  Walk the list, since we took the first region and it
				 *  was removed from the list by the dissassociate call
				 *  we just get the first item on the list again
				 */
				region_obj = handler_obj->addr_handler.region_list;

			}

			/*
			 *  Remove this Handler object from the list
			 */
			*last_obj_ptr = handler_obj->addr_handler.next;

			/*
			 *  Now we can delete the handler object
			 */
			acpi_cm_remove_reference (handler_obj);
			acpi_cm_remove_reference (handler_obj);

			goto unlock_and_exit;
		}

		/*
		 *  Move through the linked list of handlers
		 */
		last_obj_ptr = &handler_obj->addr_handler.next;
		handler_obj = handler_obj->addr_handler.next;
	}


	/*
	 *  The handler does not exist
	 */
	status = AE_NOT_EXIST;


unlock_and_exit:
	acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
	return (status);
}


