/******************************************************************************
 *
 * Name: acconfig.h - Global configuration constants
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef _ACCONFIG_H
#define _ACCONFIG_H


/******************************************************************************
 *
 * Configuration options
 *
 *****************************************************************************/

/*
 * ACPI_DEBUG_OUTPUT    - This switch enables all the debug facilities of the
 *                        ACPI subsystem.  This includes the DEBUG_PRINT output
 *                        statements.  When disabled, all DEBUG_PRINT
 *                        statements are compiled out.
 *
 * ACPI_APPLICATION     - Use this switch if the subsystem is going to be run
 *                        at the application level.
 *
 */

/*
 * OS name, used for the _OS object.  The _OS object is essentially obsolete,
 * but there is a large base of ASL/AML code in existing machines that check
 * for the string below.  The use of this string usually guarantees that
 * the ASL will execute down the most tested code path.  Also, there is some
 * code that will not execute the _OSI method unless _OS matches the string
 * below.  Therefore, change this string at your own risk.
 */
#define ACPI_OS_NAME                    "Microsoft Windows NT"

/* Maximum objects in the various object caches */

#define ACPI_MAX_STATE_CACHE_DEPTH      96          /* State objects */
#define ACPI_MAX_PARSE_CACHE_DEPTH      96          /* Parse tree objects */
#define ACPI_MAX_EXTPARSE_CACHE_DEPTH   96          /* Parse tree objects */
#define ACPI_MAX_OBJECT_CACHE_DEPTH     96          /* Interpreter operand objects */
#define ACPI_MAX_NAMESPACE_CACHE_DEPTH  96          /* Namespace objects */

/*
 * Should the subsystem abort the loading of an ACPI table if the
 * table checksum is incorrect?
 */
#define ACPI_CHECKSUM_ABORT             FALSE


/******************************************************************************
 *
 * Subsystem Constants
 *
 *****************************************************************************/

/* Version of ACPI supported */

#define ACPI_CA_SUPPORT_LEVEL           3

/* Maximum count for a semaphore object */

#define ACPI_MAX_SEMAPHORE_COUNT        256

/* Maximum object reference count (detects object deletion issues) */

#define ACPI_MAX_REFERENCE_COUNT        0x800

/* Default page size for use in mapping memory for operation regions */

#define ACPI_DEFAULT_PAGE_SIZE          4096    /* Must be power of 2 */

/* OwnerId tracking. 8 entries allows for 255 OwnerIds */

#define ACPI_NUM_OWNERID_MASKS          8

/* Size of the root table array is increased by this increment */

#define ACPI_ROOT_TABLE_SIZE_INCREMENT  4

/* Maximum number of While() loop iterations before forced abort */

#define ACPI_MAX_LOOP_ITERATIONS        0xFFFF


/******************************************************************************
 *
 * ACPI Specification constants (Do not change unless the specification changes)
 *
 *****************************************************************************/

/* Method info (in WALK_STATE), containing local variables and argumetns */

#define ACPI_METHOD_NUM_LOCALS          8
#define ACPI_METHOD_MAX_LOCAL           7

#define ACPI_METHOD_NUM_ARGS            7
#define ACPI_METHOD_MAX_ARG             6

/*
 * Operand Stack (in WALK_STATE), Must be large enough to contain METHOD_MAX_ARG
 */
#define ACPI_OBJ_NUM_OPERANDS           8
#define ACPI_OBJ_MAX_OPERAND            7

/* Number of elements in the Result Stack frame, can be an arbitrary value */

#define ACPI_RESULTS_FRAME_OBJ_NUM      8

/*
 * Maximal number of elements the Result Stack can contain,
 * it may be an arbitray value not exceeding the types of
 * ResultSize and ResultCount (now UINT8).
 */
#define ACPI_RESULTS_OBJ_NUM_MAX        255

/* Constants used in searching for the RSDP in low memory */

#define ACPI_EBDA_PTR_LOCATION          0x0000040E     /* Physical Address */
#define ACPI_EBDA_PTR_LENGTH            2
#define ACPI_EBDA_WINDOW_SIZE           1024
#define ACPI_HI_RSDP_WINDOW_BASE        0x000E0000     /* Physical Address */
#define ACPI_HI_RSDP_WINDOW_SIZE        0x00020000
#define ACPI_RSDP_SCAN_STEP             16

/* Operation regions */

#define ACPI_NUM_PREDEFINED_REGIONS     9
#define ACPI_USER_REGION_BEGIN          0x80

/* Maximum SpaceIds for Operation Regions */

#define ACPI_MAX_ADDRESS_SPACE          255

/* Array sizes.  Used for range checking also */

#define ACPI_MAX_MATCH_OPCODE           5

/* RSDP checksums */

#define ACPI_RSDP_CHECKSUM_LENGTH       20
#define ACPI_RSDP_XCHECKSUM_LENGTH      36

/* SMBus and IPMI bidirectional buffer size */

#define ACPI_SMBUS_BUFFER_SIZE          34
#define ACPI_IPMI_BUFFER_SIZE           66

/* _SxD and _SxW control methods */

#define ACPI_NUM_SxD_METHODS            4
#define ACPI_NUM_SxW_METHODS            5


/******************************************************************************
 *
 * ACPI AML Debugger
 *
 *****************************************************************************/

#define ACPI_DEBUGGER_MAX_ARGS          8  /* Must be max method args + 1 */

#define ACPI_DEBUGGER_COMMAND_PROMPT    '-'
#define ACPI_DEBUGGER_EXECUTE_PROMPT    '%'


#endif /* _ACCONFIG_H */

