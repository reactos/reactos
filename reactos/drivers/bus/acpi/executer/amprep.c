
/******************************************************************************
 *
 * Module Name: amprep - ACPI AML (p-code) execution - field prep utilities
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
#include "amlcode.h"
#include "acnamesp.h"
#include "acparser.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amprep")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_decode_field_access_type
 *
 * PARAMETERS:  Access          - Encoded field access bits
 *
 * RETURN:      Field granularity (8, 16, or 32)
 *
 * DESCRIPTION: Decode the Access_type bits of a field definition.
 *
 ******************************************************************************/

static u32
acpi_aml_decode_field_access_type (
	u32                     access,
	u16                     length)
{

	switch (access) {
	case ACCESS_ANY_ACC:
		if (length <= 8) {
			return (8);
		}
		else if (length <= 16) {
			return (16);
		}
		else if (length <= 32) {
			return (32);
		}
		else {
			return (8);
		}
		break;

	case ACCESS_BYTE_ACC:
		return (8);
		break;

	case ACCESS_WORD_ACC:
		return (16);
		break;

	case ACCESS_DWORD_ACC:
		return (32);
		break;

	default:
		/* Invalid field access type */

		return (0);
	}
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_prep_common_field_objec
 *
 * PARAMETERS:  Obj_desc            - The field object
 *              Field_flags         - Access, Lock_rule, or Update_rule.
 *                                    The format of a Field_flag is described
 *                                    in the ACPI specification
 *              Field_position      - Field position
 *              Field_length        - Field length
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the areas of the field object that are common
 *              to the various types of fields.
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_aml_prep_common_field_object (
	ACPI_OPERAND_OBJECT     *obj_desc,
	u8                      field_flags,
	u8                      field_attribute,
	u32                     field_position,
	u32                     field_length)
{
	u32                     granularity;


	/*
	 * Note: the structure being initialized is the
	 * ACPI_COMMON_FIELD_INFO;  Therefore, we can just use the Field union to
	 * access this common area.  No structure fields outside of the common area
	 * are initialized by this procedure.
	 */

	/* Decode the Field_flags */

	obj_desc->field.access          = (u8) ((field_flags & ACCESS_TYPE_MASK)
			 >> ACCESS_TYPE_SHIFT);
	obj_desc->field.lock_rule       = (u8) ((field_flags & LOCK_RULE_MASK)
			 >> LOCK_RULE_SHIFT);
	obj_desc->field.update_rule     = (u8) ((field_flags & UPDATE_RULE_MASK)
			 >> UPDATE_RULE_SHIFT);

	/* Other misc fields */

	obj_desc->field.length          = (u16) field_length;
	obj_desc->field.access_attribute = field_attribute;

	/* Decode the access type so we can compute offsets */

	granularity = acpi_aml_decode_field_access_type (obj_desc->field.access, obj_desc->field.length);
	if (!granularity) {
		return (AE_AML_OPERAND_VALUE);
	}

	/* Access granularity based fields */

	obj_desc->field.granularity     = (u8) granularity;
	obj_desc->field.bit_offset      = (u8) (field_position % granularity);
	obj_desc->field.offset          = (u32) field_position / granularity;


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_prep_def_field_value
 *
 * PARAMETERS:  Node            - Owning Node
 *              Region              - Region in which field is being defined
 *              Field_flags         - Access, Lock_rule, or Update_rule.
 *                                    The format of a Field_flag is described
 *                                    in the ACPI specification
 *              Field_position      - Field position
 *              Field_length        - Field length
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct an ACPI_OPERAND_OBJECT  of type Def_field and
 *              connect it to the parent Node.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_prep_def_field_value (
	ACPI_NAMESPACE_NODE     *node,
	ACPI_HANDLE             region,
	u8                      field_flags,
	u8                      field_attribute,
	u32                     field_position,
	u32                     field_length)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	u32                     type;
	ACPI_STATUS             status;


	/* Parameter validation */

	if (!region) {
		return (AE_AML_NO_OPERAND);
	}

	type = acpi_ns_get_type (region);
	if (type != ACPI_TYPE_REGION) {
		return (AE_AML_OPERAND_TYPE);
	}

	/* Allocate a new object */

	obj_desc = acpi_cm_create_internal_object (INTERNAL_TYPE_DEF_FIELD);
	if (!obj_desc) {
		return (AE_NO_MEMORY);
	}


	/* Obj_desc and Region valid */

	/* Initialize areas of the object that are common to all fields */

	status = acpi_aml_prep_common_field_object (obj_desc, field_flags, field_attribute,
			  field_position, field_length);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Initialize areas of the object that are specific to this field type */

	obj_desc->field.container = acpi_ns_get_attached_object (region);

	/* An additional reference for the container */

	acpi_cm_add_reference (obj_desc->field.container);


	/* Debug info */

	/*
	 * Store the constructed descriptor (Obj_desc) into the Named_obj whose
	 * handle is on TOS, preserving the current type of that Named_obj.
	 */
	status = acpi_ns_attach_object ((ACPI_HANDLE) node, obj_desc,
			  (u8) acpi_ns_get_type ((ACPI_HANDLE) node));

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_prep_bank_field_value
 *
 * PARAMETERS:  Node            - Owning Node
 *              Region              - Region in which field is being defined
 *              Bank_reg            - Bank selection register
 *              Bank_val            - Value to store in selection register
 *              Field_flags         - Access, Lock_rule, or Update_rule
 *              Field_position      - Field position
 *              Field_length        - Field length
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct an ACPI_OPERAND_OBJECT  of type Bank_field and
 *              connect it to the parent Node.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_prep_bank_field_value (
	ACPI_NAMESPACE_NODE     *node,
	ACPI_HANDLE             region,
	ACPI_HANDLE             bank_reg,
	u32                     bank_val,
	u8                      field_flags,
	u8                      field_attribute,
	u32                     field_position,
	u32                     field_length)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	u32                     type;
	ACPI_STATUS             status;


	/* Parameter validation */

	if (!region) {
		return (AE_AML_NO_OPERAND);
	}

	type = acpi_ns_get_type (region);
	if (type != ACPI_TYPE_REGION) {
		return (AE_AML_OPERAND_TYPE);
	}

	/* Allocate a new object */

	obj_desc = acpi_cm_create_internal_object (INTERNAL_TYPE_BANK_FIELD);
	if (!obj_desc) {
		return (AE_NO_MEMORY);
	}

	/*  Obj_desc and Region valid   */

	/* Initialize areas of the object that are common to all fields */

	status = acpi_aml_prep_common_field_object (obj_desc, field_flags, field_attribute,
			  field_position, field_length);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Initialize areas of the object that are specific to this field type */

	obj_desc->bank_field.value      = bank_val;
	obj_desc->bank_field.container  = acpi_ns_get_attached_object (region);
	obj_desc->bank_field.bank_select = acpi_ns_get_attached_object (bank_reg);

	/* An additional reference for the container and bank select */
	/* TBD: [Restructure] is "Bank_select" ever a real internal object?? */

	acpi_cm_add_reference (obj_desc->bank_field.container);
	acpi_cm_add_reference (obj_desc->bank_field.bank_select);

	/* Debug info */

	/*
	 * Store the constructed descriptor (Obj_desc) into the Named_obj whose
	 * handle is on TOS, preserving the current type of that Named_obj.
	 */
	status = acpi_ns_attach_object ((ACPI_HANDLE) node, obj_desc,
			 (u8) acpi_ns_get_type ((ACPI_HANDLE) node));

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_prep_index_field_value
 *
 * PARAMETERS:  Node            - Owning Node
 *              Index_reg           - Index register
 *              Data_reg            - Data register
 *              Field_flags         - Access, Lock_rule, or Update_rule
 *              Field_position      - Field position
 *              Field_length        - Field length
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Construct an ACPI_OPERAND_OBJECT  of type Index_field and
 *              connect it to the parent Node.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_prep_index_field_value (
	ACPI_NAMESPACE_NODE     *node,
	ACPI_HANDLE             index_reg,
	ACPI_HANDLE             data_reg,
	u8                      field_flags,
	u8                      field_attribute,
	u32                     field_position,
	u32                     field_length)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Parameter validation */

	if (!index_reg || !data_reg) {
		return (AE_AML_NO_OPERAND);
	}

	/* Allocate a new object descriptor */

	obj_desc = acpi_cm_create_internal_object (INTERNAL_TYPE_INDEX_FIELD);
	if (!obj_desc) {
		return (AE_NO_MEMORY);
	}

	/* Initialize areas of the object that are common to all fields */

	status = acpi_aml_prep_common_field_object (obj_desc, field_flags, field_attribute,
			  field_position, field_length);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Initialize areas of the object that are specific to this field type */

	obj_desc->index_field.value     = (u32) (field_position /
			   obj_desc->field.granularity);
	obj_desc->index_field.index     = index_reg;
	obj_desc->index_field.data      = data_reg;

	/* Debug info */

	/*
	 * Store the constructed descriptor (Obj_desc) into the Named_obj whose
	 * handle is on TOS, preserving the current type of that Named_obj.
	 */
	status = acpi_ns_attach_object ((ACPI_HANDLE) node, obj_desc,
			 (u8) acpi_ns_get_type ((ACPI_HANDLE) node));

	return (status);
}

