/******************************************************************************
 *
 * Module Name: nsinit - namespace initialization
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
#include "acnamesp.h"
#include "acdispat.h"

#define _COMPONENT          ACPI_NAMESPACE
	 MODULE_NAME         ("nsinit")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_initialize_objects
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk the entire namespace and perform any necessary
 *              initialization on the objects found therein
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_initialize_objects (
	void)
{
	ACPI_STATUS             status;
	ACPI_INIT_WALK_INFO     info;


	info.field_count = 0;
	info.field_init = 0;
	info.op_region_count = 0;
	info.op_region_init = 0;
	info.object_count = 0;


	/* Walk entire namespace from the supplied root */

	status = acpi_walk_namespace (ACPI_TYPE_ANY, ACPI_ROOT_OBJECT,
			  ACPI_UINT32_MAX, acpi_ns_init_one_object,
			  &info, NULL);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_initialize_devices
 *
 * PARAMETERS:  None
 *
 * RETURN:      ACPI_STATUS
 *
 * DESCRIPTION: Walk the entire namespace and initialize all ACPI devices.
 *              This means running _INI on all present devices.
 *
 *              Note: We install PCI config space handler on region access,
 *              not here.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_initialize_devices (
	void)
{
	ACPI_STATUS             status;
	ACPI_DEVICE_WALK_INFO   info;


	info.device_count = 0;
	info.num_STA = 0;
	info.num_INI = 0;


	status = acpi_ns_walk_namespace (ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
			  ACPI_UINT32_MAX, FALSE, acpi_ns_init_one_device, &info, NULL);



	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_init_one_object
 *
 * PARAMETERS:  Obj_handle      - Node
 *              Level           - Current nesting level
 *              Context         - Points to a init info struct
 *              Return_value    - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Callback from Acpi_walk_namespace. Invoked for every object
 *              within the  namespace.
 *
 *              Currently, the only objects that require initialization are:
 *              1) Methods
 *              2) Op Regions
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_init_one_object (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *context,
	void                    **return_value)
{
	OBJECT_TYPE_INTERNAL    type;
	ACPI_STATUS             status;
	ACPI_INIT_WALK_INFO     *info = (ACPI_INIT_WALK_INFO *) context;
	ACPI_NAMESPACE_NODE     *node = (ACPI_NAMESPACE_NODE *) obj_handle;
	ACPI_OPERAND_OBJECT     *obj_desc;


	info->object_count++;


	/* And even then, we are only interested in a few object types */

	type = acpi_ns_get_type (obj_handle);
	obj_desc = node->object;
	if (!obj_desc) {
		return (AE_OK);
	}

	switch (type) {

	case ACPI_TYPE_REGION:

		info->op_region_count++;
		if (obj_desc->common.flags & AOPOBJ_DATA_VALID) {
			break;
		}

		info->op_region_init++;
		status = acpi_ds_get_region_arguments (obj_desc);


		break;


	case ACPI_TYPE_FIELD_UNIT:

		info->field_count++;
		if (obj_desc->common.flags & AOPOBJ_DATA_VALID) {
			break;
		}

		info->field_init++;
		status = acpi_ds_get_field_unit_arguments (obj_desc);


		break;

	default:
		break;
	}

	/*
	 * We ignore errors from above, and always return OK, since
	 * we don't want to abort the walk on a single error.
	 */
	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_init_one_device
 *
 * PARAMETERS:  WALK_CALLBACK
 *
 * RETURN:      ACPI_STATUS
 *
 * DESCRIPTION: This is called once per device soon after ACPI is enabled
 *              to initialize each device. It determines if the device is
 *              present, and if so, calls _INI.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_init_one_device (
	ACPI_HANDLE             obj_handle,
	u32                     nesting_level,
	void                    *context,
	void                    **return_value)
{
	ACPI_STATUS             status;
	ACPI_NAMESPACE_NODE    *node;
	u32                     flags;
	ACPI_DEVICE_WALK_INFO  *info = (ACPI_DEVICE_WALK_INFO *) context;



	info->device_count++;

	acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);

	node = acpi_ns_convert_handle_to_entry (obj_handle);
	if (!node) {
		acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
		return (AE_BAD_PARAMETER);
	}

	acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);

	/*
	 * Run _STA to determine if we can run _INI on the device.
	 */

	status = acpi_cm_execute_STA (node, &flags);
	if (ACPI_FAILURE (status)) {
		/* Ignore error and move on to next device */

		return (AE_OK);
	}

	info->num_STA++;

	if (!(flags & 0x01)) {
		/* don't look at children of a not present device */
		return(AE_CTRL_DEPTH);
	}


	/*
	 * The device is present. Run _INI.
	 */

	status = acpi_ns_evaluate_relative (obj_handle, "_INI", NULL, NULL);
	if (AE_NOT_FOUND == status) {
		/* No _INI means device requires no initialization */
		status = AE_OK;
	}

	else if (ACPI_FAILURE (status)) {
		/* Ignore error and move on to next device */

	}

	else {
		/* Count of successful INIs */

		info->num_INI++;
	}

	return (AE_OK);
}
