/******************************************************************************
 *
 * Module Name: psargs - Parse AML opcode arguments
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
#include "acnamesp.h"

#define _COMPONENT          ACPI_PARSER
	 MODULE_NAME         ("psargs")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_package_length
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Decoded package length.  On completion, the AML pointer points
 *              past the length byte or bytes.
 *
 * DESCRIPTION: Decode and return a package length field
 *
 ******************************************************************************/

u32
acpi_ps_get_next_package_length (
	ACPI_PARSE_STATE        *parser_state)
{
	u32                     encoded_length;
	u32                     length = 0;


	encoded_length = (u32) GET8 (parser_state->aml);
	parser_state->aml++;


	switch (encoded_length >> 6) /* bits 6-7 contain encoding scheme */ {
	case 0: /* 1-byte encoding (bits 0-5) */

		length = (encoded_length & 0x3F);
		break;


	case 1: /* 2-byte encoding (next byte + bits 0-3) */

		length = ((GET8 (parser_state->aml) << 04) |
				 (encoded_length & 0x0F));
		parser_state->aml++;
		break;


	case 2: /* 3-byte encoding (next 2 bytes + bits 0-3) */

		length = ((GET8 (parser_state->aml + 1) << 12) |
				  (GET8 (parser_state->aml)    << 04) |
				  (encoded_length & 0x0F));
		parser_state->aml += 2;
		break;


	case 3: /* 4-byte encoding (next 3 bytes + bits 0-3) */

		length = ((GET8 (parser_state->aml + 2) << 20) |
				  (GET8 (parser_state->aml + 1) << 12) |
				  (GET8 (parser_state->aml)    << 04) |
				  (encoded_length & 0x0F));
		parser_state->aml += 3;
		break;
	}

	return (length);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_package_end
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Pointer to end-of-package +1
 *
 * DESCRIPTION: Get next package length and return a pointer past the end of
 *              the package.  Consumes the package length field
 *
 ******************************************************************************/

u8 *
acpi_ps_get_next_package_end (
	ACPI_PARSE_STATE        *parser_state)
{
	u8                      *start = parser_state->aml;
	NATIVE_UINT             length;


	length = (NATIVE_UINT) acpi_ps_get_next_package_length (parser_state);

	return (start + length); /* end of package */
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_namestring
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Pointer to the start of the name string (pointer points into
 *              the AML.
 *
 * DESCRIPTION: Get next raw namestring within the AML stream.  Handles all name
 *              prefix characters.  Set parser state to point past the string.
 *              (Name is consumed from the AML.)
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_ps_get_next_namestring (
	ACPI_PARSE_STATE        *parser_state)
{
	u8                       *start = parser_state->aml;
	u8                       *end = parser_state->aml;
	u32                     length;


	/* Handle multiple prefix characters */

	while (acpi_ps_is_prefix_char (GET8 (end))) {
		/* include prefix '\\' or '^' */

		end++;
	}

	/* Decode the path */

	switch (GET8 (end)) {
	case 0:

		/* Null_name */

		if (end == start) {
			start = NULL;
		}
		end++;
		break;


	case AML_DUAL_NAME_PREFIX:

		/* two name segments */

		end += 9;
		break;


	case AML_MULTI_NAME_PREFIX_OP:

		/* multiple name segments */

		length = (u32) GET8 (end + 1) * 4;
		end += 2 + length;
		break;


	default:

		/* single name segment */
		/* assert (Acpi_ps_is_lead (GET8 (End))); */

		end += 4;
		break;
	}

	parser_state->aml = (u8*) end;

	return ((NATIVE_CHAR *) start);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_namepath
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *              Arg                 - Where the namepath will be stored
 *              Arg_count           - If the namepath points to a control method
 *                                    the method's argument is returned here.
 *              Method_call         - Whether the namepath can be the start
 *                                    of a method call
 *
 * RETURN:      None
 *
 * DESCRIPTION: Get next name (if method call, push appropriate # args).  Names
 *              are looked up in either the parsed or internal namespace to
 *              determine if the name represents a control method.  If a method
 *              is found, the number of arguments to the method is returned.
 *              This information is critical for parsing to continue correctly.
 *
 ******************************************************************************/


#ifdef PARSER_ONLY

void
acpi_ps_get_next_namepath (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *arg,
	u32                     *arg_count,
	u8                      method_call)
{
	NATIVE_CHAR             *path;
	ACPI_PARSE_OBJECT       *name_op;
	ACPI_PARSE_OBJECT       *op;
	ACPI_PARSE_OBJECT       *count;


	path = acpi_ps_get_next_namestring (parser_state);
	if (!path || !method_call) {
		/* Null name case, create a null namepath object */

		acpi_ps_init_op (arg, AML_NAMEPATH_OP);
		arg->value.name = path;
		return;
	}


	if (acpi_gbl_parsed_namespace_root) {
		/*
		 * Lookup the name in the parsed namespace
		 */

		op = NULL;
		if (method_call) {
			op = acpi_ps_find (acpi_ps_get_parent_scope (parser_state),
					   path, AML_METHOD_OP, 0);
		}

		if (op) {
			if (op->opcode == AML_METHOD_OP) {
				/*
				 * The name refers to a control method, so this namepath is a
				 * method invocation.  We need to 1) Get the number of arguments
				 * associated with this method, and 2) Change the NAMEPATH
				 * object into a METHODCALL object.
				 */

				count = acpi_ps_get_arg (op, 0);
				if (count && count->opcode == AML_BYTE_OP) {
					name_op = acpi_ps_alloc_op (AML_NAMEPATH_OP);
					if (name_op) {
						/* Change arg into a METHOD CALL and attach the name */

						acpi_ps_init_op (arg, AML_METHODCALL_OP);

						name_op->value.name = path;

						/* Point METHODCALL/NAME to the METHOD Node */

						name_op->node = (ACPI_NAMESPACE_NODE *) op;
						acpi_ps_append_arg (arg, name_op);

						*arg_count = count->value.integer &
								 METHOD_FLAGS_ARG_COUNT;
					}
				}

				return;
			}

			/*
			 * Else this is normal named object reference.
			 * Just init the NAMEPATH object with the pathname.
			 * (See code below)
			 */
		}
	}


	/*
	 * Either we didn't find the object in the namespace, or the object is
	 * something other than a control method.  Just initialize the Op with the
	 * pathname
	 */

	acpi_ps_init_op (arg, AML_NAMEPATH_OP);
	arg->value.name = path;


	return;
}


#else


void
acpi_ps_get_next_namepath (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *arg,
	u32                     *arg_count,
	u8                      method_call)
{
	NATIVE_CHAR             *path;
	ACPI_PARSE_OBJECT       *name_op;
	ACPI_STATUS             status;
	ACPI_NAMESPACE_NODE     *method_node = NULL;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_GENERIC_STATE      scope_info;


	path = acpi_ps_get_next_namestring (parser_state);
	if (!path || !method_call) {
		/* Null name case, create a null namepath object */

		acpi_ps_init_op (arg, AML_NAMEPATH_OP);
		arg->value.name = path;
		return;
	}


	if (method_call) {
		/*
		 * Lookup the name in the internal namespace
		 */
		scope_info.scope.node = NULL;
		node = parser_state->start_node;
		if (node) {
			scope_info.scope.node = node;
		}

		/*
		 * Lookup object.  We don't want to add anything new to the namespace
		 * here, however.  So we use MODE_EXECUTE.  Allow searching of the
		 * parent tree, but don't open a new scope -- we just want to lookup the
		 * object  (MUST BE mode EXECUTE to perform upsearch)
		 */

		status = acpi_ns_lookup (&scope_info, path, ACPI_TYPE_ANY, IMODE_EXECUTE,
				 NS_SEARCH_PARENT | NS_DONT_OPEN_SCOPE, NULL,
				 &node);
		if (ACPI_SUCCESS (status)) {
			if (node->type == ACPI_TYPE_METHOD) {
				method_node = node;
				name_op = acpi_ps_alloc_op (AML_NAMEPATH_OP);
				if (name_op) {
					/* Change arg into a METHOD CALL and attach name to it */

					acpi_ps_init_op (arg, AML_METHODCALL_OP);

					name_op->value.name = path;

					/* Point METHODCALL/NAME to the METHOD Node */

					name_op->node = method_node;
					acpi_ps_append_arg (arg, name_op);

					if (!(ACPI_OPERAND_OBJECT  *) method_node->object) {
						return;
					}

					*arg_count = ((ACPI_OPERAND_OBJECT *) method_node->object)->method.param_count;
				}

				return;
			}

			/*
			 * Else this is normal named object reference.
			 * Just init the NAMEPATH object with the pathname.
			 * (See code below)
			 */
		}
	}

	/*
	 * Either we didn't find the object in the namespace, or the object is
	 * something other than a control method.  Just initialize the Op with the
	 * pathname.
	 */

	acpi_ps_init_op (arg, AML_NAMEPATH_OP);
	arg->value.name = path;


	return;
}

#endif

/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_simple_arg
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *              Arg_type            - The argument type (AML_*_ARG)
 *              Arg                 - Where the argument is returned
 *
 * RETURN:      None
 *
 * DESCRIPTION: Get the next simple argument (constant, string, or namestring)
 *
 ******************************************************************************/

void
acpi_ps_get_next_simple_arg (
	ACPI_PARSE_STATE        *parser_state,
	u32                     arg_type,
	ACPI_PARSE_OBJECT       *arg)
{


	switch (arg_type) {

	case ARGP_BYTEDATA:

		acpi_ps_init_op (arg, AML_BYTE_OP);
		arg->value.integer = (u32) GET8 (parser_state->aml);
		parser_state->aml++;
		break;


	case ARGP_WORDDATA:

		acpi_ps_init_op (arg, AML_WORD_OP);

		/* Get 2 bytes from the AML stream */

		MOVE_UNALIGNED16_TO_32 (&arg->value.integer, parser_state->aml);
		parser_state->aml += 2;
		break;


	case ARGP_DWORDDATA:

		acpi_ps_init_op (arg, AML_DWORD_OP);

		/* Get 4 bytes from the AML stream */

		MOVE_UNALIGNED32_TO_32 (&arg->value.integer, parser_state->aml);
		parser_state->aml += 4;
		break;


	case ARGP_CHARLIST:

		acpi_ps_init_op (arg, AML_STRING_OP);
		arg->value.string = (char*) parser_state->aml;

		while (GET8 (parser_state->aml) != '\0') {
			parser_state->aml++;
		}
		parser_state->aml++;
		break;


	case ARGP_NAME:
	case ARGP_NAMESTRING:

		acpi_ps_init_op (arg, AML_NAMEPATH_OP);
		arg->value.name = acpi_ps_get_next_namestring (parser_state);
		break;
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_field
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      A newly allocated FIELD op
 *
 * DESCRIPTION: Get next field (Named_field, Reserved_field, or Access_field)
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
acpi_ps_get_next_field (
	ACPI_PARSE_STATE        *parser_state)
{
	ACPI_PTRDIFF            aml_offset = parser_state->aml -
			 parser_state->aml_start;
	ACPI_PARSE_OBJECT       *field;
	u16                     opcode;
	u32                     name;


	/* determine field type */

	switch (GET8 (parser_state->aml)) {

	default:

		opcode = AML_NAMEDFIELD_OP;
		break;


	case 0x00:

		opcode = AML_RESERVEDFIELD_OP;
		parser_state->aml++;
		break;


	case 0x01:

		opcode = AML_ACCESSFIELD_OP;
		parser_state->aml++;
		break;
	}


	/* Allocate a new field op */

	field = acpi_ps_alloc_op (opcode);
	if (field) {
		field->aml_offset = aml_offset;

		/* Decode the field type */

		switch (opcode) {
		case AML_NAMEDFIELD_OP:

			/* Get the 4-character name */

			MOVE_UNALIGNED32_TO_32 (&name, parser_state->aml);
			acpi_ps_set_name (field, name);
			parser_state->aml += 4;

			/* Get the length which is encoded as a package length */

			field->value.size = acpi_ps_get_next_package_length (parser_state);
			break;


		case AML_RESERVEDFIELD_OP:

			/* Get the length which is encoded as a package length */

			field->value.size = acpi_ps_get_next_package_length (parser_state);
			break;


		case AML_ACCESSFIELD_OP:

			/* Get Access_type and Access_atrib and merge into the field Op */

			field->value.integer = ((GET8 (parser_state->aml) << 8) |
					  GET8 (parser_state->aml));
			parser_state->aml += 2;
			break;
		}
	}

	return (field);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_next_arg
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *              Arg_type            - The argument type (AML_*_ARG)
 *              Arg_count           - If the argument points to a control method
 *                                    the method's argument is returned here.
 *
 * RETURN:      An op object containing the next argument.
 *
 * DESCRIPTION: Get next argument (including complex list arguments that require
 *              pushing the parser stack)
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
acpi_ps_get_next_arg (
	ACPI_PARSE_STATE        *parser_state,
	u32                     arg_type,
	u32                     *arg_count)
{
	ACPI_PARSE_OBJECT       *arg = NULL;
	ACPI_PARSE_OBJECT       *prev = NULL;
	ACPI_PARSE_OBJECT       *field;
	u32                     subop;


	switch (arg_type) {
	case ARGP_BYTEDATA:
	case ARGP_WORDDATA:
	case ARGP_DWORDDATA:
	case ARGP_CHARLIST:
	case ARGP_NAME:
	case ARGP_NAMESTRING:

		/* constants, strings, and namestrings are all the same size */

		arg = acpi_ps_alloc_op (AML_BYTE_OP);
		if (arg) {
			acpi_ps_get_next_simple_arg (parser_state, arg_type, arg);
		}
		break;


	case ARGP_PKGLENGTH:

		/* package length, nothing returned */

		parser_state->pkg_end = acpi_ps_get_next_package_end (parser_state);
		break;


	case ARGP_FIELDLIST:

		if (parser_state->aml < parser_state->pkg_end) {
			/* non-empty list */

			while (parser_state->aml < parser_state->pkg_end) {
				field = acpi_ps_get_next_field (parser_state);
				if (!field) {
					break;
				}

				if (prev) {
					prev->next = field;
				}

				else {
					arg = field;
				}

				prev = field;
			}

			/* skip to End of byte data */

			parser_state->aml = parser_state->pkg_end;
		}
		break;


	case ARGP_BYTELIST:

		if (parser_state->aml < parser_state->pkg_end) {
			/* non-empty list */

			arg = acpi_ps_alloc_op (AML_BYTELIST_OP);
			if (arg) {
				/* fill in bytelist data */

				arg->value.size = (parser_state->pkg_end - parser_state->aml);
				((ACPI_PARSE2_OBJECT *) arg)->data = parser_state->aml;
			}

			/* skip to End of byte data */

			parser_state->aml = parser_state->pkg_end;
		}
		break;


	case ARGP_TARGET:
	case ARGP_SUPERNAME: {
			subop = acpi_ps_peek_opcode (parser_state);
			if (subop == 0              ||
				acpi_ps_is_leading_char (subop) ||
				acpi_ps_is_prefix_char (subop)) {
				/* Null_name or Name_string */

				arg = acpi_ps_alloc_op (AML_NAMEPATH_OP);
				if (arg) {
					acpi_ps_get_next_namepath (parser_state, arg, arg_count, 0);
				}
			}

			else {
				/* single complex argument, nothing returned */

				*arg_count = 1;
			}
		}
		break;


	case ARGP_DATAOBJ:
	case ARGP_TERMARG:

		/* single complex argument, nothing returned */

		*arg_count = 1;
		break;


	case ARGP_DATAOBJLIST:
	case ARGP_TERMLIST:
	case ARGP_OBJLIST:

		if (parser_state->aml < parser_state->pkg_end) {
			/* non-empty list of variable arguments, nothing returned */

			*arg_count = ACPI_VAR_ARGS;
		}
		break;
	}

	return (arg);
}
