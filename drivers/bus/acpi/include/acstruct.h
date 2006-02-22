/******************************************************************************
 *
 * Name: acstruct.h - Internal structs
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

#ifndef __ACSTRUCT_H__
#define __ACSTRUCT_H__


/*****************************************************************************
 *
 * Tree walking typedefs and structs
 *
 ****************************************************************************/


/*
 * Walk state - current state of a parse tree walk.  Used for both a leisurely stroll through
 * the tree (for whatever reason), and for control method execution.
 */

#define NEXT_OP_DOWNWARD    1
#define NEXT_OP_UPWARD      2

#define WALK_NON_METHOD     0
#define WALK_METHOD         1
#define WALK_METHOD_RESTART 2

typedef struct acpi_walk_state
{
	u8                      data_type;                          /* To differentiate various internal objs */\
	ACPI_OWNER_ID           owner_id;                           /* Owner of objects created during the walk */
	u8                      last_predicate;                     /* Result of last predicate */
	u8                      next_op_info;                       /* Info about Next_op */
	u8                      num_operands;                       /* Stack pointer for Operands[] array */
	u8                      current_result;                     /* */

	struct acpi_walk_state  *next;                              /* Next Walk_state in list */
	ACPI_PARSE_OBJECT       *origin;                            /* Start of walk [Obsolete] */

/* TBD: Obsolete with removal of WALK procedure ? */
	ACPI_PARSE_OBJECT       *prev_op;                           /* Last op that was processed */
	ACPI_PARSE_OBJECT       *next_op;                           /* next op to be processed */


	ACPI_GENERIC_STATE      *results;                           /* Stack of accumulated results */
	ACPI_GENERIC_STATE      *control_state;                     /* List of control states (nested IFs) */
	ACPI_GENERIC_STATE      *scope_info;                        /* Stack of nested scopes */
	ACPI_PARSE_STATE        *parser_state;                      /* Current state of parser */
	u8                      *aml_last_while;
	ACPI_OPCODE_INFO        *op_info;                           /* Info on current opcode */
	ACPI_PARSE_DOWNWARDS    descending_callback;
	ACPI_PARSE_UPWARDS      ascending_callback;

	union acpi_operand_obj  *return_desc;                       /* Return object, if any */
	union acpi_operand_obj  *method_desc;                       /* Method descriptor if running a method */
	struct acpi_node        *method_node;                       /* Method Node if running a method */
	ACPI_PARSE_OBJECT       *method_call_op;                    /* Method_call Op if running a method */
	struct acpi_node        *method_call_node;                  /* Called method Node*/
	union acpi_operand_obj  *operands[OBJ_NUM_OPERANDS];        /* Operands passed to the interpreter */
	struct acpi_node        arguments[MTH_NUM_ARGS];            /* Control method arguments */
	struct acpi_node        local_variables[MTH_NUM_LOCALS];    /* Control method locals */
	struct acpi_walk_list   *walk_list;
	u32                     parse_flags;
	u8                      walk_type;
	u8                      return_used;
	u16                     opcode;                             /* Current AML opcode */
	u32                     prev_arg_types;
	u16                     current_sync_level;                 /* Mutex Sync (nested acquire) level */

	/* Debug support */

	u32                     method_breakpoint;


} ACPI_WALK_STATE;


/*
 * Walk list - head of a tree of walk states.  Multiple walk states are created when there
 * are nested control methods executing.
 */
typedef struct acpi_walk_list
{

	ACPI_WALK_STATE         *walk_state;
	ACPI_OBJECT_MUTEX       acquired_mutex_list;               /* List of all currently acquired mutexes */

} ACPI_WALK_LIST;


/* Info used by Acpi_ps_init_objects */

typedef struct acpi_init_walk_info
{
	u16                     method_count;
	u16                     op_region_count;
	u16                     field_count;
	u16                     op_region_init;
	u16                     field_init;
	u16                     object_count;
	ACPI_TABLE_DESC         *table_desc;

} ACPI_INIT_WALK_INFO;


/* Info used by TBD */

typedef struct acpi_device_walk_info
{
	u16                     device_count;
	u16                     num_STA;
	u16                     num_INI;
	ACPI_TABLE_DESC         *table_desc;

} ACPI_DEVICE_WALK_INFO;


/* TBD: [Restructure] Merge with struct above */

typedef struct acpi_walk_info
{
	u32                     debug_level;
	u32                     owner_id;

} ACPI_WALK_INFO;

typedef struct acpi_get_devices_info
{
	WALK_CALLBACK           user_function;
	void                    *context;
	NATIVE_CHAR             *hid;

} ACPI_GET_DEVICES_INFO;


#endif
