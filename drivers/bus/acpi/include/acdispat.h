/******************************************************************************
 *
 * Name: acdispat.h - dispatcher (parser to interpreter interface)
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


#ifndef _ACDISPAT_H_
#define _ACDISPAT_H_


#define NAMEOF_LOCAL_NTE    "__L0"
#define NAMEOF_ARG_NTE      "__A0"


/* Common interfaces */

ACPI_STATUS
acpi_ds_obj_stack_push (
	void                    *object,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_obj_stack_pop (
	u32                     pop_count,
	ACPI_WALK_STATE         *walk_state);

void *
acpi_ds_obj_stack_get_value (
	u32                     index,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_obj_stack_pop_object (
	ACPI_OPERAND_OBJECT     **object,
	ACPI_WALK_STATE         *walk_state);


/* dsopcode - support for late evaluation */

ACPI_STATUS
acpi_ds_get_field_unit_arguments (
	ACPI_OPERAND_OBJECT     *obj_desc);

ACPI_STATUS
acpi_ds_get_region_arguments (
	ACPI_OPERAND_OBJECT     *rgn_desc);


/* dsctrl - Parser/Interpreter interface, control stack routines */


ACPI_STATUS
acpi_ds_exec_begin_control_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

ACPI_STATUS
acpi_ds_exec_end_control_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);


/* dsexec - Parser/Interpreter interface, method execution callbacks */


ACPI_STATUS
acpi_ds_get_predicate_value (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	u32                     has_result_obj);

ACPI_STATUS
acpi_ds_exec_begin_op (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op);

ACPI_STATUS
acpi_ds_exec_end_op (
	ACPI_WALK_STATE         *state,
	ACPI_PARSE_OBJECT       *op);


/* dsfield - Parser/Interpreter interface for AML fields */


ACPI_STATUS
acpi_ds_create_field (
	ACPI_PARSE_OBJECT       *op,
	ACPI_NAMESPACE_NODE     *region_node,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_create_bank_field (
	ACPI_PARSE_OBJECT       *op,
	ACPI_NAMESPACE_NODE     *region_node,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_create_index_field (
	ACPI_PARSE_OBJECT       *op,
	ACPI_HANDLE             region_node,
	ACPI_WALK_STATE         *walk_state);


/* dsload - Parser/Interpreter interface, namespace load callbacks */

ACPI_STATUS
acpi_ds_load1_begin_op (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op);

ACPI_STATUS
acpi_ds_load1_end_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

ACPI_STATUS
acpi_ds_load2_begin_op (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op);

ACPI_STATUS
acpi_ds_load2_end_op (
	ACPI_WALK_STATE         *state,
	ACPI_PARSE_OBJECT       *op);

ACPI_STATUS
acpi_ds_load3_begin_op (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op);

ACPI_STATUS
acpi_ds_load3_end_op (
	ACPI_WALK_STATE         *state,
	ACPI_PARSE_OBJECT       *op);


/* dsmthdat - method data (locals/args) */


ACPI_STATUS
acpi_ds_store_object_to_local (
	u16                     opcode,
	u32                     index,
	ACPI_OPERAND_OBJECT     *src_desc,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_method_data_get_entry (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     ***node);

ACPI_STATUS
acpi_ds_method_data_delete_all (
	ACPI_WALK_STATE         *walk_state);

u8
acpi_ds_is_method_value (
	ACPI_OPERAND_OBJECT     *obj_desc);

OBJECT_TYPE_INTERNAL
acpi_ds_method_data_get_type (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_method_data_get_value (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     **dest_desc);

ACPI_STATUS
acpi_ds_method_data_delete_value (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_method_data_init_args (
	ACPI_OPERAND_OBJECT     **params,
	u32                     max_param_count,
	ACPI_WALK_STATE         *walk_state);

ACPI_NAMESPACE_NODE *
acpi_ds_method_data_get_node (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_method_data_init (
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_method_data_set_entry (
	u16                     opcode,
	u32                     index,
	ACPI_OPERAND_OBJECT     *object,
	ACPI_WALK_STATE         *walk_state);


/* dsmethod - Parser/Interpreter interface - control method parsing */

ACPI_STATUS
acpi_ds_parse_method (
	ACPI_HANDLE             obj_handle);

ACPI_STATUS
acpi_ds_call_control_method (
	ACPI_WALK_LIST          *walk_list,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

ACPI_STATUS
acpi_ds_restart_control_method (
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     *return_desc);

ACPI_STATUS
acpi_ds_terminate_control_method (
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_begin_method_execution (
	ACPI_NAMESPACE_NODE     *method_node,
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_NAMESPACE_NODE     *calling_method_node);


/* dsobj - Parser/Interpreter interface - object initialization and conversion */

ACPI_STATUS
acpi_ds_init_one_object (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *context,
	void                    **return_value);

ACPI_STATUS
acpi_ds_initialize_objects (
	ACPI_TABLE_DESC         *table_desc,
	ACPI_NAMESPACE_NODE     *start_node);

ACPI_STATUS
acpi_ds_build_internal_package_obj (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_OPERAND_OBJECT     **obj_desc);

ACPI_STATUS
acpi_ds_build_internal_object (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_OPERAND_OBJECT     **obj_desc_ptr);

ACPI_STATUS
acpi_ds_init_object_from_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	u16                     opcode,
	ACPI_OPERAND_OBJECT     **obj_desc);

ACPI_STATUS
acpi_ds_create_node (
	ACPI_WALK_STATE         *walk_state,
	ACPI_NAMESPACE_NODE     *node,
	ACPI_PARSE_OBJECT       *op);


/* dsregn - Parser/Interpreter interface - Op Region parsing */

ACPI_STATUS
acpi_ds_eval_field_unit_operands (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

ACPI_STATUS
acpi_ds_eval_region_operands (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

ACPI_STATUS
acpi_ds_initialize_region (
	ACPI_HANDLE             obj_handle);


/* dsutils - Parser/Interpreter interface utility routines */

u8
acpi_ds_is_result_used (
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state);

void
acpi_ds_delete_result_if_not_used (
	ACPI_PARSE_OBJECT       *op,
	ACPI_OPERAND_OBJECT     *result_obj,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_create_operand (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *arg,
	u32                     args_remaining);

ACPI_STATUS
acpi_ds_create_operands (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *first_arg);

ACPI_STATUS
acpi_ds_resolve_operands (
	ACPI_WALK_STATE         *walk_state);

OBJECT_TYPE_INTERNAL
acpi_ds_map_opcode_to_data_type (
	u16                     opcode,
	u32                     *out_flags);

OBJECT_TYPE_INTERNAL
acpi_ds_map_named_opcode_to_data_type (
	u16                     opcode);


/*
 * dswscope - Scope Stack manipulation
 */

ACPI_STATUS
acpi_ds_scope_stack_push (
	ACPI_NAMESPACE_NODE     *node,
	OBJECT_TYPE_INTERNAL    type,
	ACPI_WALK_STATE         *walk_state);


ACPI_STATUS
acpi_ds_scope_stack_pop (
	ACPI_WALK_STATE         *walk_state);

void
acpi_ds_scope_stack_clear (
	ACPI_WALK_STATE         *walk_state);


/* Acpi_dswstate - parser WALK_STATE management routines */

ACPI_WALK_STATE *
acpi_ds_create_walk_state (
	ACPI_OWNER_ID           owner_id,
	ACPI_PARSE_OBJECT       *origin,
	ACPI_OPERAND_OBJECT     *mth_desc,
	ACPI_WALK_LIST          *walk_list);

ACPI_STATUS
acpi_ds_obj_stack_delete_all (
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_obj_stack_pop_and_delete (
	u32                     pop_count,
	ACPI_WALK_STATE         *walk_state);

void
acpi_ds_delete_walk_state (
	ACPI_WALK_STATE         *walk_state);

ACPI_WALK_STATE *
acpi_ds_pop_walk_state (
	ACPI_WALK_LIST          *walk_list);

ACPI_STATUS
acpi_ds_result_stack_pop (
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_result_stack_push (
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_result_stack_clear (
	ACPI_WALK_STATE         *walk_state);

ACPI_WALK_STATE *
acpi_ds_get_current_walk_state (
	ACPI_WALK_LIST          *walk_list);

void
acpi_ds_delete_walk_state_cache (
	void);

ACPI_STATUS
acpi_ds_result_insert (
	void                    *object,
	u32                     index,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_result_remove (
	ACPI_OPERAND_OBJECT     **object,
	u32                     index,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_result_pop (
	ACPI_OPERAND_OBJECT     **object,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_result_push (
	ACPI_OPERAND_OBJECT     *object,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ds_result_pop_from_bottom (
	ACPI_OPERAND_OBJECT     **object,
	ACPI_WALK_STATE         *walk_state);

#endif /* _ACDISPAT_H_ */
