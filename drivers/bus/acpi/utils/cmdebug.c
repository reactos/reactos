/******************************************************************************
 *
 * Module Name: cmdebug - Debug print routines
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

#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmdebug")


/*****************************************************************************
 *
 * FUNCTION:    Get/Set debug level
 *
 * DESCRIPTION: Get or set value of the debug flag
 *
 *              These are used to allow user's to get/set the debug level
 *
 ****************************************************************************/


u32
get_debug_level (void)
{

	return (acpi_dbg_level);
}

void
set_debug_level (
	u32                     new_debug_level)
{

	acpi_dbg_level = new_debug_level;
}


/*****************************************************************************
 *
 * FUNCTION:    Function_trace
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level
 *
 ****************************************************************************/

void
function_trace (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name)
{

	acpi_gbl_nesting_level++;

	debug_print (module_name, line_number, component_id,
			 TRACE_FUNCTIONS,
			 " %2.2ld Entered Function: %s\n",
			 acpi_gbl_nesting_level, function_name);
}


/*****************************************************************************
 *
 * FUNCTION:    Function_trace_ptr
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *              Pointer             - Pointer to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level
 *
 ****************************************************************************/

void
function_trace_ptr (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	void                    *pointer)
{

	acpi_gbl_nesting_level++;
	debug_print (module_name, line_number, component_id, TRACE_FUNCTIONS,
			 " %2.2ld Entered Function: %s, %p\n",
			 acpi_gbl_nesting_level, function_name, pointer);
}


/*****************************************************************************
 *
 * FUNCTION:    Function_trace_str
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *              String              - Additional string to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level
 *
 ****************************************************************************/

void
function_trace_str (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	NATIVE_CHAR             *string)
{

	acpi_gbl_nesting_level++;
	debug_print (module_name, line_number, component_id, TRACE_FUNCTIONS,
			 " %2.2ld Entered Function: %s, %s\n",
			 acpi_gbl_nesting_level, function_name, string);
}


/*****************************************************************************
 *
 * FUNCTION:    Function_trace_u32
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *              Integer             - Integer to display
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function entry trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level
 *
 ****************************************************************************/

void
function_trace_u32 (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	u32                     integer)
{

	acpi_gbl_nesting_level++;
	debug_print (module_name, line_number, component_id, TRACE_FUNCTIONS,
			 " %2.2ld Entered Function: %s, %lX\n",
			 acpi_gbl_nesting_level, function_name, integer);
}


/*****************************************************************************
 *
 * FUNCTION:    Function_exit
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level
 *
 ****************************************************************************/

void
function_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name)
{

	debug_print (module_name, line_number, component_id, TRACE_FUNCTIONS,
			 " %2.2ld Exiting Function: %s\n",
			 acpi_gbl_nesting_level, function_name);

	acpi_gbl_nesting_level--;
}


/*****************************************************************************
 *
 * FUNCTION:    Function_status_exit
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *              Status              - Exit status code
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level. Prints exit status also.
 *
 ****************************************************************************/

void
function_status_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	ACPI_STATUS             status)
{

	debug_print (module_name, line_number, component_id,
		TRACE_FUNCTIONS,
		" %2.2ld Exiting Function: %s, %s\n",
		acpi_gbl_nesting_level,
		function_name,
		acpi_cm_format_exception (status));

	acpi_gbl_nesting_level--;
}


/*****************************************************************************
 *
 * FUNCTION:    Function_value_exit
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *              Value               - Value to be printed with exit msg
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level. Prints exit value also.
 *
 ****************************************************************************/

void
function_value_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	ACPI_INTEGER            value)
{

	debug_print (module_name, line_number, component_id, TRACE_FUNCTIONS,
			 " %2.2ld Exiting Function: %s, %X\n",
			 acpi_gbl_nesting_level, function_name, value);

	acpi_gbl_nesting_level--;
}


/*****************************************************************************
 *
 * FUNCTION:    Function_ptr_exit
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Function_name       - Name of Caller's function
 *              Value               - Value to be printed with exit msg
 *
 * RETURN:      None
 *
 * DESCRIPTION: Function exit trace.  Prints only if TRACE_FUNCTIONS bit is
 *              set in Debug_level. Prints exit value also.
 *
 ****************************************************************************/

void
function_ptr_exit (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	NATIVE_CHAR             *function_name,
	u8                      *ptr)
{

	debug_print (module_name, line_number, component_id, TRACE_FUNCTIONS,
			 " %2.2ld Exiting Function: %s, %p\n",
			 acpi_gbl_nesting_level, function_name, ptr);

	acpi_gbl_nesting_level--;
}


/*****************************************************************************
 *
 * FUNCTION:    Debug_print
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Print_level         - Requested debug print level
 *              Format              - Printf format field
 *              ...                 - Optional printf arguments
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message with prefix consisting of the module name,
 *              line number, and component ID.
 *
 ****************************************************************************/

void
debug_print (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	u32                     print_level,
	NATIVE_CHAR             *format,
	...)
{
	va_list                 args;


	/* Both the level and the component must be enabled */

	if ((print_level & acpi_dbg_level) &&
		(component_id & acpi_dbg_layer)) {
		va_start (args, format);

		acpi_os_printf ("%8s-%04d: ", module_name, line_number);
		acpi_os_vprintf (format, args);
	}
}


/*****************************************************************************
 *
 * FUNCTION:    Debug_print_prefix
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print the prefix part of an error message, consisting of the
 *              module name, and line number
 *
 ****************************************************************************/

void
debug_print_prefix (
	NATIVE_CHAR             *module_name,
	u32                     line_number)
{


	acpi_os_printf ("%8s-%04d: ", module_name, line_number);
}


/*****************************************************************************
 *
 * FUNCTION:    Debug_print_raw
 *
 * PARAMETERS:  Format              - Printf format field
 *              ...                 - Optional printf arguments
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message -- without module/line indentifiers
 *
 ****************************************************************************/

void
debug_print_raw (
	NATIVE_CHAR             *format,
	...)
{
	va_list                 args;


	va_start (args, format);

	acpi_os_vprintf (format, args);

	va_end (args);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_cm_dump_buffer
 *
 * PARAMETERS:  Buffer              - Buffer to dump
 *              Count               - Amount to dump, in bytes
 *              Component_iD        - Caller's component ID
 *
 * RETURN:      None
 *
 * DESCRIPTION: Generic dump buffer in both hex and ascii.
 *
 ****************************************************************************/

void
acpi_cm_dump_buffer (
	u8                      *buffer,
	u32                     count,
	u32                     display,
	u32                     component_id)
{
	u32                     i = 0;
	u32                     j;
	u32                     temp32;
	u8                      buf_char;


	/* Only dump the buffer if tracing is enabled */

	if (!((TRACE_TABLES & acpi_dbg_level) &&
		(component_id & acpi_dbg_layer))) {
		return;
	}


	/*
	 * Nasty little dump buffer routine!
	 */
	while (i < count) {
		/* Print current offset */

		acpi_os_printf ("%05X  ", i);


		/* Print 16 hex chars */

		for (j = 0; j < 16;) {
			if (i + j >= count) {
				acpi_os_printf ("\n");
				return;
			}

			/* Make sure that the s8 doesn't get sign-extended! */

			switch (display) {
			/* Default is BYTE display */

			default:

				acpi_os_printf ("%02X ",
						*((u8 *) &buffer[i + j]));
				j += 1;
				break;


			case DB_WORD_DISPLAY:

				MOVE_UNALIGNED16_TO_32 (&temp32,
						 &buffer[i + j]);
				acpi_os_printf ("%04X ", temp32);
				j += 2;
				break;


			case DB_DWORD_DISPLAY:

				MOVE_UNALIGNED32_TO_32 (&temp32,
						 &buffer[i + j]);
				acpi_os_printf ("%08X ", temp32);
				j += 4;
				break;


			case DB_QWORD_DISPLAY:

				MOVE_UNALIGNED32_TO_32 (&temp32,
						 &buffer[i + j]);
				acpi_os_printf ("%08X", temp32);

				MOVE_UNALIGNED32_TO_32 (&temp32,
						 &buffer[i + j + 4]);
				acpi_os_printf ("%08X ", temp32);
				j += 8;
				break;
			}
		}


		/*
		 * Print the ASCII equivalent characters
		 * But watch out for the bad unprintable ones...
		 */

		for (j = 0; j < 16; j++) {
			if (i + j >= count) {
				acpi_os_printf ("\n");
				return;
			}

			buf_char = buffer[i + j];
			if ((buf_char > 0x1F && buf_char < 0x2E) ||
				(buf_char > 0x2F && buf_char < 0x61) ||
				(buf_char > 0x60 && buf_char < 0x7F)) {
				acpi_os_printf ("%c", buf_char);
			}
			else {
				acpi_os_printf (".");
			}
		}

		/* Done with that line. */

		acpi_os_printf ("\n");
		i += 16;
	}

	return;
}


