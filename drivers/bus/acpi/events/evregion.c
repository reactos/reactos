/******************************************************************************
 *
 * Module Name: evregion - ACPI Address_space (Op_region) handler dispatch
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
#include "amlcode.h"

#define _COMPONENT          ACPI_EVENTS
	 MODULE_NAME         ("evregion")


/**************************************************************************
 *
 * FUNCTION:    Acpi_ev_install_default_address_space_handlers
 *
 * PARAMETERS:
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Installs the core subsystem address space handlers.
 *
 *************************************************************************/

ACPI_STATUS
acpi_ev_install_default_address_space_handlers (
	void)
{
	ACPI_STATUS             status;


	/*
	 * All address spaces (PCI Config, EC, SMBus) are scope dependent
	 * and registration must occur for a specific device.  In the case
	 * system memory and IO address spaces there is currently no device
	 * associated with the address space.  For these we use the root.
	 * We install the default PCI config space handler at the root so
	 * that this space is immediately available even though the we have
	 * not enumerated all the PCI Root Buses yet.  This is to conform
	 * to the ACPI specification which states that the PCI config
	 * space must be always available -- even though we are nowhere
	 * near ready to find the PCI root buses at this point.
	 *
	 * NOTE: We ignore AE_EXIST because this means that a handler has
	 * already been installed (via Acpi_install_address_space_handler)
	 */

	status = acpi_install_address_space_handler (acpi_gbl_root_node,
			   ADDRESS_SPACE_SYSTEM_MEMORY,
			   ACPI_DEFAULT_HANDLER, NULL, NULL);
	if ((ACPI_FAILURE (status)) &&
		(status != AE_EXIST)) {
		return (status);
	}

	status = acpi_install_address_space_handler (acpi_gbl_root_node,
			   ADDRESS_SPACE_SYSTEM_IO,
			   ACPI_DEFAULT_HANDLER, NULL, NULL);
	if ((ACPI_FAILURE (status)) &&
		(status != AE_EXIST)) {
		return (status);
	}

	status = acpi_install_address_space_handler (acpi_gbl_root_node,
			   ADDRESS_SPACE_PCI_CONFIG,
			   ACPI_DEFAULT_HANDLER, NULL, NULL);
	if ((ACPI_FAILURE (status)) &&
		(status != AE_EXIST)) {
		return (status);
	}


	return (AE_OK);
}


/* TBD: [Restructure] Move elsewhere */

/**************************************************************************
 *
 * FUNCTION:    Acpi_ev_execute_reg_method
 *
 * PARAMETERS:  Region_obj          - Object structure
 *              Function            - On (1) or Off (0)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute _REG method for a region
 *
 *************************************************************************/

static ACPI_STATUS
acpi_ev_execute_reg_method (
	ACPI_OPERAND_OBJECT    *region_obj,
	u32                     function)
{
	ACPI_OPERAND_OBJECT    *params[3];
	ACPI_OPERAND_OBJECT     space_id_desc;
	ACPI_OPERAND_OBJECT     function_desc;
	ACPI_STATUS             status;


	if (region_obj->region.extra->extra.method_REG == NULL) {
		return (AE_OK);
	}

	/*
	 *  _REG method has two arguments
	 *  Arg0:   Integer: Operation region space ID
	 *          Same value as Region_obj->Region.Space_id
	 *  Arg1:   Integer: connection status
	 *          1 for connecting the handler,
	 *          0 for disconnecting the handler
	 *          Passed as a parameter
	 */

	acpi_cm_init_static_object (&space_id_desc);
	acpi_cm_init_static_object (&function_desc);

	/*
	 *  Method requires two parameters.
	 */
	params [0] = &space_id_desc;
	params [1] = &function_desc;
	params [2] = NULL;

	/*
	 *  Set up the parameter objects
	 */
	space_id_desc.common.type  = ACPI_TYPE_INTEGER;
	space_id_desc.integer.value = region_obj->region.space_id;

	function_desc.common.type  = ACPI_TYPE_INTEGER;
	function_desc.integer.value = function;

	/*
	 *  Execute the method, no return value
	 */
	status = acpi_ns_evaluate_by_handle (region_obj->region.extra->extra.method_REG, params, NULL);
	return (status);
}


/**************************************************************************
 *
 * FUNCTION:    Acpi_ev_address_space_dispatch
 *
 * PARAMETERS:  Region_obj          - internal region object
 *              Space_id            - ID of the address space (0-255)
 *              Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              Bit_width           - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dispatch an address space or operation region access to
 *              a previously installed handler.
 *
 *************************************************************************/

ACPI_STATUS
acpi_ev_address_space_dispatch (
	ACPI_OPERAND_OBJECT     *region_obj,
	u32                     function,
	ACPI_PHYSICAL_ADDRESS   address,
	u32                     bit_width,
	u32                     *value)
{
	ACPI_STATUS             status;
	ADDRESS_SPACE_HANDLER   handler;
	ADDRESS_SPACE_SETUP     region_setup;
	ACPI_OPERAND_OBJECT     *handler_desc;
	void                    *region_context = NULL;


	/*
	 * Ensure that there is a handler associated with this region
	 */
	handler_desc = region_obj->region.addr_handler;
	if (!handler_desc) {
		return(AE_NOT_EXIST);
	}

	/*
	 * It may be the case that the region has never been initialized
	 * Some types of regions require special init code
	 */
	if (!(region_obj->region.flags & AOPOBJ_INITIALIZED)) {
		/*
		 * This region has not been initialized yet, do it
		 */
		region_setup = handler_desc->addr_handler.setup;
		if (!region_setup) {
			/*
			 *  Bad news, no init routine and not init'd
			 */
			return (AE_UNKNOWN_STATUS);
		}

		/*
		 * We must exit the interpreter because the region setup will potentially
		 * execute control methods
		 */
		acpi_aml_exit_interpreter ();

		status = region_setup (region_obj, ACPI_REGION_ACTIVATE,
				  handler_desc->addr_handler.context,
				  &region_context);

		/* Re-enter the interpreter */

		acpi_aml_enter_interpreter ();

		/*
		 *  Init routine may fail
		 */
		if (ACPI_FAILURE (status)) {
			return(status);
		}

		region_obj->region.flags |= AOPOBJ_INITIALIZED;

		/*
		 *  Save the returned context for use in all accesses to
		 *  this particular region.
		 */
		region_obj->region.extra->extra.region_context = region_context;
	}

	/*
	 *  We have everything we need, begin the process
	 */
	handler = handler_desc->addr_handler.handler;

	if (!(handler_desc->addr_handler.flags & ADDR_HANDLER_DEFAULT_INSTALLED)) {
		/*
		 *  For handlers other than the default (supplied) handlers, we must
		 *  exit the interpreter because the handler *might* block -- we don't
		 *  know what it will do, so we can't hold the lock on the intepreter.
		 */
		acpi_aml_exit_interpreter();
	}

	/*
	 *  Invoke the handler.
	 */
	status = handler (function, address, bit_width, value,
			 handler_desc->addr_handler.context,
			 region_obj->region.extra->extra.region_context);


	if (!(handler_desc->addr_handler.flags & ADDR_HANDLER_DEFAULT_INSTALLED)) {
		/* We just returned from a non-default handler, we must re-enter the
		interpreter */

		acpi_aml_enter_interpreter ();
	}

	return (status);
}

/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_disassociate_region_from_handler
 *
 * PARAMETERS:  Region_obj      - Region Object
 *              Acpi_ns_is_locked - Namespace Region Already Locked?
 *
 * RETURN:      None
 *
 * DESCRIPTION: Break the association between the handler and the region
 *              this is a two way association.
 *
 ******************************************************************************/

void
acpi_ev_disassociate_region_from_handler(
	ACPI_OPERAND_OBJECT     *region_obj,
	u8                      acpi_ns_is_locked)
{
	ACPI_OPERAND_OBJECT     *handler_obj;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     **last_obj_ptr;
	ADDRESS_SPACE_SETUP     region_setup;
	void                    *region_context;
	ACPI_STATUS             status;


	region_context = region_obj->region.extra->extra.region_context;

	/*
	 *  Get the address handler from the region object
	 */

	handler_obj = region_obj->region.addr_handler;
	if (!handler_obj) {
		/*
		 *  This region has no handler, all done
		 */
		return;
	}


	/*
	 *  Find this region in the handler's list
	 */

	obj_desc = handler_obj->addr_handler.region_list;
	last_obj_ptr = &handler_obj->addr_handler.region_list;

	while (obj_desc) {
		/*
		 *  See if this is the one
		 */
		if (obj_desc == region_obj) {
			/*
			 *  This is it, remove it from the handler's list
			 */
			*last_obj_ptr = obj_desc->region.next;
			obj_desc->region.next = NULL;           /* Must clear field */

			if (acpi_ns_is_locked) {
				acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
			}

			/*
			 *  Now stop region accesses by executing the _REG method
			 */
			acpi_ev_execute_reg_method (region_obj, 0);

			if (acpi_ns_is_locked) {
				acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);
			}

			/*
			 *  Call the setup handler with the deactivate notification
			 */
			region_setup = handler_obj->addr_handler.setup;
			status = region_setup (region_obj, ACPI_REGION_DEACTIVATE,
					  handler_obj->addr_handler.context,
					  &region_context);

			/*
			 *  Init routine may fail, Just ignore errors
			 */

			region_obj->region.flags &= ~(AOPOBJ_INITIALIZED);

			/*
			 *  Remove handler reference in the region
			 *
			 *  NOTE: this doesn't mean that the region goes away
			 *  The region is just inaccessible as indicated to
			 *  the _REG method
			 *
			 *  If the region is on the handler's list
			 *  this better be the region's handler
			 */
			ACPI_ASSERT (region_obj->region.addr_handler == handler_obj);

			region_obj->region.addr_handler = NULL;

			return;

		} /* found the right handler */

		/*
		 *  Move through the linked list of handlers
		 */
		last_obj_ptr = &obj_desc->region.next;
		obj_desc = obj_desc->region.next;
	}

	/*
	 *  If we get here, the region was not in the handler's region list
	 */
	return;
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_associate_region_and_handler
 *
 * PARAMETERS:  Handler_obj     - Handler Object
 *              Region_obj      - Region Object
 *              Acpi_ns_is_locked - Namespace Region Already Locked?
 *
 * RETURN:      None
 *
 * DESCRIPTION: Create the association between the handler and the region
 *              this is a two way association.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ev_associate_region_and_handler (
	ACPI_OPERAND_OBJECT     *handler_obj,
	ACPI_OPERAND_OBJECT     *region_obj,
	u8                      acpi_ns_is_locked)
{
	ACPI_STATUS     status;


	ACPI_ASSERT (region_obj->region.space_id == handler_obj->addr_handler.space_id);
	ACPI_ASSERT (region_obj->region.addr_handler == 0);

	/*
	 *  Link this region to the front of the handler's list
	 */

	region_obj->region.next = handler_obj->addr_handler.region_list;
	handler_obj->addr_handler.region_list = region_obj;

	/*
	 *  set the region's handler
	 */

/*
	Handler_obj->Common.Reference_count =
			  (u16) (Handler_obj->Common.Reference_count +
			  Region_obj->Common.Reference_count - 1);
*/
	region_obj->region.addr_handler = handler_obj;

	/*
	 *  Last thing, tell all users that this region is usable
	 */
	if (acpi_ns_is_locked) {
		acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
	}

	status = acpi_ev_execute_reg_method (region_obj, 1);

	if (acpi_ns_is_locked) {
		acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);
	}

	return (status);
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_ev_addr_handler_helper
 *
 * PARAMETERS:  Handle              - Node to be dumped
 *              Level               - Nesting level of the handle
 *              Context             - Passed into Acpi_ns_walk_namespace
 *
 * DESCRIPTION: This routine checks to see if the object is a Region if it
 *              is then the address handler is installed in it.
 *
 *              If the Object is a Device, and the device has a handler of
 *              the same type then the search is terminated in that branch.
 *
 *              This is because the existing handler is closer in proximity
 *              to any more regions than the one we are trying to install.
 *
 ***************************************************************************/

ACPI_STATUS
acpi_ev_addr_handler_helper (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *context,
	void                    **return_value)
{
	ACPI_OPERAND_OBJECT     *handler_obj;
	ACPI_OPERAND_OBJECT     *tmp_obj;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_STATUS             status;


	handler_obj = (ACPI_OPERAND_OBJECT *) context;

	/* Parameter validation */

	if (!handler_obj) {
		return (AE_OK);
	}

	/* Convert and validate the device handle */

	node = acpi_ns_convert_handle_to_entry (obj_handle);
	if (!node) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 *  We only care about regions.and objects
	 *  that can have address handlers
	 */

	if ((node->type != ACPI_TYPE_DEVICE) &&
		(node->type != ACPI_TYPE_REGION) &&
		(node != acpi_gbl_root_node)) {
		return (AE_OK);
	}

	/* Check for an existing internal object */

	obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) node);
	if (!obj_desc) {
		/*
		 *  The object DNE, we don't care about it
		 */
		return (AE_OK);
	}

	/*
	 *  Devices are handled different than regions
	 */
	if (IS_THIS_OBJECT_TYPE (obj_desc, ACPI_TYPE_DEVICE)) {
		/*
		 *  See if this guy has any handlers
		 */
		tmp_obj = obj_desc->device.addr_handler;
		while (tmp_obj) {
			/*
			 *  Now let's see if it's for the same address space.
			 */
			if (tmp_obj->addr_handler.space_id == handler_obj->addr_handler.space_id) {
				/*
				 *  It's for the same address space
				 */
				/*
				 *  Since the object we found it on was a device, then it
				 *  means that someone has already installed a handler for
				 *  the branch of the namespace from this device on.  Just
				 *  bail out telling the walk routine to not traverse this
				 *  branch.  This preserves the scoping rule for handlers.
				 */
				return (AE_CTRL_DEPTH);
			}

			/*
			 *  Move through the linked list of handlers
			 */
			tmp_obj = tmp_obj->addr_handler.next;
		}

		/*
		 *  As long as the device didn't have a handler for this
		 *  space we don't care about it.  We just ignore it and
		 *  proceed.
		 */
		return (AE_OK);
	}

	/*
	 *  Only here if it was a region
	 */
	ACPI_ASSERT (obj_desc->common.type == ACPI_TYPE_REGION);

	if (obj_desc->region.space_id != handler_obj->addr_handler.space_id) {
		/*
		 *  This region is for a different address space
		 *  ignore it
		 */
		return (AE_OK);
	}

	/*
	 *  Now we have a region and it is for the handler's address
	 *  space type.
	 *
	 *  First disconnect region for any previous handler (if any)
	 */
	acpi_ev_disassociate_region_from_handler (obj_desc, FALSE);

	/*
	 *  Then connect the region to the new handler
	 */
	status = acpi_ev_associate_region_and_handler (handler_obj, obj_desc, FALSE);

	return (status);
}


