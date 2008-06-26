/******************************************************************************
 *
 * Name: acdebug.h - ACPI/AML debugger
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

#ifndef __ACDEBUG_H__
#define __ACDEBUG_H__


#define DB_MAX_ARGS             8  /* Must be max method args + 1 */

#define DB_COMMAND_PROMPT      '-'
#define DB_EXECUTE_PROMPT      '%'


extern int                      optind;
extern NATIVE_CHAR              *optarg;
extern u8                       *aml_ptr;
extern u32                      acpi_aml_length;

extern u8                       opt_tables;
extern u8                       opt_disasm;
extern u8                       opt_stats;
extern u8                       opt_parse_jit;
extern u8                       opt_verbose;
extern u8                       opt_ini_methods;


extern NATIVE_CHAR              *args[DB_MAX_ARGS];
extern NATIVE_CHAR              line_buf[80];
extern NATIVE_CHAR              scope_buf[40];
extern NATIVE_CHAR              debug_filename[40];
extern u8                       output_to_file;
extern NATIVE_CHAR              *buffer;
extern NATIVE_CHAR              *filename;
extern NATIVE_CHAR              *INDENT_STRING;
extern u8                       acpi_gbl_db_output_flags;
extern u32                      acpi_gbl_db_debug_level;
extern u32                      acpi_gbl_db_console_debug_level;

extern u32                      num_names;
extern u32                      num_methods;
extern u32                      num_regions;
extern u32                      num_packages;
extern u32                      num_aliases;
extern u32                      num_devices;
extern u32                      num_field_defs;
extern u32                      num_thermal_zones;
extern u32                      num_nodes;
extern u32                      num_grammar_elements;
extern u32                      num_method_elements ;
extern u32                      num_mutexes;
extern u32                      num_power_resources;
extern u32                      num_bank_fields ;
extern u32                      num_index_fields;
extern u32                      num_events;

extern u32                      size_of_parse_tree;
extern u32                      size_of_method_trees;
extern u32                      size_of_nTes;
extern u32                      size_of_acpi_objects;


#define BUFFER_SIZE             4196

#define DB_REDIRECTABLE_OUTPUT  0x01
#define DB_CONSOLE_OUTPUT       0x02
#define DB_DUPLICATE_OUTPUT     0x03


typedef struct command_info
{
	NATIVE_CHAR             *name;          /* Command Name */
	u8                      min_args;       /* Minimum arguments required */

} COMMAND_INFO;


typedef struct argument_info
{
	NATIVE_CHAR             *name;          /* Argument Name */

} ARGUMENT_INFO;


#define PARAM_LIST(pl)                  pl

#define DBTEST_OUTPUT_LEVEL(lvl)        if (opt_verbose)

#define VERBOSE_PRINT(fp)               DBTEST_OUTPUT_LEVEL(lvl) {\
			  acpi_os_printf PARAM_LIST(fp);}

#define EX_NO_SINGLE_STEP       1
#define EX_SINGLE_STEP          2


/* Prototypes */


/*
 * dbapi - external debugger interfaces
 */

int
acpi_db_initialize (
	void);

ACPI_STATUS
acpi_db_single_step (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	u8                      op_type);


/*
 * dbcmds - debug commands and output routines
 */


void
acpi_db_display_table_info (
	NATIVE_CHAR             *table_arg);

void
acpi_db_unload_acpi_table (
	NATIVE_CHAR             *table_arg,
	NATIVE_CHAR             *instance_arg);

void
acpi_db_set_method_breakpoint (
	NATIVE_CHAR             *location,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_set_method_call_breakpoint (
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_disassemble_aml (
	NATIVE_CHAR             *statements,
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_dump_namespace (
	NATIVE_CHAR             *start_arg,
	NATIVE_CHAR             *depth_arg);

void
acpi_db_dump_namespace_by_owner (
	NATIVE_CHAR             *owner_arg,
	NATIVE_CHAR             *depth_arg);

void
acpi_db_send_notify (
	NATIVE_CHAR             *name,
	u32                     value);

void
acpi_db_set_method_data (
	NATIVE_CHAR             *type_arg,
	NATIVE_CHAR             *index_arg,
	NATIVE_CHAR             *value_arg);

ACPI_STATUS
acpi_db_display_objects (
	NATIVE_CHAR             *obj_type_arg,
	NATIVE_CHAR             *display_count_arg);

ACPI_STATUS
acpi_db_find_name_in_namespace (
	NATIVE_CHAR             *name_arg);

void
acpi_db_set_scope (
	NATIVE_CHAR             *name);

void
acpi_db_find_references (
	NATIVE_CHAR             *object_arg);

void
acpi_db_display_locks (void);


void
acpi_db_display_resources (
	NATIVE_CHAR             *object_arg);


/*
 * dbdisasm - AML disassembler
 */

void
acpi_db_display_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *origin,
	u32                     num_opcodes);

void
acpi_db_display_namestring (
	NATIVE_CHAR             *name);

void
acpi_db_display_path (
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_display_opcode (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_decode_internal_object (
	ACPI_OPERAND_OBJECT     *obj_desc);


/*
 * dbdisply - debug display commands
 */


void
acpi_db_display_method_info (
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_decode_and_display_object (
	NATIVE_CHAR             *target,
	NATIVE_CHAR             *output_type);

void
acpi_db_display_result_object (
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_WALK_STATE         *walk_state);

ACPI_STATUS
acpi_db_display_all_methods (
	NATIVE_CHAR             *display_count_arg);

void
acpi_db_display_internal_object (
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_WALK_STATE         *walk_state);

void
acpi_db_display_arguments (
	void);

void
acpi_db_display_locals (
	void);

void
acpi_db_display_results (
	void);

void
acpi_db_display_calling_tree (
	void);

void
acpi_db_display_argument_object (
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_WALK_STATE         *walk_state);


/*
 * dbexec - debugger control method execution
 */

void
acpi_db_execute (
	NATIVE_CHAR             *name,
	NATIVE_CHAR             **args,
	u32                     flags);

void
acpi_db_create_execution_threads (
	NATIVE_CHAR             *num_threads_arg,
	NATIVE_CHAR             *num_loops_arg,
	NATIVE_CHAR             *method_name_arg);


/*
 * dbfileio - Debugger file I/O commands
 */

OBJECT_TYPE_INTERNAL
acpi_db_match_argument (
	NATIVE_CHAR             *user_argument,
	ARGUMENT_INFO           *arguments);


void
acpi_db_close_debug_file (
	void);

void
acpi_db_open_debug_file (
	NATIVE_CHAR             *name);

ACPI_STATUS
acpi_db_load_acpi_table (
	NATIVE_CHAR             *filename);


/*
 * dbhistry - debugger HISTORY command
 */

void
acpi_db_add_to_history (
	NATIVE_CHAR             *command_line);

void
acpi_db_display_history (void);

NATIVE_CHAR *
acpi_db_get_from_history (
	NATIVE_CHAR             *command_num_arg);


/*
 * dbinput - user front-end to the AML debugger
 */

ACPI_STATUS
acpi_db_command_dispatch (
	NATIVE_CHAR             *input_buffer,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op);

void
acpi_db_execute_thread (
	void                    *context);

ACPI_STATUS
acpi_db_user_commands (
	NATIVE_CHAR             prompt,
	ACPI_PARSE_OBJECT       *op);


/*
 * dbstats - Generation and display of ACPI table statistics
 */

void
acpi_db_generate_statistics (
	ACPI_PARSE_OBJECT       *root,
	u8                      is_method);


ACPI_STATUS
acpi_db_display_statistics (
	NATIVE_CHAR             *type_arg);


/*
 * dbutils - AML debugger utilities
 */

void
acpi_db_set_output_destination (
	u32                     where);

void
acpi_db_dump_buffer (
	u32                     address);

void
acpi_db_dump_object (
	ACPI_OBJECT             *obj_desc,
	u32                     level);

void
acpi_db_prep_namestring (
	NATIVE_CHAR             *name);


ACPI_STATUS
acpi_db_second_pass_parse (
	ACPI_PARSE_OBJECT       *root);

ACPI_NAMESPACE_NODE *
acpi_db_local_ns_lookup (
	NATIVE_CHAR             *name);


#endif  /* __ACDEBUG_H__ */
