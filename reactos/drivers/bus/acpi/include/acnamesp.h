/******************************************************************************
 *
 * Name: acnamesp.h - Namespace subcomponent prototypes and defines
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

#ifndef __ACNAMESP_H__
#define __ACNAMESP_H__


/* To search the entire name space, pass this as Search_base */

#define NS_ALL                  ((ACPI_HANDLE)0)

/*
 * Elements of Acpi_ns_properties are bit significant
 * and should be one-to-one with values of ACPI_OBJECT_TYPE
 */
#define NSP_NORMAL              0
#define NSP_NEWSCOPE            1   /* a definition of this type opens a name scope */
#define NSP_LOCAL               2   /* suppress search of enclosing scopes */


/* Definitions of the predefined namespace names  */

#define ACPI_UNKNOWN_NAME       (u32) 0x3F3F3F3F     /* Unknown name is  "????" */
#define ACPI_ROOT_NAME          (u32) 0x2F202020     /* Root name is     "/   " */
#define ACPI_SYS_BUS_NAME       (u32) 0x5F53425F     /* Sys bus name is  "_SB_" */

#define NS_ROOT_PATH            "/"
#define NS_SYSTEM_BUS           "_SB_"


/* Flags for Acpi_ns_lookup, Acpi_ns_search_and_enter */

#define NS_NO_UPSEARCH          0
#define NS_SEARCH_PARENT        0x01
#define NS_DONT_OPEN_SCOPE      0x02
#define NS_NO_PEER_SEARCH       0x04
#define NS_ERROR_IF_FOUND       0x08

#define NS_WALK_UNLOCK          TRUE
#define NS_WALK_NO_UNLOCK       FALSE


ACPI_STATUS
acpi_ns_load_namespace (
	void);

ACPI_STATUS
acpi_ns_initialize_objects (
	void);

ACPI_STATUS
acpi_ns_initialize_devices (
	void);


/* Namespace init - nsxfinit */

ACPI_STATUS
acpi_ns_init_one_device (
	ACPI_HANDLE             obj_handle,
	u32                     nesting_level,
	void                    *context,
	void                    **return_value);

ACPI_STATUS
acpi_ns_init_one_object (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *context,
	void                    **return_value);


ACPI_STATUS
acpi_ns_walk_namespace (
	OBJECT_TYPE_INTERNAL    type,
	ACPI_HANDLE             start_object,
	u32                     max_depth,
	u8                      unlock_before_callback,
	WALK_CALLBACK           user_function,
	void                    *context,
	void                    **return_value);


ACPI_NAMESPACE_NODE *
acpi_ns_get_next_object (
	OBJECT_TYPE_INTERNAL    type,
	ACPI_NAMESPACE_NODE     *parent,
	ACPI_NAMESPACE_NODE     *child);


ACPI_STATUS
acpi_ns_delete_namespace_by_owner (
	u16                     table_id);


/* Namespace loading - nsload */

ACPI_STATUS
acpi_ns_one_complete_parse (
	u32                     pass_number,
	ACPI_TABLE_DESC         *table_desc);

ACPI_STATUS
acpi_ns_parse_table (
	ACPI_TABLE_DESC         *table_desc,
	ACPI_NAMESPACE_NODE     *scope);

ACPI_STATUS
acpi_ns_load_table (
	ACPI_TABLE_DESC         *table_desc,
	ACPI_NAMESPACE_NODE     *node);

ACPI_STATUS
acpi_ns_load_table_by_type (
	ACPI_TABLE_TYPE         table_type);


/*
 * Top-level namespace access - nsaccess
 */


ACPI_STATUS
acpi_ns_root_initialize (
	void);

ACPI_STATUS
acpi_ns_lookup (
	ACPI_GENERIC_STATE      *scope_info,
	NATIVE_CHAR             *name,
	OBJECT_TYPE_INTERNAL    type,
	OPERATING_MODE          interpreter_mode,
	u32                     flags,
	ACPI_WALK_STATE         *walk_state,
	ACPI_NAMESPACE_NODE     **ret_node);


/*
 * Named object allocation/deallocation - nsalloc
 */


ACPI_NAMESPACE_NODE *
acpi_ns_create_node (
	u32                     acpi_name);

void
acpi_ns_delete_node (
	ACPI_NAMESPACE_NODE     *node);

ACPI_STATUS
acpi_ns_delete_namespace_subtree (
	ACPI_NAMESPACE_NODE     *parent_handle);

void
acpi_ns_detach_object (
	ACPI_NAMESPACE_NODE     *node);

void
acpi_ns_delete_children (
	ACPI_NAMESPACE_NODE     *parent);


/*
 * Namespace modification - nsmodify
 */

ACPI_STATUS
acpi_ns_unload_namespace (
	ACPI_HANDLE             handle);

ACPI_STATUS
acpi_ns_delete_subtree (
	ACPI_HANDLE             start_handle);


/*
 * Namespace dump/print utilities - nsdump
 */

void
acpi_ns_dump_tables (
	ACPI_HANDLE             search_base,
	u32                     max_depth);

void
acpi_ns_dump_entry (
	ACPI_HANDLE             handle,
	u32                     debug_level);

ACPI_STATUS
acpi_ns_dump_pathname (
	ACPI_HANDLE             handle,
	NATIVE_CHAR             *msg,
	u32                     level,
	u32                     component);

void
acpi_ns_dump_root_devices (
	void);

void
acpi_ns_dump_objects (
	OBJECT_TYPE_INTERNAL    type,
	u32                     max_depth,
	u32                     ownder_id,
	ACPI_HANDLE             start_handle);


/*
 * Namespace evaluation functions - nseval
 */

ACPI_STATUS
acpi_ns_evaluate_by_handle (
	ACPI_NAMESPACE_NODE     *prefix_node,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **return_object);

ACPI_STATUS
acpi_ns_evaluate_by_name (
	NATIVE_CHAR             *pathname,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **return_object);

ACPI_STATUS
acpi_ns_evaluate_relative (
	ACPI_NAMESPACE_NODE     *prefix_node,
	NATIVE_CHAR             *pathname,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **return_object);

ACPI_STATUS
acpi_ns_execute_control_method (
	ACPI_NAMESPACE_NODE     *method_node,
	ACPI_OPERAND_OBJECT     **params,
	ACPI_OPERAND_OBJECT     **return_obj_desc);

ACPI_STATUS
acpi_ns_get_object_value (
	ACPI_NAMESPACE_NODE     *object_node,
	ACPI_OPERAND_OBJECT     **return_obj_desc);


/*
 * Parent/Child/Peer utility functions - nsfamily
 */

ACPI_NAME
acpi_ns_find_parent_name (
	ACPI_NAMESPACE_NODE     *node_to_search);

u8
acpi_ns_exist_downstream_sibling (
	ACPI_NAMESPACE_NODE     *this_node);


/*
 * Scope manipulation - nsscope
 */

u32
acpi_ns_opens_scope (
	OBJECT_TYPE_INTERNAL    type);

NATIVE_CHAR *
acpi_ns_get_table_pathname (
	ACPI_NAMESPACE_NODE     *node);

NATIVE_CHAR *
acpi_ns_name_of_current_scope (
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_ns_handle_to_pathname (
	ACPI_HANDLE             obj_handle,
	u32                     *buf_size,
	NATIVE_CHAR             *user_buffer);

u8
acpi_ns_pattern_match (
	ACPI_NAMESPACE_NODE     *obj_node,
	NATIVE_CHAR             *search_for);

ACPI_STATUS
acpi_ns_name_compare (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *context,
	void                    **return_value);

ACPI_STATUS
acpi_ns_get_node (
	NATIVE_CHAR             *pathname,
	ACPI_NAMESPACE_NODE     *in_prefix_node,
	ACPI_NAMESPACE_NODE     **out_node);

u32
acpi_ns_get_pathname_length (
	ACPI_NAMESPACE_NODE     *node);


/*
 * Object management for NTEs - nsobject
 */

ACPI_STATUS
acpi_ns_attach_object (
	ACPI_NAMESPACE_NODE     *node,
	ACPI_OPERAND_OBJECT     *object,
	OBJECT_TYPE_INTERNAL    type);


void *
acpi_ns_compare_value (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *obj_desc);


/*
 * Namespace searching and entry - nssearch
 */

ACPI_STATUS
acpi_ns_search_and_enter (
	u32                     entry_name,
	ACPI_WALK_STATE         *walk_state,
	ACPI_NAMESPACE_NODE     *node,
	OPERATING_MODE          interpreter_mode,
	OBJECT_TYPE_INTERNAL    type,
	u32                     flags,
	ACPI_NAMESPACE_NODE     **ret_node);

ACPI_STATUS
acpi_ns_search_node (
	u32                     entry_name,
	ACPI_NAMESPACE_NODE     *node,
	OBJECT_TYPE_INTERNAL    type,
	ACPI_NAMESPACE_NODE     **ret_node);

void
acpi_ns_install_node (
	ACPI_WALK_STATE         *walk_state,
	ACPI_NAMESPACE_NODE     *parent_node,   /* Parent */
	ACPI_NAMESPACE_NODE     *node,      /* New Child*/
	OBJECT_TYPE_INTERNAL    type);


/*
 * Utility functions - nsutils
 */

u8
acpi_ns_valid_root_prefix (
	NATIVE_CHAR             prefix);

u8
acpi_ns_valid_path_separator (
	NATIVE_CHAR             sep);

OBJECT_TYPE_INTERNAL
acpi_ns_get_type (
	ACPI_HANDLE             obj_handle);

void *
acpi_ns_get_attached_object (
	ACPI_HANDLE             obj_handle);

u32
acpi_ns_local (
	OBJECT_TYPE_INTERNAL    type);

ACPI_STATUS
acpi_ns_internalize_name (
	NATIVE_CHAR             *dotted_name,
	NATIVE_CHAR             **converted_name);

ACPI_STATUS
acpi_ns_externalize_name (
	u32                     internal_name_length,
	NATIVE_CHAR             *internal_name,
	u32                     *converted_name_length,
	NATIVE_CHAR             **converted_name);

ACPI_NAMESPACE_NODE *
acpi_ns_convert_handle_to_entry (
	ACPI_HANDLE             handle);

ACPI_HANDLE
acpi_ns_convert_entry_to_handle(
	ACPI_NAMESPACE_NODE     *node);

void
acpi_ns_terminate (
	void);

ACPI_NAMESPACE_NODE *
acpi_ns_get_parent_object (
	ACPI_NAMESPACE_NODE     *node);


ACPI_NAMESPACE_NODE *
acpi_ns_get_next_valid_object (
	ACPI_NAMESPACE_NODE     *node);


#endif /* __ACNAMESP_H__ */
