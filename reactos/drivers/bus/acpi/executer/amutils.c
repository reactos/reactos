
/******************************************************************************
 *
 * Module Name: amutils - interpreter/scanner utilities
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
#include "acparser.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"
#include "acevents.h"

#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amutils")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_enter_interpreter
 *
 * PARAMETERS:  None
 *
 * DESCRIPTION: Enter the interpreter execution region
 *              TBD: should be a macro
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_enter_interpreter (void)
{
	ACPI_STATUS             status;


	status = acpi_cm_acquire_mutex (ACPI_MTX_EXECUTE);
	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exit_interpreter
 *
 * PARAMETERS:  None
 *
 * DESCRIPTION: Exit the interpreter execution region
 *
 * Cases where the interpreter is unlocked:
 *      1) Completion of the execution of a control method
 *      2) Method blocked on a Sleep() AML opcode
 *      3) Method blocked on an Acquire() AML opcode
 *      4) Method blocked on a Wait() AML opcode
 *      5) Method blocked to acquire the global lock
 *      6) Method blocked to execute a serialized control method that is
 *          already executing
 *      7) About to invoke a user-installed opregion handler
 *
 *              TBD: should be a macro
 *
 ******************************************************************************/

void
acpi_aml_exit_interpreter (void)
{

	acpi_cm_release_mutex (ACPI_MTX_EXECUTE);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_validate_object_type
 *
 * PARAMETERS:  Type            Object type to validate
 *
 * DESCRIPTION: Determine if a type is a valid ACPI object type
 *
 ******************************************************************************/

u8
acpi_aml_validate_object_type (
	ACPI_OBJECT_TYPE        type)
{

	if ((type > ACPI_TYPE_MAX && type < INTERNAL_TYPE_BEGIN) ||
		(type > INTERNAL_TYPE_MAX)) {
		return (FALSE);
	}

	return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_truncate_for32bit_table
 *
 * PARAMETERS:  Obj_desc        - Object to be truncated
 *              Walk_state      - Current walk state
 *                                (A method must be executing)
 *
 * RETURN:      none
 *
 * DESCRIPTION: Truncate a number to 32-bits if the currently executing method
 *              belongs to a 32-bit ACPI table.
 *
 ******************************************************************************/

void
acpi_aml_truncate_for32bit_table (
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_WALK_STATE         *walk_state)
{

	/*
	 * Object must be a valid number and we must be executing
	 * a control method
	 */

	if ((!obj_desc) ||
		(obj_desc->common.type != ACPI_TYPE_INTEGER) ||
		(!walk_state->method_node)) {
		return;
	}

	if (walk_state->method_node->flags & ANOBJ_DATA_WIDTH_32) {
		/*
		 * We are running a method that exists in a 32-bit ACPI table.
		 * Truncate the value to 32 bits by zeroing out the upper 32-bit field
		 */
		obj_desc->integer.value &= (ACPI_INTEGER) ACPI_UINT32_MAX;
	}
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_acquire_global_lock
 *
 * PARAMETERS:  Rule            - Lock rule: Always_lock, Never_lock
 *
 * RETURN:      TRUE/FALSE indicating whether the lock was actually acquired
 *
 * DESCRIPTION: Obtain the global lock and keep track of this fact via two
 *              methods.  A global variable keeps the state of the lock, and
 *              the state is returned to the caller.
 *
 ******************************************************************************/

u8
acpi_aml_acquire_global_lock (
	u32                     rule)
{
	u8                      locked = FALSE;
	ACPI_STATUS             status;


	/* Only attempt lock if the Rule says so */

	if (rule == (u32) GLOCK_ALWAYS_LOCK) {
		/* We should attempt to get the lock */

		status = acpi_ev_acquire_global_lock ();
		if (ACPI_SUCCESS (status)) {
			locked = TRUE;
		}

	}

	return (locked);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_release_global_lock
 *
 * PARAMETERS:  Locked_by_me    - Return value from corresponding call to
 *                                Acquire_global_lock.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release the global lock if it is locked.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_release_global_lock (
	u8                      locked_by_me)
{


	/* Only attempt unlock if the caller locked it */

	if (locked_by_me) {
		/* OK, now release the lock */

		acpi_ev_release_global_lock ();
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_digits_needed
 *
 * PARAMETERS:  val             - Value to be represented
 *              base            - Base of representation
 *
 * RETURN:      the number of digits needed to represent val in base
 *
 ******************************************************************************/

u32
acpi_aml_digits_needed (
	ACPI_INTEGER            val,
	u32                     base)
{
	u32                     num_digits = 0;


	if (base < 1) {
		REPORT_ERROR (("Aml_digits_needed: Internal error - Invalid base\n"));
	}

	else {
		for (num_digits = 1 + (val < 0); (val = ACPI_DIVIDE (val,base)); ++num_digits) { ; }
	}

	return (num_digits);
}


/*******************************************************************************
 *
 * FUNCTION:    ntohl
 *
 * PARAMETERS:  Value           - Value to be converted
 *
 * DESCRIPTION: Convert a 32-bit value to big-endian (swap the bytes)
 *
 ******************************************************************************/

static u32
_ntohl (
	u32                     value)
{
	union {
		u32                 value;
		u8                  bytes[4];
	} out;

	union {
		u32                 value;
		u8                  bytes[4];
	} in;


	in.value = value;

	out.bytes[0] = in.bytes[3];
	out.bytes[1] = in.bytes[2];
	out.bytes[2] = in.bytes[1];
	out.bytes[3] = in.bytes[0];

	return (out.value);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_eisa_id_to_string
 *
 * PARAMETERS:  Numeric_id      - EISA ID to be converted
 *              Out_string      - Where to put the converted string (8 bytes)
 *
 * DESCRIPTION: Convert a numeric EISA ID to string representation
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_eisa_id_to_string (
	u32                     numeric_id,
	NATIVE_CHAR             *out_string)
{
	u32                     id;

	/* swap to big-endian to get contiguous bits */

	id = _ntohl (numeric_id);

	out_string[0] = (char) ('@' + ((id >> 26) & 0x1f));
	out_string[1] = (char) ('@' + ((id >> 21) & 0x1f));
	out_string[2] = (char) ('@' + ((id >> 16) & 0x1f));
	out_string[3] = acpi_gbl_hex_to_ascii[(id >> 12) & 0xf];
	out_string[4] = acpi_gbl_hex_to_ascii[(id >> 8) & 0xf];
	out_string[5] = acpi_gbl_hex_to_ascii[(id >> 4) & 0xf];
	out_string[6] = acpi_gbl_hex_to_ascii[id & 0xf];
	out_string[7] = 0;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_unsigned_integer_to_string
 *
 * PARAMETERS:  Value           - Value to be converted
 *              Out_string      - Where to put the converted string (8 bytes)
 *
 * RETURN:      Convert a number to string representation
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_unsigned_integer_to_string (
	ACPI_INTEGER            value,
	NATIVE_CHAR             *out_string)
{
	u32                     count;
	u32                     digits_needed;


	digits_needed = acpi_aml_digits_needed (value, 10);

	out_string[digits_needed] = '\0';

	for (count = digits_needed; count > 0; count--) {
		out_string[count-1] = (NATIVE_CHAR) ('0' + (ACPI_MODULO (value, 10)));
		value = ACPI_DIVIDE (value, 10);
	}

	return (AE_OK);
}


