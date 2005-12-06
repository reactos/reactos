/******************************************************************************
 *
 * Module Name: cmeval - Object evaluation
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
#include "acinterp.h"


#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmeval")


/****************************************************************************
 *
 * FUNCTION:    Acpi_cm_evaluate_numeric_object
 *
 * PARAMETERS:  *Object_name        - Object name to be evaluated
 *              Device_node         - Node for the device
 *              *Address            - Where the value is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: evaluates a numeric namespace object for a selected device
 *              and stores results in *Address.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ***************************************************************************/

ACPI_STATUS
acpi_cm_evaluate_numeric_object (
	NATIVE_CHAR             *object_name,
	ACPI_NAMESPACE_NODE     *device_node,
	ACPI_INTEGER            *address)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Execute the method */

	status = acpi_ns_evaluate_relative (device_node, object_name, NULL, &obj_desc);
	if (ACPI_FAILURE (status)) {

		return (status);
	}


	/* Did we get a return object? */

	if (!obj_desc) {
		return (AE_TYPE);
	}

	/* Is the return object of the correct type? */

	if (obj_desc->common.type != ACPI_TYPE_INTEGER) {
		status = AE_TYPE;
	}
	else {
		/*
		 * Since the structure is a union, setting any field will set all
		 * of the variables in the union
		 */
		*address = obj_desc->integer.value;
	}

	/* On exit, we must delete the return object */

	acpi_cm_remove_reference (obj_desc);

	return (status);
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_cm_execute_HID
 *
 * PARAMETERS:  Device_node         - Node for the device
 *              *Hid                - Where the HID is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes the _HID control method that returns the hardware
 *              ID of the device.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ***************************************************************************/

ACPI_STATUS
acpi_cm_execute_HID (
	ACPI_NAMESPACE_NODE     *device_node,
	DEVICE_ID               *hid)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Execute the method */

	status = acpi_ns_evaluate_relative (device_node,
			 METHOD_NAME__HID, NULL, &obj_desc);
	if (ACPI_FAILURE (status)) {


		return (status);
	}

	/* Did we get a return object? */

	if (!obj_desc) {
		return (AE_TYPE);
	}

	/*
	 *  A _HID can return either a Number (32 bit compressed EISA ID) or
	 *  a string
	 */

	if ((obj_desc->common.type != ACPI_TYPE_INTEGER) &&
		(obj_desc->common.type != ACPI_TYPE_STRING)) {
		status = AE_TYPE;
	}

	else {
		if (obj_desc->common.type == ACPI_TYPE_INTEGER) {
			/* Convert the Numeric HID to string */

			acpi_aml_eisa_id_to_string ((u32) obj_desc->integer.value, hid->buffer);
		}

		else {
			/* Copy the String HID from the returned object */

			STRNCPY(hid->buffer, obj_desc->string.pointer, sizeof(hid->buffer));
		}
	}


	/* On exit, we must delete the return object */

	acpi_cm_remove_reference (obj_desc);

	return (status);
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_cm_execute_UID
 *
 * PARAMETERS:  Device_node         - Node for the device
 *              *Uid                - Where the UID is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes the _UID control method that returns the hardware
 *              ID of the device.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ***************************************************************************/

ACPI_STATUS
acpi_cm_execute_UID (
	ACPI_NAMESPACE_NODE     *device_node,
	DEVICE_ID               *uid)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Execute the method */

	status = acpi_ns_evaluate_relative (device_node,
			 METHOD_NAME__UID, NULL, &obj_desc);
	if (ACPI_FAILURE (status)) {


		return (status);
	}

	/* Did we get a return object? */

	if (!obj_desc) {
		return (AE_TYPE);
	}

	/*
	 *  A _UID can return either a Number (32 bit compressed EISA ID) or
	 *  a string
	 */

	if ((obj_desc->common.type != ACPI_TYPE_INTEGER) &&
		(obj_desc->common.type != ACPI_TYPE_STRING)) {
		status = AE_TYPE;
	}

	else {
		if (obj_desc->common.type == ACPI_TYPE_INTEGER) {
			/* Convert the Numeric UID to string */

			acpi_aml_unsigned_integer_to_string (obj_desc->integer.value, uid->buffer);
		}

		else {
			/* Copy the String UID from the returned object */

			STRNCPY(uid->buffer, obj_desc->string.pointer, sizeof(uid->buffer));
		}
	}


	/* On exit, we must delete the return object */

	acpi_cm_remove_reference (obj_desc);

	return (status);
}

/****************************************************************************
 *
 * FUNCTION:    Acpi_cm_execute_STA
 *
 * PARAMETERS:  Device_node         - Node for the device
 *              *Flags              - Where the status flags are returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Executes _STA for selected device and stores results in
 *              *Flags.
 *
 *              NOTE: Internal function, no parameter validation
 *
 ***************************************************************************/

ACPI_STATUS
acpi_cm_execute_STA (
	ACPI_NAMESPACE_NODE     *device_node,
	u32                     *flags)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Execute the method */

	status = acpi_ns_evaluate_relative (device_node,
			 METHOD_NAME__STA, NULL, &obj_desc);
	if (AE_NOT_FOUND == status) {
		*flags = 0x0F;
		status = AE_OK;
	}


	else /* success */ {
		/* Did we get a return object? */

		if (!obj_desc) {
			return (AE_TYPE);
		}

		/* Is the return object of the correct type? */

		if (obj_desc->common.type != ACPI_TYPE_INTEGER) {
			status = AE_TYPE;
		}

		else {
			/* Extract the status flags */

			*flags = (u32) obj_desc->integer.value;
		}

		/* On exit, we must delete the return object */

		acpi_cm_remove_reference (obj_desc);
	}

	return (status);
}
