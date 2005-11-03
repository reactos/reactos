/******************************************************************************
 *
 * Module Name: psparse - Parser top level AML parse routines
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


/*
 * Parse the AML and build an operation tree as most interpreters,
 * like Perl, do.  Parsing is done by hand rather than with a YACC
 * generated parser to tightly constrain stack and dynamic memory
 * usage.  At the same time, parsing is kept flexible and the code
 * fairly compact by parsing based on a list of AML opcode
 * templates in Aml_op_info[]
 */

#include "acpi.h"
#include "acparser.h"
#include "acdispat.h"
#include "amlcode.h"
#include "acnamesp.h"
#include "acdebug.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_PARSER
	 MODULE_NAME         ("psparse")


u32                         acpi_gbl_depth = 0;
extern u32                  acpi_gbl_scope_depth;


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_peek_opcode
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get next AML opcode (without incrementing AML pointer)
 *
 ******************************************************************************/

static u32
acpi_ps_get_opcode_size (
	u32                     opcode)
{

	/* Extended (2-byte) opcode if > 255 */

	if (opcode > 0x00FF) {
		return (2);
	}

	/* Otherwise, just a single byte opcode */

	return (1);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_peek_opcode
 *
 * PARAMETERS:  Parser_state        - A parser state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get next AML opcode (without incrementing AML pointer)
 *
 ******************************************************************************/

u16
acpi_ps_peek_opcode (
	ACPI_PARSE_STATE        *parser_state)
{
	u8                      *aml;
	u16                     opcode;


	aml = parser_state->aml;
	opcode = (u16) GET8 (aml);

	aml++;


	/*
	 * Original code special cased LNOTEQUAL, LLESSEQUAL, LGREATEREQUAL.
	 * These opcodes are no longer recognized. Instead, they are broken into
	 * two opcodes.
	 *
	 *
	 *    if (Opcode == AML_EXTOP
	 *       || (Opcode == AML_LNOT
	 *          && (GET8 (Acpi_aml) == AML_LEQUAL
	 *               || GET8 (Acpi_aml) == AML_LGREATER
	 *               || GET8 (Acpi_aml) == AML_LLESS)))
	 *
	 *     extended Opcode, !=, <=, or >=
	 */

	if (opcode == AML_EXTOP) {
		/* Extended opcode */

		opcode = (u16) ((opcode << 8) | GET8 (aml));
		aml++;
	}

	/* don't convert bare name to a namepath */

	return (opcode);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_create_state
 *
 * PARAMETERS:  Acpi_aml            - Acpi_aml code pointer
 *              Acpi_aml_size       - Length of AML code
 *
 * RETURN:      A new parser state object
 *
 * DESCRIPTION: Create and initialize a new parser state object
 *
 ******************************************************************************/

ACPI_PARSE_STATE *
acpi_ps_create_state (
	u8                      *aml,
	u32                     aml_size)
{
	ACPI_PARSE_STATE        *parser_state;


	parser_state = acpi_cm_callocate (sizeof (ACPI_PARSE_STATE));
	if (!parser_state) {
		return (NULL);
	}

	parser_state->aml      = aml;
	parser_state->aml_end  = aml + aml_size;
	parser_state->pkg_end  = parser_state->aml_end;
	parser_state->aml_start = aml;


	return (parser_state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_find_object
 *
 * PARAMETERS:  Opcode          - Current opcode
 *              Parser_state    - Current state
 *              Walk_state      - Current state
 *              *Op             - Where found/new op is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Find a named object.  Two versions - one to search the parse
 *              tree (for parser-only applications such as acpidump), another
 *              to search the ACPI internal namespace (the parse tree may no
 *              longer exist)
 *
 ******************************************************************************/

#ifdef PARSER_ONLY

ACPI_STATUS
acpi_ps_find_object (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op)
{
	NATIVE_CHAR             *path;


	/* We are only interested in opcodes that have an associated name */

	if (!acpi_ps_is_named_op (opcode)) {
		*out_op = op;
		return (AE_OK);
	}

	/* Find the name in the parse tree */

	path = acpi_ps_get_next_namestring (walk_state->parser_state);

	*out_op = acpi_ps_find (acpi_ps_get_parent_scope (walk_state->parser_state),
			  path, opcode, 1);

	if (!(*out_op)) {
		return (AE_NOT_FOUND);
	}

	return (AE_OK);
}

#endif


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_complete_this_op
 *
 * PARAMETERS:  Walk_state      - Current State
 *              Op              - Op to complete
 *
 * RETURN:      TRUE if Op and subtree was deleted
 *
 * DESCRIPTION: Perform any cleanup at the completion of an Op.
 *
 ******************************************************************************/

static u8
acpi_ps_complete_this_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op)
{
#ifndef PARSER_ONLY
	ACPI_PARSE_OBJECT       *prev;
	ACPI_PARSE_OBJECT       *next;
	ACPI_OPCODE_INFO        *op_info;
	ACPI_OPCODE_INFO        *parent_info;
	u32                     opcode_class;
	ACPI_PARSE_OBJECT       *replacement_op = NULL;


	op_info     = acpi_ps_get_opcode_info (op->opcode);
	opcode_class = ACPI_GET_OP_CLASS (op_info);


	/* Delete this op and the subtree below it if asked to */

	if (((walk_state->parse_flags & ACPI_PARSE_TREE_MASK) == ACPI_PARSE_DELETE_TREE) &&
		(opcode_class != OPTYPE_CONSTANT)       &&
		(opcode_class != OPTYPE_LITERAL)        &&
		(opcode_class != OPTYPE_LOCAL_VARIABLE) &&
		(opcode_class != OPTYPE_METHOD_ARGUMENT) &&
		(opcode_class != OPTYPE_DATA_TERM)      &&
		(op->opcode  != AML_NAMEPATH_OP)) {
		/* Make sure that we only delete this subtree */

		if (op->parent) {
			/*
			 * Check if we need to replace the operator and its subtree
			 * with a return value op (placeholder op)
			 */

			parent_info = acpi_ps_get_opcode_info (op->parent->opcode);

			switch (ACPI_GET_OP_CLASS (parent_info)) {
			case OPTYPE_CONTROL:        /* IF, ELSE, WHILE only */
				break;

			case OPTYPE_NAMED_OBJECT:   /* Scope, method, etc. */

				/*
				 * These opcodes contain Term_arg operands. The current
				 * op must be replace by a placeholder return op
				 */

				if ((op->parent->opcode == AML_REGION_OP)       ||
					(op->parent->opcode == AML_CREATE_FIELD_OP) ||
					(op->parent->opcode == AML_BIT_FIELD_OP)    ||
					(op->parent->opcode == AML_BYTE_FIELD_OP)   ||
					(op->parent->opcode == AML_WORD_FIELD_OP)   ||
					(op->parent->opcode == AML_DWORD_FIELD_OP)  ||
					(op->parent->opcode == AML_QWORD_FIELD_OP)) {
					replacement_op = acpi_ps_alloc_op (AML_RETURN_VALUE_OP);
					if (!replacement_op) {
						return (FALSE);
					}
				}

				break;

			default:
				replacement_op = acpi_ps_alloc_op (AML_RETURN_VALUE_OP);
				if (!replacement_op) {
					return (FALSE);
				}
			}

			/* We must unlink this op from the parent tree */

			prev = op->parent->value.arg;
			if (prev == op) {
				/* This op is the first in the list */

				if (replacement_op) {
					replacement_op->parent   = op->parent;
					replacement_op->value.arg = NULL;
					op->parent->value.arg    = replacement_op;
					replacement_op->next     = op->next;
				}
				else {
					op->parent->value.arg    = op->next;
				}
			}

			/* Search the parent list */

			else while (prev) {
				/* Traverse all siblings in the parent's argument list */

				next = prev->next;
				if (next == op) {
					if (replacement_op) {
						replacement_op->parent = op->parent;
						replacement_op->value.arg = NULL;
						prev->next = replacement_op;
						replacement_op->next = op->next;
						next = NULL;
					}
					else {
						prev->next = op->next;
						next = NULL;
					}
				}

				prev = next;
			}

		}

		/* Now we can actually delete the subtree rooted at op */

		acpi_ps_delete_parse_tree (op);

		return (TRUE);
	}

	return (FALSE);

#else
	return (FALSE);
#endif
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_next_parse_state
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_ps_next_parse_state (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_STATUS             callback_status)
{
	ACPI_PARSE_STATE        *parser_state = walk_state->parser_state;
	ACPI_STATUS             status = AE_CTRL_PENDING;
	u8                      *start;
	u32                     package_length;


	switch (callback_status) {
	case AE_CTRL_TERMINATE:

		/*
		 * A control method was terminated via a RETURN statement.
		 * The walk of this method is complete.
		 */

		parser_state->aml = parser_state->aml_end;
		status = AE_CTRL_TERMINATE;
		break;


	case AE_CTRL_PENDING:

			/*
			 * Predicate of a WHILE was true and the loop just completed an
			 * execution.  Go back to the start of the loop and reevaluate the
			 * predicate.
			 */
/*            Walk_state->Control_state->Common.State =
					CONTROL_PREDICATE_EXECUTING;*/

		/* TBD: How to handle a break within a while. */
		/* This code attempts it */

		parser_state->aml = walk_state->aml_last_while;
		break;


	case AE_CTRL_TRUE:
			/*
			 * Predicate of an IF was true, and we are at the matching ELSE.
			 * Just close out this package
			 *
			 * Note: Parser_state->Aml is modified by the package length procedure
			 * TBD: [Investigate] perhaps it shouldn't, too much trouble
			 */
		start = parser_state->aml;
		package_length = acpi_ps_get_next_package_length (parser_state);
		parser_state->aml = start + package_length;
		break;


	case AE_CTRL_FALSE:

		/*
		 * Either an IF/WHILE Predicate was false or we encountered a BREAK
		 * opcode.  In both cases, we do not execute the rest of the
		 * package;  We simply close out the parent (finishing the walk of
		 * this branch of the tree) and continue execution at the parent
		 * level.
		 */

		parser_state->aml = parser_state->scope->parse_scope.pkg_end;

		/* In the case of a BREAK, just force a predicate (if any) to FALSE */

		walk_state->control_state->common.value = FALSE;
		status = AE_CTRL_END;
		break;


	case AE_CTRL_TRANSFER:

		/*
		 * A method call (invocation) -- transfer control
		 */
		status = AE_CTRL_TRANSFER;
		walk_state->prev_op = op;
		walk_state->method_call_op = op;
		walk_state->method_call_node = (op->value.arg)->node;

		/* Will return value (if any) be used by the caller? */

		walk_state->return_used = acpi_ds_is_result_used (op, walk_state);
		break;


	default:
		status = callback_status;
		if ((callback_status & AE_CODE_MASK) == AE_CODE_CONTROL) {
			status = AE_OK;
		}
		break;
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_parse_loop
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse AML (pointed to by the current parser state) and return
 *              a tree of ops.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ps_parse_loop (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_PARSE_OBJECT       *op = NULL;     /* current op */
	ACPI_OPCODE_INFO        *op_info;
	ACPI_PARSE_OBJECT       *arg = NULL;
	ACPI_PARSE2_OBJECT      *deferred_op;
	u32                     arg_count;      /* push for fixed or var args */
	u32                     arg_types = 0;
	ACPI_PTRDIFF            aml_offset;
	u16                     opcode;
	ACPI_PARSE_OBJECT       pre_op;
	ACPI_PARSE_STATE        *parser_state;
	u8                      *aml_op_start;


	parser_state = walk_state->parser_state;

#ifndef PARSER_ONLY
	if (walk_state->walk_type & WALK_METHOD_RESTART) {
		/* We are restarting a preempted control method */

		if (acpi_ps_has_completed_scope (parser_state)) {
			/*
			 * We must check if a predicate to an IF or WHILE statement
			 * was just completed
			 */
			if ((parser_state->scope->parse_scope.op) &&
				((parser_state->scope->parse_scope.op->opcode == AML_IF_OP) ||
				(parser_state->scope->parse_scope.op->opcode == AML_WHILE_OP)) &&
				(walk_state->control_state) &&
				(walk_state->control_state->common.state ==
					CONTROL_PREDICATE_EXECUTING)) {

				/*
				 * A predicate was just completed, get the value of the
				 * predicate and branch based on that value
				 */

				status = acpi_ds_get_predicate_value (walk_state, NULL, TRUE);
				if (ACPI_FAILURE (status) &&
					((status & AE_CODE_MASK) != AE_CODE_CONTROL)) {
					return (status);
				}

				status = acpi_ps_next_parse_state (walk_state, op, status);
			}

			acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);
		}

		else if (walk_state->prev_op) {
			/* We were in the middle of an op */

			op = walk_state->prev_op;
			arg_types = walk_state->prev_arg_types;
		}
	}
#endif

	/*
	 * Iterative parsing loop, while there is more aml to process:
	 */
	while ((parser_state->aml < parser_state->aml_end) || (op)) {
		if (!op) {
			/* Get the next opcode from the AML stream */

			aml_op_start = parser_state->aml;
			aml_offset = parser_state->aml - parser_state->aml_start;
			opcode     = acpi_ps_peek_opcode (parser_state);

			/*
			 * First cut to determine what we have found:
			 * 1) A valid AML opcode
			 * 2) A name string
			 * 3) An unknown/invalid opcode
			 */

			op_info = acpi_ps_get_opcode_info (opcode);
			switch (ACPI_GET_OP_TYPE (op_info)) {
			case ACPI_OP_TYPE_OPCODE:

				/* Found opcode info, this is a normal opcode */

				parser_state->aml += acpi_ps_get_opcode_size (opcode);
				arg_types = op_info->parse_args;
				break;

			case ACPI_OP_TYPE_ASCII:
			case ACPI_OP_TYPE_PREFIX:
				/*
				 * Starts with a valid prefix or ASCII char, this is a name
				 * string.  Convert the bare name string to a namepath.
				 */

				opcode = AML_NAMEPATH_OP;
				arg_types = ARGP_NAMESTRING;
				break;

			case ACPI_OP_TYPE_UNKNOWN:

				/* The opcode is unrecognized.  Just skip unknown opcodes */

				/* Assume one-byte bad opcode */

				parser_state->aml++;
				continue;
			}


			/* Create Op structure and append to parent's argument list */

			if (acpi_ps_is_named_op (opcode)) {
				pre_op.value.arg = NULL;
				pre_op.opcode = opcode;

				while (GET_CURRENT_ARG_TYPE (arg_types) != ARGP_NAME) {
					arg = acpi_ps_get_next_arg (parser_state,
							 GET_CURRENT_ARG_TYPE (arg_types),
							 &arg_count);
					acpi_ps_append_arg (&pre_op, arg);
					INCREMENT_ARG_LIST (arg_types);
				}


				/* We know that this arg is a name, move to next arg */

				INCREMENT_ARG_LIST (arg_types);

				if (walk_state->descending_callback != NULL) {
					/*
					 * Find the object.  This will either insert the object into
					 * the namespace or simply look it up
					 */
					status = walk_state->descending_callback (opcode, NULL, walk_state, &op);
					if (op == NULL) {
						continue;
					}
					status = acpi_ps_next_parse_state (walk_state, op, status);
					if (status == AE_CTRL_PENDING) {
						status = AE_OK;
						goto close_this_op;
					}

					if (ACPI_FAILURE (status)) {
						goto close_this_op;
					}
				}

				acpi_ps_append_arg (op, pre_op.value.arg);
				acpi_gbl_depth++;


				if (op->opcode == AML_REGION_OP) {
					deferred_op = acpi_ps_to_extended_op (op);
					if (deferred_op) {
						/*
						 * Defer final parsing of an Operation_region body,
						 * because we don't have enough info in the first pass
						 * to parse it correctly (i.e., there may be method
						 * calls within the Term_arg elements of the body.
						 *
						 * However, we must continue parsing because
						 * the opregion is not a standalone package --
						 * we don't know where the end is at this point.
						 *
						 * (Length is unknown until parse of the body complete)
						 */

						deferred_op->data   = aml_op_start;
						deferred_op->length = 0;
					}
				}
			}


			else {
				/* Not a named opcode, just allocate Op and append to parent */

				op = acpi_ps_alloc_op (opcode);
				if (!op) {
					return (AE_NO_MEMORY);
				}


				if ((op->opcode == AML_CREATE_FIELD_OP) ||
					(op->opcode == AML_BIT_FIELD_OP)    ||
					(op->opcode == AML_BYTE_FIELD_OP)   ||
					(op->opcode == AML_WORD_FIELD_OP)   ||
					(op->opcode == AML_DWORD_FIELD_OP)) {
					/*
					 * Backup to beginning of Create_xXXfield declaration
					 * Body_length is unknown until we parse the body
					 */
					deferred_op = (ACPI_PARSE2_OBJECT *) op;

					deferred_op->data   = aml_op_start;
					deferred_op->length = 0;
				}

				acpi_ps_append_arg (acpi_ps_get_parent_scope (parser_state), op);

				if ((walk_state->descending_callback != NULL)) {
					/*
					 * Find the object.  This will either insert the object into
					 * the namespace or simply look it up
					 */
					status = walk_state->descending_callback (opcode, op, walk_state, &op);
					status = acpi_ps_next_parse_state (walk_state, op, status);
					if (status == AE_CTRL_PENDING) {
						status = AE_OK;
						goto close_this_op;
					}

					if (ACPI_FAILURE (status)) {
						goto close_this_op;
					}
				}
			}

			op->aml_offset = aml_offset;

		}


		/* Start Arg_count at zero because we don't know if there are any args yet */

		arg_count = 0;


		if (arg_types)  /* Are there any arguments that must be processed? */ {
			/* get arguments */

			switch (op->opcode) {
			case AML_BYTE_OP:       /* AML_BYTEDATA_ARG */
			case AML_WORD_OP:       /* AML_WORDDATA_ARG */
			case AML_DWORD_OP:      /* AML_DWORDATA_ARG */
			case AML_STRING_OP:     /* AML_ASCIICHARLIST_ARG */

				/* fill in constant or string argument directly */

				acpi_ps_get_next_simple_arg (parser_state,
						 GET_CURRENT_ARG_TYPE (arg_types), op);
				break;

			case AML_NAMEPATH_OP:   /* AML_NAMESTRING_ARG */

				acpi_ps_get_next_namepath (parser_state, op, &arg_count, 1);
				arg_types = 0;
				break;


			default:

				/* Op is not a constant or string, append each argument */

				while (GET_CURRENT_ARG_TYPE (arg_types) && !arg_count) {
					aml_offset = parser_state->aml - parser_state->aml_start;
					arg = acpi_ps_get_next_arg (parser_state,
							 GET_CURRENT_ARG_TYPE (arg_types),
							 &arg_count);
					if (arg) {
						arg->aml_offset = aml_offset;
						acpi_ps_append_arg (op, arg);
					}

					INCREMENT_ARG_LIST (arg_types);
				}


				/* For a method, save the length and address of the body */

				if (op->opcode == AML_METHOD_OP) {
					deferred_op = acpi_ps_to_extended_op (op);
					if (deferred_op) {
						/*
						 * Skip parsing of control method or opregion body,
						 * because we don't have enough info in the first pass
						 * to parse them correctly.
						 */

						deferred_op->data   = parser_state->aml;
						deferred_op->length = parser_state->pkg_end -
								  parser_state->aml;

						/*
						 * Skip body of method.  For Op_regions, we must continue
						 * parsing because the opregion is not a standalone
						 * package (We don't know where the end is).
						 */
						parser_state->aml   = parser_state->pkg_end;
						arg_count           = 0;
					}
				}

				break;
			}
		}


		/*
		 * Zero Arg_count means that all arguments for this op have been processed
		 */
		if (!arg_count) {
			/* completed Op, prepare for next */

			if (acpi_ps_is_named_op (op->opcode)) {
				if (acpi_gbl_depth) {
					acpi_gbl_depth--;
				}

				if (op->opcode == AML_REGION_OP) {
					deferred_op = acpi_ps_to_extended_op (op);
					if (deferred_op) {
						/*
						 * Skip parsing of control method or opregion body,
						 * because we don't have enough info in the first pass
						 * to parse them correctly.
						 *
						 * Completed parsing an Op_region declaration, we now
						 * know the length.
						 */

						deferred_op->length = parser_state->aml -
								 deferred_op->data;
					}
				}
			}

			if ((op->opcode == AML_CREATE_FIELD_OP) ||
				(op->opcode == AML_BIT_FIELD_OP)    ||
				(op->opcode == AML_BYTE_FIELD_OP)   ||
				(op->opcode == AML_WORD_FIELD_OP)   ||
				(op->opcode == AML_DWORD_FIELD_OP)  ||
				(op->opcode == AML_QWORD_FIELD_OP)) {
				/*
				 * Backup to beginning of Create_xXXfield declaration (1 for
				 * Opcode)
				 *
				 * Body_length is unknown until we parse the body
				 */
				deferred_op = (ACPI_PARSE2_OBJECT *) op;
				deferred_op->length = parser_state->aml - deferred_op->data;
			}

			/* This op complete, notify the dispatcher */

			if (walk_state->ascending_callback != NULL) {
				status = walk_state->ascending_callback (walk_state, op);
				status = acpi_ps_next_parse_state (walk_state, op, status);
				if (status == AE_CTRL_PENDING) {
					status = AE_OK;
					goto close_this_op;
				}
			}


close_this_op:

			/*
			 * Finished one argument of the containing scope
			 */
			parser_state->scope->parse_scope.arg_count--;

			/* Close this Op (may result in parse subtree deletion) */

			if (acpi_ps_complete_this_op (walk_state, op)) {
				op = NULL;
			}


			switch (status) {
			case AE_OK:
				break;


			case AE_CTRL_TRANSFER:

				/*
				 * We are about to transfer to a called method.
				 */
				walk_state->prev_op = op;
				walk_state->prev_arg_types = arg_types;
				return (status);
				break;


			case AE_CTRL_END:

				acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);

				status = walk_state->ascending_callback (walk_state, op);
				status = acpi_ps_next_parse_state (walk_state, op, status);

				acpi_ps_complete_this_op (walk_state, op);
				op = NULL;
				status = AE_OK;
				break;


			case AE_CTRL_TERMINATE:

				status = AE_OK;

				/* Clean up */
				do {
					if (op) {
						acpi_ps_complete_this_op (walk_state, op);
					}

					acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);
				} while (op);

				return (status);
				break;


			default:  /* All other non-AE_OK status */

				if (op == NULL) {
					acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);
				}
				walk_state->prev_op = op;
				walk_state->prev_arg_types = arg_types;

				/*
				 * TEMP:
				 */

				return (status);
				break;
			}


			/* This scope complete? */

			if (acpi_ps_has_completed_scope (parser_state)) {
				acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);
			}

			else {
				op = NULL;
			}

		}


		/* Arg_count is non-zero */

		else {
			/* complex argument, push Op and prepare for argument */

			acpi_ps_push_scope (parser_state, op, arg_types, arg_count);
			op = NULL;
		}

	} /* while Parser_state->Aml */


	/*
	 * Complete the last Op (if not completed), and clear the scope stack.
	 * It is easily possible to end an AML "package" with an unbounded number
	 * of open scopes (such as when several AML blocks are closed with
	 * sequential closing braces).  We want to terminate each one cleanly.
	 */

	do {
		if (op) {
			if (walk_state->ascending_callback != NULL) {
				status = walk_state->ascending_callback (walk_state, op);
				status = acpi_ps_next_parse_state (walk_state, op, status);
				if (status == AE_CTRL_PENDING) {
					status = AE_OK;
					goto close_this_op;
				}

				if (status == AE_CTRL_TERMINATE) {
					status = AE_OK;

					/* Clean up */
					do {
						if (op) {
							acpi_ps_complete_this_op (walk_state, op);
						}

						acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);

					} while (op);

					return (status);
				}

				else if (ACPI_FAILURE (status)) {
					acpi_ps_complete_this_op (walk_state, op);
					return (status);
				}
			}

			acpi_ps_complete_this_op (walk_state, op);
		}

		acpi_ps_pop_scope (parser_state, &op, &arg_types, &arg_count);

	} while (op);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_parse_aml
 *
 * PARAMETERS:  Start_scope     - The starting point of the parse.  Becomes the
 *                                root of the parsed op tree.
 *              Aml             - Pointer to the raw AML code to parse
 *              Aml_size        - Length of the AML to parse
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse raw AML and return a tree of ops
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ps_parse_aml (
	ACPI_PARSE_OBJECT       *start_scope,
	u8                      *aml,
	u32                     aml_size,
	u32                     parse_flags,
	ACPI_NAMESPACE_NODE     *method_node,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **caller_return_desc,
	ACPI_PARSE_DOWNWARDS    descending_callback,
	ACPI_PARSE_UPWARDS      ascending_callback)
{
	ACPI_STATUS             status;
	ACPI_PARSE_STATE        *parser_state;
	ACPI_WALK_STATE         *walk_state;
	ACPI_WALK_LIST          walk_list;
	ACPI_NAMESPACE_NODE     *node = NULL;
	ACPI_WALK_LIST          *prev_walk_list = acpi_gbl_current_walk_list;
	ACPI_OPERAND_OBJECT     *return_desc;
	ACPI_OPERAND_OBJECT     *mth_desc = NULL;


	/* Create and initialize a new parser state */

	parser_state = acpi_ps_create_state (aml, aml_size);
	if (!parser_state) {
		return (AE_NO_MEMORY);
	}

	acpi_ps_init_scope (parser_state, start_scope);

	if (method_node) {
		mth_desc = acpi_ns_get_attached_object (method_node);
	}

	/* Create and initialize a new walk list */

	walk_list.walk_state = NULL;
	walk_list.acquired_mutex_list.prev = NULL;
	walk_list.acquired_mutex_list.next = NULL;

	walk_state = acpi_ds_create_walk_state (TABLE_ID_DSDT, parser_state->start_op, mth_desc, &walk_list);
	if (!walk_state) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	walk_state->method_node         = method_node;
	walk_state->parser_state        = parser_state;
	walk_state->parse_flags         = parse_flags;
	walk_state->descending_callback = descending_callback;
	walk_state->ascending_callback  = ascending_callback;

	/* TBD: [Restructure] TEMP until we pass Walk_state to the interpreter
	 */
	acpi_gbl_current_walk_list = &walk_list;


	if (method_node) {
		parser_state->start_node = method_node;
		walk_state->walk_type   = WALK_METHOD;

		/* Push start scope on scope stack and make it current  */

		status = acpi_ds_scope_stack_push (method_node, ACPI_TYPE_METHOD, walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/* Init arguments if this is a control method */
		/* TBD: [Restructure] add walkstate as a param */

		acpi_ds_method_data_init_args (params, MTH_NUM_ARGS, walk_state);
	}

	else {
		/* Setup the current scope */

		node = parser_state->start_op->node;
		parser_state->start_node = node;

		if (node) {
			/* Push start scope on scope stack and make it current  */

			status = acpi_ds_scope_stack_push (node, node->type,
					   walk_state);
			if (ACPI_FAILURE (status)) {
				goto cleanup;
			}

		}
	}


	status = AE_OK;

	/*
	 * Execute the walk loop as long as there is a valid Walk State.  This
	 * handles nested control method invocations without recursion.
	 */

	while (walk_state) {
		if (ACPI_SUCCESS (status)) {
			status = acpi_ps_parse_loop (walk_state);
		}

		if (status == AE_CTRL_TRANSFER) {
			/*
			 * A method call was detected.
			 * Transfer control to the called control method
			 */

			status = acpi_ds_call_control_method (&walk_list, walk_state, NULL);

			/*
			 * If the transfer to the new method method call worked, a new walk
			 * state was created -- get it
			 */

			walk_state = acpi_ds_get_current_walk_state (&walk_list);
			continue;
		}

		else if (status == AE_CTRL_TERMINATE) {
			status = AE_OK;
		}

		/* We are done with this walk, move on to the parent if any */


		walk_state = acpi_ds_pop_walk_state (&walk_list);

		/* Extract return value before we delete Walk_state */

		return_desc = walk_state->return_desc;

		/* Reset the current scope to the beginning of scope stack */

		acpi_ds_scope_stack_clear (walk_state);

		/*
		 * If we just returned from the execution of a control method,
		 * there's lots of cleanup to do
		 */

		if ((walk_state->parse_flags & ACPI_PARSE_MODE_MASK) == ACPI_PARSE_EXECUTE) {
			acpi_ds_terminate_control_method (walk_state);
		}

		 /* Delete this walk state and all linked control states */

		acpi_ps_cleanup_scope (walk_state->parser_state);
		acpi_cm_free (walk_state->parser_state);
		acpi_ds_delete_walk_state (walk_state);

	   /* Check if we have restarted a preempted walk */

		walk_state = acpi_ds_get_current_walk_state (&walk_list);
		if (walk_state &&
			ACPI_SUCCESS (status)) {
			/* There is another walk state, restart it */

			/*
			 * If the method returned value is not used by the parent,
			 * The object is deleted
			 */

			acpi_ds_restart_control_method (walk_state, return_desc);
			walk_state->walk_type |= WALK_METHOD_RESTART;
		}

		/*
		 * Just completed a 1st-level method, save the final internal return
		 * value (if any)
		 */

		else if (caller_return_desc) {
			*caller_return_desc = return_desc; /* NULL if no return value */
		}

		else if (return_desc) {
			/* Caller doesn't want it, must delete it */

			acpi_cm_remove_reference (return_desc);
		}
	}


	/* Normal exit */

	acpi_aml_release_all_mutexes ((ACPI_OPERAND_OBJECT *) &walk_list.acquired_mutex_list);
	acpi_gbl_current_walk_list = prev_walk_list;
	return (status);


cleanup:

	/* Cleanup */

	acpi_ds_delete_walk_state (walk_state);
	acpi_ps_cleanup_scope (parser_state);
	acpi_cm_free (parser_state);

	acpi_aml_release_all_mutexes ((ACPI_OPERAND_OBJECT *)&walk_list.acquired_mutex_list);
	acpi_gbl_current_walk_list = prev_walk_list;

	return (status);
}


