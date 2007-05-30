/******************************************************************************
 *
 * Module Name: acparser.h - AML Parser subcomponent prototypes and defines
 *       $Revision: 1.1 $
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


#ifndef __ACPARSER_H__
#define __ACPARSER_H__


#define OP_HAS_RETURN_VALUE         1

/* variable # arguments */

#define ACPI_VAR_ARGS               ACPI_UINT32_MAX

/* maximum virtual address */

#define ACPI_MAX_AML                ((u8 *)(~0UL))


#define ACPI_PARSE_DELETE_TREE          0x0001
#define ACPI_PARSE_NO_TREE_DELETE       0x0000
#define ACPI_PARSE_TREE_MASK            0x0001

#define ACPI_PARSE_LOAD_PASS1           0x0010
#define ACPI_PARSE_LOAD_PASS2           0x0020
#define ACPI_PARSE_EXECUTE              0x0030
#define ACPI_PARSE_MODE_MASK            0x0030

/* psapi - Parser external interfaces */

ACPI_STATUS
acpi_psx_load_table (
	u8                      *pcode_addr,
	u32                     pcode_length);

ACPI_STATUS
acpi_psx_execute (
	ACPI_NAMESPACE_NODE     *method_node,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **return_obj_desc);


u8
acpi_ps_is_namespace_object_op (
	u16                     opcode);
u8
acpi_ps_is_namespace_op (
	u16                     opcode);


/******************************************************************************
 *
 * Parser interfaces
 *
 *****************************************************************************/


/* psargs - Parse AML opcode arguments */

u8 *
acpi_ps_get_next_package_end (
	ACPI_PARSE_STATE        *parser_state);

u32
acpi_ps_get_next_package_length (
	ACPI_PARSE_STATE        *parser_state);

NATIVE_CHAR *
acpi_ps_get_next_namestring (
	ACPI_PARSE_STATE        *parser_state);

void
acpi_ps_get_next_simple_arg (
	ACPI_PARSE_STATE        *parser_state,
	u32                     arg_type,       /* type of argument */
	ACPI_PARSE_OBJECT       *arg);           /* (OUT) argument data */

void
acpi_ps_get_next_namepath (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *arg,
	u32                     *arg_count,
	u8                      method_call);

ACPI_PARSE_OBJECT *
acpi_ps_get_next_field (
	ACPI_PARSE_STATE        *parser_state);

ACPI_PARSE_OBJECT *
acpi_ps_get_next_arg (
	ACPI_PARSE_STATE        *parser_state,
	u32                     arg_type,
	u32                     *arg_count);


/* psopcode - AML Opcode information */

ACPI_OPCODE_INFO *
acpi_ps_get_opcode_info (
	u16                     opcode);

NATIVE_CHAR *
acpi_ps_get_opcode_name (
	u16                     opcode);


/* psparse - top level parsing routines */

ACPI_STATUS
acpi_ps_find_object (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op);

void
acpi_ps_delete_parse_tree (
	ACPI_PARSE_OBJECT       *root);

ACPI_STATUS
acpi_ps_parse_loop (
	ACPI_WALK_STATE         *walk_state);

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
	ACPI_PARSE_UPWARDS      ascending_callback);

ACPI_STATUS
acpi_ps_parse_table (
	u8                      *aml,
	u32                     aml_size,
	ACPI_PARSE_DOWNWARDS    descending_callback,
	ACPI_PARSE_UPWARDS      ascending_callback,
	ACPI_PARSE_OBJECT       **root_object);

u16
acpi_ps_peek_opcode (
	ACPI_PARSE_STATE        *state);


/* psscope - Scope stack management routines */


ACPI_STATUS
acpi_ps_init_scope (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *root);

ACPI_PARSE_OBJECT *
acpi_ps_get_parent_scope (
	ACPI_PARSE_STATE        *state);

u8
acpi_ps_has_completed_scope (
	ACPI_PARSE_STATE        *parser_state);

void
acpi_ps_pop_scope (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       **op,
	u32                     *arg_list,
	u32                     *arg_count);

ACPI_STATUS
acpi_ps_push_scope (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *op,
	u32                     remaining_args,
	u32                     arg_count);

void
acpi_ps_cleanup_scope (
	ACPI_PARSE_STATE        *state);


/* pstree - parse tree manipulation routines */

void
acpi_ps_append_arg(
	ACPI_PARSE_OBJECT       *op,
	ACPI_PARSE_OBJECT       *arg);

ACPI_PARSE_OBJECT*
acpi_ps_find (
	ACPI_PARSE_OBJECT       *scope,
	NATIVE_CHAR             *path,
	u16                     opcode,
	u32                     create);

ACPI_PARSE_OBJECT *
acpi_ps_get_arg(
	ACPI_PARSE_OBJECT       *op,
	u32                      argn);

ACPI_PARSE_OBJECT *
acpi_ps_get_child (
	ACPI_PARSE_OBJECT       *op);

ACPI_PARSE_OBJECT *
acpi_ps_get_depth_next (
	ACPI_PARSE_OBJECT       *origin,
	ACPI_PARSE_OBJECT       *op);


/* pswalk - parse tree walk routines */

ACPI_STATUS
acpi_ps_walk_parsed_aml (
	ACPI_PARSE_OBJECT       *start_op,
	ACPI_PARSE_OBJECT       *end_op,
	ACPI_OPERAND_OBJECT     *mth_desc,
	ACPI_NAMESPACE_NODE     *start_node,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **caller_return_desc,
	ACPI_OWNER_ID           owner_id,
	ACPI_PARSE_DOWNWARDS    descending_callback,
	ACPI_PARSE_UPWARDS      ascending_callback);

ACPI_STATUS
acpi_ps_get_next_walk_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_PARSE_UPWARDS      ascending_callback);


/* psutils - parser utilities */


ACPI_PARSE_STATE *
acpi_ps_create_state (
	u8                      *aml,
	u32                     aml_size);

void
acpi_ps_init_op (
	ACPI_PARSE_OBJECT       *op,
	u16                     opcode);

ACPI_PARSE_OBJECT *
acpi_ps_alloc_op (
	u16                     opcode);

void
acpi_ps_free_op (
	ACPI_PARSE_OBJECT       *op);

void
acpi_ps_delete_parse_cache (
	void);

u8
acpi_ps_is_leading_char (
	u32                     c);

u8
acpi_ps_is_prefix_char (
	u32                     c);

u8
acpi_ps_is_named_op (
	u16                     opcode);

u8
acpi_ps_is_node_op (
	u16                     opcode);

u8
acpi_ps_is_deferred_op (
	u16                     opcode);

u8
acpi_ps_is_bytelist_op(
	u16                     opcode);

u8
acpi_ps_is_field_op(
	u16                     opcode);

u8
acpi_ps_is_create_field_op (
	u16                     opcode);

ACPI_PARSE2_OBJECT*
acpi_ps_to_extended_op(
	ACPI_PARSE_OBJECT       *op);

u32
acpi_ps_get_name(
	ACPI_PARSE_OBJECT       *op);

void
acpi_ps_set_name(
	ACPI_PARSE_OBJECT       *op,
	u32                     name);


/* psdump - display parser tree */

u32
acpi_ps_sprint_path (
	NATIVE_CHAR             *buffer_start,
	u32                     buffer_size,
	ACPI_PARSE_OBJECT       *op);

u32
acpi_ps_sprint_op (
	NATIVE_CHAR             *buffer_start,
	u32                     buffer_size,
	ACPI_PARSE_OBJECT       *op);

void
acpi_ps_show (
	ACPI_PARSE_OBJECT       *op);


#endif /* __ACPARSER_H__ */
