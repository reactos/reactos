/******************************************************************************
 *
 * Module Name: nsload - namespace loading/expanding/contracting procedures
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
#include "amlcode.h"
#include "acparser.h"
#include "acdispat.h"
#include "acdebug.h"


#define _COMPONENT          ACPI_NAMESPACE
	 MODULE_NAME         ("nsload")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_load_namespace
 *
 * PARAMETERS:  Display_aml_during_load
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load the name space from what ever is pointed to by DSDT.
 *              (DSDT points to either the BIOS or a buffer.)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_load_namespace (
	void)
{
	ACPI_STATUS             status;


	/* There must be at least a DSDT installed */

	if (acpi_gbl_DSDT == NULL) {
		return (AE_NO_ACPI_TABLES);
	}


	/*
	 * Load the namespace.  The DSDT is required,
	 * but the SSDT and PSDT tables are optional.
	 */

	status = acpi_ns_load_table_by_type (ACPI_TABLE_DSDT);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Ignore exceptions from these */

	acpi_ns_load_table_by_type (ACPI_TABLE_SSDT);
	acpi_ns_load_table_by_type (ACPI_TABLE_PSDT);


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_one_parse_pass
 *
 * PARAMETERS:
 *
 * RETURN:      Status
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_one_complete_parse (
	u32                     pass_number,
	ACPI_TABLE_DESC         *table_desc)
{
	ACPI_PARSE_DOWNWARDS    descending_callback;
	ACPI_PARSE_UPWARDS      ascending_callback;
	ACPI_PARSE_OBJECT       *parse_root;
	ACPI_STATUS             status;


	switch (pass_number) {
	case 1:
		descending_callback = acpi_ds_load1_begin_op;
		ascending_callback = acpi_ds_load1_end_op;
		break;

	case 2:
		descending_callback = acpi_ds_load2_begin_op;
		ascending_callback = acpi_ds_load2_end_op;
		break;

	case 3:
		descending_callback = acpi_ds_exec_begin_op;
		ascending_callback = acpi_ds_exec_end_op;
		break;

	default:
		return (AE_BAD_PARAMETER);
	}

	/* Create and init a Root Node */

	parse_root = acpi_ps_alloc_op (AML_SCOPE_OP);
	if (!parse_root) {
		return (AE_NO_MEMORY);
	}

	((ACPI_PARSE2_OBJECT *) parse_root)->name = ACPI_ROOT_NAME;


	/* Pass 1:  Parse everything except control method bodies */

	status = acpi_ps_parse_aml (parse_root, table_desc->aml_pointer,
			 table_desc->aml_length,
			 ACPI_PARSE_LOAD_PASS1 | ACPI_PARSE_DELETE_TREE,
			 NULL, NULL, NULL, descending_callback,
			 ascending_callback);

	acpi_ps_delete_parse_tree (parse_root);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_parse_table
 *
 * PARAMETERS:  Table_desc      - An ACPI table descriptor for table to parse
 *              Start_node      - Where to enter the table into the namespace
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse AML within an ACPI table and return a tree of ops
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_parse_table (
	ACPI_TABLE_DESC         *table_desc,
	ACPI_NAMESPACE_NODE     *start_node)
{
	ACPI_STATUS             status;


	/*
	 * AML Parse, pass 1
	 *
	 * In this pass, we load most of the namespace.  Control methods
	 * are not parsed until later.  A parse tree is not created.  Instead,
	 * each Parser Op subtree is deleted when it is finished.  This saves
	 * a great deal of memory, and allows a small cache of parse objects
	 * to service the entire parse.  The second pass of the parse then
	 * performs another complete parse of the AML..
	 */

	status = acpi_ns_one_complete_parse (1, table_desc);
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/*
	 * AML Parse, pass 2
	 *
	 * In this pass, we resolve forward references and other things
	 * that could not be completed during the first pass.
	 * Another complete parse of the AML is performed, but the
	 * overhead of this is compensated for by the fact that the
	 * parse objects are all cached.
	 */

	status = acpi_ns_one_complete_parse (2, table_desc);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_load_table
 *
 * PARAMETERS:  *Pcode_addr         - Address of pcode block
 *              Pcode_length        - Length of pcode block
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load one ACPI table into the namespace
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_load_table (
	ACPI_TABLE_DESC         *table_desc,
	ACPI_NAMESPACE_NODE     *node)
{
	ACPI_STATUS             status;


	if (!table_desc->aml_pointer) {
		return (AE_BAD_PARAMETER);
	}


	if (!table_desc->aml_length) {
		return (AE_BAD_PARAMETER);
	}


	/*
	 * Parse the table and load the namespace with all named
	 * objects found within.  Control methods are NOT parsed
	 * at this time.  In fact, the control methods cannot be
	 * parsed until the entire namespace is loaded, because
	 * if a control method makes a forward reference (call)
	 * to another control method, we can't continue parsing
	 * because we don't know how many arguments to parse next!
	 */

	acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);
	status = acpi_ns_parse_table (table_desc, node->child);
	acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);

	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/*
	 * Now we can parse the control methods.  We always parse
	 * them here for a sanity check, and if configured for
	 * just-in-time parsing, we delete the control method
	 * parse trees.
	 */

	status = acpi_ds_initialize_objects (table_desc, node);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_load_table_by_type
 *
 * PARAMETERS:  Table_type          - Id of the table type to load
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load an ACPI table or tables into the namespace.  All tables
 *              of the given type are loaded.  The mechanism allows this
 *              routine to be called repeatedly.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_load_table_by_type (
	ACPI_TABLE_TYPE         table_type)
{
	u32                     i;
	ACPI_STATUS             status = AE_OK;
	ACPI_TABLE_HEADER       *table_ptr;
	ACPI_TABLE_DESC         *table_desc;


	acpi_cm_acquire_mutex (ACPI_MTX_TABLES);


	/*
	 * Table types supported are:
	 * DSDT (one), SSDT/PSDT (multiple)
	 */

	switch (table_type) {

	case ACPI_TABLE_DSDT:

		table_desc = &acpi_gbl_acpi_tables[ACPI_TABLE_DSDT];

		/* If table already loaded into namespace, just return */

		if (table_desc->loaded_into_namespace) {
			goto unlock_and_exit;
		}

		table_desc->table_id = TABLE_ID_DSDT;

		/* Now load the single DSDT */

		status = acpi_ns_load_table (table_desc, acpi_gbl_root_node);
		if (ACPI_SUCCESS (status)) {
			table_desc->loaded_into_namespace = TRUE;
		}

		break;


	case ACPI_TABLE_SSDT:

		/*
		 * Traverse list of SSDT tables
		 */

		table_desc = &acpi_gbl_acpi_tables[ACPI_TABLE_SSDT];
		for (i = 0; i < acpi_gbl_acpi_tables[ACPI_TABLE_SSDT].count; i++) {
			table_ptr = table_desc->pointer;

			/*
			 * Only attempt to load table if it is not
			 * already loaded!
			 */

			if (!table_desc->loaded_into_namespace) {
				status = acpi_ns_load_table (table_desc, acpi_gbl_root_node);
				if (ACPI_FAILURE (status)) {
					break;
				}

				table_desc->loaded_into_namespace = TRUE;
			}

			table_desc = table_desc->next;
		}
		break;


	case ACPI_TABLE_PSDT:

		/*
		 * Traverse list of PSDT tables
		 */

		table_desc = &acpi_gbl_acpi_tables[ACPI_TABLE_PSDT];

		for (i = 0; i < acpi_gbl_acpi_tables[ACPI_TABLE_PSDT].count; i++) {
			table_ptr = table_desc->pointer;

			/* Only attempt to load table if it is not already loaded! */

			if (!table_desc->loaded_into_namespace) {
				status = acpi_ns_load_table (table_desc, acpi_gbl_root_node);
				if (ACPI_FAILURE (status)) {
					break;
				}

				table_desc->loaded_into_namespace = TRUE;
			}

			table_desc = table_desc->next;
		}

		break;


	default:
		status = AE_SUPPORT;
		break;
	}


unlock_and_exit:

	acpi_cm_release_mutex (ACPI_MTX_TABLES);

	return (status);

}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_delete_subtree
 *
 * PARAMETERS:  Start_handle        - Handle in namespace where search begins
 *
 * RETURNS      Status
 *
 * DESCRIPTION: Walks the namespace starting at the given handle and deletes
 *              all objects, entries, and scopes in the entire subtree.
 *
 *              TBD: [Investigate] What if any part of this subtree is in use?
 *              (i.e. on one of the object stacks?)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_delete_subtree (
	ACPI_HANDLE             start_handle)
{
	ACPI_STATUS             status;
	ACPI_HANDLE             child_handle;
	ACPI_HANDLE             parent_handle;
	ACPI_HANDLE             next_child_handle;
	ACPI_HANDLE             dummy;
	u32                     level;


	parent_handle = start_handle;
	child_handle = 0;
	level        = 1;

	/*
	 * Traverse the tree of objects until we bubble back up
	 * to where we started.
	 */

	while (level > 0) {
		/* Attempt to get the next object in this scope */

		status = acpi_get_next_object (ACPI_TYPE_ANY, parent_handle,
				  child_handle, &next_child_handle);

		child_handle = next_child_handle;


		/* Did we get a new object? */

		if (ACPI_SUCCESS (status)) {
			/* Check if this object has any children */

			if (ACPI_SUCCESS (acpi_get_next_object (ACPI_TYPE_ANY, child_handle,
					 0, &dummy))) {
				/*
				 * There is at least one child of this object,
				 * visit the object
				 */

				level++;
				parent_handle = child_handle;
				child_handle = 0;
			}
		}

		else {
			/*
			 * No more children in this object, go back up to
			 * the object's parent
			 */
			level--;

			/* Delete all children now */

			acpi_ns_delete_children (child_handle);

			child_handle = parent_handle;
			acpi_get_parent (parent_handle, &parent_handle);
		}
	}

	/* Now delete the starting object, and we are done */

	acpi_ns_delete_node (child_handle);

	return (AE_OK);
}


/*******************************************************************************
 *
 *  FUNCTION:       Acpi_ns_unload_name_space
 *
 *  PARAMETERS:     Handle          - Root of namespace subtree to be deleted
 *
 *  RETURN:         Status
 *
 *  DESCRIPTION:    Shrinks the namespace, typically in response to an undocking
 *                  event.  Deletes an entire subtree starting from (and
 *                  including) the given handle.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_unload_namespace (
	ACPI_HANDLE             handle)
{
	ACPI_STATUS             status;


	/* Parameter validation */

	if (!acpi_gbl_root_node) {
		return (AE_NO_NAMESPACE);
	}

	if (!handle) {
		return (AE_BAD_PARAMETER);
	}


	/* This function does the real work */

	status = acpi_ns_delete_subtree (handle);

	return (status);
}


