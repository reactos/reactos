/******************************************************************************
 *
 * Name: acconfig.h - Global configuration constants
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

#ifndef _ACCONFIG_H
#define _ACCONFIG_H


/******************************************************************************
 *
 * Compile-time options
 *
 *****************************************************************************/

/*
 * ACPI_DEBUG           - This switch enables all the debug facilities of the ACPI
 *                          subsystem.  This includes the DEBUG_PRINT output statements
 *                          When disabled, all DEBUG_PRINT statements are compiled out.
 *
 * ACPI_APPLICATION     - Use this switch if the subsystem is going to be run
 *                          at the application level.
 *
 */


/******************************************************************************
 *
 * Subsystem Constants
 *
 *****************************************************************************/


/* Version string */

#define ACPI_CA_VERSION             0x20010313


/* Maximum objects in the various object caches */

#define MAX_STATE_CACHE_DEPTH       64         /* State objects for stacks */
#define MAX_PARSE_CACHE_DEPTH       96          /* Parse tree objects */
#define MAX_EXTPARSE_CACHE_DEPTH    64          /* Parse tree objects */
#define MAX_OBJECT_CACHE_DEPTH      64          /* Interpreter operand objects */
#define MAX_WALK_CACHE_DEPTH        2           /* Objects for parse tree walks (method execution) */


/* String size constants */

#define MAX_STRING_LENGTH           512
#define PATHNAME_MAX                256     /* A full namespace pathname */


/* Maximum count for a semaphore object */

#define MAX_SEMAPHORE_COUNT         256


/* Max reference count (for debug only) */

#define MAX_REFERENCE_COUNT         0x200


/* Size of cached memory mapping for system memory operation region */

#define SYSMEM_REGION_WINDOW_SIZE   4096


/*
 * Debugger threading model
 * Use single threaded if the entire subsystem is contained in an application
 * Use multiple threaded when the subsystem is running in the kernel.
 *
 * By default the model is single threaded if ACPI_APPLICATION is set,
 * multi-threaded if ACPI_APPLICATION is not set.
 */

#define DEBUGGER_SINGLE_THREADED    0
#define DEBUGGER_MULTI_THREADED     1

#ifdef ACPI_APPLICATION
#define DEBUGGER_THREADING          DEBUGGER_SINGLE_THREADED

#else
#define DEBUGGER_THREADING          DEBUGGER_MULTI_THREADED
#endif


/******************************************************************************
 *
 * ACPI Specification constants (Do not change unless the specification changes)
 *
 *****************************************************************************/

/*
 * Method info (in WALK_STATE), containing local variables and argumetns
 */

#define MTH_NUM_LOCALS              8
#define MTH_MAX_LOCAL               7

#define MTH_NUM_ARGS                7
#define MTH_MAX_ARG                 6

/* Maximum length of resulting string when converting from a buffer */

#define ACPI_MAX_STRING_CONVERSION  200

/*
 * Operand Stack (in WALK_STATE), Must be large enough to contain MTH_MAX_ARG
 */

#define OBJ_NUM_OPERANDS            8
#define OBJ_MAX_OPERAND             7

/* Names within the namespace are 4 bytes long */

#define ACPI_NAME_SIZE              4
#define PATH_SEGMENT_LENGTH         5           /* 4 chars for name + 1 s8 for separator */
#define PATH_SEPARATOR              '.'


/* Constants used in searching for the RSDP in low memory */

#define LO_RSDP_WINDOW_BASE         0           /* Physical Address */
#define HI_RSDP_WINDOW_BASE         0xE0000     /* Physical Address */
#define LO_RSDP_WINDOW_SIZE         0x400
#define HI_RSDP_WINDOW_SIZE         0x20000
#define RSDP_SCAN_STEP              16


#endif /* _ACCONFIG_H */

