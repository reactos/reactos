/******************************************************************************
 *
 * Module Name: psutils - Parser miscellaneous utilities (Parser only)
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
#include "amlcode.h"

#define _COMPONENT          ACPI_PARSER
	 MODULE_NAME         ("psutils")


#define PARSEOP_GENERIC     0x01
#define PARSEOP_NAMED       0x02
#define PARSEOP_DEFERRED    0x03
#define PARSEOP_BYTELIST    0x04
#define PARSEOP_IN_CACHE    0x80


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_init_op
 *
 * PARAMETERS:  Op              - A newly allocated Op object
 *              Opcode          - Opcode to store in the Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate an acpi_op, choose op type (and thus size) based on
 *              opcode
 *
 ******************************************************************************/

void
acpi_ps_init_op (
	ACPI_PARSE_OBJECT       *op,
	u16                     opcode)
{
	ACPI_OPCODE_INFO        *aml_op;


	op->data_type = ACPI_DESC_TYPE_PARSER;
	op->opcode = opcode;

	aml_op = acpi_ps_get_opcode_info (opcode);

	DEBUG_ONLY_MEMBERS (STRNCPY (op->op_name, aml_op->name,
			   sizeof (op->op_name)));
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_alloc_op
 *
 * PARAMETERS:  Opcode          - Opcode that will be stored in the new Op
 *
 * RETURN:      Pointer to the new Op.
 *
 * DESCRIPTION: Allocate an acpi_op, choose op type (and thus size) based on
 *              opcode.  A cache of opcodes is available for the pure
 *              GENERIC_OP, since this is by far the most commonly used.
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT*
acpi_ps_alloc_op (
	u16                     opcode)
{
	ACPI_PARSE_OBJECT       *op = NULL;
	u32                     size;
	u8                      flags;


	/* Allocate the minimum required size object */

	if (acpi_ps_is_deferred_op (opcode)) {
		size = sizeof (ACPI_PARSE2_OBJECT);
		flags = PARSEOP_DEFERRED;
	}

	else if (acpi_ps_is_named_op (opcode)) {
		size = sizeof (ACPI_PARSE2_OBJECT);
		flags = PARSEOP_NAMED;
	}

	else if (acpi_ps_is_bytelist_op (opcode)) {
		size = sizeof (ACPI_PARSE2_OBJECT);
		flags = PARSEOP_BYTELIST;
	}

	else {
		size = sizeof (ACPI_PARSE_OBJECT);
		flags = PARSEOP_GENERIC;
	}


	if (size == sizeof (ACPI_PARSE_OBJECT)) {
		/*
		 * The generic op is by far the most common (16 to 1), and therefore
		 * the op cache is implemented with this type.
		 *
		 * Check if there is an Op already available in the cache
		 */

		acpi_cm_acquire_mutex (ACPI_MTX_CACHES);
		acpi_gbl_parse_cache_requests++;
		if (acpi_gbl_parse_cache) {
			/* Extract an op from the front of the cache list */

			acpi_gbl_parse_cache_depth--;
			acpi_gbl_parse_cache_hits++;

			op = acpi_gbl_parse_cache;
			acpi_gbl_parse_cache = op->next;


			/* Clear the previously used Op */

			MEMSET (op, 0, sizeof (ACPI_PARSE_OBJECT));

		}
		acpi_cm_release_mutex (ACPI_MTX_CACHES);
	}

	else {
		/*
		 * The generic op is by far the most common (16 to 1), and therefore
		 * the op cache is implemented with this type.
		 *
		 * Check if there is an Op already available in the cache
		 */

		acpi_cm_acquire_mutex (ACPI_MTX_CACHES);
		acpi_gbl_ext_parse_cache_requests++;
		if (acpi_gbl_ext_parse_cache) {
			/* Extract an op from the front of the cache list */

			acpi_gbl_ext_parse_cache_depth--;
			acpi_gbl_ext_parse_cache_hits++;

			op = (ACPI_PARSE_OBJECT *) acpi_gbl_ext_parse_cache;
			acpi_gbl_ext_parse_cache = (ACPI_PARSE2_OBJECT *) op->next;


			/* Clear the previously used Op */

			MEMSET (op, 0, sizeof (ACPI_PARSE2_OBJECT));

		}
		acpi_cm_release_mutex (ACPI_MTX_CACHES);
	}


	/* Allocate a new Op if necessary */

	if (!op) {
		op = acpi_cm_callocate (size);
	}

	/* Initialize the Op */
	if (op) {
		acpi_ps_init_op (op, opcode);
		op->flags = flags;
	}

	return (op);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_free_op
 *
 * PARAMETERS:  Op              - Op to be freed
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free an Op object.  Either put it on the GENERIC_OP cache list
 *              or actually free it.
 *
 ******************************************************************************/

void
acpi_ps_free_op (
	ACPI_PARSE_OBJECT       *op)
{



	if (op->flags == PARSEOP_GENERIC) {
		/* Is the cache full? */

		if (acpi_gbl_parse_cache_depth < MAX_PARSE_CACHE_DEPTH) {
			/* Put a GENERIC_OP back into the cache */

			/* Clear the previously used Op */

			MEMSET (op, 0, sizeof (ACPI_PARSE_OBJECT));
			op->flags = PARSEOP_IN_CACHE;

			acpi_cm_acquire_mutex (ACPI_MTX_CACHES);
			acpi_gbl_parse_cache_depth++;

			op->next = acpi_gbl_parse_cache;
			acpi_gbl_parse_cache = op;

			acpi_cm_release_mutex (ACPI_MTX_CACHES);
			return;
		}
	}

	else {
		/* Is the cache full? */

		if (acpi_gbl_ext_parse_cache_depth < MAX_EXTPARSE_CACHE_DEPTH) {
			/* Put a GENERIC_OP back into the cache */

			/* Clear the previously used Op */

			MEMSET (op, 0, sizeof (ACPI_PARSE2_OBJECT));
			op->flags = PARSEOP_IN_CACHE;

			acpi_cm_acquire_mutex (ACPI_MTX_CACHES);
			acpi_gbl_ext_parse_cache_depth++;

			op->next = (ACPI_PARSE_OBJECT *) acpi_gbl_ext_parse_cache;
			acpi_gbl_ext_parse_cache = (ACPI_PARSE2_OBJECT *) op;

			acpi_cm_release_mutex (ACPI_MTX_CACHES);
			return;
		}
	}


	/*
	 * Not a GENERIC OP, or the cache is full, just free the Op
	 */

	acpi_cm_free (op);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_delete_parse_cache
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Free all objects that are on the parse cache list.
 *
 ******************************************************************************/

void
acpi_ps_delete_parse_cache (
	void)
{
	ACPI_PARSE_OBJECT       *next;


	/* Traverse the global cache list */

	while (acpi_gbl_parse_cache) {
		/* Delete one cached state object */

		next = acpi_gbl_parse_cache->next;
		acpi_cm_free (acpi_gbl_parse_cache);
		acpi_gbl_parse_cache = next;
		acpi_gbl_parse_cache_depth--;
	}

	/* Traverse the global cache list */

	while (acpi_gbl_ext_parse_cache) {
		/* Delete one cached state object */

		next = acpi_gbl_ext_parse_cache->next;
		acpi_cm_free (acpi_gbl_ext_parse_cache);
		acpi_gbl_ext_parse_cache = (ACPI_PARSE2_OBJECT *) next;
		acpi_gbl_ext_parse_cache_depth--;
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Utility functions
 *
 * DESCRIPTION: Low level functions
 *
 * TBD: [Restructure]
 * 1) Some of these functions should be macros
 * 2) Some can be simplified
 *
 ******************************************************************************/


/*
 * Is "c" a namestring lead character?
 */


u8
acpi_ps_is_leading_char (
	u32                     c)
{
	return ((u8) (c == '_' || (c >= 'A' && c <= 'Z')));
}


/*
 * Is "c" a namestring prefix character?
 */
u8
acpi_ps_is_prefix_char (
	u32                     c)
{
	return ((u8) (c == '\\' || c == '^'));
}


u8
acpi_ps_is_namespace_object_op (
	u16                     opcode)
{
	return ((u8)
		   (opcode == AML_SCOPE_OP          ||
			opcode == AML_DEVICE_OP         ||
			opcode == AML_THERMAL_ZONE_OP   ||
			opcode == AML_METHOD_OP         ||
			opcode == AML_POWER_RES_OP      ||
			opcode == AML_PROCESSOR_OP      ||
			opcode == AML_DEF_FIELD_OP      ||
			opcode == AML_INDEX_FIELD_OP    ||
			opcode == AML_BANK_FIELD_OP     ||
			opcode == AML_NAMEDFIELD_OP     ||
			opcode == AML_NAME_OP           ||
			opcode == AML_ALIAS_OP          ||
			opcode == AML_MUTEX_OP          ||
			opcode == AML_EVENT_OP          ||
			opcode == AML_REGION_OP         ||
			opcode == AML_CREATE_FIELD_OP   ||
			opcode == AML_BIT_FIELD_OP      ||
			opcode == AML_BYTE_FIELD_OP     ||
			opcode == AML_WORD_FIELD_OP     ||
			opcode == AML_DWORD_FIELD_OP    ||
			opcode == AML_METHODCALL_OP     ||
			opcode == AML_NAMEPATH_OP));
}

u8
acpi_ps_is_namespace_op (
	u16                     opcode)
{
	return ((u8)
		   (opcode == AML_SCOPE_OP          ||
			opcode == AML_DEVICE_OP         ||
			opcode == AML_THERMAL_ZONE_OP   ||
			opcode == AML_METHOD_OP         ||
			opcode == AML_POWER_RES_OP      ||
			opcode == AML_PROCESSOR_OP      ||
			opcode == AML_DEF_FIELD_OP      ||
			opcode == AML_INDEX_FIELD_OP    ||
			opcode == AML_BANK_FIELD_OP     ||
			opcode == AML_NAME_OP           ||
			opcode == AML_ALIAS_OP          ||
			opcode == AML_MUTEX_OP          ||
			opcode == AML_EVENT_OP          ||
			opcode == AML_REGION_OP         ||
			opcode == AML_NAMEDFIELD_OP));
}


/*
 * Is opcode for a named object Op?
 * (Includes all named object opcodes)
 *
 * TBD: [Restructure] Need a better way than this brute force approach!
 */
u8
acpi_ps_is_node_op (
	u16                     opcode)
{
	return ((u8)
		   (opcode == AML_SCOPE_OP          ||
			opcode == AML_DEVICE_OP         ||
			opcode == AML_THERMAL_ZONE_OP   ||
			opcode == AML_METHOD_OP         ||
			opcode == AML_POWER_RES_OP      ||
			opcode == AML_PROCESSOR_OP      ||
			opcode == AML_NAMEDFIELD_OP     ||
			opcode == AML_NAME_OP           ||
			opcode == AML_ALIAS_OP          ||
			opcode == AML_MUTEX_OP          ||
			opcode == AML_EVENT_OP          ||
			opcode == AML_REGION_OP         ||


			opcode == AML_CREATE_FIELD_OP   ||
			opcode == AML_BIT_FIELD_OP      ||
			opcode == AML_BYTE_FIELD_OP     ||
			opcode == AML_WORD_FIELD_OP     ||
			opcode == AML_DWORD_FIELD_OP    ||
			opcode == AML_METHODCALL_OP     ||
			opcode == AML_NAMEPATH_OP));
}


/*
 * Is opcode for a named Op?
 */
u8
acpi_ps_is_named_op (
	u16                     opcode)
{
	return ((u8)
		   (opcode == AML_SCOPE_OP          ||
			opcode == AML_DEVICE_OP         ||
			opcode == AML_THERMAL_ZONE_OP   ||
			opcode == AML_METHOD_OP         ||
			opcode == AML_POWER_RES_OP      ||
			opcode == AML_PROCESSOR_OP      ||
			opcode == AML_NAME_OP           ||
			opcode == AML_ALIAS_OP          ||
			opcode == AML_MUTEX_OP          ||
			opcode == AML_EVENT_OP          ||
			opcode == AML_REGION_OP         ||
			opcode == AML_NAMEDFIELD_OP));
}


u8
acpi_ps_is_deferred_op (
	u16                     opcode)
{
	return ((u8)
		   (opcode == AML_METHOD_OP         ||
			opcode == AML_CREATE_FIELD_OP   ||
			opcode == AML_BIT_FIELD_OP      ||
			opcode == AML_BYTE_FIELD_OP     ||
			opcode == AML_WORD_FIELD_OP     ||
			opcode == AML_DWORD_FIELD_OP    ||
			opcode == AML_REGION_OP));
}


/*
 * Is opcode for a bytelist?
 */
u8
acpi_ps_is_bytelist_op (
	u16                     opcode)
{
	return ((u8) (opcode == AML_BYTELIST_OP));
}


/*
 * Is opcode for a Field, Index_field, or Bank_field
 */
u8
acpi_ps_is_field_op (
	u16                     opcode)
{
	return ((u8)
			  (opcode == AML_CREATE_FIELD_OP
			|| opcode == AML_DEF_FIELD_OP
			|| opcode == AML_INDEX_FIELD_OP
			|| opcode == AML_BANK_FIELD_OP));
}


/*
 * Is field creation op
 */
u8
acpi_ps_is_create_field_op (
	u16                     opcode)
{
	return ((u8)
		   (opcode == AML_CREATE_FIELD_OP   ||
			opcode == AML_BIT_FIELD_OP      ||
			opcode == AML_BYTE_FIELD_OP     ||
			opcode == AML_WORD_FIELD_OP     ||
			opcode == AML_DWORD_FIELD_OP));
}


/*
 * Cast an acpi_op to an acpi_extended_op if possible
 */

/* TBD: This is very inefficient, fix */
ACPI_PARSE2_OBJECT *
acpi_ps_to_extended_op (
	ACPI_PARSE_OBJECT       *op)
{
	return ((acpi_ps_is_deferred_op (op->opcode) || acpi_ps_is_named_op (op->opcode) || acpi_ps_is_bytelist_op (op->opcode))
			? ( (ACPI_PARSE2_OBJECT *) op) : NULL);
}


/*
 * Get op's name (4-byte name segment) or 0 if unnamed
 */
u32
acpi_ps_get_name (
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_PARSE2_OBJECT      *named = acpi_ps_to_extended_op (op);

	return (named ? named->name : 0);
}


/*
 * Set op's name
 */
void
acpi_ps_set_name (
	ACPI_PARSE_OBJECT       *op,
	u32                     name)
{
	ACPI_PARSE2_OBJECT      *named = acpi_ps_to_extended_op (op);

	if (named) {
		named->name = name;
	}
}

