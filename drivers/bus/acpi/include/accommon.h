/******************************************************************************
 *
 * Name: accommon.h -- prototypes for the common (subsystem-wide) procedures
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

#ifndef _ACCOMMON_H
#define _ACCOMMON_H


typedef
ACPI_STATUS (*ACPI_PKG_CALLBACK) (
	u8                      object_type,
	ACPI_OPERAND_OBJECT     *source_object,
	ACPI_GENERIC_STATE      *state,
	void                    *context);


ACPI_STATUS
acpi_cm_walk_package_tree (
	ACPI_OPERAND_OBJECT     *source_object,
	void                    *target_object,
	ACPI_PKG_CALLBACK       walk_callback,
	void                    *context);


typedef struct acpi_pkg_info
{
	u8                      *free_space;
	u32                     length;
	u32                     object_space;
	u32                     num_packages;
} ACPI_PKG_INFO;

#define REF_INCREMENT       (u16) 0
#define REF_DECREMENT       (u16) 1
#define REF_FORCE_DELETE    (u16) 2

/* Acpi_cm_dump_buffer */

#define DB_BYTE_DISPLAY     1
#define DB_WORD_DISPLAY     2
#define DB_DWORD_DISPLAY    4
#define DB_QWORD_DISPLAY    8


/* Global initialization interfaces */

void
acpi_cm_init_globals (
	void);

void
acpi_cm_terminate (
	void);


/*
 * Cm_init - miscellaneous initialization and shutdown
 */

ACPI_STATUS
acpi_cm_hardware_initialize (
	void);

ACPI_STATUS
acpi_cm_subsystem_shutdown (
	void);

ACPI_STATUS
acpi_cm_validate_fadt (
	void);

/*
 * Cm_global - Global data structures and procedures
 */

#ifdef ACPI_DEBUG

NATIVE_CHAR *
acpi_cm_get_mutex_name (
	u32                     mutex_id);

NATIVE_CHAR *
acpi_cm_get_type_name (
	u32                     type);

NATIVE_CHAR *
acpi_cm_get_region_name (
	u8                      space_id);

#endif


u8
acpi_cm_valid_object_type (
	u32                     type);

ACPI_OWNER_ID
acpi_cm_allocate_owner_id (
	u32                     id_type);


/*
 * Cm_clib - Local implementations of C library functions
 */

#ifndef ACPI_USE_SYSTEM_CLIBRARY

u32
acpi_cm_strlen (
	const NATIVE_CHAR       *string);

NATIVE_CHAR *
acpi_cm_strcpy (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string);

NATIVE_CHAR *
acpi_cm_strncpy (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string,
	NATIVE_UINT             count);

u32
acpi_cm_strncmp (
	const NATIVE_CHAR       *string1,
	const NATIVE_CHAR       *string2,
	NATIVE_UINT             count);

u32
acpi_cm_strcmp (
	const NATIVE_CHAR       *string1,
	const NATIVE_CHAR       *string2);

NATIVE_CHAR *
acpi_cm_strcat (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string);

NATIVE_CHAR *
acpi_cm_strncat (
	NATIVE_CHAR             *dst_string,
	const NATIVE_CHAR       *src_string,
	NATIVE_UINT             count);

NATIVE_UINT
acpi_cm_strtoul (
	const NATIVE_CHAR       *string,
	NATIVE_CHAR             **terminator,
	NATIVE_UINT             base);

NATIVE_CHAR *
acpi_cm_strstr (
	NATIVE_CHAR             *string1,
	NATIVE_CHAR             *string2);

NATIVE_CHAR *
acpi_cm_strupr (
	NATIVE_CHAR             *src_string);

void *
acpi_cm_memcpy (
	void                    *dest,
	const void              *src,
	NATIVE_UINT             count);

void *
acpi_cm_memset (
	void                    *dest,
	NATIVE_UINT             value,
	NATIVE_UINT             count);

u32
acpi_cm_to_upper (
	u32                     c);

u32
acpi_cm_to_lower (
	u32                     c);

#endif /* ACPI_USE_SYSTEM_CLIBRARY */

/*
 * Cm_copy - Object construction and conversion interfaces
 */

ACPI_STATUS
acpi_cm_build_simple_object(
	ACPI_OPERAND_OBJECT     *obj,
	ACPI_OBJECT             *user_obj,
	u8                      *data_space,
	u32                     *buffer_space_used);

ACPI_STATUS
acpi_cm_build_package_object (
	ACPI_OPERAND_OBJECT     *obj,
	u8                      *buffer,
	u32                     *space_used);

ACPI_STATUS
acpi_cm_copy_iobject_to_eobject (
	ACPI_OPERAND_OBJECT     *obj,
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_cm_copy_esimple_to_isimple(
	ACPI_OBJECT             *user_obj,
	ACPI_OPERAND_OBJECT     *obj);

ACPI_STATUS
acpi_cm_copy_eobject_to_iobject (
	ACPI_OBJECT             *obj,
	ACPI_OPERAND_OBJECT     *internal_obj);

ACPI_STATUS
acpi_cm_copy_isimple_to_isimple (
	ACPI_OPERAND_OBJECT     *source_obj,
	ACPI_OPERAND_OBJECT     *dest_obj);

ACPI_STATUS
acpi_cm_copy_ipackage_to_ipackage (
	ACPI_OPERAND_OBJECT     *source_obj,
	ACPI_OPERAND_OBJECT     *dest_obj,
	ACPI_WALK_STATE         *walk_state);


/*
 * Cm_create - Object creation
 */

ACPI_STATUS
acpi_cm_update_object_reference (
	ACPI_OPERAND_OBJECT     *object,
	u16                     action);

ACPI_OPERAND_OBJECT  *
_cm_create_internal_object (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	OBJECT_TYPE_INTERNAL    type);


/*
 * Cm_debug - Debug interfaces
 */

u32
get_debug_level (
	void);

void
set_debug_level (
	u32                     level);

void
function_trace (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name);

void
function_trace_ptr (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	void                    *pointer);

void
function_trace_u32 (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	u32                     integer);

void
function_trace_str (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	NATIVE_CHAR             *string);

void
function_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name);

void
function_status_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	ACPI_STATUS             status);

void
function_value_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	ACPI_INTEGER            value);

void
function_ptr_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	u8                      *ptr);

void
debug_print_prefix (
	NATIVE_CHAR             *module_name,
	u32                     line_number);

void
debug_print (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	u32                     print_level,
	NATIVE_CHAR             *format, ...);

void
debug_print_raw (
	NATIVE_CHAR             *format, ...);

void
_report_info (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id);

void
_report_error (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id);

void
_report_warning (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id);

void
acpi_cm_dump_buffer (
	u8                      *buffer,
	u32                     count,
	u32                     display,
	u32                     component_id);


/*
 * Cm_delete - Object deletion
 */

void
acpi_cm_delete_internal_obj (
	ACPI_OPERAND_OBJECT     *object);

void
acpi_cm_delete_internal_package_object (
	ACPI_OPERAND_OBJECT     *object);

void
acpi_cm_delete_internal_simple_object (
	ACPI_OPERAND_OBJECT     *object);

ACPI_STATUS
acpi_cm_delete_internal_object_list (
	ACPI_OPERAND_OBJECT     **obj_list);


/*
 * Cm_eval - object evaluation
 */

/* Method name strings */

#define METHOD_NAME__HID        "_HID"
#define METHOD_NAME__UID        "_UID"
#define METHOD_NAME__ADR        "_ADR"
#define METHOD_NAME__STA        "_STA"
#define METHOD_NAME__REG        "_REG"
#define METHOD_NAME__SEG        "_SEG"
#define METHOD_NAME__BBN        "_BBN"


ACPI_STATUS
acpi_cm_evaluate_numeric_object (
	NATIVE_CHAR             *object_name,
	ACPI_NAMESPACE_NODE     *device_node,
	ACPI_INTEGER            *address);

ACPI_STATUS
acpi_cm_execute_HID (
	ACPI_NAMESPACE_NODE     *device_node,
	DEVICE_ID               *hid);

ACPI_STATUS
acpi_cm_execute_STA (
	ACPI_NAMESPACE_NODE     *device_node,
	u32                     *status_flags);

ACPI_STATUS
acpi_cm_execute_UID (
	ACPI_NAMESPACE_NODE     *device_node,
	DEVICE_ID               *uid);


/*
 * Cm_error - exception interfaces
 */

NATIVE_CHAR *
acpi_cm_format_exception (
	ACPI_STATUS             status);


/*
 * Cm_mutex - mutual exclusion interfaces
 */

ACPI_STATUS
acpi_cm_mutex_initialize (
	void);

void
acpi_cm_mutex_terminate (
	void);

ACPI_STATUS
acpi_cm_create_mutex (
	ACPI_MUTEX_HANDLE       mutex_id);

ACPI_STATUS
acpi_cm_delete_mutex (
	ACPI_MUTEX_HANDLE       mutex_id);

ACPI_STATUS
acpi_cm_acquire_mutex (
	ACPI_MUTEX_HANDLE       mutex_id);

ACPI_STATUS
acpi_cm_release_mutex (
	ACPI_MUTEX_HANDLE       mutex_id);


/*
 * Cm_object - internal object create/delete/cache routines
 */

void *
_cm_allocate_object_desc (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id);

#define acpi_cm_create_internal_object(t) _cm_create_internal_object(_THIS_MODULE,__LINE__,_COMPONENT,t)
#define acpi_cm_allocate_object_desc()  _cm_allocate_object_desc(_THIS_MODULE,__LINE__,_COMPONENT)

void
acpi_cm_delete_object_desc (
	ACPI_OPERAND_OBJECT     *object);

u8
acpi_cm_valid_internal_object (
	void                    *object);


/*
 * Cm_ref_cnt - Object reference count management
 */

void
acpi_cm_add_reference (
	ACPI_OPERAND_OBJECT     *object);

void
acpi_cm_remove_reference (
	ACPI_OPERAND_OBJECT     *object);

/*
 * Cm_size - Object size routines
 */

ACPI_STATUS
acpi_cm_get_simple_object_size (
	ACPI_OPERAND_OBJECT     *obj,
	u32                     *obj_length);

ACPI_STATUS
acpi_cm_get_package_object_size (
	ACPI_OPERAND_OBJECT     *obj,
	u32                     *obj_length);

ACPI_STATUS
acpi_cm_get_object_size(
	ACPI_OPERAND_OBJECT     *obj,
	u32                     *obj_length);


/*
 * Cm_state - Generic state creation/cache routines
 */

void
acpi_cm_push_generic_state (
	ACPI_GENERIC_STATE      **list_head,
	ACPI_GENERIC_STATE      *state);

ACPI_GENERIC_STATE *
acpi_cm_pop_generic_state (
	ACPI_GENERIC_STATE      **list_head);


ACPI_GENERIC_STATE *
acpi_cm_create_generic_state (
	void);

ACPI_GENERIC_STATE *
acpi_cm_create_update_state (
	ACPI_OPERAND_OBJECT     *object,
	u16                     action);

ACPI_GENERIC_STATE *
acpi_cm_create_pkg_state (
	void                    *internal_object,
	void                    *external_object,
	u16                     index);

ACPI_STATUS
acpi_cm_create_update_state_and_push (
	ACPI_OPERAND_OBJECT     *object,
	u16                     action,
	ACPI_GENERIC_STATE      **state_list);

ACPI_STATUS
acpi_cm_create_pkg_state_and_push (
	void                    *internal_object,
	void                    *external_object,
	u16                     index,
	ACPI_GENERIC_STATE      **state_list);

ACPI_GENERIC_STATE *
acpi_cm_create_control_state (
	void);

void
acpi_cm_delete_generic_state (
	ACPI_GENERIC_STATE      *state);

void
acpi_cm_delete_generic_state_cache (
	void);

void
acpi_cm_delete_object_cache (
	void);

/*
 * Cmutils
 */

u8
acpi_cm_valid_acpi_name (
	u32                     name);

u8
acpi_cm_valid_acpi_character (
	NATIVE_CHAR             character);

ACPI_STATUS
acpi_cm_resolve_package_references (
	ACPI_OPERAND_OBJECT     *obj_desc);

#ifdef ACPI_DEBUG

void
acpi_cm_display_init_pathname (
	ACPI_HANDLE             obj_handle,
	char                    *path);

#endif


/*
 * Memory allocation functions and related macros.
 * Macros that expand to include filename and line number
 */

void *
_cm_allocate (
	u32                     size,
	u32                     component,
	NATIVE_CHAR             *module,
	u32                     line);

void *
_cm_callocate (
	u32                     size,
	u32                     component,
	NATIVE_CHAR             *module,
	u32                     line);

void
_cm_free (
	void                    *address,
	u32                     component,
	NATIVE_CHAR             *module,
	u32                     line);

void
acpi_cm_init_static_object (
	ACPI_OPERAND_OBJECT     *obj_desc);

#define acpi_cm_allocate(a)             _cm_allocate(a,_COMPONENT,_THIS_MODULE,__LINE__)
#define acpi_cm_callocate(a)            _cm_callocate(a, _COMPONENT,_THIS_MODULE,__LINE__)
#define acpi_cm_free(a)                 _cm_free(a,_COMPONENT,_THIS_MODULE,__LINE__)

#ifndef ACPI_DEBUG_TRACK_ALLOCATIONS

#define acpi_cm_add_element_to_alloc_list(a,b,c,d,e,f)
#define acpi_cm_delete_element_from_alloc_list(a,b,c,d)
#define acpi_cm_dump_current_allocations(a,b)
#define acpi_cm_dump_allocation_info()

#define DECREMENT_OBJECT_METRICS(a)
#define INCREMENT_OBJECT_METRICS(a)
#define INITIALIZE_ALLOCATION_METRICS()
#define DECREMENT_NAME_TABLE_METRICS(a)
#define INCREMENT_NAME_TABLE_METRICS(a)

#else

#define INITIALIZE_ALLOCATION_METRICS() \
	acpi_gbl_current_object_count = 0; \
	acpi_gbl_current_object_size = 0; \
	acpi_gbl_running_object_count = 0; \
	acpi_gbl_running_object_size = 0; \
	acpi_gbl_max_concurrent_object_count = 0; \
	acpi_gbl_max_concurrent_object_size = 0; \
	acpi_gbl_current_alloc_size = 0; \
	acpi_gbl_current_alloc_count = 0; \
	acpi_gbl_running_alloc_size = 0; \
	acpi_gbl_running_alloc_count = 0; \
	acpi_gbl_max_concurrent_alloc_size = 0; \
	acpi_gbl_max_concurrent_alloc_count = 0; \
	acpi_gbl_current_node_count = 0; \
	acpi_gbl_current_node_size = 0; \
	acpi_gbl_max_concurrent_node_count = 0


#define DECREMENT_OBJECT_METRICS(a) \
	acpi_gbl_current_object_count--; \
	acpi_gbl_current_object_size -= a

#define INCREMENT_OBJECT_METRICS(a) \
	acpi_gbl_current_object_count++; \
	acpi_gbl_running_object_count++; \
	if (acpi_gbl_max_concurrent_object_count < acpi_gbl_current_object_count) \
	{ \
		acpi_gbl_max_concurrent_object_count = acpi_gbl_current_object_count; \
	} \
	acpi_gbl_running_object_size += a; \
	acpi_gbl_current_object_size += a; \
	if (acpi_gbl_max_concurrent_object_size < acpi_gbl_current_object_size) \
	{ \
		acpi_gbl_max_concurrent_object_size = acpi_gbl_current_object_size; \
	}

#define DECREMENT_NAME_TABLE_METRICS(a) \
	acpi_gbl_current_node_count--; \
	acpi_gbl_current_node_size -= (a)

#define INCREMENT_NAME_TABLE_METRICS(a) \
	acpi_gbl_current_node_count++; \
	acpi_gbl_current_node_size+= (a); \
	if (acpi_gbl_max_concurrent_node_count < acpi_gbl_current_node_count) \
	{ \
		acpi_gbl_max_concurrent_node_count = acpi_gbl_current_node_count; \
	} \


void
acpi_cm_dump_allocation_info (
	void);

void
acpi_cm_dump_current_allocations (
	u32                     component,
	NATIVE_CHAR             *module);

#endif


#endif /* _ACCOMMON_H */
