/******************************************************************************
 *
 * Module Name: cmxface - External interfaces for "global" ACPI functions
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
#include "acevents.h"
#include "achware.h"
#include "acnamesp.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acdebug.h"


#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmxface")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_initialize_subsystem
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initializes all global variables.  This is the first function
 *              called, so any early initialization belongs here.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_initialize_subsystem (
	void)
{
	ACPI_STATUS             status;


	/* Initialize all globals used by the subsystem */

	acpi_cm_init_globals ();

	/* Initialize the OS-Dependent layer */

	status = acpi_os_initialize ();
	if (ACPI_FAILURE (status)) {
		REPORT_ERROR (("OSD failed to initialize, %s\n",
			acpi_cm_format_exception (status)));
		return (status);
	}

	/* Create the default mutex objects */

	status = acpi_cm_mutex_initialize ();
	if (ACPI_FAILURE (status)) {
		REPORT_ERROR (("Global mutex creation failure, %s\n",
			acpi_cm_format_exception (status)));
		return (status);
	}

	/*
	 * Initialize the namespace manager and
	 * the root of the namespace tree
	 */

	status = acpi_ns_root_initialize ();
	if (ACPI_FAILURE (status)) {
		REPORT_ERROR (("Namespace initialization failure, %s\n",
			acpi_cm_format_exception (status)));
		return (status);
	}


	/* If configured, initialize the AML debugger */

	DEBUGGER_EXEC (acpi_db_initialize ());

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_enable_subsystem
 *
 * PARAMETERS:  Flags           - Init/enable Options
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Completes the subsystem initialization including hardware.
 *              Puts system into ACPI mode if it isn't already.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_enable_subsystem (
	u32                     flags)
{
	ACPI_STATUS             status = AE_OK;


	/* Sanity check the FADT for valid values */

	status = acpi_cm_validate_fadt ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/*
	 * Install the default Op_region handlers. These are
	 * installed unless other handlers have already been
	 * installed via the Install_address_space_handler interface
	 */

	if (!(flags & ACPI_NO_ADDRESS_SPACE_INIT)) {
		status = acpi_ev_install_default_address_space_handlers ();
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/*
	 * We must initialize the hardware before we can enable ACPI.
	 */

	if (!(flags & ACPI_NO_HARDWARE_INIT)) {
		status = acpi_hw_initialize ();
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/*
	 * Enable ACPI on this platform
	 */

	if (!(flags & ACPI_NO_ACPI_ENABLE)) {
		status = acpi_enable ();
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/*
	 * Note:
	 * We must have the hardware AND events initialized before we can execute
	 * ANY control methods SAFELY.  Any control method can require ACPI hardware
	 * support, so the hardware MUST be initialized before execution!
	 */

	if (!(flags & ACPI_NO_EVENT_INIT)) {
		status = acpi_ev_initialize ();
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}


	/*
	 * Initialize all device objects in the namespace
	 * This runs the _STA and _INI methods.
	 */

	if (!(flags & ACPI_NO_DEVICE_INIT)) {
		status = acpi_ns_initialize_devices ();
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}


	/*
	 * Initialize the objects that remain uninitialized.  This
	 * runs the executable AML that is part of the declaration of Op_regions
	 * and Fields.
	 */

	if (!(flags & ACPI_NO_OBJECT_INIT)) {
		status = acpi_ns_initialize_objects ();
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_terminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Shutdown the ACPI subsystem.  Release all resources.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_terminate (void)
{

	/* Terminate the AML Debuger if present */

	DEBUGGER_EXEC(acpi_gbl_db_terminate_threads = TRUE);

	/* TBD: [Investigate] This is no longer needed?*/
/*    Acpi_cm_release_mutex (ACPI_MTX_DEBUG_CMD_READY); */


	/* Shutdown and free all resources */

	acpi_cm_subsystem_shutdown ();


	/* Free the mutex objects */

	acpi_cm_mutex_terminate ();


	/* Now we can shutdown the OS-dependent layer */

	acpi_os_terminate ();

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_get_system_info
 *
 * PARAMETERS:  Out_buffer      - a pointer to a buffer to receive the
 *                                resources for the device
 *              Buffer_length   - the number of bytes available in the buffer
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get information about the current
 *              state of the ACPI subsystem.  It will return system information
 *              in the Out_buffer.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of Out_buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_system_info (
	ACPI_BUFFER             *out_buffer)
{
	ACPI_SYSTEM_INFO        *info_ptr;
	u32                     i;


	/*
	 *  Must have a valid buffer
	 */
	if ((!out_buffer)         ||
		(!out_buffer->pointer)) {
		return (AE_BAD_PARAMETER);
	}

	if (out_buffer->length < sizeof (ACPI_SYSTEM_INFO)) {
		/*
		 *  Caller's buffer is too small
		 */
		out_buffer->length = sizeof (ACPI_SYSTEM_INFO);

		return (AE_BUFFER_OVERFLOW);
	}


	/*
	 *  Set return length and get data
	 */
	out_buffer->length = sizeof (ACPI_SYSTEM_INFO);
	info_ptr = (ACPI_SYSTEM_INFO *) out_buffer->pointer;

	info_ptr->acpi_ca_version   = ACPI_CA_VERSION;

	/* System flags (ACPI capabilities) */

	info_ptr->flags             = acpi_gbl_system_flags;

	/* Timer resolution - 24 or 32 bits  */
	if (!acpi_gbl_FADT) {
		info_ptr->timer_resolution = 0;
	}
	else if (acpi_gbl_FADT->tmr_val_ext == 0) {
		info_ptr->timer_resolution = 24;
	}
	else {
		info_ptr->timer_resolution = 32;
	}

	/* Clear the reserved fields */

	info_ptr->reserved1         = 0;
	info_ptr->reserved2         = 0;

	/* Current debug levels */

	info_ptr->debug_layer       = acpi_dbg_layer;
	info_ptr->debug_level       = acpi_dbg_level;

	/* Current status of the ACPI tables, per table type */

	info_ptr->num_table_types = NUM_ACPI_TABLES;
	for (i = 0; i < NUM_ACPI_TABLES; i++) {
		info_ptr->table_info[i].count = acpi_gbl_acpi_tables[i].count;
	}

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_format_exception
 *
 * PARAMETERS:  Out_buffer      - a pointer to a buffer to receive the
 *                                exception name
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function translates an ACPI exception into an ASCII string.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_format_exception (
	ACPI_STATUS             exception,
	ACPI_BUFFER             *out_buffer)
{
	u32                     length;
	NATIVE_CHAR             *formatted_exception;


	/*
	 *  Must have a valid buffer
	 */
	if ((!out_buffer)         ||
		(!out_buffer->pointer)) {
		return (AE_BAD_PARAMETER);
	}


	/* Convert the exception code (Handles bad exception codes) */

	formatted_exception = acpi_cm_format_exception (exception);

	/*
	 * Get length of string and check if it will fit in caller's buffer
	 */

	length = STRLEN (formatted_exception);
	if (out_buffer->length < length) {
		out_buffer->length = length;
		return (AE_BUFFER_OVERFLOW);
	}


	/* Copy the string, all done */

	STRCPY (out_buffer->pointer, formatted_exception);

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_allocate
 *
 * PARAMETERS:  Size                - Size of the allocation
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: The subsystem's equivalent of malloc.
 *              External front-end to the Cm* memory manager
 *
 ****************************************************************************/

void *
acpi_allocate (
	u32                     size)
{

	return (acpi_cm_allocate (size));
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_callocate
 *
 * PARAMETERS:  Size                - Size of the allocation
 *
 * RETURN:      Address of the allocated memory on success, NULL on failure.
 *
 * DESCRIPTION: The subsystem's equivalent of calloc.
 *              External front-end to the Cm* memory manager
 *
 ****************************************************************************/

void *
acpi_callocate (
	u32                     size)
{

	return (acpi_cm_callocate (size));
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_free
 *
 * PARAMETERS:  Address             - Address of the memory to deallocate
 *
 * RETURN:      None
 *
 * DESCRIPTION: Frees the memory at Address
 *              External front-end to the Cm* memory manager
 *
 ****************************************************************************/

void
acpi_free (
	void                    *address)
{

	acpi_cm_free (address);
}
