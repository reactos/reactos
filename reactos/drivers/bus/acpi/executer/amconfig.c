/******************************************************************************
 *
 * Module Name: amconfig - Namespace reconfiguration (Load/Unload opcodes)
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
#include "actables.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amconfig")


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_load_table
 *
 * PARAMETERS:  Rgn_desc        - Op region where the table will be obtained
 *              Ddb_handle      - Where a handle to the table will be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load an ACPI table
 *
 ****************************************************************************/

static ACPI_STATUS
acpi_aml_exec_load_table (
	ACPI_OPERAND_OBJECT     *rgn_desc,
	ACPI_HANDLE             *ddb_handle)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *table_desc = NULL;
	u8                      *table_ptr;
	u8                      *table_data_ptr;
	ACPI_TABLE_HEADER       table_header;
	ACPI_TABLE_DESC         table_info;
	u32                     i;


	/* TBD: [Unhandled] Object can be either a field or an opregion */


	/* Get the table header */

	table_header.length = 0;
	for (i = 0; i < sizeof (ACPI_TABLE_HEADER); i++) {
		status = acpi_ev_address_space_dispatch (rgn_desc, ADDRESS_SPACE_READ,
				   (ACPI_PHYSICAL_ADDRESS) i, 8,
				   (u32 *) ((u8 *) &table_header + i));
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/* Allocate a buffer for the entire table */

	table_ptr = acpi_cm_allocate (table_header.length);
	if (!table_ptr) {
		return (AE_NO_MEMORY);
	}

	/* Copy the header to the buffer */

	MEMCPY (table_ptr, &table_header, sizeof (ACPI_TABLE_HEADER));
	table_data_ptr = table_ptr + sizeof (ACPI_TABLE_HEADER);


	/* Get the table from the op region */

	for (i = 0; i < table_header.length; i++) {
		status = acpi_ev_address_space_dispatch (rgn_desc, ADDRESS_SPACE_READ,
				   (ACPI_PHYSICAL_ADDRESS)i, 8,
				   (u32 *) (table_data_ptr + i));
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}
	}


	/* Table must be either an SSDT or a PSDT */

	if ((!STRNCMP (table_header.signature,
			  acpi_gbl_acpi_table_data[ACPI_TABLE_PSDT].signature,
			  acpi_gbl_acpi_table_data[ACPI_TABLE_PSDT].sig_length)) &&
		(!STRNCMP (table_header.signature,
				 acpi_gbl_acpi_table_data[ACPI_TABLE_SSDT].signature,
				 acpi_gbl_acpi_table_data[ACPI_TABLE_SSDT].sig_length))) {
		status = AE_BAD_SIGNATURE;
		goto cleanup;
	}

	/* Create an object to be the table handle */

	table_desc = acpi_cm_create_internal_object (INTERNAL_TYPE_REFERENCE);
	if (!table_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}


	/* Install the new table into the local data structures */

	table_info.pointer     = (ACPI_TABLE_HEADER *) table_ptr;
	table_info.length      = table_header.length;
	table_info.allocation  = ACPI_MEM_ALLOCATED;
	table_info.base_pointer = table_ptr;

	status = acpi_tb_install_table (NULL, &table_info);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	/* Add the table to the namespace */

	/* TBD: [Restructure] - change to whatever new interface is appropriate */
/*
	Status = Acpi_load_namespace ();
	if (ACPI_FAILURE (Status))
	{
*/
		/* TBD: [Errors] Unload the table on failure ? */
/*
		goto Cleanup;
	}
*/


	/* TBD: [Investigate] we need a pointer to the table desc */

	/* Init the table handle */

	table_desc->reference.opcode = AML_LOAD_OP;
	table_desc->reference.object = table_info.installed_desc;

	*ddb_handle = table_desc;

	return (status);


cleanup:

	acpi_cm_free (table_desc);
	acpi_cm_free (table_ptr);
	return (status);

}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_unload_table
 *
 * PARAMETERS:  Ddb_handle          - Handle to a previously loaded table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Unload an ACPI table
 *
 ****************************************************************************/

static ACPI_STATUS
acpi_aml_exec_unload_table (
	ACPI_HANDLE             ddb_handle)
{
	ACPI_STATUS             status = AE_NOT_IMPLEMENTED;
	ACPI_OPERAND_OBJECT     *table_desc = (ACPI_OPERAND_OBJECT *) ddb_handle;
	ACPI_TABLE_DESC         *table_info;


	/* Validate the handle */
	/* Although the handle is partially validated in Acpi_aml_exec_reconfiguration(),
	 *  when it calls Acpi_aml_resolve_operands(), the handle is more completely
	 *  validated here.
	 */

	if ((!ddb_handle) ||
		(!VALID_DESCRIPTOR_TYPE (ddb_handle, ACPI_DESC_TYPE_INTERNAL)) ||
		(((ACPI_OPERAND_OBJECT  *)ddb_handle)->common.type !=
				INTERNAL_TYPE_REFERENCE)) {
		return (AE_BAD_PARAMETER);
	}


	/* Get the actual table descriptor from the Ddb_handle */

	table_info = (ACPI_TABLE_DESC *) table_desc->reference.object;

	/*
	 * Delete the entire namespace under this table Node
	 * (Offset contains the Table_id)
	 */

	status = acpi_ns_delete_namespace_by_owner (table_info->table_id);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Delete the table itself */

	acpi_tb_uninstall_table (table_info->installed_desc);

	/* Delete the table descriptor (Ddb_handle) */

	acpi_cm_remove_reference (table_desc);

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_reconfiguration
 *
 * PARAMETERS:  Opcode              - The opcode to be executed
 *              Walk_state          - Current state of the parse tree walk
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reconfiguration opcodes such as LOAD and UNLOAD
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_reconfiguration (
	u16                     opcode,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *region_desc = NULL;
	ACPI_HANDLE             *ddb_handle;


	/* Resolve the operands */

	status = acpi_aml_resolve_operands (opcode, WALK_OPERANDS, walk_state);
	/* Get the table handle, common for both opcodes */

	status |= acpi_ds_obj_stack_pop_object ((ACPI_OPERAND_OBJECT **) &ddb_handle,
			 walk_state);

	switch (opcode) {

	case AML_LOAD_OP:

		/* Get the region or field descriptor */

		status |= acpi_ds_obj_stack_pop_object (&region_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			acpi_cm_remove_reference (region_desc);
			return (status);
		}

		status = acpi_aml_exec_load_table (region_desc, ddb_handle);
		break;


	case AML_UNLOAD_OP:

		if (ACPI_FAILURE (status)) {
			return (status);
		}

		status = acpi_aml_exec_unload_table (ddb_handle);
		break;


	default:

		status = AE_AML_BAD_OPCODE;
		break;
	}


	return (status);
}

