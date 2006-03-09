/******************************************************************************
 *
 * Module Name: evrgnini- ACPI Address_space (Op_region) init
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
	 MODULE_NAME         ("evrgnini")


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ev_system_memory_region_setup
 *
 * PARAMETERS:  Region_obj          - region we are interested in
 *              Function            - start or stop
 *              Handler_context     - Address space handler context
 *              Region_context      - Region specific context
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Do any prep work for region handling, a nop for now
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ev_system_memory_region_setup (
	ACPI_HANDLE             handle,
	u32                     function,
	void                    *handler_context,
	void                    **region_context)
{

	if (function == ACPI_REGION_DEACTIVATE) {
		if (*region_context) {
			acpi_cm_free (*region_context);
			*region_context = NULL;
		}
		return (AE_OK);
	}


	/* Activate.  Create a new context */

	*region_context = acpi_cm_callocate (sizeof (MEM_HANDLER_CONTEXT));
	if (!(*region_context)) {
		return (AE_NO_MEMORY);
	}

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ev_io_space_region_setup
 *
 * PARAMETERS:  Region_obj          - region we are interested in
 *              Function            - start or stop
 *              Handler_context     - Address space handler context
 *              Region_context      - Region specific context
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Do any prep work for region handling
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ev_io_space_region_setup (
	ACPI_HANDLE             handle,
	u32                     function,
	void                    *handler_context,
	void                    **region_context)
{
	if (function == ACPI_REGION_DEACTIVATE) {
		*region_context = NULL;
	}
	else {
		*region_context = handler_context;
	}

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ev_pci_config_region_setup
 *
 * PARAMETERS:  Region_obj          - region we are interested in
 *              Function            - start or stop
 *              Handler_context     - Address space handler context
 *              Region_context      - Region specific context
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Do any prep work for region handling
 *
 * MUTEX:       Assumes namespace is not locked
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ev_pci_config_region_setup (
	ACPI_HANDLE             handle,
	u32                     function,
	void                    *handler_context,
	void                    **region_context)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_INTEGER            temp;
	PCI_HANDLER_CONTEXT     *pci_context = *region_context;
	ACPI_OPERAND_OBJECT     *handler_obj;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_OPERAND_OBJECT     *region_obj = (ACPI_OPERAND_OBJECT *) handle;
	DEVICE_ID               object_hID;

	handler_obj = region_obj->region.addr_handler;

	if (!handler_obj) {
		/*
		 *  No installed handler. This shouldn't happen because the dispatch
		 *  routine checks before we get here, but we check again just in case.
		 */
		return(AE_NOT_EXIST);
	}

	if (function == ACPI_REGION_DEACTIVATE) {
		if (pci_context) {
			acpi_cm_free (pci_context);
			*region_context = NULL;
		}

		return (status);
	}


	/* Create a new context */

	pci_context = acpi_cm_callocate (sizeof(PCI_HANDLER_CONTEXT));
	if (!pci_context) {
		return (AE_NO_MEMORY);
	}

	/*
	 *  For PCI Config space access, we have to pass the segment, bus,
	 *  device and function numbers.  This routine must acquire those.
	 */

	/*
	 *  First get device and function numbers from the _ADR object
	 *  in the parent's scope.
	 */
	ACPI_ASSERT(region_obj->region.node);

	node = acpi_ns_get_parent_object (region_obj->region.node);


	/* Acpi_evaluate the _ADR object */

	status = acpi_cm_evaluate_numeric_object (METHOD_NAME__ADR, node, &temp);
	/*
	 *  The default is zero, since the allocation above zeroed the data, just
	 *  do nothing on failures.
	 */
	if (ACPI_SUCCESS (status)) {
		/*
		 *  Got it..
		 */
		pci_context->dev_func = (u32) temp;
	}

	/*
	 *  Get the _SEG and _BBN values from the device upon which the handler
	 *  is installed.
	 *
	 *  We need to get the _SEG and _BBN objects relative to the PCI BUS device.
	 *  This is the device the handler has been registered to handle.
	 */

	/*
	 *  If the Addr_handler.Node is still pointing to the root, we need
	 *  to scan upward for a PCI Root bridge and re-associate the Op_region
	 *  handlers with that device.
	 */
	if (handler_obj->addr_handler.node == acpi_gbl_root_node) {
		/*
		 * Node is currently the parent object
		 */
		while (node != acpi_gbl_root_node) {
			status = acpi_cm_execute_HID(node, &object_hID);

			if (ACPI_SUCCESS (status)) {
				if (!(STRNCMP(object_hID.buffer, PCI_ROOT_HID_STRING,
						   sizeof (PCI_ROOT_HID_STRING)))) {
					acpi_install_address_space_handler(node,
							   ADDRESS_SPACE_PCI_CONFIG,
							   ACPI_DEFAULT_HANDLER, NULL, NULL);

					break;
				}
			}

			node = acpi_ns_get_parent_object(node);
		}
	}
	else {
		node = handler_obj->addr_handler.node;
	}

	status = acpi_cm_evaluate_numeric_object (METHOD_NAME__SEG, node, &temp);
	if (ACPI_SUCCESS (status)) {
		/*
		 *  Got it..
		 */
		pci_context->seg = (u32) temp;
	}

	status = acpi_cm_evaluate_numeric_object (METHOD_NAME__BBN, node, &temp);
	if (ACPI_SUCCESS (status)) {
		/*
		 *  Got it..
		 */
		pci_context->bus = (u32) temp;
	}

	*region_context = pci_context;

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ev_default_region_setup
 *
 * PARAMETERS:  Region_obj          - region we are interested in
 *              Function            - start or stop
 *              Handler_context     - Address space handler context
 *              Region_context      - Region specific context
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Do any prep work for region handling
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ev_default_region_setup (
	ACPI_HANDLE             handle,
	u32                     function,
	void                    *handler_context,
	void                    **region_context)
{
	if (function == ACPI_REGION_DEACTIVATE) {
		*region_context = NULL;
	}
	else {
		*region_context = handler_context;
	}

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_initialize_region
 *
 * PARAMETERS:  Region_obj - Region we are initializing
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initializes the region, finds any _REG methods and saves them
 *              for execution at a later time
 *
 *              Get the appropriate address space handler for a newly
 *              created region.
 *
 *              This also performs address space specific intialization.  For
 *              example, PCI regions must have an _ADR object that contains
 *              a PCI address in the scope of the defintion.  This address is
 *              required to perform an access to PCI config space.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ev_initialize_region (
	ACPI_OPERAND_OBJECT     *region_obj,
	u8                      acpi_ns_locked)
{
	ACPI_OPERAND_OBJECT    *handler_obj;
	ACPI_OPERAND_OBJECT    *obj_desc;
	ACPI_ADDRESS_SPACE_TYPE space_id;
	ACPI_NAMESPACE_NODE    *node;
	ACPI_STATUS             status;
	ACPI_NAMESPACE_NODE    *method_node;
	ACPI_NAME              *reg_name_ptr = (ACPI_NAME *) METHOD_NAME__REG;


	if (!region_obj) {
		return (AE_BAD_PARAMETER);
	}

	ACPI_ASSERT(region_obj->region.node);

	node = acpi_ns_get_parent_object (region_obj->region.node);
	space_id = region_obj->region.space_id;

	region_obj->region.addr_handler = NULL;
	region_obj->region.extra->extra.method_REG = NULL;
	region_obj->region.flags &= ~(AOPOBJ_INITIALIZED);

	/*
	 *  Find any "_REG" associated with this region definition
	 */
	status = acpi_ns_search_node (*reg_name_ptr, node,
			  ACPI_TYPE_METHOD, &method_node);
	if (ACPI_SUCCESS (status)) {
		/*
		 *  The _REG method is optional and there can be only one per region
		 *  definition.  This will be executed when the handler is attached
		 *  or removed
		 */
		region_obj->region.extra->extra.method_REG = method_node;
	}

	/*
	 *  The following loop depends upon the root Node having no parent
	 *  ie: Acpi_gbl_Root_node->Parent_entry being set to NULL
	 */
	while (node) {
		/*
		 *  Check to see if a handler exists
		 */
		handler_obj = NULL;
		obj_desc = acpi_ns_get_attached_object ((ACPI_HANDLE) node);
		if (obj_desc) {
			/*
			 *  can only be a handler if the object exists
			 */
			switch (node->type) {
			case ACPI_TYPE_DEVICE:

				handler_obj = obj_desc->device.addr_handler;
				break;

			case ACPI_TYPE_PROCESSOR:

				handler_obj = obj_desc->processor.addr_handler;
				break;

			case ACPI_TYPE_THERMAL:

				handler_obj = obj_desc->thermal_zone.addr_handler;
				break;
			}

			while (handler_obj) {
				/*
				 *  This guy has at least one address handler
				 *  see if it has the type we want
				 */
				if (handler_obj->addr_handler.space_id == space_id) {
					/*
					 *  Found it! Now update the region and the handler
					 */
					acpi_ev_associate_region_and_handler (handler_obj, region_obj, acpi_ns_locked);
					return (AE_OK);
				}

				handler_obj = handler_obj->addr_handler.next;

			} /* while handlerobj */
		}

		/*
		 *  This one does not have the handler we need
		 *  Pop up one level
		 */
		node = acpi_ns_get_parent_object (node);

	} /* while Node != ROOT */

	/*
	 *  If we get here, there is no handler for this region
	 */
	return (AE_NOT_EXIST);
}

