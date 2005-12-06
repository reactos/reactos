/******************************************************************************
 *
 * Module Name: dsfield - Dispatcher field routines
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
#include "amlcode.h"
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dsfield")


/*
 * Field flags: Bits 00 - 03 : Access_type (Any_acc, Byte_acc, etc.)
 *                   04      : Lock_rule (1 == Lock)
 *                   05 - 06 : Update_rule
 */

#define FIELD_ACCESS_TYPE_MASK      0x0F
#define FIELD_LOCK_RULE_MASK        0x10
#define FIELD_UPDATE_RULE_MASK      0x60


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_create_field
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *              Region_node - Object for the containing Operation Region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new field in the specified operation region
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_create_field (
	ACPI_PARSE_OBJECT       *op,
	ACPI_NAMESPACE_NODE     *region_node,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_AML_ERROR;
	ACPI_PARSE_OBJECT       *arg;
	ACPI_NAMESPACE_NODE     *node;
	u8                      field_flags;
	u8                      access_attribute = 0;
	u32                     field_bit_position = 0;


	/* First arg is the name of the parent Op_region */

	arg = op->value.arg;
	if (!region_node) {
		status = acpi_ns_lookup (walk_state->scope_info, arg->value.name,
				 ACPI_TYPE_REGION, IMODE_EXECUTE,
				 NS_SEARCH_PARENT, walk_state,
				 &region_node);

		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/* Second arg is the field flags */

	arg = arg->next;
	field_flags = (u8) arg->value.integer;

	/* Each remaining arg is a Named Field */

	arg = arg->next;
	while (arg) {
		switch (arg->opcode) {
		case AML_RESERVEDFIELD_OP:

			field_bit_position += arg->value.size;
			break;


		case AML_ACCESSFIELD_OP:

			/*
			 * Get a new Access_type and Access_attribute for all
			 * entries (until end or another Access_as keyword)
			 */

			access_attribute = (u8) arg->value.integer;
			field_flags     = (u8)
					  ((field_flags & FIELD_ACCESS_TYPE_MASK) ||
					  ((u8) (arg->value.integer >> 8)));
			break;


		case AML_NAMEDFIELD_OP:

			status = acpi_ns_lookup (walk_state->scope_info,
					  (NATIVE_CHAR *) &((ACPI_PARSE2_OBJECT *)arg)->name,
					  INTERNAL_TYPE_DEF_FIELD,
					  IMODE_LOAD_PASS1,
					  NS_NO_UPSEARCH | NS_DONT_OPEN_SCOPE,
					  NULL, &node);

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/*
			 * Initialize an object for the new Node that is on
			 * the object stack
			 */

			status = acpi_aml_prep_def_field_value (node, region_node, field_flags,
					  access_attribute, field_bit_position, arg->value.size);

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/* Keep track of bit position for *next* field */

			field_bit_position += arg->value.size;
			break;
		}

		arg = arg->next;
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_create_bank_field
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *              Region_node - Object for the containing Operation Region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new bank field in the specified operation region
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_create_bank_field (
	ACPI_PARSE_OBJECT       *op,
	ACPI_NAMESPACE_NODE     *region_node,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_AML_ERROR;
	ACPI_PARSE_OBJECT       *arg;
	ACPI_NAMESPACE_NODE     *register_node;
	ACPI_NAMESPACE_NODE     *node;
	u32                     bank_value;
	u8                      field_flags;
	u8                      access_attribute = 0;
	u32                     field_bit_position = 0;


	/* First arg is the name of the parent Op_region */

	arg = op->value.arg;
	if (!region_node) {
		status = acpi_ns_lookup (walk_state->scope_info, arg->value.name,
				 ACPI_TYPE_REGION, IMODE_EXECUTE,
				 NS_SEARCH_PARENT, walk_state,
				 &region_node);

		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/* Second arg is the Bank Register */

	arg = arg->next;

	status = acpi_ns_lookup (walk_state->scope_info, arg->value.string,
			 INTERNAL_TYPE_BANK_FIELD_DEFN,
			 IMODE_LOAD_PASS1,
			 NS_NO_UPSEARCH | NS_DONT_OPEN_SCOPE,
			 NULL, &register_node);

	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Third arg is the Bank_value */

	arg = arg->next;
	bank_value = arg->value.integer;


	/* Next arg is the field flags */

	arg = arg->next;
	field_flags = (u8) arg->value.integer;

	/* Each remaining arg is a Named Field */

	arg = arg->next;
	while (arg) {
		switch (arg->opcode) {
		case AML_RESERVEDFIELD_OP:

			field_bit_position += arg->value.size;
			break;


		case AML_ACCESSFIELD_OP:

			/*
			 * Get a new Access_type and Access_attribute for
			 * all entries (until end or another Access_as keyword)
			 */

			access_attribute = (u8) arg->value.integer;
			field_flags     = (u8)
					  ((field_flags & FIELD_ACCESS_TYPE_MASK) ||
					  ((u8) (arg->value.integer >> 8)));
			break;


		case AML_NAMEDFIELD_OP:

			status = acpi_ns_lookup (walk_state->scope_info,
					  (NATIVE_CHAR *) &((ACPI_PARSE2_OBJECT *)arg)->name,
					  INTERNAL_TYPE_DEF_FIELD,
					  IMODE_LOAD_PASS1,
					  NS_NO_UPSEARCH | NS_DONT_OPEN_SCOPE,
					  NULL, &node);

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/*
			 * Initialize an object for the new Node that is on
			 * the object stack
			 */

			status = acpi_aml_prep_bank_field_value (node, region_node, register_node,
					  bank_value, field_flags, access_attribute,
					  field_bit_position, arg->value.size);

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/* Keep track of bit position for the *next* field */

			field_bit_position += arg->value.size;
			break;

		}

		arg = arg->next;
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_create_index_field
 *
 * PARAMETERS:  Op              - Op containing the Field definition and args
 *              Region_node - Object for the containing Operation Region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new index field in the specified operation region
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_create_index_field (
	ACPI_PARSE_OBJECT       *op,
	ACPI_HANDLE             region_node,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_PARSE_OBJECT       *arg;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_NAMESPACE_NODE     *index_register_node;
	ACPI_NAMESPACE_NODE     *data_register_node;
	u8                      field_flags;
	u8                      access_attribute = 0;
	u32                     field_bit_position = 0;


	arg = op->value.arg;

	/* First arg is the name of the Index register */

	status = acpi_ns_lookup (walk_state->scope_info, arg->value.string,
			 ACPI_TYPE_ANY, IMODE_LOAD_PASS1,
			 NS_NO_UPSEARCH | NS_DONT_OPEN_SCOPE,
			 NULL, &index_register_node);

	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Second arg is the data register */

	arg = arg->next;

	status = acpi_ns_lookup (walk_state->scope_info, arg->value.string,
			 INTERNAL_TYPE_INDEX_FIELD_DEFN,
			 IMODE_LOAD_PASS1,
			 NS_NO_UPSEARCH | NS_DONT_OPEN_SCOPE,
			 NULL, &data_register_node);

	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/* Next arg is the field flags */

	arg = arg->next;
	field_flags = (u8) arg->value.integer;


	/* Each remaining arg is a Named Field */

	arg = arg->next;
	while (arg) {
		switch (arg->opcode) {
		case AML_RESERVEDFIELD_OP:

			field_bit_position += arg->value.size;
			break;


		case AML_ACCESSFIELD_OP:

			/*
			 * Get a new Access_type and Access_attribute for all
			 * entries (until end or another Access_as keyword)
			 */

			access_attribute = (u8) arg->value.integer;
			field_flags     = (u8)
					   ((field_flags & FIELD_ACCESS_TYPE_MASK) ||
					   ((u8) (arg->value.integer >> 8)));
			break;


		case AML_NAMEDFIELD_OP:

			status = acpi_ns_lookup (walk_state->scope_info,
					 (NATIVE_CHAR *) &((ACPI_PARSE2_OBJECT *)arg)->name,
					 INTERNAL_TYPE_INDEX_FIELD,
					 IMODE_LOAD_PASS1,
					 NS_NO_UPSEARCH | NS_DONT_OPEN_SCOPE,
					 NULL, &node);

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/*
			 * Initialize an object for the new Node that is on
			 * the object stack
			 */

			status = acpi_aml_prep_index_field_value (node, index_register_node, data_register_node,
					  field_flags, access_attribute,
					  field_bit_position, arg->value.size);

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/* Keep track of bit position for the *next* field */

			field_bit_position += arg->value.size;
			break;


		default:

			status = AE_AML_ERROR;
			break;
		}

		arg = arg->next;
	}

	return (status);
}


