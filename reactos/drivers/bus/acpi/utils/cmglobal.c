/******************************************************************************
 *
 * Module Name: cmglobal - Global variables for the ACPI subsystem
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

#define DEFINE_ACPI_GLOBALS

#include "acpi.h"
#include "acevents.h"
#include "acnamesp.h"
#include "acinterp.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmglobal")


/******************************************************************************
 *
 * Static global variable initialization.
 *
 ******************************************************************************/

/*
 * We want the debug switches statically initialized so they
 * are already set when the debugger is entered.
 */

/* Debug switch - level and trace mask */

u32                         acpi_dbg_level = NORMAL_DEFAULT;

/* Debug switch - layer (component) mask */

u32                         acpi_dbg_layer = ACPI_COMPONENT_DEFAULT;
u32                         acpi_gbl_nesting_level = 0;


/* Debugger globals */

u8                          acpi_gbl_db_terminate_threads = FALSE;
u8                          acpi_gbl_method_executing = FALSE;

/* System flags */

u32                         acpi_gbl_system_flags = 0;
u32                         acpi_gbl_startup_flags = 0;

/* System starts unitialized! */
u8                          acpi_gbl_shutdown = TRUE;


u8                          acpi_gbl_decode_to8bit [8] = {1,2,4,8,16,32,64,128};


/******************************************************************************
 *
 * Namespace globals
 *
 ******************************************************************************/


/*
 * Names built-in to the interpreter
 *
 * Initial values are currently supported only for types String and Number.
 * To avoid type punning, both are specified as strings in this table.
 *
 * NOTES:
 * 1) _SB_ is defined to be a device to allow _SB_/_INI to be run
 *    during the initialization sequence.
 */

PREDEFINED_NAMES            acpi_gbl_pre_defined_names[] =
{ {"_GPE",    INTERNAL_TYPE_DEF_ANY},
	{"_PR_",    INTERNAL_TYPE_DEF_ANY},
	{"_SB_",    ACPI_TYPE_DEVICE},
	{"_SI_",    INTERNAL_TYPE_DEF_ANY},
	{"_TZ_",    INTERNAL_TYPE_DEF_ANY},
	{"_REV",    ACPI_TYPE_INTEGER, "2"},
	{"_OS_",    ACPI_TYPE_STRING, ACPI_OS_NAME},
	{"_GL_",    ACPI_TYPE_MUTEX, "0"},
	{NULL,      ACPI_TYPE_ANY}           /* Table terminator */
};


/*
 * Properties of the ACPI Object Types, both internal and external.
 *
 * Elements of Acpi_ns_properties are bit significant
 * and the table is indexed by values of ACPI_OBJECT_TYPE
 */

u8                          acpi_gbl_ns_properties[] =
{
	NSP_NORMAL,                 /* 00 Any              */
	NSP_NORMAL,                 /* 01 Number           */
	NSP_NORMAL,                 /* 02 String           */
	NSP_NORMAL,                 /* 03 Buffer           */
	NSP_LOCAL,                  /* 04 Package          */
	NSP_NORMAL,                 /* 05 Field_unit       */
	NSP_NEWSCOPE | NSP_LOCAL,   /* 06 Device           */
	NSP_LOCAL,                  /* 07 Acpi_event       */
	NSP_NEWSCOPE | NSP_LOCAL,   /* 08 Method           */
	NSP_LOCAL,                  /* 09 Mutex            */
	NSP_LOCAL,                  /* 10 Region           */
	NSP_NEWSCOPE | NSP_LOCAL,   /* 11 Power            */
	NSP_NEWSCOPE | NSP_LOCAL,   /* 12 Processor        */
	NSP_NEWSCOPE | NSP_LOCAL,   /* 13 Thermal          */
	NSP_NORMAL,                 /* 14 Buffer_field     */
	NSP_NORMAL,                 /* 15 Ddb_handle       */
	NSP_NORMAL,                 /* 16 Debug Object     */
	NSP_NORMAL,                 /* 17 Def_field        */
	NSP_NORMAL,                 /* 18 Bank_field       */
	NSP_NORMAL,                 /* 19 Index_field      */
	NSP_NORMAL,                 /* 20 Reference        */
	NSP_NORMAL,                 /* 21 Alias            */
	NSP_NORMAL,                 /* 22 Notify           */
	NSP_NORMAL,                 /* 23 Address Handler  */
	NSP_NEWSCOPE | NSP_LOCAL,   /* 24 Resource         */
	NSP_NORMAL,                 /* 25 Def_field_defn   */
	NSP_NORMAL,                 /* 26 Bank_field_defn  */
	NSP_NORMAL,                 /* 27 Index_field_defn */
	NSP_NORMAL,                 /* 28 If               */
	NSP_NORMAL,                 /* 29 Else             */
	NSP_NORMAL,                 /* 30 While            */
	NSP_NEWSCOPE,               /* 31 Scope            */
	NSP_LOCAL,                  /* 32 Def_any          */
	NSP_NORMAL,                 /* 33 Extra            */
	NSP_NORMAL                  /* 34 Invalid          */
};


/* Hex to ASCII conversion table */

NATIVE_CHAR                 acpi_gbl_hex_to_ascii[] =
			  {'0','1','2','3','4','5','6','7',
			  '8','9','A','B','C','D','E','F'};


/******************************************************************************
 *
 * Table globals
 *
 * NOTE: This table includes ONLY the ACPI tables that the subsystem consumes.
 * it is NOT an exhaustive list of all possible ACPI tables.  All ACPI tables
 * that are not used by the subsystem are simply ignored.
 *
 ******************************************************************************/


ACPI_TABLE_DESC             acpi_gbl_acpi_tables[NUM_ACPI_TABLES];


ACPI_TABLE_SUPPORT          acpi_gbl_acpi_table_data[NUM_ACPI_TABLES] =
{
	/***********    Name,    Signature,  Signature size,    How many allowed?,   Supported?  Global typed pointer */

	/* RSDP 0 */ {RSDP_NAME, RSDP_SIG, sizeof (RSDP_SIG)-1, ACPI_TABLE_SINGLE,   AE_OK,      NULL},
	/* DSDT 1 */ {DSDT_SIG,  DSDT_SIG, sizeof (DSDT_SIG)-1, ACPI_TABLE_SINGLE,   AE_OK,      (void **) &acpi_gbl_DSDT},
	/* FADT 2 */ {FADT_SIG,  FADT_SIG, sizeof (FADT_SIG)-1, ACPI_TABLE_SINGLE,   AE_OK,      (void **) &acpi_gbl_FADT},
	/* FACS 3 */ {FACS_SIG,  FACS_SIG, sizeof (FACS_SIG)-1, ACPI_TABLE_SINGLE,   AE_OK,      (void **) &acpi_gbl_FACS},
	/* PSDT 4 */ {PSDT_SIG,  PSDT_SIG, sizeof (PSDT_SIG)-1, ACPI_TABLE_MULTIPLE, AE_OK,      NULL},
	/* SSDT 5 */ {SSDT_SIG,  SSDT_SIG, sizeof (SSDT_SIG)-1, ACPI_TABLE_MULTIPLE, AE_OK,      NULL},
	/* XSDT 6 */ {XSDT_SIG,  XSDT_SIG, sizeof (RSDT_SIG)-1, ACPI_TABLE_SINGLE,   AE_OK,      NULL},
};


/*****************************************************************************
 *
 * FUNCTION:    Acpi_cm_valid_object_type
 *
 * PARAMETERS:  None.
 *
 * RETURN:      TRUE if valid object type
 *
 * DESCRIPTION: Validate an object type
 *
 ****************************************************************************/

u8
acpi_cm_valid_object_type (
	u32                     type)
{

	if (type > ACPI_TYPE_MAX)
	{
		if ((type < INTERNAL_TYPE_BEGIN) ||
			(type > INTERNAL_TYPE_MAX))
		{
			return (FALSE);
		}
	}

	return (TRUE);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_cm_format_exception
 *
 * PARAMETERS:  Status              - Acpi status to be formatted
 *
 * RETURN:      Formatted status string
 *
 * DESCRIPTION: Convert an ACPI exception to a string
 *
 ****************************************************************************/

NATIVE_CHAR *
acpi_cm_format_exception (
	ACPI_STATUS             status)
{
	NATIVE_CHAR             *exception = "UNKNOWN_STATUS";
	ACPI_STATUS             sub_status;


	sub_status = (status & ~AE_CODE_MASK);


	switch (status & AE_CODE_MASK)
	{
	case AE_CODE_ENVIRONMENTAL:

		if (sub_status <= AE_CODE_ENV_MAX)
		{
			exception = acpi_gbl_exception_names_env [sub_status];
		}
		break;

	case AE_CODE_PROGRAMMER:

		if (sub_status <= AE_CODE_PGM_MAX)
		{
			exception = acpi_gbl_exception_names_pgm [sub_status -1];
		}
		break;

	case AE_CODE_ACPI_TABLES:

		if (sub_status <= AE_CODE_TBL_MAX)
		{
			exception = acpi_gbl_exception_names_tbl [sub_status -1];
		}
		break;

	case AE_CODE_AML:

		if (sub_status <= AE_CODE_AML_MAX)
		{
			exception = acpi_gbl_exception_names_aml [sub_status -1];
		}
		break;

	case AE_CODE_CONTROL:

		if (sub_status <= AE_CODE_CTRL_MAX)
		{
			exception = acpi_gbl_exception_names_ctrl [sub_status -1];
		}
		break;

	default:
		break;
	}


	return (exception);
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_cm_allocate_owner_id
 *
 * PARAMETERS:  Id_type         - Type of ID (method or table)
 *
 * DESCRIPTION: Allocate a table or method owner id
 *
 ***************************************************************************/

ACPI_OWNER_ID
acpi_cm_allocate_owner_id (
	u32                     id_type)
{
	ACPI_OWNER_ID           owner_id = 0xFFFF;


	acpi_cm_acquire_mutex (ACPI_MTX_CACHES);

	switch (id_type)
	{
	case OWNER_TYPE_TABLE:

		owner_id = acpi_gbl_next_table_owner_id;
		acpi_gbl_next_table_owner_id++;

		if (acpi_gbl_next_table_owner_id == FIRST_METHOD_ID)
		{
			acpi_gbl_next_table_owner_id = FIRST_TABLE_ID;
		}
		break;


	case OWNER_TYPE_METHOD:

		owner_id = acpi_gbl_next_method_owner_id;
		acpi_gbl_next_method_owner_id++;

		if (acpi_gbl_next_method_owner_id == FIRST_TABLE_ID)
		{
			acpi_gbl_next_method_owner_id = FIRST_METHOD_ID;
		}
		break;
	}


	acpi_cm_release_mutex (ACPI_MTX_CACHES);

	return (owner_id);
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_cm_init_globals
 *
 * PARAMETERS:  none
 *
 * DESCRIPTION: Init library globals.  All globals that require specific
 *              initialization should be initialized here!
 *
 ***************************************************************************/

void
acpi_cm_init_globals (
	void)
{
	u32                     i;


	/* ACPI table structure */

	for (i = 0; i < NUM_ACPI_TABLES; i++)
	{
		acpi_gbl_acpi_tables[i].prev        = &acpi_gbl_acpi_tables[i];
		acpi_gbl_acpi_tables[i].next        = &acpi_gbl_acpi_tables[i];
		acpi_gbl_acpi_tables[i].pointer     = NULL;
		acpi_gbl_acpi_tables[i].length      = 0;
		acpi_gbl_acpi_tables[i].allocation  = ACPI_MEM_NOT_ALLOCATED;
		acpi_gbl_acpi_tables[i].count       = 0;
	}


	/* Address Space handler array */

	for (i = 0; i < ACPI_NUM_ADDRESS_SPACES; i++)
	{
		acpi_gbl_address_spaces[i].handler  = NULL;
		acpi_gbl_address_spaces[i].context  = NULL;
	}

	/* Mutex locked flags */

	for (i = 0; i < NUM_MTX; i++)
	{
		acpi_gbl_acpi_mutex_info[i].mutex   = NULL;
		acpi_gbl_acpi_mutex_info[i].locked  = FALSE;
		acpi_gbl_acpi_mutex_info[i].use_count = 0;
	}

	/* Global notify handlers */

	acpi_gbl_sys_notify.handler         = NULL;
	acpi_gbl_drv_notify.handler         = NULL;

	/* Global "typed" ACPI table pointers */

	acpi_gbl_RSDP                       = NULL;
	acpi_gbl_XSDT                       = NULL;
	acpi_gbl_FACS                       = NULL;
	acpi_gbl_FADT                       = NULL;
	acpi_gbl_DSDT                       = NULL;


	/* Global Lock support */

	acpi_gbl_global_lock_acquired       = FALSE;
	acpi_gbl_global_lock_thread_count   = 0;

	/* Miscellaneous variables */

	acpi_gbl_system_flags               = 0;
	acpi_gbl_startup_flags              = 0;
	acpi_gbl_rsdp_original_location     = 0;
	acpi_gbl_cm_single_step             = FALSE;
	acpi_gbl_db_terminate_threads       = FALSE;
	acpi_gbl_shutdown                   = FALSE;
	acpi_gbl_ns_lookup_count            = 0;
	acpi_gbl_ps_find_count              = 0;
	acpi_gbl_acpi_hardware_present      = TRUE;
	acpi_gbl_next_table_owner_id        = FIRST_TABLE_ID;
	acpi_gbl_next_method_owner_id       = FIRST_METHOD_ID;
	acpi_gbl_debugger_configuration     = DEBUGGER_THREADING;

	/* Cache of small "state" objects */

	acpi_gbl_generic_state_cache        = NULL;
	acpi_gbl_generic_state_cache_depth  = 0;
	acpi_gbl_state_cache_requests       = 0;
	acpi_gbl_state_cache_hits           = 0;

	acpi_gbl_parse_cache                = NULL;
	acpi_gbl_parse_cache_depth          = 0;
	acpi_gbl_parse_cache_requests       = 0;
	acpi_gbl_parse_cache_hits           = 0;

	acpi_gbl_ext_parse_cache            = NULL;
	acpi_gbl_ext_parse_cache_depth      = 0;
	acpi_gbl_ext_parse_cache_requests   = 0;
	acpi_gbl_ext_parse_cache_hits       = 0;

	acpi_gbl_object_cache               = NULL;
	acpi_gbl_object_cache_depth         = 0;
	acpi_gbl_object_cache_requests      = 0;
	acpi_gbl_object_cache_hits          = 0;

	acpi_gbl_walk_state_cache           = NULL;
	acpi_gbl_walk_state_cache_depth     = 0;
	acpi_gbl_walk_state_cache_requests  = 0;
	acpi_gbl_walk_state_cache_hits      = 0;

	/* Hardware oriented */

	acpi_gbl_gpe0enable_register_save   = NULL;
	acpi_gbl_gpe1_enable_register_save  = NULL;
	acpi_gbl_original_mode              = SYS_MODE_UNKNOWN;   /*  original ACPI/legacy mode   */
	acpi_gbl_gpe_registers              = NULL;
	acpi_gbl_gpe_info                   = NULL;

	/* Namespace */

	acpi_gbl_root_node                  = NULL;

	acpi_gbl_root_node_struct.name      = ACPI_ROOT_NAME;
	acpi_gbl_root_node_struct.data_type = ACPI_DESC_TYPE_NAMED;
	acpi_gbl_root_node_struct.type      = ACPI_TYPE_ANY;
	acpi_gbl_root_node_struct.child     = NULL;
	acpi_gbl_root_node_struct.peer      = NULL;
	acpi_gbl_root_node_struct.object    = NULL;
	acpi_gbl_root_node_struct.flags     = ANOBJ_END_OF_PEER_LIST;

	/* Memory allocation metrics - compiled out in non-debug mode. */

	INITIALIZE_ALLOCATION_METRICS();

	return;
}


