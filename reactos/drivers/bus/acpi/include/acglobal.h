/******************************************************************************
 *
 * Name: acglobal.h - Declarations for global variables
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

#ifndef __ACGLOBAL_H__
#define __ACGLOBAL_H__


/*
 * Ensure that the globals are actually defined only once.
 *
 * The use of these defines allows a single list of globals (here) in order
 * to simplify maintenance of the code.
 */
#ifdef DEFINE_ACPI_GLOBALS
#define ACPI_EXTERN
#else
#define ACPI_EXTERN extern
#endif


extern      NATIVE_CHAR                 *msg_acpi_error_break;

/*****************************************************************************
 *
 * Debug support
 *
 ****************************************************************************/

/* Runtime configuration of debug print levels */

extern      u32                         acpi_dbg_level;
extern      u32                         acpi_dbg_layer;


/* Procedure nesting level for debug output */

extern      u32                         acpi_gbl_nesting_level;


/*****************************************************************************
 *
 * ACPI Table globals
 *
 ****************************************************************************/

/*
 * Table pointers.
 * Although these pointers are somewhat redundant with the global Acpi_table,
 * they are convenient because they are typed pointers.
 *
 * These tables are single-table only; meaning that there can be at most one
 * of each in the system.  Each global points to the actual table.
 *
 */
ACPI_EXTERN RSDP_DESCRIPTOR             *acpi_gbl_RSDP;
ACPI_EXTERN XSDT_DESCRIPTOR             *acpi_gbl_XSDT;
ACPI_EXTERN FADT_DESCRIPTOR             *acpi_gbl_FADT;
ACPI_EXTERN ACPI_TABLE_HEADER           *acpi_gbl_DSDT;
ACPI_EXTERN ACPI_COMMON_FACS            *acpi_gbl_FACS;

/*
 * Since there may be multiple SSDTs and PSDTS, a single pointer is not
 * sufficient; Therefore, there isn't one!
 */


/*
 * ACPI Table info arrays
 */
extern      ACPI_TABLE_DESC             acpi_gbl_acpi_tables[NUM_ACPI_TABLES];
extern      ACPI_TABLE_SUPPORT          acpi_gbl_acpi_table_data[NUM_ACPI_TABLES];

/*
 * Predefined mutex objects.  This array contains the
 * actual OS mutex handles, indexed by the local ACPI_MUTEX_HANDLEs.
 * (The table maps local handles to the real OS handles)
 */
ACPI_EXTERN ACPI_MUTEX_INFO             acpi_gbl_acpi_mutex_info [NUM_MTX];


/*****************************************************************************
 *
 * Miscellaneous globals
 *
 ****************************************************************************/


ACPI_EXTERN u8                         *acpi_gbl_gpe0enable_register_save;
ACPI_EXTERN u8                         *acpi_gbl_gpe1_enable_register_save;
ACPI_EXTERN ACPI_WALK_STATE            *acpi_gbl_breakpoint_walk;
ACPI_EXTERN ACPI_GENERIC_STATE         *acpi_gbl_generic_state_cache;
ACPI_EXTERN ACPI_PARSE_OBJECT          *acpi_gbl_parse_cache;
ACPI_EXTERN ACPI_PARSE2_OBJECT         *acpi_gbl_ext_parse_cache;
ACPI_EXTERN ACPI_OPERAND_OBJECT        *acpi_gbl_object_cache;
ACPI_EXTERN ACPI_WALK_STATE            *acpi_gbl_walk_state_cache;
ACPI_EXTERN ACPI_HANDLE                 acpi_gbl_global_lock_semaphore;


ACPI_EXTERN u32                         acpi_gbl_global_lock_thread_count;
ACPI_EXTERN u32                         acpi_gbl_restore_acpi_chipset;
ACPI_EXTERN u32                         acpi_gbl_original_mode;
ACPI_EXTERN u32                         acpi_gbl_edge_level_save;
ACPI_EXTERN u32                         acpi_gbl_irq_enable_save;
ACPI_EXTERN u32                         acpi_gbl_rsdp_original_location;

ACPI_EXTERN u32                         acpi_gbl_state_cache_requests;
ACPI_EXTERN u32                         acpi_gbl_state_cache_hits;
ACPI_EXTERN u32                         acpi_gbl_parse_cache_requests;
ACPI_EXTERN u32                         acpi_gbl_parse_cache_hits;
ACPI_EXTERN u32                         acpi_gbl_ext_parse_cache_requests;
ACPI_EXTERN u32                         acpi_gbl_ext_parse_cache_hits;
ACPI_EXTERN u32                         acpi_gbl_object_cache_requests;
ACPI_EXTERN u32                         acpi_gbl_object_cache_hits;
ACPI_EXTERN u32                         acpi_gbl_walk_state_cache_requests;
ACPI_EXTERN u32                         acpi_gbl_walk_state_cache_hits;
ACPI_EXTERN u32                         acpi_gbl_ns_lookup_count;
ACPI_EXTERN u32                         acpi_gbl_ps_find_count;


ACPI_EXTERN u16                         acpi_gbl_generic_state_cache_depth;
ACPI_EXTERN u16                         acpi_gbl_parse_cache_depth;
ACPI_EXTERN u16                         acpi_gbl_ext_parse_cache_depth;
ACPI_EXTERN u16                         acpi_gbl_object_cache_depth;
ACPI_EXTERN u16                         acpi_gbl_walk_state_cache_depth;
ACPI_EXTERN u16                         acpi_gbl_pm1_enable_register_save;
ACPI_EXTERN u16                         acpi_gbl_next_table_owner_id;
ACPI_EXTERN u16                         acpi_gbl_next_method_owner_id;

ACPI_EXTERN u8                          acpi_gbl_debugger_configuration;
ACPI_EXTERN u8                          acpi_gbl_global_lock_acquired;
ACPI_EXTERN u8                          acpi_gbl_step_to_next_call;
ACPI_EXTERN u8                          acpi_gbl_acpi_hardware_present;
ACPI_EXTERN u8                          acpi_gbl_global_lock_present;

ACPI_EXTERN ACPI_OBJECT_NOTIFY_HANDLER  acpi_gbl_drv_notify;
ACPI_EXTERN ACPI_OBJECT_NOTIFY_HANDLER  acpi_gbl_sys_notify;


extern u8                               acpi_gbl_shutdown;
extern u32                              acpi_gbl_system_flags;
extern u32                              acpi_gbl_startup_flags;
extern u8                               acpi_gbl_decode_to8bit[8];
extern NATIVE_CHAR                      acpi_gbl_hex_to_ascii[16];


/*****************************************************************************
 *
 * Namespace globals
 *
 ****************************************************************************/

#define NUM_NS_TYPES                    INTERNAL_TYPE_INVALID+1
#define NUM_PREDEFINED_NAMES            9


ACPI_EXTERN ACPI_NAMESPACE_NODE         acpi_gbl_root_node_struct;
ACPI_EXTERN ACPI_NAMESPACE_NODE        *acpi_gbl_root_node;

extern u8                               acpi_gbl_ns_properties[NUM_NS_TYPES];
extern PREDEFINED_NAMES                 acpi_gbl_pre_defined_names [NUM_PREDEFINED_NAMES];


/* Used to detect memory leaks (DEBUG ONLY) */

#ifdef ACPI_DEBUG
ACPI_EXTERN ALLOCATION_INFO            *acpi_gbl_head_alloc_ptr;
ACPI_EXTERN ALLOCATION_INFO            *acpi_gbl_tail_alloc_ptr;
#endif


/*****************************************************************************
 *
 * Interpreter globals
 *
 ****************************************************************************/


ACPI_EXTERN ACPI_WALK_LIST             *acpi_gbl_current_walk_list;

/*
 * Handle to the last method found - used during pass1 of load
 */
ACPI_EXTERN ACPI_HANDLE                 acpi_gbl_last_method;

/*
 * Table of Address Space handlers
 */

ACPI_EXTERN ACPI_ADDRESS_SPACE_INFO     acpi_gbl_address_spaces[ACPI_NUM_ADDRESS_SPACES];


/* Control method single step flag */

ACPI_EXTERN u8                          acpi_gbl_cm_single_step;


/*****************************************************************************
 *
 * Parser globals
 *
 ****************************************************************************/

ACPI_EXTERN ACPI_PARSE_OBJECT           *acpi_gbl_parsed_namespace_root;

/*****************************************************************************
 *
 * Hardware globals
 *
 ****************************************************************************/

extern ACPI_C_STATE_HANDLER             acpi_hw_cx_handlers[MAX_CX_STATES];
extern u32                              acpi_hw_active_cx_state;


/*****************************************************************************
 *
 * Event globals
 *
 ****************************************************************************/

ACPI_EXTERN ACPI_FIXED_EVENT_INFO       acpi_gbl_fixed_event_handlers[NUM_FIXED_EVENTS];

ACPI_EXTERN ACPI_HANDLE                 acpi_gbl_gpe_obj_handle;
ACPI_EXTERN u32                         acpi_gbl_gpe_register_count;
ACPI_EXTERN ACPI_GPE_REGISTERS          *acpi_gbl_gpe_registers;
ACPI_EXTERN ACPI_GPE_LEVEL_INFO         *acpi_gbl_gpe_info;

/*
 * Gpe validation and translation table
 * Indexed by the GPE number, returns GPE_INVALID if the GPE is not supported.
 * Otherwise, returns a valid index into the global GPE table.
 *
 * This table is needed because the GPE numbers supported by block 1 do not
 * have to be contiguous with the GPE numbers supported by block 0.
 */
ACPI_EXTERN u8                          acpi_gbl_gpe_valid [NUM_GPE];

/* Acpi_event counter for debug only */

#ifdef ACPI_DEBUG
ACPI_EXTERN u32                         acpi_gbl_event_count[NUM_FIXED_EVENTS];
#endif


/*****************************************************************************
 *
 * Debugger globals
 *
 ****************************************************************************/

#ifdef ENABLE_DEBUGGER
ACPI_EXTERN u8                          acpi_gbl_method_executing;
ACPI_EXTERN u8                          acpi_gbl_db_terminate_threads;
#endif

/* Memory allocation metrics - Debug Only! */

#ifdef ACPI_DEBUG

ACPI_EXTERN u32                         acpi_gbl_current_alloc_size;
ACPI_EXTERN u32                         acpi_gbl_current_alloc_count;
ACPI_EXTERN u32                         acpi_gbl_running_alloc_size;
ACPI_EXTERN u32                         acpi_gbl_running_alloc_count;
ACPI_EXTERN u32                         acpi_gbl_max_concurrent_alloc_size;
ACPI_EXTERN u32                         acpi_gbl_max_concurrent_alloc_count;
ACPI_EXTERN u32                         acpi_gbl_current_object_count;
ACPI_EXTERN u32                         acpi_gbl_current_object_size;
ACPI_EXTERN u32                         acpi_gbl_max_concurrent_object_count;
ACPI_EXTERN u32                         acpi_gbl_max_concurrent_object_size;
ACPI_EXTERN u32                         acpi_gbl_running_object_count;
ACPI_EXTERN u32                         acpi_gbl_running_object_size;
ACPI_EXTERN u32                         acpi_gbl_current_node_count;
ACPI_EXTERN u32                         acpi_gbl_current_node_size;
ACPI_EXTERN u32                         acpi_gbl_max_concurrent_node_count;

#endif


#endif /* __ACGLOBAL_H__ */
